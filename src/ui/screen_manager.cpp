/**
 * @file screen_manager.cpp
 * @brief Wizard Academy (אקדמיית הקוסמים) Screen Manager, Profiles, Dashboard, Quiz Engine & OTA UI
 */

#include "screen_manager.h"
#include "theme_manager.h"
#include "quiz_engine.h"
#include "player_data.h"
#include "audio_manager.h"
#include "ota_manager.h"
#include "hal_lvgl.h"
#include "bsp_config.h"
#include <Arduino.h>
#include "esp_heap_caps.h"

/* Global active child profile */
WizardProfile_t current_profile = PROFILE_NONE;

/* Active screen tracking */
static ScreenID_t current_screen_id = SCREEN_NONE;

/* Active Quiz Question reference and answer button handles */
static const Question_t *current_quiz_question = NULL;
static lv_obj_t *quiz_answer_btns[4] = { NULL, NULL, NULL, NULL };
static int current_subject_id = 0;

/* Child Profiles Metadata */
static const ProfileInfo_t PROFILES_DATA[] = {
    {
        .id = PROFILE_ORI,
        .name_hebrew = "אורי",
        .title_hebrew = "קוסם בכיר",
        .age = 11,
        .color_accent = 0x3B82F6, // Sapphire Blue
        .badge_symbol = "O"
    },
    {
        .id = PROFILE_ETHAN,
        .name_hebrew = "איתן",
        .title_hebrew = "קוסם חניך",
        .age = 8,
        .color_accent = 0xF59E0B, // Amber Gold
        .badge_symbol = "E"
    },
    {
        .id = PROFILE_AYALA,
        .name_hebrew = "אילה",
        .title_hebrew = "קוסמת צעירה",
        .age = 3,
        .color_accent = 0xEC4899, // Arcane Rose / Pink
        .badge_symbol = "A"
    }
};

const ProfileInfo_t* sm_get_profile_info(WizardProfile_t profile) {
    for (size_t i = 0; i < sizeof(PROFILES_DATA) / sizeof(PROFILES_DATA[0]); i++) {
        if (PROFILES_DATA[i].id == profile) {
            return &PROFILES_DATA[i];
        }
    }
    return &PROFILES_DATA[0];
}

void sm_init(void) {
    current_profile = PROFILE_NONE;
    current_screen_id = SCREEN_NONE;
    theme_manager_init();
    Serial.println("[SM] Screen Manager initialized.");
}

ScreenID_t sm_get_current_screen(void) {
    return current_screen_id;
}

/* ========================================================================== */
/*                         NAVIGATION CALLBACKS                               */
/* ========================================================================== */

static void on_profile_selected(lv_event_t *e) {
    if (lv_event_get_code(e) == LV_EVENT_CLICKED) {
        audio_play_click();

        WizardProfile_t selected = (WizardProfile_t)(uintptr_t)lv_event_get_user_data(e);
        current_profile = selected;

        const ProfileInfo_t *info = sm_get_profile_info(selected);
        Serial.printf("[UI] Selected Profile: %s (Age %u, Title: %s)\n", 
                      info->name_hebrew, info->age, info->title_hebrew);

        sm_load_screen(SCREEN_DASHBOARD);
    }
}

static void on_back_to_profiles_clicked(lv_event_t *e) {
    if (lv_event_get_code(e) == LV_EVENT_CLICKED) {
        audio_play_click();
        sm_load_screen(SCREEN_PROFILES);
    }
}

static void on_play_subject_clicked(lv_event_t *e) {
    if (lv_event_get_code(e) == LV_EVENT_CLICKED) {
        audio_play_click();
        int subject_id = (int)(uintptr_t)lv_event_get_user_data(e);
        current_subject_id = subject_id;
        Serial.printf("[UI] Play clicked for subject ID: %d\n", subject_id);
        sm_load_screen(SCREEN_QUIZ);
    }
}

static void on_back_to_dashboard_clicked(lv_event_t *e) {
    if (lv_event_get_code(e) == LV_EVENT_CLICKED) {
        audio_play_click();
        sm_load_screen(SCREEN_DASHBOARD);
    }
}

static void on_splash_start_clicked(lv_event_t *e) {
    if (lv_event_get_code(e) == LV_EVENT_CLICKED) {
        audio_play_click();
        sm_load_screen(SCREEN_PROFILES);
    }
}

static void on_msgbox_continue_clicked(lv_event_t *e) {
    if (lv_event_get_code(e) == LV_EVENT_CLICKED) {
        audio_play_click();
        sm_load_screen(SCREEN_QUIZ);
    }
}

/* ========================================================================== */
/*                         OTA UPDATE TRIGGER & PROGRESS MODAL                */
/* ========================================================================== */

static lv_obj_t *ota_modal_box = NULL;
static lv_obj_t *ota_status_lbl = NULL;
static lv_obj_t *ota_pct_lbl = NULL;
static lv_obj_t *ota_progress_bar = NULL;
static lv_obj_t *ota_close_btn = NULL;
static lv_timer_t *ota_update_timer = NULL;

static void on_ota_close_clicked(lv_event_t *e) {
    if (lv_event_get_code(e) == LV_EVENT_CLICKED) {
        audio_play_click();
        if (ota_update_timer) {
            lv_timer_delete(ota_update_timer);
            ota_update_timer = NULL;
        }
        if (ota_modal_box) {
            lv_obj_delete(ota_modal_box);
            ota_modal_box = NULL;
        }
        ota_reset_status();
    }
}

