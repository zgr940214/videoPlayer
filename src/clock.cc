#include "clock.h"
#include <assert.h>

void start_clock(clock_t *clock) {
    QueryPerformanceCounter(&clock->start);
    QueryPerformanceCounter(&clock->end);
    clock->tick = clock->end.QuadPart - clock->start.QuadPart;
    clock->started.store(true, std::memory_order_release);
};

void tick_clock(clock_t *clock) {
    if (!clock->started) {
        start_clock(clock);
        return;
    }
    QueryPerformanceCounter(&clock->end);
    clock->tick.store(clock->end.QuadPart - clock->start.QuadPart, std::memory_order_relaxed);
};