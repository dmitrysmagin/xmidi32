# xmi_play progress

## Goal
Build a working `xmi_play.exe` that plays XMIDI files on Windows via Nuked OPL3 emulation + SDL2 audio, with zero compiler warnings in project code.

## Build
- MinGW (MSYS2 UCRT64) under Git Bash on Windows
- `mingw32-make` (native PE binary avoids msys spawn temp-file failures)
- Console subsystem (`-mconsole`)
- Links: SDL2, SDL2main, mingw32

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

## Build Warnings
- **Zero warnings** in project source files (all `-Wall -Wextra` clean)
- Pre-existing warnings in `opl3.c` (third-party, not our code)

## How to run
```
xmi_play.exe <file.xmi> [file2.xmi ...]
```

## Files changed
- `Makefile` — added `-mconsole`
- `src/xmidi32_backend.h` — added `XMI_EMULATION`
- `src/xmidi32_find_seq.c` — fixed tag constants, FORM start return, XDIR skip
- `src/xmidi32_register.c` — fixed tag constants
- `src/xmidi32_tempo.c` — fixed RBRN tag constant
- `xmi_play.c` — fixed 1-based channel mapping
