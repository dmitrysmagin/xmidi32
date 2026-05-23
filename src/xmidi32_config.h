#ifndef XMIDI32_CONFIG_H
#define XMIDI32_CONFIG_H

#include <stdint.h>

#define FALSE 0
#define TRUE  1

#define QUANT_RATE       120
#define QUANT_TIME       8333
#define QUANT_TIME_16    0x0208D5U

#define MAX_NOTES        32
#define FOR_NEST         4
#define NSEQS            8
#define QUANT_ADVANCE    1

#define DEF_PITCH_L      0x00
#define DEF_PITCH_H      0x40

#define BRANCH_EXIT      TRUE

#define NUM_CHANS        16

#define SEQ_STOPPED      0
#define SEQ_PLAYING      1
#define SEQ_DONE         2

#define PART_VOLUME      7
#define MODULATION       1
#define PANPOT           10
#define EXPRESSION       11
#define SUSTAIN          64
#define PATCH_BANK_SEL   114
#define CHAN_LOCK        110
#define CHAN_PROTECT     111
#define VOICE_PROTECT    112
#define INDIRECT_C_PFX   115
#define FOR_LOOP         116
#define NEXT_LOOP        117
#define CLEAR_BEAT_BAR   118
#define CALLBACK_TRIG    119
#define TIMBRE_PROTECT   113
#define RESET_ALL_CTRLS  121
#define ALL_NOTES_OFF    123

#ifndef DEF_SYNTH_VOL
#define DEF_SYNTH_VOL    100
#endif

#ifndef MIN_TRUE_CHAN
#define MIN_TRUE_CHAN    1
#endif

#ifndef MAX_TRUE_CHAN
#define MAX_TRUE_CHAN    16
#endif

#ifndef MAX_REC_CHAN
#define MAX_REC_CHAN     17
#endif

#endif
