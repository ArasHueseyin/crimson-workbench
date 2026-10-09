//! Transactional local item index. This module never opens game archives or saves.
//!
//! [`Index::open`] is a low-level storage operation: the caller MUST validate the
//! destination against the application's local cache directory and protected
//! game/save roots before calling it. SQLite can create journal files beside the
//! database, so the parent directory must also be an approved writable location.

use std::{path::Path, time::Duration};

use rusqlite::{Connection, OpenFlags, OptionalExtension, params, params_from_iter, types::Value};
use serde::{Deserialize, Serialize};
use thiserror::Error;

const APPLICATION_ID: i64 = 0x4357_4958; // CWIX
const STORAGE_VERSION: &str = "1";
const MAX_PAGE_SIZE: u32 = 200;
const MAX_QUERY_CHARS: usize = 512;
const MAX_QUERY_TERMS: usize = 32;

#[derive(Debug, Clone, PartialEq, Eq, Serialize, Deserialize)]
pub struct IndexedItem {
    pub key: u32,
    pub internal_key: String,
    pub name: String,
    pub description: String,
    pub item_type: i64,
    pub category: Option<String>,
    pub max_stack_count: u64,
    pub detail_json: String,
}

#[derive(Debug, Clone, PartialEq, Eq, Serialize, Deserialize)]
pub struct IndexIdentity {
    pub fingerprint: String,
    pub language: String,
    pub schema_version: String,
}

#[derive(Debug, Clone, PartialEq, Eq, Serialize, Deserialize)]
pub struct SearchQuery {
    pub text: String,
    pub limit: u32,
    pub offset: u32,
    pub item_type: Option<i64>,
    pub category: Option<String>,
}

impl Default for SearchQuery {
    fn default() -> Self {
        Self {
            text: String::new(),
            limit: 50,
            offset: 0,
            item_type: None,
            category: None,
        }
    }
}

#[derive(Debug, Clone, PartialEq, Eq, Serialize, Deserialize)]
pub struct SearchPage {
    pub items: Vec<IndexedItem>,
    pub total: u64,
    /// Effective page size, clamped to 1..=200.
    pub limit: u32,
    pub offset: u32,
}

