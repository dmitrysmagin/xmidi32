# xmidi32 Project Analysis

## Overview

This is a port of the **Miles Design, Inc. XMIDI32** library — an IBM Audio Interface Library (AIL) extended MIDI sound driver for 32-bit DOS (Rational Systems DOS/4G, Flashtek X32). The original code was written in 80386 assembly (MASM) by John Miles and John Lemberger (1992-1993). The `src/` folder contains a port of that assembly code to portable C.

## Project Structure

### Original ASM/INC Files (in `ail32_V105/`)

All original MASM source files have been moved into the `ail32_V105/` subdirectory:

| File | Description |
|---|---|
| `XMIDI32.ASM` | Main XMIDI interpreter driver (~2179 lines) — sequence playback, event dispatching, controller handling, note queue, tempo/volume ramping, channel locking |
| `AIL32.INC` | AIL assembly equate definitions — MIDI/XMIDI controller numbers, API function indices, status constants |
| `AIL32.H` | C API function prototypes for the AIL interface |
| `YAMAHA32.INC` | Yamaha OPL2/OPL3 synthesizer driver (~2800 lines) — register I/O, voice management, timbre cache, note/pitch/controller handling |
| `MT3232.INC` | Roland MT-32-compatible synthesizer driver (not yet ported) |
| `SPKR32.INC` | Internal PC/Tandy speaker driver (not yet ported) |
| `MPU40132.INC` | MPU-401 UART MIDI interface (not yet ported) |
| `SBMIDI32.INC` | Sound Blaster MIDI interface (not yet ported) |
| `386.MAC` | DOS extender macros |
| `AIL32.ASM`, `DMASND32.ASM` | Additional AIL drivers |
| `MIX32.C`, `STP32.C`, `VP32.C`, `XP32.C`, `DLLLOAD.C` | C support files |

### Ported C Files (`src/`)

#### Core XMIDI Interpreter (ported from `XMIDI32.ASM`)

| File | Corresponding ASM | Description |
|---|---|---|
| `xmidi32_api.c` | `get_state_size`, `install_callback`, `cancel_callback`, `start_seq`, `stop_seq`, `resume_seq`, `release_seq`, `get_seq_status` | Top-level API entry points |
| `xmidi32_serve.c` | `serve_driver` | Main periodic service routine: interval timing, note queue expiration, event dispatching, tempo/volume gradient |
| `xmidi32_control.c` | `XMIDI_control` | Process XMIDI controller change messages (volume, sustain, lock, protect, for/next loops, callback trigger, branch, indirect) |
| `xmidi32_note_on.c` | `XMIDI_note_on` | Note-on with VLN (variable-length) duration encoding, note queue insertion |
| `xmidi32_meta.c` | `XMIDI_meta` | Meta-event interpreter (end-of-track, time signature, set tempo) |
| `xmidi32_sysex.c` | `XMIDI_sysex` | System Exclusive event dispatcher |
| `xmidi32_volume.c` | `XMIDI_volume` | Scaled volume update across all channels |
| `xmidi32_find_seq.c` | `find_seq` | IFF parser to locate FORM XMID chunks in CAT/FORM files |
| `xmidi32_flush.c` | `flush_note_queue`, `flush_channel_notes` | Note-off flushing for sequences and channels |
| `xmidi32_reset.c` | `reset_sequence`, `restore_sequence` | Sequence resource cleanup and state restoration |
| `xmidi32_register.c` | `register_seq`, `rewind_seq` | Sequence registration (handle allocation, IFF chunk parsing) and state initialization |
| `xmidi32_channel.c` | `lock_channel`, `release_channel` | Physical channel locking/unlocking with LRU selection |
| `xmidi32_count.c` | `get_beat_count`, `get_bar_count` (via `advance_count`) | Beat/bar counting with quantized advance |
| `xmidi32_tempo.c` | `get_rel_tempo`, `get_rel_volume`, `set_rel_tempo`, `set_rel_volume`, `get_control_val`, `set_control_val`, `get_chan_notes`, `map_seq_channel`, `true_seq_channel`, `branch_index` | Tempo/volume queries, controller I/O, channel mapping, marker branching |
| `xmidi32_init.c` | `init_driver`, `shutdown_driver` | Driver init: hardware reset, controller defaults, ctrl_hash table, pitch/program init |
| `xmidi32_midi.c` | — (wrappers) | MIDI message send wrappers (note on/off, controller, program change, pitch bend, raw, sysex) |
| `xmidi32_utils.c` | `ul_divide`, `advance_count`, plus helpers | Utility functions (big-endian read, VLN decode, advance_count) |

#### Dispatcher / Driver Index

