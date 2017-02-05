#!/usr/bin/env python3
'''Syntax highlighting script'''
#SYN:xkopec44


import sys
import re
import fun


# parse arguments from command line
ARGS = fun.parse_args()
# print(ARGS)

#  Manage files
# try to open INPUT file
if ARGS.input is None:
    ARGS.input = sys.stdin.read()
else:
    try:
        ARGS.input = open(ARGS.input).read()
    except OSError:
        MSG = 'Cant open file: ' + ARGS.input
        fun.error(MSG, 2)


# try to open OUTPUT file
if ARGS.output is None:
    ARGS.output = sys.__stdout__
else:
    try:
        ARGS.output = open(ARGS.output, 'w')
    except OSError:
        MSG = 'Cant open file: ' + ARGS.output
        fun.error(MSG, 3)

# try to open FOMRAT file
# if can not open, then output is input
if ARGS.format is None:
    ARGS.output.write(fun.add_br(ARGS.br, ARGS.input))
    exit(0)
else:
    try:
        # only time formating file is parsed
        ARGS.format = open(ARGS.format).read()
        if not ARGS.format:
            ARGS.output.write(fun.add_br(ARGS.br, ARGS.input))
            exit(0)
    except OSError:
        ARGS.output.write(fun.add_br(ARGS.br, ARGS.input))
        exit(0)

# Parse format file
FORMRULES = fun.parse_format(ARGS.format)

# check validity of regural expression and convert it to python type
for regx in FORMRULES:
    if not fun.valid_neg(regx["regex"]):
        fun.error("Wrong regex format in format file.", 4)

for regx in FORMRULES:
    regx["regex"] = fun.convert_regex(regx["regex"])
    try:
        temp = re.compile(regx["regex"])
    except re.error:
        fun.error("Wrong regex format in format file.", 4)


# scan for matches and store positions for tags
ST_POS = [] # positions of starting tags
ET_POS = [] # positions of ending tags
ST_TAGS = [] # save what tags should be inserted
ET_TAGS = []

for line in FORMRULES:
    for command in line["command"]:
        match = re.finditer(line["regex"], ARGS.input, re.A)
        for word in match:
            if word.start() == word.end():
                continue
            ST_POS.append(word.start())
            ET_POS.append(word.end())
            ST_TAGS.append(fun.get_stag(command))
            ET_TAGS.append(fun.get_etag(command))


# combine these lists into one for simplification
POS = []
ET_POS.reverse()
POS.extend(ET_POS)
POS.extend(ST_POS)
# combine list of tags also
TAGS = []
ET_TAGS.reverse()
TAGS.extend(ET_TAGS)
TAGS.extend(ST_TAGS)


# new output string with tags
OUTFILE = ""
CNT = 0
for char in ARGS.input:
    TAG_CNT = 0
    for pos in POS:
        if pos == CNT:
            OUTFILE += TAGS[TAG_CNT]
        TAG_CNT += 1
    OUTFILE += char
    CNT += 1
TAG_CNT = 0
for pos in POS:
    if pos == CNT:
        OUTFILE += TAGS[TAG_CNT]
    TAG_CNT += 1

# write to output file
ARGS.output.write(fun.add_br(ARGS.br, OUTFILE))
ARGS.output.close()
