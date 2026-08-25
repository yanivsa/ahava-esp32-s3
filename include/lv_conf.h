/**
 * @file lv_conf.h
 * Configuration file for LVGL v9.x on ESP32-S3 (Wizard Academy Project)
 * Updated with Bidirectional (Bidi) text processing & RTL Hebrew support.
 */

#ifndef LV_CONF_H
#define LV_CONF_H

#include <stdint.h>
#include "esp_heap_caps.h"

/*====================
   COLOR SETTINGS
 *====================*/
/* Color depth: 16 (RGB565), 24 (RGB888), 32 (ARGB8888) */
#define LV_COLOR_DEPTH 16

/* Swap the 2 bytes of RGB565 color. Useful if the display controller receives big-endian bytes over SPI */
#define LV_COLOR_16_SWAP 0

/*=========================
   MEMORY ALLOCATION SETTINGS
 *=========================*/
/*
 * In LVGL 9.x, memory allocator can be Custom, Builtin, or C-Library.
 * To ensure game assets, fonts, and screens never exhaust internal SRAM,
 * we route all general LVGL allocations (widgets, styles, caches) to ESP32-S3 PSRAM (SPIRAM).
 */
#define LV_USE_STDLIB_MALLOC    LV_STDLIB_CLIB
#define LV_USE_STDLIB_STRING    LV_STDLIB_CLIB
#define LV_USE_STDLIB_SPRINTF   LV_STDLIB_CLIB

/* Number of dynamic image caches / decoders */
#define LV_CACHE_DEF_SIZE       (512 * 1024) /* 512KB Image cache in PSRAM */

/*=========================
   HAL / OS SETTINGS
 *=========================*/
/* Operating System integration */
#define LV_USE_OS               LV_OS_NONE

/* Default display refresh period in milliseconds */
#define LV_DEF_REFR_PERIOD      16 /* ~60 FPS target */

/* Input device read period in milliseconds */
#define LV_DEF_INDEV_READ_PERIOD 16

/* Use custom tick source: LVGL 9 allows setting callback via lv_tick_set_cb() */
#define LV_USE_CUSTOM_TICK      0

/*=========================
   TEXT & BIDI / RTL SETTINGS
 *=========================*/
/**
 * Select character encoding for strings (UTF-8 required for Hebrew Unicode)
 */
#define LV_TXT_ENC LV_TXT_ENC_UTF8

/* Can break (wrap) texts on these chars */
#define LV_TXT_BREAK_CHARS " ,.;:-_!?"

/* Break long words if needed */
#define LV_TXT_LINE_BREAK_LONG_LEN 0
#define LV_TXT_LINE_BREAK_LONG_PRE_MIN_LEN 3
#define LV_TXT_LINE_BREAK_LONG_POST_MIN_LEN 3

/* The control character to use for signalling text recoloring */
#define LV_TXT_COLOR_CMD "#"

/**
 * Support bidirectional texts. Allows mixing Left-to-Right and Right-to-Left texts.
 * Processed according to Unicode Bidirectional Algorithm (UAX #9).
 */
#define LV_USE_BIDI 1
#if LV_USE_BIDI
    /*
     * Set default base direction:
     * - LV_BASE_DIR_AUTO: Detects base direction automatically from the first strong char (Recommended for multilingual)
     * - LV_BASE_DIR_RTL:  Enforces Right-to-Left by default (Ideal for Hebrew-only interfaces)
     * - LV_BASE_DIR_LTR:  Left-to-Right
     */
    #define LV_BIDI_BASE_DIR_DEF LV_BASE_DIR_AUTO
#endif

/* Disable Arabic contextual shaping since Hebrew does not use connected cursive glyph forms */
#define LV_USE_ARABIC_PERSIAN_CHARS 0

/*=========================
   FEATURE CONFIGURATION
 *=========================*/
/* Drawing pipeline */
#define LV_USE_DRAW_SW          1
#define LV_DRAW_SW_DRAW_UNIT_CNT 1
#define LV_DRAW_SW_COMPLEX      1

/* Runtime monitors are disabled on the release UI to save screen and CPU. */
#define LV_USE_PERF_MONITOR     0
#define LV_USE_MEM_MONITOR      0

/* Logging */
#define LV_USE_LOG              1
#define LV_LOG_LEVEL            LV_LOG_LEVEL_WARN
#define LV_LOG_PRINTF           1

/* Assertions */
#define LV_USE_ASSERT_NULL          1
#define LV_USE_ASSERT_MALLOC        1
#define LV_USE_ASSERT_STYLE         0
#define LV_USE_ASSERT_MEM_INTEGRITY 0
#define LV_USE_ASSERT_OBJ           0

/*==================
 * WIDGET SELECTION
 *==================*/
#define LV_USE_ANIMIMG    1
#define LV_USE_ARC        1
#define LV_USE_BAR        1
#define LV_USE_BTN        1
#define LV_USE_BTNMATRIX  1
#define LV_USE_CANVAS     1
#define LV_USE_CHECKBOX   1
#define LV_USE_DROPDOWN   1
#define LV_USE_IMAGE      1
#define LV_USE_LABEL      1
#define LV_USE_LINE       1
#define LV_USE_ROLLER     1
#define LV_USE_SLIDER     1
#define LV_USE_SWITCH     1
#define LV_USE_TEXTAREA   1
#define LV_USE_TABLE      1

/*==================
 * EXTRA WIDGETS
 *==================*/
#define LV_USE_CALENDAR   1
#define LV_USE_CHART      1
#define LV_USE_KEYBOARD   1
#define LV_USE_LIST       1
#define LV_USE_MENU       1
#define LV_USE_MSGBOX     1
#define LV_USE_SPINBOX    1
#define LV_USE_SPINNER    1
#define LV_USE_TABVIEW    1
#define LV_USE_TILEVIEW   1
#define LV_USE_WIN        1

/*==================
 * THEMES & STYLES
 *==================*/
#define LV_USE_THEME_DEFAULT 1
#define LV_THEME_DEFAULT_DARK 1
#define LV_THEME_DEFAULT_GROW 1
#define LV_THEME_DEFAULT_TRANSITION_TIME 80

/*==================
 * FONTS
 *==================*/
#define LV_FONT_MONTSERRAT_10 1
#define LV_FONT_MONTSERRAT_12 1
#define LV_FONT_MONTSERRAT_14 1
#define LV_FONT_MONTSERRAT_16 1
#define LV_FONT_MONTSERRAT_18 1
#define LV_FONT_MONTSERRAT_20 1
#define LV_FONT_MONTSERRAT_24 1
#define LV_FONT_MONTSERRAT_28 1
#define LV_FONT_MONTSERRAT_32 1
#define LV_FONT_DEFAULT &lv_font_montserrat_14

/* Declare custom fonts if needed */
#define LV_FONT_CUSTOM_DECLARE

#endif /* LV_CONF_H */
