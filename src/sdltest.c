#define SDL_MAIN_HANDLED
#include <SDL2/SDL.h>
#include <stdio.h>
#include <math.h>

static void beep_cb(void *userdata, uint8_t *stream, int len) {
    (void)userdata;
    static double phase = 0.0;
    int16_t *buf = (int16_t *)stream;
    int samples = len / 4;
    for (int i = 0; i < samples; i++) {
        int16_t s = (int16_t)(sin(phase) * 20000.0);
        buf[i*2] = buf[i*2+1] = s;
        phase += 440.0 * 2.0 * 3.1415926535 / 44100.0;
    }
}

int main(void) {
    SDL_SetMainReady();
    if (SDL_Init(SDL_INIT_AUDIO) < 0) {
        fprintf(stderr, "SDL_Init: %s\n", SDL_GetError());
        return 1;
    }
    SDL_AudioSpec want, have;
    want.freq = 44100;
    want.format = AUDIO_S16SYS;
    want.channels = 2;
    want.samples = 1024;
    want.callback = beep_cb;
    want.userdata = NULL;
    if (SDL_OpenAudio(&want, &have) < 0) {
        fprintf(stderr, "SDL_OpenAudio: %s\n", SDL_GetError());
        SDL_Quit();
        return 1;
    }
    printf("Opened audio: freq=%d fmt=0x%x ch=%d samples=%d\n",
           have.freq, have.format, have.channels, have.samples);
    SDL_PauseAudio(0);
    printf("Playing 440Hz tone for 3 seconds...\n");
    SDL_Delay(3000);
    SDL_CloseAudio();
    SDL_Quit();
    printf("Done.\n");
    return 0;
}
