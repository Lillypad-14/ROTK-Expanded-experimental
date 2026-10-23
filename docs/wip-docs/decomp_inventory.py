import hashlib
import json
from pathlib import Path

from rotkit.decomp import defined_in_c
from rotkit.paths import ROOT
from rotkit.rom import regions
from rotkit.stores import func_symbols

defined = defined_in_c()
symbols = func_symbols()
code = [r for r in regions() if (ROOT / r.src).is_relative_to(ROOT / 'src/c')]
report = {
    'decompiled_functions': len(defined),
    'mapped_functions': len(symbols),
    'compiled_code_bytes': sum(Path(r.bin_path).stat().st_size for r in code),
    'source_regions': len(code),
    'rom_sha1': hashlib.sha1((ROOT / 'build/rom.gba').read_bytes()).hexdigest(),
    'remaining_mapped_functions': [
        {'address': f'0x{f.addr:08x}', 'mode': f.mode, 'name': f.name}
        for f in symbols if f.name not in defined
    ],
}
out = ROOT / 'local/wip-docs/decomp-inventory.json'
out.write_text(json.dumps(report, indent=2) + '\n')
print(json.dumps({k: v for k, v in report.items() if k != 'remaining_mapped_functions'}, indent=2))
