#include "audio_device.h"
#include "audio.h"
#include <endpointvolume.h>

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

// /// @brief 用于在windows wasapi中注册 当前默认设备更改的回调函数
// ///         Windows COM组件接口
// class AudioDeviceNotification : public IMMNotificationClient {
//     private:
//         LONG _cRef;
//         IMMDeviceEnumerator *_pEnumerator;
//         audio_source_t      *source;

//     public:
//         AudioDeviceNotification(audio_source_t *src): _cRef(1), 
//             _pEnumerator(NULL), source(src){};
        
//         ~AudioDeviceNotification() {
//             if (_pEnumerator) {
//                 _pEnumerator->Release();
//             }
//         }

//         ULONG STDMETHODCALLTYPE AddRef() {
//             return InterlockedIncrement(&_cRef);
//         }

//         ULONG STDMETHODCALLTYPE Release() {
//             ULONG ulRef = InterlockedDecrement(&_cRef);
//             if (0 == ulRef) {
//                 delete this;
//             }
//             return ulRef;
//         }

//         HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, VOID **ppvInterface) {
//             if (IID_IUnknown == riid) {
//                 AddRef();
//                 *ppvInterface = (IUnknown*)this;
//             } else if (__uuidof(IMMNotificationClient) == riid) {
//                 AddRef();
//                 *ppvInterface = (IMMNotificationClient*)this;
//             } else {
//                 *ppvInterface = NULL;
//                 return E_NOINTERFACE;
//             }
//             return S_OK;
//         }

//         HRESULT STDMETHODCALLTYPE OnDefaultDeviceChanged(EDataFlow flow, ERole role, LPCWSTR pwstrDeviceId) {
//             // Handle default device change
//             ///printf("Default audio device changed.\n");
//             HRESULT hr;
//             if (flow == eRender) {
//                 if (role == eConsole) {
//                     AudioDeviceWin32* dev = (AudioDeviceWin32*)source->audio_device;
//                     if (!_pEnumerator) {
//                         hr = CoCreateInstance(CLSID_MMDeviceEnumerator,
//                             NULL, 0, IID_IMMDeviceEnumerator, (void**)&_pEnumerator);
//                         //TODO: 重新更换设备 IAudioCLient IAudioRenderCLient
//                     }
//                 }   
//             }
//             // Reinitialize IAudioClient here
//             return S_OK;
//         }
// };

static int GUID_cmp(GUID id1, GUID id2) {
    if (id1.Data1 == id2.Data1 &&
        id1.Data2 == id2.Data2 &&
        id1.Data3 == id2.Data3 &&
        id1.Data4 == id2.Data4) {
            return 0;
        }
    return -1;
}
#define GUID_EQ(id1, id2) (GUID_cmp(id1, id2) == 0) 

/// @brief 通过wfx结构来简单的推测出AVSampleFormat格式
/// @param wfx wasapi的设备音频格式
/// @return ffmpeg AVSampleFormat
static enum AVSampleFormat GuessSampleFormat(WAVEFORMATEXTENSIBLE *pwfe) {
    switch (pwfe->Format.wBitsPerSample) {
        case 8:
            return AV_SAMPLE_FMT_U8;

        case 16:
            return AV_SAMPLE_FMT_S16;

        case 24:
        case 32:
            if (pwfe->Format.wFormatTag == WAVE_FORMAT_IEEE_FLOAT || 
                (pwfe->Format.wFormatTag == WAVE_FORMAT_EXTENSIBLE &&
                GUID_EQ(pwfe->SubFormat, KSDATAFORMAT_SUBTYPE_IEEE_FLOAT))) {
                    return AV_SAMPLE_FMT_FLT;
            } else {
                return AV_SAMPLE_FMT_S32;
            }

        default:
            break;
    }

};

