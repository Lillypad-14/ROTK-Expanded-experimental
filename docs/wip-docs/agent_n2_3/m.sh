#!/bin/bash
# usage: m.sh Func file  -> diff count line
export PATH="~/AppData/Local/rotk-tools/arm-gcc/bin:~/AppData/Local/rotk-tools/armips:$PATH"
cd ~/Desktop/lotr/rotk
uv run rotkit match $1 $2 ${3:---full} 2>&1 | grep -E "^(MATCH|DIFF|OKSRC|compile|.*error)" | head -5
