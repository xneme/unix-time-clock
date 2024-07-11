use chrono::{DateTime, Utc};
use esp_idf_hal::cpu::Core;
use esp_idf_hal::delay::FreeRtos;
use esp_idf_hal::peripherals::Peripherals;
use esp_idf_hal::task::thread::ThreadSpawnConfiguration;
use esp_idf_svc::sntp::{EspSntp, SyncStatus};
use smart_leds::SmartLedsWrite;
use std::time::SystemTime;
use ws2812_esp32_rmt_driver::driver::color::LedPixelColorGrb24;
use ws2812_esp32_rmt_driver::{LedPixelEsp32Rmt, RGB8};

const SSID: &'static str = "SSID"; // Your SSID
const PASS: &'static str = "PASSWORD"; // Your Wifi Password

mod wifi;

fn main() -> ! {
    // It is necessary to call this function once. Otherwise some patches to the runtime
    // implemented by esp-idf-sys might not link properly. See https://github.com/esp-rs/esp-idf-template/issues/71
    esp_idf_sys::link_patches();

    let peripherals = Peripherals::take().unwrap();
    let modem = peripherals.modem;
    let led_pin = peripherals.pins.gpio15;
    let channel = peripherals.rmt.channel0;

    let _w = wifi::configure(SSID, PASS, modem).expect("Could not configure wifi");

    let ntp = EspSntp::new_default().unwrap();

    // Synchronize NTP
    println!("Synchronizing with NTP server");
    while ntp.get_sync_status() != SyncStatus::Completed {}
    println!("Time sync completed");

    println!("Starting LEDs");

    // Led control task has to be pinned to core 1 to avoid timing issues caused by wifi
    // interrupts on core 0
    ThreadSpawnConfiguration {
        pin_to_core: Some(Core::Core1),
        priority: 24,
        ..Default::default()
    }
    .set()
    .expect("Could not set thread spawn config");

    std::thread::spawn(move || {
        let mut led_strip =
            LedPixelEsp32Rmt::<RGB8, LedPixelColorGrb24>::new(channel, led_pin).unwrap();
        let pixels = std::iter::repeat(RGB8::from((0, 0, 0))).take(32);
        led_strip.write(pixels).unwrap();

        // Obtain System Time, convert to UTC Time, and get Unix Timestamp
        let st_now = SystemTime::now();
        let dt_now_utc: DateTime<Utc> = st_now.clone().into();
        let unix_timestamp = dt_now_utc.timestamp() + 2;

        let unix_timestamp_bits = format!("{unix_timestamp:032b}");

        let pixels = bitstring_to_pixels(&unix_timestamp_bits);
        startup_animation(&mut led_strip, pixels);
        loop {
            let st_now = SystemTime::now();
            let dt_now_utc: DateTime<Utc> = st_now.clone().into();
            let unix_timestamp = dt_now_utc.timestamp();

            let unix_timestamp_bits = format!("{unix_timestamp:032b}");

            let pixels = bitstring_to_pixels(&unix_timestamp_bits);
            led_strip.write(pixels).unwrap();

            // Print Time
            println!("{} = {}", unix_timestamp_bits, unix_timestamp);
            // Delay
            FreeRtos::delay_ms(1000);
        }
    });

    loop {
        FreeRtos::delay_ms(1000);
    }
}

fn bitstring_to_pixels(bitstring: &str) -> Vec<RGB8> {
    let mut pixels = Vec::new();
    for c in bitstring.chars().rev() {
        match c {
            '0' => pixels.push(RGB8::from((0, 0, 0))),
            '1' => pixels.push(RGB8::from((20, 0, 0))),
            _ => (),
        }
    }
    pixels
}

// Draws green mask over the clock face and then reveals the pixels representing the time
fn startup_animation(
    led_strip: &mut LedPixelEsp32Rmt<RGB8, LedPixelColorGrb24>,
    time_pixels: Vec<RGB8>,
) {
    let mask = std::iter::repeat(RGB8::from((0, 5, 0)))
        .take(32)
        .collect::<Vec<RGB8>>();

    let mut pixels = std::iter::repeat(RGB8::from((0, 0, 0)))
        .take(32)
        .collect::<Vec<RGB8>>();

    for index in 0..32 {
        pixels[index] = mask[index];
        led_strip.write(pixels.clone()).unwrap();
        FreeRtos::delay_ms(25);
    }
    FreeRtos::delay_ms(200);
    for index in (0..32).rev() {
        pixels[index] = time_pixels[index];
        led_strip.write(pixels.clone()).unwrap();
        FreeRtos::delay_ms(25);
    }
}
