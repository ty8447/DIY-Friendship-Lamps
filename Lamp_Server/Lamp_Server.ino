#include <TinyMqtt.h>
#include <WiFiManager.h>
#include <WiFi.h>
#include <iostream>


const uint16_t PORT = 1883;
//const uint8_t RETAIN = 10;

MqttBroker broker(PORT);

const char* ssid = "Cracosudan";
const char* password = "7817293793";

WiFiManager wm;

void setup() {
  WiFi.mode(WIFI_STA);
  Serial.begin(115200);
  // Define static IP address, gateway, and subnet mask
  IPAddress ip(192, 168, 1, 70);
  IPAddress gateway(192, 168, 1, 1);
  IPAddress subnet(255, 255, 255, 0);

  // Set static IP
  WiFi.config(ip, gateway, subnet);

  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    Serial << '.';
    delay(500);
  }

  //Serial << "\033[32mConnected to " << ssid << "IP address: " << WiFi.localIP() << endl;

  broker.begin();
  //Serial << "Broker ready : " << WiFi.localIP() << " on port " << PORT << endl;
}

void loop() {
  broker.loop();
}

void updateWifi(){
  Serial.println("Starting Wifi Portal");
    if (!wm.autoConnect("FriendLampServer", "Lamp1229")) {
          Serial.println("Failed to connect and hit timeout");
          delay(3000);
          ESP.restart();
          delay(5000);
        }
        Serial.println("CONNECTED!");
}
