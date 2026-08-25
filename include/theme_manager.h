/**
 * @file theme_manager.h
 * @brief Wizard Academy (אקדמיית הקוסמים) Global Theme & Styles for LVGL 9.x
 */

#pragma once

#include "lvgl.h"
#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ========================================================================== */
/*                         COLOR PALETTE DEFINITIONS                          */
/* ========================================================================== */
#define COLOR_BG_PARCHMENT_DARK   0x8B5CF6  /* Bright Magical Purple Background */
#define COLOR_BG_CARD             0x172554  /* Deep blue cards with high-contrast light text */
#define COLOR_BORDER_CARD         0x60A5FA  /* Accessible blue border */
#define COLOR_GOLD_PRIMARY        0x10B981  /* Emerald Green Primary Button */
#define COLOR_GOLD_GRADIENT_END   0x059669  /* Darker Green Gradient End */
#define COLOR_GOLD_PRESSED        0x047857  /* Deep Green Pressed State */
#define COLOR_GOLD_BORDER_LIGHT   0x34D399  /* Highlight Top Border */
#define COLOR_TEXT_LIGHT          0x1E293B  /* Dark Text for readability on white */
#define COLOR_TEXT_MUTED          0x64748B  /* Soft Slate Muted Text */
#define COLOR_MAGIC_PURPLE        0x3B82F6  /* Bright Sky Blue Accent */

/* ========================================================================== */
/*                         FONT DECLARATIONS                                  */
/* ========================================================================== */
LV_FONT_DECLARE(lv_font_hebrew_24);
LV_FONT_DECLARE(lv_font_hebrew_16);

/* ========================================================================== */
/*                         GLOBAL STYLES EXPORT                               */
/* ========================================================================== */
extern lv_style_t style_screen_bg;
extern lv_style_t style_btn_main;
extern lv_style_t style_btn_main_pressed;
extern lv_style_t style_card;
extern lv_style_t style_title_hebrew;
extern lv_style_t style_subtitle_hebrew;

/**
 * @brief Initialize all global theme styles, gradients, transitions, and RTL defaults.
 */
void theme_manager_init(void);

/**
 * @brief Apply the primary Wizard Academy theme to a button widget.
 * @param btn Pointer to the LVGL button object.
 */
void theme_apply_btn_main(lv_obj_t *btn);

/**
 * @brief Apply the standard container card styling.
 * @param card Pointer to the LVGL obj container.
 */
void theme_apply_card(lv_obj_t *card);

#ifdef __cplusplus
}
#endif
