#include "time.h"
#include <WiFi.h>
#include <esp_wifi.h>
#include <rmt_led_strip.hpp>
#include "credentials.h"

// 32 LEDs, pin 15
htcw::ws2812 leds(15, 32);

// Vertical order for top-to-bottom animation
// Pairs LEDs at the same height, starting from top
const int verticalOrder[32] = {
  0, 31, 1, 30, 2, 29, 3, 28, 4, 27, 5, 26, 6, 25, 7, 24,
  8, 23, 9, 22, 10, 21, 11, 20, 12, 19, 13, 18, 14, 17, 15, 16
};

// Vertical rank: 0 = top, 15 = bottom
// LEDs on opposite sides of the circle at the same height have the same rank
const int verticalRank[32] = {
  0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15,  // LEDs 0-15
  15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0   // LEDs 16-31
};

// Get vertical position (0.0 = top, 1.0 = bottom)
float getVerticalPosition(int ledIndex) {
  return verticalRank[ledIndex] / 15.0f;
}

const char *ntpServer = "pool.ntp.org";
const long gmtOffset_sec = 0;
const int daylightOffset_sec = 3600;

// Resync NTP every hour (in milliseconds)
const unsigned long NTP_RESYNC_INTERVAL = 3600000;
unsigned long lastNtpSync = 0;

// Task handle for LED control
TaskHandle_t ledTaskHandle = NULL;

// Shared time variable (updated by main core, read by LED core)
volatile time_t currentTime = 0;
portMUX_TYPE timeMux = portMUX_INITIALIZER_UNLOCKED;

void syncNtpTime() {
  Serial.println("Syncing time with NTP server...");
  configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);

  struct tm timeinfo;
  int retries = 0;
  while (!getLocalTime(&timeinfo) && retries < 10) {
    Serial.println("Waiting for NTP sync...");
    delay(500);
    retries++;
  }

  if (retries < 10) {
    Serial.println(&timeinfo, "Time synced: %A, %B %d %Y %H:%M:%S");
    lastNtpSync = millis();
  } else {
    Serial.println("NTP sync failed, will retry later");
  }
}

// HSV to RGB conversion for rainbow effect
void hsvToRgb(uint8_t h, uint8_t s, uint8_t v, uint8_t *r, uint8_t *g, uint8_t *b) {
  uint8_t region = h / 43;
  uint8_t remainder = (h - (region * 43)) * 6;

  uint8_t p = (v * (255 - s)) >> 8;
  uint8_t q = (v * (255 - ((s * remainder) >> 8))) >> 8;
  uint8_t t = (v * (255 - ((s * (255 - remainder)) >> 8))) >> 8;

  switch (region) {
    case 0:  *r = v; *g = t; *b = p; break;
    case 1:  *r = q; *g = v; *b = p; break;
    case 2:  *r = p; *g = v; *b = t; break;
    case 3:  *r = p; *g = q; *b = v; break;
    case 4:  *r = t; *g = p; *b = v; break;
    default: *r = v; *g = p; *b = q; break;
  }
}

// Rainbow animation - shown immediately on boot
// Colors form a gradient from top to bottom
void rainbowAnimation() {
  // Random starting offset so rainbow doesn't always start with red on top
  int startOffset = esp_random() % 256;

  // Smooth color flow (5 seconds = 250 cycles at 20ms)
  for (int cycle = 0; cycle < 250; cycle++) {
    for (int i = 0; i < 32; i++) {
      int hue = (int)(getVerticalPosition(i) * 170) + startOffset - cycle * 4;
      while (hue < 0) hue += 256;
      uint8_t r, g, b;
      hsvToRgb((uint8_t)(hue % 256), 255, 13, &r, &g, &b);
      // Color correction: reduce blue and green to warm up the rainbow
      g = (g * 85) / 100;  // 85% green
      b = (b * 40) / 100;  // 40% blue
      leds.color(i, r, g, b);
    }
    leds.update();
    delay(20);
  }
}

// Green mask - shown while waiting for time sync
void showGreenMask() {
  for (int i = 0; i < 32; i++) {
    leds.color(i, 0, 7, 0);  // Green
    leds.update();
    delay(25);
  }
}

