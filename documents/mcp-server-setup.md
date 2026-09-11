# MCP Server Setup — VoltraBloom Workstation

Setup date: 2026-09-11 · Host: opencode 1.18.30 on WSL (Linux) / Windows hybrid
Repo: `/mnt/c/Users/Arfi/Downloads/VOLTRA` · Global config: `~/.config/opencode/opencode.jsonc`

This document records the MCP servers added for this workstation, why they were
chosen, and how to enable the ones that were deferred because their prerequisites
were not present in this environment. It was produced by a research pass over
https://github.com/punkpeye/awesome-mcp-servers with review gates.

## 1. Current MCP inventory

14 servers total: 13 declared in `opencode.jsonc` plus `gh_grep`, which is managed
by the `oh-my-opencode-slim` plugin (not in the config). 13 connected, 1 currently
disabled (semgrep — see §3).

| Server | Type | Scope / notes |
|--------|------|----------------|
| context7 | remote | Up-to-date library docs (pre-existing) |
| gh_grep | remote | GitHub code search — **declared by the `oh-my-opencode-slim` plugin**, not in `opencode.jsonc`. Do not duplicate it here. |
| supabase, github, vercel, sentry | remote | Project services (pre-existing) |
| sequential-thinking | local (npx) | Reasoning tool (pre-existing) |
| playwright | local (npx) | Browser automation (pre-existing) |
| filesystem | local (npx) | Scoped to this repo (pre-existing) |
| codegraph | local | Local code intelligence (pre-existing) |
| **firecrawl** | remote | 🎖️ official — web scrape/search/deep-research with JS rendering. **NEW** |
| **exa** | remote | 🎖️ official — semantic web search + clean markdown content fetch. **NEW** |
| **arduino** | local (npx) | arduino-cli wrapper — compile/upload/serial as MCP tools. **NEW** |
| **semgrep** | local (uvx) | SAST scanning — **disabled**, see §3 |

## 2. New servers (live)

### firecrawl
- Repo: `firecrawl/firecrawl-mcp-server` (official, MIT)
- Hosted keyless endpoint (free tier, rate-limited; no key configured)
- Tools: scrape a URL, web search, interact with a loaded page, deep research — all with JS rendering. Useful for: documentation of Three.js/Chart.js/Supabase, component references, and anything a plain fetch can't render.
- Config (already applied):
```jsonc
"firecrawl": { "type": "remote", "url": "https://mcp.firecrawl.dev/v2/mcp", "enabled": true }
```
- Security note: crawled content is processed by the Firecrawl service. **Do not crawl private URLs, local dashboards, or anything containing credentials.** This repo's HTML pages use a publishable Supabase anon key only — fine. Firmware `.ino` files with per-device WiFi creds must never be handed to this tool.

### exa
- Repo: `exa-labs/exa-mcp-server` (official, MIT)
- Hosted anonymous free tier (rate-limited) + OAuth; no key configured
- Tools: `web_search_exa` (semantic search), `web_fetch_exa` (page → clean markdown). Complements opencode's built-in websearch and firecrawl: Exa fetches cleaner content for API docs/datasheets.
- Config (already applied):
```jsonc
"exa": { "type": "remote", "url": "https://mcp.exa.ai/mcp", "enabled": true }
```
- Security note: same data-egress rule as firecrawl — search/fetch only public content.

### arduino
- Package: `arduino-mcp-server` v0.2.8 (`hardware-mcp/arduino-mcp-server`, open source)
- Wraps `arduino-cli` into MCP tools: detect hardware, compile/upload sketches, stateful serial sessions (open/read/expect/write), electrical-safety preflight, core/library management, board pin reference.
- Config (already applied) — note the sandboxed sketch root and the explicit WSL-native npx:
```jsonc
"arduino": {
  "type": "local",
  "command": ["/home/arfi/.local/lib/nodejs/bin/npx", "-y", "arduino-mcp-server"],
  "environment": {
    "PATH": "/home/arfi/.local/lib/nodejs/bin:{env:PATH}",
    "ARDUINO_SKETCH_ROOT": "/mnt/c/Users/Arfi/Downloads/VOLTRA/firmware"
  },
  "enabled": true,
  "timeout": 90000
}
```
- Status: the MCP server connects and runs. `arduino-cli` itself is **not yet installed in WSL** — use the server's `install_arduino_cli` tool, or see §5. Compile needs the `esp32:esp32` core (`arduino-cli core install esp32:esp32` after `config init` + `update-index`).
- Flashing/serial from WSL additionally requires **usbipd** USB passthrough (see §5). Compile + board/library management work without hardware.
- Security note: this server executes build tooling (`arduino-cli`, compilers). It is confined to `ARDUINO_SKETCH_ROOT` (repo firmware only) — do not widen that path.

