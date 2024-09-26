#include "video_codec.h"
#include "video.h"

#include <stdio.h>

int open_codec_context(int *stream_idx,
                              AVCodecContext **dec_ctx, AVFormatContext *fmt_ctx, enum AVMediaType type)
{
    int                ret;
    AVStream          *st;
    const AVCodec     *dec;
    ret = av_find_best_stream(fmt_ctx, type, -1, -1, NULL, 0);
    if (ret < 0) {
        fprintf(stderr, "Could not find %s stream in input file '%s'\n",
                av_get_media_type_string(type), fmt_ctx->url);
        return -1;
    } else {
        *stream_idx = ret;
        st = fmt_ctx->streams[*stream_idx]; 

        dec = avcodec_find_decoder(st->codecpar->codec_id);
        if (!dec) {
            fprintf(stderr, "Failed to find %s codec\n",
                    av_get_media_type_string(type));
            return AVERROR(EINVAL);
        }

        *dec_ctx = avcodec_alloc_context3(dec);
        if (!*dec_ctx) {
            fprintf(stderr, "Failed to allocate the %s codec context\n",
                    av_get_media_type_string(type));
            return AVERROR(ENOMEM);
        }

        if ((ret = avcodec_parameters_to_context(*dec_ctx, st->codecpar)) < 0) {
            fprintf(stderr, "Failed to copy %s codec parameters to decoder context\n",
                    av_get_media_type_string(type));
            return ret;
        }

        if ((ret = avcodec_open2(*dec_ctx, dec, NULL)) < 0) {
            fprintf(stderr, "Failed to open %s codec\n",
                    av_get_media_type_string(type));
            return ret;
        }
    }
    return 0;
}




int decode_packet(AVCodecContext *dec, const AVPacket *pkt)
{
    int ret = 0;
 
    // submit the packet to the decoder
    ret = avcodec_send_packet(dec, pkt);
    if (ret < 0) {
        fprintf(stderr, "Error submitting a packet for decoding (%s)\n", av_err2str(ret));
        return ret;
    }
 
    // get all the available frames from the decoder
    while (ret >= 0) {
        ret = avcodec_receive_frame(dec, frame);
        if (ret < 0) {
            // those two return values are special and mean there is no output
            // frame available, but there were no errors during decoding
            if (ret == AVERROR_EOF || ret == AVERROR(EAGAIN))
                return 0;
 
            fprintf(stderr, "Error during decoding (%s)\n", av_err2str(ret));
            return ret;
        }
 
        // write the frame data to output file
        if (dec->codec->type == AVMEDIA_TYPE_VIDEO)
            ret = output_video_frame(frame);
        else
            ret = output_audio_frame(frame);
 
        av_frame_unref(frame);
        if (ret < 0)
            return ret;
    }
 
    return 0;
}

static int output_video_frame(AVFrame *frame, video_source_t *src)
{
    int width = src->codec_ctx->frame_width;
    int height = src->codec_ctx->frame_height;
    enum AVPixelFormat pix_fmt = src->codec_ctx->pix_fmt;

    if (frame->width != width || frame->height != height ||
        frame->format != pix_fmt) {
        /* To handle this change, one could call av_image_alloc again and
         * decode the following frames into another rawvideo file. */
        fprintf(stderr, "Error: Width, height and pixel format have to be "
                "constant in a rawvideo file, but the width, height or "
                "pixel format of the input video changed:\n"
                "old: width = %d, height = %d, format = %s\n"
                "new: width = %d, height = %d, format = %s\n",
                width, height, av_get_pix_fmt_name(pix_fmt),
                frame->width, frame->height,
                av_get_pix_fmt_name(frame->format));
        return -1;
    }
 
    //printf("video_frame n:%d\n",
    //      video_frame_count++);
 
    /* copy decoded frame to destination buffer:
     * this is required since rawvideo expects non aligned data */
    cqueue_t *q = src->vframe_queue;
    cq_elem_t* elem = circular_queue_get_next(q);
    av_image_copy2(elem->data, src->codec_ctx->video_line_size,
                   frame->data, frame->linesize,
                   pix_fmt, width, height);
    
    /* write to rawvideo file */
    fwrite(video_dst_data[0], 1, video_dst_bufsize, video_dst_file);
    return 0;
}
 