#[derive(Debug, Error)]
pub enum IndexError {
    #[error("SQLite index error: {0}")]
    Sqlite(#[from] rusqlite::Error),
    #[error("Refusing to use an unrelated SQLite database (application ID {found:#x})")]
    ForeignDatabase { found: i64 },
    #[error("Unsupported local index storage version {found:?}; expected {STORAGE_VERSION}")]
    UnsupportedStorageVersion { found: String },
    #[error("Index identity field {field} must not be blank")]
    EmptyIdentity { field: &'static str },
    #[error("Item {key} has invalid detail JSON: {source}")]
    InvalidDetail {
        key: u32,
        #[source]
        source: serde_json::Error,
    },
    #[error("Search is limited to {MAX_QUERY_CHARS} characters and {MAX_QUERY_TERMS} words")]
    QueryTooLong,
}

pub type Result<T> = std::result::Result<T, IndexError>;

pub struct Index {
    connection: Connection,
}

impl Index {
    /// Open/create a Workbench index at an ALREADY VALIDATED local cache path.
    ///
    /// The application path guard must reject game/save paths, symlinks or
    /// junctions into protected roots, and any unapproved output directory before
    /// this call. This method writes SQLite files and is not a read-only opener.
    /// It does not create parent directories or migrate unrelated databases.
    pub fn open(path: &Path) -> Result<Self> {
        let connection = Connection::open_with_flags(
            path,
            OpenFlags::SQLITE_OPEN_READ_WRITE
                | OpenFlags::SQLITE_OPEN_CREATE
                | OpenFlags::SQLITE_OPEN_NO_MUTEX,
        )?;
        Self::initialize(connection)
    }

    #[cfg(test)]
    pub fn in_memory() -> Result<Self> {
        Self::initialize(Connection::open_in_memory()?)
    }

    fn initialize(mut connection: Connection) -> Result<Self> {
        connection.busy_timeout(Duration::from_secs(5))?;
        let application_id: i64 =
            connection.pragma_query_value(None, "application_id", |row| row.get(0))?;
        if application_id == 0 {
            let existing_objects: i64 = connection.query_row(
                "SELECT COUNT(*) FROM sqlite_schema WHERE name NOT LIKE 'sqlite_%'",
                [],
                |row| row.get(0),
            )?;
            if existing_objects != 0 {
                return Err(IndexError::ForeignDatabase { found: 0 });
            }
            let transaction = connection.transaction()?;
            transaction.execute_batch(
                "CREATE TABLE index_storage (
                    singleton INTEGER PRIMARY KEY CHECK(singleton = 1),
                    version TEXT NOT NULL
                );
                CREATE TABLE index_identity (
                    singleton INTEGER PRIMARY KEY CHECK(singleton = 1),
                    fingerprint TEXT NOT NULL,
                    language TEXT NOT NULL,
                    schema_version TEXT NOT NULL
                );
                CREATE TABLE items (
                    key INTEGER PRIMARY KEY CHECK(key BETWEEN 0 AND 4294967295),
                    internal_key TEXT NOT NULL,
                    name TEXT NOT NULL,
                    description TEXT NOT NULL,
                    item_type INTEGER NOT NULL,
                    category TEXT,
                    max_stack_count TEXT NOT NULL,
                    detail_json TEXT NOT NULL
                );
                CREATE INDEX items_type ON items(item_type);
                CREATE INDEX items_category ON items(category);
                CREATE VIRTUAL TABLE items_fts USING fts5(
                    internal_key, name, description,
                    content='items', content_rowid='key',
                    tokenize='unicode61 remove_diacritics 2'
                );",
            )?;
            transaction.execute(
                "INSERT INTO index_storage(singleton, version) VALUES(1, ?1)",
                [STORAGE_VERSION],
            )?;
            transaction.pragma_update(None, "application_id", APPLICATION_ID)?;
            transaction.commit()?;
        } else if application_id != APPLICATION_ID {
            return Err(IndexError::ForeignDatabase {
                found: application_id,
            });
        }
        let version: String = connection.query_row(
            "SELECT version FROM index_storage WHERE singleton = 1",
            [],
            |row| row.get(0),
        )?;
        if version != STORAGE_VERSION {
            return Err(IndexError::UnsupportedStorageVersion { found: version });
        }
        Ok(Self { connection })
    }

    /// Replace the complete index iff build, language, or parser schema changed.
    ///
    /// `fingerprint` must include the hashes of every data input used by the
    /// caller. A matching identity deliberately does not inspect the supplied
    /// records. Items, FTS data and identity commit in one transaction; any
    /// validation/SQLite failure leaves the previous index usable.
    pub fn rebuild_if_needed(
        &mut self,
        identity: &IndexIdentity,
        items: &[IndexedItem],
    ) -> Result<bool> {
        for (field, value) in [
            ("fingerprint", &identity.fingerprint),
            ("language", &identity.language),
            ("schema_version", &identity.schema_version),
        ] {
            if value.trim().is_empty() {
                return Err(IndexError::EmptyIdentity { field });
            }
        }
        let transaction = self.connection.transaction()?;
        let current = transaction
            .query_row(
                "SELECT fingerprint, language, schema_version FROM index_identity WHERE singleton = 1",
                [],
                |row| {
                    Ok(IndexIdentity {
                        fingerprint: row.get(0)?,
                        language: row.get(1)?,
                        schema_version: row.get(2)?,
                    })
                },
            )
            .optional()?;
        if current.as_ref() == Some(identity) {
            transaction.commit()?;
            return Ok(false);
        }
        transaction.execute("DELETE FROM items", [])?;
        {
            let mut insert = transaction.prepare(
                "INSERT INTO items(key, internal_key, name, description, item_type,
                                   category, max_stack_count, detail_json)
                 VALUES(?1, ?2, ?3, ?4, ?5, ?6, ?7, ?8)",
            )?;
            for item in items {
                serde_json::from_str::<serde_json::Value>(&item.detail_json).map_err(|source| {
                    IndexError::InvalidDetail {
                        key: item.key,
                        source,
                    }
                })?;
                // SQLite INTEGER is signed. Decimal TEXT retains all u64 values.
                insert.execute(params![
                    i64::from(item.key),
                    item.internal_key,
                    item.name,
                    item.description,
                    item.item_type,
                    item.category,
                    item.max_stack_count.to_string(),
                    item.detail_json,
                ])?;
            }
        }
        transaction.execute("INSERT INTO items_fts(items_fts) VALUES('rebuild')", [])?;
        // rank=1 also compares the FTS index against the external content table.
        transaction.execute(
            "INSERT INTO items_fts(items_fts, rank) VALUES('integrity-check', 1)",
            [],
        )?;
        transaction.execute(
            "INSERT INTO index_identity(singleton, fingerprint, language, schema_version)
             VALUES(1, ?1, ?2, ?3)
             ON CONFLICT(singleton) DO UPDATE SET fingerprint=excluded.fingerprint,
                 language=excluded.language, schema_version=excluded.schema_version",
            params![
                identity.fingerprint,
                identity.language,
                identity.schema_version
            ],
        )?;
        transaction.commit()?;
        Ok(true)
    }

