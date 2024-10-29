#!/usr/bin/python3

import os
import curses
import fnmatch
import subprocess

def locate(pattern, root_path):
    for path, dirs, files in os.walk(os.path.abspath(root_path)):
        for filename in fnmatch.filter(files, pattern):
            yield os.path.join(path, filename)

cboy_path = "cboy"
sdcard_path = "/mnt/sd"
local_path = "/var/root/games/"

classes_full = ["Quit"] + [js for js in locate('*.gb', local_path)] + [js for js in locate('*.gbc', local_path)] + \
     [js for js in locate('*.gb', sdcard_path)] + [js for js in locate('*.gbc', sdcard_path)]

chosen = ""

def character(stdscr):
    global chosen

    attributes = {}
    curses.init_pair(1, curses.COLOR_WHITE, curses.COLOR_BLACK)
    attributes['normal'] = curses.color_pair(1)

    curses.init_pair(2, curses.COLOR_BLACK, curses.COLOR_WHITE)
    attributes['highlighted'] = curses.color_pair(2)

    c = 0  # last character read
    option = 0  # the current option that is marked
    classes = classes_full[:25]
    while c != 10:  # Enter in ascii
        stdscr.erase()
        stdscr.addstr("Select Gameboy Game\n", curses.A_UNDERLINE)
        stdscr.addstr("You can insert an SD to load more games\n")
        stdscr.addstr("Press Power + Encoder to exit emulator!\n")
        for i in range(len(classes)):
            if i == option:
                attr = attributes['highlighted']
            else:
                attr = attributes['normal']
            stdscr.addstr(" - ")
            stdscr.addstr(os.path.basename(classes[i]) + '\n', attr)
        c = stdscr.getch()
        if c == curses.KEY_UP and option > 0:
            option -= 1
        elif c == curses.KEY_DOWN and option < len(classes) - 1:
            option += 1

    if option == 0:
        exit(0)
    else:
        chosen = classes[option]


curses.wrapper(character)

if chosen:
    subprocess.call([cboy_path, chosen])
