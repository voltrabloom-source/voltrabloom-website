#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <WiFi.h>
#include "soc/soc.h"
#include "soc/rtc_cntl_reg.h"

LiquidCrystal_I2C lcd(0x27, 20, 4);
WiFiServer server(80);

const char* ssid     = "ESP32";
const char* password = "12345678";

// PERBAIKAN: Seluruh 6 pin di bawah ini menggunakan blok ADC1 (Aman bersama Wi-Fi)
// pinAmpsOut sekarang menggunakan pin VP (GPIO 36) sesuai konfigurasi baru Anda
const int pinSolar = 32, pinWind = 35, pinSoil = 34, pinOutput = 33, pinAmpsIn = 39, pinAmpsOut = 36; 

const float VREF = 3.3, MAX_ADC = 4095.0, BATT_CAPACITY_AH = 2.6; 

const int numReadings = 10;
int readSol[numReadings], readWnd[numReadings], readSli[numReadings], readOut[numReadings], readAIn[numReadings], readAOt[numReadings];
int rIdx = 0;
long tSol = 0, tWnd = 0, tSli = 0, tOut = 0, tAIn = 0, tAOt = 0;

float vDisplaySolar, vDisplayWind, vDisplaySoil, vDisplayOutput, arusMasukA, arusKeluarA;
float bateraiIsiAh = 1.3, persenBaterai = 50.0; // Memulai dari asumsi tengah (50%) demi keamanan kalibrasi awal
unsigned long waktuLamaMilli = 0;

void printFormat(float value) {
  char buffer[10]; 
  sprintf(buffer, "%05.2f", value); 
  lcd.print(buffer);
}

void setup() {
  WRITE_PERI_REG(RTC_CNTL_BROWN_OUT_REG, 0); 
  Serial.begin(115200);
  lcd.init();
  lcd.backlight();
  
  lcd.setCursor(0, 0); lcd.print("       Hello        ");
  lcd.setCursor(0, 1); lcd.print("       I am         ");
  lcd.setCursor(0, 2); lcd.print("  VOLTRABLOOM :)    ");
  delay(2000); 
  lcd.clear();

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
  lcd.setCursor(0, 2); lcd.print("IP:");
  lcd.setCursor(4, 2); lcd.print(WiFi.localIP()); 
  delay(3000); 
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

  // PERBAIKAN: Ditambahkan cast (float) agar kalkulasi pecahan akurat (bukan pembagian integer bulat)
  float vPinSolar = (((float)tSol / numReadings) / MAX_ADC) * VREF;
  float vPinWind  = (((float)tWnd / numReadings) / MAX_ADC) * VREF;
  float vPinSoil  = (((float)tSli / numReadings) / MAX_ADC) * VREF;
  float vPinOut   = (((float)tOut / numReadings) / MAX_ADC) * VREF;

  // --- KALIBRASI NILAI VOLTASE FISIK NYATA ---
  vDisplaySolar  = (vPinSolar * (12.0 / 3.3) * 1.1) * 2;
  vDisplayWind   = vPinWind * (5.0 / 3.3);
  vDisplaySoil   = vPinSoil * (1.0 / 1.0);
  vDisplayOutput = vPinOut * (10.0 / 3.3) * 1.1;

  // --- KALIBRASI DATA SENSOR ARUS ---
  arusMasukA  = ((((float)tAIn / numReadings) / MAX_ADC) * VREF - 1.65) / 0.185;
  arusKeluarA = ((((float)tAOt / numReadings) / MAX_ADC) * VREF - 1.65) / 0.185;
  
  if (arusMasukA < 0.05) arusMasukA = 0.0;
  if (arusKeluarA < 0.05) arusKeluarA = 0.0;

  unsigned long wktSkrg = millis();
  float JedaJam = (wktSkrg - waktuLamaMilli) / 3600000.0;
  waktuLamaMilli = wktSkrg;

  float arusMasuk_mA = arusMasukA * 1000.0;

  // PERBAIKAN LOGIKA KELISTRIKAN: Kalkulasi SoC Berbasis Integrasi Arus (Coulomb Counting murni)
  if (arusMasuk_mA > 15.0 || arusKeluarA > 0.05) {
    bateraiIsiAh = bateraiIsiAh + (arusMasukA * JedaJam) - (arusKeluarA * JedaJam);
    
    // Pembatasan kapasitas fisik baterai
    if (bateraiIsiAh > BATT_CAPACITY_AH) bateraiIsiAh = BATT_CAPACITY_AH;
    if (bateraiIsiAh < 0.0) bateraiIsiAh = 0.0;
    
    persenBaterai = (bateraiIsiAh / BATT_CAPACITY_AH) * 100.0;
    
    // Logika Tapering Tambahan: Jika arus masuk mulai mengecil (<50mA) tetapi panel surya mendeteksi tegangan penuh (>12V), paksa SoC ke 100%
    if (arusMasuk_mA < 50.0 && vDisplaySolar > 12.0 && arusMasuk_mA > 15.0) {
       persenBaterai = 100.0;
       bateraiIsiAh = BATT_CAPACITY_AH;
    }
  } else {
    // Mode standby ketika arus masuk/keluar di bawah ambang batas deteksi
    persenBaterai = (bateraiIsiAh / BATT_CAPACITY_AH) * 100.0;
  }

  if (persenBaterai > 100.0) persenBaterai = 100.0;
  if (persenBaterai < 0.0)   persenBaterai = 0.0;

  // --- VISUALISASI LAYAR LCD 20x4 ---
  lcd.setCursor(0, 0); lcd.print("Solar : "); printFormat(vDisplaySolar); lcd.print(" V   "); 
  lcd.setCursor(0, 1); lcd.print("Wind  : "); printFormat(vDisplayWind);  lcd.print(" V   ");
  lcd.setCursor(0, 2); lcd.print("Soil  : "); printFormat(vDisplaySoil);  lcd.print(" V   ");
  
  lcd.setCursor(0, 3); lcd.print("Out: "); printFormat(vDisplayOutput); lcd.print("V"); 
  lcd.setCursor(13, 3); lcd.print("B:"); lcd.print((int)persenBaterai); lcd.print("%   ");

  // --- SISTEM WEB SERVER ---
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
            client.print("<div class='card' style='border-left-color:#27ae60'>BATERAI REAL 2.6Ah (SoC)<div class='value' style='color:#27ae60'>"); client.print((int)persenBaterai); client.println(" %</div></div>");
            client.println("</body></html>\n");
            break;
          } else { currentLine = ""; }
        } else if (c != '\r') { currentLine += c; }
      }
    }
    client.stop();
  }
  delay(30); 
}
