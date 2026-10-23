#!/bin/bash
export PATH="~/AppData/Local/rotk-tools/arm-gcc/bin:~/AppData/Local/rotk-tools/armips:$PATH"
cd ~/Desktop/lotr/rotk
d=${D:-local/wip-docs/agent_n2_2}
for f in "$@"; do echo "## $f"; uv run rotkit match $f $d/$f.c ${FULL:+--full} 2>&1 | grep -v "no previous prototype\|implicit declaration\|In function" | head -${LINES_MAX:-30}; done
