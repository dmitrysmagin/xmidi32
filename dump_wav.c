#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "backend.h"
#include "timbre_bank.h"
#include "src/xmidi32_driver.h"
#include "src/xmidi32_timbre_internal.h"

#define TIMBRE_CACHE_SIZE 12288

static uint8_t *load_file(const char *path, uint32_t *size) {
    FILE *f = fopen(path, "rb");
    if (!f) return NULL;
    fseek(f, 0, SEEK_END);
    long len = ftell(f);
    fseek(f, 0, SEEK_SET);
    uint8_t *buf = malloc((size_t)len);
    if (fread(buf, 1, (size_t)len, f) != (size_t)len) {
        free(buf); fclose(f); return NULL;
    }
    fclose(f);
    *size = (uint32_t)len;
    return buf;
}

int main(int argc, char **argv) {
    (void)argc; (void)argv;
    xmi_backend_init();
    xmidi32_init_globals();
    xmidi32_detect_device(0, 0, 0, 0, 0);
    xmidi32_init_driver(0, 0, 0, 0, 0);

    void *cache = malloc(TIMBRE_CACHE_SIZE);
    xmidi32_define_timbre_cache(0, cache, TIMBRE_CACHE_SIZE);
    timbre_bank_load_ad();

    uint32_t size;
    uint8_t *data = load_file("./xmi/AGU16.XMI", &size);
    if (!data) { fprintf(stderr, "Failed to load\n"); return 1; }

    struct sequence_state st;
    HSEQUENCE h = xmidi32_register_seq(data, 0, &st, NULL);
    if (h == (HSEQUENCE)-1) { fprintf(stderr, "Failed to register\n"); return 1; }

    uint32_t j;
    for (j = 0; j < NUM_CHANS; j++)
        xmidi32_map_seq_channel(h, j + 1, j + 1);
    xmidi32_start_seq(h);

    uint32_t total_samples = 88200 * 4; // 8 seconds
    int16_t *buf = malloc(total_samples * 2 * 2);
    uint32_t spt = (uint32_t)((uint64_t)44100 * 8333ULL / 1000000ULL);
    uint32_t accum = 0;

    fprintf(stderr, "Generating %u samples (spt=%u)...\n", total_samples, spt);

    uint32_t max_abs = 0;
    uint32_t non_zero = 0;
    for (uint32_t i = 0; i < total_samples; i++) {
        accum++;
        if (accum >= spt) {
            accum -= spt;
            xmidi32_serve_driver();
        }
        OPL3_GenerateResampled(xmi_backend_get_chip(), &buf[i*2]);
        int16_t s = buf[i*2]; if (s < 0) s = -s;
        if ((uint32_t)s > max_abs) max_abs = (uint32_t)s;
        if (s > 100) non_zero++;
    }

    fprintf(stderr, "Done: max_abs=%u non_zero=%u / %u\n", max_abs, non_zero, total_samples);

    // Write WAV
    FILE *f = fopen("test_output.wav", "wb");
    uint32_t data_bytes = total_samples * 4;
    uint32_t riff_size = 36 + data_bytes;
    uint16_t fmt = 1, ch = 2, bps = 16;
    uint32_t sr = 44100, br = 44100 * 4;
    fwrite("RIFF",1,4,f); fwrite(&riff_size,4,1,f);
    fwrite("WAVE",1,4,f); fwrite("fmt ",1,4,f);
    uint32_t fsz = 16; fwrite(&fsz,4,1,f);
    fwrite(&fmt,2,1,f); fwrite(&ch,2,1,f);
    fwrite(&sr,4,1,f); fwrite(&br,4,1,f);
    uint16_t ba = 4; fwrite(&ba,2,1,f); fwrite(&bps,2,1,f);
    fwrite("data",1,4,f); fwrite(&data_bytes,4,1,f);
    fwrite(buf, 2, total_samples * 2, f);
    fclose(f);
    fprintf(stderr, "Wrote test_output.wav\n");

    free(buf); free(data); free(cache);
    return 0;
}
