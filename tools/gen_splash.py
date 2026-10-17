#!/usr/bin/env python3
"""Compile the editable Tiled splash to shared tile IDs and small RLE runs."""
from pathlib import Path
import sys,xml.etree.ElementTree as ET
NATIVE=set(range(16))|{16,17,24,25,32,33,40,41}|set(range(80,128))
FONT='ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789-:'
def props(obj):return {p.get('name'):p.get('value',p.text or '') for p in obj.findall('./properties/property')}
def generate(source,destination):
 r=ET.parse(source).getroot();assert (r.get('width'),r.get('height'))==('40','30'),'Splash must be 40x30 16px tiles'
 assert len(r.findall('tileset'))==1 and r.find('tileset').get('firstgid')=='1','Use only the local tiles.tsx'
 cells=[8]*1200;actors={};flash=None
 for layer in r.findall('layer'):
  if layer.get('visible')=='0':continue
  vs=[int(x) for x in layer.find('data').text.replace('\n','').split(',') if x.strip()]
  assert len(vs)==1200
  for n,gid in enumerate(vs):
   if gid:
    assert gid-1 in NATIVE,f'Tile {gid-1} is an editor-only animation pose, not a resident background tile'
    cells[n]=gid-1
 for obj in r.findall('./objectgroup/object'):
  if obj.get('visible')=='0':continue
  pr=props(obj);text=obj.find('text')
  if text is not None:
   value=(text.text or '').upper();x=int(float(obj.get('x')));y=int(float(obj.get('y')))
   assert x%16==y%16==0 and 0<=x//16<40 and 0<=y//16<30,'Place text on the 16px grid'
   assert '\n' not in value and x//16+len(value)<=40,'Use one line per text object, within 40 columns'
   assert all(c in FONT+' ' for c in value),'Font supports A-Z, 0-9, spaces, hyphen and colon'
   tiles=[15 if c==' ' else 80+FONT.index(c) for c in value];pos=(y//16)*40+x//16
   if pr.get('blink')=='true':
    assert flash is None,'Use one blinking Start prompt';flash=(x//16,y//16,cells[pos:pos+len(value)],tiles)
   cells[pos:pos+len(value)]=tiles
  elif pr.get('role')=='headquarters':
   x=int(float(obj.get('x')))//16;y=(int(float(obj.get('y')))-32)//16
   assert 0<=x<39 and 0<=y<29
   for q in range(4):cells[(y+q//2)*40+x+q%2]=160+q
  elif pr.get('role')=='tank':
   slot=int(pr['slot']);x=int(float(obj.get('x')))+16;y=int(float(obj.get('y')))-16
   heading=int(pr['heading']);distance=int(pr.get('shot_distance','0'));patrol=int(pr.get('patrol','0'))
   assert slot in range(3) and slot not in actors and heading in (0,4,8,12)
   assert 16<=x<=624 and 16<=y<=464 and 0<=patrol<=16 and distance>=0
   dx,dy={0:(1,0),4:(0,1),8:(-1,0),12:(0,-1)}[heading]
   assert 16<=x+dx*max(distance,patrol)<=624 and 16<=y+dy*max(distance,patrol)<=464,'Tank patrol/shot endpoint leaves the screen'
   actors[slot]=[x,y,heading,distance,patrol]
 assert len(actors)==3 and flash and flash[3],'Keep three tank objects (slots 0-2) and one nonempty blinking Start text object'
 runs=[]
 for tile in cells:
  if runs and runs[-1][1]==tile and runs[-1][0]<255:runs[-1][0]+=1
  else:runs.append([1,tile])
 def arr(name,values,typ='uint8_t'):return f'static const {typ} {name}[]={{'+','.join(map(str,values))+'};\n'
 x,y,off,on=flash
 out='/* Generated from assets/splash.tmx: edit the TMX, not this header. */\n'
 out+=arr('splash_runs',[v for pair in runs for v in pair])
 out+='static const int16_t splash_actors[3][5]={'+','.join('{'+','.join(map(str,actors[n]))+'}' for n in range(3))+'};\n'
 out+=f'#define SPLASH_FLASH_X {x}\n#define SPLASH_FLASH_Y {y}\n#define SPLASH_FLASH_LENGTH {len(on)}\n'
 out+=arr('splash_flash_on',on)+arr('splash_flash_off',off)
 destination.parent.mkdir(parents=True,exist_ok=True);destination.write_text(out)
 print(f'Editable splash: {len(runs)*2} map bytes; {len(NATIVE)} shared native source tiles, no new font or title graphics.')
if __name__=='__main__':generate(*(Path(p) for p in sys.argv[1:]))
