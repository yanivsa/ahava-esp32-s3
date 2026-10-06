/**
 * @file muse_manager.h
 * @brief Muse / Magic Assistant (עוזר קסם) Speech & AI Assistant Engine
 */

#pragma once

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    MUSE_STATE_IDLE = 0,
    MUSE_STATE_RECORDING,
    MUSE_STATE_PROCESSING,
    MUSE_STATE_SPEAKING,
    MUSE_STATE_ERROR
} MuseState_t;

/**
 * @brief Initialize Muse assistant engine and FreeRTOS worker task.
 * @return true on success, false otherwise.
 */
bool muse_manager_init(void);

/**
 * @brief Triggered when the user presses down on the Push-To-Talk button.
 *        Starts hardware audio recording via I2S RX from the ES8311 codec.
 */
void muse_manager_on_ptt_down(void);

/**
 * @brief Triggered when the user releases the Push-To-Talk button.
 *        Stops recording and dispatches the query for processing.
 */
void muse_manager_on_ptt_up(void);

/**
 * @brief Cancel any active recording, processing or speech playback immediately.
 */
void muse_manager_cancel(void);

/**
 * @brief Get the current lifecycle state of the Muse assistant.
 */
MuseState_t muse_manager_get_state(void);

/**
 * @brief Get human-readable Hebrew status description.
 */
const char *muse_manager_get_status_text(void);

/**
 * @brief Get the latest query text in Hebrew.
 */
const char *muse_manager_get_last_query(void);

/**
 * @brief Get the latest response text in Hebrew.
 */
const char *muse_manager_get_last_response(void);

/**
 * @brief Check if a new answer is available to be displayed.
 *        Resets the flag once called.
 */
bool muse_manager_has_new_response(void);

#ifdef __cplusplus
}
#endif
