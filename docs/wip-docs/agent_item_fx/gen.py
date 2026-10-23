import itertools,subprocess,sys,re
src=open('local/wip-docs/candidates/item_getEquipSlot.c').read()
decl_old='''    Item mainhand = PLAYER(playerIndex).inventory.slots.mainhand;
    u32 result = 8;
    Player *player = &PLAYER(playerIndex);
    u32 hero = player->heroId;
'''
decls={'m':'    Item mainhand = PLAYER(playerIndex).inventory.slots.mainhand;\n','r':'    u32 result = 8;\n','p':'    Player *player = &PLAYER(playerIndex);\n'}
res={}
for perm in itertools.permutations('mrp'):
    for hs in (0,1):
        d=''.join(decls[c] for c in perm)
        d+= '    u32 hero = player->heroId;\n' if hs==0 else '    u32 hero;\n'
        s=src.replace(decl_old,d)
        if hs==1: s=s.replace('    item.word = itemHandle;\n','    item.word = itemHandle;\n    hero = player->heroId;\n',1)
        name=f"local/wip-docs/agent_item_fx/p_{''.join(perm)}{hs}.c"
        open(name,'w').write(s)
        o=subprocess.run(['uv','run','rotkit','match','item_getEquipSlot',name],capture_output=True,text=True).stdout
        m=re.search(r'(MATCH|DIFF|OKSRC).*',o)
        print(name,m.group(0) if m else o[:100],flush=True)
