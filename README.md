# DIY Friendship Lamps

Create a pair of DIY friendship lamps that can communicate with each other using an ESP32 microcontroller, MQTT protocol, and LED strips. This project allows you to light up your friend's lamp from anywhere in the world.

## Parts List

- 3x ESP32 microcontrollers
- 1x RGB LED
- 1x Breadboard
- Jumper wires
- 2x WS2812B flexible LED strips (any length)

---

## Client

### Introduction

This section details the setup and configuration for the client-side ESP32 devices, which control the LED strips.

### Wiring

Ensure the ESP32 is wired correctly to the WS2812B LED strip and RGB LED for status indications. Use the breadboard and jumper wires to make the connections.

### Code Setup

**File:** `client_code.ino`

#### Important Lines to Change:

```cpp
// Replace with your WiFi credentials
const char* ssid = "your_SSID";
const char* password = "your_PASSWORD";

// Replace with your MQTT Broker address
const char* mqtt_server = "broker_address";
