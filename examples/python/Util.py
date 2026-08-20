# SPDX-License-Identifier: BSD-3-Clause
# Copyright Contributors to the DJV project.

import weakref

def weak(method):
    """
    Wrap a bound method for use as a widget or action callback.

    A callback handed to the C++ side is held by a std::function, an
    edge the Python garbage collector cannot see. A bound method or a
    lambda that captures self strongly therefore forms a cycle that is
    never collected, and everything the cycle holds -- models, settings,
    the context -- leaks until process exit. The settings are written
    when those objects are destroyed, so the leaks turn into settings
    that never make it to disk.
    """
    r = weakref.WeakMethod(method)
    def call(*args):
        m = r()
        if m is not None:
            return m(*args)
    return call
