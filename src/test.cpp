#include <windows.h>
#include <mmsystem.h>
#include <functional>
#include <fstream>
#include <vector>
#include "audio/audio_resampler.h"

#include <iostream>
#include <Audioclient.h>
#include <mmdeviceapi.h>
#include <functiondiscoverykeys.h>
#include <Propsys.h>

extern "C" {
    
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
AVFormatContext *fmt_ctx = NULL;
AVCodecContext  *dec_ctx = NULL;
AVStream        *audio = NULL;
int             a_stream_id = -1;
static const char *file_name = "test.wav";

struct AudioPlaybackParams {
    int sampleRate;
    int numChannels;
    int bitsPerSample;
    uint32_t bufferFrameSize;
    IAudioClient        *audio_client;
    IAudioRenderClient  *render;
    WAVEFORMATEX        wfx;  // 音频格式描述符
    WAVEFORMATEX        *devWfx;
};

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
    params.wfx.wFormatTag = WAVE_FORMAT_PCM;
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
        if (ret != 0) {
            return ret;
        }
        if (codec_ctx->codec_type == AVMEDIA_TYPE_AUDIO) {
            callback(frame);
        }
    }
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
    // 检查输入格式是否支持
    WAVEFORMATEX *close_wfx;
    if (params.devWfx) {
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
        }
    }

    DWORD initStreamFlags = ( AUDCLNT_STREAMFLAGS_RATEADJUST 
                        | AUDCLNT_STREAMFLAGS_AUTOCONVERTPCM
                        | AUDCLNT_STREAMFLAGS_SRC_DEFAULT_QUALITY );

    std::cerr << "\naudio_client wfx: " << "bits per sample: " << params.devWfx->wBitsPerSample
        << "nsamples :" << params.devWfx->nSamplesPerSec << "\n"
        <<"block align: " << params.devWfx->nBlockAlign << "format Tag:" << params.devWfx->wFormatTag 
        <<"\n======================\n";

    hr = params.audio_client->Initialize(
        AUDCLNT_SHAREMODE_SHARED, initStreamFlags, 10000000, 0,
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

    hr = params.audio_client->Start();
    if (FAILED(hr)) {
        fprintf(stderr, "Failed to start audio client: %08lX\n", hr);
        return hr;
    }
    
    IMMDeviceCollection* pDeviceCollections;
    hr = pDeviceEnumerator->EnumAudioEndpoints(eAll, DEVICE_STATE_ACTIVE, &pDeviceCollections);
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

    pDeviceEnumerator->Release();
    pDefaultDevice->Release();
    return 0;
}


#define REFTIMES_PER_SEC  10000000
#define REFTIMES_PER_MILLISEC  10000
// 示例函数来播放音频
HRESULT PlayAudio(AVFrame *frame, AudioPlaybackParams audio_params) {
    HRESULT hr;
    UINT32 bufferFrameCount;
    BYTE* pData = NULL;
    DWORD flags = 0;
    UINT32 numFramesAvailable;
    UINT32 numFramesToWrite;
    UINT32 numFrameReserve;
    const int sampleSize = audio_params.bitsPerSample; // 假设目标格式是 16-bit PCM
    const int frameSize = audio_params.bitsPerSample * audio_params.numChannels / 8;

    bufferFrameCount = audio_params.bufferFrameSize;
    // 获取音频帧数据
    printf("nbsamples %d, chns %d, sampleSize %d, frameSize: %d \n", 
        frame->nb_samples, frame->channels, sampleSize, frameSize);
    int num_samples = frame->nb_samples;  // 获取 AVFrame 中的样本数量
    int channels = frame->channels; // 获取通道数
    int frameBufferSize = num_samples * channels * (sampleSize / 8); // 计算帧的大小
    numFrameReserve = frameBufferSize / (sampleSize / 8);
    
    DWORD hnsActualDuration = (double)REFTIMES_PER_SEC *
                        bufferFrameCount / audio_params.devWfx->nSamplesPerSec;
    
    do {
        // 等待有可写入的缓冲区
        hr = audio_params.audio_client->GetCurrentPadding(&numFramesAvailable);
        if (FAILED(hr)) {
            fprintf(stderr, "Failed to get current padding: %08lX\n", hr);
            break;
        }

        numFramesToWrite = (bufferFrameCount - numFramesAvailable) > numFrameReserve ? 
            numFrameReserve : (bufferFrameCount - numFramesAvailable); // 计算可写入的帧数

        numFrameReserve -= numFramesToWrite;

        if (numFramesToWrite == 0) {
            break;
        }

        hr = audio_params.render->GetBuffer(numFramesToWrite, &pData);
        if (FAILED(hr)) {
            fprintf(stderr, "Failed to get buffer: %08lX\n", hr);
            break;
        }

        if (frame->data[1] == nullptr) {
            memcpy(pData, frame->data[0], numFramesToWrite * frameSize);
        } 

        hr = audio_params.render->ReleaseBuffer(numFramesToWrite, 0);
        if (FAILED(hr)) {
            fprintf(stderr, "Failed to release buffer: %08lX\n", hr);
            break;
        }
        printf("numFramesToWrite:%d\n", numFramesToWrite);
        Sleep((DWORD)(hnsActualDuration/REFTIMES_PER_MILLISEC/2));
    }while(true);
    Sleep((DWORD)(hnsActualDuration/REFTIMES_PER_MILLISEC/2));

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
        PlayAudio(frame, params);
        fprintf(stderr, "audio callback end\n");
    };

    AVPacket *pkt = NULL;
    pkt = av_packet_alloc();
    while(av_read_frame(fmt_ctx, pkt) >= 0) {
        DecodePacket(dec_ctx, pkt, callback);
        av_packet_free(&pkt);
        pkt = av_packet_alloc();
    }

    AudioClientStop(params);
    getchar();
    return 0;
}