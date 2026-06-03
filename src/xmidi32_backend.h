#ifndef XMIDI32_BACKEND_H
#define XMIDI32_BACKEND_H

#include <stdint.h>

#define XMI_EMULATION 1

#define NUM_VOICES  18
#define NUM_SLOTS   20

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
