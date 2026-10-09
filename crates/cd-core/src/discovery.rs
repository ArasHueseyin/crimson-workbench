//! Read-only installation/save discovery and a platform-independent PE version reader.
use serde::{Deserialize, Serialize};
use serde_json::Value;
use std::collections::{BTreeMap, HashSet};
use std::fs::{self, File};
use std::io::{self, Read, Seek, SeekFrom};
use std::path::{Path, PathBuf};

pub const STEAM_APP_ID: &str = "3321460";

#[derive(Debug, Clone, Copy, PartialEq, Eq, Serialize, Deserialize)]
#[serde(rename_all = "snake_case")]
pub enum Platform {
    Explicit,
    Steam,
    Epic,
    GamePass,
}

impl std::fmt::Display for Platform {
    fn fmt(&self, f: &mut std::fmt::Formatter<'_>) -> std::fmt::Result {
        f.write_str(match self {
            Self::Explicit => "explicit",
            Self::Steam => "steam",
            Self::Epic => "epic",
            Self::GamePass => "game_pass",
        })
    }
}

#[derive(Debug, Clone, PartialEq, Eq, Serialize, Deserialize)]
pub struct Installation {
    pub path: PathBuf,
    pub platform: Platform,
    pub build_id: Option<String>,
}
impl Installation {
    pub fn executable(&self) -> PathBuf {
        self.path.join("bin64").join("CrimsonDesert.exe")
    }
}

/// Empty defaults are deterministic for tests. `from_environment` supplies host
/// defaults. Callers loading `.env` should inject those values explicitly.
#[derive(Debug, Clone, Default)]
pub struct DiscoveryOptions {
    pub explicit_game_dir: Option<PathBuf>,
    pub explicit_save_dir: Option<PathBuf>,
    pub steam_roots: Vec<PathBuf>,
    pub epic_manifest_dirs: Vec<PathBuf>,
    pub xbox_roots: Vec<PathBuf>,
    pub local_app_data: Option<PathBuf>,
    pub package_roots: Vec<PathBuf>,
}

impl DiscoveryOptions {
    pub fn from_environment() -> Self {
        let mut options = Self {
            explicit_game_dir: env_path("CD_GAME_DIR"),
            explicit_save_dir: env_path("CD_SAVE_DIR"),
            local_app_data: env_path("LOCALAPPDATA"),
            ..Self::default()
        };
        for name in ["STEAM_DIR", "STEAM_PATH"] {
            if let Some(path) = env_path(name) {
                options.steam_roots.push(path);
            }
        }
        for name in ["ProgramFiles(x86)", "ProgramFiles"] {
            if let Some(path) = env_path(name) {
                options.steam_roots.push(path.join("Steam"));
            }
        }
        if let Some(path) = env_path("ProgramData") {
            options
                .epic_manifest_dirs
                .push(path.join("Epic/EpicGamesLauncher/Data/Manifests"));
        }
        if let Some(path) = &options.local_app_data {
            options.package_roots.push(path.join("Packages"));
        }
        #[cfg(windows)]
        {
            // Read-only registry query supports a non-default Steam client location.
            for (key, value) in [
                (r"HKCU\Software\Valve\Steam", "SteamPath"),
                (r"HKLM\SOFTWARE\WOW6432Node\Valve\Steam", "InstallPath"),
            ] {
                if let Some(path) = registry_path(key, value) {
                    options.steam_roots.push(path);
                }
            }
            for drive in b'C'..=b'Z' {
                options
                    .xbox_roots
                    .push(PathBuf::from(format!("{}:/XboxGames", drive as char)));
            }
            if let Some(path) = env_path("ProgramFiles") {
                options.xbox_roots.push(path.join("WindowsApps"));
            }
        }
        #[cfg(not(windows))]
        if let Some(home) = env_path("HOME") {
            options
                .steam_roots
                .extend([home.join(".steam/steam"), home.join(".local/share/Steam")]);
        }
        options
    }
}
fn env_path(name: &str) -> Option<PathBuf> {
    std::env::var_os(name)
        .filter(|s| !s.is_empty())
        .map(PathBuf::from)
}
#[cfg(windows)]
fn registry_path(key: &str, value: &str) -> Option<PathBuf> {
    use std::os::windows::process::CommandExt;
    let output = std::process::Command::new("reg.exe")
        .args(["query", key, "/v", value])
        .creation_flags(0x0800_0000)
        .output()
        .ok()?;
    if !output.status.success() {
        return None;
    }
    String::from_utf8_lossy(&output.stdout)
        .lines()
        .find_map(|line| {
            let (_, path) = line.split_once("REG_SZ")?;
            (!path.trim().is_empty()).then(|| PathBuf::from(path.trim()))
        })
}

