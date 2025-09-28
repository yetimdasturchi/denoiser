#include "compat.h"
#include "denoise.h"

#if BENCH
#include "bench.h"
#endif

int main(int argc, char **argv) {
    if (argc < 4 || argc > 8) {
        fprintf(stderr, "Foydalanish: %s shovqin.wav kirish.wav chiqish.wav [nfft=1024] [hop=512] [alpha=1.2] [beta=0.02]\n", argv[0]);
        return 2;
    }
    const char* npath = argv[1];
    const char* inpath= argv[2];
    const char* opath = argv[3];

    int nfft  = (argc>4)? atoi(argv[4]) : 1024;
    int hop   = (argc>5)? atoi(argv[5]) : nfft/2;
    float alpha = (argc>6)? (float)atof(argv[6]) : 1.2f;
    float beta  = (argc>7)? (float)atof(argv[7]) : 0.02f;

#if BENCH
    BenchSnapshot s0, s1;
    bench_snapshot(&s0);
#endif

    int rc = denoise_wav_files(npath, inpath, opath, nfft, hop, alpha, beta);

#if BENCH
    bench_snapshot(&s1);
    bench_report_diff(&s0, &s1, "denoise run");
#endif

    if (rc==0) fprintf(stderr, "Tozalangan audio %s\n", opath);
    return rc;
}