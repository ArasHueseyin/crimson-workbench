use crate::service::{AppError, Result};
use serde::{Deserialize, Serialize};
#[cfg(any(windows, target_os = "macos", test))]
use std::collections::HashSet;
use std::path::Path;

const MAGIC: u32 = 0x54505743;
const PROTOCOL: u16 = 1;
#[cfg(any(windows, target_os = "macos", test))]
const REPLY_BYTES: usize = 14016;
const SAVED_SLOTS: usize = 16;
#[cfg(any(windows, target_os = "macos", test))]
const MAX_MARKERS: usize = 520;

#[derive(Clone, Copy, Debug, Deserialize, Serialize)]
#[serde(rename_all = "snake_case")]
pub enum MarkerSource {
    MapPin,
    Waypoint,
}
impl MarkerSource {
    fn wire(self) -> u32 {
        match self {
            Self::MapPin => 0,
            Self::Waypoint => 1,
        }
    }
}
#[derive(Debug, Serialize)]
#[serde(rename_all = "snake_case")]
#[cfg_attr(
    not(any(windows, target_os = "macos", test)),
    expect(
        dead_code,
        reason = "Native marker layers are constructed by the native reply decoder."
    )
)]
pub enum MarkerLayer {
    Unknown,
    Mainland,
    Alternate,
}

// Each operation has a distinct DTO. Unknown fields, including frontend XYZ
// and payloads belonging to another operation, are rejected by deserialization.
#[derive(Clone, Debug, Deserialize, Serialize)]
#[serde(tag = "operation", rename_all = "snake_case", deny_unknown_fields)]
pub enum ControlRequest {
    SaveCurrent {
        pid: u32,
        id: String,
        session: String,
        epoch: String,
        not_after_ms: String,
        slot: u32,
        name: String,
    },
    RemoveSaved {
        pid: u32,
        id: String,
        session: String,
        epoch: String,
        not_after_ms: String,
        slot: u32,
    },
    TravelSaved {
        pid: u32,
        id: String,
        session: String,
        epoch: String,
        not_after_ms: String,
        slot: u32,
    },
    TravelMarker {
        pid: u32,
        id: String,
        session: String,
        epoch: String,
        not_after_ms: String,
        marker: String,
        marker_source: MarkerSource,
    },
}
struct Context<'a> {
    pid: u32,
    id: &'a str,
    session: &'a str,
    epoch: &'a str,
    not_after_ms: &'a str,
}
impl ControlRequest {
    fn context(&self) -> Context<'_> {
        match self {
            Self::SaveCurrent {
                pid,
                id,
                session,
                epoch,
                not_after_ms,
                ..
            }
            | Self::RemoveSaved {
                pid,
                id,
                session,
                epoch,
                not_after_ms,
                ..
            }
            | Self::TravelSaved {
                pid,
                id,
                session,
                epoch,
                not_after_ms,
                ..
            }
            | Self::TravelMarker {
                pid,
                id,
                session,
                epoch,
                not_after_ms,
                ..
            } => Context {
                pid: *pid,
                id,
                session,
                epoch,
                not_after_ms,
            },
        }
    }
    #[cfg(any(windows, target_os = "macos", test))]
    pub fn pid(&self) -> u32 {
        self.context().pid
    }
}

