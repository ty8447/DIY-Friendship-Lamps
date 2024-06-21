#include <FS.h>  // this needs to be first, or it all crashes and burns...
#include <WiFi.h>
#include <WiFiManager.h>
#include <PubSubClient.h>
#include <Adafruit_NeoPixel.h>
#include <String.h>
#include <SPIFFS.h>

#include <ArduinoJson.h>  //https://github.com/bblanchon/ArduinoJson

#define rstPin 12
#define LEDPin 13
#define touchPin 14
#define LEDCount 37

const int maxRetries = 5;
const int retryDelay = 3000;  // milliseconds
int tries = 0;

int brightness = 100;
int varBright = 255;
int rstTime = 0;
bool canRst = true;
bool holdLight = false;
bool hasShowPattern = true;
bool canReply = false;
bool canSecret = false;
bool pressable = true;
bool canConfirm = false;
int LEDNum = 0;
const int maxCount = 6500;
unsigned long lastPressTime = 0;
bool isFirstPress = true;
bool isButtonPressed = false;  // Flag to track button state
int timeout = 120;             // seconds to run for
int pattern = 0;
int numPatterns = 6;
bool canChange = true;
bool inMessage = false;
bool canPub = false;
bool firstTouch = true;
bool patternOn = false;
bool brokerConnect = false;
int touchState = HIGH;
int lastTouchState = HIGH;
int touchAmounts = 0;
int secretLocationsTract[LEDCount];  //Tracking State of LEDs
unsigned long touchTime = 0;
unsigned long lastTouchTime = 0;
unsigned long lastDebounceTime = 0;  // the last time the output pin was toggled
unsigned long lightOnTime = 0;
unsigned long debounceDelay = 50;  // the debounce time; increase if the output flickers
unsigned long lastConnectCheck = 0;
int checkTime = 5 * 1000;  // 5 seconds
char mqtt_Server[40];
const char* filename = "/mqtt_Server";
const int mqttPort = 1883;
const char* clientId = "ESP32_1";  // Change for other ESP32
const char* publishTopic = "esp32/ESP32_2/data";
const char* subscribeTopic = "esp32/ESP32_1/data";
bool canAcceptMessage = true;
String apName = "Lamp ";
char* msgTypeOp[4] = { "Send", "Confirm", "Reply", "Secret" };
char* msgFrom;
char* msgType;
int msgValue;

Adafruit_NeoPixel strip(LEDCount, LEDPin, NEO_GRB + NEO_KHZ800);
WiFiClient espClient;
PubSubClient client(espClient);
WiFiManager wm;
WiFiManagerParameter custom_mqtt_Server("server", "MQTT Server", mqtt_Server, 40);

void setup() {
  apName += clientId;  // Uses String object's overloaded + operator
  WiFi.mode(WIFI_STA);
  Serial.begin(115200);
  delay(1000);
  Serial.println("_____________________________________");

  strip.begin();
  strip.show();
  strip.setBrightness(brightness);
  pinMode(rstPin, INPUT_PULLUP);
  pinMode(touchPin, INPUT_PULLUP);

  if (!SPIFFS.begin(true)) {
    Serial.println("An error occurred while mounting the SPIFFS.");
    return;
  } else {
    Serial.println("SPIFFS Started!");
  }

  if (!loadMqttServer()) {
    // If the file does not exist, create it and save an initial value
    Serial.println("Created Filesystem");
    saveMqttServer("192.168.1.1");  // Set your initial IP address here
    loadMqttServer();               // Reload the value to update the mqtt_Server variable
  } else {
    Serial.println("Should have loaded Filesystem");
  }

  wm.addParameter(&custom_mqtt_Server);
  strip.clear();
  startAnim();
}

