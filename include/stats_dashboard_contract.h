#pragma once

/**
 * The weekly statistics entry point must be a visible dashboard card, not a
 * hidden/floating affordance. Keeping this tiny contract platform-neutral lets
 * the native test suite lock the UX requirement without pulling in LVGL.
 */
static inline bool stats_dashboard_card_enabled(void) {
    return true;
}

static inline const char *stats_dashboard_card_title(void) {
    return "סטטיסטיקה - 7 ימים";
}
