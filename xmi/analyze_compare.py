#!/usr/bin/env python3
"""
Per-instrument byte-level comparison: SAMPLE.AD vs DRO vs CAP.

BNK timbre (14 bytes):
  [0-1] len  [2] transp  [3] mod_AVEKM  [4] mod_KSLTL
  [5] mod_AD  [6] mod_SR  [7] mod_WS  [8] FBC
  [9] car_AVEKM  [10] car_KSLTL  [11] car_AD  [12] car_SR  [13] car_WS

Strategy: Build reverse lookup (chip, reg) -> param_index for each voice.
For each instrument-load block, find which voice by looking at the regs,
then extract param values and compare against SAMPLE.AD.
"""

import struct
from collections import defaultdict

# ─── OPL3 register mapping tables (from yamaha_tables.c) ────────────

OP_0 = [0, 1, 2, 6, 7, 8, 12, 13, 14, 18, 19, 20, 24, 25, 26, 30, 31, 32]
OP_1 = [3, 4, 5, 9, 10, 11, 15, 16, 17, 21, 22, 23, 27, 28, 29, 33, 34, 35]
OP_INDEX = [0, 1, 2, 3, 4, 5, 8, 9, 10, 11, 12, 13, 16, 17, 18, 19, 20, 21,
            0, 1, 2, 3, 4, 5, 8, 9, 10, 11, 12, 13, 16, 17, 18, 19, 20, 21]
OP_ARRAY = [0]*18 + [1]*18
VOICE_NUM  = [0, 1, 2, 3, 4, 5, 6, 7, 8, 0, 1, 2, 3, 4, 5, 6, 7, 8]
VOICE_ARRAY = [0]*9 + [1]*9

PARAM_NAMES = ['mod_AVEKM','mod_KSLTL','mod_AD','mod_SR','mod_WS','FBC',
               'car_AVEKM','car_KSLTL','car_AD','car_SR','car_WS']
BNK_OFFSET = [3,4,5,6,7,8,9,10,11,12,13]

# Precompute reverse mapping: (chip, reg) -> list of (voice, param_idx)
# for all 18 voices and all register bases (0x20, 0x40, 0x60, 0x80, 0xE0, 0xC0)
REG_TO_PARAM = defaultdict(list)  # (chip, reg) -> [(voice, param_idx)]

for v in range(18):
    op_a = OP_0[v]
    op_b = OP_1[v]
    chip_a = OP_ARRAY[op_a]
    chip_b = OP_ARRAY[op_b]
    idx_a = OP_INDEX[op_a]
    idx_b = OP_INDEX[op_b]

    # Modulator params (0-4)
    param_bases = [0x20, 0x40, 0x60, 0x80, 0xE0]
    for pi, base in enumerate(param_bases):
        reg_a = base + idx_a
        REG_TO_PARAM[(chip_a, reg_a)].append((v, pi))
        reg_b = base + idx_b
        REG_TO_PARAM[(chip_b, reg_b)].append((v, pi + 6))

    # FBC (param 5)
    reg_fbc = 0xC0 + VOICE_NUM[v]
    chip_fbc = VOICE_ARRAY[v]
    REG_TO_PARAM[(chip_fbc, reg_fbc)].append((v, 5))


def decode_params(events, start, end):
    """
    Extract instrument parameter values from a block of events.
    Returns (param_dict, voice) or (None, None) if ambiguous.
    param_dict: {param_idx: value}
    voice: the voice number this block targets (or -1 if undetermined)
    """
    # For each event, find what (voice, param_idx) it maps to
    param_candidates = defaultdict(set)  # param_idx -> set of values

    for i in range(start, end + 1):
        t, chip, reg, val = events[i]
        matches = REG_TO_PARAM.get((chip, reg), [])
        for voice, param_idx in matches:
            param_candidates[param_idx].add((voice, val))

    # Determine the voice: look at all candidates and find the one voice
    # that appears most consistently
    voice_votes = defaultdict(int)
    for param_idx, candidates in param_candidates.items():
        for voice, val in candidates:
            voice_votes[voice] += 1

    if not voice_votes:
        return None, -1

    # Pick the voice with the most matches
    best_voice = max(voice_votes, key=voice_votes.get)
    best_count = voice_votes[best_voice]

    # If no clear winner or too few matches, return None
    if best_count < 6:
        return None, -1

    # Extract params for this voice only
    params = {}
    for param_idx, candidates in param_candidates.items():
        for voice, val in candidates:
            if voice == best_voice:
                if param_idx not in params:
                    params[param_idx] = val
                break

    return params, best_voice


# ─── SAMPLE.AD parsing ───────────────────────────────────────────────

