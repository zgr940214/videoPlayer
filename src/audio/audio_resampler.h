#ifndef _AUDIO_RESAMPLER_H_
#define _AUDIO_RESAMPLER_H_

#include "audio_common.h"
extern "C" {
    #include <libavutil/avutil.h>
    #include <libavformat/avformat.h>
	#include <libavutil/opt.h>
	#include <libavutil/channel_layout.h>
	#include <libavutil/samplefmt.h>
    #include <libswresample/swresample.h>
}

#define MAX_AV_PLANES 8

typedef struct audio_resampler {
	void							*data; 	// audio_source_t
    struct SwrContext       		*context;
	
	uint8_t							*output_buf[MAX_AV_PLANES];
	const audio_format_info_t		*input_info;
	const audio_format_info_t		*output_info;
} audio_resampler_t;

int create_audio_resampler(audio_resampler_t **resampler, void *src);

int init_audio_resampler(audio_resampler_t *resampler, 
							audio_format_info_t *src_fmt, audio_format_info_t *dev_fmt);

int resample(uint8_t** input, uint32_t sample_count_per_chn, audio_resampler_t *resampler);

#endif