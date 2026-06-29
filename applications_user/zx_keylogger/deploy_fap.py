import sys, os
sys.path.insert(0, os.path.expanduser('~/.ufbt/current/scripts'))
from flipper.storage import FlipperStorage

fap_path = os.path.expanduser('~/.ufbt/build/zx_keylogger.fap')
dest = '/ext/apps_data/zx_keylogger/zx_keylogger.fap'
dir_path = '/ext/apps_data/zx_keylogger'

port = 'COM5'
print(f'Using port: {port}')

with FlipperStorage(port) as fs:
    if not fs.exist_dir(dir_path):
        fs.mkdir(dir_path)
    if fs.exist_file(dest):
        fs.remove(dest)
    
    md5_local = fs.hash_local(fap_path)
    print(f'Local MD5: {md5_local}')
    print(f'Sending {fap_path}')
    fs.send_file(fap_path, dest)
    
    md5_remote = fs.hash_flipper(dest)
    match = md5_local == md5_remote
    print(f'Remote MD5: {md5_remote}')
    print(f'Match: {match}')
