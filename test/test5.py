#!/usr/bin/python3

# This test tries document reload action.

import os
os.environ['LANG']='C'
srcdir = os.environ['srcdir']

from dogtail.procedural import *

run('lector', arguments=' '+srcdir+'/test-page-labels.pdf')

focus.application('lector')
focus.widget('page-label-entry')
focus.widget.text = "iii"
activate()

if focus.widget.text != "III":
    focus.application('lector')
    click('File', roleName='menu')
    click('Close', roleName='menu item')
    exit (1)

# Close lector
focus.application('lector')
click('File', roleName='menu')
click('Close', roleName='menu item')
