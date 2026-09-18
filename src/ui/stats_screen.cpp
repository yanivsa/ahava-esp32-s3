#include "stats_screen.h"

#include "audio_manager.h"
#include "bsp_config.h"
#include "hal_lvgl.h"
#include "screen_manager.h"
#include "subjects.h"
#include "stats_dashboard_contract.h"
#include "theme_manager.h"
#include "weekly_stats_store.h"

#include <Arduino.h>
#include <stdio.h>

namespace {

static lv_timer_t *s_stats_timer = nullptr;
static lv_obj_t *s_stats_card = nullptr;
static lv_obj_t *s_overlay = nullptr;
static bool s_stats_landscape = false;

static constexpr int STATS_SCREEN_W = 480;
static constexpr int STATS_SCREEN_H = 320;
static constexpr int STATS_MARGIN_X = 12;
static constexpr int STATS_HEADER_H = 48;
static constexpr int STATS_CONTENT_W = STATS_SCREEN_W - (STATS_MARGIN_X * 2);
static constexpr int STATS_CONTENT_H = STATS_SCREEN_H - STATS_HEADER_H - 8;

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
    lv_obj_set_style_line_color(up, lv_color_hex(0x0F172A), LV_PART_MAIN);
    lv_obj_set_style_line_width(up, 2, LV_PART_MAIN);
    lv_obj_center(up);

    lv_obj_t *down = lv_line_create(box);
    lv_line_set_points(down, STAR_DOWN, sizeof(STAR_DOWN) / sizeof(STAR_DOWN[0]));
    lv_obj_set_style_line_color(down, lv_color_hex(0x0F172A), LV_PART_MAIN);
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
                              const char *symbol, uint16_t value, bool large) {
    lv_obj_t *label = lv_label_create(parent);
    char buf[32];
    snprintf(buf, sizeof(buf), "%s  %u", symbol, (unsigned)value);
    lv_label_set_text(label, buf);
    lv_obj_set_width(label, width);
    lv_obj_set_pos(label, x, y);
    lv_obj_set_style_text_font(label, large ? &lv_font_hebrew_24 : &lv_font_hebrew_16, LV_PART_MAIN);
    lv_obj_set_style_text_color(label, lv_color_hex(0x0F172A), LV_PART_MAIN);
    lv_obj_set_style_text_align(label, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN);
    lv_obj_set_style_base_dir(label, LV_BASE_DIR_LTR, LV_PART_MAIN);
}


static void create_hebrew_value(lv_obj_t *parent, int x, int y, int width,
                                uint16_t value, bool large) {
    const lv_font_t *font = large ? &lv_font_hebrew_24 : &lv_font_hebrew_16;
    const int symbol_w = large ? 24 : 18;
    const int count_w = large ? 36 : 30;
    const int gap = 4;
    const int group_w = symbol_w + gap + count_w;
    const int start_x = x + (width - group_w) / 2;

    lv_obj_t *symbol = lv_label_create(parent);
    lv_label_set_text(symbol, "א");
    lv_obj_set_pos(symbol, start_x, y);
    lv_obj_set_width(symbol, symbol_w);
    lv_obj_set_style_text_font(symbol, font, LV_PART_MAIN);
    lv_obj_set_style_text_color(symbol, lv_color_hex(0x0F172A), LV_PART_MAIN);
    lv_obj_set_style_text_align(symbol, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN);
    lv_obj_set_style_base_dir(symbol, LV_BASE_DIR_RTL, LV_PART_MAIN);

    lv_obj_t *count = lv_label_create(parent);
    char buf[12];
    snprintf(buf, sizeof(buf), "%u", (unsigned)value);
    lv_label_set_text(count, buf);
    lv_obj_set_pos(count, start_x + symbol_w + gap, y);
    lv_obj_set_width(count, count_w);
    lv_obj_set_style_text_font(count, font, LV_PART_MAIN);
    lv_obj_set_style_text_color(count, lv_color_hex(0x0F172A), LV_PART_MAIN);
    lv_obj_set_style_text_align(count, LV_TEXT_ALIGN_LEFT, LV_PART_MAIN);
    lv_obj_set_style_base_dir(count, LV_BASE_DIR_LTR, LV_PART_MAIN);
}

