#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <SDL.h>
#include "backend.h"
#include "sdl_audio.h"
#include "timbre_bank.h"
#include "src/xmidi32_driver.h"
#include "src/xmidi32_timbre_internal.h"

#define TIMBRE_CACHE_SIZE 12288
#define WAV_MAX_SAMPLES   (44100 * 120)

static volatile sig_atomic_t g_stop_requested = 0;

static void sigint_handler(int sig) {
    (void)sig;
    g_stop_requested = 1;
}

static uint8_t *load_file(const char *path, uint32_t *size) {
    FILE *f = fopen(path, "rb");
    if (!f) return NULL;
    fseek(f, 0, SEEK_END);
    long len = ftell(f);
    if (len < 0) { fclose(f); return NULL; }
    fseek(f, 0, SEEK_SET);
    uint8_t *buf = (uint8_t *)malloc((size_t)len);
    if (!buf) { fclose(f); return NULL; }
    if (fread(buf, 1, (size_t)len, f) != (size_t)len) {
        free(buf); fclose(f); return NULL;
    }
    fclose(f);
    *size = (uint32_t)len;
    return buf;
}

static void write_wav(const char *path, int16_t *buf, uint32_t total_samples) {
    FILE *f = fopen(path, "wb");
    if (!f) { fprintf(stderr, "Failed to write %s\n", path); return; }
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
    fprintf(stderr, "Wrote %s (%u samples)\n", path, total_samples);
}

static int run_wav_mode(int argc, char *argv[], int seq_first) {
    dro_start("xmi/capture.txt");
    xmi_backend_init();

    xmidi32_init_globals();
    xmidi32_detect_device(0, 0, 0, 0, 0);
    xmidi32_init_driver(0, 0, 0, 0, 0);

    void *cache = malloc(TIMBRE_CACHE_SIZE);
    if (!cache) { fprintf(stderr, "Failed to allocate timbre cache\n"); return 1; }
    xmidi32_define_timbre_cache(0, cache, TIMBRE_CACHE_SIZE);
    timbre_bank_load_ad();

    int nseqs = argc - seq_first;
    struct sequence_state *states = (struct sequence_state *)
        calloc((size_t)nseqs, sizeof(struct sequence_state));
    HSEQUENCE *handles = (HSEQUENCE *)calloc((size_t)nseqs, sizeof(HSEQUENCE));
    uint8_t **seq_data = (uint8_t **)calloc((size_t)nseqs, sizeof(uint8_t *));
    uint32_t *seq_sizes = (uint32_t *)calloc((size_t)nseqs, sizeof(uint32_t));

    int i;
    int registered = 0;
    for (i = 0; i < nseqs; i++) {
        seq_data[i] = load_file(argv[seq_first + i], &seq_sizes[i]);
        if (!seq_data[i]) {
            fprintf(stderr, "Failed to load %s\n", argv[seq_first + i]);
            continue;
        }
        HSEQUENCE h = xmidi32_register_seq(seq_data[i], 0, &states[registered], NULL);
        if (h == (HSEQUENCE)-1) {
            fprintf(stderr, "Failed to register %s\n", argv[seq_first + i]);
            continue;
        }
        handles[registered] = h;
        uint32_t j;
        for (j = 0; j < NUM_CHANS; j++)
            xmidi32_map_seq_channel(h, j + 1, j + 1);
        xmidi32_start_seq(h);
        registered++;
    }

    if (registered == 0) {
        fprintf(stderr, "No sequences registered\n");
        goto wav_cleanup;
    }

    uint32_t spt = (uint32_t)((uint64_t)44100 * 8333ULL / 1000000ULL);
    uint32_t accum = 0;
    uint32_t total = 0;
    uint32_t chunk = 1024;

    int16_t *buf = (int16_t *)malloc(WAV_MAX_SAMPLES * 4);
    if (!buf) { fprintf(stderr, "Out of memory\n"); goto wav_cleanup; }

    fprintf(stderr, "Rendering WAV (spt=%u chunk=%u)...\n", spt, chunk);

    while (total < WAV_MAX_SAMPLES && !g_stop_requested) {
        uint32_t todo = chunk;
        if (total + todo > WAV_MAX_SAMPLES)
            todo = WAV_MAX_SAMPLES - total;

        accum += todo;
        while (accum >= spt) {
            accum -= spt;
            xmidi32_serve_driver();
        }

        uint32_t k;
        for (k = 0; k < todo; k++)
            OPL3_GenerateResampled(xmi_backend_get_chip(), buf + (total + k) * 2);
        total += todo;

        int playing = 0;
        for (i = 0; i < registered; i++) {
            if (xmidi32_get_seq_status(handles[i]) == SEQ_PLAYING) {
                playing = 1;
                break;
            }
        }
        if (!playing) break;
    }

    dro_stop();
    fprintf(stderr, "Rendered %u samples\n", total);

    if (total > 0) {
        char wav_name[256];
        snprintf(wav_name, sizeof(wav_name), "%s.wav",
                 argv[seq_first]);
        write_wav(wav_name, buf, total);
    }

    free(buf);

wav_cleanup:
    for (i = 0; i < registered; i++) {
        xmidi32_stop_seq(handles[i]);
        xmidi32_release_seq(handles[i]);
    }
    xmidi32_shutdown_driver(0, NULL);
    free(cache);
    xmi_backend_shutdown();
    for (i = 0; i < nseqs; i++) free(seq_data[i]);
    free(states); free(handles); free(seq_data); free(seq_sizes);
    return 0;
}

