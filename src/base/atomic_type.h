#ifndef _ATOMIC_TYPE_H_
#define _ATOMIC_TYPE_H_
typedef volatile int                vp_atomic_int;
typedef volatile unsigned int       vp_atomic_uint;
typedef volatile long long          vp_atomic_int64_t;
typedef volatile unsigned long long vp_atomic_uint64_t;


typedef vp_atomic_int               vp_atomic_int32_t;
typedef vp_atomic_uint              vp_atomic_uint32_t;

#endif