void loop() {
  wm.process();
  if (lastConnectCheck + checkTime < millis()) {
    lastConnectCheck = millis();
    checkNetworkConnection();
  }
  client.loop();
  int rstState = digitalRead(rstPin);
  unsigned long currentTime = millis();
  if (touchRead(touchPin) < 40) {
    touchState = LOW;
  } else {
    touchState = HIGH;
  }
  if ((millis() - lightOnTime) > 3000 && patternOn) {
    strip.clear();
    strip.show();
    pattern = 0;
    patternOn = false;
  }
  // Button pressed
  if (rstState == LOW && touchState == HIGH) {
    // Debounce to avoid false detections
    if (!isButtonPressed) {
      isButtonPressed = true;

      // First press
      if (isFirstPress) {
        lastPressTime = currentTime;
        isFirstPress = false;
        strip.clear();
        strip.fill(strip.Color(255, 120, 0), 0, LEDCount);
        strip.show();
        patternOn = false;
      }
      // Second press within 2.5 seconds
      else if (currentTime - lastPressTime < 2500) {
        startWebPortal();
      }
    }
    if (rstTime < maxCount && canRst) {
      rstTime++;
    } else if (rstTime >= maxCount) {  //Reset Confirmed
      int i = 0;
      rstTime = 1;
      canRst = false;
      while (i < 255 * 2) {
        strip.fill(strip.Color(0, 255 - i / 2, 0), 0, LEDCount);
        strip.show();
        i++;
      }
    }
    if (rstTime > 500 && rstTime < maxCount && rstState == LOW) {
      LEDNum = map(rstTime, 500, maxCount, 0, LEDCount);
      strip.setPixelColor(LEDNum, strip.Color(0, 0, 255));
      strip.show();
      canSecret = false;
      if (LEDNum >= 5 && LEDNum <= 7) {
        canSecret = true;
      }
    }
  }
  // Button not pressed
  else {
    // Debounce to avoid false detections
    if (isButtonPressed) {
      isButtonPressed = false;
      strip.fill(strip.Color(0, 0, 0), 0, LEDCount);
      strip.show();
      // Reset for a single press
      if (currentTime - lastPressTime >= 2500) {
        Serial.println("3");
        isFirstPress = true;
      }
      rstTime = 0;
      canRst = true;
    }
  }
  if (touchState != lastTouchState) {
    touchTime = millis();
    if (touchState == HIGH) {
      delay(50);
      if (touchState == HIGH) {
        varBright = 255;
      }
    } else {
      if (inMessage) {
        if (!firstTouch && touchTime - lastTouchTime <= 600) {
          replySendAnim();
          firstTouch = true;
          String message = "<" + String(clientId) + "," + String(msgTypeOp[2]) + "," + String(msgValue) + ">\n";
          client.publish(publishTopic, message.c_str());
          canReply = false;
          inMessage = false;
        } else if (firstTouch) {
          canReply = true;
          firstTouch = false;
          lastTouchTime = touchTime;
        }
      } else {
        if (canConfirm) {
          canConfirm = false;
          confAnim();
        }
        if (canSecret && firstTouch) {
          lastTouchTime = touchTime;
          firstTouch = false;
          touchAmounts++;
        } else if (canSecret && !firstTouch && millis() - lastTouchTime <= 600) {
          touchAmounts++;
          lastTouchTime = millis();
        }
        if (touchAmounts >= 10) {
          String message = "<" + String(clientId) + "," + String(msgTypeOp[3]) + "," + String(pattern) + ">\n";
          client.publish(publishTopic, message.c_str());
          sendAnim();
          touchAmounts = 0;
          canSecret = false;
          firstTouch = true;
        }
      }
      canChange = true;
      hasShowPattern = false;
    }

    lastTouchState = touchState;
  } else if ((millis() - touchTime) > 500 && touchState == LOW && rstState == HIGH) {  //** If the Button is Held **
    if (varBright > 0) {
      varBright--;
      patterns(pattern);
      canChange = false;
    } else {
      if (canPub) {
        canPub = false;
        String message = "<" + String(clientId) + "," + String(msgTypeOp[0]) + "," + String(pattern) + ">\n";
        client.publish(publishTopic, message.c_str());
        sendAnim();
      }
    }
  } else if ((millis() - touchTime) < 500 && touchState == HIGH && (millis() - touchTime) > 0 && rstState == HIGH) {  //** If the Button is Pressed (On Release) **
    if (inMessage) {
      pressable = true;
    }
    if (canChange && !inMessage) {
      canChange = false;
      if (patternOn) {

        if (pattern >= numPatterns) {
          pattern = 0;
        } else {
          pattern++;
        }
      } else {
        patternOn = true;
      }
    } else if (!hasShowPattern && !inMessage && !canConfirm) {
      hasShowPattern = true;
      patterns(pattern);
    }
  } else if (touchState == HIGH && inMessage && canReply) {
    if (millis() - lastTouchTime > 600) {
      confAnim();
      inMessage = false;
      firstTouch = true;
      canReply = false;
    }
  }
}

