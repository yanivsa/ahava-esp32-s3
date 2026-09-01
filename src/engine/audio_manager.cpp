/**
 * @file audio_manager.cpp
 * @brief Wizard Academy I2S Audio Synthesizer & FreeRTOS Non-Blocking Task
 */

#include "audio_manager.h"
#include "bsp_config.h"
#include "voice_assets.h"
#include <Arduino.h>
#include <math.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "driver/i2s.h"

/* Audio Command Queue Definitions */
typedef enum {
    AUDIO_CMD_CLICK = 1,
    AUDIO_CMD_SUCCESS,
    AUDIO_CMD_FAIL,
    AUDIO_CMD_VOICE
} AudioCommandType_t;

typedef struct {
    AudioCommandType_t type;
    const uint8_t *pcm_data;
    size_t pcm_size;
} AudioMsg_t;

static QueueHandle_t audio_queue = NULL;
static TaskHandle_t audio_task_handle = NULL;
static uint8_t master_volume = 25; // Gentle 25% default volume (study-friendly)

/* ========================================================================== */
/*                         SYNTHESIS & I2S PLAYBACK                           */
/* ========================================================================== */

static void play_tone(float freq_hz, uint32_t duration_ms, float gain, bool fade_out) {
    if (freq_hz <= 0 || duration_ms == 0) return;

    float volume = (master_volume / 100.0f) * gain;
    uint32_t total_samples = (BSP_I2S_SAMPLE_RATE * duration_ms) / 1000;
    float phase_increment = (2.0f * (float)M_PI * freq_hz) / (float)BSP_I2S_SAMPLE_RATE;
    float phase = 0.0f;

    const size_t CHUNK_SAMPLES = 256;
    int16_t buffer[CHUNK_SAMPLES * 2]; // 16-bit Stereo (L, R)

    uint32_t samples_generated = 0;
    while (samples_generated < total_samples) {
        uint32_t to_generate = total_samples - samples_generated;
        if (to_generate > CHUNK_SAMPLES) to_generate = CHUNK_SAMPLES;

        for (uint32_t i = 0; i < to_generate; i++) {
            uint32_t current_idx = samples_generated + i;
            float current_vol = volume;

            // Smooth attack envelope: 5ms
            uint32_t attack_samples = (BSP_I2S_SAMPLE_RATE * 5) / 1000;
            if (current_idx < attack_samples) {
                current_vol *= ((float)current_idx / (float)attack_samples);
            } else if (fade_out) {
                float progress = (float)current_idx / (float)total_samples;
                current_vol *= (1.0f - progress) * (1.0f - progress);
            }

            int16_t sample = (int16_t)(sinf(phase) * current_vol * 32767.0f);
            buffer[i * 2]     = sample; // Left Channel
            buffer[i * 2 + 1] = sample; // Right Channel

            phase += phase_increment;
            if (phase >= 2.0f * (float)M_PI) {
                phase -= 2.0f * (float)M_PI;
            }
        }

        size_t bytes_written = 0;
        i2s_write(BSP_I2S_NUM, buffer, to_generate * 2 * sizeof(int16_t), &bytes_written, portMAX_DELAY);
        samples_generated += to_generate;
    }
}

static void play_silence(uint32_t duration_ms) {
    uint32_t total_samples = (BSP_I2S_SAMPLE_RATE * duration_ms) / 1000;
    const size_t CHUNK_SAMPLES = 128;
    int16_t buffer[CHUNK_SAMPLES * 2] = {0};

    uint32_t sent = 0;
    while (sent < total_samples) {
        uint32_t to_send = total_samples - sent;
        if (to_send > CHUNK_SAMPLES) to_send = CHUNK_SAMPLES;
        size_t bytes_written = 0;
        i2s_write(BSP_I2S_NUM, buffer, to_send * 2 * sizeof(int16_t), &bytes_written, portMAX_DELAY);
        sent += to_send;
    }
}

