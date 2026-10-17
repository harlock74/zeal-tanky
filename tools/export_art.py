#!/usr/bin/env python3
"""Refresh derived Tank artwork automatically; SDK gif2zeal packs game assets."""
from pathlib import Path
from io import BytesIO
import argparse,struct,zlib
from PIL import Image
from check_atlas import read_atlas
ROOT=Path(__file__).resolve().parents[1]

def write_changed(path,data):
    if not path.exists() or path.read_bytes()!=data:path.write_bytes(data)

def preview(data,pixels,palette):
    # Preserve the existing preview's layer, named tags, durations and metadata.
    # Replace only palette colours and each frame's indexed tile pixels.
    assert struct.unpack_from('<4H',data,6)==(128,16,16,8)
    frames=[];offset=128
    for tile in range(128):
        size,magic,count=struct.unpack_from('<IHH',data,offset)
        assert magic==0xF1FA
        q=offset+16;chunks=[];cels=0
        for _ in range(count):
            length,kind=struct.unpack_from('<IH',data,q)
            body=bytearray(data[q+6:q+length]);q+=length
            if kind==0x2019:
                _,first,last=struct.unpack_from('<III',body);at=20
                for index in range(first,last+1):
                    flags=struct.unpack_from('<H',body,at)[0]
                    body[at+2:at+5]=bytes(palette[index*3:index*3+3]);at+=6
                    if flags&1:n=struct.unpack_from('<H',body,at)[0];at+=2+n
            elif kind==0x2005:
                cels+=1
                raw=b''.join(bytes(pixels[(tile//16*16+y)*256+tile%16*16:(tile//16*16+y)*256+tile%16*16+16]) for y in range(16))
                struct.pack_into('<hhBH',body,2,0,0,255,2)
                body=body[:16]+struct.pack('<HH',16,16)+zlib.compress(raw)
            chunks.append(struct.pack('<IH',len(body)+6,kind)+body)
        assert q==offset+size and cels==1
        frame=bytearray(data[offset:offset+16]);payload=b''.join(chunks)
        struct.pack_into('<I',frame,0,len(payload)+16);frames.append(bytes(frame)+payload);offset+=size
    assert offset==len(data)
    result=bytearray(data[:128]+b''.join(frames));struct.pack_into('<I',result,0,len(result))
    return bytes(result)

def main():
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('--packed',type=Path);args=parser.parse_args()
    pixels,palette=read_atlas(ROOT/'assets/tiles.aseprite')
    if args.packed:
        expected=bytes((pixels[(n//16*16+y)*256+n%16*16+x]<<4)|pixels[(n//16*16+y)*256+n%16*16+x+1] for n in range(128) for y in range(16) for x in range(0,16,2))
        assert args.packed.read_bytes()==expected,'Packed tiles differ from Aseprite'
        colours=[((palette[i]>>3)<<11)|((palette[i+1]>>2)<<5)|(palette[i+2]>>3) for i in range(0,48,3)]
        assert args.packed.with_suffix('.ztp').read_bytes()==struct.pack('<16H',*colours),'Packed palette differs from Aseprite'
        print('Packed Tank pixels and palette match the authoring atlas exactly.');return
    image=Image.frombytes('P',(256,128),bytes(pixels));image.putpalette(palette)
    for fmt in ('GIF','PNG'):
        output=BytesIO();image.save(output,format=fmt,optimize=False,transparency=0)
        write_changed(ROOT/('assets/tiles.'+fmt.lower()),output.getvalue())
    path=ROOT/'assets/animations.aseprite';write_changed(path,preview(path.read_bytes(),pixels,palette))
    print('Tank GIF, PNG and tagged animation preview match the editable atlas; unchanged files preserved.')

if __name__=='__main__':main()
