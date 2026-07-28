#ifndef RV_LOG_H
#define RV_LOG_H

#include <stdarg.h>

enum {
    RV_LOG_OFF   = 0,
    RV_LOG_ERROR = 1,
    RV_LOG_WARN  = 2,
    RV_LOG_INFO  = 3,
    RV_LOG_DEBUG = 4,
    RV_LOG_TRACE = 5
};

void rv_log_set_level(int level);
int  rv_log_get_level(void);
void rv_log_set_prefix(const char *prefix);

void rv_log_error(const char *fmt, ...);
void rv_log_warn(const char *fmt, ...);
void rv_log_info(const char *fmt, ...);
void rv_log_debug(const char *fmt, ...);
void rv_log_trace(const char *fmt, ...);
void rv_log_v(int level, const char *fmt, va_list ap);

#endif /* RV_LOG_H */
