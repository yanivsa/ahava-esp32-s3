#!/usr/bin/env python3
"""One-shot integration patcher for the exam-prep feature branch.

This exists only to safely make deterministic edits to the large embedded UI/engine
translation units in CI. The final bootstrap workflow deletes this helper after the
full native + firmware verification is green.
"""
from pathlib import Path
import subprocess
import sys

ROOT = Path(__file__).resolve().parents[1]


def read(rel):
    return (ROOT / rel).read_text(encoding="utf-8")


def write(rel, text):
    (ROOT / rel).write_text(text, encoding="utf-8")


def replace_once(rel, old, new):
    text = read(rel)
    count = text.count(old)
    if count != 1:
        raise RuntimeError(f"{rel}: expected exactly one match, found {count}: {old[:80]!r}")
    write(rel, text.replace(old, new, 1))


def append_before(rel, marker, block):
    replace_once(rel, marker, block + "\n" + marker)


# ---------------------------------------------------------------------------
# 1) Generate the canonical source JSON + C++ include.
# ---------------------------------------------------------------------------
subprocess.run([sys.executable, str(ROOT / "scripts/generate_ori_exam_prep_questions.py"), "--write-source"], check=True)

# ---------------------------------------------------------------------------
# 2) Player-data persistence: separate 70-bit mastery store, never academic.
# ---------------------------------------------------------------------------
replace_once(
    "src/engine/player_data.cpp",
    '#include "quiz_policy.h"\n',
    '#include "quiz_policy.h"\n#include "exam_prep_progress.h"\n',
)

append_before(
    "src/engine/player_data.cpp",
    "void player_data_reset_all(void) {",
    r'''
static bool exam_prep_load_words(WizardProfile_t profile, uint32_t words[3]) {
    if (!words) return false;
    memset(words, 0, sizeof(uint32_t) * EXAM_PREP_PROGRESS_WORDS);
    if (profile != PROFILE_ORI) return false;
    nvs_handle_t handle;
    if (nvs_open(NVS_STORAGE_NAMESPACE, NVS_READONLY, &handle) != ESP_OK) return true;
    size_t size = sizeof(uint32_t) * EXAM_PREP_PROGRESS_WORDS;
    esp_err_t err = nvs_get_blob(handle, "exam_prep_1", words, &size);
    nvs_close(handle);
    return err == ESP_OK || err == ESP_ERR_NVS_NOT_FOUND;
}

static bool exam_prep_save_words(WizardProfile_t profile, const uint32_t words[3]) {
    if (profile != PROFILE_ORI || !words) return false;
    nvs_handle_t handle;
    if (nvs_open(NVS_STORAGE_NAMESPACE, NVS_READWRITE, &handle) != ESP_OK) return false;
    esp_err_t err = nvs_set_blob(handle, "exam_prep_1", words,
                                 sizeof(uint32_t) * EXAM_PREP_PROGRESS_WORDS);
    if (err == ESP_OK) err = nvs_commit(handle);
    nvs_close(handle);
    return err == ESP_OK;
}

bool player_data_exam_prep_is_solved(WizardProfile_t profile, uint16_t ordinal) {
    uint32_t words[3] = {};
    exam_prep_load_words(profile, words);
    return exam_prep_progress_is_solved(words, ordinal);
}

void player_data_exam_prep_mark_solved(WizardProfile_t profile, uint16_t ordinal) {
    if (profile != PROFILE_ORI || ordinal >= EXAM_PREP_QUESTION_COUNT) return;
    uint32_t words[3] = {};
    exam_prep_load_words(profile, words);
    if (exam_prep_progress_is_solved(words, ordinal)) return;
    exam_prep_progress_mark_solved(words, ordinal);
    exam_prep_save_words(profile, words);
    Serial.printf("[EXAM_PREP] Mastered ordinal=%u total=%u/70.\n",
                  (unsigned)ordinal, (unsigned)exam_prep_progress_total_solved(words));
}

uint16_t player_data_exam_prep_total_solved(WizardProfile_t profile) {
    uint32_t words[3] = {};
    exam_prep_load_words(profile, words);
    return exam_prep_progress_total_solved(words);
}

uint8_t player_data_exam_prep_current_tier(WizardProfile_t profile) {
    uint32_t words[3] = {};
    exam_prep_load_words(profile, words);
    return exam_prep_progress_current_tier(words);
}

uint8_t player_data_exam_prep_tier_solved(WizardProfile_t profile, uint8_t tier) {
    uint32_t words[3] = {};
    exam_prep_load_words(profile, words);
    return exam_prep_progress_tier_solved(words, tier);
}
'''.strip(),
)