void patterns(int op) {
  canPub = true;
  lightOnTime = millis();
  switch (op) {
    case 0:
      strip.fill(strip.Color(varBright, 0, 0), 0, LEDCount);
      break;
    case 1:
      strip.fill(strip.Color(0, varBright, 0), 0, LEDCount);
      break;
    case 2:
      strip.fill(strip.Color(0, 0, varBright), 0, LEDCount);
      break;
    case 3:
      strip.fill(strip.Color(varBright, varBright, 0), 0, LEDCount);
      break;
    case 4:
      strip.fill(strip.Color(varBright, 0, varBright), 0, LEDCount);
      break;
    case 5:
      strip.fill(strip.Color(0, varBright, varBright), 0, LEDCount);
      break;
    case 6:
      strip.fill(strip.Color(varBright, varBright, varBright), 0, LEDCount);
      break;
  }
  strip.show();
  if (!holdLight) {
    patternOn = true;
  }
}

bool isMqttServerEmpty(const char* server) {
  return server == nullptr || strlen(server) == 0;
}

void checkNetworkConnection() {
  // Check internet connectivity after successful connection
  if (WiFi.isConnected()) {
    if (isMqttServerEmpty(mqtt_Server)) {
      startWebPortal();
    } else {
      if (!brokerConnect) {
        brokerConnect = true;
        client.setServer(mqtt_Server, mqttPort);
        client.setCallback(callback);
        // Connect to the MQTT broker
        reconnect();
      } else if (!client.connected()) {
        reconnect();
      }
    }
  } else {
    Serial.println("No internet connection. Retrying...");
    strip.fill(strip.Color(255, 100, 0), 0, LEDCount);
    strip.show();
    Serial.print(".");
    delay(retryDelay);
    // Check internet again
    if (wm.autoConnect(apName.c_str(), "Lamp1229")) {
      strip.fill(strip.Color(0, 255, 100), 0, LEDCount);  //CONNECTED!!
      strip.show();
      delay(500);
      strip.clear();
      strip.show();
    } else {
      Serial.println("Failed to connect and hit timeout");
      strip.fill(strip.Color(255, 100, 0), 0, LEDCount);
      strip.show();
      delay(500);
      strip.clear();
      strip.show();
      delay(3000);
      ESP.restart();
      delay(5000);
    }
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
      tries++;
      if (tries < 5) {
        Serial.print("failed, rc=");
        Serial.print(client.state());
        Serial.println(" try again in 5 seconds");
        delay(5000);
        strip.fill(strip.Color(255, 0, 0), 0, LEDCount);
        strip.show();
        delay(200);
        strip.clear();
        strip.show();
      } else {
        startWebPortal();
      }
    }
  }
}

char message[128];     // Define an array to store the incoming message
int messageIndex = 0;  // Keeps track of the current index in the message array

void callback(char* topic, byte* payload, unsigned int length) {
  // Check if we can accept another message

  // Store the full incoming message
  char message[length + 1];
  for (int i = 0; i < length; i++) {
    message[i] = (char)payload[i];
  }
  message[length] = '\0';

  // Parse the message using a custom parsing function
  parseMessage(message);

  playAnim();

  // Reset the canAcceptMessage flag
  canAcceptMessage = true;
}

