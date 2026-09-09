# AGENTS.md

Compact guidance for working in this repo. Read this before editing.

## What this repo is

VoltraBloom — a hybrid energy harvesting system (solar + VAWT wind + Soil Microbial Fuel Cell) on ESP32, with static HTML dashboards for live telemetry and 3D visualization. There is **no build system, no package manager, no test runner, no linter, no CI**. Everything is plain `.ino` and `.html` files plus design assets. Deployed via Vercel (`npx vercel --prod`).

## Structure (the parts that matter)

- `index.html` — main live 3D dashboard + telemetry (Chart.js + Three.js r128 + Supabase). Opens directly in a browser; no server needed. Requires internet on first load for CDNs.
- `viewer_3d.html` — standalone Three.js GLB viewer. Loads `3d_models/3D_model _voltrabloom.glb` (note: space in filename). Language: `lang="id"` (Indonesian).
- `box_akrilik_designer.html` — Three.js GLB showcase + animated HEMS controls (loads `Frantic_Kasi_v1.glb`, sliders for turbine/current speed and model scale, toggles for components; dark UI `#0f172a`, uses Font Awesome). Despite the name it is no longer a procedural parametric CAD designer.
- `main.js` + `style.css` — **production-optimized build** (ES module `import * as THREE from 'three'`, hand-written CSS replacing Tailwind CDN). **No committed HTML file loads `main.js`** — there is no import map in any tracked HTML. `main.js` cannot be opened via `file://`; requires an HTTP server (`python3 -m http.server 8080` or `npx serve .`).
- `firmware/Project_Voltrabloom_Unified/` — **the recommended firmware**: FreeRTOS dual-core, 100 Hz ADC1 sensing on Core 1, WiFi + REST + Supabase on Core 0. Edit this one by default.
- `firmware/Project_Voltrabloom/` — older standalone WiFi AP + local web server sketch.
- `firmware/Project_Voltrabloom_Supabase/` — cloud-only logger variant. **Bug:** its `.ino` is named `Project_Voltrabloom.ino` (not `Project_Voltrabloom_Supabase.ino`), so folder and sketch names don't match — Arduino IDE may refuse to compile it.
- `firmware/tester_pertama/` — sensor calibration sketch, no WiFi.
- `documents/schemas/supabase_telemetry_schema.sql` — run this in Supabase SQL Editor to provision `telemetry`, `telemetry_hourly`, the `aggregate_telemetry_hourly()` function, and the `cleanup_old_raw_telemetry(days)` retention function.
- `3d_models/` — `.glb` / `.stl` / `.svg` of the physical hardware. `Frantic_Kasi_v1.glb` is the original; `Frantic_Kasi.glb` is optimized. `3D_model _voltrabloom.glb` (with space) is loaded by `viewer_3d.html`.
- `media/` — logos, posters, photos, screenshots. Non-source — do not run formatters.
- `gallery/` — optimized gallery assets (cad_model, fabrication, wiring).
- `tools/Arduino IDE/` — a vendored **portable** Arduino IDE binary. Do not delete.
- `_archive_raw_downloads/` — read-only raw Google Drive backup, treat as immutable.
- `keperluan web voltra/` — early web drafts, gitignored.

## How to run / verify

- **Web pages** (`index.html`, `viewer_3d.html`, `box_akrilik_designer.html`): open directly in a browser. They pull Three.js / Tailwind / Chart.js / GSAP / Supabase from CDNs — an internet connection is required on first load.
- **Production build** (`main.js` + `style.css`): serve via HTTP (`python3 -m http.server 8080`). No committed HTML loads `main.js` as a module.
- **Firmware**: open the sketch folder in the portable IDE at `tools/Arduino IDE/Arduino IDE.exe` (or your own Arduino IDE). Board: `ESP32 Dev Module`. Upload speed: `115200`. Library required: `LiquidCrystal_I2C` (Frank de Brabander). Built-ins used: `WiFi`, `HTTPClient`, `WiFiClientSecure`.
- **SQL schema**: paste `documents/schemas/supabase_telemetry_schema.sql` into the Supabase SQL editor and run.
- **Deploy**: `npx vercel --prod`. Vercel config in `vercel.json` — static only, no build step, immutable cache on `style.css`/`main.js`, 7-day cache on PDFs.
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
- `.env.local` contains a `VERCEL_OIDC_TOKEN`. It's tracked but `.gitignore` excludes `.env*` — it was committed before the rule existed.

## Three.js model assets

