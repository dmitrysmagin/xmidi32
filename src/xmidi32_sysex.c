#include "xmidi32_driver.h"

uint32_t xmidi32_XMIDI_sysex(struct sequence_state *st) {
    const uint8_t *p = st->EVNT_ptr;
    uint8_t type = p[0];

    const uint8_t *data_start = p + 1;
    const uint8_t *vln_ptr = data_start;
    uint32_t data_len = read_vln(&vln_ptr);

    uint32_t header_len = (uint32_t)(vln_ptr - data_start);
    uint32_t total_len = header_len + data_len;

    xmidi32_send_sysex(vln_ptr, data_len);

    return total_len;
}
