#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <WiFi.h>
#include "soc/soc.h"
#include "soc/rtc_cntl_reg.h"
#include "SupabaseLogger.h"

LiquidCrystal_I2C lcd(0x27, 20, 4);
WiFiServer server(80);

// FIX: Changed SSID from '..," (typo) to a valid placeholder.
// SECURITY WARNING: Move credentials to a separate config_secrets.h file before production deployment.
#warning "SECURITY: WiFi credentials are hardcoded. Move to a secrets file before deployment!"
const char* ssid     = "ESP32";        // Replace with your actual network SSID
const char* password = "12345678";     // Replace with your actual network password

// --- KONFIGURASI SUPABASE DATABASE ---
const char* SUPABASE_URL      = "https://qucbixztkrneifeerkaa.supabase.co";
const char* SUPABASE_ANON_KEY = "eyJhbGciOiJIUzI1NiIsInR5cCI6IkpXVCJ9.eyJpc3MiOiJzdXBhYmFzZSIsInJlZiI6InF1Y2JpeHp0a3JuZWlmZWVya2FhIiwicm9sZSI6ImFub24iLCJpYXQiOjE3ODc0MDk0NjAsImV4cCI6MjEwMjk4NTQ2MH0.81XcwBD0kSUausyFieQ0QMwKz7l8GEtKjec1yObbBpQ";

SupabaseLogger supabase;

const int pinSolar = 32, pinWind = 35, pinSoil = 34, pinOutput = 33, pinAmpsIn = 39, pinAmpsOut = 27;

// AKURAT: Batas kapasitas diatur tepat ke 2.6 Ah (2600 mAh) sesuai baterai seri Anda
const float VREF = 3.3, MAX_ADC = 4095.0, BATT_CAPACITY_AH = 2.6; 

const int numReadings = 10;
int readSol[numReadings], readWnd[numReadings], readSli[numReadings], readOut[numReadings], readAIn[numReadings], readAOt[numReadings];
int rIdx = 0;
long tSol = 0, tWnd = 0, tSli = 0, tOut = 0, tAIn = 0, tAOt = 0;

float vDisplaySolar, vDisplayWind, vDisplaySoil, vDisplayOutput, arusMasukA, arusKeluarA;
float bateraiIsiAh = 0.0, persenBaterai = 0.0; // Memulai kalkulasi aman dari 0%
unsigned long waktuLamaMilli = 0;

void printFormat(float value) {
  char buffer[10]; // Aman: Ukuran buffer 10 digit, anti-stuck memori ESP32
  sprintf(buffer, "%05.2f", value); 
  lcd.print(buffer);
}

void setup() {
  WRITE_PERI_REG(RTC_CNTL_BROWN_OUT_REG, 0); // Matikan brownout reset agar kuat ditenagai baterai seri
  Serial.begin(115200);
  lcd.init();
  lcd.backlight();
  
  lcd.setCursor(0, 0); lcd.print("       Hello        ");
  lcd.setCursor(0, 1); lcd.print("       I am         ");
  lcd.setCursor(0, 2); lcd.print("  VOLTRABLOOM :)    ");
  delay(7000);
  lcd.clear();

  // Inisialisasi awal seluruh memori filter dengan nilai 0
  for (int i = 0; i < numReadings; i++) {
    readSol[i] = 0; readWnd[i] = 0; readSli[i] = 0;
    readOut[i] = 0; readAIn[i] = 0; readAOt[i] = 0;
  }

  lcd.setCursor(0, 0); lcd.print("Connecting WiFi ");
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) { delay(500); lcd.print("."); }
  
  server.begin();
  lcd.clear();
  lcd.setCursor(0, 0); lcd.print("WIFI CONNECTED!");
  lcd.setCursor(0, 2); lcd.print("Buka Web lewat IP:");
  lcd.setCursor(0, 3); lcd.print(WiFi.localIP()); 
  
  // Inisialisasi Supabase REST Client
  supabase.init(SUPABASE_URL, SUPABASE_ANON_KEY, "telemetry");

  delay(5000); 
  lcd.clear();
  waktuLamaMilli = millis();
}