# ---------------------------------------------------------------------------
# 3) Quiz engine: bank wiring, strict Ori-only selection, mandatory work phases,
#    mastery progression and Time+ isolation.
# ---------------------------------------------------------------------------
replace_once(
    "src/engine/quiz_engine.cpp",
    '#include "quiz_policy.h"\n#include "theme_manager.h"\n',
    '#include "quiz_policy.h"\n#include "exam_prep_contract.h"\n#include "exam_prep_progress.h"\n#include "exam_prep_workflow.h"\n#include "theme_manager.h"\n',
)
replace_once(
    "src/engine/quiz_engine.cpp",
    '#include "generated_ori_math.inc"\n',
    '#include "generated_ori_math.inc"\n#include "generated_ori_exam_prep.inc"\n',
)
replace_once(
    "src/engine/quiz_engine.cpp",
    '''constexpr size_t ORI_MATH_COUNT =
    sizeof(ORI_MATH_QUESTIONS) / sizeof(ORI_MATH_QUESTIONS[0]);
constexpr size_t QUESTION_COUNT = BASE_QUESTION_COUNT + ORI_RELIGION_COUNT + ORI_MATH_COUNT;
''',
    '''constexpr size_t ORI_MATH_COUNT =
    sizeof(ORI_MATH_QUESTIONS) / sizeof(ORI_MATH_QUESTIONS[0]);
constexpr size_t ORI_EXAM_PREP_COUNT =
    sizeof(ORI_EXAM_PREP_QUESTIONS) / sizeof(ORI_EXAM_PREP_QUESTIONS[0]);
constexpr size_t QUESTION_COUNT = BASE_QUESTION_COUNT + ORI_RELIGION_COUNT + ORI_MATH_COUNT + ORI_EXAM_PREP_COUNT;
''',
)
replace_once(
    "src/engine/quiz_engine.cpp",
    'static int hinted_question_id = -1;\n',
    'static int hinted_question_id = -1;\nstatic ExamPrepWorkState_t exam_work_state = {EXAM_WORK_FINAL, 0, 0, false};\n',
)
replace_once(
    "src/engine/quiz_engine.cpp",
    '''    if (index < ORI_MATH_COUNT) return &ORI_MATH_QUESTIONS[index];
    return nullptr;
}''',
    '''    if (index < ORI_MATH_COUNT) return &ORI_MATH_QUESTIONS[index];
    index -= ORI_MATH_COUNT;
    if (index < ORI_EXAM_PREP_COUNT) return &ORI_EXAM_PREP_QUESTIONS[index];
    return nullptr;
}''',
)

# Add activation helpers immediately after valid_profile.
replace_once(
    "src/engine/quiz_engine.cpp",
    '''static bool valid_profile(WizardProfile_t profile) {
    return profile > PROFILE_NONE && profile < PROFILE_MAX;
}
''',
    '''static bool valid_profile(WizardProfile_t profile) {
    return profile > PROFILE_NONE && profile < PROFILE_MAX;
}

static void activate_question(const Question_t *q) {
    active_question = q;
    hinted_question_id = -1;
    if (q && q->subject_id == AHAVA_SUBJECT_EXAM_PREP) {
        const bool has_second = q->work_prompt_2 && *q->work_prompt_2;
        exam_prep_workflow_reset(&exam_work_state, q->work_correct_idx_1,
                                 has_second, q->work_correct_idx_2);
    } else {
        exam_work_state.phase = EXAM_WORK_FINAL;
        exam_work_state.has_second = false;
    }
}
''',
)

