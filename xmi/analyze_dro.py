#!/usr/bin/env python3
import struct

with open('ail32_V105/SAMPLE.AD', 'rb') as f:
    ad = f.read()

# Parse directory
ad_entries = {}
pos = 0
while pos + 6 <= len(ad):
    bank_patch = struct.unpack_from('<H', ad, pos)[0]
    offset = struct.unpack_from('<H', ad, pos+2)[0]
    extra = struct.unpack_from('<H', ad, pos+4)[0]
    bank = (bank_patch >> 8) & 0xFF
    patch = bank_patch & 0xFF
    if bank <= 127 and patch <= 127 and offset + 14 <= len(ad) and extra == 0:
        ad_entries[(bank, patch)] = offset
    else:
        break
    pos += 6

# Build expected OPL values for each instrument
all_instrs = []
for (bank, patch), off in ad_entries.items():
    raw = ad[off:off+14]
    instr = {
        'bank': bank, 'patch': patch,
        'opl_values': {
            0x20: raw[3],  0x23: raw[9],
            0x40: raw[4],  0x43: raw[10],
            0x60: raw[5],  0x63: raw[11],
            0x80: raw[6],  0x83: raw[12],
            0xE0: raw[7],  0xE3: raw[13],
            0xC0: raw[8],
        }
    }
    all_instrs.append(instr)

# Parse DRO
dro_data = open('xmi/demo.dro', 'rb').read()
codemap = list(dro_data[26:26+dro_data[25]])
data_start = 26 + dro_data[25]

events_dro = []
off = data_start
td = 0
while off < len(dro_data):
    code = dro_data[off]
    off += 1
    if code == dro_data[23]:
        td += dro_data[off] + 1; off += 1
    elif code == dro_data[24]:
        td += (dro_data[off] + 1) << 8; off += 1
    else:
        val = dro_data[off]; off += 1
        chip = (code >> 7); idx = code & 0x7F
        if idx < len(codemap):
            events_dro.append((td, chip, codemap[idx], val))

# Find blocks
blocks = []
in_block = False
s = 0
for i, (t, chip, reg, val) in enumerate(events_dro):
    is_instr = (0x20 <= reg <= 0x35) or (0x40 <= reg <= 0x55) or \
               (0x60 <= reg <= 0x75) or (0x80 <= reg <= 0x95) or \
               (0xE0 <= reg <= 0xF5) or (0xC0 <= reg <= 0xC8)
    if is_instr and not in_block:
        in_block = True; s = i
    elif not is_instr and in_block:
        if i - s >= 8:
            blocks.append((s, i-1))
        in_block = False
if in_block and len(events_dro) - s >= 8:
    blocks.append((s, len(events_dro)-1))

print(f"Total DRO blocks: {len(blocks)}")

# Parse capture.txt
events_cap = []
with open('xmi/capture.txt') as f:
    for line in f:
        line = line.strip()
        if not line or line.startswith('#'):
            continue
        parts = line.split()
        if len(parts) == 4:
            events_cap.append((int(parts[0]), int(parts[1]), int(parts[2]), int(parts[3])))

# Find blocks in capture
blocks_cap = []
in_block = False
s = 0
for i, (t, chip, reg, val) in enumerate(events_cap):
    is_instr = (0x20 <= reg <= 0x35) or (0x40 <= reg <= 0x55) or \
               (0x60 <= reg <= 0x75) or (0x80 <= reg <= 0x95) or \
               (0xE0 <= reg <= 0xF5) or (0xC0 <= reg <= 0xC8)
    if is_instr and not in_block:
        in_block = True; s = i
    elif not is_instr and in_block:
        if i - s >= 8:
            blocks_cap.append((s, i-1))
        in_block = False
if in_block and len(events_cap) - s >= 8:
    blocks_cap.append((s, len(events_cap)-1))

print(f"Total CAP blocks: {len(blocks_cap)}")

# Helper: extract block values
def block_vals(events, s, e):
    v = {}
    for i in range(s, e+1):
        t, chip, reg, val = events[i]
        v[reg] = val
    return v

# Match DRO blocks to SAMPLE.AD instruments
print("\n=== DRO Block -> SAMPLE.AD matching ===")
matched_dro = 0
for j, (s, e) in enumerate(blocks):
    if j >= 50:
        break
    dro_v = block_vals(events_dro, s, e)
    dro_keys = set(dro_v.keys())
    
    best = None
    best_score = 0
    for instr in all_instrs:
        opl = instr['opl_values']
        overlap = dro_keys & set(opl.keys())
        if not overlap:
            continue
        score = sum(1 for k in overlap if dro_v.get(k) == opl.get(k))
        if score > best_score:
            best_score = score
            best = instr
    
    if best and best_score >= 6:
        instr = best
        matched_dro += 1
        opl = instr['opl_values']
        dro_v = block_vals(events_dro, s, e)
        diffs = [(r, dro_v[r], opl[r]) for r in sorted(set(dro_v.keys()) | set(opl.keys()))
                 if dro_v.get(r) != opl.get(r) and dro_v.get(r) is not None and opl.get(r) is not None]
        if diffs:
            print(f"  DRO Block {j}: Bank {instr['bank']} Patch {instr['patch']} ({best_score} OK, {len(diffs)} diffs)")
            for r, vd, ve in diffs[:5]:
                print(f"    DIFF: reg=0x{r:02x} DRO=0x{vd:02x} EXPECTED=0x{ve:02x}")

print(f"\nMatched DRO blocks: {matched_dro}")

# Now compare DRO with capture.txt directly
print("\n=== Direct DRO vs Capture comparison ===")
# Map blocks from DRO to capture by checking which capture block has the same values
for j, (s_dro, e_dro) in enumerate(blocks[:30]):
    dro_v = block_vals(events_dro, s_dro, e_dro)
    dro_keys = set(dro_v.keys())
    
    # Find matching capture block
    best_cap = None
    best_score = 0
    for k, (s_cap, e_cap) in enumerate(blocks_cap):
        cap_v = block_vals(events_cap, s_cap, e_cap)
        overlap = dro_keys & set(cap_v.keys())
        if not overlap:
            continue
        score = sum(1 for r in overlap if dro_v.get(r) == cap_v.get(r))
        if score > best_score:
            best_score = score
            best_cap = (k, cap_v)
    
    if best_cap and best_score >= 5:
        k, cap_v = best_cap
        diffs = [(r, dro_v[r], cap_v[r]) for r in sorted(set(dro_v.keys()) | set(cap_v.keys()))
                 if dro_v.get(r) != cap_v.get(r) and dro_v.get(r) is not None and cap_v.get(r) is not None]
        if diffs:
            print(f"\n  Block {j}: DRO [{s_dro}..{e_dro}] vs CAP [{blocks_cap[k][0]}..{blocks_cap[k][1]}] ({best_score} match, {len(diffs)} diffs)")
            for r, vd, vc in diffs[:10]:
                print(f"    DIFF: reg=0x{r:02x} DRO=0x{vd:02x} CAP=0x{vc:02x}")
        else:
            print(f"  Block {j}: IDENTICAL (DRO [{s_dro}..{e_dro}] vs CAP [{blocks_cap[k][0]}..{blocks_cap[k][1]}])")
    else:
        print(f"  Block {j}: No match found (best={best_score})")
