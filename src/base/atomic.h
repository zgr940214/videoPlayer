#ifndef _ATOMIC_H_
#define _ATOMIC_H_

#ifdef _MSC_VER
    
#include "base/atomic_msvc_sup.h"

#elif defined(__GNUC__)

#if defined(__amd64__) || defined(__x86_64__)

#include "base/atomic_gcc_amd64_sup.h"

#endif

#endif

#endif