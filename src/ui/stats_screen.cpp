#include "stats_screen.h"

#include "audio_manager.h"
#include "bsp_config.h"
#include "screen_manager.h"
#include "theme_manager.h"
#include "weekly_stats_store.h"

#include <Arduino.h>
#include <stdio.h>

namespace {

static lv_timer_t *s_stats_timer = nullptr;
static lv_obj_t *s_launcher = nullptr;
static lv_obj_t *s_overlay = nullptr;

static const lv_point_precise_t STAR_UP[] = {
    {3, 16}, {11, 2}, {19, 16}, {3, 16}
};
static const lv_point_precise_t STAR_DOWN[] = {
    {3, 7}, {11, 21}, {19, 7}, {3, 7}
};

static void clear_obj_style(lv_obj_t *obj) {
    lv_obj_set_style_bg_opa(obj, LV_OPA_TRANSP, LV_PART_MAIN);
    lv_obj_set_style_border_width(obj, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_all(obj, 0, LV_PART_MAIN);
    lv_obj_set_style_shadow_width(obj, 0, LV_PART_MAIN);
}

static void make_star(lv_obj_t *parent, int x, int y, int size) {
    lv_obj_t *box = lv_obj_create(parent);
    lv_obj_set_pos(box, x, y);
    lv_obj_set_size(box, size, size);
    clear_obj_style(box);
    lv_obj_remove_flag(box, (lv_obj_flag_t)(LV_OBJ_FLAG_SCROLLABLE | LV_OBJ_FLAG_CLICKABLE));

    lv_obj_t *up = lv_line_create(box);
    lv_line_set_points(up, STAR_UP, sizeof(STAR_UP) / sizeof(STAR_UP[0]));
    lv_obj_set_style_line_color(up, lv_color_hex(0xF8FAFC), LV_PART_MAIN);
    lv_obj_set_style_line_width(up, 2, LV_PART_MAIN);
    lv_obj_center(up);

    lv_obj_t *down = lv_line_create(box);
    lv_line_set_points(down, STAR_DOWN, sizeof(STAR_DOWN) / sizeof(STAR_DOWN[0]));
    lv_obj_set_style_line_color(down, lv_color_hex(0xF8FAFC), LV_PART_MAIN);
    lv_obj_set_style_line_width(down, 2, LV_PART_MAIN);
    lv_obj_center(down);
}

static void format_date(char *buf, size_t len, uint32_t date) {
    if (!buf || len == 0) return;
    if (date == 0) {
        snprintf(buf, len, "--/--");
        return;
    }
    const unsigned month = (unsigned)((date / 100u) % 100u);
    const unsigned day = (unsigned)(date % 100u);
    snprintf(buf, len, "%u/%u", day, month);
}

static void create_text_value(lv_obj_t *parent, int x, int y, int width,
                              const char *symbol, uint16_t value,
                              const lv_font_t *font) {
    lv_obj_t *label = lv_label_create(parent);
    char buf[32];
    snprintf(buf, sizeof(buf), "%s %u", symbol, (unsigned)value);
    lv_label_set_text(label, buf);
    lv_obj_set_width(label, width);
    lv_obj_set_pos(label, x, y);
    lv_obj_set_style_text_font(label, font, LV_PART_MAIN);
    lv_obj_set_style_text_color(label, lv_color_hex(0xF8FAFC), LV_PART_MAIN);
    lv_obj_set_style_text_align(label, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN);
    lv_obj_set_style_base_dir(label, LV_BASE_DIR_LTR, LV_PART_MAIN);
}

static void create_star_value(lv_obj_t *parent, int x, int y, int width,
                              uint16_t value, bool large) {
    const int icon_size = large ? 25 : 23;
    make_star(parent, x + 2, y - (large ? 4 : 3), icon_size);

    lv_obj_t *count = lv_label_create(parent);
    char buf[12];
    snprintf(buf, sizeof(buf), "%u", (unsigned)value);
    lv_label_set_text(count, buf);
    lv_obj_set_pos(count, x + icon_size + 2, y);
    lv_obj_set_width(count, width - icon_size - 4);
    lv_obj_set_style_text_font(count, &lv_font_hebrew_16, LV_PART_MAIN);
    lv_obj_set_style_text_color(count, lv_color_hex(0xF8FAFC), LV_PART_MAIN);
    lv_obj_set_style_text_align(count, LV_TEXT_ALIGN_LEFT, LV_PART_MAIN);
    lv_obj_set_style_base_dir(count, LV_BASE_DIR_LTR, LV_PART_MAIN);
}

static void create_subject_values(lv_obj_t *parent, const WeeklyStatsDay_t &day,
                                  int y, bool large) {
    // Visual order (right-to-left after the date): ×÷, א, ABC, Star of David.
    // All subjects intentionally use the same neutral color.
    const lv_font_t *font = &lv_font_hebrew_16;
    create_star_value(parent, 5, y, 58, day.correct[3], large);
    create_text_value(parent, 65, y, 66, "ABC", day.correct[2], font);
    create_text_value(parent, 133, y, 52, "א", day.correct[1], font);
    create_text_value(parent, 187, y, 72, "×÷", day.correct[0], font);
}

static lv_obj_t *create_day_card(lv_obj_t *parent, int y, int height,
                                 const WeeklyStatsDay_t &day,
                                 const char *prefix, bool large) {
    lv_obj_t *card = lv_obj_create(parent);
    lv_obj_set_size(card, 294, height);
    lv_obj_set_pos(card, 13, y);
    lv_obj_set_style_bg_color(card, lv_color_hex(0x172554), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(card, LV_OPA_COVER, LV_PART_MAIN);
    lv_obj_set_style_border_color(card, lv_color_hex(0x475569), LV_PART_MAIN);
    lv_obj_set_style_border_width(card, 1, LV_PART_MAIN);
    lv_obj_set_style_radius(card, large ? 14 : 10, LV_PART_MAIN);
    lv_obj_set_style_pad_all(card, 6, LV_PART_MAIN);
    lv_obj_remove_flag(card, LV_OBJ_FLAG_SCROLLABLE);

    char date_buf[16];
    format_date(date_buf, sizeof(date_buf), day.date);

    lv_obj_t *date_lbl = lv_label_create(card);
    char title_buf[48];
    if (prefix && *prefix) snprintf(title_buf, sizeof(title_buf), "%s %s", prefix, date_buf);
    else snprintf(title_buf, sizeof(title_buf), "%s", date_buf);
    lv_label_set_text(date_lbl, title_buf);
    lv_obj_set_style_text_font(date_lbl, large ? &lv_font_hebrew_24 : &lv_font_hebrew_16, LV_PART_MAIN);
    lv_obj_set_style_text_color(date_lbl, lv_color_hex(0xF8FAFC), LV_PART_MAIN);
    lv_obj_set_style_base_dir(date_lbl, LV_BASE_DIR_RTL, LV_PART_MAIN);
    lv_obj_align(date_lbl, large ? LV_ALIGN_TOP_RIGHT : LV_ALIGN_RIGHT_MID,
                 large ? -2 : -5, large ? 0 : 0);

    if (large) {
        create_subject_values(card, day, 36, true);
    } else {
        // Reserve the rightmost 67 px for the date and keep all four symbols visible.
        create_star_value(card, 2, 7, 47, day.correct[3], false);
        create_text_value(card, 48, 8, 58, "ABC", day.correct[2], &lv_font_hebrew_16);
        create_text_value(card, 106, 8, 42, "א", day.correct[1], &lv_font_hebrew_16);
        create_text_value(card, 148, 8, 65, "×÷", day.correct[0], &lv_font_hebrew_16);
    }
    return card;
}

static void close_stats(lv_event_t *e) {
    if (lv_event_get_code(e) != LV_EVENT_CLICKED) return;
    audio_play_click();
    if (s_overlay && lv_obj_is_valid(s_overlay)) {
        lv_obj_delete(s_overlay);
    }
    s_overlay = nullptr;
}

static void open_stats(lv_event_t *e) {
    if (lv_event_get_code(e) != LV_EVENT_CLICKED) return;
    if (current_profile == PROFILE_NONE) return;
    audio_play_click();

    if (s_overlay && lv_obj_is_valid(s_overlay)) return;

    WeeklyStatsDay_t days[WEEKLY_STATS_DAYS]{};
    const bool has_date = weekly_stats_store_get_last_seven(current_profile, days);

    lv_obj_t *root = lv_screen_active();
    s_overlay = lv_obj_create(root);
    lv_obj_set_size(s_overlay, BSP_LCD_H_RES, BSP_LCD_V_RES);
    lv_obj_set_pos(s_overlay, 0, 0);
    lv_obj_set_style_bg_color(s_overlay, lv_color_hex(0x07111F), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(s_overlay, LV_OPA_COVER, LV_PART_MAIN);
    lv_obj_set_style_border_width(s_overlay, 0, LV_PART_MAIN);
    lv_obj_set_style_radius(s_overlay, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_all(s_overlay, 0, LV_PART_MAIN);
    lv_obj_remove_flag(s_overlay, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t *title = lv_label_create(s_overlay);
    lv_label_set_text(title, "ההתקדמות שלי");
    lv_obj_set_style_text_font(title, &lv_font_hebrew_24, LV_PART_MAIN);
    lv_obj_set_style_text_color(title, lv_color_hex(0xF8FAFC), LV_PART_MAIN);
    lv_obj_set_style_base_dir(title, LV_BASE_DIR_RTL, LV_PART_MAIN);
    lv_obj_align(title, LV_ALIGN_TOP_RIGHT, -14, 12);

    lv_obj_t *back = lv_button_create(s_overlay);
    lv_obj_set_size(back, 62, 36);
    lv_obj_align(back, LV_ALIGN_TOP_LEFT, 10, 8);
    lv_obj_set_style_bg_color(back, lv_color_hex(0x334155), LV_PART_MAIN);
    lv_obj_set_style_radius(back, 9, LV_PART_MAIN);
    lv_obj_add_event_cb(back, close_stats, LV_EVENT_CLICKED, nullptr);
    lv_obj_t *back_lbl = lv_label_create(back);
    lv_label_set_text(back_lbl, "חזור");
    lv_obj_set_style_text_font(back_lbl, &lv_font_hebrew_16, LV_PART_MAIN);
    lv_obj_set_style_text_color(back_lbl, lv_color_hex(0xFFFFFF), LV_PART_MAIN);
    lv_obj_center(back_lbl);

    if (!has_date) {
        lv_obj_t *empty = lv_label_create(s_overlay);
        lv_label_set_text(empty, "הסטטיסטיקה תופיע כאן אחרי סנכרון השעה והתרגול הראשון");
        lv_obj_set_width(empty, 280);
        lv_label_set_long_mode(empty, LV_LABEL_LONG_WRAP);
        lv_obj_set_style_text_font(empty, &lv_font_hebrew_16, LV_PART_MAIN);
        lv_obj_set_style_text_color(empty, lv_color_hex(0xCBD5E1), LV_PART_MAIN);
        lv_obj_set_style_text_align(empty, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN);
        lv_obj_set_style_base_dir(empty, LV_BASE_DIR_RTL, LV_PART_MAIN);
        lv_obj_center(empty);
        lv_obj_move_foreground(s_overlay);
        return;
    }

    create_day_card(s_overlay, 54, 76, days[0], "היום", true);
    create_day_card(s_overlay, 136, 76, days[1], "אתמול", true);

    const int compact_y = 220;
    const int compact_h = 40;
    const int compact_gap = 5;
    for (int i = 2; i < WEEKLY_STATS_DAYS; ++i) {
        create_day_card(s_overlay,
                        compact_y + (i - 2) * (compact_h + compact_gap),
                        compact_h, days[i], "", false);
    }

    lv_obj_move_foreground(s_overlay);
}

static void create_launcher(lv_obj_t *root) {
    s_launcher = lv_button_create(root);
    lv_obj_set_size(s_launcher, 34, 56);
    lv_obj_align(s_launcher, LV_ALIGN_LEFT_MID, 3, 0);
    lv_obj_set_style_bg_color(s_launcher, lv_color_hex(0x1E293B), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(s_launcher, (lv_opa_t)LV_OPA_90, LV_PART_MAIN);
    lv_obj_set_style_border_color(s_launcher, lv_color_hex(0x64748B), LV_PART_MAIN);
    lv_obj_set_style_border_width(s_launcher, 1, LV_PART_MAIN);
    lv_obj_set_style_radius(s_launcher, 10, LV_PART_MAIN);
    lv_obj_set_style_pad_all(s_launcher, 0, LV_PART_MAIN);
    lv_obj_add_event_cb(s_launcher, open_stats, LV_EVENT_CLICKED, nullptr);

    for (int i = 0; i < 3; ++i) {
        lv_obj_t *dot = lv_obj_create(s_launcher);
        lv_obj_set_size(dot, 5, 5);
        lv_obj_set_style_radius(dot, LV_RADIUS_CIRCLE, LV_PART_MAIN);
        lv_obj_set_style_bg_color(dot, lv_color_hex(0xF8FAFC), LV_PART_MAIN);
        lv_obj_set_style_bg_opa(dot, LV_OPA_COVER, LV_PART_MAIN);
        lv_obj_set_style_border_width(dot, 0, LV_PART_MAIN);
        lv_obj_set_pos(dot, 14, 14 + i * 12);
        lv_obj_remove_flag(dot, (lv_obj_flag_t)(LV_OBJ_FLAG_SCROLLABLE | LV_OBJ_FLAG_CLICKABLE));
    }
    lv_obj_move_foreground(s_launcher);
}

static void stats_timer_cb(lv_timer_t *timer) {
    (void)timer;

    // Capture only when counters changed; the store itself suppresses redundant NVS writes.
    if (current_profile != PROFILE_NONE) {
        weekly_stats_store_capture_profile(current_profile);
    }

    if (sm_get_current_screen() != SCREEN_DASHBOARD) {
        s_launcher = nullptr;
        s_overlay = nullptr;
        return;
    }

    if (s_launcher && lv_obj_is_valid(s_launcher)) return;
    create_launcher(lv_screen_active());
}

} // namespace

void stats_ui_init(void) {
    if (s_stats_timer) return;
    s_stats_timer = lv_timer_create(stats_timer_cb, 2000, nullptr);
    // Attach immediately instead of waiting two seconds after boot.
    stats_timer_cb(s_stats_timer);
    Serial.println("[STATS] Weekly statistics UI initialized.");
}
