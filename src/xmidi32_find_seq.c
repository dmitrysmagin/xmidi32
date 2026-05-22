#include "xmidi32_driver.h"
#include "xmidi32_utils.h"

uint8_t *find_seq(uint8_t *XMID, uint32_t seq_num) {
    uint8_t *p = XMID;
    uint8_t *end_addr;
    uint32_t chunk_len;
    uint32_t form_count = seq_num + 1;
    uint32_t i;

    for (i = 0; i < form_count; ) {
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
                if (tag == 0x584D4944u) {
                    form_count++;
                    if (form_count == seq_num + 1) {
                        uint32_t xmid_len = read_be32(p + 4);
                        end_addr = p + 8 + xmid_len - 5;
                        if (read_be32(p) != 0x43415420U) {
                            return p;
                        }
                        if (seq_num == 0) return p;
                        return NULL;
                    }
                }
                chunk_len = read_be32(p + 4);
                p += 8 + chunk_len;
            }
        }

        if (tag == 0x464F524Du) {
            if (read_be32(p + 8) != 0x584D4944u) {
                chunk_len = read_be32(p + 4);
                p += 8 + chunk_len;
                continue;
            }
            uint32_t xmid_len = read_be32(p + 4);
            end_addr = p + 8 + xmid_len - 5;
            return p;
        }

        chunk_len = read_be32(p + 4);
        p += 8 + chunk_len;
        i++;
    }

    (void)end_addr;
    return NULL;
}