    /// Search literal whitespace-separated terms across names, descriptions and
    /// internal keys. Terms are ANDed; FTS operators, quotes, `*` and SQL text are
    /// never executed as query syntax. Empty text lists all filtered records.
    /// Ordering is name (SQLite NOCASE), internal key, then numeric item key.
    pub fn search(&self, query: SearchQuery) -> Result<SearchPage> {
        let fts_query = literal_fts_query(&query.text)?;
        let limit = query.limit.clamp(1, MAX_PAGE_SIZE);
        let mut conditions = Vec::new();
        let mut values = Vec::<Value>::new();
        if let Some(text) = fts_query {
            conditions.push("i.key IN (SELECT rowid FROM items_fts WHERE items_fts MATCH ?)");
            values.push(Value::Text(text));
        }
        if let Some(item_type) = query.item_type {
            conditions.push("i.item_type = ?");
            values.push(Value::Integer(item_type));
        }
        if let Some(category) = query.category {
            conditions.push("i.category = ?");
            values.push(Value::Text(category));
        }
        let predicate = if conditions.is_empty() {
            "1 = 1".to_owned()
        } else {
            conditions.join(" AND ")
        };
        // Count and page share a read snapshot if another process rebuilds.
        let transaction = self.connection.unchecked_transaction()?;
        let total: i64 = transaction.query_row(
            &format!("SELECT COUNT(*) FROM items i WHERE {predicate}"),
            params_from_iter(values.iter()),
            |row| row.get(0),
        )?;
        values.push(Value::Integer(i64::from(limit)));
        values.push(Value::Integer(i64::from(query.offset)));
        let items = {
            let mut statement = transaction.prepare(&format!(
                "SELECT i.key, i.internal_key, i.name, i.description, i.item_type,
                        i.category, i.max_stack_count, i.detail_json
                 FROM items i WHERE {predicate}
                 ORDER BY i.name COLLATE NOCASE, i.internal_key, i.key
                 LIMIT ? OFFSET ?"
            ))?;
            let rows = statement.query_map(params_from_iter(values.iter()), read_item)?;
            rows.collect::<std::result::Result<Vec<_>, _>>()?
        };
        transaction.commit()?;
        Ok(SearchPage {
            items,
            total: total as u64,
            limit,
            offset: query.offset,
        })
    }

