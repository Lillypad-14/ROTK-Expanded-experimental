import sys,struct
d=open('~/Desktop/lotr/rotk/build/rom.gba','rb').read()
for a in sys.argv[1:]:
    a=int(a,16); print(hex(a), hex(struct.unpack_from('<I',d,a-0x08000000)[0]))
