# SPDX-License-Identifier: BSD-3-Clause
# Copyright Contributors to the DJV project.

import ftkPy as ftk
import tlRenderPy as tl
import djvPy as djv

import os
import tempfile
import unittest

@unittest.skipIf(os.environ.get("DJV_TESTS_NO_GL"), "OpenGL is not available")
class SettingsModelTest(unittest.TestCase):

    def setUp(self):
        self.context = ftk.Context()
        tl.ui.init(self.context)
        self.tempDir = tempfile.mkdtemp()
        self.settingsPath = os.path.join(self.tempDir, "settings.json")
        self.settings = ftk.Settings(self.context, self.settingsPath)

    def test_window(self):
        model = djv.models.SettingsModel(self.context, self.settings, 1.0)
        sizes = []
        observer = djv.models.WindowSettingsObserver(
            model.observeWindow,
            lambda value: sizes.append(value.size))

        # Settings are returned by copy; modifying the copy does not
        # change the model.
        window = model.window
        window.size = ftk.Size2I(1920, 1080)
        self.assertNotEqual(ftk.Size2I(1920, 1080), model.window.size)

        model.window = window
        self.assertEqual(ftk.Size2I(1920, 1080), model.window.size)
        self.assertEqual(ftk.Size2I(1920, 1080), sizes[-1])
        self.assertEqual(2, len(sizes))

    def test_playback(self):
        model = djv.models.SettingsModel(self.context, self.settings, 1.0)
        playback = model.playback
        playback.startPlayback = not playback.startPlayback
        model.playback = playback
        self.assertEqual(playback, model.playback)

    def test_timeline(self):
        model = djv.models.SettingsModel(self.context, self.settings, 1.0)
        timeline = model.timeline
        timeline.frameView = not timeline.frameView
        model.timeline = timeline
        self.assertEqual(timeline, model.timeline)

    def test_style(self):
        model = djv.models.SettingsModel(self.context, self.settings, 1.0)
        style = model.style
        style.displayScale = 2.0
        model.style = style
        self.assertEqual(2.0, model.style.displayScale)

    def test_save(self):
        model = djv.models.SettingsModel(self.context, self.settings, 1.0)
        model.save()
        self.settings.save()
        self.assertTrue(os.path.exists(self.settingsPath))

if __name__ == '__main__':
    unittest.main()
