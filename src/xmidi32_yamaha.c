#include <stdio.h>
#include "xmidi32_driver.h"
#include "xmidi32_backend.h"
#include "xmidi32_yamaha.h"
#include "xmidi32_yamaha_tables.h"
#include "xmidi32_utils.h"

#if SYNTH_TYPE == YM3812 || SYNTH_TYPE == YMF262

#ifdef XMI_EMULATION
void xmi_backend_opl_write(uint16_t reg, uint8_t val);
#endif

#define MAX_TIMBS     192
#define DEF_TC_SIZE   3584
#define NUM_CHANS_MAX 16

#define DEF_PITCH_RANGE 12
#define DEF_AV_DEPTH   0xC0

#define U_ALL_REGS   0xF9
#define U_AVEKM      0x80
#define U_KSLTL      0x40
#define U_ADSR       0x20
#define U_WS         0x10
#define U_FBC        0x08
#define U_FREQ       0x01

#define SLOT_FREE    0
#define SLOT_KEYON   1
#define SLOT_KEYOFF  2

#define BNK_INST     0
#define TV_INST      1
#define TV_EFFECT    2
#define OPL3_INST    3

#if SYNTH_TYPE == YMF262
#define NUM_4OP_VOICES  6
#define NUM_VOICES_MAX  18
#else
#define NUM_VOICES_MAX  9
#endif

#define NUM_SLOTS_MAX  20

#define MAX_REC_CHAN_MAX  10
#define MIN_TRUE_CHAN_MAX 2

#if SYNTH_TYPE == YMF262
#define R_PAN_THRESH  27
#define L_PAN_THRESH  100
#define LEFT_MASK     0xEF
#define RIGHT_MASK    0xDF
#endif

#ifndef VEL_SENS
#define VEL_SENS  1
#endif

#ifndef VEL_TRUE
#define VEL_TRUE  0
#endif

#pragma pack(push, 1)
struct BNK_timbre  {
    uint16_t B_length;
    int8_t   B_transpose;
    uint8_t  B_mod_AVEKM;
    uint8_t  B_mod_KSLTL;
    uint8_t  B_mod_AD;
    uint8_t  B_mod_SR;
    uint8_t  B_mod_WS;
    uint8_t  B_fb_c;
    uint8_t  B_car_AVEKM;
    uint8_t  B_car_KSLTL;
    uint8_t  B_car_AD;
    uint8_t  B_car_SR;
    uint8_t  B_car_WS;
} __attribute__((packed));

struct OPL3BNK_timbre {
    struct BNK_timbre base;
    uint8_t  O_mod_AVEKM;
    uint8_t  O_mod_KSLTL;
    uint8_t  O_mod_AD;
    uint8_t  O_mod_SR;
    uint8_t  O_mod_WS;
    uint8_t  O_fb_c;
    uint8_t  O_car_AVEKM;
    uint8_t  O_car_KSLTL;
    uint8_t  O_car_AD;
    uint8_t  O_car_SR;
    uint8_t  O_car_WS;
} __attribute__((packed));
#pragma pack(pop)

static void release_voice(int32_t slot);
static void update_voice(int32_t slot);
static uint32_t index_timbre(uint16_t gnum);
static void delete_LRU(void);

static uint32_t DATA_OUT;
static uint32_t ADDR_STAT;
static uint32_t note_event_ctr;
static uint32_t timb_hist[MAX_TIMBS];
static uint32_t timb_offsets[MAX_TIMBS];
static uint8_t  timb_bank[MAX_TIMBS];
static uint8_t  timb_num[MAX_TIMBS];
static uint8_t  timb_attribs[MAX_TIMBS];
static uint8_t *cache_base;
static uint32_t cache_size;
static uint32_t cache_end;
static uint16_t TV_accum;
static uint16_t pri_accum;
static uint8_t  vol_update;
static int32_t  rover_2op;
#if SYNTH_TYPE == YMF262
static int32_t  rover_4op;
#endif
uint8_t conn_shadow;

static uint8_t  S_timbre_off_l[NUM_SLOTS_MAX];
static uint8_t  S_timbre_off_h[NUM_SLOTS_MAX];
static uint16_t S_duration[NUM_SLOTS_MAX];
static uint8_t  S_status[NUM_SLOTS_MAX];
static uint8_t  S_type[NUM_SLOTS_MAX];
static int8_t   S_voice[NUM_SLOTS_MAX];
static uint8_t  S_channel[NUM_SLOTS_MAX];
static uint8_t  S_note[NUM_SLOTS_MAX];
static uint8_t  S_keynum[NUM_SLOTS_MAX];
static int8_t   S_transpose[NUM_SLOTS_MAX];
static uint8_t  S_velocity[NUM_SLOTS_MAX];
static uint8_t  S_sustain[NUM_SLOTS_MAX];
static uint8_t  S_update[NUM_SLOTS_MAX];

static uint8_t  S_KBF_shadow[NUM_SLOTS_MAX];
static uint8_t  S_BLOCK[NUM_SLOTS_MAX];
static uint8_t  S_FBC[NUM_SLOTS_MAX];
static uint8_t  S_KSLTL_0[NUM_SLOTS_MAX];
static uint8_t  S_KSLTL_1[NUM_SLOTS_MAX];
static uint8_t  S_AVEKM_0[NUM_SLOTS_MAX];
static uint8_t  S_AVEKM_1[NUM_SLOTS_MAX];
static uint8_t  S_AD_0[NUM_SLOTS_MAX];
static uint8_t  S_AD_1[NUM_SLOTS_MAX];
static uint8_t  S_SR_0[NUM_SLOTS_MAX];
static uint8_t  S_SR_1[NUM_SLOTS_MAX];
static uint8_t  S_scale_01[NUM_SLOTS_MAX];

