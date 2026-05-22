#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "src/xmidi32_driver.h"
#include "src/xmidi32_timbre_internal.h"

static uint8_t *load_file(const char *path, uint32_t *size) {
    FILE *f = fopen(path, "rb");
    if (!f) return NULL;
    fseek(f, 0, SEEK_END);
    long len = ftell(f);
    if (len < 0) { fclose(f); return NULL; }
    fseek(f, 0, SEEK_SET);
    uint8_t *buf = (uint8_t *)malloc((size_t)len);
    if (!buf) { fclose(f); return NULL; }
    if (fread(buf, 1, (size_t)len, f) != (size_t)len) {
        free(buf); fclose(f); return NULL;
    }
    fclose(f);
    *size = (uint32_t)len;
    return buf;
}

int main(int argc, char *argv[]) {
    if (argc < 2) {
        fprintf(stderr, "Usage: test_xmidi <file.xmi>\n");
        return 1;
    }
    uint32_t size;
    uint8_t *data = load_file(argv[1], &size);
    if (!data) { fprintf(stderr, "load failed\n"); return 1; }
    fprintf(stderr, "loaded %u bytes\n", size);

    xmidi32_init_globals();
    struct sequence_state st;
    memset(&st, 0, sizeof(st));
    HSEQUENCE h = xmidi32_register_seq(data, 0, &st, NULL);
    if (h == (HSEQUENCE)-1) { fprintf(stderr, "register failed\n"); free(data); return 1; }
    fprintf(stderr, "registered handle=%d\n", (int)h);
    fprintf(stderr, "TIMB=%p RBRN=%p EVNT=%p\n", (void*)st.TIMB, (void*)st.RBRN, (void*)st.EVNT);

    if (st.TIMB) fprintf(stderr, "TIMB tag OK? %c%c%c%c\n", st.TIMB[0],st.TIMB[1],st.TIMB[2],st.TIMB[3]);
    if (st.EVNT) fprintf(stderr, "EVNT tag OK? %c%c%c%c\n", st.EVNT[0],st.EVNT[1],st.EVNT[2],st.EVNT[3]);

    free(data);
    return 0;
}
