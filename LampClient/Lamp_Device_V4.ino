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
int numPatterns = 9;
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
char mqtt_Username[40];
char mqtt_Password[40];
const char* filename = "/mqtt_Server";
const int mqttPort = 1883;
const char* clientId = "ESP32_2";                   // Change for other ESP32
const char* publishTopic = "esp32/ESP32_2/data";    //This should match the clientId
const char* subscribeTopic = "esp32/ESP32_1/data";  //This should be the clientId of the other Lamp
bool canAcceptMessage = true;
String apName = "Lamp ";
char* msgTypeOp[4] = { "Send", "Confirm", "Reply", "Secret" };
char* msgFrom;
char* msgType;
int msgValue;
int cCode[4] = { 0, 0, 0, 0 };  // Color Passcode Holder
bool canUpdateCCode = true;
bool cCodeMode = false;
int storedCColorNum = 0;
int cCodeCount = 0;  //Color Passcode Counter
int heartsCode = 1129;

Adafruit_NeoPixel strip(LEDCount, LEDPin, NEO_GRB + NEO_KHZ800);
WiFiClient espClient;
PubSubClient client(espClient);
WiFiManager wm;
WiFiManagerParameter custom_mqtt_Server("server", "MQTT Server", mqtt_Server, 40);
WiFiManagerParameter custom_mqtt_Username("username", "Username", mqtt_Username, 40);
WiFiManagerParameter custom_mqtt_Password("password", "Password", mqtt_Password, 40);

