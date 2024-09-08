#ifndef _CLOCK_H_
#define _CLOCK_H_
#include <windows.h>
#include <stdint.h>
#include <atomic>

#include "memory_pool.h"
// central clock tick ..
#ifdef _WIN32

    typedef struct clock_win32 {
        enum TIMEUNIT{
            MILLISECONDS = 0,
            MICROSECONDS,
            NANOSECONDS
        } unit;
        LARGE_INTEGER frequency;
        LARGE_INTEGER start;
        LARGE_INTEGER end;
        std::atomic_bool started;
        std::atomic_int64_t tick; // atomic tick
        std::atomic_int64_t diff; 
    } clock_t;

#elif defined(__linux__)



#endif

static int create_clock(clock_t **clock, clock_t::TIMEUNIT unit, memory_pool *pool) {
    *clock = (clock_t *)mem_alloc(pool, sizeof(clock_t), 1);
    (*clock)->unit = unit;
#ifdef _WIN32
    QueryPerformanceFrequency(&(*clock)->frequency);
#elif defined(__linux__)
#endif
    return 0;
}; 

void start_clock(clock_t *clock);

void tick_clock(clock_t *clock);

static inline int64_t transform_clock_to_audio_pts(clock_t* clock, int64_t sample_rate) {
    int64_t pts;
#ifdef _WIN32
    pts = clock->tick * sample_rate / clock->frequency.QuadPart;
#elif defined(__linux__)
#endif

return pts;
};

static inline int transform_clock_to_video_pts(clock_t* clock, int64_t den, int frame_rate) {
    int64_t pts;
#ifdef _WIN32
    pts = clock->tick * den / clock->frequency.QuadPart;
#elif defined(__linux__)
#endif
};

#endif