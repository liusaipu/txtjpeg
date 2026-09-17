/* compress.c -- compression pipeline */
#include "txtjpeg.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

int tj_compress(const char *input_path, const char *output_path,
                int quality, const char *shape_str)
{
    tj_data_t data;
    tj_container_t c;
    uint32_t w;
    uint32_t h;
    uint64_t total_pixels;
    uint8_t *pixels;
    uint8_t *jpeg_data;
    uint64_t jpeg_len;
    int ret;

    memset(&data, 0, sizeof(data));
    memset(&c, 0, sizeof(c));

    if (tj_infer_format(input_path, data.fmt_str, sizeof(data.fmt_str)) != 0) {
        fprintf(stderr, "error: failed to infer format\n");
        return 1;
    }
    printf("[info] inferred format: %s\n", data.fmt_str);

    if (tj_parse_floats(input_path, &data) != 0) {
        fprintf(stderr, "error: failed to parse floats\n");
        return 1;
    }

    if (data.count == 0) {
        fprintf(stderr, "error: no floats found\n");
        return 1;
    }

    printf("[info] total floats: %llu, range [%g, %g]\n",
           (unsigned long long)data.count, data.vmin, data.vmax);
    if (data.columns > 0) {
        printf("[info] columns per line: %u\n", data.columns);
    } else {
        printf("[info] no newline detected\n");
    }

    tj_compute_shape(data.count, &h, &w, shape_str);
    total_pixels = (uint64_t)h * w;
    printf("[info] image shape: %ux%u (padding %llu)\n",
           h, w, (unsigned long long)(total_pixels - data.count));

    pixels = (uint8_t *)calloc((size_t)total_pixels, 1);
    if (pixels == NULL) {
        fprintf(stderr, "error: out of memory\n");
        free(data.values);
        return 1;
    }

    tj_normalize(data.values, data.count, data.vmin, data.vmax, pixels);

    ret = tj_jpeg_encode(pixels, w, h, quality, "4:4:4", &jpeg_data, &jpeg_len);
    if (ret != 0) {
        fprintf(stderr, "error: jpeg encode failed\n");
        free(pixels);
        free(data.values);
        return 1;
    }

    memcpy(c.magic, TJPEG_MAGIC, 8);
    c.version = TJPEG_VERSION;
    c.quality = (uint8_t)quality;
    c.width = w;
    c.height = h;
    c.n = data.count;
    c.vmin = data.vmin;
    c.vmax = data.vmax;
    c.columns = data.columns;
    c.fmt_len = (uint16_t)strlen(data.fmt_str);
    c.fmt_str = data.fmt_str;
    c.jpeg_len = jpeg_len;
    c.jpeg_data = jpeg_data;

    if (tj_container_write(output_path, &c) != 0) {
        fprintf(stderr, "error: failed to write container\n");
        free(jpeg_data);
        free(pixels);
        free(data.values);
        return 1;
    }

    printf("[info] compressed to: %s\n", output_path);

    free(jpeg_data);
    free(pixels);
    free(data.values);
    return 0;
}
