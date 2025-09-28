#pragma once
#include "compat.h"

typedef struct {
    int sample_rate;
    int channels;
    int16_t *pcm;
    size_t frames;
} WavPCM16;

int  read_wav16(const char *path, WavPCM16 *out);
int  write_wav16_mono(const char *path, const int16_t *mono, size_t frames, int sample_rate);
void free_wav(WavPCM16 *w);