#[derive(Debug, Serialize)]
pub struct Position {
    pub x: f32,
    pub y: f32,
    pub z: f32,
}
#[derive(Debug, Serialize)]
pub struct SavedPosition {
    pub field_definition: u32,
    pub stage: u16,
    pub position: Position,
    pub yaw: f32,
}
#[derive(Debug, Serialize)]
pub struct Bookmark {
    pub occupied: bool,
    pub name: String,
    pub position: Option<SavedPosition>,
}
#[derive(Debug, Serialize)]
pub struct Marker {
    pub id: String,
    pub position: Position,
    pub source: MarkerSource,
    pub layer: MarkerLayer,
    pub shape: u8,
    pub color: u8,
}
#[derive(Debug, Serialize)]
pub struct Snapshot {
    pub pid: u32,
    pub state: &'static str,
    pub id: String,
    pub session: String,
    pub epoch: String,
    pub observed_ms: String,
    pub reason: u32,
    pub features: u32,
    pub identity: u32,
    pub world: u32,
    pub native_reason: u32,
    pub native_error: i32,
    pub storage_status: u32,
    pub bookmarks_revision: String,
    pub current: Option<SavedPosition>,
    pub bookmarks: [Bookmark; SAVED_SLOTS],
    pub marker_status: u32,
    pub markers_observed_ms: String,
    pub markers: Vec<Marker>,
}
fn error(message: impl Into<String>) -> AppError {
    AppError {
        code: "teleport",
        message: message.into(),
    }
}
fn number(text: &str) -> Result<u64> {
    if text.is_empty()
        || text.len() > 20
        || !text.bytes().all(|b| b.is_ascii_digit())
        || (text.len() > 1 && text.starts_with('0'))
    {
        return Err(error("Invalid teleport identifier."));
    }
    text.parse()
        .map_err(|_| error("Teleport identifier is out of range."))
}
fn valid_name(name: &str) -> bool {
    !name.is_empty() && name.len() <= 47 && !name.chars().any(char::is_control)
}
fn encode(request: Option<&ControlRequest>) -> Result<[u8; 112]> {
    let mut out = [0; 112];
    out[0..4].copy_from_slice(&MAGIC.to_le_bytes());
    out[4..6].copy_from_slice(&PROTOCOL.to_le_bytes());
    out[6..8].copy_from_slice(&112u16.to_le_bytes());
    let Some(r) = request else { return Ok(out) };
    let c = r.context();
    if c.pid == 0 {
        return Err(error("Missing game instance."));
    }
    for (offset, text) in [
        (8, c.session),
        (16, c.id),
        (24, c.epoch),
        (32, c.not_after_ms),
    ] {
        let value = number(text)?;
        if value == 0 {
            return Err(error("Teleport mutation identifiers must not be zero."));
        }
        out[offset..offset + 8].copy_from_slice(&value.to_le_bytes());
    }
    let (operation, slot) = match r {
        ControlRequest::SaveCurrent { slot, name, .. } => {
            if !valid_name(name) {
                return Err(error(
                    "Choose a name of 1 through 47 UTF-8 bytes without control characters.",
                ));
            }
            out[56..56 + name.len()].copy_from_slice(name.as_bytes());
            (1, *slot)
        }
        ControlRequest::RemoveSaved { slot, .. } => (2, *slot),
        ControlRequest::TravelSaved { slot, .. } => (3, *slot),
        ControlRequest::TravelMarker {
            marker,
            marker_source,
            ..
        } => {
            let id = number(marker)?;
            if id == u64::MAX {
                return Err(error("Invalid native marker identifier."));
            }
            out[48..56].copy_from_slice(&id.to_le_bytes());
            out[104..108].copy_from_slice(&marker_source.wire().to_le_bytes());
            (4, 0)
        }
    };
    if slot >= SAVED_SLOTS as u32 {
        return Err(error("Teleport bookmark slot is out of range."));
    }
    out[40..44].copy_from_slice(&u32::to_le_bytes(operation));
    out[44..48].copy_from_slice(&slot.to_le_bytes());
    Ok(out)
}

