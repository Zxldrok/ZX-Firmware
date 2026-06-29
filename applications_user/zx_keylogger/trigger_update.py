import sys, os
sys.path.insert(0, os.path.expanduser('~/.ufbt/current/scripts'))
from flipper.storage import FlipperStorage

port = 'COM5'
manifest = '/ext/update/f7-update-local/update.fuf'

print(f'Connecting to {port}...')
with FlipperStorage(port) as fs:
    print(f'Sending: update install {manifest}')
    fs.send_and_wait_eol(f'update install {manifest}\r')
    result = fs.read.until(fs.CLI_EOL)
    print(f'Response: {result.decode(errors="replace")}')
    if b'Verifying' in result:
        result2 = fs.read.until(fs.CLI_EOL)
        print(f'Status: {result2.decode(errors="replace")}')
