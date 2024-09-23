#include <windows.h>
#include <mmsystem.h>
#include <functional>
#include <fstream>
#include <vector>
#include "audio_resampler.h"

#include <iostream>
#include <Audioclient.h>
#include <mmdeviceapi.h>
#include <functiondiscoverykeys.h>
#include <Propsys.h>

extern "C" {
    #include <libswresample/swresample.h>
    #include <libavutil/frame.h>
    #include <libavutil/imgutils.h>
    #include <libavcodec/avcodec.h>
    #include <libavformat/avformat.h>
    #include <libavformat/avio.h>
    #include <libavutil/error.h>
}

#pragma comment(lib, "winmm.lib")
#pragma comment(lib, "Propsys.lib")

#define ACTUALLY_DEFINE_GUID(name, l, w1, w2, b1, b2, b3, b4, b5, b6, b7, b8) \
	EXTERN_C const GUID DECLSPEC_SELECTANY                                \
		name = {l, w1, w2, {b1, b2, b3, b4, b5, b6, b7, b8}}

ACTUALLY_DEFINE_GUID(CLSID_MMDeviceEnumerator, 0xBCDE0395, 0xE52F, 0x467C, 0x8E,
		     0x3D, 0xC4, 0x57, 0x92, 0x91, 0x69, 0x2E);
ACTUALLY_DEFINE_GUID(IID_IMMDeviceEnumerator, 0xA95664D2, 0x9614, 0x4F35, 0xA7,
		     0x46, 0xDE, 0x8D, 0xB6, 0x36, 0x17, 0xE6);
ACTUALLY_DEFINE_GUID(IID_IAudioClient, 0x1CB9AD4C, 0xDBFA, 0x4C32, 0xB1, 0x78,
		     0xC2, 0xF5, 0x68, 0xA7, 0x03, 0xB2);
ACTUALLY_DEFINE_GUID(IID_IAudioRenderClient, 0xF294ACFC, 0x3146, 0x4483, 0xA7,
		     0xBF, 0xAD, 0xDC, 0xA7, 0xC2, 0x60, 0xE2);

using audio_callback = std::function<void(AVFrame*)>; 
AVFormatContext         *fmt_ctx = NULL;
AVCodecContext          *dec_ctx = NULL;
AVStream                *audio = NULL;
HANDLE                  eventHandle;          
int                     a_stream_id = -1;
static const char       *file_name = "test.mp4";

struct AudioPlaybackParams {
    int                         sampleRate;
    int                         numChannels;
    int                         bitsPerSample;
    uint32_t                    bufferFrameSize;
    IAudioClient                *audio_client;
    IAudioRenderClient          *render;
    WAVEFORMATEXTENSIBLE        *wfx;  // 音频格式描述符
    WAVEFORMATEXTENSIBLE        *devWfx;
};

static inline void SampleFmt2WaveFmt(WORD &WaveFormatTag, enum AVSampleFormat sample_fmt) {
    switch (sample_fmt) {
        case AV_SAMPLE_FMT_FLT:
        case AV_SAMPLE_FMT_FLTP:
            {
                WaveFormatTag = WAVE_FORMAT_IEEE_FLOAT;
                return;
            }
        case AV_SAMPLE_FMT_S16:
        case AV_SAMPLE_FMT_S16P:
        case AV_SAMPLE_FMT_S32:
        case AV_SAMPLE_FMT_S32P:
        default:
             {
                WaveFormatTag = WAVE_FORMAT_PCM;
                return;
            }
    }
}

void InitializeAudioParams(AudioPlaybackParams& params, AVCodecContext* codecCtx) {
    // 初始化音频参数
    params.sampleRate = codecCtx->sample_rate;
    params.numChannels = codecCtx->ch_layout.nb_channels;
    params.bitsPerSample = av_get_bytes_per_sample(codecCtx->sample_fmt) * 8;

    // 设置WAVE格式
    params.wfx.nSamplesPerSec = params.sampleRate;
    params.wfx.wBitsPerSample = params.bitsPerSample;
    params.wfx.nChannels = params.numChannels;
    params.wfx.cbSize = 0;
    SampleFmt2WaveFmt(params.wfx.wFormatTag, codecCtx->sample_fmt);
    params.wfx.nBlockAlign = (params.wfx.wBitsPerSample / 8) * params.wfx.nChannels;
    params.wfx.nAvgBytesPerSec = params.wfx.nSamplesPerSec * params.wfx.nBlockAlign;

    printf("source bps:%d, nchns:%d, blockALign:%d\n", 
        params.wfx.wBitsPerSample, params.wfx.nChannels, params.wfx.nBlockAlign);
}

