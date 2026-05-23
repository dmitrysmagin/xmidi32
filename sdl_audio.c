#include <SDL.h>
#include "sdl_audio.h"
#include "backend.h"
#include "src/xmidi32_driver.h"

static void sdl_audio_callback(void *userdata, uint8_t *stream, int len) {
    (void)userdata;
    int16_t *buf = (int16_t *)stream;
    uint32_t samples = (uint32_t)len / 4;
    static uint32_t accum = 0;
    uint32_t spt = (uint32_t)((uint64_t)44100 * 8333ULL / 1000000ULL);

    accum += samples;
    while (accum >= spt) {
        accum -= spt;
        xmidi32_serve_driver();
    }

    uint32_t i;
    for (i = 0; i < samples; i++)
        OPL3_GenerateResampled(xmi_backend_get_chip(), buf + i * 2);
}

int sdl_audio_init(uint32_t sample_rate) {
    (void)sample_rate;

    SDL_AudioSpec want, have;
    SDL_zero(want);
    want.freq = 44100;
    want.format = AUDIO_S16SYS;
    want.channels = 2;
    want.samples = 1024;
    want.callback = sdl_audio_callback;
    if (SDL_OpenAudio(&want, &have) < 0) {
        fprintf(stderr, "SDL_OpenAudio failed: %s\n", SDL_GetError());
        return -1;
    }
    fprintf(stderr, "SDL audio: freq=%d fmt=0x%X ch=%d samples=%d\n",
            have.freq, have.format, have.channels, have.samples);
    SDL_PauseAudio(0);
    return 0;
}

void sdl_audio_close(void) {
    SDL_CloseAudio();
}
