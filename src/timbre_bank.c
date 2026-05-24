#include "timbre_bank.h"
#include "xmidi32_utils.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

extern const unsigned char g_fat_opl_data[];
extern unsigned int g_fat_opl_len;

static const unsigned char *s_bank_data = NULL;
static unsigned int s_bank_len = 0;
static int s_dynamic = 0;

static const unsigned char *bank_data(void) {
    return s_bank_data ? s_bank_data : g_fat_opl_data;
}

static unsigned int bank_len(void) {
    return s_bank_data ? s_bank_len : g_fat_opl_len;
}

const unsigned char *timbre_bank_find(unsigned char bank, unsigned char patch) {
    const unsigned char *data = bank_data();
    unsigned int len = bank_len();
    unsigned int pos = 0;

    while (pos + 6 <= len) {
        unsigned char p = data[pos];
        unsigned char b = data[pos + 1];
        unsigned int offset = read_le_32(data + pos + 2);

        if (b == 0xFF) break;

        if (b == bank && p == patch) {
            if (offset > 0 && offset + 2 <= len) {
                return data + offset;
            }
        }
        pos += 6;
    }
    return NULL;
}

/* ----------------------------------------------------------- */
/* BNK1 format constants (from Miles Design GLIB.C)            */
/* ----------------------------------------------------------- */
#define BNK1_HDR_SZ     20
#define BNK1_NSEC_SZ    12
#define BNK1_INST_SZ    30

/* Convert BNK1 instrument (30 bytes at src) -> 11-byte AD_instrument */
static void bnk1_to_ad(const uint8_t *src, uint8_t *ad) {
    /* BNK1 instrument offsets */
#define OFF_SOUND_MODE       0
#define OFF_VOICE_NUM        1
#define OFF_MOD_KEY_SCL      2
#define OFF_MOD_FREQ_MULT    3
#define OFF_MOD_FEEDBACK     4
#define OFF_MOD_ATTACK       5
#define OFF_MOD_SUST_LVL     6
#define OFF_MOD_SUST_SND     7
#define OFF_MOD_DECAY        8
#define OFF_MOD_RELEASE      9
#define OFF_MOD_OUT_LVL     10
#define OFF_MOD_AMP_VIB     11
#define OFF_MOD_PITCH_VIB   12
#define OFF_MOD_ENV_SCL     13
#define OFF_MOD_CONN        14
#define OFF_CAR_KEY_SCL     15
#define OFF_CAR_FREQ_MULT   16
#define OFF_CAR_FEEDBACK    17
#define OFF_CAR_ATTACK      18
#define OFF_CAR_SUST_LVL    19
#define OFF_CAR_SUST_SND    20
#define OFF_CAR_DECAY       21
#define OFF_CAR_RELEASE     22
#define OFF_CAR_OUT_LVL     23
#define OFF_CAR_AMP_VIB     24
#define OFF_CAR_PITCH_VIB   25
#define OFF_CAR_ENV_SCL     26
#define OFF_CAR_CONN        27
#define OFF_MOD_WAVEFORM    28
#define OFF_CAR_WAVEFORM    29

    /* ad[0]=mod_avekm, [1]=mod_ksl_tl, [2]=mod_ad, [3]=mod_sr,
       [4]=mod_ws, [5]=fb_c, [6]=car_avekm, [7]=car_ksl_tl,
       [8]=car_ad, [9]=car_sr, [10]=car_ws */

    ad[6] = (uint8_t)((src[OFF_CAR_FREQ_MULT] & 0x0f) |
             (src[OFF_CAR_ENV_SCL] ? 0x10 : 0x00) |
             (src[OFF_CAR_SUST_SND] ? 0x20 : 0x00) |
             (src[OFF_CAR_AMP_VIB] ? 0x40 : 0x00) |
             (src[OFF_CAR_PITCH_VIB] ? 0x80 : 0x00));

    ad[7] = (uint8_t)((src[OFF_CAR_OUT_LVL] & 0x3f) |
             (src[OFF_CAR_KEY_SCL] << 6));

    ad[8] = (uint8_t)((src[OFF_CAR_DECAY] & 0x0f) |
             ((src[OFF_CAR_ATTACK] & 0x0f) << 4));

    ad[9] = (uint8_t)((src[OFF_CAR_RELEASE] & 0x0f) |
             ((src[OFF_CAR_SUST_LVL] & 0x0f) << 4));

    ad[10] = src[OFF_CAR_WAVEFORM];

    ad[0] = (uint8_t)((src[OFF_MOD_FREQ_MULT] & 0x0f) |
             (src[OFF_MOD_ENV_SCL] ? 0x10 : 0x00) |
             (src[OFF_MOD_SUST_SND] ? 0x20 : 0x00) |
             (src[OFF_MOD_AMP_VIB] ? 0x40 : 0x00) |
             (src[OFF_MOD_PITCH_VIB] ? 0x80 : 0x00));

    ad[1] = (uint8_t)((src[OFF_MOD_OUT_LVL] & 0x3f) |
             (src[OFF_MOD_KEY_SCL] << 6));

    ad[2] = (uint8_t)((src[OFF_MOD_DECAY] & 0x0f) |
             ((src[OFF_MOD_ATTACK] & 0x0f) << 4));

    ad[3] = (uint8_t)((src[OFF_MOD_RELEASE] & 0x0f) |
             ((src[OFF_MOD_SUST_LVL] & 0x0f) << 4));

    ad[4] = src[OFF_MOD_WAVEFORM];

    ad[5] = (uint8_t)(((src[OFF_MOD_FEEDBACK] & 7) << 1) |
             ((src[OFF_MOD_CONN] & 1) ^ 1));

#undef OFF_SOUND_MODE
#undef OFF_VOICE_NUM
#undef OFF_MOD_KEY_SCL
#undef OFF_MOD_FREQ_MULT
#undef OFF_MOD_FEEDBACK
#undef OFF_MOD_ATTACK
#undef OFF_MOD_SUST_LVL
#undef OFF_MOD_SUST_SND
#undef OFF_MOD_DECAY
#undef OFF_MOD_RELEASE
#undef OFF_MOD_OUT_LVL
#undef OFF_MOD_AMP_VIB
#undef OFF_MOD_PITCH_VIB
#undef OFF_MOD_ENV_SCL
#undef OFF_MOD_CONN
#undef OFF_CAR_KEY_SCL
#undef OFF_CAR_FREQ_MULT
#undef OFF_CAR_FEEDBACK
#undef OFF_CAR_ATTACK
#undef OFF_CAR_SUST_LVL
#undef OFF_CAR_SUST_SND
#undef OFF_CAR_DECAY
#undef OFF_CAR_RELEASE
#undef OFF_CAR_OUT_LVL
#undef OFF_CAR_AMP_VIB
#undef OFF_CAR_PITCH_VIB
#undef OFF_CAR_ENV_SCL
#undef OFF_CAR_CONN
#undef OFF_MOD_WAVEFORM
#undef OFF_CAR_WAVEFORM
}

