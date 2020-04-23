#ifndef COMMON_H
#define COMMON_H

#include <stdint.h>

#define arraylen(x) ((int) (sizeof(x) / sizeof(*(x))))

#define MIN(x, y) ((x) < (y) ? (x) : (y))
#define MAX(x, y) ((x) > (y) ? (x) : (y))

#define likely(expr)   __builtin_expect(!!(expr), 1)
#define unlikely(expr) __builtin_expect(!!(expr), 0)

#define ALWAYS_INLINE inline __attribute__((always_inline))
#define NOINLINE __attribute__((noinline))

/* silence warnings about void const */
#define VOIDPTR(ptr) \
    (void *)(uintptr_t)(ptr)

#endif /* COMMON_H */

