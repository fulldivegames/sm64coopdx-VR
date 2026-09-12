"""Check zipper clearance against the locally decoded native torso triangles."""
from pathlib import Path
import re, struct
root=Path(__file__).resolve().parents[1]
source=(root/'actors/mario/model.inc.c').read_text()
data=(root/'build/propeller-fit-mario.bin').read_bytes()
vertices={}
for name,offset,size in re.findall(r'ROM_ASSET_LOAD_VTX\((\w+),\s*0x[\da-f]+,\s*\d+,\s*(0x[\da-f]+),\s*(\d+)\)',source):
    vertices[name]=[struct.unpack_from('>hhh',data,int(offset,16)+i) for i in range(0,int(size),16)]
triangles=[]
for name in ('mario_pants_overalls_shared_dl','mario_tshirt_shared_dl','mario_yellow_button_dl'):
    body=re.search(r'const Gfx '+name+r'\[\] = \{(.*?)\n\};',source,re.S).group(1)
    current=[]
    for command,args in re.findall(r'(gsSPVertex|gsSP1Triangle|gsSP2Triangles)\((.*?)\)',body,re.S):
        args=[x.strip() for x in args.split(',')]
        if command=='gsSPVertex':current=vertices[args[0]]
        else:
            for j in range(0,len(args),4):triangles.append([current[int(x,0)] for x in args[j:j+3]])
for x,y in [(100,4),(82,40),(47,78),(28,88),(9,92),(-15,91),(-43,86)]:
    surfaces=[]
    for a,b,c in triangles:
        den=(b[2]-c[2])*(a[0]-c[0])+(c[0]-b[0])*(a[2]-c[2])
        if not den:continue
        u=((b[2]-c[2])*(x-c[0])+(c[0]-b[0])*(-c[2]))/den
        v=((c[2]-a[2])*(x-c[0])+(a[0]-c[0])*(-c[2]))/den
        w=1-u-v
        if min(u,v,w)>=-1e-6:surfaces.append(u*a[1]+v*b[1]+w*c[1])
    print(x,'zipper',y,'front surface',max(surfaces) if surfaces else None)
