#ifndef XMIDI32_YAMAHA_H
#define XMIDI32_YAMAHA_H

#include <stdint.h>

void yamaha_note_on(uint32_t chan, uint32_t note, uint32_t vel);
void yamaha_note_off(uint32_t chan, uint32_t note);
void yamaha_controller(uint32_t chan, uint32_t ctrl, uint32_t val);
void yamaha_program_change(uint32_t chan, uint32_t program);
void yamaha_pitch_bend(uint32_t chan, uint32_t pitch_l, uint32_t pitch_h);

#endif
