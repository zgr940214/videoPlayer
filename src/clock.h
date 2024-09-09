#ifndef _CLOCK_H_
#define _CLOCK_H_
#include <windows.h>
#include <stdint.h>
#include <atomic>

#include "memory_pool.h"
// central clock tick ..
enum class TIMEUNIT{
    MILLISECONDS = 0,
    MICROSECONDS,
    NANOSECONDS
};
#ifdef _WIN32

    typedef struct clock_win32 {
        TIMEUNIT unit;
        LARGE_INTEGER frequency;
        LARGE_INTEGER start;
        LARGE_INTEGER end;
        bool started;
        std::atomic_int64_t tick; // atomic tick
        std::atomic_int64_t diff; 
    } sys_clock_t;

#elif defined(__linux__)



#endif
#include <stdio.h>
static int create_clock(sys_clock_t **clock, TIMEUNIT tu, memory_pool *pool) {
    *clock = (sys_clock_t *)mem_alloc(pool, sizeof(sys_clock_t), 1);
    memset(*clock, 0, sizeof(sys_clock_t));
    (*clock)->unit = tu;
    (*clock)->started = false;
#ifdef _WIN32
    QueryPerformanceFrequency(&(*clock)->frequency);
    printf("freq: %llu\n", (*clock)->frequency.QuadPart);
#elif defined(__linux__)
#endif
    return 0;
}; 

void start_clock(sys_clock_t *clock);

void tick_clock(sys_clock_t *clock);

static inline int64_t transform_clock_to_audio_pts(sys_clock_t* clock, int64_t sample_rate) {
    int64_t pts;
#ifdef _WIN32
    pts = clock->tick * sample_rate / clock->frequency.QuadPart;
#elif defined(__linux__)
#endif

return pts;
};

#include <stdio.h>
static inline int transform_clock_to_video_pts(sys_clock_t* clock, int64_t den, int64_t num, int frame_rate) {
    int64_t pts;
#ifdef _WIN32
    pts = (int64_t)((double)(clock->tick.load()) / (double)(clock->frequency.QuadPart * (double)den) / (double)num );
    printf("tick %llu, den %llu , num %llu, pts %llu\n", clock->tick.load(), den, num, pts);
#elif defined(__linux__)
#endif
    return pts;
};

#endif