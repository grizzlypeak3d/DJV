// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the DJV project.

#include <djv/ModelsPy/Bindings.h>

#include <djv/Models/ExportQueue.h>

#include <ftk/Core/Context.h>

#include <nanobind/stl/shared_ptr.h>
#include <nanobind/stl/string.h>
#include <nanobind/stl/vector.h>

namespace nb = nanobind;

namespace djv
{
    namespace python
    {
        void exportQueue(nb::module_& m)
        {
            using namespace models;

            nb::enum_<ExportStatus>(m, "ExportStatus")
                .value("Queued", ExportStatus::Queued)
                .value("Running", ExportStatus::Running)
                .value("Finished", ExportStatus::Finished)
                .value("Failed", ExportStatus::Failed)
                .value("Cancelled", ExportStatus::Cancelled);

            nb::class_<ExportProgress>(m, "ExportProgress")
                .def(nb::init())
                .def_rw("status", &ExportProgress::status)
                .def_rw("framesDone", &ExportProgress::framesDone)
                .def_rw("frames", &ExportProgress::frames)
                .def_rw("timeLeft", &ExportProgress::timeLeft)
                .def_rw("error", &ExportProgress::error);

            m.def("getLabel", nb::overload_cast<const ExportProgress&>(&getLabel), nb::arg("progress"));

            nb::class_<ExportJob>(m, "ExportJob")
                .def_prop_ro("name", [](const ExportJob& job) { return job.getOptions().name; })
                .def_prop_ro("path", [](const ExportJob& job) { return job.getOptions().path.get(); })
                .def_prop_ro("progress", &ExportJob::getProgress);

            nb::class_<ExportQueue>(m, "ExportQueue")
                .def_prop_ro(
                    "jobs",
                    [](const ExportQueue& queue) { return queue.observeJobs()->get(); })
                .def_prop_ro(
                    "busy",
                    [](const ExportQueue& queue) { return queue.observeBusy()->get(); })
                .def("cancel", &ExportQueue::cancel, nb::arg("job"))
                .def("cancelAll", &ExportQueue::cancelAll)
                .def("remove", &ExportQueue::remove, nb::arg("job"))
                .def("clearDone", &ExportQueue::clearDone);
        }
    }
}
