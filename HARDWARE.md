# SpotPear ESP32S3-MAX35 hardware notes

Verified against the manufacturer's `ESP32S3-3.5inch-AI-1` schematic:

| Signal | ESP32-S3 GPIO |
| --- | ---: |
| LCD CS | 5 |
| LCD DC | 47 |
| LCD SCLK | 46 |
| LCD MOSI | 48 |
| LCD backlight PWM | 42 (active-low) |
| GT911 touch SDA | 15 |
| GT911 touch SCL | 14 |

The LCD controller is ST7796S, 320x480. LCD reset is handled by the panel's
on-board RC circuit and is not connected to an ESP32 GPIO.

## Display diagnostic mode

`BSP_DISPLAY_DIAGNOSTIC_MODE` in `include/bsp_config.h` can be set to `1` to
display a persistent six-color pattern and ASCII labels without starting LVGL.
The physical panel test passed on 2026-08-25, so normal game builds use `0`.

The `PWR` push button toggles board power. If the USB serial device disappears,
press `PWR` once. For ROM download mode, hold `BOOT`, connect USB, wait three
seconds, then release `BOOT`.
