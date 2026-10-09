//! Ten native additional sockets for individually selected equipment UIDs.
//! Game saves are never edited here; a separate checked file holds the extras.
use crate::{Error, GameData, Result, fingerprint::hash_bytes};
use serde::{Deserialize, Serialize};
use sha2::{Digest, Sha256};
use std::{
    fs::{self, File, OpenOptions},
    io::{Read, Seek, SeekFrom, Write},
    path::{Path, PathBuf},
    time::{SystemTime, UNIX_EPOCH},
};
const LENGTH: usize = 48 + 256 * 76;
const EXE: &str = "57da440d72f4db974f25fef047cf84c4dadd999a88cb2a3c5af4c9bd67fde1e7";
const ASI: &str = "a624d59ac51dc4cf9cd85d7da7a02e78a54e60e9f91a480290d841418e96ec42";
// Compiled into each release after building its paired native modules. Never
// trust a digest read from an installed/modifiable manifest for admission.
const RELEASE_ASI: Option<&str> = option_env!("CRIMSON_EXTRA_SOCKETS_SHA256");
const TARGETS: [(u64, u32, u8); 3] = [
    (1001416, 111005, 0),
    (1003059, 1000380, 1),
    (1003664, 1001062, 5),
];
#[derive(Clone, Debug, PartialEq, Serialize, Deserialize)]
#[serde(deny_unknown_fields)]
pub struct Gem {
    pub key: u32,
    pub endurance: u16,
}
#[derive(Clone, Serialize)]
pub struct OccupiedGem {
    pub key: u32,
    pub endurance: u16,
    pub name: Option<String>,
}
#[derive(Clone, Serialize)]
pub struct Record {
    pub uid: String,
    pub item_key: u32,
    pub name: String,
    pub baseline: u8,
    pub additional: u8,
    pub gems: Vec<OccupiedGem>,
}
#[derive(Serialize)]
pub struct Snapshot {
    pub revision: String,
    pub game_running: bool,
    pub records: Vec<Record>,
    pub runtime_log: String,
}
#[derive(Deserialize)]
#[serde(deny_unknown_fields)]
pub struct Request {
    pub revision: String,
    pub uid: String,
    pub index: u8,
    pub gem_key: u32,
}
#[derive(Serialize)]
pub struct Receipt {
    pub backup: PathBuf,
    pub snapshot: Snapshot,
}
#[derive(Clone, Debug)]
struct Setting {
    uid: u64,
    key: u32,
    base: u8,
    gems: Vec<Gem>,
}
pub(crate) fn gear_keys() -> Vec<u32> {
    serde_json::from_str(include_str!("../schemas/extra-sockets-gear-2976.json"))
        .expect("checked equipment schema")
}
fn bad(s: impl Into<String>) -> Error {
    Error::Invalid(s.into())
}
fn allowed() -> Vec<Gem> {
    serde_json::from_str(include_str!("../schemas/extra-sockets-gems-2976.json"))
        .expect("checked gem schema")
}
fn decode(raw: &[u8]) -> Result<Vec<Setting>> {
    if raw.len() < 48
        || raw.len() > LENGTH
        || &raw[..8] != b"CWEXSOCK"
        || Sha256::digest(&raw[48..]).as_slice() != &raw[16..48]
    {
        return Err(bad(
            "Zusatzsockel-Datei fehlt, ist beschädigt oder hat eine unbekannte Version.",
        ));
    }
    let version = u32::from_le_bytes(raw[8..12].try_into().unwrap());
    let count = u32::from_le_bytes(raw[12..16].try_into().unwrap()) as usize;
    if !matches!(version, 1 | 2)
        || count > 256
        || raw.len() != 48 + count * 76
        || (version == 1 && count != 3)
    {
        return Err(bad("Unbekannte Sockelversion oder ungültige Anzahl."));
    }
    let allow = allowed();
    let gear = gear_keys();
    let mut seen = std::collections::BTreeSet::new();
    let mut records = Vec::new();
    for i in 0..count {
        let at = 48 + i * 76;
        let uid = u64::from_le_bytes(raw[at..at + 8].try_into().unwrap());
        let key = u32::from_le_bytes(raw[at + 8..at + 12].try_into().unwrap());
        let base = raw[at + 12];
        if uid == 0
            || uid == u64::MAX
            || !seen.insert(uid)
            || !gear.contains(&key)
            || base > 5
            || raw[at + 13..at + 16] != [10, 0, 0]
            || (version == 1 && TARGETS.get(i) != Some(&(uid, key, base)))
        {
            return Err(bad(
                "Ungültige oder doppelte Gegenstandszuordnung für Zusatzsockel.",
            ));
        }
        let mut gems = Vec::new();
        for n in 0..10 {
            let p = at + 16 + n * 6;
            let g = Gem {
                key: u32::from_le_bytes(raw[p..p + 4].try_into().unwrap()),
                endurance: u16::from_le_bytes(raw[p + 4..p + 6].try_into().unwrap()),
            };
            if (g.key == 0 && g.endurance != 0)
                || (g.key != 0
                    && !allow
                        .iter()
                        .any(|a| a.key == g.key && g.endurance <= a.endurance))
            {
                return Err(bad("Ungültiger Sockelstein in Zusatzsockel-Datei."));
            }
            gems.push(g);
        }
        records.push(Setting {
            uid,
            key,
            base,
            gems,
        });
    }
    Ok(records)
}
fn change(raw: &[u8], request: &Request) -> Result<Vec<u8>> {
    let records = decode(raw)?;
    if hash_bytes(raw) != request.revision {
        return Err(bad(
            "Die Sockel wurden zwischenzeitlich geändert. Bitte neu einlesen.",
        ));
    }
    let target = records
        .iter()
        .position(|t| t.uid.to_string() == request.uid)
        .ok_or_else(|| bad("Unbekannter Gegenstand."))?;
    if request.index >= 10 {
        return Err(bad("Zusatzsockel muss zwischen 1 und 10 liegen."));
    }
    let endurance = if request.gem_key == 0 {
        0
    } else {
        allowed()
            .iter()
            .find(|a| a.key == request.gem_key)
            .ok_or_else(|| bad("Dieses Item ist kein freigegebener Abyss-Sockelstein."))?
            .endurance
    };
    let mut out = raw.to_vec();
    let at = 48 + target * 76 + 16 + request.index as usize * 6;
    out[at..at + 4].copy_from_slice(&request.gem_key.to_le_bytes());
    out[at + 4..at + 6].copy_from_slice(&endurance.to_le_bytes());
    let digest = Sha256::digest(&out[48..]);
    out[16..48].copy_from_slice(&digest);
    decode(&out)?;
    Ok(out)
}
fn plain(path: &Path, directory: bool) -> Result<()> {
    let m = fs::symlink_metadata(path)?;
    #[cfg(windows)]
    {
        use std::os::windows::fs::MetadataExt;
        if m.file_attributes() & 0x400 != 0 {
            return Err(bad("Verknüpfter Sockelpfad: keine Änderung."));
        }
    }
    if m.file_type().is_symlink()
        || m.is_dir() != directory
        || (!directory && crate::paths::link_count(path, &m)? != 1)
    {
        return Err(bad("Kein regulärer Sockelpfad."));
    }
    Ok(())
}
fn read(path: &Path, limit: usize) -> Result<Vec<u8>> {
    plain(path, false)?;
    let mut f = File::open(path)?;
    let size = f.metadata()?.len();
    if size > limit as u64 {
        return Err(bad("Sockeldatei überschreitet Größenlimit."));
    }
    let mut raw = Vec::new();
    (&mut f).take(limit as u64 + 1).read_to_end(&mut raw)?;
    if raw.len() as u64 != size {
        return Err(bad("Datei wurde beim Einlesen geändert."));
    }
    Ok(raw)
}
fn config_path(game: &Path) -> Result<PathBuf> {
    let dir = game.join("bin64");
    for p in dir.ancestors() {
        plain(p, true)?;
    }
    Ok(dir.join("CrimsonExtraSockets.dat"))
}
fn installed(game: &Path) -> Result<()> {
    let digest = hash_bytes(&read(
        &game.join("bin64/CrimsonExtraSockets.asi"),
        1_048_576,
    )?);
    if digest != ASI && RELEASE_ASI != Some(digest.as_str()) {
        return Err(bad(
            "Die passende Zusatzsockel-Mod ist noch nicht installiert.",
        ));
    }
    Ok(())
}
fn snapshot_raw(data: &GameData, raw: &[u8]) -> Result<Snapshot> {
    let decoded = decode(raw)?;
    let items = data.localized_items()?;
    let records = decoded
        .iter()
        .map(|r| Record {
            uid: r.uid.to_string(),
            item_key: r.key,
            name: items
                .iter()
                .find(|x| x.key == r.key)
                .map(|x| x.name.clone())
                .unwrap_or_else(|| r.key.to_string()),
            baseline: r.base,
            additional: 10,
            gems: r
                .gems
                .iter()
                .map(|g| OccupiedGem {
                    key: g.key,
                    endurance: g.endurance,
                    name: items
                        .iter()
                        .find(|x| x.key == g.key)
                        .map(|x| x.name.clone()),
                })
                .collect(),
        })
        .collect();
    // Log is diagnostic output, never instructions or proof of a current game session.
    let log = data.game_root().join("bin64/CrimsonExtraSockets.log");
    let runtime_log = if log.try_exists()? {
        plain(&log, false)?;
        let mut f = File::open(&log)?;
        let start = f.metadata()?.len().saturating_sub(16_384);
        f.seek(SeekFrom::Start(start))?;
        let mut raw = Vec::new();
        f.take(16_384).read_to_end(&mut raw)?;
        String::from_utf8_lossy(&raw)
            .lines()
            .rev()
            .take(8)
            .collect::<Vec<_>>()
            .into_iter()
            .rev()
            .collect::<Vec<_>>()
            .join("\n")
    } else {
        String::new()
    };
    Ok(Snapshot {
        revision: hash_bytes(raw),
        game_running: crate::apply::guard::game_running()?,
        records,
        runtime_log,
    })
}
pub fn snapshot(data: &GameData) -> Result<Snapshot> {
    let path = config_path(data.game_root())?;
    installed(data.game_root())?;
    snapshot_raw(data, &read(&path, LENGTH)?)
}
fn stopped() -> Result<()> {
    if crate::apply::guard::game_running()? {
        return Err(bad(
            "Crimson Desert läuft. Erst speichern und vollständig schließen, bevor Sockel geändert werden.",
        ));
    }
    Ok(())
}
fn game_guard(game: &Path) -> Result<File> {
    let path = game.join("bin64/CrimsonDesert.exe");
    plain(&path, false)?;
    let mut options = OpenOptions::new();
    options.read(true);
    #[cfg(windows)]
    {
        use std::os::windows::fs::OpenOptionsExt;
        options.share_mode(0).custom_flags(0x00200000);
    }
    let mut file = options.open(path)?;
    let mut h = Sha256::new();
    let mut buffer = [0; 65536];
    loop {
        let n = file.read(&mut buffer)?;
        if n == 0 {
            break;
        }
        h.update(&buffer[..n]);
    }
    if format!("{:x}", h.finalize()) != EXE {
        return Err(Error::UnsupportedBuild(
            "Zusatzsockel unterstützen ausschließlich Build 1.0.0.2976.".into(),
        ));
    }
    stopped()?;
    Ok(file)
}
pub fn set(data: &GameData, project: &Path, request: &Request) -> Result<Receipt> {
    stopped()?;
    let game = data.game_root();
    let path = config_path(game)?;
    installed(game)?;
    let before = read(&path, LENGTH)?;
    let after = change(&before, request)?;
    commit(data, project, before, after, || Ok(()))
}
pub fn add(
    data: &GameData,
    project: &Path,
    save_root: &Path,
    request: &crate::extra_sockets_candidates::AddRequest,
) -> Result<Receipt> {
    stopped()?;
    let game = data.game_root();
    let path = config_path(game)?;
    installed(game)?;
    let before = read(&path, LENGTH)?;
    let settings = decode(&before)?;
    if hash_bytes(&before) != request.revision {
        return Err(bad("Die Sockeldatei wurde geändert. Bitte neu einlesen."));
    }
    if settings.len() >= 256 {
        return Err(bad("Höchstens 256 Gegenstandsinstanzen pro Konfiguration."));
    }
    let root = save_root;
    let (raw, _) = crate::mounts::files::read_pair(root, &request.save)?;
    if hash_bytes(&raw) != request.save_revision {
        return Err(bad(
            "Der Spielstand wurde geändert. Bitte die Ausrüstung neu einlesen.",
        ));
    }
    let items = crate::extra_sockets_candidates::parse(data, &raw)?;
    let item = items
        .iter()
        .find(|i| i.uid == request.uid)
        .ok_or_else(|| bad("Dieser Gegenstand ist nicht im ausgewählten Spielstand vorhanden."))?;
    if !item.eligible {
        return Err(bad(item.reason.clone().unwrap_or_else(|| {
            "Dieser Gegenstand ist nicht unterstützt.".into()
        })));
    }
    let uid = item
        .uid
        .parse::<u64>()
        .map_err(|_| bad("Ungültige Instanz-ID."))?;
    if settings.iter().any(|r| r.uid == uid) {
        return Err(bad(
            "Diese Instanz ist bereits erweitert. Es werden keine doppelten Zusatzsockel angelegt.",
        ));
    }
    let mut after = before.clone();
    after[8..12].copy_from_slice(&2u32.to_le_bytes());
    after[12..16].copy_from_slice(&((settings.len() + 1) as u32).to_le_bytes());
    after.extend(uid.to_le_bytes());
    after.extend(item.item_key.to_le_bytes());
    after.extend([item.baseline, 10, 0, 0]);
    after.extend([0; 60]);
    let digest = Sha256::digest(&after[48..]);
    after[16..48].copy_from_slice(&digest);
    decode(&after)?;
    commit(data, project, before, after, || {
        let (now, _) = crate::mounts::files::read_pair(root, &request.save)?;
        if now != raw {
            return Err(bad(
                "Spielstand während der Auswahl geändert; keine Sockelerweiterung.",
            ));
        }
        Ok(())
    })
}
fn commit(
    data: &GameData,
    project: &Path,
    before: Vec<u8>,
    after: Vec<u8>,
    verify: impl Fn() -> Result<()>,
) -> Result<Receipt> {
    stopped()?;
    let game = data.game_root();
    let path = config_path(game)?;
    installed(game)?;
    let _guard = game_guard(game)?;
    verify()?;
    let policy = crate::output_policy(project, Some(game))?;
    let stamp = SystemTime::now()
        .duration_since(UNIX_EPOCH)
        .map_err(|e| bad(e.to_string()))?
        .as_nanos();
    let backup = policy.write_new(
        &PathBuf::from(format!("backups/extra-sockets-{stamp}.dat")),
        &before,
    )?;
    let temp = path.with_file_name(format!("CrimsonExtraSockets.dat.workbench-{stamp}"));
    let mut f = OpenOptions::new()
        .write(true)
        .create_new(true)
        .open(&temp)?;
    f.write_all(&after)?;
    f.sync_all()?;
    drop(f);
    let result = (|| {
        stopped()?;
        installed(game)?;
        verify()?;
        if read(&path, LENGTH)? != before {
            return Err(bad(
                "Zusatzsockel-Datei seit Auswahl geändert. Keine Installation.",
            ));
        }
        replace(&temp, &path)?;
        if read(&path, LENGTH)? != after {
            return Err(bad("Zusatzsockel-Readback fehlgeschlagen."));
        }
        snapshot_raw(data, &after)
    })();
    if result.is_err() && temp.try_exists()? {
        plain(&temp, false)?;
        fs::remove_file(&temp)?;
    }
    Ok(Receipt {
        backup,
        snapshot: result?,
    })
}
fn replace(from: &Path, to: &Path) -> Result<()> {
    #[cfg(windows)]
    {
        use std::os::windows::ffi::OsStrExt;
        use windows_sys::Win32::Storage::FileSystem::*;
        let a: Vec<u16> = from.as_os_str().encode_wide().chain(Some(0)).collect();
        let b: Vec<u16> = to.as_os_str().encode_wide().chain(Some(0)).collect();
        if unsafe {
            MoveFileExW(
                a.as_ptr(),
                b.as_ptr(),
                MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH,
            )
        } == 0
        {
            return Err(std::io::Error::last_os_error().into());
        }
    }
    #[cfg(not(windows))]
    {
        fs::rename(from, to)?;
        File::open(to.parent().unwrap())?.sync_all()?;
    }
    Ok(())
}
#[cfg(test)]
mod tests {
    use super::*;
    fn initial() -> Vec<u8> {
        let mut raw = b"CWEXSOCK".to_vec();
        raw.extend(1u32.to_le_bytes());
        raw.extend(3u32.to_le_bytes());
        raw.extend([0; 32]);
        for (uid, key, base) in TARGETS {
            raw.extend(uid.to_le_bytes());
            raw.extend(key.to_le_bytes());
            raw.extend([base, 10, 0, 0]);
            raw.extend([0; 60]);
        }
        let hash = Sha256::digest(&raw[48..]);
        raw[16..48].copy_from_slice(&hash);
        raw
    }
    #[test]
    fn scoped_roundtrip_and_inverse() {
        let raw = initial();
        for (uid, _, _) in TARGETS {
            let r = Request {
                revision: hash_bytes(&raw),
                uid: uid.to_string(),
                index: 9,
                gem_key: 1002787,
            };
            let changed = change(&raw, &r).unwrap();
            assert_eq!(
                decode(&changed)
                    .unwrap()
                    .iter()
                    .flat_map(|s| s.gems.iter())
                    .filter(|g| g.key != 0)
                    .count(),
                1
            );
            let clear = Request {
                revision: hash_bytes(&changed),
                gem_key: 0,
                ..r
            };
            assert_eq!(change(&changed, &clear).unwrap(), raw);
        }
    }
    #[test]
    fn corrupt_scope_length_and_checksum_reject() {
        let raw = initial();
        for at in [0, 8, 12, 16, 48, 60, 62, 63, 150, 275] {
            let mut v = raw.clone();
            v[at] ^= 1;
            assert!(decode(&v).is_err());
        }
        assert!(decode(&raw[..275]).is_err());
        let mut v = raw.clone();
        v.push(0);
        assert!(decode(&v).is_err());
    }
    #[test]
    fn stale_and_non_gem_reject() {
        let raw = initial();
        let mut r = Request {
            revision: "stale".into(),
            uid: "1003664".into(),
            index: 0,
            gem_key: 1002787,
        };
        assert!(change(&raw, &r).is_err());
        r.revision = hash_bytes(&raw);
        r.gem_key = 111005;
        assert!(change(&raw, &r).is_err());
        r.gem_key = 1002787;
        r.index = 10;
        assert!(change(&raw, &r).is_err());
        r.index = 0;
        r.uid = "../1003664".into();
        assert!(change(&raw, &r).is_err());
    }
    #[test]
    fn generalized_codec_migration_duplicate_and_bounds() {
        let original = initial();
        let mut raw = original.clone();
        raw[8..12].copy_from_slice(&2u32.to_le_bytes());
        raw[12..16].copy_from_slice(&4u32.to_le_bytes());
        raw.extend(2000000u64.to_le_bytes());
        raw.extend(1001062u32.to_le_bytes());
        raw.extend([3, 10, 0, 0]);
        raw.extend([0; 60]);
        let seal = |v: &mut Vec<u8>| {
            let h = Sha256::digest(&v[48..]);
            v[16..48].copy_from_slice(&h);
        };
        seal(&mut raw);
        assert_eq!(decode(&raw).unwrap()[3].base, 3);
        assert_eq!(&raw[48..276], &original[48..]);
        let r = Request {
            revision: hash_bytes(&raw),
            uid: "2000000".into(),
            index: 9,
            gem_key: 1002787,
        };
        let changed = change(&raw, &r).unwrap();
        assert_eq!(&changed[48..276], &original[48..]);
        assert_eq!(decode(&changed).unwrap()[3].gems[9].key, 1002787);
        let mut bad = raw.clone();
        bad[276..284].copy_from_slice(&1001416u64.to_le_bytes());
        seal(&mut bad);
        assert!(decode(&bad).is_err());
        let mut bad = raw.clone();
        bad[288] = 6;
        seal(&mut bad);
        assert!(decode(&bad).is_err());
        let mut bad = raw.clone();
        bad[284..288].copy_from_slice(&1002787u32.to_le_bytes());
        seal(&mut bad);
        assert!(decode(&bad).is_err());
        let mut empty = b"CWEXSOCK".to_vec();
        empty.extend(2u32.to_le_bytes());
        empty.extend(0u32.to_le_bytes());
        empty.extend([0; 32]);
        seal(&mut empty);
        assert!(decode(&empty).unwrap().is_empty());
    }
}
