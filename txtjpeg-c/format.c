/* format.c -- infer original float format template */
#include "txtjpeg.h"
#include <stdio.h>
#include <string.h>

int tj_infer_format(const char *path, char *fmt_out, size_t fmt_size)
{
    FILE *f;
    char buf[4096];
    size_t n;
    size_t i;
    size_t comma_pos;
    size_t width;
    size_t dot_pos;
    int has_dot;
    size_t decimals;

    comma_pos = 0;
    has_dot = 0;
    dot_pos = 0;

    f = fopen(path, "rb");
    if (f == NULL) {
        tjpeg_snprintf(fmt_out, fmt_size, "%%g,");
        return 0;
    }

    n = fread(buf, 1, sizeof(buf), f);
    fclose(f);

    if (n == 0) {
        tjpeg_snprintf(fmt_out, fmt_size, "%%g,");
        return 0;
    }

    for (i = 0; i < n; i++) {
        if (buf[i] == ',') {
            comma_pos = i;
            break;
        }
    }

    if (comma_pos == 0) {
        tjpeg_snprintf(fmt_out, fmt_size, "%%g,");
        return 0;
    }

    width = comma_pos;

    for (i = 0; i < comma_pos; i++) {
        if (buf[i] == '.') {
            dot_pos = i;
            has_dot = 1;
            break;
        }
    }

    if (has_dot) {
        decimals = comma_pos - dot_pos - 1;
    } else {
        decimals = 0;
    }

    tjpeg_snprintf(fmt_out, fmt_size, "%%%d.%df,", (int)width, (int)decimals);
    return 0;
}
