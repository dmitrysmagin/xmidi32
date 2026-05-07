#ifndef XMIDI32_DRIVER_H
#define XMIDI32_DRIVER_H

#include "xmidi32_config.h"
#include "xmidi32_types.h"

typedef int32_t HTIMER;
typedef int32_t HDRIVER;
typedef int32_t HSEQUENCE;

struct driver_entry {
    uint32_t service;
    void *address;
};

#define AIL_DESC_DRVR         100
#define AIL_DET_DEV           101
#define AIL_INIT_DRVR         102
#define AIL_SERVE_DRVR        103
#define AIL_SHUTDOWN_DRVR     104
#define AIL_STATE_TAB_SIZE    150
#define AIL_REG_SEQ           151
#define AIL_REL_SEQ_HND       152
#define AIL_T_CACHE_SIZE      153
#define AIL_DEFINE_T_CACHE    154
#define AIL_T_REQ             155
#define AIL_INSTALL_T         156
#define AIL_PROTECT_T         157
#define AIL_UNPROTECT_T       158
#define AIL_T_STATUS          159
#define AIL_START_SEQ         170
#define AIL_STOP_SEQ          171
#define AIL_RESUME_SEQ        173
#define AIL_SEQ_STAT          174
#define AIL_REL_VOL           175
#define AIL_REL_TEMPO         176
#define AIL_SET_REL_VOL       177
#define AIL_SET_REL_TEMPO     178
#define AIL_BEAT_CNT          179
#define AIL_BAR_CNT           180
#define AIL_BRA_INDEX         181
#define AIL_CON_VAL           182
#define AIL_SET_CON_VAL       183
#define AIL_CHAN_NOTES        185
#define AIL_SEND_CV_MSG       186
#define AIL_SEND_SYSEX_MSG    187
#define AIL_WRITE_DISP        188
#define AIL_INSTALL_CB        189
#define AIL_CANCEL_CB         190
#define AIL_LOCK_CHAN         191
#define AIL_MAP_SEQ_CHAN      192
#define AIL_RELEASE_CHAN      193
#define AIL_TRUE_SEQ_CHAN     194

#define NUM_DRIVER_ENTRIES    40

extern struct driver_entry driver_index[NUM_DRIVER_ENTRIES];

extern int32_t sequence_count;
extern int32_t current_handle;
extern uint16_t service_active;
extern int32_t (*trigger_fn)(int32_t, int32_t);

extern struct ctrl_log global_controls;
extern uint8_t global_program[NUM_CHANS];
extern uint8_t global_pitch_l[NUM_CHANS];
extern uint8_t global_pitch_h[NUM_CHANS];
extern uint8_t active_notes[NUM_CHANS];
extern uint8_t lock_status[NUM_CHANS];
extern uint16_t init_OK;

extern struct sequence_state *sequence_states[NSEQS];

extern const uint8_t logged_ctrls[9];
extern const uint8_t ctrl_default[9];
extern const uint8_t prg_default[15];
extern uint8_t ctrl_hash[256];

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
void xmidi32_start_seq(HSEQUENCE sequence);
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

void xmidi32_send_channel_voice_message(uint32_t status, uint32_t data_1, uint32_t data_2);
void xmidi32_send_sysex_message(uint32_t addr_a, uint32_t addr_b, uint32_t addr_c,
                               void *data, uint32_t size, uint32_t delay);
void xmidi32_write_display(const char *string);

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

void xmidi32_XMIDI_control(struct sequence_state *st, uint32_t log_chan,
                          uint32_t ctrl, uint32_t val);
void xmidi32_XMIDI_volume(struct sequence_state *st);
uint32_t xmidi32_XMIDI_note_on(struct sequence_state *st);
uint32_t xmidi32_XMIDI_meta(struct sequence_state *st);
void xmidi32_XMIDI_sysex(const uint8_t *data, uint32_t size, uint32_t type);
void xmidi32_release_seq(HSEQUENCE sequence);

uint32_t xmidi32_get_beat_count(HSEQUENCE sequence);
uint32_t xmidi32_get_bar_count(HSEQUENCE sequence);

void xmidi32_map_seq_channel(HSEQUENCE sequence, uint32_t seq_chan, uint32_t phys_chan);
uint32_t xmidi32_true_seq_channel(HSEQUENCE sequence, uint32_t seq_chan);
void xmidi32_branch_index(HSEQUENCE sequence, uint32_t marker);

void xmidi32_init_driver(HDRIVER h, uint32_t IO_ADDR, uint32_t IRQ,
                          uint32_t DMA, uint32_t DRQ);
void xmidi32_describe_driver(void *desc);

uint8_t *find_seq(uint8_t *XMID, uint32_t seq_num);
void rewind_seq(HSEQUENCE sequence);
void flush_channel_notes(uint32_t chan);
void flush_note_queue(struct sequence_state *st);

#endif
