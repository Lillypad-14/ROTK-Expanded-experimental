import concurrent.futures
import json
from pathlib import Path
import subprocess
import sys

CLIENT = Path.home() / "AppData/Local/rotk-tools/gh.py"
OUT = Path(__file__).parent

def inspect(address):
    tool = "get_pseudocode"
    if address.startswith("disasm:"):
        tool = "get_disassembly"
        address = address.removeprefix("disasm:")
    result = subprocess.run(
        [sys.executable, "-I", str(CLIENT), tool, json.dumps({"function": address})],
        capture_output=True, text=True, encoding="utf-8", errors="replace", check=True,
    )
    (OUT / f"ghidra-{tool}-{address}.txt").write_text(result.stdout, encoding="utf-8")
    return address, result.stdout

with concurrent.futures.ThreadPoolExecutor() as executor:
    for address, result in executor.map(inspect, sys.argv[1:]):
        print(address, result)
