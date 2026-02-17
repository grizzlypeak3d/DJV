// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the DJV project.

#pragma once

#include <tlRender/Timeline/Player.h>

#include <ftk/UI/App.h>

#include <filesystem>

namespace ftk
{
    class DiagModel;
    class Settings;
}

namespace tl
{
#if defined(TLRENDER_BMD)
    namespace bmd
    {
        class OutputDevice;
    }
#endif // TLRENDER_BMD
}

namespace djv
{
    //! DJV application
    namespace app
    {
        struct FilesModelItem;

        class AudioModel;
        class ColorModel;
        class FilesModel;
        class MainWindow;
        class RecentFilesModel;
        class SettingsModel;
        class TimeUnitsModel;
        class ToolsModel;
        class ViewportModel;
#if defined(TLRENDER_BMD)
        class BMDDevicesModel;
#endif // TLRENDER_BMD

        //! Application.
        class App : public ftk::App
        {
            FTK_NON_COPYABLE(App);

        protected:
            void _init(
                const std::shared_ptr<ftk::Context>&,
                std::vector<std::string>&);

            App();

        public:
            ~App();

            //! Create a new application.
            static std::shared_ptr<App> create(
                const std::shared_ptr<ftk::Context>&,
                std::vector<std::string>&);

            //! Open a file.
            void open(
                const ftk::Path& path,
                const ftk::Path& audioPath = ftk::Path());

            //! Open a file dialog.
            void openDialog();

            //! Open a file and separate audio file dialog.
            void openSeparateAudioDialog();

            //! Get the settings.
            const std::shared_ptr<ftk::Settings>& getSettings() const;

            //! Get the settings model.
            const std::shared_ptr<SettingsModel>& getSettingsModel() const;

            //! Get the time units model.
            const std::shared_ptr<TimeUnitsModel>& getTimeUnitsModel() const;

            //! Get the files model.
            const std::shared_ptr<FilesModel>& getFilesModel() const;

            //! Get the recent files model.
            const std::shared_ptr<RecentFilesModel>& getRecentFilesModel() const;

            //! Get the diagnostics model.
            const std::shared_ptr<ftk::DiagModel>& getDiagModel() const;

            //! Reload the active files.
            void reload();

            //! Observe the timeline player.
            std::shared_ptr<ftk::IObservable<std::shared_ptr<tl::Player> > > observePlayer() const;

            //! Get the color model.
            const std::shared_ptr<ColorModel>& getColorModel() const;

            //! Get the viewport model.
            const std::shared_ptr<ViewportModel>& getViewportModel() const;

            //! Get the audio model.
            const std::shared_ptr<AudioModel>& getAudioModel() const;

            //! Get the tools model.
            const std::shared_ptr<ToolsModel>& getToolsModel() const;

            //! Get the main window.
            const std::shared_ptr<MainWindow>& getMainWindow() const;

            //! Observe whether the secondary window is active.
            std::shared_ptr<ftk::IObservable<bool> > observeSecondaryWindow() const;

            //! Set whether the secondary window is active.
            void setSecondaryWindow(bool);

#if defined(TLRENDER_BMD)
            //! Get the BMD devices model.
            const std::shared_ptr<BMDDevicesModel>& getBMDDevicesModel() const;

            //! Get the BMD output device.
            const std::shared_ptr<tl::bmd::OutputDevice>& getBMDOutputDevice() const;
#endif // TLRENDER_BMD

            //! Print the version and exit.
            bool hasPrintVersion() const;

            void run() override;

        private:
            void _modelsInit();
            void _devicesInit();
            void _observersInit();
            void _inputFilesInit();
            void _windowsInit();

            std::filesystem::path _appDocsPath();
            std::filesystem::path _getLogFilePath(
                const std::string& appName,
                const std::filesystem::path& appDocsPath);
            std::filesystem::path _getSettingsPath(
                const std::string& appName,
                const std::filesystem::path& appDocsPath);
            tl::IOOptions _getIOOptions() const;

            void _filesUpdate(const std::vector<std::shared_ptr<FilesModelItem> >&);
            void _activeUpdate(const std::vector<std::shared_ptr<FilesModelItem> >&);
            void _layersUpdate(const std::vector<int>&);
            void _viewUpdate(const ftk::V2I& pos, double zoom, bool frame);
            void _audioUpdate();

            FTK_PRIVATE();
        };
    }
}
