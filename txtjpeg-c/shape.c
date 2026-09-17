/* shape.c -- 2D image shape computation */
#include "txtjpeg.h"
#include <stdio.h>
#include <math.h>

static int parse_shape(const char *s, uint32_t *height, uint32_t *width)
{
    int h;
    int w;
    if (s == NULL) {
        return -1;
    }
    if (sscanf(s, "%dx%d", &h, &w) != 2) {
        return -1;
    }
    *height = (uint32_t)h;
    *width = (uint32_t)w;
    return 0;
}

void tj_compute_shape(uint64_t n, uint32_t *height, uint32_t *width,
                      const char *shape_str)
{
    uint32_t h;
    uint32_t w;
    uint32_t tmp;

    if (parse_shape(shape_str, &h, &w) == 0) {
        if ((uint64_t)h * w < n) {
            /* fallback to auto if shape too small */
            h = 8;
            w = 8;
        }
        h = ((h + 7) / 8) * 8;
        w = ((w + 7) / 8) * 8;
        *height = h;
        *width = w;
        return;
    }

    tmp = (uint32_t)ceil(sqrt((double)n));
    if (tmp < 8) {
        tmp = 8;
    }
    w = ((tmp + 7) / 8) * 8;
    h = (uint32_t)((n + w - 1) / w);
    h = ((h + 7) / 8) * 8;

    *height = h;
    *width = w;
}
