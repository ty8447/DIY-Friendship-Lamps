#include <WiFi.h>
#include <WiFiManager.h>
#include <PubSubClient.h>
#include <Adafruit_NeoPixel.h>
#include <String.h>

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
bool hasShowPattern = true;
int LEDNum = 0;
const int maxCount = 6500;
unsigned long lastPressTime = 0;
bool isFirstPress = true;
bool isButtonPressed = false;  // Flag to track button state
int timeout = 120;             // seconds to run for
int pattern = 0;
int numPatterns = 6;
bool canChange = true;
bool canPub = false;
bool patternOn = false;
bool brokerConnect = false;
int touchState = HIGH;
int lastTouchState = HIGH;
unsigned long touchTime = 0;
unsigned long lastDebounceTime = 0;  // the last time the output pin was toggled
unsigned long lightOnTime = 0;
unsigned long debounceDelay = 50;  // the debounce time; increase if the output flickers
unsigned long lastConnectCheck = 0;
int checkTime = 5 * 1000;  // 5 seconds

const char* mqttUser = "User";
const char* mqttPassword = "Password";
const char* mqttServer = "75.130.245.232";
const int mqttPort = 1883;
const char* clientId = "ESP32_2";  // Change for other ESP32
const char* publishTopic = "esp32/client2/data";
const char* subscribeTopic = "esp32/client1/data";
bool canAcceptMessage = true;
String apName = "Lamp ";
char* msgTypeOp[3] = { "Send", "Confirm", "Reply" };
char* msgFrom;
char* msgType;
int msgValue;

Adafruit_NeoPixel strip(LEDCount, LEDPin, NEO_GRB + NEO_KHZ800);
WiFiClient espClient;
PubSubClient client(espClient);

void setup() {
  apName += clientId;  // Uses String object's overloaded + operator
  WiFi.mode(WIFI_STA);
  Serial.begin(115200);
  strip.begin();
  strip.show();
  strip.setBrightness(brightness);
  pinMode(rstPin, INPUT_PULLUP);
  pinMode(touchPin, INPUT_PULLUP);
}

void loop() {
  if (lastConnectCheck < millis() + checkTime) {
    lastConnectCheck = millis();
    checkNetworkConnection();
  }
  client.loop();
  int rstState = digitalRead(rstPin);
  unsigned long currentTime = millis();
  int touchState = digitalRead(touchPin);
  if ((millis() - lightOnTime) > 3000 && patternOn) {
    strip.clear();
    strip.show();
    patternOn = false;
    Serial.println("LIGHT OFF");
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
        strip.show();
        patternOn = false;
      }
      // Second press within 3 seconds
      else if (currentTime - lastPressTime < 3000) {
        // Perform specific actions here (e.g., enter AP mode)
        WiFiManager wm;
        //wm.resetSettings();
        wm.setConfigPortalTimeout(timeout);

        if (!wm.startConfigPortal(apName.c_str())) {
          Serial.println("Failed to connect and hit timeout");
          delay(3000);
          ESP.restart();
          delay(5000);
        }

        Serial.println("Connected...yeey :)");
        isFirstPress = true;
      }
    }
    if (rstTime < maxCount && canRst) {
      rstTime++;
    } else if (rstTime >= maxCount) {
      int i = 0;
      rstTime = 1;
      canRst = false;
      while (i < 255 * 2) {
        strip.fill(strip.Color(0, 255 - i / 2, 0), 0, LEDCount);
        strip.show();
        i++;
      }
    }
    if (rstTime > 500 && rstTime < maxCount) {
      LEDNum = map(rstTime, 500, maxCount, 0, LEDCount);
      isFirstPress = true;
      strip.setPixelColor(LEDNum, strip.Color(0, 0, 255));
      strip.show();
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
      if (currentTime - lastPressTime >= 3000) {
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
        Serial.println("NOT PRESSED");
      }
    } else {
      canChange = true;
      hasShowPattern = false;
      Serial.println("PRESSED");
    }
    lastTouchState = touchState;
  } else if ((millis() - touchTime) > 500 && touchState == LOW && rstState == HIGH) {
    Serial.println("HELD");
    if (varBright > 0) {
      varBright--;
      patterns(pattern);
      strip.show();
      canChange = false;
    } else {
      if (canPub) {
        canPub = false;
        String message = "<" + String(clientId) + "," + String(msgTypeOp[0]) + "," + String(pattern) + ">\n";
        Serial.print("Published message: ");
        Serial.println(message.c_str()); 
        client.publish(publishTopic, message.c_str());
        sendAnim();
      }
  }
}
else if ((millis() - touchTime) < 500 && touchState == HIGH && (millis() - touchTime) > 0 && rstState == HIGH) {
  if (canChange) {
    Serial.println("SHORT");
    canChange = false;
    Serial.print("Previous Pattern: ");
    Serial.println(pattern);
    if (patternOn) {

      if (pattern >= numPatterns) {
        pattern = 0;
      } else {
        pattern++;
      }
    } else {
      patternOn = true;
    }
    Serial.print("Pattern: ");
    Serial.println(pattern);
  }
  if (!hasShowPattern) {
    hasShowPattern = true;
    patterns(pattern);
    strip.show();
  }
}
}

