# Linux Port Implementation Report

**Plan**: `.agent-state/plans/completed/linux-port.plan.md`  
**Branch**: `codex/linux-port`  
**Date**: 2026-09-28  
**Status**: COMPLETE

## Outcome

PBO Manager now has a shared C++20/Qt 6.8+ codebase for Windows and Linux. The
repository builds separate internal core and UI libraries, a Qt Widgets GUI, and
a Qt Core-only CLI. Windows Explorer integration remains Windows-gated. Linux
desktop assets, portable packaging, and CI are included.

## Implemented

- Added platform build switches, CTest registration, GCC/Clang Linux CI, and
  standard GNU installation directories.
- Added platform factories and Linux implementations for file icons, external
  preview, window attention, and taskbar progress fallback.
- Added structured task results, stable CLI exit codes, SIGINT cancellation,
  fixed prefix-option detection, and per-input `--beside-input` behavior.
- Made settings tests use a temporary INI store and replaced the live GitHub
  update test with a deterministic local response.
- Added deterministic filename sanitization and collision reservations,
  cross-platform prefix checks, symlink-component rejection, staged extraction,
  atomic publication, checked reads/writes, and truncating replacement.
- Made archive integers fixed-width little-endian, string sizes UTF-8 based,
  header ranges validated, and LZH reads bounded to the selected entry.
- Retained preview, clipboard, and drag materializations in private locked
  cache sessions. Sessions older than seven days are cleaned asynchronously.
- Added `application/x-pbo`, a desktop entry, icons, Dolphin service menus,
  a Nautilus 3/4 MenuProvider, and idempotent per-user install/uninstall scripts.
- Added one AppDir staging path for tarball and AppImage output, an AppRun
  dispatcher, pinned linuxdeploy downloads, checksums, and AlmaLinux 9 packaging
  in CI.

## Validation

- GCC 16 Debug GUI + CLI + tests: passed.
- CTest: passed; the executable reports 434 passing tests.
- GCC 16 Release CLI-only build: passed.
- Clang 22 Release GUI + CLI build: passed.
- Pack/unpack `--beside-input` round trip and partial-batch exit behavior:
  passed.
- Staged Linux install, AppRun CLI dispatch, MIME recognition including
  uppercase `.PBO`, desktop metadata, Python compilation, shell syntax, YAML
  parsing, and repeated integration install/uninstall: passed.
- `git diff --check`: passed.

## Release-only checks

The local host cannot validate Windows MSVC, Explorer registration, MSI/MSIX,
or clean GNOME/KDE sessions under both X11 and Wayland. The final AppImage was
not assembled locally because linuxdeploy is downloaded by the packaging CI
job. Clean-distribution VM checks, real Arma smoke tests, and release artifact
ABI audits remain release-gate work.

Extraction uses same-filesystem temporary files and rejects symlink path
components before publication. It does not yet use Linux `openat2` directory
handles, because that would require a separate Windows handle abstraction.
The current implementation materially improves safety and preserves the shared
cross-platform engine, while the release process should still treat hostile
concurrent directory replacement as outside the tested threat model.
