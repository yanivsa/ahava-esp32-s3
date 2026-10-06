#pragma once

#ifndef AHAVA_MUSE_API_URL
#define AHAVA_MUSE_API_URL "https://ahava-device-results-api.yanivsa.workers.dev/v1/muse/query"
#endif

#if __has_include("muse_secret.h")
#include "muse_secret.h"
#else
#define AHAVA_MUSE_API_TOKEN ""
#endif
