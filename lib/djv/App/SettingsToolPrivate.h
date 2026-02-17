// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the DJV project.

#pragma once

#include <djv/App/SettingsTool.h>

#include <djv/Models/SettingsModel.h>

namespace djv
{
    namespace app
    {
        class App;

        //! Base class for settings widgets.
        class ISettingsWidget : public ftk::IWidget
        {
            FTK_NON_COPYABLE(ISettingsWidget);

        protected:
            ISettingsWidget() = default;

        public:
            virtual ~ISettingsWidget();
        };

        //! Advanced settings widget.
        class AdvancedSettingsWidget : public ISettingsWidget
        {
            FTK_NON_COPYABLE(AdvancedSettingsWidget);

        protected:
            void _init(
                const std::shared_ptr<ftk::Context>&,
                const std::shared_ptr<App>&,
                const std::shared_ptr<IWidget>& parent);

            AdvancedSettingsWidget();

        public:
            virtual ~AdvancedSettingsWidget();

            static std::shared_ptr<AdvancedSettingsWidget> create(
                const std::shared_ptr<ftk::Context>&,
                const std::shared_ptr<App>&,
                const std::shared_ptr<IWidget>& parent = nullptr);

            ftk::Size2I getSizeHint() const override;
            void setGeometry(const ftk::Box2I&) override;

            FTK_PRIVATE();
        };

        //! Cache settings widget.
        class CacheSettingsWidget : public ISettingsWidget
        {
            FTK_NON_COPYABLE(CacheSettingsWidget);

        protected:
            void _init(
                const std::shared_ptr<ftk::Context>&,
                const std::shared_ptr<App>&,
                const std::shared_ptr<IWidget>& parent);

            CacheSettingsWidget();

        public:
            virtual ~CacheSettingsWidget();

            static std::shared_ptr<CacheSettingsWidget> create(
                const std::shared_ptr<ftk::Context>&,
                const std::shared_ptr<App>&,
                const std::shared_ptr<IWidget>& parent = nullptr);

            ftk::Size2I getSizeHint() const override;
            void setGeometry(const ftk::Box2I&) override;

        private:
            FTK_PRIVATE();
        };

#if defined(FTK_NFD)
        //! File browser settings widget.
        class FileBrowserSettingsWidget : public ISettingsWidget
        {
            FTK_NON_COPYABLE(FileBrowserSettingsWidget);

        protected:
            void _init(
                const std::shared_ptr<ftk::Context>&,
                const std::shared_ptr<App>&,
                const std::shared_ptr<IWidget>& parent);

            FileBrowserSettingsWidget();

        public:
            virtual ~FileBrowserSettingsWidget();

            static std::shared_ptr<FileBrowserSettingsWidget> create(
                const std::shared_ptr<ftk::Context>&,
                const std::shared_ptr<App>&,
                const std::shared_ptr<IWidget>& parent = nullptr);

            ftk::Size2I getSizeHint() const override;
            void setGeometry(const ftk::Box2I&) override;

            FTK_PRIVATE();
        };
#endif // FTK_NFD

        //! Image sequence settings widget.
        class ImageSeqSettingsWidget : public ISettingsWidget
        {
            FTK_NON_COPYABLE(ImageSeqSettingsWidget);

        protected:
            void _init(
                const std::shared_ptr<ftk::Context>&,
                const std::shared_ptr<App>&,
                const std::shared_ptr<IWidget>& parent);

            ImageSeqSettingsWidget();

        public:
            virtual ~ImageSeqSettingsWidget();

            static std::shared_ptr<ImageSeqSettingsWidget> create(
                const std::shared_ptr<ftk::Context>&,
                const std::shared_ptr<App>&,
                const std::shared_ptr<IWidget>& parent = nullptr);

            ftk::Size2I getSizeHint() const override;
            void setGeometry(const ftk::Box2I&) override;

        private:
            FTK_PRIVATE();
        };

        //! Miscellaneous settings widget.
        class MiscSettingsWidget : public ISettingsWidget
        {
            FTK_NON_COPYABLE(MiscSettingsWidget);