void patterns(int op) {
  Serial.print("Run: ");
  Serial.println(op);
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
  patternOn = true;
}

void checkNetworkConnection() {
  // Check internet connectivity after successful connection
  //Serial.print("Checking internet connection...");
  if (WiFi.isConnected()) {
    //Serial.println("Connected to Internet!");
    //Serial.println(WiFi.localIP());
    if (!brokerConnect) {
      brokerConnect = true;
      client.setServer(mqttServer, mqttPort);
      client.setCallback(callback);
      // Connect to the MQTT broker
      reconnect();
    } else if (!client.connected()) {
      reconnect();
    }
  } else {
    Serial.println("No internet connection. Retrying...");

    // Retry connection for a limited time
    for (int i = 0; i < maxRetries; i++) {
      Serial.print(".");
      delay(retryDelay);

      // Check internet again
      if (WiFi.isConnected()) {
        break;
      }
      tries++;
    }

    // If still no connection, start AP
    if (tries == maxRetries) {
      tries = 0;
      Serial.println("\nTimeout reached. Starting AP...");
      WiFiManager wm;
      //wm.resetSettings();
      wm.setConfigPortalTimeout(timeout);

      if (!wm.startConfigPortal(apName.c_str())) {
        Serial.println("Failed to connect and hit timeout");
        delay(3000);
        ESP.restart();
        delay(5000);
      }

      Serial.println("Connected...yeey :)");
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
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      delay(5000);
    }
  }
}

char message[128];     // Define an array to store the incoming message
int messageIndex = 0;  // Keeps track of the current index in the message array

void callback(char* topic, byte* payload, unsigned int length) {
  // Check if we can accept another message
  if (!canAcceptMessage) {
    Serial.println("Busy, please try again later.");
    return;
  }

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
  Serial.println("DONE!");
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

  if (i != 3) {
    // Invalid message format
    Serial.println("Invalid message format");
    return;
  }

  // Extract message components
  msgFrom = tokens[0];
  msgType = tokens[1];
  strncpy(msgFrom, tokens[0], sizeof(msgFrom));
  strncpy(msgType, tokens[1], sizeof(msgType));
  msgValue = atoi(tokens[2]);

  // Process parsed values
  Serial.print("Message from: ");
  Serial.println(msgFrom);
  Serial.print("Message type: ");
  Serial.println(msgType);
  Serial.print("Value: ");
  Serial.println(msgValue);
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
      for (int i = 255; i > 0 ; i--) {
        strip.fill(strip.Color(0, i / 2, i), 0, LEDCount);
        strip.show();
      }
    } else if (s == 2 || s == 4 || s == 6) {
      for (int i = 0; i < 255 ; i++) {
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
      for (int i = 255; i > 0 ; i--) {
        strip.fill(strip.Color(0, i / 2, i), 0, LEDCount);
        strip.show();
      }
    } else if (s == 2 || s == 4 || s == 6) {
      for (int i = 0; i < 255 ; i++) {
        strip.fill(strip.Color(0, i / 2, i), 0, LEDCount);
        strip.show();
      }
    }
    s++;
  }
}

void replyAnim() {
}

void confAnim() {
}

void errorAnim() {
}

void playAnim() {
  Serial.print("Message Type: ");
  Serial.println(msgType);
  Serial.print("Playing Animation: ");

  if (strcmp(msgType, msgTypeOp[0]) == 0) {
    recAnim();
    Serial.println("Send");
  } else if (strcmp(msgType, msgTypeOp[1]) == 0) {
    confAnim();
    Serial.println("Confirm");
  } else if (strcmp(msgType, msgTypeOp[2]) == 0) {
    replyAnim();
    Serial.println("Reply");
  } else {
    errorAnim();
    Serial.println("Error");
  }
}