def parse_sample_ad(path):
    entries = {}
    with open(path, 'rb') as f:
        data = f.read()
    pos = 0
    while pos + 6 <= len(data):
        bp = struct.unpack_from('<H', data, pos)[0]
        offset = struct.unpack_from('<I', data, pos + 2)[0]
        bank = (bp >> 8) & 0xFF
        patch = bp & 0xFF
        if bank > 127 or patch > 127:
            break
        if offset + 14 > len(data):
            break
        entries[(bank, patch)] = data[offset:offset+14]
        pos += 6
    return entries


# ─── DRO parsing ─────────────────────────────────────────────────────

def parse_dro(path):
    with open(path, 'rb') as f:
        data = f.read()
    delay_short = data[23]
    delay_long  = data[24]
    codemap_len = data[25]
    codemap = list(data[26:26 + codemap_len])
    data_start = 26 + codemap_len
    events = []
    off = data_start
    td = 0
    while off < len(data):
        code = data[off]; off += 1
        if code == delay_short:
            td += data[off] + 1; off += 1
        elif code == delay_long:
            td += (data[off] + 1) << 8; off += 1
        else:
            val = data[off]; off += 1
            chip = (code >> 7) & 1
            idx = code & 0x7F
            if idx < len(codemap):
                events.append((td, chip, codemap[idx], val))
    return events


# ─── CAP parsing ─────────────────────────────────────────────────────

def parse_cap(path):
    events = []
    with open(path) as f:
        for line in f:
            line = line.strip()
            if not line or line.startswith('#'):
                continue
            parts = line.split()
            if len(parts) == 4:
                events.append((int(parts[0]), int(parts[1]), int(parts[2]), int(parts[3])))
    return events


# ─── Find instrument-load blocks ─────────────────────────────────────

def is_instr_event(chip, reg):
    for base in [0x20, 0x40, 0x60, 0x80, 0xE0]:
        if base <= reg <= base + 0x15:
            return True
    if 0xC0 <= reg <= 0xC8:
        return True
    return False


def find_blocks(events, min_params=6):
    """Find blocks of consecutive instrument register writes, decode each."""
    blocks = []
    in_block = False
    s = 0
    for i, (t, chip, reg, val) in enumerate(events):
        ii = is_instr_event(chip, reg)
        if ii and not in_block:
            in_block = True
            s = i
        elif not ii and in_block:
            params, voice = decode_params(events, s, i - 1)
            if params and len(params) >= min_params:
                blocks.append((s, i - 1, params, voice))
            in_block = False

    if in_block:
        params, voice = decode_params(events, s, len(events) - 1)
        if params and len(params) >= min_params:
            blocks.append((s, len(events) - 1, params, voice))

    return blocks


def match_instrument(params, sample_instrs):
    """Find the best SAMPLE.AD instrument match for a set of decoded params."""
    best = None
    best_score = -1
    for instr in sample_instrs:
        score = sum(1 for pi, val in params.items()
                    if pi < len(instr['params']) and instr['params'][pi] == val)
        if score > best_score:
            best_score = score
            best = instr
    return best, best_score


# ─── Main ────────────────────────────────────────────────────────────

