#ifndef XMIDI32_TIMBRE_INTERNAL_H
#define XMIDI32_TIMBRE_INTERNAL_H

#include <stdint.h>

void yamaha_install_timbre(uint32_t bank, uint32_t patch, const void *data);
void yamaha_protect_timbre(uint32_t bank, uint32_t patch);
void yamaha_unprotect_timbre(uint32_t bank, uint32_t patch);
int32_t yamaha_timbre_status(uint32_t bank, uint32_t patch);
void yamaha_define_cache(void *addr, uint32_t size);
uint32_t yamaha_get_cache_size(void);
void yamaha_set_note_event(uint32_t ctr);
uint32_t yamaha_get_note_event(void);

#endif
