# include <stdio.h>

#ifndef DEBUG_LEVEL

# ifdef __GNUC__

#  define dbprintf(args...) while(0)
# else

#  define dbprintf(...) while(0)
# endif

#else

# ifdef __GNUC__
#  define dbprintf(level, args...) do { \
        if((level) <= DEBUG_LEVEL)      \
            fprintf(stderr, args);      \
    } while(0)
# else
#  define dbprintf(level, ...) do {         \
        if((level) <= DEBUG_LEVEL)          \
            fprintf(stderr, __VA_ARGS__);   \
    } while(0)
# endif

#endif

#ifndef NDEBUG

# include <stdio.h>

#if __STDC_VERSION__ < 199901L && !defined(__func__)
# if __GNUC__ >= 2
#  define __func__ __FUNCTION__
# else
#  define __func__ "<unknown>"
# endif
#endif

# define zassert(condition, retval, format, ...) do {                   \
        if(!(condition)) {                                              \
            fprintf(stderr, "WARNING: %s:%d: %s:"                       \
                    " Assertion \"%s\" failed.\n\t" format,             \
                    __FILE__, __LINE__, __func__, #condition ,          \
                    ##__VA_ARGS__);                                     \
            return(retval);                                             \
        }                                                               \
    } while(0)

#else

# define zassert(condition, retval, format, ...) do {   \
        if(!(condition))                                \
            return(retval);                             \
    } while(0)

#endif