/* ----------------------------------------------------------- */
static int is_bnk1(const uint8_t *data, unsigned int len) {
    if (len < BNK1_HDR_SZ) return 0;
    return (memcmp(data + 2, "ADLIB-", 6) == 0);
}

static int load_bnk(const char *path) {
    FILE *f = fopen(path, "rb");
    if (!f) { fprintf(stderr, "Can't open %s\n", path); return -1; }

    fseek(f, 0, SEEK_END);
    long flen = ftell(f);
    fseek(f, 0, SEEK_SET);

    uint8_t *fdata = (uint8_t *)malloc((size_t)flen);
    if (!fdata) { fclose(f); return -1; }
    if (fread(fdata, 1, (size_t)flen, f) != (size_t)flen) {
        free(fdata); fclose(f); return -1;
    }
    fclose(f);

    if (!is_bnk1(fdata, (unsigned int)flen)) {
        fprintf(stderr, "%s: not a BNK1 file (missing ADLIB- signature)\n", path);
        free(fdata);
        return -1;
    }

    unsigned int entries = read_le_16(fdata + 10);
    unsigned int name_list = read_le_32(fdata + 12);
    unsigned int inst_data = read_le_32(fdata + 16);

    if (entries > 256) {
        fprintf(stderr, "%s: too many entries (%u)\n", path, entries);
        free(fdata);
        return -1;
    }

    /* Count how many used melodic entries */
    unsigned int used_cnt = 0;
    unsigned int i;
    for (i = 0; i < entries; i++) {
        unsigned int noff = name_list + i * BNK1_NSEC_SZ;
        if (noff + BNK1_NSEC_SZ > (unsigned int)flen) continue;
        uint8_t used = fdata[noff + 2];
        if (!used) continue;
        unsigned int idx = read_le_16(fdata + noff);
        unsigned int ioff = inst_data + idx * BNK1_INST_SZ;
        if (ioff + BNK1_INST_SZ > (unsigned int)flen) continue;
        if (fdata[ioff + 1] == 0) used_cnt++;
    }

    if (used_cnt == 0) {
        fprintf(stderr, "%s: no usable melodic instruments found\n", path);
        free(fdata);
        return -1;
    }

    /* Build GTL buffer:
       header: used_cnt * 6 bytes GTL_hdr + 2 byte terminator
       body:   used_cnt * 16 bytes (2 LE16 length + 14 data) */
    unsigned int hdr_size = used_cnt * 6;
    unsigned int body_size = used_cnt * 16;
    unsigned int total = hdr_size + 2 + body_size;

    uint8_t *gtl = (uint8_t *)malloc(total);
    if (!gtl) { free(fdata); return -1; }

    unsigned int out_pos = 0;
    unsigned int patch_num = 0;

    for (i = 0; i < entries; i++) {
        unsigned int noff = name_list + i * BNK1_NSEC_SZ;
        if (noff + BNK1_NSEC_SZ > (unsigned int)flen) continue;
        if (!fdata[noff + 2]) continue;
        unsigned int idx = read_le_16(fdata + noff);
        unsigned int ioff = inst_data + idx * BNK1_INST_SZ;
        if (ioff + BNK1_INST_SZ > (unsigned int)flen) continue;
        if (fdata[ioff + 1] != 0) continue;

        uint32_t body_off = hdr_size + 2 + patch_num * 16;
        gtl[out_pos++] = (uint8_t)patch_num;
        gtl[out_pos++] = 0;
        gtl[out_pos++] = (uint8_t)(body_off);
        gtl[out_pos++] = (uint8_t)(body_off >> 8);
        gtl[out_pos++] = (uint8_t)(body_off >> 16);
        gtl[out_pos++] = (uint8_t)(body_off >> 24);

        patch_num++;
    }

    /* Terminator */
    gtl[out_pos++] = 0xFF;
    gtl[out_pos++] = 0xFF;

    /* Body data */
    patch_num = 0;
    for (i = 0; i < entries; i++) {
        unsigned int noff = name_list + i * BNK1_NSEC_SZ;
        if (noff + BNK1_NSEC_SZ > (unsigned int)flen) continue;
        if (!fdata[noff + 2]) continue;
        unsigned int idx = read_le_16(fdata + noff);
        unsigned int ioff = inst_data + idx * BNK1_INST_SZ;
        if (ioff + BNK1_INST_SZ > (unsigned int)flen) continue;
        if (fdata[ioff + 1] != 0) continue;

        gtl[out_pos++] = 0x0E;
        gtl[out_pos++] = 0x00;
        gtl[out_pos++] = 0x00;

        bnk1_to_ad(fdata + ioff, gtl + out_pos);
        out_pos += 11;

        gtl[out_pos++] = 0x00;
        gtl[out_pos++] = 0x00;

        patch_num++;
    }

    free(fdata);

    timbre_bank_free();
    s_bank_data = gtl;
    s_bank_len = total;
    s_dynamic = 1;

    fprintf(stderr, "Loaded %u instruments from %s\n", used_cnt, path);
    return 0;
}

