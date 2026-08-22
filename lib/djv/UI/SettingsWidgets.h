// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the DJV project.

#pragma once

#include <djv/Models/Export.h>
#include <djv/Models/SettingsModel.h>
#include <djv/Models/ViewportModel.h>
#include <djv/Models/TimeUnitsModel.h>

#include <ftk/UI/IContainer.h>

namespace djv
{
    namespace ui
    {
        //! Base class for settings widgets.
        class DJV_API_TYPE ISettingsWidget : public ftk::IContainer
        {
            FTK_NON_COPYABLE(ISettingsWidget);

        protected:
            ISettingsWidget() = default;

        public:
            DJV_API virtual ~ISettingsWidget();
        };

        //! Audio settings widget.
        class DJV_API_TYPE AudioSettingsWidget : public ISettingsWidget
        {
            FTK_NON_COPYABLE(AudioSettingsWidget);

        protected:
            void _init(
                const std::shared_ptr<ftk::Context>&,
                const std::shared_ptr<models::SettingsModel>&,
                const std::shared_ptr<IWidget>& parent);

            AudioSettingsWidget();

        public:
            DJV_API virtual ~AudioSettingsWidget();

            DJV_API static std::shared_ptr<AudioSettingsWidget> create(
                const std::shared_ptr<ftk::Context>&,
                const std::shared_ptr<models::SettingsModel>&,
                const std::shared_ptr<IWidget>& parent = nullptr);

            FTK_PRIVATE();
        };

        //! Cache settings widget.
        class DJV_API_TYPE CacheSettingsWidget : public ISettingsWidget
        {
            FTK_NON_COPYABLE(CacheSettingsWidget);

        protected:
            void _init(
                const std::shared_ptr<ftk::Context>&,
                const std::shared_ptr<models::SettingsModel>&,
                const std::shared_ptr<IWidget>& parent);

            CacheSettingsWidget();

        public:
            DJV_API virtual ~CacheSettingsWidget();

            DJV_API static std::shared_ptr<CacheSettingsWidget> create(
                const std::shared_ptr<ftk::Context>&,
                const std::shared_ptr<models::SettingsModel>&,
                const std::shared_ptr<IWidget>& parent = nullptr);

        private:
            FTK_PRIVATE();
        };

        //! File browser settings widget.
        class DJV_API_TYPE FileBrowserSettingsWidget : public ISettingsWidget
        {
            FTK_NON_COPYABLE(FileBrowserSettingsWidget);

        protected:
            void _init(
                const std::shared_ptr<ftk::Context>&,
                const std::shared_ptr<models::SettingsModel>&,
                const std::shared_ptr<IWidget>& parent);

            FileBrowserSettingsWidget();

        public:
            DJV_API virtual ~FileBrowserSettingsWidget();

            DJV_API static std::shared_ptr<FileBrowserSettingsWidget> create(
                const std::shared_ptr<ftk::Context>&,
                const std::shared_ptr<models::SettingsModel>&,
                const std::shared_ptr<IWidget>& parent = nullptr);

            FTK_PRIVATE();
        };

        //! Image sequence settings widget.
        //! OTIO settings widget.
        class DJV_API_TYPE OTIOSettingsWidget : public ISettingsWidget
        {
            FTK_NON_COPYABLE(OTIOSettingsWidget);

        protected:
            void _init(
                const std::shared_ptr<ftk::Context>&,
                const std::shared_ptr<models::SettingsModel>&,
                const std::shared_ptr<IWidget>& parent);

            OTIOSettingsWidget();

        public:
            DJV_API virtual ~OTIOSettingsWidget();

            DJV_API static std::shared_ptr<OTIOSettingsWidget> create(
                const std::shared_ptr<ftk::Context>&,
                const std::shared_ptr<models::SettingsModel>&,
                const std::shared_ptr<IWidget>& parent = nullptr);

        private:
            FTK_PRIVATE();
        };

        class DJV_API_TYPE ImageSeqSettingsWidget : public ISettingsWidget
        {
            FTK_NON_COPYABLE(ImageSeqSettingsWidget);

        protected:
            void _init(
                const std::shared_ptr<ftk::Context>&,
                const std::shared_ptr<models::SettingsModel>&,
                const std::shared_ptr<models::ViewportModel>&,
                const std::shared_ptr<IWidget>& parent);

            ImageSeqSettingsWidget();

        public:
            DJV_API virtual ~ImageSeqSettingsWidget();

            DJV_API static std::shared_ptr<ImageSeqSettingsWidget> create(
                const std::shared_ptr<ftk::Context>&,
                const std::shared_ptr<models::SettingsModel>&,
                const std::shared_ptr<models::ViewportModel>&,
                const std::shared_ptr<IWidget>& parent = nullptr);

        private:
            FTK_PRIVATE();
        };

