#include <Arduino.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <ArduinoJson.h>
#include "AS726X.h"
#include <class1>
#include <vector>
#include <LiquidCrystal_I2C.h>
#include <secrets.h>

// --- Configuration ---
const char *ssid = SSID;
const char *password = PASSWORD;
const char *apiKey = GEMINI_API_KEY; // !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
const char *host = "generativelanguage.googleapis.com";

// Pin Definitions
#define BUTTON_PIN 4
#define I2C_SDA_PIN 21
#define I2C_SCL_PIN 22
#define RST_PIN 5
#define LCD_SDA 19
#define LCD_SCL 18

TwoWire I2C1 = TwoWire(1);
LiquidCrystal_I2C lcd(0x27, 16, 2);
AS726X sensor;

byte GAIN = 3;
byte MEASUREMENT_MODE = 3;

std::vector<float> valueR, valueS, valueT, valueU, valueV, valueW;

// Function Prototypes
void sensorValueCollectionAndResult();
void askGemini(String sensorData);

void setup()
{
    Serial.begin(115200);
    delay(1000);

    // 1. WiFi Setup
    WiFi.mode(WIFI_STA);
    WiFi.begin(ssid, password);

    // 2. LCD Init
    Wire.begin(LCD_SDA, LCD_SCL);
    lcd.init();
    lcd.backlight();
    lcd.print("iGrowEmbed v1.1");

    // 3. Sensor & Hardware Init
    pinMode(BUTTON_PIN, INPUT_PULLUP);
    I2C1.begin(I2C_SDA_PIN, I2C_SCL_PIN, 400000);
    pinMode(RST_PIN, OUTPUT);
    digitalWrite(RST_PIN, LOW);
    delay(100);
    digitalWrite(RST_PIN, HIGH);

    if (sensor.begin(I2C1, GAIN, MEASUREMENT_MODE) == false)
    {
        lcd.clear();
        lcd.print("Sensor Error!");
        while (1)
            ;
    }
    sensor.setIntegrationTime(20);

    while (WiFi.status() != WL_CONNECTED)
    {
        delay(500);
        Serial.print(".");
    }

    lcd.clear();
    lcd.print("Ready to Scan");
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
                delay(10);
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
        lcd.setCursor(i + 11, 0);
        lcd.print(".");
    }

    ActualMeanCalc stuff;
    String dataString = "R:" + String(stuff.calculateTrimmedMean(valueR)) +
                        ",S:" + String(stuff.calculateTrimmedMean(valueS)) +
                        ",T:" + String(stuff.calculateTrimmedMean(valueT)) +
                        ",U:" + String(stuff.calculateTrimmedMean(valueU)) +
                        ",V:" + String(stuff.calculateTrimmedMean(valueV)) +
                        ",W:" + String(stuff.calculateTrimmedMean(valueW));

    lcd.clear();
    lcd.print("Analyzing...");
    askGemini(dataString);
}

void askGemini(String sensorData)
{
    WiFiClientSecure client;
    client.setInsecure();
    client.setTimeout(10); // 10 seconds is plenty even for 4G

    // 1. Connection with immediate exit on fail
    if (!client.connect(host, 443))
    {
        lcd.clear();
        lcd.print("Conn Failed");
        delay(2000);
        return; // Returns to loop() so button works again
    }

    String systemPrompt = "Analyze NIR data for Watermelon. Only output 'Ripeness: X%' and 'HD: +/- X weeks'.";

    JsonDocument doc;
    doc["system_instruction"]["parts"][0]["text"] = systemPrompt;
    doc["contents"][0]["parts"][0]["text"] = sensorData;
    doc["generationConfig"]["maxOutputTokens"] = 40;
    doc["generationConfig"]["temperature"] = 0.2;

    String requestBody;
    serializeJson(doc, requestBody);

    String url = "/v1beta/models/gemini-3.1-flash-lite-preview:streamGenerateContent?alt=sse&key=" + String(apiKey);

    client.print("POST " + url + " HTTP/1.1\r\n");
    client.print("Host: " + String(host) + "\r\n");
    client.print("Content-Type: application/json\r\n");
    client.print("Content-Length: " + String(requestBody.length()) + "\r\n");
    client.print("Connection: close\r\n\r\n");
    client.print(requestBody);

    // 2. Skip Headers with a safety counter
    unsigned long headerTimeout = millis();
    while (client.connected())
    {
        if (millis() - headerTimeout > 5000)
            break; // Don't hang here for more than 5s
        String line = client.readStringUntil('\n');
        if (line == "\r")
            break;
    }

    lcd.clear();
    lcd.print("Receiving...");

    unsigned long streamStartTime = millis();
    bool receivedAnyData = false;
    String fullResponse = "";

    // 3. Robust Stream Processing
    while (client.connected() || client.available())
    {
        // Absolute cutoff to prevent permanent freeze
        if (millis() - streamStartTime > 15000)
            break;

        if (client.available())
        {
            String line = client.readStringUntil('\n');
            if (line.startsWith("data: "))
            {
                if (!receivedAnyData)
                {
                    lcd.clear();
                    receivedAnyData = true;
                }

                JsonDocument chunkDoc;
                DeserializationError error = deserializeJson(chunkDoc, line.substring(6));

                if (!error)
                {
                    const char *text = chunkDoc["candidates"][0]["content"]["parts"][0]["text"];
                    if (text)
                    {
                        fullResponse += text;
                        Serial.print(text);

                        // Simple 2-line wrap for 16x2 LCD
                        lcd.setCursor(0, 0);
                        lcd.print(fullResponse.substring(0, 16));
                        if (fullResponse.length() > 16)
                        {
                            lcd.setCursor(0, 1);
                            lcd.print(fullResponse.substring(16, 32));
                        }
                    }
                }
            }
        }
    }

    client.stop(); // Release memory/socket
    if (!receivedAnyData)
    {
        lcd.clear();
        lcd.print("No AI Response");
    }
    delay(3000); // Hold result on screen
}