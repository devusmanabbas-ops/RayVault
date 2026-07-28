#ifndef RV_PACKAGE_H
#define RV_PACKAGE_H

#include "rv_arena.h"
#include "rv_cache.h"
#include "rv_checkpoint.h"
#include "rv_config.h"
#include "rv_diag.h"
#include "rv_index.h"
#include "rv_notify.h"
#include "rv_parser.h"
#include "rv_reader.h"
#include "rv_stats.h"
#include "rv_stream.h"
#include "rayvault/rayvault.h"

struct rv_package {
    rv_reader  reader;
    rv_arena   arena;
    rv_parser  parser;
    rv_diag    diag;
    rv_config  cfg;
    uint32_t   flags;
    int        opened;
    int        legacy;
};

struct rv_session {
    rv_package         *pkg;
    rv_index            index;
    rv_wave_cache       cache;
    rv_derived_summary  derived;
    rv_notify_state     notify;
    int                 index_ready;
};

struct rv_stream {
    rv_session     *sess;
    rv_stream_impl  impl;
};

struct rv_cursor {
    rv_session     *sess;
    rv_cursor_impl  impl;
};

struct rv_checkpoint {
    rv_session         *sess;
    rv_checkpoint_impl  impl;
};

rv_status rv_package_finish_open(rv_package *pkg);
rv_status rv_session_ensure_index(rv_session *sess);

#endif /* RV_PACKAGE_H */
