import sys, os
sys.path.insert(0, os.path.expanduser('~/.ufbt/current/scripts'))
from flipper.storage import FlipperStorage

with FlipperStorage('COM5') as fs:
    fs.send('help\r')
    r = fs.read.until('>: ')
    for line in r.decode(errors='replace').split('\n'):
        line = line.strip()
        if line:
            print(line)