    pub fn get(&self, key: u32) -> Result<Option<IndexedItem>> {
        self.connection
            .query_row(
                "SELECT key, internal_key, name, description, item_type,
                        category, max_stack_count, detail_json FROM items WHERE key = ?1",
                [i64::from(key)],
                read_item,
            )
            .optional()
            .map_err(IndexError::from)
    }
}

fn literal_fts_query(text: &str) -> Result<Option<String>> {
    if text.chars().count() > MAX_QUERY_CHARS {
        return Err(IndexError::QueryTooLong);
    }
    // Embedded NUL/control bytes must never reach SQLite's FTS query parser.
    let normalized: String = text
        .chars()
        .map(|ch| if ch.is_control() { ' ' } else { ch })
        .collect();
    let terms: Vec<_> = normalized.split_whitespace().collect();
    if terms.len() > MAX_QUERY_TERMS {
        return Err(IndexError::QueryTooLong);
    }
    if terms.is_empty() {
        return Ok(None);
    }
    Ok(Some(
        terms
            .iter()
            .map(|term| format!("\"{}\"", term.replace('"', "\"\"")))
            .collect::<Vec<_>>()
            .join(" AND "),
    ))
}

fn read_item(row: &rusqlite::Row<'_>) -> rusqlite::Result<IndexedItem> {
    let raw_stack: String = row.get(6)?;
    let max_stack_count = raw_stack.parse::<u64>().map_err(|error| {
        rusqlite::Error::FromSqlConversionFailure(6, rusqlite::types::Type::Text, Box::new(error))
    })?;
    Ok(IndexedItem {
        key: row.get(0)?,
        internal_key: row.get(1)?,
        name: row.get(2)?,
        description: row.get(3)?,
        item_type: row.get(4)?,
        category: row.get(5)?,
        max_stack_count,
        detail_json: row.get(7)?,
    })
}

#[cfg(test)]
mod tests {
    use super::*;

    fn identity() -> IndexIdentity {
        IndexIdentity {
            fingerprint: "synthetic-table-sha256".into(),
            language: "ger".into(),
            schema_version: "items-v1".into(),
        }
    }

    fn item(key: u32, name: &str) -> IndexedItem {
        IndexedItem {
            key,
            internal_key: format!("Item_Test_{key}"),
            name: name.into(),
            description: String::new(),
            item_type: 1,
            category: Some("Nahrung".into()),
            max_stack_count: 99,
            detail_json: "{\"raw\":[]}".into(),
        }
    }

    fn search(text: &str) -> SearchQuery {
        SearchQuery {
            text: text.into(),
            ..SearchQuery::default()
        }
    }

    #[test]
    fn actual_fts5_indexes_german_names_descriptions_and_internal_keys() -> Result<()> {
        let mut index = Index::in_memory()?;
        let mut apple = item(1, "Äpfel aus Hernand");
        apple.description = "Frische Früchte für müde Reisende".into();
        let mut sword = item(2, "Schwert");
        sword.internal_key = "SwordDragonUnique".into();
        index.rebuild_if_needed(&identity(), &[apple.clone(), sword.clone()])?;
        assert_eq!(index.search(search("apfel"))?.items, vec![apple.clone()]);
        assert_eq!(index.search(search("fruchte reisende"))?.items, vec![apple]);
        assert_eq!(
            index.search(search("SwordDragonUnique"))?.items,
            vec![sword]
        );
        assert_eq!(index.search(search("unbekannt"))?.total, 0);
        Ok(())
    }

    #[test]
    fn identity_changes_rebuild_and_remove_old_fts_terms() -> Result<()> {
        let mut index = Index::in_memory()?;
        let mut id = identity();
        assert!(index.rebuild_if_needed(&id, &[item(1, "Alt")])?);
        assert!(!index.rebuild_if_needed(&id, &[item(2, "Ignored")])?);
        assert!(index.get(1)?.is_some());
        id.fingerprint = "updated-table-hash".into();
        assert!(index.rebuild_if_needed(&id, &[item(2, "Neu")])?);
        assert_eq!(index.search(search("Alt"))?.total, 0);
        assert_eq!(index.search(search("Neu"))?.total, 1);
        id.language = "eng".into();
        assert!(index.rebuild_if_needed(&id, &[item(2, "New")])?);
        id.schema_version = "items-v2".into();
        assert!(index.rebuild_if_needed(&id, &[])?);
        assert_eq!(index.search(SearchQuery::default())?.total, 0);
        assert!(!index.rebuild_if_needed(&id, &[])?);
        Ok(())
    }