        protected:
            void _init(
                const std::shared_ptr<ftk::Context>&,
                const std::shared_ptr<App>&,
                const std::shared_ptr<IWidget>& parent);

            MiscSettingsWidget();

        public:
            virtual ~MiscSettingsWidget();

            static std::shared_ptr<MiscSettingsWidget> create(
                const std::shared_ptr<ftk::Context>&,
                const std::shared_ptr<App>&,
                const std::shared_ptr<IWidget>& parent = nullptr);

            ftk::Size2I getSizeHint() const override;
            void setGeometry(const ftk::Box2I&) override;

        private:
            FTK_PRIVATE();
        };

        //! Mouse settings widget.
        class MouseSettingsWidget : public ISettingsWidget
        {
            FTK_NON_COPYABLE(MouseSettingsWidget);

        protected:
            void _init(
                const std::shared_ptr<ftk::Context>&,
                const std::shared_ptr<App>&,
                const std::shared_ptr<IWidget>& parent);

            MouseSettingsWidget();

        public:
            virtual ~MouseSettingsWidget();

            static std::shared_ptr<MouseSettingsWidget> create(
                const std::shared_ptr<ftk::Context>&,
                const std::shared_ptr<App>&,
                const std::shared_ptr<IWidget>& parent = nullptr);

            ftk::Size2I getSizeHint() const override;
            void setGeometry(const ftk::Box2I&) override;

        private:
            FTK_PRIVATE();
        };

        //! Keyboard shortcut editor.
        class ShortcutEdit : public ftk::IMouseWidget
        {
            FTK_NON_COPYABLE(ShortcutEdit);

        protected:
            void _init(
                const std::shared_ptr<ftk::Context>&,
                const std::shared_ptr<IWidget>& parent);

            ShortcutEdit();

        public:
            virtual ~ShortcutEdit();

            static std::shared_ptr<ShortcutEdit> create(
                const std::shared_ptr<ftk::Context>&,
                const std::shared_ptr<IWidget>& parent = nullptr);

            void setShortcut(const ftk::KeyShortcut&);
            void setCallback(const std::function<void(const ftk::KeyShortcut&)>&);
            void setCollision(bool);

            ftk::Size2I getSizeHint() const override;
            void setGeometry(const ftk::Box2I&) override;
            ftk::Box2I getChildrenClipRect() const override;
            void sizeHintEvent(const ftk::SizeHintEvent&) override;
            void drawEvent(const ftk::Box2I& drawRect, const ftk::DrawEvent&) override;
            void mouseEnterEvent(ftk::MouseEnterEvent&) override;
            void mouseLeaveEvent() override;
            void mousePressEvent(ftk::MouseClickEvent&) override;
            void keyFocusEvent(bool) override;
            void keyPressEvent(ftk::KeyEvent&) override;
            void keyReleaseEvent(ftk::KeyEvent&) override;

        private:
            void _widgetUpdate();

            FTK_PRIVATE();
        };

        //! Keyboard shortcut widget.
        class ShortcutWidget : public ftk::IWidget
        {
            FTK_NON_COPYABLE(ShortcutWidget);

        protected:
            void _init(
                const std::shared_ptr<ftk::Context>&,
                const std::shared_ptr<IWidget>& parent);

            ShortcutWidget();

        public:
            virtual ~ShortcutWidget();

            static std::shared_ptr<ShortcutWidget> create(
                const std::shared_ptr<ftk::Context>&,
                const std::shared_ptr<IWidget>& parent = nullptr);

            void setShortcut(const ftk::KeyShortcut&);
            void setCallback(const std::function<void(const ftk::KeyShortcut&)>&);
            void setCollision(bool);

            ftk::Size2I getSizeHint() const override;
            void setGeometry(const ftk::Box2I&) override;

        private:
            void _widgetUpdate();

            FTK_PRIVATE();
        };

        //! Keyboard shortcuts settings widget.
        class ShortcutsSettingsWidget : public ISettingsWidget
        {
            FTK_NON_COPYABLE(ShortcutsSettingsWidget);

        protected:
            void _init(
                const std::shared_ptr<ftk::Context>&,
                const std::shared_ptr<App>&,
                const std::shared_ptr<IWidget>& parent);