#include <string>
#include <locale>
#include <codecvt>

std::string wcharToString(const wchar_t* pwstr)
{
    std::wstring_convert<std::codecvt_utf8<wchar_t>> converter;
    return converter.to_bytes(pwstr);
}

HRESULT AudioClientRun(AudioPlaybackParams &params) {
    // 将 IAudioClient 设置为运行状态
    HRESULT hr;
    hr = params.audio_client->Start();
    if (FAILED(hr)) {
        fprintf(stderr, "Failed to start audio client: %08lX\n", hr);
        return hr;
    }
    
    return 0;
}

HRESULT AudioClientStop(AudioPlaybackParams &params) {
    // 将 IAudioClient 设置为stop
    HRESULT hr;
    hr = params.audio_client->Stop();
    if (FAILED(hr)) {
        fprintf(stderr, "Failed to Stop audio client: %08lX\n", hr);
        return hr;
    }

    return 0;
}

static int openCodecContext(int *stream_id, AVCodecContext **codec_ctx, 
    AVFormatContext *fmt_ctx, enum AVMediaType type) {
        int ret, stream_idx;
        AVStream *st;
        const AVCodec *codec;
        // 首先 找到 best stream 
        // 然后 通过 stream codecpar codec_id 找到 decoder
        // 用找到的AVCodec 来分配一个AVCodecContext
        // 把stream 中的 codecpar 参数复制到 AVCodecContext中
        // 最后调用 av_open_codec 来把codec_context中的codec打开
        ret = av_find_best_stream(fmt_ctx, type, -1, -1, NULL, 0);
        if (ret < 0) {
            fprintf(stderr, "Could not find %s stream in input file \n",
                av_get_media_type_string(type));
            return ret;
        } else {
            stream_idx = ret;
            st = fmt_ctx->streams[stream_idx];

            codec = avcodec_find_decoder(st->codecpar->codec_id);
            if (!codec) {
                fprintf(stderr, "Failed to find codec\n");
                return -1;
            }

            *codec_ctx = avcodec_alloc_context3(codec);
            if (!*codec_ctx) {
                fprintf(stderr, "Failed to alloc codec context\n");
                return -1;
            }

            if ((ret = avcodec_parameters_to_context(*codec_ctx, st->codecpar)) < 0) {
                fprintf(stderr, "Failed to copy codec pars to decoder context\n");
                return ret;
            }

            if ((ret = avcodec_open2(*codec_ctx, codec, NULL)) < 0) {
                fprintf(stderr, "Failed to open avcodec\n");
                return ret;
            }

            printf("================codec info===============\n");
            printf(" samplerate: %d, channels: %d", (*codec_ctx)->sample_rate, 
                (*codec_ctx)->channels);
            const char* profile_name = av_get_profile_name((*codec_ctx)->codec, (*codec_ctx)->profile);
            printf(" codec_id: %d codec_name: %s profile_name:%s\n", 
                (*codec_ctx)->codec_id, (*codec_ctx)->codec->name, profile_name);
            printf("time base: num: %d, den: %d",(*codec_ctx)->time_base.num, (*codec_ctx)->time_base.den);
            printf("========================================\n");

            *stream_id = stream_idx;
            return 0;
        }
};

static int openFile(const char *fn) {
    if(avformat_open_input(&fmt_ctx, fn, NULL, NULL) < 0) {
        fprintf(stderr, "Could not open source file %s\n", fn);
        return -1;
    }

    if (avformat_find_stream_info(fmt_ctx, NULL) < 0) {
        fprintf(stderr, "failed to find stream info\n");
        return -1;
    }

    if (openCodecContext(&a_stream_id, &dec_ctx, fmt_ctx, AVMEDIA_TYPE_AUDIO) < 0) {
        fprintf(stderr, "failed to open codec context\n");
        return -1;
    }

    return 0;
}

