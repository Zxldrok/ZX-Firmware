#!/usr/bin/env python3
import sys, os, argparse
sys.path.insert(0, os.path.expanduser('~/.ufbt/current/scripts'))
from flipper.storage import FlipperStorage

def main():
    parser = argparse.ArgumentParser(description='Check update file on Flipper Zero')
    parser.add_argument('--port', '-p', default='COM5', help='Serial port')
    args = parser.parse_args()

    try:
        with FlipperStorage(args.port) as fs:
            fs.send('storage stat "/ext/update/f7-update-local/update.fuf"\r')
            r = fs.read.until('>: ')
            print('update.fuf:', r.decode(errors='replace'))

            fs.send('storage list "/ext/update/f7-update-local"\r')
            r = fs.read.until('>: ')
            print('dir:', r.decode(errors='replace'))
    except Exception as e:
        print(f'Error: {e}')

if __name__ == '__main__':
    main()