static uint16_t S_ws_val[NUM_SLOTS_MAX];
static uint16_t S_m1_val[NUM_SLOTS_MAX];
static uint16_t S_m0_val[NUM_SLOTS_MAX];
static uint16_t S_fb_val[NUM_SLOTS_MAX];
static uint16_t S_p_val[NUM_SLOTS_MAX];
static uint16_t S_v1_val[NUM_SLOTS_MAX];
static uint16_t S_v0_val[NUM_SLOTS_MAX];

#if SYNTH_TYPE == YMF262
static uint8_t  S_KSLTL_2[NUM_SLOTS_MAX];
static uint8_t  S_KSLTL_3[NUM_SLOTS_MAX];
static uint8_t  S_AVEKM_2[NUM_SLOTS_MAX];
static uint8_t  S_AVEKM_3[NUM_SLOTS_MAX];
static uint8_t  S_AD_2[NUM_SLOTS_MAX];
static uint8_t  S_AD_3[NUM_SLOTS_MAX];
static uint8_t  S_SR_2[NUM_SLOTS_MAX];
static uint8_t  S_SR_3[NUM_SLOTS_MAX];
static uint8_t  S_scale_23[NUM_SLOTS_MAX];
static uint16_t S_ws_val_2[NUM_SLOTS_MAX];
static uint16_t S_m3_val[NUM_SLOTS_MAX];
static uint16_t S_m2_val[NUM_SLOTS_MAX];
static uint16_t S_v3_val[NUM_SLOTS_MAX];
static uint16_t S_v2_val[NUM_SLOTS_MAX];
#endif

static uint8_t  MIDI_vol[NUM_CHANS_MAX];
static uint8_t  MIDI_pan[NUM_CHANS_MAX];
static uint8_t  MIDI_pitch_l[NUM_CHANS_MAX];
static uint8_t  MIDI_pitch_h[NUM_CHANS_MAX];
static uint8_t  MIDI_express[NUM_CHANS_MAX];
static uint8_t  MIDI_mod[NUM_CHANS_MAX];
static uint8_t  MIDI_sus[NUM_CHANS_MAX];
static uint8_t  MIDI_vprot[NUM_CHANS_MAX];
static int8_t   MIDI_timbre[NUM_CHANS_MAX];
static uint8_t  MIDI_bank[NUM_CHANS_MAX];
static int8_t   MIDI_program[NUM_CHANS_MAX];
static int8_t   RBS_timbres[128];
static uint8_t  MIDI_voices[NUM_CHANS_MAX];
static int8_t   V_channel[NUM_VOICES_MAX];
static uint16_t S_V_priority[NUM_SLOTS_MAX];

#if SYNTH_TYPE == YMF262
static const uint8_t alt_op_0[18] = { 6, 7, 8, 0, 1, 2, 0xFF,0xFF,0xFF, 24,25,26,18,19,20,0xFF,0xFF,0xFF };
static const uint8_t alt_op_1[18] = { 9,10,11, 3, 4, 5, 0xFF,0xFF,0xFF, 27,28,29,21,22,23,0xFF,0xFF,0xFF };
static const uint8_t conn_sel[18] = { 1, 2, 4, 1, 2, 4, 0, 0, 0, 8,16,32, 8,16,32, 0, 0, 0 };
static const uint8_t op4_voice[6]  = { 0, 1, 2, 9,10,11 };
#endif

#if SYNTH_TYPE == YMF262
static const uint8_t carrier_01_data[18] = { 0,1,2,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1 };
static const uint8_t carrier_23_data[18] = { 1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1 };
#endif

#ifdef XMI_EMULATION
static inline void outport(uint16_t port, uint8_t val) { (void)port; (void)val; }
static inline uint8_t inport(uint16_t port) { (void)port; return 0; }
#else
static inline void outport(uint16_t port, uint8_t val) {
#if defined(_MSC_VER)
    _outp((unsigned short)port, (int)val);
#elif defined(__GNUC__) || defined(__clang__)
    __asm__ volatile ("outb %0, %1" : : "a"(val), "Nd"((uint16_t)port));
#endif
}

static inline uint8_t inport(uint16_t port) {
#if defined(_MSC_VER)
    return (uint8_t)_inp((unsigned short)port);
#elif defined(__GNUC__) || defined(__clang__)
    uint8_t val;
    __asm__ volatile ("inb %1, %0" : "=a"(val) : "Nd"((uint16_t)port));
    return val;
#endif
}
#endif

static void io_delay(void) {
    volatile int i;
    for (i = 0; i < 6; i++) (void)0;
}

static void update_reg(uint8_t oper, uint8_t base, uint8_t val) {
    uint8_t bh = op_array[oper];
    uint8_t bl = (uint8_t)((op_index[oper] + base) & 0xFF);

#ifdef XMI_EMULATION
    xmi_backend_opl_write(((uint16_t)bh << 8) | bl, val);
#else
    io_delay();
    outport((uint16_t)(ADDR_STAT + bh * 2), bl);
    io_delay();
    outport((uint16_t)(ADDR_STAT + 1 + bh * 2), val);
    io_delay();
#endif
}

static void send_byte(uint8_t voice, uint8_t base, uint8_t val) {
    uint8_t bh = voice_array[voice];
    uint8_t bl = (uint8_t)((voice_num[voice] + base) & 0xFF);

#ifdef XMI_EMULATION
    xmi_backend_opl_write(((uint16_t)bh << 8) | bl, val);
#else
    io_delay();
    outport((uint16_t)(ADDR_STAT + bh * 2), bl);
    io_delay();
    outport((uint16_t)(ADDR_STAT + 1 + bh * 2), val);
    io_delay();
#endif
}

