// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the DJV project.

#include <djv/UI/ViewportBinding.h>

#include <djv/Models/ColorModel.h>
#include <djv/Models/FilesModel.h>
#include <djv/Models/ViewportModel.h>

#include <tlRender/UI/Viewport.h>

namespace djv
{
    namespace ui
    {
        struct ViewportBinding::Private
        {
            std::weak_ptr<tl::ui::Viewport> viewport;
            size_t videoFramesSize = 0;
            std::vector<std::string> ocioInputs;
            ftk::ImageOptions imageOptions;
            tl::DisplayOptions displayOptions;
            std::function<void(void)> changedCallback;

            std::shared_ptr<ftk::Observer<tl::CompareOptions> > compareOptionsObserver;
            std::shared_ptr<ftk::Observer<tl::OCIOOptions> > ocioOptionsObserver;
            std::shared_ptr<ftk::Observer<std::vector<std::string> > > resolvedInputsObserver;
            std::shared_ptr<ftk::Observer<tl::LUTOptions> > lutOptionsObserver;
            std::shared_ptr<ftk::Observer<ftk::ImageOptions> > imageOptionsObserver;
            std::shared_ptr<ftk::Observer<tl::DisplayOptions> > displayOptionsObserver;
            std::shared_ptr<ftk::Observer<tl::BackgroundOptions> > bgOptionsObserver;
            std::shared_ptr<ftk::Observer<ftk::ImageType> > colorBufferObserver;
            std::shared_ptr<ftk::Observer<tl::HDR_EOTF> > hdrTransferObserver;
            std::shared_ptr<ftk::Observer<float> > hdrWhiteObserver;
        };

        void ViewportBinding::_init(
            const std::shared_ptr<models::FilesModel>& filesModel,
            const std::shared_ptr<models::ColorModel>& colorModel,
            const std::shared_ptr<models::ViewportModel>& viewportModel,
            const std::shared_ptr<tl::ui::Viewport>& viewport)
        {
            FTK_P();
            p.viewport = viewport;

            p.compareOptionsObserver = ftk::Observer<tl::CompareOptions>::create(
                filesModel->observeCompareOptions(),
                [this](const tl::CompareOptions& value)
                {
                    if (auto viewport = _p->viewport.lock())
                    {
                        viewport->setCompareOptions(value);
                    }
                    _changed();
                });

            // The options as written, not the resolved ones: the per layer
            // display options carry each file's resolved input, so a file
            // that resolves nothing falls back to what the user chose
            // rather than to whatever the active file resolved to.
            p.ocioOptionsObserver = ftk::Observer<tl::OCIOOptions>::create(
                colorModel->observeOCIOOptions(),
                [this](const tl::OCIOOptions& value)
                {
                    if (auto viewport = _p->viewport.lock())
                    {
                        viewport->setOCIOOptions(value);
                    }
                    _changed();
                });

            p.resolvedInputsObserver = ftk::Observer<std::vector<std::string> >::create(
                colorModel->observeResolvedInputs(),
                [this](const std::vector<std::string>& value)
                {
                    _p->ocioInputs = value;
                    _videoUpdate();
                });

            // Layers of a timeline resolve their own input color spaces --
            // each clip of an OTIO file is its own media -- but only when
            // the input is automatic; one the user chose applies to every
            // layer.
            viewport->setOCIOInputResolver(
                [colorModel](const std::string& path, const ftk::ImageTags& tags)
                {
                    return colorModel->getOCIOOptions().input.empty() ?
                        colorModel->resolveInput(path, tags) :
                        std::string();
                });

            p.lutOptionsObserver = ftk::Observer<tl::LUTOptions>::create(
                colorModel->observeLUTOptions(),
                [this](const tl::LUTOptions& value)
                {
                    if (auto viewport = _p->viewport.lock())
                    {
                        viewport->setLUTOptions(value);
                    }
                    _changed();
                });

            p.imageOptionsObserver = ftk::Observer<ftk::ImageOptions>::create(
                viewportModel->observeImageOptions(),
                [this](const ftk::ImageOptions& value)
                {
                    _p->imageOptions = value;
                    _videoUpdate();
                });

            p.displayOptionsObserver = ftk::Observer<tl::DisplayOptions>::create(
                viewportModel->observeDisplayOptions(),
                [this](const tl::DisplayOptions& value)
                {
                    _p->displayOptions = value;
                    _videoUpdate();
                });

            p.bgOptionsObserver = ftk::Observer<tl::BackgroundOptions>::create(
                viewportModel->observeBackgroundOptions(),
                [this](const tl::BackgroundOptions& value)
                {
                    if (auto viewport = _p->viewport.lock())
                    {
                        viewport->setBackgroundOptions(value);
                    }
                    _changed();
                });

            p.colorBufferObserver = ftk::Observer<ftk::ImageType>::create(
                viewportModel->observeColorBuffer(),
                [this](ftk::ImageType value)
                {
                    if (auto viewport = _p->viewport.lock())
                    {
                        viewport->setColorBuffer(value);
                    }
                    _changed();
                });

            p.hdrTransferObserver = ftk::Observer<tl::HDR_EOTF>::create(
                viewportModel->observeHDRTransfer(),
                [this](tl::HDR_EOTF value)
                {
                    if (auto viewport = _p->viewport.lock())
                    {
                        viewport->setHDRTransfer(value);
                    }
                    _changed();
                });

            p.hdrWhiteObserver = ftk::Observer<float>::create(
                viewportModel->observeHDRWhite(),
                [this](float value)
                {
                    if (auto viewport = _p->viewport.lock())
                    {
                        viewport->setHDRWhite(value);
                    }
                    _changed();
                });
        }

