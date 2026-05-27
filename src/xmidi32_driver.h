#ifndef XMIDI32_DRIVER_H
#define XMIDI32_DRIVER_H

#include "xmidi32_config.h"
#include "xmidi32_types.h"
#include "xmidi32_backend.h"

typedef int32_t HTIMER;
typedef int32_t HDRIVER;
typedef int32_t HSEQUENCE;



extern int32_t sequence_count;
extern int32_t current_handle;
extern uint32_t service_active;
extern int32_t (*trigger_fn)(int32_t, int32_t);


extern struct ctrl_log global_controls;
extern uint8_t global_program[NUM_CHANS];
extern uint8_t global_pitch_l[NUM_CHANS];
extern uint8_t global_pitch_h[NUM_CHANS];
extern uint32_t active_notes[NUM_CHANS];
extern uint8_t lock_status[NUM_CHANS];
extern uint16_t init_OK;
extern uint32_t trigger_active;

extern struct sequence_state *sequence_states[NSEQS];

extern const uint8_t logged_ctrls[9];
extern const uint8_t ctrl_default[9];
extern const uint8_t prg_default[15];
extern uint8_t ctrl_hash[256];

#define GCTL(ch, idx) (((uint8_t *)&global_controls)[(idx) + (ch)])

#define CTRL_LOG(st, ctrl_idx) (((uint8_t *)&(st)->chan_controls)[(ctrl_idx)])

void xmidi32_inc_sequence_count(void);
void xmidi32_dec_sequence_count(void);

void xmidi32_init_globals(void);
void xmidi32_shutdown(void);
uint32_t xmidi32_get_state_size(void);

void xmidi32_install_callback(void *fn);
void xmidi32_cancel_callback(void);

HSEQUENCE xmidi32_register_seq(
    uint8_t *XMID,
    uint32_t sequence_num,
    struct sequence_state *state,
    uint8_t *ctrl
);

void xmidi32_release_seq(HSEQUENCE sequence);
HSEQUENCE xmidi32_start_seq(HSEQUENCE sequence);
void xmidi32_stop_seq(HSEQUENCE sequence);
void xmidi32_resume_seq(HSEQUENCE sequence);
uint16_t xmidi32_get_seq_status(HSEQUENCE sequence);

int32_t xmidi32_get_rel_tempo(HSEQUENCE sequence);
int32_t xmidi32_get_rel_volume(HSEQUENCE sequence);
void xmidi32_set_rel_tempo(HSEQUENCE sequence, int32_t tempo, int32_t grad);
void xmidi32_set_rel_volume(HSEQUENCE sequence, int32_t volume, int32_t grad);

int32_t xmidi32_get_control_val(HSEQUENCE sequence, uint32_t chan, uint32_t control);
void xmidi32_set_control_val(HSEQUENCE sequence, uint32_t chan, uint32_t control, uint32_t val);
uint32_t xmidi32_get_chan_notes(HSEQUENCE sequence, uint32_t chan);

void xmidi32_map_seq_channel(HSEQUENCE sequence, uint32_t seq_chan, uint32_t phys_chan);
uint32_t xmidi32_true_seq_channel(HSEQUENCE sequence, uint32_t seq_chan);
void xmidi32_branch_index(HSEQUENCE sequence, uint32_t marker);

uint32_t xmidi32_get_beat_count(HSEQUENCE sequence);
uint32_t xmidi32_get_bar_count(HSEQUENCE sequence);



uint32_t xmidi32_lock_channel(void);
void xmidi32_release_channel(uint32_t chan);

void xmidi32_serve_driver(void);

void xmidi32_send_note_off(uint32_t phys_chan, uint32_t note, uint32_t vel);
void xmidi32_send_note_on(uint32_t phys_chan, uint32_t note, uint32_t vel);
void xmidi32_send_controller(uint32_t phys_chan, uint32_t ctrl, uint32_t val);
void xmidi32_send_program_change(uint32_t phys_chan, uint32_t program);
void xmidi32_send_pitch_bend(uint32_t phys_chan, uint32_t pitch_l, uint32_t pitch_h);
void xmidi32_send_raw_message(uint32_t status, uint32_t data_1, uint32_t data_2);
void xmidi32_send_sysex(const uint8_t *data, uint32_t size);

uint32_t xmidi32_XMIDI_control(struct sequence_state *st, uint32_t log_chan,
                              uint32_t ctrl, uint32_t val);
void xmidi32_XMIDI_volume(struct sequence_state *st);
uint32_t xmidi32_XMIDI_note_on(struct sequence_state *st);
uint32_t xmidi32_XMIDI_meta(struct sequence_state *st);
uint32_t xmidi32_XMIDI_sysex(struct sequence_state *st);

uint32_t xmidi32_get_timbre_cache_size(HDRIVER h);
void xmidi32_define_timbre_cache(HDRIVER h, void *addr, uint32_t size);
uint32_t xmidi32_timbre_request(HDRIVER h, HSEQUENCE seq);
void xmidi32_install_timbre(HDRIVER h, uint32_t bank, uint32_t patch, const void *data);
void xmidi32_protect_timbre(HDRIVER h, uint32_t bank, uint32_t patch);
void xmidi32_unprotect_timbre(HDRIVER h, uint32_t bank, uint32_t patch);
uint32_t xmidi32_timbre_status(HDRIVER h, int32_t bank, int32_t patch);
uint32_t xmidi32_detect_device(HDRIVER h, uint32_t IO, uint32_t IRQ, uint32_t DMA, uint32_t DRQ);

void xmidi32_init_driver(HDRIVER h, uint32_t IO_ADDR, uint32_t IRQ,
                         uint32_t DMA, uint32_t DRQ);
void xmidi32_shutdown_driver(HDRIVER h, const char *msg);

uint8_t *find_seq(uint8_t *XMID, uint32_t seq_num);
void rewind_seq(HSEQUENCE sequence);
void flush_channel_notes(uint32_t chan);
void flush_note_queue(struct sequence_state *st);

#endif
