"""How much of ROTK's code also exists in TTT: per decomp name prefix, the ROTK functions whose first
12 instructions (calls and literal loads masked) occur in the TTT ROM."""

import collections

from roms import ROM_BASE, ROTK_ROM, TTT_ROM, load, masked_thumb, rotk_functions, shape_index

K = 12

rotk = load(ROTK_ROM)
ttt = load(TTT_ROM)
index = shape_index(ttt, K)
funcs = rotk_functions()
per_prefix = collections.defaultdict(lambda: [0, 0])
found = total = 0
for i, (address, name) in enumerate(funcs):
    end = funcs[i + 1][0] if i + 1 < len(funcs) else address + 64
    if end - address < 2 * K or address >= 0x08400000:
        continue
    total += 1
    prefix = name.split("_")[0]
    per_prefix[prefix][1] += 1
    if masked_thumb(rotk, address - ROM_BASE, K) in index:
        found += 1
        per_prefix[prefix][0] += 1

print(f"{found}/{total} ROTK functions have their first {K} instructions in TTT")
for prefix, (hit, count) in sorted(per_prefix.items(), key=lambda kv: -kv[1][1])[:24]:
    print(f"  {prefix or '(unnamed)':14} {hit:4}/{count}")
