#include "xmidi32_driver.h"
#include "xmidi32_backend.h"
#include "xmidi32_yamaha.h"

void send_MIDI_message(uint32_t status, uint32_t d1, uint32_t d2) {
    uint32_t chan = status & 0x0F;
    uint32_t type = status & 0xF0;

    if (type == 0x80 || (type == 0x90 && d2 == 0)) {
        yamaha_note_off(chan, d1);
        return;
    }
    if (type == 0x90) {
        yamaha_note_on(chan, d1, d2);
        return;
    }
    if (type == 0xB0) {
        yamaha_controller(chan, d1, d2);
        return;
    }
    if (type == 0xC0) {
        yamaha_program_change(chan, d1);
        return;
    }
    if (type == 0xE0) {
        yamaha_pitch_bend(chan, d1, d2);
        return;
    }
    (void)chan;
    (void)d1;
    (void)d2;
}

void send_MIDI_sysex(const uint8_t *data, uint32_t size) {
    (void)data;
    (void)size;
}

void reset_synth(void) {}
void init_synth(void) {}
void shutdown_synth(void) {}
void serve_synth(void) {}

uint32_t detect_device(uint32_t IO, uint32_t IRQ, uint32_t DMA, uint32_t DRQ) {
    (void)IO;
    (void)IRQ;
    (void)DMA;
    (void)DRQ;
    return 1;
}
void set_IO_parms(uint32_t IO, uint32_t IRQ, uint32_t DMA, uint32_t DRQ) {
    (void)IO;
    (void)IRQ;
    (void)DMA;
    (void)DRQ;
}
void reset_interface(void) {}
void init_interface(void) {}
void sysex_wait(uint32_t ms) { (void)ms; }
