# SPDX-License-Identifier: BSD-3-Clause
# Copyright Contributors to the DJV project.

"""The DJV application, started from Python and extended with a tool and a
command of its own; see App.py for the extensions and NotesTool.py for the
tool."""

import sys
import feather_tk as ftk
import tlrender as tl
import djv

import App

context = ftk.Context()
tl.ui.init(context)
app = App.App(context, sys.argv)
if app.hasCmdLineHelp:
    sys.exit(0)
app.run()
app = None
