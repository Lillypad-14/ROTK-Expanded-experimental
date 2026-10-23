import sys,glob,subprocess
a=int(sys.argv[1],16); n=int(sys.argv[2])
rom=open(glob.glob('../Lord of the Rings*.gba')[0],'rb').read()
open('local/wip-docs/_raw.bin','wb').write(rom[a-0x08000000:a-0x08000000+n])
print(subprocess.run(['arm-none-eabi-objdump','-D','-b','binary','-m','arm','-M','force-thumb',f'--adjust-vma={hex(a)}','local/wip-docs/_raw.bin'],capture_output=True,text=True).stdout.split('<.data>:')[-1])
