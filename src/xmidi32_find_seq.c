#include "xmidi32_driver.h"
#include "xmidi32_utils.h"

static uint32_t read_be32(const uint8_t *p) {
    return ((uint32_t)p[0] << 24) | ((uint32_t)p[1] << 16) |
           ((uint32_t)p[2] << 8)  | (uint32_t)p[3];
}

uint8_t *find_seq(uint8_t *XMID, uint32_t seq_num) {
    uint8_t *p = XMID;
    uint8_t *end_addr;
    uint32_t chunk_len;
    uint32_t form_count = seq_num + 1;
    uint32_t i;

    for (i = 0; i < form_count; i++) {
        uint32_t tag = read_be32(p);
        if (tag == 0x54414320U) {
            p = XMID;
            return NULL;
        }

        if (tag == 0x43415420U) {
            uint32_t root_len = read_be32(p + 4);
            uint8_t *root_end = p + 8 + root_len;

            p += 12;
            form_count = 0;
            for (;;) {
                if (p >= root_end) return NULL;
                tag = read_be32(p + 8);
                if (tag == 0x44494D58U) {
                    form_count++;
                    if (form_count == seq_num + 1) {
                        uint32_t xmid_len = read_be32(p + 4);
                        end_addr = p + 8 + xmid_len - 5;
                        if (read_be32(p) != 0x43415420U) {
                            return p + 8;
                        }
                        if (seq_num == 0) return p + 8;
                        return NULL;
                    }
                }
                chunk_len = read_be32(p + 4);
                p += 8 + chunk_len;
            }
        }

        if (tag == 0x4D524F46U) {
            uint32_t xmid_len = read_be32(p + 4);
            end_addr = p + 8 + xmid_len - 5;
            if (seq_num == 0) return p + 8;
            return NULL;
        }

        chunk_len = read_be32(p + 4);
        p += 8 + chunk_len;
    }

    (void)end_addr;
    return NULL;
}
