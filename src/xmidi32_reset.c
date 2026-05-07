#include "xmidi32_driver.h"
#include "xmidi32_reset.h"

#define CTRL_VAL(st, ctrl, ch) (((uint8_t *)(&(st)->chan_controls))[(ctrl) * NUM_CHANS + (ch)])

void reset_sequence(struct sequence_state *st) {
    uint32_t i;
    for (i = 0; i < NUM_CHANS; i++) {
        if (st->chan_controls.SUS[i] >= 64) {
            global_controls.SUS[i] = 0;
            xmidi32_send_controller(i, SUSTAIN, 0);
        }

        if (st->chan_controls.C_LOCK[i] >= 64) {
            flush_channel_notes(i);
            if (st->chan_map[i] < NUM_CHANS) {
                xmidi32_release_channel(st->chan_map[i] + 1);
            }
            st->chan_map[i] = (uint8_t)i;
        }

        if (st->chan_controls.C_PROT[i] >= 64) {
            lock_status[i] &= 0xBF;
        }

        if (st->chan_controls.V_PROT[i] >= 64) {
            xmidi32_send_controller(i, VOICE_PROTECT, 0);
        }
    }
}

void restore_sequence(struct sequence_state *st) {
    uint32_t i;

    for (i = 0; i < NUM_CHANS; i++) {
        int8_t lock_val = st->chan_controls.C_LOCK[i];
        if (lock_val != -1 && lock_val >= 64) {
            int32_t phys = (int32_t)xmidi32_lock_channel() - 1;
            if (phys == -1) phys = (int32_t)i;
            st->chan_map[i] = (uint8_t)phys;
        }
    }

    for (i = 0; i < 9; i++) {
        uint8_t ctrl = logged_ctrls[i];
        if (ctrl == CHAN_LOCK) continue;
        uint32_t j;
        for (j = 0; j < NUM_CHANS; j++) {
            uint8_t val = CTRL_VAL(st, i, j);
            if (val == 0xFF) continue;
            xmidi32_XMIDI_control(st, j, ctrl, val);
        }
    }

    for (i = 0; i < NUM_CHANS; i++) {
        if (st->chan_pitch_l[i] != 0xFF && st->chan_pitch_h[i] != 0xFF) {
            uint32_t phys = st->chan_map[i];
            xmidi32_send_pitch_bend(phys, st->chan_pitch_l[i], st->chan_pitch_h[i]);
        }
        if (st->chan_program[i] != 0xFF) {
            uint32_t phys = st->chan_map[i];
            xmidi32_send_program_change(phys, st->chan_program[i]);
        }
    }
}