        ViewportBinding::ViewportBinding() :
            _p(new Private)
        {}

        ViewportBinding::~ViewportBinding()
        {}

        std::shared_ptr<ViewportBinding> ViewportBinding::create(
            const std::shared_ptr<models::FilesModel>& filesModel,
            const std::shared_ptr<models::ColorModel>& colorModel,
            const std::shared_ptr<models::ViewportModel>& viewportModel,
            const std::shared_ptr<tl::ui::Viewport>& viewport)
        {
            auto out = std::shared_ptr<ViewportBinding>(new ViewportBinding);
            out->_init(filesModel, colorModel, viewportModel, viewport);
            return out;
        }

        void ViewportBinding::setVideoFramesSize(size_t value)
        {
            FTK_P();
            if (value == p.videoFramesSize)
                return;
            p.videoFramesSize = value;
            _videoUpdate();
        }

        const ftk::ImageOptions& ViewportBinding::getImageOptions() const
        {
            return _p->imageOptions;
        }

        const tl::DisplayOptions& ViewportBinding::getDisplayOptions() const
        {
            return _p->displayOptions;
        }

        void ViewportBinding::setChangedCallback(const std::function<void(void)>& value)
        {
            _p->changedCallback = value;
            _changed();
        }

        void ViewportBinding::_videoUpdate()
        {
            FTK_P();
            std::vector<ftk::ImageOptions> imageOptions;
            std::vector<tl::DisplayOptions> displayOptions;
            for (size_t i = 0; i < p.videoFramesSize; ++i)
            {
                imageOptions.push_back(p.imageOptions);
                displayOptions.push_back(p.displayOptions);
                displayOptions.back().ocioInput =
                    i < p.ocioInputs.size() ? p.ocioInputs[i] : std::string();
            }
            if (auto viewport = p.viewport.lock())
            {
                viewport->setImageOptions(imageOptions);
                viewport->setDisplayOptions(displayOptions);
            }
            _changed();
        }

        void ViewportBinding::_changed()
        {
            FTK_P();
            if (p.changedCallback)
            {
                p.changedCallback();
            }
        }
    }
}
