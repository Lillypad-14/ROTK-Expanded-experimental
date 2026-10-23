import sys,subprocess,re
src=open(sys.argv[1]).read()
for k,(a,b) in enumerate(eval(open(sys.argv[2]).read())):
    s=src.replace(a,b)
    open('t_v.c','w').write(s)
    r=subprocess.run(['bash','-c','export PATH="~/AppData/Local/rotk-tools/arm-gcc/bin:~/AppData/Local/rotk-tools/armips:$PATH"; cd ~/Desktop/lotr/rotk; uv run rotkit match %s local/wip-docs/agent_n2_0/t_v.c 2>&1 | grep "DIFF\|MATCH\|error"'%sys.argv[3]],capture_output=True,text=True)
    print(k,r.stdout.strip()[:150])
