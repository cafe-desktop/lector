#!/usr/bin/python3

# This test opens a file with wrong extenstion.

import os
os.environ['LANG']='C'
srcdir = os.environ['srcdir']

from dogtail.procedural import *

run('lector', arguments=' '+srcdir+'/test-mime.bin')

# Close lector
focus.application('lector')
click('File', roleName='menu')
click('Close', roleName='menu item')
