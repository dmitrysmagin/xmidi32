#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <SDL2/SDL.h>
#include "backend.h"
#include "sdl_audio.h"
#include "timbre_bank.h"
#include "src/xmidi32_driver.h"
#include "src/xmidi32_timbre_internal.h"

#define TIMBRE_CACHE_SIZE 12288

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

int main(int argc, char *argv[]) {
    if (argc < 2) {
        fprintf(stderr, "Usage: xmi_play <file.xmi> [file2.xmi ...]\n");
        return 1;
    }

    if (SDL_Init(SDL_INIT_AUDIO) < 0) {
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

    timbre_bank_load_ad();

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
            xmidi32_map_seq_channel(h, j, j);
        }

        xmidi32_start_seq(h);
        registered++;
    }

    if (registered == 0) {
        fprintf(stderr, "No sequences registered\n");
        goto cleanup;
    }

    printf("Playing %d sequence(s)... Press Ctrl+C to stop.\n", registered);

    int playing = 1;
    while (playing) {
        xmidi32_serve_driver();
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