        //! Miscellaneous settings widget.
        class DJV_API_TYPE MiscSettingsWidget : public ISettingsWidget
        {
            FTK_NON_COPYABLE(MiscSettingsWidget);

        protected:
            void _init(
                const std::shared_ptr<ftk::Context>&,
                const std::shared_ptr<models::SettingsModel>&,
                const std::shared_ptr<IWidget>& parent);

            MiscSettingsWidget();

        public:
            DJV_API virtual ~MiscSettingsWidget();

            DJV_API static std::shared_ptr<MiscSettingsWidget> create(
                const std::shared_ptr<ftk::Context>&,
                const std::shared_ptr<models::SettingsModel>&,
                const std::shared_ptr<IWidget>& parent = nullptr);

        private:
            FTK_PRIVATE();
        };

        //! Mouse settings widget.
        class DJV_API_TYPE MouseSettingsWidget : public ISettingsWidget
        {
            FTK_NON_COPYABLE(MouseSettingsWidget);

        protected:
            void _init(
                const std::shared_ptr<ftk::Context>&,
                const std::shared_ptr<models::SettingsModel>&,
                const std::shared_ptr<IWidget>& parent);

            MouseSettingsWidget();

        public:
            DJV_API virtual ~MouseSettingsWidget();

            DJV_API static std::shared_ptr<MouseSettingsWidget> create(
                const std::shared_ptr<ftk::Context>&,
                const std::shared_ptr<models::SettingsModel>&,
                const std::shared_ptr<IWidget>& parent = nullptr);

        private:
            FTK_PRIVATE();
        };

        //! Playback settings widget.
        class DJV_API_TYPE PlaybackSettingsWidget : public ISettingsWidget
        {
            FTK_NON_COPYABLE(PlaybackSettingsWidget);

        protected:
            void _init(
                const std::shared_ptr<ftk::Context>&,
                const std::shared_ptr<models::SettingsModel>&,
                const std::shared_ptr<IWidget>& parent);

            PlaybackSettingsWidget();

        public:
            DJV_API virtual ~PlaybackSettingsWidget();

            DJV_API static std::shared_ptr<PlaybackSettingsWidget> create(
                const std::shared_ptr<ftk::Context>&,
                const std::shared_ptr<models::SettingsModel>&,
                const std::shared_ptr<IWidget>& parent = nullptr);

        private:
            FTK_PRIVATE();
        };

        //! Keyboard shortcut editor.
        class DJV_API_TYPE ShortcutEdit : public ftk::IMouseWidget
        {
            FTK_NON_COPYABLE(ShortcutEdit);

        protected:
            void _init(
                const std::shared_ptr<ftk::Context>&,
                const std::shared_ptr<IWidget>& parent);

            ShortcutEdit();

        public:
            DJV_API virtual ~ShortcutEdit();

            DJV_API static std::shared_ptr<ShortcutEdit> create(
                const std::shared_ptr<ftk::Context>&,
                const std::shared_ptr<IWidget>& parent = nullptr);

            DJV_API void setShortcut(const ftk::KeyShortcut&);
            DJV_API void setCallback(const std::function<void(const ftk::KeyShortcut&)>&);
            DJV_API void setCollision(bool);

            DJV_API ftk::Size2I getSizeHint() const override;
            DJV_API void setGeometry(const ftk::Box2I&) override;
            DJV_API ftk::Box2I getChildrenClipRect() const override;
            DJV_API void styleEvent(const ftk::StyleEvent&) override;
            DJV_API void sizeHintEvent(const ftk::SizeHintEvent&) override;
            DJV_API void drawEvent(const ftk::Box2I& drawRect, const ftk::DrawEvent&) override;
            DJV_API void mouseEnterEvent(ftk::MouseEnterEvent&) override;
            DJV_API void mouseLeaveEvent() override;
            DJV_API void mousePressEvent(ftk::MouseClickEvent&) override;
            DJV_API void keyFocusEvent(bool) override;
            DJV_API void keyPressEvent(ftk::KeyEvent&) override;
            DJV_API void keyReleaseEvent(ftk::KeyEvent&) override;

        private:
            void _widgetUpdate();

            FTK_PRIVATE();
        };

        //! Keyboard shortcut widget.
        class DJV_API_TYPE ShortcutWidget : public ftk::IContainer
        {
            FTK_NON_COPYABLE(ShortcutWidget);

        protected:
            void _init(
                const std::shared_ptr<ftk::Context>&,
                const std::shared_ptr<IWidget>& parent);

            ShortcutWidget();

        public:
            DJV_API virtual ~ShortcutWidget();

            DJV_API static std::shared_ptr<ShortcutWidget> create(
                const std::shared_ptr<ftk::Context>&,
                const std::shared_ptr<IWidget>& parent = nullptr);

