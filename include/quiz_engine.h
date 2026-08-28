/**
 * @file quiz_engine.h
 * @brief Wizard Academy (אקדמיית הקוסמים) Static Quiz Engine & question UI helpers
 */

#pragma once

#include <stdint.h>
#include <stdbool.h>
#include "lvgl.h"
#include "screen_manager.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    int id;                          /**< Unique Question ID */
    int subject_id;                  /**< 0 Math, 1 Hebrew, 2 English, 3 Judaism */
    WizardProfile_t target_profile;  /**< Ori, Ethan, Ayala */
    const char *text;                /**< Question prompt */
    const char *answers[4];          /**< Four multiple-choice options */
    uint8_t correct_idx;             /**< 0-3 index of the correct answer */
    const char *feedback;            /**< Final explanation after the question ends */
    const char *hint;                /**< First-error hint; should not reveal the answer */
} Question_t;

const Question_t* quiz_get_next_question(WizardProfile_t profile, int subject_id);
bool quiz_validate_database(void);
size_t quiz_get_total_questions(void);
size_t quiz_get_question_count(WizardProfile_t profile, int subject_id);

/**
 * Register an LVGL event callback. Answer-button callbacks are proxied so a
 * first wrong answer shows q->hint and keeps the same question active. All
 * other callbacks are forwarded unchanged.
 */
lv_event_dsc_t* quiz_register_event_cb(lv_obj_t *obj, lv_event_cb_t cb,
                                      lv_event_code_t filter, void *user_data,
                                      const char *callback_name);

#ifdef __cplusplus
}
#endif

/*
 * screen_manager.cpp already uses lv_obj_add_event_cb everywhere. Wrapping the
 * registration here lets the quiz add first-error hints without duplicating
 * the screen manager. quiz_engine.cpp undefines this macro before it calls the
 * real LVGL function.
 */
#ifndef QUIZ_ENGINE_DISABLE_EVENT_PROXY
#define lv_obj_add_event_cb(obj, cb, filter, user_data) \
    quiz_register_event_cb((obj), (cb), (filter), (user_data), #cb)
#endif
