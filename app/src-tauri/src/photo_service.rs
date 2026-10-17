use crate::service::{AppError, Result};
use serde::{Deserialize, Serialize};
use std::path::Path;

const MAGIC: u32 = 0x48505743;
const PROTOCOL: u16 = 1;

#[derive(Clone, Copy, Deserialize, Serialize)]
#[serde(rename_all = "snake_case")]
pub enum Operation {
    Enter,
    HideHud,
    ShowHud,
    Leave,
}
impl Operation {
    fn wire(self) -> u32 {
        match self {
            Self::Enter => 1,
            Self::HideHud => 2,
            Self::ShowHud => 3,
            Self::Leave => 4,
        }
    }
}

#[derive(Clone, Deserialize, Serialize)]
#[serde(deny_unknown_fields)]
pub struct ControlRequest {
    pub pid: u32,
    pub id: String,
    pub session: String,
    pub epoch: String,
    pub not_after_ms: String,
    pub operation: Operation,
    pub hide_on_enter: Option<bool>,
}

#[derive(Serialize, Debug)]
pub struct Snapshot {
    pub pid: u32,
    pub state: &'static str,
    pub id: String,
    pub session: String,
    pub epoch: String,
    pub observed_ms: String,
    pub reason: u32,
    pub native_photo: bool,
    pub owned: bool,
    pub hud_hidden: bool,
    pub scene_frozen: bool,
}

fn error(message: impl Into<String>) -> AppError {
    AppError {
        code: "photo_mode",
        message: message.into(),
    }
}
fn number(text: &str) -> Result<u64> {
    if text.is_empty()
        || text.len() > 20
        || !text.bytes().all(|b| b.is_ascii_digit())
        || (text.len() > 1 && text.starts_with('0'))
    {
        return Err(error("Invalid photo-mode identifier."));
    }
    text.parse()
        .map_err(|_| error("Photo-mode identifier is out of range."))
}

fn encode(request: Option<&ControlRequest>) -> Result<[u8; 56]> {
    let mut out = [0; 56];
    out[0..4].copy_from_slice(&MAGIC.to_le_bytes());
    out[4..6].copy_from_slice(&PROTOCOL.to_le_bytes());
    out[6..8].copy_from_slice(&56u16.to_le_bytes());
    // Native status and all non-enter mutations require this word to be one.
    out[44..48].copy_from_slice(&1u32.to_le_bytes());
    let Some(r) = request else { return Ok(out) };
    if r.pid == 0 {
        return Err(error("Missing game instance."));
    }
    for (offset, text) in [
        (8, &r.session),
        (16, &r.id),
        (24, &r.epoch),
        (32, &r.not_after_ms),
    ] {
        let value = number(text)?;
        if value == 0 {
            return Err(error("Photo-mode mutation identifiers must not be zero."));
        }
        out[offset..offset + 8].copy_from_slice(&value.to_le_bytes());
    }
    out[40..44].copy_from_slice(&r.operation.wire().to_le_bytes());
    match (r.operation, r.hide_on_enter) {
        (Operation::Enter, Some(hide)) => {
            out[44..48].copy_from_slice(&u32::from(hide).to_le_bytes());
        }
        (Operation::HideHud | Operation::ShowHud | Operation::Leave, None) => {}
        _ => return Err(error("Invalid photo-mode operation payload.")),
    }
    Ok(out)
}

