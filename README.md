# 🌿 VoltraBloom — Hybrid Energy Harvesting System

> **A smart IoT device** that simultaneously harvests energy from **solar panels**, **vertical-axis wind turbines (VAWT)**, and **Soil Microbial Fuel Cells (SMFC)**, monitored in real-time by an ESP32 microcontroller with FreeRTOS dual-core tasking, an interactive 3D web dashboard, and Supabase cloud database logging.

---

## 📁 Project Structure

```text
VOLTRA/
├── README.md                               ← Master documentation & hardware wiring guide
│
├── 🌐 Web & 3D Applications
│   ├── index.html                          ← Live 3D showcase & real-time telemetry dashboard (Chart.js + Three.js + Supabase)
│   ├── viewer_3d.html                      ← Standalone 3D prototype viewer
│   ├── box_akrilik_designer.html           ← 3D acrylic enclosure CAD designer
│   ├── main.js                             ← Production website ES module (Three.js 0.167 + GSAP scroll)
│   ├── style.css                           ← Hand-written CSS design system (replaces Tailwind CDN)
│   ├── vercel.json                         ← Vercel deployment config
│   ├── gallery/                            ← Optimized gallery assets (cad_model, fabrication, wiring)
│   └── logo/                               ← Optimized logos
│
├── ⚡ firmware/                            ← ESP32 Firmware Source Code
│   ├── Project_Voltrabloom_Unified/        ← ⭐ RECOMMENDED: FreeRTOS Dual-Core Firmware v3.0
│   │   ├── Project_Voltrabloom_Unified.ino ← Dual-Core tasking (Core 1: 100Hz Sensors / Core 0: IoT REST & Cloud)
│   │   ├── SupabaseLogger.h                ← Non-blocking HTTPS logger
│   │   ├── supabase_schema.sql             ← PostgreSQL table definitions & hourly aggregation
│   │   └── .clangd
│   ├── Project_Voltrabloom/                ← Standalone WiFi AP + Local Web Server
│   ├── Project_Voltrabloom_Supabase/       ← Cloud IoT logger (Supabase telemetry)
│   └── tester_pertama/                     ← 🧪 Hardware & Sensor Calibration Firmware (No WiFi)
│
├── 📐 3d_models/                           ← 3D CAD, STL, GLB & SVG Model Assets
│   ├── Frantic_Kasi.glb / .stl / .svg      ← Optimized 3D model iterations
│   ├── Frantic_Kasi_v1.glb / .stl / .svg   ← Original 3D models
│   └── archives/                           ← Zip packages of 3D exports
│
├── 📑 documents/                           ← Research & Engineering Documentation
│   ├── papers/                             ← Scientific papers & research publications
│   ├── proposals/                          ← Project proposals & compliance forms
│   ├── schemas/                            ← System wiring & hybrid electrical schemas
│   │   └── supabase_telemetry_schema.sql   ← Master SQL with hourly aggregation & retention
│   └── scripts/                            ← Documentation scripts & presentation drafts
│
├── 🎨 media/                               ← Visual Assets, Graphics & Photos
│   ├── posters/, logos/, photos/, design_projects/
│   └── screenshots/
└── 🛠️ tools/                               ← Portable Arduino IDE (gitignored, ~500MB)
```

