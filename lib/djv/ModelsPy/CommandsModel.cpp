// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the DJV project.

#include <djv/ModelsPy/Bindings.h>

#include <tlRender/TimelinePy/OTIOCasters.h>

#include <nanobind/stl/shared_ptr.h>
#include <nanobind/stl/string.h>

#include <djv/Models/CommandsModel.h>

#include <ftk/Core/Context.h>

#include <nanobind/stl/function.h>
#include <nanobind/stl/string.h>
#include <nanobind/stl/vector.h>
#include <nanobind/stl/list.h>
#include <nanobind/stl/map.h>
#include <nanobind/stl/pair.h>
#include <nanobind/stl/optional.h>
#include <nanobind/stl/shared_ptr.h>
#include <nanobind/stl/filesystem.h>

namespace nb = nanobind;

namespace djv
{
    namespace python
    {
        void commandsModel(nb::module_& m)
        {
            using namespace models;

            nb::class_<CommandInfo>(m, "CommandInfo")
                .def(nb::init())
                .def_rw("name", &CommandInfo::name)
                .def_rw("doc", &CommandInfo::doc);

            // JSON crosses the language boundary as strings, following the
            // ftk::Settings bindings.
            nb::class_<CommandsModel>(
                m, "CommandsModel",
                // The Python examples hold this through weakref, which
                // nanobind classes opt into.
                nb::is_weak_referenceable())
                .def(
                    nb::new_(&CommandsModel::create),
                    nb::arg("context"))
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
                    nb::arg("name"),
                    nb::arg("doc"),
                    nb::arg("func"))
                .def("remove", &CommandsModel::remove, nb::arg("name"))
                .def_prop_ro("commands", &CommandsModel::getCommands)
                .def(
                    "exec",
                    [](CommandsModel& model,
                        const std::string& name,
                        const std::string& args)
                    {
                        return model.exec(name, nlohmann::json::parse(args));
                    },
                    nb::arg("name"),
                    nb::arg("args") = std::string("null"));
        }
    }
}
