#ifndef RV_CONFIG_H
#define RV_CONFIG_H

#include "rayvault/rayvault.h"

void rv_config_sanitize(rv_config *cfg);
size_t rv_config_arena_initial(const rv_config *cfg);
size_t rv_config_arena_max(const rv_config *cfg);
size_t rv_config_cache_capacity(const rv_config *cfg);

#endif /* RV_CONFIG_H */