static void detect_send(uint8_t address, uint8_t data) {
    io_delay();
    outport((uint16_t)ADDR_STAT, address);
    io_delay();
    outport((uint16_t)DATA_OUT, data);
    io_delay();
    outport((uint16_t)ADDR_STAT, 4);
    io_delay();
    outport((uint16_t)DATA_OUT, 0x60);
    io_delay();
    outport((uint16_t)ADDR_STAT, 4);
    io_delay();
    outport((uint16_t)DATA_OUT, 0x80);
}

static uint8_t read_status(void) {
    io_delay();
    return inport((uint16_t)ADDR_STAT);
}

#ifdef XMI_EMULATION
static int32_t detect_Adlib(void) { return 1; }
#else
static int32_t detect_Adlib(void) {
    detect_send(4, 0x60);
    detect_send(4, 0x80);
    uint8_t s1 = read_status();
    detect_send(2, 0xFF);
    detect_send(4, 0x21);
    int32_t i;
    for (i = 0; i < 200; i++) {
        if (read_status() != s1) break;
    }
    uint8_t s2 = read_status();
    detect_send(4, 0x60);
    detect_send(4, 0x80);
    s1 &= 0xE0;
    s2 &= 0xE0;
    if (s1 != 0) return 0;
    if (s2 != 0xC0) return 0;
    return 1;
}
#endif

static uint32_t index_timbre(uint16_t gnum) {
    uint8_t num = (uint8_t)(gnum & 0xFF);
    uint8_t bank = (uint8_t)((gnum >> 8) & 0xFF);
    int32_t i;
    for (i = 0; i < (int32_t)MAX_TIMBS; i++) {
        if (!(timb_attribs[i] & 0x80)) continue;
        if (timb_bank[i] != bank) continue;
        if (timb_num[i] == num) return (uint32_t)i;
    }
    return 0xFFFFFFFF;
}

static void delete_LRU(void) {
    int32_t lru_idx = -1;
    uint32_t lru_ctr = 0xFFFFFFFF;
    int32_t i;
    for (i = 0; i < (int32_t)MAX_TIMBS; i++) {
        if (!(timb_attribs[i] & 0x80)) continue;
        if (timb_attribs[i] & 0x40) continue;
        if (timb_hist[i] >= lru_ctr) continue;
        lru_ctr = timb_hist[i];
        lru_idx = i;
    }
    if (lru_idx < 0) return;

    uint32_t toff = timb_offsets[lru_idx];
    uint8_t *timb_ptr = cache_base + toff;
    uint16_t tsize = read_le_16(timb_ptr);

    uint8_t *dst = cache_base + toff;
    uint8_t *src = dst + tsize;
    uint32_t count = (uint32_t)((cache_base + cache_end) - src);
    int32_t j;
    for (j = 0; j < (int32_t)count; j++) dst[j] = src[j];

    timb_attribs[lru_idx] = 0;
    cache_end -= tsize;

    int32_t ch;
    for (ch = 0; ch < (int32_t)NUM_CHANS_MAX; ch++) {
        if (MIDI_timbre[ch] == lru_idx) MIDI_timbre[ch] = -1;
    }
    for (ch = 0; ch < 128; ch++) {
        if (RBS_timbres[ch] == lru_idx) RBS_timbres[ch] = -1;
    }
    for (ch = 0; ch < (int32_t)MAX_TIMBS; ch++) {
        if (!(timb_attribs[ch] & 0x80)) continue;
        if (timb_offsets[ch] <= toff) continue;
        timb_offsets[ch] -= tsize;
    }
    for (ch = 0; ch < (int32_t)NUM_SLOTS_MAX; ch++) {
        if (S_status[ch] == SLOT_FREE) continue;
        uint32_t off = ((uint32_t)S_timbre_off_h[ch] << 8) | S_timbre_off_l[ch];
        if (off <= toff) continue;
        if (off == toff) {
            release_voice(ch);
            S_status[ch] = SLOT_FREE;
        } else {
            off -= tsize;
            S_timbre_off_l[ch] = (uint8_t)(off & 0xFF);
            S_timbre_off_h[ch] = (uint8_t)((off >> 8) & 0xFF);
        }
    }
}

uint32_t yamaha_get_note_event(void) { return note_event_ctr; }
void yamaha_set_note_event(uint32_t ctr) { note_event_ctr = ctr; }

void yamaha_define_cache(void *addr, uint32_t size) {
    cache_base = (uint8_t *)addr;
    cache_size = size;
    cache_end = 0;
}

uint32_t yamaha_get_cache_size(void) { return cache_size; }

