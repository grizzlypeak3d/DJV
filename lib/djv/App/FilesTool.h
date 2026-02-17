// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the DJV project.

#pragma once

#include <djv/App/IToolWidget.h>

#include <djv/Models/FilesModel.h>

namespace djv
{
    namespace app
    {
        class App;

        //! Files tool.
        class FilesTool : public IToolWidget
        {
            FTK_NON_COPYABLE(FilesTool);

        protected:
            void _init(
                const std::shared_ptr<ftk::Context>&,
                const std::shared_ptr<App>&,
                const std::shared_ptr<IWidget>& parent);

            FilesTool();

        public:
            virtual ~FilesTool();

            static std::shared_ptr<FilesTool> create(
                const std::shared_ptr<ftk::Context>&,
                const std::shared_ptr<App>&,
                const std::shared_ptr<IWidget>& parent = nullptr);

        private:
            void _filesUpdate(const std::vector<std::shared_ptr<models::FilesModelItem> >&);
            void _aUpdate(const std::shared_ptr<models::FilesModelItem>&);
            void _bUpdate(const std::vector<std::shared_ptr<models::FilesModelItem> >&);
            void _layersUpdate(const std::vector<int>&);
            void _compareUpdate(const tl::CompareOptions&);

            FTK_PRIVATE();
        };
    }
}
