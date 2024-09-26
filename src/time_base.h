#ifndef _UTIME_BASE_H_
#define _UTIME_BASE_H_
#include <stdint.h>

typedef struct utime_base_t {
    uint64_t num;
    uint64_t den;    
} utime_base_t;

/// @brief av_rescale for unsigned
/// @param x 
/// @param num 
/// @param den 
/// @return scaled value
static inline uint64_t uint64_scale(uint64_t x, uint64_t num, uint64_t den) {
    return (x / den) * num
        + ((x % den) * (num / den))
        + ((x % den) * (num % den)) / den;
}

/// @brief av_rescale for unsigned
/// @param x 
/// @param time_base
/// @return scaled value
static inline uint64_t uint64_scale_tb(uint64_t x, utime_base_t time_base) {
    return (x / time_base.den) * time_base.num
        + ((x % time_base.den) * (time_base.num / time_base.den))
        + ((x % time_base.den) * (time_base.num % time_base.den)) / time_base.den;
}




#endif