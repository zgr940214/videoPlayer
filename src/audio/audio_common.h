#ifndef _AUDIO_COMMON_H_
#define _AUDIO_COMMON_H_

extern "C" {
    #include <libavcodec/avcodec.h>    // 编解码器相关功能
    #include <libavformat/avformat.h>  // 容器格式处理
    #include <libavutil/samplefmt.h>   // 音频样本格式定义
    #include <libavutil/avutil.h>      // 工具集（日志等）
    #include <libswresample/swresample.h> // 音频重采样
}


enum class audio_format {
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


static inline enum AVSampleFormat convert_audio_format_d2f(enum audio_format format)
{
	switch (format) {
	case audio_format::AUDIO_FORMAT_UNKNOWN:
		return AV_SAMPLE_FMT_S16;
	case audio_format::AUDIO_FORMAT_U8BIT:
		return AV_SAMPLE_FMT_U8;
	case audio_format::AUDIO_FORMAT_16BIT:
		return AV_SAMPLE_FMT_S16;
	case audio_format::AUDIO_FORMAT_32BIT:
		return AV_SAMPLE_FMT_S32;
	case audio_format::AUDIO_FORMAT_FLOAT:
		return AV_SAMPLE_FMT_FLT;
	case audio_format::AUDIO_FORMAT_U8BIT_PLANAR:
		return AV_SAMPLE_FMT_U8P;
	case audio_format::AUDIO_FORMAT_16BIT_PLANAR:
		return AV_SAMPLE_FMT_S16P;
	case audio_format::AUDIO_FORMAT_32BIT_PLANAR:
		return AV_SAMPLE_FMT_S32P;
	case audio_format::AUDIO_FORMAT_FLOAT_PLANAR:
		return AV_SAMPLE_FMT_FLTP;
	}

	/* shouldn't get here */
	return AV_SAMPLE_FMT_S16;
}

static inline enum audio_format convert_audio_format_f2d(enum AVSampleFormat format)
{
	switch (format) {
	case AV_SAMPLE_FMT_U8:
		return audio_format::AUDIO_FORMAT_U8BIT;
	case AV_SAMPLE_FMT_S16: 
		return audio_format::AUDIO_FORMAT_16BIT;
	case AV_SAMPLE_FMT_S32:
		return audio_format::AUDIO_FORMAT_32BIT;
	case AV_SAMPLE_FMT_FLT: 
		return audio_format::AUDIO_FORMAT_FLOAT;
	case AV_SAMPLE_FMT_U8P:
		return audio_format::AUDIO_FORMAT_U8BIT_PLANAR;
	case AV_SAMPLE_FMT_S16P:
		return audio_format::AUDIO_FORMAT_16BIT_PLANAR;
	case AV_SAMPLE_FMT_S32P:
		return audio_format::AUDIO_FORMAT_32BIT_PLANAR;
	case AV_SAMPLE_FMT_FLTP:
		return audio_format::AUDIO_FORMAT_FLOAT_PLANAR;
	}

	/* shouldn't get here */
	return audio_format::AUDIO_FORMAT_UNKNOWN;
}


static inline size_t get_audio_bytes_per_channel(audio_format format)
{
	switch (format) {
	case audio_format::AUDIO_FORMAT_U8BIT:
	case audio_format::AUDIO_FORMAT_U8BIT_PLANAR:
		return 1;

	case audio_format::AUDIO_FORMAT_16BIT:
	case audio_format::AUDIO_FORMAT_16BIT_PLANAR:
		return 2;

	case audio_format::AUDIO_FORMAT_FLOAT:
	case audio_format::AUDIO_FORMAT_FLOAT_PLANAR:
	case audio_format::AUDIO_FORMAT_32BIT:
	case audio_format::AUDIO_FORMAT_32BIT_PLANAR:
		return 4;

	case audio_format::AUDIO_FORMAT_UNKNOWN:
		return 0;
	}

	return 0;
}

#endif