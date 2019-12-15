# Python script for converting an 8 bit mono PCM wave file 
# to a byte array C header file
# compatible with the XTronical DAC Audio library for ESP32. 
# https://www.xtronical.com/basics/audio/dacs-for-sound/playing-wav-files/

import sys
import os

if len(sys.argv) < 2:
	print("Source audio wav filename parameter missing.")
	sys.exit()
	
filename = sys.argv[1]
filebase = os.path.splitext(filename)[0] + '_wav'
filehdr  = filebase + '.h'

with open(filename, "rb") as wav:
	wav_data = wav.read()
	wav.close()

sample_cnt = len(wav_data)

f = open(filehdr, "w+")

f.write("/* Include file for 8-bit PCM audio data \r\n")
f.write(" * Original filename: ")
f.write(filename)
f.write("\r\n")
f.write(" * Samples: ")
f.write(str(sample_cnt))
f.write("\r\n")
f.write(" */\r\n\r\n")

f.write("const PROGMEM uint8_t ") 
f.write(filebase)
f.write("[%d] = {\r\n" % (sample_cnt))

curr_byte = 0
for byte in wav_data:
	curr_byte = curr_byte + 1
	#print byte	
	f.write('0x{0:0{1}X}'.format(ord(byte), 2))
	#f.write(hex(byte))	
	f.write(", ")
	if (curr_byte) % 8 == 0:
		f.write("\r\n")
		
f.write("};\r\n")
f.close()

print("Audio PCM data output to %s" % (filehdr))
