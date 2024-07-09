# DIY Friendship Lamps

Create a pair of DIY friendship lamps that can communicate with each other using an ESP32 microcontroller, MQTT protocol, and LED strips. This project allows you to light up your friend's lamp from anywhere in the world.

## Parts List

- 3x ESP32 microcontrollers
- 1x RGB LED
- 1x Breadboard
- Jumper wires
- 2x WS2812B flexible LED strips (any length)
- 2x buttons
- 2x lamp housings with some metal section (for interacting with the lamp)

---

## Client

### Introduction

This section details the setup and configuration for the client-side ESP32 devices, which control the LED strips.

### Wiring

Ensure the ESP32 is wired correctly to the WS2812B LED strip and a wire is connected to the touch pin. Use the breadboard and jumper wires to make the connections.

Wiring Setup:
- Connect a wire from the metal surface to D14 on the ESP32
- Connect a button to D12 on the ESP32 for reset
- Connect the DIN pin from the WS2812B to D13 on the ESP32, the +5V pin from the led strip to the 3V3 on the ESP32, and GND to GND

### Code Setup

**File:** `Lamp_Device_V3.ino`

#### Important Lines to Change:

```cpp
// Replace with the number of leds on your WS2812B strip
#define LEDCount 37

// Modify if necessary
const int mqttPort = 1883; // This is default for MQTT so leave it unless necessary
const char* clientId = "ESP32_1";  // Change for other ESP32
const char* publishTopic = "esp32/ESP32_2/data"; // Change from ESP32_2 to ESP32_1 if the clientId is ESP32_2
const char* subscribeTopic = "esp32/ESP32_1/data"; // Change from ESP32_1 to ESP32_2 if the clientId is ESP32_2
```

### Uploading Code

Upload the modified `Lamp_Device_V3.ino` file to your ESP32 devices.

### Things to note

When needing to access the Wifi Configuration for the server, ensure that the esp32 is in AP mode (shown by a Blue LED) and then go to the Wifi settings on a device and connect to "Lamp ESP32_x" with the default password Lamp1229. From here, open a browser and go to 192.168.1.4 and configure the wifi network. NOTE: When modifying the MQTT server ip address, the wifi credentials need to be re-added in order to save the configuration.

---

## Local Hosted ESP32 Server (*Recommended*)

### Introduction

This section explains how to set up the server-side ESP32, which will act as the MQTT broker and handle communications between the client lamps.

### Wiring

The server ESP32 will need minimal wiring. Connect the RGB LED to indicate the status of the server.

RGB LED Wiring Setup:
  LED --> ESP32
- Red --> D5
- Green --> D18
- Blue --> D19
- VSS --> 3V3

### Code Setup

**File:** `Lamp_Server.ino`

#### Important Lines to Change:

```cpp
// Replace with your WiFi credentials
const char* ssid = "Replace with your SSID";
const char* password = "Replace with your Password";

// Replace with your router settings
  IPAddress ip(192, 168, 1, 70); // Sets a static IP for the server
  IPAddress gateway(192, 168, 1, 1); // Sets the IP address of the router that gives out DHCP addresses
  IPAddress subnet(255, 255, 255, 0); // Sets the subnet of the network
```
### Uploading Code

Upload the modified `Lamp_Server.ino` file to your ESP32 devices.

### Things to note

When needing to access the Wifi Configuration for the server, ensure that the esp32 is in AP mode (shown by a Blue LED) and then go to the Wifi settings on a device and connect to FriendLampServer with the default password Lamp1229. From here, open a browser and go to 192.168.1.4 and configure the wifi network. 

### Network Configuration

If you plan to use the lamps outside of your local network, you will need to port forward the internal IP address and port of the server ESP32. This will allow the clients to communicate with the server from anywhere.

1. Log in to your router’s admin page.
2. Find the port forwarding section.
3. Forward the port used by your server ESP32 (default: 1883 for MQTT) to its internal IP address.
4. Save the changes and restart your router if necessary.

---

## Free to use open MQTT Server (*Not Recommended*)

### Introduction
If you would prefer to not worry about setting up an MQTT server and are fine with the messages being open to the public (if a user subscribes to the channels), this is a much easier solution to getting the lamps to work.

### What needs to change
Inside the WifiManager for each lamp client, instead of setting the MQTT Server to the ip address of your local server, you can use broker.mqtt-dashboard.com and save that. This however has drawbacks as it would allow anyone to publish to the channels you are listening on, so it is recommended that you replace the following lines with more personalized ones:
```cpp
// Replace these to more personalized ones so that there is less of a chance of interference.
// *Note: Make sure that whatever you set the publishTopic to on one lamp you have the subscripeTopic on the other lamp the exact same and vice versa.*
const char* publishTopic = "esp32/ESP32_2/data";
const char* subscribeTopic = "esp32/ESP32_1/data";

```
---

## To-Do List

- Add authentication for clients
- Increase Reliability between the lamps and the ESP32 Server

Feel free to contribute to this project by submitting issues and pull requests on GitHub. Enjoy building and using your DIY friendship lamps!
