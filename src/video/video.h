// common struct class var
// 公共数据结构，定义在这里
#ifndef _VIDEO_H_
#define _VIDEO_H_

#include <iostream>
#include <string.h>

#include "video_common.h"
#include "video_codec.h"
#include "memory_pool.h"
#include "circular_buf.h"

#define MAX_AUDIO_STREAM    8
#define MAX_AUDIO_CHNS      8

typedef struct video_format_context {
    void                *data;

    AVFormatContext     *fm_fmt_ctx;
    int                  fm_vstream_id;
    int                  fm_astream_id[MAX_AUDIO_STREAM];

    const char          *filename;


} video_format_context_t;

typedef struct video_source {
    void                        *data;
    const char                  *file_name;
    video_format_context_t      *fmt_ctx;
    video_codec_t               *codec_ctx;

    cqueue_t                    *vframe_queue;
    cqueue_t                    *aframe_queue;
    
    mem_pool_t                  *pool;
} video_source_t;

typedef struct video_frame {
    char        *vdata;
    int64_t     pts;
} video_frame_t;

static video_source_t* video_source_create(const char* filename, mem_pool_t *p, ) {
    video_source_t *src = (video_source_t*)mem_alloc(p, sizeof(video_source_t), 1);
    src->pool = p;
    src->file_name = strdup(filename);
    if (src->file_name == NULL) {
        mem_free(p, src);
        return NULL;
    }

    video_fmt_context_create(filename, src);
    
}

int video_fmt_context_create(const char *filename, video_source_t *src) {
    int ret;
    AVFormatContext **ctx = &src->fmt_ctx->fm_fmt_ctx;
    if (avformat_open_input(ctx, filename, NULL, NULL)) {
        fprintf(stderr, "Could not open source file %s\n", filename);
        return -1;
    }

    if (avformat_find_stream_info(*ctx, NULL) < 0) {
        fprintf(stderr, "Could not find stream information\n");
        return -1;
    }

    //创建并且初始化 video audio 的解码器
    (void)create_codec_context(src);


}

#endif