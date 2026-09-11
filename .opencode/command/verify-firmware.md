---
description: Verify all firmware sketches compile (arduino-cli) and obey repo invariants (ADC1 pins, folder/sketch name match)
---

# Firmware Verification

Verify every sketch in `firmware/` against the repo invariants, then compile each one.

## Channel Use

- Run the checks yourself as the agent, using the arduino MCP (`compile_sketch`, `get_board_details`) or `arduino-cli` via bash.
- Do NOT delegate to @fixer or ask the user to flash anything. This is a read-only/compile-only verification.

## Inventory (current)

| Folder | Sketch file | Name match | Notes |
|--------|-------------|------------|-------|
| `Project_Voltrabloom` | `Project_Voltrabloom.ino` | ✅ | Older standalone AP + web server |
| `Project_Voltrabloom_Supabase` | `Project_Voltrabloom.ino` | ❌ MISMATCH | Known issue; folder != sketch name, Arduino IDE may refuse. Do not rename without user intent — report only. |
| `Project_Voltrabloom_Unified` | `Project_Voltrabloom_Unified.ino` | ✅ | Recommended firmware (FreeRTOS dual-core) |
| `tester_pertama` | `tester_pertama.ino` | ✅ | Sensor calibration, no WiFi; pins 33/35 deliberately swapped (see comment) |

Re-scan `firmware/` with glob first — do not trust this table if new folders appeared.

## Steps

1. **Inventory**: glob `firmware/*/`. For each folder, list files; confirm exactly one `.ino` whose basename matches the folder name. Any mismatch is a FAIL (report, don't fix).
2. **Pin audit** (all sketches except `tester_pertama` unless it changed): every analog sensor pin must be on ADC1 (32, 33, 34, 35, 36, 39). Pitfall: ADC2 is unusable while WiFi is on. Grep `firmware/**/*.ino` and `*.h` for `pin(Solar|Wind|Soil|Output|AmpsIn|AmpsOut)` and for any other raw `analogRead(` argument → each must be an ADC1 pin. Any ADC2 pin (25, 26, 27, 13, 12, 14) is a FAIL.
3. **Compile**: for each sketch folder, run `arduino_compile_sketch` with fqbn `esp32:esp32:esp32` (auto-install core if missing). Record pass/fail per sketch. Add `--warnings all`-equivalent via the MCP `warnings` param if supported.
   - The Supabase sketch's folder/name mismatch is EXPECTED to fail or be refused — that is a known FAIL, not a regression. State it as such.
4. **Report**: a compact table:

   | Sketch | Name match | ADC1 ok | Compile |
   |--------|-----------|---------|---------|

   Plus a one-line summary: `N/4 compile clean, M known issues, K new problems`. If everything passes, say so plainly and stop.

## Hard rules

- Never upload or flash anything. Compile only.
- Never rename `Project_Voltrabloom_Supabase/Project_Voltrabloom.ino` — known issue, report-only.
- Report new failures loudly; confirm known failures don't regress (no new warnings beyond what was there).