
#include "atomic_type.h"
/*
 * "cmpxchgq  r, [m]":
 *
 *     if (rax == [m]) {
 *         zf = 1;
 *         [m] = r;
 *     } else {
 *         zf = 0;
 *         rax = [m];
 *     }
 *
 *
 * The "r" is any register, %rax (%r0) - %r16.
 * The "=a" and "a" are the %rax register.
 * Although we can return result in any register, we use "a" because it is
 * used in cmpxchgq anyway.  The result is actually in %al but not in $rax,
 * however as the code is inlined gcc can test %al as well as %rax.
 *
 * The "cc" means that flags were changed.
 */

static inline int 
vp_atomic_exchange(vp_atomic_int *ptr, int value) {
    int ret = value;
    __asm__ volatile(
        "       lock;         "
        "       xchgl %1, %0; "
        : "=a" (ret)
        : "m" (ptr)
        : "memory"
    );
    return ret;
}

static inline long long 
vp_atomic_exchange_64(vp_atomic_int64_t *ptr, long long value) {
    long long ret = value;
    __asm__ volatile(
        "       lock;          "
        "       xchgq %1, %0;  "
        : "=a" (ret)
        : "m" (ptr)
        : "memory"
    );
    return ret;
}

static inline int
vp_atomic_fetch_add(vp_atomic_int *ptr, int value) {
    int ret = value;
    __asm__ volatile(
        "       lock;         "
        "       xaddl %1, %0; "
        : "=a" (ret)
        : "m" (ptr)
        : "cc", "memory"
    );
    return ret;
}   

static inline long long
vp_atomic_fetch_add64(vp_atomic_int64_t *ptr, long long value) {
    long long ret = value;
    __asm__ volatile(
        "       lock;         "
        "       xaddq %1, %0; "
        : "=a" (ret)
        : "m" (ptr)
        : "cc", "memory"
    );
    return ret;
};

static inline int 
vp_atomic_inc(vp_atomic_int32_t *ptr) {
    return vp_atomic_fetch_add(ptr, 1);
};

static inline long long
vp_atomic_inc64(vp_atomic_int64_t *ptr) {
    return vp_atomic_fetch_add64(ptr, 1);
}

static inline int 
vp_atomic_compare_exchange(vp_atomic_int *ptr, long desired, long expected) {
    unsigned char  res;

    __asm__ volatile (

    "    lock;               "
    "    cmpxchgl   %3, %1;  "
    "    sete      %0;       "

    : "=a" (res) : "m" (*ptr), "a" (expected), "r" (desired) : "cc", "memory");

    return res;
}

static inline int 
vp_atomic_compare_exchange_64(vp_atomic_int64_t *ptr, long long desired, long long expected) {
    unsigned char  res;

    __asm__ volatile (

    "    lock;               "
    "    cmpxchgq  %3, %1;   "
    "    sete      %0;       "

    : "=a" (res) : "m" (*ptr), "a" (expected), "r" (desired) : "cc", "memory");

    return res;
}

#define vp_memory_barrier()    __asm__ volatile ("" ::: "memory")

#define vp_cpu_pause()         __asm__ ("pause")
