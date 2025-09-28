#pragma once
#include "compat.h"

int denoise_wav_files(const char* noise_wav,
                      const char* in_wav,
                      const char* out_wav,
                      int nfft, int hop,
                      float alpha, float beta);
