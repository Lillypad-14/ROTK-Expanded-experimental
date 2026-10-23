import itertools
import re
import subprocess
from pathlib import Path

source = Path('src/c/sprite_free.c').read_text()
for kept_count in range(4):
    for kept in itertools.combinations(('flagsRead', 'flags', 'spriteFlags'), kept_count):
        def replace(match):
            ctype, name, register = match.groups()
            return match.group(0) if name in kept else f'{ctype} {name}'
        candidate = re.sub(r'MATCH_PIN\(([^,]+), (\w+), "([^"]+)"\)', replace, source)
        path = Path('local/wip-docs/free-pin-variant.c')
        path.write_text(candidate)
        result = subprocess.run(['uv', 'run', 'rotkit', 'match', 'sprite_free', str(path)], capture_output=True, text=True)
        verdict = next((line for line in result.stdout.splitlines() if line.startswith(('MATCH', 'DIFF'))), result.stderr)
        print(kept, verdict, flush=True)
