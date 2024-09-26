#ifndef _UTILS_H_
#define _UTILS_H_
#include <assert.h>
#include "core/logger.h"

#define ASRT_EQ(a, b)   assert((a) == (b))
#define ASRT_NEQ(a, b)  assert((a) != (b))
#define ASRT_GT(a, b)   assert((a) > (b))
#define ASRT_GE(a, b)   assert((a) >= (b))
#define ASRT_LT(a, b)   assert((a) < (b))
#define ASRT_LE(a, b)   assert((a) <= (b))


#ifdef _WIN32
#include <windows.h>

static inline void 
handleError(int(*log)(const char*, ...)) {
    DWORD error = GetLastError(); // 获取错误代码
    LPVOID lpMsgBuf;
    
    // 使用 FormatMessage 获取错误描述
    FormatMessage(
        FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
        NULL,
        error,
        MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
        (LPTSTR)&lpMsgBuf,
        0,
        NULL);

    // 输出错误消息
    log("Error: %d - %s\n", error, (char*)lpMsgBuf);

    // 释放缓冲区
    LocalFree(lpMsgBuf);
}

#else 
#endif

#endif