#include "xmidi32_driver.h"

#define GCTL(chan, ctrl_idx) (((uint8_t *)&global_controls)[(ctrl_idx) * NUM_CHANS + (chan)])

uint32_t xmidi32_lock_channel(void) {
    int32_t best = -1;
    uint8_t mask = 0xC0;

    for (;;) {
        int32_t cand = -1;
        uint8_t cand_notes = 0;
        uint32_t i;

        for (i = MIN_TRUE_CHAN - 1; i < MAX_TRUE_CHAN; i++) {
            if ((lock_status[i] & mask) != 0) continue;
            if (cand == -1 || active_notes[i] < cand_notes) {
                cand = (int32_t)i;
                cand_notes = active_notes[i];
            }
        }

        if (cand != -1) {
            best = cand;
            break;
        }

        if (mask == 0x80) break;
        mask = 0x80;
    }

    if (best == -1) return 0;

    xmidi32_send_controller((uint32_t)best, SUSTAIN, 0);
    flush_channel_notes((uint32_t)best);
    active_notes[best] = 0;
    lock_status[best] |= 0x80;

    return (uint32_t)(best + 1);
}

void xmidi32_release_channel(uint32_t chan) {
    if (chan == 0 || chan > NUM_CHANS) return;

    uint32_t c = chan - 1;
    if ((lock_status[c] & 0x80) == 0) return;
    lock_status[c] &= 0x7F;

    active_notes[c] = 0;
    xmidi32_send_controller(c, SUSTAIN, 0);
    xmidi32_send_controller(c, ALL_NOTES_OFF, 0);

    uint32_t i;
    for (i = 0; i < 9; i++) {
        uint8_t ctrl = logged_ctrls[i];
        uint8_t val = GCTL(c, i);
        if (val == 0xFF) continue;
        xmidi32_send_controller(c, ctrl, val);
    }

    if (global_program[c] != 0xFF) {
        xmidi32_send_program_change(c, global_program[c]);
    }

    if (global_pitch_l[c] != 0xFF && global_pitch_h[c] != 0xFF) {
        xmidi32_send_pitch_bend(c, global_pitch_l[c], global_pitch_h[c]);
    }
}
