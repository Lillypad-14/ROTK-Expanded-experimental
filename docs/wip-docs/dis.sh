#!/bin/bash
# usage: dis.sh addr len  -> asm + literal pool words
cd ~/Desktop/lotr/rotk
python local/wip-docs/gh_call.py disassemble_bytes address=$1 length=$2 2>&1 | grep -E '^0[0-9a-f]{7}:'
