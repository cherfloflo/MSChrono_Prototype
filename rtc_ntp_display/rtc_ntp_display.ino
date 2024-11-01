#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <RTClib.h>
#include <ESP8266WiFi.h>
#include <time.h>
#include <ESP8266WebServer.h>
#include <ESP8266mDNS.h>
#include <ESP8266HTTPUpdateServer.h>

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET -1
#define OLED_I2C_ADDRESS 0x3C

const char *ssid = "Cherwifi";
const char *password = "ChErViN200681";

const char* ntpServer = "fr.pool.ntp.org";
const long gmtOffset_sec = 3600;
const int daylightOffset_sec = 0;

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);
RTC_DS3231 rtc;

ESP8266WebServer httpServer(80);
ESP8266HTTPUpdateServer httpUpdater;
const char* host = "esp8266-webupdate";

unsigned long lastSyncTime = 0;
const unsigned long syncInterval = 10 * 60 * 1000;

void setup() {
  display.begin(SSD1306_SWITCHCAPVCC, OLED_I2C_ADDRESS);
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);

  displayYellowMessage("Initialisation...");
  delay(1000);

  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  connectWiFi();

  if (!rtc.begin()) {
    displayYellowMessage("Erreur RTC");
    while (1);
  }

  setupOTA();
  syncRTCWithNTP();
}

void loop() {
  httpServer.handleClient();
  MDNS.update();

  if (millis() - lastSyncTime >= syncInterval) {
    syncRTCWithNTP();
    lastSyncTime = millis();
  }

  DateTime now = rtc.now();
  display.clearDisplay();

  int32_t rssi = WiFi.RSSI();
  int barWidth = map(rssi, -100, -30, 0, SCREEN_WIDTH);
  display.fillRect(0, 0, barWidth, 10, SSD1306_WHITE);

  display.setCursor(0, 20);
  display.printf("Date: %02d/%02d/%04d", now.day(), now.month(), now.year());
  display.setCursor(0, 40);
  display.printf("Heure: %02d:%02d:%02d", now.hour(), now.minute(), now.second());

  display.display();
  delay(1000);
}

void connectWiFi() {
  int retryCount = 0;
  while (WiFi.status() != WL_CONNECTED && retryCount < 10) {
    delay(1000);
    displayYellowMessage("Connexion Wi-Fi...");
    display.setCursor(0, 8);
    display.printf("Essai %d", retryCount + 1);
    display.display();
    retryCount++;
  }

  if (WiFi.status() == WL_CONNECTED) {
    displayYellowMessage("Wi-Fi connecte!");
  } else {
    displayYellowMessage("Echec Wi-Fi");
  }
}

void setupOTA() {
  if (MDNS.begin(host)) {
    displayYellowMessage("MDNS configure");
  } else {
    displayYellowMessage("Erreur MDNS");
  }
  httpUpdater.setup(&httpServer);
  httpServer.begin();
  displayYellowMessage("Serveur OTA pret");
}

void syncRTCWithNTP() {
  if (WiFi.status() != WL_CONNECTED) {
    connectWiFi();
  }

  displayYellowMessage("Sync NTP en cours...");

  // Configurer le fuseau horaire de Paris avec changement automatique été/hiver
  setenv("TZ", "CET-1CEST,M3.5.0,M10.5.0/3", 1);  // Fuseau horaire de Paris
  tzset();
  configTime(0, 0, ntpServer);

  struct tm timeinfo;
  if (getLocalTime(&timeinfo)) {
    rtc.adjust(DateTime(timeinfo.tm_year + 1900, timeinfo.tm_mon + 1, timeinfo.tm_mday,
                        timeinfo.tm_hour, timeinfo.tm_min, timeinfo.tm_sec));
    displayYellowMessage("RTC mis a jour");
    delay(1000);
  } else {
    displayYellowMessage("Erreur Sync NTP");
  }
}

void displayYellowMessage(const char* message) {
  display.clearDisplay();
  display.setCursor(0, 0);
  display.println(message);
  display.display();
  delay(3000);
}
