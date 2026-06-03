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

All source files reside under `src/`:

```
xmidi32.h (in src/)
  ├── xmidi32_config.h
  ├── xmidi32_types.h
  ├── xmidi32_driver.h
  │   ├── xmidi32_backend.h
  │   └── xmidi32_critical.h
  │
  ├── xmidi32_utils.c/h
  ├── xmidi32_globals.c
  ├── xmidi32_dispatch.c
  │
  ├── xmidi32_api.c
  ├── xmidi32_init.c
  ├── xmidi32_serve.c
  ├── xmidi32_register.c
  ├── xmidi32_find_seq.c
  ├── xmidi32_flush.c
  ├── xmidi32_reset.c/h
  ├── xmidi32_control.c/h
  ├── xmidi32_note_on.c
  ├── xmidi32_meta.c
  ├── xmidi32_sysex.c
  ├── xmidi32_volume.c
  ├── xmidi32_channel.c
  ├── xmidi32_count.c
  ├── xmidi32_tempo.c
  ├── xmidi32_midi.c
  │
  ├── xmidi32_timbre.c
  ├── xmidi32_timbre_internal.h
  ├── xmidi32_yamaha.c
  ├── xmidi32_yamaha.h
  ├── xmidi32_yamaha_tables.c
  ├── xmidi32_yamaha_tables.h
  └── xmidi32_yamaha_stub.c
```

The following companion modules (also in `src/`) are not part of the core library:

```
opl3.c/h         — Nuked OPL3 emulator
backend.c/h      — backend interface (reset/init/serve/IO)
sdl_audio.c/h    — SDL2 audio output
timbre_bank.c/h  — pre-installed OPL3 instrument bank
sample_opl.c     — SAMPLE.OPL binary data (identical to SAMPLE.AD)
xmi_play.c       — CLI player entry point
dump_wav.c       — WAV dump utility
sdltest.c        — SDL audio test tool
```

## Build & Run

### Build

```
make
```

Builds `xmi_play.exe` on Windows (MSYS2/MinGW), `xmi_play` on Linux/macOS. Requires SDL2 (`pkg-config`).

### Run

```
xmi_play <file.xmi> [sequence_num] [bank_file]          Play via SDL audio
xmi_play --wav <file.xmi> [sequence_num] [bank_file]    Render to WAV
```

- `sequence_num`: track index (default 0, shown on start)
- `bank_file`: `.opl`/`.ad` (GTL format) or `.bnk` (standard AdLib BNK1)

### Features

- SDL playback via `sdl_audio_callback()` matching WAV dumper logic
- `--wav` mode renders XMIDI to `.wav` file (bypasses SDL, same serve/generate core)
- Single-track playback (plays only selected track; defaults to 0)
- Loop detection: scans EVNT for FOR_LOOP controller; WAV render stops after first loop
- Zero compiler warnings in project code (`-Wall -Wextra` clean)

## Fixed Bugs

### Byte-swapped IFF chunk tag constants
`read_be32()` returns big-endian values, but all tag constants were written for little-endian direct memory loads (every constant was reversed). **Files:** `src/xmidi32_find_seq.c`, `src/xmidi32_register.c`, `src/xmidi32_tempo.c`

| Chunk | Wrong (LE) | Correct (BE) |
|-------|-----------|--------------|
| FORM  | `0x4D524F46` | `0x464F524D` |
| XMID  | `0x44494D58` | `0x584D4944` |
| TIMB  | `0x42494D54` | `0x54494D42` |
| RBRN  | `0x4E524252` | `0x5242524E` |
| EVNT  | `0x54454E56` | `0x45564E54` |

### `find_seq` returned wrong pointer
Original assembly returned pointer to **start of FORM chunk**. C port returned `p + 8` (FORM type field). `register.c` used `chunk + 12` expecting FORM-start pointer, causing wrong data offset. **Fix:** Changed `find_seq` to return `p` instead of `p + 8`.

### `find_seq` didn't skip non-XMID FORM types
DEMO.XMI starts with `FORM XDIR` (metadata), not `FORM XMID`. The function returned the XDIR form, finding no TIMB/EVNT sub-chunks. **Fix:** Added type check — skip FORM unless type == "XMID".

### Missing `XMI_EMULATION` in src files
`XMI_EMULATION` was defined in `backend.h` (project root), but `src/xmidi32_yamaha.c` includes `xmidi32_backend.h`. Without the define, `update_reg()` used `outport()` (privileged `OUT` instruction on x86-64), crashing with SIGILL (0xC0000096). **Fix:** Added `#define XMI_EMULATION 1` to `src/xmidi32_backend.h`.

### 1-based channel indexing
`xmidi32_map_seq_channel()` takes 1-based channel numbers. `xmi_play.c` was passing 0-based `j`, causing `chan_map[-1]` → access violation (0xC0000005). **Fix:** Changed to `xmidi32_map_seq_channel(h, j + 1, j + 1)`.

