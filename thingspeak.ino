/*
  ESP32 + DHT11 -> ThingSpeak
  - Requires Adafruit DHT library
  - ThingSpeak rate limit: 15 seconds between updates
  - Change SSID, PASSWORD, and THINGSPEAK_API_KEY below
*/

#include <WiFi.h>
#include <HTTPClient.h>
#include <DHT.h>

// ------------ User settings ------------
const char* SSID = "biswajit";
const char* PASSWORD = "12345678";
const char* THINGSPEAK_API_KEY = "F07SYXMR41YQUY0A";  // ✅ removed extra space
// Use the default ThingSpeak API endpoint:
const char* THINGSPEAK_HOST = "http://api.thingspeak.com/update";

// DHT settings
#define DHTPIN 4        // data pin (GPIO) where DHT is connected
#define DHTTYPE DHT11   // DHT 11
// ---------------------------------------

DHT dht(DHTPIN, DHTTYPE);

unsigned long lastUpdate = 0;
const unsigned long THINGSPEAK_INTERVAL = 15000UL; // 15 seconds (ThingSpeak minimum)

// Helper: connect to WiFi
void connectWiFi() {
  if (WiFi.status() == WL_CONNECTED) return;

  Serial.printf("Connecting to WiFi '%s' ...\n", SSID);
  WiFi.begin(SSID, PASSWORD);

  unsigned long start = millis();
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
    if (millis() - start > 20000UL) {
      Serial.println("\nFailed to connect within 20s — retrying");
      start = millis();
      WiFi.disconnect();
      WiFi.begin(SSID, PASSWORD);
    }
  }
  Serial.println();
  Serial.print("WiFi connected. IP: ");
  Serial.println(WiFi.localIP());
}

// Send data to ThingSpeak using HTTP GET
bool sendToThingSpeak(float temperature, float humidity) {
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("Not connected to WiFi. Aborting ThingSpeak send.");
    return false;
  }

  HTTPClient http;
  String url = String(THINGSPEAK_HOST) + "?api_key=" + THINGSPEAK_API_KEY +
               "&field1=" + String(temperature, 2) +   // ✅ fixed variable name
               "&field2=" + String(humidity, 2);       // ✅ fixed variable name

  Serial.println("Sending to ThingSpeak:");
  Serial.println(url);

  http.begin(url); // HTTP
  int httpResponseCode = http.GET();

  if (httpResponseCode > 0) {
    String payload = http.getString();
    Serial.printf("ThingSpeak response code: %d\n", httpResponseCode);
    Serial.printf("ThingSpeak payload: %s\n", payload.c_str());
    http.end();
    return (httpResponseCode == HTTP_CODE_OK || payload.toInt() > 0);
  } else {
    Serial.printf("Error on HTTP request: %d\n", httpResponseCode);
    http.end();
    return false;
  }
}

void setup() {
  Serial.begin(115200);
  delay(100);

  dht.begin();
  connectWiFi();

  Serial.println("Setup complete.");
}

void loop() {
  unsigned long now = millis();

  if (now - lastUpdate >= THINGSPEAK_INTERVAL) {
    lastUpdate = now;

    float humidity = dht.readHumidity();
    float temperature = dht.readTemperature(); // Celsius

    if (isnan(humidity) || isnan(temperature)) {
      Serial.println("Failed to read from DHT sensor! Skipping...");
      return;
    }

    Serial.printf("Temperature: %.2f °C, Humidity: %.2f %%\n", temperature, humidity);

    bool ok = sendToThingSpeak(temperature, humidity);
    if (ok) {
      Serial.println("✅ Successfully sent to ThingSpeak.");
    } else {
      Serial.println("❌ Failed to send to ThingSpeak.");
    }
  }

  delay(200);
}
