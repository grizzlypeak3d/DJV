// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the DJV project.

#pragma once

#include <djv/App/Export.h>
#include <djv/Models/Export.h>
#include <djv/Models/SettingsModel.h>

#include <tlRender/Timeline/Player.h>

#include <ftk/UI/App.h>
#include <ftk/Core/ObservableList.h>

#include <filesystem>
#include <functional>
#include <optional>

namespace ftk
{
    class ICmdLineOption;
    class Settings;
    class SysLogModel;

    enum class FileBrowserMode;
}

namespace djv
{
    namespace models
    {
        struct FilesModelItem;
        struct Review;

        class AnnotationsModel;
        class AppInfoModel;
        class AudioModel;
        class ColorModel;
        class CommandsModel;
        class DrawModel;
        class FilesModel;
        class MarkersModel;
        class RecentFilesModel;
        class TimeUnitsModel;
        class ToolsModel;
        class ViewportModel;
    }

    namespace ui
    {
        class StatusIndicator;
    }

    //! DJV Application
    namespace app
    {
        class MainWindow;
        class ToolWidgetFactory;

        //! Application.
        class DJV_APP_API_TYPE App : public ftk::App
        {
            FTK_NON_COPYABLE(App);

        protected:
            //! The trailing options are a derived application's own, added
            //! to this one's. Without them a suite application has nowhere
            //! to register an option: the list here is fixed, and anything
            //! it does not know falls through to the inputs argument.
            DJV_APP_API void _init(
                const std::shared_ptr<ftk::Context>&,
                std::vector<std::string>&,
                const std::shared_ptr<models::AppInfoModel>&,
                const std::vector<std::shared_ptr<ftk::ICmdLineOption> >& = {});

            DJV_APP_API App();

        public:
            DJV_APP_API ~App();

            //! Create a new application.
            DJV_APP_API static std::shared_ptr<App> create(
                const std::shared_ptr<ftk::Context>&,
                std::vector<std::string>&,
                const std::shared_ptr<models::AppInfoModel>& = nullptr);

            //! Get the application information model.
            DJV_APP_API const std::shared_ptr<models::AppInfoModel>& getAppInfoModel() const;

            //! Get the settings model.
            DJV_APP_API const std::shared_ptr<models::SettingsModel>& getSettingsModel() const;

            //! Get the system log model.
            DJV_APP_API const std::shared_ptr<ftk::SysLogModel>& getSysLogModel() const;

            //! Get the time units model.
            DJV_APP_API const std::shared_ptr<models::TimeUnitsModel>& getTimeUnitsModel() const;

            //! Get the files model.
            DJV_APP_API const std::shared_ptr<models::FilesModel>& getFilesModel() const;

            //! Get the recent files model.
            DJV_APP_API const std::shared_ptr<models::RecentFilesModel>& getRecentFilesModel() const;

            //! Get the color model.
            DJV_APP_API const std::shared_ptr<models::ColorModel>& getColorModel() const;

            //! Get the viewport model.
            DJV_APP_API const std::shared_ptr<models::ViewportModel>& getViewportModel() const;

            //! Get the audio model.
            DJV_APP_API const std::shared_ptr<models::AudioModel>& getAudioModel() const;

            //! Get the tools model.
            DJV_APP_API const std::shared_ptr<models::ToolsModel>& getToolsModel() const;

            //! Get the commands model.
            DJV_APP_API const std::shared_ptr<models::CommandsModel>& getCommandsModel() const;

            //! Get the review markers model.
            const std::shared_ptr<models::MarkersModel>& getMarkersModel() const;

            //! Get the review annotations model.
            const std::shared_ptr<models::AnnotationsModel>& getAnnotationsModel() const;


            //! Get the drawing state model.
            const std::shared_ptr<models::DrawModel>& getDrawModel() const;

            //! Observe the frames that carry a note or a drawing, sorted and
            //! deduplicated.
            //!
            //! The timeline draws them as markers and the playback bar jumps
            //! between them, so both read the same list.
            std::shared_ptr<ftk::IObservableList<int> > observeReviewMarkers() const;

            //! Seek to the review marker after (or before) the current frame,
            //! wrapping around at the ends. Does nothing without a marker.
            void seekReviewMarker(bool next);

            //! Get whether the setup dialog should be hidden. The setup
            //! dialog is hidden by the "-hideSetup" command line flag, by
            //! automation (the "-command" and "-listCommands" flags), and
            //! during screenshot capture.
            DJV_APP_API bool getHideSetup() const;

            //! Open a file.
            //!
            //! The frame range, when given, is the range an image sequence is
            //! meant to cover, which need not be the frames on disk: a render
            //! in progress is watched over the range it will have when it
            //! finishes. It applies to the first file opened, since a
            //! directory holds sequences that each have their own.
            //! Open a file.
            //!
            //! Set gatherSeq to false to open the file named rather than the
            //! sequence it belongs to. A path cannot say which is meant -- a
            //! frame parsed out of a file name looks exactly like a range of
            //! one -- so it is the caller that knows: a browser listing the
            //! frames one by one, or a recent file, is naming the file.
            DJV_APP_API void open(
                const ftk::Path& path,
                const ftk::Path& audioPath = ftk::Path(),
                const std::optional<ftk::RangeI64>& frames = std::optional<ftk::RangeI64>(),
                bool gatherSeq = true);

