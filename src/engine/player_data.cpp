/**
 * @file player_data.cpp
 * @brief Wizard Academy Persistent Player Data Implementation using ESP-IDF NVS
 */

#include "player_data.h"
#include <Arduino.h>
#include "nvs_flash.h"
#include "nvs.h"
#include "esp_err.h"
#include <limits.h>

#define NVS_STORAGE_NAMESPACE "wizard_nvs"

/* ========================================================================== */
/*                         INTERNAL NVS HELPERS                               */
/* ========================================================================== */

static uint32_t nvs_read_u32_val(const char *key, uint32_t default_val) {
    nvs_handle_t handle;
    esp_err_t err = nvs_open(NVS_STORAGE_NAMESPACE, NVS_READONLY, &handle);
    if (err != ESP_OK) {
        return default_val;
    }

    uint32_t value = default_val;
    err = nvs_get_u32(handle, key, &value);
    nvs_close(handle);

    return (err == ESP_OK) ? value : default_val;
}

static bool nvs_write_u32_val(const char *key, uint32_t val) {
    nvs_handle_t handle;
    esp_err_t err = nvs_open(NVS_STORAGE_NAMESPACE, NVS_READWRITE, &handle);
    if (err != ESP_OK) {
        Serial.printf("[NVS] ERROR: Failed to open NVS for writing (err=0x%x)\n", err);
        return false;
    }

    err = nvs_set_u32(handle, key, val);
    if (err == ESP_OK) {
        err = nvs_commit(handle);
    }
    nvs_close(handle);

    return (err == ESP_OK);
}

/* ========================================================================== */
/*                         PUBLIC API IMPLEMENTATION                          */
/* ========================================================================== */

bool player_data_init(void) {
    esp_err_t err = nvs_flash_init();
    if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        Serial.println("[NVS] WARN: NVS partition corrupted or updated. Formatting...");
        esp_err_t erase_err = nvs_flash_erase();
        if (erase_err != ESP_OK) {
            Serial.printf("[NVS] ERROR: nvs_flash_erase failed (0x%x)\n", erase_err);
            return false;
        }
        err = nvs_flash_init();
    }

    if (err != ESP_OK) {
        Serial.printf("[NVS] FATAL: nvs_flash_init failed (0x%x)\n", err);
        return false;
    }

    Serial.println("[NVS] Non-Volatile Storage initialized successfully.");
    return true;
}

uint32_t player_data_get_coins(WizardProfile_t profile) {
    if (profile <= PROFILE_NONE || profile >= PROFILE_MAX) return 0;
    char key[16];
    snprintf(key, sizeof(key), "c_%d", (int)profile);
    return nvs_read_u32_val(key, 0);
}

void player_data_add_coins(WizardProfile_t profile, uint32_t amount) {
    if (profile <= PROFILE_NONE || profile >= PROFILE_MAX) return;
    char key[16];
    snprintf(key, sizeof(key), "c_%d", (int)profile);

    uint32_t current_coins = nvs_read_u32_val(key, 0);
    uint32_t new_coins = (UINT32_MAX - current_coins < amount) ? UINT32_MAX : current_coins + amount;

    if (nvs_write_u32_val(key, new_coins)) {
        Serial.printf("[NVS] Profile %d: Added %u coins -> Total: %u 🪙\n", 
                      (int)profile, amount, new_coins);
    }
}

uint32_t player_data_get_xp(WizardProfile_t profile) {
    if (profile <= PROFILE_NONE || profile >= PROFILE_MAX) return 0;
    char key[16];
    snprintf(key, sizeof(key), "xp_%d", (int)profile);
    return nvs_read_u32_val(key, 0);
}

void player_data_add_xp(WizardProfile_t profile, uint32_t amount) {
    if (profile <= PROFILE_NONE || profile >= PROFILE_MAX) return;
    char key[16];
    snprintf(key, sizeof(key), "xp_%d", (int)profile);

    uint32_t current_xp = nvs_read_u32_val(key, 0);
    uint32_t new_xp = (UINT32_MAX - current_xp < amount) ? UINT32_MAX : current_xp + amount;

    if (nvs_write_u32_val(key, new_xp)) {
        Serial.printf("[NVS] Profile %d: Added %u XP -> Total: %u ⚡\n", 
                      (int)profile, amount, new_xp);
    }
}

void player_data_reset_all(void) {
    nvs_handle_t handle;
    if (nvs_open(NVS_STORAGE_NAMESPACE, NVS_READWRITE, &handle) == ESP_OK) {
        nvs_erase_all(handle);
        nvs_commit(handle);
        nvs_close(handle);
        Serial.println("[NVS] All wizard profiles reset to 0.");
    }
}
