/* txtjpeg.h -- public API and container format */
#ifndef TJPEG_H
#define TJPEG_H

#include "compat.h"
#include <stdint.h>
#include <stddef.h>

#define TJPEG_MAGIC       "TXTJPEG\0"
#define TJPEG_VERSION     2
#define TJPEG_HEADER_SIZE (8 + 1 + 1 + 4 + 4 + 8 + 8 + 8 + 4 + 2)

/* .tj container structure */
typedef struct {
    uint8_t  magic[8];
    uint8_t  version;
    uint8_t  quality;
    uint32_t width;
    uint32_t height;
    uint64_t n;
    double   vmin;
    double   vmax;
    uint32_t columns;
    uint16_t fmt_len;
    char    *fmt_str;
    uint64_t jpeg_len;
    uint8_t *jpeg_data;
} tj_container_t;

/* parsed float data */
typedef struct {
    uint64_t count;
    float   *values;
    double   vmin;
    double   vmax;
    uint32_t columns;
    char     fmt_str[64];
} tj_data_t;

/* main operations */
int tj_compress(const char *input_path, const char *output_path,
                int quality, const char *shape_str);
int tj_decompress(const char *input_path, const char *output_path);

/* helpers */
int  tj_infer_format(const char *path, char *fmt_out, size_t fmt_size);
int  tj_parse_floats(const char *path, tj_data_t *out);
void tj_compute_shape(uint64_t n, uint32_t *height, uint32_t *width,
                      const char *shape_str);
void tj_normalize(const float *values, uint64_t n,
                  double vmin, double vmax, uint8_t *pixels);
void tj_denormalize(const uint8_t *pixels, uint64_t n,
                    double vmin, double vmax, double *values);

int tj_jpeg_encode(const uint8_t *gray, uint32_t width, uint32_t height,
                   int quality, const char *subsampling,
                   uint8_t **out_jpeg, uint64_t *out_len);
int tj_jpeg_decode(const uint8_t *jpeg_data, uint64_t jpeg_len,
                   uint8_t **out_gray, uint32_t *out_width,
                   uint32_t *out_height);

int tj_container_read(const char *path, tj_container_t *c);
int tj_container_write(const char *path, const tj_container_t *c);

#endif /* TJPEG_H */
