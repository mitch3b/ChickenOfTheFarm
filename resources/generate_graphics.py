# this script is will seach through the Background and sprite folders and use
# the image files it finds to generate all of the pattern table, name table and
# attribute table information. It will update the data in the header files for
# each resource accordingly.
#
# Note that this script has a dependency on the sprite_mapper.py tool from
# https://github.com/pgrimsrud/sprite_mapper which in turn has a dependency on
# pygame (https://www.pygame.org/)
#
# Additional requirements:
# Each folder must have a png file to be converted and this file must have the
#   same name as the folder.
# Each folder must have a header file also named the same as the folder.
# Each header file must have an array named [folder name]Palette with the colors
#   required (from the colors list below).
#     Note that for backgrounds the background color must be first and there can
#       be as many as 4 sets of 3 after that. Each 16x16 tile can only use one set
#       of 3 and the background color.
#     Note that for sprites the first color is required for pattern creation
#       purposes, but it is treated as an alpha channel. You will be able to see
#       backgrounds or other sprites on anything painted this color, it will
#       effectively be invisible. You are allowed up to 3 additional colors.
# Names must be unique, sprite and background names can't be the same.
# Any additional code must be above the line:
# // DO NOT MAKE CHANGES BELOW THIS LINE!
# This script will stomp on anything below that with the results generated from
# the image.
#
# Note that when you make the image you should use the colors you specified
# (see the approximate RGB below). If you use other colors they will be mapped
# down. If your results seem unexpected check the [folder name]-reduced.png for
# comparison

import os
import sys
import re
from subprocess import call
from sprite_mapper.sprite_pattern_mapper import create_rle

colors = {
           'BLACK'        : "  0,  0,  0,255",
           'BLACK_2'      : "  0,  0,  0,255",
           'BLACK_3'      : "  0,  0,  0,255",
           'BLACK_4'      : "  0,  0,  0,255",
           'BLACK_5'      : "  0,  0,  0,255",
           'BLACK_6'      : "  0,  0,  0,255",
           'BLACK_7'      : "  0,  0,  0,255",
           'BLACK_8'      : "  0,  0,  0,255",
           'BLACK_9'      : "  0,  0,  0,255",
           'BLACK_10'     : "  0,  0,  0,255",
      'DARK_GRAY'         : " 84, 84, 84,255",
      'DARK_GRAY_2'       : " 60, 60, 60,255",
           'GRAY'         : "152,150,152,255",
           'GRAY_2'       : "160,162,160,255",
     'LIGHT_GRAY'         : "255,255,255,255",
           'WHITE'        : "255,255,255,255",
      'DARK_GRAY_BLUE'    : "  0, 30,116,255",
           'GRAY_BLUE'    : "  8, 76,196,255",
     'LIGHT_GRAY_BLUE'    : " 76,154,236,255",
'VERY_LIGHT_GRAY_BLUE'    : "168,204,236,255",
      'DARK_BLUE'         : "  8, 16,144,255",
           'BLUE'         : " 48, 50,236,255",
     'LIGHT_BLUE'         : "120,124,236,255",
'VERY_LIGHT_BLUE'         : "188,188,236,255",
      'DARK_PURPLE'       : " 48,  0,136,255",
           'PURPLE'       : " 92, 30,228,255",
     'LIGHT_PURPLE'       : "176, 98,236,255",
'VERY_LIGHT_PURPLE'       : "212,178,236,255",
      'DARK_PINK'         : " 68,  0,100,255",
           'PINK'         : "136, 20,176,255",
     'LIGHT_PINK'         : "228, 84,236,255",
'VERY_LIGHT_PINK'         : "236,174,236,255",
      'DARK_FUCHSIA'      : " 92,  0, 48,255",
           'FUCHSIA'      : "160, 20,100,255",
     'LIGHT_FUCHSIA'      : "236, 88,180,255",
'VERY_LIGHT_FUCHSIA'      : "236,174,212,255",
      'DARK_RED'          : " 84,  4,  0,255",
           'RED'          : "152, 34, 32,255",
     'LIGHT_RED'          : "236,106,100,255",
'VERY_LIGHT_RED'          : "236,180,176,255",
      'DARK_ORANGE'       : " 60, 24,  0,255",
           'ORANGE'       : "120, 60,  0,255",
     'LIGHT_ORANGE'       : "212,136, 32,255",
'VERY_LIGHT_ORANGE'       : "228,196,144,255",
      'DARK_TAN'          : " 32, 42,  0,255",
           'TAN'          : " 84, 90,  0,255",
     'LIGHT_TAN'          : "160,170,  0,255",
'VERY_LIGHT_TAN'          : "204,210,120,255",
      'DARK_GREEN'        : "  8, 58,  0,255",
           'GREEN'        : " 40,114,  0,255",
     'LIGHT_GREEN'        : "116,196,  0,255",
'VERY_LIGHT_GREEN'        : "180,222,120,255",
      'DARK_LIME_GREEN'   : "  0, 64,  0,255",
           'LIME_GREEN'   : "  8,124,  0,255",
     'LIGHT_LIME_GREEN'   : " 76,208, 32,255",
'VERY_LIGHT_LIME_GREEN'   : "168,226,144,255",
      'DARK_SEAFOAM_GREEN': "  0, 60,  0,255",
           'SEAFOAM_GREEN': "  0,118, 40,255",
     'LIGHT_SEAFOAM_GREEN': " 56,204,108,255",
'VERY_LIGHT_SEAFOAM_GREEN': "152,226,180,255",
      'DARK_CYAN'         : "  0, 50, 60,255",
           'CYAN'         : "  0,102,120,255",
     'LIGHT_CYAN'         : " 56,180,204,255",
'VERY_LIGHT_CYAN'         : "160,214,228,255",
}

