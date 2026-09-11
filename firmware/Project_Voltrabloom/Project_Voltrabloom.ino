#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <WiFi.h>
#include "soc/soc.h"
#include "soc/rtc_cntl_reg.h"
#include "SensorReader.h"

LiquidCrystal_I2C lcd(0x27, 20, 4);
WiFiServer server(80);

const char* ssid     = "ESP32";
const char* password = "12345678";

const float BATT_CAPACITY_AH = 2.6;

SensorReader sensor;

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

  sensor.begin();

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
  sensor.update();
  vDisplaySolar  = sensor.getSolarV();
  vDisplayWind   = sensor.getWindV();
  vDisplaySoil   = sensor.getSoilV();
  vDisplayOutput = sensor.getOutputV();
  arusMasukA     = sensor.getAmpsInA();
  arusKeluarA    = sensor.getAmpsOutA();

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

    // Logika Tapering Tambahan: Jika arus masuk mulai mengecil (<50 mA) tetapi panel surya mendeteksi tegangan penuh (>12 V), paksa SoC ke 100%
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

  // LCD 20x4 display
  lcd.setCursor(0, 0); lcd.print("Solar : "); printFormat(vDisplaySolar); lcd.print(" V   ");
  lcd.setCursor(0, 1); lcd.print("Wind  : "); printFormat(vDisplayWind);  lcd.print(" V   ");
  lcd.setCursor(0, 2); lcd.print("Soil  : "); printFormat(vDisplaySoil);  lcd.print(" V   ");
  lcd.setCursor(0, 3); lcd.print("Out: "); printFormat(vDisplayOutput); lcd.print(" V ");
  lcd.setCursor(13, 3); lcd.print("B:"); lcd.print((int)persenBaterai); lcd.print("%   ");

  // HTTP web server
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
  delay(30);
}