static void do_install_timbre(uint16_t gnum, const void *data) {
    uint8_t num = (uint8_t)(gnum & 0xFF);
    uint8_t bank = (uint8_t)((gnum >> 8) & 0xFF);

    uint32_t idx = index_timbre(gnum);
    if (idx != 0xFFFFFFFF) {
        if (data != NULL) {
            int32_t slot = -1;
            int32_t i;
            for (i = 0; i < (int32_t)MAX_TIMBS; i++) {
                if (!(timb_attribs[i] & 0x80)) { slot = i; break; }
            }
            if (slot >= 0) {
                int32_t ch;
                for (ch = 0; ch < (int32_t)NUM_CHANS_MAX; ch++) {
                    if (MIDI_program[ch] == num && MIDI_bank[ch] == bank) {
                        MIDI_timbre[ch] = (int8_t)slot;
                    }
                }
            }
        }
        return;
    }

    if (data == NULL) return;

    const uint8_t *src = (const uint8_t *)data;
    uint16_t tsize = read_le_16(src);

    while ((cache_end + tsize) > cache_size) delete_LRU();

    uint8_t slot = 0xFF;
    int32_t i;
    for (i = 0; i < (int32_t)MAX_TIMBS; i++) {
        if (!(timb_attribs[i] & 0x80)) { slot = (uint8_t)i; break; }
    }
    if (slot == 0xFF) { delete_LRU(); slot = 0; }

    uint32_t off = cache_end;
    uint8_t *dst = cache_base + off;
    uint32_t k;
    for (k = 0; k < tsize; k++) dst[k] = ((const uint8_t *)data)[k];

    timb_num[slot] = num;
    timb_bank[slot] = bank;
    timb_attribs[slot] = 0x80;
    note_event_ctr++;
    timb_hist[slot] = note_event_ctr;
    timb_offsets[slot] = off;
    cache_end += tsize;

    int32_t ch;
    for (ch = 0; ch < (int32_t)NUM_CHANS_MAX; ch++) {
        if (MIDI_program[ch] == num && MIDI_bank[ch] == bank) {
            MIDI_timbre[ch] = (int8_t)slot;
        }
    }
}

void yamaha_install_timbre(uint32_t bank, uint32_t patch, const void *data) {
    uint16_t gnum = (uint16_t)((bank << 8) | patch);
    do_install_timbre(gnum, data);
}

void yamaha_protect_timbre(uint32_t bank, uint32_t patch) {
    uint16_t gnum = (uint16_t)((bank << 8) | patch);
    uint32_t idx = index_timbre(gnum);
    if (idx == 0xFFFFFFFF) return;
    timb_attribs[idx] |= 0x40;
}

void yamaha_unprotect_timbre(uint32_t bank, uint32_t patch) {
    uint16_t gnum = (uint16_t)((bank << 8) | patch);
    uint32_t idx = index_timbre(gnum);
    if (idx == 0xFFFFFFFF) return;
    timb_attribs[idx] &= 0xBF;
}

int32_t yamaha_timbre_status(uint32_t bank, uint32_t patch) {
    uint16_t gnum = (uint16_t)((bank << 8) | patch);
    uint32_t idx = index_timbre(gnum);
    if (idx == 0xFFFFFFFF) return 0;
    return (int32_t)(timb_offsets[idx] + 1);
}

static void release_voice(int32_t slot) {
    if (S_voice[slot] < 0) return;

    S_BLOCK[slot] &= 0xDF;
    S_update[slot] |= U_FREQ;
    update_voice(slot);

    int32_t ch = S_channel[slot];
    MIDI_voices[ch]--;
    int32_t v = S_voice[slot];

#if SYNTH_TYPE == YMF262
    if (S_type[slot] == OPL3_INST) {
        V_channel[v + 3] = -1;
    }
#endif
    V_channel[v] = -1;
    S_voice[slot] = -1;

    if (S_type[slot] == OPL3_INST) {
        S_status[slot] = SLOT_FREE;
    } else if (S_type[slot] == BNK_INST) {
        S_status[slot] = SLOT_FREE;
    }
}

static void update_priority(void);

static void assign_voice(int32_t slot) {
#if SYNTH_TYPE == YMF262
    if (S_type[slot] == OPL3_INST) {
        int32_t dx = -1;
        int32_t di = rover_4op;
        int32_t v;
        for (v = 0; v < NUM_4OP_VOICES; v++) {
            dx++;
            if (dx >= NUM_4OP_VOICES) dx = 0;
            rover_4op = dx;
            int32_t vi = op4_voice[dx];
            if (V_channel[vi] >= 0) continue;
            if (V_channel[vi + 3] >= 0) continue;
            S_voice[slot] = (int8_t)vi;
            int32_t ch = S_channel[slot];
            MIDI_voices[vi]++;
            V_channel[vi]     = (int8_t)ch;
            V_channel[vi + 3] = (int8_t)ch;
            S_update[slot] = U_ALL_REGS;
            update_voice(slot);
            return;
        }
        update_priority();
        return;
    }
#endif

    int32_t dx = -1;
    int32_t v;
    for (v = 0; v < NUM_VOICES_MAX; v++) {
        dx++;
        if (dx >= NUM_VOICES_MAX) dx = 0;
        rover_2op = dx;
        if (V_channel[dx] >= 0) continue;
        S_voice[slot] = (int8_t)dx;
        int32_t ch = S_channel[slot];
        MIDI_voices[ch]++;
        V_channel[dx] = (int8_t)ch;
        S_update[slot] = U_ALL_REGS;
        update_voice(slot);
        return;
    }
    update_priority();
}

static void BNK_phase(int32_t slot);
#if SYNTH_TYPE == YMF262
static void OPL_phase(int32_t slot);
#endif

static void BNK_phase(int32_t slot) {
    uint32_t off = ((uint32_t)S_timbre_off_h[slot] << 8) | S_timbre_off_l[slot];
    uint8_t *tptr = cache_base + off;

    S_BLOCK[slot] = 0x20;
    S_type[slot]  = BNK_INST;
    S_duration[slot] = 0xFFFF;
    S_p_val[slot] = 32767;

    uint8_t fb_c      = tptr[8];
    uint8_t m_ksltl   = tptr[4];
    uint8_t c_ksltl   = tptr[10];
    uint8_t m_avekm   = tptr[3];
    uint8_t c_avekm   = tptr[9];

    S_FBC[slot] = fb_c & 1;
    S_fb_val[slot] = ((uint16_t)(fb_c & 0x0E)) << 3;

    S_KSLTL_0[slot] = m_ksltl & 0xC0;
    S_v0_val[slot] = (uint16_t)(((~m_ksltl) & 0x3F) << 2);

    S_KSLTL_1[slot] = c_ksltl & 0xC0;
    S_v1_val[slot] = (uint16_t)(((~c_ksltl) & 0x3F) << 2);

    S_AVEKM_0[slot] = m_avekm;
    S_m0_val[slot] = ((uint16_t)m_avekm & 0x0F) << 4;

    S_AVEKM_1[slot] = c_avekm;
    S_m1_val[slot] = ((uint16_t)c_avekm & 0x0F) << 4;

    S_AD_0[slot] = tptr[5];
    S_SR_0[slot] = tptr[6];
    S_AD_1[slot] = tptr[11];
    S_SR_1[slot] = tptr[12];

    S_ws_val[slot] = ((uint16_t)tptr[13] << 8) | tptr[7];

    S_scale_01[slot] = (uint8_t)(S_FBC[slot] | 2);
    S_update[slot] = U_ALL_REGS;
}