static void ota_ui_timer_cb(lv_timer_t *timer) {
    (void)timer;
    if (!ota_modal_box) return;

    OtaStatus_t status = ota_get_status();
    uint8_t pct = ota_get_progress_pct();
    const char *msg = ota_get_status_message();

    if (ota_status_lbl) {
        lv_label_set_text(ota_status_lbl, msg);
    }

    if (ota_progress_bar) {
        lv_bar_set_value(ota_progress_bar, pct, LV_ANIM_ON);
    }

    if (ota_pct_lbl) {
        char buf[32];
        snprintf(buf, sizeof(buf), "%u%%", (unsigned int)pct);
        lv_label_set_text(ota_pct_lbl, buf);
    }

    if (status == OTA_STATUS_SUCCESS) {
        lv_obj_set_style_border_color(ota_modal_box, lv_color_hex(0x10B981), LV_PART_MAIN); // Green
        if (ota_close_btn) lv_obj_add_flag(ota_close_btn, LV_OBJ_FLAG_HIDDEN);
    } else if (status == OTA_STATUS_FAILED) {
        lv_obj_set_style_border_color(ota_modal_box, lv_color_hex(0xEF4444), LV_PART_MAIN); // Red
        if (ota_close_btn) lv_obj_remove_flag(ota_close_btn, LV_OBJ_FLAG_HIDDEN);
    } else if (status == OTA_STATUS_PORTAL_ACTIVE) {
        lv_obj_set_style_border_color(ota_modal_box, lv_color_hex(0xF59E0B), LV_PART_MAIN); // Amber
        if (ota_close_btn) lv_obj_remove_flag(ota_close_btn, LV_OBJ_FLAG_HIDDEN);
    } else {
        // Downloading / connecting
        if (ota_close_btn) lv_obj_add_flag(ota_close_btn, LV_OBJ_FLAG_HIDDEN);
    }
}

