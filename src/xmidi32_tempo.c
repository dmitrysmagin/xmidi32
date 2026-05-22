#include "xmidi32_driver.h"
#include "xmidi32_utils.h"

int32_t xmidi32_get_rel_tempo(HSEQUENCE sequence) {
    if (sequence == (HSEQUENCE)-1) return -1;

    struct sequence_state *st = sequence_states[sequence];
    if (st == NULL) return -1;

    return st->tempo_percent;
}

int32_t xmidi32_get_rel_volume(HSEQUENCE sequence) {
    if (sequence == (HSEQUENCE)-1) return -1;

    struct sequence_state *st = sequence_states[sequence];
    if (st == NULL) return -1;

    return st->vol_percent;
}

void xmidi32_set_rel_tempo(HSEQUENCE sequence, int32_t tempo, int32_t grad) {
    if (sequence == (HSEQUENCE)-1) return;

    struct sequence_state *st = sequence_states[sequence];
    if (st == NULL) return;

    st->tempo_target = tempo;

    if (grad == 0) {
        st->tempo_percent = tempo;
        return;
    }

    int32_t delta = tempo - st->tempo_percent;
    if (delta == 0) return;

    if (delta < 0) delta = -delta;

    int32_t period = (10 * grad) / delta;
    if (period == 0) period = 1;
    st->tempo_period = period;
    st->tempo_accum = 0;
}

void xmidi32_set_rel_volume(HSEQUENCE sequence, int32_t volume, int32_t grad) {
    if (sequence == (HSEQUENCE)-1) return;

    struct sequence_state *st = sequence_states[sequence];
    if (st == NULL) return;

    st->vol_target = volume;

    if (grad == 0) {
        st->vol_percent = volume;
        xmidi32_XMIDI_volume(st);
        return;
    }

    int32_t delta = volume - st->vol_percent;
    if (delta == 0) return;

    if (delta < 0) delta = -delta;

    int32_t period = (10 * grad) / delta;
    if (period == 0) period = 1;
    st->vol_period = period;
    st->vol_accum = 0;
}

int32_t xmidi32_get_control_val(HSEQUENCE sequence, uint32_t chan, uint32_t ctrl) {
    if (sequence == (HSEQUENCE)-1) return -1;

    struct sequence_state *st = sequence_states[sequence];
    if (st == NULL) return -1;

    if (ctrl == CALLBACK_TRIG) {
        return (int32_t)(intptr_t)st->cur_callback;
    }

    uint8_t hash = ctrl_hash[ctrl & 0x7F];
    if (hash == 0xFF) return -1;

    uint32_t idx = hash * NUM_CHANS + (chan - 1);
    return (int32_t)(int8_t)((uint8_t *)&st->chan_controls)[idx];
}

void xmidi32_set_control_val(HSEQUENCE sequence, uint32_t chan,
                             uint32_t ctrl, uint32_t val) {
    if (sequence == (HSEQUENCE)-1) return;

    struct sequence_state *st = sequence_states[sequence];
    if (st == NULL) return;

    xmidi32_XMIDI_control(st, chan - 1, ctrl, val);
}

uint32_t xmidi32_get_chan_notes(HSEQUENCE sequence, uint32_t chan) {
    if (sequence == (HSEQUENCE)-1) return 0;

    struct sequence_state *st = sequence_states[sequence];
    if (st == NULL) return 0;

    uint32_t count = 0;
    uint32_t i;
    uint8_t target = (uint8_t)(chan - 1);
    for (i = 0; i < MAX_NOTES; i++) {
        if ((st->note_queue[i].chan & 0x0F) == target) count++;
    }
    return count;
}

void xmidi32_map_seq_channel(HSEQUENCE sequence, uint32_t seq_chan, uint32_t phys_chan) {
    if (sequence == (HSEQUENCE)-1) return;

    struct sequence_state *st = sequence_states[sequence];
    if (st == NULL) return;

    st->chan_map[seq_chan - 1] = (uint8_t)(phys_chan - 1);
}

uint32_t xmidi32_true_seq_channel(HSEQUENCE sequence, uint32_t seq_chan) {
    if (sequence == (HSEQUENCE)-1) return 0xFFFFFFFFU;

    struct sequence_state *st = sequence_states[sequence];
    if (st == NULL) return 0xFFFFFFFFU;

    return (uint32_t)(st->chan_map[seq_chan - 1] + 1);
}

void xmidi32_branch_index(HSEQUENCE sequence, uint32_t marker) {
    if (sequence == (HSEQUENCE)-1) return;

    struct sequence_state *st = sequence_states[sequence];
    if (st == NULL) return;
    if (st->RBRN == NULL) return;
    if (read_be32(st->RBRN) != 0x4E524252U) return;

    uint16_t count = (uint16_t)(st->RBRN[8] << 8) | st->RBRN[9];
    const uint8_t *p = st->RBRN + 10;
    uint16_t i;
    for (i = 0; i < count; i++) {
        if (p[0] == (uint8_t)marker) {
            uint32_t offset = read_be32(p + 2);
            st->EVNT_ptr = st->EVNT + 8 + offset;
            st->interval_cnt = 0;
            flush_note_queue(st);

            if (BRANCH_EXIT) {
                uint32_t j;
                for (j = 0; j < FOR_NEST; j++) {
                    st->FOR_loop_cnt[j] = -1;
                }
            }
            return;
        }
        p += 6;
    }
}