# Subject name.
replace_once(
    "src/engine/quiz_engine.cpp",
    '''            case 3: return "יהדות והלכה";
            default: return "לימוד";''',
    '''            case 3: return "יהדות והלכה";
            case AHAVA_SUBJECT_EXAM_PREP: return "הכנה למבחן";
            default: return "לימוד";''',
)

# Work phases must pass through to screen_manager without finalization.
replace_once(
    "src/engine/quiz_engine.cpp",
    '''    const bool correct = clicked_idx == active_question->correct_idx;
    const bool hint_already_used = hinted_question_id == active_question->id;
''',
    '''    if (active_question->subject_id == AHAVA_SUBJECT_EXAM_PREP &&
        exam_work_state.phase != EXAM_WORK_FINAL) {
        return;  // screen_manager consumes the mandatory work checkpoint.
    }

    const bool correct = clicked_idx == active_question->correct_idx;
    const bool hint_already_used = hinted_question_id == active_question->id;
''',
)
replace_once(
    "src/engine/quiz_engine.cpp",
    '''    const int stats_subject_id = subject_id == AHAVA_SUBJECT_CHALLENGES
        ? active_question->stats_subject_id
        : subject_id;
''',
    '''    const int stats_subject_id =
        (subject_id == AHAVA_SUBJECT_CHALLENGES || subject_id == AHAVA_SUBJECT_EXAM_PREP)
            ? active_question->stats_subject_id : subject_id;
''',
)
replace_once(
    "src/engine/quiz_engine.cpp",
    '''    if (correct || hint_already_used) {
        const bool eligible = quiz_policy_should_count(correct, hint_already_used);
        player_data_prepare_question_count(profile, subject_id, stats_subject_id, eligible);

        if (ordinal >= 0) {
''',
    '''    if (correct || hint_already_used) {
        if (subject_id == AHAVA_SUBJECT_EXAM_PREP && correct && ordinal >= 0) {
            player_data_exam_prep_mark_solved(profile, (uint16_t)ordinal);
        }
        const bool eligible = quiz_policy_grants_timeplus_credit(subject_id) &&
                              quiz_policy_should_count(correct, hint_already_used);
        player_data_prepare_question_count(profile, subject_id, stats_subject_id, eligible);

        if (ordinal >= 0 && subject_id != AHAVA_SUBJECT_EXAM_PREP) {
''',
)
replace_once(
    "src/engine/quiz_engine.cpp",
    '''    if (ordinal >= 0) {
        player_data_retry_set(profile, subject_id, (uint16_t)ordinal, true);
        retry_cooldown[profile][subject_id] = RETRY_COOLDOWN_QUESTIONS;
    }
    audio_play_fail();
''',
    '''    if (ordinal >= 0 && subject_id != AHAVA_SUBJECT_EXAM_PREP) {
        player_data_retry_set(profile, subject_id, (uint16_t)ordinal, true);
        retry_cooldown[profile][subject_id] = RETRY_COOLDOWN_QUESTIONS;
    }
    audio_play_fail();
''',
)

