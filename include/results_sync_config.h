#pragma once

#define AHAVA_RESULTS_SYNC_URL "https://ahava-device-results-api.yanivsa.workers.dev/v1/results/batch"

#if __has_include("results_sync_secret.h")
#include "results_sync_secret.h"
#else
#define AHAVA_RESULTS_SYNC_TOKEN ""
#endif
