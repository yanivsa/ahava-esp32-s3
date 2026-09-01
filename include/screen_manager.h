/**
 * @file screen_manager.h
 * @brief Wizard Academy (אקדמיית הקוסמים) Screen Manager & Child Profiles
 */

#pragma once

#include "lvgl.h"
#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ========================================================================== */
/*                         SCREEN IDENTIFIERS                                 */
/* ========================================================================== */
typedef enum {
    SCREEN_NONE = 0,
    SCREEN_SPLASH,      /**< Initial Wizard Academy splash screen */
    SCREEN_PROFILES,    /**< Child profile selection screen */
    SCREEN_DASHBOARD,   /**< Main wizard dashboard / spellbook */
    SCREEN_QUIZ,        /**< Quiz / Question screen */
    SCREEN_SYSTEM,      /**< System info, Wi-Fi status and OTA update screen */
    SCREEN_COUNT
} ScreenID_t;

/* ========================================================================== */
/*                         WIZARD PROFILES                                    */
/* ========================================================================== */
typedef enum {
    PROFILE_NONE = 0,
    PROFILE_ORI,        /**< Ori (Age 11) - Senior Mage */
    PROFILE_ETHAN,      /**< Ethan (Age 8) - Apprentice Mage */
    PROFILE_AYALA,      /**< Ayala (Age 3) - Young Spark */
    PROFILE_MAX
} WizardProfile_t;

/**
 * @brief Profile metadata descriptor structure
 */
typedef struct {
    WizardProfile_t id;
    const char *name_hebrew;
    const char *title_hebrew;
    uint8_t age;
    uint32_t color_accent;
    const char *badge_symbol;
} ProfileInfo_t;

/* Global state: Active child profile playing the game */
extern WizardProfile_t current_profile;

/* ========================================================================== */
/*                         SCREEN MANAGER API                                 */
/* ========================================================================== */

/**
 * @brief Initialize the Screen Manager and state variables.
 */
void sm_init(void);

/**
 * @brief Load and switch to a new screen cleanly.
 *        Deallocates the previous screen hierarchy from PSRAM to prevent leaks.
 * @param screen_id The target ScreenID_t to load.
 */
void sm_load_screen(ScreenID_t screen_id);

/**
 * @brief Get the identifier of the currently active screen.
 * @return Active ScreenID_t.
 */
ScreenID_t sm_get_current_screen(void);

/**
 * @brief Retrieve metadata for a specific wizard profile.
 * @param profile Profile enum.
 * @return Pointer to read-only ProfileInfo_t descriptor.
 */
const ProfileInfo_t* sm_get_profile_info(WizardProfile_t profile);

/* ========================================================================== */
/*                         SCREEN BUILDERS                                    */
/* ========================================================================== */
void ui_screen_splash_init(lv_obj_t *scr);
void ui_screen_profiles_init(lv_obj_t *scr);
void ui_screen_dashboard_init(lv_obj_t *scr);
void ui_screen_quiz_init(lv_obj_t *scr);
void ui_screen_system_init(lv_obj_t *scr);

#ifdef __cplusplus
}
#endif
