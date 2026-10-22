//! Fixed-purpose companion to the desktop installer. No arbitrary paths/actions.
use std::{path::PathBuf, process::ExitCode};
fn main() -> ExitCode {
    let result = (|| -> Result<_, Box<dyn std::error::Error>> {
        let args: Vec<_> = std::env::args_os().skip(1).collect();
        if args.len() != 4 || args[0] != "--game" || args[2] != "--mode" {
            return Err("Expected --game <installation> --mode Status|Install|Remove".into());
        }
        let game = PathBuf::from(&args[1]);
        match args[3].to_str() {
            Some("Status") => Ok(cd_core::apply::region_mod::status(&game)),
            Some("Install") => Ok(cd_core::apply::region_mod::change(&game, true)?),
            Some("Remove") => Ok(cd_core::apply::region_mod::change(&game, false)?),
            _ => Err("Unknown archive-mod operation".into()),
        }
    })();
    match result {
        Ok(status) => {
            println!("{}", serde_json::json!({"ok":true,"status":status}));
            ExitCode::SUCCESS
        }
        Err(error) => {
            println!(
                "{}",
                serde_json::json!({"ok":false,"message":error.to_string()})
            );
            ExitCode::FAILURE
        }
    }
}