static int output_audio_frame(AVFrame *frame)
{
    size_t unpadded_linesize = frame->nb_samples * av_get_bytes_per_sample(frame->format);
    printf("audio_frame n:%d nb_samples:%d pts:%s\n",
           audio_frame_count++, frame->nb_samples,
           av_ts2timestr(frame->pts, &audio_dec_ctx->time_base));
 
    /* Write the raw audio data samples of the first plane. This works
     * fine for packed formats (e.g. AV_SAMPLE_FMT_S16). However,
     * most audio decoders output planar audio, which uses a separate
     * plane of audio samples for each channel (e.g. AV_SAMPLE_FMT_S16P).
     * In other words, this code will write only the first audio channel
     * in these cases.
     * You should use libswresample or libavfilter to convert the frame
     * to packed data. */
    fwrite(frame->extended_data[0], 1, unpadded_linesize, audio_dst_file);
 
    return 0;
}

#ifdef _WIN32 
static inline 
WORD ffmpeg_to_wave_format(enum AVSampleFormat fmt) {
    switch (fmt) {
        case AV_SAMPLE_FMT_U8:
            return WAVE_FORMAT_PCM;
        case AV_SAMPLE_FMT_S16:
            return WAVE_FORMAT_PCM;
        case AV_SAMPLE_FMT_S32:
            return WAVE_FORMAT_PCM;
        case AV_SAMPLE_FMT_FLT:
            return WAVE_FORMAT_IEEE_FLOAT;
        case AV_SAMPLE_FMT_DBL:
            return WAVE_FORMAT_IEEE_FLOAT;
        default:
            return WAVE_FORMAT_UNKNOWN;
    }
}
#endif

#ifdef __linux__


#endif

int avcodec_to_codec_context(video_codec_t *codec_ctx) {
    AVCodecContext *cdv, *cda;
    cdv = codec_ctx->fmv_codec_ctx;
    cda = codec_ctx->fma_codec_ctx;

    if (cdv) {
        codec_ctx->pix_fmt = cdv->pix_fmt;
        codec_ctx->frame_width = cdv->width;
        codec_ctx->frame_height = cdv->height;
        codec_ctx->time_base_numv = cdv->time_base.num;
        codec_ctx->time_base_denv = cdv->time_base.den;
        codec_ctx->plane_count = av_pix_fmt_count_planes(cdv->pix_fmt);
        //codec_ctx->video_line_size
    } 

    if (cda) {
        codec_ctx->samplerate = cda->sample_rate;
        codec_ctx->time_base_dena = cda->time_base.den;
        codec_ctx->time_base_numa = cda->time_base.num;
        codec_ctx->sample_format = ffmpeg_to_wave_format(cda->sample_fmt);
        codec_ctx->delay = cda->delay;
        codec_ctx->channels = av_get_channel_layout_nb_channels(cda->ch_layout);
    }
}

int create_codec_context(void* data) {
    int ret;
    video_source_t  *src = (video_source_t *)data;
    codec_params_t   *codec_ctx;
    src->codec_ctx = mem_alloc(src->pool, sizeof(codec_params_t), 1);
    codec_ctx = src->codec_ctx;

    // 初始化 视频解码器context
    ret = open_codec_context(&src->fmt_ctx->fm_vstream_id, 
            &codec_ctx->fmv_codec_ctx, src->fmt_ctx, AVMEDIA_TYPE_VIDEO);
    if (ret < 0) {
        fprintf(stderr, "Could not open codec context for video stream\n");
        goto failed;
    }
    src->codec_ctx->data = src;
    
    // 初始化 音频解码器context
    ret = open_codec_context(&src->fmt_ctx->fm_astream_id, 
            &codec_ctx->fma_codec_ctx, src->fmt_ctx, AVMEDIA_TYPE_AUDIO);
    if (ret < 0) {
        fprintf(stderr, "Could not open codec context for audio stream\n");
        goto failed;
    }
    src->acodec_ctx->data = src;

    return 0;

failed:
    if (codec_ctx->fma_codec_ctx) {
        avcodec_close(codec_ctx->fma_codec_ctx);
        avcodec_free_context(codec_ctx->fma_codec_ctx);
    }

    if (codec_ctx->fmv_codec_ctx) {
        avcodec_close(codec_ctx->fmv_codec_ctx);
        avcodec_free_context(codec_ctx->fmv_codec_ctx);
    }
    mem_free(src->pool, codec_ctx);
    return -1;
};