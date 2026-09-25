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
#include "stats_screen.h"
#include "quiz_engine.h"
#include "player_data.h"
#include "weekly_stats_store.h"
#include "time_service.h"
#include "audio_manager.h"
#include "ota_manager.h"
#include "hal_battery.h"
#include "hal_touch.h"
#include "esp_heap_caps.h"
#include "esp_sleep.h"

#define AUTO_SLEEP_TIMEOUT_MS (5UL * 60UL * 1000UL) // 5 minutes inactivity timeout
#define NIGHT_OTA_HOUR 1                            // 01:00 AM

/**
 * @brief Calculate seconds remaining until next target hour (01:00 AM) local time.
 */
static uint64_t calculate_seconds_until_night_ota(void) {
    time_t now = time(NULL);
    struct tm timeinfo;
    if (localtime_r(&now, &timeinfo) == NULL || (timeinfo.tm_year + 1900 < 2025)) {
        // Time not synced yet, fallback to checking again in 4 hours
        return 4ULL * 3600ULL;
    }

    struct tm target = timeinfo;
    target.tm_hour = NIGHT_OTA_HOUR;
    target.tm_min = 0;
    target.tm_sec = 0;

    time_t target_time = mktime(&target);
    if (target_time <= now) {
        // If 01:00 AM has already passed today, schedule for 01:00 AM tomorrow
        target_time += 24ULL * 3600ULL;
    }

    uint64_t diff = (uint64_t)difftime(target_time, now);
    if (diff < 60) {
        // Avoid immediate loop if waking up right around 01:00
        diff = 24ULL * 3600ULL;
    }
    return diff;
}

/**
 * @brief Enter ultra-low power deep sleep to preserve battery after inactivity
 */
static void enter_power_save_sleep(void) {
    Serial.println("[POWER] Entering Deep Sleep...");

    // 1. Fade/turn off LCD Backlight
    hal_display_set_backlight(0);

    // 2. Mute Audio Amplifier
    audio_set_volume(0);

    // 3. Disconnect and power down Wi-Fi
    ota_wifi_disconnect();

    // 4. Calculate time until next 01:00 AM check and enable timer wakeup
    uint64_t sleep_sec = calculate_seconds_until_night_ota();
    Serial.printf("[POWER] Scheduling next 01:00 AM OTA check in %llu seconds (%llu hours, %llu mins).\n",
                  sleep_sec, sleep_sec / 3600ULL, (sleep_sec % 3600ULL) / 60ULL);
    esp_sleep_enable_timer_wakeup(sleep_sec * 1000000ULL);

    delay(200);

    // 5. Enter Deep Sleep
    esp_deep_sleep_start();
}

/**
 * @brief Background task monitoring telemetry, battery and auto-sleep inactivity
 */
static void background_telemetry_task(void *pvParameters) {
    (void)pvParameters;
    uint32_t uptime_sec = 0;

    while (1) {
        vTaskDelay(pdMS_TO_TICKS(5000));
        uptime_sec += 5;

        uint32_t inactive_ms = hal_touch_get_last_activity_ms();

        Serial.printf("[TELEMETRY] Uptime: %us | Profile: %d | Inactive: %us | Bat: %u mV (%u%%, %s) | Free SRAM: %u KB | Free PSRAM: %u KB\n",
                      uptime_sec,
                      (int)current_profile,
                      (unsigned int)(inactive_ms / 1000),
                      hal_battery_get_voltage_mv(),
                      hal_battery_get_percentage(),
                      hal_battery_is_charging() ? "CHG" : "BAT",
                      (unsigned int)(heap_caps_get_free_size(MALLOC_CAP_INTERNAL) / 1024),
                      (unsigned int)(heap_caps_get_free_size(MALLOC_CAP_SPIRAM) / 1024));

        // Auto-Sleep trigger if inactive for > 5 minutes and no OTA update is in progress
        if (inactive_ms >= AUTO_SLEEP_TIMEOUT_MS) {
            OtaStatus_t ota_st = ota_get_status();
            if (ota_st != OTA_STATUS_DOWNLOADING && ota_st != OTA_STATUS_PORTAL_ACTIVE) {
                enter_power_save_sleep();
            }
        }
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

    // 2. Initialize Non-Volatile Storage (NVS) for persistent player stats.
    // Archive any saved daily bucket before normal getters can roll it over at midnight.
    const bool player_data_ready = player_data_init();
    if (!player_data_ready) {
        Serial.println("[SYS] WARN: NVS initialization encountered an issue.");
    } else {
        time_service_init();
        if (!weekly_stats_store_init()) {
            Serial.println("[SYS] WARN: Weekly statistics history could not be fully initialized.");
        }
    }

    // 2.1. Initialize Hardware Battery & Power Monitor (GPIO 6 / GPIO 7)
    hal_battery_init();

    // 2.2. Check wake-up cause: If waking up via RTC Timer, handle silent night OTA check
    esp_sleep_wakeup_cause_t wakeup_cause = esp_sleep_get_wakeup_cause();
    if (wakeup_cause == ESP_SLEEP_WAKEUP_TIMER) {
        Serial.println("[NIGHT_OTA] Woke up from Deep Sleep via Timer (01:00 AM scheduled check).");

        uint8_t bat_pct = hal_battery_get_percentage();
        Serial.printf("[NIGHT_OTA] Current battery level: %u%%\n", bat_pct);

        // Safety check: preserve battery if under 20%
        if (bat_pct < 20) {
            Serial.println("[NIGHT_OTA] Battery below 20%. Skipping OTA check to protect battery.");
            enter_power_save_sleep();
            return;
        }

        Serial.println("[NIGHT_OTA] Checking for OTA updates silently (screen and audio off)...");
        ota_manager_init();
        ota_perform_silent_check(DEFAULT_OTA_FIRMWARE_URL);

        Serial.println("[NIGHT_OTA] OTA check complete. Returning to Deep Sleep...");
        enter_power_save_sleep();
        return;
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
        } else {
            // Configure Israel timezone (IST-2IDT) and SNTP time sync
            player_data_sync_time();
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

    // 6. Initialize Screen Manager and load active screen
    if (hal_lvgl_lock(portMAX_DELAY)) {
        sm_init();
        if (current_profile != PROFILE_NONE) {
            sm_load_screen(SCREEN_DASHBOARD);
        } else {
            sm_load_screen(SCREEN_PROFILES);
        }
        stats_ui_init();
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
    time_service_poll();
    // Main Arduino loop yields CPU since GUI, Audio, OTA and background tasks run in FreeRTOS tasks.
    vTaskDelay(pdMS_TO_TICKS(1000));
}