#[cfg(any(windows, test))]
fn decode(bytes: &[u8; 64], pid: u32) -> Result<Snapshot> {
    let u16_at = |n| u16::from_le_bytes(bytes[n..n + 2].try_into().unwrap());
    let u32_at = |n| u32::from_le_bytes(bytes[n..n + 4].try_into().unwrap());
    let u64_at = |n| u64::from_le_bytes(bytes[n..n + 8].try_into().unwrap());
    if u32_at(0) != MAGIC
        || u16_at(4) != PROTOCOL
        || u16_at(6) != 64
        || u32_at(44) > 12
        || [48, 52, 56, 60].iter().any(|offset| u32_at(*offset) > 1)
    {
        return Err(error("Invalid response from the photo-mode module."));
    }
    let state = [
        "waiting",
        "ready",
        "pending",
        "native_queued",
        "active",
        "hud_hidden",
        "hud_visible",
        "left",
        "rejected",
        "uncertain",
    ]
    .get(u32_at(40) as usize)
    .copied()
    .ok_or_else(|| error("Unknown photo-mode status."))?;
    // Zero context is a valid unavailable/busy native status. Only mutations
    // require nonzero context, enforced when encoding the command above.
    Ok(Snapshot {
        pid,
        state,
        id: u64_at(16).to_string(),
        session: u64_at(8).to_string(),
        epoch: u64_at(24).to_string(),
        observed_ms: u64_at(32).to_string(),
        reason: u32_at(44),
        native_photo: u32_at(48) != 0,
        owned: u32_at(52) != 0,
        hud_hidden: u32_at(56) != 0,
        scene_frozen: u32_at(60) != 0,
    })
}

pub fn exchange(game: &Path, request: Option<&ControlRequest>) -> Result<Snapshot> {
    let wire = encode(request)?;
    #[cfg(windows)]
    {
        let pid = crate::spawn_service::running_game_pid(game)?;
        if request.is_some_and(|r| r.pid != pid) {
            return Err(error(
                "The game instance changed. Read the live status before another change.",
            ));
        }
        let reply =
            crate::native_pipe::transact::<64>(crate::native_pipe::PipeKind::Photo, pid, &wire)?;
        decode(&reply, pid)
    }
    #[cfg(not(windows))]
    {
        let _ = (game, wire);
        Err(error("Live photo-mode controls require Windows."))
    }
}

#[cfg(test)]
mod tests {
    use super::*;

    fn request() -> ControlRequest {
        ControlRequest {
            pid: 123,
            id: "9007199254740999".into(),
            session: "18364758544493064720".into(),
            epoch: "9007199254740993".into(),
            not_after_ms: "18446744073709551615".into(),
            operation: Operation::Enter,
            hide_on_enter: Some(true),
        }
    }
    fn response() -> [u8; 64] {
        let mut out = [0; 64];
        out[0..4].copy_from_slice(&MAGIC.to_le_bytes());
        out[4..6].copy_from_slice(&PROTOCOL.to_le_bytes());
        out[6..8].copy_from_slice(&64u16.to_le_bytes());
        out
    }

    #[test]
    fn exact_photo_request_layout_and_lossless_json_identifiers() {
        let r = request();
        let bytes = encode(Some(&r)).unwrap();
        assert_eq!(&bytes[..8], &[67, 87, 80, 72, 1, 0, 56, 0]);
        assert_eq!(
            u64::from_le_bytes(bytes[8..16].try_into().unwrap()),
            0xfedcba9876543210
        );
        assert_eq!(
            u64::from_le_bytes(bytes[16..24].try_into().unwrap()),
            9_007_199_254_740_999
        );
        assert_eq!(
            u64::from_le_bytes(bytes[24..32].try_into().unwrap()),
            9_007_199_254_740_993
        );
        assert_eq!(
            u64::from_le_bytes(bytes[32..40].try_into().unwrap()),
            u64::MAX
        );
        assert_eq!(&bytes[40..48], &[1, 0, 0, 0, 1, 0, 0, 0]);
        assert_eq!(&bytes[48..], &[0; 8]);
        let json = serde_json::to_value(&r).unwrap();
        assert_eq!(json["operation"], "enter");
        assert_eq!(json["session"], "18364758544493064720");
        assert_eq!(json["not_after_ms"], u64::MAX.to_string());
        assert!(serde_json::from_value::<ControlRequest>(json).is_ok());
    }

