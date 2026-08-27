#include "ArduinoCompat.h"
#include "SupabaseLogger.h"

/*
 * ==============================================================================
 * VOLTRABLOOM UNIFIED HYBRID FIRMWARE v3.0 (Dual-Core FreeRTOS Architecture)
 * Hybrid Energy Harvesting & Management System (HEMS)
 * 
 * Multi-Core Task Distribution:
 * - CORE 1 (High Priority Real-Time): ADC1 sensor acquisition (100Hz), eFuse calibration,
 *   moving-average filtering, Coulomb Counting battery SoC, and I2C LCD refresh.
 * - CORE 0 (Network & IoT Cloud): Wi-Fi Station / SoftAP manager, REST JSON API (/api/telemetry),
 *   and non-blocking background HTTPS Supabase cloud database logging.
 * 
 * Hardware Safety:
 * - All analog sensors strictly allocated to ADC1 (GPIOs 32-39) to eliminate Wi-Fi ADC2 lockups.
 * - Thread-safe state synchronization via FreeRTOS Mutex Semaphores.
 * ==============================================================================
 */

// --- HARDWARE PIN CONFIGURATION (ALL STRICTLY ADC1) ---
const int pinSolar   = 32; // ADC1_CH4 (12V Solar Panel Divider)
const int pinWind    = 35; // ADC1_CH7 (5V Wind Generator Divider)
const int pinSoil    = 34; // ADC1_CH6 (Soil Microbial Fuel Cell)
const int pinOutput  = 33; // ADC1_CH5 (10V DC-DC Output Divider)
const int pinAmpsIn  = 39; // ADC1_CH3 / VN (ACS712 Inflow Current)
const int pinAmpsOut = 36; // ADC1_CH0 / VP (ACS712 Outflow Current)

// --- NETWORK CONFIGURATION ---
const char* ST_SSID     = "ESP32";            // Target router SSID (change to your local Wi-Fi)
const char* ST_PASSWORD = "12345678";         // Target router Password
const char* AP_SSID     = "VoltraBloom_AP";   // Fallback Access Point SSID
const char* AP_PASSWORD = "voltrabloom";      // Fallback Access Point Password

// --- SUPABASE CLOUD DATABASE CONFIGURATION ---
const char* SUPABASE_URL      = "https://qucbixztkrneifeerkaa.supabase.co";
const char* SUPABASE_ANON_KEY = "eyJhbGciOiJIUzI1NiIsInR5cCI6IkpXVCJ9.eyJpc3MiOiJzdXBhYmFzZSIsInJlZiI6InF1Y2JpeHp0a3JuZWlmZWVya2FhIiwicm9sZSI6ImFub24iLCJpYXQiOjE3ODc0MDk0NjAsImV4cCI6MjEwMjk4NTQ2MH0.81XcwBD0kSUausyFieQ0QMwKz7l8GEtKjec1yObbBpQ";

// Hardware Objects
LiquidCrystal_I2C lcd(0x27, 20, 4);
WiFiServer server(80);
SupabaseLogger supabase;

// Electrical & Battery Constants
const float VREF = 3.3;
const float MAX_ADC = 4095.0;
const float BATT_CAPACITY_AH = 2.6;

// Moving average filters (10-sample window)
const int numReadings = 10;
int readSol[numReadings], readWnd[numReadings], readSli[numReadings];
int readOut[numReadings], readAIn[numReadings], readAOt[numReadings];
int rIdx = 0;
long tSol = 0, tWnd = 0, tSli = 0, tOut = 0, tAIn = 0, tAOt = 0;

// Thread-Safe Shared Telemetry Structure
struct SystemTelemetry {
  float vSolar;
  float vWind;
  float vSoil;
  float vOutput;
  float iIn;
  float iOut;
  float battAh;
  float battPercent;
  float powerW;
  bool isAP;
  char ipStr[24];
};

SystemTelemetry sharedTelemetry = { 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 1.3, 50.0, 0.0, false, "192.168.4.1" };

// FreeRTOS Mutex Semaphore for safe cross-core data exchange
SemaphoreHandle_t xTelemetryMutex = NULL;

// Format float for 20x4 LCD
void printFormat(float value) {
  char buffer[10];
  sprintf(buffer, "%05.2f", value);
  lcd.print(buffer);
}

