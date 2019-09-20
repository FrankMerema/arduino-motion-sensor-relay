#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <ESP8266WebServer.h>
#include <ArduinoJson.h>

// Define Wifi credentials
#define WIFISSID "Fam Merema"
#define WIFIPSK "Mylo08072016!"

#define RELAYPIN 0
#define SENSORPIN 2
#define LEDPIN LED_BUILTIN

IPAddress staticIp(192, 168, 2, 127);
IPAddress gateway(192, 168, 2, 254);
IPAddress subnet(255, 255, 255, 0);

// Initialize web server
ESP8266WebServer server(80);

int relayStatus = HIGH; // OFF
int motionStatus = LOW; // NO MOTION
int statusLed = HIGH; // ON

void ICACHE_RAM_ATTR toggleRelay() {
  relayStatus = relayStatus == LOW ? HIGH : LOW;
  motionStatus = motionStatus == LOW ? HIGH : LOW;
  Serial.println("Toggling Relay to: " + String(relayStatus) + " via interrupt!");
  digitalWrite(RELAYPIN, relayStatus);
}

// REQUEST HANDLERS -----------------------------------------------
void handleStatusRequest() {
  Serial.println("Handling status request");
  DynamicJsonDocument doc(JSON_ARRAY_SIZE(2) + 3 * JSON_OBJECT_SIZE(2));

  doc["status"] = "OK";

  JsonArray sensors = doc.createNestedArray("sensors");

  JsonObject sensors_0 = sensors.createNestedObject();
  sensors_0["type"] = "relay";
  sensors_0["sensorStatus"] = relayStatus;

  JsonObject sensors_1 = sensors.createNestedObject();
  sensors_1["type"] = "motion";
  sensors_1["sensorStatus"] = motionStatus;

  String st;
  serializeJson(doc, st);
  server.send(200, "application/json", st);
}

void handeRestartRequest() {
  Serial.println("Handling restart request");
  Serial.println("Restarting...");

  server.send(200, "text/plain", "");

  ESP.restart();
}

void handleRelayToggleRequest() {
  Serial.println("Handling toggle request");
  
  relayStatus = relayStatus == LOW ? HIGH : LOW;
  digitalWrite(RELAYPIN, relayStatus);

  DynamicJsonDocument doc(JSON_OBJECT_SIZE(2));

  doc["sensor"] = "relay";
  doc["status"] = relayStatus;

  String st;
  serializeJson(doc, st);
  server.send(200, "application/json", st);
}
// END REQUEST HANDLERS -------------------------------------------

// PIN SETUP ------------------------------------------------------
void setupPins() {  
  Serial.println("Setting up pins");
  pinMode(LEDPIN, OUTPUT);
  pinMode(RELAYPIN, OUTPUT);
  pinMode(SENSORPIN, INPUT_PULLUP);

  digitalWrite(LEDPIN, statusLed);
  digitalWrite(RELAYPIN, relayStatus);

  attachInterrupt(digitalPinToInterrupt(SENSORPIN), toggleRelay, CHANGE);
  Serial.println("Done setting up pins");
}
// END PIN SETUP --------------------------------------------------

void setup() {
  Serial.begin(115200);

  WiFi.config(staticIp, gateway, subnet);
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFISSID, WIFIPSK);

  Serial.println("Starting to connect");

  while (WiFi.status() != WL_CONNECTED) {
    statusLed = statusLed == LOW ? HIGH : LOW;
    digitalWrite(LEDPIN, statusLed);
    Serial.print(".");
    delay(500);
  }

  Serial.println("");
  Serial.print("Connected to ");
  Serial.println(WiFi.SSID());
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());

  setupPins();

  server.on("/toggle", handleRelayToggleRequest);
  server.on("/hard/restart", handeRestartRequest);
  server.on("/", handleStatusRequest);

  server.begin();
  Serial.println("HTTP server started");
}

void loop() {
  server.handleClient();
}
