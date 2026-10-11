use crate::service::{AppError, Result};
use serde::{Deserialize, Serialize};
use std::path::Path;

const MAGIC: u32 = 0x49574443;
const WIRE_VERSION: u16 = 2;
#[derive(Clone, Deserialize)]
pub struct GrantRequest {
    pub id: String,
    #[cfg_attr(not(any(windows, test)), allow(dead_code))]
    pub pid: u32,
    pub epoch: u64,
    pub key: u32,
    pub quantity: u32,
}
#[derive(Serialize, Debug)]
pub struct Snapshot {
    pub state: &'static str,
    pub pid: u32,
    pub epoch: u64,
    pub id: String,
    pub key: u32,
    pub quantity: u32,
    pub message: String,
}
fn error(message: impl Into<String>) -> AppError {
    AppError {
        code: "live_items",
        message: message.into(),
    }
}
fn id_bytes(id: &str) -> Result<[u8; 16]> {
    let compact = id.replace('-', "");
    if compact.len() != 32 || !compact.is_ascii() {
        return Err(error("Invalid request ID."));
    }
    let mut out = [0; 16];
    for (i, b) in out.iter_mut().enumerate() {
        *b = u8::from_str_radix(&compact[i * 2..i * 2 + 2], 16)
            .map_err(|_| error("Invalid request ID."))?;
    }
    if out == [0; 16] {
        return Err(error("Empty request ID."));
    }
    Ok(out)
}
fn encode(operation: u16, request: Option<&GrantRequest>) -> Result<[u8; 40]> {
    let mut out = [0; 40];
    out[..4].copy_from_slice(&MAGIC.to_le_bytes());
    out[4..6].copy_from_slice(&WIRE_VERSION.to_le_bytes());
    out[6..8].copy_from_slice(&operation.to_le_bytes());
    if let Some(r) = request {
        out[8..24].copy_from_slice(&id_bytes(&r.id)?);
        out[24..28].copy_from_slice(&r.key.to_le_bytes());
        out[28..32].copy_from_slice(&r.quantity.to_le_bytes());
        out[32..].copy_from_slice(&r.epoch.to_le_bytes());
    }
    Ok(out)
}
#[cfg(any(windows, test))]
fn decode(bytes: &[u8; 48], expected_pid: u32, request: Option<&GrantRequest>) -> Result<Snapshot> {
    let u16_at = |n| u16::from_le_bytes(bytes[n..n + 2].try_into().unwrap());
    let u32_at = |n| u32::from_le_bytes(bytes[n..n + 4].try_into().unwrap());
    if u32_at(0) != MAGIC || u16_at(4) != WIRE_VERSION || u32_at(8) != expected_pid {
        return Err(error("Invalid response from live module."));
    }
    let epoch = u64::from_le_bytes(bytes[40..48].try_into().unwrap());
    if let Some(r) = request {
        if r.epoch != epoch || r.pid != expected_pid {
            return Err(error(
                "The game instance changed. Check inventory before making a new request.",
            ));
        }
        if bytes[16..32] != id_bytes(&r.id)? {
            return Err(error("Response belongs to another request."));
        }
        if u32_at(32) != r.key || u32_at(36) != r.quantity {
            return Err(error(
                "Item or quantity in the response does not match the request.",
            ));
        }
    }
    let state = match u16_at(6) {
        0 => "offline",
        1 => "ready",
        2 => "queued",
        3 => "executing",
        4 => "applied",
        5 => "rejected",
        6 => "uncertain",
        7 => "expired",
        8 => "cancelled",
        9 => "starting",
        _ => return Err(error("Unbekannter Live-Status.")),
    };
    let detail = match u32_at(12) {
        0 => match state {
            "starting" => {
                "Load a save and return to the game briefly. The module connects to the active inventory automatically."
            }
            "ready" => "Live-Spawner verbunden.",
            "queued" => {
                "Request is waiting for the next character update. Return to the game briefly; you do not need to pick anything up."
            }
            "executing" => "The game is processing the request.",
            "applied" => "The additional item quantity was confirmed in game. Save normally now.",
            "expired" => {
                "The game did not process a matching character call within 30 seconds. No items were given."
            }
            "cancelled" => "Request cancelled before execution.",
            _ => "Live module not connected.",
        },
        1 => "Protocol version mismatch.",
        2 => "Invalid item quantity or changed request ID.",
        3 => "Another request is already running.",
        4 => "This request is unknown to the module.",
        5 => "The module is not ready or is blocked after an uncertain result.",
        6 => "The selected item ID is absent from the native item table of this build.",
        7 => {
            "This item type is limited to 100 units per request by the live creation path. Choose a smaller quantity."
        }
        8 => {
            "Inventory context unverified or changed after loading/switching characters. No items given."
        }
        9 => "Native exception. Result uncertain; no automatic retry.",
        10 => "The game reported an error. Result uncertain; check inventory.",
        11 => "The additional quantity could not be confirmed. Result uncertain; check inventory.",
        12 => "Request log is full. Save and restart the game.",
        13 => "Native item definition is missing or does not match the selected ID.",
        14 => "This item has more than five preset sockets. Native output supports at most five.",
        15 => "Native conversion changed the item ID or quantity. Nothing was given.",
        _ => return Err(error("Unknown live module error.")),
    };
    Ok(Snapshot {
        state,
        pid: expected_pid,
        epoch,
        id: bytes[16..32].iter().map(|b| format!("{b:02x}")).collect(),
        key: u32_at(32),
        quantity: u32_at(36),
        message: detail.into(),
    })
}
pub fn exchange(game: &Path, operation: u16, request: Option<&GrantRequest>) -> Result<Snapshot> {
    if operation == 2 {
        let r = request.ok_or_else(|| error("Request missing."))?;
        if r.key == 0 || r.quantity == 0 || r.quantity > 10000 {
            return Err(error("Quantity must be between 1 and 10,000."));
        }
    }
    let wire = encode(operation, request)?;
    #[cfg(windows)]
    {
        let pid = windows::game_pid(game)?;
        if let Some(r) = request
            && r.pid != pid
        {
            return Err(error(
                "The game instance changed. Check inventory before making a new request.",
            ));
        }
        let bytes = windows::transact(pid, &wire)?;
        decode(&bytes, pid, request)
    }
    #[cfg(not(windows))]
    {
        let _ = (game, wire);
        Err(error("The live spawner requires Windows."))
    }
}
#[cfg(windows)]
mod windows {
    use super::*;
    use std::{
        fs::OpenOptions,
        io::{Read, Write},
        os::windows::io::AsRawHandle,
        time::{Duration, Instant},
    };
    use windows_sys::Win32::{
        Foundation::{CloseHandle, INVALID_HANDLE_VALUE},
        System::{
            Diagnostics::ToolHelp::{
                CreateToolhelp32Snapshot, PROCESSENTRY32W, Process32FirstW, Process32NextW,
                TH32CS_SNAPPROCESS,
            },
            Pipes::{GetNamedPipeServerProcessId, SetNamedPipeHandleState},
            Threading::{
                OpenProcess, PROCESS_QUERY_LIMITED_INFORMATION, QueryFullProcessImageNameW,
            },
        },
    };
    pub fn game_pid(game: &Path) -> Result<u32> {
        unsafe {
            let expected = game
                .join("bin64/CrimsonDesert.exe")
                .canonicalize()
                .map_err(|e| error(e.to_string()))?;
            let snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
            if snapshot == INVALID_HANDLE_VALUE {
                return Err(error("Process list unavailable."));
            }
            let mut entry: PROCESSENTRY32W = std::mem::zeroed();
            entry.dwSize = std::mem::size_of_val(&entry) as u32;
            let mut found = Vec::new();
            let mut valid = Process32FirstW(snapshot, &mut entry) != 0;
            while valid {
                let end = entry
                    .szExeFile
                    .iter()
                    .position(|c| *c == 0)
                    .unwrap_or(entry.szExeFile.len());
                if String::from_utf16_lossy(&entry.szExeFile[..end])
                    .eq_ignore_ascii_case("CrimsonDesert.exe")
                {
                    let process =
                        OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, 0, entry.th32ProcessID);
                    if !process.is_null() {
                        let mut path = vec![0u16; 32768];
                        let mut size = path.len() as u32;
                        if QueryFullProcessImageNameW(process, 0, path.as_mut_ptr(), &mut size) != 0
                        {
                            let path = std::path::PathBuf::from(String::from_utf16_lossy(
                                &path[..size as usize],
                            ));
                            if path.canonicalize().ok().as_ref() == Some(&expected) {
                                found.push(entry.th32ProcessID);
                            }
                        }
                        CloseHandle(process);
                    }
                }
                valid = Process32NextW(snapshot, &mut entry) != 0;
            }
            CloseHandle(snapshot);
            if found.len() != 1 {
                return Err(error(
                    "A single running Crimson Desert instance could not be identified.",
                ));
            }
            Ok(found[0])
        }
    }
    pub fn transact(pid: u32, request: &[u8; 40]) -> Result<[u8; 48]> {
        let name = format!(r"\\.\pipe\CrimsonWorkbench.Items.v2.{pid}");
        let opening_deadline = Instant::now() + Duration::from_millis(500);
        let mut file = loop {
            match OpenOptions::new().read(true).write(true).open(&name) {
                Ok(file) => break file,
                Err(e) if e.raw_os_error() == Some(231) && Instant::now() < opening_deadline => {
                    std::thread::sleep(Duration::from_millis(10))
                }
                _ => {
                    return Err(error(
                        "Live module not connected. Install the matching ASI with the game closed, then restart the game to load it.",
                    ));
                }
            }
        };
        let handle = file.as_raw_handle();
        let mut server = 0;
        let mode = 3u32; // PIPE_READMODE_MESSAGE | PIPE_NOWAIT
        unsafe {
            if GetNamedPipeServerProcessId(handle, &mut server) == 0
                || server != pid
                || SetNamedPipeHandleState(handle, &mode, std::ptr::null(), std::ptr::null()) == 0
            {
                return Err(error("Could not check the connection to the game."));
            }
        }
        // A single write forms one message. Never retry a grant after a timeout.
        if file.write(request).map_err(|e| error(e.to_string()))? != request.len() {
            return Err(error(
                "Request only partially transmitted. Check the result.",
            ));
        }
        let deadline = Instant::now() + Duration::from_millis(1500);
        let mut reply = [0; 48];
        loop {
            match file.read(&mut reply) {
                Ok(48) => {
                    let _ = file.write(&[0xac]);
                    return Ok(reply);
                }
                Ok(0) if Instant::now() < deadline => std::thread::sleep(Duration::from_millis(10)),
                Err(e) if e.raw_os_error() == Some(232) && Instant::now() < deadline => {
                    std::thread::sleep(Duration::from_millis(10))
                }
                other => {
                    return Err(error(format!(
                        "No complete response ({other:?}). Check the result using the same request ID; do not grant again."
                    )));
                }
            }
        }
    }
}
#[cfg(test)]
mod tests {
    use super::*;
    fn request() -> GrantRequest {
        GrantRequest {
            id: "12345678-1234-4234-8234-123456789abc".into(),
            pid: 123,
            epoch: 456,
            key: 2200,
            quantity: 10,
        }
    }
    #[test]
    fn exact_wire_and_identity() {
        let r = request();
        let wire = encode(2, Some(&r)).unwrap();
        assert_eq!(&wire[24..28], &2200u32.to_le_bytes());
        assert_eq!(&wire[4..6], &WIRE_VERSION.to_le_bytes());
        assert!(id_bytes("../invalid").is_err());
        assert!(id_bytes(&"0".repeat(32)).is_err());
        let mut reply = [0; 48];
        reply[..8].copy_from_slice(&wire[..8]);
        reply[6..8].copy_from_slice(&4u16.to_le_bytes());
        reply[8..12].copy_from_slice(&123u32.to_le_bytes());
        reply[16..32].copy_from_slice(&wire[8..24]);
        reply[32..40].copy_from_slice(&wire[24..32]);
        reply[40..].copy_from_slice(&456u64.to_le_bytes());
        assert_eq!(decode(&reply, 123, Some(&r)).unwrap().state, "applied");
        reply[4..6].copy_from_slice(&1u16.to_le_bytes());
        assert!(decode(&reply, 123, Some(&r)).is_err());
        reply[4..6].copy_from_slice(&WIRE_VERSION.to_le_bytes());
        reply[32] += 1;
        assert!(decode(&reply, 123, Some(&r)).is_err());
        reply[32] -= 1;
        reply[40] += 1;
        assert!(decode(&reply, 123, Some(&r)).is_err());
    }
    #[cfg(windows)]
    #[test]
    #[ignore = "Requires the own-process C++ pipe host via CD_LIVE_PIPE_HOST; never connects to the game"]
    fn real_windows_pipe() {
        use std::os::windows::process::CommandExt;
        struct Host(std::process::Child);
        impl Drop for Host {
            fn drop(&mut self) {
                let _ = self.0.kill();
                let _ = self.0.wait();
            }
        }
        let host = std::env::var_os("CD_LIVE_PIPE_HOST").expect("own-process host path");
        let mut child = Host(
            std::process::Command::new(host)
                .creation_flags(0x08000000)
                .spawn()
                .unwrap(),
        );
        let pid = child.0.id();
        let mut reply = None;
        for _ in 0..100 {
            if let Ok(bytes) = windows::transact(pid, &encode(1, None).unwrap()) {
                reply = Some(decode(&bytes, pid, None).unwrap());
                break;
            }
            assert!(child.0.try_wait().unwrap().is_none());
            std::thread::sleep(std::time::Duration::from_millis(20));
        }
        let ready = reply.expect("host ready");
        assert_eq!(ready.state, "ready");
        let mut r = request();
        r.pid = pid;
        r.epoch = ready.epoch;
        let send = |op, r: &GrantRequest| {
            decode(
                &windows::transact(pid, &encode(op, Some(r)).unwrap()).unwrap(),
                pid,
                Some(r),
            )
            .unwrap()
        };
        assert_eq!(send(2, &r).state, "queued");
        assert_eq!(send(2, &r).state, "queued");
        assert_eq!(send(3, &r).state, "queued");
        assert_eq!(send(4, &r).state, "cancelled");
        assert_eq!(send(2, &r).state, "cancelled");
        r.id = "22222222-2222-4222-8222-222222222222".into();
        r.epoch += 1;
        let bytes = windows::transact(pid, &encode(2, Some(&r)).unwrap()).unwrap();
        assert_eq!(u16::from_le_bytes(bytes[6..8].try_into().unwrap()), 5);
        assert!(decode(&bytes, pid, Some(&r)).is_err());
    }
}
