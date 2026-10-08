// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the DJV project.

#include <djv/AppPy/Bindings.h>

#include <djv/App/App.h>
#include <djv/App/IToolWidget.h>
#include <djv/App/MainWindow.h>
#include <djv/Models/AnnotationsModel.h>
#include <djv/Models/AppInfoModel.h>
#include <djv/Models/AudioModel.h>
#include <djv/Models/ColorModel.h>
#include <djv/Models/CommandsModel.h>
#include <djv/Models/DrawModel.h>
#include <djv/Models/FilesModel.h>
#include <djv/Models/MarkersModel.h>
#include <djv/Models/RecentFilesModel.h>
#include <djv/Models/SettingsModel.h>
#include <djv/Models/TimeUnitsModel.h>
#include <djv/Models/ToolsModel.h>
#include <djv/Models/ViewportModel.h>

#include <tlRender/Timeline/Player.h>

#include <ftk/UIPy/WidgetTrampoline.h>
#include <ftk/UI/SysLogModel.h>
#include <ftk/Core/Path.h>

#include <nanobind/trampoline.h>

namespace nb = nanobind;

namespace djv
{
    namespace python
    {
        namespace
        {
            //! The application, made from Python. The initialization hooks
            //! are what a Python program overrides to extend it: the tools
            //! and the factory exist after _uiInit(), and the window after
            //! _mainWindowInit().
            class PyApp : public app::App
            {
            public:
                NB_TRAMPOLINE(app::App);

                PyApp() :
                    _anchor(this, [](app::App*) {})
                {}

                void pyInit(
                    const std::shared_ptr<ftk::Context>& context,
                    std::vector<std::string> argv,
                    const std::shared_ptr<models::AppInfoModel>& appInfoModel)
                {
                    _init(context, argv, appInfoModel);
                }

                void tick() override
                {
                    NB_OVERRIDE(tick);
                }

                void _modelsInit() override
                {
                    NB_OVERRIDE(_modelsInit);
                }

                void _observersInit() override
                {
                    NB_OVERRIDE(_observersInit);
                }

                void _inputFilesInit() override
                {
                    NB_OVERRIDE(_inputFilesInit);
                }

                void _uiInit() override
                {
                    NB_OVERRIDE(_uiInit);
                }

                void _mainWindowInit() override
                {
                    NB_OVERRIDE(_mainWindowInit);
                }

                //! The base's part of each hook, for an override to call.
                void baseModelsInit() { app::App::_modelsInit(); }
                void baseObserversInit() { app::App::_observersInit(); }
                void baseInputFilesInit() { app::App::_inputFilesInit(); }
                void baseUIInit() { app::App::_uiInit(); }
                void baseMainWindowInit() { app::App::_mainWindowInit(); }

            private:
                std::shared_ptr<app::App> _anchor;
            };
        }

        void app(nb::module_& m)
        {
            using namespace djv::app;

            nb::class_<App, ftk::App, PyApp>(m, "App")
                .def(
                    "__init__",
                    [](App* self,
                        const std::shared_ptr<ftk::Context>& context,
                        const std::vector<std::string>& argv,
                        const std::shared_ptr<models::AppInfoModel>& appInfoModel)
                    {
                        ftk::python::pyConstruct<PyApp>(self,
                            [&](PyApp& a)
                            {
                                a.pyInit(context, argv, appInfoModel);
                            });
                    },
                    nb::arg("context"),
                    nb::arg("argv"),
                    nb::arg("appInfoModel") = nullptr,
                    // The context outlives the application; see
                    // ftk::Window.
                    nb::keep_alive<1, 2>())
                .def("getAppInfoModel", &App::getAppInfoModel)
                .def("getSettingsModel", &App::getSettingsModel)
                .def("getSysLogModel", &App::getSysLogModel)
                .def("getTimeUnitsModel", &App::getTimeUnitsModel)
                .def("getFilesModel", &App::getFilesModel)
                .def("getRecentFilesModel", &App::getRecentFilesModel)
                .def("getColorModel", &App::getColorModel)
                .def("getViewportModel", &App::getViewportModel)
                .def("getAudioModel", &App::getAudioModel)
                .def("getToolsModel", &App::getToolsModel)
                .def("getCommandsModel", &App::getCommandsModel)
                .def("getMarkersModel", &App::getMarkersModel)
                .def("getAnnotationsModel", &App::getAnnotationsModel)
                .def("getDrawModel", &App::getDrawModel)
                .def("getToolWidgetFactory", &App::getToolWidgetFactory)
                .def("getMainWindow", &App::getMainWindow)
                .def_prop_ro("observePlayer", &App::observePlayer)
                .def(
                    "open",
                    [](App& self, const ftk::Path& path)
                    {
                        self.open(path);
                    },
                    nb::arg("path"))
                .def("openDialog", &App::openDialog)
                .def("closeFile", &App::closeFile, nb::arg("index") = -1)
                .def("closeAllFiles", &App::closeAllFiles)
                .def("reload", &App::reload)
                // The hooks, callable from an override as the base's part.
                // An application made from Python is always the trampoline.
                .def("_modelsInit", [](App& self) { static_cast<PyApp&>(self).baseModelsInit(); })
                .def("_observersInit", [](App& self) { static_cast<PyApp&>(self).baseObserversInit(); })
                .def("_inputFilesInit", [](App& self) { static_cast<PyApp&>(self).baseInputFilesInit(); })
                .def("_uiInit", [](App& self) { static_cast<PyApp&>(self).baseUIInit(); })
                .def("_mainWindowInit", [](App& self) { static_cast<PyApp&>(self).baseMainWindowInit(); });
        }
    }
}