# Database validation: explicit exam bank checks instead of treating every tail row as math.
replace_once(
    "src/engine/quiz_engine.cpp",
    '''    if (ORI_MATH_COUNT < 1000) {
        Serial.printf("[QUIZ] Expected at least 1000 new Ori math questions, got %u\\n",
                      (unsigned)ORI_MATH_COUNT);
        ok = false;
    }
''',
    '''    if (ORI_MATH_COUNT < 1000) {
        Serial.printf("[QUIZ] Expected at least 1000 new Ori math questions, got %u\\n",
                      (unsigned)ORI_MATH_COUNT);
        ok = false;
    }
    if (ORI_EXAM_PREP_COUNT != EXAM_PREP_QUESTION_COUNT) {
        Serial.printf("[QUIZ] Expected 70 Ori exam-prep questions, got %u\\n",
                      (unsigned)ORI_EXAM_PREP_COUNT);
        ok = false;
    }
''',
)
replace_once(
    "src/engine/quiz_engine.cpp",
    '''        } else if (i >= BASE_QUESTION_COUNT + ORI_RELIGION_COUNT) {
            if (q.target_profile != PROFILE_ORI || q.subject_id != 0 || !q.hint || !*q.hint) {
                row_ok = false;
            }
        }
''',
    '''        } else if (i < BASE_QUESTION_COUNT + ORI_RELIGION_COUNT + ORI_MATH_COUNT) {
            if (q.target_profile != PROFILE_ORI || q.subject_id != AHAVA_SUBJECT_MATH || !q.hint || !*q.hint) {
                row_ok = false;
            }
        } else {
            const bool second_ok = !q.work_prompt_2 || !*q.work_prompt_2 || q.work_correct_idx_2 <= 3;
            if (q.target_profile != PROFILE_ORI || q.subject_id != AHAVA_SUBJECT_EXAM_PREP ||
                !q.hint || !*q.hint || !exam_prep_tier_valid(q.difficulty_tier) ||
                !exam_prep_operation_valid(q.exam_prep_operation) || !q.work_prompt_1 ||
                !*q.work_prompt_1 || q.work_correct_idx_1 > 3 || !second_ok || q.stats_subject_id != -1) {
                row_ok = false;
            }
        }
''',
)
replace_once(
    "src/engine/quiz_engine.cpp",
    '''    const size_t challenge_count = selectable_count(PROFILE_ETHAN, AHAVA_SUBJECT_CHALLENGES);
''',
    '''    const size_t exam_count = selectable_count(PROFILE_ORI, AHAVA_SUBJECT_EXAM_PREP);
    if (exam_count != EXAM_PREP_QUESTION_COUNT) {
        Serial.printf("[QUIZ] Expected 70 Ori exam-prep questions, got %u\\n", (unsigned)exam_count);
        ok = false;
    }

    const size_t challenge_count = selectable_count(PROFILE_ETHAN, AHAVA_SUBJECT_CHALLENGES);
''',
)
replace_once(
    "src/engine/quiz_engine.cpp",
    '''    Serial.printf("[QUIZ] Database validation: %u rows (%u Ori Judaism, %u Ori Math), %s\\n",
                  (unsigned)QUESTION_COUNT, (unsigned)ORI_RELIGION_COUNT, (unsigned)ORI_MATH_COUNT,
                  ok ? "PASS" : "FAIL");
''',
    '''    Serial.printf("[QUIZ] Database validation: %u rows (%u Ori Judaism, %u Ori Math, %u Exam Prep), %s\\n",
                  (unsigned)QUESTION_COUNT, (unsigned)ORI_RELIGION_COUNT, (unsigned)ORI_MATH_COUNT,
                  (unsigned)ORI_EXAM_PREP_COUNT, ok ? "PASS" : "FAIL");
''',
)

# Dedicated exam-prep selector inserted before public quiz_get_next_question.
insert_marker = "const Question_t* quiz_get_next_question(WizardProfile_t profile, int subject_id) {"
exam_selector = r'''
static const Question_t* next_exam_prep_question(WizardProfile_t profile) {
    if (profile != PROFILE_ORI || ORI_EXAM_PREP_COUNT != EXAM_PREP_QUESTION_COUNT) return nullptr;

    const uint16_t total = player_data_exam_prep_total_solved(profile);
    uint8_t tier = player_data_exam_prep_current_tier(profile);
    const bool master = total >= EXAM_PREP_QUESTION_COUNT;
    const uint16_t start = master ? 0u : (uint16_t)(tier - 1u) * EXAM_PREP_PER_TIER;
    const uint16_t span = master ? EXAM_PREP_QUESTION_COUNT : EXAM_PREP_PER_TIER;

    uint32_t seq = player_data_get_quiz_seq(profile, AHAVA_SUBJECT_EXAM_PREP);
    uint32_t seed = player_data_get_quiz_seed(profile, AHAVA_SUBJECT_EXAM_PREP);
    if (seed == 0u) seed = 1u;
    const uint32_t stride = compute_coprime_stride(span);

    for (uint16_t attempt = 0; attempt < span; ++attempt) {
        const uint16_t local = (uint16_t)(((seed - 1u + (seq + attempt) * stride) % span));
        const uint16_t ordinal = start + local;
        if (!master && player_data_exam_prep_is_solved(profile, ordinal)) continue;
        player_data_set_quiz_seq(profile, AHAVA_SUBJECT_EXAM_PREP, seq + attempt + 1u);
        const Question_t *q = &ORI_EXAM_PREP_QUESTIONS[ordinal];
        activate_question(q);
        Serial.printf("[EXAM_PREP] tier=%u mastered=%u/70 question=%d ordinal=%u.\n",
                      (unsigned)(master ? 8u : tier), (unsigned)total, q->id, (unsigned)ordinal);
        return q;
    }

    // Defensive recovery if persistence changed between reads: advance tier on next load.
    player_data_set_quiz_seq(profile, AHAVA_SUBJECT_EXAM_PREP, seq + span);
    return nullptr;
}

'''
replace_once("src/engine/quiz_engine.cpp", insert_marker, exam_selector + insert_marker)
replace_once(
    "src/engine/quiz_engine.cpp",
    '''    if (subject_id == AHAVA_SUBJECT_CHALLENGES && profile != PROFILE_ETHAN) return nullptr;

    if (daily_limit_reached(profile, subject_id)) {
''',
    '''    if (subject_id == AHAVA_SUBJECT_CHALLENGES && profile != PROFILE_ETHAN) return nullptr;
    if (subject_id == AHAVA_SUBJECT_EXAM_PREP) {
        return profile == PROFILE_ORI ? next_exam_prep_question(profile) : nullptr;
    }

    if (daily_limit_reached(profile, subject_id)) {
''',
)

