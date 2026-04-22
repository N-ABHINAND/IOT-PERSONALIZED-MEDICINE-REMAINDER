#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <RTClib.h>
#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <WiFiClientSecureBearSSL.h>

// ===== PINS =====
#define BUZZER_PIN D6
#define LED_PIN D7

LiquidCrystal_I2C lcd(0x27, 16, 2);
RTC_DS3231 rtc;

// ===== WiFi & Telegram =====
const char* ssid = " ABHI";
const char* password = "abhikuttu";
const char* telegramToken = "8321601032:AAE_1e0kGH-4OU2vTCXKy3r71l_4HwZRIEE";
const char* chatID = "5464248909";

// ===== Reminder times (24-HR) =====
const int REM_HOURS[3] = {9, 17, 17};  // e.g., 17:40, 17:42, 17:44
const int REM_MINS[3]  = {34, 42, 44};
bool sent[3] = {false, false, false};

// ===== SETUP =====
void setup() {
  Serial.begin(115200);
  Wire.begin(D2, D1);

  lcd.init();
  lcd.backlight();
  lcd.clear();
  lcd.print("Initializing...");

  if (!rtc.begin()) {
    lcd.clear();
    lcd.print("RTC not found!");
    while (1);
  }

  if (rtc.lostPower()) {
    rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));  // Sync RTC with upload time
  }

  pinMode(BUZZER_PIN, OUTPUT);
  pinMode(LED_PIN, OUTPUT);
  digitalWrite(BUZZER_PIN, LOW);
  digitalWrite(LED_PIN, LOW);

  WiFi.begin(ssid, password);
  lcd.clear();
  lcd.print("WiFi Connecting...");
  int tries = 0;
  while (WiFi.status() != WL_CONNECTED && tries < 30) {
    delay(500);
    Serial.print(".");
    tries++;
  }

  lcd.clear();
  if (WiFi.status() == WL_CONNECTED) {
    lcd.print("WiFi Connected");
    Serial.println("✅ WiFi connected!");
  } else {
    lcd.print("WiFi Failed!");
    Serial.println("❌ WiFi failed");
  }

  delay(2000);
  lcd.clear();
  sendTelegram("💊 Medicine Reminder System is active  take medicine ✅");
}

// ===== LOOP =====
void loop() {
  DateTime now = rtc.now();

  // Display current time
  lcd.setCursor(0, 0);
  lcd.print("Time ");
  print2(now.hour());
  lcd.print(":");
  print2(now.minute());
  lcd.print(":");
  print2(now.second());

  lcd.setCursor(0, 1);
  lcd.print("Date ");
  print2(now.day());
  lcd.print("/");
  print2(now.month());
  lcd.print(" ");

  // Check reminders
  for (int i = 0; i < 3; i++) {
    if (now.hour() == REM_HOURS[i] && now.minute() == REM_MINS[i] && !sent[i]) {
      triggerAlarm(i);
      sent[i] = true;
    }
  }

  // Reset next day
  if (now.hour() == 0 && now.minute() == 0) {
    for (int i = 0; i < 3; i++) sent[i] = false;
  }

  delay(1000);
}

// ===== ALARM =====
void triggerAlarm(int index) {
  String label;
  if (index == 0) label = "Morning";
  else if (index == 1) label = "Afternoon";
  else label = "Night";

  Serial.printf("🔔 Reminder %d triggered: %s\n", index + 1, label.c_str());

  lcd.clear();
  lcd.print("Take Medicine!");
  lcd.setCursor(0, 1);
  lcd.print(label);

  // Buzzer + LED
  for (int i = 0; i < 6; i++) {
    digitalWrite(LED_PIN, HIGH);
    digitalWrite(BUZZER_PIN, HIGH);
    delay(400);
    digitalWrite(LED_PIN, LOW);
    digitalWrite(BUZZER_PIN, LOW);
    delay(400);
  }

  // Telegram alert
  sendTelegram("💊 Reminder: Take your " + label + " medicine now!");
  delay(2000);
  lcd.clear();
}

// ===== TELEGRAM SENDER =====
void sendTelegram(String message) {
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("⚠️ WiFi reconnecting...");
    WiFi.begin(ssid, password);
    int retry = 0;
    while (WiFi.status() != WL_CONNECTED && retry < 10) {
      delay(500);
      Serial.print(".");
      retry++;
    }
    if (WiFi.status() != WL_CONNECTED) {
      Serial.println("❌ WiFi reconnect failed!");
      return;
    }
  }

  // New client for each send
  WiFiClientSecure client;
  client.setInsecure();
  HTTPClient https;

  String url = "https://api.telegram.org/bot" + String(telegramToken) +
               "/sendMessage?chat_id=" + String(chatID) +
               "&text=" + urlencode(message);

  Serial.println("🌐 Sending Telegram...");
  if (https.begin(client, url)) {
    int code = https.GET();
    if (code == 200) {
      Serial.println("✅ Telegram message sent!");
    } else {
      Serial.printf("❌ Telegram Error: %d\n", code);
    }
    https.end();
  } else {
    Serial.println("⚠️ HTTPS begin failed!");
  }
}

// ===== HELPERS =====
void print2(int num) {
  if (num < 10) lcd.print("0");
  lcd.print(num);
}

String urlencode(String str) {
  String enc = "";
  for (int i = 0; i < str.length(); i++) {
    char c = str.charAt(i);
    if (isalnum(c)) enc += c;
    else {
      enc += '%';
      char buf[3];
      sprintf(buf, "%02X", c);
      enc += buf;
    }
  }
  return enc;
}
