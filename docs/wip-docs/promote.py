import re,sys,subprocess,os
d=sys.argv[1]; names=sys.argv[2:]
for n in names:
    s=open('local/wip-docs/%s/%s.c'%(d,n),encoding='utf8').read().replace('\r','')
    addr=re.search(r'@romaddress (0x[0-9a-fA-F]+)',s).group(1)
    s=re.sub(r'^#define '+n+r' hdr_dummy\n','',s,flags=re.M)
    s=re.sub(r'^#undef '+n+r'\n','',s,flags=re.M)
    open('src/c/%s.c'%n,'w',newline='\n').write(s)
    subprocess.run(['python','local/wip-docs/addsplit.py',addr,'src/c/%s.c'%n])
subprocess.run(['python','local/wip-docs/strip.py']+names)
