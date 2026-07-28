#include "rv_diag.h"

#include <stdarg.h>
#include <stdio.h>
#include <string.h>

void rv_diag_clear(rv_diag *d)
{
    if (!d)
        return;
    memset(d, 0, sizeof(*d));
}

void rv_diag_set_last(rv_diag *d, const char *msg)
{
    if (!d)
        return;
    if (!msg) {
        d->last[0] = '\0';
        return;
    }
    strncpy(d->last, msg, sizeof(d->last) - 1);
    d->last[sizeof(d->last) - 1] = '\0';
}

void rv_diag_add(rv_diag *d, rv_status code, const char *fmt, ...)
{
    va_list ap;
    rv_diag_entry *e;
    if (!d)
        return;
    if (d->count >= RV_DIAG_MAX)
        return;
    e = &d->entries[d->count++];
    e->code = code;
    va_start(ap, fmt);
    vsnprintf(e->message, sizeof(e->message), fmt, ap);
    va_end(ap);
    rv_diag_set_last(d, e->message);
}

int rv_diag_count(const rv_diag *d)
{
    return d ? d->count : 0;
}

rv_status rv_diag_at(const rv_diag *d, int index, int *code, const char **msg)
{
    if (!d || index < 0 || index >= d->count)
        return RV_ERR_INVALID_ARG;
    if (code)
        *code = (int)d->entries[index].code;
    if (msg)
        *msg = d->entries[index].message;
    return RV_OK;
}
