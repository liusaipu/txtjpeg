/* decompress.c -- decompression pipeline */
#include "txtjpeg.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

int tj_decompress(const char *input_path, const char *output_path)
{
    tj_container_t c;
    uint8_t *gray;
    uint32_t w;
    uint32_t h;
    double *values;
    FILE *f;
    uint64_t i;
    uint64_t line_start;
    uint64_t end;
    uint64_t j;
    int ret;

    memset(&c, 0, sizeof(c));

    if (tj_container_read(input_path, &c) != 0) {
        fprintf(stderr, "error: failed to read container\n");
        return 1;
    }

    printf("[info] container: %ux%u, N=%llu, quality=%u, fmt=%s, columns=%u\n",
           c.width, c.height, (unsigned long long)c.n,
           c.quality, c.fmt_str, c.columns);

    ret = tj_jpeg_decode(c.jpeg_data, c.jpeg_len, &gray, &w, &h);
    if (ret != 0) {
        fprintf(stderr, "error: jpeg decode failed\n");
        free(c.fmt_str);
        free(c.jpeg_data);
        return 1;
    }

    if (w != c.width || h != c.height) {
        fprintf(stderr, "error: jpeg dimension mismatch\n");
        free(gray);
        free(c.fmt_str);
        free(c.jpeg_data);
        return 1;
    }

    values = (double *)malloc((size_t)c.n * sizeof(double));
    if (values == NULL) {
        fprintf(stderr, "error: out of memory\n");
        free(gray);
        free(c.fmt_str);
        free(c.jpeg_data);
        return 1;
    }

    tj_denormalize(gray, c.n, c.vmin, c.vmax, values);

    f = fopen(output_path, "wb");
    if (f == NULL) {
        fprintf(stderr, "error: failed to open output\n");
        free(values);
        free(gray);
        free(c.fmt_str);
        free(c.jpeg_data);
        return 1;
    }

    if (c.columns > 0) {
        for (line_start = 0; line_start < c.n; line_start += c.columns) {
            end = line_start + c.columns;
            if (end > c.n) {
                end = c.n;
            }
            for (j = line_start; j < end; j++) {
                char buf[128];
                tjpeg_snprintf(buf, sizeof(buf), c.fmt_str, values[j]);
                fputs(buf, f);
            }
            fputs("\n", f);
        }
    } else {
        for (i = 0; i < c.n; i++) {
            char buf[128];
            tjpeg_snprintf(buf, sizeof(buf), c.fmt_str, values[i]);
            fputs(buf, f);
        }
    }

    fclose(f);

    free(values);
    free(gray);
    free(c.fmt_str);
    free(c.jpeg_data);

    printf("[info] decompressed to: %s\n", output_path);
    return 0;
}
