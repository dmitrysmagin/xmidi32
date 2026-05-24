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
#include "src/xmidi32_utils.h"

#define TIMBRE_CACHE_SIZE 12288
#define WAV_MAX_SAMPLES   (44100 * 120)

static volatile sig_atomic_t g_stop_requested = 0;

static int install_sequence_timbres(HSEQUENCE h) {
    static const uint8_t dummy_2op[14] = { 0x0E,0x00, 0,0x3F,0xFF,0xFF, 0,0x3F,0xFF,0xFF, 0,0,0,0 };
    uint32_t treq;
    int count = 0;
    while ((treq = xmidi32_timbre_request(0, h)) != 0xFFFFFFFF) {
        uint8_t bank = (uint8_t)(treq >> 8);
        uint8_t patch = (uint8_t)(treq & 0xFF);
        const uint8_t *timb = timbre_bank_find(bank, patch);
        if (timb != NULL) {
            yamaha_install_timbre(bank, patch, timb);
            fprintf(stderr, "Installed timbre bank %u, patch %u\n", bank, patch);
            count++;
        } else {
            yamaha_install_timbre(bank, patch, dummy_2op);
            fprintf(stderr, "Timbre bank %u, patch %u not found - installed silent\n", bank, patch);
        }
    }
    return count;
}

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

static int count_xmi_tracks(const uint8_t *data) {
    int count = 0;
    while (find_seq((uint8_t *)data, count) != NULL) count++;
    return count;
}

static int has_for_loop(const struct sequence_state *state) {
    if (!state || !state->EVNT) return 0;
    uint32_t len = read_be32(state->EVNT + 4);
    if (len < 3) return 0;
    uint8_t *data = state->EVNT + 8;
    uint32_t i;
    for (i = 0; i + 2 < len; i++) {
        if ((data[i] & 0xF0) == 0xB0 && data[i + 1] == 0x74)
            return 1;
    }
    return 0;
}

static int run_seq(const char *path, int seq_num, int wav_mode);

int main(int argc, char *argv[]) {
    const char *path;
    int seq_num = 0;
    int wav_mode_flag = 0;

    if (argc < 2) {
        fprintf(stderr, "Usage:\n");
        fprintf(stderr, "  xmi_play <file.xmi> [sequence_num]         Play via SDL audio\n");
        fprintf(stderr, "  xmi_play --wav <file.xmi> [sequence_num]   Render to WAV\n");
        return 1;
    }

    int arg_idx = 1;
    if (strcmp(argv[1], "--wav") == 0) {
        wav_mode_flag = 1;
        arg_idx++;
        if (arg_idx >= argc) {
            fprintf(stderr, "Usage: xmi_play --wav <file.xmi> [sequence_num]\n");
            return 1;
        }
    }

    path = argv[arg_idx++];
    if (arg_idx < argc) {
        char *end;
        long n = strtol(argv[arg_idx], &end, 10);
        if (*end == '\0') seq_num = (int)n;
    }

    return run_seq(path, seq_num, wav_mode_flag);
}

static int run_seq(const char *path, int seq_num, int wav_mode) {
    uint32_t file_size;
    uint8_t *data = load_file(path, &file_size);
    if (!data) {
        fprintf(stderr, "Failed to load %s\n", path);
        return 1;
    }

    int num_tracks = count_xmi_tracks(data);
    if (num_tracks == 0) {
        fprintf(stderr, "No FORM XMID tracks found in %s\n", path);
        free(data);
        return 1;
    }

    if (seq_num < 0 || seq_num >= num_tracks) {
        fprintf(stderr, "Track %d out of range (0-%d). File has %d track(s).\n",
                seq_num, num_tracks - 1, num_tracks);
        free(data);
        return 1;
    }

    fprintf(stderr, "File has %d track(s). Playing track %d.\n", num_tracks, seq_num);

    if (wav_mode) dro_start("xmi/capture.txt");
    xmi_backend_init();

    xmidi32_init_globals();
    xmidi32_detect_device(0, 0, 0, 0, 0);
    xmidi32_init_driver(0, 0, 0, 0, 0);

    void *cache = malloc(TIMBRE_CACHE_SIZE);
    if (!cache) { fprintf(stderr, "Out of memory\n"); free(data); return 1; }
    xmidi32_define_timbre_cache(0, cache, TIMBRE_CACHE_SIZE);

    struct sequence_state *state = (struct sequence_state *)
        calloc(1, sizeof(struct sequence_state));

    HSEQUENCE h = xmidi32_register_seq(data, seq_num, state, NULL);
    if (h == (HSEQUENCE)-1) {
        fprintf(stderr, "Failed to register track %d\n", seq_num);
        free(data); free(cache); free(state);
        return 1;
    }

    uint32_t j;
    for (j = 0; j < NUM_CHANS; j++)
        xmidi32_map_seq_channel(h, j + 1, j + 1);

    install_sequence_timbres(h);

    int has_loop = has_for_loop(state);
    if (has_loop)
        fprintf(stderr, "Sequence uses loops - will stop after one complete playthrough.\n");

    xmidi32_start_seq(h);

    if (wav_mode) {
        uint32_t spt = (uint32_t)((uint64_t)44100 * 8333ULL / 1000000ULL);
        uint32_t accum = 0;
        uint32_t total = 0;
        uint32_t chunk = 1024;
        int loop_count = 0;
        uint8_t *max_evnt = state->EVNT_ptr;

        int16_t *buf = (int16_t *)malloc(WAV_MAX_SAMPLES * 4);
        if (!buf) { fprintf(stderr, "Out of memory\n"); goto wav_done; }

        fprintf(stderr, "Rendering WAV (spt=%u chunk=%u)...\n", spt, chunk);

        signal(SIGINT, sigint_handler);

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

            if (xmidi32_get_seq_status(h) != SEQ_PLAYING)
                break;

            if (has_loop) {
                if (state->EVNT_ptr > max_evnt)
                    max_evnt = state->EVNT_ptr;
                else if (state->EVNT_ptr < max_evnt - 10) {
                    loop_count++;
                    if (loop_count >= 1) {
                        fprintf(stderr, "Loop detected, stopping render.\n");
                        break;
                    }
                }
            }
        }

        dro_stop();
        fprintf(stderr, "Rendered %u samples\n", total);

        if (total > 0) {
            char wav_name[256];
            snprintf(wav_name, sizeof(wav_name), "%s.wav", path);
            write_wav(wav_name, buf, total);
        }

        free(buf);
    wav_done:;
    } else {
        if (SDL_Init(SDL_INIT_AUDIO | SDL_INIT_NOPARACHUTE) < 0) {
            fprintf(stderr, "SDL_Init failed: %s\n", SDL_GetError());
            goto done;
        }

        if (sdl_audio_init(44100) < 0) {
            fprintf(stderr, "SDL audio init failed: %s\n", SDL_GetError());
            SDL_Quit();
            goto done;
        }

        signal(SIGINT, sigint_handler);

        printf("Playing... Press Ctrl+C to stop.\n");
        while (xmidi32_get_seq_status(h) == SEQ_PLAYING && !g_stop_requested)
            SDL_Delay(8);

        printf("Playback complete.\n");
        SDL_Delay(500);

        sdl_audio_close();
        SDL_Quit();
    }

done:
    xmidi32_stop_seq(h);
    xmidi32_release_seq(h);
    xmidi32_shutdown_driver(0, NULL);
    free(cache);
    free(state);
    free(data);
    xmi_backend_shutdown();
    return 0;
}
