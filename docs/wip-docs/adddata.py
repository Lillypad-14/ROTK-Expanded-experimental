import sys
# usage: adddata.py data|functions addr name [type]
kind,addr,name=sys.argv[1:4]; typ=sys.argv[4] if len(sys.argv)>4 else 'thumb'
a=int(addr,16); p='config/%s.cfg'%kind
L=open(p).read().replace('\r','').split('\n')
row='%s\t%s\t%s'%(addr,name if kind=='data' else typ,typ if kind=='data' else name) if kind=='data' else '%s\t%s\t%s'%(addr,typ,name)
if kind=='data': row='%s\t%s\t%s'%(addr,name,typ)
for i,l in enumerate(L):
    q=l.split('\t')
    if len(q)>=3 and q[0].startswith('0x') and int(q[0],16)>a and (int(q[0],16)>>24)==(a>>24):
        L.insert(i,row); break
else: raise SystemExit('no slot')
open(p,'w',newline='').write('\n'.join(L))
