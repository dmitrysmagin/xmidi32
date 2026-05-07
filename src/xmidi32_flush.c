#include "xmidi32_driver.h"

extern void xmidi32_send_note_off(uint32_t phys_chan, uint32_t note, uint32_t vel);

void flush_note_queue(struct sequence_state *st) {
    uint32_t i;
    for (i = 0; i < MAX_NOTES; i++) {
        if (st->note_queue[i].chan == 0xFF) continue;
        uint32_t note = st->note_queue[i].num;
        uint32_t phys_chan = st->chan_map[st->note_queue[i].chan & 0x0F];
        st->note_queue[i].chan = 0xFF;
        if (phys_chan < NUM_CHANS && active_notes[phys_chan] != 0) {
            active_notes[phys_chan]--;
        }
        xmidi32_send_note_off(phys_chan, note, 0);
    }
    st->note_count = 0;
}

void flush_channel_notes(uint32_t chan) {
    int32_t i;
    for (i = 0; i < NSEQS; i++) {
        struct sequence_state *st = sequence_states[i];
        if (st == NULL) continue;
        if (st->note_count == 0) continue;

        uint32_t j;
        for (j = 0; j < MAX_NOTES; j++) {
            if (st->note_queue[j].chan == 0xFF) continue;
            if ((st->note_queue[j].chan & 0x0F) != (uint8_t)chan) continue;
            uint32_t note = st->note_queue[j].num;
            uint32_t phys_chan = st->chan_map[chan];
            st->note_queue[j].chan = 0xFF;
            if (phys_chan < NUM_CHANS && active_notes[phys_chan] != 0) {
                active_notes[phys_chan]--;
            }
            xmidi32_send_note_off(phys_chan, note, 0);
            st->note_count--;
        }
    }
}
