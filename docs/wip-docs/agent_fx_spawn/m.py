import sys
sys.path.insert(0, "tools")
from rotkit import compile as c
o = c._stores
def s():
    d = o()
    d["actor_unk0800103c"] = (0x0800103c, True)
    d["g_FxDescTable"] = (0x08287b7c, False)
    return d
c._stores = s
from rotkit.cli import app
app()