## 3. Deferred servers & how to enable them

These were researched and verified to be the right tools, but their prerequisites are
not present in this environment yet. Config entries marked **disabled** are already in
`opencode.jsonc`; entries marked "not added" are absent from config.

### semgrep (disabled in config)
- Official SAST. Config entry exists with `"enabled": false`.
- **Why disabled:** semgrep-mcp v0.9.0's startup check runs `semgrep --pro --version`,
  which requires the **Pro engine**. Building/using it demands a Semgrep account:
  `semgrep install-semgrep-pro` fails with *"Run `semgrep login` … or ensure your
  SEMGREP_APP_TOKEN variable is set"*. Anonymous usage is impossible with v0.9.0.
- **Prerequisite already done:** `uv tool install semgrep` → `semgrep` 1.177.0 is on
  PATH at `~/.local/bin` (plus `pysemgrep`). The MCP still needs the token.
- **Enable steps:**
  1. Create a free account at https://semgrep.dev and generate an API token
     (dashboard → Settings → API tokens).
  2. Either `semgrep login` in a terminal, or set `SEMGREP_APP_TOKEN=<token>` on the
     server's `environment` in `opencode.jsonc`.
  3. Flip `"enabled": false` → `true` in the semgrep entry.
  4. `opencode mcp list` → semgrep should show connected.
- **Do not** drop the `--python 3.12` flag: semgrep-mcp crashes on import under the
  system Python 3.14 (opentelemetry incompatibility). The entry uses
  `["/home/arfi/.local/bin/uvx", "--python", "3.12", "semgrep-mcp"]` — keep it.

### fw-context-mcp (not added to config)
- Repo: `turbyho/fw-context-mcp` — build-aware code intelligence for embedded C/C++
  firmware (37 tools: symbol lookup, callers/callees, active preprocessor branches,
  Kconfig/Devicetree, HAL selections). Works with Arduino, PlatformIO, ESP-IDF, CMake.
- **Why deferred:** requires (a) `libclang` system library, (b) a `compile_commands.json`
  compilation database — this Arduino repo does not produce one, and generating it needs
  `arduino-cli compile --compilation-database ` (or `orf/arduino-generate-compile-commands`),
  (c) manual OpenCode MCP config was not documented by upstream. `codegraph` (already
  installed) covers basic symbol/call-graph lookup for this small firmware tree.
