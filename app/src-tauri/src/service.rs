use cd_core::{
    DiscoveryReport, Language,
    browser::{BrowserInfo, BrowserPage, BrowserQuery, BrowserSession},
};
use serde::Serialize;
use std::{
    path::{Path, PathBuf},
    sync::{
        Arc, Mutex,
        atomic::{AtomicU64, Ordering},
    },
};

#[derive(Debug, Serialize)]
pub struct AppError {
    pub code: &'static str,
    pub message: String,
}
impl From<cd_core::Error> for AppError {
    fn from(error: cd_core::Error) -> Self {
        let code = match &error {
            cd_core::Error::UnsupportedBuild(_) => "unsupported_build",
            _ => "read_error",
        };
        Self {
            code,
            message: error.to_string(),
        }
    }
}
pub type Result<T> = std::result::Result<T, AppError>;
fn error(code: &'static str, message: impl Into<String>) -> AppError {
    AppError {
        code,
        message: message.into(),
    }
}
#[derive(Serialize)]
pub struct Bootstrap {
    pub project: PathBuf,
    pub discovery: DiscoveryReport,
    pub languages: Vec<Language>,
}
#[derive(Serialize)]
pub struct Catalog {
    pub session: u64,
    pub info: BrowserInfo,
    pub game_path: PathBuf,
}
pub struct Session {
    id: u64,
    browser: BrowserSession,
}
pub struct AppService {
    live: crate::live_service::LiveService,
    audits: crate::audit_service::AuditService,
    project: PathBuf,
    generation: AtomicU64,
    session: Mutex<Option<Session>>,
}
impl AppService {
    pub fn new(project: PathBuf) -> Self {
        Self {
            live: crate::live_service::LiveService::default(),
            audits: crate::audit_service::AuditService::default(),
            project,
            generation: AtomicU64::new(0),
            session: Mutex::new(None),
        }
    }
    pub fn bootstrap(&self) -> Result<Bootstrap> {
        Ok(Bootstrap {
            project: self.project.clone(),
            discovery: cd_core::discover_project(&self.project, None)?,
            languages: cd_core::supported_languages(),
        })
    }
    pub fn open(&self, game: Option<String>, language: String) -> Result<Catalog> {
        // Reserve generation and clear the old snapshot under the SAME lock.
        // An older suspended request must never clear a newer completed session.
        let id = {
            let mut session = self
                .session
                .lock()
                .map_err(|_| error("internal", "Sitzungssperre beschädigt"))?;
            let id = self.generation.fetch_add(1, Ordering::SeqCst) + 1;
            self.audits.cancel_current();
            self.live.cancel_current();
            *session = None;
            id
        };
        let path = game
            .as_deref()
            .filter(|v| !v.trim().is_empty())
            .map(Path::new);
        let report = cd_core::discover_project(&self.project, path)?;
        let game_path = cd_core::select_game(&report)?;
        let browser = BrowserSession::open(&self.project, Some(&game_path), &language)?;
        let info = browser.info();
        let mut session = self
            .session
            .lock()
            .map_err(|_| error("internal", "Sitzungssperre beschädigt"))?;
        if id != self.generation.load(Ordering::SeqCst) {
            return Err(error(
                "stale_session",
                "Eine neuere Installation wird bereits geladen.",
            ));
        }
        *session = Some(Session { id, browser });
        Ok(Catalog {
            session: id,
            info,
            game_path,
        })
    }
    pub fn with_session<T>(
        &self,
        id: u64,
        f: impl FnOnce(&BrowserSession) -> cd_core::Result<T>,
    ) -> Result<T> {
        let guard = self
            .session
            .lock()
            .map_err(|_| error("internal", "Sitzungssperre beschädigt"))?;
        let session = guard
            .as_ref()
            .filter(|s| s.id == id && self.generation.load(Ordering::SeqCst) == id)
            .ok_or_else(|| {
                error(
                    "stale_session",
                    "Daten wurden neu geladen. Bitte erneut auswählen.",
                )
            })?;
        Ok(f(&session.browser)?)
    }
    pub fn search(&self, id: u64, query: BrowserQuery) -> Result<BrowserPage> {
        self.with_session(id, |s| s.search(query))
    }
    pub fn set_extra_socket(
        &self,
        id: u64,
        request: &cd_core::extra_sockets::Request,
    ) -> Result<cd_core::extra_sockets::Receipt> {
        self.with_session(id, |s| s.set_extra_socket(&self.project, request))
    }
    pub fn add_extra_sockets(
        &self,
        id: u64,
        request: &cd_core::extra_sockets_candidates::AddRequest,
    ) -> Result<cd_core::extra_sockets::Receipt> {
        self.with_session(id, |s| s.add_extra_sockets(&self.project, request))
    }
    pub fn mount_catalog(&self, id: u64, save: Option<&str>) -> Result<cd_core::mounts::Snapshot> {
        self.with_session(id, |s| {
            cd_core::mounts::snapshot(s.mounts()?, &cd_core::mounts::save_root()?, save)
        })
    }
    pub fn mount_register(
        &self,
        id: u64,
        request: &cd_core::mounts::Request,
    ) -> Result<cd_core::mounts::Receipt> {
        self.with_session(id, |s| {
            cd_core::mounts::register(
                s.game_root(),
                &self.project,
                &s.mounts()?,
                &cd_core::mounts::save_root()?,
                request,
            )
        })
    }
    pub fn baselines(&self, session: u64) -> Result<cd_core::installation::baseline::Catalog> {
        self.with_session(session, |s| {
            cd_core::installation::baseline::catalog(&cd_core::output_policy(
                &self.project,
                Some(s.game_root()),
            )?)
        })
    }
    pub fn baseline_preview(
        &self,
        session: u64,
        report_name: &str,
    ) -> Result<cd_core::installation::baseline::Preview> {
        self.with_session(session, |s| {
            cd_core::installation::baseline::preview(
                &cd_core::output_policy(&self.project, Some(s.game_root()))?,
                s.game_root(),
                report_name,
            )
        })
    }
    pub fn baseline_capture(
        &self,
        session: u64,
        report_name: &str,
        review_id: &str,
    ) -> Result<cd_core::installation::baseline::Saved> {
        self.with_session(session, |s| {
            cd_core::installation::baseline::capture(
                &cd_core::output_policy(&self.project, Some(s.game_root()))?,
                s.game_root(),
                report_name,
                review_id,
            )
        })
    }
    pub fn baseline_inspect(
        &self,
        session: u64,
        id: &str,
    ) -> Result<cd_core::installation::baseline::Saved> {
        self.with_session(session, |s| {
            cd_core::installation::baseline::inspect(
                &cd_core::output_policy(&self.project, Some(s.game_root()))?,
                s.game_root(),
                id,
            )
        })
    }
    pub fn rehearsals(&self, session: u64) -> Result<cd_core::apply::recovery::Listing> {
        self.with_session(session, |s| {
            cd_core::apply::recovery::list(&cd_core::output_policy(
                &self.project,
                Some(s.game_root()),
            )?)
        })
    }
    pub fn recovery_review(
        &self,
        session: u64,
        directory: &Path,
    ) -> Result<cd_core::apply::recovery::Review> {
        self.with_session(session, |s| {
            cd_core::apply::recovery::inspect(
                &cd_core::output_policy(&self.project, Some(s.game_root()))?,
                directory,
            )
        })
    }
    pub fn recovery_restore(
        &self,
        session: u64,
        directory: &Path,
        review_id: &str,
    ) -> Result<cd_core::apply::RehearsalRecovery> {
        self.with_session(session, |s| {
            cd_core::apply::recovery::restore(
                &cd_core::output_policy(&self.project, Some(s.game_root()))?,
                directory,
                review_id,
            )
        })
    }
    pub fn audit_start(&self, session: u64) -> Result<crate::audit_service::Snapshot> {
        // Keep the short catalog lock through reservation, never through hashing.
        let guard = self
            .session
            .lock()
            .map_err(|_| error("internal", "Sitzungssperre beschädigt"))?;
        let s = guard
            .as_ref()
            .filter(|s| s.id == session && self.generation.load(Ordering::SeqCst) == session)
            .ok_or_else(|| error("stale_session", "Daten wurden neu geladen."))?;
        if self.live.active()? {
            return Err(error(
                "live_busy",
                "Live-Vorgang läuft; Inhaltsprüfung danach starten.",
            ));
        }
        self.audits.start(
            session,
            s.browser.game_root().to_path_buf(),
            cd_core::output_policy(&self.project, Some(s.browser.game_root()))?,
        )
    }
    pub fn audit_status(&self, session: u64) -> Result<Option<crate::audit_service::Snapshot>> {
        self.with_session(session, |_| Ok(()))?;
        self.audits.status(session)
    }
    pub fn foreign_preview(
        &self,
        session: u64,
    ) -> Result<cd_core::apply::live::foreign::Inspection> {
        self.with_session(session, |s| {
            cd_core::apply::live::foreign::inspect(
                &cd_core::output_policy(&self.project, Some(s.game_root()))?,
                s.game_root(),
            )
        })
    }
    pub fn foreign_confirm(
        &self,
        session: u64,
        review_id: &str,
        preserve: bool,
    ) -> Result<cd_core::apply::live::foreign::Receipt> {
        self.with_session(session, |s| {
            if self
                .live
                .active()
                .map_err(|e| cd_core::Error::Invalid(e.message))?
            {
                return Err(cd_core::Error::Invalid(
                    "Live-Vorgang läuft; Bestätigung danach ändern".into(),
                ));
            }
            cd_core::apply::live::foreign::confirm(
                &cd_core::output_policy(&self.project, Some(s.game_root()))?,
                s.game_root(),
                review_id,
                preserve,
            )
        })
    }
    pub fn foreign_revoke(
        &self,
        session: u64,
        approval_id: &str,
    ) -> Result<cd_core::apply::live::foreign::Receipt> {
        self.with_session(session, |s| {
            if self
                .live
                .active()
                .map_err(|e| cd_core::Error::Invalid(e.message))?
            {
                return Err(cd_core::Error::Invalid(
                    "Live-Vorgang läuft; Bestätigung danach ändern".into(),
                ));
            }
            cd_core::apply::live::foreign::revoke(
                &cd_core::output_policy(&self.project, Some(s.game_root()))?,
                s.game_root(),
                approval_id,
            )
        })
    }
    pub fn live_status(&self, session: u64) -> Result<cd_core::apply::live::Status> {
        self.with_session(session, |s| {
            cd_core::apply::live::status(
                &cd_core::output_policy(&self.project, Some(s.game_root()))?,
                s.game_root(),
            )
        })
    }
    pub fn live_setup_preview(
        &self,
        session: u64,
        id: &str,
    ) -> Result<cd_core::apply::live::SetupPreview> {
        self.with_session(session, |s| {
            cd_core::apply::live::setup_preview(
                &cd_core::output_policy(&self.project, Some(s.game_root()))?,
                s.game_root(),
                id,
            )
        })
    }
    pub fn live_update_preview(
        &self,
        session: u64,
        id: &str,
    ) -> Result<cd_core::apply::live::updates::Review> {
        self.with_session(session, |s| {
            cd_core::apply::live::updates::preview(
                &cd_core::output_policy(&self.project, Some(s.game_root()))?,
                s.game_root(),
                id,
            )
        })
    }
    pub fn live_preview(
        &self,
        session: u64,
        request: &cd_core::apply::live::Request,
    ) -> Result<cd_core::apply::live::Review> {
        self.with_session(session, |s| {
            cd_core::apply::live::preview(
                &cd_core::output_policy(&self.project, Some(s.game_root()))?,
                s.game_root(),
                request,
            )
        })
    }
    pub fn live_start(
        &self,
        session: u64,
        operation: crate::live_service::Operation,
    ) -> Result<crate::live_service::Snapshot> {
        let guard = self
            .session
            .lock()
            .map_err(|_| error("internal", "Sitzungssperre beschädigt"))?;
        let s = guard
            .as_ref()
            .filter(|s| s.id == session && self.generation.load(Ordering::SeqCst) == session)
            .ok_or_else(|| error("stale_session", "Daten wurden neu geladen."))?;
        if self
            .audits
            .status(session)?
            .is_some_and(|s| matches!(s.phase, "preparing" | "hashing" | "validating"))
        {
            return Err(error(
                "audit_busy",
                "Inhaltsprüfung läuft; auf Abschluss warten.",
            ));
        }
        self.live.start(
            session,
            cd_core::output_policy(&self.project, Some(s.browser.game_root()))?,
            s.browser.game_root().to_owned(),
            operation,
        )
    }
    pub fn live_job_status(&self, session: u64) -> Result<Option<crate::live_service::Snapshot>> {
        self.with_session(session, |_| Ok(()))?;
        self.live.status(session)
    }
    pub fn live_cancel(&self, session: u64, id: u64) -> Result<()> {
        self.with_session(session, |_| Ok(()))?;
        self.live.cancel(session, id)
    }
    pub fn audit_cancel(&self, session: u64, id: u64) -> Result<()> {
        self.with_session(session, |_| Ok(()))?;
        self.audits.cancel(session, id)
    }
    pub fn audit_export(&self, session: u64, id: u64) -> Result<String> {
        let guard = self
            .session
            .lock()
            .map_err(|_| error("internal", "Sitzungssperre beschädigt"))?;
        if !guard
            .as_ref()
            .is_some_and(|s| s.id == session && self.generation.load(Ordering::SeqCst) == session)
        {
            return Err(error("stale_session", "Daten wurden neu geladen."));
        }
        let report = self.audits.report(session, id)?;
        let policy = cd_core::output_policy(&self.project, Some(&report.game_path))?;
        static NEXT: AtomicU64 = AtomicU64::new(0);
        let file = PathBuf::from(format!(
            "exports/installation-audit-{}-{}-{}-{}.json",
            report.started_at,
            std::process::id(),
            id,
            NEXT.fetch_add(1, Ordering::SeqCst)
        ));
        Ok(cd_core::write_generated(
            &policy,
            &file,
            &serde_json::to_vec_pretty(&report).map_err(cd_core::Error::from)?,
        )?
        .to_string_lossy()
        .into_owned())
    }
}
pub fn project_root() -> PathBuf {
    let args: Vec<_> = std::env::args_os().collect();
    if let Some(index) = args.iter().position(|a| a == "--project")
        && let Some(path) = args.get(index + 1)
    {
        return PathBuf::from(path);
    }
    if let Some(path) = std::env::var_os("CD_WORKBENCH_PROJECT") {
        return path.into();
    }
    for base in [
        std::env::current_exe()
            .ok()
            .and_then(|p| p.parent().map(Path::to_path_buf)),
        std::env::current_dir().ok(),
    ]
    .into_iter()
    .flatten()
    {
        for path in base.ancestors() {
            if path.join("SPEC.md").is_file() && path.join("Cargo.toml").is_file() {
                return path.to_path_buf();
            }
        }
    }
    // Moving the executable outside the project requires --project. No guessing
    // a writable game folder or changing working directories globally.
    std::env::current_dir().unwrap_or_else(|_| PathBuf::from("."))
}
pub type SharedService = Arc<AppService>;

