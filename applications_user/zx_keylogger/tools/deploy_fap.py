#!/usr/bin/env python3
import sys, os, argparse
sys.path.insert(0, os.path.expanduser('~/.ufbt/current/scripts'))
from flipper.storage import FlipperStorage

def main():
    parser = argparse.ArgumentParser(description='Deploy FAP to Flipper Zero')
    parser.add_argument('--port', '-p', default='COM5', help='Serial port')
    parser.add_argument('--fap', default=os.path.expanduser('~/.ufbt/build/zx_keylogger.fap'), help='Local .fap path')
    parser.add_argument('--dest', default='/ext/apps_data/zx_keylogger/zx_keylogger.fap', help='Remote destination')
    args = parser.parse_args()

    print(f'Using port: {args.port}')
    print(f'Local FAP: {args.fap}')
    dest = args.dest
    dir_path = '/'.join(dest.split('/')[:-1])

    with FlipperStorage(args.port) as fs:
        if not fs.exist_dir(dir_path):
            fs.mkdir(dir_path)
        if fs.exist_file(dest):
            fs.remove(dest)

        md5_local = fs.hash_local(args.fap)
        print(f'Local MD5: {md5_local}')
        print(f'Sending {args.fap}')
        fs.send_file(args.fap, dest)

        md5_remote = fs.hash_flipper(dest)
        match = md5_local == md5_remote
        print(f'Remote MD5: {md5_remote}')
        print(f'Match: {match}')

if __name__ == '__main__':
    main()
