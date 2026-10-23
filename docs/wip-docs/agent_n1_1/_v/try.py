import sys
f = sys.argv[1]
s = open(sys.argv[2]).read()
a = sys.argv[3:]
for i in range(0, len(a), 2):
    assert a[i] in s, a[i]
    s = s.replace(a[i], a[i + 1].replace(chr(92) + 'n', chr(10)))
open('~/Desktop/lotr/rotk/local/wip-docs/agent_n1_1/%s.c' % f, 'w').write(s)
