#include "time.h"
#include <WiFi.h>
#include <rmt_led_strip.hpp>

// 1 LED, pin 15
arduino::ws2812 leds(0, 16);

const char *ssid = "SSID"; // Your wifi ssid
const char *password = "PASSWORD"; // Your wifi password

const char *ntpServer = "pool.ntp.org";
const long gmtOffset_sec = 0;
const int daylightOffset_sec = 3600;
time_t now;
struct tm timeinfo;

void printLocalTime() {
  struct tm timeinfo;
  if (!getLocalTime(&timeinfo)) {
    Serial.println("Failed to obtain time");
    return;
  }
  Serial.println(&timeinfo, "%A, %B %d %Y %H:%M:%S");
}

void full_green() {
  for (int i = 0; i < 32; i++) {
    leds.color(i, 0, 10, 0);
  }
  leds.update();
}

void full_blue() {
  for (int i = 0; i < 32; i++) {
    leds.color(i, 0, 0, 10);
  }
  leds.update();
}

void revealTime(time_t time) {
  full_green();
  delay(1000);

  time = time + 2; // Reveal takes 2 seconds

  for (int reveal_index = 32; reveal_index >= 0; reveal_index--) {
    uint32_t bitmask = 0b10000000000000000000000000000000;
    for (int led_index = 31; led_index >= reveal_index; led_index--) {
      if (bitmask & time) {
        leds.color(led_index, 30, 0, 0);
      } else {
        leds.color(led_index, 0, 0, 0);
      }
      bitmask >>= 1;
    }
    leds.update();
    delay(30);
  }
  delay(10); // Wait for to make reveal take exactly 2 seconds
}

void setup() {

  Serial.begin(115200);

  leds.initialize();
  full_blue();
  // connect to WiFi
  Serial.printf("Connecting to %s ", ssid);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println(" CONNECTED");

  // init and get the time
  configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);
  getLocalTime(&timeinfo);

  // disconnect WiFi as it's no longer needed
  WiFi.disconnect(true);
  WiFi.mode(WIFI_OFF);
  time(&now);
  revealTime(now);
}

void loop() {
  time(&now);
  Serial.println(now, BIN);
  int led_index = 31;
  uint32_t bitmask = 0b10000000000000000000000000000000;
  for (; bitmask > 0; bitmask >>= 1) {
    if (bitmask & now) {
      leds.color(led_index, 30, 0, 0);
    } else {
      leds.color(led_index, 0, 0, 0);
    }
    led_index--;
  }
  leds.update();
  delay(1000);
}