#if SYNTH_TYPE == YMF262
static void OPL_phase(int32_t slot) {
    BNK_phase(slot);

    uint32_t off = ((uint32_t)S_timbre_off_h[slot] << 8) | S_timbre_off_l[slot];
    uint8_t *tptr = cache_base + off;

    S_type[slot] = OPL3_INST;

    uint8_t fb_c = tptr[8];
    S_FBC[slot] = (S_FBC[slot] & 1) | ((fb_c & 0x80) >> 6);

    uint8_t c01 = carrier_01_data[S_FBC[slot] & 3];
    uint8_t c23 = carrier_23_data[S_FBC[slot] & 3];
    S_scale_01[slot] = c01;
    S_scale_23[slot] = c23;

    uint8_t om_ksltl = tptr[15];
    S_KSLTL_2[slot] = om_ksltl & 0xC0;
    S_v2_val[slot] = (uint16_t)(((~om_ksltl) & 0x3F) << 2);

    uint8_t oc_ksltl = tptr[21];
    S_KSLTL_3[slot] = oc_ksltl & 0xC0;
    S_v3_val[slot] = (uint16_t)(((~oc_ksltl) & 0x3F) << 2);

    uint8_t om_avekm = tptr[14];
    S_AVEKM_2[slot] = om_avekm & 0xF0;
    S_m2_val[slot] = ((uint16_t)om_avekm & 0x0F) << 4;

    uint8_t oc_avekm = tptr[20];
    S_AVEKM_3[slot] = oc_avekm & 0xF0;
    S_m3_val[slot] = ((uint16_t)oc_avekm & 0x0F) << 4;

    S_AD_2[slot] = tptr[16];
    S_SR_2[slot]  = tptr[17];
    S_AD_3[slot]  = tptr[22];
    S_SR_3[slot]  = tptr[23];

    S_ws_val_2[slot] = ((uint16_t)tptr[24] << 8) | tptr[18];
}
#endif