            //! Open a file dialog.
            DJV_APP_API void openDialog();

            //! Open a file and separate audio file dialog.
            DJV_APP_API void openSeparateAudioDialog();

            //! Open a playlist: the file list, not a timeline. A ".otio"
            //! given to open() plays as a timeline in one tab; this expands
            //! it into the file list instead. The file cannot say which is
            //! meant, so the caller does.
            DJV_APP_API void openPlaylist(const ftk::Path&);

            //! Open a playlist dialog.
            DJV_APP_API void openPlaylistDialog();

            //! Save the file list as a playlist.
            DJV_APP_API void savePlaylist(const ftk::Path&);

            //! Save a playlist dialog.
            DJV_APP_API void savePlaylistDialog();

            //! \name Reviews
            //! A review (".djvr") is a saved session: the open files, the active
            //! tab, the comparison setup, and the viewport, color and interface
            //! state, together with its notes and drawings.
            ///@{

            //! Open a review, replacing the current session.
            void openReview(const std::filesystem::path&);

            //! Open a review file dialog.
            void openReviewDialog();

            //! Save the current session to the active review, prompting for a
            //! location if none is set.
            void saveReview();

            //! Save the current session to the given review path.
            void saveReview(const std::filesystem::path&);

            //! Save the current session to a new review, always prompting.
            void saveReviewAs();

            //! Close the current review and reset to the empty startup state,
            //! prompting to save first if there are unsaved changes.
            void closeReview();

            //! Get the path of the active review, or empty if none.
            const std::filesystem::path& getReviewPath() const;

            //! Get the recent reviews model.
            const std::shared_ptr<models::RecentFilesModel>& getRecentReviewsModel() const;

            //! Get the recent playlists model.
            DJV_APP_API const std::shared_ptr<models::RecentFilesModel>& getRecentPlaylistsModel() const;

            //! If the review has unsaved changes, prompt the user to save,
            //! discard, or cancel; otherwise proceed immediately. The proceed
            //! callback runs when it is safe to close.
            void confirmClose(const std::function<void()>& onProceed);

            //! Quit asks about unsaved review changes first: this override is
            //! what the window system's quit (e.g. Command-Q) reaches.
            DJV_APP_API void exit() override;

            ///@}

            //! Reload the active files.
            DJV_APP_API void reload();

            //! Observe the timeline player.
            DJV_APP_API std::shared_ptr<ftk::IObservable<std::shared_ptr<tl::Player> > > observePlayer() const;

            //! Get the tool widget factory.
            DJV_APP_API const std::shared_ptr<ToolWidgetFactory>& getToolWidgetFactory() const;

            //! Create the status indicator.
            DJV_APP_API virtual std::shared_ptr<ui::StatusIndicator> createIndicator();

            //! Get the main window.
            DJV_APP_API const std::shared_ptr<MainWindow>& getMainWindow() const;

            //! Observe whether the secondary window is active.
            DJV_APP_API std::shared_ptr<ftk::IObservable<bool> > observeSecondaryWindow() const;

            //! Set whether the secondary window is active.
            DJV_APP_API void setSecondaryWindow(bool);

            //! Get system information.
            DJV_APP_API std::vector<std::string> getSysInfo() const;

            DJV_APP_API void run() override;

        protected:
            DJV_APP_API virtual void _modelsInit();
            DJV_APP_API virtual void _observersInit();
            DJV_APP_API virtual void _inputFilesInit();
            DJV_APP_API virtual void _uiInit();
            DJV_APP_API virtual void _mainWindowInit();

            void _setAudioDeviceMute(bool);

            DJV_APP_API virtual void _viewUpdate(const ftk::V2I& pos, double zoom, bool frame);

        private:
            void _debugState(const nlohmann::json&);
            void _saveSettings();
            void _closeFailed();
            void _filesUpdate(const std::vector<std::shared_ptr<models::FilesModelItem> >&);
            void _activeUpdate(const std::vector<std::shared_ptr<models::FilesModelItem> >&);
            void _colorModelUpdate();
            // Reopen the active files. When the timeline is about to be a
            // different shape, the position and the in/out range cannot be
            // carried over as they are, since both are in timeline time.
            void _reload(bool restructured);
            void _reloadUpdate(const std::shared_ptr<models::FilesModelItem>&);
            void _layersUpdate(const std::vector<int>&);
            void _audioUpdate();

            void _reviewFileDialog(
                ftk::FileBrowserMode,
                const std::string& title,
                const std::function<void(const std::filesystem::path&)>&);
            void _saveReviewAs(const std::function<void()>& onSaved);
            models::Review _buildReview(const std::filesystem::path& base);
            void _applyReview(
                const models::Review&,
                const std::filesystem::path& base,
                const std::filesystem::path& reviewPath,
                const std::filesystem::path& substituteRoot);
            void _logUnreadSections(
                const models::Review&,
                const std::filesystem::path& reviewPath);
            void _closeReview();
            void _applyReviewView();

            std::filesystem::path _autosavePath();
            void _writeAutosave();
            void _deleteAutosave();
            void _recoverAutosave();
            void _markModified();
            void _updateWindowTitle();
            void _markersUpdate();

            FTK_PRIVATE();
        };
    }
}
