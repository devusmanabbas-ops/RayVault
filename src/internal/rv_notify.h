#ifndef RV_NOTIFY_H
#define RV_NOTIFY_H

#include "rayvault/rayvault.h"

enum {
    RV_EVENT_OPENED = 1,
    RV_EVENT_INDEXED = 2,
    RV_EVENT_CACHE_WARM = 3,
    RV_EVENT_REOPEN = 4,
    RV_EVENT_EXPORT = 5,
    RV_EVENT_REPAIR = 6,
    RV_EVENT_MIGRATE = 7,
    RV_EVENT_ERROR = 8
};

typedef struct rv_notify_state {
    rv_notify_fn fn;
    void        *userdata;
} rv_notify_state;

void rv_notify_emit(rv_notify_state *st, int event, const char *msg);

#endif /* RV_NOTIFY_H */
