#include "xmidi32_driver.h"
#include "xmidi32_utils.h"

HSEQUENCE xmidi32_register_seq(
    uint8_t *XMID,
    uint32_t sequence_num,
    struct sequence_state *state,
    uint8_t *ctrl
) {
    uint32_t i;
    HSEQUENCE handle = -1;
    uint8_t *chunk;

    for (i = 0; i < NSEQS; i++) {
        if (sequence_states[i] == NULL) {
            handle = (HSEQUENCE)i;
            break;
        }
    }
    if (handle == -1) return -1;

    chunk = find_seq(XMID, sequence_num);
    if (chunk == NULL) return -1;

    sequence_states[handle] = state;

    state->TIMB = NULL;
    state->RBRN = NULL;
    state->EVNT = NULL;
    state->seq_handle = (int32_t)handle;
    state->post_release = 0;
    state->seq_started = 0;
    state->status = SEQ_STOPPED;
    state->ctrl_ptr = ctrl;

    uint8_t *p = chunk + 12;

    for (;;) {
        uint32_t tag = read_be32(p);
        uint32_t chunk_len = read_be32(p + 4);

        if (tag == 0x54494D42u) {
            state->TIMB = p;
        } else if (tag == 0x5242524Eu) {
            state->RBRN = p;
        } else if (tag == 0x45564E54u) {
            state->EVNT = p;
            break;
        }

        p += 8 + chunk_len;
    }

    xmidi32_inc_sequence_count();
    rewind_seq(handle);

    return handle;
}

void rewind_seq(HSEQUENCE sequence) {
    uint32_t i;
    struct sequence_state *st = sequence_states[sequence];
    if (st == NULL) return;

    for (i = 0; i < FOR_NEST; i++) {
        st->FOR_loop_cnt[i] = -1;
    }

    for (i = 0; i < NUM_CHANS; i++) {
        st->chan_map[i] = (uint8_t)i;
        st->chan_program[i] = 0xFF;
        st->chan_pitch_l[i] = 0xFF;
        st->chan_pitch_h[i] = 0xFF;
        st->chan_indirect[i] = -1;
    }

    for (i = 0; i < sizeof(st->chan_controls); i++) {
        ((uint8_t *)&st->chan_controls)[i] = 0xFF;
    }

    for (i = 0; i < MAX_NOTES; i++) {
        st->note_queue[i].chan = 0xFF;
    }

    st->cur_callback = NULL;
    st->interval_cnt = 0;
    st->note_count = 0;
    st->vol_percent = DEF_SYNTH_VOL;
    st->vol_target = DEF_SYNTH_VOL;
    st->tempo_percent = 100;
    st->tempo_target = 100;
    st->tempo_error = 0;

    st->beat_count = 0;
    st->measure_count = -1;

    st->beat_fraction = 0;
    st->time_fraction = 0;
    st->time_numerator = 4;
    st->time_per_beat = 0x07A1200U;
}
