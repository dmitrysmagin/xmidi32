#include "xmidi32_driver.h"
#include "xmidi32_backend.h"

static void send_default_controllers(uint32_t chan) {
    uint32_t i;
    for (i = 0; i < 9; i++) {
        uint8_t ctrl = logged_ctrls[i];
        uint8_t def = ctrl_default[i];
        if (def == 0xFF) continue;
        GCTL(chan, i) = def;
        xmidi32_send_controller(chan, ctrl, def);
    }
}

void xmidi32_init_driver(HDRIVER h, uint32_t IO_ADDR, uint32_t IRQ,
                          uint32_t DMA, uint32_t DRQ) {
    (void)h;

    service_active = 0;
    sequence_count = 0;

    uint32_t gsize = sizeof(struct ctrl_log) + NUM_CHANS * 3;
    uint32_t i;
    for (i = 0; i < gsize; i++) {
        ((uint8_t *)&global_controls)[i] = 0xFF;
    }
    for (i = 0; i < 256; i++) {
        ctrl_hash[i] = 0xFF;
    }
    for (i = 0; i < NSEQS; i++) {
        sequence_states[i] = NULL;
    }
    for (i = 0; i < NUM_CHANS; i++) {
        lock_status[i] = 0;
        active_notes[i] = 0;
    }
    for (i = 0; i < 9; i++) {
        ctrl_hash[logged_ctrls[i]] = (uint8_t)i;
    }

    set_IO_parms(IO_ADDR, IRQ, DMA, DRQ);
    reset_interface();
    init_interface();
    reset_synth();
    init_synth();

    xmidi32_cancel_callback();

    for (i = MIN_TRUE_CHAN - 1; i < (uint32_t)(MAX_REC_CHAN - 1); i++) {
        send_default_controllers(i);
    }

    sysex_wait(10);

    for (i = MIN_TRUE_CHAN - 1; i < (uint32_t)(MAX_REC_CHAN - 1); i++) {
        global_pitch_l[i] = DEF_PITCH_L;
        global_pitch_h[i] = DEF_PITCH_H;
        xmidi32_send_pitch_bend(i, DEF_PITCH_L, DEF_PITCH_H);

        uint8_t idx = (uint8_t)(i - (MIN_TRUE_CHAN - 1));
        uint8_t prg = prg_default[idx];
        if (prg != 0xFF) {
            global_program[i] = prg;
            xmidi32_send_program_change(i, prg);
        }
    }

    sysex_wait(10);

    init_OK = 1;
}

void xmidi32_shutdown(void) {
    if (init_OK == 0) return;

    int32_t i;
    for (i = 0; i < NSEQS; i++) {
        if (sequence_states[i] == NULL) continue;
        xmidi32_stop_seq((HSEQUENCE)i);
        xmidi32_release_seq((HSEQUENCE)i);
    }

    init_OK = 0;
}

void xmidi32_shutdown_driver(HDRIVER h, const char *msg) {
    (void)h;
    if (init_OK == 0) return;

    int32_t i;
    for (i = 0; i < NSEQS; i++) {
        if (sequence_states[i] == NULL) continue;
        xmidi32_stop_seq((HSEQUENCE)i);
        xmidi32_release_seq((HSEQUENCE)i);
    }

    reset_synth();

    if (msg != NULL) {
        xmidi32_write_display(msg);
    }

    reset_interface();
    shutdown_synth();

    init_OK = 0;
}