/// @brief 通过wfx结构来简单的推测出AVSampleFormat格式
/// @param wfx wasapi的设备音频格式
/// @return ffmpeg AVSampleFormat
static AVSampleFormat GuessSampleFormat(WAVEFORMATEX *wfx) {
    // 检查采样点的位宽和通道数
    switch (wfx->wBitsPerSample) {
        case 8:
            // 8位采样数据，通常是无符号的
            return AV_SAMPLE_FMT_U8;
        case 16:
            // 16位采样数据，通常是有符号的
            return AV_SAMPLE_FMT_S16;
        case 24:
            // 24位采样数据，以 32 位整数存储，但有效数据为低 24 位
            return AV_SAMPLE_FMT_S32;
        case 32:
            // 32位采样数据，可以是整数或浮点
            // 这里我们假设如果 wFormatTag 是 WAVE_FORMAT_IEEE_FLOAT，则使用浮点格式
            if (wfx->wFormatTag == WAVE_FORMAT_IEEE_FLOAT)
                return AV_SAMPLE_FMT_FLT;
            else
                return AV_SAMPLE_FMT_S32;
        default:
            // 如果没有匹配的格式，返回错误或未知格式
            return AV_SAMPLE_FMT_NONE;
    }
};

/// @brief 由于WAVEFORMATEX不包含声道布局，因此这里根据声道数量进行默认赋值
///        ，后期测试听一下效果，不过对于普通立体声（双声道来说AV_CH_LAYOUT_STEREO 就可以了）。
/// @param channels // 声道数量
/// @return AV_CH_LAYOUT_XXX 
uint64_t GuessChannelLayout(int channels) {
    switch (channels) {
        case 1:
            return AV_CH_LAYOUT_MONO; // 单声道
        case 2:
            return AV_CH_LAYOUT_STEREO; // 立体声
        case 3:
            return AV_CH_LAYOUT_2POINT1; // 2.1声道 (左右前+低频)
        case 4:
            return AV_CH_LAYOUT_QUAD; // 四声道 (左右前+左右后)
        case 5:
            return AV_CH_LAYOUT_5POINT0; // 5.0声道 (左右前+中心+左右后)
        case 6:
            return AV_CH_LAYOUT_5POINT1; // 5.1声道 (左右前+中心+低频+左右后)
        case 7:
            return AV_CH_LAYOUT_6POINT1; // 6.1声道 (左右前+中心+低频+左右后+后中)
        case 8:
            return AV_CH_LAYOUT_7POINT1; // 7.1声道 (左右前+中心+低频+左右后+左右中)
        default:
            return 0; // 未知布局，或者不支持的通道数
    }
}


