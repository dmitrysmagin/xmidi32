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
