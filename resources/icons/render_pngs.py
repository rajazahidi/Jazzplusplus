import math
from PIL import Image, ImageDraw

def render_concept1_icon(size):
    scale = 4
    canvas_size = 256 * scale
    img = Image.new("RGBA", (canvas_size, canvas_size), (0, 0, 0, 0))

    # Mask for rounded squircle
    pad = 12 * scale
    radius = 52 * scale
    mask = Image.new("L", (canvas_size, canvas_size), 0)
    mask_draw = ImageDraw.Draw(mask)
    mask_draw.rounded_rectangle([pad, pad, canvas_size - pad, canvas_size - pad], radius=radius, fill=255)

    # Base background with rich dark studio gradient (#0F172A -> #0B0F19 -> #020617)
    bg = Image.new("RGBA", (canvas_size, canvas_size))
    bg_draw = ImageDraw.Draw(bg)
    for y in range(canvas_size):
        t = y / canvas_size
        r = int(15 * (1 - t) + 2 * t)
        g = int(23 * (1 - t) + 6 * t)
        b = int(42 * (1 - t) + 23 * t)
        bg_draw.line([(0, y), (canvas_size, y)], fill=(r, g, b, 255))

    img.paste(bg, (0, 0), mask)
    draw = ImageDraw.Draw(img)

    # Squircle border
    draw.rounded_rectangle(
        [pad, pad, canvas_size - pad, canvas_size - pad],
        radius=radius,
        outline=(30, 41, 59, 230),
        width=int(2.5 * scale)
    )

    # Subtle DAW sequencer dashed grid lines
    for line_y in [72, 128, 184]:
        py = line_y * scale
        for x in range(32 * scale, 224 * scale, 12 * scale):
            draw.line([(x, py), (x + 6 * scale, py)], fill=(51, 65, 85, 100), width=int(1.5 * scale))

    # Ambient warm glow behind saxophone bell
    glow_center_x, glow_center_y = int(156 * scale), int(108 * scale)
    glow_r = int(65 * scale)
    for gr in range(glow_r, 0, -2 * scale):
        alpha = int(25 * (1.0 - gr / glow_r))
        draw.ellipse(
            [glow_center_x - gr, glow_center_y - gr, glow_center_x + gr, glow_center_y + gr],
            fill=(245, 158, 11, alpha)
        )

    # Saxophone body tube curve (bezier approximation points)
    # Tube starts from bottom left (72, 216), curves around (40, 150), up to (120, 56)
    steps = 80
    tube_points = []
    # Cubic bezier: P0=(72, 216), P1=(35, 216), P2=(38, 90), P3=(120, 56)
    p0 = (72 * scale, 216 * scale)
    p1 = (30 * scale, 216 * scale)
    p2 = (35 * scale, 85 * scale)
    p3 = (120 * scale, 56 * scale)

    for step in range(steps + 1):
        t = step / steps
        bx = (1-t)**3 * p0[0] + 3*(1-t)**2*t * p1[0] + 3*(1-t)*t**2 * p2[0] + t**3 * p3[0]
        by = (1-t)**3 * p0[1] + 3*(1-t)**2*t * p1[1] + 3*(1-t)*t**2 * p2[1] + t**3 * p3[1]
        tube_points.append((bx, by))

    # Draw base brass tube (shadow to highlight layers)
    tube_width = int(28 * scale)
    # Dark bronze base
    draw.line(tube_points, fill=(110, 55, 10, 255), width=tube_width, joint="curve")
    # Rich warm gold midtone
    draw.line(tube_points, fill=(245, 158, 11, 255), width=int(tube_width * 0.8), joint="curve")
    # Bright brass gleam
    draw.line(tube_points, fill=(254, 240, 138, 255), width=int(tube_width * 0.4), joint="curve")
    # Thin specular reflection line
    draw.line(tube_points, fill=(255, 255, 240, 210), width=int(2.5 * scale), joint="curve")

    # Saxophone pearl tone hole keys
    keys = [(48, 144, 7.5), (58, 112, 8.5), (78, 86, 9.0)]
    for kx, ky, kr in keys:
        px, py, pr = int(kx * scale), int(ky * scale), int(kr * scale)
        # Gold bevel
        draw.ellipse([px - pr - 2*scale, py - pr - 2*scale, px + pr + 2*scale, py + pr + 2*scale], fill=(120, 53, 15, 255))
        # Pearl touch
        draw.ellipse([px - pr, py - pr, px + pr, py + pr], fill=(253, 224, 71, 255))
        # Pearl highlight
        draw.ellipse([px - pr//2, py - pr//2, px, py], fill=(255, 255, 255, 220))

    # Flared Bell Opening (Angled ellipse)
    # Center (156, 108), Outer radius (58, 68), angle -20 deg
    bell_img = Image.new("RGBA", (canvas_size, canvas_size), (0, 0, 0, 0))
    bell_draw = ImageDraw.Draw(bell_img)
    bx, by = int(156 * scale), int(108 * scale)
    rx, ry = int(58 * scale), int(68 * scale)

    # Outer flared bell rim with metallic gold gradient
    for r_step in range(12 * scale):
        curr_rx = rx - r_step
        curr_ry = ry - r_step
        t = r_step / (12 * scale)
        # Gold gradient from light to dark bronze
        cr = int(254 * (1 - t) + 180 * t)
        cg = int(240 * (1 - t) + 83 * t)
        cb = int(138 * (1 - t) + 9 * t)
        bell_draw.ellipse([bx - curr_rx, by - curr_ry, bx + curr_rx, by + curr_ry], fill=(cr, cg, cb, 255))

    # Inner deep sound chamber (hole)
    inner_rx, inner_ry = int(45 * scale), int(54 * scale)
    for hole_step in range(inner_rx):
        ht = hole_step / inner_rx
        # Fade to deep acoustic black/indigo
        hr = int(120 * (1 - ht) + 5 * ht)
        hg = int(53 * (1 - ht) + 8 * ht)
        hb = int(15 * (1 - ht) + 20 * ht)
        h_rx = inner_rx - hole_step
        h_ry = inner_ry - int(hole_step * (54/45))
        if h_rx > 0 and h_ry > 0:
            bell_draw.ellipse([bx - h_rx, by - h_ry, bx + h_rx, by + h_ry], fill=(hr, hg, hb, 255))

    # Rotate bell by -20 degrees around (bx, by)
    rotated_bell = bell_img.rotate(20, resample=Image.Resampling.BICUBIC, center=(bx, by))
    img.paste(rotated_bell, (0, 0), rotated_bell)

    # Re-acquire draw
    draw = ImageDraw.Draw(img)

    # Glowing Soundwaves burst from bell
    # Wave 1: Cyan ribbon (upper)
    w1_pts = [
        (156 * scale, 108 * scale),
        (185 * scale, 88 * scale),
        (212 * scale, 76 * scale),
        (236 * scale, 56 * scale)
    ]
    # Wave 2: Amber beam (middle)
    w2_pts = [
        (156 * scale, 108 * scale),
        (192 * scale, 116 * scale),
        (220 * scale, 112 * scale),
        (244 * scale, 100 * scale)
    ]
    # Wave 3: Cyan ribbon (lower)
    w3_pts = [
        (156 * scale, 108 * scale),
        (188 * scale, 136 * scale),
        (216 * scale, 148 * scale),
        (240 * scale, 164 * scale)
    ]

    def draw_curve(pts, color, width):
        # Sample cubic bezier or lines
        sampled = []
        for i in range(len(pts) - 1):
            p_start = pts[i]
            p_end = pts[i+1]
            sub = 15
            for s in range(sub):
                st = s / sub
                sampled.append((
                    p_start[0] * (1 - st) + p_end[0] * st,
                    p_start[1] * (1 - st) + p_end[1] * st
                ))
        sampled.append(pts[-1])
        # Outer glow
        draw.line(sampled, fill=(color[0], color[1], color[2], 80), width=int(width * 2.2), joint="curve")
        # Core wave
        draw.line(sampled, fill=color, width=int(width), joint="curve")

    draw_curve(w1_pts, (56, 189, 248, 255), 3.8 * scale)
    draw_curve(w2_pts, (245, 158, 11, 255), 3.4 * scale)
    draw_curve(w3_pts, (56, 189, 248, 255), 3.4 * scale)

    # Glowing Sound Particles
    particles = [
        (218 * scale, 74 * scale, 3.2 * scale, (56, 189, 248, 255)),
        (238 * scale, 98 * scale, 2.8 * scale, (253, 224, 71, 255)),
        (226 * scale, 152 * scale, 3.2 * scale, (56, 189, 248, 255)),
    ]
    for px, py, pr, pcol in particles:
        # Glow halo
        draw.ellipse([px - pr*2, py - pr*2, px + pr*2, py + pr*2], fill=(pcol[0], pcol[1], pcol[2], 70))
        # Particle core
        draw.ellipse([px - pr, py - pr, px + pr, py + pr], fill=pcol)

    # Glowing "++" Emblem in Cyan
    def draw_plus(cx, cy, arm_len, thickness):
        h_len = arm_len * scale
        th = thickness * scale
        # Horizontal
        draw.rounded_rectangle([cx - h_len, cy - th, cx + h_len, cy + th], radius=2*scale, fill=(56, 189, 248, 255))
        # Vertical
        draw.rounded_rectangle([cx - th, cy - h_len, cx + th, cy + h_len], radius=2*scale, fill=(56, 189, 248, 255))
        # Bright core
        draw.rounded_rectangle([cx - h_len + scale, cy - th + scale, cx + h_len - scale, cy + th - scale], radius=scale, fill=(240, 253, 250, 220))
        draw.rounded_rectangle([cx - th + scale, cy - h_len + scale, cx + th - scale, cy + h_len - scale], radius=scale, fill=(240, 253, 250, 220))

    plus1_x = int(190 * scale)
    plus1_y = int(84 * scale)
    plus2_x = int(210 * scale)
    plus2_y = int(84 * scale)

    draw_plus(plus1_x, plus1_y, 7.5, 2.2)
    draw_plus(plus2_x, plus2_y, 7.5, 2.2)

    # Downsample using Lanczos
    final_img = img.resize((size, size), Image.Resampling.LANCZOS)
    return final_img

# Generate all standard resolutions
sizes = [256, 128, 64, 48, 32, 16]
images = {}

for s in sizes:
    icon = render_concept1_icon(s)
    images[s] = icon
    icon.save(f"resources/icons/jazz-{s}.png")
    if s == 256:
        icon.save("resources/icons/jazz.png")

# Save multi-size Windows .ico file
images[256].save(
    "resources/icons/jazz.ico",
    format="ICO",
    sizes=[(16, 16), (32, 32), (48, 48), (64, 64), (128, 128), (256, 256)]
)

# Generate 32x32 XPM for wxWidgets window icon: src/Bitmaps/jazz_icon.xpm
img32 = images[32]
pixels = img32.load()
width, height = 32, 32

# Build color palette
palette = {}
color_chars = " .#abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789+*@$"
palette["None"] = " "
char_idx = 1

lines_data = []
for y in range(height):
    line_chars = []
    for x in range(width):
        r, g, b, a = pixels[x, y]
        if a < 32:
            key = "None"
        else:
            key = f"#{r:02X}{g:02X}{b:02X}"
        if key not in palette:
            if char_idx < len(color_chars):
                palette[key] = color_chars[char_idx]
                char_idx += 1
            else:
                palette[key] = f"_{char_idx}"
                char_idx += 1
        line_chars.append(palette[key])
    lines_data.append("".join(line_chars))

with open("src/Bitmaps/jazz_icon.xpm", "w", encoding="utf-8") as f:
    f.write("/* XPM */\n")
    f.write("static const char *jazz_icon_xpm[] = {\n")
    f.write(f'"{width} {height} {len(palette)} 1",\n')
    for color, ch in palette.items():
        if color == "None":
            f.write(f'"{ch} c None",\n')
        else:
            f.write(f'"{ch} c {color}",\n')
    for i, line in enumerate(lines_data):
        comma = "," if i < len(lines_data) - 1 else ""
        f.write(f'"{line}"{comma}\n')
    f.write("};\n")

print("Rendered Concept 1 (The Golden Horn & Digital Flow) successfully!")