void setup() {
  apName += clientId;  // Uses String object's overloaded + operator
  WiFi.mode(WIFI_STA);
  wm.setCountry("US");  // Or use "JP", "CN", etc. for wider range support
  Serial.begin(115200);
  delay(1000);
  Serial.println("_____________________________________");
  //wm.resetSettings();
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

  if (!loadMqttConfig()) {
    // If the file does not exist, create it and save an initial value
    Serial.println("Created Filesystem");
    File file = SPIFFS.open(filename, "w");
    if (!file) {
      Serial.println("Failed to open file for writing");
      return;
    }

    file.println(String("mqtt_Server: ") + "192.168.1.109");
    file.println(String("mqtt_Username: ") + "");
    file.println(String("mqtt_Password: ") + "");

    file.close();
    Serial.println("MQTT config saved to file");
    loadMqttConfig();  // Reload the value to update the mqtt_Server variable
  } else {
    Serial.println("Should have loaded Filesystem");
  }

  wm.addParameter(&custom_mqtt_Server);
  wm.addParameter(&custom_mqtt_Username);
  wm.addParameter(&custom_mqtt_Password);
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

  if (cCodeCount > 0 && cCodeMode) {
    checkColorCode();
  }

  int rstState = digitalRead(rstPin);
  unsigned long currentTime = millis();
  if (touchRead(touchPin) < 40) {
    if (lastTouchState != LOW) {
      touchState = LOW;
    }
  } else {
    if (lastTouchState != HIGH) {
      touchState = HIGH;
      Serial.println("High!");
    }
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
      //Serial.println(rstState);
      //Serial.println("Callout1");
    } else if (rstTime >= maxCount) {  //Reset Confirmed
      int i = 0;
      rstTime = 1;
      //Serial.println("Callout2");
      canRst = false;
      while (i < 255 * 2) {
        strip.fill(strip.Color(0, 255 - i / 2, 0), 0, LEDCount);
        strip.show();
        i++;
      }
      wm.resetSettings();
    }
    if (rstTime > 500 && rstTime < maxCount && rstState == LOW) {
      LEDNum = map(rstTime, 500, maxCount, 0, LEDCount);
      strip.setPixelColor(LEDNum, strip.Color(0, 0, 255));
      strip.show();
      //Serial.println("Callout3");
      canSecret = false;
      if (LEDNum < 2) {
        //Serial.println("Callout4");
        if (cCodeMode) {
          updateCCode();
        }
      } else if (LEDNum >= 2 && LEDNum <= 5) {
        if (!cCodeMode) {
          cCodeMode = true;
          Serial.println("Password Mode Enabled");
        }
      } else if (LEDNum >= 5 && LEDNum <= 7) {
        canSecret = true;
        if (cCodeMode) {
          cCodeMode = false;
          Serial.println("Password Mode Disabled(5-7)");
        }
      } else {
        canSecret = false;
        if (cCodeMode) {
          cCodeMode = false;
          Serial.println(LEDNum);
          Serial.println("Password Mode Disabled(>5)");
        }
      }
    } else if (rstTime < 500 && rstState == LOW) {
      if (cCodeMode) {
        updateCCode();
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
        Serial.println("Reset Button Debounce");
        canUpdateCCode = true;
        isFirstPress = true;
      }
      rstTime = 0;
      Serial.print("rstTime Reset: ");
      Serial.println(rstTime);
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
          String message = "<" + String(clientId) + "," + String(msgTypeOp[3]) + "," + 0 + ">\n";
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
      if (cCodeMode == true) {
        storedCColorNum = pattern;
        Serial.print("Pattern Number: ");
        Serial.println(pattern);
      }
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
      strip.fill(strip.Color(varBright, 0, 0), 0, LEDCount);  //Red
      break;
    case 1:
      strip.fill(strip.Color(varBright, varBright / 5, 0), 0, LEDCount);  //Amber
      break;
    case 2:
      strip.fill(strip.Color(varBright, varBright, 0), 0, LEDCount);  //Yellow
      break;
    case 3:
      strip.fill(strip.Color(varBright / 6, varBright, varBright / 6), 0, LEDCount);  //Lime
      break;
    case 4:
      strip.fill(strip.Color(0, varBright, 0), 0, LEDCount);  //Green
      break;
    case 5:
      strip.fill(strip.Color(0, varBright, varBright), 0, LEDCount);  //Cyan
      break;
    case 6:
      strip.fill(strip.Color(0, 0, varBright), 0, LEDCount);  //Blue
      break;
    case 7:
      strip.fill(strip.Color(varBright, 0, varBright), 0, LEDCount);  //Purple
      break;
    case 8:
      strip.fill(strip.Color(varBright, varBright / 3.64, varBright / 1.7), 0, LEDCount);  //Pink
      break;
    case 9:
      strip.fill(strip.Color(varBright, varBright, varBright), 0, LEDCount);  //White
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

void updateCCode() {
  if (canUpdateCCode) {
    canUpdateCCode = false;
    Serial.println("Updating Color Code");
    if (cCodeCount < 4) {
      Serial.print(storedCColorNum);
      cCode[cCodeCount] = storedCColorNum;
      Serial.print(" ~~ Locked in Color ");
      Serial.println(cCode[cCodeCount]);
      cCodeCount++;
      patterns(6);
    } else {
      checkColorCode();
      cCodeCount = 0;
      memset(cCode, 0, sizeof(cCode));
      Serial.println("Reset Code");
      patterns(2);
    }
  }
}

void checkColorCode() {
  if (cCodeCount >= 4) {
    String cCodeFull = "";
    for (int i = 0; i < 4; i++) {
      cCodeFull += String(cCode[i]);
    }
    Serial.print("Entered Passcode: " + cCodeFull + "...");
    if (cCodeFull == "1229") {
      Serial.println("It's a match!");
      patterns(4);
      delay(50);
      String message = "<" + String(clientId) + "," + msgTypeOp[3] + "," + 1 + ">\n";
      client.publish(publishTopic, message.c_str());
      sendAnim();
    } else {
      Serial.println("No match.");
      patterns(0);
    }
    cCodeFull = "";
    cCodeMode = false;
    cCodeCount = 0;
  }
}

void checkNetworkConnection() {
  // Check internet connectivity after successful connection
  if (WiFi.isConnected()) {
    Serial.print("WiFi connected. ");
    if (isMqttServerEmpty(mqtt_Server)) {
      Serial.println("MQTT Server not set. Starting web portal...");
      startWebPortal();
    } else {
      Serial.print("C_MQTT Server: ");
      Serial.println(mqtt_Server);
      //Serial.print("C_Username: ");
      //Serial.println(mqtt_Username);
      //Serial.print("C_Password: ");
      //Serial.println(mqtt_Password);
      checkTime = 60 * 1000;
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
    checkTime = 5 * 1000;
    strip.show();
    Serial.print(".");
    delay(retryDelay);
    // Check internet again
    if (wm.autoConnect(apName.c_str(), "Lamp1229")) {
      strip.fill(strip.Color(0, 255, 100), 0, LEDCount);  //CONNECTED!!
      //saveConfigCallback();
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
  if (rstTime == 0) {
    while (!client.connected()) {
      Serial.print("Attempting MQTT connection to ");
      Serial.print(mqtt_Server);
      Serial.print("...");
      Serial.print("~~");
      Serial.print(clientId);
      Serial.print(":=:");
      Serial.print(mqtt_Username);
      Serial.print(":=:");
      Serial.print(mqtt_Password);
      Serial.print("~~");
      if (client.connect(clientId, mqtt_Username, mqtt_Password)) {
        Serial.println("connected");
        client.subscribe(subscribeTopic);
      } else {
        tries++;
        if (tries < 5) {
          Serial.print("failed, rc=");
          Serial.print(client.state());
          Serial.println(" try again in 5 seconds");
          delay(5000);
          //strip.fill(strip.Color(255, 0, 0), 0, LEDCount);
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
}

char message[128];     // Define an array to store the incoming message
int messageIndex = 0;  // Keeps track of the current index in the message array

void callback(char* topic, byte* payload, unsigned int length) {
  // Check if we can accept another message

  // Store the full incoming message
  //char message[length + 1];
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
  if (int(msgValue) == 0) {
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
  } else {
    int beats = 0;
    int rampBright = 0;
    int pumps = 0;
    const long inRamp = 50;
    const long outRamp = 20;
    while (beats < 5) {
      while (pumps < 2) {
        while (rampBright < 255) {
          strip.fill(strip.Color(rampBright, 0, 0), 0, LEDCount);
          strip.show();
          rampBright++;
          delay(inRamp / 255);
        }
        while (rampBright > 0) {
          strip.fill(strip.Color(rampBright, 0, 0), 0, LEDCount);
          strip.show();
          rampBright--;
          delay(outRamp / 255);
        }
        pumps++;
      }
      delay(200);
      pumps = 0;
      beats++;
    }
  }
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

bool loadMqttConfig() {
  Serial.println("Loading MQTT config from file...");

  File file = SPIFFS.open(filename, "r");
  if (!file) {
    Serial.println("Failed to open file for reading");
    return false;
  }

  bool serverLoaded = false;
  bool usernameLoaded = false;
  bool passwordLoaded = false;

  while (file.available()) {
    String line = file.readStringUntil('\n');
    Serial.print("FILE DATA:");
    Serial.print(line);
    Serial.println("++");
    line.trim();

    if (line.startsWith("mqtt_Server: ")) {
      line.remove(0, String("mqtt_Server: ").length());
      strncpy(mqtt_Server, line.c_str(), sizeof(mqtt_Server));
      Serial.print("Loaded mqtt_Server: ");
      Serial.println(mqtt_Server);
      serverLoaded = true;
    } else if (line.startsWith("mqtt_Username: ")) {
      line.remove(0, String("mqtt_Username: ").length());
      strncpy(mqtt_Username, line.c_str(), sizeof(mqtt_Username));
      Serial.print("Loaded mqtt_Username: ");
      Serial.println(mqtt_Username);
      usernameLoaded = true;
    } else if (line.startsWith("mqtt_Password: ")) {
      line.remove(0, String("mqtt_Password: ").length());
      strncpy(mqtt_Password, line.c_str(), sizeof(mqtt_Password));
      Serial.print("Loaded mqtt_Password: ");
      Serial.println(mqtt_Password);
      passwordLoaded = true;
    }
  }

  file.close();

  if (serverLoaded && usernameLoaded && passwordLoaded) {
    Serial.println("All MQTT config values loaded successfully.");
    return true;
  } else {
    Serial.println("Failed to load complete MQTT config.");
    return false;
  }
}


void saveConfigCallback() {
  Serial.println("Saving MQTT config...");
  tries = 0;

  strncpy(mqtt_Server, custom_mqtt_Server.getValue(), sizeof(mqtt_Server));
  strncpy(mqtt_Username, custom_mqtt_Username.getValue(), sizeof(mqtt_Username));
  strncpy(mqtt_Password, custom_mqtt_Password.getValue(), sizeof(mqtt_Password));

  Serial.print("mqtt_Server: ");
  Serial.println(mqtt_Server);
  Serial.print("mqtt_Username: ");
  Serial.println(mqtt_Username);
  Serial.print("mqtt_Password: ");
  Serial.println(mqtt_Password);

  File file = SPIFFS.open(filename, "w");
  if (!file) {
    Serial.println("Failed to open file for writing");
    return;
  }

  file.println(String("mqtt_Server: ") + mqtt_Server);
  file.println(String("mqtt_Username: ") + mqtt_Username);
  file.println(String("mqtt_Password: ") + mqtt_Password);

  file.close();
  Serial.println("MQTT config saved to file");

  strip.clear();
  strip.show();
  reconnect();
}

void startWebPortal() {
  strip.fill(strip.Color(0, 0, 255), 0, LEDCount);
  strip.show();

  wm.setAPCallback([](WiFiManager* myWiFiManager) {
    Serial.println("AP Mode started.");
    Serial.print("AP SSID: ");
    Serial.println(myWiFiManager->getConfigPortalSSID());
  });

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
  } else {
    Serial.println("WiFi connected... attempting to save custom parameters");
    //saveConfigCallback(); // Manually call your save callback after successful WiFi connection
    strip.fill(strip.Color(0, 255, 100), 0, LEDCount);  //CONNECTED!!
    strip.show();
    delay(500);
    strip.clear();
    strip.show();
    Serial.println("Web portal finished.");
    isFirstPress = true;
  }
}
