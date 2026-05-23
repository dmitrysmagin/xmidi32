#include "backend.h"
#include "opl3.h"
#include <stdio.h>

static opl3_chip g_opl3;

static FILE *dro_fp = NULL;
static unsigned long dro_tick_count = 0;

void dro_start(const char *path) {
    dro_fp = fopen(path, "w");
    dro_tick_count = 0;
}

void dro_stop(void) {
    if (dro_fp) fclose(dro_fp);
    dro_fp = NULL;
}

void dro_tick(void) {
    dro_tick_count++;
}

void xmi_backend_opl_write(uint16_t reg, uint8_t val) {
    OPL3_WriteReg(&g_opl3, reg, val);
    if (dro_fp) {
        unsigned chip = (reg >> 8) & 1;
        unsigned addr = reg & 0xFF;
        fprintf(dro_fp, "%lu %u %u %u\n", dro_tick_count, chip, addr, val);
    }
}

uint8_t xmi_backend_opl_read(uint16_t port) {
    (void)port;
    return 0;
}

void xmi_backend_init(void) {
    OPL3_Reset(&g_opl3, 44100);
}

void xmi_backend_shutdown(void) {
}

int32_t xmi_backend_fill_buffer(int16_t *buf, uint32_t samples) {
    uint32_t i;
    for (i = 0; i < samples; i++) {
        OPL3_GenerateResampled(&g_opl3, buf + i * 2);
    }
    return 0;
}

opl3_chip *xmi_backend_get_chip(void) {
    return &g_opl3;
}