            ShortcutsSettingsWidget();

        public:
            virtual ~ShortcutsSettingsWidget();

            static std::shared_ptr<ShortcutsSettingsWidget> create(
                const std::shared_ptr<ftk::Context>&,
                const std::shared_ptr<App>&,
                const std::shared_ptr<IWidget>& parent = nullptr);

            ftk::Size2I getSizeHint() const override;
            void setGeometry(const ftk::Box2I&) override;

        private:
            void _widgetUpdate(const models::ShortcutsSettings&);
            void _searchUpdate(const std::string&);

            FTK_PRIVATE();
        };

        //! Style settings widget.
        class StyleSettingsWidget : public ISettingsWidget
        {
            FTK_NON_COPYABLE(StyleSettingsWidget);

        protected:
            void _init(
                const std::shared_ptr<ftk::Context>&,
                const std::shared_ptr<App>&,
                const std::shared_ptr<IWidget>& parent);

            StyleSettingsWidget();

        public:
            virtual ~StyleSettingsWidget();

            static std::shared_ptr<StyleSettingsWidget> create(
                const std::shared_ptr<ftk::Context>&,
                const std::shared_ptr<App>&,
                const std::shared_ptr<IWidget>& parent = nullptr);

            ftk::Size2I getSizeHint() const override;
            void setGeometry(const ftk::Box2I&) override;

        private:
            void _widgetUpdate(const models::StyleSettings&);

            FTK_PRIVATE();
        };

        //! Time settings widget.
        class TimeSettingsWidget : public ISettingsWidget
        {
            FTK_NON_COPYABLE(TimeSettingsWidget);

        protected:
            void _init(
                const std::shared_ptr<ftk::Context>&,
                const std::shared_ptr<App>&,
                const std::shared_ptr<IWidget>& parent);

            TimeSettingsWidget();

        public:
            virtual ~TimeSettingsWidget();

            static std::shared_ptr<TimeSettingsWidget> create(
                const std::shared_ptr<ftk::Context>&,
                const std::shared_ptr<App>&,
                const std::shared_ptr<IWidget>& parent = nullptr);

            ftk::Size2I getSizeHint() const override;
            void setGeometry(const ftk::Box2I&) override;

        private:
            FTK_PRIVATE();
        };

#if defined(TLRENDER_FFMPEG)
        //! FFmpeg settings widget.
        class FFmpegSettingsWidget : public ISettingsWidget
        {
            FTK_NON_COPYABLE(FFmpegSettingsWidget);

        protected:
            void _init(
                const std::shared_ptr<ftk::Context>&,
                const std::shared_ptr<App>&,
                const std::shared_ptr<IWidget>& parent);

            FFmpegSettingsWidget();

        public:
            virtual ~FFmpegSettingsWidget();

            static std::shared_ptr<FFmpegSettingsWidget> create(
                const std::shared_ptr<ftk::Context>&,
                const std::shared_ptr<App>&,
                const std::shared_ptr<IWidget>& parent = nullptr);

            ftk::Size2I getSizeHint() const override;
            void setGeometry(const ftk::Box2I&) override;

        private:
            FTK_PRIVATE();
        };
#endif // TLRENDER_FFMPEG

#if defined(TLRENDER_USD)
        //! USD settings widget.
        class USDSettingsWidget : public ISettingsWidget
        {
            FTK_NON_COPYABLE(USDSettingsWidget);

        protected:
            void _init(
                const std::shared_ptr<ftk::Context>&,
                const std::shared_ptr<App>&,
                const std::shared_ptr<IWidget>& parent);

            USDSettingsWidget();

        public:
            virtual ~USDSettingsWidget();

            static std::shared_ptr<USDSettingsWidget> create(
                const std::shared_ptr<ftk::Context>&,
                const std::shared_ptr<App>&,
                const std::shared_ptr<IWidget>& parent = nullptr);

            ftk::Size2I getSizeHint() const override;
            void setGeometry(const ftk::Box2I&) override;

        private:
            FTK_PRIVATE();
        };
#endif // TLRENDER_USD
    }
}
