from pathlib import Path
import sys

namespace = {}
source = Path('~/AppData/Local/rotk-tools/gh.py').read_text()
exec(source[:source.index('\nrpc("initialize"')], namespace)
rpc = namespace['rpc']
rpc('initialize', {'protocolVersion': '2025-03-26', 'capabilities': {}, 'clientInfo': {'name': 'schema', 'version': '1'}})
rpc('notifications/initialized', None, None)
for tool in rpc('tools/list', {}, 2)['result']['tools']:
    if tool['name'] in sys.argv[1:]:
        print(namespace['json'].dumps(tool, indent=2))
