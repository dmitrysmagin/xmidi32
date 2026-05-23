# XMIDI32 ASM→C Port Audit

## Conventions
- `[ ]` — not yet addressed
- `[x]` — fixed (add note with commit/date)

---

## CRITICAL Bugs (affect correct playback)

- [x] **NEXT_LOOP off-by-one** (`src/xmidi32_control.c:68-73`)
  ASM decrements counter **first**, then checks `jnz` (jump if not zero). C checks for zero **first**, then decrements. Result: counter `val` gives `val+1` iterations instead of `val`. Fix: decrement first, then check, and only jump back if still > 0.

- [x] **INDIRECT_C_PFX uses index as value** (`src/xmidi32_control.c:7-9`)
  ASM stores an **index** into `chan_indirect[]`, then on the next controller call looks up `ctrl_ptr[index]` for the actual value. C uses the stored index **directly as the value**, bypassing the `ctrl_ptr` indirection. Fix: `val = st->ctrl_ptr[(uint8_t)st->chan_indirect[log_chan]];`

- [x] **global_controls.PV written for ALL controllers** (`src/xmidi32_control.c:14`)
  ASM writes to the hash-offsetted field (`PV`, `MODUL`, `PAN`, etc.) based on which controller triggered. C always writes to `global_controls.PV[log_chan]`. Fix: use `GCTL(log_chan, hash)` or equivalent byte-offset write.

- [x] **Gradient accumulators store wrong value** (`src/xmidi32_serve.c:195,217`)
  ASM stores the **remainder** after subtracting `steps * period`. C stores the **full initial value** (`tempo_grad` / `vol_grad`). Fix: store `tempo_rem` / `vol_rem` instead.

- [x] **Gradient accumulators not updated when steps==0** (`src/xmidi32_serve.c:194,216`)
  ASM always updates accum. C only updates inside `if (steps != 0)`. Fix: move accum update outside the `if`.

- [x] **Volume gradient `> 0` instead of `>= period`** (`src/xmidi32_serve.c:212`)
  ASM uses `jge` (signed `>= 0`) after `sub eax,period`, which is `>= period` in C. C uses `> 0`. When `vol_rem` is between 1 and `period-1`, ASM does 0 steps (correct), C does 1 step (wrong). Fix: change `while (vol_rem > 0)` to `while (vol_rem >= st->vol_period)`.

---

## HIGH — Functional Issues

- [x] **Missing re-entrancy guard** (`src/xmidi32_serve.c:6`)
  No `xm32_try_enter(&service_active)` at the top of `xmidi32_serve_driver()`. ASM checks and increments `service_active` before processing. **Fixed**: added `if (xm32_try_enter(&service_active) != 0) return;` at function entry.

- [x] **No multi-interval loop-back** (`src/xmidi32_serve.c:19`)
  When `tempo_err >= 100` after beat handling, ASM loops back to reprocess (handling `tempo_percent > 100%`). C processes exactly one interval per service call. **Fixed**: added `rep_interval:` label before tempo check and `if (st->tempo_error >= 100) goto rep_interval;` after gradient blocks.

- [ ] **XMIDI controller numbers differ from AIL spec** (`src/xmidi32_config.h:34-42`)
  `CHAN_LOCK=96` (was 110), `FOR_LOOP=100` (was 116), `CALLBACK_TRIG=110` (was 119), `PATCH_BANK_SEL=0` (was 114), etc. The C port **renumbered all extended XMIDI controllers**.

- [x] **`TIMBRE_PROTECT` (ctrl 113) not handled** (`src/xmidi32_yamaha.c:962`)
  ASM handles controller 113 to protect/unprotect individual timbres in the cache via `index_timbre()`. C port has no equivalent. **Fixed**: added `case TIMBRE_PROTECT:` in `yamaha_controller()` switch — sets/clears bit 6 of `timb_attribs[MIDI_timbre[chan]]` based on `val >= 64`.

