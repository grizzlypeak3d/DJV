// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the DJV project.

#pragma once

#include <ftk/UI/IWidget.h>

#include <nlohmann/json.hpp>

#include <filesystem>
#include <memory>
#include <string>

namespace ftk
{
    class Context;
}

namespace djv
{
    namespace app
    {
        class App;

        const std::string screenshotKey = "Screenshot";

        //! Tag a widget so the capture system can find it and emit its
        //! bounding box. The tag is a stable, semantic id, e.g.
        //! "MainWindow.Viewport".
        inline void setDocTag(
            const std::shared_ptr<ftk::IWidget>& widget,
            const std::string& id)
        {
            if (widget)
            {
                widget->setProperty(screenshotKey, id);
            }
        }

        //! Automated screenshot capture for the documentation.
        //!
        //! One shot per process. Capture is driven from a timer running inside
        //! the normal event loop (ftk::App::run()), which is what realizes and
        //! sizes the window and produces a valid offscreen buffer to read back.
        class Capture : public std::enable_shared_from_this<Capture>
        {
        protected:
            void _init(
                const std::shared_ptr<ftk::Context>&,
                const std::shared_ptr<App>&,
                const std::filesystem::path& manifest,
                const std::string& shotId,
                const std::filesystem::path& outputDir);

            Capture();

        public:
            ~Capture();

            static std::shared_ptr<Capture> create(
                const std::shared_ptr<ftk::Context>&,
                const std::shared_ptr<App>&,
                const std::filesystem::path& manifest,
                const std::string& shotId,
                const std::filesystem::path& outputDir);

            //! Parse the manifest, set up deterministic window state, open the
            //! shot's files, and arm the capture timer. Returns false on a
            //! setup error (bad manifest, unknown shot, no window). After this
            //! returns true, the caller runs the event loop.
            bool begin();

            //! Whether the capture completed and wrote its outputs.
            bool succeeded() const;

        private:
            void _onTick();
            void _applyOpens(const nlohmann::json& setup);
            void _applyRest(const nlohmann::json& setup);
            void _applyStep(const nlohmann::json& step);
            int _fileIndex(const nlohmann::json& value) const;
            bool _ready() const;
            void _finish(bool ok);

            bool _writePNG(const std::filesystem::path&) const;
            void _writeMetadata(const std::filesystem::path&) const;

            FTK_PRIVATE();
        };
    }
}
