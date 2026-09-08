// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the DJV project.

#include <djv/ModelsPy/Bindings.h>

#include <tlRender/TimelinePy/OTIOCasters.h>

#include <nanobind/stl/shared_ptr.h>
#include <nanobind/stl/string.h>

#include <djv/Models/ColorModel.h>

#include <ftk/CorePy/Bindings.h>
#include <ftk/Core/Context.h>
#include <ftk/UI/Settings.h>

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
        void colorModel(nb::module_& m)
        {
            using namespace models;

            ftk::python::observable<tl::OCIOOptions>(m, "OCIOOptions");
            ftk::python::observable<tl::LUTOptions>(m, "LUTOptions");
            ftk::python::observable<std::vector<std::string> >(m, "StringVector");
            ftk::python::observable<ftk::ImageTags>(m, "ImageTags");

            nb::class_<ColorModel>(m, "ColorModel")
                .def(
                    nb::new_(&ColorModel::create),
                    nb::arg("context"),
                    nb::arg("settings"))
                .def("save", &ColorModel::save)
                .def_prop_rw("ocioOptions", &ColorModel::getOCIOOptions, &ColorModel::setOCIOOptions, nb::rv_policy::copy)
                .def_prop_ro("observeOCIOOptions", &ColorModel::observeOCIOOptions)
                .def_prop_ro("observeResolvedOCIOOptions", &ColorModel::observeResolvedOCIOOptions)
                .def("setActiveFiles", &ColorModel::setActiveFiles)
                .def("resolveInput", &ColorModel::resolveInput,
                    nb::arg("path"),
                    nb::arg("tags") = ftk::ImageTags())
                .def_prop_ro("observeResolvedInputs", &ColorModel::observeResolvedInputs)
                .def_prop_ro("observeResolvedInput", &ColorModel::observeResolvedInput)
                .def_prop_rw("extColorSpaces", &ColorModel::getExtColorSpaces, &ColorModel::setExtColorSpaces)
                .def_prop_ro("observeExtColorSpaces", &ColorModel::observeExtColorSpaces)
                .def_prop_rw("lutOptions", &ColorModel::getLUTOptions, &ColorModel::setLUTOptions, nb::rv_policy::copy)
                .def_prop_ro("observeLUTOptions", &ColorModel::observeLUTOptions);
        }
    }
}
