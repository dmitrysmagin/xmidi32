#include "xmidi32_driver.h"
#include "xmidi32_timbre_internal.h"

#define DEF_TC_SIZE 3584

static uint32_t cache_size = 0;

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
    (void)seq;
    return 0;
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
