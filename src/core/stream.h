#ifndef _STREAM_H_
#define _STREAM_H_
#include "core/core.h"
#include "audio/audio.h"
#include "video/video_codec.h"

typedef enum {
    STREAM_UNKNOWN = 0,
    STREAM_VIDEO,
    STREAM_AUDIO,
    STREAM_SUBTITLE
} stream_type;

struct sh_stream_t {
    demuxer_t           *demuxer; //ref
    stream_type         st_type;
    codec_params_t      *codec;

    //STREAM VIDEO
    enum AVPixelFormat  pixel_fmt;
    uint32_t            width;
    uint32_t            height;
    uint32_t            bytes_per_pixel;
};


#endif