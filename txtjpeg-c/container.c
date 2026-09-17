/* container.c -- .tj container read/write */
#include "txtjpeg.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static uint16_t read_u16_le(const uint8_t *p)
{
    return (uint16_t)p[0] | ((uint16_t)p[1] << 8);
}

static uint32_t read_u32_le(const uint8_t *p)
{
    return (uint32_t)p[0]
         | ((uint32_t)p[1] << 8)
         | ((uint32_t)p[2] << 16)
         | ((uint32_t)p[3] << 24);
}

static uint64_t read_u64_le(const uint8_t *p)
{
    uint64_t v;
    int i;
    v = 0;
    for (i = 0; i < 8; i++) {
        v |= ((uint64_t)p[i]) << (i * 8);
    }
    return v;
}

static double read_f64_le(const uint8_t *p)
{
    double v;
    memcpy(&v, p, sizeof(v));
    return v;
}

static void write_u16_le(uint8_t *p, uint16_t v)
{
    p[0] = (uint8_t)(v & 0xFF);
    p[1] = (uint8_t)((v >> 8) & 0xFF);
}

static void write_u32_le(uint8_t *p, uint32_t v)
{
    p[0] = (uint8_t)(v & 0xFF);
    p[1] = (uint8_t)((v >> 8) & 0xFF);
    p[2] = (uint8_t)((v >> 16) & 0xFF);
    p[3] = (uint8_t)((v >> 24) & 0xFF);
}

static void write_u64_le(uint8_t *p, uint64_t v)
{
    int i;
    for (i = 0; i < 8; i++) {
        p[i] = (uint8_t)(v & 0xFF);
        v >>= 8;
    }
}

static void write_f64_le(uint8_t *p, double v)
{
    memcpy(p, &v, sizeof(v));
}

int tj_container_read(const char *path, tj_container_t *c)
{
    FILE *f;
    uint8_t header[TJPEG_HEADER_SIZE];
    size_t n;
    uint8_t len_bytes[8];

    f = fopen(path, "rb");
    if (f == NULL) {
        return -1;
    }

    n = fread(header, 1, TJPEG_HEADER_SIZE, f);
    if (n != TJPEG_HEADER_SIZE) {
        fclose(f);
        return -1;
    }

    memcpy(c->magic, header, 8);
    if (memcmp(c->magic, TJPEG_MAGIC, 8) != 0) {
        fclose(f);
        return -1;
    }

    c->version = header[8];
    if (c->version != TJPEG_VERSION) {
        fclose(f);
        return -1;
    }

    c->quality = header[9];
    c->width = read_u32_le(header + 10);
    c->height = read_u32_le(header + 14);
    c->n = read_u64_le(header + 18);
    c->vmin = read_f64_le(header + 26);
    c->vmax = read_f64_le(header + 34);
    c->columns = read_u32_le(header + 42);
    c->fmt_len = read_u16_le(header + 46);

    c->fmt_str = (char *)malloc((size_t)c->fmt_len + 1);
    if (c->fmt_str == NULL) {
        fclose(f);
        return -1;
    }
    if (fread(c->fmt_str, 1, c->fmt_len, f) != c->fmt_len) {
        free(c->fmt_str);
        fclose(f);
        return -1;
    }
    c->fmt_str[c->fmt_len] = '\0';

    if (fread(len_bytes, 1, 8, f) != 8) {
        free(c->fmt_str);
        fclose(f);
        return -1;
    }
    c->jpeg_len = read_u64_le(len_bytes);

    c->jpeg_data = (uint8_t *)malloc((size_t)c->jpeg_len);
    if (c->jpeg_data == NULL) {
        free(c->fmt_str);
        fclose(f);
        return -1;
    }
    if (fread(c->jpeg_data, 1, (size_t)c->jpeg_len, f) != (size_t)c->jpeg_len) {
        free(c->jpeg_data);
        free(c->fmt_str);
        fclose(f);
        return -1;
    }

    fclose(f);
    return 0;
}

int tj_container_write(const char *path, const tj_container_t *c)
{
    FILE *f;
    uint8_t header[TJPEG_HEADER_SIZE];
    uint8_t len_bytes[8];

    f = fopen(path, "wb");
    if (f == NULL) {
        return -1;
    }

    memcpy(header, c->magic, 8);
    header[8] = c->version;
    header[9] = c->quality;
    write_u32_le(header + 10, c->width);
    write_u32_le(header + 14, c->height);
    write_u64_le(header + 18, c->n);
    write_f64_le(header + 26, c->vmin);
    write_f64_le(header + 34, c->vmax);
    write_u32_le(header + 42, c->columns);
    write_u16_le(header + 46, c->fmt_len);

    if (fwrite(header, 1, TJPEG_HEADER_SIZE, f) != TJPEG_HEADER_SIZE) {
        fclose(f);
        return -1;
    }
    if (fwrite(c->fmt_str, 1, c->fmt_len, f) != c->fmt_len) {
        fclose(f);
        return -1;
    }

    write_u64_le(len_bytes, c->jpeg_len);
    if (fwrite(len_bytes, 1, 8, f) != 8) {
        fclose(f);
        return -1;
    }

    if (fwrite(c->jpeg_data, 1, (size_t)c->jpeg_len, f) != (size_t)c->jpeg_len) {
        fclose(f);
        return -1;
    }

    fclose(f);
    return 0;
}
