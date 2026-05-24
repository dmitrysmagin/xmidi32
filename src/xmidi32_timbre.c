#include "xmidi32_driver.h"
#include "xmidi32_timbre_internal.h"
#include "xmidi32_utils.h"

#define DEF_TC_SIZE 3584

uint32_t xmidi32_detect_device(HDRIVER h, uint32_t IO, uint32_t IRQ,
                                uint32_t DMA, uint32_t DRQ) {
    (void)h;
    (void)IO;
    (void)IRQ;
    (void)DMA;
    (void)DRQ;
    return 1;
}

uint32_t xmidi32_get_timbre_cache_size(HDRIVER h) {
    (void)h;
    return yamaha_get_cache_size();
}

void xmidi32_define_timbre_cache(HDRIVER h, void *addr, uint32_t size) {
    (void)h;
    yamaha_define_cache(addr, size > 0 ? size : DEF_TC_SIZE);
}

uint32_t xmidi32_timbre_request(HDRIVER h, HSEQUENCE seq) {
    (void)h;

    if (seq == (HSEQUENCE)-1) return 0xFFFFFFFFU;

    if ((uint32_t)seq >= (uint32_t)NSEQS) return 0xFFFFFFFFU;

    struct sequence_state *st = sequence_states[seq];
    if (st == NULL) return 0xFFFFFFFFU;

    const uint8_t *timb = st->TIMB;
    if (timb == NULL) return 0xFFFFFFFFU;

    if (timb[0] != 'T' || timb[1] != 'I' ||
        timb[2] != 'M' || timb[3] != 'B') {
        return 0xFFFFFFFFU;
    }

    uint32_t chunk_len = ((uint32_t)timb[4] << 24) |
                          ((uint32_t)timb[5] << 16) |
                          ((uint32_t)timb[6] <<  8) |
                          (uint32_t)timb[7];

    if (chunk_len < 2) return 0xFFFFFFFFU;

    uint32_t offset = 8;
    uint32_t count = read_le_16(timb + offset);
    offset += 2;

    if (count == 0) return 0xFFFFFFFFU;

    uint32_t i;
    for (i = 0; i < count; i++) {
        if (offset + 1 >= chunk_len) break;

        uint32_t patch = (uint32_t)timb[offset];
        uint32_t bank  = (uint32_t)timb[offset + 1];
        uint32_t gnum  = (bank << 8) | patch;

        if (yamaha_timbre_status(bank, patch) == 0) {
            return gnum;
        }

        offset += 2;
    }

    return 0xFFFFFFFFU;
}

void xmidi32_install_timbre(HDRIVER h, uint32_t bank, uint32_t patch,
                             const void *data) {
    (void)h;
    if (bank >= 128 || patch >= 128) return;
    yamaha_install_timbre(bank, patch, data);
}

void xmidi32_protect_timbre(HDRIVER h, uint32_t bank, uint32_t patch) {
    (void)h;
    if (bank >= 128 || patch >= 128) return;
    yamaha_protect_timbre(bank, patch);
}

void xmidi32_unprotect_timbre(HDRIVER h, uint32_t bank, uint32_t patch) {
    (void)h;
    if (bank >= 128 || patch >= 128) return;
    yamaha_unprotect_timbre(bank, patch);
}

uint32_t xmidi32_timbre_status(HDRIVER h, int32_t bank, int32_t patch) {
    (void)h;
    if (bank < 0 || bank >= 128 || patch < 0 || patch >= 128) return 0;
    return (uint32_t)yamaha_timbre_status((uint32_t)bank, (uint32_t)patch);
}