> **Two website builds:** `index.html` (this repo's engineering dashboard) opens directly in browser with Tailwind CDN + Chart.js + Supabase. `main.js` + `style.css` + `vercel.json` is the production-optimized build from `voltrabloom-website` (ES modules via import map, no CDN runtime) — deploy that via Vercel for best performance.

---

## ⚡ FreeRTOS Dual-Core Firmware Architecture (v3.0)

The recommended **Unified Firmware** leverages the ESP32's dual Tensilica LX6 cores for zero-latency deterministic execution:

```mermaid
flowchart LR
    subgraph Core1["🧠 CPU Core 1 (Sensor Core - 100Hz)"]
        ADC["ADC1 Hardware Sampling\n(GPIO 32, 33, 34, 35, 36, 39)"]
        Filter["10-Sample Moving Average Filter"]
        Coulomb["Coulomb Counting SoC Math"]
        LCD["20x4 I2C LCD Refresh (2Hz)"]
        ADC --> Filter --> Coulomb --> LCD
    end
    subgraph Mutex["🔒 FreeRTOS Mutex"]
        SharedState["Thread-Safe Telemetry State"]
    end
    subgraph Core0["🌐 CPU Core 0 (Network Core - Background)"]
        WiFiManager["Wi-Fi Auto-Reconnect / AP Mode"]
        REST["HTTP REST API (/api/telemetry)"]
        Supabase["HTTPS Cloud Upload (5s Interval)"]
        WiFiManager --> REST --> Supabase
    end
    Core1 <==> Mutex <==> Core0
```

---

## 🔌 Hardware — ESP32 Pin Mapping

| Sensor / Module | GPIO Pin | ADC Channel | Signal / Function |
| --- | --- | --- | --- |
| **Solar Voltage** | GPIO 32 | ADC1_CH4 | Monocrystalline solar panel via 12V→3.3V divider |
| **Wind Voltage** | GPIO 35 | ADC1_CH7 | Vertical-axis wind generator via 5V→3.3V divider |
| **Soil MFC Voltage** | GPIO 34 | ADC1_CH6 | Soil Microbial Fuel Cell millivolt signal |
| **DC-DC Output** | GPIO 33 | ADC1_CH5 | Regulated DC bus voltage (10V→3.3V divider) |
| **Current Sensor IN** | GPIO 39 (VP) | ADC1_CH3 | ACS712-05B charging current input |
| **Current Sensor OUT** | GPIO 36 (VP) | ADC1_CH0 | ACS712-05B load discharge current output |
| **I2C LCD Display** | GPIO 21 (SDA) / 22 (SCL) | — | 20x4 Character LCD (I2C address: `0x27`) |

> ⚠️ **ADC1 Invariant:** All analog pins are strictly assigned to **ADC1** (GPIOs 32–39). ADC2 is hardware-blocked whenever Wi-Fi is active on ESP32.

---

## 🛠️ Required Libraries (Arduino IDE)

Install via **Arduino IDE → Sketch → Include Library → Manage Libraries**:

| Library | Author | Purpose |
| --- | --- | --- |
| `LiquidCrystal_I2C` | Frank de Brabander | 20x4 I2C character LCD control |
| `WiFi` | Espressif (Built-in) | ESP32 Wi-Fi Station & SoftAP management |
| `HTTPClient` | Espressif (Built-in) | REST API HTTPS client for Supabase logging |
| `WiFiClientSecure` | Espressif (Built-in) | TLS secure socket connection |

**Board Selection:** `ESP32 Dev Module`  
**Upload Speed:** `115200` baud

---

## ☁️ Supabase Cloud Database & Automated Hourly Aggregation

Execute [`documents/schemas/supabase_telemetry_schema.sql`](documents/schemas/supabase_telemetry_schema.sql) in your Supabase SQL Editor to provision:

1. **`telemetry` table**: Fast index on `created_at DESC` for live 5-second streaming.
2. **`telemetry_hourly` table**: Automated hourly rollup for long-term historical analytics.
3. **`aggregate_telemetry_hourly()`**: SQL function computing average voltages, peak generation, and total Watt-hours harvested.
4. **`cleanup_old_raw_telemetry(days)`**: Retention policy pruning raw 5s rows older than 30 days while preserving lifetime hourly summaries.

Current project (reused): `https://qucbixztkrneifeerkaa.supabase.co` — anon key in `firmware/Project_Voltrabloom_Unified/Project_Voltrabloom_Unified.ino:36-37` and `index.html:792-793`. Update both when creating new Supabase project.

---

## 🌐 Website — Production Build (voltrabloom-website)

This repo merges the engineering `VOLTRA` codebase with the optimized `voltrabloom-website` production build.

**Tech Stack:**
- **Three.js 0.167** (ES Module via import map) — 3D product visualization
- **GSAP + ScrollTrigger** — Scroll-driven camera animations
- **Vanilla CSS** (`style.css` ~3KB) — No Tailwind runtime (hand-written design system)
- **Inline SVG icons** — No icon library dependency
- **Vercel** — Deploy via `vercel.json`

**Local Development (production build):**
```bash
# ES modules require HTTP server (not file://)
python3 -m http.server 8080
npx serve .
# VS Code Live Server also works
```
Engineering dashboard (`index.html` with Tailwind CDN + Supabase) still opens directly as file — no server needed but requires internet for CDNs.

**Deployment:**
```bash
npx vercel --prod
# or: GitHub Pages (Settings → Pages → Source: main), Netlify drag-drop
```

**Production Checklist:**
- [ ] Replace gallery image URLs if using expiring CDN auth tokens (now `gallery/` local)
- [ ] Add `PAPER_VOLTRA.pdf` to project root if needed
- [ ] Update `og:url` in `index.html` with actual domain
- [ ] Update Live Data button to your Supabase dashboard URL

**Performance Optimizations (from website build):**
- Glass materials use `MeshStandardMaterial` with opacity instead of costly `transmission` (8× faster)
- Shadow map 1024×1024 with manual updates only on camera movement
- Render loop pauses when canvas off-screen
- Pixel ratio capped at 1.5 for mobile
- Debounced resize handler

**Accessibility:**
- `prefers-reduced-motion` respected
- Semantic HTML, ARIA labels on 3D canvas, keyboard-navigable

---

© 2026 VoltraBloom — Autonomous Hybrid Energy Harvesting Architecture.
