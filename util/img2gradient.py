#!/usr/bin/python3
import sys
from PIL import Image

"""
Converts a PNG into a gradient compatible
with aptdec. Requires Pillow:

 pip3 install Pillow

"""

if len(sys.argv) == 1:
    print("Usage: {} filename.png".format(sys.argv[0]))
    exit()

image = Image.open(sys.argv[1])
pixels = image.load()

if len(pixels[0, 0]) != 3:
    print("Image must be RGB")
    exit()

if image.size[0] != 1:
    print("Image must be 1px wide")
    exit()

print("uint32_t gradient[{}] = {{\n    ".format(image.size[1]), end="")
for y in range(image.size[1]):
    print("0x" + "".join("{:02X}".format(a) for a in pixels[0, y]), end="")

    if y != image.size[1] - 1:
        print(", ", end="")
    if y % 7 == 6:
        print("\n    ", end="")

print("\n};")
