/**
 * @file player_data.h
 * @brief Wizard Academy (אקדמיית הקוסמים) Persistent Player Data (NVS)
 */

#pragma once

#include <stdint.h>
#include "subjects.h"
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
uint32_t player_data_get_questions_today(WizardProfile_t profile);
uint32_t player_data_get_subject_questions_today(WizardProfile_t profile, int subject_id);
void player_data_prepare_question_count(WizardProfile_t profile, int subject_id, int stats_subject_id, bool eligible);
uint32_t player_data_increment_questions_today(WizardProfile_t profile);
void player_data_retry_set(WizardProfile_t profile, int subject_id, uint16_t ordinal, bool pending);
int player_data_retry_find_next(WizardProfile_t profile, int subject_id,
                                uint16_t start_ordinal, uint16_t question_count);
uint32_t player_data_get_quiz_seq(WizardProfile_t profile, int subject_id);
void player_data_set_quiz_seq(WizardProfile_t profile, int subject_id, uint32_t seq);
uint32_t player_data_get_quiz_seed(WizardProfile_t profile, int subject_id);
void player_data_set_quiz_seed(WizardProfile_t profile, int subject_id, uint32_t seed);
WizardProfile_t player_data_get_active_profile(void);
void player_data_set_active_profile(WizardProfile_t profile);

bool player_data_exam_prep_is_solved(WizardProfile_t profile, uint16_t ordinal);
void player_data_exam_prep_mark_solved(WizardProfile_t profile, uint16_t ordinal);
uint16_t player_data_exam_prep_total_solved(WizardProfile_t profile);
uint8_t player_data_exam_prep_current_tier(WizardProfile_t profile);
uint8_t player_data_exam_prep_tier_solved(WizardProfile_t profile, uint8_t tier);

void player_data_reset_all(void);
void player_data_sync_time(void);
bool player_data_is_time_synced(void);
uint32_t player_data_get_current_date(void);

#ifdef __cplusplus
}
#endif
