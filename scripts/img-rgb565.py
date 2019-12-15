# Python script for converting an image file 
# to an RGB565 16-bit integer array C header file
# compatible with the Adafruit GFX library for Arduino. 
#
# Requires Pillow Python Image Library
# https://pillow.readthedocs.io/en/3.1.x/installation.html

import sys
import os
from PIL import Image

if len(sys.argv) < 2:
	print("Source image filename parameter missing.")
	sys.exit()
	
filename = sys.argv[1]
filebase = os.path.splitext(filename)[0]
filehdr  = filebase + '.h'

im = Image.open(filename)
rgb_im = im.convert('RGB')
width, height = im.size

f = open(filehdr, "w+")

f.write("/* Image include file for RGB565 bitmap \r\n")
f.write(" * Original filename: ")
f.write(filename)
f.write("\r\n")
f.write(" * Width: ")
f.write(str(width))
f.write(" * Height: ")
f.write(str(height))
f.write("\r\n")
f.write(" */\r\n\r\n")

f.write("const PROGMEM uint16_t ") 
f.write(filebase)
f.write("[%d] = {\r\n" % (width * height))

for y in range(height):
	for x in range(width):
		r,g,b = rgb_im.getpixel((x, y))
		r = (r << 8) & 0xF100
		g = (g << 3) & 0x07E0
		b = (b >> 3) & 0x001F
		pixel = r + g + b
		#print(hex(pixel))
		f.write('0x{0:0{1}X}'.format(pixel, 6))
		f.write(", ")
		if (y * width + x) % 8 == 7:
			f.write("// ")
			f.write(str(y * width + x + 1)) 
			f.write(" pixels\r\n")

f.write("};\r\n")
f.close()

print("RGB565 data output to %s" % (filehdr))
