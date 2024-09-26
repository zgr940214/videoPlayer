#ifndef _VIDEO_CODEC_H_
#define _VIDEO_CODEC_H_
#include "video_common.h"

#include <libavutil/samplefmt.h>
#include <libavutil/channel_layout.h>

#ifdef _WIN32
    #include <windows.h>
    #include <mmsystem.h>
    #include <mmreg.h>
#endif

#ifdef __linux__
    #include <alsa/asoundlib.h>
#endif

// 封装 avcodec 初始化等操作 看情况 实现解码部分线程的功能函数
// 需要有能够根据文件读取信息 以及 初始化对应的解码器的能力
// 需要有固定解码的功能函数，并且我们需要一个状态结构 记录当前播放位置，以便能实现一个 寻址播放的功能。
// 后续可能实现快进 慢进 播放


typedef struct codec_params
{
    void                *data;

    // VIDEO_CODEC
    AVCodecContext      *fmv_codec_ctx; // video codec
    enum AVPixelFormat  pix_fmt;
    uint8_t             plane_count;
    uint16_t            frame_width;
    uint16_t            frame_height; 
    uint32_t            video_line_size;

    uint32_t            time_base_numv;
    uint32_t            time_base_denv;

    // AUDIO_CODEC
    AVCodecContext      *fma_codec_ctx; // audio codec
    uint8_t             channels;   
    uint32_t            samplerate;
    uint32_t            bitrate;
    uint32_t            frame_size;
    uint32_t            delay;

#ifdef _WIN32
    WORD                sample_format;
#elif defined(__linux__)
    snd_pcm_format_t    sample_format;
#endif

    uint32_t            time_base_numa;
    uint32_t            time_base_dena;
} codec_params_t;

int open_codec_context(int *stream_idx,
    AVCodecContext **dec_ctx, AVFormatContext *fmt_ctx, enum AVMediaType type);

int decode_packet(AVCodecContext *dec, const AVPacket *pkt);

codec_params_t *create_codec_context(void* data);

#endif