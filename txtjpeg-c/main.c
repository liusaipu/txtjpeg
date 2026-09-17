/* main.c -- command line entry point */
#include "txtjpeg.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef _MSC_VER
#  include <sys/timeb.h>
#else
#  include <sys/time.h>
#endif

static double get_time(void)
{
#ifdef _MSC_VER
    struct _timeb tb;
    _ftime(&tb);
    return (double)tb.time + (double)tb.millitm / 1000.0;
#else
    struct timeval tv;
    if (gettimeofday(&tv, NULL) != 0) {
        return 0.0;
    }
    return (double)tv.tv_sec + (double)tv.tv_usec / 1000000.0;
#endif
}

static void print_usage(const char *prog)
{
    fprintf(stderr, "Usage:\n");
    fprintf(stderr, "  %s compress <input.txt> <output.tj> [--quality N] [--shape HxW]\n", prog);
    fprintf(stderr, "  %s decompress <input.tj> <output.txt>\n", prog);
}

int main(int argc, char **argv)
{
    int i;
    const char *cmd;
    const char *input;
    const char *output;
    int quality;
    const char *shape_str;
    double t0;
    double elapsed;
    int ret;

    if (argc < 4) {
        print_usage(argv[0]);
        return 1;
    }

    cmd = argv[1];
    input = argv[2];
    output = argv[3];
    quality = 85;
    shape_str = NULL;

    for (i = 4; i < argc; i++) {
        if (strcmp(argv[i], "--quality") == 0 && i + 1 < argc) {
            quality = atoi(argv[++i]);
        } else if (strcmp(argv[i], "--shape") == 0 && i + 1 < argc) {
            shape_str = argv[++i];
        }
    }

    t0 = get_time();

    if (strcmp(cmd, "compress") == 0) {
        if (quality < 1 || quality > 100) {
            fprintf(stderr, "error: --quality must be between 1 and 100\n");
            return 1;
        }
        ret = tj_compress(input, output, quality, shape_str);
    } else if (strcmp(cmd, "decompress") == 0) {
        ret = tj_decompress(input, output);
    } else {
        fprintf(stderr, "error: unknown command '%s'\n", cmd);
        print_usage(argv[0]);
        return 1;
    }

    elapsed = get_time() - t0;
    printf("[info] elapsed: %.2f s\n", elapsed);

    return ret;
}
