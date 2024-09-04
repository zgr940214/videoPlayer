#ifndef _AUDIO_WIN32_H_
#define _AUDIO_WIN32_H_
#include <stdio.h>

#include "audio_common.h"
#include "memory_pool.h"
#include "audio_common.h"

#ifdef  _WIN32

    #include <windows.h>
    #include <mmsystem.h>
    #include <mmdeviceapi.h>
    #include <combaseapi.h>
    #include <mmreg.h>
    #include <Audioclient.h>
    #include <Functiondiscoverykeys_devpkey.h>

    #pragma comment(lib, "winmm.lib")

    typedef struct AudioDeviceWin32 {
        IAudioClient        *client;
        IAudioRenderClient  *render;
        WAVEFORMATEX        *wfx; // 默认设备格式
        WAVEFORMATEX        *inputWfx; // 输入格式
        int                 need_resample; // 是否需要格式转换
        size_t              numBuffers;
        size_t              currentBuffer;
        CRITICAL_SECTION    cs;      // 互斥
        HANDLE              event;  //同步事件
    } AudioDevice_t;

    int ConvertWfx2AudioFormat(WAVEFORMATEX *wfx, audio_format_info_t *info);

#elif defined(__linux__)

    typedef struct AudioDeviceAlsa {

    } AudioDevice_t;

#endif

void InitializeCom();

int InitAudioDev(mem_pool_t *pool, AVCodecContext *ctx, AudioDevice_t ** aDev);

audio_format_info_t GetAudioDevFormat(AudioDevice_t *dev);

#endif