    #[test]
    fn failed_insert_rolls_back_items_fts_and_identity() -> Result<()> {
        let mut index = Index::in_memory()?;
        let old_id = identity();
        let old_item = item(1, "Erhalten");
        index.rebuild_if_needed(&old_id, std::slice::from_ref(&old_item))?;
        let mut new_id = old_id.clone();
        new_id.fingerprint = "new-build".into();
        let duplicates = [item(2, "Neu"), item(2, "Doppelter Schlüssel")];
        assert!(matches!(
            index.rebuild_if_needed(&new_id, &duplicates),
            Err(IndexError::Sqlite(_))
        ));
        assert_eq!(index.get(1)?, Some(old_item.clone()));
        assert_eq!(index.search(search("Erhalten"))?.items, vec![old_item]);
        assert_eq!(index.search(search("Neu"))?.total, 0);
        assert!(!index.rebuild_if_needed(&old_id, &[])?);
        assert!(index.rebuild_if_needed(&new_id, &[item(2, "Neu")])?);
        Ok(())
    }

    #[test]
    fn invalid_json_after_an_insert_rolls_back_the_batch() -> Result<()> {
        let mut index = Index::in_memory()?;
        index.rebuild_if_needed(&identity(), &[item(8, "Vorher")])?;
        let mut updated = identity();
        updated.schema_version = "v2".into();
        let mut malformed = item(2, "Fehler");
        malformed.detail_json = "{broken".into();
        assert!(matches!(
            index.rebuild_if_needed(&updated, &[item(1, "Nachher"), malformed]),
            Err(IndexError::InvalidDetail { key: 2, .. })
        ));
        assert_eq!(index.search(search("Vorher"))?.total, 1);
        assert!(index.get(1)?.is_none());
        Ok(())
    }

    #[test]
    fn user_text_is_literal_and_never_fts_or_sql_syntax() -> Result<()> {
        let mut index = Index::in_memory()?;
        index.rebuild_if_needed(
            &identity(),
            &[
                item(1, "Apfel"),
                item(2, "Birne"),
                item(3, "Apfel OR Birne"),
            ],
        )?;
        assert_eq!(index.search(search("Apfel OR Birne"))?.items[0].key, 3);
        assert_eq!(index.search(search("Apfel OR Birne"))?.total, 1);
        for literal in [
            "\"",
            "***",
            "(Apfel",
            "name:Apfel",
            "NEAR(Apfel)",
            "' OR 1=1 --",
            "Apfel\"*",
        ] {
            index.search(search(literal))?;
        }
        assert_eq!(index.search(SearchQuery::default())?.total, 3);
        assert!(matches!(
            index.search(search(&"a".repeat(MAX_QUERY_CHARS + 1))),
            Err(IndexError::QueryTooLong)
        ));
        assert!(matches!(
            index.search(search(&vec!["a"; MAX_QUERY_TERMS + 1].join(" "))),
            Err(IndexError::QueryTooLong)
        ));
        Ok(())
    }

