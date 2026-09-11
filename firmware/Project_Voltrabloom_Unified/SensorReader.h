#ifndef SENSOR_READER_H
#define SENSOR_READER_H

#include "ArduinoCompat.h"

/*
 * ==============================================================================
 * SENSOR READER — Shared ADC1 Moving-Average Filter
 * ==============================================================================
 * Encapsulates the 6-channel circular-buffer moving average used by all
 * production VoltraBloom sketches. Call begin() once in setup(), then
 * update() every iteration (or inside a FreeRTOS task loop).
 *
 * After update(), read calibrated values via:
 *   getSolarV(), getWindV(), getSoilV(), getOutputV(),
 *   getAmpsInA(), getAmpsOutA()
 *
 * Pin allocation (ALL ADC1 — safe with WiFi):
 *   GPIO 32 = Solar, GPIO 35 = Wind, GPIO 34 = Soil,
 *   GPIO 33 = Output, GPIO 39 = AmpsIn (VN), GPIO 36 = AmpsOut (VP)
 *
 * Calibrated for:
 *   Solar 12V divider (×12/3.3, ×1.1 cal, ×2 gain)
 *   Wind 5V divider (×5/3.3)
 *   Soil MFC (1:1)
 *   Output 10V divider (×10/3.3, ×1.1 cal)
 *   ACS712 5A current sensor (1.65 V zero, 185 mV/A)
 * ==============================================================================
 */

// --- DEFAULT PIN CONFIGURATION (ADC1) ---
const int DEFAULT_PIN_SOLAR   = 32;
const int DEFAULT_PIN_WIND    = 35;
const int DEFAULT_PIN_SOIL    = 34;
const int DEFAULT_PIN_OUTPUT  = 33;
const int DEFAULT_PIN_AMPS_IN = 39;
const int DEFAULT_PIN_AMPS_OUT= 36;

// --- CALIBRATION CONSTANTS ---
const float SR_VREF     = 3.3;
const float SR_MAX_ADC  = 4095.0;
const float SR_SOLAR_DIV  = 12.0 / 3.3;
const float SR_SOLAR_CAL  = 1.1;
const float SR_SOLAR_GAIN = 2.0;
const float SR_WIND_DIV   = 5.0 / 3.3;
const float SR_OUT_DIV    = 10.0 / 3.3;
const float SR_OUT_CAL    = 1.1;
const float SR_ACS712_ZERO = 1.65;
const float SR_ACS712_SENS = 0.185;  // 185 mV/A

const int SR_DEFAULT_WINDOW = 10;

class SensorReader {
private:
    int pinSolar, pinWind, pinSoil, pinOutput, pinAmpsIn, pinAmpsOut;
    int windowSize;
    int rIdx;

    // Circular buffers
    int* bufSol;
    int* bufWnd;
    int* bufSli;
    int* bufOut;
    int* bufAIn;
    int* bufAOt;

    // Running sums
    long tSol, tWnd, tSli, tOut, tAIn, tAOt;

    // Calibrated outputs (updated each cycle)
    float vSolar, vWind, vSoil, vOutput, iIn, iOut;

public:
    SensorReader() : windowSize(SR_DEFAULT_WINDOW), rIdx(0),
        tSol(0), tWnd(0), tSli(0), tOut(0), tAIn(0), tAOt(0),
        vSolar(0), vWind(0), vSoil(0), vOutput(0), iIn(0), iOut(0),
        bufSol(nullptr), bufWnd(nullptr), bufSli(nullptr),
        bufOut(nullptr), bufAIn(nullptr), bufAOt(nullptr) {}

    ~SensorReader() {
        delete[] bufSol;  delete[] bufWnd;  delete[] bufSli;
        delete[] bufOut;  delete[] bufAIn;  delete[] bufAOt;
    }

    // Initialize with default pins and window size
    void begin(int window = SR_DEFAULT_WINDOW) {
        begin(DEFAULT_PIN_SOLAR, DEFAULT_PIN_WIND, DEFAULT_PIN_SOIL,
              DEFAULT_PIN_OUTPUT, DEFAULT_PIN_AMPS_IN, DEFAULT_PIN_AMPS_OUT,
              window);
    }