| File | Corresponding ASM | Description |
|---|---|---|
| `xmidi32_dispatch.c` | `driver_index` table in `XMIDI32.ASM` | Dispatch table mapping service numbers to function pointers; `describe_driver` DDT |

#### Global Variables

| File | Corresponding ASM | Description |
|---|---|---|
| `xmidi32_globals.c` | `global_controls`, `active_notes`, `lock_status`, `ctrl_hash`, `logged_ctrls`, etc. | Global state: controller shadows, sequence state pointers, counters, lookup tables |

#### Header Files

| File | Source | Description |
|---|---|---|
| `xmidi32.h` | — | Top-level include |
| `xmidi32_config.h` | `AIL32.INC` + `XMIDI32.ASM` config equates | Configuration: QUANT_RATE, MAX_NOTES, FOR_NEST, NSEQS, controller numbers, channel limits |
| `xmidi32_types.h` | `state_table` STRUC, `ctrl_log` STRUC | `struct ctrl_log`, `struct note_entry`, `struct sequence_state` |
| `xmidi32_driver.h` | `AIL32.H` + `AIL32.INC` API indices | Driver entry struct, AIL service number defines, all function declarations, global externs |
| `xmidi32_backend.h` | `YAMAHA32.INC` + hardware interface | Backend abstraction: `send_MIDI_message`, `send_MIDI_sysex`, reset/init/serve synth, I/O params |
| `xmidi32_critical.h` | — (new) | Platform-portable atomic operations (CAS, add, exchange, 16-bit ops, critical section enter/leave) |
| `xmidi32_control.h` | — | `xmidi32_XMIDI_control` declaration |
| `xmidi32_reset.h` | — | `reset_sequence`, `restore_sequence` declarations |
| `xmidi32_utils.h` | — | Utility function declarations |

#### Yamaha Synthesizer Driver (ported from `YAMAHA32.INC`)

| File | Description |
|---|---|
| `xmidi32_yamaha.c` (~1145 lines) | Main driver: register I/O, `yamaha_note_on/off`, `yamaha_controller`, `yamaha_program_change`, `yamaha_pitch_bend`, voice management (S_slot arrays), timbre cache management (index, install, delete_LRU, protect/unprotect, status, request), synth init/reset/detect, BNK/OPL3 timbre structs |
| `xmidi32_yamaha.h` | Yamaha function declarations |
| `xmidi32_yamaha_tables.c` | Data tables: `freq_table`, `note_octave`, `note_halftone`, `array0_init`, `array1_init`, `vel_graph`, `pan_graph`, `op_index`, `op_array`, `voice_num`, `voice_array`, `op4_base`, `alt_voice`, `conn_sel`, `carrier_01/23` |
| `xmidi32_yamaha_tables.h` | Table declarations/externs |
| `xmidi32_yamaha_stub.c` | Stub for non-Yamaha synth types |

#### Timbre Cache (split between `YAMAHA32.INC` and driver layer)

| File | Description |
|---|---|
| `xmidi32_timbre.c` | High-level timbre cache API: `xmidi32_get_timbre_cache_size`, `define_timbre_cache`, `timbre_request`, `install_timbre`, `protect_timbre`, `unprotect_timbre`, `timbre_status`, `detect_device` |
| `xmidi32_timbre_internal.h` | Internal declarations bridging timbre API to yamaha layer |

## Architecture Diagram

```
xmidi32.h
  ├── xmidi32_config.h (equates, controller #s)
  ├── xmidi32_types.h  (ctrl_log, sequence_state structs)
  ├── xmidi32_driver.h (function decls, API indices, globals)
  │   ├── xmidi32_backend.h  (hardware abstraction)
  │   └── xmidi32_critical.h (atomic ops)
  │
  ├── xmidi32_utils.c/h   (read_be32, read_vln, ul_divide, advance_count)
  ├── xmidi32_globals.c   (global state variables)
  ├── xmidi32_dispatch.c  (driver_index table, DDT, describe_driver)
  │
  ├── xmidi32_api.c       (start/stop/resume/release/get_status/install_callback)
  ├── xmidi32_init.c      (init_driver, shutdown_driver)
  ├── xmidi32_serve.c     (serve_driver — main service loop)
  ├── xmidi32_register.c  (register_seq, rewind_seq)
  ├── xmidi32_find_seq.c  (find_seq — IFF parser)
  ├── xmidi32_flush.c     (flush_note_queue, flush_channel_notes)
  ├── xmidi32_reset.c/h   (reset_sequence, restore_sequence)
  ├── xmidi32_control.c/h (XMIDI_control)
  ├── xmidi32_note_on.c   (XMIDI_note_on)
  ├── xmidi32_meta.c      (XMIDI_meta)
  ├── xmidi32_sysex.c     (XMIDI_sysex)
  ├── xmidi32_volume.c    (XMIDI_volume)
  ├── xmidi32_channel.c   (lock_channel, release_channel)
  ├── xmidi32_count.c     (get_beat_count, get_bar_count)
  ├── xmidi32_tempo.c     (get/set rel tempo/volume, controller I/O, channel mapping, branch_index)
  ├── xmidi32_midi.c      (MIDI send wrappers)
  │
  ├── xmidi32_timbre.c       (timbre cache API)
  ├── xmidi32_timbre_internal.h
  ├── xmidi32_yamaha.c       (Yamaha OPL2/OPL3 driver)
  ├── xmidi32_yamaha.h
  ├── xmidi32_yamaha_tables.c (data tables)
  ├── xmidi32_yamaha_tables.h
  └── xmidi32_yamaha_stub.c  (stub for non-Yamaha backends)
```