#[cfg(any(windows, target_os = "macos", test))]
fn decode_name(bytes: &[u8], occupied: bool) -> Result<String> {
    let end = bytes
        .iter()
        .position(|b| *b == 0)
        .ok_or_else(|| error("Unterminated teleport bookmark name."))?;
    if bytes[end..].iter().any(|b| *b != 0) {
        return Err(error("Invalid teleport bookmark name padding."));
    }
    let name = std::str::from_utf8(&bytes[..end])
        .map_err(|_| error("Invalid UTF-8 teleport bookmark name."))?;
    if (occupied && !valid_name(name)) || (!occupied && !name.is_empty()) {
        return Err(error("Invalid teleport bookmark name."));
    }
    Ok(name.to_owned())
}
#[cfg(any(windows, target_os = "macos", test))]
fn decode_position(bytes: &[u8]) -> Result<Option<SavedPosition>> {
    let u16_at = |n| u16::from_le_bytes(bytes[n..n + 2].try_into().unwrap());
    let u32_at = |n| u32::from_le_bytes(bytes[n..n + 4].try_into().unwrap());
    let f32_at = |n| f32::from_le_bytes(bytes[n..n + 4].try_into().unwrap());
    // A native unavailable Reply contains exactly SavedPosition{}, not a
    // valid world coordinate. Do not invent a location for that placeholder.
    let mut empty = [0; 32];
    empty[4..6].copy_from_slice(&u16::MAX.to_le_bytes());
    empty[24..28].copy_from_slice(&1u32.to_le_bytes());
    if bytes == empty {
        return Ok(None);
    }
    if u32_at(0) == 0
        || u16_at(4) == u16::MAX
        || u16_at(6) != 0
        || u32_at(24) != 1
        || u32_at(28) != 0
        || [8, 12, 16, 20].iter().any(|n| !f32_at(*n).is_finite())
    {
        return Err(error("Invalid native teleport position."));
    }
    Ok(Some(SavedPosition {
        field_definition: u32_at(0),
        stage: u16_at(4),
        position: Position {
            x: f32_at(8),
            y: f32_at(12),
            z: f32_at(16),
        },
        yaw: f32_at(20),
    }))
}
#[cfg(any(windows, target_os = "macos", test))]
fn decode(bytes: &[u8; REPLY_BYTES], pid: u32) -> Result<Snapshot> {
    let u16_at = |n| u16::from_le_bytes(bytes[n..n + 2].try_into().unwrap());
    let u32_at = |n| u32::from_le_bytes(bytes[n..n + 4].try_into().unwrap());
    let u64_at = |n| u64::from_le_bytes(bytes[n..n + 8].try_into().unwrap());
    let f32_at = |n| f32::from_le_bytes(bytes[n..n + 4].try_into().unwrap());
    if u32_at(0) != MAGIC
        || u16_at(4) != PROTOCOL
        || usize::from(u16_at(6)) != REPLY_BYTES
        || u32_at(44) > 21
        || u32_at(48) & !7 != 0
        || u32_at(60) > 13
        || u32_at(68) > 2
        || u32_at(1520) > MAX_MARKERS as u32
        || u32_at(1524) > 11
    {
        return Err(error("Invalid response from the teleport module."));
    }
    let state = [
        "waiting",
        "ready",
        "pending",
        "captured",
        "removed",
        "native_queued",
        "position_observed",
        "rejected",
        "uncertain",
        "faulted",
    ]
    .get(u32_at(40) as usize)
    .copied()
    .ok_or_else(|| error("Unknown teleport status."))?;
    let current = decode_position(&bytes[80..112])?;
    let mut bookmarks = Vec::with_capacity(SAVED_SLOTS);
    for slot in 0..SAVED_SLOTS {
        let at = 112 + slot * 88;
        let occupied = u32_at(at + 80);
        if occupied > 1 || u32_at(at + 84) != 0 {
            return Err(error("Invalid teleport bookmark flags."));
        }
        let name = decode_name(&bytes[at + 32..at + 80], occupied != 0)?;
        // Unoccupied positions are ignored by the native persistence format.
        let position = if occupied == 1 {
            Some(
                decode_position(&bytes[at..at + 32])?
                    .ok_or_else(|| error("Occupied teleport bookmark has no position."))?,
            )
        } else {
            None
        };
        bookmarks.push(Bookmark {
            occupied: occupied != 0,
            name,
            position,
        });
    }
    let mut markers = Vec::with_capacity(u32_at(1520) as usize);
    let mut identities = HashSet::new();
    let mut counts = [0usize; 2];
    for row in 0..u32_at(1520) as usize {
        let at = 1536 + row * 24;
        let id = u64_at(at);
        let source = bytes[at + 20];
        let layer = bytes[at + 21];
        let shape = bytes[at + 22];
        let color = bytes[at + 23];
        if id == u64::MAX
            || source > 1
            || layer > 2
            || shape >= 14
            || color >= 15
            || [8, 12, 16].iter().any(|n| !f32_at(at + *n).is_finite())
            || !identities.insert((source, id))
        {
            return Err(error("Invalid native teleport marker."));
        }
        counts[source as usize] += 1;
        if counts[0] > 500 || counts[1] > 20 {
            return Err(error("Native teleport marker list is out of range."));
        }
        markers.push(Marker {
            id: id.to_string(),
            position: Position {
                x: f32_at(at + 8),
                y: f32_at(at + 12),
                z: f32_at(at + 16),
            },
            source: if source == 0 {
                MarkerSource::MapPin
            } else {
                MarkerSource::Waypoint
            },
            layer: match layer {
                0 => MarkerLayer::Unknown,
                1 => MarkerLayer::Mainland,
                _ => MarkerLayer::Alternate,
            },
            shape,
            color,
        });
    }
    Ok(Snapshot {
        pid,
        state,
        id: u64_at(16).to_string(),
        session: u64_at(8).to_string(),
        epoch: u64_at(24).to_string(),
        observed_ms: u64_at(32).to_string(),
        reason: u32_at(44),
        features: u32_at(48),
        identity: u32_at(52),
        world: u32_at(56),
        native_reason: u32_at(60),
        native_error: i32::from_le_bytes(bytes[64..68].try_into().unwrap()),
        storage_status: u32_at(68),
        bookmarks_revision: u64_at(72).to_string(),
        current,
        bookmarks: bookmarks
            .try_into()
            .map_err(|_| error("Invalid teleport bookmark count."))?,
        marker_status: u32_at(1524),
        markers_observed_ms: u64_at(1528).to_string(),
        markers,
    })
}
pub fn exchange(game: &Path, request: Option<&ControlRequest>) -> Result<Snapshot> {
    let wire = encode(request)?;
    #[cfg(windows)]
    {
        let pid = crate::spawn_service::running_game_pid(game)?;
        if request.is_some_and(|r| r.pid() != pid) {
            return Err(error(
                "The game instance changed. Read teleport status before another change.",
            ));
        }
        let reply = crate::native_pipe::transact::<REPLY_BYTES>(
            crate::native_pipe::PipeKind::Teleport,
            pid,
            &wire,
        )?;
        decode(&reply, pid)
    }
    #[cfg(target_os = "macos")]
    {
        let pid = crate::spawn_service::running_game_pid(game)?;
        if request.is_some_and(|r| r.pid() != pid) {
            return Err(error(
                "The game instance changed. Read teleport status before another change.",
            ));
        }
        decode(
            &crate::spawn_service::macos::teleport_transact(pid, &wire)?,
            pid,
        )
    }
    #[cfg(not(any(windows, target_os = "macos")))]
    {
        let _ = (game, wire);
        Err(error("Live teleport controls require Windows or macOS."))
    }
}

