#include "xmidi32_driver.h"

uint32_t xmidi32_XMIDI_note_on(struct sequence_state *st) {
    const uint8_t *p = st->EVNT_ptr;
    const uint8_t *event_start = p;

    uint8_t chan = p[0] & 0x0F;
    uint8_t note = p[1];
    uint8_t velocity = p[2];

    const uint8_t *dur_ptr = p + 3;
    uint32_t duration = 0;
    int shift_count = 0;

    for (;;) {
        uint8_t byte = *dur_ptr++;
        duration = (duration << 7) | (byte & 0x7F);
        shift_count++;
        if ((byte & 0x80) == 0) break;
        if (shift_count >= 4) break;
    }

    uint32_t event_len = (uint32_t)(dur_ptr - event_start);

    if ((lock_status[chan] & 0x80) != 0) return event_len;

    uint32_t phys_chan = st->chan_map[chan];

    if (velocity == 0) {
        int ns;
        for (ns = 0; ns < MAX_NOTES; ns++) {
            if (st->note_queue[ns].chan == chan && st->note_queue[ns].num == note) {
                st->note_queue[ns].chan = 0xFF;
                if (st->chan_map[chan & 0x0F] < NUM_CHANS)
                    active_notes[phys_chan]--;
                xmidi32_send_note_off(phys_chan, note, 0);
                st->note_count--;
                break;
            }
        }
        return event_len;
    }

    uint32_t slot;
    for (slot = 0; slot < MAX_NOTES; slot++) {
        if (st->note_queue[slot].chan == 0xFF) break;
    }

    if (slot == MAX_NOTES) {
        slot = 0;
    } else {
        st->note_count++;
    }

    st->note_queue[slot].chan = (uint8_t)chan;
    st->note_queue[slot].num = note;
    st->note_queue[slot].time = (int32_t)duration - 1;

    active_notes[phys_chan]++;
    xmidi32_send_note_on(phys_chan, note, velocity);

    return event_len;
}
