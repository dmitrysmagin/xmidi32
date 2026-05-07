#include "xmidi32_driver.h"

static void *dispatch(uint32_t service) {
    switch (service) {
    case AIL_DESC_DRVR:       return (void *)xmidi32_describe_driver;
    case AIL_DET_DEV:        return (void *)xmidi32_detect_device;
    case AIL_INIT_DRVR:      return (void *)xmidi32_init_driver;
    case AIL_SERVE_DRVR:     return (void *)xmidi32_serve_driver;
    case AIL_SHUTDOWN_DRVR:  return (void *)xmidi32_shutdown_driver;
    case AIL_STATE_TAB_SIZE: return (void *)xmidi32_get_state_size;
    case AIL_INSTALL_CB:     return (void *)xmidi32_install_callback;
    case AIL_CANCEL_CB:      return (void *)xmidi32_cancel_callback;
    case AIL_REG_SEQ:        return (void *)xmidi32_register_seq;
    case AIL_REL_SEQ_HND:    return (void *)xmidi32_release_seq;
    case AIL_START_SEQ:      return (void *)xmidi32_start_seq;
    case AIL_STOP_SEQ:       return (void *)xmidi32_stop_seq;
    case AIL_RESUME_SEQ:     return (void *)xmidi32_resume_seq;
    case AIL_SEQ_STAT:       return (void *)xmidi32_get_seq_status;
    case AIL_REL_VOL:        return (void *)xmidi32_get_rel_volume;
    case AIL_SET_REL_VOL:    return (void *)xmidi32_set_rel_volume;
    case AIL_REL_TEMPO:      return (void *)xmidi32_get_rel_tempo;
    case AIL_SET_REL_TEMPO:  return (void *)xmidi32_set_rel_tempo;
    case AIL_CON_VAL:        return (void *)xmidi32_get_control_val;
    case AIL_SET_CON_VAL:    return (void *)xmidi32_set_control_val;
    case AIL_CHAN_NOTES:     return (void *)xmidi32_get_chan_notes;
    case AIL_MAP_SEQ_CHAN:   return (void *)xmidi32_map_seq_channel;
    case AIL_TRUE_SEQ_CHAN:  return (void *)xmidi32_true_seq_channel;
    case AIL_BEAT_CNT:       return (void *)xmidi32_get_beat_count;
    case AIL_BAR_CNT:        return (void *)xmidi32_get_bar_count;
    case AIL_BRA_INDEX:      return (void *)xmidi32_branch_index;
    case AIL_SEND_CV_MSG:    return (void *)xmidi32_send_channel_voice_message;
    case AIL_SEND_SYSEX_MSG: return (void *)xmidi32_send_sysex_message;
    case AIL_WRITE_DISP:     return (void *)xmidi32_write_display;
    case AIL_LOCK_CHAN:      return (void *)xmidi32_lock_channel;
    case AIL_RELEASE_CHAN:   return (void *)xmidi32_release_channel;
    case AIL_T_CACHE_SIZE:   return (void *)xmidi32_get_timbre_cache_size;
    case AIL_DEFINE_T_CACHE:  return (void *)xmidi32_define_timbre_cache;
    case AIL_T_REQ:          return (void *)xmidi32_timbre_request;
    case AIL_INSTALL_T:       return (void *)xmidi32_install_timbre;
    case AIL_PROTECT_T:      return (void *)xmidi32_protect_timbre;
    case AIL_UNPROTECT_T:    return (void *)xmidi32_unprotect_timbre;
    case AIL_T_STATUS:       return (void *)xmidi32_timbre_status;
    default:                 return NULL;
    }
}

void *AIL_API_per_service(void *driver, uint32_t service) {
    (void)driver;
    return dispatch(service);
}

