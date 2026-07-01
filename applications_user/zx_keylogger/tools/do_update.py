#!/usr/bin/env python3
import sys, os, time, argparse
sys.path.insert(0, os.path.expanduser('~/.ufbt/current/scripts'))
from flipper.storage import FlipperStorage

def main():
    parser = argparse.ArgumentParser(description='Reboot Flipper Zero into update mode')
    parser.add_argument('--port', '-p', default='COM5', help='Serial port')
    args = parser.parse_args()

    with FlipperStorage(args.port) as fs:
        print('Triggering reboot into update...')
        fs.send('power reboot update\r')
        time.sleep(5)
        print('Flipper should be updating now...')
        print('Waiting for it to come back...')
        time.sleep(20)
        try:
            with FlipperStorage(args.port, timeout=10) as fs2:
                fs2.send('device_info\r')
                r = fs2.read.until('>: ')
                for line in r.decode(errors='replace').split('\n'):
                    if 'firmware' in line.lower():
                        print(line.strip())
        except Exception as e:
            print(f'Not back yet: {e}')

if __name__ == '__main__':
    main()