static SwrContext *swr_ctx = NULL;
int swrAVFrame(AVFrame* frame, AVCodecContext *codec_ctx, WAVEFORMATEX wfx, uint8_t **output) {
    int64_t dst_chn_layout;
    int32_t dst_sample_rate;
    enum AVSampleFormat dst_sample_fmt;
    if (swr_ctx == NULL) {
        swr_ctx = swr_alloc();
        if (!swr_ctx) {
            fprintf(stderr, "failed to alloc swrcontext\n");
            return -1;
        }
        if (AV_CH_LAYOUT_STEREO == GuessChannelLayout(wfx.nChannels)) {
            printf("AV_CH_LAYOUT_STEREO\n");
        }
        printf("output sample fmt: %d\n", GuessSampleFormat(&wfx));
        av_opt_set_int(swr_ctx,"in_channel_layout", AV_CH_LAYOUT_STEREO, 0);
        av_opt_set_int(swr_ctx, "out_channel_layout", AV_CH_LAYOUT_STEREO, 0);
        av_opt_set_sample_fmt(swr_ctx, "in_sample_fmt", AV_SAMPLE_FMT_S16, 0);
        av_opt_set_sample_fmt(swr_ctx, "out_sample_fmt", AV_SAMPLE_FMT_S32, 0);
        av_opt_set_int(swr_ctx, "in_sample_rate", 44100, 0);
        av_opt_set_int(swr_ctx, "out_sample_rate", 48000, 0);

        if (swr_init(swr_ctx) < 0) {
            fprintf(stderr, "failed to init swr_ctx\n");
            return -1;
        }
    }

    uint32_t input_samples = frame->nb_samples;
    uint32_t input_channels = frame->channels;
    uint32_t wbitsPerSample = [frame](){
        switch(frame->format) {
            case AV_SAMPLE_FMT_U8:
            case AV_SAMPLE_FMT_U8P:
                {
                    return 8;
                }
            case AV_SAMPLE_FMT_S16:
            case AV_SAMPLE_FMT_S16P:
                {
                    return 16;
                }
            case AV_SAMPLE_FMT_S32:
            case AV_SAMPLE_FMT_S32P:
            case AV_SAMPLE_FMT_FLT:
            case AV_SAMPLE_FMT_FLTP:
                {
                    return 32;
                }
            default:
                {
                    fprintf(stderr, "wrong AVSampleFormat\n");
                    return -1;
                }
        }
    }();

    uint32_t outputSample;
    if (!output) {
        fprintf(stderr, "swr output buf null\n");
        return -1;
    }

    printf("delay %d\n",swr_get_delay(swr_ctx, 1000));
    outputSample = input_samples * 2;
    printf("input samples %d\n", input_samples *2);
    outputSample = swr_get_out_samples(swr_ctx, input_samples);
    printf("outsample num %d\n", outputSample);
    if (output[0]) {
        free(output[0]);
    }
    output[0] = (uint8_t*) malloc(outputSample * 8);

    int ret = swr_convert(swr_ctx, output, outputSample, 
        (const uint8_t **)&frame->data[0], input_samples);
    if (ret < 0) {
        fprintf(stderr, "swr_convert failed\n");
    }

    return ret;
}

static int DecodePacket(AVCodecContext *codec_ctx, const AVPacket *pkt, audio_callback callback) {
    int ret = 0;
    AVFrame *frame = av_frame_alloc();
    // 把AVPacket 送进 解码器
    ret = avcodec_send_packet(codec_ctx, pkt);
    if (ret < 0) {
        char errbuf[AV_ERROR_MAX_STRING_SIZE]{0};
        av_make_error_string(errbuf, AV_ERROR_MAX_STRING_SIZE, ret);
        fprintf(stderr, "Error submitting a packet for decoding (%s)\n", errbuf);
        return -1;
    }

    while (ret >= 0) {
        ret = avcodec_receive_frame(codec_ctx, frame);
        if (ret < 0) {
            return ret;
        }
        if (codec_ctx->codec_type == AVMEDIA_TYPE_AUDIO) {
            callback(frame);
        }
    }
    av_frame_free(&frame);
    return 0;
}