static void play_pcm_stream(const uint8_t *pcm_data, size_t pcm_bytes) {
    if (!pcm_data || pcm_bytes < 2) return;

    float vol = master_volume / 100.0f;
    const size_t CHUNK_SAMPLES = 256;
    int16_t buffer[CHUNK_SAMPLES * 2]; // 16-bit Stereo (L, R)

    const int16_t *samples = (const int16_t *)pcm_data;
    size_t total_samples = pcm_bytes / sizeof(int16_t);
    size_t samples_sent = 0;

    while (samples_sent < total_samples) {
        size_t to_send = total_samples - samples_sent;
        if (to_send > CHUNK_SAMPLES) to_send = CHUNK_SAMPLES;

        for (size_t i = 0; i < to_send; i++) {
            int16_t s = samples[samples_sent + i];
            int16_t scaled = (int16_t)(s * vol);
            buffer[i * 2]     = scaled; // Left Channel
            buffer[i * 2 + 1] = scaled; // Right Channel
        }

        size_t bytes_written = 0;
        i2s_write(BSP_I2S_NUM, buffer, to_send * 2 * sizeof(int16_t), &bytes_written, portMAX_DELAY);
        samples_sent += to_send;
    }

    play_silence(20);
}

/* ========================================================================== */
/*                         SOUND EFFECTS COMPOSITIONS                         */
/* ========================================================================== */

static void render_sound_click(void) {
    // Subtle, soft micro-tick for UI touches
    play_tone(900.0f, 15, 0.25f, true);
    play_silence(10);
}

static void render_sound_success(void) {
    // Gentle music box / marimba chime: E5 -> A5 -> C#6 (Major chord, warm and quiet)
    play_tone(659.25f, 60, 0.35f, true);  // E5
    play_tone(880.00f, 65, 0.40f, true);  // A5
    play_tone(1108.73f, 130, 0.45f, true); // C#6 (warm resolved ring)
    play_silence(20);
}

static void render_sound_fail(void) {
    // Gentle, soft low woodblock tap (non-punishing, discreet)
    play_tone(240.0f, 35, 0.30f, true);
    play_silence(15);
    play_tone(190.0f, 45, 0.25f, true);
    play_silence(20);
}

/* ========================================================================== */
/*                         FREERTOS AUDIO TASK                                */
/* ========================================================================== */

static void audio_task_worker(void *pvParameters) {
    (void)pvParameters;
    AudioMsg_t msg;

    Serial.printf("[AUDIO] FreeRTOS task started on Core %d\n", xPortGetCoreID());

    while (1) {
        // Block indefinitely until an audio event is queued
        if (xQueueReceive(audio_queue, &msg, portMAX_DELAY) == pdTRUE) {
            switch (msg.type) {
                case AUDIO_CMD_CLICK:
                    render_sound_click();
                    break;
                case AUDIO_CMD_SUCCESS:
                    render_sound_success();
                    break;
                case AUDIO_CMD_FAIL:
                    render_sound_fail();
                    break;
                case AUDIO_CMD_VOICE:
                    if (msg.pcm_data && msg.pcm_size > 0) {
                        play_pcm_stream(msg.pcm_data, msg.pcm_size);
                    }
                    break;
                default:
                    break;
            }
        }
    }
}

/* ========================================================================== */
/*                         PUBLIC API IMPLEMENTATION                          */
/* ========================================================================== */

