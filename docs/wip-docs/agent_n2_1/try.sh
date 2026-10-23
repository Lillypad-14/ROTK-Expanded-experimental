#!/bin/bash
# usage: try.sh Func 'replacement'   (tpl at agent_n2_1/<Func>.tpl, marker @@)
D=~/Desktop/lotr/rotk/local/wip-docs/agent_n2_1
python - "$1" "$2" <<'PY'
import sys
f,r=sys.argv[1:3]
D='~/Desktop/lotr/rotk/local/wip-docs/agent_n2_1/'
open(D+f+'.c','w').write(open(D+f+'.tpl').read().replace('@@',r))
PY
echo "== $2"; $D/m.sh $1 | grep -E "^(\*|MATCH|DIFF|OKSRC)"|head -${N:-12}
