#!/usr/bin/env python3
import sys, os, time, serial
import argparse

def read_until(ser, prompt, timeout=10):
    data = b''
    ser.timeout = timeout
    while True:
        c = ser.read(1)
        if not c:
            break
        data += c
        if data.endswith(prompt):
            break
    return data

def main():
    parser = argparse.ArgumentParser(description='Flash firmware over serial CLI')
    parser.add_argument('--port', '-p', default='COM5', help='Serial port')
    parser.add_argument('--fuf', default='/ext/update/f7-update-local/update.fuf', help='Path to update.fuf on device')
    args = parser.parse_args()

    print(f'Connecting to {args.port}...')
    ser = serial.Serial(args.port, baudrate=115200, timeout=10)
    time.sleep(0.5)

    ser.reset_input_buffer()
    data = read_until(ser, b'>: ', 2)
    print(f'Initial: {data.decode(errors="replace")}')

    cmd = f'update install {args.fuf}\r'
    print(f'Sending: {cmd.strip()}')
    ser.write(cmd.encode())
    time.sleep(1)
    data = read_until(ser, b'>: ', 30)
    print(f'Response: {data.decode(errors="replace")}')

    if b'OK' in data:
        print('Update installed successfully. Rebooting...')
        ser.write(b'power reboot\r')
        time.sleep(2)
        print('Reboot command sent.')

    print('Waiting 45s for Flipper to reboot and update...')
    ser.close()
    time.sleep(45)

    print('Reconnecting...')
    ser = serial.Serial(args.port, baudrate=115200, timeout=5)
    time.sleep(1)
    ser.reset_input_buffer()
    data = read_until(ser, b'>: ', 5)
    print(f'After reboot: {data.decode(errors="replace")}')

    ser.write(b'device_info\r')
    time.sleep(0.5)
    data = read_until(ser, b'>: ', 5)
    for line in data.decode(errors='replace').split('\n'):
        if 'firmware' in line.lower():
            print(line.strip())

    ser.close()

if __name__ == '__main__':
    main()