pub fn discover() -> io::Result<Vec<Installation>> {
    discover_with(&DiscoveryOptions::from_environment())
}

/// Explicit paths have priority, but other installations are still returned so
/// callers can protect every installation when validating generated outputs.
pub fn discover_with(options: &DiscoveryOptions) -> io::Result<Vec<Installation>> {
    let mut found = Vec::new();
    if let Some(path) = &options.explicit_game_dir {
        let root = game_root(path).ok_or_else(|| {
            io::Error::new(
                io::ErrorKind::NotFound,
                format!(
                    "configured game directory has no bin64/CrimsonDesert.exe and meta/0.papgt: {}",
                    path.display()
                ),
            )
        })?;
        add_installation(&mut found, root, Platform::Explicit, None);
    }
    let libraries = steam_libraries(options);
    let mut seen_libraries = HashSet::new();
    for library in libraries {
        if !seen_libraries.insert(path_key(&library)) {
            continue;
        }
        let steamapps = if library
            .file_name()
            .is_some_and(|n| n.eq_ignore_ascii_case("steamapps"))
        {
            library
        } else {
            library.join("steamapps")
        };
        let manifest = steamapps.join(format!("appmanifest_{STEAM_APP_ID}.acf"));
        if let Ok(text) = read_small_text(&manifest)
            && let Ok(vdf) = parse_vdf(&text)
            && let Some(Vdf::Object(app)) = lookup(&vdf, "AppState")
        {
            if string(app, "appid") != Some(STEAM_APP_ID) {
                continue;
            }
            if let Some(dir) = string(app, "installdir") {
                // A manifest install name is a relative child, never an arbitrary root.
                if !safe_relative(dir) {
                    continue;
                }
                if let Some(root) = game_root(&steamapps.join("common").join(dir)) {
                    add_installation(
                        &mut found,
                        root,
                        Platform::Steam,
                        string(app, "buildid").map(str::to_owned),
                    );
                }
            }
        }
    }
    for directory in &options.epic_manifest_dirs {
        for entry in sorted_entries(directory) {
            if !entry
                .extension()
                .is_some_and(|e| e.eq_ignore_ascii_case("item"))
            {
                continue;
            }
            if let Ok(text) = read_small_text(&entry)
                && let Ok(value) = serde_json::from_str::<Value>(&text)
                && let Some(location) = value.get("InstallLocation").and_then(Value::as_str)
                && let Some(root) = game_root(Path::new(location))
            {
                add_installation(
                    &mut found,
                    root,
                    Platform::Epic,
                    value
                        .get("AppVersionString")
                        .and_then(Value::as_str)
                        .map(str::to_owned),
                );
            }
        }
    }
    // Xbox app installs normally use XboxGames/<title>/Content; protected MSIX
    // directories may be unreadable. Discovery skips them, accepting an explicit
    // accessible path instead. No ownership/ACL changes or package calls are made.
    for base in &options.xbox_roots {
        let candidates = std::iter::once(base.clone()).chain(sorted_entries(base));
        for path in candidates {
            if let Some(root) = game_root(&path) {
                let build = [
                    root.join("MicrosoftGame.config"),
                    root.join("AppxManifest.xml"),
                ]
                .iter()
                .filter_map(|p| read_small_text(p).ok())
                .find_map(|text| identity_version(&text));
                add_installation(&mut found, root, Platform::GamePass, build);
            }
        }
    }
    Ok(found)
}

