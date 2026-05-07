#include "xmidi32_driver.h"

uint32_t xmidi32_get_beat_count(HSEQUENCE sequence) {
    if (sequence == (HSEQUENCE)-1) return 0;

    struct sequence_state *st = sequence_states[sequence];
    if (st == NULL) return 0;

    uint16_t beat = st->beat_count;
    int16_t measure = st->measure_count;

    if (QUANT_ADVANCE != 0) {
        uint32_t i;
        int32_t frac = (int32_t)st->beat_fraction;
        for (i = 0; i < QUANT_ADVANCE; i++) {
            frac += (int32_t)st->time_fraction;
            if (frac >= (int32_t)st->time_per_beat) {
                frac -= (int32_t)st->time_per_beat;
                beat++;
                if (beat >= st->time_numerator) {
                    beat = 0;
                    measure++;
                }
            }
        }
    }

    (void)measure;
    return beat;
}

uint32_t xmidi32_get_bar_count(HSEQUENCE sequence) {
    if (sequence == (HSEQUENCE)-1) return 0;

    struct sequence_state *st = sequence_states[sequence];
    if (st == NULL) return 0;

    uint16_t beat = st->beat_count;
    int16_t measure = st->measure_count;

    if (QUANT_ADVANCE != 0) {
        uint32_t i;
        int32_t frac = (int32_t)st->beat_fraction;
        for (i = 0; i < QUANT_ADVANCE; i++) {
            frac += (int32_t)st->time_fraction;
            if (frac >= (int32_t)st->time_per_beat) {
                frac -= (int32_t)st->time_per_beat;
                beat++;
                if (beat >= st->time_numerator) {
                    beat = 0;
                    measure++;
                }
            }
        }
    }

    return (uint32_t)measure;
}
