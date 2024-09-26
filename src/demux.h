#ifndef _DEMUX_H_
#define _DEMUX_H_

// 这个模块的设计是这样的， 通过url 接受本地文件或网络流
// 解码器的tempo需要仔细考虑，总之解码/解复用 也是时钟同步的一环
// 解码/解复用 直接access 音视频缓冲
// 我们需要设计一个输入流 输出流
// 输入流管理解复用的缓冲
// 输出流则是经过filter 之后的缓冲， 而输出流缓冲直接被 renderer access（包含视频/音频的renderer）



#endif