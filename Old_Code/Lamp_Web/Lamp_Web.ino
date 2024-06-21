#include <WiFiManager.h>
#include <Adafruit_NeoPixel.h>

#define rstPin 12
#define LEDPin 13
#define touchPin 14
#define LEDCount 37

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
bool patternOn = false;
int touchState = HIGH;
int lastTouchState = HIGH;
unsigned long touchTime = 0;
unsigned long lastDebounceTime = 0;  // the last time the output pin was toggled
unsigned long lightOnTime = 0;
unsigned long debounceDelay = 50;    // the debounce time; increase if the output flickers

Adafruit_NeoPixel strip(LEDCount, LEDPin, NEO_GRB + NEO_KHZ800);

void setup() {
  WiFi.mode(WIFI_STA);
  Serial.begin(115200);
  strip.begin();
  strip.show();
  strip.setBrightness(brightness);
  pinMode(rstPin, INPUT_PULLUP);
  pinMode(touchPin, INPUT_PULLUP);
}

void loop() {
  int rstState = digitalRead(rstPin);
  unsigned long currentTime = millis();
  int touchState = digitalRead(touchPin);
  if ((millis()-lightOnTime)>3000 && patternOn){
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
        wm.resetSettings();
        wm.setConfigPortalTimeout(timeout);

        if (!wm.startConfigPortal("OnDemandAP")) {
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
    }
  } else if ((millis() - touchTime) < 500 && touchState == HIGH && (millis() - touchTime) > 0 && rstState == HIGH) {
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
    if (!hasShowPattern){
    hasShowPattern = true;
    patterns(pattern);
    strip.show();
    }
  }
}

void patterns(int op) {
  Serial.print("Run: ");
  Serial.println(op);
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
}