static void update_voice(int32_t slot) {
    if (S_voice[slot] < 0) return;

    uint8_t vol = 64;
    if (S_update[slot] & U_KSLTL) {
        int32_t ch = S_channel[slot] & 0x0F;
        uint32_t v = ((uint32_t)MIDI_vol[ch] * (uint32_t)MIDI_express[ch]) >> 7;
        if (v == 0) v = 1;
        uint32_t vel = S_velocity[slot];
        vol = (uint8_t)((((v * vel) >> 7) == 0) ? 1 : ((v * vel) >> 7));
    }

    int32_t arr = 0;
    if (S_type[slot] == OPL3_INST) arr = 1;

#if SYNTH_TYPE == YMF262
    int32_t vi = S_voice[slot];
    uint8_t conn = conn_sel[vi];
    uint8_t cur_shadow = conn_shadow;
    if (arr) {
        uint8_t ncs = (uint8_t)(cur_shadow | conn);
        if (ncs != cur_shadow) {
            conn_shadow = ncs;
            update_reg(0, 0x04, ncs);
        }
    } else {
        uint8_t ncs = (uint8_t)(cur_shadow & ~conn);
        if (ncs != cur_shadow) {
            conn_shadow = ncs;
            update_reg(0, 0x04, ncs);
        }
    }
#endif

    do {
        if (arr) {
#if SYNTH_TYPE == YMF262
            int32_t vi2 = S_voice[slot] + 3;
            uint8_t vop0 = op_0[vi2];
            uint8_t vop1 = op_1[vi2];
            (void)vop0; (void)vop1;
#endif
        } else {
            (void)S_voice[slot];
        }

        if (S_update[slot] & U_AVEKM) {
            int32_t ch = S_channel[slot] & 0x0F;
            uint8_t am_vib = (MIDI_mod[ch] >= 64) ? 0x40 : 0x00;

            if (!arr) {
                uint8_t m0 = S_m0_val[slot];
                uint8_t reg0 = (uint8_t)(((m0 >> 4) & 0x0F) | am_vib | S_AVEKM_0[slot]);
                update_reg(op_0[S_voice[slot]], 0x20, reg0);

                uint8_t m1 = S_m1_val[slot];
                uint8_t reg1 = (uint8_t)(((m1 >> 4) & 0x0F) | am_vib | S_AVEKM_1[slot]);
                update_reg(op_1[S_voice[slot]], 0x20, reg1);
            } else {
#if SYNTH_TYPE == YMF262
                uint8_t m2 = S_m2_val[slot];
                uint8_t r2 = (uint8_t)(((m2 >> 4) & 0x0F) | am_vib | S_AVEKM_2[slot]);
                update_reg(op_0[S_voice[slot] + 3], 0x20, r2);

                uint8_t m3 = S_m3_val[slot];
                uint8_t r3 = (uint8_t)(((m3 >> 4) & 0x0F) | am_vib | S_AVEKM_3[slot]);
                update_reg(op_1[S_voice[slot] + 3], 0x20, r3);
#endif
            }
            S_update[slot] &= (uint8_t)(~U_AVEKM);
        }

        if (S_update[slot] & U_KSLTL) {
            uint8_t v0_lev = (uint8_t)((S_v0_val[slot] >> 2) & 0x3F);
            if (S_scale_01[slot] & 1) {
                v0_lev = (uint8_t)((((uint32_t)v0_lev * vol) + 126) / 127);
            }
            uint8_t att0 = (uint8_t)(~(v0_lev) & 0x3F);
            update_reg(op_0[S_voice[slot]], 0x40, (uint8_t)(att0 | S_KSLTL_0[slot]));

            uint8_t v1_lev = (uint8_t)((S_v1_val[slot] >> 2) & 0x3F);
            if (S_scale_01[slot] & 2) {
                v1_lev = (uint8_t)((((uint32_t)v1_lev * vol) + 126) / 127);
            }
            uint8_t att1 = (uint8_t)(~(v1_lev) & 0x3F);
            update_reg(op_1[S_voice[slot]], 0x40, (uint8_t)(att1 | S_KSLTL_1[slot]));

            S_update[slot] &= (uint8_t)(~U_KSLTL);
        }

        if (S_update[slot] & U_ADSR) {
            update_reg(op_0[S_voice[slot]], 0x60, S_AD_0[slot]);
            update_reg(op_1[S_voice[slot]], 0x60, S_AD_1[slot]);
            update_reg(op_0[S_voice[slot]], 0x80, S_SR_0[slot]);
            update_reg(op_1[S_voice[slot]], 0x80, S_SR_1[slot]);
            S_update[slot] &= (uint8_t)(~U_ADSR);
        }

        if (S_update[slot] & U_WS) {
            uint8_t ws_lo = (uint8_t)(S_ws_val[slot] & 0xFF);
            uint8_t ws_hi = (uint8_t)(S_ws_val[slot] >> 8);
            update_reg(op_0[S_voice[slot]], 0xE0, ws_lo);
            update_reg(op_1[S_voice[slot]], 0xE0, ws_hi);
            S_update[slot] &= (uint8_t)(~U_WS);
        }

        if (S_update[slot] & U_FBC) {
            int32_t fbc = (uint8_t)(((S_FBC[slot] & 1) | 0x30) & 0xFF);
#if SYNTH_TYPE == YMF262
            uint8_t pan = MIDI_pan[S_channel[slot] & 0x0F];
            if (pan <= R_PAN_THRESH) {
                fbc &= RIGHT_MASK;
            } else if (pan >= L_PAN_THRESH) {
                fbc &= LEFT_MASK;
            }
#endif
            send_byte((uint8_t)S_voice[slot], 0xC0, fbc);
            S_update[slot] &= (uint8_t)(~U_FBC);
        }

        if (S_update[slot] & U_FREQ) {
            if (arr) {
                S_update[slot] &= (uint8_t)(~U_FREQ);
            } else {
                if (!(S_BLOCK[slot] & 0x20)) {
                    uint8_t kb = (uint8_t)(S_KBF_shadow[slot] & 0xDF);
                    send_byte((uint8_t)S_voice[slot], 0xB0, kb);
                } else {
                    int32_t ch = S_channel[slot] & 0x0F;
                    int32_t pb = (((int32_t)MIDI_pitch_h[ch] << 9) | ((int32_t)MIDI_pitch_l[ch] << 2)) - 0x2000;
                    int32_t range = DEF_PITCH_RANGE;
                    pb = (pb * range) >> 5;

                    int32_t note = S_note[slot] + S_transpose[slot] - 24;
                    while (note < 0) note += 12;
                    while (note >= 96) note -= 12;

                    note = (note << 4) + 8;

                    note += pb;
                    note -= 192;
                    while (note < 0) note += 192;
                    while (note >= 1536) note -= 192;

                    int32_t idx = note >> 4;
                    uint8_t htone = note_halftone[idx];
                    int32_t tbl_idx = ((htone << 5) | (note & 0x0F)) >> 1;
                    uint16_t fval = freq_table[tbl_idx];

                    int32_t oct = note_octave[idx];
                    oct--;
                    if (fval >= 0x8000) oct++;
                    if (oct < 0) { oct++; fval >>= 1; }

                    uint8_t blk_fnum = (uint8_t)((((uint8_t)oct << 2) & 0x1C) | ((fval >> 8) & 3));
                    uint8_t fnum_lo  = (uint8_t)(fval & 0xFF);

                    S_KBF_shadow[slot] = (uint8_t)((blk_fnum & 0xDF) | (S_BLOCK[slot] & 0x20));

                    send_byte((uint8_t)S_voice[slot], 0xA0, fnum_lo);
                    send_byte((uint8_t)S_voice[slot], 0xB0, S_KBF_shadow[slot]);
                }
                S_update[slot] &= (uint8_t)(~U_FREQ);
            }
        }
    } while (arr-- > 0);
}

