#ifndef __GLOBAL_MARCO_H__
#define __GLOBAL_MARCO_H__

#define EXEC_JNI_ONLOAD_FATAL		(-1)

#define PACKED(x) __attribute__ ((__aligned__(x), __packed__))

#define LIKELY(x)       __builtin_expect((x), true)
#define UNLIKELY(x)     __builtin_expect((x), false)

#ifdef PAGE_SHIFT
#define SYSTEM_PAGE_SIZE        (1<<PAGE_SHIFT)
#else
#define SYSTEM_PAGE_SIZE        4096
#endif

#define ALIGN_UP(x, n) (((size_t)(x) + (n) - 1) & ~((n) - 1))
#define ALIGN_DOWN(x, n) ((size_t)(x) & -(n))
#define ALIGN_UP_TO_PAGE_SIZE(p) ALIGN_UP(p, SYSTEM_PAGE_SIZE)
#define ALIGN_DOWN_TO_PAGE_SIZE(p) ALIGN_DOWN(p, SYSTEM_PAGE_SIZE)

#define CPU_CACHE_WIDTH         32
#define CPU_CACHE_WIDTH_1       (CPU_CACHE_WIDTH-1)

#ifndef O_BINARY
#define O_BINARY 0
#endif

#endif
