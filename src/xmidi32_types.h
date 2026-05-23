#ifndef XMIDI32_TYPES_H
#define XMIDI32_TYPES_H

#include "xmidi32_config.h"

struct ctrl_log {
    uint8_t PV[NUM_CHANS];
    uint8_t MODUL[NUM_CHANS];
    uint8_t PAN[NUM_CHANS];
    uint8_t EXP[NUM_CHANS];
    uint8_t SUS[NUM_CHANS];
    uint8_t PBS[NUM_CHANS];
    uint8_t C_LOCK[NUM_CHANS];
    uint8_t C_PROT[NUM_CHANS];
    uint8_t V_PROT[NUM_CHANS];
};

struct note_entry {
    uint8_t  chan;
    uint8_t  num;
    int32_t  time;
};

struct sequence_state {
    uint8_t *TIMB;
    uint8_t *RBRN;
    uint8_t *EVNT;
    uint8_t *EVNT_ptr;

    void *cur_callback;
    uint8_t *ctrl_ptr;
    int32_t seq_handle;

    uint16_t seq_started;
    uint16_t status;
    uint16_t post_release;
    uint16_t interval_cnt;
    uint16_t note_count;

    int32_t vol_error;
    int32_t vol_percent;
    int32_t vol_target;
    int32_t vol_accum;
    int32_t vol_period;

    int32_t tempo_error;
    int32_t tempo_percent;
    int32_t tempo_target;
    int32_t tempo_accum;
    int32_t tempo_period;

    uint16_t beat_count;
    int16_t  measure_count;
    uint16_t time_numerator;
    int32_t  time_fraction;
    int32_t  beat_fraction;
    int32_t  time_per_beat;

    uint8_t *FOR_ptrs[FOR_NEST];
    int16_t FOR_loop_cnt[FOR_NEST];

    uint8_t chan_map[NUM_CHANS];
    uint8_t chan_program[NUM_CHANS];
    uint8_t chan_pitch_l[NUM_CHANS];
    uint8_t chan_pitch_h[NUM_CHANS];
    int8_t  chan_indirect[NUM_CHANS];

    struct ctrl_log chan_controls;
    struct note_entry note_queue[MAX_NOTES];
};

#endif