int InitializeAudioClient(AudioPlaybackParams& params) {
    HRESULT hr;
    IMMDeviceEnumerator *pDeviceEnumerator = nullptr;
    IMMDevice           *pDefaultDevice = nullptr;
    
    CoInitialize(NULL);

    hr = CoCreateInstance(CLSID_MMDeviceEnumerator, NULL, 
            CLSCTX_ALL, IID_IMMDeviceEnumerator, (void **)&pDeviceEnumerator);
    if (FAILED(hr)) {
        fprintf(stderr, "1\n");
        exit(1);
    }

    hr = pDeviceEnumerator->GetDefaultAudioEndpoint(eRender, eConsole, &pDefaultDevice);
    if (FAILED(hr)) {
        fprintf(stderr, "2\n");
        exit(1);
    }
    // 枚举设备
    IMMDeviceCollection* pDeviceCollections;
    hr = pDeviceEnumerator->EnumAudioEndpoints(eRender, DEVICE_STATE_ACTIVE, &pDeviceCollections);
    if (FAILED(hr)) {
        std::cerr << "Failed to enumerate audio endpoints: " << std::hex << hr << std::endl;
        pDeviceEnumerator->Release();
        CoUninitialize();
        return -1;
    }

    UINT32 deviceCount;
     // Get the number of audio devices
    hr = pDeviceCollections->GetCount(&deviceCount);
    if (FAILED(hr)) {
        std::cerr << "Failed to get device count: " << std::hex << hr << std::endl;
        pDeviceCollections->Release();
        pDeviceEnumerator->Release();
        CoUninitialize();
        return -1;
    }

    printf("device count : %d\n", deviceCount);

    for (UINT i = 0; i < deviceCount; i++) {
        IMMDevice *pDevice = NULL;
        LPWSTR pwszID = NULL;
        IPropertyStore *pProps = NULL;
        PROPVARIANT varName;
        PropVariantInit(&varName);

        //Get the device at index i
        hr = pDeviceCollections->Item(i, &pDevice);
        if (FAILED(hr)) {
            fprintf(stderr, "Failed to get device:\n");
            pDeviceCollections->Release();
            exit(1);
        }

        // Get the device ID
        hr = pDevice->GetId(&pwszID);
        if (FAILED(hr)) {
            fprintf(stderr, "Failed to get device ID:\n");
            pDevice->Release();
            pDeviceCollections->Release();
            exit(1);
        }

     
        std::cerr << "Device ID: " << wcharToString(pwszID) << std::endl;
        

        // Open the property store
        hr = pDevice->OpenPropertyStore(STGM_READ, &pProps);
        if (FAILED(hr)) {
            fprintf(stderr, "Failed to open property store:\n");
            CoTaskMemFree(pwszID);
            pDevice->Release();
            pDeviceCollections->Release();
            exit(1);
        }

        // Get the friendly name of the device
        hr = pProps->GetValue(PKEY_Device_FriendlyName, &varName);
        if (FAILED(hr)) {
            fprintf(stderr, "Failed to get device friendly name:\n");
        }
        std::cerr << "Device Friendly Name: " << wcharToString(varName.pwszVal) << std::endl;

        // Clean up
        PropVariantClear(&varName);
        pProps->Release();
        pDevice->Release();
        CoTaskMemFree(pwszID);

        std::cout << "==============\n";
    }

   // 激活AudioCLient 接口
    hr = pDefaultDevice->Activate(IID_IAudioClient, CLSCTX_ALL, 
        NULL, (void **)&params.audio_client);
    if (FAILED(hr)) {
        fprintf(stderr, "%s: Failed to activate device: %08lX", 
            __FUNCTION__, hr);
            return -1;
    }
    
    // 获取设备最佳格式
    hr = params.audio_client->GetMixFormat(&params.devWfx);
    if (FAILED(hr)) {
        fprintf(stderr, "%s: Failed to get mix format: %08lX",
            __FUNCTION__, hr);
            return -1;
    }
    printf("format %p =====\n", params.devWfx->wFormatTag);
  
    // 检查输入格式是否支持
    WAVEFORMATEX *close_wfx;
    if (params.devWfx) {
        printf("support test\n");
        hr = params.audio_client->IsFormatSupported(AUDCLNT_SHAREMODE_SHARED, &params.wfx, &close_wfx);
        if (FAILED(hr)) {
            if (close_wfx != nullptr) { // 如果查询到最接近的格式，那么意味着该音频流需要做resample处理转换格式
                CoTaskMemFree(params.devWfx);
                params.devWfx = close_wfx;
                fprintf(stderr, "Find closest fotmat\n");
            } else { // format is not supported
                fprintf(stderr, "%s: Format not supported: %08lX",
                __FUNCTION__, hr);
                return -1;
            }
        } else {
            memcpy(params.devWfx, &params.wfx, sizeof(WAVEFORMATEX));
            params.devWfx->Format.wFormatTag = WAVE_FORMAT_IEEE_FLOAT;
        }
    }

    DWORD initStreamFlags = (AUDCLNT_STREAMFLAGS_EVENTCALLBACK | AUDCLNT_STREAMFLAGS_AUTOCONVERTPCM
                        | AUDCLNT_STREAMFLAGS_SRC_DEFAULT_QUALITY );

    std::cerr << "\naudio_client wfx: " << "bits per sample: " << params.devWfx->Format.wBitsPerSample
        << "nsamples :" << params.devWfx->Format.nSamplesPerSec << "\n"
        <<"block align: " << params.devWfx->Format.nBlockAlign << "format Tag:" << params.devWfx->Format.wFormatTag 
        <<"\n======================\n";

    int buffer_length_msec = 5000;
	REFERENCE_TIME dur = buffer_length_msec * 1000 * 10;

    hr = params.audio_client->Initialize(
        AUDCLNT_SHAREMODE_SHARED, initStreamFlags, dur, 0,
        params.devWfx, NULL);
    if (FAILED(hr)) {
        fprintf(stderr, "%s: Failed to initialize: %08lX", 
            __FUNCTION__, hr);
        return -1;
    }

    // 初始化 IAudioRenderCLient
    hr = params.audio_client->GetService(IID_IAudioRenderClient, (void**)&params.render);
    if (FAILED(hr)) {
        fprintf(stderr, "Failed to get audio render client: %08lX\n", hr);
        return -1;
    }

     // 获取缓冲区大小
    hr = params.audio_client->GetBufferSize(&params.bufferFrameSize);
    if (FAILED(hr)) {
        fprintf(stderr, "Failed to get buffer size: %08lX\n", hr);
        return -1;
    }

    eventHandle = CreateEvent(NULL, FALSE, FALSE, NULL);
    if (eventHandle == NULL) {
        //
    }

    hr = params.audio_client->SetEventHandle(eventHandle);
    if (FAILED(hr)) {
        // 处理错误
    }

    hr = params.audio_client->Start();
    if (FAILED(hr)) {
        fprintf(stderr, "Failed to start audio client: %08lX\n", hr);
        return hr;
    }

    pDeviceEnumerator->Release();
    pDefaultDevice->Release();
    return 0;
}