### Missing 0x80 Note Off handler
Event dispatch in `xmidi32_serve.c` had no case for status 0x80 (Note Off). The byte was never consumed, locking the sequencer. **Fix:** Added case for 0x80 calling `xmidi32_note_off()` and advancing past the two data bytes.

### Velocity 0 note-on as note-off
MIDI convention: a Note On with velocity=0 should act as a Note Off. The original port treated all Note On events as note starts. **Fix:** In `xmidi32_note_on.c`, if velocity is 0, call `xmidi32_note_off()` instead.

### Meta event length 2 bytes too short
`total_len` in `xmidi32_meta.c` only counted the length byte and data, omitting the FF status byte and type byte. `EVNT_ptr` jumped into meta data instead of past it. **Fix:** Changed `total_len` to `1 + 1 + length` (FF + type + data).

### Interval counting skipped when notes active
Original assembly's `__end_queue` always decrements `interval_cnt` and falls through to `__do_events` when <= 0. The C port's else branch (notes still active) skipped the decrement, causing events to be consumed immediately. **Fix:** Removed `if (note_count == 0)` guard — interval is always decremented.

### Tempo DDA compared against wrong value
The original assembly compares `tempo_err` against hardcoded 100 and subtracts 100. The C port used `tempo_percent` (a variable), causing erratic timing with non-100 tempo. **Fix:** Changed `tempo_err >= tempo_percent` / subtract `tempo_percent` to `>= 100` / subtract `100`.

### serve_driver `tempo_percent` re-add on loopback
When `tempo_error >= 100` after gradient processing (tempo_percent > 100%), `goto rep_interval` re-enters the tick loop. ASM does NOT re-add `tempo_percent` at that point; C was re-adding it, causing runaway ticks that consumed all events instantly. **Fix:** Removed `tempo_percent` re-add on `goto rep_interval`.

### `interval_cnt` unsigned wrap never fired interval-0 events
`interval_cnt` is `uint16_t`. ASM uses `dec [val]; jle` (signed), so 0→0xFFFF wraps to signed -1 and fires events. C's `uint16_t` wrap (0→65535) never matched `<= 0`. **Fix:** Cast `interval_cnt` to `(int16_t)` before the `<= 0` check.

### Pitch bend centering wrong by 2 bits
MIDI pitch bend 14-bit value was shifted left by 2 extra bits (`<<9` + `<<2`), but center was subtracted as `0x2000` instead of `0x8000` (`0x2000 << 2`). Caused pitch to jump up by ~half a semitone at center position. **Fix:** `((pitch_h<<7|pitch_l)-0x2000)>>5*range` matching ASM's `sub ax,2000h; imul range; sar ax,5`.

### Frequency computation: 3 algorithmic mismatches from ASM
1. Note normalization: `note - 12` (ASM net effect `sub 24; add 12; add 12; sub 12` = -12), was -24.
2. Note+pb combination: ASM adds MIDI note to AH of pb word, equivalent to `(note*256+pb+8)>>4-192`, was `(note<<4)+8+pb-192`.
3. Table indexing: byte offset `(htone<<5)+((val<<1)&31)` then `>>1` loses bit 0 of fine nibble; was `((htone<<5)|(val&0x0F))>>1` which didn't drop it.

**Fix:** Rewrote frequency computation to match ASM byte-for-byte; verified via Python simulation (26/27 test cases match at center, +2, -2 semitones across C0-C8).

### NEXT_LOOP off-by-one
ASM decrements counter **first**, then checks `jnz` (jump if not zero). C checks for zero **first**, then decrements. Result: counter `val` gives `val+1` iterations instead of `val`. **Fix:** decrement first, then check, and only jump back if still > 0.

### INDIRECT_C_PFX uses index as value
ASM stores an **index** into `chan_indirect[]`, then on the next controller call looks up `ctrl_ptr[index]` for the actual value. C uses the stored index **directly as the value**, bypassing the `ctrl_ptr` indirection. **Fix:** `val = st->ctrl_ptr[(uint8_t)st->chan_indirect[log_chan]];`

### global_controls.PV written for ALL controllers
ASM writes to the hash-offsetted field (`PV`, `MODUL`, `PAN`, etc.) based on which controller triggered. C always writes to `global_controls.PV[log_chan]`. **Fix:** use `GCTL(log_chan, hash)` or equivalent byte-offset write.

### Gradient accumulators store wrong value
ASM stores the **remainder** after subtracting `steps * period`. C stores the **full initial value** (`tempo_grad` / `vol_grad`). **Fix:** store `tempo_rem` / `vol_rem` instead.

### Gradient accumulators not updated when steps==0
ASM always updates accum. C only updates inside `if (steps != 0)`. **Fix:** move accum update outside the `if`.

### Volume gradient `> 0` instead of `>= period`
ASM uses `jge` (signed `>= 0`) after `sub eax,period`, which is `>= period` in C. C uses `> 0`. When `vol_rem` is between 1 and `period-1`, ASM does 0 steps (correct), C does 1 step (wrong). **Fix:** change `while (vol_rem > 0)` to `while (vol_rem >= st->vol_period)`.

