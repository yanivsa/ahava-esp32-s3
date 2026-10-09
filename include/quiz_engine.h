/**
 * @file quiz_engine.h
 * @brief Wizard Academy (אקדמיית הקוסמים) Static Quiz Engine & question UI helpers
 */

#pragma once

#include <stdint.h>
#include "subjects.h"
#include <stdbool.h>
#include "lvgl.h"
#include "screen_manager.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    int id;
    int subject_id;
    WizardProfile_t target_profile;
    const char *text;
    const char *answers[4];
    uint8_t correct_idx;
    const char *feedback;
    const char *hint;
    int8_t stats_subject_id;
    uint8_t difficulty_tier;          /**< 0 legacy, 1..7 exam prep */
    uint8_t exam_prep_operation;     /**< 0 legacy, 1 fraction×whole, 2 mixed×whole */
    const char *work_prompt_1;
    const char *work_answers_1[4];
    uint8_t work_correct_idx_1;
    const char *work_prompt_2;       /**< Optional second checkpoint */
    const char *work_answers_2[4];
    uint8_t work_correct_idx_2;
} Question_t;

typedef enum {
    QUIZ_PHASE_WORK_1 = 1,
    QUIZ_PHASE_WORK_2 = 2,
    QUIZ_PHASE_FINAL = 3
} QuizPhase_t;

const Question_t* quiz_get_next_question(WizardProfile_t profile, int subject_id);
bool quiz_validate_database(void);
size_t quiz_get_total_questions(void);
size_t quiz_get_question_count(WizardProfile_t profile, int subject_id);
QuizPhase_t quiz_get_phase(void);
bool quiz_check_work_choice(uint8_t selected_idx);
const char* quiz_get_active_answer_text(uint8_t index);
const char* quiz_get_active_work_prompt(void);

lv_event_dsc_t* quiz_register_event_cb(lv_obj_t *obj, lv_event_cb_t cb,
                                      lv_event_code_t filter, void *user_data,
                                      const char *callback_name);

#ifdef __cplusplus
}
#endif

#ifndef QUIZ_ENGINE_DISABLE_EVENT_PROXY
#define lv_obj_add_event_cb(obj, cb, filter, user_data) \
    quiz_register_event_cb((obj), (cb), (filter), (user_data), #cb)
#endif