static void update_priority(void) {
    int32_t slot_cnt = 0;
    int32_t si;
    for (si = 0; si < (int32_t)NUM_SLOTS_MAX; si++) {
        if (S_status[si] == SLOT_FREE) continue;
        slot_cnt++;

        int32_t ch = S_channel[si] & 0x0F;
        uint32_t pri = (MIDI_vprot[ch] >= 64) ? 0xFFFF : S_p_val[si];
        pri -= MIDI_voices[ch];
        if ((int32_t)pri < 0) pri = 0;
        S_V_priority[si] = (uint16_t)pri;
    }

    int32_t high_unvoiced = -1;
    int32_t low_voiced = -1;
    int32_t low_voiced_pri = 0xFFFF;
#if SYNTH_TYPE == YMF262
    int32_t low_4op = -1;
    int32_t low_4op_pri = 0xFFFF;
#endif

    for (si = 0; si < (int32_t)NUM_SLOTS_MAX; si++) {
        if (S_status[si] == SLOT_FREE) continue;
        uint32_t pri = S_V_priority[si];
        int32_t vi = S_voice[si];

        if (vi < 0) {
            if ((int32_t)pri > high_unvoiced) {
                high_unvoiced = si;
            }
        } else {
#if SYNTH_TYPE == YMF262
            if (op4_base[vi] && pri < low_4op_pri) {
                low_4op_pri = (int32_t)pri;
                low_4op = si;
            }
#endif
            if ((int32_t)pri < low_voiced_pri) {
                low_voiced_pri = (int32_t)pri;
                low_voiced = si;
            }
        }
    }

    if (high_unvoiced < 0 || low_voiced < 0) return;

    int32_t victim = low_voiced;
#if SYNTH_TYPE == YMF262
    int32_t new_slot = high_unvoiced;
    if (S_type[new_slot] == OPL3_INST) {
        victim = low_4op;
        if (S_type[victim] == OPL3_INST) {
            int8_t alt_v = alt_voice[S_voice[victim]];
            if (alt_v >= 0) {
                int32_t k;
                for (k = 0; k < (int32_t)NUM_SLOTS_MAX; k++) {
                    if (S_status[k] == SLOT_FREE) continue;
                    if (S_voice[k] != alt_v) continue;
                    release_voice(k);
                    break;
                }
            }
        }
    }
#endif

    int32_t old_v = S_voice[victim];
    release_voice(victim);

    int32_t new_vi = high_unvoiced;
    S_voice[new_vi] = (int8_t)old_v;
    int32_t ch = S_channel[new_vi] & 0x0F;
    MIDI_voices[ch]++;
    V_channel[old_v] = (int8_t)ch;
#if SYNTH_TYPE == YMF262
    if (S_type[new_vi] == OPL3_INST) {
        V_channel[old_v + 3] = (int8_t)ch;
    }
#endif
    S_update[new_vi] = U_ALL_REGS;
    update_voice(new_vi);
}

void yamaha_note_on(uint32_t chan, uint32_t note, uint32_t vel) {
    if (chan >= NUM_CHANS_MAX) return;
    if (MIDI_timbre[chan] < 0) return;

    int32_t timb_idx = MIDI_timbre[chan];
    uint32_t off = timb_offsets[timb_idx];
    uint8_t *tptr = cache_base + off;

    note_event_ctr++;
    timb_hist[timb_idx] = note_event_ctr;

    int32_t slot = -1;
    int32_t si;
    for (si = 0; si < (int32_t)NUM_SLOTS_MAX; si++) {
        if (S_status[si] == SLOT_FREE) { slot = si; break; }
    }
    if (slot < 0) return;

    S_channel[slot] = (uint8_t)chan;
    S_keynum[slot]  = (uint8_t)note;

    uint8_t transp = tptr[2];
    if (chan != 9) {
        S_transpose[slot] = (int8_t)transp;
        S_note[slot] = (uint8_t)note;
    } else {
        S_transpose[slot] = 0;
        S_note[slot] = (uint8_t)(note + transp);
    }

#if VEL_SENS
    uint8_t v = (uint8_t)vel;
#if !VEL_TRUE
    v >>= 3;
    if (v < 16) v = vel_graph[v];
#endif
#else
    uint8_t v = 127;
#endif
    S_velocity[slot] = v;

    uint32_t tsize = read_le_16(tptr);
    S_timbre_off_l[slot] = (uint8_t)(off & 0xFF);
    S_timbre_off_h[slot] = (uint8_t)((off >> 8) & 0xFF);

    S_status[slot] = SLOT_KEYON;
    S_sustain[slot] = 0;

    if (tsize == 25) {
#if SYNTH_TYPE == YMF262
        OPL_phase(slot);
#else
        BNK_phase(slot);
#endif
    } else {
        BNK_phase(slot);
    }

    S_voice[slot] = -1;
    assign_voice(slot);
}

void yamaha_note_off(uint32_t chan, uint32_t note) {
    if (chan >= NUM_CHANS_MAX) return;
    int32_t si;
    for (si = 0; si < (int32_t)NUM_SLOTS_MAX; si++) {
        if (S_status[si] != SLOT_KEYON) continue;
        if (S_keynum[si] != note) continue;
        if (S_channel[si] != chan) continue;

        if (MIDI_sus[chan] < 64) {
            if (S_type[si] == OPL3_INST || S_type[si] == BNK_INST) {
                release_voice(si);
                S_status[si] = SLOT_FREE;
            } else {
                S_duration[si] = 1;
            }
        } else {
            S_sustain[si] = 1;
        }
    }
}

