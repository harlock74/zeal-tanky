#!/usr/bin/env python3
"""Read the authoritative single-frame indexed Tank atlas without desktop tools."""
from pathlib import Path
import struct,zlib

def read_atlas(path):
    data = path.read_bytes()
    assert struct.unpack_from('<H',data,4)[0] == 0xA5E0, 'Invalid Aseprite file'
    frames,w,h,depth = struct.unpack_from('<4H',data,6)
    assert (frames,w,h,depth) == (1,256,128,8), 'Keep a single-frame 256x128 indexed atlas (128 tile slots)'
    pixels = bytearray(w*h); palette = [0]*768; pos = 144; cels = 0
    while pos < len(data):
        size,kind = struct.unpack_from('<IH',data,pos)
        assert size >= 6 and pos+size <= len(data), 'Invalid Aseprite chunk'
        chunk = data[pos+6:pos+size]
        if kind in (4,17):
            packets = struct.unpack_from('<H',chunk)[0]; q=2; index=0
            for _ in range(packets):
                skip,count = chunk[q:q+2];q+=2;index+=skip;count=count or 256
                for i in range(count*3): palette[index*3+i]=chunk[q+i] if kind==4 else chunk[q+i]*255//63
                q+=count*3;index+=count
        elif kind == 0x2019:
            _,first,last = struct.unpack_from('<III',chunk);q=20
            for index in range(first,last+1):
                flags=struct.unpack_from('<H',chunk,q)[0];rgba=chunk[q+2:q+6];q+=6
                assert rgba[3] == 255 or index == 0, 'Use opaque palette colours apart from transparent index 0'
                palette[index*3:index*3+3]=rgba[:3]
                if flags&1:
                    length=struct.unpack_from('<H',chunk,q)[0];q+=2+length
        elif kind == 0x2005:
            cels += 1
            _,x,y,opacity,celtype = struct.unpack_from('<HhhBH',chunk)
            assert opacity == 255 and celtype in (0,2), 'Use one opaque indexed cel'
            cw,ch=struct.unpack_from('<HH',chunk,16)
            raw=zlib.decompress(chunk[20:]) if celtype==2 else chunk[20:]
            assert len(raw)==cw*ch and x>=0 and y>=0 and x+cw<=w and y+ch<=h
            for row in range(ch):pixels[(y+row)*w+x:(y+row)*w+x+cw]=raw[row*cw:(row+1)*cw]
        pos+=size
    assert cels==1, 'Flatten visible artwork to a single cel before exporting; retain the 256x128 canvas'
    assert max(pixels)<16, 'The game uses one 16-colour palette'
    return pixels,palette

if __name__=='__main__':
    import sys
    read_atlas(Path(sys.argv[1]))
    print('Editable indexed atlas: 256x128, one frame, 16x16 tile grid: PASS')
