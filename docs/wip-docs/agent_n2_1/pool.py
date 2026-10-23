import sys,struct,re
rom=open('~/Desktop/lotr/rotk/rotk.gba','rb').read()
a=int(sys.argv[1],16)-0x08000000; n=int(sys.argv[2],0)
# print pool words referenced by ldr pc in range (thumb)
out=[]
for o in range(a,a+n,2):
    h=struct.unpack_from('<H',rom,o)[0]
    if h>>11==9:
        t=((o+4)&~3)+(h&0xff)*4
        out.append((hex(0x8000000+o),hex(0x8000000+t),hex(struct.unpack_from('<I',rom,t)[0])))
for x in out: print(*x)
