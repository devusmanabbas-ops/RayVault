#include "rv_builder.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main(int argc, char **argv)
{
    rv_builder b;
    const char *out = "span.rvp";
    const char *site = "Central-Office";
    const char *far = "Cabinet-14";
    float km = 9.2f;
    uint32_t samples = 512;

    if (argc >= 2)
        out = argv[1];
    if (argc >= 3)
        site = argv[2];
    if (argc >= 4)
        far = argv[3];
    if (argc >= 5)
        km = (float)atof(argv[4]);
    if (argc >= 6)
        samples = (uint32_t)atoi(argv[5]);

    if (rv_builder_init(&b) != 0) {
        fprintf(stderr, "builder init failed\n");
        return 1;
    }
    rv_builder_set_created(&b, 1710000000ull);
    if (rv_builder_demo_span(&b, site, far, km, samples) != 0) {
        fprintf(stderr, "demo span failed\n");
        rv_builder_destroy(&b);
        return 1;
    }
    if (rv_builder_write_file(&b, out) != 0) {
        fprintf(stderr, "write failed: %s\n", out);
        rv_builder_destroy(&b);
        return 1;
    }
    printf("wrote %s (%u samples, %.2f km)\n", out, samples, km);
    rv_builder_destroy(&b);
    return 0;
}
