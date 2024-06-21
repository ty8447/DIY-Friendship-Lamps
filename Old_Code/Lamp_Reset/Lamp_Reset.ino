#include <Adafruit_NeoPixel.h>

#define rstPin 12
#define LEDPin 13
#define LEDCount 37

int brightness = 100;
int rstTime = 0;
bool canRst = true;
int LEDNum = 0;
const int maxCount = 6500;

Adafruit_NeoPixel strip(LEDCount, LEDPin, NEO_GRB + NEO_KHZ800);

void setup() {
  strip.begin();           // INITIALIZE NeoPixel strip object (REQUIRED)
  strip.fill(strip.Color(255,0,0),0,LEDCount);
  strip.show();            // Turn OFF all pixels ASAP
  strip.setBrightness(brightness); // Set BRIGHTNESS to about 1/5 (max = 255)

  Serial.begin(9600);
  pinMode(rstPin, INPUT_PULLUP);

}

void loop() {
  int rstState = digitalRead(rstPin);
  if (rstState == 0 && rstTime < maxCount && canRst){
    rstTime++;
  } else if (rstState != 0 && rstTime > 0){
    rstTime=0;
    strip.fill(strip.Color(255,0,0),0,LEDCount);
    strip.show();
    canRst=true;
  } else if (rstTime >= maxCount){
      int i = 0;
      rstTime = 1;
      canRst=false;
      while(i<255*2){
      strip.fill(strip.Color(0,255-i/2,0),0,LEDCount);
      strip.show();
      i++;
      }
    }
  if (rstTime > 1 && rstTime < maxCount){
    LEDNum = map(rstTime, 0, maxCount, 0, LEDCount);
    strip.setPixelColor(LEDNum,strip.Color(0,0,255));
    strip.show();
  }
}
