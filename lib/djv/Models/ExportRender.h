// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the DJV project.

#pragma once

#include <djv/Models/Review.h>

#include <tlRender/Timeline/ColorOptions.h>
#include <tlRender/Timeline/CompareOptions.h>
#include <tlRender/Timeline/DisplayOptions.h>
#include <tlRender/Timeline/TimeUnits.h>

#include <ftk/Core/Image.h>
#include <ftk/Core/Path.h>

#include <functional>

namespace ftk
{
    class Context;
}

namespace tl
{
    class Player;
}

namespace djv
{
    namespace models
    {
        class AnnotationsModel;
        class ColorModel;
        class FilesModel;
        class ViewportModel;

        //! \name Export Rendering
        //! Rendering a player's frames the way the viewport shows them --
        //! the files compared laid out alongside, the color as it is
        //! managed and adjusted, and the drawings if they are asked for --
        //! for the desktop's Export tool and Export Still, and the iPad's
        //! frames shared as images.
        ///@{

        //! The video of each source a player shows: the A file, then each
        //! file it is compared with. A source without video still takes a
        //! place, as an empty information, so that the sources stay lined
        //! up with the frames asked for them. A held frame the comparison
        //! is against takes the place of the compared files.
        DJV_MODELS_API std::vector<ftk::ImageInfo> getExportInfos(
            const std::shared_ptr<tl::Player>&,
            const std::optional<tl::VideoFrame>& heldFrame = std::nullopt);

        //! The size the sources render at together, as the viewport lays
        //! them out. The B comparison sizes itself from the B file, which
        //! there need not be one of yet: then the A file's size, so that
        //! there is a size to work with.
        DJV_MODELS_API ftk::Size2I getExportDefaultSize(
            const tl::CompareOptions&,
            const tl::AspectRatioOptions&,
            const std::vector<ftk::ImageInfo>&);

        //! Boxes laid out at one size, at another. Edges rather than the
        //! origin and the size, which keeps neighbouring boxes touching
        //! instead of leaving a seam between them.
        DJV_MODELS_API std::vector<ftk::Box2I> scaleBoxes(
            const std::vector<ftk::Box2I>&,
            const ftk::Size2I& from,
            const ftk::Size2I& to);

        //! What frames are rendered with: one entry a source in the lists,
        //! the A file first. Taken once, so that the layout cannot shift
        //! part way through a movie.
        struct DJV_MODELS_API_TYPE ExportRenderOptions
        {
            //! What is read back: the size, and the type the writer takes.
            ftk::ImageInfo info;

            //! What the picture is rendered into on the way to the file:
            //! the most this build has, as a file may well be asked to hold
            //! it, rather than the viewport's buffer, which is a choice
            //! about showing.
            ftk::ImageType colorBuffer = ftk::ImageType::None;

            tl::CompareOptions compareOptions;
            std::vector<ftk::Box2I> boxes;
            std::vector<ftk::ImageOptions> imageOptions;
            std::vector<tl::DisplayOptions> displayOptions;
            tl::OCIOOptions ocioOptions;
            tl::LUTOptions lutOptions;

            //! Each layer's input color space, as the viewport resolves it.
            std::function<std::string(const std::string&, const ftk::ImageTags&)> ocioInputResolver;

            //! The drawings written into the picture, with the sources they
            //! are on in the order of the boxes, and the sizes their points
            //! are in.
            bool burnAnnotations = false;
            std::vector<ReviewAnnotation> annotations;
            std::vector<std::string> sourceIds;
            std::vector<ftk::ImageInfo> infos;

            //! The held frame the comparison is against, in place of the
            //! compared files' frames (FilesModel::getCompareHeldFrame()).
            std::optional<tl::VideoFrame> heldFrame;
        };

        //! The options to render a player's frames with, at a size and for
        //! a writer's image type, from the models the viewport draws with.
        //! The source ids say which file each source is, in the order the
        //! player has them, for the drawings: the desktop's Files model and
        //! the iPad name them differently. The annotations model may be
        //! null when the drawings are not burned in.
        DJV_MODELS_API ExportRenderOptions getExportRenderOptions(
            const std::shared_ptr<tl::Player>&,
            const std::shared_ptr<ViewportModel>&,
            const std::shared_ptr<ColorModel>&,
            const std::shared_ptr<FilesModel>&,
            const std::shared_ptr<AnnotationsModel>&,
            const std::vector<std::string>& sourceIds,
            const ftk::Size2I&,
            ftk::ImageType,
            bool burnAnnotations);

        //! Renders frames off the screen, with whichever renderer the
        //! application draws with.
        class DJV_MODELS_API_TYPE ExportRender : public std::enable_shared_from_this<ExportRender>
        {
            FTK_NON_COPYABLE(ExportRender);

        protected:
            void _init(
                const std::shared_ptr<ftk::Context>&,
                const ExportRenderOptions&);

            ExportRender();

        public:
            DJV_MODELS_API ~ExportRender();

            //! Create a new renderer. It throws when the image type cannot
            //! be read back.
            DJV_MODELS_API static std::shared_ptr<ExportRender> create(
                const std::shared_ptr<ftk::Context>&,
                const ExportRenderOptions&);

            //! Render the frame at a time of the player's timeline: the A
            //! file's, and each compared file's at the time the player maps
            //! it to, so that what is rendered is what was on screen.
            DJV_MODELS_API std::shared_ptr<ftk::Image> render(
                const std::shared_ptr<tl::Player>&,
                const OTIO_NS::RationalTime&);

        private:
            FTK_PRIVATE();
        };

        //! The name of a still of a frame: the file's name and the time it is
        //! at, as the time units label it, with what a file name cannot hold
        //! made a dash, and the extension: "Train-Shaded_01-00-00-03.tif".
        DJV_MODELS_API std::string getStillFileName(
            const ftk::Path& source,
            const std::string& time,
            const std::string& ext);

        //! The time part of a still's name, in the units the timeline shows
        //! so the name matches it, in a form that sorts in a folder and is
        //! not taken for a sequence's frame number: frames padded to the
        //! last frame's digits ("0042"), seconds padded the same way with
        //! two decimals and an "s" ("012.50s"), and timecode as the label
        //! given for it.
        DJV_MODELS_API std::string getStillTimeLabel(
            const OTIO_NS::RationalTime& time,
            const OTIO_NS::TimeRange& range,
            tl::TimeUnits,
            const std::string& timecodeLabel);

        //! Write a still of the frame a player is at, as the viewport shows
        //! it, at the size its sources are shown at together, into a file
        //! of a type its extension says. Throws when it cannot.
        DJV_MODELS_API void writeStill(
            const std::shared_ptr<ftk::Context>&,
            const std::shared_ptr<tl::Player>&,
            const std::shared_ptr<ViewportModel>&,
            const std::shared_ptr<ColorModel>&,
            const std::shared_ptr<FilesModel>&,
            const std::shared_ptr<AnnotationsModel>&,
            const std::vector<std::string>& sourceIds,
            bool burnAnnotations,
            const ftk::Path&);

        ///@}
    }
}
