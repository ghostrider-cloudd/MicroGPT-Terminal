#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <ArduinoJson.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include "secrets.h"

// ================= OLED =================
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 32
#define OLED_ADDR 0x3C
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

// ================= BUTTONS =================
#define BTN_UP     2
#define BTN_DOWN   3
#define BTN_LEFT   4
#define BTN_RIGHT  5
#define BTN_SELECT 10

// ================= MODES =================
enum Mode { TYPING, READING };
Mode mode = TYPING;

// ================= KEYBOARD =================
#define ROWS 3
#define COLS 9

const char* keyboard[ROWS][COLS] = {
  {"A","B","C","D","E","F","G","H","I"},
  {"J","K","L","M","N","O","P","Q","R"},
  {"S","T","U","V","W","X","Y","Z","SD"}
};

int curRow = 0;
int curCol = 0;
String inputText = "";

// ================= RESPONSE =================
String lines[40];
int lineCount = 0;
int scrollIndex = 0;

// ================= UTILS =================
bool pressed(int pin) {
  return digitalRead(pin) == LOW;
}

// ================= WORD WRAP =================
void wrapText(String text) {
  Serial.println("[WRAP] Wrapping response text");
  lineCount = 0;
  scrollIndex = 0;

  const int maxChars = 21;
  String line = "";

  for (int i = 0; i < text.length(); i++) {
    char c = text[i];
    if (c == '\n' || line.length() >= maxChars) {
      lines[lineCount++] = line;
      line = "";
      if (lineCount >= 40) break;
    }
    if (c != '\n') line += c;
  }

  if (line.length() && lineCount < 40) {
    lines[lineCount++] = line;
  }

  Serial.print("[WRAP] Total wrapped lines: ");
  Serial.println(lineCount);
}

// ================= UI =================
void drawTypingUI() {
  Serial.println("[UI] Drawing TYPING screen");
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);

  display.setCursor(0,0);
  display.print("MSG: ");
  display.print(inputText);
  display.print("_");

  for (int r = 0; r < ROWS; r++) {
    int y = 8 + r * 8;
    for (int c = 0; c < COLS; c++) {
      int x = c * 14;
      if (r == curRow && c == curCol) {
        display.fillRect(x, y, 14, 8, SSD1306_WHITE);
        display.setTextColor(SSD1306_BLACK);
      } else {
        display.setTextColor(SSD1306_WHITE);
      }
      display.setCursor(x + 1, y);
      display.print(keyboard[r][c]);
    }
  }
  display.display();
}

void drawReadingUI() {
  Serial.print("[UI] Drawing READING screen, scrollIndex=");
  Serial.println(scrollIndex);

  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);

  for (int i = 0; i < 4; i++) {
    int idx = scrollIndex + i;
    if (idx < lineCount) {
      display.setCursor(0, i * 8);
      display.println(lines[idx]);
    }
  }
  display.display();
}

// ================= GEMINI =================
String sendToGemini(String prompt) {
  WiFiClientSecure client;
  client.setInsecure();

Serial.println("[GEMINI] Using gemini-1.5-flash-001");


  if (!client.connect("generativelanguage.googleapis.com", 443)) {
    Serial.println("[GEMINI] ❌ HTTPS connection failed");
    return "Network error";
  }

  String body =
    "{"
      "\"contents\": ["
        "{"
          "\"parts\": ["
            "{ \"text\": \"Answer in ONE short sentence: " + prompt + "\" }"
          "]"
        "}"
      "]"
    "}";

  Serial.println("[GEMINI] Sending request...");

  client.println(
    "POST /v1beta/models/gemini-1.5-flash:generateContent?key=" +
    String(API_KEY) + " HTTP/1.1"
  );
  client.println("Host: generativelanguage.googleapis.com");
  client.println("Content-Type: application/json");
  client.print("Content-Length: ");
  client.println(body.length());
  client.println();
  client.println(body);

  while (client.connected() && !client.available()) delay(10);

  String resp = client.readString();
  Serial.println("[GEMINI] Raw response:");
  Serial.println(resp);

  int jsonStart = resp.indexOf("{");
  if (jsonStart < 0) return "Bad response";

  StaticJsonDocument<2048> doc;
  if (deserializeJson(doc, resp.substring(jsonStart))) {
    return "JSON error";
  }

  if (doc["error"]) {
    return "Quota hit";
  }

  if (!doc["candidates"] || doc["candidates"].size() == 0) {
    return "No response";
  }

  return doc["candidates"][0]["content"]["parts"][0]["text"].as<String>();
}



