#!/usr/bin/env python3
"""Convert DRO v2 to simple text: tick chip reg val"""
import struct, sys

with open(sys.argv[1], 'rb') as f:
    magic = f.read(8)
    version = struct.unpack('<I', f.read(4))[0]
    if version != 2:
        sys.exit(1)
    data_len_words = struct.unpack('<I', f.read(4))[0]
    length_ms = struct.unpack('<I', f.read(4))[0]
    opl_type = struct.unpack('B', f.read(1))[0]
    fmt = struct.unpack('B', f.read(1))[0]
    compr = struct.unpack('B', f.read(1))[0]
    cmd_delay_s = struct.unpack('B', f.read(1))[0]
    cmd_delay_l = struct.unpack('B', f.read(1))[0]
    conv_len = struct.unpack('B', f.read(1))[0]
    conv_table = list(f.read(conv_len))
    data = f.read(data_len_words * 2)

tick = 0
pos = 0
while pos < len(data):
    idx = data[pos]; pos += 1
    val = data[pos]; pos += 1
    if idx == cmd_delay_s:
        tick += val + 1
    elif idx == cmd_delay_l:
        tick += (val + 1) << 8
    else:
        chip = 1 if (idx & 0x80) else 0
        reg_idx = idx & 0x7F
        reg = conv_table[reg_idx] if reg_idx < conv_len else reg_idx
        print(f"{tick} {chip} {reg} {val}")
