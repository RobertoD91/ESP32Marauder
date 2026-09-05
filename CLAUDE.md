# ESP32 Marauder

Arduino sketch (C++) for ESP-family boards: WiFi/Bluetooth analysis and testing
firmware. One source tree builds ~24 hardware targets, each selected by a single
`-D<FLAG>` passed on the compiler command line by CI.

## Layout

| Path | What it holds |
| --- | --- |
| `esp32_marauder/` | The sketch. `esp32_marauder.ino` is the entry point, `configs.h` is the per-board configuration hub |
| `User_Setup_*.h`, `User_Setup_Select.h` | TFT_eSPI display setups, copied into the checked-out TFT_eSPI library at build time |
| `.github/workflows/build_parallel.yml` | The build matrix; the single source of truth for what hardware exists |
| `.github/workflows/build_installer_manifests.yml` | Compiles the same matrix and emits web-installer metadata |
| `installer/targets.json` | Installer target registry; must stay in sync with the build matrix |
| `tools/installer_manifest.py` | Validates the registry, exports the matrix, generates manifests |
| `tools/test_installer_manifest.py` | Unit tests over the registry/matrix pair (run in CI) |
| `libraries/` | Vendored libraries that are not fetched from GitHub during the build |

## Adding or changing a hardware target

A target is not one edit; it is a set that has to agree. Missing one breaks CI,
because `installer_manifest.py --validate-registry` and the unit tests compare
the build matrix against the registry.

1. `esp32_marauder/configs.h` — add the flag to the commented `BOARD TARGETS`
   list, a `HARDWARE_NAME`, a board-features block (`HAS_*` macros), a display
   block, a menu-geometry block, `SD_CS`, `MEM_LOWER_LIM`, and any I2C/GPS/SD
   pin entries the feature macros pull in.
2. `User_Setup_<board>.h` plus a commented `//#include <...>` line in
   `User_Setup_Select.h`. CI uncomments that exact line with `sed`, so the line
   must start with `//#include <` and match the matrix `tft_file` value.
3. `.github/workflows/build_parallel.yml` — one flow-mapping line in the matrix.
   The parser in `tools/installer_manifest.py` is strict: the line must be
   `- { key: "value", ... }` on one line, with exactly the required keys.
4. `installer/targets.json` — a registry entry with the same build flag.
5. `tools/test_installer_manifest.py` — bump the hard-coded target and board
   counts.

`idf_ver` in the matrix and the `HAS_IDF_3` / `HAS_NIMBLE_2` macros in
`configs.h` must agree: `3.3.4` means both macros are defined, `2.0.11` means
neither is.

## Configuration conventions

- Feature macros (`HAS_SCREEN`, `HAS_TOUCH`, `HAS_SD`, `HAS_BATTERY`, ...) are
  set per board and then tested everywhere else. Prefer adding a feature macro
  over testing a board flag in the sketch sources.
- Full-screen touch targets run the panel in portrait, 240x320,
  `SCREEN_ORIENTATION 0`, with `HAS_ILI9341` enabling the touch UI.
- `HAS_C5_SD` means the SD card shares an SPI bus, so the sketch creates a
  shared `SPIClass` and hands it to `SDInterface`.
- `HAS_CAP_TOUCH` selects the FT6336 driver in `ft6336.h`; `HAS_CYD_TOUCH`
  selects the XPT2046 resistive path.
- `AXP192 axp192_obj` is defined in `BatteryInterface.h` and is visible wherever
  that header is included. Multiple definitions across translation units are
  tolerated because CI patches the toolchain with `-zmuldefs`.

## Building

There is no local Arduino toolchain in this repo. Builds run in GitHub Actions:

- `build_parallel.yml` — the release build; on `workflow_dispatch` it also
  creates a draft release, so avoid it for compile checks.
- `build_installer_manifests.yml` — same matrix, validates the registry, runs
  the unit tests, uploads artifacts, and creates nothing. This is the workflow
  to dispatch when you only want to know whether the tree compiles.

Registry checks can be run locally without a toolchain:

```
python3 tools/installer_manifest.py --validate-registry
python3 tools/installer_manifest.py --export-build-matrix
python3 -m unittest tools/test_installer_manifest.py -v
```
