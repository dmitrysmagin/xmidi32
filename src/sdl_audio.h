#ifndef SDL_AUDIO_H
#define SDL_AUDIO_H

#include <stdint.h>

int sdl_audio_init(uint32_t sample_rate);
void sdl_audio_close(void);

#endif