#[cfg(test)]
mod tests {
    use super::*;
    use serde_json::{Value, json};

    fn json_request(operation: &str) -> Value {
        json!({"operation":operation,"pid":123,"id":"9007199254740999","session":"18364758544493064720","epoch":"9007199254740993","not_after_ms":"18446744073709551615"})
    }
    fn request() -> ControlRequest {
        let mut value = json_request("save_current");
        value["slot"] = json!(15);
        value["name"] = json!("Äußere Brücke");
        serde_json::from_value(value).unwrap()
    }
    fn put32(bytes: &mut [u8], at: usize, value: u32) {
        bytes[at..at + 4].copy_from_slice(&value.to_le_bytes());
    }
    fn put64(bytes: &mut [u8], at: usize, value: u64) {
        bytes[at..at + 8].copy_from_slice(&value.to_le_bytes());
    }
    fn position(bytes: &mut [u8], at: usize) {
        put32(bytes, at, 1);
        bytes[at + 4..at + 6].copy_from_slice(&7u16.to_le_bytes());
        for (offset, value) in [(8, -1234.5f32), (12, 123.25), (16, 9876.0), (20, -1.5)] {
            put32(bytes, at + offset, value.to_bits());
        }
        put32(bytes, at + 24, 1);
    }
    fn response() -> [u8; REPLY_BYTES] {
        let mut bytes = [0; REPLY_BYTES];
        put32(&mut bytes, 0, MAGIC);
        bytes[4..6].copy_from_slice(&PROTOCOL.to_le_bytes());
        bytes[6..8].copy_from_slice(&(REPLY_BYTES as u16).to_le_bytes());
        bytes[84..86].copy_from_slice(&u16::MAX.to_le_bytes());
        put32(&mut bytes, 104, 1);
        bytes
    }
    fn marker(bytes: &mut [u8], index: usize, id: u64, source: u8) {
        let at = 1536 + index * 24;
        put64(bytes, at, id);
        bytes[at + 20] = source;
        bytes[at + 21] = 1;
        bytes[at + 22] = 13;
        bytes[at + 23] = 14;
    }