// ================= SEND =================
void sendMessage() {
  Serial.println("[SEND] Sending message to Gemini");
  display.clearDisplay();
  display.setCursor(0,0);
  display.println("Thinking...");
  display.display();

  String reply = sendToGemini(inputText);
  wrapText(reply);

  mode = READING;
  Serial.println("[MODE] Switched to READING");
}

// ================= SETUP =================
void setup() {
  Serial.begin(115200);
  delay(1000);
  Serial.println("=== ESP32-C3 Gemini Keyboard Boot ===");

  pinMode(BTN_UP, INPUT_PULLUP);
  pinMode(BTN_DOWN, INPUT_PULLUP);
  pinMode(BTN_LEFT, INPUT_PULLUP);
  pinMode(BTN_RIGHT, INPUT_PULLUP);
  pinMode(BTN_SELECT, INPUT_PULLUP);

  Serial.println("[INIT] Initializing I2C & OLED");
  Wire.begin(6,7);
  display.begin(SSD1306_SWITCHCAPVCC, OLED_ADDR);

  Serial.print("[WIFI] Connecting to ");
  Serial.println(WIFI_SSID);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    delay(300);
  }
  Serial.println("\n[WIFI] Connected");
  Serial.print("[WIFI] IP: ");
  Serial.println(WiFi.localIP());

  drawTypingUI();
  Serial.println("[MODE] TYPING ready");
}

// ================= LOOP =================
void loop() {

  if (mode == TYPING) {

    if (pressed(BTN_UP)) {
      Serial.println("[BTN] UP");
      curRow = (curRow + ROWS - 1) % ROWS;
      drawTypingUI();
      delay(150);
    }

    if (pressed(BTN_DOWN)) {
      Serial.println("[BTN] DOWN");
      curRow = (curRow + 1) % ROWS;
      drawTypingUI();
      delay(150);
    }

    if (pressed(BTN_LEFT)) {
      Serial.println("[BTN] LEFT");
      curCol = (curCol + COLS - 1) % COLS;
      drawTypingUI();
      delay(150);
    }

    if (pressed(BTN_RIGHT)) {
      Serial.println("[BTN] RIGHT");
      curCol = (curCol + 1) % COLS;
      drawTypingUI();
      delay(150);
    }

    if (pressed(BTN_SELECT)) {
      String key = keyboard[curRow][curCol];
      Serial.print("[BTN] SELECT -> ");
      Serial.println(key);

      if (key == "SD") {
        sendMessage();
      } else {
        inputText += key;
        Serial.print("[TEXT] Current input: ");
        Serial.println(inputText);
        drawTypingUI();
      }
      delay(200);
    }
  }

  else if (mode == READING) {

    if (pressed(BTN_UP) && scrollIndex > 0) {
      Serial.println("[SCROLL] UP");
      scrollIndex--;
      drawReadingUI();
      delay(120);
    }

    if (pressed(BTN_DOWN) && scrollIndex < lineCount - 4) {
      Serial.println("[SCROLL] DOWN");
      scrollIndex++;
      drawReadingUI();
      delay(120);
    }

    if (pressed(BTN_SELECT)) {
      Serial.println("[MODE] Back to TYPING");
      inputText = "";
      curRow = 0;
      curCol = 0;
      mode = TYPING;
      drawTypingUI();
      delay(200);
    }
  }
}
