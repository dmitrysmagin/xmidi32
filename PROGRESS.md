# xmi_play progress

## Goal
Build a working `xmi_play.exe` that plays XMIDI files on Windows via Nuked OPL3 emulation + SDL2 audio, with zero compiler warnings in project code.

## Build
- MinGW (MSYS2 UCRT64) under Git Bash on Windows
- `mingw32-make` (native PE binary avoids msys spawn temp-file failures)
- Console subsystem (`-mconsole`)
- Links: SDL2, SDL2main, mingw32, libm
- `pkg-config` for SDL2 flags (avoids space-in-path issues with `sdl2-config`)
- `.build_tmp` + per-command `TMP/TEMP/TMPDIR` to fix gcc temp-file permission errors
- Include style: `<SDL.h>` (both pkg-config include paths provide it)

## Features
- **SDL playback:** real-time audio via `sdl_audio_callback()` matching WAV dumper logic
- **`--wav` mode:** render XMIDI to `.wav` file (bypasses SDL, same serve/generate core)
- **Single-track playback:** `xmi_play.exe <file.xmi> [sequence_num]` — plays only one track; defaults to track 0
- **Loop detection:** Scans EVNT data for FOR_LOOP controller; during WAV render detects EVNT_ptr jump-back and stops after one playthrough

## Fixed Bugs

### Byte-swapped IFF chunk tag constants
`read_be32()` returns big-endian values, but all tag constants were written for little-endian direct memory loads (every constant was reversed).

**Files:** `src/xmidi32_find_seq.c`, `src/xmidi32_register.c`, `src/xmidi32_tempo.c`

**Chunk IDs fixed:**
| Chunk | Wrong (LE) | Correct (BE) |
|-------|-----------|--------------|
| FORM  | `0x4D524F46` | `0x464F524D` |
| XMID  | `0x44494D58` | `0x584D4944` |
| TIMB  | `0x42494D54` | `0x54494D42` |
| RBRN  | `0x4E524252` | `0x5242524E` |
| EVNT  | `0x54454E56` | `0x45564E54` |

### `find_seq` returned wrong pointer
Original assembly returned pointer to **start of FORM chunk**. C port returned `p + 8` (FORM type field). `register.c` used `chunk + 12` expecting FORM-start pointer, causing wrong data offset.

**Fix:** Changed `find_seq` to return `p` instead of `p + 8`.

### `find_seq` didn't skip non-XMID FORM types
DEMO.XMI starts with `FORM XDIR` (metadata), not `FORM XMID`. The function returned the XDIR form, finding no TIMB/EVNT sub-chunks.

**Fix:** Added type check — skip FORM unless type == "XMID".

### Missing `XMI_EMULATION` in src files
`XMI_EMULATION` was defined in `backend.h` (project root), but `src/xmidi32_yamaha.c` includes `xmidi32_backend.h`, not `backend.h`. Without the define, `update_reg()` used `outport()` (privileged `OUT` instruction on x86-64), crashing with SIGILL (0xC0000096).

**Fix:** Added `#define XMI_EMULATION 1` to `src/xmidi32_backend.h`.

### 1-based channel indexing
`xmidi32_map_seq_channel()` takes 1-based channel numbers. `xmi_play.c` was passing 0-based `j`, causing `chan_map[-1]` → access violation (0xC0000005).

**Fix:** Changed `xmidi32_map_seq_channel(h, j, j)` to `xmidi32_map_seq_channel(h, j + 1, j + 1)`.

### Missing 0x80 Note Off handler
Event dispatch in `xmidi32_serve.c` had no case for status 0x80 (Note Off). The byte was never consumed, locking the sequencer.

**Fix:** Added case for 0x80 calling `xmidi32_note_off()` and advancing past the two data bytes.

### Velocity 0 note-on as note-off
MIDI convention: a Note On with velocity=0 should act as a Note Off. The original port treated all Note On events as note starts.

**Fix:** In `xmidi32_note_on.c`, if velocity is 0, call `xmidi32_note_off()` instead.

### Meta event length 2 bytes too short
`total_len` in `xmidi32_meta.c` only counted the length byte and data, omitting the FF status byte and type byte. `EVNT_ptr` jumped into meta data instead of past it.

**Fix:** Changed `total_len` to `1 + 1 + length` (FF + type + data).

### Interval counting skipped when notes active
Original assembly's `__end_queue` always decrements `interval_cnt` and falls through to `__do_events` when <= 0. The C port's else branch (notes still active) skipped the decrement, causing events to be consumed immediately.

