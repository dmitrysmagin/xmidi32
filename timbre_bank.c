#include "timbre_bank.h"
#include "src/xmidi32_timbre_internal.h"

typedef struct {
    unsigned char patch;
    unsigned char bank;
    unsigned int offset;
} gtl_entry;

static int parse_gtl(const unsigned char *data, unsigned int len) {
    const unsigned char *p = data;
    unsigned int pos = 0;
    int count = 0;

    while (pos + 6 <= len) {
        unsigned char patch = p[0];
        unsigned char bank = p[1];
        unsigned int offset = (unsigned int)p[2]
                            | ((unsigned int)p[3] << 8)
                            | ((unsigned int)p[4] << 16)
                            | ((unsigned int)p[5] << 24);

        if (bank == 0xFF) {
            break;
        }

        if (offset > 0 && offset + 2 <= len) {
            const unsigned char *bnk = data + offset;
            yamaha_install_timbre(bank, patch, bnk);
        } else if (offset + 2 <= len) {
            yamaha_install_timbre(bank, patch, NULL);
        }

        pos += 6;
        p += 6;
        count++;
    }

    return count;
}

extern const unsigned char g_sample_ad_data[];
extern unsigned int g_sample_ad_len;
extern const unsigned char g_sample_opl_data[];
extern unsigned int g_sample_opl_len;

int timbre_bank_load_ad(void) {
    return parse_gtl(g_sample_ad_data, g_sample_ad_len);
}

int timbre_bank_load_opl(void) {
    return parse_gtl(g_sample_opl_data, g_sample_opl_len);
}

int timbre_bank_parse(const unsigned char *data, unsigned int len) {
    return parse_gtl(data, len);
}
