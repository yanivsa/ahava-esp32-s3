/**
 * @file hal_touch.h
 * @brief Hardware Abstraction Layer for Capacitive Touch (GT911 / CST820)
 */

#pragma once

#include <stdint.h>
#include <stdbool.h>
#include "lvgl.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Initialize the I2C bus and capacitive touch IC.
 * @return true on success, false otherwise.
 */
bool hal_touch_init(void);

/**
 * @brief LVGL 9 input device read callback for touch screen.
 * @param indev Pointer to the LVGL input device object.
 * @param data Pointer to input data structure to populate.
 */
void hal_touch_read_cb(lv_indev_t *indev, lv_indev_data_t *data);

/**
 * @brief Get milliseconds since the last detected touch or user activity.
 */
uint32_t hal_touch_get_last_activity_ms(void);

/**
 * @brief Record user activity timestamp.
 */
void hal_touch_record_activity(void);

#ifdef __cplusplus
}
#endif
