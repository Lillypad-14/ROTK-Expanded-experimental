#!/bin/bash
# w.sh addr len : print words as little-endian
cd ~/Desktop/lotr/rotk
python local/wip-docs/gh_call.py get_hexdump address=$1 length=${2:-4} 2>&1 | head -5
