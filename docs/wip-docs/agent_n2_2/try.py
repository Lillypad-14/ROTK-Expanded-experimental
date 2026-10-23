import sys,subprocess,re,os
# usage: try.py base.c Func  variantfile(lines separated by ====) ; each variant is a python-ish replacement spec: "old|||new" pairs separated by ;;; 
base,func,specf=sys.argv[1:4]
src=open(base).read()
specs=open(specf).read().split("\n====\n")
env=dict(os.environ)
for i,sp in enumerate(specs):
    s=src
    for pair in sp.split("\n;;;\n"):
        if "|||" not in pair: continue
        o,n=pair.split("|||",1)
        if o not in s: print("MISSING",o[:40]); 
        s=s.replace(o,n)
    open("_try.c","w").write(s)
    r=subprocess.run(["uv","run","rotkit","match",func,"local/wip-docs/agent_n2_2/_try.c"],cwd="../../..",capture_output=True,text=True)
    m=re.search(r"^(DIFF|MATCH|OKSRC).*$",r.stdout,re.M)
    print(i,m.group(0)[:100] if m else (r.stdout+r.stderr)[:300])