    #[test]
    fn strict_decimal_context_and_operation_payloads() {
        for bad in [
            "",
            "01",
            "-1",
            "+1",
            "1.0",
            " 1",
            "18446744073709551616",
            "１",
        ] {
            assert!(number(bad).is_err());
        }
        assert_eq!(number("0").unwrap(), 0);
        assert_eq!(number("18446744073709551615").unwrap(), u64::MAX);
        for field in 0..5 {
            let mut r = request();
            match field {
                0 => r.pid = 0,
                1 => r.id = "0".into(),
                2 => r.session = "0".into(),
                3 => r.epoch = "0".into(),
                _ => r.not_after_ms = "0".into(),
            }
            assert!(encode(Some(&r)).is_err());
        }
        let mut r = request();
        r.hide_on_enter = Some(false);
        assert_eq!(encode(Some(&r)).unwrap()[44], 0);
        r.hide_on_enter = None;
        assert!(encode(Some(&r)).is_err());
        for (operation, value) in [
            (Operation::HideHud, 2),
            (Operation::ShowHud, 3),
            (Operation::Leave, 4),
        ] {
            r.operation = operation;
            let bytes = encode(Some(&r)).unwrap();
            assert_eq!(bytes[40], value);
            assert_eq!(bytes[44], 1);
            for hide in [false, true] {
                r.hide_on_enter = Some(hide);
                assert!(encode(Some(&r)).is_err());
            }
            r.hide_on_enter = None;
        }
        let mut json = serde_json::to_value(request()).unwrap();
        json["extra"] = serde_json::json!(true);
        assert!(serde_json::from_value::<ControlRequest>(json).is_err());
        let mut json = serde_json::to_value(request()).unwrap();
        json["operation"] = serde_json::json!("status");
        assert!(serde_json::from_value::<ControlRequest>(json).is_err());
    }

    #[test]
    fn status_allows_zero_native_context_and_every_declared_state_and_reason() {
        let bytes = encode(None).unwrap();
        assert!(bytes[8..44].iter().all(|b| *b == 0));
        assert_eq!(&bytes[44..48], &1u32.to_le_bytes());
        assert_eq!(&bytes[48..], &[0; 8]);
        let mut reply = response();
        for state in 0..10u32 {
            reply[40..44].copy_from_slice(&state.to_le_bytes());
            for reason in 0..13u32 {
                reply[44..48].copy_from_slice(&reason.to_le_bytes());
                let snapshot = decode(&reply, 123).unwrap();
                assert_eq!(snapshot.id, "0");
                assert_eq!(snapshot.session, "0");
                assert_eq!(snapshot.epoch, "0");
                assert_eq!(snapshot.observed_ms, "0");
                assert_eq!(snapshot.reason, reason);
                assert!(
                    !snapshot.native_photo
                        && !snapshot.owned
                        && !snapshot.hud_hidden
                        && !snapshot.scene_frozen
                );
            }
        }
    }

    #[test]
    fn strict_photo_response_and_full_width_snapshot() {
        let mut reply = response();
        for offset in [8, 16, 24, 32] {
            reply[offset..offset + 8].copy_from_slice(&u64::MAX.to_le_bytes());
        }
        reply[40..44].copy_from_slice(&4u32.to_le_bytes());
        for offset in [48, 52, 56, 60] {
            reply[offset..offset + 4].copy_from_slice(&1u32.to_le_bytes());
        }
        let snapshot = decode(&reply, 456).unwrap();
        assert_eq!(snapshot.state, "active");
        assert!(
            snapshot.native_photo && snapshot.owned && snapshot.hud_hidden && snapshot.scene_frozen
        );
        let json = serde_json::to_value(snapshot).unwrap();
        for field in ["id", "session", "epoch", "observed_ms"] {
            assert_eq!(json[field], u64::MAX.to_string());
        }
        for offset in [0, 4, 6] {
            let old = reply[offset];
            reply[offset] ^= 1;
            assert!(decode(&reply, 123).is_err());
            reply[offset] = old;
        }
        for (offset, invalid) in [(40, 10u32), (44, 13), (48, 2), (52, 2), (56, 2), (60, 2)] {
            let old = reply[offset..offset + 4].to_vec();
            reply[offset..offset + 4].copy_from_slice(&invalid.to_le_bytes());
            assert!(decode(&reply, 123).is_err());
            reply[offset..offset + 4].copy_from_slice(&old);
        }
    }

