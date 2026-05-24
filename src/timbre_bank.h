#ifndef TIMBRE_BANK_H
#define TIMBRE_BANK_H

#include <stdint.h>

const unsigned char *timbre_bank_find(unsigned char bank, unsigned char patch);
int timbre_bank_load(const char *path);
void timbre_bank_free(void);

#endif
