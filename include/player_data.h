/**
 * @file player_data.h
 * @brief Wizard Academy (אקדמיית הקוסמים) Persistent Player Data (NVS)
 */

#pragma once

#include <stdint.h>
#include <stdbool.h>
#include "screen_manager.h"

#ifdef __cplusplus
extern "C" {
#endif

bool player_data_init(void);

uint32_t player_data_get_coins(WizardProfile_t profile);
void player_data_add_coins(WizardProfile_t profile, uint32_t amount);
uint32_t player_data_get_xp(WizardProfile_t profile);
void player_data_add_xp(WizardProfile_t profile, uint32_t amount);

/** Total correctly-completed questions today for the profile. */
uint32_t player_data_get_questions_today(WizardProfile_t profile);

/** Correctly-completed questions today for one subject (0..3). */
uint32_t player_data_get_subject_questions_today(WizardProfile_t profile, int subject_id);

/**
 * Prepare the next legacy increment call with the answer result.
 * eligible=true only for a correct answer on the first attempt.
 */
void player_data_prepare_question_count(WizardProfile_t profile, int subject_id, bool eligible);

/**
 * Consumes the prepared result. If the answer was not eligible, the count is
 * returned unchanged. Kept under the legacy name because screen_manager.cpp
 * already calls this function after every finalized answer.
 */
uint32_t player_data_increment_questions_today(WizardProfile_t profile);

/** Persistent retry markers, indexed by the question ordinal within a subject. */
void player_data_retry_set(WizardProfile_t profile, int subject_id, uint16_t ordinal, bool pending);
int player_data_retry_find_next(WizardProfile_t profile, int subject_id,
                                uint16_t start_ordinal, uint16_t question_count);

void player_data_reset_all(void);
void player_data_sync_time(void);
bool player_data_is_time_synced(void);
uint32_t player_data_get_current_date(void);

#ifdef __cplusplus
}
#endif
