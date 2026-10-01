#pragma once

#include <stdbool.h>

static inline const char *results_sync_profile_key_from_id(int profile_id) {
    switch (profile_id) {
        case 1: return "ori";
        case 2: return "eitan";
        default: return nullptr;
    }
}

static inline bool results_sync_profile_id_enabled(int profile_id) {
    return profile_id == 1 || profile_id == 2;
}
