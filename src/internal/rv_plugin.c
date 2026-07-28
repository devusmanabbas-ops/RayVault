#include "rv_plugin.h"

#include <string.h>

void rv_plugin_registry_init(rv_plugin_registry *reg)
{
    if (reg)
        memset(reg, 0, sizeof(*reg));
}

int rv_plugin_register(rv_plugin_registry *reg, const rv_plugin *p)
{
    if (!reg || !p || !p->name || !p->cb)
        return -1;
    if (reg->count >= 16)
        return -1;
    reg->plugins[reg->count++] = *p;
    return 0;
}

int rv_plugin_unregister(rv_plugin_registry *reg, const char *name)
{
    uint32_t i, j;
    if (!reg || !name)
        return -1;
    for (i = 0; i < reg->count; i++) {
        if (strcmp(reg->plugins[i].name, name) != 0)
            continue;
        for (j = i + 1; j < reg->count; j++)
            reg->plugins[j - 1] = reg->plugins[j];
        reg->count--;
        return 0;
    }
    return -1;
}

int rv_plugin_emit(rv_plugin_registry *reg, rv_plugin_hook hook,
                   rv_session *sess)
{
    uint32_t i;
    int rc = 0;
    if (!reg)
        return 0;
    for (i = 0; i < reg->count; i++) {
        rv_plugin *p = &reg->plugins[i];
        if (p->hooks_mask && (p->hooks_mask & (1u << (unsigned)hook)) == 0)
            continue;
        if (p->cb(p->userdata, hook, sess) != 0)
            rc = -1;
    }
    return rc;
}
