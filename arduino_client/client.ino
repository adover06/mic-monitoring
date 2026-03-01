#include <WiFi.h>
#include <HTTPClient.h>

const char* ssid = "SJSU_guest";
const char* password = "";
const char* serverUrl = "http://[IP_ADDRESS]/log"; // Your server's IP

const int micPin = 34;
const int sampleWindow = 50; // Sample window width in mS (50 mS = 20Hz)

void setup() {
  Serial.begin(115200);
  WiFi.begin(ssid, password);
  
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nWiFi Connected!");
}

void loop() {
  unsigned long startMillis = millis();
  unsigned int peakToPeak = 0;
  unsigned int signalMax = 0;
  unsigned int signalMin = 4095;

  // Collect data for 50 milliseconds
  while (millis() - startMillis < sampleWindow) {
    int sample = analogRead(micPin);
    if (sample < 4095) { // toss out spurious readings
      if (sample > signalMax) signalMax = sample;
      else if (sample < signalMin) signalMin = sample;
    }
  }
  
  peakToPeak = signalMax - signalMin; // Amplitude of the sound wave
  
  // Send data to local server if WiFi is up
  if (WiFi.status() == WL_CONNECTED) {
    HTTPClient http;
    http.begin(serverUrl);
    http.addHeader("Content-Type", "application/x-www-form-urlencoded");
    
    String httpRequestData = "level=" + String(peakToPeak);
    int httpResponseCode = http.POST(httpRequestData);
    
    if (httpResponseCode > 0) {
      Serial.print("Data Sent. Level: ");
      Serial.println(peakToPeak);
    } else {
      Serial.print("Error on sending POST: ");
      Serial.println(httpResponseCode);
    }
    http.end();
  }
  
  delay(1000); // Wait 1 second before next sample
}