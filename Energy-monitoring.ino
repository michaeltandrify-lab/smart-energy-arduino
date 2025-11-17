#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>

// --- CONFIG ---
const char* ssid = "realme";
const char* password = "michael21";
const char* serverUrl = "http://172.30.239.185:3000/api/logs";  
const int device_id = 6;
const unsigned long sendInterval = 5000;

// --- LEDS ---
const int LED_RUN  = 2;   // clignote = system OK
const int LED_WIFI = 4;   // ON si WiFi connecté
const int LED_POST = 5;   // ON si POST OK

// --- PINS ---
const int PIN_V = 34;  // ZMPT101B
const int PIN_I = 35;  // SCT-013 avec bias 1.65V

// --- ADC ---
const float ADC_REF = 3.3;
const int ADC_MAX = 4095;

// --- CALIBRATION ---
const float FACTEUR_V = 203.70;
const float CURRENT_SENSITIVITY = 20.86;

// --- Variables ---
double totalKWh = 0.0;
unsigned long lastSend = 0;
unsigned long lastMillisEnergy = 0;
unsigned long lastBlink = 0;
bool ledRunState = false;

struct {
  float vrms, irms, real_power, apparent_power, power_factor, delta_kwh;
} sendBuffer;

const float V_BIAS = 1.65;
const float I_BIAS = 1.65;

void setup() {
  Serial.begin(115200);
  
  // --- LED OUTPUTS ---
  pinMode(LED_RUN, OUTPUT);
  pinMode(LED_WIFI, OUTPUT);
  pinMode(LED_POST, OUTPUT);

  digitalWrite(LED_RUN, LOW);
  digitalWrite(LED_WIFI, LOW);
  digitalWrite(LED_POST, LOW);

  analogReadResolution(12);
  pinMode(PIN_V, INPUT);
  pinMode(PIN_I, INPUT);

  Serial.println("Démarrage EnergyMonitor JIRAMA - calibration appliquée");

  WiFi.setAutoReconnect(true);
  WiFi.begin(ssid, password);
  Serial.print("Connexion WiFi");

  int attempts = 0;
  while (WiFi.status() != WL_CONNECTED && attempts < 40) {
    delay(500);
    Serial.print(".");
    attempts++;
  }

  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("\nWiFi connecté");
    Serial.print("IP: ");
    Serial.println(WiFi.localIP());
    digitalWrite(LED_WIFI, HIGH);   // WiFi OK
  } else {
    Serial.println("\nÉchec WiFi");
    digitalWrite(LED_WIFI, LOW);    // WiFi NO
  }

  lastMillisEnergy = millis();
}

// --- CLIGNOTEMENT LED_RUN ---
void blinkRunLed(unsigned long t) {
  if (t - lastBlink > 500) { // clignote chaque 0.5s
    ledRunState = !ledRunState;
    digitalWrite(LED_RUN, ledRunState);
    lastBlink = t;
  }
}

float measureAndCompute(unsigned long nowMillis) {
  const int SAMPLES = 1800;
  const int SAMPLE_US = 150;

  double sumV2 = 0, sumI2 = 0, sumP = 0;

  for (int n = 0; n < SAMPLES; n++) {
    float v_adc = analogRead(PIN_V) * ADC_REF / ADC_MAX;
    float i_adc = analogRead(PIN_I) * ADC_REF / ADC_MAX;

    float v_inst = v_adc - V_BIAS;
    float i_inst = (i_adc - I_BIAS) / CURRENT_SENSITIVITY;
    float v_primary = v_inst * FACTEUR_V;

    sumV2 += v_inst * v_inst;
    sumI2 += i_inst * i_inst;
    sumP += v_primary * i_inst;

    delayMicroseconds(SAMPLE_US);
  }

  float vrms_adc = sqrt(sumV2 / SAMPLES);
  float vrms = vrms_adc * FACTEUR_V;
  float irms = sqrt(sumI2 / SAMPLES);
  float realPower = sumP / SAMPLES;
  if (realPower < 0) realPower = 0;

  float apparent = vrms * irms;
  float pf = (apparent > 0.1f) ? (realPower / apparent) : 0.0f;
  pf = constrain(pf, 0.0f, 1.0f);

  double dt = (nowMillis - lastMillisEnergy) / 1000.0;
  lastMillisEnergy = nowMillis;
  float delta_kwh = (realPower * dt) / 3600000.0f;

  totalKWh += delta_kwh;

  sendBuffer = { vrms, irms, realPower, apparent, pf, delta_kwh };

  Serial.printf("\n=== MESURE JIRAMA (9W) ===\n");
  Serial.printf("Vrms: %.1f V\n", vrms);
  Serial.printf("Irms: %.4f A\n", irms);
  Serial.printf("P: %.2f W | S: %.2f VA | PF: %.3f\n", realPower, apparent, pf);
  Serial.printf("ΔkWh: %.9f | Total: %.6f kWh\n", delta_kwh, totalKWh);

  return delta_kwh;
}

void sendData(float delta_kwh) {
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("WiFi perdu");
    digitalWrite(LED_WIFI, LOW);
    return;
  }

  digitalWrite(LED_WIFI, HIGH);

  HTTPClient http;
  WiFiClient client;

  http.begin(client, serverUrl);
  http.addHeader("Content-Type", "application/json");

  StaticJsonDocument<256> doc;
  doc["vrms"] = sendBuffer.vrms;
  doc["irms"] = sendBuffer.irms;
  doc["real_power"] = sendBuffer.real_power;
  doc["apparent_power"] = sendBuffer.apparent_power;
  doc["power_factor"] = sendBuffer.power_factor;
  doc["delta_kwh"] = delta_kwh;
  doc["device_id"] = device_id;

  String json;
  serializeJson(doc, json);

  int code = http.POST(json);
  if (code > 0) {
    Serial.printf("Envoyé | Code: %d\n", code);
    digitalWrite(LED_POST, HIGH);   // POST OK
  } else {
    Serial.printf("Échec | %s\n", http.errorToString(code).c_str());
    digitalWrite(LED_POST, LOW);    // POST FAIL
  }
  http.end();
}

void loop() {
  unsigned long t = millis();

  blinkRunLed(t);  // LED système

  if (t - lastSend >= sendInterval) {
    float dkwh = measureAndCompute(t);
    sendData(dkwh);
    lastSend = t;
  }
}