#define REFTIMES_PER_SEC  10000000
#define REFTIMES_PER_MILLISEC  10000
// 示例函数来播放音频
HRESULT PlayAudio(AVFrame *frame, AudioPlaybackParams audio_params, AVCodecContext *codec_ctx) {
    HRESULT hr;
    static uint8_t *output[8];
    static int out_size = 0;
    static int ssize = 0;
    BYTE* pData = NULL;
    DWORD flags = 0;
    UINT32 bufferFrameCount;
    UINT32 numFramesAvailable;
    UINT32 numFramesToWrite;
    UINT32 numFrameReserve;
    const int sampleSize = 32; // 假设目标格式是 16-bit PCM
    const int frameSize = 4 * audio_params.numChannels;

    bufferFrameCount = audio_params.bufferFrameSize;
    // 获取音频帧数据
    printf("pts:%d, pkt-dts:%d\n", frame->pts, frame->pkt_dts);
    // printf("nbsamples %d, chns %d, sampleSize %d, frameSize: %d \n", 
    //     frame->nb_samples, frame->channels, sampleSize, frameSize);
    int num_samples = frame->nb_samples;  // 获取 AVFrame 中的样本数量
    int channels = frame->channels; // 获取通道数
    
    DWORD hnsActualDuration = (double)REFTIMES_PER_SEC *
                        bufferFrameCount / audio_params.devWfx->nSamplesPerSec;
    
    // printf("intput sr %d, output sr %d, intput wbits %d, output wbits %d\n", 
    //     frame->sample_rate, audio_params.devWfx->nSamplesPerSec, 
    //     codec_ctx->sample_fmt, audio_params.devWfx->wBitsPerSample);

    switch (codec_ctx->sample_fmt)
    {
        case AV_SAMPLE_FMT_U8:
            {
                printf("AV_SAMPLE_FMT_U8\n");
                break;
            }
        case AV_SAMPLE_FMT_U8P:
            {
                printf("AV_SAMPLE_FMT_U8P\n");
                break;
            }
        case AV_SAMPLE_FMT_S16:
            {
                printf("AV_SAMPLE_FMT_S16\n");
                break;
            }
        case AV_SAMPLE_FMT_S16P:
            {
                printf("AV_SAMPLE_FMT_S16P\n");
                break;
            }
        case AV_SAMPLE_FMT_S32:
            {
                printf("AV_SAMPLE_FMT_S32\n");
                break;
            }
        case AV_SAMPLE_FMT_S32P:
            {
                printf("AV_SAMPLE_FMT_S32P\n");
                break;
            }
        case AV_SAMPLE_FMT_FLT:
            {
                printf("AV_SAMPLE_FMT_FLT\n");
                break;
            }
        case AV_SAMPLE_FMT_FLTP:
            {
                printf("AV_SAMPLE_FMT_FLTP\n");
                break;
            }
        default:
            {
                fprintf(stderr, "wrong AVSampleFormat\n");
                return -1;
            }
    }

    if (output[0] == nullptr) {
        out_size = bufferFrameCount * frameSize * 2;
        output[0] = (uint8_t *)malloc(out_size);
        out_size /= 2;
    }
    // int outSampleSize = 
    //     swrAVFrame(frame, codec_ctx, *audio_params.devWfx, (uint8_t **)&output[0]);

    if (ssize + frame->nb_samples * 8 < out_size) {
        float *p = (float*)(output[0] + ssize);
        float *c1 = (float*)frame->data[0];
        float *c2 = (float*)frame->data[1];
        for(int i = 0; i < frame->nb_samples; i++) {
            p[2*i] = c1[i];
            p[2*i + 1] = c2[i];
        }
        ssize += frame->nb_samples * 8;
        return 0;
    }
    numFrameReserve = ssize / 8;
    do {
        WaitForSingleObject(eventHandle, INFINITE);

        while(true) {
            hr = audio_params.audio_client->GetCurrentPadding(&numFramesAvailable);
            if (FAILED(hr)) {
                fprintf(stderr, "Failed to get current padding: %08lX\n", hr);
                break;
            }
            if (numFramesAvailable == 0)
                break;
        }

        numFramesToWrite = (bufferFrameCount - numFramesAvailable) > numFrameReserve ? 
            numFrameReserve : (bufferFrameCount - numFramesAvailable); // 计算可写入的帧数

        numFrameReserve -= numFramesToWrite;

        if (numFramesToWrite <= 0) {
            break;
        }

        hr = audio_params.render->GetBuffer(numFramesToWrite, &pData);
        if (FAILED(hr)) {
            fprintf(stderr, "Failed to get buffer: %08lX\n", hr);
            break;
        }

        memcpy(pData, output[0], numFramesToWrite * frameSize);

        hr = audio_params.render->ReleaseBuffer(numFramesToWrite, 0);
        if (FAILED(hr)) {
            fprintf(stderr, "Failed to release buffer: %08lX\n", hr);
            break;
        }
        printf("numFramesToWrite:%d\n", numFramesToWrite);

        // QueryPerformanceCounter(&end);
        // double elapsedTime = (double)(end.QuadPart - start.QuadPart) * 1000.0 / freq.QuadPart;

        //Sleep((DWORD)(((double)hnsActualDuration/(double)REFTIMES_PER_MILLISEC)));
    }while(true);
    ssize = 0;

    return 0;
}

int main() {
    SetConsoleOutputCP(CP_UTF8);
    // 从FFmpeg解码音频数据并填充此缓冲区
    int ret = openFile(file_name);
    if (ret) {
        exit(1);
    }
    AudioPlaybackParams params;
    InitializeAudioParams(params, dec_ctx);
    ret = InitializeAudioClient(params);
    if (ret) {
        exit(1);
    }

    audio_callback callback = [&](AVFrame *frame) {
        fprintf(stderr, "audio callback\n");
        PlayAudio(frame, params, dec_ctx);
        fprintf(stderr, "audio callback end\n");
    };

    AVPacket *pkt = NULL;
    pkt = av_packet_alloc();
    ret = 0;
    while((ret = av_read_frame(fmt_ctx, pkt)) >= 0) {
        if (pkt->stream_index == a_stream_id) {
            DecodePacket(dec_ctx, pkt, callback);
        }
        av_packet_unref(pkt);
    }

    AudioClientStop(params);
    getchar();
    return 0;
}