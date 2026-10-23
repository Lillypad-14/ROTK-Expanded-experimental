#!/bin/bash
cd ~/Desktop/lotr/rotk
export PATH="~/AppData/Local/rotk-tools/arm-gcc/bin:~/AppData/Local/rotk-tools/armips:$PATH"
uv run python local/wip-docs/agent_fx_spawn/m.py match fx_spawnOnActor "$@" 2>&1