// ==============================================================================
// CORE 1 TASK: SENSOR ACQUISITION, COULOMB COUNTING & LCD REFRESH (100 Hz)
// ==============================================================================
void TaskSensorAcquisition(void *pvParameters) {
  (void) pvParameters;

  unsigned long lastLcdUpdate = 0;
  unsigned long lastTimeMilli = millis();

  for (;;) {
    // 1. Moving average filtering
    tSol -= readSol[rIdx]; tWnd -= readWnd[rIdx]; tSli -= readSli[rIdx];
    tOut -= readOut[rIdx]; tAIn -= readAIn[rIdx]; tAOt -= readAOt[rIdx];

    readSol[rIdx] = analogRead(pinSolar);  readWnd[rIdx] = analogRead(pinWind);
    readSli[rIdx] = analogRead(pinSoil);   readOut[rIdx] = analogRead(pinOutput);
    readAIn[rIdx] = analogRead(pinAmpsIn); readAOt[rIdx] = analogRead(pinAmpsOut);

    tSol += readSol[rIdx]; tWnd += readWnd[rIdx]; tSli += readSli[rIdx];
    tOut += readOut[rIdx]; tAIn += readAIn[rIdx]; tAOt += readAOt[rIdx];

    if (++rIdx >= numReadings) rIdx = 0;

    // 2. High-precision float conversion (strict adherence to esp32_iot_guidelines.md)
    float vPinSolar = (((float)tSol / numReadings) / MAX_ADC) * VREF;
    float vPinWind  = (((float)tWnd / numReadings) / MAX_ADC) * VREF;
    float vPinSoil  = (((float)tSli / numReadings) / MAX_ADC) * VREF;
    float vPinOut   = (((float)tOut / numReadings) / MAX_ADC) * VREF;

    // 3. Calibrate physical values
    float localSolarV  = (vPinSolar * (12.0 / 3.3) * 1.1) * 2.0;
    float localWindV   = vPinWind * (5.0 / 3.3);
    float localSoilV   = vPinSoil * (1.0 / 1.0);
    float localOutputV = vPinOut * (10.0 / 3.3) * 1.1;

    // Current Sensor ACS712 5A module (185mV/A, zero-offset ~1.65V)
    float localArusIn  = ((((float)tAIn / numReadings) / MAX_ADC) * VREF - 1.65) / 0.185;
    float localArusOut = ((((float)tAOt / numReadings) / MAX_ADC) * VREF - 1.65) / 0.185;

    if (localArusIn < 0.05)  localArusIn = 0.0;
    if (localArusOut < 0.05) localArusOut = 0.0;

    float localPowerW = (localSolarV * localArusIn) + (localWindV * 0.2);

    // 4. Coulomb Counting battery integration
    unsigned long now = millis();
    float dtHours = (now - lastTimeMilli) / 3600000.0;
    lastTimeMilli = now;

    float currentIn_mA = localArusIn * 1000.0;
    float currentBattAh = 1.3;
    float currentSoc = 50.0;

    if (xSemaphoreTake(xTelemetryMutex, pdMS_TO_TICKS(10)) == pdTRUE) {
      currentBattAh = sharedTelemetry.battAh;
      
      if (currentIn_mA > 15.0 || localArusOut > 0.05) {
        currentBattAh = currentBattAh + (localArusIn * dtHours) - (localArusOut * dtHours);

        if (currentBattAh > BATT_CAPACITY_AH) currentBattAh = BATT_CAPACITY_AH;
        if (currentBattAh < 0.0) currentBattAh = 0.0;

        currentSoc = (currentBattAh / BATT_CAPACITY_AH) * 100.0;

        // Float charge tapering detection at full voltage
        if (currentIn_mA < 50.0 && localSolarV > 12.0 && currentIn_mA > 15.0) {
          currentSoc = 100.0;
          currentBattAh = BATT_CAPACITY_AH;
        }
      } else {
        currentSoc = (currentBattAh / BATT_CAPACITY_AH) * 100.0;
      }

      if (currentSoc > 100.0) currentSoc = 100.0;
      if (currentSoc < 0.0)   currentSoc = 0.0;

      // Update shared telemetry state
      sharedTelemetry.vSolar = localSolarV;
      sharedTelemetry.vWind = localWindV;
      sharedTelemetry.vSoil = localSoilV;
      sharedTelemetry.vOutput = localOutputV;
      sharedTelemetry.iIn = localArusIn;
      sharedTelemetry.iOut = localArusOut;
      sharedTelemetry.battAh = currentBattAh;
      sharedTelemetry.battPercent = currentSoc;
      sharedTelemetry.powerW = localPowerW;

      xSemaphoreGive(xTelemetryMutex);
    }

    // 5. Update 20x4 I2C LCD (2 Hz rate to avoid I2C bus congestion)
    if (now - lastLcdUpdate >= 500) {
      lastLcdUpdate = now;
      lcd.setCursor(0, 0); lcd.print("Solar: "); printFormat(localSolarV); lcd.print("V  ");
      lcd.setCursor(0, 1); lcd.print("Wind : "); printFormat(localWindV);  lcd.print("V  ");
      lcd.setCursor(0, 2); lcd.print("Soil : "); printFormat(localSoilV);  lcd.print("V  ");
      lcd.setCursor(0, 3); lcd.print("Out: "); printFormat(localOutputV); lcd.print("V ");
      lcd.setCursor(12, 3); lcd.print("B:"); lcd.print((int)currentSoc); lcd.print("%   ");
    }

    vTaskDelay(pdMS_TO_TICKS(10)); // 100 Hz sampling rate
  }
}

