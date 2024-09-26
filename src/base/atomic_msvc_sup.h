#include "atomic_type.h"
#include "windows.h"

static inline LONG 
vp_atomic_exchange(vp_atomic_int *ptr, LONG value) {
    return InterlockedExchange((volatile LONG *)ptr, value);
}

static inline LONGLONG 
vp_atomic_exchange_64(vp_atomic_int64_t *ptr, LONGLONG value) {
    return InterlockedExchange64(ptr, value);
}

static inline LONG
vp_atomic_fetch_add(vp_atomic_int *ptr, LONG value) {
    return InterlockedAdd((volatile LONG *)ptr, value);
}   

static inline LONGLONG
vp_atomic_fetch_add64(vp_atomic_int64_t *ptr, long long value) {
    return InterlockedAdd64(ptr, value);
};

static inline LONG 
vp_atomic_increament(vp_atomic_int32_t *ptr) {
    return InterlockedIncrement((volatile LONG *)ptr);
};

static inline LONGLONG
vp_atomic_increament64(vp_atomic_int64_t *ptr) {
    return InterlockedIncrement64(ptr);
}

static inline LONG 
vp_atomic_compare_exchange(vp_atomic_int *ptr, LONG desired, LONG expected) {
    return InterlockedCompareExchange((volatile LONG *)ptr, desired, expected);
}

static inline LONG
vp_atomic_compare_exchange_64(vp_atomic_int64_t *ptr, LONGLONG desired, LONGLONG expected) {
    LONGLONG before = InterlockedCompareExchange64(ptr, desired, expected);
    return before == expected;
}