palettes = {}
new_headers = {}
raw_patterns = {}
reduced_patterns = []
raw_nametables = {}
reduced_nametables = {}
background_list = []
sprite_list = []
background_id = 0

def create_patterns(folder, pattern_type):
    global palettes
    global new_headers
    global raw_patterns
    global raw_nametables
    global reduced_nametables

    base_name = os.getcwd() + "\\" + pattern_type + "\\" + folder + "\\" + folder
    image = base_name + ".png"
    header = base_name + ".h"
    color_map = ""
        
    print(folder)
    if not os.path.isfile(image):
        print("missing " + image)
        exit()
    if not os.path.isfile(header):
        print("missing " + header)
        exit()
    
    palettes[folder] = []
    new_headers[folder] = []
    raw_patterns[folder] = []
    raw_nametables[folder] = []
    reduced_nametables[folder] = []
    
    if pattern_type == "Sprite":
        sprite_list.append(folder)
    if pattern_type == "Background":
        background_list.append(folder)
    
    header_handle = open(header, 'r')
    palette_found = 0
    palette_end_found = 0
    
    count = 0
    for header_line in header_handle.readlines():
        new_headers[folder].append(header_line)
        match = re.search(folder + "Palette", header_line)
        if match:
            palette_found = 1
        if palette_found == 1 and palette_end_found == 0:
            match = re.search("}", header_line)
            if match:
                palette_end_found = 1
            match = re.search("(\w+),", header_line)
            if match:
                #print(match.group(0))
                palettes[folder].append(match.group(1))
                color_map = color_map + colors[match.group(1)] + ":"
                count = count + 1
        match = re.search("\/\/ DO NOT MAKE CHANGES BELOW THIS LINE!", header_line)
        if match:
            break
    header_handle.close()
    
    if pattern_type == "Sprite" and count > 4:
        print(folder + " specifies too many colors.")
        exit()
    if pattern_type == "Background" and count > 13:
        print(folder + " specifies too many colors.")
        exit()
    if palette_found == 0:
        print("No palette found in " + folder)
        exit()
    
    #print(palettes)
    #print(new_headers)
    
    #print(color_map)
    long_options = ["help", "in=", "out=", "map=", "c=", "offset=", "RLE"]
    call(["python","sprite_mapper\sprite_pattern_mapper.py",
          "--in=" + image,
          "--out=" + base_name + "-reduced.png",
          "--map=" + color_map,
          "--c=" + base_name + "-tmp.h"])

    header_handle = open(base_name + "-tmp.h", 'r')
    
    for header_line in header_handle.readlines():
        match = re.search("pattern.*\{(.*)\}", header_line)
        if match:
            tmp = match.group(1).split(",")
            for num in tmp:
                if num != '':
                    raw_patterns[folder].append(int(num))
        match = re.search("nametable.*\{(.*)\}", header_line)
        if match:
            tmp = match.group(1).split(",")
            for num in tmp:
                if num != '':
                    raw_nametables[folder].append(int(num))
                    reduced_nametables[folder].append(int(num))
                    
    #print(raw_patterns)
    #print(raw_nametables)
    
print("Creating patterns:")
for folder in os.listdir(os.getcwd() + "\Sprite"):
    create_patterns(folder, "Sprite")
    
for folder in os.listdir(os.getcwd() + "\Background"):
    create_patterns(folder, "Background")
    
#print(background_list)
#print(sprite_list)
#print(raw_nametables)

def reduce_patterns( resource ):
    global palettes
    global new_headers
    global raw_patterns
    global raw_nametables
    global reduced_patterns
    global reduced_nametables
    global background_id
    
    for i in range(0, int(len(raw_patterns[resource])/16)):
        exists = False
        for j in range(0, int(len(reduced_patterns)/16)):
            match = True
            for k in range(0, 16):
                if reduced_patterns[j*16 + k] != raw_patterns[resource][i*16 + k]:
                    match = False
                    
            if match == True:
                exists = True
                for k in range(0, len(raw_nametables[resource])):
                    if raw_nametables[resource][k] == i:
                        reduced_nametables[resource][k] = j
        
        if exists == False:
            #print(resource + ": %d map to %d" % (i, int(len(reduced_patterns)/16)))
            #print(raw_nametables[resource])
            for j in range(0, len(raw_nametables[resource])):
                if raw_nametables[resource][j] == i:
                    reduced_nametables[resource][j] = int(len(reduced_patterns)/16)
            zero_pattern = True
            for j in range(0, 16):
                reduced_patterns.append(raw_patterns[resource][i*16 + j])
                if raw_patterns[resource][i*16 + j] != 0:
                    zero_pattern = False
            if zero_pattern == True:
                background_id = int(len(reduced_patterns)/16) - 1
            #print(reduced_nametables[resource])
           
