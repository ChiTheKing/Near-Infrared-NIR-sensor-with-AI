
#include "AS726X.h"
#include <Arduino.h>
#include <class1>

#define BUTTON_PIN 4
#define I2C_SDA_PIN 21
#define I2C_SCL_PIN 22

AS726X sensor; // Creates the sensor object

byte GAIN = 3;
byte MEASUREMENT_MODE = 3;

void sensorValueCollectionAndResult();

std::vector<float> valueR; // What we want is to solve for the avarage value in each array!
std::vector<float> valueS;
std::vector<float> valueT;
std::vector<float> valueU;
std::vector<float> valueV;
std::vector<float> valueW;

void setup()
{

  pinMode(4, INPUT_PULLUP);
  Wire.begin(I2C_SDA_PIN, I2C_SCL_PIN);
  Serial.begin(115200);
  sensor.begin(Wire, GAIN, MEASUREMENT_MODE);
  sensor.setIntegrationTime(9);
}

void loop()
{

  if (digitalRead(4) == LOW)
  {
    delay(100);
    if (digitalRead(4) == LOW)
    {
      sensorValueCollectionAndResult();
    }
    while (digitalRead(4) == LOW)
    {
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

  for (int i = 0; i < 10; i++)
  {
    sensor.takeMeasurementsWithBulb();
    // Near IR readings
    float r = sensor.getCalibratedR();
    float s = sensor.getCalibratedS();
    float t = sensor.getCalibratedT();
    float u = sensor.getCalibratedU();
    float v = sensor.getCalibratedV();
    float w = sensor.getCalibratedW();

    valueR.push_back(r);
    valueS.push_back(s);
    valueT.push_back(t);
    valueU.push_back(u);
    valueV.push_back(v);
    valueW.push_back(w);
  }

  Stuff stuff;
  float meanOfR = stuff.calculateTrimmedMean(valueR);
  float meanOfS = stuff.calculateTrimmedMean(valueS);
  float meanOfT = stuff.calculateTrimmedMean(valueT);
  float meanOfU = stuff.calculateTrimmedMean(valueU);
  float meanOfV = stuff.calculateTrimmedMean(valueV);
  float meanOfW = stuff.calculateTrimmedMean(valueW);

  Serial.println("Ripeness Information is processing...");
  Serial.println(meanOfR);
  Serial.println(meanOfS);
  Serial.println(meanOfT);
  Serial.println(meanOfU);
  Serial.println(meanOfV);
  Serial.println(meanOfW);
}
