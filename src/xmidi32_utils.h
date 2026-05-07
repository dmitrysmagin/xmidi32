#ifndef XMIDI32_UTILS_H
#define XMIDI32_UTILS_H

#include "xmidi32_types.h"
#include <stddef.h>

uint32_t read_be32(const uint8_t *p);

uint32_t read_vln(const uint8_t **ptr);

uint32_t vln_size(const uint8_t *start);

uint32_t ul_divide(uint32_t num, uint32_t den);

void advance_count(struct sequence_state *st, int32_t *out_bar, int32_t *out_beat);

#endif
