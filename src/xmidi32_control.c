#include "xmidi32_driver.h"
#include "xmidi32_control.h"

uint32_t xmidi32_XMIDI_control(struct sequence_state *st, uint32_t log_chan,
                              uint32_t ctrl, uint32_t val) {
    if (st->chan_indirect[log_chan] != -1) {
        uint32_t idx = (uint8_t)st->chan_indirect[log_chan];
        val = st->ctrl_ptr[idx];
        st->chan_indirect[log_chan] = -1;
    }

    uint8_t hash = ctrl_hash[ctrl & 0x7F];
    if (hash != 0xFF) {
        ((uint8_t *)&global_controls)[hash + log_chan] = (uint8_t)val;
        ((uint8_t *)&(st)->chan_controls)[hash + log_chan] = (uint8_t)val;
    }

    if (ctrl == PART_VOLUME) {
        if (st->vol_percent != 100) {
            uint32_t scaled = ((uint32_t)val * (uint32_t)st->vol_percent) / 100;
            if (scaled > 127) scaled = 127;
            global_controls.PV[log_chan] = (uint8_t)scaled;
            val = scaled;
        }
        if ((lock_status[log_chan] & 0x80) != 0) return 3;
        uint32_t phys = st->chan_map[log_chan];
        xmidi32_send_controller(phys, ctrl, val);
        return 3;
    }

    if (ctrl == CLEAR_BEAT_BAR) {
        st->beat_count = 0;
        st->measure_count = 0;
        st->beat_fraction = -(int32_t)st->time_fraction;
        return 3;
    }

    if (ctrl == CALLBACK_TRIG) {
        st->cur_callback = (void *)(intptr_t)val;
        if (trigger_fn != NULL) {
            trigger_fn(st->seq_handle, val);
        }
        return 3;
    }

    if (ctrl == FOR_LOOP) {
        uint32_t slot;
        for (slot = 0; slot < FOR_NEST; slot++) {
            if (st->FOR_loop_cnt[slot] == -1) break;
        }
        if (slot == FOR_NEST) return 3;
        st->FOR_loop_cnt[slot] = (int16_t)val;
        st->FOR_ptrs[slot] = st->EVNT_ptr;
        return 3;
    }

    if (ctrl == NEXT_LOOP) {
        if (val < 64) return 3;
        uint32_t slot;
        for (slot = FOR_NEST; slot > 0; slot--) {
            if (st->FOR_loop_cnt[slot - 1] != -1) break;
        }
        if (slot == 0) return 3;
        slot--;
        st->FOR_loop_cnt[slot]--;
        if (st->FOR_loop_cnt[slot] <= 0) {
            st->FOR_loop_cnt[slot] = -1;
            return 3;
        }
        st->EVNT_ptr = st->FOR_ptrs[slot];
        return 3;
    }

    if (ctrl == CHAN_PROTECT) {
        if (val >= 64) lock_status[log_chan] |= 0x40;
        else          lock_status[log_chan] &= 0xBF;
        return 3;
    }

    if (ctrl == CHAN_LOCK) {
        if (val >= 64) {
            int32_t phys = (int32_t)xmidi32_lock_channel() - 1;
            if (phys == -1) phys = (int32_t)log_chan;
            st->chan_map[log_chan] = (uint8_t)phys;
        } else {
            flush_channel_notes(log_chan);
            if (st->chan_map[log_chan] < NUM_CHANS) {
                xmidi32_release_channel(st->chan_map[log_chan] + 1);
            }
            st->chan_map[log_chan] = (uint8_t)log_chan;
        }
        return 3;
    }

    if (ctrl == INDIRECT_C_PFX) {
        st->chan_indirect[log_chan] = (int8_t)val;
        return 3;
    }

    if ((lock_status[log_chan] & 0x80) != 0) return 3;
    uint32_t phys = st->chan_map[log_chan];
    xmidi32_send_controller(phys, ctrl, val);
    return 3;
}
