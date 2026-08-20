# SPDX-License-Identifier: BSD-3-Clause
# Copyright Contributors to the DJV project.

import ftkPy as ftk
import tlRenderPy as tl
import djvPy as djv

import json
import os
import tempfile
import unittest

class ModelsTest(unittest.TestCase):

    def setUp(self):
        self.context = ftk.Context()
        tl.init(self.context)
        self.tempDir = tempfile.mkdtemp()
        self.settings = ftk.Settings(
            self.context,
            os.path.join(self.tempDir, "settings.json"))

    def test_appInfoModel(self):
        model = djv.models.AppInfoModel()
        self.assertEqual("DJV", model.fullName)
        self.assertTrue(len(model.version) > 0)

    def filesCallback(self, value):
        self.files = value

    def compareOptionsCallback(self, value):
        self.compareOptions.append(value)

    def test_filesModel(self):
        model = djv.models.FilesModel(self.settings)

        self.files = None
        observer = djv.models.FilesModelItemListObserver(
            model.observeFiles,
            self.filesCallback)
        self.assertEqual([], self.files)

        self.compareOptions = []
        observer2 = djv.models.CompareOptionsObserver(
            model.observeCompareOptions,
            self.compareOptionsCallback)
        options = tl.CompareOptions()
        options.compare = tl.Compare.Wipe
        model.compareOptions = options
        self.assertEqual(options, model.compareOptions)
        # "None" is a Python keyword so the enum value is accessed
        # with getattr().
        self.assertEqual(
            [tl.Compare._None, tl.Compare.Wipe],
            [i.compare for i in self.compareOptions])

    def lutOptionsCallback(self, value):
        self.lutOptions.append(value)

    def ocioOptionsCallback(self, value):
        self.ocioOptions.append(value)

    def test_colorModel(self):
        model = djv.models.ColorModel(self.context, self.settings)

        # Options are returned by copy; modifying the copy does not
        # change the model.
        options = model.lutOptions
        options.enabled = True
        self.assertFalse(model.lutOptions.enabled)

        self.lutOptions = []
        observer = djv.models.LUTOptionsObserver(
            model.observeLUTOptions,
            self.lutOptionsCallback)
        model.lutOptions = options
        self.assertTrue(model.lutOptions.enabled)
        self.assertEqual([False, True], [i.enabled for i in self.lutOptions])

        options = model.ocioOptions
        options.enabled = True
        self.assertFalse(model.ocioOptions.enabled)

        self.ocioOptions = []
        observer2 = djv.models.OCIOOptionsObserver(
            model.observeOCIOOptions,
            self.ocioOptionsCallback)
        model.ocioOptions = options
        self.assertTrue(model.ocioOptions.enabled)
        self.assertEqual([False, True], [i.enabled for i in self.ocioOptions])

    def ocioDataCallback(self, value):
        self.ocioData = value

    def test_ocioModel(self):
        model = djv.models.OCIOModel(self.context)
        self.ocioData = None
        observer = djv.models.OCIOModelDataObserver(
            model.observeData,
            self.ocioDataCallback)
        self.assertIsInstance(self.ocioData, djv.models.OCIOModelData)

    def openToolsCallback(self, value):
        self.openTools = value

    def test_toolsModel(self):
        model = djv.models.ToolsModel(self.settings)
        tools = model.tools
        self.assertTrue(len(tools) > 0)
        names = [i.name for i in tools]
        self.assertIn("Files", names)

        self.openTools = None
        observer = ftk.StringListObserver(
            model.observeOpenTools,
            self.openToolsCallback)
        self.assertEqual([], self.openTools)
        model.setToolOpen("Files", True)
        self.assertTrue(model.isToolOpen("Files"))
        self.assertEqual(["Files"], self.openTools)
        model.setToolOpen("Files", False)
        self.assertFalse(model.isToolOpen("Files"))
        self.assertEqual([], self.openTools)

    def test_timeUnitsModel(self):
        model = djv.models.TimeUnitsModel(self.context, self.settings)
        self.assertIsInstance(model, tl.TimeUnitsModel)

    def test_recentFilesModel(self):
        model = djv.models.RecentFilesModel(self.context, self.settings)
        self.assertIsInstance(model, ftk.RecentFilesModel)
        path = os.path.join(self.tempDir, "foo.mov")
        model.addRecent(ftk.Path(path))
        self.assertEqual([path], model.recent)

    def hudOptionsCallback(self, value):
        self.hudOptions.append(value)

    def aspectRatioOptionsCallback(self, value):
        self.aspectRatioOptions.append(value)

    def backgroundOptionsCallback(self, value):
        self.backgroundOptions.append(value)

    def test_viewportModel(self):
        model = djv.models.ViewportModel(self.context, self.settings)

        # Options are returned by copy; modifying the copy does not
        # change the model.
        options = model.hudOptions
        options.enabled = True
        self.assertFalse(model.hudOptions.enabled)

        self.hudOptions = []
        observer = djv.models.HUDOptionsObserver(
            model.observeHUDOptions,
            self.hudOptionsCallback)
        model.hudOptions = options
        self.assertTrue(model.hudOptions.enabled)
        self.assertEqual([False, True], [i.enabled for i in self.hudOptions])

        options = model.aspectRatioOptions
        options.index = 1
        self.aspectRatioOptions = []
        observer2 = djv.models.AspectRatioOptionsObserver(
            model.observeAspectRatioOptions,
            self.aspectRatioOptionsCallback)
        model.aspectRatioOptions = options
        self.assertEqual(1, model.aspectRatioOptions.index)
        self.assertEqual([0, 1], [i.index for i in self.aspectRatioOptions])

        options = model.backgroundOptions
        options.type = tl.Background.Checkers
        self.backgroundOptions = []
        observer3 = djv.models.BackgroundOptionsObserver(
            model.observeBackgroundOptions,
            self.backgroundOptionsCallback)
        model.backgroundOptions = options
        self.assertEqual(tl.Background.Checkers, model.backgroundOptions.type)
        self.assertEqual(
            [tl.Background.Solid, tl.Background.Checkers],
            [i.type for i in self.backgroundOptions])

    def commandCallback(self, args):
        self.commandArgs = args

    def test_commandsModel(self):
        model = djv.models.CommandsModel(self.context)
        self.commandArgs = None
        model.add("Test", "Test documentation", self.commandCallback)
        names = [i.name for i in model.commands]
        self.assertIn("Test", names)
        self.assertTrue(model.exec("Test", "[1, 2]"))
        self.assertEqual([1, 2], json.loads(self.commandArgs))

    def test_shortcut(self):
        a = djv.models.Shortcut("Name", "Text")
        b = djv.models.Shortcut("Name", "Text")
        self.assertEqual(a, b)
        b.name = "Name2"
        self.assertNotEqual(a, b)

    def volumeCallback(self, value):
        self.volume = value

    def test_audioModel(self):
        model = djv.models.AudioModel(self.context, self.settings)

        self.volume = None
        observer = ftk.FloatObserver(
            model.observeVolume,
            self.volumeCallback)
        model.volume = .5
        self.assertAlmostEqual(.5, model.volume)
        self.assertAlmostEqual(.5, self.volume)

        model.volume = 1.0
        model.volumeUp()
        self.assertAlmostEqual(1.0, model.volume)
        model.volume = 0.0
        model.volumeDown()
        self.assertAlmostEqual(0.0, model.volume)

if __name__ == '__main__':
    unittest.main()
