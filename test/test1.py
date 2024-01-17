#!/usr/bin/python3

# This test opens the interface and just clicks around a bit.

import os
import dogtail.config
dogtail.config.config.logDebugToStdOut = True
dogtail.config.config.logDebugToFile = False

from dogtail.procedural import *

os.environ['LANG']='C'
run('lector')

# Test file->open
focus.application('lector')
click('File', roleName='menu')
click('Open…', roleName='menu item')
click('Cancel', roleName='push button')

# Toolbar editor
focus.application('lector')
click('Edit', roleName='menu')
click('Toolbar', roleName='menu item')
click('Close', roleName='push button')

# About dialog
focus.application('lector')
click('Help', roleName='menu')
click('About', roleName='menu item')
click('Credits', roleName='toggle button')
click('Credits', roleName='toggle button')
click('Close', roleName='push button')

# Close lector
focus.application('lector')
click.menu('File')
click('Close', roleName='menu item')
