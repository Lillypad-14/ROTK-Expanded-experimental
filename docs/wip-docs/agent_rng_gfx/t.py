import subprocess,sys
def run(fn,path):
    r=subprocess.run(f'uv run rotkit match {fn} {path}',shell=True,capture_output=True,text=True)
    return [l[:100] for l in r.stdout.splitlines() if l.startswith(('MATCH','DIFF','OKSRC'))] or (r.stdout+r.stderr)[-400:]
if __name__=='__main__':
    print(run(sys.argv[1],sys.argv[2]))
