#!/usr/bin/env python3
"""Compress each packed tile independently; decode only during startup."""
from pathlib import Path
import sys
def encode(data):
    out=bytearray();i=0
    while i<len(data):
        run=1
        while i+run<len(data) and data[i+run]==data[i] and run<128:run+=1
        if run>=3:out.extend([0x80|run-1,data[i]]);i+=run
        else:
            start=i;i+=run
            while i<len(data) and i-start<128:
                r=1
                while i+r<len(data) and data[i+r]==data[i] and r<3:r+=1
                if r>=3:break
                i+=min(r,128-(i-start))
            out.append(i-start-1);out.extend(data[start:i])
    return out
def encode_tile(data):
    pixels=[v for b in data for v in (b>>4,b&15)]
    colors=sorted(set(pixels));packed=bytearray([0])+encode(data)
    if len(colors)==2:
        binary=bytearray([1,colors[0]*16+colors[1]])
        for start in range(0,256,8):
            binary.append(sum((pixels[start+n]==colors[1])<<(7-n) for n in range(8)))
        if len(binary)<len(packed):return binary
    return packed
def generate(source,header,payload):
    raw=source.read_bytes();assert len(raw)==16384
    result=bytearray();offsets=[0]
    used=set(range(16))|{16,17,24,25,32,33,40,41}|set(range(80,128))
    for n in range(128):
        if n in used:result.extend(encode_tile(raw[n*128:(n+1)*128]))
        offsets.append(len(result))
    header.parent.mkdir(parents=True,exist_ok=True)
    header.write_text('/* Generated packed-tile offsets. */\n#define TILE_PAYLOAD_SIZE '+str(len(result))+'\nstatic const uint16_t tile_offsets[129]={'+','.join(map(str,offsets))+'};\n')
    payload.write_bytes(result)
    print(f'{len(used)} shared native tiles from 128 editor slots: {len(raw)} -> {len(result)} bytes; 128-byte startup decode cache.')
if __name__=='__main__':generate(*(Path(p) for p in sys.argv[1:]))
