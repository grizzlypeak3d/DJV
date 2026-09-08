// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the DJV project.

#pragma once

#include <djv/App/Export.h>

#include <ftk/UI/Capture.h>

namespace djv
{
    namespace app
    {
        class App;

        //! Automated screenshot capture for the documentation.
        //!
        //! The manifest machinery and the generic input steps (clicks, keys,
        //! text, scrolling, tabs) live in ftk::Capture. This adds the DJV
        //! steps -- opening media, playback, comparison, color, viewport,
        //! and tool state -- and the media readiness checks that gate them.
        class DJV_APP_API_TYPE Capture : public ftk::Capture
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
            DJV_APP_API virtual ~Capture();

            DJV_APP_API static std::shared_ptr<Capture> create(
                const std::shared_ptr<ftk::Context>&,
                const std::shared_ptr<App>&,
                const std::filesystem::path& manifest,
                const std::string& shotId,
                const std::filesystem::path& outputDir);

        protected:
            DJV_APP_API void _setupWindow(const nlohmann::json&) override;
            DJV_APP_API void _applyEarly(const nlohmann::json&) override;
            DJV_APP_API bool _ready() const override;
            DJV_APP_API bool _frameReady() const override;
            DJV_APP_API std::string _mediaError() const override;
            DJV_APP_API std::string _waitingFor() const override;
            DJV_APP_API bool _isEarlyStep(const nlohmann::json&) const override;
            DJV_APP_API bool _isLateStep(const nlohmann::json&) const override;
            DJV_APP_API bool _applyStep(const nlohmann::json&) override;

        private:
            int _fileIndex(const nlohmann::json& value) const;

            FTK_PRIVATE();
        };
    }
}
