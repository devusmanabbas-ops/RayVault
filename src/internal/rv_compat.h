#ifndef RV_COMPAT_H
#define RV_COMPAT_H

#include "rayvault/rayvault.h"

/*
 * Compatibility wrappers retained from the 1.x API surface.
 * Prefer the session-based APIs for new code.
 */

typedef struct rv_handle rv_handle; /* opaque alias era */

rv_status rv_compat_open(rv_package **out, const char *path);
rv_status rv_compat_open_mem(rv_package **out, const uint8_t *data, size_t n);
rv_status rv_compat_validate(rv_package *pkg);
rv_status rv_compat_route_count(rv_package *pkg, uint32_t *out);
rv_status rv_compat_export(rv_package *pkg, uint8_t **buf, size_t *n);
void      rv_compat_close(rv_package *pkg);

/* Older spelling kept in headers used by partner tools. */
#define rayvault_open_file rv_package_open_file
#define rayvault_close     rv_package_close

#endif /* RV_COMPAT_H */
