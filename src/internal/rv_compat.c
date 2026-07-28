#include "rv_compat.h"
#include "rv_package.h"

rv_status rv_compat_open(rv_package **out, const char *path)
{
    rv_config cfg;
    rv_config_init(&cfg);
    return rv_package_open_file(out, path,
                                RV_OPEN_BUILD_INDEX | RV_OPEN_ALLOW_LEGACY,
                                &cfg);
}

rv_status rv_compat_open_mem(rv_package **out, const uint8_t *data, size_t n)
{
    rv_config cfg;
    rv_config_init(&cfg);
    return rv_package_open_memory(out, data, n,
                                  RV_OPEN_BUILD_INDEX | RV_OPEN_ALLOW_LEGACY,
                                  &cfg);
}

rv_status rv_compat_validate(rv_package *pkg)
{
    return rv_package_validate(pkg, 1);
}

rv_status rv_compat_route_count(rv_package *pkg, uint32_t *out)
{
    rv_package_info info;
    rv_status st;
    if (!pkg || !out)
        return RV_ERR_INVALID_ARG;
    st = rv_package_get_info(pkg, &info);
    if (st != RV_OK)
        return st;
    *out = info.route_count;
    return RV_OK;
}

rv_status rv_compat_export(rv_package *pkg, uint8_t **buf, size_t *n)
{
    rv_session *sess = NULL;
    rv_status st;
    if (!pkg)
        return RV_ERR_INVALID_ARG;
    st = rv_session_create(&sess, pkg);
    if (st != RV_OK)
        return st;
    st = rv_export_buffer(sess, RV_EXPORT_INCLUDE_WAVE | RV_EXPORT_WITH_SUMMARY,
                          buf, n);
    rv_session_destroy(sess);
    return st;
}

void rv_compat_close(rv_package *pkg)
{
    rv_package_close(pkg);
}
