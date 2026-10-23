#!/bin/bash
# usage: m.sh FUNC file.c [--full]
export PATH="~/AppData/Local/rotk-tools/arm-gcc/bin:~/AppData/Local/rotk-tools/armips:$PATH"
cd ~/Desktop/lotr/rotk
uv run rotkit match "$@" 2>&1
