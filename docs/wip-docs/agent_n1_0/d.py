import subprocess, re, sys
R='~/Desktop/lotr/rotk/'
sym={}
for f in ('config/functions.cfg','config/data.cfg'):
    for l in open(R+f):
        p=l.split()
        if len(p)>=2 and p[0].startswith('0x'):
            sym[int(p[0],16)]=p[2] if p[1]=='thumb' or p[1]=='arm' else p[1]
def gh(tool,**kw):
    return subprocess.run(['python',R+'local/wip-docs/gh_call.py',tool]+[f'{k}={v}' for k,v in kw.items()],capture_output=True,text=True).stdout
addr=sys.argv[1]; ln=sys.argv[2]
out=gh('disassemble_bytes',address=addr,length=ln)
for l in out.splitlines():
    if not re.match(r'^0[0-9a-f]{7}:',l): continue
    m=re.search(r'\[0x([0-9a-f]{8})\]',l)
    extra=''
    if m:
        a=int(m.group(1),16)
        h=gh('get_hexdump',address=hex(a),length=4)
        mm=re.search(r'^\s*[0-9a-f]+\s+((?:[0-9a-f]{2} ){4})',h,re.M)
        if mm:
            b=bytes.fromhex(mm.group(1).replace(' ',''))
            v=int.from_bytes(b,'little')
            extra=f'  ={v:#x} {sym.get(v,sym.get(v&~1,""))}'
    m=re.search(r'bl 0x([0-9a-f]{8})',l)
    if m: extra='  '+sym.get(int(m.group(1),16),'?')
    print(l+extra)
