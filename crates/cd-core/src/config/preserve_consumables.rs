//! Selected runtime consumables; CWC1 is shared with the native module.
use serde::Serialize;
use std::{
    fs,
    io::{self, Read, Write},
    path::Path,
    sync::atomic::{AtomicU64, Ordering},
};

pub const FILE_NAME: &str = "preserve-consumables.dat";
pub const ITEMS: [(u32, &str); 561] = [
    (2200, "Arrow"),
    (10033, "Food"),
    (21018, "Food"),
    (21019, "Food"),
    (22007, "Food"),
    (22008, "Food"),
    (22009, "Healing"),
    (25913, "Food"),
    (25914, "Food"),
    (27932, "Food"),
    (28080, "Food"),
    (28901, "Food"),
    (28917, "Food"),
    (29501, "Food"),
    (29502, "Food"),
    (29503, "Food"),
    (29504, "Food"),
    (29505, "Food"),
    (29506, "Food"),
    (29511, "Food"),
    (29513, "Food"),
    (29601, "Food"),
    (29602, "Food"),
    (29603, "Food"),
    (29604, "Food"),
    (29605, "Food"),
    (29606, "Food"),
    (29607, "Food"),
    (29608, "Food"),
    (29609, "Food"),
    (29612, "Food"),
    (29613, "Food"),
    (29615, "Food"),
    (29616, "Food"),
    (29617, "Food"),
    (29801, "Food"),
    (29802, "Food"),
    (29803, "Food"),
    (29804, "Food"),
    (29805, "Food"),
    (29806, "Food"),
    (29807, "Food"),
    (29808, "Food"),
    (29809, "Food"),
    (29810, "Food"),
    (29811, "Food"),
    (29812, "Food"),
    (29813, "Food"),
    (29814, "Food"),
    (29815, "Food"),
    (29816, "Food"),
    (29817, "Food"),
    (29818, "Food"),
    (29819, "Food"),
    (29821, "Food"),
    (50001, "Arrow"),
    (50003, "Arrow"),
    (50011, "Arrow"),
    (750002, "Food"),
    (750003, "Food"),
    (750004, "Food"),
    (750006, "Food"),
    (750008, "Food"),
    (750009, "Food"),
    (750018, "Food"),
    (750019, "Food"),
    (750024, "Food"),
    (750025, "Food"),
    (750030, "Food"),
    (750035, "Food"),
    (750040, "Food"),
    (750041, "Food"),
    (750049, "Food"),
    (751004, "Food"),
    (751103, "Food"),
    (751104, "Food"),
    (751105, "Food"),
    (751106, "Food"),
    (751107, "Food"),
    (751108, "Food"),
    (751109, "Food"),
    (751110, "Food"),
    (751111, "Food"),
    (751112, "Food"),
    (751118, "Food"),
    (751122, "Food"),
    (751123, "Food"),
    (751127, "Food"),
    (751128, "Food"),
    (751132, "Food"),
    (751133, "Food"),
    (751134, "Food"),
    (752001, "Food"),
    (752003, "Food"),
    (752021, "Food"),
    (754004, "Food"),
    (754062, "Food"),
    (755015, "Food"),
    (756512, "Food"),
    (756513, "Food"),
    (756518, "Food"),
    (756601, "Food"),
    (756604, "Food"),
    (756701, "Food"),
    (756703, "Food"),
    (756802, "Food"),
    (756803, "Food"),
    (756804, "Food"),
    (756901, "Food"),
    (757002, "Food"),
    (757004, "Food"),
    (757006, "Food"),
    (757007, "Food"),
    (757009, "Food"),
    (757010, "Food"),
    (757011, "Food"),
    (757012, "Food"),
    (757013, "Food"),
    (1000015, "Food"),
    (1000020, "Food"),
    (1000025, "Food"),
    (1000028, "Potion"),
    (1000029, "Food"),
    (1000030, "Potion"),
    (1000034, "Potion"),
    (1000035, "Food"),
    (1000042, "Food"),
    (1000055, "Food"),
    (1000056, "Food"),
    (1000070, "Arrow"),
    (1000075, "Food"),
    (1000079, "Food"),
    (1000085, "Potion"),
    (1000112, "Potion"),
    (1000119, "Food"),
    (1000120, "Food"),
    (1000126, "Food"),
    (1000142, "Food"),
    (1000144, "Food"),
    (1000145, "Food"),
    (1000147, "Food"),
    (1000172, "Food"),
    (1000174, "Potion"),
    (1000175, "Potion"),
    (1000177, "Food"),
    (1000185, "Food"),
    (1000213, "Food"),
    (1000222, "Food"),
    (1000223, "Food"),
    (1000226, "Food"),
    (1000227, "Food"),
    (1000228, "Potion"),
    (1000229, "Potion"),
    (1000230, "Potion"),
    (1000254, "Food"),
    (1000304, "Potion"),
    (1000320, "Potion"),
    (1000377, "Potion"),
    (1000392, "Potion"),
    (1000396, "Food"),
    (1000397, "Food"),
    (1000398, "Food"),
    (1000400, "Food"),
    (1000431, "Food"),
    (1000448, "Potion"),
    (1000454, "Food"),
    (1000463, "Food"),
    (1000469, "Food"),
    (1000470, "Food"),
    (1000476, "Healing"),
    (1000518, "Food"),
    (1000561, "Food"),
    (1000564, "Food"),
    (1000565, "Food"),
    (1000579, "Food"),
    (1000582, "Food"),
    (1000583, "Food"),
    (1000623, "Food"),
    (1000633, "Food"),
    (1000642, "Food"),
    (1000644, "Food"),
    (1000645, "Food"),
    (1000649, "Food"),
    (1000650, "Food"),
    (1000651, "Food"),
    (1000652, "Food"),
    (1000653, "Food"),
    (1000654, "Food"),
    (1000655, "Food"),
    (1000656, "Food"),
    (1000663, "Food"),
    (1000669, "Food"),
    (1000680, "Food"),
    (1000684, "Food"),
    (1000694, "Food"),
    (1000709, "Food"),
    (1000717, "Food"),
    (1000730, "Food"),
    (1000750, "Food"),
    (1000754, "Food"),
    (1000797, "Food"),
    (1000804, "Food"),
    (1000807, "Food"),
    (1000875, "Food"),
    (1000883, "Food"),
    (1000963, "Food"),
    (1000964, "Food"),
    (1000965, "Food"),
    (1000966, "Food"),
    (1000974, "Food"),
    (1000991, "Food"),
    (1001003, "Food"),
    (1001004, "Food"),
    (1001005, "Food"),
    (1001006, "Food"),
    (1001007, "Food"),
    (1001008, "Food"),
    (1001065, "Food"),
    (1001134, "Food"),
    (1001135, "Food"),
    (1001154, "Food"),
    (1001155, "Food"),
    (1001156, "Food"),
    (1001157, "Food"),
    (1001158, "Food"),
    (1001159, "Food"),
    (1001160, "Food"),
    (1001161, "Food"),
    (1001165, "Food"),
    (1001187, "Food"),
    (1001190, "Food"),
    (1001198, "Food"),
    (1001213, "Food"),
    (1001214, "Food"),
    (1001215, "Food"),
    (1001216, "Food"),
    (1001217, "Food"),
    (1001218, "Food"),
    (1001219, "Food"),
    (1001220, "Food"),
    (1001221, "Food"),
    (1001222, "Food"),
    (1001223, "Food"),
    (1001224, "Food"),
    (1001225, "Food"),
    (1001226, "Food"),
    (1001227, "Food"),
    (1001228, "Food"),
    (1001229, "Food"),
    (1001230, "Food"),
    (1001231, "Food"),
    (1001233, "Food"),
    (1001237, "Food"),
    (1001238, "Food"),
    (1001239, "Food"),
    (1001241, "Food"),
    (1001242, "Food"),
    (1001243, "Food"),
    (1001244, "Food"),
    (1001245, "Food"),
    (1001253, "Food"),
    (1001254, "Food"),
    (1001292, "Food"),
    (1001307, "Food"),
    (1001312, "Food"),
    (1001314, "Arrow"),
    (1001315, "Arrow"),
    (1001316, "Arrow"),
    (1001321, "Arrow"),
    (1001323, "Food"),
    (1001330, "Food"),
    (1001331, "Food"),
    (1001332, "Food"),
    (1001333, "Food"),
    (1001334, "Food"),
    (1001421, "Food"),
    (1001566, "Food"),
    (1001567, "Food"),
    (1001568, "Food"),
    (1001569, "Food"),
    (1001570, "Food"),
    (1001668, "Food"),
    (1001670, "Food"),
    (1001671, "Food"),
    (1001672, "Food"),
    (1001673, "Food"),
    (1001674, "Food"),
    (1001675, "Food"),
    (1001676, "Food"),
    (1001677, "Food"),
    (1001705, "Food"),
    (1001706, "Food"),
    (1001741, "Food"),
    (1001748, "Food"),
    (1001794, "Food"),
    (1001808, "Food"),
    (1001811, "Food"),
    (1001817, "Food"),
    (1001843, "Food"),
    (1001859, "Food"),
    (1001860, "Food"),
    (1001861, "Food"),
    (1001877, "Food"),
    (1001882, "Food"),
    (1001892, "Food"),
    (1001913, "Food"),
    (1001914, "Food"),
    (1001915, "Food"),
    (1001917, "Food"),
    (1001918, "Food"),
    (1001919, "Food"),
    (1001920, "Food"),
    (1001921, "Food"),
    (1001928, "Food"),
    (1001929, "Food"),
    (1001930, "Food"),
    (1001931, "Food"),
    (1001932, "Food"),
    (1001933, "Food"),
    (1001957, "Food"),
    (1002050, "Food"),
    (1002051, "Food"),
    (1002052, "Food"),
    (1002053, "Food"),
    (1002054, "Food"),
    (1002055, "Food"),
    (1002056, "Food"),
    (1002057, "Food"),
    (1002058, "Food"),
    (1002059, "Food"),
    (1002060, "Food"),
    (1002061, "Food"),
    (1002062, "Food"),
    (1002064, "Food"),
    (1002065, "Food"),
    (1002066, "Food"),
    (1002067, "Food"),
    (1002068, "Food"),
    (1002069, "Food"),
    (1002070, "Food"),
    (1002071, "Food"),
    (1002072, "Food"),
    (1002073, "Food"),
    (1002074, "Food"),
    (1002075, "Food"),
    (1002076, "Food"),
    (1002080, "Food"),
    (1002081, "Food"),
    (1002082, "Food"),
    (1002083, "Food"),
    (1002084, "Food"),
    (1002085, "Food"),
    (1002090, "Food"),
    (1002095, "Food"),
    (1002110, "Food"),
    (1002495, "Food"),
    (1002656, "Food"),
    (1002657, "Food"),
    (1002880, "Food"),
    (1002881, "Food"),
    (1002883, "Food"),
    (1002884, "Food"),
    (1002885, "Food"),
    (1002886, "Food"),
    (1002892, "Food"),
    (1002893, "Food"),
    (1002894, "Food"),
    (1003232, "Food"),
    (1003233, "Food"),
    (1003234, "Food"),
    (1003235, "Food"),
    (1003236, "Food"),
    (1003237, "Food"),
    (1003238, "Food"),
    (1003239, "Food"),
    (1003240, "Food"),
    (1003241, "Food"),
    (1003242, "Food"),
    (1003243, "Food"),
    (1003362, "Food"),
    (1003364, "Food"),
    (1003365, "Food"),
    (1003366, "Food"),
    (1003367, "Food"),
    (1003368, "Food"),
    (1003369, "Food"),
    (1003370, "Food"),
    (1003371, "Food"),
    (1003372, "Food"),
    (1003373, "Food"),
    (1003374, "Food"),
    (1003375, "Food"),
    (1003376, "Food"),
    (1003377, "Food"),
    (1003378, "Food"),
    (1003379, "Food"),
    (1003380, "Food"),
    (1003381, "Food"),
    (1003382, "Food"),
    (1003383, "Food"),
    (1003384, "Food"),
    (1003385, "Food"),
    (1003386, "Food"),
    (1003387, "Food"),
    (1003388, "Food"),
    (1003389, "Food"),
    (1003390, "Food"),
    (1003391, "Food"),
    (1003392, "Food"),
    (1003393, "Food"),
    (1003394, "Food"),
    (1003395, "Food"),
    (1003396, "Food"),
    (1003397, "Food"),
    (1003398, "Food"),
    (1003399, "Food"),
    (1003400, "Food"),
    (1003401, "Food"),
    (1003402, "Food"),
    (1003403, "Food"),
    (1003404, "Food"),
    (1003405, "Food"),
    (1003406, "Food"),
    (1003407, "Food"),
    (1003408, "Food"),
    (1003409, "Food"),
    (1003410, "Food"),
    (1003411, "Food"),
    (1003412, "Food"),
    (1003413, "Food"),
    (1003414, "Food"),
    (1003415, "Food"),
    (1003416, "Food"),
    (1003417, "Food"),
    (1003418, "Food"),
    (1003419, "Food"),
    (1003420, "Food"),
    (1003421, "Food"),
    (1003422, "Food"),
    (1003423, "Food"),
    (1003424, "Food"),
    (1003425, "Food"),
    (1003426, "Food"),
    (1003427, "Food"),
    (1003428, "Food"),
    (1003429, "Food"),
    (1003430, "Food"),
    (1003431, "Food"),
    (1003432, "Food"),
    (1003433, "Food"),
    (1003434, "Food"),
    (1003435, "Food"),
    (1003436, "Food"),
    (1003437, "Food"),
    (1003438, "Food"),
    (1003439, "Food"),
    (1003440, "Food"),
    (1003441, "Food"),
    (1003442, "Food"),
    (1003443, "Food"),
    (1003444, "Food"),
    (1003445, "Food"),
    (1003446, "Food"),
    (1003447, "Food"),
    (1003448, "Food"),
    (1003449, "Food"),
    (1003450, "Food"),
    (1003451, "Food"),
    (1003452, "Food"),
    (1003453, "Food"),
    (1003454, "Food"),
    (1003455, "Food"),
    (1003456, "Food"),
    (1003457, "Food"),
    (1003458, "Food"),
    (1003459, "Food"),
    (1003460, "Food"),
    (1003461, "Food"),
    (1003462, "Food"),
    (1003463, "Food"),
    (1003464, "Food"),
    (1003465, "Food"),
    (1003466, "Food"),
    (1003467, "Food"),
    (1003468, "Food"),
    (1003469, "Food"),
    (1003470, "Food"),
    (1003471, "Food"),
    (1003472, "Food"),
    (1003473, "Food"),
    (1003474, "Food"),
    (1003475, "Food"),
    (1003476, "Food"),
    (1003477, "Food"),
    (1003478, "Food"),
    (1003479, "Food"),
    (1003480, "Food"),
    (1003481, "Food"),
    (1003482, "Food"),
    (1003483, "Food"),
    (1003484, "Food"),
    (1003485, "Food"),
    (1003486, "Food"),
    (1003487, "Food"),
    (1003488, "Food"),
    (1003489, "Food"),
    (1003490, "Food"),
    (1003491, "Food"),
    (1003492, "Food"),
    (1003493, "Food"),
    (1003494, "Food"),
    (1003495, "Food"),
    (1003496, "Food"),
    (1003497, "Food"),
    (1003498, "Food"),
    (1003499, "Food"),
    (1003500, "Food"),
    (1003501, "Food"),
    (1003502, "Food"),
    (1003503, "Food"),
    (1003504, "Food"),
    (1003505, "Food"),
    (1003506, "Food"),
    (1003507, "Food"),
    (1003508, "Food"),
    (1003509, "Food"),
    (1003510, "Food"),
    (1003511, "Food"),
    (1003512, "Food"),
    (1003513, "Food"),
    (1003514, "Food"),
    (1003515, "Food"),
    (1003516, "Food"),
    (1003517, "Food"),
    (1003518, "Food"),
    (1003519, "Food"),
    (1003520, "Food"),
    (1003521, "Food"),
    (1003522, "Food"),
    (1003523, "Food"),
    (1003524, "Food"),
    (1003525, "Food"),
    (1003526, "Food"),
    (1003527, "Food"),
    (1003528, "Food"),
    (1003529, "Food"),
    (1003530, "Food"),
    (1003531, "Food"),
    (1003532, "Food"),
    (1003533, "Food"),
    (1003534, "Food"),
    (1003535, "Food"),
    (1003536, "Food"),
    (1003537, "Food"),
    (1003538, "Food"),
    (1003539, "Food"),
    (1003540, "Food"),
    (1003541, "Food"),
    (1003542, "Food"),
    (1003543, "Food"),
    (1003544, "Food"),
];
static NEXT: AtomicU64 = AtomicU64::new(0);
#[derive(Debug, Serialize)]
pub struct Choice {
    pub key: u32,
    pub name: String,
    pub kind: String,
}
#[derive(Debug, Serialize)]
pub struct Snapshot {
    pub item_keys: Vec<u32>,
    pub choices: Vec<Choice>,
    pub game_running: bool,
    pub can_change: bool,
    pub message: String,
}
fn invalid() -> io::Error {
    io::Error::new(
        io::ErrorKind::InvalidData,
        "Invalid selected-consumable settings.",
    )
}
fn decode(bytes: &[u8]) -> io::Result<Vec<u32>> {
    if bytes.len() < 8 || &bytes[..4] != b"CWC1" {
        return Err(invalid());
    }
    let count = u32::from_le_bytes(bytes[4..8].try_into().unwrap()) as usize;
    if count > ITEMS.len() || bytes.len() != 8 + count * 4 {
        return Err(invalid());
    }
    let keys: Vec<u32> = bytes[8..]
        .chunks_exact(4)
        .map(|c| u32::from_le_bytes(c.try_into().unwrap()))
        .collect();
    if keys.iter().any(|k| !ITEMS.iter().any(|i| i.0 == *k))
        || keys.windows(2).any(|w| w[0] >= w[1])
    {
        return Err(invalid());
    }
    Ok(keys)
}
fn encode(keys: &[u32]) -> io::Result<Vec<u8>> {
    if keys.len() > ITEMS.len() {
        return Err(invalid());
    }
    let mut sorted = keys.to_vec();
    sorted.sort_unstable();
    let mut bytes = b"CWC1".to_vec();
    bytes.extend((sorted.len() as u32).to_le_bytes());
    for key in sorted {
        bytes.extend(key.to_le_bytes());
    }
    decode(&bytes)?;
    Ok(bytes)
}
fn load(directory: &Path) -> io::Result<Vec<u32>> {
    let path = directory.join(FILE_NAME);
    let meta = match fs::symlink_metadata(&path) {
        Ok(meta) => meta,
        Err(e) if e.kind() == io::ErrorKind::NotFound => return Ok(vec![]),
        Err(e) => return Err(e),
    };
    super::ordinary_settings_file(&path, &meta)?;
    let mut bytes = Vec::new();
    fs::File::open(path)?
        .take((9 + ITEMS.len() * 4) as u64)
        .read_to_end(&mut bytes)?;
    decode(&bytes)
}
pub fn snapshot(
    directory: &Path,
    version: Option<&str>,
    choices: Vec<Choice>,
) -> io::Result<Snapshot> {
    let running = crate::apply::guard::game_running()?;
    let (keys, invalid) = match load(directory) {
        Ok(keys) => (keys, false),
        Err(e) if e.kind() == io::ErrorKind::InvalidData => (vec![], true),
        Err(e) => return Err(e),
    };
    Ok(Snapshot {
        item_keys: keys,
        choices,
        game_running: running,
        can_change: !running && version == Some("1.0.0.2976"),
        message: if invalid {
            "Saved selection is invalid. No items are protected until you save a new selection."
                .into()
        } else {
            "Selection loads at game startup when the consumable mod is installed.".into()
        },
    })
}
pub fn save(
    directory: &Path,
    version: Option<&str>,
    keys: &[u32],
    choices: Vec<Choice>,
) -> io::Result<Snapshot> {
    if version != Some("1.0.0.2976") {
        return Err(io::Error::other(
            "Consumable protection requires game version 1.0.0.2976.",
        ));
    }
    store(directory, keys, crate::apply::guard::game_running)?;
    snapshot(directory, version, choices)
}
fn store(
    directory: &Path,
    keys: &[u32],
    mut running: impl FnMut() -> io::Result<bool>,
) -> io::Result<()> {
    let bytes = encode(keys)?;
    let stopped = |running: &mut dyn FnMut() -> io::Result<bool>| -> io::Result<()> {
        if running()? {
            Err(io::Error::other(
                "Close Crimson Desert before saving consumable protection settings.",
            ))
        } else {
            Ok(())
        }
    };
    stopped(&mut running)?;
    fs::create_dir_all(directory)?;
    let destination = directory.join(FILE_NAME);
    match fs::symlink_metadata(&destination) {
        Ok(meta) => super::ordinary_settings_file(&destination, &meta)?,
        Err(e) if e.kind() == io::ErrorKind::NotFound => {}
        Err(e) => return Err(e),
    }
    let temporary = directory.join(format!(
        ".consumables-{}-{}.tmp",
        std::process::id(),
        NEXT.fetch_add(1, Ordering::Relaxed)
    ));
    let mut file = fs::OpenOptions::new()
        .write(true)
        .create_new(true)
        .open(&temporary)?;
    let result = (|| {
        file.write_all(&bytes)?;
        file.sync_all()?;
        drop(file);
        stopped(&mut running)?;
        #[cfg(windows)]
        {
            use std::os::windows::ffi::OsStrExt;
            use windows_sys::Win32::Storage::FileSystem::{
                MOVEFILE_REPLACE_EXISTING, MOVEFILE_WRITE_THROUGH, MoveFileExW,
            };
            let from: Vec<u16> = temporary.as_os_str().encode_wide().chain(Some(0)).collect();
            let to: Vec<u16> = destination
                .as_os_str()
                .encode_wide()
                .chain(Some(0))
                .collect();
            if unsafe {
                MoveFileExW(
                    from.as_ptr(),
                    to.as_ptr(),
                    MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH,
                )
            } == 0
            {
                return Err(io::Error::last_os_error());
            }
        }
        #[cfg(not(windows))]
        {
            fs::rename(&temporary, &destination)?;
            fs::File::open(directory)?.sync_all()?;
        }
        if load(directory)? != decode(&bytes)? {
            return Err(io::Error::other(
                "Saved consumable selection did not verify.",
            ));
        }
        Ok(())
    })();
    if result.is_err() {
        let _ = fs::remove_file(&temporary);
    }
    result
}
#[cfg(test)]
mod tests {
    use super::*;
    #[test]
    fn food_healing_and_legacy_selection_share_the_same_bounded_codec() {
        for (kind, expected) in [("Arrow", 9), ("Potion", 15), ("Healing", 2), ("Food", 535)] {
            assert_eq!(ITEMS.iter().filter(|item| item.1 == kind).count(), expected);
        }
        assert!(ITEMS.contains(&(22009, "Healing")));
        assert!(ITEMS.contains(&(1000476, "Healing")));
        assert!(ITEMS.contains(&(10033, "Food")));
        let old_profile = [
            b'C', b'W', b'C', b'1', 2, 0, 0, 0, 152, 8, 0, 0, 98, 66, 15, 0,
        ];
        assert_eq!(decode(&old_profile).unwrap(), vec![2200, 1000034]);
        let all: Vec<_> = ITEMS.iter().map(|item| item.0).collect();
        let encoded = encode(&all).unwrap();
        assert_eq!(encoded.len(), 8 + 561 * 4);
        assert_eq!(decode(&encoded).unwrap(), all);
        assert_eq!(
            decode(&encode(&[10033, 22009, 1000476]).unwrap()).unwrap(),
            vec![10033, 22009, 1000476]
        );
    }
    #[test]
    fn shared_native_key_scope_and_codec_are_exact() {
        let native =
            include_str!("../../../../runtime/marketplace/consumables/consumables_catalog.h");
        let pattern = regex::Regex::new(r"\{(\d+), Kind::(Arrow|Potion|Healing|Food),").unwrap();
        let native: Vec<_> = pattern
            .captures_iter(native)
            .map(|m| (m[1].parse::<u32>().unwrap(), m[2].to_owned()))
            .collect();
        assert_eq!(
            native,
            ITEMS
                .iter()
                .map(|i| (i.0, i.1.to_owned()))
                .collect::<Vec<_>>()
        );
        for keys in [
            vec![],
            vec![2200],
            vec![1000034, 2200],
            ITEMS.iter().map(|i| i.0).collect(),
        ] {
            let mut expected = keys.clone();
            expected.sort_unstable();
            assert_eq!(decode(&encode(&keys).unwrap()).unwrap(), expected);
        }
        for keys in [vec![1], vec![2200, 2200], vec![1000034; ITEMS.len() + 1]] {
            assert!(encode(&keys).is_err());
        }
        let mut bytes = encode(&[2200, 1000034]).unwrap();
        bytes[8..12].copy_from_slice(&1000034u32.to_le_bytes());
        assert!(decode(&bytes).is_err());
        assert!(decode(b"CWC1\x19\0\0\0").is_err());
        assert!(decode(b"CWC1\0\0\0\0x").is_err());
    }
    #[test]
    fn atomic_selection_preserves_other_settings_and_refuses_running_or_unknown_state() {
        let root = tempfile::tempdir().unwrap();
        let directory = root.path().join("friend's üser & settings");
        assert!(load(&directory).unwrap().is_empty());
        assert!(store(&directory, &[2200], || Ok(true)).is_err());
        assert!(!directory.exists());
        assert!(store(&directory, &[2200], || Err(io::Error::other("unknown"))).is_err());
        assert!(!directory.exists());
        store(&directory, &[1000034, 2200], || Ok(false)).unwrap();
        fs::write(directory.join("other.dat"), b"unchanged").unwrap();
        let mut calls = 0;
        assert!(
            store(&directory, &[], || {
                calls += 1;
                Ok(calls == 2)
            })
            .is_err()
        );
        assert_eq!(load(&directory).unwrap(), vec![2200, 1000034]);
        store(&directory, &[], || Ok(false)).unwrap();
        assert!(load(&directory).unwrap().is_empty());
        assert_eq!(fs::read(directory.join("other.dat")).unwrap(), b"unchanged");
        assert_eq!(fs::read_dir(directory).unwrap().count(), 2);
    }
}
