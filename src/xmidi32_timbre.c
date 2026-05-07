#include "xmidi32_driver.h"

#define DEF_TC_SIZE 3584

static uint8_t timbre_cache[DEF_TC_SIZE];
static uint32_t cache_size = 0;
static uint8_t timbre_protected[192];
static uint32_t timbre_installed[192];

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
    return cache_size;
}

void xmidi32_define_timbre_cache(HDRIVER h, void *addr, uint32_t size) {
    (void)h;
    if (addr != NULL && size > 0) {
        cache_size = size;
    } else {
        cache_size = DEF_TC_SIZE;
    }
    (void)addr;
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
    uint16_t idx = (uint16_t)(bank * 128 + patch);
    if (idx >= 192) return;
    if (timbre_protected[idx] != 0) return;
    timbre_installed[idx] = 1;
    (void)data;
}

void xmidi32_protect_timbre(HDRIVER h, uint32_t bank, uint32_t patch) {
    (void)h;
    if (bank >= 128 || patch >= 128) return;
    uint16_t idx = (uint16_t)(bank * 128 + patch);
    if (idx >= 192) return;
    timbre_protected[idx] = 1;
}

void xmidi32_unprotect_timbre(HDRIVER h, uint32_t bank, uint32_t patch) {
    (void)h;
    if (bank >= 128 || patch >= 128) return;
    uint16_t idx = (uint16_t)(bank * 128 + patch);
    if (idx >= 192) return;
    timbre_protected[idx] = 0;
}

uint32_t xmidi32_timbre_status(HDRIVER h, int32_t bank, int32_t patch) {
    (void)h;
    if (bank < 0 || bank >= 128 || patch < 0 || patch >= 128) return 0;
    uint16_t idx = (uint16_t)(bank * 128 + patch);
    if (idx >= 192) return 0;
    return timbre_installed[idx] ? 1 : 0;
}
