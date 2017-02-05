'''File containing function for syntax highlinging script located in file syn.py'''

#SYN:xkopec44


import sys
import argparse
import re

def error(messsage, exit_code):
    '''Prints message to STDERR and exits'''
    print(messsage, file=sys.__stderr__)
    exit(exit_code)
# def error



def parse_format(fstr):
    '''Parse regural expressions in format file'''
    fmt = []
    for line in fstr.splitlines():
        if line == "":
            continue
        fline = {}
        entry = line.split("\t", 1) # split on first occurence
        if len(entry) < 2 or entry[0] == "" or entry[1] == "":
            error("Bad format of format file", 4)
        fline["regex"] = entry[0]
        entry[1] = re.sub("\t*", "", entry[1]) # remove tabs
        entry[1] = re.sub(" *", "", entry[1]) # remove spaces
        command = re.split(",", entry[1])
        fline["command"] = command
        fmt.append(fline)

    for fline in fmt:
        for attr in fline["command"]:
            if re.search("(bold|italic|underline|teletype)", attr) is not None:
                continue
            if attr.find("size") != -1: # found
                if re. search("(size:[1-7])", attr) is None:
                    error("Bad format of format file", 4)
                else:
                    continue
            if attr.find("color") != -1: # found
                if re.search("(color:[0-9a-fA-F]{6})", attr) is None:
                    error("Bad format of format file", 4)
                else:
                    continue
            error("Bad format of format file", 4)
    return fmt
# def parse_format


def negate(old, flag):
    '''Create new string from old. If flag is set to True.
    new = [^old]'''
    if flag:
        if old.find("[") > 0:
            new = re.sub("[", "[^", old)
        elif old == '(':
            new = "[^"
        elif old == ')':
            new = "]"
        else:
            new = "[^" + old + "]"
        return new
    else:
        return old
# def negate


def convert_special(char):
    '''Convert special regex parts starting with %'''
    new = None
    same = "sdtn.|+*()"
    if char == 'a':
        new = "[\\x00-\\x7F]" # re.DOTALL
    elif char == 'l':
        new = "[a-z]"
    elif char == 'L':
        new = "[A-Z]"
    elif char == 'w':
        new = "[A-Za-z]"
    elif char == 'W':
        new = "\\w"
    else:
        if char in same:
            new = "\\" + char
    if new is None:
        error("Wrong regural expression in format file", 4)
    else:
        return new
# def convert_special


def convert_meta(char):
    '''Escape special regex meta characters.'''
    meta = "^[]$\\{}?"
    new = char
    if char in meta:
        new = '\\' + char
        return new
    return new
# def convert_meta


def valid_neg(regx):
    '''Checks if negation ! is used only over one symbol.
    Return True if regex is valid.'''
    neg_flag = False
    bracket_cnt = 0
    char_cnt = 0

    for char in regx:
        if char == '!':
            neg_flag = True
            continue
        if neg_flag:
            if char == '(':
                bracket_cnt += 1
                continue
            if char == ')':
                bracket_cnt -= 1
                char_cnt = 0
                continue
        if bracket_cnt > 0:
            if char != '|':
                char_cnt += 1
            else:
                char_cnt = 0
            if char_cnt > 1:
                return False
        else: # apply negation on one character
            neg_flag = False

    return True
# def valid_neg


def valid_regex(regx):
    '''Check for validity of regex.'''

    # check for valid usage of "." and "|" symbols
    if re.search(r"(^[|.])|([^%][|.]{2,})|([^%][|.]$)", regx) is not None:
        error("Bad format of format file", 4)

    # can not be "*" or "+" at start of regex
    if re.search(r"^[*+]", regx) is not None:
        error("Bad format of format file", 4)
    if re.search(r"([^%]|^)!([.|*+]|$)", regx) is not None:
        error("Bad format of format file", 4)

    # valid parenthesis
    if re.search(r"([^%]|^)[()]", regx) is not None:
        if re.search(r"\(\)", regx) is not None:
            error("Bad format of format file", 4)
        if re.search(r"([^%]|^)\(.+[^%]\)", regx) is None:
            error("Bad format of format file", 4)
