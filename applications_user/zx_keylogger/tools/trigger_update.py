#!/usr/bin/env python3
import sys, os, argparse
sys.path.insert(0, os.path.expanduser('~/.ufbt/current/scripts'))
from flipper.storage import FlipperStorage

def main():
    parser = argparse.ArgumentParser(description='Trigger firmware update on Flipper Zero')
    parser.add_argument('--port', '-p', default='COM5', help='Serial port')
    parser.add_argument('--manifest', default='/ext/update/f7-update-local/update.fuf', help='Path to update manifest')
    args = parser.parse_args()

    print(f'Connecting to {args.port}...')
    with FlipperStorage(args.port) as fs:
        cmd = f'update install {args.manifest}\r'
        print(f'Sending: {cmd.strip()}')
        fs.send_and_wait_eol(cmd)
        result = fs.read.until(fs.CLI_EOL)
        print(f'Response: {result.decode(errors="replace")}')
        if b'Verifying' in result:
            result2 = fs.read.until(fs.CLI_EOL)
            print(f'Status: {result2.decode(errors="replace")}')

if __name__ == '__main__':
    main()
