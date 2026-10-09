//! Read-only game portrait diagnostic. Only new private project artifacts are written.
use base64::Engine;
use cd_core::browser::BrowserSession;
use std::{fs::OpenOptions, io::Write, path::PathBuf};
fn main() -> Result<(), Box<dyn std::error::Error>> {
    let game = PathBuf::from(std::env::args_os().nth(1).ok_or("Game root required")?);
    let project = std::env::current_dir()?;
    let session = BrowserSession::open(&project, Some(&game), "ger")?;
    let out = PathBuf::from(
        std::env::args_os()
            .nth(2)
            .ok_or("New private output directory required")?,
    );
    let mut mapped = Vec::new();
    let mut missing = Vec::new();
    for mount in session.mounts()? {
        let icon = session.mount_icon(mount.key)?;
        if let Some(url) = icon.data_url {
            if mount.key == 1002316 {
                let png = base64::engine::general_purpose::STANDARD.decode(
                    url.strip_prefix("data:image/png;base64,")
                        .ok_or("Not PNG")?,
                )?;
                OpenOptions::new()
                    .write(true)
                    .create_new(true)
                    .open(out.join("real-lion-portrait.png"))?
                    .write_all(&png)?;
            }
            mapped.push(serde_json::json!({"key":mount.key,"name":mount.name,"internal":mount.internal,"source":icon.source,"caption":icon.caption}));
        } else {
            missing.push(serde_json::json!({"key":mount.key,"name":mount.name,"internal":mount.internal,"reason":icon.reason}));
        }
    }
    OpenOptions::new()
        .write(true)
        .create_new(true)
        .open(out.join("decoded-mount-portraits.json"))?
        .write_all(&serde_json::to_vec_pretty(&mapped)?)?;
    OpenOptions::new()
        .write(true)
        .create_new(true)
        .open(out.join("missing-mount-portraits.json"))?
        .write_all(&serde_json::to_vec_pretty(&missing)?)?;
    println!(
        "{} mount portraits decoded (exact and explicitly labelled examples). No game/save writes.",
        mapped.len()
    );
    Ok(())
}
