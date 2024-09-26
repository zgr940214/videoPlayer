#ifndef _AUDIO_SOURCE_H_
#define _AUDIO_SOURCE_H_
#include <stdint.h>
#include "circular_buf.h"
#include "memory_pool.h"
#include "audio_common.h"

#define MAX_AUDIO_CHNS      8

typedef struct audio_source_t {
    mem_pool_t               *pool;

    const AVCodecContext        *codec_ctx; //ref

    void                        *audio_device;
    void                        *audio_resampler; 
    const char                  *source_name;
    cqueue_t                    audio_buf;
} audio_source_t;


#endif