            DJV_API void setShortcut(const ftk::KeyShortcut&);
            DJV_API void setCallback(const std::function<void(const ftk::KeyShortcut&)>&);
            DJV_API void setCollision(bool);

        private:
            void _widgetUpdate();

            FTK_PRIVATE();
        };

        //! Keyboard shortcuts settings widget.
        class DJV_API_TYPE ShortcutsSettingsWidget : public ISettingsWidget
        {
            FTK_NON_COPYABLE(ShortcutsSettingsWidget);

        protected:
            void _init(
                const std::shared_ptr<ftk::Context>&,
                const std::shared_ptr<models::SettingsModel>&,
                const std::shared_ptr<IWidget>& parent);

            ShortcutsSettingsWidget();

        public:
            DJV_API virtual ~ShortcutsSettingsWidget();

            DJV_API static std::shared_ptr<ShortcutsSettingsWidget> create(
                const std::shared_ptr<ftk::Context>&,
                const std::shared_ptr<models::SettingsModel>&,
                const std::shared_ptr<IWidget>& parent = nullptr);

        private:
            void _widgetUpdate(const models::ShortcutsSettings&);
            void _searchUpdate(const std::string&);

            FTK_PRIVATE();
        };

        //! Style settings widget.
        class DJV_API_TYPE StyleSettingsWidget : public ISettingsWidget
        {
            FTK_NON_COPYABLE(StyleSettingsWidget);

        protected:
            void _init(
                const std::shared_ptr<ftk::Context>&,
                const std::shared_ptr<models::SettingsModel>&,
                const std::shared_ptr<IWidget>& parent);

            StyleSettingsWidget();

        public:
            DJV_API virtual ~StyleSettingsWidget();

            DJV_API static std::shared_ptr<StyleSettingsWidget> create(
                const std::shared_ptr<ftk::Context>&,
                const std::shared_ptr<models::SettingsModel>&,
                const std::shared_ptr<IWidget>& parent = nullptr);

        private:
            void _widgetUpdate(const models::StyleSettings&);

            FTK_PRIVATE();
        };

        //! Time settings widget.
        class DJV_API_TYPE TimeSettingsWidget : public ISettingsWidget
        {
            FTK_NON_COPYABLE(TimeSettingsWidget);

        protected:
            void _init(
                const std::shared_ptr<ftk::Context>&,
                const std::shared_ptr<models::TimeUnitsModel>&,
                const std::shared_ptr<IWidget>& parent);

            TimeSettingsWidget();

        public:
            DJV_API virtual ~TimeSettingsWidget();

            DJV_API static std::shared_ptr<TimeSettingsWidget> create(
                const std::shared_ptr<ftk::Context>&,
                const std::shared_ptr<models::TimeUnitsModel>&,
                const std::shared_ptr<IWidget>& parent = nullptr);

        private:
            FTK_PRIVATE();
        };

#if defined(TLRENDER_FFMPEG_PLUGIN)
        //! FFmpeg settings widget.
        class DJV_API_TYPE FFmpegSettingsWidget : public ISettingsWidget
        {
            FTK_NON_COPYABLE(FFmpegSettingsWidget);

        protected:
            void _init(
                const std::shared_ptr<ftk::Context>&,
                const std::shared_ptr<models::SettingsModel>&,
                const std::shared_ptr<IWidget>& parent);

            FFmpegSettingsWidget();

        public:
            DJV_API virtual ~FFmpegSettingsWidget();

            DJV_API static std::shared_ptr<FFmpegSettingsWidget> create(
                const std::shared_ptr<ftk::Context>&,
                const std::shared_ptr<models::SettingsModel>&,
                const std::shared_ptr<IWidget>& parent = nullptr);

        private:
            FTK_PRIVATE();
        };
#endif // TLRENDER_FFMPEG_PLUGIN

#if defined(TLRENDER_FFMPEG_CMD)
        //! FFmpeg command settings widget.
        class DJV_API_TYPE FFmpegCmdSettingsWidget : public ISettingsWidget
        {
            FTK_NON_COPYABLE(FFmpegCmdSettingsWidget);

        protected:
            void _init(
                const std::shared_ptr<ftk::Context>&,
                const std::shared_ptr<models::SettingsModel>&,
                const std::shared_ptr<IWidget>& parent);

            FFmpegCmdSettingsWidget();

        public:
            DJV_API virtual ~FFmpegCmdSettingsWidget();

            DJV_API static std::shared_ptr<FFmpegCmdSettingsWidget> create(
                const std::shared_ptr<ftk::Context>&,
                const std::shared_ptr<models::SettingsModel>&,
                const std::shared_ptr<IWidget>& parent = nullptr);

        private:
            FTK_PRIVATE();
        };
#endif // TLRENDER_FFMPEG_CMD

    }
}
