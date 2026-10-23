import json
import runpy
import sys

tool = sys.argv[1]
args = dict(arg.split('=', 1) for arg in sys.argv[2:])
for key in args:
    if key in {'length', 'limit', 'offset', 'depth', 'clear_length'}:
        args[key] = int(args[key], 0)
sys.argv = ['gh.py', tool, json.dumps(args)]
runpy.run_path('~/AppData/Local/rotk-tools/gh.py', run_name='__main__')
