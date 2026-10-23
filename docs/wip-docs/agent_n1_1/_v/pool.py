import sys,struct
d=open('~/Desktop/lotr/rotk/rotk.gba','rb').read()
for a in sys.argv[1:]:
    o=int(a,16)&0xffffff
    v=struct.unpack('<I',d[o:o+4])[0]
    print(a,hex(v))
