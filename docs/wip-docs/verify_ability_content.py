from pathlib import Path
import importlib.util, struct
root=Path.cwd();spec=importlib.util.spec_from_file_location('validator',root/'hacks/tools/validate_abilities.py');m=importlib.util.module_from_spec(spec);spec.loader.exec_module(m)
elf=(root/'build/hack.elf').read_bytes();rom=(root/'rotk.gba').read_bytes()
def content(): return m.LinkedContent(elf,rom)
c=content();a,n=c.symbols['SkillExpansionValidationSchema'];schema=dict(zip(m.SCHEMA,struct.unpack(f'<{len(m.SCHEMA)}I',c.read(a,n))))
kit=c.pointer(c.symbols['HeroKits'][0]+8*4);icon=c.pointer(kit+schema['kit_icons']);active=c.pointer(kit+schema['kit_active']);levels=c.pointer(kit+schema['kit_levels']);new=c.pointer(kit+schema['kit_new'])
cases=[('missing icon',icon,b'\0'*4),('missing callback',active+schema['active_size']+schema['active_callback'],b'\0'*4),('bad base row',new+schema['def_base'],b'\x07'),('broken curve',levels+schema['levels']-1,b'\0'),('invalid passive',kit+schema['kit_new_passives'],b'\xfe')]
assert m.validate(c)==16
for name,address,value in cases:
 c=content()
 for i,(start,data) in enumerate(c.regions):
  if start<=address and address+len(value)<=start+len(data):
   pos=address-start;c.regions[i]=(start,data[:pos]+value+data[pos+len(value):]);break
 try: m.validate(c)
 except ValueError as error: print('PASS rejected',name,str(error))
 else: raise AssertionError(name)
