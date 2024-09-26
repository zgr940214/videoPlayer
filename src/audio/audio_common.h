#ifndef _AUDIO_COMMON_H_
#define _AUDIO_COMMON_H_

#include <libavcodec/avcodec.h>    // 编解码器相关功能
#include <libavformat/avformat.h>  // 容器格式处理
#include <libavutil/samplefmt.h>   // 音频样本格式定义
#include <libavutil/avutil.h>      // 工具集（日志等）
#include <libswresample/swresample.h> // 音频重采样



enum audio_format {
	AUDIO_FORMAT_UNKNOWN,

	AUDIO_FORMAT_U8BIT,
	AUDIO_FORMAT_16BIT,
	AUDIO_FORMAT_32BIT,
	AUDIO_FORMAT_FLOAT,

	AUDIO_FORMAT_U8BIT_PLANAR,
	AUDIO_FORMAT_16BIT_PLANAR,
	AUDIO_FORMAT_32BIT_PLANAR,
	AUDIO_FORMAT_FLOAT_PLANAR,
};

typedef struct audio_format_info {
	uint16_t				wBitsPerSample;		
	uint16_t				nChannels;
	uint16_t				nBlockAlign;
	uint16_t				nBlocks;
	uint32_t				nSamplesPerSec;
	enum AVSampleFormat     sampleFormat;
	int64_t					chn_layout;
} audio_format_info_t;


static inline enum AVSampleFormat convert_sample_format_d2f(enum audio_format format)
{
	switch (format) {
	case AUDIO_FORMAT_UNKNOWN:
		return AV_SAMPLE_FMT_S16;
	case AUDIO_FORMAT_U8BIT:
		return AV_SAMPLE_FMT_U8;
	case AUDIO_FORMAT_16BIT:
		return AV_SAMPLE_FMT_S16;
	case AUDIO_FORMAT_32BIT:
		return AV_SAMPLE_FMT_S32;
	case AUDIO_FORMAT_FLOAT:
		return AV_SAMPLE_FMT_FLT;
	case AUDIO_FORMAT_U8BIT_PLANAR:
		return AV_SAMPLE_FMT_U8P;
	case AUDIO_FORMAT_16BIT_PLANAR:
		return AV_SAMPLE_FMT_S16P;
	case AUDIO_FORMAT_32BIT_PLANAR:
		return AV_SAMPLE_FMT_S32P;
	case AUDIO_FORMAT_FLOAT_PLANAR:
		return AV_SAMPLE_FMT_FLTP;
	}

	/* shouldn't get here */
	return AV_SAMPLE_FMT_S16;
}

static inline enum audio_format convert_sample_format_f2d(enum AVSampleFormat format)
{
	switch (format) {
	case AV_SAMPLE_FMT_U8:
		return AUDIO_FORMAT_U8BIT;
	case AV_SAMPLE_FMT_S16: 
		return AUDIO_FORMAT_16BIT;
	case AV_SAMPLE_FMT_S32:
		return AUDIO_FORMAT_32BIT;
	case AV_SAMPLE_FMT_FLT: 
		return AUDIO_FORMAT_FLOAT;
	case AV_SAMPLE_FMT_U8P:
		return AUDIO_FORMAT_U8BIT_PLANAR;
	case AV_SAMPLE_FMT_S16P:
		return AUDIO_FORMAT_16BIT_PLANAR;
	case AV_SAMPLE_FMT_S32P:
		return AUDIO_FORMAT_32BIT_PLANAR;
	case AV_SAMPLE_FMT_FLTP:
		return AUDIO_FORMAT_FLOAT_PLANAR;
	}

	/* shouldn't get here */
	return AUDIO_FORMAT_UNKNOWN;
}


static inline uint64_t get_audio_bytes_per_channel(enum audio_format format)
{
	switch (format) {
	case AUDIO_FORMAT_U8BIT:
	case AUDIO_FORMAT_U8BIT_PLANAR:
		return 1;

	case AUDIO_FORMAT_16BIT:
	case AUDIO_FORMAT_16BIT_PLANAR:
		return 2;

	case AUDIO_FORMAT_FLOAT:
	case AUDIO_FORMAT_FLOAT_PLANAR:
	case AUDIO_FORMAT_32BIT:
	case AUDIO_FORMAT_32BIT_PLANAR:
		return 4;

	case AUDIO_FORMAT_UNKNOWN:
		return 0;
	}

	return 0;
}

static inline void Codec2AudioFomat(const AVCodecContext *codec_ctx, audio_format_info_t *info) {
	info->nChannels = codec_ctx->channels;
	info->nSamplesPerSec = codec_ctx->sample_rate;
	info->sampleFormat = codec_ctx->sample_fmt;
	info->chn_layout = codec_ctx->channel_layout;

	info->wBitsPerSample = av_get_bytes_per_sample(codec_ctx->sample_fmt) * 8;
	info->nBlockAlign = info->nChannels * info->wBitsPerSample / 8;
}

#endif