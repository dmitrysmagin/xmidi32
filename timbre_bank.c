#include "timbre_bank.h"
#include "src/xmidi32_utils.h"

extern const unsigned char g_sample_opl_data[];
extern unsigned int g_sample_opl_len;

const unsigned char *timbre_bank_find(unsigned char bank, unsigned char patch) {
    const unsigned char *data = g_sample_opl_data;
    unsigned int len = g_sample_opl_len;
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