- **Enable steps** (when the repo's build is worth indexing):
  1. Install libclang: `sudo apt install libclang` (WSL).
  2. Install the tool: `uv tool install fw-context-mcp` (no pip on this Python; uv is present).
  3. Generate the database: set up arduino-cli (see §5), then
     `arduino-cli compile --fqbn esp32:esp32:<board> --compilation-database <sketch>`.
  4. Index: `cd firmware/<SketchName> && fw-context init && fw-context index --build`.
  5. Add to `opencode.jsonc` (stdlib server — check `fw-context-mcp --help` for the exact
     entry point first) with the WSL-native executable path.

### serial-mcp-server (not added to config)
- Repo: `adancurusul/serial-mcp-server` — Rust MCP server for serial ports
  (list/open/read/write/control lines + JSON macro automation). MIT.
- **Why deferred:** needs a Rust toolchain (`cargo` absent), and in WSL the ESP32's USB
  serial is only reachable via **usbipd** USB passthrough — without it the server would
  launch with zero usable ports.
- **Enable steps**:
  1. Install Rust: `curl --proto '=https' --tlsv1.2 -sSf https://sh.rustup.rs | sh`.
  2. Build it: `git clone https://github.com/adancurusul/serial-mcp-server && cd
     serial-mcp-server && cargo install --path . --locked` (binary: `serial-mcp-server`).
  3. Set up USB passthrough (admin PowerShell / Windows side):
     `usbipd bind --busid <ESP32-busid>` then in WSL `usbipd attach --wsl --busid <id>`
     (see https://learn.microsoft.com/windows/wsl/connect-usb). The device appears as
     `/dev/ttyUSB0`; add your user to the `dialout` group.
  4. Add to `opencode.jsonc`:
```jsonc
"serial": {
  "type": "local",
  "command": ["/home/<user>/.cargo/bin/serial-mcp-server", "serve"],
  "environment": { "RUST_LOG": "info" },
  "enabled": true
}
```

## 4. Rejected servers (for the record)

- **ESP RainMaker MCP** (espressif) — official and even has a hosted endpoint, but this
  project's firmware talks to Supabase, not RainMaker cloud.
- **maartenvanels/mcp-arduino-cloud** — Arduino Cloud API control; same Supabase-not-Arduino-Cloud reason.
- **Volt23/mcp-arduino-server** — needs an OpenAI/OpenRouter key + WireViz; the
  hardware-mcp server is better for this repo.
- **Mem0 / memory servers** — the `claude-mem` plugin already provides conversation memory.
- **serpapi / Brave / Tavily / Kagi** — keyed search APIs, redundant with firecrawl + exa.
- **Netdata / Grafana / Redis / Neon / PlanetScale** — infra/monitoring; this project's
  infra is Supabase + Vercel, already covered. Nothing self-hosted to monitor.
- **esp-mcp (ESP-IDF)** — ESP-IDF-specific; this project uses the Arduino framework.

## 5. Post-steps & operational notes

- **arduino-cli in WSL** (for Arduino MCP compile/upload): run the MCP tool
  `opencode mcp call arduino install_arduino_cli`, or run the install commands
  manually:
  ```bash
  curl -fsSL https://raw.githubusercontent.com/arduino/arduino-cli/master/install.sh | sh
  arduino-cli config init && arduino-cli core update-index
  arduino-cli core install esp32:esp32   # ~200 MB toolchain
  ```
- **usbipd** (for flashing/serial from WSL): Windows-side tool — install
  `usbipd-win` (winget), then `usbipd bind` + `usbipd attach --wsl` per device. Until
  then, upload from the portable Arduino IDE on the Windows side; Arduino MCP remains
  useful for compile, board/library management, and pin reference.
- **Local MCP + WSL rule** (invariant): local servers must use the WSL-native Node at
  `~/.local/lib/nodejs/bin/npx` and prepend its `bin` dir to `PATH` as shown above.
  Never "fix" local servers back to a bare `npx`.
- **Config ownership:** `context7`/`gh_grep` come from the `oh-my-opencode-slim`
  plugin — never add duplicates to `opencode.jsonc`. `.secrets/github-pat`
  (`~/.config/opencode/.secrets/github-pat`) is consumed by the github server as
  `{file:...}`; verify it exists and run `chmod 600` on it.
- **Verify after any config change** (after any `opencode.jsonc` edit, restart
  opencode or run `opencode mcp list` to refresh connections):
  ```bash
  opencode mcp list
  ```

## 6. Validation evidence (2026-09-11)

- `opencode mcp list` after Phase 2: 13/14 servers connected; semgrep reported
  `MCP error -32000: Connection closed` (diagnosed as the Pro-engine account gate,
  now disabled). firecrawl, exa, arduino connected successfully.
- `uv 0.12.12` and WSL `npx 10.9.2` verified before configuring local servers.
- `semgrep-mcp --help` verified under `--python 3.12` (fails under system Py 3.14).
- Research sources: awesome-mcp-servers list + upstream READMEs; decisions reviewed by
  two independent review gates (shortlist, config).