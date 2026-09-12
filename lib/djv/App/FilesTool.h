// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the DJV project.

#pragma once

#include <djv/App/Export.h>
#include <djv/App/IToolWidget.h>
#include <djv/Models/Export.h>

#include <djv/Models/FilesModel.h>

namespace djv
{
    namespace ui
    {
        class FileDragDropData;
    }

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

            DJV_APP_API void setGeometry(const ftk::Box2I&) override;
            DJV_APP_API void sizeHintEvent(const ftk::SizeHintEvent&) override;
            DJV_APP_API void drawOverlayEvent(const ftk::Box2I&, const ftk::DrawEvent&) override;
            DJV_APP_API void dragEnterEvent(ftk::DragDropEvent&) override;
            DJV_APP_API void dragLeaveEvent(ftk::DragDropEvent&) override;
            DJV_APP_API void dragMoveEvent(ftk::DragDropEvent&) override;
            DJV_APP_API void dropEvent(ftk::DragDropEvent&) override;

        private:
            ftk::Box2I _getRowGeom(size_t) const;
            int _dropIndex(
                const ftk::V2I&,
                const std::shared_ptr<ui::FileDragDropData>&) const;
            int _getDropIndex(const ftk::V2I&) const;
            ftk::Box2I _getDropGeom(int) const;

            void _rangeUpdate(
                const std::shared_ptr<models::FilesModelItem>&,
                const ftk::RangeI64&);
            // What the file turned out to hold, which arrives with the row's
            // thumbnail rather than by opening the file. Takes what it needs
            // rather than the I/O information itself, to keep the readers out
            // of this header.
            void _infoUpdate(
                const std::shared_ptr<models::FilesModelItem>&,
                const std::optional<OTIO_NS::TimeRange>&,
                const std::vector<std::string>&);
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
