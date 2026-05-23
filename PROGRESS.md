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
- Multi-file playback: `xmi_play.exe file1.xmi file2.xmi ...`

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

## Build Warnings
- **Zero warnings** in project source files (all `-Wall -Wextra` clean)
- Pre-existing warnings in `opl3.c` (third-party, not our code)

## How to run
```
xmi_play.exe <file.xmi> [file2.xmi ...]
xmi_play.exe --wav <file.xmi>     # render to WAV instead of playback
```

## Files changed
- `Makefile` — `-mconsole`, `pkg-config`, `.build_tmp` + `TMP_ENV`, `.DEFAULT_GOAL`, `SYSTEMROOT` detection
- `xmi_play.c` — `--wav` mode, `#include <SDL.h>`
- `sdl_audio.c` — callback rewritten to match WAV loop (static accum, hardcoded spt), `#include <SDL.h>`, hardcoded 44100
- `src/xmidi32_backend.h` — added `XMI_EMULATION`
- `src/xmidi32_find_seq.c` — fixed tag constants, FORM start return, XDIR skip
- `src/xmidi32_register.c` — fixed tag constants
- `src/xmidi32_tempo.c` — fixed RBRN tag constant
- `src/xmidi32_serve.c` — added 0x80 Note Off handler, removed interval-count guard, fixed DDA comparison
- `src/xmidi32_note_on.c` — velocity 0 → note off
- `src/xmidi32_meta.c` — fixed total_len calculation