- [x] **`RESET_ALL_CTRLS` (ctrl 121) not handled** (`src/xmidi32_yamaha.c:962`)
  ASM resets modulation, expression, sustain, pitch bend defaults and flags all voice params for update. C port missing. **Fixed**: added `case RESET_ALL_CTRLS:` in `yamaha_controller()` switch — resets `MIDI_mod`, `MIDI_express`, `MIDI_sus`, `MIDI_pitch_l/h`, releases sustained notes, flags all channel voices for update with `U_ALL_REGS`.

---

## MEDIUM — Incorrect/Dead Code

- [x] **Dead/bogus CAT detection in `find_seq`** (`src/xmidi32_find_seq.c:13-16`)
  Checks `0x54414320U` (`TAC `) instead of `0x43415420U` (`CAT `). Dead code, never matches. **Fixed**: removed dead check entirely.

- [x] **`end_addr` computed but unused** (`src/xmidi32_find_seq.c:60`)
  ASM uses it for bounds checking in CAT scan; C ignores it. **Fixed**: removed `end_addr` variable and its usage.

- [x] **`cur_callback` initialized to NULL (0) vs -1** (`src/xmidi32_register.c:85`)
  ASM initializes `cur_callback` to `-1` (0xFFFFFFFF). C uses `NULL` (0). Could cause issues if callback trigger compares against `-1`. **Fixed**: changed to `(void *)(intptr_t)-1`.

- [x] **`service_active` is 32-bit vs 16-bit in ASM** (`src/xmidi32_globals.c:11`)
  ASM uses 16-bit `dw`. C uses `uint32_t`. Harmless functionally but different.

- [x] **`sysex_wait()` unconditional** (`src/xmidi32_init.c:54,69`)
  ASM has it conditional on `IFDEF sysex_wait`. C always calls it.

---

## LOW — Design Differences (not bugs)

- [ ] **Channel range expansion (2-10 → 1-16)**
  Intentional change for OPL3/16-channel MIDI support.

- [ ] **Controller renumbering**
  XMIDI controllers deliberately renumbered in the C port.

- [ ] **`ctrl_hash` stores indices vs pre-multiplied offsets**
  C multiplies at access time via `GCTL()` macro — functionally equivalent.

- [ ] **Note queue: struct vs parallel arrays**
  C uses `struct note_entry {chan, num, time}` with 4-byte padding vs ASM's tightly packed arrays. 256 bytes vs 192 bytes.

- [ ] **`FOR_loop_cnt` / `FOR_ptrs` field order reversed**
  C places `cnt` before `ptrs`; ASM has `ptrs` before `cnt`. Self-consistent.

---

## What's Correct (verified)

- [x] All 33+ API entries in the dispatch table match the ASM `driver_index`
- [x] `yamaha_note_on` slot allocation and LRU eviction logic matches
- [x] `update_voice` register write logic matches (WS fix applied)
- [x] `BNK_phase` / `OPL3BNK_phase` timbre data extraction matches at all byte offsets
- [x] `freq_table`, `note_octave`, `note_halftone`, `op_0`, `op_1`, `op4_base`, `conn_sel`, etc. match byte-for-byte
- [x] Volume scaling chain (`vol * expression * velocity → attenuation`) matches
- [x] Pitch bend calculation chain matches
- [x] All YAMAHA32.INC state arrays (`S_*`, `MIDI_*`, `V_channel`, `timb_*`) present in C
- [x] `register_seq` IFF chunk parsing matches
- [x] `init_driver` global state initialization matches (aside from channel range)
- [x] Instrument cache `delete_LRU` / `install_timbre` / `index_timbre` match
- [x] 4-op OPL3 voice assignment and release match
- [x] `update_voice` 4-op second iteration (operators 2/3) now writes all registers (WS/ADSR/KSLTL/FBC/AVEKM/FREQ)
