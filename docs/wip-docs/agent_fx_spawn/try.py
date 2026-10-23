import subprocess,sys,re
def score(src,name='t.c'):
    open(name,'w').write(src)
    r=subprocess.run(["bash","run.sh","local/wip-docs/agent_fx_spawn/"+name,"--full"],capture_output=True,text=True).stdout
    m=re.search(r'(\d+) instruction',r)
    return (int(m.group(1)) if m else (0 if r.startswith('MATCH') else r[:100]))
