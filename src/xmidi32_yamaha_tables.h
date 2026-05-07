#ifndef XMIDI32_YAMAHA_TABLES_H
#define XMIDI32_YAMAHA_TABLES_H

#include <stdint.h>

#define DEF_AV_DEPTH 0xC0

extern const uint16_t freq_table[128];
extern const uint8_t note_octave[128];
extern const uint8_t note_halftone[128];
extern const uint8_t array0_init[245];
extern const uint8_t array1_init[245];
extern const uint8_t vel_graph[16];
extern const uint8_t pan_graph[128];
extern const uint8_t op_0[18];
extern const uint8_t op_1[18];
extern const uint8_t op_index[36];
extern const uint8_t op_array[36];
extern const uint8_t voice_num[18];
extern const uint8_t voice_array[18];
extern const uint8_t op4_base[18];
extern const int8_t alt_voice[18];
extern uint8_t conn_shadow;
extern const uint8_t carrier_01[18];
extern const uint8_t carrier_23[18];

#endif
