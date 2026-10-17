#![cfg(windows)]

use crate::service::{AppError, Result};
use std::{
    fs::OpenOptions,
    io::{Read, Write},
    os::windows::io::AsRawHandle,
    time::{Duration, Instant},
};
use windows_sys::Win32::System::Pipes::{GetNamedPipeServerProcessId, SetNamedPipeHandleState};

// Pipe names are selected entirely by trusted Rust code. Neither a frontend
// path nor a frontend-selected pipe prefix reaches this transport.
#[derive(Clone, Copy)]
pub enum PipeKind {
    World,
    Photo,
    Teleport,
}
impl PipeKind {
    fn name(self) -> &'static str {
        match self {
            Self::World => "World",
            Self::Photo => "Photo",
            Self::Teleport => "Teleport",
        }
    }
    fn reply_bytes(self) -> usize {
        match self {
            Self::World => 176,
            Self::Photo => 64,
            Self::Teleport => 14016,
        }
    }
    fn error(self, message: impl Into<String>) -> AppError {
        AppError {
            code: match self {
                Self::World => "world_controls",
                Self::Photo => "photo_mode",
                Self::Teleport => "teleport",
            },
            message: message.into(),
        }
    }
}

pub fn transact<const N: usize>(kind: PipeKind, pid: u32, request: &[u8]) -> Result<[u8; N]> {
    if pid == 0 || N != kind.reply_bytes() {
        return Err(kind.error("Invalid native control connection."));
    }
    let name = format!(r"\\.\pipe\CrimsonWorkbench.{}.v1.{pid}", kind.name());
    let opening_deadline = Instant::now() + Duration::from_millis(500);
    let mut file = loop {
        match OpenOptions::new().read(true).write(true).open(&name) {
            Ok(file) => break file,
            Err(e) if e.raw_os_error() == Some(231) && Instant::now() < opening_deadline => {
                std::thread::sleep(Duration::from_millis(10));
            }
            _ => {
                return Err(kind.error(format!(
                    "Live {} controls are not connected. Install the matching module with the game closed, then start the game and load a save.",
                    kind.name().to_ascii_lowercase()
                )));
            }
        }
    };
    let mut server = 0;
    let mode = 3u32; // MESSAGE | NOWAIT: bounded reads, one message per write.
    unsafe {
        if GetNamedPipeServerProcessId(file.as_raw_handle(), &mut server) == 0
            || server != pid
            || SetNamedPipeHandleState(
                file.as_raw_handle(),
                &mode,
                std::ptr::null(),
                std::ptr::null(),
            ) == 0
        {
            return Err(kind.error("Could not verify the connection to this game instance."));
        }
    }
    // Busy opening can wait; transmission is exactly one attempt. Retrying a
    // partial or failed write could repeat a mutation with an unknown outcome.
    if file.write(request).map_err(|_| {
        kind.error(
            "The control request could not be transmitted. Read status before another change.",
        )
    })? != request.len()
    {
        return Err(kind.error(
            "Incomplete control transmission. Read status; this command was not automatically retried.",
        ));
    }
    let deadline = Instant::now() + Duration::from_millis(1500);
    let mut reply = [0; N];
    loop {
        match file.read(&mut reply) {
            Ok(count) if count == N => {
                // The native server waits for this bounded consumption ACK
                // before disconnecting, which would discard unread output.
                let _ = file.write(&[0xac]);
                return Ok(reply);
            }
            Ok(0) if Instant::now() < deadline => std::thread::sleep(Duration::from_millis(10)),
            Err(e) if e.raw_os_error() == Some(232) && Instant::now() < deadline => {
                std::thread::sleep(Duration::from_millis(10));
            }
            _ => {
                return Err(kind.error(
                    "No complete control response. Read current status before another change; this command was not automatically retried.",
                ));
            }
        }
    }
}
