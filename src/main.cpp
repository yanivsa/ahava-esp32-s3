/**
 * @file main.cpp
 * @brief Application Entry Point for Wizard Academy (ESP32-S3 + FreeRTOS + LVGL 9.x)
 */

#include <Arduino.h>
#include <Wire.h>
#include "bsp_config.h"
#include "hal_lvgl.h"
#include "hal_display.h"
#include "theme_manager.h"
#include "screen_manager.h"
#include "quiz_engine.h"
#include "player_data.h"
#include "audio_manager.h"
#include "ota_manager.h"
#include "esp_heap_caps.h"

/**
 * @brief Background task demonstrating thread-safe LVGL UI updates from non-GUI tasks
 */
static void background_telemetry_task(void *pvParameters) {
    (void)pvParameters;
    uint32_t uptime_sec = 0;

    while (1) {
        vTaskDelay(pdMS_TO_TICKS(5000));
        uptime_sec += 5;

        Serial.printf("[TELEMETRY] Uptime: %us | Active Profile: %d | Free SRAM: %u KB | Free PSRAM: %u KB\n",
                      uptime_sec,
                      (int)current_profile,
                      (unsigned int)(heap_caps_get_free_size(MALLOC_CAP_INTERNAL) / 1024),
                      (unsigned int)(heap_caps_get_free_size(MALLOC_CAP_SPIRAM) / 1024));
    }
}

void setup() {
    Serial.begin(115200);

    // Wait for USB CDC port to enumerate
    for (int i = 0; i < 20; i++) {
        delay(100);
        if (Serial) break;
    }

    Serial.println("=================================================");
    Serial.println("   WIZARD ACADEMY - ESP32-S3 HAL INIT (LVGL 9)   ");
    Serial.println("=================================================");

    // 1. Validate PSRAM availability
    if (psramFound()) {
        Serial.printf("[SYS] PSRAM initialized: Total %u MB, Free %u MB\n",
                      (unsigned int)(ESP.getPsramSize() / (1024 * 1024)),
                      (unsigned int)(heap_caps_get_free_size(MALLOC_CAP_SPIRAM) / (1024 * 1024)));
    } else {
        Serial.println("[SYS] WARNING: PSRAM init failed or not detected!");
    }

    Serial.printf("[SYS] Internal SRAM Free: %u KB\n",
                  (unsigned int)(heap_caps_get_free_size(MALLOC_CAP_INTERNAL) / 1024));

    if (!quiz_validate_database()) {
        Serial.println("[SYS] FATAL: quiz database validation failed.");
        while (true) delay(1000);
    }

    // 2. Initialize Non-Volatile Storage (NVS) for persistent player stats
    if (!player_data_init()) {
        Serial.println("[SYS] WARN: NVS initialization encountered an issue.");
    }

    // 3. Initialize I2S Audio Synthesizer on Core 0
    #if BSP_AUDIO_ENABLED
        if (!audio_manager_init()) {
            Serial.println("[SYS] WARN: Audio manager initialization failed. Running in silent mode.");
        }
    #else
        Serial.println("[SYS] Audio disabled until the onboard ES8311 codec is initialized.");
    #endif

    // 4. Initialize Wi-Fi & OTA Manager
    #if BSP_OTA_ENABLED
        if (!ota_manager_init()) {
            Serial.println("[SYS] WARN: OTA manager initialization failed.");
        }
    #else
        Serial.println("[SYS] Wi-Fi/OTA disabled until a trusted update endpoint is provisioned.");
    #endif

    #if BSP_DISPLAY_DIAGNOSTIC_MODE
        Serial.println("[SYS] DISPLAY DIAGNOSTIC MODE: game startup is intentionally paused.");
        hal_display_init();
        hal_display_show_diagnostics();
        Serial.println("[SYS] Diagnostic screen will remain visible until the next firmware update.");
        return;
    #endif

    // 5. Initialize LVGL HAL (Core, Buffers, Display, Touch, FreeRTOS GUI Task on Core 1)
    if (!hal_lvgl_init()) {
        Serial.println("[SYS] FATAL: LVGL HAL initialization failed. Halting system.");
        while (1) {
            delay(1000);
        }
    }

    // 6. Initialize Screen Manager and load Profile Selection screen
    if (hal_lvgl_lock(portMAX_DELAY)) {
        sm_init();
        sm_load_screen(SCREEN_PROFILES);
        hal_lvgl_unlock();
    }

    // 7. Spawn background worker/telemetry task on Core 0
    xTaskCreatePinnedToCore(
        background_telemetry_task,
        "bg_telemetry",
        3072,
        NULL,
        1,
        NULL,
        0 // Core 0 (leaving Core 1 dedicated to GUI)
    );

    #if defined(OTA_AUTOTEST_ON_BOOT) && OTA_AUTOTEST_ON_BOOT
        Serial.println("[OTA TEST] Starting one-time automatic OTA test.");
        ota_start_async_update(DEFAULT_OTA_FIRMWARE_URL);
    #endif

    Serial.println("[SYS] Boot complete. FreeRTOS Core 1 handling GUI rendering.");
}

void loop() {
    // Main Arduino loop yields CPU since GUI, Audio, OTA and background tasks run in FreeRTOS tasks
    vTaskDelay(pdMS_TO_TICKS(1000));
}
