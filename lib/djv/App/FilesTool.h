// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the DJV project.

#pragma once

#include <djv/App/Export.h>
#include <djv/App/IToolWidget.h>
#include <djv/Models/Export.h>

#include <djv/Models/FilesModel.h>

namespace djv
{
    namespace app
    {
        //! Files tool.
        class DJV_APP_API_TYPE FilesTool : public IToolWidget
        {
            FTK_NON_COPYABLE(FilesTool);

        protected:
            void _init(
                const std::shared_ptr<ftk::Context>&,
                const std::shared_ptr<App>&,
                const std::shared_ptr<MainWindow>&,
                const std::shared_ptr<IWidget>& parent);

            FilesTool();

        public:
            DJV_APP_API virtual ~FilesTool();

            DJV_APP_API static std::shared_ptr<FilesTool> create(
                const std::shared_ptr<ftk::Context>&,
                const std::shared_ptr<App>&,
                const std::shared_ptr<MainWindow>&,
                const std::shared_ptr<IWidget>& parent = nullptr);

        private:
            void _rangeUpdate(
                const std::shared_ptr<models::FilesModelItem>&,
                const ftk::RangeI64&);
            void _filesUpdate(const std::vector<std::shared_ptr<models::FilesModelItem> >&);
            void _showRangePopup(
                const std::shared_ptr<models::FilesModelItem>&,
                const ftk::RangeI64&,
                const std::shared_ptr<ftk::IWidget>&);
            void _aUpdate(const std::shared_ptr<models::FilesModelItem>&);
            void _bUpdate(const std::vector<std::shared_ptr<models::FilesModelItem> >&);
            void _layersUpdate(const std::vector<int>&);
            void _compareUpdate(const tl::CompareOptions&);

            FTK_PRIVATE();
        };
    }
}