static void on_ota_button_clicked(lv_event_t *e) {
    if (lv_event_get_code(e) != LV_EVENT_CLICKED) return;
    audio_play_click();

    // Prevent opening multiple modal dialogs
    if (ota_modal_box != NULL) return;

    lv_obj_t *active_scr = lv_screen_active();

    // 1. Create Modal Dialog Container
    ota_modal_box = lv_obj_create(active_scr);
    theme_apply_card(ota_modal_box);
    lv_obj_set_size(ota_modal_box, 290, 290);
    lv_obj_center(ota_modal_box);
    lv_obj_set_style_border_color(ota_modal_box, lv_color_hex(0x38BDF8), LV_PART_MAIN);
    lv_obj_set_style_border_width(ota_modal_box, 2, LV_PART_MAIN);
    lv_obj_set_style_shadow_width(ota_modal_box, 25, LV_PART_MAIN);
    lv_obj_set_style_shadow_color(ota_modal_box, lv_color_hex(0x000000), LV_PART_MAIN);
    lv_obj_set_style_shadow_opa(ota_modal_box, (lv_opa_t)LV_OPA_70, LV_PART_MAIN);
    lv_obj_remove_flag(ota_modal_box, LV_OBJ_FLAG_SCROLLABLE);

    // 2. Title Label
    lv_obj_t *title_lbl = lv_label_create(ota_modal_box);
    lv_obj_add_style(title_lbl, &style_title_hebrew, LV_PART_MAIN);
    lv_label_set_text(title_lbl, "עדכון קושחה OTA");
    lv_obj_set_width(title_lbl, 250);
    lv_obj_set_style_text_align(title_lbl, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN);
    lv_obj_set_style_text_font(title_lbl, &lv_font_hebrew_24, LV_PART_MAIN);
    lv_obj_align(title_lbl, LV_ALIGN_TOP_MID, 0, 5);

    // 3. Status Message Label
    ota_status_lbl = lv_label_create(ota_modal_box);
    lv_label_set_text(ota_status_lbl, "מתחבר ל-Wi-Fi...");
    lv_obj_set_width(ota_status_lbl, 250);
    lv_label_set_long_mode(ota_status_lbl, LV_LABEL_LONG_WRAP);
    lv_obj_set_style_text_font(ota_status_lbl, &lv_font_hebrew_16, LV_PART_MAIN);
    lv_obj_set_style_text_color(ota_status_lbl, lv_color_hex(0xE2E8F0), LV_PART_MAIN);
    lv_obj_set_style_text_align(ota_status_lbl, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN);
    lv_obj_set_style_base_dir(ota_status_lbl, LV_BASE_DIR_RTL, LV_PART_MAIN);
    lv_obj_align(ota_status_lbl, LV_ALIGN_TOP_MID, 0, 55);

    // 4. Progress Bar
    ota_progress_bar = lv_bar_create(ota_modal_box);
    lv_obj_set_size(ota_progress_bar, 240, 18);
    lv_obj_align(ota_progress_bar, LV_ALIGN_CENTER, 0, 15);
    lv_bar_set_range(ota_progress_bar, 0, 100);
    lv_bar_set_value(ota_progress_bar, 0, LV_ANIM_OFF);
    lv_obj_set_style_bg_color(ota_progress_bar, lv_color_hex(0x1E293B), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(ota_progress_bar, LV_OPA_COVER, LV_PART_MAIN);
    lv_obj_set_style_bg_color(ota_progress_bar, lv_color_hex(0x38BDF8), LV_PART_INDICATOR);
    lv_obj_set_style_bg_grad_color(ota_progress_bar, lv_color_hex(0x0284C7), LV_PART_INDICATOR);
    lv_obj_set_style_bg_grad_dir(ota_progress_bar, LV_GRAD_DIR_HOR, LV_PART_INDICATOR);
    lv_obj_set_style_radius(ota_progress_bar, 9, LV_PART_MAIN);
    lv_obj_set_style_radius(ota_progress_bar, 9, LV_PART_INDICATOR);

    // 5. Percent Label
    ota_pct_lbl = lv_label_create(ota_modal_box);
    lv_label_set_text(ota_pct_lbl, "0%");
    lv_obj_set_style_text_font(ota_pct_lbl, &lv_font_hebrew_16, LV_PART_MAIN);
    lv_obj_set_style_text_color(ota_pct_lbl, lv_color_hex(0x38BDF8), LV_PART_MAIN);
    lv_obj_align(ota_pct_lbl, LV_ALIGN_CENTER, 0, 42);

    // 6. Close / Cancel Button
    ota_close_btn = lv_button_create(ota_modal_box);
    lv_obj_set_size(ota_close_btn, 110, 48);
    lv_obj_align(ota_close_btn, LV_ALIGN_BOTTOM_MID, 0, -5);
    lv_obj_set_style_bg_color(ota_close_btn, lv_color_hex(0x475569), LV_PART_MAIN);
    lv_obj_set_style_radius(ota_close_btn, 12, LV_PART_MAIN);
    lv_obj_add_event_cb(ota_close_btn, on_ota_close_clicked, LV_EVENT_CLICKED, NULL);

    lv_obj_t *close_lbl = lv_label_create(ota_close_btn);
    lv_label_set_text(close_lbl, "סגור");
    lv_obj_set_style_text_font(close_lbl, &lv_font_hebrew_16, LV_PART_MAIN);
    lv_obj_set_style_text_color(close_lbl, lv_color_hex(0xFFFFFF), LV_PART_MAIN);
    lv_obj_center(close_lbl);

    // Initially hide close button while starting
    lv_obj_add_flag(ota_close_btn, LV_OBJ_FLAG_HIDDEN);

    // 7. Start LVGL GUI timer to update progress every 150ms
    ota_update_timer = lv_timer_create(ota_ui_timer_cb, 150, NULL);

    Serial.println("[UI] Launching OTA update sequence...");
    ota_start_async_update(DEFAULT_OTA_FIRMWARE_URL);
}

/* ========================================================================== */
/*                         QUIZ ANSWER VALIDATION & REWARDS                   */
/* ========================================================================== */

static void on_answer_clicked(lv_event_t *e) {
    if (lv_event_get_code(e) != LV_EVENT_CLICKED) return;
    if (!current_quiz_question) return;

    uint8_t clicked_idx = (uint8_t)(uintptr_t)lv_event_get_user_data(e);
    lv_obj_t *clicked_btn = (lv_obj_t *)lv_event_get_target(e);
    bool is_correct = (clicked_idx == current_quiz_question->correct_idx);

    Serial.printf("[QUIZ] Answer clicked: %u (Correct: %u) -> %s\n",
                  clicked_idx, current_quiz_question->correct_idx,
                  is_correct ? "CORRECT" : "WRONG");

    // 1. Play Non-Blocking Audio Feedback
    if (is_correct) {
        audio_play_success();
    } else {
        audio_play_fail();
    }

    // 2. Disable all 4 buttons to prevent double answers
    for (int i = 0; i < 4; i++) {
        if (quiz_answer_btns[i]) {
            lv_obj_remove_flag(quiz_answer_btns[i], LV_OBJ_FLAG_CLICKABLE);
        }
    }

    // 3. Process Reward & Visual feedback on buttons
    if (is_correct) {
        player_data_add_coins(current_profile, 10);
        player_data_add_xp(current_profile, 25);

        Serial.printf("[REWARD] Profile %d rewarded! New Coins: %u 🪙 | New XP: %u ⚡\n",
                      (int)current_profile,
                      player_data_get_coins(current_profile),
                      player_data_get_xp(current_profile));

        // Correct Answer: Radiant Emerald Green (#10B981)
        lv_obj_set_style_bg_color(clicked_btn, lv_color_hex(0x10B981), LV_PART_MAIN);
        lv_obj_set_style_bg_grad_color(clicked_btn, lv_color_hex(0x059669), LV_PART_MAIN);
        lv_obj_set_style_border_color(clicked_btn, lv_color_hex(0x34D399), LV_PART_MAIN);
        lv_obj_set_style_shadow_color(clicked_btn, lv_color_hex(0x10B981), LV_PART_MAIN);
    } else {
        // Wrong Answer: Crimson Red (#EF4444)
        lv_obj_set_style_bg_color(clicked_btn, lv_color_hex(0xEF4444), LV_PART_MAIN);
        lv_obj_set_style_bg_grad_color(clicked_btn, lv_color_hex(0xDC2626), LV_PART_MAIN);
        lv_obj_set_style_border_color(clicked_btn, lv_color_hex(0xF87171), LV_PART_MAIN);
        lv_obj_set_style_shadow_color(clicked_btn, lv_color_hex(0xEF4444), LV_PART_MAIN);

        // Highlight correct answer in Emerald Green
        uint8_t c_idx = current_quiz_question->correct_idx;
        if (c_idx < 4 && quiz_answer_btns[c_idx]) {
            lv_obj_set_style_bg_color(quiz_answer_btns[c_idx], lv_color_hex(0x10B981), LV_PART_MAIN);
            lv_obj_set_style_bg_grad_color(quiz_answer_btns[c_idx], lv_color_hex(0x059669), LV_PART_MAIN);
            lv_obj_set_style_border_color(quiz_answer_btns[c_idx], lv_color_hex(0x34D399), LV_PART_MAIN);
        }
    }

    // 4. Display Feedback Message Box
    lv_obj_t *mbox = lv_msgbox_create(lv_screen_active());
    lv_obj_set_style_base_dir(mbox, LV_BASE_DIR_RTL, LV_PART_MAIN);
    lv_obj_set_style_bg_color(mbox, lv_color_hex(COLOR_BG_CARD), LV_PART_MAIN);
    lv_obj_set_style_border_color(mbox, lv_color_hex(is_correct ? 0x10B981 : 0xEF4444), LV_PART_MAIN);
    lv_obj_set_style_border_width(mbox, 2, LV_PART_MAIN);
    lv_obj_set_style_radius(mbox, 18, LV_PART_MAIN);
    lv_obj_set_size(mbox, 300, 300);
    lv_obj_center(mbox);

    // Title & Feedback Text
    lv_obj_t *result_title = lv_msgbox_add_title(mbox, is_correct ? "נכון מאוד!" : "כמעט!");
    lv_obj_t *result_text = lv_msgbox_add_text(mbox, current_quiz_question->feedback);
    for (lv_obj_t *label : {result_title, result_text}) {
        if (label) {
            lv_obj_set_style_text_font(label, &lv_font_hebrew_16, LV_PART_MAIN);
            lv_obj_set_style_text_color(label, lv_color_hex(0xFFFFFF), LV_PART_MAIN);
            lv_obj_set_style_text_align(label, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN);
            lv_obj_set_style_base_dir(label, LV_BASE_DIR_RTL, LV_PART_MAIN);
        }
    }
    lv_obj_set_width(result_text, 250);
    lv_label_set_long_mode(result_text, LV_LABEL_LONG_WRAP);

    // Continue Button
    lv_obj_t *continue_btn = lv_msgbox_add_footer_button(mbox, "המשך");
    theme_apply_btn_main(continue_btn);
    lv_obj_set_size(continue_btn, 150, 48);
    lv_obj_add_event_cb(continue_btn, on_msgbox_continue_clicked, LV_EVENT_CLICKED, NULL);

    // LVGL's RTL footer can push a single button partly outside the message
    // box. Center the footer explicitly and keep layout direction neutral.
    lv_obj_t *footer = lv_obj_get_parent(continue_btn);
    if (footer) {
        lv_obj_set_width(footer, LV_PCT(100));
        lv_obj_set_style_base_dir(footer, LV_BASE_DIR_LTR, LV_PART_MAIN);
        lv_obj_set_flex_align(footer, LV_FLEX_ALIGN_CENTER,
                             LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    }

    lv_obj_t *btn_lbl = lv_obj_get_child(continue_btn, 0);
    if (btn_lbl) {
        lv_obj_set_style_text_font(btn_lbl, &lv_font_hebrew_24, LV_PART_MAIN);
        lv_obj_set_style_base_dir(btn_lbl, LV_BASE_DIR_RTL, LV_PART_MAIN);
    }
}

/* ========================================================================== */
/*                         SCREEN BUILDER: QUIZ SCREEN                        */
/* ========================================================================== */

void ui_screen_quiz_init(lv_obj_t *scr) {
    const ProfileInfo_t *p = sm_get_profile_info(current_profile);

    // 1. Fetch Question from Static Engine
    current_quiz_question = quiz_get_next_question(current_profile, current_subject_id);

    /* ---------------------------------------------------------------------- */
    /* 2. Top HUD Bar                                                         */
    /* ---------------------------------------------------------------------- */
    lv_obj_t *hud = lv_obj_create(scr);
    lv_obj_set_size(hud, BSP_LCD_H_RES, 50);
    lv_obj_align(hud, LV_ALIGN_TOP_MID, 0, 0);
    lv_obj_set_style_bg_color(hud, lv_color_hex(0x000000), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(hud, (lv_opa_t)LV_OPA_60, LV_PART_MAIN);
    lv_obj_set_style_border_width(hud, 0, LV_PART_MAIN);
    lv_obj_set_style_radius(hud, 0, LV_PART_MAIN);
    lv_obj_set_layout(hud, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(hud, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(hud, LV_FLEX_ALIGN_SPACE_BETWEEN, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_remove_flag(hud, LV_OBJ_FLAG_SCROLLABLE);

    // Child Profile Name + Badge on Right
    lv_obj_t *lbl_profile = lv_label_create(hud);
    char prof_buf[64];
    snprintf(prof_buf, sizeof(prof_buf), "%s %s", p->name_hebrew, p->badge_symbol);
    lv_label_set_text(lbl_profile, prof_buf);
    lv_obj_set_style_text_font(lbl_profile, &lv_font_hebrew_24, LV_PART_MAIN);
    lv_obj_set_style_text_color(lbl_profile, lv_color_hex(p->color_accent), LV_PART_MAIN);
    lv_obj_set_style_base_dir(lbl_profile, LV_BASE_DIR_RTL, LV_PART_MAIN);

    // Back to Dashboard Button on Left
    lv_obj_t *back_btn = lv_button_create(hud);
    lv_obj_set_size(back_btn, 65, 35);
    lv_obj_set_style_bg_color(back_btn, lv_color_hex(0x334155), LV_PART_MAIN);
    lv_obj_set_style_radius(back_btn, 8, LV_PART_MAIN);
    lv_obj_add_event_cb(back_btn, on_back_to_dashboard_clicked, LV_EVENT_CLICKED, NULL);

    lv_obj_t *back_lbl = lv_label_create(back_btn);
    lv_label_set_text(back_lbl, "חזור");
    lv_obj_set_style_text_font(back_lbl, &lv_font_hebrew_24, LV_PART_MAIN);
    lv_obj_set_style_text_color(back_lbl, lv_color_hex(0xFFFFFF), LV_PART_MAIN);
    lv_obj_center(back_lbl);

    /* ---------------------------------------------------------------------- */
    /* 3. Question Prompt Card (Top Half)                                     */
    /* ---------------------------------------------------------------------- */
    lv_obj_t *question_card = lv_obj_create(scr);
    theme_apply_card(question_card);
    lv_obj_set_size(question_card, 290, 140);
    lv_obj_align(question_card, LV_ALIGN_TOP_MID, 0, 60);
    lv_obj_set_style_border_color(question_card, lv_color_hex(p->color_accent), LV_PART_MAIN);
    lv_obj_set_style_border_width(question_card, 2, LV_PART_MAIN);
    lv_obj_remove_flag(question_card, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t *q_label = lv_label_create(question_card);
    lv_label_set_text(q_label, current_quiz_question ? current_quiz_question->text : "טוען שאלה...");
    lv_obj_set_width(q_label, 250);
    lv_label_set_long_mode(q_label, LV_LABEL_LONG_WRAP);
    lv_obj_set_style_text_font(q_label, &lv_font_hebrew_16, LV_PART_MAIN);
    lv_obj_set_style_text_color(q_label, lv_color_hex(0xFFFFFF), LV_PART_MAIN);
    lv_obj_set_style_text_align(q_label, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN);
    lv_obj_set_style_base_dir(q_label, LV_BASE_DIR_RTL, LV_PART_MAIN);
    lv_obj_center(q_label);

    /* ---------------------------------------------------------------------- */
    /* 4. 2x2 Grid for 4 Answer Options (Bottom Half)                         */
    /* ---------------------------------------------------------------------- */
    lv_obj_t *answers_grid = lv_obj_create(scr);
    lv_obj_set_size(answers_grid, 300, 240);
    lv_obj_align(answers_grid, LV_ALIGN_BOTTOM_MID, 0, -20);
    lv_obj_set_style_bg_opa(answers_grid, LV_OPA_TRANSP, LV_PART_MAIN);
    lv_obj_set_style_border_width(answers_grid, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_all(answers_grid, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_row(answers_grid, 12, LV_PART_MAIN);
    lv_obj_set_style_pad_column(answers_grid, 12, LV_PART_MAIN);
    lv_obj_set_layout(answers_grid, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(answers_grid, LV_FLEX_FLOW_ROW_WRAP);
    lv_obj_set_flex_align(answers_grid, LV_FLEX_ALIGN_SPACE_BETWEEN, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_SPACE_BETWEEN);
    lv_obj_remove_flag(answers_grid, LV_OBJ_FLAG_SCROLLABLE);

    // 5. Generate the 4 Answer Buttons
    for (int i = 0; i < 4; i++) {
        lv_obj_t *btn = lv_button_create(answers_grid);
        theme_apply_btn_main(btn);
        lv_obj_set_size(btn, 138, 95);
        lv_obj_set_style_radius(btn, 16, LV_PART_MAIN);
        lv_obj_add_event_cb(btn, on_answer_clicked, LV_EVENT_CLICKED, (void *)(uintptr_t)i);

        lv_obj_t *btn_lbl = lv_label_create(btn);
        lv_label_set_text(btn_lbl, current_quiz_question ? current_quiz_question->answers[i] : "");
        lv_obj_set_width(btn_lbl, 116);
        lv_label_set_long_mode(btn_lbl, LV_LABEL_LONG_WRAP);
        lv_obj_set_style_text_font(btn_lbl, &lv_font_hebrew_16, LV_PART_MAIN);
        lv_obj_set_style_text_color(btn_lbl, lv_color_hex(0xFFFFFF), LV_PART_MAIN);
        lv_obj_set_style_text_align(btn_lbl, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN);
        lv_obj_set_style_base_dir(btn_lbl, LV_BASE_DIR_RTL, LV_PART_MAIN);
        lv_obj_center(btn_lbl);

        quiz_answer_btns[i] = btn;
    }
}

/* ========================================================================== */
/*                         SCREEN BUILDER: PROFILES                           */
/* ========================================================================== */

void ui_screen_profiles_init(lv_obj_t *scr) {
    // 1. Screen Title: "מי הקוסם שמשחק עכשיו?"
    lv_obj_t *title_label = lv_label_create(scr);
    lv_obj_add_style(title_label, &style_title_hebrew, LV_PART_MAIN);
    lv_label_set_text(title_label, "מי הקוסם שמשחק עכשיו?");
    lv_obj_set_width(title_label, 300);
    lv_obj_set_style_text_align(title_label, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN);
    lv_obj_align(title_label, LV_ALIGN_TOP_MID, 0, 35);

    // 2. Subtitle: "בחר את הפרופיל שלך כדי להתחיל"
    lv_obj_t *sub_label = lv_label_create(scr);
    lv_obj_add_style(sub_label, &style_subtitle_hebrew, LV_PART_MAIN);
    lv_label_set_text(sub_label, "בחר את הפרופיל שלך כדי להתחיל");
    lv_obj_set_width(sub_label, 300);
    lv_obj_set_style_text_align(sub_label, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN);
    lv_obj_align(sub_label, LV_ALIGN_TOP_MID, 0, 72);

    // 3. Centered Vertical Flex Container for Child Cards
    lv_obj_t *cont = lv_obj_create(scr);
    lv_obj_set_size(cont, 290, 340);
    lv_obj_align(cont, LV_ALIGN_TOP_MID, 0, 105);
    
    lv_obj_set_style_bg_opa(cont, LV_OPA_TRANSP, LV_PART_MAIN);
    lv_obj_set_style_border_width(cont, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_all(cont, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_row(cont, 14, LV_PART_MAIN);
    lv_obj_set_layout(cont, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(cont, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(cont, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_remove_flag(cont, LV_OBJ_FLAG_SCROLLABLE);

    // 4. Generate one card for each child profile.
    for (size_t i = 0; i < sizeof(PROFILES_DATA) / sizeof(PROFILES_DATA[0]); i++) {
        const ProfileInfo_t *p = &PROFILES_DATA[i];

        lv_obj_t *btn = lv_button_create(cont);
        lv_obj_set_size(btn, 280, 85);
        lv_obj_set_style_radius(btn, 18, LV_PART_MAIN);
        lv_obj_set_style_bg_color(btn, lv_color_hex(COLOR_BG_CARD), LV_PART_MAIN);
        lv_obj_set_style_bg_opa(btn, LV_OPA_COVER, LV_PART_MAIN);

        // Radiant accent border per profile color
        lv_obj_set_style_border_width(btn, 2, LV_PART_MAIN);
        lv_obj_set_style_border_color(btn, lv_color_hex(p->color_accent), LV_PART_MAIN);
        lv_obj_set_style_border_opa(btn, (lv_opa_t)LV_OPA_80, LV_PART_MAIN);

        // 3D Shadow with profile glow
        lv_obj_set_style_shadow_width(btn, 14, LV_PART_MAIN);
        lv_obj_set_style_shadow_ofs_y(btn, 4, LV_PART_MAIN);
        lv_obj_set_style_shadow_color(btn, lv_color_hex(p->color_accent), LV_PART_MAIN);
        lv_obj_set_style_shadow_opa(btn, (lv_opa_t)LV_OPA_30, LV_PART_MAIN);

        // Pressed tactile animation
        lv_obj_set_style_translate_y(btn, 3, LV_STATE_PRESSED);
        lv_obj_set_style_transform_scale_x(btn, 252, LV_STATE_PRESSED);
        lv_obj_set_style_transform_scale_y(btn, 252, LV_STATE_PRESSED);
        lv_obj_set_style_shadow_ofs_y(btn, 1, LV_STATE_PRESSED);
        lv_obj_set_style_shadow_width(btn, 6, LV_STATE_PRESSED);
        lv_obj_set_style_bg_color(btn, lv_color_hex(0x0F172A), LV_STATE_PRESSED);

        lv_obj_set_style_base_dir(btn, LV_BASE_DIR_RTL, LV_PART_MAIN);
        lv_obj_add_event_cb(btn, on_profile_selected, LV_EVENT_CLICKED, (void *)(uintptr_t)p->id);

        // Avatar / Badge Icon
        lv_obj_t *badge_box = lv_obj_create(btn);
        lv_obj_set_size(badge_box, 48, 48);
        lv_obj_set_style_radius(badge_box, 12, LV_PART_MAIN);
        lv_obj_set_style_bg_color(badge_box, lv_color_hex(p->color_accent), LV_PART_MAIN);
        lv_obj_set_style_bg_opa(badge_box, (lv_opa_t)LV_OPA_30, LV_PART_MAIN);
        lv_obj_set_style_border_width(badge_box, 1, LV_PART_MAIN);
        lv_obj_set_style_border_color(badge_box, lv_color_hex(p->color_accent), LV_PART_MAIN);
        lv_obj_align(badge_box, LV_ALIGN_RIGHT_MID, -6, 0);
        lv_obj_remove_flag(badge_box, (lv_obj_flag_t)(LV_OBJ_FLAG_SCROLLABLE | LV_OBJ_FLAG_CLICKABLE));

        lv_obj_t *badge_txt = lv_label_create(badge_box);
        lv_label_set_text(badge_txt, p->badge_symbol);
        lv_obj_set_style_text_font(badge_txt, &lv_font_hebrew_24, LV_PART_MAIN);
        lv_obj_set_style_text_color(badge_txt, lv_color_hex(0xFFFFFF), LV_PART_MAIN);
        lv_obj_center(badge_txt);

        // Child name only; ages are intentionally not displayed.
        lv_obj_t *name_lbl = lv_label_create(btn);
        lv_label_set_text(name_lbl, p->name_hebrew);
        lv_obj_set_style_text_font(name_lbl, &lv_font_hebrew_24, LV_PART_MAIN);
        lv_obj_set_style_text_color(name_lbl, lv_color_hex(0xFFFFFF), LV_PART_MAIN);
        lv_obj_set_style_base_dir(name_lbl, LV_BASE_DIR_RTL, LV_PART_MAIN);
        lv_obj_align(name_lbl, LV_ALIGN_RIGHT_MID, -64, -12);

        // Child Subtitle / Title Label: e.g. "קוסם בכיר"
        lv_obj_t *role_lbl = lv_label_create(btn);
        lv_label_set_text(role_lbl, p->title_hebrew);
        lv_obj_set_style_text_font(role_lbl, &lv_font_hebrew_24, LV_PART_MAIN);
        lv_obj_set_style_text_color(role_lbl, lv_color_hex(p->color_accent), LV_PART_MAIN);
        lv_obj_set_style_base_dir(role_lbl, LV_BASE_DIR_RTL, LV_PART_MAIN);
        lv_obj_align(role_lbl, LV_ALIGN_RIGHT_MID, -64, 14);
    }
}

/* ========================================================================== */
/*                         SCREEN BUILDER: DASHBOARD                          */
/* ========================================================================== */

void ui_screen_dashboard_init(lv_obj_t *scr) {
    const ProfileInfo_t *p = sm_get_profile_info(current_profile);

    // Fetch actual persistent values from NVS
    uint32_t current_coins = player_data_get_coins(current_profile);
    uint32_t current_xp = player_data_get_xp(current_profile);

    /* ---------------------------------------------------------------------- */
    /* 1. HUD (Heads-Up Display) Top Bar                                      */
    /* ---------------------------------------------------------------------- */
    lv_obj_t *hud = lv_obj_create(scr);
    lv_obj_set_size(hud, BSP_LCD_H_RES, 50);
    lv_obj_align(hud, LV_ALIGN_TOP_MID, 0, 0);
    lv_obj_set_style_bg_color(hud, lv_color_hex(0x000000), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(hud, (lv_opa_t)LV_OPA_60, LV_PART_MAIN);
    lv_obj_set_style_border_width(hud, 0, LV_PART_MAIN);
    lv_obj_set_style_radius(hud, 0, LV_PART_MAIN);
    
    lv_obj_set_layout(hud, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(hud, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(hud, LV_FLEX_ALIGN_SPACE_BETWEEN, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_remove_flag(hud, LV_OBJ_FLAG_SCROLLABLE);

    // Energy
    lv_obj_t *lbl_energy = lv_label_create(hud);
    lv_obj_add_style(lbl_energy, &style_subtitle_hebrew, LV_PART_MAIN);
    lv_label_set_text(lbl_energy, "5/5");
    lv_obj_set_style_text_font(lbl_energy, &lv_font_hebrew_16, LV_PART_MAIN);

    // Coins 🪙 (Dynamically loaded from NVS)
    lv_obj_t *lbl_coins = lv_label_create(hud);
    lv_obj_add_style(lbl_coins, &style_subtitle_hebrew, LV_PART_MAIN);
    char coins_buf[32];
    snprintf(coins_buf, sizeof(coins_buf), "%u מטבעות", (unsigned int)current_coins);
    lv_label_set_text(lbl_coins, coins_buf);
    lv_obj_set_style_text_font(lbl_coins, &lv_font_hebrew_16, LV_PART_MAIN);

    // XP (Dynamically loaded from NVS)
    lv_obj_t *lbl_xp = lv_label_create(hud);
    lv_obj_add_style(lbl_xp, &style_subtitle_hebrew, LV_PART_MAIN);
    char xp_buf[32];
    snprintf(xp_buf, sizeof(xp_buf), "%u XP", (unsigned int)current_xp);
    lv_label_set_text(lbl_xp, xp_buf);
    lv_obj_set_style_text_font(lbl_xp, &lv_font_hebrew_16, LV_PART_MAIN);
    
    #if BSP_OTA_ENABLED
    // OTA button is exposed only when a trusted endpoint is provisioned.
    lv_obj_t *ota_btn = lv_button_create(hud);
    lv_obj_set_size(ota_btn, 42, 35);
    lv_obj_set_style_bg_color(ota_btn, lv_color_hex(0x0284C7), LV_PART_MAIN); // Sky Blue
    lv_obj_set_style_radius(ota_btn, 8, LV_PART_MAIN);
    lv_obj_add_event_cb(ota_btn, on_ota_button_clicked, LV_EVENT_CLICKED, NULL);
    lv_obj_t *ota_lbl = lv_label_create(ota_btn);
    lv_label_set_text(ota_lbl, "OTA");
    lv_obj_set_style_text_font(ota_lbl, &lv_font_hebrew_16, LV_PART_MAIN);
    lv_obj_center(ota_lbl);
    #endif

    /* Back to Profiles Button in HUD */
    lv_obj_t *back_btn = lv_button_create(hud);
    lv_obj_set_size(back_btn, 50, 35);
    lv_obj_set_style_bg_color(back_btn, lv_color_hex(0xEF4444), LV_PART_MAIN); // Red
    lv_obj_set_style_radius(back_btn, 8, LV_PART_MAIN);
    lv_obj_add_event_cb(back_btn, on_back_to_profiles_clicked, LV_EVENT_CLICKED, NULL);
    lv_obj_t *back_lbl = lv_label_create(back_btn);
    lv_label_set_text(back_lbl, "החלף");
    lv_obj_set_style_text_font(back_lbl, &lv_font_hebrew_16, LV_PART_MAIN);
    lv_obj_center(back_lbl);

    /* ---------------------------------------------------------------------- */
    /* 2. Scrollable Flex Container for Worlds                                */
    /* ---------------------------------------------------------------------- */
    lv_obj_t *scroll = lv_obj_create(scr);
    lv_obj_set_size(scroll, BSP_LCD_H_RES, BSP_LCD_V_RES - 50);
    lv_obj_align(scroll, LV_ALIGN_TOP_MID, 0, 50);
    lv_obj_set_style_bg_opa(scroll, LV_OPA_TRANSP, LV_PART_MAIN);
    lv_obj_set_style_border_width(scroll, 0, LV_PART_MAIN);
    
    lv_obj_set_layout(scroll, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(scroll, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(scroll, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

    /* Generate Subjects Based on Profile */
    const char* titles[4];
    const char* emojis[4];
    int num_subjects = 0;

    titles[0] = "חשבון"; emojis[0] = "M";
    titles[1] = "עברית"; emojis[1] = "H";
    titles[2] = "אנגלית"; emojis[2] = "EN";
    titles[3] = "יהדות"; emojis[3] = "J";
    num_subjects = 4;

    for (int i = 0; i < num_subjects; i++) {
        lv_obj_t *card = lv_obj_create(scroll);
        theme_apply_card(card);
        lv_obj_set_size(card, 290, 110);
        lv_obj_set_style_border_color(card, lv_color_hex(p->color_accent), LV_PART_MAIN);
        lv_obj_remove_flag(card, LV_OBJ_FLAG_SCROLLABLE);

        // Emoji
        lv_obj_t *emoji_lbl = lv_label_create(card);
        lv_label_set_text(emoji_lbl, emojis[i]);
        lv_obj_set_style_text_font(emoji_lbl, &lv_font_hebrew_24, LV_PART_MAIN);
        lv_obj_align(emoji_lbl, LV_ALIGN_TOP_LEFT, 0, 0);

        // Title
        lv_obj_t *title_lbl = lv_label_create(card);
        char title_buf[64];
        snprintf(title_buf, sizeof(title_buf), "%s - %u שאלות", titles[i],
                 (unsigned)quiz_get_question_count(current_profile, i));
        lv_label_set_text(title_lbl, title_buf);
        lv_obj_set_style_text_font(title_lbl, &lv_font_hebrew_16, LV_PART_MAIN);
        lv_obj_set_style_text_color(title_lbl, lv_color_hex(0xFFFFFF), LV_PART_MAIN);
        lv_obj_set_style_base_dir(title_lbl, LV_BASE_DIR_RTL, LV_PART_MAIN);
        lv_obj_align(title_lbl, LV_ALIGN_TOP_RIGHT, 0, 0);

        // Play Button
        lv_obj_t *play_btn = lv_button_create(card);
        theme_apply_btn_main(play_btn);
        lv_obj_set_size(play_btn, 120, 45);
        lv_obj_align(play_btn, LV_ALIGN_BOTTOM_MID, 0, 0);
        lv_obj_add_event_cb(play_btn, on_play_subject_clicked, LV_EVENT_CLICKED, (void*)(uintptr_t)i);

        lv_obj_t *play_lbl = lv_label_create(play_btn);
        lv_label_set_text(play_lbl, "שחק");
        lv_obj_set_style_text_font(play_lbl, &lv_font_hebrew_24, LV_PART_MAIN);
        lv_obj_center(play_lbl);
    }
}

/* ========================================================================== */
/*                         SCREEN BUILDER: SPLASH                             */
/* ========================================================================== */

void ui_screen_splash_init(lv_obj_t *scr) {
    lv_obj_t *title_label = lv_label_create(scr);
    lv_obj_add_style(title_label, &style_title_hebrew, LV_PART_MAIN);
    lv_label_set_text(title_label, "אקדמיית הקוסמים");
    lv_obj_align(title_label, LV_ALIGN_TOP_MID, 0, 55);

    lv_obj_t *sub_label = lv_label_create(scr);
    lv_obj_add_style(sub_label, &style_subtitle_hebrew, LV_PART_MAIN);
    lv_label_set_text(sub_label, "עולם הכישוף וההרפתקאות");
    lv_obj_align(sub_label, LV_ALIGN_TOP_MID, 0, 95);

    lv_obj_t *start_btn = lv_button_create(scr);
    theme_apply_btn_main(start_btn);
    lv_obj_set_size(start_btn, 240, 64);
    lv_obj_align(start_btn, LV_ALIGN_CENTER, 0, 60);
    lv_obj_add_event_cb(start_btn, on_splash_start_clicked, LV_EVENT_CLICKED, NULL);

    lv_obj_t *btn_label = lv_label_create(start_btn);
    lv_label_set_text(btn_label, "התחל הרפתקה");
    lv_obj_set_style_text_font(btn_label, &lv_font_hebrew_24, LV_PART_MAIN);
    lv_obj_set_style_text_color(btn_label, lv_color_hex(0xFFFFFF), LV_PART_MAIN);
    lv_obj_set_style_base_dir(btn_label, LV_BASE_DIR_RTL, LV_PART_MAIN);
    lv_obj_center(btn_label);
}

/* ========================================================================== */
/*                         DYNAMIC SCREEN LIFECYCLE                           */
/* ========================================================================== */

void sm_load_screen(ScreenID_t screen_id) {
    Serial.printf("[SM] Loading screen ID: %d...\n", (int)screen_id);

    // Clean up active OTA timer/modal references if leaving the dashboard
    if (ota_update_timer) {
        lv_timer_delete(ota_update_timer);
        ota_update_timer = NULL;
    }
    ota_modal_box = NULL;
    ota_status_lbl = NULL;
    ota_pct_lbl = NULL;
    ota_progress_bar = NULL;
    ota_close_btn = NULL;

    // 1. Create fresh screen object in PSRAM
    lv_obj_t *new_scr = lv_obj_create(NULL);
    if (!new_scr) {
        Serial.println("[SM] ERROR: Failed to allocate new screen object!");
        return;
    }

    // 2. Apply theme background and RTL direction
    lv_obj_add_style(new_scr, &style_screen_bg, LV_PART_MAIN);
    lv_obj_set_style_base_dir(new_scr, LV_BASE_DIR_RTL, LV_PART_MAIN);

    // 3. Build UI tree onto the new screen
    switch (screen_id) {
        case SCREEN_SPLASH:
            ui_screen_splash_init(new_scr);
            break;
        case SCREEN_PROFILES:
            ui_screen_profiles_init(new_scr);
            break;
        case SCREEN_DASHBOARD:
            ui_screen_dashboard_init(new_scr);
            break;
        case SCREEN_QUIZ:
            ui_screen_quiz_init(new_scr);
            break;
        default:
            Serial.printf("[SM] WARN: Unknown screen ID %d, loading profiles.\n", (int)screen_id);
            ui_screen_profiles_init(new_scr);
            break;
    }

    // 4. Smooth fade transition with automatic cleanup of previous screen from PSRAM
    lv_screen_load_anim(new_scr, LV_SCR_LOAD_ANIM_FADE_ON, 150, 0, true);

    current_screen_id = screen_id;

    Serial.printf("[SM] Screen transition complete. Free PSRAM: %u KB | Free SRAM: %u KB\n",
                  (unsigned int)(heap_caps_get_free_size(MALLOC_CAP_SPIRAM) / 1024),
                  (unsigned int)(heap_caps_get_free_size(MALLOC_CAP_INTERNAL) / 1024));
}
