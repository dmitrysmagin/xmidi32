#ifndef XMIDI32_BACKEND_H
#define XMIDI32_BACKEND_H

#include <stdint.h>

#define XMI_EMULATION 1

#define YM3812  1
#define YMF262  2
#define ROLAND  3
#define SPKR    4

#ifndef SYNTH_TYPE
#define SYNTH_TYPE YM3812
#endif

#if SYNTH_TYPE == YM3812 || SYNTH_TYPE == YMF262
#define NUM_VOICES  (SYNTH_TYPE == YMF262 ? 18 : 9)
#define NUM_SLOTS   (SYNTH_TYPE == YMF262 ? 20 : 16)
#elif SYNTH_TYPE == ROLAND
#define NUM_VOICES  9
#define NUM_SLOTS   64
#elif SYNTH_TYPE == SPKR
#define NUM_VOICES  3
#define NUM_SLOTS   4
#endif

#define SLOT_FREE   0
#define SLOT_KEYON  1
#define SLOT_KEYOFF 2

void send_MIDI_message(uint32_t status, uint32_t d1, uint32_t d2);
void send_MIDI_sysex(const uint8_t *data, uint32_t size);

void reset_synth(void);
void init_synth(void);
void shutdown_synth(void);
void serve_synth(void);

uint32_t detect_device(uint32_t IO, uint32_t IRQ, uint32_t DMA, uint32_t DRQ);
void set_IO_parms(uint32_t IO, uint32_t IRQ, uint32_t DMA, uint32_t DRQ);

void reset_interface(void);
void init_interface(void);
void sysex_wait(uint32_t ms);

#endif
