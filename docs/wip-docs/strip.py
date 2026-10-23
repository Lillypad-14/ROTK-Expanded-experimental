import sys,re,glob
for name in sys.argv[1:]:
    for h in glob.glob('include/*.h'):
        s=open(h,newline='').read()
        for m in re.finditer(r'/\*\*(?:(?!\*/).)*?\*/[ \t]*\r?\n(?=[ \t]*[\w][\w \*]*\b'+re.escape(name)+r'\([^\n]*\);)',s,re.S):
            s=s[:m.start()]+s[m.end():]; open(h,'w',newline='').write(s); print('stripped',name,'in',h); break