- `3d_models/Frantic_Kasi_v1.glb` is the original iteration; `Frantic_Kasi.glb` is the optimized revision. `index.html` loads `Frantic_Kasi_v1.glb` via `GLTFLoader` at `index.html:574`, and `box_akrilik_designer.html` also loads it (`box_akrilik_designer.html:350`).
- `3d_models/3D_model _voltrabloom.glb` (note the space in the filename) is loaded by `viewer_3d.html:105`.
- `viewer_3d.html` is **not** procedural — it loads and displays a GLB file. `main.js` is the one that builds the enclosure procedurally with primitives (`BoxGeometry`/`CylinderGeometry`).
- HTML files are opened from repo root, so keep relative paths as `3d_models/...`.

## When adding a new sketch

1. Create `firmware/<SketchName>/<SketchName>.ino` (match the existing pattern: `firmware/<SketchName>/<SketchName>.ino` where the `.ino` filename matches the containing folder).
2. `#include "ArduinoCompat.h"` first so it compiles both in the IDE and under clangd language-server for IntelliSense.
3. Keep pins on ADC1.
4. If you add cloud upload, reuse `SupabaseLogger.h` (copy the file into your sketch folder — Arduino does not share headers across sketches).

## Style / workflow conventions

- Web stack: vanilla HTML + Tailwind (CDN) + Three.js r128 + Chart.js 4.4.0 + GSAP 3.12.5. No bundler, no transpiler. The HTML pages are designed to be opened as files.
- Production build (`main.js` + `style.css`): ES modules, hand-written CSS design system, no Tailwind runtime. Needs HTTP server to run.
- Firmware is C++ Arduino with FreeRTOS, pinned tasks per core. Prefer the Unified sketch's pattern (mutex-protected `SystemTelemetry` struct, two `xTaskCreatePinnedToCore` calls in `setup()`).
- Documentation lives in `documents/` (papers, proposals, schemas, scripts). Use Indonesian or English as found in the existing file; do not translate.
- Media and design files are non-source — do not run formatters on `media/photos/` or `.ipv` design files.
- The Supabase aggregation function estimates wind energy as `wind_v * 0.2` (hardcoded 0.2A approximation) — this is intentional, not a bug.

## Things that are easy to miss

- `_archive_raw_downloads/` is intentionally a backup — do not "tidy" it.
- `desktop.ini` is a Windows folder-setting file, leave it.
- `.clangd` files exist at the repo root and in `firmware/Project_Voltrabloom_Unified/` and `firmware/Project_Voltrabloom_Supabase/`. They enable IDE features via `ArduinoCompat.h` stubs. Don't delete them.
- The repo mixes Indonesian and English (filenames like `tester_pertama`, `keperluan web voltra`, `box_akrilik_designer`). Don't rename without asking.
- The two `Frantic_Kasi*` model files are iterations, not duplicates — the `_v1` is the original, the unversioned one is the optimized revision. `3D_model _voltrabloom.glb` is a separate model (used by `viewer_3d.html`).
- `firmware/Project_Voltrabloom_Supabase/Project_Voltrabloom.ino` — sketch name does not match folder name. This is a known issue; don't silently rename it without confirming intent.
- `README.md` references a `logo/` directory and "Three.js 0.167" production build — neither is committed. The `logo/` directory does not exist; the production HTML that would load `main.js` is not tracked.

## What is NOT here (don't go looking)

- No `package.json`, `requirements.txt`, `Cargo.toml`, `Makefile`, `pyproject.toml`.
- No `.github/` directory, no CI.
- No `tests/` directory.
- No `.eslintrc`, `.prettierrc`, `tsconfig.json`, etc.
- No import map in any committed HTML file — `main.js` cannot be loaded as a module from any tracked file.

## MCP servers (global config)

MCP servers are configured globally at `~/.config/opencode/opencode.jsonc` (not in this repo, so it applies to all projects). To manage them run `opencode mcp list` / `opencode mcp auth <name>`.

