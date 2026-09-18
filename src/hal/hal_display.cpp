/**
 * @file hal_display.cpp
 * @brief Display controller HAL implementation using TFT_eSPI & LEDC Backlight
 */

#include "hal_display.h"
#include "bsp_config.h"
#include <Arduino.h>
#include <TFT_eSPI.h>
#include "esp_heap_caps.h"
#include "draw/sw/lv_draw_sw.h"

static TFT_eSPI tft = TFT_eSPI();
static uint8_t *rotation_buf = nullptr;
static size_t rotation_buf_size = 0;

static bool ensure_rotation_buffer(size_t required) {
    if (rotation_buf && rotation_buf_size >= required) return true;

    if (rotation_buf) {
        heap_caps_free(rotation_buf);
        rotation_buf = nullptr;
        rotation_buf_size = 0;
    }

    const size_t alloc_size = required > BSP_LCD_DRAW_BUF_BYTES ? required : BSP_LCD_DRAW_BUF_BYTES;
    rotation_buf = (uint8_t *)heap_caps_malloc(alloc_size, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
    if (!rotation_buf) {
        rotation_buf = (uint8_t *)heap_caps_malloc(alloc_size, MALLOC_CAP_8BIT);
    }
    if (!rotation_buf) {
        Serial.printf("[HAL_DISP] ERROR: rotation buffer allocation failed (%u bytes)\n",
                      (unsigned)alloc_size);
        return false;
    }

    rotation_buf_size = alloc_size;
    Serial.printf("[HAL_DISP] Rotation buffer ready: %u bytes\n", (unsigned)alloc_size);
    return true;
}

bool hal_display_init(void) {
    // 1. Enable the MAX35 active-low backlight before initializing the panel.
    #if ESP_ARDUINO_VERSION_MAJOR >= 3
        ledcAttach(BSP_LCD_BL_PIN, BSP_LCD_BL_PWM_FREQ, BSP_LCD_BL_PWM_RES_BITS);
        ledcWrite(BSP_LCD_BL_PIN, BSP_LCD_BL_DEFAULT_DUTY);
    #else
        ledcSetup(0, BSP_LCD_BL_PWM_FREQ, BSP_LCD_BL_PWM_RES_BITS);
        ledcAttachPin(BSP_LCD_BL_PIN, 0);
        ledcWrite(0, BSP_LCD_BL_DEFAULT_DUTY);
    #endif

    // 2. Initialize TFT display controller.
    tft.init();
    tft.setRotation(BSP_LCD_ROTATION);

    // Visible power-on self-test: proves backlight, SPI wiring and controller init
    // independently of LVGL and the game UI.
    Serial.println("[HAL_DISP] Running RED/GREEN/BLUE panel self-test...");
    tft.fillScreen(TFT_RED);
    delay(750);
    tft.fillScreen(TFT_GREEN);
    delay(750);
    tft.fillScreen(TFT_BLUE);
    delay(750);
    tft.fillScreen(TFT_BLACK);

    Serial.printf("[HAL_DISP] Display initialized: %dx%d, Rotation=%d\n", 
                  BSP_LCD_H_RES, BSP_LCD_V_RES, BSP_LCD_ROTATION);
    return true;
}

void hal_display_show_diagnostics(void) {
    const int16_t w = tft.width();
    const int16_t h = tft.height();
    const int16_t bar_h = h / 6;

    tft.fillScreen(TFT_WHITE);
    tft.fillRect(0, 0 * bar_h, w, bar_h, TFT_RED);
    tft.fillRect(0, 1 * bar_h, w, bar_h, TFT_GREEN);
    tft.fillRect(0, 2 * bar_h, w, bar_h, TFT_BLUE);
    tft.fillRect(0, 3 * bar_h, w, bar_h, TFT_YELLOW);
    tft.fillRect(0, 4 * bar_h, w, bar_h, TFT_CYAN);
    tft.fillRect(0, 5 * bar_h, w, h - (5 * bar_h), TFT_MAGENTA);

    tft.setTextDatum(MC_DATUM);
    tft.setTextColor(TFT_WHITE, TFT_BLACK);
    tft.drawCentreString("DISPLAY HARDWARE TEST", w / 2, (h / 2) - 22, 2);
    tft.drawCentreString("ST7796S 320x480", w / 2, h / 2, 2);
    tft.drawCentreString("BACKLIGHT GPIO42", w / 2, (h / 2) + 22, 2);

    Serial.println("[HAL_DISP] Persistent six-color diagnostic pattern is active.");
}

void hal_display_flush_cb(lv_display_t *disp, const lv_area_t *area, uint8_t *px_map) {
    const lv_area_t *flush_area = area;
    uint8_t *flush_map = px_map;
    lv_area_t rotated_area{};

    const lv_display_rotation_t rotation = lv_display_get_rotation(disp);
    if (rotation != LV_DISPLAY_ROTATION_0) {
        rotated_area = *area;
        lv_display_rotate_area(disp, &rotated_area);

        const lv_color_format_t cf = lv_display_get_color_format(disp);
        const int32_t src_w = lv_area_get_width(area);
        const int32_t src_h = lv_area_get_height(area);
        const uint32_t src_stride = lv_draw_buf_width_to_stride(src_w, cf);
        const uint32_t dst_stride = lv_draw_buf_width_to_stride(lv_area_get_width(&rotated_area), cf);
        const size_t required = (size_t)dst_stride * (size_t)lv_area_get_height(&rotated_area);

        if (!ensure_rotation_buffer(required)) {
            Serial.println("[HAL_DISP] ERROR: dropping rotated frame because no rotation buffer is available.");
            lv_display_flush_ready(disp);
            return;
        }

        lv_draw_sw_rotate(px_map, rotation_buf,
                          src_w, src_h,
                          (int32_t)src_stride, (int32_t)dst_stride,
                          rotation, cf);

        flush_area = &rotated_area;
        flush_map = rotation_buf;
    }

    const uint32_t w = (uint32_t)lv_area_get_width(flush_area);
    const uint32_t h = (uint32_t)lv_area_get_height(flush_area);

    tft.startWrite();
    tft.setAddrWindow(flush_area->x1, flush_area->y1, w, h);
    tft.pushColors((uint16_t *)flush_map, w * h, true);
    tft.endWrite();

    lv_display_flush_ready(disp);
}

void hal_display_set_backlight(uint8_t duty) {
    // MAX35 uses an active-low PMOS switch: 255 brightness -> 0 PWM duty (full on), 0 brightness -> 255 (off)
    uint8_t hw_duty = (uint8_t)(255 - duty);
    #if ESP_ARDUINO_VERSION_MAJOR >= 3
        ledcWrite(BSP_LCD_BL_PIN, hw_duty);
    #else
        ledcWrite(0, hw_duty);
    #endif
}
