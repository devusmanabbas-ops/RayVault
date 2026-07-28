#ifndef RV_DIAG_H
#define RV_DIAG_H

#include "rayvault/rayvault_error.h"
#include "rv_arena.h"

#define RV_DIAG_MAX 64
#define RV_DIAG_MSG 192

typedef struct rv_diag_entry {
    rv_status code;
    char      message[RV_DIAG_MSG];
} rv_diag_entry;

typedef struct rv_diag {
    rv_diag_entry entries[RV_DIAG_MAX];
    int           count;
    char          last[RV_DIAG_MSG];
} rv_diag;

void rv_diag_clear(rv_diag *d);
void rv_diag_add(rv_diag *d, rv_status code, const char *fmt, ...);
void rv_diag_set_last(rv_diag *d, const char *msg);
int  rv_diag_count(const rv_diag *d);
rv_status rv_diag_at(const rv_diag *d, int index, int *code, const char **msg);

#endif /* RV_DIAG_H */
