#include <Arduino.h>
#include "AS726X.h"
#include <class1>
#include <vector>
#include <LiquidCrystal_I2C.h>

// Pin Definitions

// pin for taking sensor readings
#define BUTTON_PIN 4

// the three pins for the AS7263 NIR sensor
#define I2C_SDA_PIN 21
#define I2C_SCL_PIN 22
#define RST_PIN 5

// LCD Pins (Standard Wire Bus)
#define LCD_SDA 19
#define LCD_SCL 18

// I2C and Sensor Objects
TwoWire I2C1 = TwoWire(1);
LiquidCrystal_I2C lcd(0x27, 16, 2); // Set the LCD address to 0x27 for a 16 chars and 2 line display
AS726X sensor;

byte GAIN = 3;
byte MEASUREMENT_MODE = 3;

// Vectors for readings
std::vector<float> valueR, valueS, valueT, valueU, valueV, valueW;

void sensorValueCollectionAndResult();

void setup()
{
  Serial.begin(115200);
  delay(1000);
  Serial.println("\n--- iGrowEmbed System Starting ---");

  // 1. Initialize LCD on Pins 18/19 (Standard Wire)
  Wire.begin(LCD_SDA, LCD_SCL);
  lcd.init();
  lcd.backlight();
  lcd.setCursor(0, 0);
  lcd.print("iGrowEmbed v1.0");
  lcd.setCursor(0, 1);
  lcd.print("Initializing...");

  // 2. Initialize Button
  pinMode(BUTTON_PIN, INPUT_PULLUP);

  // 3. Initialize I2C Bus 1 for Sensor (Pins 21/22)
  I2C1.begin(I2C_SDA_PIN, I2C_SCL_PIN, 400000);

  // 4. Hardware Reset Sequence
  pinMode(RST_PIN, OUTPUT);
  digitalWrite(RST_PIN, LOW);
  delay(100);
  digitalWrite(RST_PIN, HIGH);
  delay(1000);

  // 5. Initialize Sensor
  if (sensor.begin(I2C1, GAIN, MEASUREMENT_MODE) == false)
  {
    Serial.println("ERROR: Sensor not found.");
    lcd.clear();
    lcd.print("Sensor Error!");
    while (1)
      ;
  }

  sensor.setIntegrationTime(20);

  lcd.clear();
  lcd.print("Ready to Scan");
  lcd.setCursor(0, 1);
  lcd.print("Press D4 Button");
  Serial.println("Setup Complete.");
}

void loop()
{
  if (digitalRead(BUTTON_PIN) == LOW)
  {
    delay(50); // Debounce
    if (digitalRead(BUTTON_PIN) == LOW)
    {
      lcd.clear();
      lcd.print("Scanning...");
      sensorValueCollectionAndResult();

      while (digitalRead(BUTTON_PIN) == LOW)
      {
        delay(10);
      }
    }
  }
}

void sensorValueCollectionAndResult()
{
  valueR.clear();
  valueS.clear();
  valueT.clear();
  valueU.clear();
  valueV.clear();
  valueW.clear();

  for (int i = 0; i < 3; i++)
  {
    sensor.takeMeasurements();

    valueR.push_back(sensor.getCalibratedR());
    valueS.push_back(sensor.getCalibratedS());
    valueT.push_back(sensor.getCalibratedT());
    valueU.push_back(sensor.getCalibratedU());
    valueV.push_back(sensor.getCalibratedV());
    valueW.push_back(sensor.getCalibratedW());

    // Visual feedback on LCD
    lcd.setCursor(i + 11, 0);
    lcd.print(".");
    Serial.print(".");
  }

  // don't laugh at my naming 😅 (contact me for questions though, I'll be glad to help)
  ActualMeanCalc calc;
  float meanR = calc.calculateTrimmedMean(valueR);
  float meanS = calc.calculateTrimmedMean(valueS);
  float meanT = calc.calculateTrimmedMean(valueT);
  float meanU = calc.calculateTrimmedMean(valueU);
  float meanV = calc.calculateTrimmedMean(valueV);
  float meanW = calc.calculateTrimmedMean(valueW);

  // Displaying a snapshot of values on LCD
  // (R and W represent the two ends of the spectral range for this sensor)
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print(meanR, 2);
  lcd.setCursor(5, 0);
  lcd.print(meanS, 2);
  lcd.setCursor(11, 0);
  lcd.print(meanT, 2);
  lcd.setCursor(0, 1);
  lcd.print(meanU, 2);
  lcd.setCursor(5, 1);
  lcd.print(meanV, 2);
  lcd.setCursor(11, 1);
  lcd.print(meanW, 2);
  // lcd.print("Data Logged!");

  // Detailed output to Serial for my personal use when compiling my excel spreadsheet
  Serial.println("\n--- Spectral Data (Trimmed Mean) ---");
  Serial.print("R: ");
  Serial.println(meanR);
  Serial.print("S: ");
  Serial.println(calc.calculateTrimmedMean(valueS));
  Serial.print("T: ");
  Serial.println(calc.calculateTrimmedMean(valueT));
  Serial.print("U: ");
  Serial.println(calc.calculateTrimmedMean(valueU));
  Serial.print("V: ");
  Serial.println(calc.calculateTrimmedMean(valueV));
  Serial.print("W: ");
  Serial.println(meanW);
  Serial.println("------------------------------------");
}