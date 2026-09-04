/**
 * @file hal_battery.cpp
 * @brief Battery & Power HAL Implementation for SpotPear ESP32S3-MAX35
 */

#include "hal_battery.h"
#include <Arduino.h>
#include <stdio.h>

/* Piecewise linear LiPo discharge curve (mV -> %) */
typedef struct {
    uint32_t mv;
    uint8_t pct;
} LipoPoint_t;

static const LipoPoint_t LIPO_TABLE[] = {
    {4180, 100},
    {4050, 90},
    {3950, 80},
    {3870, 70},
    {3820, 60},
    {3780, 50},
    {3740, 40},
    {3710, 30},
    {3670, 20},
    {3600, 10},
    {3450, 5},
    {3300, 0}
};
static const size_t LIPO_TABLE_SIZE = sizeof(LIPO_TABLE) / sizeof(LIPO_TABLE[0]);

// Exponential Moving Average filter state
static uint32_t s_filtered_mv = 0;
static bool s_initialized = false;

bool hal_battery_init(void) {
    // Configure ADC pin attenuation (11dB = max input ~3100mV, pin sees max 2100mV)
    analogSetPinAttenuation(BSP_BAT_ADC_PIN, ADC_11db);
    analogSetPinAttenuation(BSP_USB_SENSE_PIN, ADC_11db);

    // Initial warm-up read
    s_filtered_mv = hal_battery_get_voltage_mv();
    s_initialized = true;

    Serial.printf("[BATTERY] Initialized on GPIO %d (VBAT) & GPIO %d (USB Sense). Initial: %u mV (%u%%), USB: %s\n",
                  BSP_BAT_ADC_PIN, BSP_USB_SENSE_PIN,
                  s_filtered_mv, hal_battery_get_percentage(),
                  hal_battery_is_charging() ? "CONNECTED" : "DISCONNECTED");
    return true;
}

uint32_t hal_battery_get_voltage_mv(void) {
    const int SAMPLES = 16;
    uint32_t total = 0;
    uint32_t min_v = UINT32_MAX;
    uint32_t max_v = 0;

    for (int i = 0; i < SAMPLES; i++) {
        uint32_t v = (uint32_t)analogReadMilliVolts(BSP_BAT_ADC_PIN);
        total += v;
        if (v < min_v) min_v = v;
        if (v > max_v) max_v = v;
        delayMicroseconds(250);
    }

    // Discard min and max outliers
    uint32_t pin_mv = (total - min_v - max_v) / (SAMPLES - 2);

    // Hardware divider: R57 (100k) + R60 (100k) -> V_bat = pin_mv * 2
    uint32_t battery_mv = pin_mv * 2;

    // Apply Exponential Moving Average (EMA) to smooth out readings
    if (s_filtered_mv == 0) {
        s_filtered_mv = battery_mv;
    } else {
        // Alpha = 0.25 (75% old, 25% new)
        s_filtered_mv = (s_filtered_mv * 3 + battery_mv) / 4;
    }

    return s_filtered_mv;
}

uint8_t hal_battery_get_percentage(void) {
    uint32_t mv = hal_battery_get_voltage_mv();

    if (mv >= LIPO_TABLE[0].mv) {
        return 100;
    }
    if (mv <= LIPO_TABLE[LIPO_TABLE_SIZE - 1].mv) {
        return 0;
    }

    // Linear interpolation between table segments
    for (size_t i = 0; i < LIPO_TABLE_SIZE - 1; i++) {
        if (mv <= LIPO_TABLE[i].mv && mv >= LIPO_TABLE[i + 1].mv) {
            uint32_t v_high = LIPO_TABLE[i].mv;
            uint32_t v_low  = LIPO_TABLE[i + 1].mv;
            uint8_t  p_high = LIPO_TABLE[i].pct;
            uint8_t  p_low  = LIPO_TABLE[i + 1].pct;

            uint32_t range_v = v_high - v_low;
            uint32_t range_p = p_high - p_low;

            uint32_t pct = p_low + ((mv - v_low) * range_p) / range_v;
            return (uint8_t)(pct > 100 ? 100 : pct);
        }
    }

    return 50;
}

bool hal_battery_is_charging(void) {
    // When USB is connected, USB_VCC is 5V -> GPIO 7 sees ~2.5V (2500mV)
    // When USB is disconnected, GPIO 7 is ~0V
    uint32_t usb_pin_mv = (uint32_t)analogReadMilliVolts(BSP_USB_SENSE_PIN);
    return (usb_pin_mv > 1200);
}

const char* hal_battery_get_icon(void) {
    if (hal_battery_is_charging()) {
        uint32_t mv = hal_battery_get_voltage_mv();
        if (mv < 2500) {
            return "🔌";
        }
        return "⚡";
    }
    uint8_t pct = hal_battery_get_percentage();
    if (pct > 20) {
        return "🔋";
    }
    return "🪫"; // Low battery
}

void hal_battery_get_status_text(char *buf, size_t max_len) {
    if (!buf || max_len == 0) return;

    bool charging = hal_battery_is_charging();
    uint32_t mv = hal_battery_get_voltage_mv();
    float v = (float)mv / 1000.0f;

    if (charging) {
        if (mv < 2500) {
            snprintf(buf, max_len, "חשמל USB 🔌 (ללא סוללה)");
        } else {
            uint8_t pct = hal_battery_get_percentage();
            snprintf(buf, max_len, "טעינה %u%% ⚡ (%.2fV)", (unsigned)pct, v);
        }
    } else {
        uint8_t pct = hal_battery_get_percentage();
        snprintf(buf, max_len, "סוללה %u%% %s (%.2fV)", (unsigned)pct, (pct > 20) ? "🔋" : "🪫", v);
    }
}