def main():
    entries = parse_sample_ad('ail32_V105/SAMPLE.OPL')
    print(f"Loaded {len(entries)} instruments from SAMPLE.OPL")

    sample_instrs = []
    for (bank, patch), raw in sorted(entries.items()):
        sample_instrs.append({'bank': bank, 'patch': patch, 'raw': raw,
                              'params': [raw[BNK_OFFSET[i]] for i in range(11)]})

    dro_events = parse_dro('xmi/demo.dro')
    cap_events = parse_cap('xmi/capture.txt')
    print(f"\nDRO events: {len(dro_events)}")
    print(f"CAP events: {len(cap_events)}")

    dro_blocks = find_blocks(dro_events)
    cap_blocks = find_blocks(cap_events)
    print(f"\nDRO blocks: {len(dro_blocks)}")
    print(f"CAP blocks: {len(cap_blocks)}")

    # Show first 10 blocks with details
    print("\nFirst DRO blocks:")
    for i, (s, e, params, voice) in enumerate(dro_blocks[:10]):
        pkeys = sorted(params.keys())
        pstr = ','.join(f'{k}={params[k]:02x}' for k in pkeys)
        print(f"  [{i}] events={s}-{e} voice={voice} params({len(params)}): {pstr}")

    print("\nFirst CAP blocks:")
    for i, (s, e, params, voice) in enumerate(cap_blocks[:10]):
        pkeys = sorted(params.keys())
        pstr = ','.join(f'{k}={params[k]:02x}' for k in pkeys)
        print(f"  [{i}] events={s}-{e} voice={voice} params({len(params)}): {pstr}")

    # Match blocks
    def classify(blocks):
        matched = 0
        for bi, (s, e, params, voice) in enumerate(blocks):
            best, score = match_instrument(params, sample_instrs)
            if best and score >= 6:
                matched += 1
        return matched

    print(f"\nDRO matched: {classify(dro_blocks)}/{len(dro_blocks)}")
    print(f"CAP matched: {classify(cap_blocks)}/{len(cap_blocks)}")

    # ─── Per-instrument comparison ───────────────────────────────
    print("\n" + "=" * 90)
    print("PER-INSTRUMENT DRO vs CAP vs SAMPLE.AD")
    print("=" * 90)

    instr_dro = defaultdict(list)
    instr_cap = defaultdict(list)

    for bi, (s, e, params, voice) in enumerate(dro_blocks):
        best, score = match_instrument(params, sample_instrs)
        if best and score >= 6:
            key = (best['bank'], best['patch'])
            instr_dro[key].append((params, voice, score, bi))

    for bi, (s, e, params, voice) in enumerate(cap_blocks):
        best, score = match_instrument(params, sample_instrs)
        if best and score >= 6:
            key = (best['bank'], best['patch'])
            instr_cap[key].append((params, voice, score, bi))

    all_keys = sorted(set(list(instr_dro.keys()) + list(instr_cap.keys())))
    total_diffs = 0
    total_identical = 0

    for key in all_keys:
        bank, patch = key
        dro_list = instr_dro.get(key, [])
        cap_list = instr_cap.get(key, [])

        instr = None
        for si in sample_instrs:
            if si['bank'] == bank and si['patch'] == patch:
                instr = si
                break
        if not instr:
            continue

        expected = instr['params']
        dro_first = dro_list[0][0] if dro_list else {}
        cap_first = cap_list[0][0] if cap_list else {}

        all_pi = sorted(set(list(range(11)) + list(dro_first.keys()) + list(cap_first.keys())))
        diffs = [(pi, dro_first.get(pi), cap_first.get(pi), expected[pi])
                 for pi in all_pi
                 if dro_first.get(pi) != expected[pi] or cap_first.get(pi) != expected[pi]]

        if diffs:
            total_diffs += 1
            tag = '(DRO only)' if not cap_list else '(CAP only)' if not dro_list else ''
            print(f"\n  Bank {bank:3d} Patch {patch:3d} {tag}")
            raw = instr['raw']
            print(f"    BNK raw: {' '.join(f'{b:02x}' for b in raw)}")
            print(f"    BNK: len={raw[0]|raw[1]<<8} transp={raw[2]:3d}  "
                  f"mod=[{raw[3]:02x} {raw[4]:02x} {raw[5]:02x} {raw[6]:02x} {raw[7]:02x}]  "
                  f"FBC={raw[8]:02x}  car=[{raw[9]:02x} {raw[10]:02x} {raw[11]:02x} {raw[12]:02x} {raw[13]:02x}]")
            print(f"    {'Param':>12}  {'BNK':>8}  {'DRO':>8}  {'CAP':>8}  {'DvB':>6}  {'CvB':>6}")
            for pi, dv, cv, ev in diffs:
                ev_s = f"0x{ev:02x}"
                dv_s = f"0x{dv:02x}" if dv is not None else 'N/A'
                cv_s = f"0x{cv:02x}" if cv is not None else 'N/A'
                db = 'OK' if dv == ev else ('MIS' if dv is not None else '--')
                cb = 'OK' if cv == ev else ('MIS' if cv is not None else '--')
                print(f"    {PARAM_NAMES[pi]:>12}  {ev_s:>8}  {dv_s:>8}  {cv_s:>8}  {db:>6}  {cb:>6}")
        else:
            total_identical += 1

    print(f"\n{'='*90}")
    print(f"SUMMARY: {total_diffs} with diffs, {total_identical} identical")
    print(f"{'='*90}")

    # Multi-load summary
    multi = [(k, instr_dro.get(k,[]), instr_cap.get(k,[])) for k in all_keys
             if len(instr_dro.get(k,[])) > 3 or len(instr_cap.get(k,[])) > 3]
    if multi:
        print(f"\nMulti-load instruments:")
        for key, dro_b, cap_b in multi[:15]:
            print(f"  Bank {key[0]:3d} Patch {key[1]:3d}: DRO={len(dro_b)} CAP={len(cap_b)} "
                  f"voices DRO={[v for p,v,s,b in dro_b]} CAP={[v for p,v,s,b in cap_b]}")

    # Key on/off
    print(f"\n{'='*90}")
    for name, evts in [("DRO", dro_events), ("CAP", cap_events)]:
        key_on  = sum(1 for t,c,r,v in evts if 0xB0 <= r <= 0xB8 and (v & 0x20))
        key_off = sum(1 for t,c,r,v in evts if 0xB0 <= r <= 0xB8 and not (v & 0x20))
        print(f"  {name}: key_on={key_on}, key_off={key_off}")


if __name__ == '__main__':
    main()
