/* compat.h -- VS2013 / C89 compatibility helpers */
#ifndef TJPEG_COMPAT_H
#define TJPEG_COMPAT_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stddef.h>

/* MSVC before VS2015 lacks snprintf; use _vsnprintf_s with truncation */
#ifdef _MSC_VER
#  if _MSC_VER < 1900
#    include <stdarg.h>
#    include <stdio.h>
static int tjpeg_vsnprintf(char *buf, size_t size, const char *fmt, va_list ap)
{
    int n;
    n = _vsnprintf_s(buf, size, _TRUNCATE, fmt, ap);
    if (n < 0) {
        /* truncated or error: ensure null termination */
        if (size > 0) {
            buf[size - 1] = '\0';
        }
        return (int)size;
    }
    return n;
}
static int tjpeg_snprintf(char *buf, size_t size, const char *fmt, ...)
{
    int n;
    va_list ap;
    va_start(ap, fmt);
    n = tjpeg_vsnprintf(buf, size, fmt, ap);
    va_end(ap);
    return n;
}
#  else
#    define tjpeg_snprintf snprintf
#  endif
#  define TJ_INLINE __inline
#  define TJ_RESTRICT __restrict
#  ifndef __func__
#    define __func__ __FUNCTION__
#  endif
#else
#  include <stdarg.h>
#  include <stdio.h>
#  define tjpeg_snprintf snprintf
#  define TJ_INLINE inline
#  define TJ_RESTRICT restrict
#endif

#ifndef M_PI
#  define M_PI 3.14159265358979323846
#endif

/* simple boolean type, no stdbool.h dependency */
typedef int tj_bool;
#define TJ_FALSE 0
#define TJ_TRUE  1

#ifdef __cplusplus
}
#endif

#endif /* TJPEG_COMPAT_H */
