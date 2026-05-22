#include <SDL2/SDL.h>
#include "sdl_audio.h"
#include "backend.h"

static void sdl_audio_callback(void *userdata, uint8_t *stream, int len) {
    (void)userdata;
    int16_t *buf = (int16_t *)stream;
    uint32_t samples = (uint32_t)len / 4;
    xmi_backend_fill_buffer(buf, samples);
}

int sdl_audio_init(uint32_t sample_rate) {
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
