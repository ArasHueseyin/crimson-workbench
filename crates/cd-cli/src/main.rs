use cd_core::{Error, Result, SearchQuery, Workspace};
use clap::{Parser, Subcommand};
use serde::Serialize;
use std::{
    io::{self, Read, Write},
    path::{Path, PathBuf},
    process::ExitCode,
};

#[derive(Parser)]
#[command(
    version,
    about = "Crimson Workbench – Spieldaten, Herstellung und Modvorschau",
    long_about = "Liest Spieldaten und berechnet Herstellung sowie Modvorschauen. Live-B0 benötigt eine ausdrücklich bestätigte Einrichtung, aktuelle Vorschau und beendetes Spiel. Save-Dateien werden niemals geschrieben."
)]
struct Cli {
    /// Projektordner mit .env; alle Ausgaben bleiben unter diesem Ordner.
    #[arg(long, global = true, default_value = ".")]
    project: PathBuf,
    /// Installation explizit wählen; sonst .env oder automatische Erkennung.
    #[arg(long, global = true)]
    game: Option<PathBuf>,
    /// Lokalisierung (ger/de, eng/en oder Code aus languages).
    #[arg(long, global = true, default_value = "ger")]
    language: String,
    #[command(subcommand)]
    command: Command,
}
#[derive(Subcommand)]
enum Command {
    /// Live-Einrichtung und Wiederherstellungsbedarf ausschließlich lesend prüfen.
    LiveStatus,
    /// Zusatzdateien prüfen; Originaldateien und fremde Registry bleiben geschützt.
    ForeignPreview,
    /// Genau geprüfte Zusatzdateien zum unveränderten Beibehalten bestätigen.
    ForeignConfirm {
        #[arg(long)]
        review_id: String,
        #[arg(long)]
        preserve: bool,
    },
    /// Aktuelle Bestätigung zurücknehmen, ohne Spieldateien zu verändern.
    ForeignRevoke {
        #[arg(long)]
        approval_id: String,
    },
    /// Neue geprüfte Basis und Archivierung alter Workbench-Dateien vorschauen.
    LiveUpdatePreview { id: String },
    /// Geprüften Basiswechsel bei beendetem Spiel ausführen oder fortsetzen.
    LiveUpdate {
        id: String,
        #[arg(long)]
        review_id: String,
        #[arg(long)]
        steam_verified_before_audit: bool,
    },
    /// Gespeicherten Ausgangsstand für eine spätere Live-Einrichtung prüfen.
    LiveSetupPreview { id: String },
    /// Vanilla-Herkunft bestätigen, alle Quellen geschützt neu hashen und Registry sichern.
    LiveSetup {
        id: String,
        #[arg(long)]
        review_id: String,
        /// Bestätigt, dass Steam Verify VOR dem gewählten Inhaltsbericht beendet wurde.
        #[arg(long)]
        steam_verified_before_audit: bool,
    },
    /// Konkrete Live-Dateiänderungen für Request-JSON (action: apply oder restore) anzeigen.
    LivePreview { request: PathBuf },
    /// Geprüfte Live-Dateiänderungen nach erneutem vollständigem Quellhash ausführen.
    LiveExecute {
        request: PathBuf,
        #[arg(long)]
        review_id: String,
    },
    /// Exportierte Prüfberichte und gespeicherte Ausgangsstände im Projekt auflisten.
    BaselineList,
    /// Prüfbericht mit aktuellen Metadaten und Registry vergleichen, ohne große Archive zu lesen.
    BaselinePreview { report_name: String },
    /// Prüfbericht und verifizierte Registry-Kopie als beobachteten Ausgangsstand speichern.
    BaselineCapture {
        report_name: String,
        #[arg(long)]
        review_id: String,
    },
    /// Gespeicherte Sicherung prüfen und aktuelle Metadatenabweichungen anzeigen.
    BaselineStatus { id: String },
    /// Alle Depotdateien SHA-1/SHA-256 prüfen; nur bei beendetem Spiel, keine Live-Schreibzugriffe.
    InstallationVerify {
        #[arg(long)]
        output: Option<PathBuf>,
    },
    /// Dateiliste und Größen mit lokalen Steam-Depots vergleichen; keine Archivhashes oder Live-Schreibzugriffe.
    InstallationCheck {
        #[arg(long)]
        output: Option<PathBuf>,
    },
    /// Unterbrochene, markierte Projektprobe wiederherstellen; kein Live-Spielpfad.
    ModRecover { directory: PathBuf },
    /// Vorhandene Projektproben auflisten; ausschließlich ihre Marker lesen.
    ModBackups,
    /// Sicherung prüfen und Rücknahme anzeigen, ohne Dateien zu ändern.
    ModRestorePreview { directory: PathBuf },
    /// Geschützte Projektkopie anhand einer aktuellen Vorschau wiederherstellen.
    ModRestore {
        directory: PathBuf,
        #[arg(long)]
        review_id: String,
    },
    /// Modmodule, belegte Felder und verbleibende Schreibsperren anzeigen.
    ModInfo,
    /// In-memory Overlay-Vorschau aus einer ModRequest-JSON.
    ModPreview {
        request: PathBuf,
        #[arg(long)]
        output: Option<PathBuf>,
    },
    /// Nicht angewendete Overlaydateien und vollständigen Plan nach exports/ schreiben.
    ModExport { request: PathBuf },
    /// Apply/Reapply/Restore plus Abbruch-Recovery nur in einer neuen Projektkopie proben.
    ModRehearse { request: PathBuf },
    /// Geprüfte Herstellungsrezepte und Abdeckung anzeigen.
    Recipes {
        #[arg(long)]
        item: Option<u32>,
    },
    /// Materialbaum aus einer PlanRequest-JSON berechnen; ausschließlich lesend.
    Craft {
        request: PathBuf,
        #[arg(long)]
        output: Option<PathBuf>,
    },
    /// Steam-, Epic-, Game-Pass-Installationen und Save-Pfade finden.
    Detect,
    /// EXE-/Metadaten- und Tabellen-Fingerprints ausgeben.
    Fingerprint {
        /// Nur Dateien hashen; unbekannte Builds niemals semantisch interpretieren.
        #[arg(long)]
        metadata_only: bool,
        #[arg(long)]
        output: Option<PathBuf>,
    },
    /// Für diesen Datenbuild verifizierte Item-Sprachen zeigen.
    Languages,
    /// Verifizierte Tabellen und ihren Interpretationsumfang zeigen.
    Tables,
    /// Items bzw. strukturelle Tabellenindizes als JSON ausgeben.
    Dump {
        table: String,
        /// Einzelnen Datensatz auswählen.
        #[arg(long)]
        item: Option<u32>,
        /// Alle Feldbereiche eines einzelnen Iteminfo-Datensatzes zeigen.
        #[arg(long)]
        fields: bool,
        /// Neue JSON-Datei, ohne bestehende Dateien zu überschreiben.
        #[arg(long)]
        output: Option<PathBuf>,
    },
    /// Bytegenaue Parser-/Serializer-Prüfung ausschließlich im Speicher.
    Roundtrip {
        /// Zusätzlich die lückenlose Feldabdeckung aller Items prüfen.
        #[arg(long)]
        all_fields: bool,
        #[arg(long)]
        output: Option<PathBuf>,
    },
    /// Suchindex aufbauen; bei geändertem Fingerprint/Sprache neu aufbauen.
    Index {
        #[arg(long)]
        cache: Option<PathBuf>,
    },
    /// Namen, Beschreibungen und interne Schlüssel mit SQLite FTS5 suchen.
    Search {
        #[arg(default_value = "")]
        text: String,
        #[arg(long, default_value_t = 50)]
        limit: u32,
        #[arg(long, default_value_t = 0)]
        offset: u32,
        #[arg(long)]
        item_type: Option<i64>,
        #[arg(long)]
        category: Option<String>,
        #[arg(long)]
        cache: Option<PathBuf>,
    },
    /// Einzelnes Item mit deutschem Namen und optional allen Rohfeldern lesen.
    Item {
        key: u32,
        #[arg(long)]
        fields: bool,
        #[arg(long)]
        output: Option<PathBuf>,
    },
    /// Zwei erzeugte Fingerprint-JSONs vergleichen, auch unbekannte Builds.
    Diff {
        before: PathBuf,
        after: PathBuf,
        #[arg(long)]
        output: Option<PathBuf>,
    },
}
fn main() -> ExitCode {
    match run(Cli::parse()) {
        Ok(()) => ExitCode::SUCCESS,
        Err(Error::Io(e)) if e.kind() == io::ErrorKind::BrokenPipe => ExitCode::SUCCESS,
        Err(Error::Json(e)) if e.io_error_kind() == Some(io::ErrorKind::BrokenPipe) => {
            ExitCode::SUCCESS
        }
        Err(e) => {
            eprintln!("Fehler: {e}");
            ExitCode::from(2)
        }
    }
}
fn run(cli: Cli) -> Result<()> {
    match &cli.command {
        Command::LiveStatus
        | Command::ForeignPreview
        | Command::ForeignConfirm { .. }
        | Command::ForeignRevoke { .. }
        | Command::LiveUpdatePreview { .. }
        | Command::LiveUpdate { .. }
        | Command::LiveSetupPreview { .. }
        | Command::LiveSetup { .. }
        | Command::LivePreview { .. }
        | Command::LiveExecute { .. } => {
            let discovery = cd_core::discover_project(&cli.project, cli.game.as_deref())?;
            let game = cd_core::select_game(&discovery)?;
            let policy = cd_core::output_policy(&cli.project, Some(&game))?;
            let cancel = std::sync::atomic::AtomicBool::new(false);
            match &cli.command {
                Command::ForeignPreview => emit(
                    &cli,
                    &cd_core::apply::live::foreign::inspect(&policy, &game)?,
                    None,
                ),
                Command::ForeignConfirm {
                    review_id,
                    preserve,
                } => emit(
                    &cli,
                    &cd_core::apply::live::foreign::confirm(&policy, &game, review_id, *preserve)?,
                    None,
                ),
                Command::ForeignRevoke { approval_id } => emit(
                    &cli,
                    &cd_core::apply::live::foreign::revoke(&policy, &game, approval_id)?,
                    None,
                ),
                Command::LiveStatus => {
                    emit(&cli, &cd_core::apply::live::status(&policy, &game)?, None)
                }
                Command::LiveUpdatePreview { id } => emit(
                    &cli,
                    &cd_core::apply::live::updates::preview(&policy, &game, id)?,
                    None,
                ),
                Command::LiveUpdate {
                    id,
                    review_id,
                    steam_verified_before_audit,
                } => emit(
                    &cli,
                    &cd_core::apply::live::updates::execute(
                        &policy,
                        &game,
                        id,
                        review_id,
                        *steam_verified_before_audit,
                        &cancel,
                    )?,
                    None,
                ),
                Command::LiveSetupPreview { id } => emit(
                    &cli,
                    &cd_core::apply::live::setup_preview(&policy, &game, id)?,
                    None,
                ),
                Command::LiveSetup {
                    id,
                    review_id,
                    steam_verified_before_audit,
                } => emit(
                    &cli,
                    &cd_core::apply::live::setup(
                        &policy,
                        &game,
                        id,
                        review_id,
                        *steam_verified_before_audit,
                        &cancel,
                    )?,
                    None,
                ),
                Command::LivePreview { request } | Command::LiveExecute { request, .. } => {
                    let request = read_json::<cd_core::apply::live::Request>(&project_path(
                        &cli.project,
                        request,
                    ))?;
                    if let Command::LiveExecute { review_id, .. } = &cli.command {
                        emit(
                            &cli,
                            &cd_core::apply::live::execute(
                                &policy, &game, &request, review_id, &cancel,
                            )?,
                            None,
                        )
                    } else {
                        emit(
                            &cli,
                            &cd_core::apply::live::preview(&policy, &game, &request)?,
                            None,
                        )
                    }
                }
                _ => unreachable!(),
            }
        }
        Command::BaselineList => {
            let policy = cd_core::output_policy(&cli.project, cli.game.as_deref())?;
            emit(
                &cli,
                &cd_core::installation::baseline::catalog(&policy)?,
                None,
            )
        }
        Command::BaselinePreview { report_name } | Command::BaselineCapture { report_name, .. } => {
            let discovery = cd_core::discover_project(&cli.project, cli.game.as_deref())?;
            let game = cd_core::select_game(&discovery)?;
            let policy = cd_core::output_policy(&cli.project, Some(&game))?;
            if let Command::BaselineCapture { review_id, .. } = &cli.command {
                emit(
                    &cli,
                    &cd_core::installation::baseline::capture(
                        &policy,
                        &game,
                        report_name,
                        review_id,
                    )?,
                    None,
                )
            } else {
                emit(
                    &cli,
                    &cd_core::installation::baseline::preview(&policy, &game, report_name)?,
                    None,
                )
            }
        }
        Command::BaselineStatus { id } => {
            let discovery = cd_core::discover_project(&cli.project, cli.game.as_deref())?;
            let game = cd_core::select_game(&discovery)?;
            let policy = cd_core::output_policy(&cli.project, Some(&game))?;
            emit(
                &cli,
                &cd_core::installation::baseline::inspect(&policy, &game, id)?,
                None,
            )
        }
        Command::InstallationVerify { output } => {
            let discovery = cd_core::discover_project(&cli.project, cli.game.as_deref())?;
            let game = cd_core::select_game(&discovery)?;
            let cancel = std::sync::atomic::AtomicBool::new(false);
            let mut last = std::time::Instant::now();
            let report = cd_core::installation::audit::verify_managed(
                &cd_core::output_policy(&cli.project, Some(&game))?,
                &game,
                &cancel,
                |p| {
                    if p.phase == "complete" || last.elapsed().as_secs() >= 1 {
                        eprintln!(
                            "{}: {}/{} Dateien, {}/{} Bytes",
                            p.phase, p.files_done, p.total_files, p.bytes_done, p.total_bytes
                        );
                        last = std::time::Instant::now();
                    }
                },
            )?;
            emit(&cli, &report, output.as_deref())?;
            if !report.all_files_match_cache {
                return Err(Error::Invalid(
                    "Inhaltsabweichungen gegenüber dem lokalen Steam-Cache; siehe Bericht.".into(),
                ));
            }
            Ok(())
        }
        Command::InstallationCheck { output } => {
            let report = cd_core::discover_project(&cli.project, cli.game.as_deref())?;
            let game = cd_core::select_game(&report)?;
            emit(
                &cli,
                &cd_core::installation::inspect_managed(
                    &cd_core::output_policy(&cli.project, Some(&game))?,
                    &game,
                )?,
                output.as_deref(),
            )
        }
        Command::ModRecover { directory } => {
            let policy = cd_core::output_policy(&cli.project, cli.game.as_deref())?;
            emit(
                &cli,
                &cd_core::apply::recover_rehearsal(&policy, directory)?,
                None,
            )
        }
        Command::ModBackups => {
            let policy = cd_core::output_policy(&cli.project, cli.game.as_deref())?;
            emit(&cli, &cd_core::apply::recovery::list(&policy)?, None)
        }
        Command::ModRestorePreview { directory } => {
            let policy = cd_core::output_policy(&cli.project, cli.game.as_deref())?;
            emit(
                &cli,
                &cd_core::apply::recovery::inspect(&policy, directory)?,
                None,
            )
        }
        Command::ModRestore {
            directory,
            review_id,
        } => {
            let policy = cd_core::output_policy(&cli.project, cli.game.as_deref())?;
            emit(
                &cli,
                &cd_core::apply::recovery::restore(&policy, directory, review_id)?,
                None,
            )
        }
        Command::Detect => emit(
            &cli,
            &cd_core::discover_project(&cli.project, cli.game.as_deref())?,
            None,
        ),
        Command::Languages => emit(&cli, &cd_core::supported_languages(), None),
        Command::Diff {
            before,
            after,
            output,
        } => {
            let left = read_manifest(&project_path(&cli.project, before))?;
            let right = read_manifest(&project_path(&cli.project, after))?;
            emit(
                &cli,
                &cd_core::fingerprint::diff(&left, &right),
                output.as_deref(),
            )
        }
        Command::Fingerprint {
            metadata_only,
            output,
        } => {
            let report = cd_core::discover_project(&cli.project, cli.game.as_deref())?;
            let game = cd_core::select_game(&report)?;
            let mut fingerprint = cd_core::fingerprint::Fingerprint::inspect(&game)?;
            if !metadata_only && fingerprint.metadata_matches {
                match cd_core::GameData::open(&game, &cli.language) {
                    Ok(data) => fingerprint = data.fingerprint().clone(),
                    Err(error) => fingerprint.diagnostics.push(error.to_string()),
                }
            }
            emit(&cli, &fingerprint, output.as_deref())?;
            if !metadata_only && !fingerprint.read_schema_supported {
                return Err(Error::UnsupportedBuild(
                    "See fingerprint diagnostics; use --metadata-only for a diagnostic manifest."
                        .into(),
                ));
            }
            Ok(())
        }
        _ => {
            let workspace = Workspace::open(&cli.project, cli.game.as_deref(), &cli.language)?;
            match &cli.command {
                Command::ModInfo => emit(
                    &cli,
                    &cd_core::mods::ModCatalog::open(workspace.data())?.info(),
                    None,
                ),
                Command::ModPreview { request, .. }
                | Command::ModExport { request }
                | Command::ModRehearse { request } => {
                    let mut bytes = Vec::new();
                    std::fs::File::open(project_path(&cli.project, request))?
                        .take(1_048_577)
                        .read_to_end(&mut bytes)?;
                    if bytes.len() > 1_048_576 {
                        return Err(Error::Invalid("Mod request exceeds 1 MiB".into()));
                    }
                    let bytes = bytes.strip_prefix(&[0xef, 0xbb, 0xbf]).unwrap_or(&bytes);
                    let built = cd_core::mods::ModCatalog::open(workspace.data())?
                        .build(serde_json::from_slice(bytes)?)?;
                    match &cli.command {
                        Command::ModPreview { output, .. } => {
                            emit(&cli, &built.preview, output.as_deref())
                        }
                        Command::ModExport { .. } => {
                            emit(&cli, &cd_core::apply::export(&workspace, &built)?, None)
                        }
                        _ => emit(&cli, &cd_core::apply::rehearse(&workspace, &built)?, None),
                    }
                }
                Command::Recipes { item } => {
                    let catalog = cd_core::crafting::CraftCatalog::open(workspace.data())?;
                    if let Some(item) = item {
                        emit(&cli, &catalog.recipes_for(*item), None)
                    } else {
                        emit(&cli, &catalog.info(), None)
                    }
                }
                Command::Craft { request, output } => {
                    let mut bytes = Vec::new();
                    std::fs::File::open(project_path(&cli.project, request))?
                        .take(1_048_577)
                        .read_to_end(&mut bytes)?;
                    if bytes.len() > 1_048_576 {
                        return Err(Error::Invalid("Plan JSON exceeds 1 MiB".into()));
                    }
                    let bytes = bytes.strip_prefix(&[0xef, 0xbb, 0xbf]).unwrap_or(&bytes);
                    let request = serde_json::from_slice(bytes)?;
                    let catalog = cd_core::crafting::CraftCatalog::open(workspace.data())?;
                    emit(&cli, &catalog.plan(request)?, output.as_deref())
                }
                Command::Tables => emit(&cli, &workspace.data().tables(), None),
                Command::Dump {
                    table,
                    item,
                    fields,
                    output,
                } => emit(
                    &cli,
                    &workspace.data().dump_table(table, *item, *fields)?,
                    output.as_deref(),
                ),
                Command::Item {
                    key,
                    fields,
                    output,
                } => emit(
                    &cli,
                    &workspace.data().item_detail(*key, *fields)?,
                    output.as_deref(),
                ),
                Command::Roundtrip { all_fields, output } => {
                    let report = workspace.data().roundtrip(*all_fields)?;
                    emit(&cli, &report, output.as_deref())?;
                    if !report.all_passed {
                        return Err(Error::Invalid(
                            "Roundtrip failed; see individual checks".into(),
                        ));
                    }
                    Ok(())
                }
                Command::Index { cache } => {
                    emit(&cli, &workspace.build_index(cache.as_deref())?, None)
                }
                Command::Search {
                    text,
                    limit,
                    offset,
                    item_type,
                    category,
                    cache,
                } => emit(
                    &cli,
                    &workspace.search(
                        SearchQuery {
                            text: text.clone(),
                            limit: *limit,
                            offset: *offset,
                            item_type: *item_type,
                            category: category.clone(),
                        },
                        cache.as_deref(),
                    )?,
                    None,
                ),
                _ => unreachable!(),
            }
        }
    }
}
fn emit(cli: &Cli, value: &impl Serialize, path: Option<&Path>) -> Result<()> {
    if let Some(path) = path {
        let mut bytes = serde_json::to_vec_pretty(value)?;
        bytes.push(b'\n');
        let policy = cd_core::output_policy(&cli.project, cli.game.as_deref())?;
        let saved = cd_core::write_generated(&policy, path, &bytes)?;
        eprintln!("Gespeichert: {}", saved.display());
    } else {
        let mut out = io::stdout().lock();
        serde_json::to_writer_pretty(&mut out, value)?;
        out.write_all(b"\n")?;
    }
    Ok(())
}
fn project_path(project: &Path, path: &Path) -> PathBuf {
    if path.is_absolute() {
        path.to_path_buf()
    } else {
        project.join(path)
    }
}
fn read_manifest(path: &Path) -> Result<cd_core::fingerprint::Fingerprint> {
    let mut bytes = Vec::new();
    std::fs::File::open(path)?
        .take(4 * 1024 * 1024 + 1)
        .read_to_end(&mut bytes)?;
    if bytes.len() > 4 * 1024 * 1024 {
        return Err(Error::Invalid("Fingerprint JSON exceeds 4 MiB".into()));
    }
    let bytes = bytes.strip_prefix(&[0xef, 0xbb, 0xbf]).unwrap_or(&bytes);
    let value: cd_core::fingerprint::Fingerprint = serde_json::from_slice(bytes)?;
    if value.format_version != 1 {
        return Err(Error::Invalid(
            "Unknown fingerprint manifest version".into(),
        ));
    }
    Ok(value)
}

