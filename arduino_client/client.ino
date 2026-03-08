#include <WiFi.h>
#include <HTTPClient.h>

// WiFi / server
const char* ssid = "SJSU_guest";
const char* password = "";
const char* serverUrl = "http://sce.sjsu.edu/sound";

// ADC / mic
const int micPin = 34;                 // Use an ADC1 pin (GPIO32-39)
const int adcMax = 4095;

// Timing
const uint32_t sampleWindowMs = 5000;  // how long we "listen" each report (ms)
const uint32_t postIntervalMs = 300000; // 5 minutes

// API
const String API_KEY = "Your_API_Key_Here"; // Replace with your actual API key

// Envelope follower tuning
// Attack: how fast it rises to new loud sounds (higher = faster)
// Release: how fast it falls back down (lower = slower)
float env = 0.0f;
const float attack  = 0.35f;   // try 0.2 - 0.6
const float release = 0.02f;   // try 0.005 - 0.05

// DC center tracking (removes bias drift)
float center = 2048.0f;
const float centerAlpha = 0.0015f; // smaller = slower tracking

// RMS window size per envelope update (tradeoff: smoothness vs responsiveness)
const int rmsBlockSamples = 200; // try 100-400

void connectWiFi() {
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);

  Serial.print("Connecting to WiFi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nWiFi Connected!");
}

void setup() {
  Serial.begin(115200);

  // ADC config
  analogReadResolution(12);
  analogSetPinAttenuation(micPin, ADC_11db);

  connectWiFi();
}

void loop() {
  unsigned long startMillis = millis();
  env = 0.0f; // reset envelope each measurement window (or remove this line to keep running)

  while (millis() - startMillis < sampleWindowMs) {

    // Compute RMS over a short block, update center while sampling
    double sumsq = 0.0;

    for (int i = 0; i < rmsBlockSamples; i++) {
      int raw = analogRead(micPin);

      // track bias center slowly
      center += (raw - center) * centerAlpha;

      float ac = raw - center;
      sumsq += (double)ac * (double)ac;
    }

    float rms = sqrt(sumsq / (double)rmsBlockSamples);

    // Envelope follower on RMS
    if (rms > env) env += (rms - env) * attack;
    else           env += (rms - env) * release;

    // Optional: print for debugging/plotting during the 5s window
    // Serial.print((int)rms); Serial.print(" "); Serial.println((int)env);
  }

  // This is the value we'll send
  int level = (int)env;

  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("WiFi lost, reconnecting...");
    WiFi.disconnect(true);
    delay(1000);
    connectWiFi();
  }

  if (WiFi.status() == WL_CONNECTED) {
    HTTPClient http;
    http.begin(serverUrl);
    http.addHeader("Content-Type", "application/x-www-form-urlencoded");
    http.addHeader("X-API-Key", API_KEY);  // use your constant

    String httpRequestData = "level=" + String(level);
    int httpResponseCode = http.POST(httpRequestData);

    if (httpResponseCode > 0) {
      Serial.print("Data Sent. Level: ");
      Serial.println(level);
    } else {
      Serial.print("Error sending POST. Level: ");
      Serial.println(level);
      Serial.print("HTTP code: ");
      Serial.println(httpResponseCode);
    }
    http.end();
  }

  delay(postIntervalMs); // Wait 5 minutes before next sample+post
}