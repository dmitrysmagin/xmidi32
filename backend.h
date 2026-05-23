#ifndef XMI_BACKEND_H
#define XMI_BACKEND_H

#include <stdint.h>
#include "src/xmidi32_config.h"

#define XMI_EMULATION 1

#include "opl3.h"

void xmi_backend_opl_write(uint16_t reg, uint8_t val);
uint8_t xmi_backend_opl_read(uint16_t port);

void xmi_backend_init(void);
void xmi_backend_shutdown(void);
int32_t xmi_backend_fill_buffer(int16_t *buf, uint32_t samples);
opl3_chip *xmi_backend_get_chip(void);

/* DRO capture */
void dro_start(const char *path);
void dro_stop(void);
void dro_tick(void);

#endif
