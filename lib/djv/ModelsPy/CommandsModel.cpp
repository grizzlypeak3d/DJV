// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the DJV project.

#include <djv/ModelsPy/Bindings.h>

#include <djv/Models/CommandsModel.h>

#include <ftk/Core/Context.h>

#include <pybind11/functional.h>
#include <pybind11/stl.h>

namespace py = pybind11;

namespace djv
{
    namespace python
    {
        void commandsModel(py::module_& m)
        {
            using namespace models;

            py::class_<CommandInfo>(m, "CommandInfo")
                .def(py::init())
                .def_readwrite("name", &CommandInfo::name)
                .def_readwrite("doc", &CommandInfo::doc);

            // JSON crosses the language boundary as strings, following the
            // ftk::Settings bindings.
            py::class_<CommandsModel, std::shared_ptr<CommandsModel> >(m, "CommandsModel")
                .def(
                    py::init(&CommandsModel::create),
                    py::arg("context"))
                .def(
                    "add",
                    [](CommandsModel& model,
                        const std::string& name,
                        const std::string& doc,
                        const std::function<void(const std::string&)>& func)
                    {
                        model.add(
                            name,
                            doc,
                            [func](const nlohmann::json& args)
                            {
                                func(args.dump());
                            });
                    },
                    py::arg("name"),
                    py::arg("doc"),
                    py::arg("func"))
                .def("remove", &CommandsModel::remove, py::arg("name"))
                .def_property_readonly("commands", &CommandsModel::getCommands)
                .def(
                    "exec",
                    [](CommandsModel& model,
                        const std::string& name,
                        const std::string& args)
                    {
                        return model.exec(name, nlohmann::json::parse(args));
                    },
                    py::arg("name"),
                    py::arg("args") = std::string("null"));
        }
    }
}