int main(int argc, char *argv[]) {
    if (argc < 2) {
        fprintf(stderr, "Usage:\n");
        fprintf(stderr, "  xmi_play <file.xmi> [file2.xmi ...]   Play via SDL audio\n");
        fprintf(stderr, "  xmi_play --wav <file.xmi>             Render to WAV\n");
        return 1;
    }

    if (strcmp(argv[1], "--wav") == 0) {
        if (argc < 3) {
            fprintf(stderr, "Usage: xmi_play --wav <file.xmi>\n");
            return 1;
        }
        return run_wav_mode(argc, argv, 2);
    }

    if (SDL_Init(SDL_INIT_AUDIO | SDL_INIT_NOPARACHUTE) < 0) {
        fprintf(stderr, "SDL_Init failed: %s\n", SDL_GetError());
        return 1;
    }

    xmi_backend_init();

    if (sdl_audio_init(44100) < 0) {
        fprintf(stderr, "SDL audio init failed: %s\n", SDL_GetError());
        xmi_backend_shutdown();
        SDL_Quit();
        return 1;
    }

    xmidi32_init_globals();
    xmidi32_detect_device(0, 0, 0, 0, 0);
    xmidi32_init_driver(0, 0, 0, 0, 0);

    void *cache = malloc(TIMBRE_CACHE_SIZE);
    if (!cache) {
        fprintf(stderr, "Failed to allocate timbre cache\n");
        return 1;
    }
    xmidi32_define_timbre_cache(0, cache, TIMBRE_CACHE_SIZE);

    #if SYNTH_TYPE == YM3812
    timbre_bank_load_op();
    #else
    timbre_bank_load_opl();
    #endif

    int nseqs = argc - 1;
    struct sequence_state *states = (struct sequence_state *)
        calloc((size_t)nseqs, sizeof(struct sequence_state));
    HSEQUENCE *handles = (HSEQUENCE *)calloc((size_t)nseqs, sizeof(HSEQUENCE));
    uint8_t **seq_data = (uint8_t **)calloc((size_t)nseqs, sizeof(uint8_t *));
    uint32_t *seq_sizes = (uint32_t *)calloc((size_t)nseqs, sizeof(uint32_t));

    int i;
    int registered = 0;
    for (i = 0; i < nseqs; i++) {
        seq_data[i] = load_file(argv[i + 1], &seq_sizes[i]);
        if (!seq_data[i]) {
            fprintf(stderr, "Failed to load %s\n", argv[i + 1]);
            continue;
        }

        HSEQUENCE h = xmidi32_register_seq(seq_data[i], 0, &states[registered], NULL);
        if (h == (HSEQUENCE)-1) {
            fprintf(stderr, "Failed to register sequence %s\n", argv[i + 1]);
            continue;
        }
        handles[registered] = h;

        uint32_t j;
        for (j = 0; j < NUM_CHANS; j++) {
            xmidi32_map_seq_channel(h, j + 1, j + 1);
        }

        xmidi32_start_seq(h);
        registered++;
    }

    if (registered == 0) {
        fprintf(stderr, "No sequences registered\n");
        goto cleanup;
    }

    printf("Playing %d sequence(s)... Press Ctrl+C to stop.\n", registered);

    signal(SIGINT, sigint_handler);

    int playing = 1;
    while (playing && !g_stop_requested) {
        SDL_Delay(8);

        playing = 0;
        int j;
        for (j = 0; j < registered; j++) {
            if (xmidi32_get_seq_status(handles[j]) == SEQ_PLAYING) {
                playing = 1;
                break;
            }
        }
    }

    printf("Playback complete.\n");
    SDL_Delay(500);

cleanup:
    for (i = 0; i < registered; i++) {
        xmidi32_stop_seq(handles[i]);
        xmidi32_release_seq(handles[i]);
    }
    xmidi32_shutdown_driver(0, NULL);
    free(cache);
    sdl_audio_close();
    xmi_backend_shutdown();
    SDL_Quit();

    for (i = 0; i < nseqs; i++) {
        free(seq_data[i]);
    }
    free(states);
    free(handles);
    free(seq_data);
    free(seq_sizes);

    return 0;
}
