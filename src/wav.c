#include "wav.h"

int read_wav16(const char *path, WavPCM16 *out) {
    memset(out, 0, sizeof(*out));
    FILE *f = fopen(path, "rb");
    if (!f) { fprintf(stderr, "%s faylini ochishda xatolik\n", path); return 1; }

    uint8_t hdr[12];
    if (fread(hdr, 1, 12, f) != 12 || memcmp(hdr, "RIFF", 4) || memcmp(hdr+8, "WAVE", 4)) {
        fprintf(stderr, "Kiritilgan fayl %s RIFF/WAVE formatiga mos emas\n", path); fclose(f); return 2;
    }

    int fmt_found = 0, data_found = 0;
    uint16_t audio_format = 0, num_channels = 0, bits_per_sample = 0;
    uint32_t sample_rate = 0;
    long data_pos = 0; uint32_t data_size = 0;

    while (!fmt_found || !data_found) {
        uint8_t chunk_hdr[8];
        if (fread(chunk_hdr, 1, 8, f) != 8) { fprintf(stderr, "%s WAV uzunligi yetarli emas\n", path); fclose(f); return 3; }
        uint32_t chunk_size = (uint32_t) (chunk_hdr[4] | (chunk_hdr[5]<<8) | (chunk_hdr[6]<<16) | (chunk_hdr[7]<<24));

        if (!memcmp(chunk_hdr, "fmt ", 4)) {
            uint8_t *fmt = (uint8_t*)malloc(chunk_size);
            if (!fmt || fread(fmt, 1, chunk_size, f) != chunk_size) { fprintf(stderr, "Fayl formatini o'qishda xatolik\n"); free(fmt); fclose(f); return 4; }
            audio_format = (uint16_t)(fmt[0] | (fmt[1]<<8));
            num_channels = (uint16_t)(fmt[2] | (fmt[3]<<8));
            sample_rate  = (uint32_t)(fmt[4] | (fmt[5]<<8) | (fmt[6]<<16) | (fmt[7]<<24));
            bits_per_sample = (uint16_t)(fmt[14] | (fmt[15]<<8));
            free(fmt);
            fmt_found = 1;
        } else if (!memcmp(chunk_hdr, "data", 4)) {
            data_pos = ftell(f);
            data_size = chunk_size;
            fseek(f, chunk_size, SEEK_CUR);
            data_found = 1;
        } else {
            fseek(f, chunk_size, SEEK_CUR);
        }
    }

    if (audio_format != 1) { fprintf(stderr, "Faqatgina PCM formati qo'llab-quvvatlanadi: %s\n", path); fclose(f); return 5; }
    if (bits_per_sample != 16) { fprintf(stderr, "Faqatgina 16-bit PCM qo'llab-quvvatlanadi: %s\n", path); fclose(f); return 6; }
    if (num_channels < 1) { fprintf(stderr, "Kanallar sonida xatolik\n"); fclose(f); return 7; }

    out->channels = (int)num_channels;
    out->sample_rate = (int)sample_rate;
    out->frames = (size_t)data_size / (size_t)(2 * num_channels);

    out->pcm = (int16_t*)malloc((size_t)out->frames * (size_t)num_channels * sizeof(int16_t));
    if (!out->pcm) { fclose(f); return 8; }

    fseek(f, data_pos, SEEK_SET);
    if (fread(out->pcm, 2 * num_channels, out->frames, f) != out->frames) {
        fprintf(stderr, "PCM o'qilmadi\n"); free(out->pcm); memset(out,0,sizeof(*out)); fclose(f); return 9;
    }

    fclose(f);
    return 0;
}

int write_wav16_mono(const char *path, const int16_t *mono, size_t frames, int sample_rate) {
    FILE *f = fopen(path, "wb");
    if (!f) return 1;

    uint32_t byte_rate = (uint32_t)sample_rate * 2;
    uint16_t block_align = 2;
    uint32_t data_size = (uint32_t)(frames * 2);
    uint32_t riff_size = 36 + data_size;

    fwrite("RIFF",1,4,f); fwrite(&riff_size,4,1,f); fwrite("WAVE",1,4,f);
    fwrite("fmt ",1,4,f); uint32_t fmt_size=16; fwrite(&fmt_size,4,1,f);
    uint16_t fmt_tag=1, channels=1, bps=16;
    fwrite(&fmt_tag,2,1,f); fwrite(&channels,2,1,f);
    fwrite(&sample_rate,4,1,f); fwrite(&byte_rate,4,1,f);
    fwrite(&block_align,2,1,f); fwrite(&bps,2,1,f);
    fwrite("data",1,4,f); fwrite(&data_size,4,1,f);
    fwrite(mono,2,frames,f);

    fclose(f);
    return 0;
}

void free_wav(WavPCM16 *w) {
    if (w && w->pcm) free(w->pcm);
    if (w) memset(w,0,sizeof(*w));
}
