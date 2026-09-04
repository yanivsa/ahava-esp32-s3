/**
 * @file hal_battery.h
 * @brief Battery & Power HAL for SpotPear ESP32S3-MAX35
 * 
 * Hardware Voltage Divider (from schematic):
 * - V_BAT -> R57 (100k) -> Node -> R60 (100k) -> GND  [Ratio 1:2]
 *   Node with C123 (1uF) connected to ESP32-S3 GPIO 6 (ADC1_CH5).
 * - USB_VCC -> R59 (100k) -> Node -> R61 (100k) -> GND
 *   Node connected to ESP32-S3 GPIO 7 (ADC1_CH6).
 */

#pragma once

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include "bsp_config.h"

#ifdef __cplusplus
extern "C" {
#endif

#ifndef BSP_BAT_ADC_PIN
#define BSP_BAT_ADC_PIN         6   // ADC1 Channel 5
#endif
#ifndef BSP_USB_SENSE_PIN
#define BSP_USB_SENSE_PIN       7   // ADC1 Channel 6
#endif

// Voltage limits for 1S LiPo / Li-ion (in millivolts)
#define BATTERY_VOLTAGE_MIN_MV  3300  // 0%
#define BATTERY_VOLTAGE_MAX_MV  4200  // 100%

/**
 * @brief Initialize battery and power sensing GPIOs and ADC configuration.
 * @return true on success.
 */
bool hal_battery_init(void);

/**
 * @brief Read battery voltage in millivolts (filtered via multi-sampling).
 * @return Battery voltage in mV (e.g. 3850 for 3.85V).
 */
uint32_t hal_battery_get_voltage_mv(void);

/**
 * @brief Calculate estimated battery percentage (0 to 100%).
 *        Uses piecewise linear approximation matching standard 1S LiPo discharge curve.
 * @return Battery percentage (0 - 100).
 */
uint8_t hal_battery_get_percentage(void);

/**
 * @brief Check if USB power is connected / battery is charging.
 * @return true if USB cable is plugged in, false if running on battery alone.
 */
bool hal_battery_is_charging(void);

/**
 * @brief Get battery status icon emoji string ("⚡" if charging, "🔋" if on battery).
 */
const char* hal_battery_get_icon(void);

/**
 * @brief Format a concise Hebrew battery status string into buffer.
 *        e.g. "85% 🔋 (4.02V)" or "טעינה ⚡ (4.15V)"
 * @param buf Output buffer
 * @param max_len Buffer size
 */
void hal_battery_get_status_text(char *buf, size_t max_len);

#ifdef __cplusplus
}
#endif
