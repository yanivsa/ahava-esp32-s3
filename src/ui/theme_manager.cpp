/**
 * @file theme_manager.cpp
 * @brief Wizard Academy (אקדמיית הקוסמים) Global Theme & Styles Implementation
 */

#include "theme_manager.h"

/* Global Styles Definitions */
lv_style_t style_screen_bg;
lv_style_t style_btn_main;
lv_style_t style_btn_main_pressed;
lv_style_t style_card;
lv_style_t style_title_hebrew;
lv_style_t style_subtitle_hebrew;

static lv_style_transition_dsc_t trans_btn_main;
static const lv_style_prop_t trans_props[] = {
    LV_STYLE_TRANSFORM_SCALE_X,
    LV_STYLE_TRANSFORM_SCALE_Y,
    LV_STYLE_TRANSLATE_Y,
    LV_STYLE_SHADOW_OFS_Y,
    LV_STYLE_SHADOW_WIDTH,
    LV_STYLE_BG_COLOR,
    (lv_style_prop_t)0
};

void theme_manager_init(void) {
    /* ---------------------------------------------------------------------- */
    /* 1. Screen Background: Dark Parchment / Mystic Slate                    */
    /* ---------------------------------------------------------------------- */
    lv_style_init(&style_screen_bg);
    lv_style_set_bg_color(&style_screen_bg, lv_color_hex(COLOR_BG_PARCHMENT_DARK));
    lv_style_set_bg_opa(&style_screen_bg, LV_OPA_COVER);

    /* ---------------------------------------------------------------------- */
    /* 2. Transition Config for Tactile / Bouncy Button                       */
    /* ---------------------------------------------------------------------- */
    lv_style_transition_dsc_init(
        &trans_btn_main, 
        trans_props, 
        lv_anim_path_ease_in_out, 
        90, /* 90ms fast responsive transition */
        0, 
        NULL
    );

    /* ---------------------------------------------------------------------- */
    /* 3. Primary Button: Golden / Amber with 3D Depth & RTL Hebrew Support  */
    /* ---------------------------------------------------------------------- */
    lv_style_init(&style_btn_main);
    
    // Geometry & Radius
    lv_style_set_radius(&style_btn_main, 20);
    lv_style_set_pad_hor(&style_btn_main, 24);
    lv_style_set_pad_ver(&style_btn_main, 14);

    // Background Gradient: Amber Gold (#F59E0B -> #D97706)
    lv_style_set_bg_color(&style_btn_main, lv_color_hex(COLOR_GOLD_PRIMARY));
    lv_style_set_bg_grad_color(&style_btn_main, lv_color_hex(COLOR_GOLD_GRADIENT_END));
    lv_style_set_bg_grad_dir(&style_btn_main, LV_GRAD_DIR_VER);
    lv_style_set_bg_opa(&style_btn_main, LV_OPA_COVER);

    // Top Highlight Border for subtle bevel
    lv_style_set_border_width(&style_btn_main, 1);
    lv_style_set_border_color(&style_btn_main, lv_color_hex(COLOR_GOLD_BORDER_LIGHT));
    lv_style_set_border_opa(&style_btn_main, (lv_opa_t)LV_OPA_60);

    // Deep 3D Drop Shadow
    lv_style_set_shadow_width(&style_btn_main, 15);
    lv_style_set_shadow_ofs_y(&style_btn_main, 5);
    lv_style_set_shadow_ofs_x(&style_btn_main, 0);
    lv_style_set_shadow_color(&style_btn_main, lv_color_hex(0x000000));
    lv_style_set_shadow_opa(&style_btn_main, (lv_opa_t)LV_OPA_50);

    // Typography & Direction
    lv_style_set_text_font(&style_btn_main, &lv_font_hebrew_24);
    lv_style_set_text_color(&style_btn_main, lv_color_hex(0xFFFFFF));
    lv_style_set_text_align(&style_btn_main, LV_TEXT_ALIGN_CENTER);
    lv_style_set_base_dir(&style_btn_main, LV_BASE_DIR_RTL);

    // Attach smooth transition
    lv_style_set_transition(&style_btn_main, &trans_btn_main);

    /* ---------------------------------------------------------------------- */
    /* 4. Pressed State: Physical 3D Press Down Effect                       */
    /* ---------------------------------------------------------------------- */
    lv_style_init(&style_btn_main_pressed);
    
    // Move down 4px and scale down slightly to simulate physical key compression
    lv_style_set_translate_y(&style_btn_main_pressed, 4);
    lv_style_set_transform_scale_x(&style_btn_main_pressed, 245);
    lv_style_set_transform_scale_y(&style_btn_main_pressed, 245);

    // Collapse drop shadow under pressed button
    lv_style_set_shadow_ofs_y(&style_btn_main_pressed, 2);
    lv_style_set_shadow_width(&style_btn_main_pressed, 6);
    lv_style_set_shadow_opa(&style_btn_main_pressed, (lv_opa_t)LV_OPA_60);

    // Darker Amber Color
    lv_style_set_bg_color(&style_btn_main_pressed, lv_color_hex(COLOR_GOLD_PRESSED));
    lv_style_set_bg_grad_color(&style_btn_main_pressed, lv_color_hex(COLOR_GOLD_PRESSED));

    /* ---------------------------------------------------------------------- */
    /* 5. Hebrew Title Style                                                  */
    /* ---------------------------------------------------------------------- */
    lv_style_init(&style_title_hebrew);
    lv_style_set_text_font(&style_title_hebrew, &lv_font_hebrew_24);
    lv_style_set_pad_left(&style_title_hebrew, 3);
    lv_style_set_pad_right(&style_title_hebrew, 3);
    lv_style_set_text_color(&style_title_hebrew, lv_color_hex(0xFFFFFF)); // White for high contrast on purple
    lv_style_set_text_align(&style_title_hebrew, LV_TEXT_ALIGN_CENTER);
    lv_style_set_base_dir(&style_title_hebrew, LV_BASE_DIR_RTL);

    /* ---------------------------------------------------------------------- */
    /* 6. Subtitle Style                                                      */
    /* ---------------------------------------------------------------------- */
    lv_style_init(&style_subtitle_hebrew);
    lv_style_set_text_font(&style_subtitle_hebrew, &lv_font_hebrew_24);
    lv_style_set_pad_left(&style_subtitle_hebrew, 3);
    lv_style_set_pad_right(&style_subtitle_hebrew, 3);
    lv_style_set_text_color(&style_subtitle_hebrew, lv_color_hex(0xF1F5F9)); // Soft bright white/slate
    lv_style_set_text_align(&style_subtitle_hebrew, LV_TEXT_ALIGN_CENTER);
    lv_style_set_base_dir(&style_subtitle_hebrew, LV_BASE_DIR_RTL);

    /* ---------------------------------------------------------------------- */
    /* 7. Container Card Style                                                */
    /* ---------------------------------------------------------------------- */
    lv_style_init(&style_card);
    lv_style_set_bg_color(&style_card, lv_color_hex(COLOR_BG_CARD));
    lv_style_set_bg_opa(&style_card, LV_OPA_COVER);
    lv_style_set_radius(&style_card, 24);
    lv_style_set_border_width(&style_card, 1);
    lv_style_set_border_color(&style_card, lv_color_hex(COLOR_BORDER_CARD));
    lv_style_set_pad_all(&style_card, 12);
}

void theme_apply_btn_main(lv_obj_t *btn) {
    if (!btn) return;
    lv_obj_add_style(btn, &style_btn_main, LV_STATE_DEFAULT);
    lv_obj_add_style(btn, &style_btn_main_pressed, LV_STATE_PRESSED);
    lv_obj_set_style_base_dir(btn, LV_BASE_DIR_RTL, LV_PART_MAIN);
}

void theme_apply_card(lv_obj_t *card) {
    if (!card) return;
    lv_obj_add_style(card, &style_card, LV_PART_MAIN);
    lv_obj_set_style_base_dir(card, LV_BASE_DIR_RTL, LV_PART_MAIN);
}