fn steam_libraries(options: &DiscoveryOptions) -> Vec<PathBuf> {
    let mut libraries = options.steam_roots.clone();
    for root in &options.steam_roots {
        for file in [
            root.join("steamapps/libraryfolders.vdf"),
            root.join("config/libraryfolders.vdf"),
        ] {
            if let Ok(text) = read_small_text(&file)
                && let Ok(vdf) = parse_vdf(&text)
                && let Some(Vdf::Object(folders)) = lookup(&vdf, "libraryfolders")
            {
                for (key, value) in folders {
                    if key.parse::<u32>().is_err() {
                        continue;
                    }
                    match value {
                        Vdf::String(path) => libraries.push(PathBuf::from(path)),
                        Vdf::Object(fields) => {
                            if let Some(path) = string(fields, "path") {
                                libraries.push(PathBuf::from(path));
                            }
                        }
                    }
                }
            }
        }
    }
    libraries
}
fn add_installation(
    found: &mut Vec<Installation>,
    path: PathBuf,
    platform: Platform,
    build_id: Option<String>,
) {
    let path = fs::canonicalize(&path).unwrap_or(path);
    if let Some(existing) = found
        .iter_mut()
        .find(|i| path_key(&i.path) == path_key(&path))
    {
        // Keep explicit ordering/selection but enrich it with discovered metadata.
        if existing.build_id.is_none() {
            existing.build_id = build_id;
        }
        if existing.platform == Platform::Explicit {
            existing.platform = platform;
        }
        return;
    }
    found.push(Installation {
        path,
        platform,
        build_id,
    });
}
fn path_key(path: &Path) -> String {
    let path = fs::canonicalize(path).unwrap_or_else(|_| path.to_path_buf());
    #[cfg(windows)]
    {
        path.to_string_lossy().replace('/', "\\").to_lowercase()
    }
    #[cfg(not(windows))]
    {
        path.to_string_lossy().into_owned()
    }
}
fn game_root(path: &Path) -> Option<PathBuf> {
    [path.to_path_buf(), path.join("Content")]
        .into_iter()
        .find(|root| {
            root.join("bin64/CrimsonDesert.exe").is_file() && root.join("meta/0.papgt").is_file()
        })
}
fn safe_relative(value: &str) -> bool {
    !value.is_empty()
        && !value.contains(':')
        && !value.starts_with(['/', '\\'])
        && value
            .split(['/', '\\'])
            .all(|part| !matches!(part, ".." | "." | ""))
}
fn sorted_entries(path: &Path) -> Vec<PathBuf> {
    let mut paths: Vec<_> = fs::read_dir(path)
        .into_iter()
        .flatten()
        .filter_map(Result::ok)
        .take(4096)
        .map(|entry| entry.path())
        .collect();
    paths.sort();
    paths
}
fn read_small_text(path: &Path) -> io::Result<String> {
    let file = File::open(path)?;
    let mut bytes = Vec::new();
    file.take(4 * 1024 * 1024 + 1).read_to_end(&mut bytes)?;
    if bytes.len() > 4 * 1024 * 1024 {
        return Err(bad("discovery metadata exceeds 4 MiB"));
    }
    String::from_utf8(bytes)
        .map(|s| s.trim_start_matches('\u{feff}').to_owned())
        .map_err(|_| bad("metadata is not UTF-8"))
}
fn identity_version(text: &str) -> Option<String> {
    let start = text.find("<Identity")?;
    let tag = text.get(start..)?.split_once('>')?.0;
    for quote in ['"', '\''] {
        let token = format!("Version={quote}");
        if let Some((_, rest)) = tag.split_once(&token) {
            let version = rest.split(quote).next()?;
            if !version.is_empty() && version.bytes().all(|c| c.is_ascii_digit() || c == b'.') {
                return Some(version.to_owned());
            }
        }
    }
    None
}

