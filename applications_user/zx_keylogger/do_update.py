import sys, os, time
sys.path.insert(0, os.path.expanduser('~/.ufbt/current/scripts'))
from flipper.storage import FlipperStorage

port = 'COM5'
with FlipperStorage(port) as fs:
    print('Triggering reboot into update...')
    fs.send('power reboot update\r')
    time.sleep(5)
    print('Flipper should be updating now...')
    print('Waiting for it to come back...')
    time.sleep(20)
    try:
        with FlipperStorage(port, timeout=10) as fs2:
            fs2.send('device_info\r')
            r = fs2.read.until('>: ')
            for line in r.decode(errors='replace').split('\n'):
                if 'firmware' in line.lower():
                    print(line.strip())
    except Exception as e:
        print(f'Not back yet: {e}')