fn read_json<T: serde::de::DeserializeOwned>(path: &Path) -> Result<T> {
    let mut bytes = Vec::new();
    std::fs::File::open(path)?
        .take(1_048_577)
        .read_to_end(&mut bytes)?;
    if bytes.len() > 1_048_576 {
        return Err(Error::Invalid("Request exceeds 1 MiB".into()));
    }
    Ok(serde_json::from_slice(
        bytes.strip_prefix(&[0xef, 0xbb, 0xbf]).unwrap_or(&bytes),
    )?)
}

#[cfg(test)]
mod tests {
    use super::*;
    use clap::CommandFactory;
    #[test]
    fn cli_help_and_commands_are_consistent() {
        Cli::command().debug_assert();
    }
    #[test]
    fn no_write_to_game_commands_are_exposed() {
        for command in ["apply", "restore", "save", "export"] {
            assert!(Cli::try_parse_from(["cd-cli", command]).is_err());
        }
    }
    #[test]
    fn manifest_size_is_bounded() {
        let dir = tempfile::tempdir().unwrap();
        let p = dir.path().join("huge.json");
        std::fs::write(&p, vec![b' '; 4 * 1024 * 1024 + 1]).unwrap();
        assert!(matches!(read_manifest(&p), Err(Error::Invalid(_))));
    }
}
