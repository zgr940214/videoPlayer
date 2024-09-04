#ifndef _AUDIO_MIX_H_
#define _AUDIO_MIX_H_
#include <cstdint>

int mix_inter_leave(uint8_t** input, uint32_t nchns, uint32_t nsamples, uint8_t **output);

#endif