/// Existing save roots, not individual save files. Save bytes are never opened by
/// discovery. Keep explicitly configured non-existing roots in PathPolicy too.
pub fn locate_saves() -> Vec<PathBuf> {
    locate_saves_with(&DiscoveryOptions::from_environment())
}
pub fn locate_saves_with(options: &DiscoveryOptions) -> Vec<PathBuf> {
    let mut candidates = Vec::new();
    if let Some(path) = &options.explicit_save_dir {
        candidates.push(path.clone());
    }
    if let Some(local) = &options.local_app_data {
        for child in [
            "Pearl Abyss/CD/save",
            "Pearl Abyss/CD_Epic/save",
            "Pearl Abyss/CD_GamePass/save",
            "CrimsonDesert/Saved/SaveGames",
        ] {
            candidates.push(local.join(child));
        }
    }
    for packages in &options.package_roots {
        for package in sorted_entries(packages) {
            let name = package
                .file_name()
                .unwrap_or_default()
                .to_string_lossy()
                .to_lowercase();
            if name.starts_with("pearlabyss.crimsondesert") {
                candidates.extend([
                    package.join("SystemAppData/wgs"),
                    package.join("LocalState"),
                ]);
            }
        }
    }
    for steam in steam_libraries(options) {
        candidates.push(steam.join(format!("steamapps/compatdata/{STEAM_APP_ID}/pfx/drive_c/users/steamuser/AppData/Local/Pearl Abyss/CD/save")));
    }
    let mut seen = HashSet::new();
    candidates
        .into_iter()
        .filter(|path| path.is_dir())
        .filter_map(|path| fs::canonicalize(path).ok())
        .filter(|path| seen.insert(path_key(path)))
        .collect()
}

#[derive(Debug)]
pub(crate) enum Vdf {
    String(String),
    Object(BTreeMap<String, Vdf>),
}
pub(crate) fn lookup<'a>(object: &'a BTreeMap<String, Vdf>, key: &str) -> Option<&'a Vdf> {
    object
        .iter()
        .find(|(name, _)| name.eq_ignore_ascii_case(key))
        .map(|(_, value)| value)
}
pub(crate) fn string<'a>(object: &'a BTreeMap<String, Vdf>, key: &str) -> Option<&'a str> {
    match lookup(object, key) {
        Some(Vdf::String(value)) => Some(value),
        _ => None,
    }
}
pub(crate) fn parse_vdf(text: &str) -> io::Result<BTreeMap<String, Vdf>> {
    #[derive(PartialEq)]
    enum Token {
        Text(String),
        Open,
        Close,
    }
    let mut tokens = Vec::new();
    let mut chars = text.chars().peekable();
    while let Some(c) = chars.next() {
        match c {
            c if c.is_whitespace() => {}
            '/' if chars.peek() == Some(&'/') => {
                for c in chars.by_ref() {
                    if c == '\n' {
                        break;
                    }
                }
            }
            '{' => tokens.push(Token::Open),
            '}' => tokens.push(Token::Close),
            '"' => {
                let mut value = String::new();
                let mut closed = false;
                while let Some(c) = chars.next() {
                    match c {
                        '"' => {
                            closed = true;
                            break;
                        }
                        '\\' => match chars.peek() {
                            Some('"' | '\\') => value.push(chars.next().unwrap()),
                            _ => value.push('\\'),
                        },
                        _ => value.push(c),
                    }
                }
                if !closed {
                    return Err(bad("unterminated VDF string"));
                }
                tokens.push(Token::Text(value));
            }
            _ => {
                let mut value = String::from(c);
                while chars
                    .peek()
                    .is_some_and(|c| !c.is_whitespace() && *c != '{' && *c != '}')
                {
                    value.push(chars.next().unwrap());
                }
                tokens.push(Token::Text(value));
            }
        }
    }
    fn object(
        tokens: &[Token],
        pos: &mut usize,
        depth: usize,
    ) -> io::Result<BTreeMap<String, Vdf>> {
        if depth > 32 {
            return Err(bad("VDF nesting too deep"));
        }
        let mut result = BTreeMap::new();
        while *pos < tokens.len() {
            if tokens[*pos] == Token::Close {
                if depth == 0 {
                    return Err(bad("unexpected VDF closing brace"));
                }
                *pos += 1;
                return Ok(result);
            }
            let key = match &tokens[*pos] {
                Token::Text(s) => s.clone(),
                _ => return Err(bad("invalid VDF key")),
            };
            *pos += 1;
            let value = match tokens.get(*pos) {
                Some(Token::Text(s)) => {
                    *pos += 1;
                    Vdf::String(s.clone())
                }
                Some(Token::Open) => {
                    *pos += 1;
                    Vdf::Object(object(tokens, pos, depth + 1)?)
                }
                _ => return Err(bad("invalid VDF value")),
            };
            if result.keys().any(|k: &String| k.eq_ignore_ascii_case(&key)) {
                return Err(bad("duplicate VDF key"));
            }
            result.insert(key, value);
        }
        if depth != 0 {
            return Err(bad("unclosed VDF object"));
        }
        Ok(result)
    }
    object(&tokens, &mut 0, 0)
}

