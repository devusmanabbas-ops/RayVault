/**
 * rayvault_error.h — status codes returned by the RayVault public API.
 *
 * Codes are stable across minor releases. New codes may be appended;
 * existing numeric values are not renumbered.
 */
#ifndef RAYVAULT_ERROR_H
#define RAYVAULT_ERROR_H

#ifdef __cplusplus
extern "C" {
#endif

typedef enum rv_status {
    RV_OK = 0,
    RV_ERR_INVALID_ARG = 1,
    RV_ERR_NOMEM = 2,
    RV_ERR_IO = 3,
    RV_ERR_FORMAT = 4,
    RV_ERR_VERSION = 5,
    RV_ERR_CHECKSUM = 6,
    RV_ERR_TRUNCATED = 7,
    RV_ERR_SECTION = 8,
    RV_ERR_OFFSET = 9,
    RV_ERR_CROSSREF = 10,
    RV_ERR_STATE = 11,
    RV_ERR_NOT_FOUND = 12,
    RV_ERR_OVERFLOW = 13,
    RV_ERR_UNSUPPORTED = 14,
    RV_ERR_LEGACY = 15,
    RV_ERR_REPAIR = 16,
    RV_ERR_CACHE = 17,
    RV_ERR_CURSOR = 18,
    RV_ERR_EXPORT = 19,
    RV_ERR_CHECKPOINT = 20,
    RV_ERR_CONFIG = 21,
    RV_ERR_INTERNAL = 22,
    RV_ERR_AGAIN = 23,
    RV_ERR_CLOSED = 24,
    RV_ERR_BUSY = 25
} rv_status;

const char *rv_status_string(rv_status st);
const char *rv_status_detail(rv_status st);

#ifdef __cplusplus
}
#endif

#endif /* RAYVAULT_ERROR_H */
