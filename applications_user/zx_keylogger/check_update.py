import sys, os
sys.path.insert(0, os.path.expanduser('~/.ufbt/current/scripts'))
from flipper.storage import FlipperStorage

try:
    with FlipperStorage('COM5') as fs:
        fs.send('storage stat "/ext/update/f7-update-local/update.fuf"\r')
        r = fs.read.until('>: ')
        print('update.fuf:', r.decode(errors='replace'))

        fs.send('storage list "/ext/update/f7-update-local"\r')
        r = fs.read.until('>: ')
        print('dir:', r.decode(errors='replace'))
except Exception as e:
    print(f'Error: {e}')
