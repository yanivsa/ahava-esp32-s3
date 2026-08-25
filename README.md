# Ahava for SpotPear ESP32S3-MAX35

An offline Hebrew quiz game adapted from Ahava for the SpotPear ESP32S3-MAX35
(ST7796S 320x480 display and GT911 capacitive touch).

## Build

```sh
pio run -e spotpear-esp32s3-max35
```

The device uses its existing, verified bootloader and OTA partition table. For
USB recovery, flash the application image only at `0x10000`; see
[`HARDWARE.md`](HARDWARE.md).

## Wi-Fi and OTA

Press **OTA** on the device. On first use it opens the access point
`Ahava-Setup` (password `Ahava1234`). Select the home Wi-Fi in the captive
portal. Credentials are stored by the ESP32 and are never committed here.

Every push to `main` builds the application and replaces the public `latest`
release asset. The device downloads that asset over HTTPS with certificate and
hostname validation.
