#include "denoise.h"
#include "wav.h"
#include "fft.h"

static void make_hann(double *w, int n) {
    for (int i=0;i<n;++i) w[i] = 0.5*(1.0 - cos(2.0*M_PI*i/(n-1)));
}

static float *to_mono_float(const WavPCM16 *w, size_t *out_len) {
    *out_len = w->frames;
    float *buf = (float*)malloc(sizeof(float)*w->frames);
    if (!buf) return NULL;
    for (size_t i=0;i<w->frames;++i) {
        long acc = 0;
        for (int c=0;c<w->channels;++c) acc += w->pcm[i*w->channels + c];
        buf[i] = (float)((double)acc / (32768.0 * (double)w->channels));
    }
    return buf;
}

static void build_noise_profile(const float *x, size_t N,
                                int nfft, int hop,
                                const double *win,
                                double *noise_mag) {
    for (int k=0;k<nfft/2+1;++k) noise_mag[k]=0.0;
    size_t frames=0;
    double *re = (double*)malloc(sizeof(double)*nfft);
    double *im = (double*)malloc(sizeof(double)*nfft);
    for (size_t pos=0; pos + (size_t)nfft <= N; pos += (size_t)hop) {
        for (int i=0;i<nfft;++i) {
            double s = (double)x[pos+i];
            re[i] = s * win[i];
            im[i] = 0.0;
        }
        fft_radix2(re, im, nfft, +1);
        for (int k=0;k<=nfft/2;++k) {
            double mag = hypot(re[k], im[k]);
            noise_mag[k] += mag;
        }
        frames++;
    }
    if (frames==0) frames=1;
    for (int k=0;k<=nfft/2;++k) noise_mag[k] = noise_mag[k] / (double)frames + 1e-12;
    free(re); free(im);
}

static void smooth_mag(double *mag, int len) {
    if (len<=2) return;
    double prev = mag[0];
    for (int k=1;k<len-1;++k) {
        double cur = mag[k];
        double nxt = mag[k+1];
        mag[k] = (prev + cur + nxt)/3.0;
        prev = cur;
    }
}

static void denoise_signal(const float *xin, size_t Nin,
                           float *xout,
                           int nfft, int hop,
                           const double *win,
                           const double *noise_mag,
                           float alpha, float beta)
{
    size_t out_len = Nin + (size_t)nfft;
    double *acc   = (double*)calloc(out_len, sizeof(double));
    double *norm  = (double*)calloc(out_len, sizeof(double));
    double *re    = (double*)malloc(sizeof(double)*nfft);
    double *im    = (double*)malloc(sizeof(double)*nfft);
    double *gain_ = (double*)malloc(sizeof(double)*(nfft/2+1));

    for (size_t pos=0; pos + (size_t)nfft <= Nin; pos += (size_t)hop) {
        for (int i=0;i<nfft;++i) { re[i]= (double)xin[pos+i]*win[i]; im[i]=0.0; }
        fft_radix2(re, im, nfft, +1);

        for (int k=0;k<=nfft/2;++k) {
            double mag = hypot(re[k], im[k]);
            double nm  = noise_mag[k];
            double est = mag - (double)alpha * nm;
            double flo = (double)beta * nm;
            if (est < flo) est = flo;
            double g = est / (mag + 1e-12);
            gain_[k] = g;
        }
        smooth_mag(gain_, nfft/2+1);

        for (int k=0;k<=nfft/2;++k) {
            re[k] *= gain_[k]; im[k] *= gain_[k];
        }
        for (int k=1;k<nfft/2;++k) {
            int kk = nfft - k;
            re[kk] *= gain_[k]; im[kk] *= gain_[k];
        }

        fft_radix2(re, im, nfft, -1);
        for (int i=0;i<nfft;++i) {
            size_t t = pos + (size_t)i;
            double w = win[i];
            acc[t]  += re[i] * w;
            norm[t] += w * w;
        }
    }

    for (size_t t=0; t<Nin; ++t) {
        double d = norm[t] > 1e-12 ? acc[t] / norm[t] : acc[t];
        if (d >  0.999) d = 0.999;
        if (d < -0.999) d = -0.999;
        xout[t] = (float)d;
    }

    free(acc); free(norm); free(re); free(im); free(gain_);
}

int denoise_wav_files(const char* noise_wav,
                      const char* in_wav,
                      const char* out_wav,
                      int nfft, int hop,
                      float alpha, float beta)
{
    if (!is_power_of_two(nfft) || hop <= 0 || hop > nfft) {
        fprintf(stderr, "nfft/hop da xatolik. nfft ikkilik darajasida olinishi lozim.\n");
        return 100;
    }

    WavPCM16 w_noise, w_in;
    if (read_wav16(noise_wav, &w_noise)) return 101;
    if (read_wav16(in_wav, &w_in)) { free_wav(&w_noise); return 102; }

    if (w_noise.sample_rate != w_in.sample_rate) {
        fprintf(stderr, "Namuna diskrezitatsiyasi farqli (%d/%d). Avval faylni diskrezitatsiya qiling.\n",
                w_noise.sample_rate, w_in.sample_rate);
        free_wav(&w_noise); free_wav(&w_in); return 103;
    }

    size_t nN=0, nX=0;
    float *noise = to_mono_float(&w_noise, &nN);
    float *input = to_mono_float(&w_in, &nX);
    free_wav(&w_noise);
    if (!noise || !input) { free(noise); free(input); free_wav(&w_in); return 104; }

    double *win = (double*)malloc(sizeof(double)*nfft);
    make_hann(win, nfft);

    double *noise_mag = (double*)malloc(sizeof(double)*(nfft/2+1));
    build_noise_profile(noise, nN, nfft, hop, win, noise_mag);

    float *out = (float*)malloc(sizeof(float)*w_in.frames);
    denoise_signal(input, nX, out, nfft, hop, win, noise_mag, alpha, beta);

    int16_t *pcm16 = (int16_t*)malloc(sizeof(int16_t)*w_in.frames);
    for (size_t i=0;i<w_in.frames;++i) {
        double v = out[i];
        long s = (long)lrint(v * 32767.0);
        if (s >  32767) s =  32767;
        if (s < -32768) s = -32768;
        pcm16[i] = (int16_t)s;
    }

    int wr = write_wav16_mono(out_wav, pcm16, w_in.frames, w_in.sample_rate);

    free(noise); free(input); free(win); free(noise_mag); free(out); free(pcm16); free_wav(&w_in);
    return wr ? 105 : 0;
}
