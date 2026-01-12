# Unix Time Clock v2 (C++ / PlatformIO)

A 32-LED binary clock displaying Unix timestamp, built for ESP32.

## Features

- **32 WS2812 LEDs** arranged in a circle displaying Unix time in binary
- **Persistent WiFi** connection with automatic reconnection
- **Hourly NTP sync** to keep time accurate
- **Core pinning** - LED updates run on Core 1 to avoid WiFi interrupt jitter
- **Startup animation** - Rainbow effect while connecting, green mask with time reveal

## Hardware

- ESP32 (tested on LOLIN D32 Pro)
- 32x WS2812 LEDs on GPIO 15
- LED arrangement: circular, LED 0 just right of top (12 o'clock), going clockwise

## Setup

1. Copy credentials template:
   ```bash
   cp src/credentials.h.example src/credentials.h
   ```

2. Edit `src/credentials.h` with your WiFi credentials:
   ```cpp
   const char *WIFI_SSID = "your_ssid";
   const char *WIFI_PASSWORD = "your_password";
   ```

3. Build and upload:
   ```bash
   pio run -e lolin_d32_pro -t upload
   ```

## Supported Boards

- `lolin_d32_pro` - WEMOS LOLIN D32 Pro
- `esp32cam` - ESP32-CAM
- `esp32-c3-supermini` - ESP32-C3 Super Mini
- `lolin_s3_mini` - LOLIN S3 Mini

## Startup Sequence

1. Rainbow animation (5 seconds) - WiFi connects in background
2. Green mask builds up
3. NTP time sync
4. Time reveal animation
5. Normal operation - displays Unix timestamp in binary

## LED Mapping

- LED 31 (MSB) - just left of 12 o'clock
- LED 0 (LSB) - just right of 12 o'clock
- Red LED = bit is 1
- LED off = bit is 0

## Improvements over v1

- WiFi stays connected for periodic time resync
- LED task pinned to Core 1 (no timing jitter from WiFi)
- Credentials stored in separate gitignored file
- Warm color-corrected rainbow animation
