#!/usr/bin/env python3
import struct
import zlib
import os

def create_png(width, height, pixels):
    def chunk(chunk_type, data):
        c = chunk_type + data
        return struct.pack('>I', len(data)) + c + struct.pack('>I', zlib.crc32(c) & 0xffffffff)

    raw = b''
    for row in pixels:
        raw += b'\x00'
        for r, g, b, a in row:
            raw += struct.pack('BBBB', r, g, b, a)

    ihdr = struct.pack('>IIBBBBB', width, height, 8, 6, 0, 0, 0)
    return b'\x89PNG\r\n\x1a\n' + chunk(b'IHDR', ihdr) + chunk(b'IDAT', zlib.compress(raw)) + chunk(b'IEND', b'')

# Pixel data for frames
def make_frame(offset):
    pixels = []
    for y in range(14):
        row = []
        for x in range(14):
            r, g, b, a = 0, 0, 0, 0
            cx, cy = 7, 7
            dx, dy = x - cx, y - cy
            dist = (dx*dx + dy*dy) ** 0.5

            if dist < 6.0:
                r, g, b, a = 70, 190, 50, 255
            if 5.5 < dist < 6.5:
                r, g, b, a = 35, 130, 25, 255

            if dist < 6.0:
                # Eyes
                ex = 4 if offset == 0 else 3 if offset == 1 else 5
                if x in [ex, 14-ex-1] and y in [3, 4]:
                    r, g, b, a = 255, 255, 255, 255
                if x in [ex, 14-ex-1] and y == 4:
                    r, g, b, a = 0, 0, 0, 255

                # Mouth
                if y == 9 and x in range(4, 11):
                    r, g, b, a = 255, 255, 255, 255
                if y == 10 and x in range(5, 10):
                    r, g, b, a = 255, 0, 0, 255

                # Eyebrows
                if y == 2 and x in [3, 11]:
                    r, g, b, a = 20, 80, 20, 255

            row.append((r, g, b, a))
        pixels.append(row)
    return pixels

out_dir = os.path.join(os.path.dirname(__file__), '..', 'assets', 'icons', 'MainMenu', 'Troll_14')
os.makedirs(out_dir, exist_ok=True)

# frame_rate
with open(os.path.join(out_dir, 'frame_rate'), 'w') as f:
    f.write('3\n')

# Generate frames
for i in range(8):
    px = make_frame(i % 3)
    png = create_png(14, 14, px)
    path = os.path.join(out_dir, f'frame_{i+1:02d}.png')
    with open(path, 'wb') as f:
        f.write(png)
    print(f'Created {path} ({len(png)} bytes)')

print('Done!')
