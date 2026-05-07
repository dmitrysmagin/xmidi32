#include "xmidi32_utils.h"

uint32_t read_be32(const uint8_t *p) {
    return ((uint32_t)p[0] << 24) | ((uint32_t)p[1] << 16) |
           ((uint32_t)p[2] << 8)  | (uint32_t)p[3];
}

uint32_t read_vln(const uint8_t **ptr) {
    const uint8_t *p = *ptr;
    uint32_t value = 0;
    int i;

    for (i = 0; i < 4; i++) {
        value = (value << 7) | (p[i] & 0x7F);
        if ((p[i] & 0x80) == 0) {
            *ptr = p + i + 1;
            return value;
        }
    }
    *ptr = p + 4;
    return value;
}

uint32_t vln_size(const uint8_t *start) {
    const uint8_t *p = start;
    read_vln(&p);
    return (uint32_t)(p - start);
}

uint32_t ul_divide(uint32_t num, uint32_t den) {
    return num / den;
}

void advance_count(struct sequence_state *st, int32_t *out_bar, int32_t *out_beat) {
    int32_t bar = st->measure_count;
    int32_t beat = st->beat_count;
#if QUANT_ADVANCE
    int32_t frac = st->beat_fraction;
    int32_t i;
    for (i = 0; i < QUANT_ADVANCE; i++) {
        frac += st->time_fraction;
        if (frac >= st->time_per_beat) {
            frac -= st->time_per_beat;
            beat++;
            if (beat >= st->time_numerator) {
                beat = 0;
                bar++;
            }
        }
    }
    (void)frac;
#endif
    *out_bar = bar;
    *out_beat = beat;
}
