/**
 * @file hal_display.h
 * @brief Hardware Abstraction Layer for Display Controller (TFT_eSPI / SPI DMA)
 */

#pragma once

#include <stdint.h>
#include <stdbool.h>
#include "lvgl.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Initialize the physical display controller, SPI bus, and backlight.
 * @return true on success, false otherwise.
 */
bool hal_display_init(void);

/** Draw a persistent, LVGL-independent panel test pattern. */
void hal_display_show_diagnostics(void);

/**
 * @brief LVGL 9 flush callback to transfer rendered pixels to the display.
 * @param disp Pointer to the LVGL display object.
 * @param area Area bounding box to update.
 * @param px_map Pointer to pixel buffer.
 */
void hal_display_flush_cb(lv_display_t *disp, const lv_area_t *area, uint8_t *px_map);

/**
 * @brief Set the LCD backlight brightness.
 * @param duty Brightness value (0 - 255).
 */
void hal_display_set_backlight(uint8_t duty);

#ifdef __cplusplus
}
#endif
