//! One cancellable live worker. A successful commit remains successful even if
//! cancellation arrives after the final check; never hide a completed mutation.
use crate::service::{AppError, Result};
use cd_core::{apply::live, paths::PathPolicy};
use serde::{Deserialize, Serialize};
use std::{
    path::PathBuf,
    sync::{
        Arc, Mutex,
        atomic::{AtomicBool, AtomicU64, Ordering},
    },
};
#[derive(Deserialize)]
#[serde(tag = "kind", rename_all = "snake_case", deny_unknown_fields)]
pub enum Operation {
    Setup {
        baseline_id: String,
        review_id: String,
        steam_verified_before_audit: bool,
    },
    Update {
        baseline_id: String,
        review_id: String,
        steam_verified_before_audit: bool,
    },
    Execute {
        request: live::Request,
        review_id: String,
    },
}
#[derive(Clone, Serialize)]
pub struct Snapshot {
    pub id: u64,
    pub session: u64,
    pub phase: &'static str,
    pub result: Option<serde_json::Value>,
    pub error: Option<String>,
}
struct Job {
    cancel: Arc<AtomicBool>,
    snapshot: Arc<Mutex<Snapshot>>,
}
#[derive(Default)]
pub struct LiveService {
    next: AtomicU64,
    job: Mutex<Option<Job>>,
}
fn error(s: impl Into<String>) -> AppError {
    AppError {
        code: "live_error",
        message: s.into(),
    }
}
impl LiveService {
    pub fn start(
        &self,
        session: u64,
        policy: PathPolicy,
        game: PathBuf,
        operation: Operation,
    ) -> Result<Snapshot> {
        self.spawn(session, move |cancel| match operation {
            Operation::Setup {
                baseline_id,
                review_id,
                steam_verified_before_audit,
            } => Ok(serde_json::to_value(live::setup(
                &policy,
                &game,
                &baseline_id,
                &review_id,
                steam_verified_before_audit,
                cancel,
            )?)?),
            Operation::Update {
                baseline_id,
                review_id,
                steam_verified_before_audit,
            } => Ok(serde_json::to_value(live::updates::execute(
                &policy,
                &game,
                &baseline_id,
                &review_id,
                steam_verified_before_audit,
                cancel,
            )?)?),
            Operation::Execute { request, review_id } => Ok(serde_json::to_value(live::execute(
                &policy, &game, &request, &review_id, cancel,
            )?)?),
        })
    }
    fn spawn(
        &self,
        session: u64,
        work: impl FnOnce(&AtomicBool) -> cd_core::Result<serde_json::Value> + Send + 'static,
    ) -> Result<Snapshot> {
        let mut jobs = self
            .job
            .lock()
            .map_err(|_| error("Live-Auftragssperre beschädigt"))?;
        if let Some(job) = jobs.as_ref()
            && job
                .snapshot
                .lock()
                .map_err(|_| error("Live-Status beschädigt"))?
                .phase
                == "running"
        {
            return Err(error(
                "Ein Live-Vorgang läuft bereits. Auf Abschluss oder Abbruch warten.",
            ));
        }
        let initial = Snapshot {
            id: self.next.fetch_add(1, Ordering::SeqCst) + 1,
            session,
            phase: "running",
            result: None,
            error: None,
        };
        let snapshot = Arc::new(Mutex::new(initial.clone()));
        let state = snapshot.clone();
        let cancel = Arc::new(AtomicBool::new(false));
        let stop = cancel.clone();
        std::thread::Builder::new().name("live-apply".into()).spawn(move||{
            let result=std::panic::catch_unwind(std::panic::AssertUnwindSafe(||work(&stop)));
            if let Ok(mut s)=state.lock(){match result {
                Ok(Ok(value))=>{s.phase="complete";s.result=Some(value);},
                Ok(Err(e))=>{s.phase=if stop.load(Ordering::Acquire){"cancelled"}else{"failed"};s.error=Some(e.to_string());},
                Err(_)=>{s.phase="failed";s.error=Some("Live-Vorgang unerwartet beendet. Zustand und Wiederherstellung prüfen.".into());},
            }}
        }).map_err(|e|error(e.to_string()))?;
        *jobs = Some(Job { cancel, snapshot });
        Ok(initial)
    }
    pub fn status(&self, session: u64) -> Result<Option<Snapshot>> {
        let jobs = self
            .job
            .lock()
            .map_err(|_| error("Live-Auftragssperre beschädigt"))?;
        jobs.as_ref()
            .map(|j| {
                j.snapshot
                    .lock()
                    .map(|s| s.clone())
                    .map_err(|_| error("Live-Status beschädigt"))
            })
            .transpose()
            .map(|s| s.filter(|s| s.session == session))
    }
    pub fn active(&self) -> Result<bool> {
        let jobs = self
            .job
            .lock()
            .map_err(|_| error("Live-Auftragssperre beschädigt"))?;
        Ok(jobs
            .as_ref()
            .map(|j| {
                j.snapshot
                    .lock()
                    .map(|s| s.phase == "running")
                    .map_err(|_| error("Live-Status beschädigt"))
            })
            .transpose()?
            .unwrap_or(false))
    }
    pub fn cancel(&self, session: u64, id: u64) -> Result<()> {
        let jobs = self
            .job
            .lock()
            .map_err(|_| error("Live-Auftragssperre beschädigt"))?;
        let j = jobs.as_ref().ok_or_else(|| error("Kein Live-Vorgang"))?;
        let s = j
            .snapshot
            .lock()
            .map_err(|_| error("Live-Status beschädigt"))?;
        if s.session != session || s.id != id {
            return Err(error("Live-Vorgang ist veraltet"));
        }
        if s.phase == "running" {
            j.cancel.store(true, Ordering::Release);
        }
        Ok(())
    }
    pub fn cancel_current(&self) {
        if let Ok(jobs) = self.job.lock()
            && let Some(j) = jobs.as_ref()
        {
            j.cancel.store(true, Ordering::Release);
        }
    }
}
impl Drop for LiveService {
    fn drop(&mut self) {
        self.cancel_current();
    }
}
#[cfg(test)]
mod tests {
    use super::*;
    fn finish(s: &LiveService) -> Snapshot {
        let start = std::time::Instant::now();
        loop {
            let v = s.status(1).unwrap().unwrap();
            if v.phase != "running" {
                return v;
            }
            assert!(start.elapsed().as_secs() < 5);
            std::thread::sleep(std::time::Duration::from_millis(5));
        }
    }
    #[test]
    fn single_job_cancel_and_late_success_have_truthful_outcomes() {
        let s = LiveService::default();
        let a = s
            .spawn(1, |stop| {
                while !stop.load(Ordering::Acquire) {
                    std::thread::yield_now();
                }
                Err(cd_core::Error::Invalid("cancelled".into()))
            })
            .unwrap();
        assert!(s.spawn(2, |_| unreachable!()).is_err());
        assert!(s.status(2).unwrap().is_none());
        assert!(s.cancel(2, a.id).is_err());
        s.cancel(1, a.id).unwrap();
        assert_eq!(finish(&s).phase, "cancelled");
        let a = s
            .spawn(1, |stop| {
                while !stop.load(Ordering::Acquire) {
                    std::thread::yield_now();
                }
                Ok(serde_json::json!({"committed":true}))
            })
            .unwrap();
        s.cancel(1, a.id).unwrap();
        let r = finish(&s);
        assert_eq!(r.phase, "complete");
        assert_eq!(r.result.unwrap()["committed"], true);
    }
}
