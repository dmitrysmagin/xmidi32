#include "xmidi32_driver.h"
#include "xmidi32_utils.h"
#include "xmidi32_reset.h"

uint32_t xmidi32_get_state_size(void) {
    return (uint32_t)sizeof(struct sequence_state);
}

void xmidi32_install_callback(void *fn) {
    trigger_active = 0;
    trigger_fn = (int32_t (*)(int32_t, int32_t))fn;
}

void xmidi32_cancel_callback(void) {
    trigger_active = 0;
    trigger_fn = NULL;
}

HSEQUENCE xmidi32_start_seq(HSEQUENCE sequence) {
    if (sequence == (HSEQUENCE)-1) return sequence;

    struct sequence_state *st = sequence_states[sequence];
    if (st == NULL) return sequence;

    if (st->status == SEQ_PLAYING) {
        xmidi32_stop_seq(sequence);
    }

    rewind_seq(sequence);

    st->EVNT_ptr = st->EVNT + 8;
    st->status = SEQ_PLAYING;
    st->seq_started = 1;

    return sequence;
}

void xmidi32_stop_seq(HSEQUENCE sequence) {
    if (sequence == (HSEQUENCE)-1) return;

    struct sequence_state *st = sequence_states[sequence];
    if (st == NULL) return;

    if (st->status != SEQ_PLAYING) return;

    flush_note_queue(st);
    reset_sequence(st);

    st->status = SEQ_STOPPED;
}

void xmidi32_resume_seq(HSEQUENCE sequence) {
    if (sequence == (HSEQUENCE)-1) return;

    struct sequence_state *st = sequence_states[sequence];
    if (st == NULL) return;

    if (st->status != SEQ_STOPPED) return;
    if (st->seq_started == 0) return;

    restore_sequence(st);

    st->status = SEQ_PLAYING;
}

uint16_t xmidi32_get_seq_status(HSEQUENCE sequence) {
    if (sequence == (HSEQUENCE)-1) return SEQ_STOPPED;

    struct sequence_state *st = sequence_states[sequence];
    if (st == NULL) return SEQ_STOPPED;

    return st->status;
}

void xmidi32_release_seq(HSEQUENCE sequence) {
    if (sequence == (HSEQUENCE)-1) return;

    struct sequence_state *st = sequence_states[sequence];
    if (st == NULL) return;

    if (st->status == SEQ_PLAYING) {
        st->post_release = 1;
        return;
    }

    sequence_states[sequence] = NULL;
    xmidi32_dec_sequence_count();
}
