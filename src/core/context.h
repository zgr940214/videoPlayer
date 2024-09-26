#ifndef _CONTEXT_H_
#define _CONTEXT_H_
#include "core/core.h"

struct player_context {
    input_context_t     *input_ctx; // 处理用户输入
    ao                  *audio_out; // 音频最终输出
    vo                  *video_out; // 视频最终输出
    demuxer_t           *demuxer;   // 解码/解复用器

    const char *url;
}

#endif