# Existing generic selection assignments must route through activation.
replace_once(
    "src/engine/quiz_engine.cpp",
    '''                active_question = retry_q;
                hinted_question_id = -1;
''',
    '''                activate_question(retry_q);
''',
)
replace_once(
    "src/engine/quiz_engine.cpp",
    '''        active_question = q;
        hinted_question_id = -1;
''',
    '''        activate_question(q);
''',
)
replace_once(
    "src/engine/quiz_engine.cpp",
    '''    if (subject_id == AHAVA_SUBJECT_CHALLENGES && profile != PROFILE_ETHAN) return 0;
    size_t count = 0;
''',
    '''    if (subject_id == AHAVA_SUBJECT_CHALLENGES && profile != PROFILE_ETHAN) return 0;
    if (subject_id == AHAVA_SUBJECT_EXAM_PREP && profile != PROFILE_ORI) return 0;
    size_t count = 0;
''',
)

# Public work-phase API before event registration.
api_marker = "lv_event_dsc_t* quiz_register_event_cb(lv_obj_t *obj, lv_event_cb_t cb,"
api_block = r'''
QuizPhase_t quiz_get_phase(void) {
    if (!active_question || active_question->subject_id != AHAVA_SUBJECT_EXAM_PREP) return QUIZ_PHASE_FINAL;
    if (exam_work_state.phase == EXAM_WORK_1) return QUIZ_PHASE_WORK_1;
    if (exam_work_state.phase == EXAM_WORK_2) return QUIZ_PHASE_WORK_2;
    return QUIZ_PHASE_FINAL;
}

bool quiz_check_work_choice(uint8_t selected_idx) {
    if (!active_question || active_question->subject_id != AHAVA_SUBJECT_EXAM_PREP || selected_idx > 3) return false;
    return exam_prep_workflow_choose(&exam_work_state, selected_idx);
}

const char* quiz_get_active_answer_text(uint8_t index) {
    if (!active_question || index > 3) return "";
    if (quiz_get_phase() == QUIZ_PHASE_WORK_1) return active_question->work_answers_1[index];
    if (quiz_get_phase() == QUIZ_PHASE_WORK_2) return active_question->work_answers_2[index];
    return active_question->answers[index];
}

const char* quiz_get_active_work_prompt(void) {
    if (!active_question) return "";
    if (quiz_get_phase() == QUIZ_PHASE_WORK_1) return active_question->work_prompt_1 ? active_question->work_prompt_1 : "";
    if (quiz_get_phase() == QUIZ_PHASE_WORK_2) return active_question->work_prompt_2 ? active_question->work_prompt_2 : "";
    return "בחר את התשובה הסופית המצומצמת.";
}

'''
replace_once("src/engine/quiz_engine.cpp", api_marker, api_block + api_marker)

