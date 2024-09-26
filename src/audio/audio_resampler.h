#ifndef _AUDIO_RESAMPLER_H_
#define _AUDIO_RESAMPLER_H_

#include "audio_common.h"
#include <libavutil/avutil.h>
#include <libavformat/avformat.h>
#include <libavutil/opt.h>
#include <libavutil/channel_layout.h>
#include <libavutil/samplefmt.h>
#include <libswresample/swresample.h>


#define MAX_AV_PLANES 8

typedef struct audio_resampler_t {
	void							*data; 	// audio_source_t
    struct SwrContext       		*context;
	
	uint8_t							*output_buf[MAX_AV_PLANES];
	uint8_t							valid_chns;
	uint32_t						nbytes_per_channels;

	audio_format_info_t				*input_info;
	audio_format_info_t				*output_info;
} audio_resampler_t;

int create_audio_resampler(audio_resampler_t **resampler, void *src);

int resample_audio(uint8_t** input, uint32_t sample_count_per_chn, audio_resampler_t *resampler);

#endif