// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the DJV project.

#pragma once

#include <djv/App/Export.h>
#include <djv/Models/Export.h>
#include <djv/Models/SettingsModel.h>

#include <tlRender/Timeline/Player.h>

#include <ftk/UI/App.h>

#include <filesystem>
#include <optional>

namespace ftk
{
    class Settings;
    class SysLogModel;
}

namespace djv
{
    namespace models
    {
        struct FilesModelItem;

        class AppInfoModel;
        class AudioModel;
        class ColorModel;
        class CommandsModel;
        class FilesModel;
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
            DJV_APP_API void _init(
                const std::shared_ptr<ftk::Context>&,
                std::vector<std::string>&,
                const std::shared_ptr<models::AppInfoModel>&);

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

            FTK_PRIVATE();
        };
    }
}