## Verification Results

### Step 1: ASM → C Mapping (Complete)

Every `PROC` in the original ASM files is mapped to a C function in `src/`. Details are in the tables above. Every struct (`state_table`, `ctrl_log`, `BNK`, `OPL3BNK`), every constant (`AIL32.INC` equates, controller numbers, API indices), and every global variable has a C equivalent.

### Step 2: Public Entry Points Verified

The `driver_index` dispatch table in `XMIDI32.ASM` lists 33 service entries (AIL_DESC_DRVR through AIL_T_STATUS). All 33 have C counterparts in `xmidi32_dispatch.c`:

| AIL Service | ASM Entry | C Function | File |
|---|---|---|---|
| 100 (AIL_DESC_DRVR) | `describe_driver` | `xmidi32_describe_driver` | `xmidi32_dispatch.c` |
| 101 (AIL_DET_DEV) | `detect_device` | `xmidi32_detect_device` | `xmidi32_timbre.c` |
| 102 (AIL_INIT_DRVR) | `init_driver` | `xmidi32_init_driver` | `xmidi32_init.c` |
| 103 (AIL_SERVE_DRVR) | `serve_driver` | `xmidi32_serve_driver` | `xmidi32_serve.c` |
| 104 (AIL_SHUTDOWN_DRVR) | `shutdown_driver` | `xmidi32_shutdown_driver` | `xmidi32_dispatch.c` |
| 150 (AIL_STATE_TAB_SIZE) | `get_state_size` | `xmidi32_get_state_size` | `xmidi32_api.c` |
| 151 (AIL_REG_SEQ) | `register_seq` | `xmidi32_register_seq` | `xmidi32_register.c` |
| 152 (AIL_REL_SEQ_HND) | `release_seq` | `xmidi32_release_seq` | `xmidi32_api.c` |
| 153 (AIL_T_CACHE_SIZE) | `get_cache_size` | `xmidi32_get_timbre_cache_size` | `xmidi32_timbre.c` |
| 154 (AIL_DEFINE_T_CACHE) | `define_cache` | `xmidi32_define_timbre_cache` | `xmidi32_timbre.c` |
| 155 (AIL_T_REQ) | `get_request` | `xmidi32_timbre_request` | `xmidi32_timbre.c` |
| 156 (AIL_INSTALL_T) | `install_timbre` | `xmidi32_install_timbre` | `xmidi32_timbre.c` |
| 157 (AIL_PROTECT_T) | `protect_timbre` | `xmidi32_protect_timbre` | `xmidi32_timbre.c` |
| 158 (AIL_UNPROTECT_T) | `unprotect_timbre` | `xmidi32_unprotect_timbre` | `xmidi32_timbre.c` |
| 159 (AIL_T_STATUS) | `timbre_status` | `xmidi32_timbre_status` | `xmidi32_timbre.c` |
| 170 (AIL_START_SEQ) | `start_seq` | `xmidi32_start_seq` | `xmidi32_api.c` |
| 171 (AIL_STOP_SEQ) | `stop_seq` | `xmidi32_stop_seq` | `xmidi32_api.c` |
| 173 (AIL_RESUME_SEQ) | `resume_seq` | `xmidi32_resume_seq` | `xmidi32_api.c` |
| 174 (AIL_SEQ_STAT) | `get_seq_status` | `xmidi32_get_seq_status` | `xmidi32_api.c` |
| 175 (AIL_REL_VOL) | `get_rel_volume` | `xmidi32_get_rel_volume` | `xmidi32_tempo.c` |
| 176 (AIL_REL_TEMPO) | `get_rel_tempo` | `xmidi32_get_rel_tempo` | `xmidi32_tempo.c` |
| 177 (AIL_SET_REL_VOL) | `set_rel_volume` | `xmidi32_set_rel_volume` | `xmidi32_tempo.c` |
| 178 (AIL_SET_REL_TEMPO) | `set_rel_tempo` | `xmidi32_set_rel_tempo` | `xmidi32_tempo.c` |
| 179 (AIL_BEAT_CNT) | `get_beat_count` | `xmidi32_get_beat_count` | `xmidi32_count.c` |
| 180 (AIL_BAR_CNT) | `get_bar_count` | `xmidi32_get_bar_count` | `xmidi32_count.c` |
| 181 (AIL_BRA_INDEX) | `branch_index` | `xmidi32_branch_index` | `xmidi32_tempo.c` |
| 182 (AIL_CON_VAL) | `get_control_val` | `xmidi32_get_control_val` | `xmidi32_tempo.c` |
| 183 (AIL_SET_CON_VAL) | `set_control_val` | `xmidi32_set_control_val` | `xmidi32_tempo.c` |
| 185 (AIL_CHAN_NOTES) | `get_chan_notes` | `xmidi32_get_chan_notes` | `xmidi32_tempo.c` |
| 186 (AIL_SEND_CV_MSG) | `send_cv_msg` | `xmidi32_send_channel_voice_message` | `xmidi32_dispatch.c` |
| 187 (AIL_SEND_SYSEX_MSG) | `send_sysex_msg` | `xmidi32_send_sysex_message` | `xmidi32_dispatch.c` |
| 188 (AIL_WRITE_DISP) | `write_display` | `xmidi32_write_display` | `xmidi32_dispatch.c` |
| 189 (AIL_INSTALL_CB) | `install_callback` | `xmidi32_install_callback` | `xmidi32_api.c` |
| 190 (AIL_CANCEL_CB) | `cancel_callback` | `xmidi32_cancel_callback` | `xmidi32_api.c` |
| 191 (AIL_LOCK_CHAN) | `lock_channel` | `xmidi32_lock_channel` | `xmidi32_channel.c` |
| 192 (AIL_MAP_SEQ_CHAN) | `map_seq_channel` | `xmidi32_map_seq_channel` | `xmidi32_tempo.c` |
| 193 (AIL_RELEASE_CHAN) | `release_channel` | `xmidi32_release_channel` | `xmidi32_channel.c` |
| 194 (AIL_TRUE_SEQ_CHAN) | `true_seq_channel` | `xmidi32_true_seq_channel` | `xmidi32_tempo.c` |