# ---------------------------------------------------------------------------
# 4) UI: Ori-only dashboard card, mastery HUD, mandatory work-step rendering.
# ---------------------------------------------------------------------------
replace_once(
    "src/ui/screen_manager.cpp",
    'static int current_subject_id = 0;\n',
    'static int current_subject_id = 0;\nstatic lv_obj_t *quiz_question_lbl = NULL;\n',
)

# Helpers before answer handler.
answer_marker = "static void on_answer_clicked(lv_event_t *e) {"
ui_helpers = r'''
static void format_exam_prep_progress(char *buf, size_t len) {
    const uint16_t total = player_data_exam_prep_total_solved(PROFILE_ORI);
    const uint8_t tier = player_data_exam_prep_current_tier(PROFILE_ORI);
    if (total >= 70 || tier > 7) {
        snprintf(buf, len, "מאסטר · %u/70", (unsigned)total);
    } else {
        snprintf(buf, len, "רמה %u · %u/10 · %u/70", (unsigned)tier,
                 (unsigned)player_data_exam_prep_tier_solved(PROFILE_ORI, tier),
                 (unsigned)total);
    }
}

static void refresh_exam_prep_quiz_ui(void) {
    if (!current_quiz_question || current_subject_id != AHAVA_SUBJECT_EXAM_PREP) return;
    if (quiz_question_lbl && lv_obj_is_valid(quiz_question_lbl)) {
        char text[512];
        snprintf(text, sizeof(text), "רמה %u · %s\n✍️ כתוב דרך מלאה במחברת לפני בחירת תשובה.\n%s",
                 (unsigned)current_quiz_question->difficulty_tier,
                 current_quiz_question->text,
                 quiz_get_active_work_prompt());
        lv_label_set_text(quiz_question_lbl, text);
    }
    for (int i = 0; i < 4; ++i) {
        if (!quiz_answer_btns[i] || !lv_obj_is_valid(quiz_answer_btns[i])) continue;
        theme_apply_btn_main(quiz_answer_btns[i]);
        lv_obj_add_flag(quiz_answer_btns[i], LV_OBJ_FLAG_CLICKABLE);
        lv_obj_t *label = lv_obj_get_child(quiz_answer_btns[i], 0);
        if (label) lv_label_set_text(label, quiz_get_active_answer_text((uint8_t)i));
    }
    if (s_active_hud_today_lbl && lv_obj_is_valid(s_active_hud_today_lbl)) {
        char progress[64];
        format_exam_prep_progress(progress, sizeof(progress));
        lv_label_set_text(s_active_hud_today_lbl, progress);
    }
}

'''
replace_once("src/ui/screen_manager.cpp", answer_marker, ui_helpers + answer_marker)

# Work checkpoint branch before normal correctness/finalization.
replace_once(
    "src/ui/screen_manager.cpp",
    '''    uint8_t clicked_idx = (uint8_t)(uintptr_t)lv_event_get_user_data(e);
    lv_obj_t *clicked_btn = (lv_obj_t *)lv_event_get_target(e);
    bool is_correct = (clicked_idx == current_quiz_question->correct_idx);
''',
    '''    uint8_t clicked_idx = (uint8_t)(uintptr_t)lv_event_get_user_data(e);
    lv_obj_t *clicked_btn = (lv_obj_t *)lv_event_get_target(e);

    if (current_subject_id == AHAVA_SUBJECT_EXAM_PREP && quiz_get_phase() != QUIZ_PHASE_FINAL) {
        const bool work_ok = quiz_check_work_choice(clicked_idx);
        if (!work_ok) {
            audio_play_fail();
            if (clicked_btn) {
                lv_obj_set_style_bg_color(clicked_btn, lv_color_hex(0xEF4444), LV_PART_MAIN);
                lv_obj_set_style_border_color(clicked_btn, lv_color_hex(0xF87171), LV_PART_MAIN);
            }
            return;
        }
        audio_play_click();
        refresh_exam_prep_quiz_ui();
        return;
    }

    bool is_correct = (clicked_idx == current_quiz_question->correct_idx);
''',
)