bool audio_manager_init(void) {
    // 1. Create Command Queue
    audio_queue = xQueueCreate(10, sizeof(AudioMsg_t));
    if (!audio_queue) {
        Serial.println("[AUDIO] ERROR: Failed to create audio queue!");
        return false;
    }

    // 2. Configure I2S Driver (Legacy standard ESP-IDF)
    i2s_config_t i2s_config = {
        .mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_TX),
        .sample_rate = BSP_I2S_SAMPLE_RATE,
        .bits_per_sample = I2S_BITS_PER_SAMPLE_16BIT,
        .channel_format = I2S_CHANNEL_FMT_RIGHT_LEFT,
        .communication_format = I2S_COMM_FORMAT_STAND_I2S,
        .intr_alloc_flags = ESP_INTR_FLAG_LEVEL1,
        .dma_buf_count = 4,
        .dma_buf_len = 256,
        .use_apll = false,
        .tx_desc_auto_clear = true,
        .fixed_mclk = 0
    };

    i2s_pin_config_t pin_config = {
        .bck_io_num = BSP_I2S_BCLK_PIN,
        .ws_io_num = BSP_I2S_LRC_PIN,
        .data_out_num = BSP_I2S_DOUT_PIN,
        .data_in_num = I2S_PIN_NO_CHANGE
    };

    esp_err_t err = i2s_driver_install(BSP_I2S_NUM, &i2s_config, 0, NULL);
    if (err != ESP_OK) {
        Serial.printf("[AUDIO] ERROR: i2s_driver_install failed (0x%x)\n", err);
        return false;
    }

    err = i2s_set_pin(BSP_I2S_NUM, &pin_config);
    if (err != ESP_OK) {
        Serial.printf("[AUDIO] ERROR: i2s_set_pin failed (0x%x)\n", err);
        return false;
    }

    i2s_zero_dma_buffer(BSP_I2S_NUM);

    // 3. Launch Audio Task pinned to Core 0
    BaseType_t task_status = xTaskCreatePinnedToCore(
        audio_task_worker,
        BSP_AUDIO_TASK_NAME,
        BSP_AUDIO_TASK_STACK_SIZE,
        NULL,
        BSP_AUDIO_TASK_PRIORITY,
        &audio_task_handle,
        BSP_AUDIO_TASK_CORE_ID
    );

    if (task_status != pdPASS) {
        Serial.println("[AUDIO] ERROR: Failed to launch FreeRTOS audio task!");
        return false;
    }

    Serial.printf("[AUDIO] I2S initialized on BCLK=%d, LRC=%d, DOUT=%d (SampleRate: %d Hz)\n",
                  BSP_I2S_BCLK_PIN, BSP_I2S_LRC_PIN, BSP_I2S_DOUT_PIN, BSP_I2S_SAMPLE_RATE);
    return true;
}

void audio_play_click(void) {
    if (!audio_queue) return;
    AudioMsg_t msg = { .type = AUDIO_CMD_CLICK, .pcm_data = NULL, .pcm_size = 0 };
    xQueueSend(audio_queue, &msg, 0); // Non-blocking
}

void audio_play_success(void) {
    if (!audio_queue) return;
    AudioMsg_t msg = { .type = AUDIO_CMD_SUCCESS, .pcm_data = NULL, .pcm_size = 0 };
    xQueueSend(audio_queue, &msg, 0);
}

void audio_play_fail(void) {
    if (!audio_queue) return;
    AudioMsg_t msg = { .type = AUDIO_CMD_FAIL, .pcm_data = NULL, .pcm_size = 0 };
    xQueueSend(audio_queue, &msg, 0);
}

void audio_play_pcm(const uint8_t *data, size_t size) {
    if (!audio_queue || !data || size == 0) return;
    AudioMsg_t msg = { .type = AUDIO_CMD_VOICE, .pcm_data = data, .pcm_size = size };
    xQueueSend(audio_queue, &msg, 0);
}

void audio_play_voice_success(void) {
    if (!audio_queue) return;
    // Pick a random praise clip
    static uint32_t praise_idx = 0;
    uint32_t idx = (praise_idx++) % VOICE_SUCCESS_COUNT;
    audio_play_pcm(VOICE_SUCCESS_CLIPS[idx].data, VOICE_SUCCESS_CLIPS[idx].size);
}

void audio_play_voice_retry(void) {
    if (!audio_queue) return;
    static uint32_t retry_idx = 0;
    uint32_t idx = (retry_idx++) % VOICE_RETRY_COUNT;
    audio_play_pcm(VOICE_RETRY_CLIPS[idx].data, VOICE_RETRY_CLIPS[idx].size);
}

bool audio_play_voice_prompt(const char *prompt_text) {
    if (!audio_queue || !prompt_text) return false;

    // Search in the ALL_VOICE_CLIPS table
    for (size_t i = 0; i < TOTAL_VOICE_CLIPS; i++) {
        if (strstr(prompt_text, ALL_VOICE_CLIPS[i].prompt) != NULL ||
            strstr(ALL_VOICE_CLIPS[i].prompt, prompt_text) != NULL) {
            audio_play_pcm(ALL_VOICE_CLIPS[i].data, ALL_VOICE_CLIPS[i].size);
            return true;
        }
    }
    return false;
}

void audio_set_volume(uint8_t volume_pct) {
    if (volume_pct > 100) volume_pct = 100;
    master_volume = volume_pct;
}
