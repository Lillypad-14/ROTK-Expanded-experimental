import subprocess,sys
def run(name, src):
    open(name+'.c','w').write(src)
    o=subprocess.run(['bash','./m.sh',name],capture_output=True,text=True).stdout
    return o.splitlines()[0] if o else '?'
