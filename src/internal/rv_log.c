#include "rv_log.h"

#include <stdio.h>
#include <string.h>

static int g_log_level = RV_LOG_WARN;
static char g_prefix[32] = "rayvault";

void rv_log_set_level(int level)
{
    if (level < RV_LOG_OFF)
        level = RV_LOG_OFF;
    if (level > RV_LOG_TRACE)
        level = RV_LOG_TRACE;
    g_log_level = level;
}

int rv_log_get_level(void)
{
    return g_log_level;
}

void rv_log_set_prefix(const char *prefix)
{
    if (!prefix) {
        g_prefix[0] = '\0';
        return;
    }
    strncpy(g_prefix, prefix, sizeof(g_prefix) - 1);
    g_prefix[sizeof(g_prefix) - 1] = '\0';
}

void rv_log_v(int level, const char *fmt, va_list ap)
{
    const char *tag;
    if (level > g_log_level || level <= RV_LOG_OFF)
        return;
    switch (level) {
    case RV_LOG_ERROR: tag = "error"; break;
    case RV_LOG_WARN:  tag = "warn";  break;
    case RV_LOG_INFO:  tag = "info";  break;
    case RV_LOG_DEBUG: tag = "debug"; break;
    default:           tag = "trace"; break;
    }
    fprintf(stderr, "%s[%s]: ", g_prefix, tag);
    vfprintf(stderr, fmt, ap);
    fputc('\n', stderr);
}

void rv_log_error(const char *fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    rv_log_v(RV_LOG_ERROR, fmt, ap);
    va_end(ap);
}

void rv_log_warn(const char *fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    rv_log_v(RV_LOG_WARN, fmt, ap);
    va_end(ap);
}

void rv_log_info(const char *fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    rv_log_v(RV_LOG_INFO, fmt, ap);
    va_end(ap);
}

void rv_log_debug(const char *fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    rv_log_v(RV_LOG_DEBUG, fmt, ap);
    va_end(ap);
}

void rv_log_trace(const char *fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    rv_log_v(RV_LOG_TRACE, fmt, ap);
    va_end(ap);
}
