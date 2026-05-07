#include "xmidi32_driver.h"

void xmidi32_XMIDI_volume(struct sequence_state *st) {
    uint32_t i;
    for (i = 0; i < NUM_CHANS; i++) {
        uint8_t pv = st->chan_controls.PV[i];
        if (pv == 0xFF) continue;

        uint32_t vol = ((uint32_t)pv * (uint32_t)st->vol_percent) / 100;
        if (vol > 127) vol = 127;

        global_controls.PV[i] = (uint8_t)vol;

        if ((lock_status[i] & 0x80) != 0) continue;

        uint32_t phys_chan = st->chan_map[i];
        xmidi32_send_controller(phys_chan, PART_VOLUME, vol);
    }
}
