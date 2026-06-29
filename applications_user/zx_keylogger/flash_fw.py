import sys, os, time, serial

port = 'COM5'
print(f'Connecting to {port}...')

ser = serial.Serial(port, baudrate=115200, timeout=10)
time.sleep(0.5)

def read_until(prompt, timeout=10):
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

# Flush any pending data
ser.reset_input_buffer()

# Wait for prompt
data = read_until(b'>: ', 2)
print(f'Initial: {data.decode(errors="replace")}')

# Run update install
cmd = b'update install /ext/update/f7-update-local/update.fuf\r'
print(f'Sending: {cmd.decode().strip()}')
ser.write(cmd)
time.sleep(1)
data = read_until(b'>: ', 30)
print(f'Response: {data.decode(errors="replace")}')

# Check if we should reboot
if b'OK' in data:
    print('Update installed successfully. Rebooting...')
    ser.write(b'power reboot\r')
    time.sleep(2)
    print('Reboot command sent.')
    
print('Waiting 45s for Flipper to reboot and update...')
ser.close()
time.sleep(45)

# Reconnect and check
print('Reconnecting...')
ser = serial.Serial(port, baudrate=115200, timeout=5)
time.sleep(1)
ser.reset_input_buffer()
data = read_until(b'>: ', 5)
print(f'After reboot: {data.decode(errors="replace")}')

ser.write(b'device_info\r')
time.sleep(0.5)
data = read_until(b'>: ', 5)
for line in data.decode(errors='replace').split('\n'):
    if 'firmware' in line.lower():
        print(line.strip())

ser.close()