### Step 3: Missing Functionality Still Only in ASM

The core XMIDI interpreter (`XMIDI32.ASM`) and Yamaha OPL2/OPL3 driver (`YAMAHA32.INC`) are fully ported to C. The following sub-drivers from the original ASM project remain **unported** (still only in ASM):

| File | Lines | Description | C Equivalent |
|---|---|---|---|
| `MT3232.INC` | 955 | Roland MT-32/LAPC-1 synthesizer driver | None |
| `SPKR32.INC` | 1084 | IBM PC/Tandy 3-voice internal speaker driver | None |
| `MPU40132.INC` | 151 | Roland MPU-401 UART MIDI interface | None |
| `SBMIDI32.INC` | 187 | Sound Blaster direct MIDI interface | None |

These are conditionally included in `XMIDI32.ASM` via `IFDEF MT32`, `IFDEF SPKR`, etc. and define their own `set_IO_parms`, `detect_device`, `send_byte`, `read_status`, `detect_send`, `detect_Adlib` and other hardware-specific entry points. The Yamaha driver's `xmidi32_yamaha_stub.c` provides a compile-time placeholder for non-Yamaha backends.

## Key Porting Decisions

1. **Atomic operations** (`xmidi32_critical.h`): Added platform-portable atomics (GCC/clang inline asm, MSVC intrinsics, fallback CAS) for thread-safety, replacing the original `cli`/`popfd` interrupt masking.

2. **Backend abstraction** (`xmidi32_backend.h`): Hardware-specific calls (`send_MIDI_message`, `reset_synth`, `init_synth`, `serve_synth`, etc.) are abstracted behind function declarations, making the core driver portable.

3. **`sequence_state` -> `note_entry` array**: The original ASM used parallel arrays (`note_chan`, `note_num`, `note_time`) indexed by slot. The C port consolidates these into `struct note_entry { chan, num, time }` arrays for clarity.

4. **Big-endian reads**: Replaced the ASM byte-swap sequences with a `read_be32()` utility function.

5. **Variable-length number (VLN) decoding**: Factored out into `read_vln()` utility used by both meta and sysex handlers.