# Final answer progress display remains exam progress, never academic daily count.
replace_once(
    "src/ui/screen_manager.cpp",
    '''    if (s_active_hud_today_lbl && lv_obj_is_valid(s_active_hud_today_lbl)) {
        char today_buf[48];
        snprintf(today_buf, sizeof(today_buf), "%u 🎯", (unsigned int)today_count);
        lv_label_set_text(s_active_hud_today_lbl, today_buf);
    }
''',
    '''    if (s_active_hud_today_lbl && lv_obj_is_valid(s_active_hud_today_lbl)) {
        char today_buf[64];
        if (current_subject_id == AHAVA_SUBJECT_EXAM_PREP) {
            format_exam_prep_progress(today_buf, sizeof(today_buf));
        } else {
            snprintf(today_buf, sizeof(today_buf), "%u 🎯", (unsigned int)today_count);
        }
        lv_label_set_text(s_active_hud_today_lbl, today_buf);
    }
''',
)

# Initial quiz HUD progress.
replace_once(
    "src/ui/screen_manager.cpp",
    '''    uint32_t questions_today = player_data_get_questions_today(current_profile);
    snprintf(today_buf, sizeof(today_buf), "%u 🎯", (unsigned int)questions_today);
''',
    '''    uint32_t questions_today = player_data_get_questions_today(current_profile);
    if (current_subject_id == AHAVA_SUBJECT_EXAM_PREP) {
        format_exam_prep_progress(today_buf, sizeof(today_buf));
    } else {
        snprintf(today_buf, sizeof(today_buf), "%u 🎯", (unsigned int)questions_today);
    }
''',
)

# Capture question label and initialize exam prompt.
replace_once(
    "src/ui/screen_manager.cpp",
    '''    lv_obj_t *q_label = lv_label_create(question_card);
    lv_label_set_text(q_label, current_quiz_question ? current_quiz_question->text : "טוען שאלה...");
''',
    '''    lv_obj_t *q_label = lv_label_create(question_card);
    quiz_question_lbl = q_label;
    lv_label_set_text(q_label, current_quiz_question ? current_quiz_question->text : "טוען שאלה...");
''',
)
replace_once(
    "src/ui/screen_manager.cpp",
    '''        lv_label_set_text(btn_lbl, current_quiz_question ? current_quiz_question->answers[i] : "");
''',
    '''        lv_label_set_text(btn_lbl, current_quiz_question ? quiz_get_active_answer_text((uint8_t)i) : "");
''',
)
# Call refresh after all four button handles exist.
replace_once(
    "src/ui/screen_manager.cpp",
    '''        quiz_answer_btns[i] = btn;
    }
}

/* ========================================================================== */
/*                         SCREEN BUILDER: PROFILES''',
    '''        quiz_answer_btns[i] = btn;
    }
    if (current_subject_id == AHAVA_SUBJECT_EXAM_PREP) refresh_exam_prep_quiz_ui();
}

/* ========================================================================== */
/*                         SCREEN BUILDER: PROFILES''',
)

# Ethan remains 5 navigation categories; Ori gets explicit non-contiguous exam card.
replace_once(
    "src/ui/screen_manager.cpp",
    '        subject_count = AHAVA_SUBJECT_COUNT;\n',
    '        subject_count = AHAVA_SUBJECT_CHALLENGES + 1;\n',
)

