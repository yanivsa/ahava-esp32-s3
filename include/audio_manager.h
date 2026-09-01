/**
 * @file audio_manager.h
 * @brief Wizard Academy (אקדמיית הקוסמים) I2S Audio Manager & Non-blocking FreeRTOS Audio Task
 */

#pragma once

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Initialize the I2S hardware driver and launch the FreeRTOS audio worker task on Core 0.
 * @return true on success, false otherwise.
 */
bool audio_manager_init(void);

/**
 * @brief Trigger a short, crisp UI click/tick sound (non-blocking).
 */
void audio_play_click(void);

/**
 * @brief Trigger a magical ascending arpeggio for correct answers (non-blocking).
 */
void audio_play_success(void);

/**
 * @brief Trigger a low buzzer sound for incorrect answers (non-blocking).
 */
void audio_play_fail(void);

/**
 * @brief Play a raw 16kHz 16-bit Mono PCM speech buffer from Flash (non-blocking).
 */
void audio_play_pcm(const uint8_t *data, size_t size);

/**
 * @brief Play a random Hebrew praise voice clip ("כל הכבוד!", "נכון מאוד!", etc.)
 */
void audio_play_voice_success(void);

/**
 * @brief Play a Hebrew retry voice clip ("בוא ננסה שוב")
 */
void audio_play_voice_retry(void);

/**
 * @brief Play a question prompt voice clip matching the given Hebrew text (if available).
 * @return true if a matching voice clip was found and queued, false otherwise.
 */
bool audio_play_voice_prompt(const char *prompt_text);

/**
 * @brief Set master volume percentage (0 - 100).
 */
void audio_set_volume(uint8_t volume_pct);

#ifdef __cplusplus
}
#endif
