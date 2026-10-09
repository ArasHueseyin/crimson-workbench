//! One cancellable content-audit worker, independent of the catalog mutex.
use crate::service::{AppError, Result};
use cd_core::installation::audit::{self, Progress, Report};
use serde::Serialize;
use std::{
    path::PathBuf,
    sync::{
        Arc, Mutex,
        atomic::{AtomicBool, AtomicU64, Ordering},
    },
};

#[derive(Clone, Serialize)]
pub struct Snapshot {
    pub id: u64,
    pub session: u64,
    pub phase: &'static str,
    pub progress: Progress,
    pub error: Option<String>,
    pub report: Option<Report>,
}
impl Snapshot {
    fn active(&self) -> bool {
        matches!(self.phase, "preparing" | "hashing" | "validating")
    }
}
struct Job {
    cancel: Arc<AtomicBool>,
    snapshot: Arc<Mutex<Snapshot>>,
}
#[derive(Default)]
pub struct AuditService {
    next: AtomicU64,
    job: Mutex<Option<Job>>,
}
fn error(message: impl Into<String>) -> AppError {
    AppError {
        code: "audit_error",
        message: message.into(),
    }
}
impl AuditService {
    pub fn start(
        &self,
        session: u64,
        game: PathBuf,
        policy: cd_core::paths::PathPolicy,
    ) -> Result<Snapshot> {
        self.spawn(session, move |cancel, progress| {
            audit::verify_managed(&policy, &game, cancel, progress)
        })
    }
    fn spawn(
        &self,
        session: u64,
        work: impl FnOnce(&AtomicBool, Box<dyn FnMut(Progress) + Send>) -> cd_core::Result<Report>
        + Send
        + 'static,
    ) -> Result<Snapshot> {
        let mut jobs = self
            .job
            .lock()
            .map_err(|_| error("Prüfauftragssperre beschädigt"))?;
        if let Some(job) = jobs.as_ref()
            && job
                .snapshot
                .lock()
                .map_err(|_| error("Prüfstatus beschädigt"))?
                .active()
        {
            return Err(error(
                "Eine Inhaltsprüfung läuft bereits; gegebenenfalls abbrechen und auf ihr Ende warten.",
            ));
        }
        let initial = Snapshot {
            id: self.next.fetch_add(1, Ordering::SeqCst) + 1,
            session,
            phase: "preparing",
            progress: Progress::default(),
            error: None,
            report: None,
        };
        let snapshot = Arc::new(Mutex::new(initial.clone()));
        let cancel = Arc::new(AtomicBool::new(false));
        let state = snapshot.clone();
        let stop = cancel.clone();
        std::thread::Builder::new().name("content-audit".into()).spawn(move ||{
            let updates=state.clone();
            let progress:Box<dyn FnMut(Progress)+Send>=Box::new(move |p|{if let Ok(mut s)=updates.lock(){
                // Only publishing a successful Report can end the job. The last
                // progress callback alone must not admit an overlapping worker.
                if p.phase!="complete"{s.phase=p.phase;}s.progress=p;
            }});
            let result=std::panic::catch_unwind(std::panic::AssertUnwindSafe(||work(&stop,progress)));
            if let Ok(mut s)=state.lock(){
                if stop.load(Ordering::Acquire){s.phase="cancelled";s.report=None;s.error=Some("Inhaltsprüfung abgebrochen; kein vollständiger Bericht erstellt.".into());}
                else {match result{
                    Ok(Ok(report))=>{s.phase="complete";s.report=Some(report);},
                    Ok(Err(e))=>{s.phase="failed";s.error=Some(e.to_string());},
                    Err(_)=>{s.phase="failed";s.error=Some("Prüfprozess fehlgeschlagen; kein vollständiger Bericht erstellt.".into());},
                }}
            }
        }).map_err(|e|error(e.to_string()))?;
        *jobs = Some(Job { cancel, snapshot });
        Ok(initial)
    }
    pub fn status(&self, session: u64) -> Result<Option<Snapshot>> {
        let jobs = self
            .job
            .lock()
            .map_err(|_| error("Prüfauftragssperre beschädigt"))?;
        jobs.as_ref()
            .map(|j| {
                j.snapshot
                    .lock()
                    .map(|s| s.clone())
                    .map_err(|_| error("Prüfstatus beschädigt"))
            })
            .transpose()
            .map(|s| s.filter(|s| s.session == session))
    }
    pub fn cancel(&self, session: u64, id: u64) -> Result<()> {
        let jobs = self
            .job
            .lock()
            .map_err(|_| error("Prüfauftragssperre beschädigt"))?;
        let job = jobs.as_ref().ok_or_else(|| error("Kein Prüfauftrag"))?;
        let s = job
            .snapshot
            .lock()
            .map_err(|_| error("Prüfstatus beschädigt"))?;
        if s.session != session || s.id != id {
            return Err(error("Prüfauftrag ist veraltet"));
        }
        if s.active() {
            job.cancel.store(true, Ordering::Release);
        }
        Ok(())
    }
    pub fn cancel_current(&self) {
        if let Ok(jobs) = self.job.lock()
            && let Some(job) = jobs.as_ref()
        {
            job.cancel.store(true, Ordering::Release);
        }
    }
    pub fn report(&self, session: u64, id: u64) -> Result<Report> {
        self.status(session)?
            .filter(|s| s.id == id && s.phase == "complete")
            .and_then(|s| s.report)
            .ok_or_else(|| error("Kein vollständiger Bericht für diesen Auftrag"))
    }
}
impl Drop for AuditService {
    fn drop(&mut self) {
        self.cancel_current();
    }
}

