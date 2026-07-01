#!/usr/bin/env python3
import sys, os, argparse
sys.path.insert(0, os.path.expanduser('~/.ufbt/current/scripts'))
from flipper.storage import FlipperStorage

def main():
    parser = argparse.ArgumentParser(description='Fetch CLI help from Flipper Zero')
    parser.add_argument('--port', '-p', default='COM5', help='Serial port')
    args = parser.parse_args()

    with FlipperStorage(args.port) as fs:
        fs.send('help\r')
        r = fs.read.until('>: ')
        for line in r.decode(errors='replace').split('\n'):
            line = line.strip()
            if line:
                print(line)

if __name__ == '__main__':
    main()
