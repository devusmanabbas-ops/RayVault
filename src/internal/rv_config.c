#include "rv_config.h"

#include <string.h>

void rv_config_init(rv_config *cfg)
{
    if (!cfg)
        return;
    memset(cfg, 0, sizeof(*cfg));
    cfg->arena_initial_bytes = 64 * 1024;
    cfg->arena_max_bytes = 64u * 1024u * 1024u;
    cfg->cache_capacity = 8;
    cfg->stream_prefetch = 2;
    cfg->options = RV_OPEN_BUILD_INDEX;
    cfg->log_level = 2;
}

void rv_config_sanitize(rv_config *cfg)
{
    if (!cfg)
        return;
    if (cfg->arena_initial_bytes < 1024)
        cfg->arena_initial_bytes = 1024;
    if (cfg->arena_max_bytes < cfg->arena_initial_bytes)
        cfg->arena_max_bytes = cfg->arena_initial_bytes;
    if (cfg->cache_capacity < 2)
        cfg->cache_capacity = 2;
    if (cfg->cache_capacity > 4096)
        cfg->cache_capacity = 4096;
}

size_t rv_config_arena_initial(const rv_config *cfg)
{
    return cfg ? cfg->arena_initial_bytes : (64u * 1024u);
}

size_t rv_config_arena_max(const rv_config *cfg)
{
    return cfg ? cfg->arena_max_bytes : (64u * 1024u * 1024u);
}

size_t rv_config_cache_capacity(const rv_config *cfg)
{
    return cfg ? cfg->cache_capacity : 8;
}