    #[test]
    fn request_layout_names_and_full_width_context_are_exact() {
        let r = request();
        let bytes = encode(Some(&r)).unwrap();
        assert_eq!(&bytes[..8], &[67, 87, 80, 84, 1, 0, 112, 0]);
        assert_eq!(r.pid(), 123);
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
        assert_eq!(bytes[40], 1);
        assert_eq!(bytes[44], 15);
        assert!(bytes[48..56].iter().all(|b| *b == 0));
        assert_eq!(
            &bytes[56..56 + "Äußere Brücke".len()],
            "Äußere Brücke".as_bytes()
        );
        assert!(bytes[104..].iter().all(|b| *b == 0));
        assert_eq!(
            serde_json::to_value(r).unwrap()["session"],
            "18364758544493064720"
        );
        let status = encode(None).unwrap();
        assert!(status[8..].iter().all(|b| *b == 0));
        for bad in [
            "",
            "01",
            "-1",
            "+1",
            "1.0",
            " 1",
            "1e2",
            "１",
            "18446744073709551616",
        ] {
            assert!(number(bad).is_err());
        }
        for field in ["id", "session", "epoch", "not_after_ms"] {
            let mut value = serde_json::to_value(request()).unwrap();
            value[field] = json!("0");
            assert!(encode(Some(&serde_json::from_value(value).unwrap())).is_err());
        }
        let mut value = serde_json::to_value(request()).unwrap();
        value["pid"] = json!(0);
        assert!(encode(Some(&serde_json::from_value(value).unwrap())).is_err());
    }

    #[test]
    fn operation_dtos_reject_foreign_payloads_and_marker_zero_is_valid() {
        for (operation, code) in [("remove_saved", 2), ("travel_saved", 3)] {
            let mut value = json_request(operation);
            value["slot"] = json!(0);
            let r: ControlRequest = serde_json::from_value(value.clone()).unwrap();
            let bytes = encode(Some(&r)).unwrap();
            assert_eq!(bytes[40], code);
            assert!(bytes[48..].iter().all(|b| *b == 0));
            for field in ["name", "marker", "marker_source", "position", "x"] {
                let mut extra = value.clone();
                extra[field] = Value::Null;
                assert!(serde_json::from_value::<ControlRequest>(extra).is_err());
            }
            value["slot"] = json!(16);
            assert!(encode(Some(&serde_json::from_value(value).unwrap())).is_err());
        }
        for source in ["map_pin", "waypoint"] {
            let mut value = json_request("travel_marker");
            value["marker"] = json!("0");
            value["marker_source"] = json!(source);
            let bytes = encode(Some(&serde_json::from_value(value.clone()).unwrap())).unwrap();
            assert_eq!(bytes[40], 4);
            assert_eq!(bytes[104], u8::from(source == "waypoint"));
            assert!(bytes[44..104].iter().all(|b| *b == 0));
            assert_eq!(&bytes[108..], &[0; 4]);
            for field in ["slot", "name", "position", "yaw", "x", "y", "z"] {
                let mut extra = value.clone();
                extra[field] = json!(1);
                assert!(serde_json::from_value::<ControlRequest>(extra).is_err());
            }
            value["marker"] = json!(u64::MAX.to_string());
            assert!(encode(Some(&serde_json::from_value(value.clone()).unwrap())).is_err());
            value["marker"] = json!("0");
            value["marker_source"] = json!("untrusted");
            assert!(serde_json::from_value::<ControlRequest>(value).is_err());
        }
        let mut value = serde_json::to_value(request()).unwrap();
        value["position"] = json!({"x":1,"y":2,"z":3});
        assert!(serde_json::from_value::<ControlRequest>(value).is_err());
    }

