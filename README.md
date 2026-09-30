# Materializr for Android - Deep Simplified Chinese (zh-CN) Localization

<div align="center">

**[English](README.md) · [简体中文](README.zh-CN.md)**

A deep Simplified Chinese localization of the **Android** build of
[Materializr](https://github.com/materializr-cad/materializr), an open-source
parametric 3D CAD tool.

The original English README (with the full feature overview) is preserved
verbatim at [`docs/upstream-README.md`](docs/upstream-README.md).

</div>

---

> **This is an unofficial community localization.** It contains no changes to
> the modeling kernel, geometry algorithms, interaction logic, or file format -
> only user-visible interface strings. All functionality and branding remain
> the property of the original Materializr authors. See
> [Credits](#credits) and [License](#license).

## What this repository is

A localization of Materializr's **Android (arm64-v8a) app** into Simplified
Chinese (zh-CN), built on top of the upstream project's own C++ i18n
mechanism.

Translated coverage includes essentially every user-visible string:

| Area | What is translated |
| --- | --- |
| Toolbar | Main toolbar buttons and tool-group headings |
| Primitive menu | Box, cylinder, sphere, cone, torus, and friends |
| Dialogs | Every operation dialog's title and body text |
| Property panel | Body/sketch properties, color picker |
| History | Modeling step names, edit and breakpoint menus |
| Navigation overlay | Touch-navigation status readouts (e.g. "Move: on", "Multi-select: off") |
| Shortcuts panel | Section headings and every binding description |
| Help panel | All User Guide chapter titles and long-form paragraphs |
| Tutorial | Guided-flow labels and copy |
| Tooltips | Tooltips and status hints throughout the app |

The other five bundled languages (es, pt, fr, de, it) are carried over
**unchanged** - nothing regressed to 100% English.

## Install

Grab the prebuilt APK `Materializr-v1.7.1-zh.1-arm64-v8a.apk` from the
[Releases](RELEASES_URL) page and sideload it on an Android device
(enable "install unknown apps"). Tablets are strongly recommended - the app is
designed for a large touch screen.

Build it yourself:

```bash
cd android
./gradlew assembleDebug
```

The artifact lands at `android/app/build/outputs/apk/debug/app-debug.apk`.

## Technical notes

Localization uses the project's existing mechanism: **the English string is
the lookup key**, and `tr()` returns the translation for the active language.
That imposes a few conventions:

- Window and dialog titles use `tr("VisibleTitle###StableID")`; everything
  after `###` is a stable ID and is not translated.
- Strings that interpolate arguments go through
  `materializr::trFormat(fmt, args...)`.
- A literal percent sign in a translated key is written `%%`, so it is not
  mistaken for a printf conversion.

Because `tr()` returns a translated value that is then handed to printf-style
functions, translated text with no arguments must be passed as an argument, not
as the format string:

```cpp
ImGui::SetTooltip("%s", tr("Measure"));
```

### Tooling added for this localization

Everything under `tools/` is part of the upstream i18n workflow, plus the
batch ingestion helpers used to build the zh-CN column:

| Script | Role |
| --- | --- |
| `i18n_extract.py` | Extract interface strings from source, report untranslated ones |
| `check_specifiers.py` | Verify every language keeps printf specifiers and their order |
| `check_tr_calls.py` | Validate the shape of `tr()` call sites |
| `check_tr_coverage.py` | Find `tr()` call sites with no catalogue entry |
| `check_tr_arguments.py` | Find English arguments still passed into translated format strings |
| `check_ui_string_arrays.py` | Verify UI string arrays are fully covered |
| `check_op_names.py` | Verify operation names and strings passed to local UI helpers |
| `no_em_dashes.py` | House style gate: no em dashes, in text or in `\xNN` escapes |
| `check_catalogue_regressions.py` | Verify the five languages upstream owns are byte-identical to upstream v1.7.1 |
| `add_catalogue_entries.py` | Batch ingestion; refreshes on content, and refuses to touch upstream's rows |
| `update_catalogue_zh.py` | Ingest the zh column and regenerate `src/i18n_catalogue.h` |
| `make_upstream_fingerprints.py` | Rebuild `upstream_fingerprints_1.7.1.txt` from an upstream checkout |
| `strip_zh_em_dashes.py` | One-off, kept as the record of the 27 zh rewrites that removed em dashes |
| `catalogue_entries_*.py` | Per-batch six-language rows |
| `zh_translations_*.py` | Per-batch zh-CN translations |

`upstream_fingerprints_1.7.1.txt` records one hash per upstream key. This branch
adds a language; it does not get to revise the five that were already there, and
a six-language batch row for a key upstream already ships looks exactly like a
row upstream does not. So the ownership boundary is data, not care: the ingester
skips those keys, and the regression check fails if any of those cells moves.

### Verification

Run from the repository root:

```bash
python3 tools/i18n_extract.py
python3 tools/check_specifiers.py
python3 tools/check_tr_calls.py
python3 tools/check_tr_coverage.py
python3 tools/check_tr_arguments.py
python3 tools/check_ui_string_arrays.py
python3 tools/check_op_names.py
python3 tools/check_catalogue_regressions.py
python3 tools/no_em_dashes.py
```

All nine exit 0. Current results on this branch (version 1.7.1, catalogue 1749
keys x 6 languages):

| Check | Result |
| --- | --- |
| `i18n_extract.py` | 0 untranslated across 444 files |
| `check_specifiers.py` | 1749 keys / 282 format strings / 0 mismatches |
| `check_tr_calls.py` | 0 suspicious call sites |
| `check_tr_coverage.py` | 0 missing entries |
| `check_tr_arguments.py` | 0 untranslated arguments |
| `check_ui_string_arrays.py` | 0 missing entries |
| `check_op_names.py` | 39 operation names / 92 helper calls / 0 missing entries |
| `check_catalogue_regressions.py` | 1192 upstream keys unchanged across es/pt/fr/de/it |
| `no_em_dashes.py` | no em-dashes |

The build was verified end to end: NDK CMake configure and compile
(`libmain.so`, which holds zero U+2014 bytes and exactly one copy of each
corrected string), Gradle `assembleDebug`, sideload onto a physical Android 10
tablet, and an on-device pass over the welcome dialog, project list, history
panel, primitive menu and empty-state screens.

## Version mapping

| Project | Version |
| --- | --- |
| Upstream Materializr | 1.7.1 |
| This localization | `v1.7.1-zh.1` |

This repository intentionally carries **only the localized code**, without the
upstream commit history. To diff against upstream, compare tag `v1.7.1` of
[materializr-cad/materializr](https://github.com/materializr-cad/materializr)
against this branch.

Upstream release notes live in [`docs/changelog.md`](docs/changelog.md).

## Credits

- **[Materializr](https://github.com/materializr-cad/materializr)** - the
  original project, by its authors and all upstream contributors. The original
  English README is kept at [`docs/upstream-README.md`](docs/upstream-README.md);
  upstream credits for R4stl1n, stevebushwa, TechHQUSA and the wider community
  are preserved there in full.
- **[OpenCode](https://opencode.ai/)** - AI coding tool that drove the string
  extraction, translation, verification and packaging workflow for this
  localization.

Thanks to the original authors and contributors for building Materializr and
for maintaining an i18n mechanism that made a localization like this tractable.

## License

Materializr is GNU GPLv3 - see [`LICENSE`](LICENSE) - with additional
permissions under GPLv3 section 7 covering app-store distribution and
platform-SDK linking; see [`LICENSE-EXCEPTIONS.md`](LICENSE-EXCEPTIONS.md).
This localization is distributed under those same terms. Please follow the
upstream license when redistributing.

[`PRIVACY.md`](PRIVACY.md) from upstream applies unchanged.