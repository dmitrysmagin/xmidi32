#include <string.h>
#include "xmidi32_driver.h"

struct ctrl_log global_controls;
uint8_t global_program[NUM_CHANS];
uint8_t global_pitch_l[NUM_CHANS];
uint8_t global_pitch_h[NUM_CHANS];
uint8_t active_notes[NUM_CHANS];
uint8_t lock_status[NUM_CHANS];
uint16_t init_OK = 0;
uint32_t service_active = 0;
uint32_t trigger_active = 0;
int32_t (*trigger_fn)(int32_t, int32_t) = NULL;

struct sequence_state *sequence_states[NSEQS];
int32_t sequence_count = 0;
int32_t current_handle = 0;

void xmidi32_inc_sequence_count(void) { sequence_count++; }
void xmidi32_dec_sequence_count(void) { sequence_count--; }

const uint8_t logged_ctrls[9] = {
    PART_VOLUME, MODULATION, PANPOT, EXPRESSION, SUSTAIN,
    PATCH_BANK_SEL,
    CHAN_LOCK, CHAN_PROTECT, VOICE_PROTECT
};
const uint8_t ctrl_default[9] = {
    127, 0, 64, 127, 0,
    0,
    0, 0, 0
};
const uint8_t prg_default[15] = {
    68, 48, 95, 78,
    41, 3, 110, 122, (uint8_t)-1,
    0, 0, 0, 0, 0
};

uint8_t ctrl_hash[256];

void xmidi32_init_globals(void) {
    uint32_t i;

    for (i = 0; i < 256; i++) {
        ctrl_hash[i] = 0xFF;
    }

    memset(&global_controls, 0xFF, sizeof(struct ctrl_log));
    memset(global_program, 0xFF, NUM_CHANS);
    memset(global_pitch_l, 0xFF, NUM_CHANS);
    memset(global_pitch_h, 0xFF, NUM_CHANS);

    for (i = 0; i < NSEQS; i++) {
        sequence_states[i] = NULL;
    }
    sequence_count = 0;
    service_active = 0;
    trigger_fn = NULL;

    for (i = 0; i < NUM_CHANS; i++) {
        lock_status[i] = 0;
        active_notes[i] = 0;
        global_program[i] = 0xFF;
        global_pitch_l[i] = 0xFF;
        global_pitch_h[i] = 0xFF;
    }

    for (i = 0; i < 9; i++) {
        uint8_t ctrl = logged_ctrls[i];
        ctrl_hash[ctrl] = (uint8_t)i;
    }
}