uint32_t ChannelMask(int channels) {
     switch (channels) {
        case 1:
            return SPEAKER_FRONT_CENTER; // 单声道
        case 2:
            return SPEAKER_FRONT_LEFT | SPEAKER_FRONT_RIGHT; // 立体声
        case 3:
            return SPEAKER_FRONT_LEFT | SPEAKER_FRONT_RIGHT | SPEAKER_LOW_FREQUENCY; // 2.1声道 (左右前+低频)
        case 4:
            return SPEAKER_FRONT_LEFT | SPEAKER_FRONT_RIGHT | SPEAKER_BACK_LEFT | SPEAKER_BACK_RIGHT; // 四声道 (左右前+左右后)
        case 5:
            return SPEAKER_FRONT_LEFT | SPEAKER_FRONT_RIGHT | SPEAKER_BACK_LEFT | SPEAKER_BACK_RIGHT | SPEAKER_FRONT_CENTER; // 5.0声道 (左右前+中心+左右后)
        case 6:
            return SPEAKER_FRONT_LEFT | SPEAKER_FRONT_RIGHT | SPEAKER_BACK_LEFT | SPEAKER_BACK_RIGHT 
                | SPEAKER_FRONT_CENTER | SPEAKER_LOW_FREQUENCY; // 5.0声道 (左右前+中心+左右后)
        case 7:
            return SPEAKER_FRONT_LEFT | SPEAKER_FRONT_RIGHT | SPEAKER_BACK_LEFT | SPEAKER_BACK_RIGHT 
                | SPEAKER_FRONT_CENTER | SPEAKER_BACK_CENTER | SPEAKER_LOW_FREQUENCY; // 6.1声道 (左右前+中心+低频+左右后+后中)
        case 8:
            return SPEAKER_FRONT_LEFT | SPEAKER_FRONT_RIGHT | SPEAKER_FRONT_CENTER | SPEAKER_LOW_FREQUENCY 
                | SPEAKER_BACK_LEFT | SPEAKER_BACK_RIGHT | SPEAKER_SIDE_LEFT | SPEAKER_SIDE_RIGHT;  // 7.1声道 (左右前+中心+低频+左右后+左右中)
        default:
            return 0; // 未知布局，或者不支持的通道数
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

/// @brief 转换windows wasapi WAVEFORMATEX 的音频格式结构体到自定义音频格式结构体
/// @param wfx WAVEFORMATEX 
/// @param info 项目中的音频格式结构
/// @return 
int ConvertWfx2AudioFormat(WAVEFORMATEXTENSIBLE *wfx, audio_format_info_t *info) {
  info->nChannels = wfx->Format.nChannels;
  info->wBitsPerSample = wfx->Format.wBitsPerSample;
  info->nSamplesPerSec = wfx->Format.nSamplesPerSec;
  info->sampleFormat = GuessSampleFormat(wfx);
  info->chn_layout = GuessChannelLayout(wfx->Format.nChannels);
  info->nBlockAlign = wfx->Format.nChannels * (wfx->Format.wBitsPerSample / 8);
  //info->nBlocks = 暂时无用
};

void InitializeCom() {
    HRESULT hr;
    hr = CoInitialize(NULL);// 初始化COM
        if (FAILED(hr)) {
            fprintf(stderr, "COM initalization failed\n");
            exit(1);
        }
};

/// @brief 从ffmpeg AVcodecContext初始化 AudioDevice
/// @param aDev 
/// @param codecCtx 
void InitializeAudioParams(audio_device_t *aDev, AVCodecContext* codecCtx) {
    // 设置WAVE格式
    WAVEFORMATEXTENSIBLE *input_wfx = 
        (WAVEFORMATEXTENSIBLE *)CoTaskMemAlloc(sizeof(WAVEFORMATEXTENSIBLE));
    input_wfx->Format.nSamplesPerSec = codecCtx->sample_rate;
    input_wfx->Format.wBitsPerSample = av_get_bytes_per_sample(codecCtx->sample_fmt) * 8;
    input_wfx->Format.nChannels = codecCtx->ch_layout.nb_channels;

    input_wfx->Format.nBlockAlign = 
        (input_wfx->Format.wBitsPerSample / 8) 
        * input_wfx->Format.nChannels;

    input_wfx->Format.nAvgBytesPerSec = 
        input_wfx->Format.nSamplesPerSec 
        * input_wfx->Format.nBlockAlign;

    input_wfx->Format.cbSize = sizeof(*input_wfx) - sizeof(WAVEFORMATEX);
    input_wfx->Format.wFormatTag = WAVE_FORMAT_EXTENSIBLE;

    input_wfx->SubFormat = KSDATAFORMAT_SUBTYPE_PCM; // 默认pcm格式
    input_wfx->Samples.wValidBitsPerSample = input_wfx->Format.wBitsPerSample;
    input_wfx->dwChannelMask = ChannelMask(input_wfx->Format.nChannels);

    aDev->inputWfx = (WAVEFORMATEX *)input_wfx;
};

/// @brief  打开 IAudioCLient IAudioRenderClien。 
///         首先需要根据AVCodecContext初始化 输入wfx格式, 每个流都会有一个AudioDevice_t 通过这里初始化
///         经过初始化之后， 后续的音频处理都会用到这个结构进行播放。         
/// @param aDev 音频设备结构
/// @return 0 成功， < 0 失败
static int OpenAudioDev(audio_device_t *aDev) {
    HRESULT hr;
    if (!aDev) {
        fprintf(stderr, "audioDevice_t is not created\n");
        return -1;
    } 

    // 获取设备枚举器
    IMMDeviceEnumerator *pEnumerator = NULL;
    hr = CoCreateInstance(&CLSID_MMDeviceEnumerator, NULL, CLSCTX_ALL, 
            &IID_IMMDeviceEnumerator, (void**)&pEnumerator);
    if (FAILED(hr)) {
        fprintf(stderr, "%s: Failed to create IMMDeviceEnumerator: %08lX\n",
            __FUNCTION__, hr);
        return -1;
    }

    // 获取默认设备
    IMMDevice *pDefaultDevice;
    LPWSTR pDefaultDeviceId;
    (pEnumerator->lpVtbl)->GetDefaultAudioEndpoint(
            pEnumerator, eRender, eConsole, &pDefaultDevice);
    
    // 激活AudioCLient 接口
    hr = pDefaultDevice->lpVtbl->Activate(pDefaultDevice , &IID_IAudioClient, CLSCTX_ALL, 
            NULL, (void **)&aDev->client);
    if (FAILED(hr)) {
        fprintf(stderr, "%s: Failed to activate device: %08lX", 
            __FUNCTION__, hr);
            return -1;
    }

    // 获取设备最佳格式
    hr = aDev->client->lpVtbl->GetMixFormat(aDev->client, &aDev->wfx);
    if (FAILED(hr)) {
        fprintf(stderr, "%s: Failed to get mix format: %08lX",
            __FUNCTION__, hr);
            return -1;
    }

    // 检查输入格式是否支持
    WAVEFORMATEX *close_wfx;
    if (aDev->inputWfx) {
        hr = aDev->client->lpVtbl->IsFormatSupported(aDev->client,
                AUDCLNT_SHAREMODE_SHARED, aDev->inputWfx, &close_wfx);
        if (FAILED(hr)) {
            if (close_wfx != NULL) { // 如果查询到最接近的格式，那么意味着该音频流需要做resample处理转换格式
                CoTaskMemFree(aDev->wfx);
                aDev->wfx = close_wfx;
                aDev->need_resample = 1;
            } else { // format is not supported
                fprintf(stderr, "%s: Format not supported: %08lX",
                __FUNCTION__, hr);
                return -1;
            }
        } else {
            aDev->need_resample = 0;
        }
    }

    hr = aDev->client->lpVtbl->Initialize(aDev->client,
            AUDCLNT_SHAREMODE_SHARED, 0, 10000000, 0,
            aDev->wfx, NULL);
    if (FAILED(hr)) {
        fprintf(stderr, "%s: Failed to initialize: %08lX", 
            __FUNCTION__, hr);
        return -1;
    }

    // 初始化 IAudioRenderCLient
    hr = aDev->client->lpVtbl->GetService(aDev->client, 
            &IID_IAudioRenderClient, (void**)&aDev->render);
    if (FAILED(hr)) {
        fprintf(stderr, "Failed to get audio render client: %08lX\n", hr);
        return -1;
    }

    pEnumerator->lpVtbl->Release(pEnumerator); // 清理枚举器
    pDefaultDevice->lpVtbl->Release(pDefaultDevice); // 释放设备
    return 0; // 成功初始化
}

/// @brief 从AVCodecContext中拿输入流的音频格式，然后找一个最近的音频格式存储在wfx中并进行设备初始化
///        播放的时候，我们首先用resampler 去把输入流resample成设备格式，然后直接在这里进行播放 
/// @param pool 
/// @param codec_ctx 
/// @param aDev 
/// @return 
int InitAudioDev(mem_pool_t *pool, AVCodecContext *codec_ctx, audio_device_t **aDev) {
    int ret;
    if (*aDev) {
        fprintf(stderr, "Audio device already initilized\n");
        return -1;
    }
    *aDev = (audio_device_t *)mem_alloc(pool, sizeof(audio_device_t), 1);
  
    InitializeAudioParams(*aDev, codec_ctx);

    ret = OpenAudioDev(*aDev);
    if (ret) {
        fprintf(stderr, "initlize Audio Device failed\n");
        return ret;
    }

    return 0;// successful
};

int GetAudioDevFormat(audio_device_t *dev, audio_format_info_t *info) {
    ConvertWfx2AudioFormat(dev->wfx, info);
    return 0;
};