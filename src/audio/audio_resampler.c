#include "audio.h"
#include "audio_resampler.h"
#include "audio_device.h"

static inline int create_swr_context(audio_resampler_t *resampler) {
    int64_t src_ch_layout = resampler->input_info->chn_layout; 
    int64_t dst_ch_layout = resampler->output_info->chn_layout;
    int src_rate = resampler->input_info->nSamplesPerSec; 
    int dst_rate = resampler->output_info->nSamplesPerSec;
    enum AVSampleFormat src_sample_fmt = resampler->input_info->sampleFormat;
    enum AVSampleFormat dst_sample_fmt = resampler->output_info->sampleFormat;

    struct SwrContext *swr_ctx = swr_alloc();
    if (!swr_ctx) {
        fprintf(stderr, "Could not allocate resampler context\n");
        return -1;
    }

    // 设置选项
    av_opt_set_int(swr_ctx, "in_channel_layout",    src_ch_layout, 0);
    av_opt_set_int(swr_ctx, "in_sample_rate",       src_rate, 0);
    av_opt_set_sample_fmt(swr_ctx, "in_sample_fmt", src_sample_fmt, 0);
    av_opt_set_int(swr_ctx, "out_channel_layout",    dst_ch_layout, 0);
    av_opt_set_int(swr_ctx, "out_sample_rate",       dst_rate, 0);
    av_opt_set_sample_fmt(swr_ctx, "out_sample_fmt", dst_sample_fmt, 0);

    // 初始化 SwrContext
    if (swr_init(swr_ctx) < 0) {
        fprintf(stderr, "Failed to initialize the resampling context\n");
        swr_free(&swr_ctx);
        return -1;
    }
    resampler->context = swr_ctx;
    return 0;
};

int create_audio_resampler(audio_resampler_t **resampler, void *source) {
    if (!resampler) {
        fprintf(stderr, "audio_resampler** null ptr. %s, %s\n", __FILE__, __LINE__);
        return -1;
    }
    if (*resampler) {
        fprintf(stderr, "audio_resampler* already allocated. %s, %s\n", __FILE__, __LINE__);
    }

    if (source) {
        audio_source_t *src = (audio_source_t *) source;
        *resampler = (audio_resampler_t *)
                        mem_alloc(src->pool, sizeof(audio_resampler_t), 1);
        (*resampler)->data = source;
        (*resampler)->input_info = (audio_format_info_t *)
                        mem_alloc(src->pool, sizeof(audio_format_info_t), 1);
        (*resampler)->output_info = (audio_format_info_t *)
                        mem_alloc(src->pool, sizeof(audio_format_info_t), 1);  

        audio_device_t *dev = (audio_device_t *)src->audio_device;
        GetAudioDevFormat(dev, (*resampler)->output_info);
        Codec2AudioFomat(src->codec_ctx, (*resampler)->input_info);
    }
    return 0;
};

int resample_audio(uint8_t** input, 
                uint32_t sample_count_per_chn, audio_resampler_t *resampler) {
    // 输入和输出参数
    int64_t src_ch_layout = resampler->input_info->chn_layout; 
    int64_t dst_ch_layout = resampler->output_info->chn_layout;
    int src_rate = resampler->input_info->nSamplesPerSec; 
    int dst_rate = resampler->output_info->nSamplesPerSec;
    enum AVSampleFormat src_sample_fmt = resampler->input_info->sampleFormat;
    enum AVSampleFormat dst_sample_fmt = resampler->output_info->sampleFormat;

    // 创建 SwrContext
    if (resampler->context == NULL) {
        create_swr_context(resampler);
    }

    int64_t max_output_samples = 
            av_rescale_rnd(sample_count_per_chn, dst_rate, src_rate, AV_ROUND_UP);

    int num_per_chn = swr_convert(resampler->context, &resampler->output_buf[0], 
                    max_output_samples,
                    input, sample_count_per_chn);
    if (num_per_chn < 0) {
        audio_source_t *src = (audio_source_t *)resampler->data;
        fprintf(stderr, "failed to convert samples from source: %s\n", 
            src->source_name);
        return -1;
    }
    
    return 0;
};