#include <SDL2/SDL.h>
#include "sdl_audio.h"
#include "backend.h"
#include "src/xmidi32_driver.h"

static uint32_t g_samples_per_tick = 1;
static uint32_t g_sample_accum = 0;

static void sdl_audio_callback(void *userdata, uint8_t *stream, int len) {
    (void)userdata;
    int16_t *buf = (int16_t *)stream;
    uint32_t samples = (uint32_t)len / 4;
    opl3_chip *chip = xmi_backend_get_chip();
    uint32_t spt = g_samples_per_tick;

    uint32_t i;
    for (i = 0; i < samples; i++) {
        g_sample_accum++;
        if (g_sample_accum >= spt) {
            g_sample_accum -= spt;
            xmidi32_serve_driver();
        }
        OPL3_GenerateResampled(chip, buf + i * 2);
    }
}

int sdl_audio_init(uint32_t sample_rate) {
    g_samples_per_tick = (uint32_t)((uint64_t)sample_rate * QUANT_TIME / 1000000ULL);

    SDL_AudioSpec want, have;
    SDL_zero(want);
    want.freq = (int)sample_rate;
    want.format = AUDIO_S16SYS;
    want.channels = 2;
    want.samples = 1024;
    want.callback = sdl_audio_callback;
    if (SDL_OpenAudio(&want, &have) < 0) {
        return -1;
    }
    SDL_PauseAudio(0);
    return 0;
}

void sdl_audio_close(void) {
    SDL_CloseAudio();
}
