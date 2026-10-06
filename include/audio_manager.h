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

/**
 * @brief Get master volume percentage (0 - 100).
 */
uint8_t audio_get_volume(void);

/**
 * @brief Start recording audio from the ES8311 microphone ADC into PSRAM.
 * @return true if recording successfully started, false otherwise.
 */
bool audio_record_start(void);

/**
 * @brief Stop recording audio from the ES8311 microphone ADC.
 * @return Total number of bytes recorded in the PSRAM buffer.
 */
size_t audio_record_stop(void);

/**
 * @brief Get the pointer to the recorded 16kHz 16-bit Mono PCM buffer in PSRAM.
 */
const uint8_t *audio_record_get_buffer(void);

/**
 * @brief Get the number of valid bytes currently in the recording buffer.
 */
size_t audio_record_get_size(void);

/**
 * @brief Check if microphone audio recording is currently in progress.
 */
bool audio_is_recording(void);

/**
 * @brief Check if audio playback (speech/SFX) is currently in progress.
 */
bool audio_is_playing(void);

/**
 * @brief Stop any ongoing audio playback.
 */
void audio_stop(void);

#ifdef __cplusplus
}
#endif
