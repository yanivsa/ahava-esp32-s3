/**
 * @file quiz_engine.h
 * @brief Wizard Academy (אקדמיית הקוסמים) Static Quiz Engine & Mock DB
 */

#pragma once

#include <stdint.h>
#include <stdbool.h>
#include "screen_manager.h"

#ifdef __cplusplus
extern "C" {
#endif

/* ========================================================================== */
/*                         QUESTION DATA STRUCTURE                            */
/* ========================================================================== */
typedef struct {
    int id;                          /**< Unique Question ID */
    int subject_id;                  /**< Subject ID (0: Math/Shapes, 1: Hebrew, etc.) */
    WizardProfile_t target_profile;  /**< Target child profile (Ori, Ethan, Ayala) */
    const char *text;                /**< Question prompt in Hebrew */
    const char *answers[4];          /**< 4 Multiple choice options */
    uint8_t correct_idx;             /**< 0-3 index of the correct answer */
    const char *feedback;            /**< Feedback explanation shown to child */
} Question_t;

/* ========================================================================== */
/*                         ENGINE API                                         */
/* ========================================================================== */

/**
 * @brief Retrieve a matching question from the static database.
 * @param profile Active child profile.
 * @param subject_id Chosen subject ID.
 * @return Pointer to read-only Question_t.
 */
const Question_t* quiz_get_next_question(WizardProfile_t profile, int subject_id);

/** Validate all question records before the UI starts. */
bool quiz_validate_database(void);

/**
 * @brief Get total number of questions available in database.
 */
size_t quiz_get_total_questions(void);

/** Number of questions available to one profile, optionally for one subject. */
size_t quiz_get_question_count(WizardProfile_t profile, int subject_id);

#ifdef __cplusplus
}
#endif