    #[test]
    fn names_are_bounded_utf8_bytes_without_controls_and_are_nul_terminated() {
        for name in [
            "a".repeat(47),
            format!("{}a", "ä".repeat(23)),
            format!("{}ab", "界".repeat(15)),
        ] {
            assert_eq!(name.len(), 47);
            let mut value = serde_json::to_value(request()).unwrap();
            value["name"] = json!(name);
            let bytes = encode(Some(&serde_json::from_value(value).unwrap())).unwrap();
            assert_eq!(bytes[103], 0);
            assert_eq!(decode_name(&bytes[56..104], true).unwrap().len(), 47);
        }
        for name in [
            String::new(),
            "a".repeat(48),
            "ä".repeat(24),
            "a\0b".into(),
            "a\nb".into(),
            "a\tb".into(),
            "a\u{7f}b".into(),
            "a\u{85}b".into(),
        ] {
            let mut value = serde_json::to_value(request()).unwrap();
            value["name"] = json!(name);
            assert!(encode(Some(&serde_json::from_value(value).unwrap())).is_err());
        }
    }

    #[test]
    fn native_placeholder_and_all_declared_states_reasons_are_accepted() {
        let mut bytes = response();
        for state in 0..10 {
            put32(&mut bytes, 40, state);
            for reason in 0..22 {
                put32(&mut bytes, 44, reason);
                let s = decode(&bytes, 123).unwrap();
                assert!(s.current.is_none());
                assert_eq!(s.bookmarks.len(), 16);
                assert!(
                    s.bookmarks
                        .iter()
                        .all(|b| !b.occupied && b.name.is_empty() && b.position.is_none())
                );
                assert!(s.markers.is_empty());
                assert_eq!(s.session, "0");
                assert_eq!(s.epoch, "0");
            }
        }
        for code in 0..12 {
            put32(&mut bytes, 1524, code);
            assert!(decode(&bytes, 123).is_ok());
        }
    }

    #[test]
    fn full_response_keeps_u64_strings_positions_all_slots_and_native_markers() {
        let mut bytes = response();
        for at in [8, 16, 24, 32, 72, 1528] {
            put64(&mut bytes, at, u64::MAX);
        }
        put32(&mut bytes, 40, 6);
        put32(&mut bytes, 48, 7);
        put32(&mut bytes, 64, i32::MIN as u32);
        put32(&mut bytes, 68, 2);
        position(&mut bytes, 80);
        for slot in 0..16 {
            let at = 112 + slot * 88;
            position(&mut bytes, at);
            let name = format!("Äußere Brücke {slot}");
            bytes[at + 32..at + 32 + name.len()].copy_from_slice(name.as_bytes());
            put32(&mut bytes, at + 80, 1);
        }
        put32(&mut bytes, 1520, 3);
        marker(&mut bytes, 0, 0, 0);
        marker(&mut bytes, 1, 0, 1);
        marker(&mut bytes, 2, u64::MAX - 1, 0);
        bytes[1536 + 21] = 0;
        bytes[1536 + 48 + 21] = 2;
        let snapshot = decode(&bytes, 123).unwrap();
        assert_eq!(snapshot.native_error, i32::MIN);
        assert_eq!(snapshot.state, "position_observed");
        assert_eq!(snapshot.current.as_ref().unwrap().position.x, -1234.5);
        let json = serde_json::to_value(snapshot).unwrap();
        for field in [
            "id",
            "session",
            "epoch",
            "observed_ms",
            "bookmarks_revision",
            "markers_observed_ms",
        ] {
            assert_eq!(json[field], u64::MAX.to_string());
        }
        assert_eq!(json["bookmarks"][15]["name"], "Äußere Brücke 15");
        assert_eq!(json["markers"][0]["id"], "0");
        assert_eq!(json["markers"][0]["source"], "map_pin");
        assert_eq!(json["markers"][1]["source"], "waypoint");
        assert_eq!(json["markers"][0]["layer"], "unknown");
        assert_eq!(json["markers"][2]["layer"], "alternate");
        assert_eq!(json["markers"][2]["id"], (u64::MAX - 1).to_string());
    }