void loop() {
  // --- KONTROL FILTER JALUR MOVING AVERAGE ---
  tSol -= readSol[rIdx]; tWnd -= readWnd[rIdx]; tSli -= readSli[rIdx];
  tOut -= readOut[rIdx]; tAIn -= readAIn[rIdx]; tAOt -= readAOt[rIdx];

  readSol[rIdx] = analogRead(pinSolar);  readWnd[rIdx] = analogRead(pinWind);
  readSli[rIdx] = analogRead(pinSoil);   readOut[rIdx] = analogRead(pinOutput);
  readAIn[rIdx] = analogRead(pinAmpsIn); readAOt[rIdx] = analogRead(pinAmpsOut);

  tSol += readSol[rIdx]; tWnd += readWnd[rIdx]; tSli += readSli[rIdx];
  tOut += readOut[rIdx]; tAIn += readAIn[rIdx]; tAOt += readAOt[rIdx];

  if (++rIdx >= numReadings) rIdx = 0;

  // --- MENGHITUNG RATA-RATA TEGANGAN PIN ADC ---
  // FIX: Added (float) casts to prevent integer division truncation.
  // Without the cast, (tSol / numReadings) performs integer division (long/int),
  // truncating the fractional component before float multiplication — causing inaccurate readings.
  float vPinSolar = (((float)tSol / numReadings) / MAX_ADC) * VREF;
  float vPinWind  = (((float)tWnd / numReadings) / MAX_ADC) * VREF;
  float vPinSoil  = (((float)tSli / numReadings) / MAX_ADC) * VREF;
  float vPinOut   = (((float)tOut / numReadings) / MAX_ADC) * VREF;

  // --- KALIBRASI NILAI VOLTASE FISIK NYATA ---
  vDisplaySolar  = (vPinSolar * (12.0 / 3.3) * 0.95) * 2;
  vDisplayWind   = vPinWind * (5.0 / 3.3) * 0.992;
  vDisplaySoil   = vPinSoil * (1.0 / 1.0);
  vDisplayOutput = vPinOut * (10.0 / 3.3) * 0.96;

  // --- KALIBRASI DATA SENSOR ARUS ---
  // FIX: Also apply float cast to current sensor calculations
  arusMasukA  = ((((float)tAIn / numReadings) / MAX_ADC) * VREF - 1.65) / 0.185;
  arusKeluarA = ((((float)tAOt / numReadings) / MAX_ADC) * VREF - 1.65) / 0.185;
  
  if (arusMasukA < 0.05) arusMasukA = 0.0;
  if (arusKeluarA < 0.05) arusKeluarA = 0.0;

  unsigned long wktSkrg = millis();
  float JedaJam = (wktSkrg - waktuLamaMilli) / 3600000.0;
  waktuLamaMilli = wktSkrg;

  // --- INTEGRASI LOGIKA ADAPTIF KAPASITAS 2.6 Ah ---
  float arusMasuk_mA = arusMasukA * 1000.0;

  // FIX: Replaced broken charging SoC logic with correct coulomb-counting approach.
  // ORIGINAL BUG: The previous logic set persenBaterai=0% when arusMasuk >= 500 mA (high charge)
  // and had a formula (arusMasuk_mA - 50.0) that was always <= 0 when arusMasuk_mA <= 50.0.
  // This is now replaced with the same correct coulomb counting used in the local version.
  if (arusMasuk_mA > 15.0 || arusKeluarA > 0.05) {
    bateraiIsiAh = bateraiIsiAh + (arusMasukA * JedaJam) - (arusKeluarA * JedaJam);
    
    // Clamp to physical battery capacity
    if (bateraiIsiAh > BATT_CAPACITY_AH) bateraiIsiAh = BATT_CAPACITY_AH;
    if (bateraiIsiAh < 0.0) bateraiIsiAh = 0.0;
    
    persenBaterai = (bateraiIsiAh / BATT_CAPACITY_AH) * 100.0;
    
    // Tapering detection: if charging current is in float-charge range (<50 mA) 
    // and solar is at full output (>12 V), assume battery is topped off at 100%.
    if (arusMasuk_mA < 50.0 && vDisplaySolar > 12.0 && arusMasuk_mA > 15.0) {
       persenBaterai = 100.0;
       bateraiIsiAh = BATT_CAPACITY_AH;
    }
  } else {
    // Standby mode: hold last calculated SoC
    persenBaterai = (bateraiIsiAh / BATT_CAPACITY_AH) * 100.0;
  }

  if (persenBaterai > 100.0) persenBaterai = 100.0;
  if (persenBaterai < 0.0)   persenBaterai = 0.0;

  // --- LOGGING SERIAL MONITOR & STREAMING SUPABASE DATABASE (Interval 5 Detik) ---
  supabase.logSerialAndSupabase(
    vDisplaySolar, vDisplayWind, vDisplaySoil, vDisplayOutput, 
    arusMasukA, arusKeluarA, persenBaterai, bateraiIsiAh, 
    5000 // Upload non-blocking setiap 5000 ms (5 detik)
  );

  // --- VISUALISASI LAYAR LCD 20x4 SESUAI REQUEST ---
  lcd.setCursor(0, 0); lcd.print("Solar : "); printFormat(vDisplaySolar); lcd.print(" V   "); 
  lcd.setCursor(0, 1); lcd.print("Wind  : "); printFormat(vDisplayWind);  lcd.print(" V   ");
  lcd.setCursor(0, 2); lcd.print("Soil  : "); printFormat(vDisplaySoil);  lcd.print(" V   ");
  
  // Baris 4 menampilkan data tegangan Output sekaligus Indikator Baterai (B)
  lcd.setCursor(0, 3); lcd.print("Out: "); printFormat(vDisplayOutput); lcd.print(" V "); 
  lcd.setCursor(13, 3); lcd.print("B:"); lcd.print((int)persenBaterai); lcd.print("%   ");

  // --- SISTEM LAYANAN WEB SERVER INTERNET HTTP ---
  WiFiClient client = server.available();
  if (client) {
    String currentLine = "";
    while (client.connected()) {
      if (client.available()) {
        char c = client.read();
        if (c == '\n') {
          if (currentLine.length() == 0) {
            client.println("HTTP/1.1 200 OK\nContent-type:text/html\nConnection: close\n");
            client.println("<!DOCTYPE html><html><head><meta name=\"viewport\" content=\"width=device-width, initial-scale=1\"><meta http-equiv=\"refresh\" content=\"1\">");
            client.println("<style>html{font-family:Arial;text-align:center;background:#f4f7f6;}.card{background:white;padding:12px;margin:8px auto;max-width:400px;border-radius:8px;box-shadow:0 4px 6px rgba(0,0,0,0.05);border-left:5px solid #007bff;text-align:left;}.value{font-size:22px;font-weight:bold;color:#2c3e50;}</style></head><body>");
            client.println("<h1>VOLTRABLOOM HUB</h1>");
            client.print("<div class='card'>Solar Panel<div class='value'>"); client.print(vDisplaySolar, 2); client.println(" V</div></div>");
            client.print("<div class='card'>Kincir Angin<div class='value'>"); client.print(vDisplayWind, 2); client.println(" V</div></div>");
            client.print("<div class='card'>Energi Tanah<div class='value'>"); client.print(vDisplaySoil, 2); client.println(" V</div></div>");
            client.print("<div class='card'>Output DC-DC<div class='value'>"); client.print(vDisplayOutput, 2); client.println(" V</div></div>");
            client.print("<div class='card' style='border-left-color:#e67e22'>Arus Masuk<div class='value'>"); client.print(arusMasukA, 2); client.println(" A</div></div>");
            client.print("<div class='card' style='border-left-color:#e74c3c'>Arus Keluar<div class='value'>"); client.print(arusKeluarA, 2); client.println(" A</div></div>");
            client.print("<div class='card' style='border-left-color:#27ae60'>BATERAI REAL 2.6 Ah (SoC)<div class='value' style='color:#27ae60'>"); client.print((int)persenBaterai); client.println(" %</div></div>");
            client.println("</body></html>\n");
            break;
          } else { currentLine = ""; }
        } else if (c != '\r') { currentLine += c; }
      }
    }
    client.stop();
  }
  delay(30); // Kecepatan filter optimal 0.3 detik stabil
}