    // Initialize with custom pins and window size
    void begin(int pSolar, int pWind, int pSoil, int pOutput,
               int pAmpsIn, int pAmpsOut, int window = SR_DEFAULT_WINDOW) {
        pinSolar   = pSolar;
        pinWind    = pWind;
        pinSoil    = pSoil;
        pinOutput  = pOutput;
        pinAmpsIn  = pAmpsIn;
        pinAmpsOut = pAmpsOut;
        windowSize = window;
        rIdx = 0;
        tSol = tWnd = tSli = tOut = tAIn = tAOt = 0;

        // Free any previous buffers first so re-calling begin() does not leak heap
        delete[] bufSol;  delete[] bufWnd;  delete[] bufSli;
        delete[] bufOut;  delete[] bufAIn;  delete[] bufAOt;
        bufSol = bufWnd = bufSli = bufOut = bufAIn = bufAOt = nullptr;

        bufSol = new int[window];
        bufWnd = new int[window];
        bufSli = new int[window];
        bufOut = new int[window];
        bufAIn = new int[window];
        bufAOt = new int[window];

        for (int i = 0; i < window; i++) {
            bufSol[i] = bufWnd[i] = bufSli[i] = 0;
            bufOut[i] = bufAIn[i] = bufAOt[i] = 0;
        }
    }

    // Read ADC channels, update moving average, compute calibrated values
    void update() {
        // Subtract oldest sample
        tSol -= bufSol[rIdx]; tWnd -= bufWnd[rIdx]; tSli -= bufSli[rIdx];
        tOut -= bufOut[rIdx]; tAIn -= bufAIn[rIdx]; tAOt -= bufAOt[rIdx];

        // Read new ADC samples (all ADC1 — safe with WiFi)
        bufSol[rIdx] = analogRead(pinSolar);  bufWnd[rIdx] = analogRead(pinWind);
        bufSli[rIdx] = analogRead(pinSoil);   bufOut[rIdx] = analogRead(pinOutput);
        bufAIn[rIdx] = analogRead(pinAmpsIn); bufAOt[rIdx] = analogRead(pinAmpsOut);

        // Add new sample to running sum
        tSol += bufSol[rIdx]; tWnd += bufWnd[rIdx]; tSli += bufSli[rIdx];
        tOut += bufOut[rIdx]; tAIn += bufAIn[rIdx]; tAOt += bufAOt[rIdx];

        // Advance circular index
        if (++rIdx >= windowSize) rIdx = 0;

        // Float conversion: ((sum / window) / MAX_ADC) * VREF
        float vPinSolar = (((float)tSol / windowSize) / SR_MAX_ADC) * SR_VREF;
        float vPinWind  = (((float)tWnd / windowSize) / SR_MAX_ADC) * SR_VREF;
        float vPinSoil  = (((float)tSli / windowSize) / SR_MAX_ADC) * SR_VREF;
        float vPinOut   = (((float)tOut  / windowSize) / SR_MAX_ADC) * SR_VREF;

        // Calibrated physical voltage values
        vSolar  = (vPinSolar * SR_SOLAR_DIV * SR_SOLAR_CAL) * SR_SOLAR_GAIN;
        vWind   = vPinWind * SR_WIND_DIV;
        vSoil   = vPinSoil;  // 1:1 MFC
        vOutput = vPinOut * SR_OUT_DIV * SR_OUT_CAL;

        // ACS712 5A current sensor
        iIn  = ((((float)tAIn / windowSize) / SR_MAX_ADC) * SR_VREF - SR_ACS712_ZERO) / SR_ACS712_SENS;
        iOut = ((((float)tAOt / windowSize) / SR_MAX_ADC) * SR_VREF - SR_ACS712_ZERO) / SR_ACS712_SENS;

        // Deadzone filtering
        const float CURRENT_DEADZONE = 0.05;
        if (iIn  < CURRENT_DEADZONE) iIn  = 0.0;
        if (iOut < CURRENT_DEADZONE) iOut = 0.0;
    }

    // Calibrated getters
    float getSolarV()   const { return vSolar; }
    float getWindV()    const { return vWind; }
    float getSoilV()    const { return vSoil; }
    float getOutputV()  const { return vOutput; }
    float getAmpsInA()  const { return iIn; }
    float getAmpsOutA() const { return iOut; }

    // Raw running sums (for Coulomb counting or custom processing)
    long getRawSolarSum()  const { return tSol; }
    long getRawWindSum()   const { return tWnd; }
    long getRawSoilSum()   const { return tSli; }
    long getRawOutputSum() const { return tOut; }
    long getRawAmpsInSum() const { return tAIn; }
    long getRawAmpsOutSum()const { return tAOt; }
    int  getWindowSize()   const { return windowSize; }
};

#endif // SENSOR_READER_H
