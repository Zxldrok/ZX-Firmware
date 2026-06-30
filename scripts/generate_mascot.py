"""
Zx Mascot animation generator v2.
Generates 1-bit 128x64 PNG animation frames with frame-to-frame motion,
breathing, blinking, and expressive body language.
"""
import math
import os
from PIL import Image, ImageDraw

OUT_DIR = os.path.join(os.path.dirname(__file__), "..", "assets", "dolphin", "external")
INTERNAL_DIR = os.path.join(os.path.dirname(__file__), "..", "assets", "dolphin", "internal")
BLOCKING_DIR = os.path.join(os.path.dirname(__file__), "..", "assets", "dolphin", "blocking")
W, H = 128, 64


def breath_offset(t, amp=1):
    return int(math.sin(t * math.pi / 2) * amp)


def blink_open(t, cycle=6, at=5):
    return 1.0 if t % cycle != at else 0.05


class ZxBot:
    """A simple rectangular robot mascot with expressive features."""

    def __init__(self, draw, ox, oy):
        self.d = draw
        self.ox = ox
        self.oy = oy

    def body(self, bw=28, bh=32):
        x0 = self.ox - bw // 2
        y0 = self.oy - bh
        self.d.rectangle([x0, y0, x0 + bw, y0 + bh], fill=0, outline=0)
        return x0, y0, bw, bh

    def head(self, hw=18, hh=18, hx=0, hy=-32):
        cx = self.ox + hx
        cy = self.oy + hy
        x0 = cx - hw // 2
        y0 = cy - hh // 2
        self.d.rectangle([x0, y0, x0 + hw, y0 + hh], fill=0, outline=0)
        return cx, cy, hw, hh

    def eyes(self, cx, cy, hw, open_pct=1.0, look_left=False, look_up=False):
        eye_spacing = hw * 0.3
        eye_y = cy - hw * 0.1
        for side in [-1, 1]:
            ex = cx + int(side * eye_spacing) + (-1 if look_left else 0)
            ey = eye_y + (-1 if look_up else 0)
            if open_pct > 0.1:
                r = max(1, int(2 * open_pct))
                self.d.ellipse([ex - r, ey - r, ex + r, ey + r], fill=0)
            else:
                self.d.line([ex - 2, ey, ex + 2, ey], fill=0)

    def mouth(self, cx, cy, hw, happy=True, open_pct=0):
        mx = cx
        my = cy + hw * 0.25
        half_w = hw * 0.2
        if open_pct > 0.5:
            r = int(half_w)
            self.d.ellipse([mx - r, my - r, mx + r, my + r], fill=0)
        elif happy:
            self.d.arc([mx - half_w, my - 2, mx + half_w, my + 3], 0, 180, fill=0)
        else:
            self.d.arc([mx - half_w, my - 3, mx + half_w, my + 2], 180, 360, fill=0)

    def antenna(self, hx=0, hy=-32, hw=18, bend=0):
        ax = self.ox + hx
        ay = self.oy + hy - hw // 2 - 4
        self.d.line([ax, ay + 4, ax + bend, ay - 6], fill=0, width=1)
        self.d.ellipse([ax + bend - 2, ay - 10, ax + bend + 2, ay - 6], fill=0)

    def arm(self, side, angle=0, bw=28, bh=32):
        x0 = self.ox + (side * bw // 2)
        y0 = self.oy - bh * 0.7
        dx = side * (8 + angle * 2)
        dy = 8 - angle * 3
        self.d.line([x0, y0, x0 + dx, y0 + dy], fill=0, width=2)

    def leg(self, side, spread=0, bw=28):
        x0 = self.ox + side * bw * 0.25
        y0 = self.oy
        x1 = x0 + side * (2 + spread)
        y1 = y0 + 6
        self.d.line([x0, y0, x1, y1], fill=0, width=2)
        self.d.line([x1, y1, x1 + side * 3, y1], fill=0, width=2)

    def draw_angry_eyes(self, cx, cy, hw, shake_x=0):
        eye_spacing = hw * 0.3
        eye_y = cy - hw * 0.1
        for side in [-1, 1]:
            ex = cx + int(side * eye_spacing) + shake_x
            ex2 = ex + side * 4
            self.d.line([ex - 2, eye_y - 2, ex2, eye_y + 2], fill=0)


def make_animation(name, frames_func, meta):
    """Generate an animation directory with PNG frames and meta.txt."""
    anim_dir = os.path.join(OUT_DIR, name)
    os.makedirs(anim_dir, exist_ok=True)

    frames = frames_func()
    for i, img in enumerate(frames):
        path = os.path.join(anim_dir, f"frame_{i}.png")
        img.save(path)

    active = meta.get('active', 0)
    lines = [
        "Filetype: Flipper Animation",
        "Version: 1",
        "",
        f"Width: {W}",
        f"Height: {H}",
        f"Passive frames: {meta.get('passive', len(frames))}",
        f"Active frames: {active}",
        f"Frames order: {' '.join(str(i) for i in range(len(frames)))}",
        f"Active cycles: {meta.get('cycles', 0)}",
        f"Frame rate: {meta.get('rate', 3)}",
        f"Duration: {meta.get('duration', 3600)}",
        f"Active cooldown: {meta.get('cooldown', 0)}",
    ]

    bubbles = meta.get('bubbles', [])
    lines.append("")
    lines.append(f"Bubble slots: {len(bubbles)}")
    for i, b in enumerate(bubbles):
        lines.extend([
            "",
            f"Slot: {i}",
            f"X: {b.get('x', 50)}",
            f"Y: {b.get('y', 20)}",
            f"Text: {b.get('text', '')}",
            f"AlignH: {b.get('align_h', 'Left')}",
            f"AlignV: {b.get('align_v', 'Bottom')}",
            f"StartFrame: {b.get('start', 0)}",
            f"EndFrame: {b.get('end', 99)}",
        ])

    with open(os.path.join(anim_dir, "meta.txt"), "w") as f:
        f.write("\n".join(lines))

    print(f"  Created {name}: {len(frames)} frames, {len(bubbles)} bubbles")


# --- Animation definitions ---

def bot_idle():
    """Breathing + occasional blink."""
    frames = []
    for t in range(6):
        img = Image.new("1", (W, H), 1)
        d = ImageDraw.Draw(img)
        bot = ZxBot(d, W // 2, 58)
        b = breath_offset(t, 1)
        bh = 32 + b
        head_hy = -(bh + 4)
        bx, by, bw, bh = bot.body(28, bh)
        cx, cy, hw, hh = bot.head(18, 18, 0, head_hy)
        bot.antenna(0, head_hy, 18)
        bot.eyes(cx, cy, hw, blink_open(t, 6, 5))
        bot.mouth(cx, cy, hw, True)
        arm_angle = int(math.sin(t * math.pi / 3))
        bot.arm(-1, arm_angle, bw, bh)
        bot.arm(1, -arm_angle, bw, bh)
        bot.leg(-1, 0, bw)
        bot.leg(1, 0, bw)
        frames.append(img)
    return frames


def bot_sleep():
    """Progressive eye closure, deep slow breathing, floating Z's."""
    frames = []
    for t in range(6):
        img = Image.new("1", (W, H), 1)
        d = ImageDraw.Draw(img)
        bot = ZxBot(d, W // 2, 58)
        b = breath_offset(t // 2, 1)  # slower breath every 2 frames
        bh = 32 + b
        head_hy = -(bh + 4)
        bx, by, bw, bh = bot.body(28, bh)
        cx, cy, hw, hh = bot.head(18, 18, 0, head_hy)
        bot.antenna(0, head_hy, 18)
        eye_open = max(0.05, 1.0 - t * 0.19)
        bot.eyes(cx, cy, hw, eye_open)
        bot.mouth(cx, cy, hw, False, 0.3)
        bot.arm(-1, 0, bw, bh)
        bot.arm(1, 0, bw, bh)
        bot.leg(-1, 0, bw)
        bot.leg(1, 0, bw)
        for i in range(t // 2 + 1):
            zx = cx + 14 + i * 5
            zy = cy - 18 - i * 6 - t
            d.text((zx, zy), "z", fill=0)
        frames.append(img)
    return frames


def bot_coding():
    """Screen content cycles, head bobs, arms type."""
    patterns = [
        [" 0101 ", " 1010 ", " 1100 ", " 0011 "],
        [" 1010 ", " 0101 ", " 0011 ", " 1100 "],
        [" 1100 ", " 0011 ", " 1010 ", " 0101 "],
        [" 0110 ", " 1001 ", " 0101 ", " 1010 "],
        [" 1001 ", " 0110 ", " 1010 ", " 0101 "],
        [" 0011 ", " 1100 ", " 1001 ", " 0110 "],
    ]
    frames = []
    for t in range(6):
        img = Image.new("1", (W, H), 1)
        d = ImageDraw.Draw(img)
        bot = ZxBot(d, 50, 58)
        head_bob = int(math.sin(t * math.pi / 3))
        bx, by, bw, bh = bot.body(24, 28)
        head_hy = -(bh + 4) + head_bob
        cx, cy, hw, hh = bot.head(16, 16, 0, head_hy)
        bot.eyes(cx, cy, hw, 0.9)
        bot.mouth(cx, cy, hw, True)
        arm_angle = 1 if t % 2 == 0 else -1
        bot.arm(-1, arm_angle, bw, bh)
        bot.arm(1, -arm_angle, bw, bh)
        bot.leg(-1, 0, bw)
        bot.leg(1, 0, bw)
        tx, ty = 80, 30
        d.rectangle([tx, ty, tx + 35, ty + 24], fill=0)
        d.rectangle([tx + 1, ty + 1, tx + 34, ty + 23], fill=1)
        for i, row in enumerate(patterns[t]):
            for j, ch in enumerate(row):
                if ch == '1':
                    d.point((tx + 4 + j * 3, ty + 4 + i * 5), fill=0)
        frames.append(img)
    return frames


def bot_happy():
    """Bouncing up and down, arms raised, big smile."""
    frames = []
    for t in range(6):
        img = Image.new("1", (W, H), 1)
        d = ImageDraw.Draw(img)
        bounce = int(math.sin(t * math.pi / 3) * 3)
        bot = ZxBot(d, W // 2, 58 - bounce)
        bx, by, bw, bh = bot.body(28, 32)
        head_hy = -(bh + 4)
        cx, cy, hw, hh = bot.head(18, 18, 0, head_hy)
        bot.antenna(0, head_hy, 18)
        bot.eyes(cx, cy, hw, 1.0)
        mouth_open = 0.6 if t % 2 == 0 else 0
        bot.mouth(cx, cy, hw, True, mouth_open)
        arm_angle = -2 if bounce > 0 else 0
        bot.arm(-1, arm_angle, bw, bh)
        bot.arm(1, arm_angle, bw, bh)
        bot.leg(-1, 0, bw)
        bot.leg(1, 0, bw)
        d.text((cx + 20, cy - 10), "*", fill=0)
        frames.append(img)
    return frames


def bot_sad():
    """Head droops, shoulders narrow, tear falls."""
    frames = []
    for t in range(6):
        img = Image.new("1", (W, H), 1)
        d = ImageDraw.Draw(img)
        droop = int(math.sin(t * math.pi / 6) * 2)
        bot = ZxBot(d, W // 2, 60)
        bx, by, bw, bh = bot.body(26 - (t // 3), 30)
        head_hy = -(bh + 4) + droop
        cx, cy, hw, hh = bot.head(16, 16, 0, head_hy)
        bot.antenna(0, head_hy, 16)
        bot.eyes(cx, cy + droop, hw, 0.7, look_up=True)
        bot.mouth(cx, cy + droop, hw, False)
        bot.arm(-1, 0, bw, bh)
        bot.arm(1, 0, bw, bh)
        bot.leg(-1, 0, bw)
        bot.leg(1, 0, bw)
        d.point((cx - 5, cy + 2 + t), fill=0)
        frames.append(img)
    return frames


def bot_angry():
    """Shaking with rage, fists clenched."""
    frames = []
    for t in range(6):
        img = Image.new("1", (W, H), 1)
        d = ImageDraw.Draw(img)
        shake_x = int(math.sin(t * math.pi / 2) * 1)
        bot = ZxBot(d, W // 2 + shake_x, 58)
        bx, by, bw, bh = bot.body(30, 34)
        head_hy = -(bh + 4)
        cx, cy, hw, hh = bot.head(20, 20, 0, head_hy)
        bot.antenna(0, head_hy, 20)
        bot.draw_angry_eyes(cx, cy, hw, shake_x)
        bot.mouth(cx, cy, hw, False, 0.4)
        fist_angle = 3 if t % 2 == 0 else -3
        bot.arm(-1, -fist_angle, bw, bh)
        bot.arm(1, fist_angle, bw, bh)
        bot.leg(-1, 0, bw)
        bot.leg(1, 0, bw)
        d.line([cx + 14, cy - 12, cx + 16, cy - 6], fill=0, width=2)
        d.line([cx + 16, cy - 6, cx + 20, cy - 10], fill=0, width=2)
        frames.append(img)
    return frames


def bot_singing():
    """Mouth opens/closes rhythmically, notes float upward."""
    frames = []
    for t in range(6):
        img = Image.new("1", (W, H), 1)
        d = ImageDraw.Draw(img)
        bot = ZxBot(d, W // 2, 58)
        b = breath_offset(t, 1)
        bh = 32 + b
        head_hy = -(bh + 4)
        bx, by, bw, bh = bot.body(28, bh)
        cx, cy, hw, hh = bot.head(18, 18, 0, head_hy)
        bot.antenna(0, head_hy, 18)
        bot.eyes(cx, cy, hw, 0.9)
        mouth_open = 0.3 + 0.4 * (0.5 + 0.5 * math.sin(t * math.pi / 2))
        bot.mouth(cx, cy, hw, True, mouth_open)
        bot.arm(-1, 0, bw, bh)
        bot.arm(1, 0, bw, bh)
        bot.leg(-1, 0, bw)
        bot.leg(1, 0, bw)
        for i in range(2):
            nx = cx + 16 + i * 8 + (t % 3) * 2
            ny = cy - 12 - i * 6 - (t % 4)
            d.text((nx, ny), "*", fill=0)
        frames.append(img)
    return frames


def bot_wave():
    """Arm waves side to side, head tilts."""
    frames = []
    for t in range(6):
        img = Image.new("1", (W, H), 1)
        d = ImageDraw.Draw(img)
        bot = ZxBot(d, W // 2, 58)
        bx, by, bw, bh = bot.body(26, 30)
        head_hy = -(bh + 4)
        cx, cy, hw, hh = bot.head(16, 16, 0, head_hy)
        bot.antenna(0, head_hy, 16)
        bot.eyes(cx, cy, hw, blink_open(t, 6, 5))
        bot.mouth(cx, cy, hw, True)
        wave_offset = 6 if t % 2 == 0 else -6
        arm_x = bx + bw
        arm_y = by + bh * 0.3
        d.line([arm_x, arm_y, arm_x + 6, arm_y - 6 + wave_offset], fill=0, width=2)
        d.line([arm_x + 6, arm_y - 6 + wave_offset, arm_x + 4, arm_y - 14 + wave_offset], fill=0, width=2)
        bot.arm(1, 0, bw, bh)
        bot.leg(-1, 0, bw)
        bot.leg(1, 0, bw)
        if t >= 2:
            d.text((bx - 16, by + bh - 20 + (t % 2)), "Hi!", fill=0)
        frames.append(img)
    return frames


def bot_laptop():
    """Screen content cycles, typing motion."""
    screens = [
        "......",
        "code..",
        "......",
        "debug.",
        "......",
        "commit",
    ]
    frames = []
    for t in range(6):
        img = Image.new("1", (W, H), 1)
        d = ImageDraw.Draw(img)
        bot = ZxBot(d, 50, 58)
        bx, by, bw, bh = bot.body(24, 28)
        head_hy = -(bh + 4)
        cx, cy, hw, hh = bot.head(16, 16, 0, head_hy)
        bot.eyes(cx, cy, hw, 0.8)
        bot.mouth(cx, cy, hw, True)
        bot.leg(-1, 0, bw)
        bot.leg(1, 0, bw)
        lx, ly = 74, 38
        d.rectangle([lx, ly, lx + 30, ly + 18], fill=0)
        d.rectangle([lx + 1, ly + 1, lx + 29, ly + 17], fill=1)
        d.rectangle([lx + 5, ly + 14, lx + 25, ly + 16], fill=0)
        d.text((lx + 6, ly + 4), screens[t], fill=0)
        arm_lift = 4 if t % 2 == 0 else 0
        d.line([bx + bw // 2, by + bh, lx + 8, ly + 12 - arm_lift], fill=0, width=1)
        arm_lift2 = 0 if t % 2 == 0 else 4
        d.line([bx + bw // 2, by + bh, lx + 22, ly + 12 - arm_lift2], fill=0, width=1)
        frames.append(img)
    return frames


def bot_excited():
    """Jumping up and down, arms raised, stars."""
    frames = []
    for t in range(6):
        img = Image.new("1", (W, H), 1)
        d = ImageDraw.Draw(img)
        jump = int(math.sin(t * math.pi / 3) * 4)
        bot = ZxBot(d, W // 2, 58 - jump)
        bx, by, bw, bh = bot.body(26, 30)
        head_hy = -(bh + 4)
        cx, cy, hw, hh = bot.head(18, 18, 0, head_hy)
        bot.antenna(0, head_hy, 18)
        bot.eyes(cx, cy, hw, 1.2)
        big_mouth = 0.7 if t % 2 == 0 else 0.3
        bot.mouth(cx, cy, hw, True, big_mouth)
        bot.arm(-1, 4, bw, bh)
        bot.arm(1, 4, bw, bh)
        bot.leg(-1, 2, bw)
        bot.leg(1, -2, bw)
        for i in range(3):
            sx = cx + 20 + i * 4
            sy = cy - 14 - i * 5 + t
            d.text((sx, sy), "*", fill=0)
        d.text((cx + 18, cy - 12 - (t % 3)), "!", fill=0)
        frames.append(img)
    return frames


# --- Generate all animations ---

ANIMATIONS = [
    ("Zx_Idle_128x64", bot_idle, {"passive": 6, "active": 0, "rate": 3, "duration": 3600}),
    ("Zx_Sleep_128x64", bot_sleep, {"passive": 6, "active": 0, "rate": 3, "duration": 3600,
     "bubbles": [{"x": 55, "y": 28, "text": "Zzz... rebooting...", "start": 0, "end": 5, "align_h": "Left", "align_v": "Bottom"}]}),
    ("Zx_Coding_128x64", bot_coding, {"passive": 6, "active": 0, "rate": 3, "duration": 3600,
     "bubbles": [{"x": 55, "y": 25, "text": "git push --force", "start": 0, "end": 5, "align_h": "Left", "align_v": "Bottom"}]}),
    ("Zx_Happy_128x64", bot_happy, {"passive": 6, "active": 0, "rate": 3, "duration": 3600}),
    ("Zx_Sad_128x64", bot_sad, {"passive": 6, "active": 0, "rate": 3, "duration": 3600}),
    ("Zx_Angry_128x64", bot_angry, {"passive": 6, "active": 0, "rate": 3, "duration": 3600}),
    ("Zx_Singing_128x64", bot_singing, {"passive": 6, "active": 0, "rate": 3, "duration": 3600}),
    ("Zx_Wave_128x64", bot_wave, {"passive": 6, "active": 0, "rate": 3, "duration": 3600}),
    ("Zx_Laptop_128x64", bot_laptop, {"passive": 6, "active": 0, "rate": 3, "duration": 3600}),
    ("Zx_Excited_128x64", bot_excited, {"passive": 6, "active": 0, "rate": 3, "duration": 3600}),
]

MANIFEST_ENTRIES = [
    ("Zx_Idle_128x64", 0, 14, 1, 3, 4),
    ("Zx_Sleep_128x64", 0, 14, 1, 3, 3),
    ("Zx_Coding_128x64", 0, 8, 1, 3, 3),
    ("Zx_Happy_128x64", 0, 6, 1, 3, 3),
    ("Zx_Sad_128x64", 8, 13, 1, 3, 3),
    ("Zx_Angry_128x64", 9, 14, 1, 3, 3),
    ("Zx_Singing_128x64", 0, 8, 1, 3, 3),
    ("Zx_Wave_128x64", 0, 6, 1, 2, 3),
    ("Zx_Laptop_128x64", 0, 10, 2, 3, 3),
    ("Zx_Excited_128x64", 0, 6, 1, 2, 3),
]


def generate_manifest():
    lines = [
        "Filetype: Flipper Animation Manifest",
        "Version: 1",
        "",
    ]
    for name, min_b, max_b, min_l, max_l, w in MANIFEST_ENTRIES:
        lines.extend([
            f"Name: {name}",
            f"Min butthurt: {min_b}",
            f"Max butthurt: {max_b}",
            f"Min level: {min_l}",
            f"Max level: {max_l}",
            f"Weight: {w}",
            "",
        ])
    path = os.path.join(OUT_DIR, "manifest.txt")
    with open(path, "w") as f:
        f.write("\n".join(lines))
    print(f"  Manifest written ({len(MANIFEST_ENTRIES)} entries)")


def generate_internal_blocking():
    for name, text in [("L1_NoSd_128x49", "No SD card!"),
                        ("L1_BadBattery_128x47", "Low battery..."),
                        ("L1_Tv_128x47", "TV mode")]:
        h_for_name = int(name.split("x")[1])
        anim_dir = os.path.join(INTERNAL_DIR, name)
        os.makedirs(anim_dir, exist_ok=True)
        frames = []
        for t in range(6):
            img = Image.new("1", (W, h_for_name), 1)
            d = ImageDraw.Draw(img)
            bot = ZxBot(d, W // 2, h_for_name - 4)
            bh_actual = min(30, h_for_name - 14) + breath_offset(t, 1)
            bx, by, bw, bh = bot.body(26, bh_actual)
            head_hy = -(bh + 4)
            cx, cy, hw, hh = bot.head(16, 16, 0, head_hy)
            bot.eyes(cx, cy, hw, blink_open(t, 6, 5))
            bot.mouth(cx, cy, hw, False)
            bot.arm(-1, 0, bw, bh)
            bot.arm(1, 0, bw, bh)
            bot.leg(-1, 0, bw)
            bot.leg(1, 0, bw)
            d.text((cx - len(text) * 3, by - 10), text, fill=0)
            frames.append(img)
        for i, img in enumerate(frames):
            img.save(os.path.join(anim_dir, f"frame_{i}.png"))
        with open(os.path.join(anim_dir, "meta.txt"), "w") as f:
            f.write(f"""Filetype: Flipper Animation
Version: 1

Width: {W}
Height: {h_for_name}
Passive frames: 6
Active frames: 0
Frames order: 0 1 2 3 4 5
Active cycles: 0
Frame rate: 3
Duration: 3600
Active cooldown: 0
Bubble slots: 0
""")
        print(f"  {name} -> internal")

    for name, text in [("L0_NoDb_128x51", "No DB!"),
                        ("L0_SdBad_128x51", "SD error"),
                        ("L0_SdOk_128x51", "SD OK!"),
                        ("L0_Url_128x51", "URL received"),
                        ("L0_NewMail_128x51", "New mail!")]:
        h_for_name = int(name.split("x")[1])
        anim_dir = os.path.join(BLOCKING_DIR, name)
        os.makedirs(anim_dir, exist_ok=True)
        frames = []
        for t in range(6):
            img = Image.new("1", (W, h_for_name), 1)
            d = ImageDraw.Draw(img)
            bot = ZxBot(d, W // 2, h_for_name - 4)
            bh_actual = min(30, h_for_name - 14) + breath_offset(t, 1)
            bx, by, bw, bh = bot.body(26, bh_actual)
            head_hy = -(bh + 4)
            cx, cy, hw, hh = bot.head(16, 16, 0, head_hy)
            bot.eyes(cx, cy, hw, blink_open(t, 6, 5))
            bot.mouth(cx, cy, hw, True)
            bot.arm(-1, 0, bw, bh)
            bot.arm(1, 0, bw, bh)
            bot.leg(-1, 0, bw)
            bot.leg(1, 0, bw)
            d.text((cx - len(text) * 3, by - 10), text, fill=0)
            frames.append(img)
        for i, img in enumerate(frames):
            img.save(os.path.join(anim_dir, f"frame_{i}.png"))
        with open(os.path.join(anim_dir, "meta.txt"), "w") as f:
            f.write(f"""Filetype: Flipper Animation
Version: 1

Width: {W}
Height: {h_for_name}
Passive frames: 6
Active frames: 0
Frames order: 0 1 2 3 4 5
Active cycles: 0
Frame rate: 3
Duration: 3600
Active cooldown: 0
Bubble slots: 0
""")
        print(f"  {name} -> blocking")


if __name__ == "__main__":
    print("Generating Zx mascot animations v2...")
    for name, func, meta in ANIMATIONS:
        make_animation(name, func, meta)
    generate_manifest()
    generate_internal_blocking()
    print("Done!")
