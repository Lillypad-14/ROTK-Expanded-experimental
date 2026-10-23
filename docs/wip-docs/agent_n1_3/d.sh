#!/bin/bash
# usage: d.sh addr size -> asm with named bl targets and literals
cd ~/Desktop/lotr/rotk
python local/wip-docs/gh_call.py disassemble_bytes address=$1 length=$(( $2 + 24 )) 2>&1 | grep -E '^0[0-9a-f]{7}:' | python -c "
import sys,re
start=int('$1',16);size=$2
names={}
rom=open('rotk.gba','rb').read()
for f in ('config/functions.cfg','config/data.cfg'):
    for l in open(f):
        p=l.split()
        if len(p)>=2 and p[0].startswith('0x'):
            names[int(p[0],16)]=p[2] if p[1]=='thumb' else p[1]
for l in sys.stdin:
    a=int(l[:8],16)
    if a>=start+size: break
    mm=re.search(r'\[(0x0[0-9a-f]{7})\]',l)
    if mm:
        v=int.from_bytes(rom[int(mm.group(1),16)-0x08000000:][:4],'little')
        l=l.rstrip()+' =%08x'%v
        if v in names: l+=' '+names[v]
    m=re.findall(r'0x0[0-9a-f]{7}',l)
    n=''
    for x in m:
        v=int(x,16)
        if v in names: n+=' ; '+names[v]
    print(l.rstrip()+n)
"
