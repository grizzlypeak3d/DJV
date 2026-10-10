// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the DJV project.

#pragma once

#include <djv/UI/Export.h>

#include <ftk/UI/IDialog.h>
#include <ftk/Core/Path.h>

namespace ftk
{
    class FileBrowserModel;
}

namespace djv
{
    namespace ui
    {
        //! Recent files dialog: the recent files as a list to search and
        //! open from, with thumbnails, where the menu has room for a few
        //! names. It is also where the recent lists are cleared (DJV
        //! #909).
        //!
        //! The list is the file browser's, so the two look and work the
        //! same; the model is the file browser's too, for its thumbnail
        //! options.
        class DJV_UI_API_TYPE RecentFilesDialog : public ftk::IDialog
        {
            FTK_NON_COPYABLE(RecentFilesDialog);

        protected:
            void _init(
                const std::shared_ptr<ftk::Context>&,
                const std::shared_ptr<ftk::FileBrowserModel>&,
                const std::vector<ftk::Path>& recent,
                bool clearOnExit,
                const std::shared_ptr<IWidget>& parent);

            RecentFilesDialog();

        public:
            DJV_UI_API virtual ~RecentFilesDialog();

            //! Create a new dialog; "recent" is the files with the most
            //! recent first.
            DJV_UI_API static std::shared_ptr<RecentFilesDialog> create(
                const std::shared_ptr<ftk::Context>&,
                const std::shared_ptr<ftk::FileBrowserModel>&,
                const std::vector<ftk::Path>& recent,
                bool clearOnExit,
                const std::shared_ptr<IWidget>& parent = nullptr);

            //! Set the recent files, with the most recent first.
            DJV_UI_API void setRecent(const std::vector<ftk::Path>&);

            //! Set the callback for opening files; the dialog then closes
            //! itself.
            DJV_UI_API void setCallback(const std::function<void(const std::vector<ftk::Path>&)>&);

            //! Set the callback for clearing the recent lists.
            DJV_UI_API void setClearCallback(const std::function<void(void)>&);

            //! Set the callback for whether the recent lists are cleared
            //! on exit.
            DJV_UI_API void setClearOnExitCallback(const std::function<void(bool)>&);

            //! The search box: the dialog is opened to find a file.
            DJV_UI_API std::shared_ptr<ftk::IWidget> getKeyFocus() const override;

        private:
            FTK_PRIVATE();
        };
    }
}
