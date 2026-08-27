#include <Wire.h>
#include <LiquidCrystal_I2C.h>

LiquidCrystal_I2C lcd(0x27, 20, 4);

const int pinSolar  = 32; 
const int pinWind   = 33; 
const int pinSoil   = 34; 
const int pinOutput = 35; 

const float VREF = 3.3;       
const float MAX_ADC = 4095.0; 

void printFormat(float value) {
  char buffer[10]; 
 
  sprintf(buffer, "%05.2f", value); 
  lcd.print(buffer);
}

void setup() {

  Serial.begin(115200);

  lcd.init();
  lcd.backlight();
  
  lcd.setCursor(0, 0);
  lcd.print("       Hello        ");
  lcd.setCursor(0, 1);
  lcd.print("       I am         ");
  lcd.setCursor(0, 2);
  lcd.print("  VOLTRABLOOM :)    ");
  
  delay(7000);
  lcd.clear();
}

void loop() {

  int rawAdcSolar  = analogRead(pinSolar);
  int rawAdcWind   = analogRead(pinWind);
  int rawAdcSoil   = analogRead(pinSoil);
  int rawAdcOutput = analogRead(pinOutput);

  float vPinSolar  = (rawAdcSolar / MAX_ADC) * VREF;
  float vPinWind   = (rawAdcWind / MAX_ADC) * VREF;
  float vPinSoil   = (rawAdcSoil / MAX_ADC) * VREF;
  float vPinOutput = (rawAdcOutput / MAX_ADC) * VREF;


  float vActualSolar = vPinSolar * (12.0 / 3.3);

  float vActualWind = vPinWind * (5.0 / 3.3); 

  float vActualSoil = vPinSoil * (1.0 / 1.0); 

  float vActualOutput = vPinOutput * (10.0 / 3.3); 

  Serial.print("Solar: "); Serial.print(vActualSolar); Serial.print(" V | ");
  Serial.print("Wind: "); Serial.print(vActualWind); Serial.print(" V | ");
  Serial.print("Soil: "); Serial.print(vActualSoil); Serial.print(" V | ");
  Serial.print("OUT DC-DC: "); Serial.print(vActualOutput); Serial.println(" V");

  
  lcd.setCursor(0, 0); 
  lcd.print("Solar : "); 
  printFormat((vActualSolar * 0.95) * 2); 
  lcd.print(" V   "); 
  
  lcd.setCursor(0, 1); 
  lcd.print("Wind  : "); 
  printFormat(vActualWind * 0.992); 
  lcd.print(" V   ");
  
  lcd.setCursor(0, 2); 
  lcd.print("Soil  : "); 
  printFormat(vActualSoil); 
  lcd.print(" V   ");
  
  lcd.setCursor(0, 3); 
  lcd.print("Output: "); 
  printFormat(vActualOutput * 0.96); 
  lcd.print(" V   ");

  delay(1000);
}