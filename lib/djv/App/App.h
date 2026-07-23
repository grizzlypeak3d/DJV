// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the DJV project.

#pragma once

#include <djv/Models/SettingsModel.h>

#include <tlRender/Timeline/Player.h>

#include <ftk/UI/App.h>

#include <filesystem>

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

    //! DJV Application
    namespace app
    {
        class MainWindow;
        class StatusBar;
        class ToolWidgetFactory;

        //! Application.
        class App : public ftk::App
        {
            FTK_NON_COPYABLE(App);

        protected:
            void _init(
                const std::shared_ptr<ftk::Context>&,
                std::vector<std::string>&,
                const std::shared_ptr<models::AppInfoModel>&);

            App();

        public:
            ~App();

            //! Create a new application.
            static std::shared_ptr<App> create(
                const std::shared_ptr<ftk::Context>&,
                std::vector<std::string>&,
                const std::shared_ptr<models::AppInfoModel>& = nullptr);

            //! Get the application information model.
            const std::shared_ptr<models::AppInfoModel>& getAppInfoModel() const;

            //! Get the settings.
            const std::shared_ptr<ftk::Settings>& getSettings() const;

            //! Get the settings model.
            const std::shared_ptr<models::SettingsModel>& getSettingsModel() const;

            //! Get the system log model.
            const std::shared_ptr<ftk::SysLogModel>& getSysLogModel() const;

            //! Get the time units model.
            const std::shared_ptr<models::TimeUnitsModel>& getTimeUnitsModel() const;

            //! Get the files model.
            const std::shared_ptr<models::FilesModel>& getFilesModel() const;

            //! Get the recent files model.
            const std::shared_ptr<models::RecentFilesModel>& getRecentFilesModel() const;

            //! Get the color model.
            const std::shared_ptr<models::ColorModel>& getColorModel() const;

            //! Get the viewport model.
            const std::shared_ptr<models::ViewportModel>& getViewportModel() const;

            //! Get the audio model.
            const std::shared_ptr<models::AudioModel>& getAudioModel() const;

            //! Get the tools model.
            const std::shared_ptr<models::ToolsModel>& getToolsModel() const;

            //! Get the commands model.
            const std::shared_ptr<models::CommandsModel>& getCommandsModel() const;

            //! Open a file.
            void open(
                const ftk::Path& path,
                const ftk::Path& audioPath = ftk::Path());

            //! Open a file dialog.
            void openDialog();

            //! Open a file and separate audio file dialog.
            void openSeparateAudioDialog();

            //! Reload the active files.
            void reload();

            //! Observe the timeline player.
            std::shared_ptr<ftk::IObservable<std::shared_ptr<tl::Player> > > observePlayer() const;

            //! Get the tool widget factory.
            const std::shared_ptr<ToolWidgetFactory>& getToolWidgetFactory() const;

            //! Create the status bar.
            virtual std::shared_ptr<StatusBar> createStatusBar();

            //! Get the main window.
            const std::shared_ptr<MainWindow>& getMainWindow() const;

            //! Observe whether the secondary window is active.
            std::shared_ptr<ftk::IObservable<bool> > observeSecondaryWindow() const;

            //! Set whether the secondary window is active.
            void setSecondaryWindow(bool);

            //! Get system information.
            std::vector<std::string> getSysInfo() const;

            void run() override;

        protected:
            virtual void _modelsInit();
            virtual void _observersInit();
            virtual void _inputFilesInit();
            virtual void _uiInit();
            virtual void _mainWindowInit();

            void _setAudioDeviceMute(bool);

            virtual void _viewUpdate(const ftk::V2I& pos, double zoom, bool frame);
            
            models::ShortcutsSettings _shortcutsSettings;

        private:
            std::filesystem::path _appDocsPath();
            std::filesystem::path _getLogFilePath();
            std::filesystem::path _getSettingsPath();

            void _filesUpdate(const std::vector<std::shared_ptr<models::FilesModelItem> >&);
            void _activeUpdate(const std::vector<std::shared_ptr<models::FilesModelItem> >&);
            void _layersUpdate(const std::vector<int>&);
            void _audioUpdate();

            FTK_PRIVATE();
        };
    }
}
