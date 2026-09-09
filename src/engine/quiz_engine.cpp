/** Offline Ahava question bank adapted for the MAX35 handheld. */
#include "quiz_engine.h"
#undef lv_obj_add_event_cb

#include "audio_manager.h"
#include "player_data.h"
#include "quiz_policy.h"
#include "theme_manager.h"
#include <Arduino.h>
#include <cstring>

namespace {
#include "generated_questions.inc"
#include "generated_ori_religion.inc"

constexpr size_t BASE_QUESTION_COUNT = sizeof(QUESTIONS) / sizeof(QUESTIONS[0]);
constexpr size_t ORI_RELIGION_COUNT =
    sizeof(ORI_RELIGION_QUESTIONS) / sizeof(ORI_RELIGION_QUESTIONS[0]);
constexpr size_t QUESTION_COUNT = BASE_QUESTION_COUNT + ORI_RELIGION_COUNT;
constexpr uint8_t RETRY_COOLDOWN_QUESTIONS = 2;

static size_t next_match[PROFILE_MAX][4] = {};
static uint16_t retry_cursor[PROFILE_MAX][4] = {};
static uint8_t retry_cooldown[PROFILE_MAX][4] = {};
static const Question_t *active_question = nullptr;
static int hinted_question_id = -1;

struct EventProxy {
    lv_event_cb_t original_cb = nullptr;
    void *original_user_data = nullptr;
};
static EventProxy answer_proxy[4];
static EventProxy subject_proxy[4];
static EventProxy continue_proxy;

static bool valid_profile(WizardProfile_t profile) {
    return profile > PROFILE_NONE && profile < PROFILE_MAX;
}

static const Question_t* question_at(size_t index) {
    if (index < BASE_QUESTION_COUNT) return &QUESTIONS[index];
    index -= BASE_QUESTION_COUNT;
    if (index < ORI_RELIGION_COUNT) return &ORI_RELIGION_QUESTIONS[index];
    return nullptr;
}

static bool selectable_for_profile(size_t index, const Question_t *q,
                                   WizardProfile_t profile, int subject_filter) {
    if (!q || q->target_profile != profile) return false;
    if (subject_filter >= 0 && q->subject_id != subject_filter) return false;

    /* Ori's Judaism world intentionally uses the reviewed precision bank only. */
    if (profile == PROFILE_ORI && q->subject_id == 3 && index < BASE_QUESTION_COUNT) {
        return false;
    }
    return true;
}

static size_t selectable_count(WizardProfile_t profile, int subject_id) {
    size_t count = 0;
    for (size_t i = 0; i < QUESTION_COUNT; ++i) {
        const Question_t *q = question_at(i);
        if (selectable_for_profile(i, q, profile, subject_id)) ++count;
    }
    return count;
}

static int question_ordinal(const Question_t *target) {
    if (!target || !valid_profile(target->target_profile) ||
        target->subject_id < 0 || target->subject_id > 3) return -1;

    int ordinal = 0;
    for (size_t i = 0; i < QUESTION_COUNT; ++i) {
        const Question_t *q = question_at(i);
        if (!selectable_for_profile(i, q, target->target_profile, target->subject_id)) continue;
        if (q == target) return ordinal;
        ++ordinal;
    }
    return -1;
}

static const Question_t* question_by_ordinal(WizardProfile_t profile, int subject_id,
                                              uint16_t ordinal) {
    uint16_t current = 0;
    for (size_t i = 0; i < QUESTION_COUNT; ++i) {
        const Question_t *q = question_at(i);
        if (!selectable_for_profile(i, q, profile, subject_id)) continue;
        if (current++ == ordinal) return q;
    }
    return nullptr;
}

static bool daily_limit_reached(WizardProfile_t profile, int subject_id) {
    const uint32_t limit = quiz_policy_daily_limit((int)profile, subject_id);
    if (limit == UINT32_MAX) return false;
    return player_data_get_subject_questions_today(profile, subject_id) >= limit;
}

static void hint_close_clicked(lv_event_t *e) {
    if (lv_event_get_code(e) != LV_EVENT_CLICKED) return;
    audio_play_click();
    lv_obj_t *mbox = static_cast<lv_obj_t *>(lv_event_get_user_data(e));
    if (mbox) lv_msgbox_close(mbox);
}

static void show_daily_limit_message(void) {
    lv_obj_t *mbox = lv_msgbox_create(NULL);
    if (!mbox) return;
    lv_obj_set_style_base_dir(mbox, LV_BASE_DIR_RTL, LV_PART_MAIN);
    lv_obj_set_style_bg_color(mbox, lv_color_hex(COLOR_BG_CARD), LV_PART_MAIN);
    lv_obj_set_style_border_color(mbox, lv_color_hex(0x10B981), LV_PART_MAIN);
    lv_obj_set_style_border_width(mbox, 2, LV_PART_MAIN);
    lv_obj_set_style_radius(mbox, 18, LV_PART_MAIN);
    lv_obj_set_size(mbox, 300, 250);
    lv_obj_center(mbox);

    lv_obj_t *title = lv_msgbox_add_title(mbox, "המשימה היומית הושלמה 🎯");
    lv_obj_t *text = lv_msgbox_add_text(mbox, "אורי השלים 10 שאלות יהדות שנענו נכון בניסיון הראשון. אפשר לחזור מחר.");
    for (lv_obj_t *label : {title, text}) {
        if (!label) continue;
        lv_obj_set_style_text_font(label, &lv_font_hebrew_16, LV_PART_MAIN);
        lv_obj_set_style_text_color(label, lv_color_hex(0xFFFFFF), LV_PART_MAIN);
        lv_obj_set_style_text_align(label, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN);
        lv_obj_set_style_base_dir(label, LV_BASE_DIR_RTL, LV_PART_MAIN);
    }
    lv_obj_set_width(text, 250);
    lv_label_set_long_mode(text, LV_LABEL_LONG_WRAP);

    lv_obj_t *close_btn = lv_msgbox_add_footer_button(mbox, "סגור");
    theme_apply_btn_main(close_btn);
    lv_obj_set_size(close_btn, 150, 48);
    lv_obj_add_event_cb(close_btn, hint_close_clicked, LV_EVENT_CLICKED, mbox);
}

static void show_hint(const Question_t *q) {
    const char *hint = (q && q->hint && *q->hint)
        ? q->hint
        : "נסה שוב. חפש את הפרט המדויק שמבדיל בין האפשרויות.";

    lv_obj_t *mbox = lv_msgbox_create(NULL);
    if (!mbox) return;
    lv_obj_set_style_base_dir(mbox, LV_BASE_DIR_RTL, LV_PART_MAIN);
    lv_obj_set_style_bg_color(mbox, lv_color_hex(COLOR_BG_CARD), LV_PART_MAIN);
    lv_obj_set_style_border_color(mbox, lv_color_hex(0xF59E0B), LV_PART_MAIN);
    lv_obj_set_style_border_width(mbox, 2, LV_PART_MAIN);
    lv_obj_set_style_radius(mbox, 18, LV_PART_MAIN);
    lv_obj_set_size(mbox, 300, 270);
    lv_obj_center(mbox);

    lv_obj_t *title = lv_msgbox_add_title(mbox, "רמז — נסה שוב");
    lv_obj_t *text = lv_msgbox_add_text(mbox, hint);
    for (lv_obj_t *label : {title, text}) {
        if (!label) continue;
        lv_obj_set_style_text_font(label, &lv_font_hebrew_16, LV_PART_MAIN);
        lv_obj_set_style_text_color(label, lv_color_hex(0xFFFFFF), LV_PART_MAIN);
        lv_obj_set_style_text_align(label, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN);
        lv_obj_set_style_base_dir(label, LV_BASE_DIR_RTL, LV_PART_MAIN);
    }
    lv_obj_set_width(text, 250);
    lv_label_set_long_mode(text, LV_LABEL_LONG_WRAP);

    lv_obj_t *retry_btn = lv_msgbox_add_footer_button(mbox, "נסה שוב");
    theme_apply_btn_main(retry_btn);
    lv_obj_set_size(retry_btn, 150, 48);
    lv_obj_add_event_cb(retry_btn, hint_close_clicked, LV_EVENT_CLICKED, mbox);

    lv_obj_t *footer = lv_obj_get_parent(retry_btn);
    if (footer) {
        lv_obj_set_width(footer, LV_PCT(100));
        lv_obj_set_style_base_dir(footer, LV_BASE_DIR_LTR, LV_PART_MAIN);
        lv_obj_set_flex_align(footer, LV_FLEX_ALIGN_CENTER,
                              LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    }
    lv_obj_t *btn_lbl = lv_obj_get_child(retry_btn, 0);
    if (btn_lbl) {
        lv_obj_set_style_text_font(btn_lbl, &lv_font_hebrew_24, LV_PART_MAIN);
        lv_obj_set_style_base_dir(btn_lbl, LV_BASE_DIR_RTL, LV_PART_MAIN);
    }
}

static void answer_proxy_clicked(lv_event_t *e) {
    if (lv_event_get_code(e) != LV_EVENT_CLICKED) return;

    EventProxy *proxy = static_cast<EventProxy *>(lv_event_get_user_data(e));
    if (!proxy || !active_question) return;

    const uint8_t clicked_idx =
        static_cast<uint8_t>(reinterpret_cast<uintptr_t>(proxy->original_user_data));
    const bool correct = clicked_idx == active_question->correct_idx;
    const bool hint_already_used = hinted_question_id == active_question->id;
    const WizardProfile_t profile = active_question->target_profile;
    const int subject_id = active_question->subject_id;
    const int ordinal = question_ordinal(active_question);

    if (correct || hint_already_used) {
        const bool eligible = quiz_policy_should_count(correct, hint_already_used);
        player_data_prepare_question_count(profile, subject_id, eligible);

        if (ordinal >= 0) {
            if (eligible) {
                player_data_retry_set(profile, subject_id, (uint16_t)ordinal, false);
            } else {
                player_data_retry_set(profile, subject_id, (uint16_t)ordinal, true);
                retry_cooldown[profile][subject_id] = RETRY_COOLDOWN_QUESTIONS;
            }
        }
        /* Let screen_manager finalize feedback/rewards and consume the count gate. */
        return;
    }

    /* First wrong answer: persist it for later, show a hint, and do not advance. */
    hinted_question_id = active_question->id;
    if (ordinal >= 0) {
        player_data_retry_set(profile, subject_id, (uint16_t)ordinal, true);
        retry_cooldown[profile][subject_id] = RETRY_COOLDOWN_QUESTIONS;
    }
    audio_play_fail();

    lv_obj_t *clicked_btn = static_cast<lv_obj_t *>(lv_event_get_target(e));
    if (clicked_btn) {
        lv_obj_set_style_bg_color(clicked_btn, lv_color_hex(0xEF4444), LV_PART_MAIN);
        lv_obj_set_style_bg_grad_color(clicked_btn, lv_color_hex(0xDC2626), LV_PART_MAIN);
        lv_obj_set_style_border_color(clicked_btn, lv_color_hex(0xF87171), LV_PART_MAIN);
        lv_obj_set_style_shadow_color(clicked_btn, lv_color_hex(0xEF4444), LV_PART_MAIN);
    }

    Serial.printf("[QUIZ] First wrong answer id=%d -> persisted for retry; hint shown.\n",
                  active_question->id);
    show_hint(active_question);
    lv_event_stop_processing(e);
}

static void subject_play_proxy_clicked(lv_event_t *e) {
    if (lv_event_get_code(e) != LV_EVENT_CLICKED) return;
    EventProxy *proxy = static_cast<EventProxy *>(lv_event_get_user_data(e));
    if (!proxy) return;

    const int subject_id = (int)reinterpret_cast<uintptr_t>(proxy->original_user_data);
    if (daily_limit_reached(current_profile, subject_id)) {
        audio_play_click();
        show_daily_limit_message();
        lv_event_stop_processing(e);
    }
}

static void continue_proxy_clicked(lv_event_t *e) {
    if (lv_event_get_code(e) != LV_EVENT_CLICKED) return;
    if (!active_question) return;

    if (daily_limit_reached(active_question->target_profile, active_question->subject_id)) {
        audio_play_click();
        sm_load_screen(SCREEN_DASHBOARD);
        lv_event_stop_processing(e);
    }
}

}  // namespace

bool quiz_validate_database(void) {
    bool ok = true;

    if (ORI_RELIGION_COUNT != 150) {
        Serial.printf("[QUIZ] Expected 150 new Ori religion questions, got %u\n",
                      (unsigned)ORI_RELIGION_COUNT);
        ok = false;
    }

    for (size_t i = 0; i < QUESTION_COUNT; ++i) {
        const Question_t *qp = question_at(i);
        if (!qp) {
            ok = false;
            continue;
        }

        const Question_t &q = *qp;
        bool row_ok = valid_profile(q.target_profile) && q.subject_id >= 0 && q.subject_id <= 3 &&
                      q.correct_idx <= 3 && q.text && *q.text && q.feedback && *q.feedback;
        for (const char *answer : q.answers) if (!answer || !*answer) row_ok = false;

        for (size_t j = 0; j < i; ++j) {
            const Question_t *other = question_at(j);
            if (other && other->id == q.id) row_ok = false;
        }

        if (i >= BASE_QUESTION_COUNT) {
            if (q.target_profile != PROFILE_ORI || q.subject_id != 3 || !q.hint || !*q.hint) {
                row_ok = false;
            }
        }

        if (!row_ok) {
            Serial.printf("[QUIZ] Invalid question at index %u (id=%d)\n",
                          (unsigned)i, q.id);
            ok = false;
        }
    }

    for (int profile = PROFILE_ORI; profile < PROFILE_MAX; ++profile) {
        for (int subject = 0; subject < 4; ++subject) {
            size_t count = selectable_count((WizardProfile_t)profile, subject);
            if (count < 3) {
                Serial.printf("[QUIZ] Missing active coverage profile=%d subject=%d count=%u\n",
                              profile, subject, (unsigned)count);
                ok = false;
            }
        }
    }

    Serial.printf("[QUIZ] Database validation: %u rows (%u new Ori Judaism), %s\n",
                  (unsigned)QUESTION_COUNT, (unsigned)ORI_RELIGION_COUNT,
                  ok ? "PASS" : "FAIL");
    return ok;
}

const Question_t* quiz_get_next_question(WizardProfile_t profile, int subject_id) {
    if (!valid_profile(profile) || subject_id < 0 || subject_id > 3) return nullptr;

    if (daily_limit_reached(profile, subject_id)) {
        Serial.printf("[QUIZ] Daily limit reached profile=%d subject=%d.\n",
                      (int)profile, subject_id);
        active_question = nullptr;
        return nullptr;
    }

    const size_t count = selectable_count(profile, subject_id);
    if (!count) return nullptr;

    if (retry_cooldown[profile][subject_id] > 0) {
        --retry_cooldown[profile][subject_id];
    } else if (count <= UINT16_MAX) {
        const int retry_ordinal = player_data_retry_find_next(
            profile, subject_id, retry_cursor[profile][subject_id], (uint16_t)count);
        if (retry_ordinal >= 0) {
            const Question_t *retry_q = question_by_ordinal(profile, subject_id,
                                                             (uint16_t)retry_ordinal);
            if (retry_q) {
                retry_cursor[profile][subject_id] =
                    (uint16_t)(((uint16_t)retry_ordinal + 1) % (uint16_t)count);
                retry_cooldown[profile][subject_id] = RETRY_COOLDOWN_QUESTIONS;
                active_question = retry_q;
                hinted_question_id = -1;
                Serial.printf("[QUIZ] Retry question id=%d profile=%d subject=%d ordinal=%d\n",
                              retry_q->id, (int)profile, subject_id, retry_ordinal);
                return retry_q;
            }
        }
    }

    size_t target = next_match[profile][subject_id]++ % count;
    for (size_t i = 0; i < QUESTION_COUNT; ++i) {
        const Question_t *q = question_at(i);
        if (selectable_for_profile(i, q, profile, subject_id) && target-- == 0) {
            active_question = q;
            hinted_question_id = -1;
            Serial.printf("[QUIZ] Question id=%d profile=%d subject=%d\n",
                          q->id, (int)profile, subject_id);
            return q;
        }
    }
    return nullptr;
}

size_t quiz_get_total_questions(void) {
    return QUESTION_COUNT;
}

size_t quiz_get_question_count(WizardProfile_t profile, int subject_id) {
    if (!valid_profile(profile) || subject_id < -1 || subject_id > 3) return 0;
    size_t count = 0;
    for (size_t i = 0; i < QUESTION_COUNT; ++i) {
        const Question_t *q = question_at(i);
        if (selectable_for_profile(i, q, profile, subject_id)) ++count;
    }
    return count;
}

lv_event_dsc_t* quiz_register_event_cb(lv_obj_t *obj, lv_event_cb_t cb,
                                      lv_event_code_t filter, void *user_data,
                                      const char *callback_name) {
    if (callback_name && filter == LV_EVENT_CLICKED &&
        std::strcmp(callback_name, "on_answer_clicked") == 0) {
        const uintptr_t idx = reinterpret_cast<uintptr_t>(user_data);
        if (idx < 4) {
            answer_proxy[idx].original_cb = cb;
            answer_proxy[idx].original_user_data = user_data;
            lv_obj_add_event_cb(obj, answer_proxy_clicked, filter, &answer_proxy[idx]);
            return lv_obj_add_event_cb(obj, cb, filter, user_data);
        }
    }

    if (callback_name && filter == LV_EVENT_CLICKED &&
        std::strcmp(callback_name, "on_play_subject_clicked") == 0) {
        const uintptr_t subject = reinterpret_cast<uintptr_t>(user_data);
        if (subject < 4) {
            subject_proxy[subject].original_cb = cb;
            subject_proxy[subject].original_user_data = user_data;
            lv_obj_add_event_cb(obj, subject_play_proxy_clicked, filter, &subject_proxy[subject]);
            return lv_obj_add_event_cb(obj, cb, filter, user_data);
        }
    }

    if (callback_name && filter == LV_EVENT_CLICKED &&
        std::strcmp(callback_name, "on_msgbox_continue_clicked") == 0) {
        continue_proxy.original_cb = cb;
        continue_proxy.original_user_data = user_data;
        lv_obj_add_event_cb(obj, continue_proxy_clicked, filter, &continue_proxy);
        return lv_obj_add_event_cb(obj, cb, filter, user_data);
    }

    return lv_obj_add_event_cb(obj, cb, filter, user_data);
}