fn bad(message: &str) -> io::Error {
    io::Error::new(io::ErrorKind::InvalidData, message)
}

/// Read the numeric Windows file version directly from PE RT_VERSION resources.
/// Works on Windows/Linux, opens only read-only handles, and uses bounded random
/// reads rather than loading a large executable into memory. Missing RT_VERSION
/// is `None`; malformed PE/resource structures are errors, never guessed versions.
pub fn read_exe_version(path: &Path) -> io::Result<Option<String>> {
    let mut file = File::open(path)?;
    let length = file.metadata()?.len();
    read_pe_version(&mut file, length)
}
#[derive(Clone, Copy)]
struct Section {
    rva: u32,
    raw_size: u32,
    raw_offset: u32,
}
fn read_at<R: Read + Seek>(
    reader: &mut R,
    length: u64,
    offset: u64,
    count: usize,
) -> io::Result<Vec<u8>> {
    if offset
        .checked_add(count as u64)
        .is_none_or(|end| end > length)
    {
        return Err(bad("PE read outside file"));
    }
    let mut data = vec![0; count];
    reader.seek(SeekFrom::Start(offset))?;
    reader.read_exact(&mut data)?;
    Ok(data)
}
fn u16_at(bytes: &[u8], offset: usize) -> io::Result<u16> {
    let slice = bytes
        .get(
            offset
                ..offset
                    .checked_add(2)
                    .ok_or_else(|| bad("offset overflow"))?,
        )
        .ok_or_else(|| bad("truncated u16"))?;
    Ok(u16::from_le_bytes([slice[0], slice[1]]))
}
fn u32_at(bytes: &[u8], offset: usize) -> io::Result<u32> {
    let slice = bytes
        .get(
            offset
                ..offset
                    .checked_add(4)
                    .ok_or_else(|| bad("offset overflow"))?,
        )
        .ok_or_else(|| bad("truncated u32"))?;
    Ok(u32::from_le_bytes([slice[0], slice[1], slice[2], slice[3]]))
}
fn file_offset(sections: &[Section], rva: u32, size: u32) -> io::Result<u64> {
    for section in sections {
        if let Some(relative) = rva.checked_sub(section.rva)
            && relative
                .checked_add(size)
                .is_some_and(|end| end <= section.raw_size)
        {
            return Ok(u64::from(section.raw_offset) + u64::from(relative));
        }
    }
    Err(bad("PE RVA is not backed by section data"))
}
fn resource_entries(bytes: &[u8], offset: usize) -> io::Result<Vec<(u32, u32)>> {
    let count = usize::from(u16_at(
        bytes,
        offset
            .checked_add(12)
            .ok_or_else(|| bad("offset overflow"))?,
    )?) + usize::from(u16_at(
        bytes,
        offset
            .checked_add(14)
            .ok_or_else(|| bad("offset overflow"))?,
    )?);
    if count > 4096 {
        return Err(bad("excessive PE resource entries"));
    }
    let mut entries = Vec::with_capacity(count);
    for index in 0..count {
        let at = offset
            .checked_add(16 + index * 8)
            .ok_or_else(|| bad("offset overflow"))?;
        entries.push((u32_at(bytes, at)?, u32_at(bytes, at + 4)?));
    }
    Ok(entries)
}
fn resource_leaf(bytes: &[u8], target: u32, depth: usize) -> io::Result<usize> {
    if depth > 4 {
        return Err(bad("cyclic or overly deep PE resource directory"));
    }
    let offset = (target & 0x7fff_ffff) as usize;
    if target & 0x8000_0000 == 0 {
        return Ok(offset);
    }
    let entries = resource_entries(bytes, offset)?;
    let (_, child) = entries
        .first()
        .ok_or_else(|| bad("empty PE version directory"))?;
    resource_leaf(bytes, *child, depth + 1)
}
fn read_pe_version<R: Read + Seek>(reader: &mut R, length: u64) -> io::Result<Option<String>> {
    let dos = read_at(reader, length, 0, 64)?;
    if &dos[0..2] != b"MZ" {
        return Err(bad("not a Windows PE executable"));
    }
    let pe = u64::from(u32_at(&dos, 60)?);
    let header = read_at(reader, length, pe, 24)?;
    if &header[0..4] != b"PE\0\0" {
        return Err(bad("invalid PE signature"));
    }
    let section_count = usize::from(u16_at(&header, 6)?);
    if section_count == 0 || section_count > 96 {
        return Err(bad("invalid PE section count"));
    }
    let optional_size = usize::from(u16_at(&header, 20)?);
    if optional_size > 4096 {
        return Err(bad("PE optional header too large"));
    }
    let optional = read_at(reader, length, pe + 24, optional_size)?;
    let directory_start = match u16_at(&optional, 0)? {
        0x10b => 96,
        0x20b => 112,
        _ => return Err(bad("unsupported PE optional header")),
    };
    if u32_at(&optional, directory_start - 4)? < 3 {
        return Ok(None);
    }
    let resource_rva = u32_at(&optional, directory_start + 16)?;
    let resource_size = u32_at(&optional, directory_start + 20)?;
    if resource_rva == 0 || resource_size == 0 {
        return Ok(None);
    }
    if resource_size > 64 * 1024 * 1024 {
        return Err(bad("PE resources exceed 64 MiB bound"));
    }
    let section_bytes = read_at(
        reader,
        length,
        pe + 24 + optional_size as u64,
        section_count * 40,
    )?;
    let mut sections = Vec::with_capacity(section_count);
    for index in 0..section_count {
        let at = index * 40;
        sections.push(Section {
            rva: u32_at(&section_bytes, at + 12)?,
            raw_size: u32_at(&section_bytes, at + 16)?,
            raw_offset: u32_at(&section_bytes, at + 20)?,
        });
    }
    let offset = file_offset(&sections, resource_rva, resource_size)?;
    let resources = read_at(reader, length, offset, resource_size as usize)?;
    let root = resource_entries(&resources, 0)?;
    let Some((_, target)) = root.into_iter().find(|(id, _)| *id == 16) else {
        return Ok(None);
    };
    let leaf = resource_leaf(&resources, target, 0)?;
    let data_rva = u32_at(&resources, leaf)?;
    let data_size = u32_at(&resources, leaf + 4)?;
    if data_size > 1024 * 1024 {
        return Err(bad("PE version payload exceeds 1 MiB bound"));
    }
    let data_offset = file_offset(&sections, data_rva, data_size)?;
    let version = read_at(reader, length, data_offset, data_size as usize)?;
    let block_len = usize::from(u16_at(&version, 0)?);
    let value_len = usize::from(u16_at(&version, 2)?);
    if block_len > version.len() || block_len < 6 || value_len < 52 || u16_at(&version, 4)? != 0 {
        return Err(bad("invalid VS_VERSIONINFO lengths/type"));
    }
    let mut at = 6;
    let mut key = Vec::new();
    loop {
        if at + 2 > block_len {
            return Err(bad("unterminated VS_VERSIONINFO key"));
        }
        let unit = u16_at(&version, at)?;
        at += 2;
        if unit == 0 {
            break;
        }
        key.push(unit);
        if key.len() > 64 {
            return Err(bad("invalid VS_VERSIONINFO key"));
        }
    }
    if String::from_utf16(&key).ok().as_deref() != Some("VS_VERSION_INFO") {
        return Err(bad("unexpected version resource key"));
    }
    at = (at + 3) & !3;
    if at.checked_add(value_len).is_none_or(|end| end > block_len)
        || u32_at(&version, at)? != 0xfeef_04bd
    {
        return Err(bad("invalid VS_FIXEDFILEINFO"));
    }
    let high = u32_at(&version, at + 8)?;
    let low = u32_at(&version, at + 12)?;
    Ok(Some(format!(
        "{}.{}.{}.{}",
        high >> 16,
        high & 0xffff,
        low >> 16,
        low & 0xffff
    )))
}

