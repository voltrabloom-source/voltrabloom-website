---
trigger: model_decision
description: Invariants and critical constraints for ESP32 firmware, ADC sensor readings, WiFi coexistence, and Arduino sketch organization.
---

# ESP32 & Embedded IoT Invariants

1. **ADC Pin Allocation & WiFi Coexistence**:
   - On ESP32, ADC2 cannot be used when WiFi is active. Always assign analog sensors to **ADC1** (GPIO 32, 33, 34, 35, 36/VP, 39/VN).
2. **Precision Floating Point Division**:
   - Never perform integer division on ADC sample averages. Always cast: `(((float)sampleSum / count) / 4095.0) * VREF`.
3. **Battery State of Charge (SoC)**:
   - For hybrid storage systems, calculate SoC using Coulomb Counting with clamping and float-charge tapering detection.
4. **Arduino Sketch Naming**:
   - Always place `.ino` files inside a directory of the exact same name: `firmware/<SketchName>/<SketchName>.ino`.
5. **C++ Header Hygiene**:
   - Never define or extern-declare global object instances in `.h` headers. Only declare classes/structs.