system_marker = '''    /* ---------------------------------------------------------------------- */
    /* 3. Card 5: System & Settings Category ("מערכת והגדרות ⚙️")             */
'''
exam_card = r'''
    if (current_profile == PROFILE_ORI) {
        lv_obj_t *exam_card = lv_obj_create(scroll);
        theme_apply_card(exam_card);
        lv_obj_set_size(exam_card, 290, 115);
        lv_obj_set_style_border_color(exam_card, lv_color_hex(0xF59E0B), LV_PART_MAIN);
        lv_obj_remove_flag(exam_card, LV_OBJ_FLAG_SCROLLABLE);

        lv_obj_t *exam_title = lv_label_create(exam_card);
        lv_label_set_text(exam_title, "הכנה למבחן");
        lv_obj_set_style_text_font(exam_title, &lv_font_hebrew_24, LV_PART_MAIN);
        lv_obj_set_style_text_color(exam_title, lv_color_hex(0xFFFFFF), LV_PART_MAIN);
        lv_obj_set_style_base_dir(exam_title, LV_BASE_DIR_RTL, LV_PART_MAIN);
        lv_obj_align(exam_title, LV_ALIGN_TOP_RIGHT, 0, 0);

        lv_obj_t *exam_progress = lv_label_create(exam_card);
        char progress[64];
        format_exam_prep_progress(progress, sizeof(progress));
        lv_label_set_text(exam_progress, progress);
        lv_obj_set_style_text_font(exam_progress, &lv_font_hebrew_16, LV_PART_MAIN);
        lv_obj_set_style_text_color(exam_progress, lv_color_hex(0xFBBF24), LV_PART_MAIN);
        lv_obj_set_style_base_dir(exam_progress, LV_BASE_DIR_RTL, LV_PART_MAIN);
        lv_obj_align(exam_progress, LV_ALIGN_TOP_RIGHT, 0, 30);

        lv_obj_t *exam_btn = lv_button_create(exam_card);
        theme_apply_btn_main(exam_btn);
        lv_obj_set_size(exam_btn, 120, 40);
        lv_obj_align(exam_btn, LV_ALIGN_BOTTOM_MID, 0, 0);
        lv_obj_add_event_cb(exam_btn, on_play_subject_clicked, LV_EVENT_CLICKED,
                            (void*)(uintptr_t)AHAVA_SUBJECT_EXAM_PREP);
        lv_obj_t *exam_lbl = lv_label_create(exam_btn);
        lv_label_set_text(exam_lbl, "תרגל");
        lv_obj_set_style_text_font(exam_lbl, &lv_font_hebrew_24, LV_PART_MAIN);
        lv_obj_center(exam_lbl);
    }

'''
replace_once("src/ui/screen_manager.cpp", system_marker, exam_card + system_marker)

# Live HUD must not overwrite exam progress every three seconds.
replace_once(
    "src/ui/screen_manager.cpp",
    '''        if (current_screen_id == SCREEN_QUIZ) {
            snprintf(today_buf, sizeof(today_buf), "%u 🎯", (unsigned int)questions_today);
        } else {
''',
    '''        if (current_screen_id == SCREEN_QUIZ && current_subject_id == AHAVA_SUBJECT_EXAM_PREP) {
            format_exam_prep_progress(today_buf, sizeof(today_buf));
        } else if (current_screen_id == SCREEN_QUIZ) {
            snprintf(today_buf, sizeof(today_buf), "%u 🎯", (unsigned int)questions_today);
        } else {
''',
)

# Reset stale quiz handles on transitions.
replace_once(
    "src/ui/screen_manager.cpp",
    '''    s_muse_orb = NULL;
    s_muse_wifi_lbl = NULL;
''',
    '''    s_muse_orb = NULL;
    s_muse_wifi_lbl = NULL;
    quiz_question_lbl = NULL;
    for (int i = 0; i < 4; ++i) quiz_answer_btns[i] = NULL;
''',
)

# ---------------------------------------------------------------------------
# 5) Preview: add explicit exam-prep representative samples without touching
#    the production layout contract.
# ---------------------------------------------------------------------------
preview = read("preview/index.html")
preview = preview.replace(
    '<div class="toolbar"><h1>Ahava ESP32 — Preview לפני OTA</h1><div class="meta">Eitan Wizard Challenges • 1,152 questions • TimeService preview</div></div>',
    '<div class="toolbar"><h1>Ahava ESP32 — Preview לפני OTA</h1><div class="meta">Exam Prep: 70 questions • 7 levels • zero Time+ credit</div></div>'
)
preview = preview.replace(
    '<div class="note">ה־Preview מדמה את מידות המכשיר:',
    '<div class="note"><b>הכנה למבחן — RTL QA:</b> רמה 1: 6 × 3/8 · רמה 5: 48 × 12 7/18 · רמה 7: 192 × 57 53/72 · בכל שאלה מוצג שלב דרך לפני תשובה סופית.</div>\n<div class="note">ה־Preview מדמה את מידות המכשיר:'
)
write("preview/index.html", preview)

print("Exam-prep integration patch applied successfully.")
