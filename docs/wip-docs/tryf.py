import subprocess,sys
def run(fn,body,inc=''):
    open('local/wip-docs/n/v.c','w').write(inc+'\n'+body)
    r=subprocess.run(f'uv run rotkit match {fn} local/wip-docs/n/v.c',shell=True,capture_output=True,text=True)
    print(fn,[l[:90] for l in r.stdout.splitlines() if l.startswith(('MATCH','DIFF','OKSRC'))] or (r.stdout+r.stderr)[-400:])
