#include <sameterkanboz-project-1_inferencing.h>

#include <WiFi.h>
#include <WebServer.h>
#include <Ticker.h>


#include <Wire.h>
#include <LiquidCrystal_I2C.h>


#include <eloquent_esp32cam.h>
#include <eloquent_esp32cam/edgeimpulse/fomo.h>

using eloq::camera;
using eloq::ei::fomo;

LiquidCrystal_I2C lcd(0x27, 16, 2);

#define SDA_PIN 14  // I2C SDA pin
#define SCL_PIN 15  // I2C SCL pin


const char* ssid = "ESP_AP";
const char* password = "12345678";

WebServer server(80);
Ticker ticker;
int fakeSensorValue = 0;
String degistirilebilirString = "____****_____";
void handleRoot() {
  // HTML ve JavaScript kodunu içeren yanıt
  String html = "<!DOCTYPE html><html lang=\"en\"><head><meta charset=\"UTF-8\">";
  html += "<title>ESP Web Server</title></head><body>";
  html += "<h1>ESP Web Sunucusu</h1>";
  html += "<p>Fake Sensor Value: <span id=\"sensorValue\">0</span></p>";
  html += "<button onclick=\"fetchSensorValue()\">Get Sensor Value</button>";
  html += "<p>FOMO Results: <pre id=\"fomoResults\"></pre></p>";
  html += "<script>";
  html += "function fetchSensorValue() {";
  html += "fetch('/sensor')";
  html += ".then(response => response.json())";
  html += ".then(data => document.getElementById('sensorValue').innerText = data.value)";
  html += ".catch(error => console.error('Error:', error));";
  html += "}";
  html += "function fetchFomoResults() {";
  html += "fetch('/fomo')";
  html += ".then(response => response.json())";
  html += ".then(data => {";
  html += "document.getElementById('fomoResults').innerText = JSON.stringify(data, null, 2);";
  html += "})";
  html += ".catch(error => console.error('Error:', error));";
  html += "}";
  html += "setInterval(fetchSensorValue, 1000);"; // Her saniye sensör değerini güncelle
  html += "setInterval(fetchFomoResults, 1000);"; // Her saniye FOMO sonuçlarını güncelle
  html += "</script></body></html>";

  server.send(200, "text/html", html);
}

void handleSensor() {
  String response = "{\"value\":" + String(fakeSensorValue) + "}";
  server.send(200, "application/json", response);
}

void handleFomo() {
  // capture picture
  if (!camera.capture().isOk()) {
    server.send(500, "text/plain", "Camera capture failed");
    return;
  }

  // run FOMO
  if (!fomo.run().isOk()) {
    server.send(500, "text/plain", "FOMO run failed");
    return;
  }

  // prepare JSON response
  String jsonResponse = "{";
  jsonResponse += "\"count\":" + String(fomo.count()) + ",";
  jsonResponse += "\"objects\":[";

  bool firstObject = true;
  fomo.forEach([&](int i, bbox_t bbox) {
    if (!firstObject) {
      jsonResponse += ",";
    } else {
      firstObject = false;
    }

    degistirilebilirString = String(bbox.label);
    lcd.print(degistirilebilirString);
    jsonResponse += "{";
    jsonResponse += "\"label\":\"" + String(bbox.label) + "\",";
    jsonResponse += "\"x\":" + String(bbox.x) + ",";
    jsonResponse += "\"y\":" + String(bbox.y) + ",";
    jsonResponse += "\"width\":" + String(bbox.width) + ",";
    jsonResponse += "\"height\":" + String(bbox.height) + ",";
    jsonResponse += "\"proba\":" + String(bbox.proba);
    jsonResponse += "}";
  });

  jsonResponse += "]";
  jsonResponse += "}";

  server.send(200, "application/json", jsonResponse);
}

void incrementSensorValue() {
  fakeSensorValue++;
}

void setup() {

Wire.begin(SDA_PIN, SCL_PIN);

  // LCD ekranını başlat
  lcd.begin(16, 2);
  lcd.init();
  lcd.backlight();

  // Ekrana "Merhaba" yaz
  lcd.setCursor(0, 0);

  delay(3000);
  Serial.begin(115200);
  Serial.println("__EDGE IMPULSE FOMO (NO-PSRAM)__");

  // camera settings
  camera.pinout.aithinker();
  camera.brownout.disable();
  camera.resolution.yolo();
  camera.pixformat.rgb565();

  // init camera
  while (!camera.begin().isOk())
    Serial.println(camera.exception.toString());

  Serial.println("Camera OK");
  Serial.println("Put object in front of camera");

  WiFi.softAP(ssid, password);

  Serial.println();
  Serial.print("Access Point Başlatıldı: ");
  Serial.println(ssid);
  Serial.print("IP Adresi: ");
  Serial.println(WiFi.softAPIP());

  server.on("/", handleRoot);
  server.on("/sensor", handleSensor);
  server.on("/fomo", handleFomo);

  server.begin();
  Serial.println("HTTP sunucusu başlatıldı");

  // Her saniye fakeSensorValue değerini artır
  ticker.attach(1, incrementSensorValue);
}

void loop() {
  server.handleClient();
  
}

