#!/usr/bin/env python3
"""Validate Tiled authoring maps and emit compact logical cells + spawns."""
from collections import deque
from pathlib import Path
import argparse
import xml.etree.ElementTree as ET
def generate(source,destination):
    root=ET.parse(source).getroot();layers=root.findall('layer');maps=[];names=[]
    width=int(root.get('width'));height=int(root.get('height'))
    assert len(root.findall('tileset'))==1 and root.find('tileset').get('source')=='tiles.tsx','Use only the local tiles.tsx; remove external backup tilesets'
    assert width==height and width in (26,28) and len(layers)==8,'Use eight 28x28 stages, including their one-tile border'
    inset=1 if width==28 else 0
    firstgid=int(root.find('tileset').get('firstgid'))
    objects={o.get('name'):(int(float(o.get('x'))),int(float(o.get('y')))) for o in root.findall('./objectgroup/object')}
    spawns=[tuple(v-inset*16 for v in objects[name]) for name in ['Player','EnemyLeft','EnemyRight','HQ']]
    for x,y in spawns:
        assert x%32==16 and y%32==16 and 0<x<416 and 0<y<416
    for layer in layers:
        values=[int(v)-firstgid for v in layer.find('data').text.replace('\n','').split(',') if v.strip()]
        assert len(values)==width*height and set(values)<={9,10,6,13},'Unknown tile: use the local tiles.tsx terrain, not an external backup tileset'
        if inset:
            assert all(values[y*width+x]==9 for y in range(width) for x in range(width) if x in (0,width-1) or y in (0,width-1)),'The visible green border must remain solid wall'
            values=[values[(y+1)*width+x+1] for y in range(26) for x in range(26)]
        cells=[]
        for y in range(13):
            for x in range(13):
                sample=[values[(y*2+dy)*26+x*2+dx] for dy in (0,1) for dx in (0,1)]
                if set(sample)<={10,13}:
                    mask=sum(1<<q for q,t in enumerate(sample) if t==13)
                    cells.append(16|mask if mask else 10)
                else:
                    assert len(set(sample))==1,'Steel and headquarters cells must be intact'
                    cells.append(sample[0])
        starts=[(y//32)*13+x//32 for x,y in spawns]
        assert all(cells[s]==10 for s in starts[:3]),'Spawn must be on safe ground'
        assert cells[starts[3]]==6 and cells.count(6)==1,'Exactly one headquarters marker is required'
        seen={starts[0]};queue=deque(seen)
        while queue:
            p=queue.popleft()
            for q in (p-13,p+13,p-1,p+1):
                if 0<=q<169 and abs(q%13-p%13)+abs(q//13-p//13)==1 and cells[q]!=9 and q not in seen:
                    seen.add(q);queue.append(q)
        assert all(s in seen for s in starts),'Every entry must connect to headquarters through destructible cover'
        maps.append(cells);names.append(layer.get('name'))
    assert len({tuple(m) for m in maps})==8,'Arena layouts must be unique'
    assert all(len(n)<=20 and set(n)<=(set('ABCDEFGHIJKLMNOPQRSTUVWXYZ ')) for n in names)
    destination.parent.mkdir(parents=True,exist_ok=True)
    text='/* Generated from assets/arenas.tmx. */\nstatic const uint8_t arena_maps[8][169]={\n'+',\n'.join('{'+','.join(map(str,m))+'}' for m in maps)+'\n};\n'
    text+='const char* const arena_names[8]={'+','.join('"'+n+'"' for n in names)+'};\n'
    text+='static const int16_t arena_spawns[4][2]={'+','.join('{'+str(x)+','+str(y)+'}' for x,y in spawns)+'};\n'
    destination.write_text(text)
    print('Eight compact stages: safe entries, single eagle and headquarters routes verified.')
if __name__=='__main__':
    parser=argparse.ArgumentParser();parser.add_argument('source',type=Path);parser.add_argument('destination',type=Path)
    args=parser.parse_args();generate(args.source,args.destination)
