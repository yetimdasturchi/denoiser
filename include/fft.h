#pragma once
#include "compat.h"

void fft_radix2(double *re, double *im, int n, int dir);
int  is_power_of_two(int x);