void yamaha_controller(uint32_t chan, uint32_t ctrl, uint32_t val) {
    if (chan >= NUM_CHANS_MAX) return;

    switch (ctrl) {
        case 0:
            MIDI_bank[chan] = (uint8_t)val;
            break;
        case 1:
            MIDI_mod[chan] = (uint8_t)val;
            break;
        case 7:
            MIDI_vol[chan] = (uint8_t)val;
            break;
        case 10:
            MIDI_pan[chan] = (uint8_t)val;
            break;
        case 11:
            MIDI_express[chan] = (uint8_t)val;
            break;
        case 64:
            MIDI_sus[chan] = (uint8_t)val;
            if (val >= 64) break;
            {
                int32_t si;
                for (si = 0; si < (int32_t)NUM_SLOTS_MAX; si++) {
                    if (S_status[si] == SLOT_FREE) continue;
                    if (S_channel[si] != chan) continue;
                    if (!S_sustain[si]) continue;
                    S_sustain[si] = 0;
                    yamaha_note_off(chan, S_keynum[si]);
                }
            }
            break;
        case 96:
            break;
        case 97:
            break;
        case 98:
            MIDI_vprot[chan] = (uint8_t)val;
            break;
        default:
            break;
    }

    if (ctrl == 1 || ctrl == 7 || ctrl == 11 || ctrl == 10) {
        uint8_t upd_flag = (ctrl == 1) ? U_AVEKM : (ctrl == 10) ? U_FBC : U_KSLTL;
        int32_t si;
        for (si = 0; si < (int32_t)NUM_SLOTS_MAX; si++) {
            if (S_status[si] == SLOT_FREE) continue;
            if (S_channel[si] != chan) continue;
            S_update[si] |= upd_flag;
            update_voice(si);
        }
    }
    (void)val;
}

void yamaha_program_change(uint32_t chan, uint32_t program) {
    if (chan >= NUM_CHANS_MAX) return;
    MIDI_program[chan] = (int8_t)program;
    uint16_t gnum = (uint16_t)((MIDI_bank[chan] << 8) | (program & 0xFF));
    uint32_t idx = index_timbre(gnum);
    MIDI_timbre[chan] = (int8_t)idx;
}

void yamaha_pitch_bend(uint32_t chan, uint32_t pitch_l, uint32_t pitch_h) {
    if (chan >= NUM_CHANS_MAX) return;
    MIDI_pitch_l[chan] = (uint8_t)pitch_l;
    MIDI_pitch_h[chan] = (uint8_t)pitch_h;
    int32_t si;
    for (si = 0; si < (int32_t)NUM_SLOTS_MAX; si++) {
        if (S_status[si] == SLOT_FREE) continue;
        if (S_channel[si] != chan) continue;
        S_update[si] |= U_FREQ;
        update_voice(si);
    }
}

void send_MIDI_message(uint32_t status, uint32_t d1, uint32_t d2) {
    uint32_t chan = status & 0x0F;
    uint32_t type = status & 0xF0;

    if (type == 0x80 || (type == 0x90 && d2 == 0)) {
        yamaha_note_off(chan, d1);
        return;
    }
    if (type == 0x90) {
        yamaha_note_on(chan, d1, d2);
        return;
    }
    if (type == 0xB0) {
        yamaha_controller(chan, d1, d2);
        return;
    }
    if (type == 0xC0) {
        yamaha_program_change(chan, d1);
        return;
    }
    if (type == 0xE0) {
        yamaha_pitch_bend(chan, d1, d2);
        return;
    }
}

void send_MIDI_sysex(const uint8_t *data, uint32_t size) {
    (void)data;
    (void)size;
}

void set_IO_parms(uint32_t IO, uint32_t IRQ, uint32_t DMA, uint32_t DRQ) {
    (void)IRQ; (void)DMA; (void)DRQ;
    ADDR_STAT = IO;
    DATA_OUT  = IO + 1;
}

uint32_t detect_device(uint32_t IO, uint32_t IRQ, uint32_t DMA, uint32_t DRQ) {
    (void)IRQ; (void)DMA; (void)DRQ;
    uint32_t saved_DATA = DATA_OUT;
    uint32_t saved_ADDR = ADDR_STAT;
    ADDR_STAT = IO;
    DATA_OUT  = IO + 1;

    int32_t res = detect_Adlib();

    DATA_OUT  = saved_DATA;
    ADDR_STAT = saved_ADDR;
    return res ? 1 : 0;
}

void reset_synth(void) {
#if SYNTH_TYPE == YMF262
    update_reg(5, 0x01, 0x01);
    update_reg(4, 0x01, 0x00);
    conn_shadow = 0;
#endif

    uint8_t bx = 1;
    do {
        update_reg(bx, 0x00, array0_init[bx - 1]);
        bx++;
    } while (bx != 0xF5);

#if SYNTH_TYPE == YMF262
    bx = 1;
    do {
        update_reg(bx, 0x01, array1_init[bx - 1]);
        bx++;
    } while (bx != 0xF5);
#endif
}

void init_synth(void) {
    note_event_ctr = 0;

    int32_t i;
    for (i = 0; i < (int32_t)MAX_TIMBS; i++) timb_attribs[i] = 0;
    for (i = 0; i < (int32_t)NUM_CHANS_MAX; i++) {
        MIDI_timbre[i] = -1;
        MIDI_voices[i] = 0;
        MIDI_program[i] = -1;
        MIDI_bank[i] = 0;
        MIDI_vol[i] = 100;
        MIDI_express[i] = 127;
        MIDI_pan[i] = 64;
        MIDI_sus[i] = 0;
        MIDI_pitch_l[i] = 0;
        MIDI_pitch_h[i] = 64;
    }
    for (i = 0; i < (int32_t)NUM_SLOTS_MAX; i++) S_status[i] = SLOT_FREE;
    for (i = 0; i < (int32_t)NUM_VOICES_MAX; i++) V_channel[i] = -1;
    for (i = 0; i < 128; i++) RBS_timbres[i] = -1;

    TV_accum = 0;
    pri_accum = 0;
    vol_update = 0;
    rover_2op = -1;
#if SYNTH_TYPE == YMF262
    rover_4op = -1;
#endif
}

void shutdown_synth(void) {
#if SYNTH_TYPE == YMF262
    update_reg(5, 0x01, 0x00);
#endif
}

void serve_synth(void) {
}

void reset_interface(void) {}
void init_interface(void) {}
void sysex_wait(uint32_t ms) { (void)ms; }

#endif