#[cfg(test)]
mod tests {
    use super::*;
    use std::sync::atomic::{AtomicU64, Ordering};
    struct Fixture(PathBuf);
    impl Fixture {
        fn new() -> Self {
            static NEXT: AtomicU64 = AtomicU64::new(0);
            let path = std::env::temp_dir().join(format!(
                "cd-discovery-{}-{}",
                std::process::id(),
                NEXT.fetch_add(1, Ordering::Relaxed)
            ));
            fs::create_dir_all(&path).unwrap();
            Self(path)
        }
        fn file(&self, relative: &str, content: &[u8]) {
            let path = self.0.join(relative);
            fs::create_dir_all(path.parent().unwrap()).unwrap();
            fs::write(path, content).unwrap();
        }
        fn game(&self, relative: &str) {
            self.file(&format!("{relative}/bin64/CrimsonDesert.exe"), b"fixture");
            self.file(&format!("{relative}/meta/0.papgt"), b"fixture");
        }
    }
    impl Drop for Fixture {
        fn drop(&mut self) {
            let _ = fs::remove_dir_all(&self.0);
        }
    }
    fn quote_path(path: &Path) -> String {
        path.to_string_lossy()
            .replace('\\', "\\\\")
            .replace('"', "\\\"")
    }

    #[test]
    fn finds_external_steam_library_and_enriches_explicit_selection() {
        let f = Fixture::new();
        f.game("second/steamapps/common/Crimson Desert");
        f.file("steam/steamapps/libraryfolders.vdf",format!("// comment\n\"libraryfolders\" {{ \"1\" {{ \"path\" \"{}\" \"apps\" {{ \"3321460\" \"1\" }} }} }}",quote_path(&f.0.join("second"))).as_bytes());
        f.file("second/steamapps/appmanifest_3321460.acf",b"\"AppState\" { \"appid\" \"3321460\" \"installdir\" \"Crimson Desert\" \"buildid\" \"25381195\" }");
        let options = DiscoveryOptions {
            explicit_game_dir: Some(f.0.join("second/steamapps/common/Crimson Desert")),
            steam_roots: vec![f.0.join("steam")],
            ..Default::default()
        };
        let results = discover_with(&options).unwrap();
        assert_eq!(results.len(), 1);
        assert_eq!(results[0].build_id.as_deref(), Some("25381195"));
        assert_eq!(results[0].platform, Platform::Steam);
    }
    #[test]
    fn reads_epic_and_xbox_metadata_without_host_environment() {
        let f = Fixture::new();
        f.game("epic-game");
        f.game("XboxGames/Crimson Desert/Content");
        f.file("manifests/game.item",serde_json::to_string(&serde_json::json!({"InstallLocation":f.0.join("epic-game"),"AppVersionString":"2.03"})).unwrap().as_bytes());
        f.file(
            "XboxGames/Crimson Desert/Content/MicrosoftGame.config",
            b"<Game><Identity Name=\"PearlAbyss.CrimsonDesert\" Version=\"1.0.0.2944\" /></Game>",
        );
        let results = discover_with(&DiscoveryOptions {
            epic_manifest_dirs: vec![f.0.join("manifests")],
            xbox_roots: vec![f.0.join("XboxGames")],
            ..Default::default()
        })
        .unwrap();
        assert_eq!(results.len(), 2);
        assert_eq!(results[0].platform, Platform::Epic);
        assert_eq!(results[1].platform, Platform::GamePass);
        assert_eq!(results[1].build_id.as_deref(), Some("1.0.0.2944"));
    }
    #[test]
    fn ignores_malformed_or_traversing_manifest_and_rejects_bad_explicit_root() {
        let f = Fixture::new();
        f.game("game");
        f.file(
            "steam/steamapps/appmanifest_3321460.acf",
            b"\"AppState\" { \"appid\" \"3321460\" \"installdir\" \"../../game\" }",
        );
        assert!(
            discover_with(&DiscoveryOptions {
                steam_roots: vec![f.0.join("steam")],
                ..Default::default()
            })
            .unwrap()
            .is_empty()
        );
        assert!(
            discover_with(&DiscoveryOptions {
                explicit_game_dir: Some(f.0.join("absent")),
                ..Default::default()
            })
            .is_err()
        );
        assert!(parse_vdf("\"AppState\" { \"key\"").is_err());
    }
    #[test]
    fn locates_platform_save_roots_and_deduplicates_explicit_path() {
        let f = Fixture::new();
        let local = f.0.join("local");
        for child in [
            "Pearl Abyss/CD/save",
            "Pearl Abyss/CD_Epic/save",
            "Pearl Abyss/CD_GamePass/save",
            "Packages/PearlAbyss.CrimsonDesert_xyz/SystemAppData/wgs",
        ] {
            fs::create_dir_all(local.join(child)).unwrap();
        }
        let saves = locate_saves_with(&DiscoveryOptions {
            explicit_save_dir: Some(local.join("Pearl Abyss/CD/save")),
            local_app_data: Some(local.clone()),
            package_roots: vec![local.join("Packages")],
            ..Default::default()
        });
        assert_eq!(saves.len(), 4);
    }
    fn fake_pe() -> Vec<u8> {
        fn w16(v: &mut [u8], p: usize, x: u16) {
            v[p..p + 2].copy_from_slice(&x.to_le_bytes());
        }
        fn w32(v: &mut [u8], p: usize, x: u32) {
            v[p..p + 4].copy_from_slice(&x.to_le_bytes());
        }
        let mut data = vec![0u8; 0x600];
        data[0..2].copy_from_slice(b"MZ");
        w32(&mut data, 60, 0x80);
        data[0x80..0x84].copy_from_slice(b"PE\0\0");
        w16(&mut data, 0x86, 1);
        w16(&mut data, 0x94, 240);
        let opt = 0x98;
        w16(&mut data, opt, 0x20b);
        w32(&mut data, opt + 108, 16);
        w32(&mut data, opt + 128, 0x1000);
        w32(&mut data, opt + 132, 0x400);
        let section = opt + 240;
        w32(&mut data, section + 12, 0x1000);
        w32(&mut data, section + 16, 0x400);
        w32(&mut data, section + 20, 0x200);
        for (at, id, target) in [
            (0x200, 16, 0x8000_0020),
            (0x220, 1, 0x8000_0040),
            (0x240, 1033, 0x60),
        ] {
            w16(&mut data, at + 14, 1);
            w32(&mut data, at + 16, id);
            w32(&mut data, at + 20, target);
        }
        w32(&mut data, 0x260, 0x1100);
        w32(&mut data, 0x264, 92);
        w16(&mut data, 0x300, 92);
        w16(&mut data, 0x302, 52);
        for (i, c) in "VS_VERSION_INFO\0".encode_utf16().enumerate() {
            w16(&mut data, 0x306 + i * 2, c);
        }
        w32(&mut data, 0x328, 0xfeef_04bd);
        w32(&mut data, 0x32c, 0x10000);
        w32(&mut data, 0x330, 0x10000);
        w32(&mut data, 0x334, 2944);
        data
    }
    #[test]
    fn pe_version_is_read_from_resource_not_a_signature_scan() {
        let data = fake_pe();
        let mut reader = io::Cursor::new(&data);
        assert_eq!(
            read_pe_version(&mut reader, data.len() as u64)
                .unwrap()
                .as_deref(),
            Some("1.0.0.2944")
        );
        let mut no_version = data;
        no_version[0x210..0x214].copy_from_slice(&99u32.to_le_bytes());
        assert_eq!(
            read_pe_version(&mut io::Cursor::new(&no_version), no_version.len() as u64).unwrap(),
            None
        );
    }
    #[test]
    fn malformed_pe_resource_is_an_error() {
        let mut data = fake_pe();
        data[0x260..0x264].copy_from_slice(&u32::MAX.to_le_bytes());
        assert!(read_pe_version(&mut io::Cursor::new(&data), data.len() as u64).is_err());
        assert!(read_pe_version(&mut io::Cursor::new(b"MZ"), 2).is_err());
    }

    #[test]
    fn proton_saves_in_external_steam_library_are_discovered() {
        let f = Fixture::new();
        f.file(
            "steam/steamapps/libraryfolders.vdf",
            format!(
                "\"libraryfolders\" {{ \"1\" \"{}\" }}",
                quote_path(&f.0.join("external"))
            )
            .as_bytes(),
        );
        let save = f.0.join(format!("external/steamapps/compatdata/{STEAM_APP_ID}/pfx/drive_c/users/steamuser/AppData/Local/Pearl Abyss/CD/save"));
        fs::create_dir_all(&save).unwrap();
        let found = locate_saves_with(&DiscoveryOptions {
            steam_roots: vec![f.0.join("steam")],
            ..Default::default()
        });
        assert_eq!(found, vec![fs::canonicalize(save).unwrap()]);
    }
}
