#!/usr/bin/python3

# This test tries document reload action.

import os
os.environ['LANG']='C'
srcdir = os.environ['srcdir']

from dogtail.procedural import *

run('lector', arguments=' '+srcdir+'/test-links.pdf')

# Reload document a few times
for i in range(1,6):
    focus.application('lector')
    click('View', roleName='menu')
    click('Reload', roleName='menu item')

# Close lector
focus.application('lector')
click('File', roleName='menu')
click('Close', roleName='menu item')
