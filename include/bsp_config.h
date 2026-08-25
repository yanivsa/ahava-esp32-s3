/**
 * @file bsp_config.h
 * @brief Board Support Package (BSP) configuration for ESP32-S3 Wizard Academy
 * Target: SpotPear ESP32S3-MAX35 3.5" 320x480 capacitive-touch board
 */

#pragma once

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ========================================================================== */
/*                         DISPLAY CONFIGURATION                              */
/* ========================================================================== */
#define BSP_LCD_H_RES              320
#define BSP_LCD_V_RES              480
#define BSP_LCD_ROTATION           0    // 0: Portrait (320x480), 1: Landscape (480x320)
#define BSP_LCD_COLOR_DEPTH        16   // 16-bit RGB565
#define BSP_DISPLAY_DIAGNOSTIC_MODE 0   // Panel was physically validated; start the game normally

/* 
 * Buffer Sizing Strategy:
 * 1/10th of display height = 48 lines
 * Draw Buffer (Pixels) = 320 * 48 = 15,360 pixels
 * Draw Buffer (Bytes)  = 15,360 * 2 = 30,720 bytes (~30 KB)
 * Double buffering in Internal SRAM DMA consumes ~60 KB of SRAM,
 * leaving ample SRAM for FreeRTOS stacks and Wi-Fi/BLE network buffers.
 */
#define BSP_LCD_BUFFER_LINES       48
#define BSP_LCD_DRAW_BUF_PIXELS    (BSP_LCD_H_RES * BSP_LCD_BUFFER_LINES)
#define BSP_LCD_DRAW_BUF_BYTES     (BSP_LCD_DRAW_BUF_PIXELS * (BSP_LCD_COLOR_DEPTH / 8))

/* ========================================================================== */
/*                         TOUCH SCREEN CONFIGURATION                         */
/* ========================================================================== */
#define BSP_TOUCH_TYPE_GT911       1
#define BSP_TOUCH_TYPE_CST820      0

#define BSP_TOUCH_I2C_PORT         I2C_NUM_0
#define BSP_TOUCH_I2C_SDA_PIN      15
#define BSP_TOUCH_I2C_SCL_PIN      14
#define BSP_TOUCH_I2C_FREQ_HZ      400000
#define BSP_TOUCH_INT_PIN          -1
#define BSP_TOUCH_RST_PIN          -1

/* GT911 Default I2C Address (0x5D or 0x14 depending on reset pin strapping) */
#define BSP_TOUCH_GT911_ADDR_1     0x5D
#define BSP_TOUCH_GT911_ADDR_2     0x14

/* ========================================================================== */
/*                         BACKLIGHT PWM CONFIGURATION                        */
/* ========================================================================== */
#define BSP_LCD_BL_PIN             42
#define BSP_LCD_BL_PWM_FREQ        5000
#define BSP_LCD_BL_PWM_RES_BITS    8
#define BSP_LCD_BL_DEFAULT_DUTY    0    // MAX35 uses a P-channel high-side switch (active-low)

/* ========================================================================== */
/*                         I2S AUDIO DAC CONFIGURATION                        */
/* ========================================================================== */
#define BSP_I2S_NUM                I2S_NUM_0
#define BSP_I2S_BCLK_PIN           4    // Bit Clock (BCLK) - safe GPIO
#define BSP_I2S_LRC_PIN            5    // Word Select / Left-Right Clock (WS/LRC)
#define BSP_I2S_DOUT_PIN           16   // Serial Data Output (DOUT)
#define BSP_I2S_SAMPLE_RATE        22050
#define BSP_AUDIO_ENABLED          0    // ES8311 codec requires board-specific initialization
#define BSP_OTA_ENABLED            1    // Wi-Fi is provisioned at runtime; firmware is fetched over verified HTTPS

#define BSP_AUDIO_TASK_NAME        "audio_task"
#define BSP_AUDIO_TASK_STACK_SIZE  (1024 * 4)   // 4 KB Stack
#define BSP_AUDIO_TASK_PRIORITY    3            // Normal audio priority
#define BSP_AUDIO_TASK_CORE_ID     0            // Core 0 (leaving Core 1 for GUI)

/* ========================================================================== */
/*                         FREERTOS GUI TASK CONFIG                           */
/* ========================================================================== */
#define BSP_GUI_TASK_NAME          "gui_task"
#define BSP_GUI_TASK_STACK_SIZE    (1024 * 8)    // 8 KB stack
#define BSP_GUI_TASK_PRIORITY      5             // Higher priority than app tasks
#define BSP_GUI_TASK_CORE_ID       1             // Pinned to Core 1 (Core 0 for Wi-Fi/Sys)
#define BSP_GUI_TICK_PERIOD_MS     5             // LVGL tick step

#ifdef __cplusplus
}
#endif
