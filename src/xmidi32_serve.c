#include "xmidi32_driver.h"
#include "xmidi32_reset.h"

void xmidi32_serve_driver(void) {
    if (service_active != 0) return;
    service_active = 1;

    current_handle = -4;

    if (sequence_count == 0) goto done_synth;

    HSEQUENCE seq = 0;
    for (seq = 0; seq < NSEQS; seq++) {
        current_handle += 4;

        struct sequence_state *st = sequence_states[seq];
        if (st == NULL) continue;
        if (st->status != SEQ_PLAYING) continue;

        int32_t tempo_err = st->tempo_error + st->tempo_percent;
        st->tempo_error = tempo_err;
        if (tempo_err >= 100) {
            st->tempo_error = tempo_err - 100;
            if (st->note_count == 0) {
                if (st->interval_cnt > 0) {
                    st->interval_cnt--;
                    goto check_beat;
                }
            } else {
                int note_slot;
                for (note_slot = 0; note_slot < MAX_NOTES; note_slot++) {
                    if (st->note_queue[note_slot].chan == 0xFF) continue;
                    st->note_queue[note_slot].time--;
                    if (st->note_queue[note_slot].time < 0) {
                        uint8_t chan = st->note_queue[note_slot].chan;
                        uint8_t note = st->note_queue[note_slot].num;
                        st->note_queue[note_slot].chan = 0xFF;
                        if (st->chan_map[chan & 0x0F] < NUM_CHANS) {
                            active_notes[st->chan_map[chan & 0x0F]]--;
                        }
                        xmidi32_send_note_off(st->chan_map[chan & 0x0F], note, 0);
                        st->note_count--;
                    }
                }
                if (st->note_count == 0) {
                    if (st->interval_cnt > 0) {
                        st->interval_cnt--;
                        goto check_beat;
                    }
                }
            }
        }

        {
            uint32_t status = st->EVNT_ptr[0];
            uint32_t phys_chan;

            if (status < 128) {
                st->interval_cnt = (uint16_t)status;
                st->EVNT_ptr++;
                goto check_beat;
            }

            uint32_t ctrl = status & 0xF0;
            uint32_t log_chan = status & 0x0F;
            uint8_t data1 = st->EVNT_ptr[1];
            uint8_t data2 = st->EVNT_ptr[2];

            if (ctrl == 0xF0) {
                if (log_chan == 0x0F) {
                    uint32_t meta_len = xmidi32_XMIDI_meta(st);
                    st->EVNT_ptr += meta_len;
                    if (st->status != SEQ_PLAYING) continue;
                } else {
                    uint32_t sysex_len = xmidi32_XMIDI_sysex(st);
                    st->EVNT_ptr += sysex_len;
                }
                goto end_event;
            }

            if (ctrl == 0xE0) {
                st->chan_pitch_l[log_chan] = data1;
                st->chan_pitch_h[log_chan] = data2;
                global_pitch_l[log_chan] = data1;
                global_pitch_h[log_chan] = data2;
                phys_chan = st->chan_map[log_chan];
                if ((lock_status[log_chan] & 0x80) != 0) {
                    st->EVNT_ptr += 3;
                    goto end_event;
                }
                xmidi32_send_raw_message(0xE0 | phys_chan, data1, data2);
                st->EVNT_ptr += 3;
                goto end_event;
            }

            if (ctrl == 0xD0) {
                phys_chan = st->chan_map[log_chan];
                if ((lock_status[log_chan] & 0x80) != 0) {
                    st->EVNT_ptr += 2;
                    goto end_event;
                }
                xmidi32_send_raw_message(0xD0 | phys_chan, data1, 0);
                st->EVNT_ptr += 2;
                goto end_event;
            }

            if (ctrl == 0xC0) {
                st->chan_program[log_chan] = data1;
                global_program[log_chan] = data1;
                phys_chan = st->chan_map[log_chan];
                if ((lock_status[log_chan] & 0x80) != 0) {
                    st->EVNT_ptr += 2;
                    goto end_event;
                }
                xmidi32_send_program_change(phys_chan, data1);
                st->EVNT_ptr += 2;
                goto end_event;
            }

            if (ctrl == 0xB0) {
                uint32_t ev_size = xmidi32_XMIDI_control(st, log_chan, data1, data2);
                st->EVNT_ptr += ev_size;
                if (st->status != SEQ_PLAYING) continue;
                goto end_event;
            }

            if (ctrl == 0xA0) {
                uint32_t ev_size = xmidi32_XMIDI_note_on(st);
                st->EVNT_ptr += ev_size;
                if (st->status != SEQ_PLAYING) continue;
                goto end_event;
            }

            if (ctrl == 0x90) {
                uint32_t ev_size = xmidi32_XMIDI_note_on(st);
                st->EVNT_ptr += ev_size;
                if (st->status != SEQ_PLAYING) continue;
            }

end_event:
            if (st->status != SEQ_PLAYING) continue;
            continue;
        }

check_beat:
        {
            int32_t beat_frac = (int32_t)st->beat_fraction + (int32_t)st->time_fraction;
            if (beat_frac >= (int32_t)st->time_per_beat) {
                st->beat_fraction = beat_frac - (int32_t)st->time_per_beat;
                st->beat_count++;
                if (st->beat_count >= st->time_numerator) {
                    st->beat_count = 0;
                    st->measure_count++;
                }
            } else {
                st->beat_fraction = beat_frac;
            }
        }

        if (st->tempo_percent != st->tempo_target) {
            int32_t tempo_grad = st->tempo_accum + (QUANT_TIME / 100);
            int32_t steps = 0;
            int32_t tempo_rem = tempo_grad;
            while (tempo_rem > 0) {
                steps++;
                tempo_rem -= st->tempo_period;
            }
            if (steps != 0) {
                st->tempo_accum = tempo_grad;
                int32_t new_tempo = st->tempo_percent;
                if (new_tempo < st->tempo_target) {
                    new_tempo += steps;
                    if (new_tempo > st->tempo_target) new_tempo = st->tempo_target;
                } else {
                    new_tempo -= steps;
                    if (new_tempo < st->tempo_target) new_tempo = st->tempo_target;
                }
                st->tempo_percent = new_tempo;
            }
        }

        if (st->vol_percent != st->vol_target) {
            int32_t vol_grad = st->vol_accum + (QUANT_TIME / 100);
            int32_t steps = 0;
            int32_t vol_rem = vol_grad;
            while (vol_rem > 0) {
                steps++;
                vol_rem -= st->vol_period;
            }
            if (steps != 0) {
                st->vol_accum = vol_grad;
                int32_t new_vol = st->vol_percent;
                if (new_vol < st->vol_target) {
                    new_vol += steps;
                    if (new_vol > st->vol_target) new_vol = st->vol_target;
                } else {
                    new_vol -= steps;
                    if (new_vol < st->vol_target) new_vol = st->vol_target;
                }
                st->vol_percent = new_vol;
                xmidi32_XMIDI_volume(st);
            }
        }
    }

done_synth:
    service_active = 0;
}
