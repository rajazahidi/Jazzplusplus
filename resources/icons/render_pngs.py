import math
from PIL import Image, ImageDraw

def create_jazz_icon(size):
    # Render at 4x for smooth anti-aliased downsampling
    scale = 4
    canvas_size = 256 * scale
    img = Image.new("RGBA", (canvas_size, canvas_size), (0, 0, 0, 0))
    draw = ImageDraw.Draw(img)

    # Rounded rectangle base
    pad = 16 * scale
    radius = 48 * scale
    # Background gradient approximation with rounded rect
    for i in range(224 * scale):
        t = i / (224 * scale)
        # Gradient #1E1B4B -> #0F172A -> #020617
        r = int(30 * (1 - t) + 2 * t)
        g = int(27 * (1 - t) + 6 * t)
        b = int(75 * (1 - t) + 23 * t)
        y = pad + i
        # We can draw the rounded rectangle outline/mask
    
    # Create mask for rounded rectangle
    mask = Image.new("L", (canvas_size, canvas_size), 0)
    mask_draw = ImageDraw.Draw(mask)
    mask_draw.rounded_rectangle([pad, pad, canvas_size - pad, canvas_size - pad], radius=radius, fill=255)

    # Base background with gradient
    bg = Image.new("RGBA", (canvas_size, canvas_size))
    for y in range(canvas_size):
        t = y / canvas_size
        r = int(26 * (1 - t) + 2 * t)
        g = int(22 * (1 - t) + 6 * t)
        b = int(60 * (1 - t) + 18 * t)
        for x in range(canvas_size):
            pass # Faster to put line
    
    bg_draw = ImageDraw.Draw(bg)
    for y in range(canvas_size):
        t = y / canvas_size
        r = int(32 * (1 - t) + 2 * t)
        g = int(25 * (1 - t) + 6 * t)
        b = int(75 * (1 - t) + 20 * t)
        bg_draw.line([(0, y), (canvas_size, y)], fill=(r, g, b, 255))
    
    # Border
    img.paste(bg, (0, 0), mask)
    border_draw = ImageDraw.Draw(img)
    border_draw.rounded_rectangle([pad, pad, canvas_size - pad, canvas_size - pad], radius=radius, outline=(60, 75, 105, 200), width=2*scale)

    # Sequencer grid lines
    for line_y in [70, 100, 130]:
        y_pos = line_y * scale
        for x in range(36 * scale, 220 * scale, 10 * scale):
            border_draw.line([(x, y_pos), (x + 6 * scale, y_pos)], fill=(70, 85, 115, 120), width=2 * scale)

    # Piano keys at bottom
    # White keys
    white_keys = 7
    key_w = 23 * scale
    key_h = 48 * scale
    start_x = 38 * scale
    start_y = 172 * scale

    for k in range(white_keys):
        kx = start_x + k * (key_w + 3 * scale)
        border_draw.rounded_rectangle([kx, start_y, kx + key_w, start_y + key_h], radius=4 * scale, fill=(245, 248, 252, 255), outline=(180, 190, 205, 255), width=scale)

    # Black keys
    black_indices = [0, 1, 3, 4, 5]
    bkey_w = 14 * scale
    bkey_h = 30 * scale
    for bi in black_indices:
        bx = start_x + bi * (key_w + 3 * scale) + 16 * scale
        border_draw.rounded_rectangle([bx, start_y, bx + bkey_w, start_y + bkey_h], radius=2 * scale, fill=(15, 20, 30, 255))

    # Beamed notes
    # Cyan to Purple gradient noteheads & stems
    beam_color = (130, 150, 250, 255)
    accent_color = (60, 200, 255, 255)

    # Left stem
    border_draw.rounded_rectangle([78 * scale, 58 * scale, 85 * scale, 140 * scale], radius=3 * scale, fill=accent_color)
    # Right stem
    border_draw.rounded_rectangle([142 * scale, 48 * scale, 149 * scale, 130 * scale], radius=3 * scale, fill=beam_color)

    # Primary beam
    border_draw.polygon([
        (78 * scale, 58 * scale),
        (149 * scale, 48 * scale),
        (149 * scale, 66 * scale),
        (78 * scale, 76 * scale)
    ], fill=accent_color)

    # Secondary beam
    border_draw.polygon([
        (78 * scale, 82 * scale),
        (149 * scale, 72 * scale),
        (149 * scale, 84 * scale),
        (78 * scale, 94 * scale)
    ], fill=beam_color)

    # Noteheads
    # Notehead 1
    nh1_x, nh1_y = 70 * scale, 140 * scale
    border_draw.ellipse([nh1_x - 18 * scale, nh1_y - 13 * scale, nh1_x + 18 * scale, nh1_y + 13 * scale], fill=accent_color)

    # Notehead 2
    nh2_x, nh2_y = 134 * scale, 130 * scale
    border_draw.ellipse([nh2_x - 18 * scale, nh2_y - 13 * scale, nh2_x + 18 * scale, nh2_y + 13 * scale], fill=beam_color)

    # Plus signs "++" in Gold
    gold = (245, 175, 25, 255)
    # First +
    border_draw.rounded_rectangle([176 * scale, 52 * scale, 181 * scale, 70 * scale], radius=2 * scale, fill=gold)
    border_draw.rounded_rectangle([169 * scale, 59 * scale, 188 * scale, 64 * scale], radius=2 * scale, fill=gold)
    # Second +
    border_draw.rounded_rectangle([196 * scale, 60 * scale, 201 * scale, 78 * scale], radius=2 * scale, fill=gold)
    border_draw.rounded_rectangle([189 * scale, 67 * scale, 208 * scale, 72 * scale], radius=2 * scale, fill=gold)

    # Resize to final requested size with Lanczos filter
    final_img = img.resize((size, size), Image.Resampling.LANCZOS)
    return final_img

for s in [256, 128, 64, 48, 32, 16]:
    icon = create_jazz_icon(s)
    icon.save(f"/home/kingzahidi/jazz/resources/icons/jazz-{s}.png")
    if s == 256:
        icon.save("/home/kingzahidi/jazz/resources/icons/jazz.png")

print("Generated icons successfully!")
