# Changelog

This file records changes made by the **zh-CN localization branch**. It does not
repeat upstream release notes, which live in
[`docs/changelog.md`](docs/changelog.md).

Notation follows [Keep a Changelog](https://keepachangelog.com/en/1.1.0/), and
this project uses semantic versioning with a `-zh.N` suffix to mark
localization-only releases.

## [v1.7.1-zh.1] - 2026-10-01

Base: upstream Materializr 1.7.1.

### Added

- Complete Simplified Chinese (zh-CN) translation of the Android app's
  user-visible interface, on top of the project's existing C++ i18n mechanism.
  Covered: toolbar, primitive menu, every operation dialog, body/sketch property
  panels, history step names and menus, touch-navigation overlay readouts,
  shortcuts panel, help panel User Guide (all chapter titles and long-form
  paragraphs), guided tutorial, and tooltips and status hints throughout.
- Batch ingestion tooling under `tools/`: `add_catalogue_entries.py` (now
  content-matched, so a corrected translation needs no removal list),
  `update_catalogue_zh.py`, and the per-batch `catalogue_entries_*.py` /
  `zh_translations_*.py` data files.
- `tools/check_catalogue_regressions.py`, `tools/make_upstream_fingerprints.py`
  and `tools/upstream_fingerprints_1.7.1.txt`: one hash per upstream key, so
  the five languages upstream already ships are verifiable as untouched and the
  ingester refuses to refresh those rows. A six-language batch row for a key
  upstream already has is indistinguishable by eye from a genuinely new key;
  ownership had to become data.
- `README.md` / `README.zh-CN.md` in both languages, this changelog, and the
  original upstream English README preserved verbatim at
  `docs/upstream-README.md`.

### Changed

- `tools/i18n_catalogue.py` and the generated `src/i18n_catalogue.h`: the zh
  column filled in to 1749 keys, matching the other five bundled languages.
  All five non-Chinese columns (es, pt, fr, de, it) are carried over unchanged,
  so nothing regressed to 100% English.
- Translated strings that take no arguments are now passed as an argument rather
  than used as a printf format string (`SetTooltip("%s", tr(...))`), which
  keeps the build clean under `-Werror=format-security`.
- 27 zh translations rewritten to drop the em dash the project bans, each with
  the punctuation that fits the clause (comma, colon or semicolon). Two pt/it
  cells written for a key upstream already had were corrected the same way.
  `tools/strip_zh_em_dashes.py` records the mapping.

### Fixed

- `add_catalogue_entries.py` could blank a row's zh cell while refreshing its
  five other columns, and compared the full row against a five-column tuple, so
  every run reported hundreds of spurious refreshes. It now carries the zh cell
  across, compares only the columns it owns, and refuses to write if any
  translated row lost its zh.
- 54 cells across the five upstream languages had been overwritten by batch
  proposals for keys upstream already had. Restored to upstream v1.7.1 verbatim.

### Verification

All nine gates exit 0:

- `i18n_extract.py`: 0 untranslated across 444 source files.
- `check_specifiers.py`: 1749 keys, 282 format strings, 0 mismatches.
- `check_tr_calls.py`, `check_tr_coverage.py`, `check_tr_arguments.py`,
  `check_ui_string_arrays.py`, `check_op_names.py`: all report 0 findings
  (39 operation names, 92 helper calls).
- `check_catalogue_regressions.py`: 1192 upstream keys unchanged across
  es/pt/fr/de/it.
- `no_em_dashes.py`: no em-dashes, in text or in `\xNN` escapes.
- Build: NDK CMake configure and compile producing `libmain.so` (zero U+2014
  bytes, one copy of each corrected string), then `./gradlew assembleDebug`.
- On-device: installed on a physical Android 10 tablet (arm64-v8a, 2000x1200
  landscape) and walked through the welcome dialog, project list, history
  panel, primitive menu and empty-state screens.

### Known gaps

- `ShortcutsPanel`'s `Binding{keys, action}` table is not yet translated. Its
  struct-initializer shape is not covered by `check_ui_string_arrays.py`.

### Notes

- Unofficial community localization. No changes to the modeling kernel,
  geometry algorithms, interaction logic or file format. Contains only the
  localized code, without upstream's commit history; diff against upstream tag
  `v1.7.1` to compare.