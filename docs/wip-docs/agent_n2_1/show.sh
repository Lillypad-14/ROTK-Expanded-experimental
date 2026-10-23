#!/bin/bash
cd ~/Desktop/lotr/rotk
a=$(grep -w "$1" local/wip-docs/n2/list1.txt | awk '{print $1, $3}')
set -- $1 $a
python local/wip-docs/gh_call.py get_pseudocode function=$2 2>&1 | grep -v "^\[Context\]\|WARNING"
bash local/wip-docs/dis.sh $2 $3 | sed 's/^0[0-9a-f]*: //' | nl -v0 | head -${DL:-400}
echo POOL; python local/wip-docs/agent_n2_1/pool.py $2 $3
