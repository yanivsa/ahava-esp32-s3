/**
 * @file hal_lvgl.h
 * @brief LVGL 9.x HAL, Draw Buffer Management, Thread-Safe Locking & FreeRTOS GUI Task
 */

#pragma once

#include <stdint.h>
#include <stdbool.h>
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "lvgl.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Initialize LVGL core, allocate DMA draw buffers, register display & touch drivers,
 *        and launch the dedicated GUI FreeRTOS task on Core 1.
 * @return true on success, false if initialization or memory allocation failed.
 */
bool hal_lvgl_init(void);

/**
 * @brief Acquire recursive mutex lock before interacting with any LVGL API from background tasks.
 * @param timeout_ms Max time to wait in milliseconds (use UINT32_MAX for portMAX_DELAY).
 * @return true if lock was acquired, false on timeout.
 */
bool hal_lvgl_lock(uint32_t timeout_ms);

/**
 * @brief Release recursive mutex lock after completing LVGL operations.
 */
void hal_lvgl_unlock(void);

/**
 * @brief Get the active LVGL display instance.
 * @return Pointer to lv_display_t.
 */
lv_display_t* hal_lvgl_get_display(void);

/**
 * @brief Get the active LVGL input device instance.
 * @return Pointer to lv_indev_t.
 */
lv_indev_t* hal_lvgl_get_indev(void);

#ifdef __cplusplus
}
#endif
