#include "xmidi32_driver.h"

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
    return 0;
}

void xmidi32_define_timbre_cache(HDRIVER h, void *addr, uint32_t size) {
    (void)h;
    (void)addr;
    (void)size;
}

uint32_t xmidi32_timbre_request(HDRIVER h, HSEQUENCE seq) {
    (void)h;
    (void)seq;
    return 0;
}

void xmidi32_install_timbre(HDRIVER h, uint32_t bank, uint32_t patch, const void *data) {
    (void)h;
    (void)bank;
    (void)patch;
    (void)data;
}

void xmidi32_protect_timbre(HDRIVER h, uint32_t bank, uint32_t patch) {
    (void)h;
    (void)bank;
    (void)patch;
}

void xmidi32_unprotect_timbre(HDRIVER h, uint32_t bank, uint32_t patch) {
    (void)h;
    (void)bank;
    (void)patch;
}

uint32_t xmidi32_timbre_status(HDRIVER h, int32_t bank, int32_t patch) {
    (void)h;
    (void)bank;
    (void)patch;
    return 0;
}
