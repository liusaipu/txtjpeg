/* normalize.c -- linear normalization between float and uint8 */
#include "txtjpeg.h"
#include <stdint.h>

void tj_normalize(const float *values, uint64_t n,
                  double vmin, double vmax, uint8_t *pixels)
{
    uint64_t i;
    int p;
    double t;

    if (vmin == vmax) {
        for (i = 0; i < n; i++) {
            pixels[i] = 0;
        }
        return;
    }

    for (i = 0; i < n; i++) {
        t = ((double)values[i] - vmin) / (vmax - vmin);
        p = (int)(t * 255.0 + 0.5);
        if (p < 0) {
            p = 0;
        }
        if (p > 255) {
            p = 255;
        }
        pixels[i] = (uint8_t)p;
    }
}

void tj_denormalize(const uint8_t *pixels, uint64_t n,
                    double vmin, double vmax, double *values)
{
    uint64_t i;

    if (vmin == vmax) {
        for (i = 0; i < n; i++) {
            values[i] = vmin;
        }
        return;
    }

    for (i = 0; i < n; i++) {
        values[i] = (double)pixels[i] / 255.0 * (vmax - vmin) + vmin;
    }
}
