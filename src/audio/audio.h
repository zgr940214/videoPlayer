#ifndef _AUDIO_SOURCE_H_
#define _AUDIO_SOURCE_H_
#include <cstdint>
#include "circular_buf.h"
#include "memory_pool.h"

#define MAX_AUDIO_CHNS      8

typedef struct audio_source {
    memory_pool     *pool;

    void            *audio_device;
    void            *audio_resampler; 
    const char      *source_name;
    cqueue_t        audio_buf;

} audio_source_t;


#endif