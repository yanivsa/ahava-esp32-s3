#include "audio_manager.h"
#include "bsp_config.h"
#include "voice_assets.h"
#include <Arduino.h>
#include <Wire.h>
#include <math.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "driver/i2s.h"
#include "driver/gpio.h"

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
static uint8_t master_volume = 75; // 75% default volume (clear & audible for children)

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
/*                         ES8311 HARDWARE CODEC DRIVER                       */
/* ========================================================================== */

static bool es8311_write_reg(uint8_t reg, uint8_t val) {
    Wire.beginTransmission((uint8_t)BSP_ES8311_I2C_ADDR);
    Wire.write(reg);
    Wire.write(val);
    return (Wire.endTransmission() == 0);
}

static uint8_t es8311_read_reg(uint8_t reg) {
    Wire.beginTransmission((uint8_t)BSP_ES8311_I2C_ADDR);
    Wire.write(reg);
    if (Wire.endTransmission(false) != 0) return 0;
    if (Wire.requestFrom((uint8_t)BSP_ES8311_I2C_ADDR, (size_t)1) != 1) return 0;
    return Wire.read();
}

static bool es8311_hardware_init(uint8_t init_vol_pct) {
    // Probe ES8311 presence on the shared I2C bus
    Wire.beginTransmission((uint8_t)BSP_ES8311_I2C_ADDR);
    if (Wire.endTransmission() != 0) {
        Serial.printf("[AUDIO] ES8311 Codec not responding at I2C 0x%02X!\n", BSP_ES8311_I2C_ADDR);
        return false;
    }

    uint8_t id1 = es8311_read_reg(0xFD);
    uint8_t id2 = es8311_read_reg(0xFE);
    Serial.printf("[AUDIO] Found ES8311 Codec (Chip ID: 0x%02X 0x%02X)\n", id1, id2);

    // 1. Reset sequence
    es8311_write_reg(0x00, 0x1F); // Reset digital blocks
    delay(20);
    es8311_write_reg(0x00, 0x00);
    es8311_write_reg(0x00, 0x80); // Release reset, power on CSM

    // 2. Clock configuration: internal MCLK from MCLK pin (GPIO 16)
    es8311_write_reg(0x01, 0x3F); // Enable all clock gates, MCLK pin input
    es8311_write_reg(0x02, 0x00); // pre_div=1, pre_multi=1
    es8311_write_reg(0x03, 0x10); // fs_mode=0, adc_osr=16
    es8311_write_reg(0x04, 0x10); // dac_osr=16
    es8311_write_reg(0x05, 0x00); // adc_div=1, dac_div=1
    es8311_write_reg(0x06, 0x04); // bclk_div=4
    es8311_write_reg(0x07, 0x00); // lrck_h=0
    es8311_write_reg(0x08, 0xFF); // lrck_l=255

    // 3. Serial data port format: 16-bit I2S Slave
    es8311_write_reg(0x09, 0x0C); // 16-bit SDP In (DAC)
    es8311_write_reg(0x0A, 0x0C); // 16-bit SDP Out (ADC)

    // 4. Power up analog circuitry & DAC
    es8311_write_reg(0x0D, 0x01); // Power up analog circuitry
    es8311_write_reg(0x0E, 0x02); // Enable analog PGA, enable ADC modulator
    es8311_write_reg(0x12, 0x00); // Power up DAC
    es8311_write_reg(0x13, 0x10); // Enable output to HP/Line drive
    es8311_write_reg(0x1C, 0x6A); // ADC Equalizer bypass
    es8311_write_reg(0x37, 0x08); // Bypass DAC equalizer

    // 5. Set DAC Volume & Unmute
    uint8_t dac_vol = (uint8_t)(((uint32_t)init_vol_pct * 255) / 100);
    es8311_write_reg(0x32, dac_vol);
    es8311_write_reg(0x31, 0x00); // Unmute DAC

    Serial.printf("[AUDIO] ES8311 initialized successfully (Hardware DAC Volume: %u%% / reg32=0x%02X)\n",
                  init_vol_pct, dac_vol);
    return true;
}

/* ========================================================================== */
/*                         PUBLIC API IMPLEMENTATION                          */
/* ========================================================================== */

bool audio_manager_init(void) {
    // 1. Initialize NS4150B Power Amplifier control pin (GPIO 43)
    pinMode(BSP_PA_PIN, OUTPUT);
    digitalWrite(BSP_PA_PIN, LOW); // Hold in shutdown during boot to prevent clicks

    // 2. Ensure I2C bus is initialized for ES8311 codec
    Wire.begin(BSP_TOUCH_I2C_SDA_PIN, BSP_TOUCH_I2C_SCL_PIN, 400000);

    // 3. Create Command Queue
    audio_queue = xQueueCreate(10, sizeof(AudioMsg_t));
    if (!audio_queue) {
        Serial.println("[AUDIO] ERROR: Failed to create audio queue!");
        return false;
    }

    // 4. Configure I2S Driver (Legacy standard ESP-IDF)
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
        .fixed_mclk = BSP_I2S_SAMPLE_RATE * 256
    };

    i2s_pin_config_t pin_config = {
        .mck_io_num = BSP_I2S_MCLK_PIN, // GPIO 16 (MCLK to ES8311)
        .bck_io_num = BSP_I2S_BCLK_PIN, // GPIO 9 (SCLK/BCLK)
        .ws_io_num = BSP_I2S_LRC_PIN,   // GPIO 45 (LRCK/WS)
        .data_out_num = BSP_I2S_DOUT_PIN, // GPIO 8 (DSDIN/DOUT)
        .data_in_num = BSP_I2S_DIN_PIN    // GPIO 10 (ASDOUT/DIN)
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

    // 5. Initialize ES8311 Hardware Codec via I2C
    if (!es8311_hardware_init(master_volume)) {
        Serial.println("[AUDIO] WARN: ES8311 codec initialization was not confirmed.");
    }

    // 6. Enable NS4150B Power Amplifier (GPIO 43 HIGH)
    digitalWrite(BSP_PA_PIN, HIGH);
    Serial.printf("[AUDIO] NS4150B Power Amplifier enabled on GPIO %d\n", BSP_PA_PIN);

    // 7. Launch Audio Task pinned to Core 0
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

    Serial.printf("[AUDIO] I2S initialized on MCLK=%d, BCLK=%d, LRC=%d, DOUT=%d, PA=%d (SampleRate: %d Hz)\n",
                  BSP_I2S_MCLK_PIN, BSP_I2S_BCLK_PIN, BSP_I2S_LRC_PIN, BSP_I2S_DOUT_PIN, BSP_PA_PIN, BSP_I2S_SAMPLE_RATE);
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

    if (volume_pct == 0) {
        // Mute amplifier and DAC
        digitalWrite(BSP_PA_PIN, LOW);
        es8311_write_reg(0x31, 0x60); // Mute DAC
    } else {
        uint8_t dac_vol = (uint8_t)(((uint32_t)volume_pct * 255) / 100);
        es8311_write_reg(0x32, dac_vol);
        es8311_write_reg(0x31, 0x00); // Unmute DAC
        digitalWrite(BSP_PA_PIN, HIGH);
    }
}

uint8_t audio_get_volume(void) {
    return master_volume;
}
