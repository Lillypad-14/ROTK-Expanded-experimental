import sys,re
addr,path=sys.argv[1],sys.argv[2]
p='config/split.cfg'
s=open(p,newline='').read()
nl='\r\n' if '\r\n' in s else '\n'
lines=s.split(nl)
key=int(addr,16)
row=f'{addr}\t{path}'
for i,l in enumerate(lines):
    m=re.match(r'0x([0-9a-fA-F]{8})\s',l)
    if m and int(m.group(1),16)>key:
        lines.insert(i,row);break
open(p,'w',newline='').write(nl.join(lines))
