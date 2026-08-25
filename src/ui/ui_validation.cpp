/**
 * @file ui_validation.cpp
 * @brief Wizard Academy (אקדמיית הקוסמים) Hebrew RTL Splash Screen
 */

#include "ui_validation.h"
#include "theme_manager.h"
#include "hal_lvgl.h"
#include "bsp_config.h"
#include <Arduino.h>
#include "esp_heap_caps.h"

static lv_obj_t *status_msg_label = NULL;
static lv_obj_t *start_adventure_btn = NULL;
static uint32_t adventure_click_count = 0;

/**
 * @brief Button click handler with dynamic tactile feedback
 */
static void on_start_adventure_clicked(lv_event_t *e) {
    lv_event_code_t code = lv_event_get_code(e);

    if (code == LV_EVENT_CLICKED) {
        adventure_click_count++;

        if (status_msg_label) {
            char status_buf[128];
            snprintf(status_buf, sizeof(status_buf), 
                     "✨ ההרפתקה מתחילה! (לחיצה #%u) ✨", 
                     (unsigned int)adventure_click_count);
            lv_label_set_text(status_msg_label, status_buf);
            lv_obj_set_style_text_color(status_msg_label, lv_color_hex(0x38BDF8), LV_PART_MAIN); // Sky Blue glow
        }

        Serial.printf("[UI_EVENT] Hebrew button 'התחל הרפתקה' pressed! Total clicks: %u\n", 
                      (unsigned int)adventure_click_count);
    }
}

void ui_validation_init(void) {
    // 2. Configure Active Screen
    lv_obj_t *scr = lv_screen_active();
    lv_obj_add_style(scr, &style_screen_bg, LV_PART_MAIN);
    lv_obj_set_style_base_dir(scr, LV_BASE_DIR_RTL, LV_PART_MAIN); // Global RTL base direction


    // 2. Main Title Label (Hebrew RTL)
    lv_obj_t *title_label = lv_label_create(scr);
    lv_label_set_text(title_label, "אקדמיית הקוסמים");
    lv_obj_add_style(title_label, &style_title_hebrew, LV_PART_MAIN);
    lv_obj_align(title_label, LV_ALIGN_TOP_MID, 0, 40);

    // 3. Subtitle / Version Label
    lv_obj_t *sub_label = lv_label_create(scr);
    lv_label_set_text(sub_label, "הכנה למבחני מחוננים");
    lv_obj_add_style(sub_label, &style_subtitle_hebrew, LV_PART_MAIN);
    lv_obj_align(sub_label, LV_ALIGN_TOP_MID, 0, 70);

    // 4. Memory Info Card (keep it for debugging, apply card style)
    lv_obj_t *info_card = lv_obj_create(scr);
    lv_obj_set_size(info_card, 280, 110);
    lv_obj_align(info_card, LV_ALIGN_TOP_MID, 0, 110);
    theme_apply_card(info_card);
    lv_obj_remove_flag(info_card, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t *info_text = lv_label_create(info_card);
    char mem_buf[160];
    snprintf(mem_buf, sizeof(mem_buf), 
             "Display: %dx%d (Portrait)\n"
             "Buffer: 2x %u lines (SRAM DMA)\n"
             "Free SRAM: %u KB\n"
             "Free PSRAM: %u KB",
             BSP_LCD_H_RES, BSP_LCD_V_RES,
             BSP_LCD_BUFFER_LINES,
             (unsigned int)(heap_caps_get_free_size(MALLOC_CAP_INTERNAL) / 1024),
             (unsigned int)(heap_caps_get_free_size(MALLOC_CAP_SPIRAM) / 1024));
    lv_label_set_text(info_text, mem_buf);
    // keep it simple LTR for English memory stats
    lv_obj_set_style_text_color(info_text, lv_color_hex(0xCBD5E1), LV_PART_MAIN);
    lv_obj_set_style_text_font(info_text, &lv_font_montserrat_12, LV_PART_MAIN);
    lv_obj_align(info_text, LV_ALIGN_CENTER, 0, 0);

    // 5. Interactive Center Button (Hebrew)
    start_adventure_btn = lv_button_create(scr);
    lv_obj_set_size(start_adventure_btn, 220, 60);
    lv_obj_align(start_adventure_btn, LV_ALIGN_CENTER, 0, 80);
    theme_apply_btn_main(start_adventure_btn); // Apply all the fancy styling!
    lv_obj_add_event_cb(start_adventure_btn, on_start_adventure_clicked, LV_EVENT_CLICKED, NULL);

    lv_obj_t *btn_label = lv_label_create(start_adventure_btn);
    lv_label_set_text(btn_label, "התחל הרפתקה");
    lv_obj_set_style_text_font(btn_label, &lv_font_hebrew_24, LV_PART_MAIN);
    lv_obj_center(btn_label);

    // 6. Interactive Touch Status Label
    status_msg_label = lv_label_create(scr);
    lv_label_set_text(status_msg_label, "לחץ על הכפתור כדי להתחיל");
    lv_obj_add_style(status_msg_label, &style_subtitle_hebrew, LV_PART_MAIN);
    lv_obj_set_style_base_dir(status_msg_label, LV_BASE_DIR_RTL, LV_PART_MAIN);
    lv_obj_align(status_msg_label, LV_ALIGN_BOTTOM_MID, 0, -30);
}
