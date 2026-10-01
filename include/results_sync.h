#pragma once

#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

void results_sync_poll(void);
void results_sync_force(void);
bool results_sync_is_configured(void);

#ifdef __cplusplus
}
#endif