#[cfg(test)]
mod tests {
    use super::*;
    fn finish(service: &AuditService, session: u64) -> Snapshot {
        let start = std::time::Instant::now();
        loop {
            let s = service.status(session).unwrap().unwrap();
            if !s.active() {
                return s;
            }
            assert!(start.elapsed().as_secs() < 5);
            std::thread::sleep(std::time::Duration::from_millis(5));
        }
    }
    #[test]
    fn single_job_cancellation_and_session_isolation() {
        let service = AuditService::default();
        let entered = Arc::new(AtomicBool::new(false));
        let signal = entered.clone();
        let job = service
            .spawn(1, move |cancel, mut progress| {
                signal.store(true, Ordering::Release);
                progress(Progress {
                    phase: "hashing",
                    ..Progress::default()
                });
                while !cancel.load(Ordering::Acquire) {
                    std::thread::sleep(std::time::Duration::from_millis(1));
                }
                Err(cd_core::Error::Invalid("cancelled".into()))
            })
            .unwrap();
        assert!(service.spawn(1, |_, _| unreachable!()).is_err());
        assert!(service.status(2).unwrap().is_none());
        assert!(service.cancel(2, job.id).is_err());
        assert!(service.report(1, job.id).is_err());
        service.cancel(1, job.id).unwrap();
        let end = finish(&service, 1);
        assert_eq!(end.phase, "cancelled");
        assert!(end.report.is_none());
        assert!(entered.load(Ordering::Acquire));
        service
            .spawn(2, |_, _| {
                Err(cd_core::Error::Invalid("game running".into()))
            })
            .unwrap();
        assert_eq!(finish(&service, 2).phase, "failed");
    }
    #[test]
    fn panic_and_unpublished_complete_progress_do_not_claim_success() {
        let service = AuditService::default();
        service
            .spawn(7, |_, mut progress| {
                progress(Progress {
                    phase: "complete",
                    ..Progress::default()
                });
                panic!("own worker panic fixture")
            })
            .unwrap();
        let s = finish(&service, 7);
        assert_eq!(s.phase, "failed");
        assert!(s.report.is_none());
        assert!(service.report(7, s.id).is_err());
    }
}
