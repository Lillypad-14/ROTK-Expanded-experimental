#!/bin/bash
export PATH="~/AppData/Local/rotk-tools/arm-gcc/bin:~/AppData/Local/rotk-tools/armips:$PATH"
cd ~/Desktop/lotr/rotk
for f in "$@"; do echo "## $f"; uv run rotkit match $f $CC local/wip-docs/agent_n1_0/$f.c --full 2>&1 | grep -v "no previous prototype\|implicit declaration\|In function" | head -${LINES_MAX:-40}; done