print("Reducing patterns:")
for resource in sprite_list:
    reduce_patterns( resource )
for resource in background_list:
    reduce_patterns( resource )
    
#print(raw_patterns)
#for i in range(0, int(len(reduced_patterns)/16)):
#    print("%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d" % (
#          reduced_patterns[i*16 + 0],
#          reduced_patterns[i*16 + 1],
#          reduced_patterns[i*16 + 2],
#          reduced_patterns[i*16 + 3],
#          reduced_patterns[i*16 + 4],
#          reduced_patterns[i*16 + 5],
#          reduced_patterns[i*16 + 6],
#          reduced_patterns[i*16 + 7],
#          reduced_patterns[i*16 + 8],
#          reduced_patterns[i*16 + 9],
#          reduced_patterns[i*16 + 10],
#          reduced_patterns[i*16 + 11],
#          reduced_patterns[i*16 + 12],
#          reduced_patterns[i*16 + 13],
#          reduced_patterns[i*16 + 14],
#          reduced_patterns[i*16 + 15]))
#print(reduced_nametables)

nametable_top_rle = {}
nametable_bottom_rle = {}

print("Encoding:")
for resource in background_list:
    #print(resource)
    nametable_tmp = []
    start = 0;
    
    if len(reduced_nametables[resource]) > 960:
        for i in range(0,960):
            nametable_tmp.append(reduced_nametables[resource][i])
        nametable_top_rle[resource] = create_rle(nametable_tmp)
        start = 960
    
    nametable_tmp = []
    for i in range(start,start+960):
        nametable_tmp.append(reduced_nametables[resource][i])
    nametable_bottom_rle[resource] = create_rle(nametable_tmp)

#print(nametable_top_rle)
#print(nametable_bottom_rle)

print("Updating headers:")
#write out the name tables and defines to a header for each resource
for resource in sprite_list:
    resource_handle = open(os.getcwd() + "\\Sprite\\" + resource + "\\" + resource + ".h", 'w+')
    
    for line in new_headers[resource]:
        resource_handle.write(line)
    resource_handle.write("\n")
    
    for i in range(0,len(reduced_nametables[resource])):
        resource_handle.write("#define PATTERN_" + resource.upper() + "_%d" % (i) + " %d" % (reduced_nametables[resource][i]) + "\n")
        
for resource in background_list:
    resource_handle = open(os.getcwd() + "\\Background\\" + resource + "\\" + resource + ".h", 'w+')
    
    for line in new_headers[resource]:
        resource_handle.write(line)
    resource_handle.write("\n")

    if resource in nametable_top_rle:
        resource_handle.write("const unsigned char Nametable_" + resource + "_top_rle[%d] = " % (len(nametable_top_rle[resource])) + "{")
        
        for i in range(0,len(nametable_top_rle[resource])):
            resource_handle.write("%d," % (nametable_top_rle[resource][i]))
        resource_handle.write("};\n\n")
        
    resource_handle.write("const unsigned char Nametable_" + resource + "_bottom_rle[%d] = " % (len(nametable_bottom_rle[resource])) + "{")
    
    for i in range(0,len(nametable_bottom_rle[resource])):
        resource_handle.write("%d," % (nametable_bottom_rle[resource][i]))
    resource_handle.write("};\n")
        
    
#write out the include tree and pattern tables remaking resources.h
resource_handle = open(os.getcwd() + "\\resources.h", 'w+')

resource_handle.write("// DO NOT MAKE CHANGES TO THIS FILE!\n")
resource_handle.write("// THIS FILE IS GENERATED BY generate_graphics.py AND WILL BE OVERWRITTEN!\n\n")

resource_handle.write("#include \"Colors.h\"\n\n")

for resource in sprite_list:
    resource_handle.write("#include \"Sprite\\" + resource + "\\" + resource + ".h\"\n")
for resource in background_list:
    resource_handle.write("#include \"Background\\" + resource + "\\" + resource + ".h\"\n")

resource_handle.write("\n")
resource_handle.write("#define PATTERN_BLANK_0 %d\n\n" % (background_id))
resource_handle.write("#pragma data-name (\"CHARS\")\n")
resource_handle.write("unsigned char pattern[%d] = {" % (len(reduced_patterns)))
for i in range(0,len(reduced_patterns)):
    resource_handle.write("%d," % (reduced_patterns[i]))
resource_handle.write("};\n")

resource_handle.close()
