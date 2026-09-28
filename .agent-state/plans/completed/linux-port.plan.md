---
name: linux-port
status: complete
created_at: 2026-09-28
updated_at: 2026-09-28
source_type: conversation
source_path: null
---

# Linux Port Implementation Plan

## Summary

Port PBO Manager to Linux while retaining Windows builds, stabilize cross-platform archive operations, add Linux desktop integration, and create portable Linux packaging.

## Progress Tracker

| Phase | Name | Status | Depends on | Output |
|---|---|---|---|---|
| 1 | Build and test baseline | done | - | Cross-platform targets and CTest |
| 2 | Platform services | done | 1 | Linux UI adapters |
| 3 | Commands and task results | done | 1 | Reliable CLI and beside-input |
| 4 | Filesystem and archive safety | done | 1 | Portable safe I/O |
| 5 | Desktop integration | done | 2,3 | MIME, Nautilus, Dolphin |
| 6 | Packaging and CI | done | 1,2,5 | AppDir/AppImage and Linux CI |
| 7 | Final validation | done | 1-6 | Passing build/tests and report |

## Phase Details

### Phase 1 — Build and test baseline

- [x] Refactor CMake into internal core/UI targets with platform-gated sources and install rules.
- [x] Enable CTest and isolate test-only dependencies.
- [x] Add Linux GCC/Clang CI while retaining Windows workflows.

### Phase 2 — Platform services

- [x] Add platform factories for icons, preview, and taskbar behavior.
- [x] Implement Linux Qt-based services and fix Release-only Windows console guards.
- [x] Retain externally published files using private session cache semantics.

### Phase 3 — Commands and task results

- [x] Fix prefix flag parsing and add beside-input command behavior.
- [x] Introduce structured task outcomes and reliable CLI diagnostics/exit codes.
- [x] Bootstrap the CLI with QCoreApplication and add command regression tests.

### Phase 4 — Filesystem and archive safety

- [x] Make filename allocation stable and collision-aware while preserving automatic renaming.
- [x] Validate prefixes and extraction paths independently of host separators.
- [x] Stage writes safely, check I/O results, and prevent trailing data or symlink traversal.
- [x] Use fixed-width little-endian fields, UTF-8 byte lengths, and bounded archive reads.

### Phase 5 — Desktop integration

- [x] Add desktop entry, MIME definition, icons, and application association.
- [x] Add fixed-label Dolphin service menus.
- [x] Add launch-only Nautilus MenuProvider and integration installer/remover.

### Phase 6 — Packaging and CI

- [x] Add one AppDir staging pipeline used for tarball and AppImage artifacts.
- [x] Bundle required Qt X11/Wayland runtime plugins without host drivers.
- [x] Document Linux source builds, portable installation, integration, and compatibility.

### Phase 7 — Final validation

- [x] Configure, build, and run all available Linux tests in Debug and Release.
- [x] Validate installed staging tree, desktop metadata, and integration scripts.
- [x] Write implementation report with deviations and remaining release-only checks.

## Validation Commands

- `cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Debug -DBUILD_TESTING=ON`
- `cmake --build build`
- `ctest --test-dir build --output-on-failure`
- `cmake -S . -B build-release -G Ninja -DCMAKE_BUILD_TYPE=Release -DBUILD_TESTING=OFF`
- `cmake --build build-release`
- `cmake --install build-release --prefix /tmp/pboman3-install`
- `desktop-file-validate linux/org.pboman3.PBOManager.desktop`

## Acceptance Criteria

- Linux GUI and CLI build from the same source as Windows.
- Windows-only Explorer and packaging code remains Windows-gated.
- Linux preview, icons, MIME association, Nautilus, and Dolphin integrations exist.
- CLI failures return nonzero and beside-input works per operand.
- Archive operations preserve compatibility and avoid unsafe or partial publication.
- Linux portable staging, CI, and build documentation are present.

## Exit Criteria

All available local validation commands pass, the staged Linux installation contains the required desktop integrations, and any checks requiring Windows or clean distribution VMs are documented for release validation.

## Completion Note

Completed on 2026-09-28. The Linux GUI, CLI, desktop integration, portable
packaging pipeline, archive hardening, and cross-platform test/build graph are
implemented. Windows and clean-distribution runtime checks remain release gates.