#[cfg(test)]
mod tests {
    use super::*;
    #[test]
    fn unopened_sessions_never_read_or_export() {
        let temp = tempfile::tempdir().unwrap();
        let service = AppService::new(temp.path().to_path_buf());
        assert_eq!(
            service.search(1, BrowserQuery::default()).unwrap_err().code,
            "stale_session"
        );
        assert_eq!(
            service
                .with_session(1, |s| s.export_item(2200))
                .unwrap_err()
                .code,
            "stale_session"
        );
        assert_eq!(std::fs::read_dir(temp.path()).unwrap().count(), 0);
        assert!(
            service
                .with_session(1, |s| s.with_mods(|c| c.advanced_item(2200)))
                .is_err()
        );
        assert!(
            service
                .with_session(1, |s| s.with_mods(|c| c.advanced_skill(30001)))
                .is_err()
        );
        assert!(service.foreign_preview(1).is_err());
        assert!(service.foreign_confirm(1, "invalid", true).is_err());
        assert!(service.foreign_revoke(1, "invalid").is_err());
        assert!(service.live_status(1).is_err());
        assert!(service.live_setup_preview(1, "invalid").is_err());
        assert!(service.live_update_preview(1, "invalid").is_err());
        assert!(
            service
                .live_preview(1, &cd_core::apply::live::Request::Restore)
                .is_err()
        );
        assert!(service.live_job_status(1).is_err());
        assert!(service.live_cancel(1, 1).is_err());
        assert!(
            service
                .live_start(
                    1,
                    crate::live_service::Operation::Setup {
                        baseline_id: "invalid".into(),
                        review_id: "invalid".into(),
                        steam_verified_before_audit: true
                    }
                )
                .is_err()
        );
        assert_eq!(std::fs::read_dir(temp.path()).unwrap().count(), 0);
        assert!(service.audit_start(1).is_err());
        assert!(service.audit_status(1).is_err());
        assert!(service.audit_cancel(1, 1).is_err());
        assert!(service.audit_export(1, 1).is_err());
        assert!(service.baselines(1).is_err());
        assert!(service.baseline_preview(1, "invalid").is_err());
        assert!(service.baseline_capture(1, "invalid", "invalid").is_err());
        assert!(service.baseline_inspect(1, "invalid").is_err());
        assert!(service.rehearsals(1).is_err());
        assert!(service.recovery_review(1, temp.path()).is_err());
        assert!(service.recovery_restore(1, temp.path(), "invalid").is_err());
    }
}