| Server | Type | How it works |
|--------|------|--------------|
| `context7` | remote | Up-to-date library docs (Three.js, Chart.js, ESP-IDF, etc.) — no key |
| `supabase` | remote | Hosted `https://mcp.supabase.com/mcp` — OAuth login |
| `github` | remote | `https://api.githubcopilot.com/mcp/` — uses `GITHUB_PERSONAL_ACCESS_TOKEN` env var (Bearer), `oauth: false` |
| `sequential-thinking` | local | Structured reasoning; runs via WSL-native Node |
| `vercel` | remote | `https://mcp.vercel.com` — OAuth login (matches this repo's Vercel deploy) |
| `playwright` | local | Browser automation; runs via WSL-native Node |
| `filesystem` | local | Scoped to this repo at `/mnt/c/Users/Arfi/Downloads/VOLTRA` |
| `sentry` | remote | Error tracking — OAuth login |

**Important for this WSL/Windows hybrid environment:** local MCP servers must use the **WSL-native** Node.js at `~/.local/lib/nodejs/bin/npx`, NOT the Windows `npx` on PATH (Windows node can't do MCP stdio with the Linux opencode binary). The config already hardcodes the WSL-native path and prepends its `bin` dir to `PATH`. Do not "fix" this back to a bare `npx`.

**Token-gated MCPs** (github, and optional supabase-locally) read their keys from env vars (`GITHUB_PERSONAL_ACCESS_TOKEN`). These fail with 400/Connection closed until the token is set — that's expected, not a bug. OAuth MCPs (supabase, vercel, sentry) authenticate on first use via `opencode mcp auth <name>`.

## Skills (global config)

35 skills installed globally at `~/.agents/skills/` + `opencode-skills-collection` plugin (1,595+ on-demand skills) configured in `~/.config/opencode/opencode.jsonc`. Restart opencode after config changes. Use `npx skills add <source> -a opencode -g -y` to install more. Use `npx skills list -g` to see what's installed.

| Skill | Source | When to use |
|-------|--------|-------------|
| `embedded-systems` | farmage/opencode-skills | ESP32/FreeRTOS firmware, peripherals, DMA, power optimization |
| `supabase` | supabase/agent-skills | Any Supabase task: DB, Auth, Edge Functions, Realtime, Storage, RLS |
| `supabase-postgres-best-practices` | supabase/agent-skills | Postgres schema, migrations, RLS, indexes, query optimization |
| `postgres` | planetscale/database-skills | Generic Postgres best practices (not PlanetScale-specific) |
| `web-design-guidelines` | vercel-labs/agent-skills | UI/accessibility/UX audit of HTML dashboards |
| `deploy-to-vercel` | vercel-labs/agent-skills | Deploy apps to Vercel |
| `vercel-optimize` | vercel-labs/agent-skills | Vercel cost/performance optimization |
| `advanced-frontend-uiux` | TANUJ0071/opencode-skills | Three.js, WebGL, GSAP, elite frontend generation |
| `skill-creator` | anthropics/skills | Create/modify/test new skills |
| `webapp-testing` | anthropics/skills | End-to-end webapp testing with Playwright |
| `agent-browser` | vercel-labs/agent-browser | Autonomous browser interaction, screenshots, navigation |
| `playwright-best-practices` | currents-dev/playwright-best-practices-skill | Playwright test writing, CI/CD, POM, mocking, accessibility |
| `playwright-cli` | microsoft/playwright-cli | Playwright CLI usage, test generation, codegen |
| `brainstorming` | obra/superpowers | Structured brainstorming for feature design and architecture |
| `dispatching-parallel-agents` | obra/superpowers | Launching and coordinating parallel subagent tasks |
| `executing-plans` | obra/superpowers | Step-by-step plan execution with verification gates |
| `finishing-a-development-branch` | obra/superpowers | Git branch cleanup, PR prep, merge workflows |
| `receiving-code-review` | obra/superpowers | Processing and acting on code review feedback |
| `requesting-code-review` | obra/superpowers | Self-review checklists before requesting review |
| `subagent-driven-development` | obra/superpowers | Delegating implementation to specialized subagents |
| `systematic-debugging` | obra/superpowers | Hypothesis-driven debugging loop for bugs and test failures |
| `test-driven-development` | obra/superpowers | TDD workflow: red-green-refactor cycle |
| `using-git-worktrees` | obra/superpowers | Git worktree management for parallel development |
| `using-superpowers` | obra/superpowers | Meta-skill: how to use the superpowers skill collection |
| `verification-before-completion` | obra/superpowers | Pre-completion verification gates and checklists |
| `writing-plans` | obra/superpowers | Structured plan writing for complex multi-step tasks |
| `writing-skills` | obra/superpowers | Creating new skills in SKILL.md format |
| `ralph-wiggum` | fstandhartinger/ralph-wiggum | Spec-driven autonomous coding with iterative bash loops |
| `ralph-loop-workflow` | andrelandgraf/fullstackrecipes | Iterative development loop workflow |
| `ralph-tui-prd` | subsy/ralph-tui | PRD-driven TUI app development |
| `ralph-tui-create-beads` | subsy/ralph-tui | TUI bead creation for Ratatui apps |
| `ralph-tui-create-beads-rust` | subsy/ralph-tui | Rust-specific TUI bead creation |
| `ralph-tui-create-json` | subsy/ralph-tui | JSON config generation for TUI apps |
| `testing-tui-runtime` | subsy/ralph-tui | Runtime testing for TUI applications |
| `dev` | microsoft/playwright-cli | General dev workflow skill (came with playwright-cli) |

**CLI tools installed:**
- `agent-browser` v0.27.0 — autonomous browser automation
- `playwright-cli` — Playwright test codegen and management
- `ralph-tui` — requires Bun runtime (`curl -fsSL https://bun.sh/install | bash`)