**Fix:** Removed `if (note_count == 0)` guard — interval is always decremented.

### Tempo DDA compared against wrong value
The original assembly compares `tempo_err` against hardcoded 100 and subtracts 100. The C port used `tempo_percent` (a variable), causing erratic timing with non-100 tempo.

**Fix:** Changed `tempo_err >= tempo_percent` / subtract `tempo_percent` to `>= 100` / subtract `100`.

### serve_driver `tempo_percent` re-add on loopback
When `tempo_error >= 100` after gradient processing (tempo_percent > 100%), `goto rep_interval` re-enters the tick loop. ASM does NOT re-add `tempo_percent` at that point; C was re-adding it, causing runaway ticks that consumed all events instantly.

**Fix:** Removed `tempo_percent` re-add on `goto rep_interval`.

### `interval_cnt` unsigned wrap never fired interval-0 events
`interval_cnt` is `uint16_t`. ASM uses `dec [val]; jle` (signed), so 0→0xFFFF wraps to signed -1 and fires events. C's `uint16_t` wrap (0→65535) never matched `<= 0`.

**Fix:** Cast `interval_cnt` to `(int16_t)` before the `<= 0` check.

### Pitch bend centering wrong by 2 bits
MIDI pitch bend 14-bit value was shifted left by 2 extra bits (`<<9` + `<<2`), but center was subtracted as `0x2000` instead of `0x8000` (`0x2000 << 2`). Caused pitch to jump up by ~half a semitone at center position.

**Fix:** `((pitch_h<<7|pitch_l)-0x2000)>>5*range` matching ASM's `sub ax,2000h; imul range; sar ax,5`.

### Frequency computation: 3 algorithmic mismatches from ASM
1. Note normalization: `note - 12` (ASM net effect `sub 24; add 12; add 12; sub 12` = -12), was -24.
2. Note+pb combination: ASM adds MIDI note to AH of pb word, equivalent to `(note*256+pb+8)>>4-192`, was `(note<<4)+8+pb-192`.
3. Table indexing: byte offset `(htone<<5)+((val<<1)&31)` then `>>1` loses bit 0 of fine nibble; was `((htone<<5)|(val&0x0F))>>1` which didn't drop it.

**Fix:** Rewrote frequency computation to match ASM byte-for-byte; verified via Python simulation (26/27 test cases match at center, +2, -2 semitones across C0-C8).

## Build Warnings
- **Zero warnings** in project source files (all `-Wall -Wextra` clean)
- Pre-existing warnings in `opl3.c` (third-party, not our code)

## How to run
```
xmi_play.exe <file.xmi> [sequence_num]    # play via SDL (default track 0)
xmi_play.exe --wav <file.xmi> [sequence_num]  # render to WAV
```

## Files changed
- `Makefile` — `-mconsole`, `pkg-config`, `.build_tmp` + `TMP_ENV`, `.DEFAULT_GOAL`, `SYSTEMROOT` detection; moved all .c/.h to src/
- `src/xmi_play.c` — `--wav` mode, `#include <SDL.h>`, single-track + seq_num arg, track count display, loop detection
- `src/sdl_audio.c` — callback rewritten to match WAV loop (static accum, hardcoded spt), `#include <SDL.h>`, hardcoded 44100
- `src/xmidi32_backend.h` — added `XMI_EMULATION`
- `src/xmidi32_find_seq.c` — fixed tag constants, FORM start return, XDIR skip
- `src/xmidi32_register.c` — fixed tag constants
- `src/xmidi32_tempo.c` — fixed RBRN tag constant
- `src/xmidi32_serve.c` — added 0x80 Note Off handler, removed interval-count guard, fixed DDA comparison, `goto rep_interval` tempo_percent fix, `(int16_t)` interval_cnt cast
- `src/xmidi32_note_on.c` — velocity 0 → note off
- `src/xmidi32_meta.c` — fixed total_len calculation
- `src/xmidi32_yamaha.c` — FBC fix (reg 0xC0 feedback), pitch bend centering fix, frequency computation rewrite, `do_install_timbre` reinstall fix
- `src/xmidi32_timbre.c` — fixed tag/count/bounds in `xmidi32_timbre_request()`
- `src/xmidi32_control.c` — NEXT_LOOP off-by-one, INDIRECT_C_PFX indirection, PV hash write, FOR_loop_cnt/ptrs field order