// ==============================================================================
// CORE 0 TASK: WI-FI NETWORKING, REST JSON API & SUPABASE CLOUD (Background)
// ==============================================================================
void TaskNetworkAndCloud(void *pvParameters) {
  (void) pvParameters;

  unsigned long lastCloudUpload = 0;

  for (;;) {
    SystemTelemetry snap;
    if (xSemaphoreTake(xTelemetryMutex, pdMS_TO_TICKS(20)) == pdTRUE) {
      snap = sharedTelemetry;
      xSemaphoreGive(xTelemetryMutex);
    }

    // 1. Background Supabase Cloud Upload (Every 5 seconds in Station mode)
    if (!snap.isAP && (millis() - lastCloudUpload >= 5000)) {
      lastCloudUpload = millis();
      supabase.logSerialAndSupabase(
        snap.vSolar, snap.vWind, snap.vSoil, snap.vOutput,
        snap.iIn, snap.iOut, snap.battPercent, snap.battAh,
        5000
      );
    }

    // 2. Handle HTTP Client requests on Port 80
    WiFiClient client = server.available();
    if (client) {
      String requestLine = "";
      while (client.connected()) {
        if (client.available()) {
          char c = client.read();
          if (c == '\n') {
            if (requestLine.length() == 0) {
              break;
            }

            // CORS-enabled JSON endpoint for index.html
            if (requestLine.indexOf("GET /api/telemetry") >= 0) {
              client.println("HTTP/1.1 200 OK");
              client.println("Content-Type: application/json");
              client.println("Access-Control-Allow-Origin: *");
              client.println("Access-Control-Allow-Methods: GET");
              client.println("Connection: close\r\n");

              char jsonBuf[384];
              snprintf(jsonBuf, sizeof(jsonBuf),
                "{"
                "\"solar_v\":%.2f,"
                "\"wind_v\":%.2f,"
                "\"soil_v\":%.2f,"
                "\"output_v\":%.2f,"
                "\"current_in_a\":%.2f,"
                "\"current_out_a\":%.2f,"
                "\"battery_percent\":%.1f,"
                "\"battery_ah\":%.2f,"
                "\"power_w\":%.2f,"
                "\"mode\":\"%s\""
                "}",
                snap.vSolar, snap.vWind, snap.vSoil, snap.vOutput,
                snap.iIn, snap.iOut, snap.battPercent, snap.battAh,
                snap.powerW, snap.isAP ? "AP" : "STA"
              );
              client.println(jsonBuf);
              break;
            }
            // Built-in Mobile Responsive Status Page
            else if (requestLine.indexOf("GET /") >= 0) {
              client.println("HTTP/1.1 200 OK\nContent-type:text/html\nConnection: close\n");
              client.println("<!DOCTYPE html><html><head><meta name=\"viewport\" content=\"width=device-width,initial-scale=1\"><meta http-equiv=\"refresh\" content=\"1\">");
              client.println("<style>body{font-family:sans-serif;background:#FAF6EE;color:#261E14;text-align:center;padding:20px;}.card{background:#fff;padding:16px;margin:10px auto;max-width:380px;border-radius:12px;box-shadow:0 4px 12px rgba(0,0,0,0.05);text-align:left;border-left:5px solid #E2A03F;}.val{font-size:24px;font-weight:700;color:#261E14;}</style></head><body>");
              client.println("<h2>🌿 VOLTRABLOOM HUB v3.0</h2><p style='color:#8a7e6e;font-size:12px;'>Dual-Core FreeRTOS Node &bull; IP: " + String(snap.ipStr) + "</p>");
              client.print("<div class='card'>Solar Panel<div class='val'>"); client.print(snap.vSolar, 2); client.println(" V</div></div>");
              client.print("<div class='card'>Wind Turbine<div class='val'>"); client.print(snap.vWind, 2); client.println(" V</div></div>");
              client.print("<div class='card'>Soil MFC<div class='val'>"); client.print(snap.vSoil, 2); client.println(" V</div></div>");
              client.print("<div class='card' style='border-left-color:#4D7C0F'>Battery SoC (2.6Ah)<div class='val' style='color:#4D7C0F'>"); client.print((int)snap.battPercent); client.println(" %</div></div>");
              client.print("<div class='card' style='border-left-color:#2c3e50'>DC Output<div class='val'>"); client.print(snap.vOutput, 2); client.println(" V</div></div>");
              client.println("<p style='font-size:11px;color:#c4b9a8'>API Endpoint: <a href='/api/telemetry'>/api/telemetry</a></p>");
              client.println("</body></html>");
              break;
            }
            requestLine = "";
          } else if (c != '\r') {
            requestLine += c;
          }
        }
      }
      client.stop();
    }

    vTaskDelay(pdMS_TO_TICKS(10)); // Yield to network stack
  }
}