// Reveal animation - shown after time sync completes
void revealTime(time_t targetTime) {
  delay(300);

  // Reveal the time from MSB to LSB
  for (int revealIndex = 31; revealIndex >= 0; revealIndex--) {
    uint32_t checkMask = 0b10000000000000000000000000000000;
    for (int ledIndex = 31; ledIndex >= revealIndex; ledIndex--) {
      if (checkMask & targetTime) {
        leds.color(ledIndex, 20, 0, 0);  // Red for 1
      } else {
        leds.color(ledIndex, 0, 0, 0);   // Off for 0
      }
      checkMask >>= 1;
    }
    leds.update();
    delay(30);
  }
}

// LED control task - runs on Core 1 to avoid WiFi interrupt jitter
void ledTask(void *parameter) {
  Serial.println("LED task started on core 1");

  // Main LED loop
  for (;;) {
    time_t displayTime;
    portENTER_CRITICAL(&timeMux);
    displayTime = currentTime;
    portEXIT_CRITICAL(&timeMux);

    // Update LEDs based on current time
    int ledIndex = 31;
    uint32_t bitmask = 0b10000000000000000000000000000000;
    for (; bitmask > 0; bitmask >>= 1) {
      if (bitmask & displayTime) {
        leds.color(ledIndex, 20, 0, 0);  // Red for 1
      } else {
        leds.color(ledIndex, 0, 0, 0);   // Off for 0
      }
      ledIndex--;
    }
    leds.update();

    delay(100);  // Update 10 times per second for smooth display
  }
}

void setup() {
  Serial.begin(115200);
  Serial.println("Unix Time Clock v2 - Starting...");

  leds.initialize();

  // Start WiFi connection early (connects in background during animation)
  Serial.printf("Connecting to %s ", WIFI_SSID);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

  // Show rainbow animation while WiFi connects
  rainbowAnimation();

  // Wait for WiFi if not connected yet (should be quick since it started earlier)
  while (WiFi.status() != WL_CONNECTED) {
    delay(100);
    Serial.print(".");
  }
  Serial.println(" CONNECTED");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());

  // Disable WiFi power saving for more reliable connectivity
  esp_wifi_set_ps(WIFI_PS_NONE);

  // Show green mask while waiting for time sync
  showGreenMask();

  // NTP sync (green mask stays visible)
  syncNtpTime();

  // Reveal the time
  time_t now;
  time(&now);
  time_t targetTime = now + 1;  // Account for reveal animation duration
  revealTime(targetTime);

  // Update shared time variable
  time(&now);
  portENTER_CRITICAL(&timeMux);
  currentTime = now;
  portEXIT_CRITICAL(&timeMux);

  // Create LED task pinned to Core 1
  // Core 0 handles WiFi, Core 1 handles LEDs for jitter-free updates
  xTaskCreatePinnedToCore(
    ledTask,        // Task function
    "LEDTask",      // Task name
    4096,           // Stack size
    NULL,           // Parameters
    24,             // Priority (high)
    &ledTaskHandle, // Task handle
    1               // Core 1
  );

  Serial.println("Setup complete - LED task running on Core 1");
}

void loop() {
  // Update shared time variable
  time_t now;
  time(&now);
  portENTER_CRITICAL(&timeMux);
  currentTime = now;
  portEXIT_CRITICAL(&timeMux);

  // Periodic NTP resync
  if (millis() - lastNtpSync > NTP_RESYNC_INTERVAL) {
    Serial.println("Periodic NTP resync...");
    syncNtpTime();
  }

  // Check WiFi connection and reconnect if needed
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("WiFi disconnected, reconnecting...");
    WiFi.reconnect();
    int retries = 0;
    while (WiFi.status() != WL_CONNECTED && retries < 20) {
      delay(500);
      retries++;
    }
    if (WiFi.status() == WL_CONNECTED) {
      Serial.println("WiFi reconnected");
      esp_wifi_set_ps(WIFI_PS_NONE);
    }
  }

  delay(1000);
}
