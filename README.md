# MicroGPT-Terminal

MicroGPT-Terminal is a compact ChatGPT-style AI chat interface built on an ESP32 with a 128×32 OLED display and a physical button-based keyboard. It allows users to type prompts, send them to the Gemini API over HTTPS, and read scrollable AI responses on a tiny terminal-style screen.

## Features

- ESP32-based AI chat terminal  
- 128×32 OLED display (SSD1306)  
- On-screen keyboard with button navigation  
- Secure HTTPS communication with Gemini API  
- Automatic word wrapping and vertical scrolling  
- Minimal terminal-style user interface  

## Hardware Requirements

- ESP32 or ESP32-C3  
- 128×32 OLED display (I2C, SSD1306)  
- Five push buttons (Up, Down, Left, Right, Select)  
- Breadboard and jumper wires  

## Software Requirements

- Arduino IDE  
- ESP32 board support package  
- Adafruit_GFX  
- Adafruit_SSD1306  
- ArduinoJson  
- WiFi  
- WiFiClientSecure  

## API Setup

Create a file named `secrets.h`:

```cpp
#define WIFI_SSID "your_wifi_name"
#define WIFI_PASSWORD "your_wifi_password"
#define API_KEY "your_gemini_api_key"
