/* jpeg_codec.c -- libjpeg grayscale encode/decode wrapper */
#include "txtjpeg.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <setjmp.h>
#include <jpeglib.h>

struct my_error_mgr {
    struct jpeg_error_mgr pub;
    jmp_buf setjmp_buffer;
};

static void my_error_exit(j_common_ptr cinfo)
{
    struct my_error_mgr *myerr;
    myerr = (struct my_error_mgr *)cinfo->err;
    (*cinfo->err->output_message)(cinfo);
    longjmp(myerr->setjmp_buffer, 1);
}

/* memory destination manager */
struct mem_dest_mgr {
    struct jpeg_destination_mgr pub;
    uint8_t *buf;
    size_t size;
    size_t used;
};

static void init_mem_destination(j_compress_ptr cinfo)
{
    struct mem_dest_mgr *dest;
    dest = (struct mem_dest_mgr *)cinfo->dest;
    dest->pub.next_output_byte = dest->buf;
    dest->pub.free_in_buffer = dest->size;
    dest->used = 0;
}

static boolean empty_mem_output_buffer(j_compress_ptr cinfo)
{
    struct mem_dest_mgr *dest;
    size_t old_size;
    size_t new_size;
    uint8_t *new_buf;

    dest = (struct mem_dest_mgr *)cinfo->dest;
    old_size = dest->size;
    new_size = old_size * 2;
    new_buf = (uint8_t *)realloc(dest->buf, new_size);
    if (new_buf == NULL) {
        return FALSE;
    }
    dest->pub.next_output_byte = new_buf + old_size;
    dest->pub.free_in_buffer = new_size - old_size;
    dest->buf = new_buf;
    dest->size = new_size;
    return TRUE;
}

static void term_mem_destination(j_compress_ptr cinfo)
{
    struct mem_dest_mgr *dest;
    dest = (struct mem_dest_mgr *)cinfo->dest;
    dest->used = dest->size - dest->pub.free_in_buffer;
}

int tj_jpeg_encode(const uint8_t *gray, uint32_t width, uint32_t height,
                   int quality, const char *subsampling,
                   uint8_t **out_jpeg, uint64_t *out_len)
{
    struct jpeg_compress_struct cinfo;
    struct my_error_mgr jerr;
    struct mem_dest_mgr dest;
    JSAMPROW row_pointer[1];
    int row_stride;
    uint8_t *buf;
    size_t buf_size;

    (void)subsampling;

    /* worst case JPEG is rarely larger than raw; double + margin for safety */
    buf_size = (size_t)width * (size_t)height * 2 + 65536;
    buf = (uint8_t *)malloc(buf_size);
    if (buf == NULL) {
        return -1;
    }

    cinfo.err = jpeg_std_error(&jerr.pub);
    jerr.pub.error_exit = my_error_exit;
    if (setjmp(jerr.setjmp_buffer)) {
        jpeg_destroy_compress(&cinfo);
        free(buf);
        return -1;
    }

    jpeg_create_compress(&cinfo);

    cinfo.dest = (struct jpeg_destination_mgr *)&dest;
    dest.pub.init_destination = init_mem_destination;
    dest.pub.empty_output_buffer = empty_mem_output_buffer;
    dest.pub.term_destination = term_mem_destination;
    dest.buf = buf;
    dest.size = buf_size;

    cinfo.image_width = width;
    cinfo.image_height = height;
    cinfo.input_components = 1;
    cinfo.in_color_space = JCS_GRAYSCALE;

    jpeg_set_defaults(&cinfo);
    jpeg_set_quality(&cinfo, quality, TRUE);
    cinfo.optimize_coding = TRUE;

    jpeg_start_compress(&cinfo, TRUE);

    row_stride = (int)width;
    while (cinfo.next_scanline < cinfo.image_height) {
        row_pointer[0] = (JSAMPROW)&gray[cinfo.next_scanline * row_stride];
        jpeg_write_scanlines(&cinfo, row_pointer, 1);
    }

    jpeg_finish_compress(&cinfo);

    *out_len = (uint64_t)dest.used;
    *out_jpeg = buf;

    jpeg_destroy_compress(&cinfo);
    return 0;
}

int tj_jpeg_decode(const uint8_t *jpeg_data, uint64_t jpeg_len,
                   uint8_t **out_gray, uint32_t *out_width,
                   uint32_t *out_height)
{
    struct jpeg_decompress_struct cinfo;
    struct my_error_mgr jerr;
    JSAMPARRAY buffer;
    int row_stride;
    uint8_t *gray;
    size_t gray_size;
    size_t row;

    cinfo.err = jpeg_std_error(&jerr.pub);
    jerr.pub.error_exit = my_error_exit;
    if (setjmp(jerr.setjmp_buffer)) {
        jpeg_destroy_decompress(&cinfo);
        return -1;
    }

    jpeg_create_decompress(&cinfo);
    jpeg_mem_src(&cinfo, jpeg_data, (unsigned long)jpeg_len);
    jpeg_read_header(&cinfo, TRUE);
    jpeg_start_decompress(&cinfo);

    *out_width = cinfo.output_width;
    *out_height = cinfo.output_height;
    row_stride = (int)(cinfo.output_width * cinfo.output_components);
    gray_size = (size_t)cinfo.output_width * cinfo.output_height;
    gray = (uint8_t *)malloc(gray_size);
    if (gray == NULL) {
        jpeg_destroy_decompress(&cinfo);
        return -1;
    }

    buffer = (*cinfo.mem->alloc_sarray)((j_common_ptr)&cinfo, JPOOL_IMAGE,
                                        (JDIMENSION)row_stride, 1);
    row = 0;
    while (cinfo.output_scanline < cinfo.output_height) {
        jpeg_read_scanlines(&cinfo, buffer, 1);
        memcpy(gray + row * cinfo.output_width, buffer[0],
               cinfo.output_width);
        row++;
    }

    jpeg_finish_decompress(&cinfo);
    jpeg_destroy_decompress(&cinfo);

    *out_gray = gray;
    return 0;
}
