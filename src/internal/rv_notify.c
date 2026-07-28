#include "rv_notify.h"

void rv_notify_emit(rv_notify_state *st, int event, const char *msg)
{
    if (st && st->fn)
        st->fn(st->userdata, event, msg ? msg : "");
}