struct driver_entry driver_index[NUM_DRIVER_ENTRIES] = {
    { AIL_DESC_DRVR,       xmidi32_describe_driver },
    { AIL_DET_DEV,         xmidi32_detect_device },
    { AIL_INIT_DRVR,       xmidi32_init_driver },
    { AIL_SERVE_DRVR,      xmidi32_serve_driver },
    { AIL_SHUTDOWN_DRVR,    xmidi32_shutdown_driver },
    { AIL_STATE_TAB_SIZE,  xmidi32_get_state_size },
    { AIL_INSTALL_CB,      xmidi32_install_callback },
    { AIL_CANCEL_CB,       xmidi32_cancel_callback },
    { AIL_REG_SEQ,         xmidi32_register_seq },
    { AIL_REL_SEQ_HND,     xmidi32_release_seq },
    { AIL_START_SEQ,       xmidi32_start_seq },
    { AIL_STOP_SEQ,        xmidi32_stop_seq },
    { AIL_RESUME_SEQ,      xmidi32_resume_seq },
    { AIL_SEQ_STAT,        xmidi32_get_seq_status },
    { AIL_REL_VOL,         xmidi32_get_rel_volume },
    { AIL_SET_REL_VOL,     xmidi32_set_rel_volume },
    { AIL_REL_TEMPO,       xmidi32_get_rel_tempo },
    { AIL_SET_REL_TEMPO,   xmidi32_set_rel_tempo },
    { AIL_CON_VAL,         xmidi32_get_control_val },
    { AIL_SET_CON_VAL,     xmidi32_set_control_val },
    { AIL_CHAN_NOTES,      xmidi32_get_chan_notes },
    { AIL_MAP_SEQ_CHAN,    xmidi32_map_seq_channel },
    { AIL_TRUE_SEQ_CHAN,   xmidi32_true_seq_channel },
    { AIL_BEAT_CNT,        xmidi32_get_beat_count },
    { AIL_BAR_CNT,         xmidi32_get_bar_count },
    { AIL_BRA_INDEX,       xmidi32_branch_index },
    { AIL_SEND_CV_MSG,     xmidi32_send_channel_voice_message },
    { AIL_SEND_SYSEX_MSG,  xmidi32_send_sysex_message },
    { AIL_WRITE_DISP,      xmidi32_write_display },
    { AIL_LOCK_CHAN,       xmidi32_lock_channel },
    { AIL_RELEASE_CHAN,    xmidi32_release_channel },
    { AIL_T_CACHE_SIZE,     xmidi32_get_timbre_cache_size },
    { AIL_DEFINE_T_CACHE,   xmidi32_define_timbre_cache },
    { AIL_T_REQ,           xmidi32_timbre_request },
    { AIL_INSTALL_T,        xmidi32_install_timbre },
    { AIL_PROTECT_T,       xmidi32_protect_timbre },
    { AIL_UNPROTECT_T,     xmidi32_unprotect_timbre },
    { AIL_T_STATUS,        xmidi32_timbre_status },
    { 0xFFFFFFFFU,          NULL }
};

void xmidi32_shutdown_driver(HDRIVER h, const char *msg) {
    (void)h;
    if (init_OK == 0) return;

    int32_t i;
    for (i = 0; i < NSEQS; i++) {
        if (sequence_states[i] == NULL) continue;
        xmidi32_stop_seq((HSEQUENCE)i);
        xmidi32_release_seq((HSEQUENCE)i);
    }

    if (msg != NULL) {
        xmidi32_write_display(msg);
    }

    init_OK = 0;
}

void xmidi32_send_channel_voice_message(uint32_t status, uint32_t data_1, uint32_t data_2) {
    xmidi32_send_raw_message(status, data_1, data_2);
}

void xmidi32_send_sysex_message(uint32_t a, uint32_t b, uint32_t c,
                                 void *data, uint32_t size, uint32_t delay_ms) {
    (void)a;
    (void)b;
    (void)c;
    (void)delay_ms;
    xmidi32_send_sysex((const uint8_t *)data, size);
}

void xmidi32_write_display(const char *s) {
    (void)s;
}

#pragma pack(push, 1)
struct xm32_DDT {
    uint32_t min_API_version;
    uint32_t drvr_type;
    char     data_suffix[4];
    void    *dev_name_table;
    int32_t  default_IO;
    int32_t  default_IRQ;
    int32_t  default_DMA;
    int32_t  default_DRQ;
    int32_t  service_rate;
    uint32_t display_size;
};
#pragma pack(pop)

static const char *device_names[] = {
    "Yamaha OPL2/OPL3 FM Sound",
    NULL
};

void xmidi32_describe_driver(void *desc) {
    struct xm32_DDT *d = (struct xm32_DDT *)desc;
    d->min_API_version = 200;
    d->drvr_type = 3;
    d->data_suffix[0] = 'A';
    d->data_suffix[1] = 'D';
    d->data_suffix[2] = 0;
    d->data_suffix[3] = 0;
    d->dev_name_table = (void*)device_names;
    d->default_IO = 0x388;
    d->default_IRQ = -1;
    d->default_DMA = -1;
    d->default_DRQ = -1;
    d->service_rate = QUANT_RATE;
    d->display_size = 0;
}


