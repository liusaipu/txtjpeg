/* parser.c -- streaming ASCII float parser */
#include "txtjpeg.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <math.h>

#define CHUNK_SIZE (1 << 20)

static uint32_t infer_columns_from_buffer(const char *buf, size_t len)
{
    size_t i;
    const char *p;
    const char *line_start;
    size_t counts[10];
    size_t line_count;
    size_t first_count;
    int has_newline;
    size_t comma_count;

    has_newline = 0;
    for (i = 0; i < len; i++) {
        if (buf[i] == '\n' || buf[i] == '\r') {
            has_newline = 1;
            break;
        }
    }
    if (!has_newline) {
        return 0;
    }

    line_count = 0;
    first_count = 0;
    p = buf;
    line_start = buf;
    while (p < buf + len && line_count < 10) {
        if (*p == '\n' || *p == '\r') {
            if (p > line_start) {
                comma_count = 0;
                for (i = 0; i < (size_t)(p - line_start); i++) {
                    if (line_start[i] == ',') {
                        comma_count++;
                    }
                }
                counts[line_count] = comma_count;
                if (line_count == 0) {
                    first_count = comma_count;
                }
                line_count++;
            }
            if (*p == '\r' && p + 1 < buf + len && *(p + 1) == '\n') {
                p++;
            }
            line_start = p + 1;
        }
        p++;
    }

    return (uint32_t)first_count;
}

static void parse_token(const char *start, const char *end,
                        float **vals, size_t *n, size_t *cap,
                        double *vmin, double *vmax)
{
    char token_buf[256];
    size_t tok_len;
    const char *tok_start;
    const char *tok_end;
    char *str_end;
    double d;

    tok_start = start;
    while (tok_start < end && isspace((unsigned char)*tok_start)) {
        tok_start++;
    }
    tok_end = end - 1;
    while (tok_end > tok_start && isspace((unsigned char)*tok_end)) {
        tok_end--;
    }
    if (tok_end < tok_start) {
        return;
    }

    tok_len = (size_t)(tok_end - tok_start + 1);
    if (tok_len >= sizeof(token_buf)) {
        tok_len = sizeof(token_buf) - 1;
    }
    memcpy(token_buf, tok_start, tok_len);
    token_buf[tok_len] = '\0';

    d = strtod(token_buf, &str_end);
    if (str_end == token_buf) {
        return;
    }

    if (*n >= *cap) {
        *cap *= 2;
        *vals = (float *)realloc(*vals, *cap * sizeof(float));
    }
    if (*vals == NULL) {
        return;
    }

    (*vals)[*n] = (float)d;
    (*n)++;
    if (d < *vmin) {
        *vmin = d;
    }
    if (d > *vmax) {
        *vmax = d;
    }
}

int tj_parse_floats(const char *path, tj_data_t *out)
{
    FILE *f;
    char *chunk;
    char *buf;
    char *carry;
    size_t carry_len;
    size_t cap;
    float *vals;
    size_t n;
    double vmin;
    double vmax;
    size_t bytes_read;
    size_t total;
    int first_chunk;
    uint32_t columns;
    char *last_comma;
    char *scan;
    char *p;
    char *comma;

    chunk = (char *)malloc(CHUNK_SIZE);
    buf = (char *)malloc(CHUNK_SIZE * 2);
    carry = (char *)malloc(CHUNK_SIZE);
    if (chunk == NULL || buf == NULL || carry == NULL) {
        free(chunk);
        free(buf);
        free(carry);
        return -1;
    }

    cap = 1024 * 1024;
    vals = (float *)malloc(cap * sizeof(float));
    if (vals == NULL) {
        free(chunk);
        free(buf);
        free(carry);
        return -1;
    }

    f = fopen(path, "rb");
    if (f == NULL) {
        free(vals);
        free(chunk);
        free(buf);
        free(carry);
        return -1;
    }

    n = 0;
    vmin = 1e300;
    vmax = -1e300;
    carry_len = 0;
    first_chunk = 1;
    columns = 0;

    while ((bytes_read = fread(chunk, 1, CHUNK_SIZE, f)) > 0) {
        if (first_chunk) {
            columns = infer_columns_from_buffer(chunk, bytes_read);
            first_chunk = 0;
        }

        memcpy(buf, carry, carry_len);
        memcpy(buf + carry_len, chunk, bytes_read);
        total = carry_len + bytes_read;

        last_comma = NULL;
        for (scan = buf + total - 1; scan >= buf; scan--) {
            if (*scan == ',') {
                last_comma = scan;
                break;
            }
        }

        if (last_comma == NULL) {
            memcpy(carry, buf, total);
            carry_len = total;
            continue;
        }

        p = buf;
        while (p <= last_comma) {
            comma = p;
            while (comma <= last_comma && *comma != ',') {
                comma++;
            }
            if (comma > last_comma) {
                break;
            }
            parse_token(p, comma, &vals, &n, &cap, &vmin, &vmax);
            p = comma + 1;
        }

        carry_len = (size_t)((buf + total) - (last_comma + 1));
        memcpy(carry, last_comma + 1, carry_len);
    }

    if (carry_len > 0) {
        parse_token(carry, carry + carry_len, &vals, &n, &cap, &vmin, &vmax);
    }

    fclose(f);
    free(chunk);
    free(buf);
    free(carry);

    out->count = (uint64_t)n;
    out->values = vals;
    out->vmin = vmin;
    out->vmax = vmax;
    out->columns = columns;
    return 0;
}