#def valid_regex


def convert_regex(regex_old):
    '''Parse regex from format file and conver it to python regex syntax'''
    new = ""
    esc_flag = False
    neg_flag = False

    valid_regex(regex_old)

    # NQS A++ = A+, A*+ = A*, etc.
    regex_old = re.sub(r"\+{2,}", r"+", regex_old)
    regex_old = re.sub(r"(\++\*+)|(\*+\++)|(\*{2,})", r"*", regex_old)

    for char in regex_old: # for loop over each character in string
        # capturing negation
        if char == '!':
            if esc_flag:
                new += char
                esc_flag = False
                continue
            neg_flag = not neg_flag
            continue
        # capturing escape sequencies
        if char == '%':
            if esc_flag:
                new += char # escaped % -> %%
                esc_flag = False
                continue
            esc_flag = True
            continue
        if esc_flag:
            new += negate(convert_special(char), neg_flag)
            esc_flag = False # reset if applied, same if not
            neg_flag = False # reset if applied, same if not
            continue
        # remove all "." dots
        if char == '.':
            continue
        # capturing everything else
        new += negate(convert_meta(char), neg_flag)
        neg_flag = False # reset if applied, same if not

    return new
# def convert_regex


def get_stag(tag_name):
    '''Get starting HTML tag of given tag name.'''
    simple = {"bold": "<b>", "italic": "<i>", "underline": "<u>", "teletype": "<tt>"}
    if tag_name in simple:
        return simple[tag_name]
    else:
        if tag_name.find("color") != -1:
            # was found; is color
            color = re.search("[0-9a-fA-F]{6}", tag_name)
            tag = "<font color=#" + color.group(0) + ">"
            return tag
        if tag_name.find("size") != -1:
            #was found; is size
            size = re. search("[1-7]", tag_name)
            tag = "<font size=" + size.group(0) + ">"
            return tag

#def get_stag


def get_etag(tag_name):
    '''Get ending HTML tag of given tag name.'''
    simple = {"bold": "</b>", "italic": "</i>", "underline": "</u>", "teletype": "</tt>"}
    if tag_name in simple:
        return simple[tag_name]
    else:
        return "</font>"
#def get_etag


def add_br(br_flag, in_str):
    '''Add <br /> tag at the end of each line'''
    if br_flag is False:
        return in_str
    out_str = re.sub("\n", "<br />\n", in_str)
    return out_str
# def add_br


def apply(args, in_str):
    '''Apply all syntax formating and return desired output'''
    in_str = add_br(args.br, in_str)
    return in_str
# def apply


def parse_args():
    '''Parse arguments into args object'''
    parser = argparse.ArgumentParser(prog='syn', usage='%(prog)s [options]',
                                     description='''Highlight syntax in given text.
                                     Script is working according to stored table of
                                     regexes to which is output format''',
                                     add_help=False)
    parser.add_argument('--format', nargs='?',
                        help='file containing format rules')
    parser.add_argument('--input', nargs='?', help='input file')
    parser.add_argument('--output', nargs='?', help='output file')
    parser.add_argument('--br', action='store_true',
                        help='add element <br /> at the end of each line')
    parser.add_argument('--help', action='store_true')
    # extension: HTML
    # parser.add_argument('--nooverlap', action='store_true',
    #                     help='stop tag overlapping')
    # parser.add_argument('--escape', action='store_true',
    #                     help='replace some symbols with HTML escape sequencies')


    # args = parser.parse_args()
    # to override exit code from 2 to 1
    try:
        args = parser.parse_args()
    except SystemExit:
        exit(1)

    if args.help is True:
        if len(sys.argv) > 2:
            error("Wrong parameters. See --help.", 2)
        parser.print_help()
        exit(0)

    return args
# def parse_args
