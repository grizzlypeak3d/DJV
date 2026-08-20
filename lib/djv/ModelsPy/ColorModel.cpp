// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the DJV project.

#include <djv/ModelsPy/Bindings.h>

#include <djv/Models/ColorModel.h>

#include <ftk/CorePy/Bindings.h>
#include <ftk/Core/Context.h>
#include <ftk/UI/Settings.h>

#include <pybind11/functional.h>
#include <pybind11/stl.h>

namespace py = pybind11;

namespace djv
{
    namespace python
    {
        void colorModel(py::module_& m)
        {
            using namespace models;

            ftk::python::observable<tl::OCIOOptions>(m, "OCIOOptions");
            ftk::python::observable<tl::LUTOptions>(m, "LUTOptions");
            ftk::python::observable<std::vector<std::string> >(m, "StringVector");
            ftk::python::observable<ftk::ImageTags>(m, "ImageTags");

            py::class_<ColorModel, std::shared_ptr<ColorModel> >(m, "ColorModel")
                .def(
                    py::init(&ColorModel::create),
                    py::arg("context"),
                    py::arg("settings"))
                .def_property("ocioOptions", &ColorModel::getOCIOOptions, &ColorModel::setOCIOOptions, py::return_value_policy::copy)
                .def_property_readonly("observeOCIOOptions", &ColorModel::observeOCIOOptions)
                .def_property_readonly("observeResolvedOCIOOptions", &ColorModel::observeResolvedOCIOOptions)
                .def("setActiveFiles", &ColorModel::setActiveFiles)
                .def("resolveInput", &ColorModel::resolveInput,
                    py::arg("path"),
                    py::arg("tags") = ftk::ImageTags())
                .def_property_readonly("observeResolvedInputs", &ColorModel::observeResolvedInputs)
                .def_property_readonly("observeResolvedInput", &ColorModel::observeResolvedInput)
                .def_property("extColorSpaces", &ColorModel::getExtColorSpaces, &ColorModel::setExtColorSpaces)
                .def_property_readonly("observeExtColorSpaces", &ColorModel::observeExtColorSpaces)
                .def_property("lutOptions", &ColorModel::getLUTOptions, &ColorModel::setLUTOptions, py::return_value_policy::copy)
                .def_property_readonly("observeLUTOptions", &ColorModel::observeLUTOptions);
        }
    }
}