void parseMessage(char* message) {
  // Strip the leading '<' character from the message
  if (message[0] == '<') {
    message++;
  }

  // Split the message into tokens based on delimiters
  char* tokens[3];
  char* token = strtok_r(message, ",", &message);
  int i = 0;

  while (token != NULL) {
    // Remove leading and trailing spaces from the token
    token = strtok_r(token, " ", &token);

    tokens[i++] = token;
    token = strtok_r(message, ",", &message);
  }

  // Extract message components
  msgFrom = tokens[0];
  msgType = tokens[1];
  strncpy(msgFrom, tokens[0], sizeof(msgFrom));
  strncpy(msgType, tokens[1], sizeof(msgType));
  msgValue = atoi(tokens[2]);
}

void sendAnim() {
  int s = 0;
  while (s < 6) {
    if (s == 0) {
      for (int i = 0; i < LEDCount; i++) {
        strip.clear();
        strip.setPixelColor(i, strip.Color(0, 127, 255));
        strip.show();
        delay(10);
      }
    } else if (s == 1 || s == 3 || s == 5) {
      for (int i = 255; i > 0; i--) {
        strip.fill(strip.Color(0, i / 2, i), 0, LEDCount);
        strip.show();
      }
    } else if (s == 2 || s == 4 || s == 6) {
      for (int i = 0; i < 255; i++) {
        strip.fill(strip.Color(0, i / 2, i), 0, LEDCount);
        strip.show();
      }
    }
    s++;
  }
}

void recAnim() {
  int s = 0;
  while (s < 6) {
    if (s == 0) {
      for (int i = LEDCount; i > 0; i--) {
        strip.clear();
        strip.setPixelColor(i, strip.Color(0, 127, 255));
        strip.show();
        delay(20);
      }
    } else if (s == 1 || s == 3 || s == 5) {
      for (int i = 255; i > 0; i--) {
        strip.fill(strip.Color(0, i / 2, i), 0, LEDCount);
        strip.show();
      }
    } else if (s == 2 || s == 4 || s == 6) {
      for (int i = 0; i < 255; i++) {
        strip.fill(strip.Color(0, i / 2, i), 0, LEDCount);
        strip.show();
      }
    }
    s++;
  }
  patterns(int(msgValue));
}

void replySendAnim() {
  for (int i = 0; i < 10; i++) {
    for (int j = 0; j < LEDCount; j++) {
      strip.clear();
      strip.setPixelColor(j, strip.Color(0, 127, 255));
      strip.show();
      delay(20 - i * 2);
    }
  }
}

void startAnim() {
  int spacing = 4;
  for (int i = 0; i < LEDCount / spacing; i++) {
    strip.fill(strip.Color(255, 255, 255), i * spacing, i * spacing + spacing - 1);
    strip.show();
    delay(400);
  }
  strip.fill(strip.Color(0, 0, 0), 0, LEDCount);
  strip.show();
}

void replyRecAnim() {
  for (int i = 10; i > 0; i--) {
    for (int j = LEDCount; j > 0; j--) {
      strip.clear();
      strip.setPixelColor(j, strip.Color(0, 127, 255));
      strip.show();
      delay(20 - i * 2);
    }
  }
}

void confAnim() {
  for (int i = LEDCount; i >= 0; i--) {
    strip.setPixelColor(i, strip.Color(0, 0, 0));
    strip.show();
    delay(20);
  }
}

void errorAnim() {
  for (long fPH = 0; fPH < 5 * 65536; fPH += 256) {
    for (int i = 0; i < strip.numPixels(); i++) {
      int pixelHue = fPH + (i * 65536L / strip.numPixels());
      strip.setPixelColor(i, strip.gamma32(strip.ColorHSV(pixelHue)));
    }
    strip.show();
    delay(20);
  }
  strip.clear();
  strip.show();
}