    #[test]
    fn pagination_filters_and_ties_are_deterministic() -> Result<()> {
        let mut index = Index::in_memory()?;
        let mut a = item(3, "Apfel");
        a.internal_key = "same".into();
        let mut b = item(1, "Apfel");
        b.internal_key = "same".into();
        let mut c = item(2, "Birne");
        c.item_type = 2;
        c.category = None;
        index.rebuild_if_needed(&identity(), &[a, c, b])?;
        let first = index.search(SearchQuery {
            limit: 1,
            ..SearchQuery::default()
        })?;
        let second = index.search(SearchQuery {
            limit: 1,
            offset: 1,
            ..SearchQuery::default()
        })?;
        assert_eq!(first.total, 3);
        assert_eq!(first.items[0].key, 1);
        assert_eq!(second.items[0].key, 3);
        assert_eq!(
            index
                .search(SearchQuery {
                    limit: 0,
                    ..SearchQuery::default()
                })?
                .limit,
            1
        );
        assert_eq!(
            index
                .search(SearchQuery {
                    limit: u32::MAX,
                    ..SearchQuery::default()
                })?
                .limit,
            200
        );
        let filtered = index.search(SearchQuery {
            text: "Apfel".into(),
            item_type: Some(1),
            category: Some("Nahrung".into()),
            ..SearchQuery::default()
        })?;
        assert_eq!(filtered.total, 2);
        assert_eq!(
            index
                .search(SearchQuery {
                    category: Some("' OR 1=1 --".into()),
                    ..SearchQuery::default()
                })?
                .total,
            0
        );
        assert_eq!(
            index
                .search(SearchQuery {
                    offset: u32::MAX,
                    ..SearchQuery::default()
                })?
                .items
                .len(),
            0
        );
        Ok(())
    }

    #[test]
    fn persisted_index_retains_full_u64_and_missing_records() -> Result<()> {
        let directory = tempfile::tempdir().unwrap();
        let path = directory.path().join("items.sqlite");
        let mut maximum = item(u32::MAX, "Übergröße");
        maximum.max_stack_count = u64::MAX;
        maximum.description.clear();
        maximum.category = None;
        maximum.detail_json = "{\"unicode\":\"größer\"}".into();
        {
            let mut index = Index::open(&path)?;
            index.rebuild_if_needed(&identity(), &[maximum.clone()])?;
        }
        let mut reopened = Index::open(&path)?;
        assert_eq!(reopened.get(u32::MAX)?, Some(maximum.clone()));

        assert_eq!(reopened.search(search("Übergröße"))?.items, vec![maximum]);
        assert!(reopened.get(99)?.is_none());
        assert!(!reopened.rebuild_if_needed(&identity(), &[])?);
        Ok(())
    }

    #[test]
    fn refuses_foreign_database_and_unknown_storage_version() -> Result<()> {
        let directory = tempfile::tempdir().unwrap();
        let path = directory.path().join("foreign.sqlite");
        {
            let foreign = Connection::open(&path)?;
            foreign.execute_batch(
                "CREATE TABLE unrelated(value TEXT); INSERT INTO unrelated VALUES('keep');",
            )?;
        }
        assert!(matches!(
            Index::open(&path),
            Err(IndexError::ForeignDatabase { .. })
        ));
        let foreign = Connection::open(&path)?;
        assert_eq!(
            foreign.query_row("SELECT value FROM unrelated", [], |row| row
                .get::<_, String>(0))?,
            "keep"
        );
        let own = Index::in_memory()?;
        own.connection
            .execute("UPDATE index_storage SET version='future'", [])?;
        // The initializer refuses a version it cannot read instead of guessing.
        assert!(matches!(
            Index::initialize(own.connection),
            Err(IndexError::UnsupportedStorageVersion { .. })
        ));
        Ok(())
    }

    #[test]
    fn blank_identity_is_rejected_without_replacing_existing_data() -> Result<()> {
        let mut index = Index::in_memory()?;
        index.rebuild_if_needed(&identity(), &[item(1, "Bleibt")])?;
        let mut invalid = identity();
        invalid.fingerprint = "  ".into();
        assert!(matches!(
            index.rebuild_if_needed(&invalid, &[]),
            Err(IndexError::EmptyIdentity {
                field: "fingerprint"
            })
        ));
        assert!(index.get(1)?.is_some());
        Ok(())
    }
}