// ==============================================================================
// MAIN SETUP (Initializes Hardware, Wi-Fi & Dispatches FreeRTOS Tasks)
// ==============================================================================
void setup() {
  WRITE_PERI_REG(RTC_CNTL_BROWN_OUT_REG, 0); // Disable brownout reset for pack stability
  Serial.begin(115200);

  // Create Mutex Semaphore
  xTelemetryMutex = xSemaphoreCreateMutex();

  // Initialize LCD
  lcd.init();
  lcd.backlight();
  lcd.setCursor(0, 0); lcd.print("     VOLTRABLOOM    ");
  lcd.setCursor(0, 1); lcd.print(" FreeRTOS Dual-Core ");
  lcd.setCursor(0, 2); lcd.print("  Firmware v3.0     ");
  lcd.setCursor(0, 3); lcd.print("  Starting Tasks... ");
  delay(2000);
  lcd.clear();

  // Reset moving average buffers
  for (int i = 0; i < numReadings; i++) {
    readSol[i] = 0; readWnd[i] = 0; readSli[i] = 0;
    readOut[i] = 0; readAIn[i] = 0; readAOt[i] = 0;
  }

  // Attempt Wi-Fi Station connection (8s timeout)
  lcd.setCursor(0, 0); lcd.print("Connecting Wi-Fi");
  WiFi.mode(WIFI_STA);
  WiFi.begin(ST_SSID, ST_PASSWORD);

  unsigned long startAttempt = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - startAttempt < 8000) {
    delay(500);
    lcd.print(".");
  }

  bool isAPMode = false;
  String currentIp = "";

  if (WiFi.status() == WL_CONNECTED) {
    isAPMode = false;
    currentIp = WiFi.localIP().toString();
    lcd.clear();
    lcd.setCursor(0, 0); lcd.print("WIFI CONNECTED!");
    lcd.setCursor(0, 1); lcd.print("Mode: Station");
    lcd.setCursor(0, 2); lcd.print("IP: " + currentIp);
    supabase.init(SUPABASE_URL, SUPABASE_ANON_KEY, "telemetry");
  } else {
    WiFi.mode(WIFI_AP);
    WiFi.softAP(AP_SSID, AP_PASSWORD);
    isAPMode = true;
    currentIp = WiFi.softAPIP().toString();
    lcd.clear();
    lcd.setCursor(0, 0); lcd.print("AP MODE STARTED");
    lcd.setCursor(0, 1); lcd.print("SSID: " + String(AP_SSID));
    lcd.setCursor(0, 2); lcd.print("IP: " + currentIp);
  }

  // Update initial shared state
  if (xSemaphoreTake(xTelemetryMutex, portMAX_DELAY) == pdTRUE) {
    sharedTelemetry.isAP = isAPMode;
    strncpy(sharedTelemetry.ipStr, currentIp.c_str(), sizeof(sharedTelemetry.ipStr) - 1);
    xSemaphoreGive(xTelemetryMutex);
  }

  server.begin();
  delay(2500);
  lcd.clear();

  // ==============================================================================
  // DISPATCH FREERTOS TASKS TO DEDICATED CPU CORES
  // ==============================================================================
  
  // Task 1: Sensor Sampling & SoC Math -> Pinned to CORE 1 (Priority 2)
  xTaskCreatePinnedToCore(
    TaskSensorAcquisition,
    "TaskSensor",
    4096,
    NULL,
    2,
    NULL,
    1 // Pin to Core 1
  );

  // Task 2: Wi-Fi REST Server & Supabase Cloud -> Pinned to CORE 0 (Priority 1)
  xTaskCreatePinnedToCore(
    TaskNetworkAndCloud,
    "TaskNetwork",
    8192,
    NULL,
    1,
    NULL,
    0 // Pin to Core 0
  );

  Serial.println("[FreeRTOS] Tasks dispatched to Core 0 (Network) and Core 1 (Sensors)");
}

// Arduino main loop is left idle as execution is handled entirely by FreeRTOS tasks
void loop() {
  vTaskDelay(pdMS_TO_TICKS(1000));
}
