use crate::{Error, Result};
use rusqlite::{Connection, OpenFlags};
use serde::Serialize;
use std::{path::Path, time::Duration};

#[derive(Debug, Serialize)]
pub struct InstalledMod {
    pub id: i64,
    pub name: String,
    pub version: Option<String>,
    pub author: Option<String>,
    pub enabled: bool,
    pub applied: bool,
}

#[derive(Debug, Serialize)]
pub struct Snapshot {
    pub registry_found: bool,
    pub mods: Vec<InstalledMod>,
}

pub fn inspect(game: &Path) -> Result<Snapshot> {
    let path = game.join("CDMods/cdumm.db");
    if !path.try_exists()? {
        return Ok(Snapshot {
            registry_found: false,
            mods: Vec::new(),
        });
    }
    let read = || -> rusqlite::Result<Vec<InstalledMod>> {
        let connection = Connection::open_with_flags(
            &path,
            OpenFlags::SQLITE_OPEN_READ_ONLY | OpenFlags::SQLITE_OPEN_NO_MUTEX,
        )?;
        connection.busy_timeout(Duration::from_millis(250))?;
        connection.execute_batch("PRAGMA query_only=ON; PRAGMA trusted_schema=OFF;")?;
        let mut query = connection.prepare(
            "SELECT id, substr(name,1,256), substr(version,1,128), substr(author,1,256), enabled, applied FROM mods ORDER BY priority, id LIMIT 501"
        )?;
        query
            .query_map([], |row| {
                Ok(InstalledMod {
                    id: row.get(0)?,
                    name: row.get(1)?,
                    version: row.get(2)?,
                    author: row.get(3)?,
                    enabled: row.get(4)?,
                    applied: row.get(5)?,
                })
            })?
            .collect()
    };
    let mods =
        read().map_err(|e| Error::Invalid(format!("Could not read the CDMods registry: {e}")))?;
    if mods.len() > 500 {
        return Err(Error::Invalid(
            "CDMods registry exceeds the 500-mod display limit.".into(),
        ));
    }
    Ok(Snapshot {
        registry_found: true,
        mods,
    })
}

#[cfg(test)]
mod tests {
    use super::*;
    use rusqlite::Connection;

    #[test]
    fn registry_flags_are_distinct_and_inspection_preserves_database() {
        let root = tempfile::tempdir().unwrap();
        std::fs::create_dir(root.path().join("CDMods")).unwrap();
        let db = root.path().join("CDMods/cdumm.db");
        let connection = Connection::open(&db).unwrap();
        connection.execute_batch("CREATE TABLE mods (id INTEGER, name TEXT, version TEXT, author TEXT, priority INTEGER DEFAULT 0, enabled INTEGER, applied INTEGER);
            INSERT INTO mods (id,name,version,author,enabled,applied) VALUES (5,'Infinite Stamina Steam Mac Byte Patch','1.0.1','Local custom mod',1,1),
            (6,'Pending mod',NULL,NULL,1,0),(7,'Disabled mod',NULL,NULL,0,0);").unwrap();
        drop(connection);
        let before = std::fs::read(&db).unwrap();
        let snapshot = inspect(root.path()).unwrap();
        assert!(snapshot.registry_found);
        assert_eq!(snapshot.mods.len(), 3);
        let stamina = snapshot.mods.iter().find(|m| m.id == 5).unwrap();
        assert_eq!(stamina.name, "Infinite Stamina Steam Mac Byte Patch");
        assert!(stamina.enabled && stamina.applied);
        let pending = snapshot.mods.iter().find(|m| m.id == 6).unwrap();
        assert!(pending.enabled && !pending.applied);
        assert_eq!(std::fs::read(&db).unwrap(), before);
    }

    #[test]
    fn missing_registry_is_not_created_and_invalid_registry_is_an_error() {
        let root = tempfile::tempdir().unwrap();
        let snapshot = inspect(root.path()).unwrap();
        assert!(!snapshot.registry_found);
        assert!(snapshot.mods.is_empty());
        assert!(!root.path().join("CDMods").exists());
        std::fs::create_dir(root.path().join("CDMods")).unwrap();
        std::fs::write(root.path().join("CDMods/cdumm.db"), b"invalid sqlite").unwrap();
        assert!(inspect(root.path()).is_err());
    }
}
