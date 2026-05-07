#include "xmidi32_driver.h"

uint32_t xmidi32_XMIDI_meta(struct sequence_state *st) {
    const uint8_t *p = st->EVNT_ptr;
    uint8_t meta_type = p[1];

    const uint8_t *data_start = p + 2;
    const uint8_t *vln_ptr = data_start;
    uint32_t data_len = read_vln(&vln_ptr);

    uint32_t header_len = (uint32_t)(vln_ptr - data_start);
    uint32_t total_len = header_len + data_len;

    if (meta_type == 0x2F) {
        reset_sequence(st);
        st->status = SEQ_DONE;
        if (st->post_release != 0) {
            xmidi32_release_seq(st->seq_handle);
        }
        return total_len;
    }

    if (meta_type == 0x58) {
        st->time_numerator = p[3];

        uint8_t denom_byte = p[4];
        uint8_t denom_exp;
        if (denom_byte >= 2) {
            denom_exp = denom_byte - 2;
        } else {
            denom_exp = 2 - denom_byte;
        }

        uint32_t base = QUANT_TIME_16 >> denom_exp;
        if (base == 0) base = 1;

        st->time_fraction = (int32_t)base;
        st->beat_fraction = -(int32_t)base;
        st->beat_count = 0;
        st->measure_count++;

        return total_len;
    }

    if (meta_type == 0x51) {
        if (data_len >= 3) {
            uint32_t tempo = ((uint32_t)p[3] << 16) |
                             ((uint32_t)p[4] << 8)  |
                             (uint32_t)p[5];
            tempo = (tempo << 4) & 0x0FFFFFF0U;
            st->time_per_beat = tempo;
        }
        return total_len;
    }

    return total_len;
}
