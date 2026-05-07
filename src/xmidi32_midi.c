#include "xmidi32_driver.h"
#include "xmidi32_backend.h"

void xmidi32_send_note_off(uint32_t phys_chan, uint32_t note, uint32_t vel) {
    (void)vel;
    send_MIDI_message(0x80 | (phys_chan & 0x0F), note, 0);
}

void xmidi32_send_note_on(uint32_t phys_chan, uint32_t note, uint32_t vel) {
    send_MIDI_message(0x90 | (phys_chan & 0x0F), note, vel);
}

void xmidi32_send_controller(uint32_t phys_chan, uint32_t ctrl, uint32_t val) {
    send_MIDI_message(0xB0 | (phys_chan & 0x0F), ctrl, val);
}

void xmidi32_send_program_change(uint32_t phys_chan, uint32_t program) {
    send_MIDI_message(0xC0 | (phys_chan & 0x0F), program, 0);
}

void xmidi32_send_pitch_bend(uint32_t phys_chan, uint32_t pitch_l, uint32_t pitch_h) {
    send_MIDI_message(0xE0 | (phys_chan & 0x0F), pitch_l, pitch_h);
}

void xmidi32_send_raw_message(uint32_t status, uint32_t data_1, uint32_t data_2) {
    send_MIDI_message(status, data_1, data_2);
}

void xmidi32_send_sysex(const uint8_t *data, uint32_t size) {
    send_MIDI_sysex(data, size);
}
