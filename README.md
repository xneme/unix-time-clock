# Binary Unix time clock

Worried about the impending Y2K38 problem? Want to see how much breathing room we still have at a single glance? This is the clock for you.
Inspired by numerical Unix time clocks, but slightly less (or more) useful.

## Hardware
This project uses an ESP32 board and a 32 pixel long WS2812 addressable led strip or ring.
Tested on:
 - [Lolin D32 Pro](https://www.aliexpress.com/item/32883116057.html)
 - [Lolin S2 Mini](https://www.aliexpress.com/item/1005003145192016.html)
 - [ESP32-C3 Super Mini](https://www.aliexpress.com/item/1005006334515931.html)
 - [32 Bit WS2812 ring](https://www.aliexpress.com/item/1005006358767684.html)

 Board with dual core cpu is preferred, making it possible to run LED control and WiFi on separate cores so that WiFi interrupts do not mess up the timings of WS2812.
 Rust version of the software uses two cores and uses built in SNTP features of ESP32 to keep internal clock accurate.
 PlatformIO (C / Arduino) version uses a single core and drops the WiFi connection after synchronizing with NTP server and relies purely on the internal clock accuracy. Internal clock might drift significantly so the board needs to be reset every now and then to keep the clock somewhat accurate.

## Installation
Rust version uses ESP-RS std approach. [Instructions to set up dev environment](https://docs.esp-rs.org/book/installation/index.html).
After installation you can compile, upload and monitor with command `cargo run`.

C version uses [PlatformIO](https://platformio.org/).
After installation you can compile, upload and monitor with `pio run -e esp-c3-supermini -t upload -t monitor`.

You need to modify `src/main.rs` or `src/main.cpp` to add your own WiFi credentials and check the specified pin for controlling the LEDs matches the pin you are planning to use.

## Level shifting
ESP32 runs on 3.3V, while according to specs WS2812 requires signal of 0.7*VCC, so 3,5V for 5V LEDs. This works fine most of the time, but in case you have bad luck, you need to level shift the control line to 5V.
More info about this at [adafruit website](https://learn.adafruit.com/neopixel-levelshifter/shifting-levels).

## TODO:
- Enclosure design

