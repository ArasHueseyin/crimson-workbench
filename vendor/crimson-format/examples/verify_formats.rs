//! Explicit read-only real-data verifier. Requires CD_GAME_DIR; never silently skips.
use crimson_format::{Archive, ItemTable, Paloc};
use std::{env, fs, io};
fn main() -> io::Result<()> {
    let game = env::var("CD_GAME_DIR").map_err(|_| {
        io::Error::other("CD_GAME_DIR is required for this explicit real-data verifier")
    })?;
    let archive = Archive::open(&game)?;
    assert_eq!(
        archive.serialize_registry()?,
        fs::read(archive.root().join("meta/0.papgt"))?
    );
    assert_eq!(
        archive.serialize_group("0008")?,
        fs::read(archive.root().join("0008/0.pamt"))?
    );
    let entries = archive.list_group("0008")?;
    let read = |name: &str| -> io::Result<Vec<u8>> {
        let entry = entries
            .iter()
            .find(|e| e.directory == "gamedata/binarystaticinfo__/bin" && e.name == name)
            .ok_or_else(|| io::Error::other(format!("missing {name}")))?;
        archive.extract(entry)
    };
    let body = read("iteminfo.staticinfobody")?;
    let header = read("iteminfo.staticinfoheader")?;
    let table = ItemTable::parse(&body, &header)?;
    assert_eq!(table.serialize_body()?, body);
    assert_eq!(table.serialize_header()?, header);
    let mut fields = 0;
    for item in table.entries() {
        fields += table.fields(item.key)?.len();
    }
    let loc_entries = archive.list_group("0027")?;
    let loc_entry = loc_entries
        .iter()
        .find(|e| e.path == "gamedata/stringtable/binary__/ger/item.paloc")
        .ok_or_else(|| io::Error::other("German item namespace absent"))?;
    let loc_bytes = archive.extract(loc_entry)?;
    let loc = Paloc::parse(&loc_bytes)?;
    assert_eq!(loc.serialize()?, loc_bytes);
    assert_eq!(loc.serialize_payload()?, loc.decoded_payload());
    println!(
        "items={} body_bytes={} header_bytes={} fields={} paloc_entries={} paloc_wrapped={} registry_and_pamt_roundtrip=true",
        table.entries().len(),
        body.len(),
        header.len(),
        fields,
        loc.entries().len(),
        loc.is_wrapped()
    );
    Ok(())
}
