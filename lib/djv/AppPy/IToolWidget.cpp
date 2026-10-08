// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the DJV project.

#include <djv/AppPy/Bindings.h>

#include <djv/App/App.h>
#include <djv/App/IToolWidget.h>
#include <djv/App/MainWindow.h>

#include <ftk/UIPy/WidgetTrampoline.h>

namespace nb = nanobind;

namespace djv
{
    namespace python
    {
        namespace
        {
            //! A tool written in Python: the frame, title and close button
            //! are the C++ base's, and the content is what Python sets.
            class PyIToolWidget : public ftk::python::PyWidget<app::IToolWidget>
            {
            public:
                void pyInit(
                    const std::shared_ptr<ftk::Context>& context,
                    const std::shared_ptr<app::App>& app,
                    const std::shared_ptr<app::MainWindow>& mainWindow,
                    const std::string& name,
                    const std::string& icon,
                    const std::string& objectName,
                    const std::shared_ptr<ftk::IWidget>& parent)
                {
                    _init(context, app, mainWindow, name, icon, objectName, parent);
                }

                void pySetWidget(const std::shared_ptr<ftk::IWidget>& value)
                {
                    _setWidget(value);
                }

                void scrollTo(const std::string& section) override
                {
                    NB_OVERRIDE(scrollTo, section);
                }
            };
        }

        void toolWidget(nb::module_& m)
        {
            using namespace djv::app;

            nb::class_<IToolWidget, ftk::IWidget, PyIToolWidget>(m, "IToolWidget")
                .def(
                    "__init__",
                    [](IToolWidget* self,
                        const std::shared_ptr<ftk::Context>& context,
                        const std::shared_ptr<App>& app,
                        const std::shared_ptr<MainWindow>& mainWindow,
                        const std::string& name,
                        const std::string& icon,
                        const std::string& objectName,
                        const std::shared_ptr<ftk::IWidget>& parent)
                    {
                        ftk::python::pyConstruct<PyIToolWidget>(self,
                            [&](PyIToolWidget& w)
                            {
                                w.pyInit(context, app, mainWindow, name, icon, objectName, parent);
                            });
                    },
                    nb::arg("context"),
                    nb::arg("app"),
                    nb::arg("mainWindow"),
                    nb::arg("name"),
                    nb::arg("icon"),
                    nb::arg("objectName") = std::string(),
                    nb::arg("parent") = nullptr)
                .def_prop_ro("toolName", &IToolWidget::getToolName)
                // The content below the title row. For a tool made in
                // Python; the C++ tools set their own.
                .def(
                    "setWidget",
                    [](IToolWidget& self, const std::shared_ptr<ftk::IWidget>& value)
                    {
                        if (auto py = dynamic_cast<PyIToolWidget*>(&self))
                        {
                            py->pySetWidget(value);
                        }
                    },
                    nb::arg("widget"))
                .def("scrollTo", &IToolWidget::scrollTo, nb::arg("section"))
                .def("openSection", &IToolWidget::openSection, nb::arg("section"));

            nb::class_<ToolWidgetFactory>(m, "ToolWidgetFactory")
                // The function takes the context, the application, the main
                // window and the parent widget, and returns the tool.
                .def("addTool", &ToolWidgetFactory::addTool, nb::arg("name"), nb::arg("func"));
        }
    }
}
