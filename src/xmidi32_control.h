#ifndef XMIDI32_CONTROL_H
#define XMIDI32_CONTROL_H

#include "xmidi32_types.h"

uint32_t xmidi32_XMIDI_control(struct sequence_state *st, uint32_t log_chan,
                              uint32_t ctrl, uint32_t val);

#endif
