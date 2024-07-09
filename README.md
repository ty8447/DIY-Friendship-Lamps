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

Ensure the ESP32 is wired correctly to the WS2812B LED strip and a wire is connected to the touch pin. Use the breadboard and jumper wires to make the connections.

### Code Setup

**File:** `client_code.ino`

#### Important Lines to Change:

```cpp
// Replace with your WiFi credentials
const char* ssid = "your_SSID";
const char* password = "your_PASSWORD";

// Replace with your MQTT Broker address
const char* mqtt_server = "broker_address";
```

### Uploading Code

Upload the modified `client_code.ino` file to your ESP32 devices.

---

## Server

### Introduction

This section explains how to set up the server-side ESP32, which will act as the MQTT broker and handle communications between the client lamps.

### Wiring

The server ESP32 will need minimal wiring. Connect the RGB LED to indicate the status of the server.

### Code Setup

**File:** `client_code.ino`

#### Important Lines to Change:

```cpp
// Replace with your WiFi credentials
const char* ssid = "your_SSID";
const char* password = "your_PASSWORD";

// Replace with your MQTT Broker address
const char* mqtt_server = "broker_address";
```
### Uploading Code

Upload the modified `client_code.ino` file to your ESP32 devices.

---

##Network Configuration

If you plan to use the lamps outside of your local network, you will need to port forward the internal IP address and port of the server ESP32. This will allow the clients to communicate with the server from anywhere.

1. Log in to your router’s admin page.
2. Find the port forwarding section.
3. Forward the port used by your server ESP32 (default: 1883 for MQTT) to its internal IP address.
4. Save the changes and restart your router if necessary.

---

##To-Do List

- Add authentication for clients

Feel free to contribute to this project by submitting issues and pull requests on GitHub. Enjoy building and using your DIY friendship lamps!
