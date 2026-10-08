// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the DJV project.

#include <djv/AppPy/Bindings.h>

#include <djv/App/IToolWidget.h>
#include <djv/App/MainWindow.h>
#include <djv/UI/Viewport.h>

#include <tlRender/UI/TimelineWidget.h>

#include <ftk/UI/Action.h>
#include <ftk/UI/MenuBar.h>

namespace nb = nanobind;

namespace djv
{
    namespace python
    {
        void mainWindow(nb::module_& m)
        {
            using namespace djv::app;

            nb::class_<MainWindow, ftk::Window>(m, "MainWindow")
                .def("getMenuBar", &MainWindow::getMenuBar)
                .def("getViewport", &MainWindow::getViewport)
                .def("getTimelineWidget", &MainWindow::getTimelineWidget)
                .def("getAction", &MainWindow::getAction, nb::arg("name"))
                .def("getToolWidget", &MainWindow::getToolWidget, nb::arg("name"))
                .def_prop_rw("presentMode", &MainWindow::hasPresentMode, &MainWindow::setPresentMode)
                .def_prop_ro("observePresentMode", &MainWindow::observePresentMode)
                .def("setSplitters", &MainWindow::setSplitters, nb::arg("splitter"), nb::arg("splitter2"))
                .def("showAboutDialog", &MainWindow::showAboutDialog)
                .def("showSysInfoDialog", &MainWindow::showSysInfoDialog);
        }
    }
}