static int load_opl(const char *path) {
    FILE *f = fopen(path, "rb");
    if (!f) { fprintf(stderr, "Can't open %s\n", path); return -1; }

    fseek(f, 0, SEEK_END);
    long flen = ftell(f);
    fseek(f, 0, SEEK_SET);

    uint8_t *data = (uint8_t *)malloc((size_t)flen);
    if (!data) { fclose(f); return -1; }
    if (fread(data, 1, (size_t)flen, f) != (size_t)flen) {
        free(data); fclose(f); return -1;
    }
    fclose(f);

    /* Quick sanity: must have at least one header entry + terminator + body */
    if ((unsigned int)flen < 8) {
        free(data);
        fprintf(stderr, "%s: file too small\n", path);
        return -1;
    }

    timbre_bank_free();
    s_bank_data = data;
    s_bank_len = (unsigned int)flen;
    s_dynamic = 1;

    fprintf(stderr, "Loaded %s\n", path);
    return 0;
}

/* ----------------------------------------------------------- */
int timbre_bank_load(const char *path) {
    const char *dot = strrchr(path, '.');
    if (!dot) {
        fprintf(stderr, "No extension on %s (expected .opl, .ad, or .bnk)\n", path);
        return -1;
    }

    /* Case-insensitive extension check */
    char ext[8];
    unsigned int i;
    for (i = 0; i < 7 && dot[i + 1]; i++) {
        char c = dot[i + 1];
        ext[i] = (c >= 'A' && c <= 'Z') ? (c + 0x20) : c;
    }
    ext[i] = '\0';

    if (strcmp(ext, "opl") == 0 || strcmp(ext, "ad") == 0)
        return load_opl(path);
    else if (strcmp(ext, "bnk") == 0)
        return load_bnk(path);
    else {
        fprintf(stderr, "Unrecognized bank file extension: .%s (expected .opl, .ad, or .bnk)\n", ext);
        return -1;
    }
}

void timbre_bank_free(void) {
    if (s_dynamic && s_bank_data) {
        free((void *)s_bank_data);
    }
    s_bank_data = NULL;
    s_bank_len = 0;
    s_dynamic = 0;
}