    #[test]
    fn invalid_complete_responses_are_rejected_without_value_repair() {
        let base = response();
        for (at, value) in [
            (0, 0u32),
            (40, 10),
            (44, 22),
            (48, 8),
            (60, 14),
            (68, 3),
            (1520, 521),
            (1524, 12),
            (112 + 80, 2),
            (112 + 84, 1),
        ] {
            let mut bytes = base;
            put32(&mut bytes, at, value);
            assert!(decode(&bytes, 123).is_err(), "offset {at}");
        }
        for at in [4, 6] {
            let mut bytes = base;
            bytes[at] ^= 1;
            assert!(decode(&bytes, 123).is_err());
        }
        for (at, value) in [
            (80, 0u32),
            (86, 1),
            (104, 2),
            (108, 1),
            (88, f32::NAN.to_bits()),
            (92, f32::INFINITY.to_bits()),
            (100, f32::NEG_INFINITY.to_bits()),
        ] {
            let mut bytes = base;
            position(&mut bytes, 80);
            put32(&mut bytes, at, value);
            assert!(decode(&bytes, 123).is_err(), "position {at}");
        }
        for bad in 0..5 {
            let mut bytes = base;
            position(&mut bytes, 112);
            put32(&mut bytes, 192, 1);
            bytes[144] = b'A';
            match bad {
                0 => bytes[144..192].fill(b'A'),
                1 => bytes[144] = 0xff,
                2 => bytes[146] = b'X',
                3 => bytes[144] = 0,
                4 => put32(&mut bytes, 120, f32::NAN.to_bits()),
                _ => unreachable!(),
            };
            assert!(decode(&bytes, 123).is_err());
        }
        // Native restore ignores position bytes of an unoccupied bookmark.
        let mut bytes = base;
        put32(&mut bytes, 120, f32::NAN.to_bits());
        assert!(decode(&bytes, 123).unwrap().bookmarks[0].position.is_none());
    }

    #[test]
    fn marker_row_validation_checks_native_bounds_and_composite_identity() {
        let mut bytes = response();
        put32(&mut bytes, 1520, 520);
        for row in 0..520 {
            marker(&mut bytes, row, (row % 500) as u64, u8::from(row >= 500));
        }
        assert_eq!(decode(&bytes, 123).unwrap().markers.len(), 520);
        let mut too_many = bytes;
        too_many[1536 + 500 * 24 + 20] = 0;
        put64(&mut too_many, 1536 + 500 * 24, 999);
        assert!(decode(&too_many, 123).is_err());
        let mut duplicate = bytes;
        put64(&mut duplicate, 1536 + 24, 0);
        assert!(decode(&duplicate, 123).is_err());
        for (offset, value) in [(20, 2), (21, 3), (22, 14), (23, 15)] {
            let mut invalid = bytes;
            invalid[1536 + offset] = value;
            assert!(decode(&invalid, 123).is_err());
        }
        for value in [f32::NAN, f32::INFINITY, f32::NEG_INFINITY] {
            let mut invalid = bytes;
            put32(&mut invalid, 1544, value.to_bits());
            assert!(decode(&invalid, 123).is_err());
        }
        let mut invalid = bytes;
        put64(&mut invalid, 1536, u64::MAX);
        assert!(decode(&invalid, 123).is_err());
        let mut waypoints = response();
        put32(&mut waypoints, 1520, 21);
        for row in 0..21 {
            marker(&mut waypoints, row, row as u64, 1);
        }
        assert!(decode(&waypoints, 123).is_err());
    }

