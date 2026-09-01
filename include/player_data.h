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

/**
 * @brief Initialize Non-Volatile Storage (NVS) flash partition.
 *        Handles corruption or version mismatch by formatting cleanly.
 * @return true on success, false on fatal failure.
 */
bool player_data_init(void);

/**
 * @brief Get total gold coins for a specific wizard profile.
 * @param profile Target profile (Ori, Ethan, Ayala).
 * @return Coins count (defaults to 0 if not previously saved).
 */
uint32_t player_data_get_coins(WizardProfile_t profile);

/**
 * @brief Add gold coins to a specific wizard profile and commit to NVS.
 * @param profile Target profile.
 * @param amount Coins to add.
 */
void player_data_add_coins(WizardProfile_t profile, uint32_t amount);

/**
 * @brief Get total Experience Points (XP) for a specific wizard profile.
 * @param profile Target profile.
 * @return XP count (defaults to 0 if not previously saved).
 */
uint32_t player_data_get_xp(WizardProfile_t profile);

/**
 * @brief Add XP to a specific wizard profile and commit to NVS.
 * @param profile Target profile.
 * @param amount XP to add.
 */
void player_data_add_xp(WizardProfile_t profile, uint32_t amount);

/**
 * @brief Get total questions answered today by a specific child profile.
 * @param profile Target profile.
 * @return Number of questions answered today.
 */
uint32_t player_data_get_questions_today(WizardProfile_t profile);

/**
 * @brief Increment questions answered today for a specific child profile.
 * @param profile Target profile.
 * @return Updated count of questions answered today.
 */
uint32_t player_data_increment_questions_today(WizardProfile_t profile);

/**
 * @brief Reset progress for all child profiles (for debugging/testing).
 */
void player_data_reset_all(void);

#ifdef __cplusplus
}
#endif
