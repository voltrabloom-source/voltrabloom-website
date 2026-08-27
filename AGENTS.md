# AGENTS.md

Compact guidance for working in this repo. Read this before editing.

## What this repo is
VoltraBloom — a hybrid energy harvesting system (solar + VAWT wind + Soil Microbial Fuel Cell) on ESP32, with static HTML dashboards for live telemetry and 3D visualization. There is **no build system, no package manager, no test runner, no linter, no CI**. Everything is plain `.ino` and `.html` files plus design assets.

## Structure (the parts that matter)
- `index.html` — main live 3D dashboard + telemetry (Chart.js + Three.js + Supabase client). Open directly in a browser, no server needed.
- `viewer_3d.html` — standalone Three.js prototype viewer (uses `3d_models/Frantic_Kasi.glb`).
- `box_akrilik_designer.html` — Three.js parametric acrylic-enclosure CAD designer.
- `firmware/Project_Voltrabloom_Unified/` — **the recommended firmware**: FreeRTOS dual-core, 100 Hz ADC1 sensing on Core 1, WiFi + REST + Supabase on Core 0. Edit this one by default.
- `firmware/Project_Voltrabloom/` — older standalone WiFi AP + local web server sketch.
- `firmware/Project_Voltrabloom_Supabase/` — cloud-only logger variant.
- `firmware/tester_pertama/` — sensor calibration sketch, no WiFi.
- `documents/schemas/supabase_telemetry_schema.sql` — run this in Supabase SQL Editor to provision `telemetry`, `telemetry_hourly`, the `aggregate_telemetry_hourly()` function, and the `cleanup_old_raw_telemetry(days)` retention function.
- `3d_models/` — `.glb` / `.stl` / `.svg` of the physical hardware (Frantic_Kasi iterations).
- `media/` — logos, posters, photos, screenshots.
- `tools/Arduino IDE/` — a vendored **portable** Arduino IDE binary. Do not delete.
- `_archive_raw_downloads/` — read-only raw Google Drive backup, treat as immutable.

## How to run / verify
- **Web pages**: just open the `.html` in a browser. No `npm install`, no dev server. They pull Three.js / Tailwind / Chart.js / GSAP / Supabase from CDNs — an internet connection is required on first load.
- **Firmware**: open the sketch folder in the portable IDE at `tools/Arduino IDE/Arduino IDE.exe` (or your own Arduino IDE). Board: `ESP32 Dev Module`. Upload speed: `115200`. Library required: `LiquidCrystal_I2C` (Frank de Brabander). Built-ins used: `WiFi`, `HTTPClient`, `WiFiClientSecure`.
- **SQL schema**: paste `documents/schemas/supabase_telemetry_schema.sql` into the Supabase SQL editor and run.
- There is **no automated test suite**. If you change a sketch, verify it compiles and flashes. If you change an HTML page, open it and click through.

## Hard invariants — do not break these
These are the rules in `.agents/rules/esp32_iot_guidelines.md` and `.agents/rules/threejs_visualizers.md`, restated because they are easy to get wrong:

1. **All analog sensors must stay on ADC1** (GPIO 32, 33, 34, 35, 36/VP, 39/VN). ADC2 is unusable while WiFi is on. Pin constants in the Unified sketch: `pinSolar=32, pinWind=35, pinSoil=34, pinOutput=33, pinAmpsIn=39, pinAmpsOut=36`.
2. **Float division on ADC samples**: never `sum / count` as integers. Always `(((float)sum / count) / 4095.0) * VREF`.
3. **Arduino sketch layout**: each `.ino` must live in a folder of the exact same name (e.g. `firmware/Foo/Foo.ino`). The IDE will not compile it otherwise.
4. **Headers (`.h`)**: declare classes/structs only. Never put global object instances (e.g. `LiquidCrystal_I2C lcd(...)`) in a header — that causes duplicate-definition errors when multiple `.ino` files include it. The shared `ArduinoCompat.h` is the only header that defines types and is structured to work with the `#ifdef ARDUINO` shim; do not add globals to it.
5. **Supabase REST endpoint** uses TLS with `client.setInsecure()` (no cert validation). Acceptable for the project, do not "fix" by adding a fingerprint.
6. **Three.js**: use explicit `.js` CDN endpoints (not bare `three` package paths). `THREE.Shape()` paths must `.closePath()`. Sub-component builders must be invoked from `window.onload` or `DOMContentLoaded`. When generating HTML files for this project, output clean HTML5 — never wrap in ```` ```html ```` fences.

## Network & secrets in source
- `firmware/Project_Voltrabloom_Unified/Project_Voltrabloom_Unified.ino` hard-codes a Supabase project URL and **anon key**, plus default WiFi creds (`ESP32` / `12345678`) and AP fallback creds (`VoltraBloom_AP` / `voltrabloom`). These are expected to be edited per-device; the anon key is a publishable public key, not a secret, but treat the file as user-specific config.
- `index.html` calls Supabase directly with the same publishable anon key. That's fine — never paste a service_role key here.

## Three.js model assets
- `3d_models/Frantic_Kasi.glb` is the optimized iteration; `Frantic_Kasi_v1.glb` is the original. The `index.html` 3D canvas and `viewer_3d.html` load `.glb` via `GLTFLoader`. If you add a new model, keep the relative path `3d_models/...` since the HTML files are opened from repo root.

## When adding a new sketch
1. Create `firmware/<SketchName>/<SketchName>/<SketchName>.ino` (note: existing sketches use a slightly different layout — `firmware/<SketchName>/<SketchName>.ino` — and that works because the parent folder name matches; copy that pattern, not a deeper nesting).
2. `#include "ArduinoCompat.h"` first so it compiles both in the IDE and under clangd language-server for IntelliSense.
3. Keep pins on ADC1.
4. If you add cloud upload, reuse `SupabaseLogger.h` (copy the file into your sketch folder — Arduino does not share headers across sketches).

## Style / workflow conventions
- Web stack: vanilla HTML + Tailwind (CDN) + Three.js r128 + Chart.js 4 + GSAP 3.12. No bundler, no transpiler. Keep it that way — the pages are designed to be opened as files.
- Firmware is C++ Arduino with FreeRTOS, pinned tasks per core. Prefer the Unified sketch's pattern (mutex-protected `SystemTelemetry` struct, two `xTaskCreatePinnedToCore` calls in `setup()`).
- Documentation lives in `documents/` (papers, proposals, schemas, scripts). Use Indonesian or English as found in the existing file; do not translate.
- Media and design files are non-source — do not run formatters on `media/photos/` or `.ipv` design files.

## Things that are easy to miss
- `_archive_raw_downloads/` is intentionally a backup — do not "tidy" it.
- `desktop.ini` is a Windows folder-setting file, leave it.
- `.clangd` files exist in the firmware sketch folders; they enable IDE features via `ArduinoCompat.h`. Don't delete them.
- The repo mixes Indonesian and English (filenames like `tester_pertama`, `keperluan web voltra`, `box_akrilik_designer`). Don't rename without asking.
- The two `Frantic_Kasi*` model files are iterations, not duplicates — the `_v1` is the original, the unversioned one is the optimized revision.

## What is NOT here (don't go looking)
- No `package.json`, `requirements.txt`, `Cargo.toml`, `Makefile`, `pyproject.toml`.
- No `.github/` directory, no CI.
- No `tests/` directory.
- No `.eslintrc`, `.prettierrc`, `tsconfig.json`, etc.
