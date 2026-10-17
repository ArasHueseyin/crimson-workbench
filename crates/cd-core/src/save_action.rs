//! Common receipt envelope for reviewed, backed-up save edits.
use serde::{Deserialize, Serialize};
use std::path::PathBuf;

#[derive(Clone, Debug, Deserialize, Serialize)]
#[serde(deny_unknown_fields)]
pub struct Receipt<Request, Plan> {
    pub request: Request,
    pub plan: Plan,
    pub backup: PathBuf,
    pub message: String,
}
