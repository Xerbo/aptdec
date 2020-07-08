#!/usr/bin/python3
import sys
from PIL import Image

'''
Converts a PNG into a palette compatible
with aptdec. Requires PIL:

 pip3 install Pillow

'''

if len(sys.argv) == 1:
    print("Usage: python3 {} filename.png".format(sys.argv[0]))
    exit()

image = Image.open(sys.argv[1])
pixels = image.load()

sys.stdout.write("char palette[{}*3] = {{\n    \"".format(image.size[1]))
for y in range(1, image.size[1]+1):
    sys.stdout.write(''.join('\\x{:02x}'.format(a) for a in pixels[0, y-1]))
    if(y % 7 == 0 and y != 0):
        sys.stdout.write("\"\n    \"")

print("\"\n};")
