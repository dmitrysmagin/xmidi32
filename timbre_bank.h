#ifndef TIMBRE_BANK_H
#define TIMBRE_BANK_H

#include <stdint.h>

typedef struct {
    const unsigned char *data;
    unsigned int len;
} bank_data;

int timbre_bank_load_ad(void);
int timbre_bank_load_opl(void);
int timbre_bank_parse(const unsigned char *data, unsigned int len);

#endif