static void create_star_value(lv_obj_t *parent, int x, int y, int width,
                              uint16_t value, bool large) {
    const int icon_size = large ? 26 : 22;
    make_star(parent, x + 4, y - (large ? 2 : 1), icon_size);

    lv_obj_t *count = lv_label_create(parent);
    char buf[12];
    snprintf(buf, sizeof(buf), "%u", (unsigned)value);
    lv_label_set_text(count, buf);
    lv_obj_set_pos(count, x + icon_size + 8, y);
    lv_obj_set_width(count, width - icon_size - 10);
    lv_obj_set_style_text_font(count, large ? &lv_font_hebrew_24 : &lv_font_hebrew_16, LV_PART_MAIN);
    lv_obj_set_style_text_color(count, lv_color_hex(0x0F172A), LV_PART_MAIN);
    lv_obj_set_style_text_align(count, LV_TEXT_ALIGN_LEFT, LV_PART_MAIN);
    lv_obj_set_style_base_dir(count, LV_BASE_DIR_LTR, LV_PART_MAIN);
}

static void add_card_divider(lv_obj_t *card, int y) {
    lv_obj_t *divider = lv_obj_create(card);
    lv_obj_set_pos(divider, 8, y);
    lv_obj_set_size(divider, STATS_CONTENT_W - 18, 1);
    lv_obj_set_style_bg_color(divider, lv_color_hex(0xE2E8F0), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(divider, LV_OPA_COVER, LV_PART_MAIN);
    lv_obj_set_style_border_width(divider, 0, LV_PART_MAIN);
    lv_obj_set_style_radius(divider, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_all(divider, 0, LV_PART_MAIN);
    lv_obj_remove_flag(divider, (lv_obj_flag_t)(LV_OBJ_FLAG_SCROLLABLE | LV_OBJ_FLAG_CLICKABLE));
}

static void create_big_day_title(lv_obj_t *card, const WeeklyStatsDay_t &day, const char *prefix) {
    char date_buf[16];
    format_date(date_buf, sizeof(date_buf), day.date);

    lv_obj_t *prefix_lbl = lv_label_create(card);
    lv_label_set_text(prefix_lbl, prefix);
    lv_obj_set_pos(prefix_lbl, 348, 6);
    lv_obj_set_width(prefix_lbl, 94);
    lv_obj_set_style_text_font(prefix_lbl, &lv_font_hebrew_24, LV_PART_MAIN);
    lv_obj_set_style_text_color(prefix_lbl, lv_color_hex(0x0F172A), LV_PART_MAIN);
    lv_obj_set_style_text_align(prefix_lbl, LV_TEXT_ALIGN_RIGHT, LV_PART_MAIN);
    lv_obj_set_style_base_dir(prefix_lbl, LV_BASE_DIR_RTL, LV_PART_MAIN);

    lv_obj_t *date_lbl = lv_label_create(card);
    lv_label_set_text(date_lbl, date_buf);
    lv_obj_set_pos(date_lbl, 278, 10);
    lv_obj_set_width(date_lbl, 66);
    lv_obj_set_style_text_font(date_lbl, &lv_font_hebrew_16, LV_PART_MAIN);
    lv_obj_set_style_text_color(date_lbl, lv_color_hex(0x475569), LV_PART_MAIN);
    lv_obj_set_style_text_align(date_lbl, LV_TEXT_ALIGN_RIGHT, LV_PART_MAIN);
    lv_obj_set_style_base_dir(date_lbl, LV_BASE_DIR_LTR, LV_PART_MAIN);
}

static void create_big_subject_values(lv_obj_t *card, const WeeklyStatsDay_t &day) {
    const int y = 43;
    if (current_profile == PROFILE_ETHAN) {
        create_text_value(card, 8, y, 76, "?", day.correct[AHAVA_SUBJECT_CHALLENGES], true);
        create_star_value(card, 88, y, 76, day.correct[AHAVA_SUBJECT_RELIGION], true);
        create_text_value(card, 168, y, 92, "ABC", day.correct[AHAVA_SUBJECT_ENGLISH], true);
        create_hebrew_value(card, 264, y, 70, day.correct[AHAVA_SUBJECT_HEBREW], true);
        create_text_value(card, 338, y, 104, "×÷", day.correct[AHAVA_SUBJECT_MATH], true);
        return;
    }
    create_star_value(card, 10, y, 98, day.correct[AHAVA_SUBJECT_RELIGION], true);
    create_text_value(card, 112, y, 104, "ABC", day.correct[AHAVA_SUBJECT_ENGLISH], true);
    create_hebrew_value(card, 220, y, 78, day.correct[AHAVA_SUBJECT_HEBREW], true);
    create_text_value(card, 304, y, 138, "×÷", day.correct[AHAVA_SUBJECT_MATH], true);
}

static lv_obj_t *create_big_day_card(lv_obj_t *parent, int y,
                                     const WeeklyStatsDay_t &day,
                                     const char *prefix) {
    lv_obj_t *card = lv_obj_create(parent);
    lv_obj_set_size(card, STATS_CONTENT_W, 78);
    lv_obj_set_pos(card, 0, y);
    lv_obj_set_style_bg_color(card, lv_color_hex(0xFFFFFF), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(card, LV_OPA_COVER, LV_PART_MAIN);
    lv_obj_set_style_border_color(card, lv_color_hex(0xCBD5E1), LV_PART_MAIN);
    lv_obj_set_style_border_width(card, 1, LV_PART_MAIN);
    lv_obj_set_style_radius(card, 12, LV_PART_MAIN);
    lv_obj_set_style_pad_all(card, 0, LV_PART_MAIN);
    lv_obj_remove_flag(card, LV_OBJ_FLAG_SCROLLABLE);

    create_big_day_title(card, day, prefix);
    add_card_divider(card, 35);
    create_big_subject_values(card, day);
    return card;
}

static lv_obj_t *create_compact_day_row(lv_obj_t *parent, int y,
                                        const WeeklyStatsDay_t &day) {
    lv_obj_t *row = lv_obj_create(parent);
    lv_obj_set_size(row, STATS_CONTENT_W, 44);
    lv_obj_set_pos(row, 0, y);
    lv_obj_set_style_bg_color(row, lv_color_hex(0xFFFFFF), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(row, LV_OPA_COVER, LV_PART_MAIN);
    lv_obj_set_style_border_color(row, lv_color_hex(0xD7DEE8), LV_PART_MAIN);
    lv_obj_set_style_border_width(row, 1, LV_PART_MAIN);
    lv_obj_set_style_radius(row, 9, LV_PART_MAIN);
    lv_obj_set_style_pad_all(row, 0, LV_PART_MAIN);
    lv_obj_remove_flag(row, LV_OBJ_FLAG_SCROLLABLE);

    char date_buf[16];
    format_date(date_buf, sizeof(date_buf), day.date);
    lv_obj_t *date_lbl = lv_label_create(row);
    lv_label_set_text(date_lbl, date_buf);
    lv_obj_set_pos(date_lbl, 372, 12);
    lv_obj_set_width(date_lbl, 70);
    lv_obj_set_style_text_font(date_lbl, &lv_font_hebrew_16, LV_PART_MAIN);
    lv_obj_set_style_text_color(date_lbl, lv_color_hex(0x334155), LV_PART_MAIN);
    lv_obj_set_style_text_align(date_lbl, LV_TEXT_ALIGN_RIGHT, LV_PART_MAIN);
    lv_obj_set_style_base_dir(date_lbl, LV_BASE_DIR_LTR, LV_PART_MAIN);

    if (current_profile == PROFILE_ETHAN) {
        create_text_value(row, 304, 12, 62, "×÷", day.correct[AHAVA_SUBJECT_MATH], false);
        create_hebrew_value(row, 246, 12, 54, day.correct[AHAVA_SUBJECT_HEBREW], false);
        create_text_value(row, 164, 12, 78, "ABC", day.correct[AHAVA_SUBJECT_ENGLISH], false);
        create_star_value(row, 92, 12, 68, day.correct[AHAVA_SUBJECT_RELIGION], false);
        create_text_value(row, 20, 12, 68, "?", day.correct[AHAVA_SUBJECT_CHALLENGES], false);
    } else {
        create_text_value(row, 286, 12, 82, "×÷", day.correct[AHAVA_SUBJECT_MATH], false);
        create_hebrew_value(row, 222, 12, 60, day.correct[AHAVA_SUBJECT_HEBREW], false);
        create_text_value(row, 116, 12, 102, "ABC", day.correct[AHAVA_SUBJECT_ENGLISH], false);
        create_star_value(row, 12, 12, 98, day.correct[AHAVA_SUBJECT_RELIGION], false);
    }
    return row;
}

static void restore_stats_orientation(void) {
    if (!s_stats_landscape) return;
    hal_lvgl_set_landscape(false);
    s_stats_landscape = false;
}

static void close_stats(lv_event_t *e) {
    if (lv_event_get_code(e) != LV_EVENT_CLICKED) return;
    audio_play_click();
    if (s_overlay && lv_obj_is_valid(s_overlay)) {
        lv_obj_delete(s_overlay);
    }
    s_overlay = nullptr;
    restore_stats_orientation();
}

static void open_stats(lv_event_t *e) {
    if (lv_event_get_code(e) != LV_EVENT_CLICKED) return;
    if (current_profile == PROFILE_NONE) return;
    audio_play_click();

    if (s_overlay && lv_obj_is_valid(s_overlay)) return;

    WeeklyStatsDay_t days[WEEKLY_STATS_DAYS]{};
    const bool has_date = weekly_stats_store_get_last_seven(current_profile, days);

    // This screen is intentionally landscape-only. LVGL's software rotation also
    // rotates pointer coordinates, while the physical panel stays in portrait mode.
    hal_lvgl_set_landscape(true);
    s_stats_landscape = true;

    lv_obj_t *root = lv_layer_top();
    s_overlay = lv_obj_create(root);
    lv_obj_set_size(s_overlay, lv_pct(100), lv_pct(100));
    lv_obj_set_pos(s_overlay, 0, 0);
    lv_obj_set_style_bg_color(s_overlay, lv_color_hex(0xF6F8FC), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(s_overlay, LV_OPA_COVER, LV_PART_MAIN);
    lv_obj_set_style_border_width(s_overlay, 0, LV_PART_MAIN);
    lv_obj_set_style_radius(s_overlay, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_all(s_overlay, 0, LV_PART_MAIN);
    lv_obj_remove_flag(s_overlay, LV_OBJ_FLAG_SCROLLABLE);

    // Fixed 48 px header: back on the left, title on the right.
    lv_obj_t *header = lv_obj_create(s_overlay);
    lv_obj_set_size(header, STATS_SCREEN_W, STATS_HEADER_H);
    lv_obj_set_pos(header, 0, 0);
    lv_obj_set_style_bg_color(header, lv_color_hex(0xFFFFFF), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(header, LV_OPA_COVER, LV_PART_MAIN);
    lv_obj_set_style_border_width(header, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_all(header, 0, LV_PART_MAIN);
    lv_obj_remove_flag(header, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t *title = lv_label_create(header);
    lv_label_set_text(title, "ההתקדמות שלי");
    lv_obj_set_width(title, 250);
    lv_obj_set_style_text_font(title, &lv_font_hebrew_24, LV_PART_MAIN);
    lv_obj_set_style_text_color(title, lv_color_hex(0x0F172A), LV_PART_MAIN);
    lv_obj_set_style_text_align(title, LV_TEXT_ALIGN_RIGHT, LV_PART_MAIN);
    lv_obj_set_style_base_dir(title, LV_BASE_DIR_RTL, LV_PART_MAIN);
    lv_obj_align(title, LV_ALIGN_RIGHT_MID, -14, 0);

    lv_obj_t *back = lv_button_create(header);
    lv_obj_set_size(back, 72, 34);
    lv_obj_align(back, LV_ALIGN_LEFT_MID, 10, 0);
    lv_obj_set_style_bg_color(back, lv_color_hex(0xE2E8F0), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(back, LV_OPA_COVER, LV_PART_MAIN);
    lv_obj_set_style_border_color(back, lv_color_hex(0xCBD5E1), LV_PART_MAIN);
    lv_obj_set_style_border_width(back, 1, LV_PART_MAIN);
    lv_obj_set_style_radius(back, 9, LV_PART_MAIN);
    lv_obj_add_event_cb(back, close_stats, LV_EVENT_CLICKED, nullptr);
    lv_obj_t *back_lbl = lv_label_create(back);
    lv_label_set_text(back_lbl, "חזור");
    lv_obj_set_style_text_font(back_lbl, &lv_font_hebrew_16, LV_PART_MAIN);
    lv_obj_set_style_text_color(back_lbl, lv_color_hex(0x0F172A), LV_PART_MAIN);
    lv_obj_set_style_base_dir(back_lbl, LV_BASE_DIR_RTL, LV_PART_MAIN);
    lv_obj_center(back_lbl);

    lv_obj_t *separator = lv_obj_create(header);
    lv_obj_set_size(separator, STATS_SCREEN_W, 1);
    lv_obj_align(separator, LV_ALIGN_BOTTOM_MID, 0, 0);
    lv_obj_set_style_bg_color(separator, lv_color_hex(0xCBD5E1), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(separator, LV_OPA_COVER, LV_PART_MAIN);
    lv_obj_set_style_border_width(separator, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_all(separator, 0, LV_PART_MAIN);
    lv_obj_remove_flag(separator, (lv_obj_flag_t)(LV_OBJ_FLAG_SCROLLABLE | LV_OBJ_FLAG_CLICKABLE));

    // Vertical scrolling only. No horizontal panning is needed in landscape.
    lv_obj_t *content = lv_obj_create(s_overlay);
    lv_obj_set_size(content, STATS_CONTENT_W, STATS_CONTENT_H);
    lv_obj_set_pos(content, STATS_MARGIN_X, STATS_HEADER_H + 4);
    lv_obj_set_style_bg_opa(content, LV_OPA_TRANSP, LV_PART_MAIN);
    lv_obj_set_style_border_width(content, 0, LV_PART_MAIN);
    lv_obj_set_style_radius(content, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_all(content, 0, LV_PART_MAIN);
    lv_obj_set_scroll_dir(content, LV_DIR_VER);
    lv_obj_set_scrollbar_mode(content, LV_SCROLLBAR_MODE_AUTO);
    lv_obj_set_style_width(content, 4, LV_PART_SCROLLBAR);
    lv_obj_set_style_bg_color(content, lv_color_hex(0x94A3B8), LV_PART_SCROLLBAR);
    lv_obj_set_style_bg_opa(content, LV_OPA_50, LV_PART_SCROLLBAR);
    lv_obj_set_style_radius(content, LV_RADIUS_CIRCLE, LV_PART_SCROLLBAR);

    if (!has_date) {
        lv_obj_t *empty = lv_label_create(content);
        lv_label_set_text(empty, "הסטטיסטיקה תופיע כאן אחרי סנכרון השעה והתרגול הראשון");
        lv_obj_set_width(empty, 420);
        lv_label_set_long_mode(empty, LV_LABEL_LONG_WRAP);
        lv_obj_set_style_text_font(empty, &lv_font_hebrew_16, LV_PART_MAIN);
        lv_obj_set_style_text_color(empty, lv_color_hex(0x475569), LV_PART_MAIN);
        lv_obj_set_style_text_align(empty, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN);
        lv_obj_set_style_base_dir(empty, LV_BASE_DIR_RTL, LV_PART_MAIN);
        lv_obj_align(empty, LV_ALIGN_TOP_MID, 0, 72);
        lv_obj_move_foreground(s_overlay);
        return;
    }

    int y = 0;
    create_big_day_card(content, y, days[0], "היום");
    y += 86;
    create_big_day_card(content, y, days[1], "אתמול");
    y += 86;

    for (int i = 2; i < WEEKLY_STATS_DAYS; ++i) {
        create_compact_day_row(content, y, days[i]);
        y += 50;
    }

    // Ensure the final row creates enough scroll range without adding visual clutter.
    lv_obj_t *spacer = lv_obj_create(content);
    lv_obj_set_pos(spacer, 0, y);
    lv_obj_set_size(spacer, 1, 8);
    clear_obj_style(spacer);
    lv_obj_remove_flag(spacer, (lv_obj_flag_t)(LV_OBJ_FLAG_SCROLLABLE | LV_OBJ_FLAG_CLICKABLE));

    lv_obj_scroll_to_y(content, 0, LV_ANIM_OFF);
    lv_obj_move_foreground(s_overlay);
}

static lv_obj_t *dashboard_scroll_container(void) {
    if (sm_get_current_screen() != SCREEN_DASHBOARD) return nullptr;
    lv_obj_t *root = lv_screen_active();
    if (!root || lv_obj_get_child_count(root) < 2) return nullptr;

    // Dashboard construction is stable: HUD is child 0 and the vertical worlds
    // list is child 1. Keep the statistics card inside that list so it scrolls
    // naturally with the subjects instead of floating over the UI.
    return lv_obj_get_child(root, 1);
}

static void create_dashboard_stats_card(void) {
    lv_obj_t *scroll = dashboard_scroll_container();
    if (!scroll) return;

    s_stats_card = lv_button_create(scroll);
    lv_obj_set_size(s_stats_card, 290, 82);
    lv_obj_set_style_bg_color(s_stats_card, lv_color_hex(0x172554), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(s_stats_card, LV_OPA_COVER, LV_PART_MAIN);
    lv_obj_set_style_border_color(s_stats_card, lv_color_hex(0x94A3B8), LV_PART_MAIN);
    lv_obj_set_style_border_width(s_stats_card, 2, LV_PART_MAIN);
    lv_obj_set_style_radius(s_stats_card, 14, LV_PART_MAIN);
    lv_obj_set_style_shadow_width(s_stats_card, 8, LV_PART_MAIN);
    lv_obj_set_style_shadow_ofs_y(s_stats_card, 2, LV_PART_MAIN);
    lv_obj_set_style_shadow_color(s_stats_card, lv_color_hex(0x020617), LV_PART_MAIN);
    lv_obj_set_style_shadow_opa(s_stats_card, (lv_opa_t)LV_OPA_30, LV_PART_MAIN);
    lv_obj_set_style_pad_all(s_stats_card, 8, LV_PART_MAIN);
    lv_obj_set_style_base_dir(s_stats_card, LV_BASE_DIR_RTL, LV_PART_MAIN);
    lv_obj_add_event_cb(s_stats_card, open_stats, LV_EVENT_CLICKED, nullptr);

    // A simple numeric badge avoids dependence on emoji glyph support.
    lv_obj_t *badge = lv_obj_create(s_stats_card);
    lv_obj_set_size(badge, 46, 46);
    lv_obj_align(badge, LV_ALIGN_LEFT_MID, 2, 0);
    lv_obj_set_style_bg_color(badge, lv_color_hex(0x334155), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(badge, LV_OPA_COVER, LV_PART_MAIN);
    lv_obj_set_style_border_color(badge, lv_color_hex(0x94A3B8), LV_PART_MAIN);
    lv_obj_set_style_border_width(badge, 1, LV_PART_MAIN);
    lv_obj_set_style_radius(badge, LV_RADIUS_CIRCLE, LV_PART_MAIN);
    lv_obj_remove_flag(badge, (lv_obj_flag_t)(LV_OBJ_FLAG_SCROLLABLE | LV_OBJ_FLAG_CLICKABLE));

    lv_obj_t *badge_lbl = lv_label_create(badge);
    lv_label_set_text(badge_lbl, "7");
    lv_obj_set_style_text_font(badge_lbl, &lv_font_hebrew_24, LV_PART_MAIN);
    lv_obj_set_style_text_color(badge_lbl, lv_color_hex(0xF8FAFC), LV_PART_MAIN);
    lv_obj_center(badge_lbl);

    lv_obj_t *title = lv_label_create(s_stats_card);
    lv_label_set_text(title, stats_dashboard_card_title());
    lv_obj_set_width(title, 205);
    lv_obj_set_style_text_font(title, &lv_font_hebrew_24, LV_PART_MAIN);
    lv_obj_set_style_text_color(title, lv_color_hex(0xFFFFFF), LV_PART_MAIN);
    lv_obj_set_style_text_align(title, LV_TEXT_ALIGN_RIGHT, LV_PART_MAIN);
    lv_obj_set_style_base_dir(title, LV_BASE_DIR_RTL, LV_PART_MAIN);
    lv_obj_align(title, LV_ALIGN_TOP_RIGHT, -2, 4);

    lv_obj_t *subtitle = lv_label_create(s_stats_card);
    lv_label_set_text(subtitle, "כמה שאלות נכונות בכל מקצוע");
    lv_obj_set_width(subtitle, 205);
    lv_obj_set_style_text_font(subtitle, &lv_font_hebrew_16, LV_PART_MAIN);
    lv_obj_set_style_text_color(subtitle, lv_color_hex(0xCBD5E1), LV_PART_MAIN);
    lv_obj_set_style_text_align(subtitle, LV_TEXT_ALIGN_RIGHT, LV_PART_MAIN);
    lv_obj_set_style_base_dir(subtitle, LV_BASE_DIR_RTL, LV_PART_MAIN);
    lv_obj_align(subtitle, LV_ALIGN_BOTTOM_RIGHT, -2, -4);

    // The dashboard already contains 4 subject cards followed by System & OTA.
    // Place statistics between those groups so it is exactly where the user
    // expects it: under the subjects and before system/update settings.
    const uint32_t child_count = lv_obj_get_child_count(scroll);
    if (child_count >= 2) {
        lv_obj_move_to_index(s_stats_card, (int32_t)child_count - 2);
    }
}

static void stats_timer_cb(lv_timer_t *timer) {
    (void)timer;

    // Capture only when counters changed; the store itself suppresses redundant NVS writes.
    if (current_profile != PROFILE_NONE) {
        weekly_stats_store_capture_profile(current_profile);
    }

    if (sm_get_current_screen() != SCREEN_DASHBOARD) {
        s_stats_card = nullptr;
        s_overlay = nullptr;
        return;
    }

    if (s_stats_card && lv_obj_is_valid(s_stats_card)) return;
    create_dashboard_stats_card();
}

} // namespace

void stats_ui_init(void) {
    if (s_stats_timer) return;
    s_stats_timer = lv_timer_create(stats_timer_cb, 1000, nullptr);
    // Attach immediately when booting directly into a restored dashboard.
    stats_timer_cb(s_stats_timer);
    Serial.println("[STATS] Weekly statistics dashboard card initialized.");
}
