/**
 * @file hal_display.cpp
 * @brief Display controller HAL implementation using TFT_eSPI & LEDC Backlight
 */

#include "hal_display.h"
#include "bsp_config.h"
#include <Arduino.h>
#include <TFT_eSPI.h>

static TFT_eSPI tft = TFT_eSPI();

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
    uint32_t w = (area->x2 - area->x1 + 1);
    uint32_t h = (area->y2 - area->y1 + 1);

    tft.startWrite();
    tft.setAddrWindow(area->x1, area->y1, w, h);
    
    /* Push pixel data over high-speed SPI */
    tft.pushColors((uint16_t *)px_map, w * h, true);
    
    tft.endWrite();

    /* Crucial in LVGL 9: Notify LVGL that the flush operation is complete */
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