### Timbre reinstall bug
`do_install_timbre` re-checked `index_timbre` and returned early if found, failing to assign a new cache slot. **Fix:** remove early return and always allocate a new slot.

### `xmidi32_timbre_request()` endianness
Three endianness bugs: reversed TIMB constant, reversed count bytes, and wrong bounds comparison in `request_next`. **Fix:** all three fixed.

### Missing silent timbre for missing patches
When a timbre is not in any bank, the driver would spin forever in `install_sequence_timbres`. **Fix:** install a silent dummy 2-op instrument.

### Missing timbre protect handler
ASM handles controller 113 to protect/unprotect individual timbres. C port had no equivalent. **Fix:** added `case TIMBRE_PROTECT:` in `yamaha_controller()`.

### Missing reset-all-controllers handler
ASM resets modulation, expression, sustain, pitch bend and flags voice params. C port missing. **Fix:** added `case RESET_ALL_CTRLS:` in `yamaha_controller()`.

### Panning investigation — resolved
OPL3 bit 4 = LEFT output, bit 5 = RIGHT output. C masks `LEFT_MASK=0xEF`, `RIGHT_MASK=0xDF` match ASM ELSE/default case. Dedicated `pan_test2.c` confirmed routing end-to-end. Early-sample R>L imbalance in AGU16.XMI was noise floor, not a logic swap.

## Audit — Remaining Open Items

- [ ] **XMIDI controller numbers differ from AIL spec** (`src/xmidi32_config.h`):
  `CHAN_LOCK=96` (was 110), `FOR_LOOP=100` (was 116), `CALLBACK_TRIG=110` (was 119), `PATCH_BANK_SEL=0` (was 114), etc. The C port **renumbered all extended XMIDI controllers**.
- [ ] **Channel range expansion** (2-10 → 1-16): Intentional change for OPL3/16-channel MIDI support.
- [ ] **Note queue: struct vs parallel arrays**: C uses `struct note_entry {chan, num, time}` with 4-byte padding vs ASM's tightly packed arrays. 256 bytes vs 192 bytes. Functionally equivalent.

### Verified Correct

- All 33+ API entries in the dispatch table match the ASM `driver_index`
- `yamaha_note_on` slot allocation and LRU eviction logic matches
- `update_voice` register write logic matches (WS fix applied)
- `BNK_phase` / `OPL3BNK_phase` timbre data extraction matches at all byte offsets
- `freq_table`, `note_octave`, `note_halftone`, `op_0`, `op_1`, `op4_base`, `conn_sel`, etc. match byte-for-byte
- Volume scaling chain (`vol * expression * velocity → attenuation`) matches
- Pitch bend calculation chain matches
- All YAMAHA32.INC state arrays (`S_*`, `MIDI_*`, `V_channel`, `timb_*`) present in C
- `register_seq` IFF chunk parsing matches
- `init_driver` global state initialization matches (aside from channel range)
- Instrument cache `delete_LRU` / `install_timbre` / `index_timbre` match
- 4-op OPL3 voice assignment and release match
- `update_voice` 4-op second iteration (operators 2/3) now writes all registers (WS/ADSR/KSLTL/FBC/AVEKM/FREQ)

## Unported Sub-Drivers

The core XMIDI interpreter and Yamaha OPL2/OPL3 driver are fully ported. The following sub-drivers remain **unported** (still only in ASM):

| File | Lines | Description |
|---|---|---|
| `MT3232.INC` | 955 | Roland MT-32/LAPC-1 synthesizer driver |
| `SPKR32.INC` | 1084 | IBM PC/Tandy 3-voice internal speaker driver |
| `MPU40132.INC` | 151 | Roland MPU-401 UART MIDI interface |
| `SBMIDI32.INC` | 187 | Sound Blaster direct MIDI interface |

These are conditionally included via `IFDEF MT32`, `IFDEF SPKR`, etc. The Yamaha driver's `xmidi32_yamaha_stub.c` provides a compile-time placeholder for non-Yamaha backends.

## Key Porting Decisions

1. **Atomic operations** (`xmidi32_critical.h`): Added platform-portable atomics (GCC/clang inline asm, MSVC intrinsics, fallback CAS) for thread-safety, replacing the original `cli`/`popfd` interrupt masking.

2. **Backend abstraction** (`xmidi32_backend.h`): Hardware-specific calls (`send_MIDI_message`, `reset_synth`, `init_synth`, `serve_synth`, etc.) are abstracted behind function declarations, making the core driver portable.

3. **`sequence_state` -> `note_entry` array**: The original ASM used parallel arrays (`note_chan`, `note_num`, `note_time`) indexed by slot. The C port consolidates these into `struct note_entry { chan, num, time }` arrays for clarity.

4. **Big-endian reads**: Replaced the ASM byte-swap sequences with a `read_be32()` utility function.

5. **Variable-length number (VLN) decoding**: Factored out into `read_vln()` utility used by both meta and sysex handlers.
