#include <WiFi.h>
#include <WebServer.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include "config.h"

// ================= Pin Definitions =================
const int POT_PIN   = 34;   // Potentiometer wiper (ADC1, input-only pin)
const int MOTOR_PIN = 27;   // Motor PWM output -> 220R -> S8050 base
const int LED_PIN   = 26;   // Independent LED controlled via web UI

// ================= Globals =================
bool ledState = false;
Adafruit_SSD1306 display(128, 64, &Wire, -1);
WebServer server(80);
const int PWM_CHANNEL = 0;
unsigned long lastOLEDUpdate = 0;
int lastMotorSpeed = 0;     // matches the ledcWrite(0) in setup()

// ================= Web Frontend =================
void handleRoot() {
  String html = "<!DOCTYPE html><html><head><meta charset='UTF-8'>";
  html += "<meta name='viewport' content='width=device-width, initial-scale=1.0'>";
  html += "<style>body{font-family:Arial;text-align:center;margin-top:50px;}";
  html += "button{padding:15px 30px;font-size:18px;margin:10px;cursor:pointer;border-radius:8px;}</style>";
  html += "</head><body>";
  html += "<h1>ESP32 Control Center</h1>";
  html += "<p>Toggle the standalone LED below</p>";
  html += "<button onclick=\"control('/led/on')\" style='background:#4CAF50;color:white;'>LED ON</button>";
  html += "<button onclick=\"control('/led/off')\" style='background:#f44336;color:white;'>LED OFF</button>";
  html += "<p id='status' style='color:gray;'>Waiting...</p>";
  html += "<script>";
  html += "function control(action) {";
  html += "  fetch(action).then(response => response.text()).then(data => {";
  html += "    document.getElementById('status').innerText = 'Status: ' + data;";
  html += "  });";
  html += "}";
  html += "</script></body></html>";
  server.send(200, "text/html", html);
}

// ================= Hardware Handlers =================
void handleLedOn() {
  digitalWrite(LED_PIN, HIGH);
  ledState = true;
  Serial.println("LED ON  - P26 driven HIGH");
  server.send(200, "text/plain", "LED ON");
}

void handleLedOff() {
  digitalWrite(LED_PIN, LOW);
  ledState = false;
  Serial.println("LED OFF - P26 driven LOW");
  server.send(200, "text/plain", "LED OFF");
}

// ================= OLED Refresh =================
void updateOLED(int motorSpeed) {
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0, 0);
  display.println("== ESP32 Station ==");
  display.print("IP: ");  display.println(WiFi.localIP());
  display.print("PWM: "); display.print(motorSpeed); display.println("/255");

  display.drawRect(0, 40, 128, 10, SSD1306_WHITE);
  int barWidth = map(motorSpeed, 0, 255, 0, 128);
  display.fillRect(0, 40, barWidth, 10, SSD1306_WHITE);

  display.setCursor(0, 55);
  display.print("Web LED: "); display.println(ledState ? "ON" : "OFF");
  display.display();
}

// ================= Setup =================
void setup() {
  Serial.begin(115200);

  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, LOW);

  ledcSetup(PWM_CHANNEL, 1000, 8);
  ledcAttachPin(MOTOR_PIN, PWM_CHANNEL);
  ledcWrite(PWM_CHANNEL, 0);

  Wire.begin(21, 22);
  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println("SSD1306 init failed");
    for (;;);
  }

  display.clearDisplay();
  display.setCursor(0, 0);
  display.println("Connecting WiFi...");
  display.display();

  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nWiFi connected. IP: " + WiFi.localIP().toString());

  server.on("/",         handleRoot);
  server.on("/led/on",   handleLedOn);
  server.on("/led/off",  handleLedOff);
  server.begin();

  updateOLED(0);
}

// ================= Main Loop =================
void loop() {
  server.handleClient();

  int motorSpeed = map(analogRead(POT_PIN), 0, 4095, 0, 255);

  // Only touch the PWM peripheral when the duty cycle actually changes: this
  // loop runs far faster than the pot can move.
  if (motorSpeed != lastMotorSpeed) {
    ledcWrite(PWM_CHANNEL, motorSpeed);
    lastMotorSpeed = motorSpeed;
  }

  if (millis() - lastOLEDUpdate >= 100) {
    updateOLED(motorSpeed);
    lastOLEDUpdate = millis();
  }
}
