# usage: uv run python local/wip-docs/agent_m4/m.py match <F> <cand.c> --full ; extra symbols in extras.txt
import sys, os
import rotkit.compile as c
ex = {}
for l in open(os.path.join(os.path.dirname(__file__), "extras.txt")):
    p = l.split()
    if len(p) == 3:
        ex[p[0]] = (int(p[1], 16), p[2] == "f")
o = c._stores
c._stores = lambda: {**o(), **ex}
from rotkit.cli import main
sys.argv = ["rotkit"] + sys.argv[1:]
main()
