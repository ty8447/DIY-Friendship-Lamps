#include <WiFi.h>
#include <PubSubClient.h>

const char* ssid = "Danny";
const char* password = "Dev1to!!";
const char* mqttUser = "User";
const char* mqttPassword = "Password";
const char* mqttServer = "75.130.245.232";
const int mqttPort = 1883;
const char* clientId = "ESP32_2"; // Change for other ESP32
const char* publishTopic = "esp32/client2/data";
const char* subscribeTopic = "esp32/client1/data";
#define swPin 14
bool canSend = true;


WiFiClient espClient;
PubSubClient client(espClient);

void setup() {
  Serial.begin(115200);
  WiFi.begin(ssid, password);
  pinMode(swPin, INPUT_PULLUP);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    while (!Serial);
  }
    Serial.println("Connecting to WiFi...");
    WiFi.begin(ssid, password);
    while (WiFi.status() != WL_CONNECTED) {
      delay(500);
      Serial.print(".");
    }
    Serial.println("");
    Serial.print("Connected to WiFi with IP: ");
    Serial.println(WiFi.localIP());

    client.setServer(mqttServer, mqttPort);
    client.setCallback(callback);

    // Connect to the MQTT broker
    reconnect();
}

void loop() {
  if (!client.connected()) {
    reconnect();
  }
  client.loop();

  if (digitalRead(swPin) == LOW && canSend) {
    canSend = false;
    String message = "<" + + String(clientId);
    client.publish(publishTopic, message.c_str());
    Serial.print("Published message: ");
    Serial.println(message);
  } else if (digitalRead(swPin) == HIGH) {
    canSend = true;

  }
}

void reconnect() {
  // Loop until reconnected
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    if (client.connect(clientId)) {
      Serial.println("connected");
      client.subscribe(subscribeTopic);
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      delay(5000);
    }
  }
}

void callback(char* topic, byte* payload, unsigned int length) {
  Serial.print("Message received on topic: ");
  Serial.println(topic);

  for (int i = 0; i < length; i++) {
    Serial.print((char)payload[i]);
  }
  Serial.println();
}
