#include <TinyMqtt.h>
#include <WiFiManager.h>
#include <WiFi.h>
#include <iostream>

// Pin definitions
const int RED_PIN = 5;
const int GREEN_PIN = 18;
const int BLUE_PIN = 19;

const uint16_t PORT = 1883;
MqttBroker broker(PORT);

const char* ssid = "Wireless";
const char* password = "7817293793";

WiFiManager wm;
int clientCount = 0;

void setup() {
  WiFi.mode(WIFI_STA);
  Serial.begin(115200);

  // Define static IP address, gateway, and subnet mask
  IPAddress ip(192, 168, 1, 70);
  IPAddress gateway(192, 168, 1, 1);
  IPAddress subnet(255, 255, 255, 0);

  // Set static IP
  WiFi.config(ip, gateway, subnet);

  // Initialize RGB LED pins
  pinMode(RED_PIN, OUTPUT);
  pinMode(GREEN_PIN, OUTPUT);
  pinMode(BLUE_PIN, OUTPUT);

  // Ensure LED is off initially
  setLEDColor(0, 0, 0);

  // Attempt to connect to WiFi
  WiFi.begin(ssid, password);

  int connectionAttempts = 0;

  while (WiFi.status() != WL_CONNECTED && connectionAttempts < 10) {
    Serial.print('.');
    blinkLED(255, 255, 0, 1, 50);
    delay(500);
    connectionAttempts++;
  }

  if (WiFi.status() != WL_CONNECTED) {
    updateWifi();
  } else {
    Serial.print("\033[32mConnected to ");
    Serial.print(ssid);
    Serial.print(" IP address: ");
    Serial.println(WiFi.localIP());
    delay(200);
    // Blink green twice and then turn off
    blinkLED(0, 255, 0, 2, 100);

    broker.begin();
    Serial.print("Broker ready: ");
    Serial.print(WiFi.localIP());
    Serial.print(" on port ");
    Serial.println(PORT);
  }
}

void loop() {
  broker.loop();

  if (WiFi.status() != WL_CONNECTED) {
    // Quickly blink orange if disconnected
    blinkLED(255, 125, 0, 1, 100);
  }

  int newClientCount = broker.clientsCount();
  if (newClientCount > clientCount) {
    // Blink purple twice when a new client connects
    blinkLED(255, 0, 255, 2, 500);    // Purple
  }
  clientCount = newClientCount;
}

void updateWifi() {
  Serial.println("Starting Wifi Portal");
  setLEDColor(0, 0, 255); // Blue for WiFi portal
  if (!wm.autoConnect("FriendLampServer", "Lamp1229")) {
    Serial.println("Failed to connect and hit timeout");
    // Slowly blink red for WiFi connection failure
    blinkLED(255, 0, 0, 1, 100);
    delay(3000);
    ESP.restart();
    delay(5000);
  }
  Serial.println("CONNECTED!");
  // Blink green twice and then turn off
  blinkLED(0, 255, 0, 2, 100);
}

void setLEDColor(int red, int green, int blue) {
  analogWrite(RED_PIN, 255 - red);
  analogWrite(GREEN_PIN, 255 - green);
  analogWrite(BLUE_PIN, 255 - blue);
}

void blinkLED(int red, int green, int blue, int times, int delayTime) {
  for (int i = 0; i < times; i++) {
    setLEDColor(red, green, blue);
    delay(delayTime);
    setLEDColor(0, 0, 0);
    delay(delayTime);
  }
}
