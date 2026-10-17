use crate::service::{AppError, Result};
use serde::{Deserialize, Serialize};
use std::path::Path;

const MAGIC: u32 = 0x31575743;
const VERSION: u32 = 1;
#[derive(Clone, Copy, Deserialize, Serialize)]
#[serde(rename_all = "snake_case")]
pub enum Operation {
    ChooseTime,
    FreezeTime,
    ReleaseTime,
    ChooseWeather,
    FreezeWeather,
    ReleaseWeather,
    AdvanceTime,
}
impl Operation {
    fn wire(self) -> u32 {
        match self {
            Self::ChooseTime => 1,
            Self::FreezeTime => 2,
            Self::ReleaseTime => 3,
            Self::ChooseWeather => 4,
            Self::FreezeWeather => 5,
            Self::ReleaseWeather => 6,
            Self::AdvanceTime => 7,
        }
    }
}
#[derive(Clone, Copy, Deserialize, Serialize)]
#[serde(deny_unknown_fields)]
pub struct TimeChoice {
    pub hour: u32,
    pub minute: u32,
    pub second: u32,
    pub millisecond: u32,
}
#[derive(Clone, Copy, Deserialize, Serialize)]
#[serde(deny_unknown_fields)]
pub struct Weather {
    pub precipitation: f32,
    pub cloudiness: f32,
    pub humidity: f32,
    pub wind_speed: f32,
    pub height_fog: f32,
    pub phase: u32,
}
impl Weather {
    fn values(self) -> [f32; 5] {
        [
            self.precipitation,
            self.cloudiness,
            self.humidity,
            self.wind_speed,
            self.height_fog,
        ]
    }
    fn valid_choice(self) -> bool {
        self.values().iter().all(|v| v.is_finite())
            && (0.0..=1.0).contains(&self.precipitation)
            && (-1.0..=1.0).contains(&self.cloudiness)
            && (0.0..=1.0).contains(&self.humidity)
            && (0.0..=60.0).contains(&self.wind_speed)
            && (0.0..=10.0).contains(&self.height_fog)
            && self.phase <= 1
    }
}
#[derive(Clone, Deserialize, Serialize)]
#[serde(deny_unknown_fields)]
pub struct ControlRequest {
    pub pid: u32,
    // Native u64 values cross JSON as canonical decimal strings, including
    // the random session, which can exceed JavaScript's exact integer range.
    pub id: String,
    pub session: String,
    pub generation: String,
    pub not_after_ms: String,
    pub operation: Operation,
    pub time: Option<TimeChoice>,
    pub weather: Option<Weather>,
    pub enabled: Option<bool>,
    pub hours: Option<u32>,
}
#[derive(Serialize, Debug)]
pub struct Clock {
    pub day: u32,
    pub hour: u32,
    pub minute: u32,
    pub second: u32,
    pub millisecond: u32,
    pub weekday: u8,
    pub rate: u16,
    pub anchor: String,
}
#[derive(Serialize)]
pub struct Snapshot {
    pub pid: u32,
    pub state: &'static str,
    pub id: String,
    pub session: String,
    pub generation: String,
    pub monotonic_ms: String,
    pub reason: u32,
    pub flags: u32,
    pub identity: u32,
    pub world: u32,
    pub stage: u32,
    pub clock_reason: u32,
    pub weather_reason: u32,
    pub time: Clock,
    pub weather: Weather,
    pub time_receipt: String,
    pub weather_receipt: String,
    pub time_result: &'static str,
    pub time_result_reason: u32,
    pub weather_result: &'static str,
    pub weather_result_reason: u32,
}
fn error(message: impl Into<String>) -> AppError {
    AppError {
        code: "world_controls",
        message: message.into(),
    }
}
fn number(text: &str) -> Result<u64> {
    if text.is_empty()
        || text.len() > 20
        || !text.bytes().all(|b| b.is_ascii_digit())
        || (text.len() > 1 && text.starts_with('0'))
    {
        return Err(error("Invalid world-control identifier."));
    }
    text.parse()
        .map_err(|_| error("World-control identifier is out of range."))
}
fn encode(request: Option<&ControlRequest>) -> Result<[u8; 96]> {
    let mut out = [0; 96];
    out[0..4].copy_from_slice(&MAGIC.to_le_bytes());
    out[4..8].copy_from_slice(&VERSION.to_le_bytes());
    out[8..12].copy_from_slice(&96u32.to_le_bytes());
    let Some(r) = request else { return Ok(out) };
    if r.pid == 0 {
        return Err(error("Missing game instance."));
    }
    out[12..16].copy_from_slice(&r.operation.wire().to_le_bytes());
    for (offset, text) in [
        (16, &r.id),
        (24, &r.session),
        (32, &r.generation),
        (40, &r.not_after_ms),
    ] {
        let value = number(text)?;
        if value == 0 {
            return Err(error("World-control identifiers must not be zero."));
        }
        out[offset..offset + 8].copy_from_slice(&value.to_le_bytes());
    }
    match (r.operation, r.time, r.weather, r.enabled, r.hours) {
        (Operation::ChooseTime, Some(t), None, None, None)
            if t.hour < 24 && t.minute < 60 && t.second < 60 && t.millisecond < 1000 =>
        {
            for (offset, value) in [
                (48, t.hour),
                (52, t.minute),
                (56, t.second),
                (60, t.millisecond),
            ] {
                out[offset..offset + 4].copy_from_slice(&value.to_le_bytes());
            }
        }
        (Operation::ChooseWeather, None, Some(w), None, None) if w.valid_choice() => {
            for (i, value) in w.values().iter().enumerate() {
                out[64 + i * 4..68 + i * 4].copy_from_slice(&value.to_le_bytes());
            }
            out[84..88].copy_from_slice(&w.phase.to_le_bytes());
        }
        (Operation::AdvanceTime, None, None, None, Some(hours)) if (1..=10000).contains(&hours) => {
            // Operation 7 uses the first word of the 16-byte time payload as
            // displayed game hours. The remaining payload stays zero.
            out[48..52].copy_from_slice(&hours.to_le_bytes());
        }
        (Operation::FreezeTime | Operation::FreezeWeather, None, None, Some(enabled), None) => {
            out[88..92].copy_from_slice(&u32::from(enabled).to_le_bytes());
        }
        (Operation::ReleaseTime | Operation::ReleaseWeather, None, None, None, None) => {}
        _ => return Err(error("Invalid time or weather control values.")),
    }
    Ok(out)
}
#[cfg(any(windows, test))]
fn decode(bytes: &[u8; 176], pid: u32) -> Result<Snapshot> {
    let u16_at = |n| u16::from_le_bytes(bytes[n..n + 2].try_into().unwrap());
    let u32_at = |n| u32::from_le_bytes(bytes[n..n + 4].try_into().unwrap());
    let u64_at = |n| u64::from_le_bytes(bytes[n..n + 8].try_into().unwrap());
    let f32_at = |n| f32::from_le_bytes(bytes[n..n + 4].try_into().unwrap());
    if u32_at(0) != MAGIC
        || u32_at(4) != VERSION
        || u32_at(8) != 176
        || u32_at(52) & !63 != 0
        || u32_at(48) > 15
        || u32_at(76) != 0
        || u64_at(152) != 0
        || u32_at(164) > 15
        || u32_at(172) > 15
    {
        return Err(error("Invalid response from the world-control module."));
    }
    let state_at = |offset| {
        [
            "unavailable",
            "ready",
            "queued",
            "published",
            "rejected",
            "expired",
            "world_changed",
            "uncertain",
            "fault_latched",
            "released",
        ]
        .get(u32_at(offset) as usize)
        .copied()
        .ok_or_else(|| error("Unknown world-control status."))
    };
    let state = state_at(12)?;
    let time = Clock {
        day: u32_at(80),
        hour: u32_at(84),
        minute: u32_at(88),
        second: u32_at(92),
        millisecond: u32_at(96),
        weekday: bytes[100],
        rate: u16_at(102),
        anchor: u64_at(104).to_string(),
    };
    let weather = Weather {
        precipitation: f32_at(112),
        cloudiness: f32_at(116),
        humidity: f32_at(120),
        wind_speed: f32_at(124),
        height_fog: f32_at(128),
        phase: u32_at(132),
    };
    // A natural weather value can be outside the conservative edit range.
    // Reject nonfinite values; do not reinterpret or clamp reported values.
    if time.hour >= 24
        || time.minute >= 60
        || time.second >= 60
        || time.millisecond >= 1000
        || time.weekday > 6
        || !weather.values().iter().all(|v| v.is_finite())
        || weather.phase > 1
    {
        return Err(error(
            "Invalid clock or weather values from the live module.",
        ));
    }
    Ok(Snapshot {
        pid,
        state,
        id: u64_at(16).to_string(),
        session: u64_at(24).to_string(),
        generation: u64_at(32).to_string(),
        monotonic_ms: u64_at(40).to_string(),
        reason: u32_at(48),
        flags: u32_at(52),
        identity: u32_at(56),
        world: u32_at(60),
        stage: u32_at(64),
        clock_reason: u32_at(68),
        weather_reason: u32_at(72),
        time,
        weather,
        time_receipt: u64_at(136).to_string(),
        weather_receipt: u64_at(144).to_string(),
        time_result: state_at(160)?,
        time_result_reason: u32_at(164),
        weather_result: state_at(168)?,
        weather_result_reason: u32_at(172),
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
        decode(&windows::transact(pid, &wire)?, pid)
    }
    #[cfg(not(windows))]
    {
        let _ = (game, wire);
        Err(error("Live time and weather controls require Windows."))
    }
}
#[cfg(windows)]
mod windows {
    use super::*;
    pub fn transact(pid: u32, request: &[u8]) -> Result<[u8; 176]> {
        crate::native_pipe::transact::<176>(crate::native_pipe::PipeKind::World, pid, request)
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
            generation: "9007199254740993".into(),
            not_after_ms: "4294967296000".into(),
            operation: Operation::ChooseTime,
            time: Some(TimeChoice {
                hour: 23,
                minute: 59,
                second: 58,
                millisecond: 999,
            }),
            weather: None,
            enabled: None,
            hours: None,
        }
    }
    #[test]
    fn request_layout_is_exact_and_identifiers_are_lossless() {
        let r = request();
        let bytes = encode(Some(&r)).unwrap();
        assert_eq!(
            &bytes[..16],
            &[67, 87, 87, 49, 1, 0, 0, 0, 96, 0, 0, 0, 1, 0, 0, 0]
        );
        assert_eq!(
            u64::from_le_bytes(bytes[16..24].try_into().unwrap()),
            9_007_199_254_740_999
        );
        assert_eq!(
            u64::from_le_bytes(bytes[24..32].try_into().unwrap()),
            0xfedcba9876543210
        );
        assert_eq!(
            &bytes[48..64],
            &[23, 0, 0, 0, 59, 0, 0, 0, 58, 0, 0, 0, 231, 3, 0, 0]
        );
        assert!(bytes[64..].iter().all(|b| *b == 0));
        assert!(encode(None).unwrap()[12..].iter().all(|b| *b == 0));
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
        assert_eq!(number("18446744073709551615").unwrap(), u64::MAX);
        let json = serde_json::to_value(&r).unwrap();
        assert_eq!(json["session"], "18364758544493064720");
    }
    #[test]
    fn only_the_payload_for_the_selected_operation_is_accepted() {
        let mut r = request();
        r.time.as_mut().unwrap().hour = 24;
        assert!(encode(Some(&r)).is_err());
        r = request();
        r.enabled = Some(true);
        assert!(encode(Some(&r)).is_err());
        r = request();
        r.operation = Operation::FreezeTime;
        assert!(encode(Some(&r)).is_err());
        r.time = None;
        r.enabled = Some(true);
        assert_eq!(encode(Some(&r)).unwrap()[88], 1);
        r.operation = Operation::ReleaseTime;
        assert!(encode(Some(&r)).is_err());
        r.enabled = None;
        assert!(encode(Some(&r)).is_ok());
        r.operation = Operation::ChooseWeather;
        r.weather = Some(Weather {
            precipitation: 1.0,
            cloudiness: -1.0,
            humidity: 0.0,
            wind_speed: 60.0,
            height_fog: 10.0,
            phase: 1,
        });
        let bytes = encode(Some(&r)).unwrap();
        assert_eq!(bytes[12], 4);
        assert_eq!(bytes[84], 1);
        r.weather.as_mut().unwrap().precipitation = f32::NAN;
        assert!(encode(Some(&r)).is_err());
        r.weather.as_mut().unwrap().precipitation = 1.01;
        assert!(encode(Some(&r)).is_err());
        r.weather.as_mut().unwrap().precipitation = 0.0;
        r.weather.as_mut().unwrap().phase = 2;
        assert!(encode(Some(&r)).is_err());
    }
    #[test]
    fn advance_hours_has_a_distinct_bounded_payload() {
        let mut r = request();
        r.operation = Operation::AdvanceTime;
        r.time = None;
        for hours in [1, 6, 24, 48, 10000] {
            r.hours = Some(hours);
            let bytes = encode(Some(&r)).unwrap();
            assert_eq!(bytes[12], 7);
            assert_eq!(u32::from_le_bytes(bytes[48..52].try_into().unwrap()), hours);
            assert!(bytes[52..].iter().all(|value| *value == 0));
        }
        for hours in [0, 10001, u32::MAX] {
            r.hours = Some(hours);
            assert!(encode(Some(&r)).is_err());
        }
        r.hours = Some(24);
        r.time = request().time;
        assert!(encode(Some(&r)).is_err());
        r.time = None;
        r.enabled = Some(false);
        assert!(encode(Some(&r)).is_err());
        r.enabled = None;
        r.operation = Operation::ReleaseTime;
        assert!(encode(Some(&r)).is_err());
        r.operation = Operation::ChooseWeather;
        assert!(encode(Some(&r)).is_err());
    }
    #[test]
    fn response_validation_and_receipts() {
        let mut bytes = [0; 176];
        for (offset, value) in [
            (0, MAGIC),
            (4, VERSION),
            (8, 176),
            (12, 3),
            (52, 5),
            (84, 23),
            (88, 59),
            (92, 58),
            (96, 999),
            (160, 3),
            (168, 9),
        ] {
            bytes[offset..offset + 4].copy_from_slice(&value.to_le_bytes());
        }
        for offset in [16, 24, 32, 40, 104, 136, 144] {
            bytes[offset..offset + 8].copy_from_slice(&u64::MAX.to_le_bytes());
        }
        bytes[100] = 6;
        bytes[101] = 0xee; // Native padding is not a semantic field.
        let snapshot = decode(&bytes, 123).unwrap();
        assert_eq!(snapshot.state, "published");
        assert_eq!(snapshot.id, u64::MAX.to_string());
        assert_eq!(snapshot.time_receipt, u64::MAX.to_string());
        assert_eq!(snapshot.time_result, "published");
        assert_eq!(snapshot.weather_result, "released");
        for offset in [0, 4, 8, 76, 152] {
            let old = bytes[offset];
            bytes[offset] ^= 1;
            assert!(decode(&bytes, 123).is_err());
            bytes[offset] = old;
        }
        bytes[52] = 64;
        assert!(decode(&bytes, 123).is_err());
        bytes[52] = 5;
        bytes[12] = 10;
        assert!(decode(&bytes, 123).is_err());
        bytes[12] = 3;
        for offset in [160, 168] {
            bytes[offset] = 10;
            assert!(decode(&bytes, 123).is_err());
            bytes[offset] = 3;
        }
        for offset in [164, 172] {
            bytes[offset] = 16;
            assert!(decode(&bytes, 123).is_err());
            bytes[offset] = 0;
        }
        bytes[112..116].copy_from_slice(&f32::NAN.to_le_bytes());
        assert!(decode(&bytes, 123).is_err());
    }
    #[cfg(windows)]
    #[test]
    #[ignore = "Uses only the owned C++ pipe host via CD_WORLD_PIPE_HOST; never connects to a game"]
    fn real_windows_pipe() {
        use std::os::windows::process::CommandExt;
        struct Host(std::process::Child);
        impl Drop for Host {
            fn drop(&mut self) {
                let _ = self.0.kill();
                let _ = self.0.wait();
            }
        }
        let host = std::path::PathBuf::from(
            std::env::var_os("CD_WORLD_PIPE_HOST").expect("owned host path"),
        );
        assert_eq!(host.file_name().unwrap(), "marketplace-world-pipe-host.exe");
        let workspace = std::path::Path::new(env!("CARGO_MANIFEST_DIR"))
            .join("../..")
            .canonicalize()
            .unwrap();
        assert_eq!(
            host.canonicalize().unwrap(),
            workspace
                .join(".local/world-controls-pipe-build/Release/marketplace-world-pipe-host.exe")
                .canonicalize()
                .unwrap()
        );
        let mut child = Host(
            std::process::Command::new(host)
                .creation_flags(0x08000000)
                .spawn()
                .unwrap(),
        );
        let pid = child.0.id();
        let mut ready = None;
        for _ in 0..100 {
            if let Ok(bytes) = windows::transact(pid, &encode(None).unwrap()) {
                ready = Some(decode(&bytes, pid).unwrap());
                break;
            }
            assert!(child.0.try_wait().unwrap().is_none());
            std::thread::sleep(std::time::Duration::from_millis(20));
        }
        let ready = ready.expect("host ready");
        assert_eq!(ready.state, "ready");
        assert_eq!(ready.session, "18364758544493064720");
        assert_eq!(ready.generation, "9007199254740993");
        assert_eq!(ready.time.anchor, "1311768467463790320");
        // The native worker must reject message boundaries before calling its
        // handler, then accept a fresh valid connection without a retry write.
        assert!(windows::transact(pid, &[0; 80]).is_err());
        assert!(windows::transact(pid, &[0; 97]).is_err());
        let mut invalid = encode(None).unwrap();
        invalid[0] ^= 1;
        let rejected = decode(&windows::transact(pid, &invalid).unwrap(), pid).unwrap();
        assert_eq!(rejected.state, "rejected");
        assert_eq!(rejected.reason, 1);
        let mut r = request();
        r.pid = pid;
        let reply = decode(
            &windows::transact(pid, &encode(Some(&r)).unwrap()).unwrap(),
            pid,
        )
        .unwrap();
        assert_eq!(reply.id, r.id);
        assert_eq!(reply.time_receipt, r.id);
        assert_eq!(reply.time.hour, 23);
        r.time = None;
        r.operation = Operation::AdvanceTime;
        r.hours = Some(48);
        let advanced = decode(
            &windows::transact(pid, &encode(Some(&r)).unwrap()).unwrap(),
            pid,
        )
        .unwrap();
        assert_eq!(advanced.time.day, 989);
        assert_eq!(advanced.time.hour, 18);
        assert_eq!(advanced.time.minute, 45);
        assert_eq!(advanced.time_receipt, r.id);
        r.hours = None;
        r.operation = Operation::ChooseWeather;
        r.weather = Some(Weather {
            precipitation: 0.5,
            cloudiness: -0.25,
            humidity: 0.75,
            wind_speed: 12.0,
            height_fog: 3.0,
            phase: 0,
        });
        let reply = decode(
            &windows::transact(pid, &encode(Some(&r)).unwrap()).unwrap(),
            pid,
        )
        .unwrap();
        assert_eq!(reply.weather_receipt, r.id);
        assert_eq!(reply.weather.wind_speed, 12.0);
    }
}
