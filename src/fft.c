#include "fft.h"

void fft_radix2(double *re, double *im, int n, int dir) {
    int j = 0;
    for (int i = 1; i < n - 1; ++i) {
        int bit = n >> 1;
        while (j & bit) { j ^= bit; bit >>= 1; }
        j |= bit;
        if (i < j) {
            double tr = re[i]; re[i] = re[j]; re[j] = tr;
            double ti = im[i]; im[i] = im[j]; im[j] = ti;
        }
    }
    for (int len = 2; len <= n; len <<= 1) {
        double ang = 2.0 * M_PI / len * (dir > 0 ? -1.0 : +1.0);
        double wlen_r = cos(ang), wlen_i = sin(ang);
        for (int i = 0; i < n; i += len) {
            double wr = 1.0, wi = 0.0;
            for (int k = 0; k < len/2; ++k) {
                int u = i + k;
                int v = u + len/2;
                double tr = wr*re[v] - wi*im[v];
                double ti = wr*im[v] + wi*re[v];
                re[v] = re[u] - tr; im[v] = im[u] - ti;
                re[u] += tr;        im[u] += ti;
                double nr = wr*wlen_r - wi*wlen_i;
                wi = wr*wlen_i + wi*wlen_r;
                wr = nr;
            }
        }
    }
    if (dir < 0) {
        double inv = 1.0 / n;
        for (int i = 0; i < n; ++i) { re[i]*=inv; im[i]*=inv; }
    }
}

int is_power_of_two(int x){ return x>0 && (x&(x-1))==0; }
