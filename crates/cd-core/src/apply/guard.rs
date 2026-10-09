use std::{io, path::Path};
#[cfg(windows)]
pub fn game_running() -> io::Result<bool> {
    scan_processes(|_, name| Ok(name.starts_with("crimsondesert") && name.ends_with(".exe")))
}
#[cfg(windows)]
fn scan_processes(mut matches: impl FnMut(u32, &str) -> io::Result<bool>) -> io::Result<bool> {
    use windows_sys::Win32::{
        Foundation::{CloseHandle, ERROR_NO_MORE_FILES, GetLastError, INVALID_HANDLE_VALUE},
        System::Diagnostics::ToolHelp::*,
    };
    // Snapshot lifetime is bounded to this call. Enumeration errors never mean "stopped".
    unsafe {
        let handle = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
        if handle == INVALID_HANDLE_VALUE {
            return Err(io::Error::last_os_error());
        }
        let result = (|| {
            let mut entry: PROCESSENTRY32W = std::mem::zeroed();
            entry.dwSize = std::mem::size_of::<PROCESSENTRY32W>() as u32;
            if Process32FirstW(handle, &mut entry) == 0 {
                return Err(io::Error::last_os_error());
            }
            loop {
                let end = entry
                    .szExeFile
                    .iter()
                    .position(|v| *v == 0)
                    .unwrap_or(entry.szExeFile.len());
                let name = String::from_utf16_lossy(&entry.szExeFile[..end]).to_ascii_lowercase();
                if matches(entry.th32ProcessID, &name)? {
                    return Ok(true);
                }
                if Process32NextW(handle, &mut entry) == 0 {
                    let code = GetLastError();
                    return if code == ERROR_NO_MORE_FILES {
                        Ok(false)
                    } else {
                        Err(io::Error::from_raw_os_error(code as i32))
                    };
                }
            }
        })();
        CloseHandle(handle);
        result
    }
}
#[cfg(not(windows))]
pub fn game_running() -> io::Result<bool> {
    Err(io::Error::new(
        io::ErrorKind::Unsupported,
        "Live process guard is supported only on Windows",
    ))
}

/// Resolve only candidate executable names; never inspect process memory or stop it.
#[cfg(windows)]
pub(super) fn paths_running(paths: &[std::path::PathBuf]) -> io::Result<bool> {
    use windows_sys::Win32::{
        Foundation::CloseHandle,
        System::Threading::{
            OpenProcess, PROCESS_QUERY_LIMITED_INFORMATION, QueryFullProcessImageNameW,
        },
    };
    let normalize = |p: &Path| {
        p.to_string_lossy()
            .trim_start_matches(r"\\?\")
            .to_lowercase()
    };
    let expected: Vec<_> = paths
        .iter()
        .map(|p| p.canonicalize().map(|p| normalize(&p)))
        .collect::<io::Result<_>>()?;
    let names: Vec<_> = paths
        .iter()
        .filter_map(|p| p.file_name())
        .map(|s| s.to_string_lossy().to_lowercase())
        .collect();
    scan_processes(|pid, name| {
        if !names.iter().any(|n| n == name) {
            return Ok(false);
        }
        // Candidate query failures remain unknown/blocked; a vanished process is
        // retried by the next session rather than silently treated as stopped.
        unsafe {
            let process = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, 0, pid);
            if process.is_null() {
                return Err(io::Error::last_os_error());
            }
            let mut buffer = vec![0u16; 32768];
            let mut length = buffer.len() as u32;
            let ok = QueryFullProcessImageNameW(process, 0, buffer.as_mut_ptr(), &mut length);
            let error = if ok == 0 {
                Some(io::Error::last_os_error())
            } else {
                None
            };
            CloseHandle(process);
            if let Some(error) = error {
                return Err(error);
            }
            let path = String::from_utf16(&buffer[..length as usize])
                .map_err(|_| io::Error::other("invalid process image path"))?;
            let canonical = Path::new(&path).canonicalize()?;
            Ok(expected.contains(&normalize(&canonical)))
        }
    })
}
#[cfg(not(windows))]
pub(super) fn paths_running(_: &[std::path::PathBuf]) -> io::Result<bool> {
    Err(io::Error::new(
        io::ErrorKind::Unsupported,
        "process image protection requires Windows",
    ))
}

/// Rehearsal/guard validation only until a trusted live build is admitted.
#[cfg(windows)]
#[cfg(test)]
fn deny_launch(path: &Path) -> io::Result<std::fs::File> {
    use std::os::windows::fs::OpenOptionsExt;
    std::fs::OpenOptions::new()
        .read(true)
        .share_mode(0)
        .open(path)
}
pub(super) fn replace(from: &Path, to: &Path) -> io::Result<()> {
    move_file(from, to, true)
}
pub(super) fn publish(from: &Path, to: &Path) -> io::Result<()> {
    move_file(from, to, false)
}
fn move_file(from: &Path, to: &Path, replace: bool) -> io::Result<()> {
    #[cfg(windows)]
    {
        use std::os::windows::ffi::OsStrExt;
        use windows_sys::Win32::Storage::FileSystem::{
            MOVEFILE_REPLACE_EXISTING, MOVEFILE_WRITE_THROUGH, MoveFileExW,
        };
        let src: Vec<u16> = from.as_os_str().encode_wide().chain(Some(0)).collect();
        let dst: Vec<u16> = to.as_os_str().encode_wide().chain(Some(0)).collect();
        if unsafe {
            MoveFileExW(
                src.as_ptr(),
                dst.as_ptr(),
                if replace {
                    MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH
                } else {
                    MOVEFILE_WRITE_THROUGH
                },
            )
        } == 0
        {
            return Err(io::Error::last_os_error());
        }
        Ok(())
    }
    #[cfg(not(windows))]
    {
        if !replace && to.try_exists()? {
            return Err(io::Error::new(
                io::ErrorKind::AlreadyExists,
                "publish destination exists",
            ));
        }
        std::fs::rename(from, to)?;
        std::fs::File::open(
            to.parent()
                .ok_or_else(|| io::Error::other("missing parent"))?,
        )?
        .sync_all()
    }
}

#[cfg(all(test, windows))]
mod tests {
    use super::*;
    #[test]
    fn exclusive_copy_handle_blocks_concurrent_executable_start() {
        use std::os::windows::process::CommandExt;
        let temp = tempfile::tempdir().unwrap();
        let copy = temp.path().join("guard-fixture.exe");
        std::fs::copy(std::env::current_exe().unwrap(), &copy).unwrap();
        let held = deny_launch(&copy).unwrap();
        let blocked = std::process::Command::new(&copy)
            .arg("--list")
            .creation_flags(0x08000000)
            .output();
        assert!(
            blocked.is_err(),
            "the lock must prevent starting the isolated executable"
        );
        drop(held);
        let allowed = std::process::Command::new(&copy)
            .arg("--list")
            .creation_flags(0x08000000)
            .output()
            .unwrap();
        assert!(allowed.status.success());
    }
}
