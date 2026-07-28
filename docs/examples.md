# Examples

## `rv_pack`

Create a demo RVP package:

```bash
./rv_pack out.rvp "Central-Office" "Cabinet-3" 11.5 1024
```

## `rv_inspect`

Print package summary and top markers:

```bash
./rv_inspect out.rvp
```

## Minimal C snippet

```c
#include "rayvault/rayvault.h"

int main(void) {
    rv_package *pkg = NULL;
    rv_session *sess = NULL;
    if (rv_package_open_file(&pkg, "out.rvp", RV_OPEN_BUILD_INDEX, NULL))
        return 1;
    if (rv_session_create(&sess, pkg) == RV_OK) {
        rv_stats_snapshot s;
        rv_session_stats(sess, &s);
        rv_session_destroy(sess);
    }
    rv_package_close(pkg);
    return 0;
}
```
