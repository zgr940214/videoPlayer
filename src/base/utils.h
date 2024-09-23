#ifndef _UTILS_H_
#define _UTILS_H_
#include <assert.h>

#define ASRT_EQ(a, b)   assert((a) == (b))
#define ASRT_NEQ(a, b)  assert((a) != (b))
#define ASRT_GT(a, b)   assert((a) > (b))
#define ASRT_GE(a, b)   assert((a) >= (b))
#define ASRT_LT(a, b)   assert((a) < (b))
#define ASRT_LE(a, b)   assert((a) <= (b))

#endif