    #[cfg(windows)]
    #[test]
    #[ignore = "Uses only the exact owned photo pipe host; never connects to a game"]
    fn real_windows_photo_pipe() {
        use std::os::windows::process::CommandExt;
        struct Host(std::process::Child);
        impl Drop for Host {
            fn drop(&mut self) {
                let _ = self.0.kill();
                let _ = self.0.wait();
            }
        }
        let workspace = std::path::Path::new(env!("CARGO_MANIFEST_DIR"))
            .join("../..")
            .canonicalize()
            .unwrap();
        let host = workspace
            .join(".local/photo-mode-pipe-build/Release/marketplace-photo-pipe-host.exe")
            .canonicalize()
            .expect("Build the owned photo pipe fixture first");
        assert_eq!(host.file_name().unwrap(), "marketplace-photo-pipe-host.exe");
        let mut child = Host(
            std::process::Command::new(&host)
                .creation_flags(0x08000000)
                .spawn()
                .unwrap(),
        );
        let pid = child.0.id();
        let transact = |wire: &[u8]| {
            crate::native_pipe::transact::<64>(crate::native_pipe::PipeKind::Photo, pid, wire)
        };
        let mut ready = None;
        for _ in 0..100 {
            if let Ok(bytes) = transact(&encode(None).unwrap()) {
                ready = Some(decode(&bytes, pid).unwrap());
                break;
            }
            assert!(child.0.try_wait().unwrap().is_none());
            std::thread::sleep(std::time::Duration::from_millis(20));
        }
        let ready = ready.expect("owned photo host ready");
        assert_eq!(ready.state, "ready");
        assert_eq!(ready.session, "18364758544493064720");
        assert_eq!(ready.epoch, "9007199254740993");
        assert!(!ready.native_photo && !ready.owned && !ready.hud_hidden && !ready.scene_frozen);

        assert!(transact(&[0; 55]).is_err());
        assert!(transact(&[0; 57]).is_err());
        let mut malformed = encode(None).unwrap();
        malformed[0] ^= 1;
        let rejected = decode(&transact(&malformed).unwrap(), pid).unwrap();
        assert_eq!(rejected.state, "rejected");
        assert_eq!(rejected.reason, 1);

        let mut r = request();
        r.pid = pid;
        r.hide_on_enter = Some(false);
        let active = decode(&transact(&encode(Some(&r)).unwrap()).unwrap(), pid).unwrap();
        assert_eq!(active.id, r.id);
        assert_eq!(active.session, r.session);
        assert_eq!(active.epoch, r.epoch);
        assert_eq!(active.state, "active");
        assert!(active.native_photo && active.owned && active.scene_frozen && !active.hud_hidden);
        r.operation = Operation::HideHud;
        r.hide_on_enter = None;
        r.id = "9007199254741000".into();
        let hidden = decode(&transact(&encode(Some(&r)).unwrap()).unwrap(), pid).unwrap();
        assert_eq!(hidden.id, r.id);
        assert_eq!(hidden.state, "hud_hidden");
        assert!(hidden.owned && hidden.scene_frozen && hidden.hud_hidden);
        r.operation = Operation::ShowHud;
        r.id = "9007199254741001".into();
        let visible = decode(&transact(&encode(Some(&r)).unwrap()).unwrap(), pid).unwrap();
        assert_eq!(visible.id, r.id);
        assert_eq!(visible.state, "hud_visible");
        assert!(visible.owned && visible.scene_frozen && !visible.hud_hidden);
        r.operation = Operation::Leave;
        r.id = "9007199254741002".into();
        let left = decode(&transact(&encode(Some(&r)).unwrap()).unwrap(), pid).unwrap();
        assert_eq!(left.id, r.id);
        assert_eq!(left.state, "left");
        assert!(!left.native_photo && !left.owned && !left.hud_hidden && !left.scene_frozen);
        assert_eq!(
            decode(&transact(&encode(None).unwrap()).unwrap(), pid)
                .unwrap()
                .id,
            r.id
        );
    }
}