    #[cfg(windows)]
    #[test]
    #[ignore = "Uses only the exact owned teleport pipe host; never connects to a game"]
    fn real_windows_teleport_pipe() {
        use std::os::windows::process::CommandExt;
        struct Host(std::process::Child);
        impl Drop for Host {
            fn drop(&mut self) {
                let _ = self.0.kill();
                let _ = self.0.wait();
            }
        }
        let workspace = Path::new(env!("CARGO_MANIFEST_DIR"))
            .join("../..")
            .canonicalize()
            .unwrap();
        let host = workspace
            .join(".local/teleport-pipe-build/Release/marketplace-teleport-pipe-host.exe")
            .canonicalize()
            .expect("Build the owned teleport pipe host first");
        assert_eq!(
            host.file_name().unwrap(),
            "marketplace-teleport-pipe-host.exe"
        );
        let mut child = Host(
            std::process::Command::new(host)
                .creation_flags(0x08000000)
                .spawn()
                .unwrap(),
        );
        let pid = child.0.id();
        let transact = |wire: &[u8]| {
            crate::native_pipe::transact::<REPLY_BYTES>(
                crate::native_pipe::PipeKind::Teleport,
                pid,
                wire,
            )
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
        let ready = ready.expect("owned teleport host ready");
        assert_eq!(ready.pid, pid);
        assert_eq!(ready.state, "ready");
        assert_eq!(ready.session, "18364758544493064720");
        assert_eq!(ready.epoch, "9007199254740993");
        assert_eq!(ready.bookmarks_revision, "9007199254740995");
        assert_eq!(ready.features, 7);
        assert_eq!(ready.bookmarks[0].name, "Camp café");
        assert_eq!(ready.bookmarks.len(), 16);
        assert_eq!(ready.markers.len(), 2);
        assert_eq!(ready.markers[0].id, "0");
        assert_eq!(ready.markers[1].id, "0");
        assert!(transact(&[0; 111]).is_err());
        assert!(transact(&[0; 113]).is_err());
        let mut malformed = encode(None).unwrap();
        malformed[0] ^= 1;
        let rejected = decode(&transact(&malformed).unwrap(), pid).unwrap();
        assert_eq!(rejected.state, "rejected");
        assert_eq!(rejected.reason, 1);
        let mut value = json_request("save_current");
        value["pid"] = json!(pid);
        value["slot"] = json!(15);
        let name = format!("{}ab", "界".repeat(15));
        value["name"] = json!(name);
        let r: ControlRequest = serde_json::from_value(value).unwrap();
        let captured = decode(&transact(&encode(Some(&r)).unwrap()).unwrap(), pid).unwrap();
        assert_eq!(captured.state, "captured");
        assert_eq!(captured.id, r.context().id);
        assert_eq!(captured.bookmarks[15].name, name);
        assert_eq!(
            captured.bookmarks[15].position.as_ref().unwrap().position.y,
            20.0
        );
        for (source, x, id) in [
            ("map_pin", 100.0, "9007199254741000"),
            ("waypoint", 300.0, "9007199254741001"),
        ] {
            let mut value = json_request("travel_marker");
            value["pid"] = json!(pid);
            value["id"] = json!(id);
            value["marker"] = json!("0");
            value["marker_source"] = json!(source);
            let r: ControlRequest = serde_json::from_value(value).unwrap();
            let moved = decode(&transact(&encode(Some(&r)).unwrap()).unwrap(), pid).unwrap();
            assert_eq!(moved.state, "position_observed");
            assert_eq!(moved.id, id);
            assert_eq!(moved.current.as_ref().unwrap().position.x, x);
            assert_eq!(moved.current.as_ref().unwrap().position.y, 42.0);
        }
        let mut value = json_request("travel_saved");
        value["pid"] = json!(pid);
        value["id"] = json!("9007199254741002");
        value["slot"] = json!(15);
        let r: ControlRequest = serde_json::from_value(value.clone()).unwrap();
        let moved = decode(&transact(&encode(Some(&r)).unwrap()).unwrap(), pid).unwrap();
        assert_eq!(moved.state, "position_observed");
        assert_eq!(moved.current.as_ref().unwrap().position.x, 10.0);
        assert_eq!(moved.current.as_ref().unwrap().position.y, 20.0);
        value["operation"] = json!("remove_saved");
        value["id"] = json!("9007199254741003");
        let r: ControlRequest = serde_json::from_value(value).unwrap();
        let removed = decode(&transact(&encode(Some(&r)).unwrap()).unwrap(), pid).unwrap();
        assert_eq!(removed.state, "removed");
        assert_eq!(removed.id, r.context().id);
        assert!(!removed.bookmarks[15].occupied);
        assert!(removed.bookmarks[15].position.is_none());
        assert_eq!(removed.bookmarks_revision, "9007199254740997");
        assert_eq!(
            decode(&transact(&encode(None).unwrap()).unwrap(), pid)
                .unwrap()
                .id,
            r.context().id
        );
    }
}