void secretAnim() {
  int i = 0;
  int nonZero;
  while (i < 2000) {
    nonZero = 0;
    for (int j = 0; j < LEDCount; j++) {
      if (secretLocationsTract[j] != 0) {
        nonZero++;
      }
    }
    if (random(50) <= 5) {
      if (nonZero < 10) {
        int rand = random(LEDCount * 2);
        if (secretLocationsTract[rand / 2] == 0) {
          secretLocationsTract[rand / 2] = random(127, 255);
        } else {
          secretLocationsTract[rand / 2 + 1] = random(127, 255);
        }
      }
    }
    for (int v = 0; v < LEDCount; v++) {
      if (secretLocationsTract[v] > 0) {
        strip.setPixelColor(v, strip.Color(secretLocationsTract[v], secretLocationsTract[v], secretLocationsTract[v]));
        strip.show();
        secretLocationsTract[v] -= 2;
      } else if (secretLocationsTract[v] < 0) {
        secretLocationsTract[v] = 0;
      }
    }
    i++;
  }
  while (nonZero > 0) {
    nonZero = 0;
    for (int j = 0; j < LEDCount; j++) {
      if (secretLocationsTract[j] != 0) {
        nonZero++;
      }
    }
    for (int v = 0; v < LEDCount; v++) {
      if (secretLocationsTract[v] > 0) {
        strip.setPixelColor(v, strip.Color(secretLocationsTract[v], secretLocationsTract[v], secretLocationsTract[v]));
        strip.show();
        secretLocationsTract[v] -= 2;
      } else if (secretLocationsTract[v] < 0) {
        secretLocationsTract[v] = 0;
      }
    }
  }
  strip.clear();
  strip.show();
}

void playAnim() {
  holdLight = true;

  if (strcmp(msgType, msgTypeOp[0]) == 0) {
    inMessage = true;
    recAnim();
  } else if (strcmp(msgType, msgTypeOp[1]) == 0) {
    confAnim();
  } else if (strcmp(msgType, msgTypeOp[2]) == 0) {
    replyRecAnim();
    patterns(msgValue);
    canConfirm = true;
  } else if (strcmp(msgType, msgTypeOp[3]) == 0) {
    secretAnim();
  } else {
    errorAnim();
  }
  holdLight = false;
}

void saveMqttServer(const char* ipAddress) {
  File file = SPIFFS.open(filename, "w");
  if (!file) {
    Serial.println("Failed to open file for writing");
    return;
  }

  file.print("Saved mqtt_Server: ");
  file.println(ipAddress);

  file.close();
  Serial.println("IP address saved to file");
  strip.clear();
  strip.show();
}

bool loadMqttServer() {
  Serial.println("Loading from Filesystem");
  File file = SPIFFS.open(filename, "r");
  if (!file) {
    Serial.println("Failed to open file for reading");
    return false;
  }

  while (file.available()) {
    String line = file.readStringUntil('\n');
    if (line.startsWith("Saved mqtt_Server: ")) {
      line.remove(0, 19);
      line.trim();
      strncpy(mqtt_Server, line.c_str(), sizeof(mqtt_Server));
      Serial.print("Loaded IP address from file: ");
      Serial.println(mqtt_Server);
      file.close();
      return true;
    }
  }

  file.close();
  Serial.println("No valid IP address found in the file");
  return false;
}

void saveConfigCallback() {
  Serial.println("SAFING");
  tries = 0;
  strncpy(mqtt_Server, custom_mqtt_Server.getValue(), sizeof(mqtt_Server));
  Serial.print("mqtt_Server: ");
  Serial.println(mqtt_Server);
  saveMqttServer(mqtt_Server);
}

void startWebPortal() {
  strip.fill(strip.Color(0, 0, 255), 0, LEDCount);
  strip.show();
  wm.setSaveParamsCallback(saveConfigCallback);
  wm.setConfigPortalTimeout(timeout);
  if (!wm.startConfigPortal(apName.c_str(), "Lamp1229")) {
          Serial.println("Failed to connect and hit timeout");
          strip.fill(strip.Color(255, 100, 0), 0, LEDCount);
          strip.show();
          delay(500);
          strip.clear();
          strip.show();
          delay(3000);
          ESP.restart();
          delay(5000);
        }
        strip.fill(strip.Color(0, 255, 100), 0, LEDCount);  //CONNECTED!!
        strip.show();
        delay(500);
        strip.clear();
        strip.show();
        Serial.println("1");
        isFirstPress = true;
        saveConfigCallback();
}