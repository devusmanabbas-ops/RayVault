#ifndef RV_PLUGIN_H
#define RV_PLUGIN_H

#include "rayvault/rayvault.h"
#include "rv_package.h"

/*
 * Lightweight observation hooks for ingest pipelines. Plugins receive
 * callbacks as sections become available and when derived indexes rebuild.
 */

typedef enum rv_plugin_hook {
    RV_HOOK_AFTER_PARSE = 1,
    RV_HOOK_AFTER_INDEX = 2,
    RV_HOOK_AFTER_CACHE = 3,
    RV_HOOK_BEFORE_EXPORT = 4,
    RV_HOOK_AFTER_REPAIR = 5
} rv_plugin_hook;

typedef int (*rv_plugin_cb)(void *userdata, rv_plugin_hook hook,
                            rv_session *sess);

typedef struct rv_plugin {
    const char  *name;
    rv_plugin_cb cb;
    void        *userdata;
    uint32_t     hooks_mask;
} rv_plugin;

typedef struct rv_plugin_registry {
    rv_plugin plugins[16];
    uint32_t  count;
} rv_plugin_registry;

void rv_plugin_registry_init(rv_plugin_registry *reg);
int  rv_plugin_register(rv_plugin_registry *reg, const rv_plugin *p);
int  rv_plugin_emit(rv_plugin_registry *reg, rv_plugin_hook hook,
                    rv_session *sess);
int  rv_plugin_unregister(rv_plugin_registry *reg, const char *name);

#endif /* RV_PLUGIN_H */
