// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the DJV project.

#include <djv/App/MainWindow.h>

#include <djv/App/App.h>
#include <djv/App/AudioActions.h>
#include <djv/App/AudioMenu.h>
#include <djv/App/BottomToolBar.h>
#include <djv/App/ColorActions.h>
#include <djv/App/ColorMenu.h>
#include <djv/App/CompareActions.h>
#include <djv/App/CompareMenu.h>
#include <djv/App/CompareToolBar.h>
#include <djv/App/FileActions.h>
#include <djv/App/FileMenu.h>
#include <djv/App/FileToolBar.h>
#include <djv/App/FrameActions.h>
#include <djv/App/FrameMenu.h>
#include <djv/App/HelpActions.h>
#include <djv/App/HelpMenu.h>
#include <djv/App/PlaybackActions.h>
#include <djv/App/PlaybackMenu.h>
#include <djv/App/StatusBar.h>
#include <djv/App/TabBar.h>
#include <djv/App/TimelineActions.h>
#include <djv/App/TimelineMenu.h>
#include <djv/App/ToolsActions.h>
#include <djv/App/ToolsMenu.h>
#include <djv/App/ToolsToolBar.h>
#include <djv/App/ToolsWidget.h>
#include <djv/App/ViewActions.h>
#include <djv/App/ViewMenu.h>
#include <djv/App/ViewToolBar.h>
#include <djv/App/Viewport.h>
#include <djv/App/WindowActions.h>
#include <djv/App/WindowMenu.h>
#include <djv/App/WindowToolBar.h>
#include <djv/UI/AboutDialog.h>
#include <djv/UI/SetupDialog.h>
#include <djv/UI/SysInfoDialog.h>
#include <djv/Models/AppInfoModel.h>
#include <djv/Models/ColorModel.h>
#include <djv/Models/TimeUnitsModel.h>
#include <djv/Models/ToolsModel.h>
#include <djv/Models/ViewportModel.h>

#include <tlRender/UI/TimelineWidget.h>
#include <tlRender/GL/Render.h>

#include <ftk/UI/ButtonGroup.h>
#include <ftk/UI/Divider.h>
#include <ftk/UI/DrawUtil.h>
#include <ftk/UI/IButton.h>
#include <ftk/UI/IconSystem.h>
#include <ftk/UI/Label.h>
#include <ftk/UI/Menu.h>
#include <ftk/UI/MenuBar.h>
#include <ftk/UI/RowLayout.h>
#include <ftk/UI/ScreenshotTag.h>
#include <ftk/UI/Splitter.h>
#include <ftk/UI/ToolButton.h>
#include <ftk/Core/Format.h>

#include <algorithm>
#include <string>
#include <vector>

#if defined(_WIN32)
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif // WIN32_LEAN_AND_MEAN
#ifndef NOMINMAX
#define NOMINMAX
#endif // NOMINMAX
#include <windows.h>
#include <windowsx.h>
#endif // _WIN32

namespace djv_resource
{
    extern std::vector<uint8_t> DJV_Icon;
}

namespace djv
{
    namespace app
    {
        namespace
        {
            //! A development build carries the date and the commit, so that two
            //! of them can be told apart. A release is identified by its
            //! version, and the rest would be noise.
            std::string _title(
                const std::shared_ptr<models::AppInfoModel>& appInfoModel)
            {
                std::string out = ftk::Format("{0} {1}").
                    arg(appInfoModel->getFullName()).
                    arg(appInfoModel->getVersion());
                if (!appInfoModel->getVersionDev().empty())
                {
                    out = ftk::Format("{0} - {1} {2}").
                        arg(out).
                        arg(appInfoModel->getCommitDate()).
                        arg(appInfoModel->getGitCommit());
                }
                return out;
            }

            // A context menu of window chrome visibility toggles. The
            // actions are the Window menu's own, so the check marks stay in
            // sync and the toggles go through the same commands; the Window
            // menu remains the complete inventory, and these are a second
            // door to part of it.
            std::function<std::shared_ptr<ftk::Menu>(void)> chromeMenuCallback(
                const std::shared_ptr<ftk::Context>& context,
                const std::shared_ptr<WindowActions>& windowActions,
                const std::vector<std::string>& names)
            {
                std::weak_ptr<ftk::Context> contextWeak(context);
                std::weak_ptr<WindowActions> windowActionsWeak(windowActions);
                return
                    [contextWeak, windowActionsWeak, names]() ->
                    std::shared_ptr<ftk::Menu>
                    {
                        auto context = contextWeak.lock();
                        auto windowActions = windowActionsWeak.lock();
                        if (!context || !windowActions)
                            return nullptr;
                        auto out = ftk::Menu::create(context);
                        auto actions = windowActions->getActions();
                        for (const auto& name : names)
                        {
                            out->addAction(actions[name]);
                        }
                        return out;
                    };
            }
        }

#if defined(_WIN32)
        namespace
        {
            constexpr int windowsTitleButtonWidth = 44;
            constexpr int windowsTitleButtonCount = 3;
            constexpr int windowsTitleDragRowHeight = 32;
            const wchar_t* windowsTitleBarOldWndProcProperty = L"DJV.WindowsTitleBarOldWndProc";
            const wchar_t* windowsTitleBarDragStartProperty = L"DJV.WindowsTitleBarDragStart";
            const wchar_t* windowsTitleBarControlsStartProperty = L"DJV.WindowsTitleBarControlsStart";

            std::wstring toWide(const std::string& value)
            {
                if (value.empty()) return std::wstring();
                const int size = ::MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS,
                    value.data(), static_cast<int>(value.size()), nullptr, 0);
                if (size <= 0) return std::wstring(value.begin(), value.end());
                std::wstring out(size, L'\0');
                ::MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, value.data(),
                    static_cast<int>(value.size()), out.data(), size);
                return out;
            }

            struct WindowSearch
            {
                DWORD processId = 0;
                std::wstring title;
                HWND hwnd = nullptr;
                LONGLONG area = 0;
            };

            BOOL CALLBACK findWindow(HWND hwnd, LPARAM userData)
            {
                auto search = reinterpret_cast<WindowSearch*>(userData);
                DWORD processId = 0;
                ::GetWindowThreadProcessId(hwnd, &processId);
                if (processId != search->processId || !::IsWindowVisible(hwnd)) return TRUE;
                const int titleSize = ::GetWindowTextLengthW(hwnd);
                if (titleSize > 0)
                {
                    std::vector<wchar_t> title(titleSize + 1, L'\0');
                    if (::GetWindowTextW(hwnd, title.data(), static_cast<int>(title.size())) > 0 &&
                        search->title == title.data())
                    {
                        search->hwnd = hwnd;
                        return FALSE;
                    }
                }
                RECT rect = {};
                if (::GetWindowRect(hwnd, &rect))
                {
                    const LONG width = std::max<LONG>(0, rect.right - rect.left);
                    const LONG height = std::max<LONG>(0, rect.bottom - rect.top);
                    const LONGLONG area = static_cast<LONGLONG>(width) * height;
                    if (width >= 64 && height >= 64 && area > search->area)
                    {
                        search->hwnd = hwnd;
                        search->area = area;
                    }
                }
                return TRUE;
            }

            HWND getWindowsWindowHandle(const std::shared_ptr<ftk::IWindow>& window)
            {
                if (!window) return nullptr;
                WindowSearch search;
                search.processId = ::GetCurrentProcessId();
                search.title = toWide(window->getTitle());
                ::EnumWindows(findWindow, reinterpret_cast<LPARAM>(&search));
                return search.hwnd;
            }

            int getWindowsDpiScaled(HWND hwnd, int value)
            {
                int dpi = 96;
                if (hwnd)
                {
                    if (HDC hdc = ::GetDC(hwnd))
                    {
                        dpi = ::GetDeviceCaps(hdc, LOGPIXELSX);
                        ::ReleaseDC(hwnd, hdc);
                    }
                }
                return ::MulDiv(value, dpi, 96);
            }

            LRESULT getWindowsFrameHitTest(HWND hwnd, const POINT& point)
            {
                RECT client = {};
                if (!hwnd || !::GetClientRect(hwnd, &client) || ::IsZoomed(hwnd)) return HTCLIENT;
                const int borderX = ::GetSystemMetrics(SM_CXSIZEFRAME) + ::GetSystemMetrics(SM_CXPADDEDBORDER);
                const int borderY = ::GetSystemMetrics(SM_CYSIZEFRAME) + ::GetSystemMetrics(SM_CXPADDEDBORDER);
                const bool left = point.x < client.left + borderX;
                const bool right = point.x >= client.right - borderX;
                const bool top = point.y < client.top + borderY;
                const bool bottom = point.y >= client.bottom - borderY;
                if (top && left) return HTTOPLEFT;
                if (top && right) return HTTOPRIGHT;
                if (bottom && left) return HTBOTTOMLEFT;
                if (bottom && right) return HTBOTTOMRIGHT;
                if (left) return HTLEFT;
                if (right) return HTRIGHT;
                if (top) return HTTOP;
                if (bottom) return HTBOTTOM;
                return HTCLIENT;
            }

            LRESULT CALLBACK windowsTitleBarWndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
            {
                const auto oldWndProc = reinterpret_cast<WNDPROC>(
                    ::GetPropW(hwnd, windowsTitleBarOldWndProcProperty));
                if (!oldWndProc) return ::DefWindowProcW(hwnd, message, wParam, lParam);
                if (message == WM_NCCALCSIZE)
                {
                    if (::IsZoomed(hwnd))
                    {
                        RECT* rect = wParam ? &reinterpret_cast<NCCALCSIZE_PARAMS*>(lParam)->rgrc[0] :
                            reinterpret_cast<RECT*>(lParam);
                        if (rect)
                        {
                            const int borderX = std::max(0, ::GetSystemMetrics(SM_CXSIZEFRAME) +
                                ::GetSystemMetrics(SM_CXPADDEDBORDER) - 1);
                            const int borderY = std::max(0, ::GetSystemMetrics(SM_CYSIZEFRAME) +
                                ::GetSystemMetrics(SM_CXPADDEDBORDER) - 1);
                            rect->left += borderX; rect->top += borderY;
                            rect->right -= borderX; rect->bottom -= borderY;
                        }
                    }
                    return 0;
                }
                if (message == WM_NCHITTEST)
                {
                    const LRESULT nativeHit = ::CallWindowProcW(oldWndProc, hwnd, message, wParam, lParam);
                    if (nativeHit == HTCLIENT)
                    {
                        POINT point = { GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam) };
                        ::ScreenToClient(hwnd, &point);
                        const LRESULT frameHit = getWindowsFrameHitTest(hwnd, point);
                        if (frameHit != HTCLIENT) return frameHit;
                        const int dragStart = static_cast<int>(
                            reinterpret_cast<INT_PTR>(
                                ::GetPropW(hwnd, windowsTitleBarDragStartProperty))) - 1;
                        const int controlsStart = static_cast<int>(
                            reinterpret_cast<INT_PTR>(
                                ::GetPropW(hwnd, windowsTitleBarControlsStartProperty))) - 1;
                        if (point.y >= 0 && point.y < getWindowsDpiScaled(hwnd, windowsTitleDragRowHeight) &&
                            dragStart >= 0 &&
                            controlsStart > dragStart &&
                            point.x >= dragStart &&
                            point.x < controlsStart)
                        {
                            return HTCAPTION;
                        }
                    }
                    return nativeHit;
                }
                if (message == WM_NCDESTROY)
                {
                    ::RemovePropW(hwnd, windowsTitleBarOldWndProcProperty);
                    ::RemovePropW(hwnd, windowsTitleBarDragStartProperty);
                    ::RemovePropW(hwnd, windowsTitleBarControlsStartProperty);
                }
                return ::CallWindowProcW(oldWndProc, hwnd, message, wParam, lParam);
            }

            void applyWindowsTitleBar(const std::shared_ptr<ftk::IWindow>& window)
            {
                if (HWND hwnd = getWindowsWindowHandle(window))
                {
                    if (!::GetPropW(hwnd, windowsTitleBarOldWndProcProperty))
                    {
                        const LONG_PTR oldWndProc = ::SetWindowLongPtrW(hwnd, GWLP_WNDPROC,
                            reinterpret_cast<LONG_PTR>(windowsTitleBarWndProc));
                        if (oldWndProc) ::SetPropW(hwnd, windowsTitleBarOldWndProcProperty,
                            reinterpret_cast<HANDLE>(oldWndProc));
                    }
                    LONG_PTR style = ::GetWindowLongPtrW(hwnd, GWL_STYLE);
                    style &= ~static_cast<LONG_PTR>(WS_CAPTION);
                    style |= WS_THICKFRAME | WS_SYSMENU | WS_MINIMIZEBOX | WS_MAXIMIZEBOX;
                    ::SetWindowLongPtrW(hwnd, GWL_STYLE, style);
                    ::SetWindowPos(hwnd, nullptr, 0, 0, 0, 0, SWP_FRAMECHANGED | SWP_NOMOVE |
                        SWP_NOSIZE | SWP_NOZORDER | SWP_NOOWNERZORDER | SWP_NOACTIVATE);
                }
            }

            void updateWindowsTitleBarHitRegion(
                const std::shared_ptr<ftk::IWindow>& window,
                int dragStart,
                int controlsStart)
            {
                if (HWND hwnd = getWindowsWindowHandle(window))
                {
                    ::SetPropW(
                        hwnd,
                        windowsTitleBarDragStartProperty,
                        reinterpret_cast<HANDLE>(
                            static_cast<INT_PTR>(std::max(0, dragStart) + 1)));
                    ::SetPropW(
                        hwnd,
                        windowsTitleBarControlsStartProperty,
                        reinterpret_cast<HANDLE>(
                            static_cast<INT_PTR>(std::max(0, controlsStart) + 1)));
                }
            }

            enum class WindowsTitleButtonType { Minimize, Maximize, Close };

            class WindowsTitleButton : public ftk::IButton
            {
            protected:
                void _init(const std::shared_ptr<ftk::Context>& context, WindowsTitleButtonType type,
                    const std::shared_ptr<ftk::IWidget>& parent)
                {
                    IButton::_init(context, "djv::app::WindowsTitleButton", parent);
                    setAcceptsKeyFocus(false);
                    _type = type;
                    _sizeHint = ftk::Size2I(windowsTitleButtonWidth, windowsTitleDragRowHeight);
                    switch (_type)
                    {
                    case WindowsTitleButtonType::Minimize:
                        setTooltip("Minimize window.");
                        break;
                    case WindowsTitleButtonType::Maximize:
                        setTooltip("Maximize or restore window.");
                        break;
                    case WindowsTitleButtonType::Close:
                        setTooltip("Close window.");
                        break;
                    default:
                        break;
                    }
                }
                WindowsTitleButton() = default;
            public:
                static std::shared_ptr<WindowsTitleButton> create(const std::shared_ptr<ftk::Context>& context,
                    WindowsTitleButtonType type, const std::shared_ptr<ftk::IWidget>& parent)
                {
                    auto out = std::shared_ptr<WindowsTitleButton>(new WindowsTitleButton);
                    out->_init(context, type, parent);
                    return out;
                }
                ftk::Size2I getSizeHint() const override { return _sizeHint; }
                void drawEvent(const ftk::Box2I& drawRect, const ftk::DrawEvent& event) override
                {
                    IButton::drawEvent(drawRect, event);
                    const auto& geometry = getGeometry();
                    if (!geometry.isValid()) return;
                    const bool active = _isMouseInside() || _isMousePressed();
                    if (active)
                    {
                        event.render->drawRect(geometry, _type == WindowsTitleButtonType::Close ?
                            ftk::Color4F(.76F, .16F, .12F, 1.F) :
                            event.style->getColorRole(ftk::ColorRole::Hover));
                    }
                    const auto color = _type == WindowsTitleButtonType::Close && active ?
                        ftk::Color4F(1.F, 1.F, 1.F, 1.F) : event.style->getColorRole(ftk::ColorRole::Text);
                    ftk::LineOptions lineOptions;
                    lineOptions.width = std::max(1.F, 1.35F * event.displayScale);
                    const float cx = static_cast<float>(geometry.x()) + geometry.w() / 2.F;
                    const float cy = static_cast<float>(geometry.y()) + geometry.h() / 2.F;
                    const float half = 5.F * event.displayScale;
                    if (_type == WindowsTitleButtonType::Minimize)
                        event.render->drawLine(ftk::V2F(cx - half, cy + half * .55F), ftk::V2F(cx + half, cy + half * .55F), color, lineOptions);
                    else if (_type == WindowsTitleButtonType::Close)
                    {
                        event.render->drawLine(ftk::V2F(cx - half, cy - half), ftk::V2F(cx + half, cy + half), color, lineOptions);
                        event.render->drawLine(ftk::V2F(cx + half, cy - half), ftk::V2F(cx - half, cy + half), color, lineOptions);
                    }
                    else
                        event.render->drawRect(ftk::Box2I(static_cast<int>(cx - half), static_cast<int>(cy - half),
                            static_cast<int>(half * 2.F), static_cast<int>(half * 2.F)), color);
                }
            private:
                WindowsTitleButtonType _type = WindowsTitleButtonType::Minimize;
                ftk::Size2I _sizeHint;
            };
        }
#endif // _WIN32

        struct MainWindow::Private
        {
            std::weak_ptr<App> app;
            std::shared_ptr<models::SettingsModel> settingsModel;
            std::shared_ptr<ftk::Observable<bool> > presentMode;
            bool shown = false;

            std::shared_ptr<Viewport> viewport;
            std::shared_ptr<tl::ui::TimelineWidget> timelineWidget;
            std::shared_ptr<FileActions> fileActions;
            std::shared_ptr<CompareActions> compareActions;
            std::shared_ptr<PlaybackActions> playbackActions;
            std::shared_ptr<FrameActions> frameActions;
            std::shared_ptr<TimelineActions> timelineActions;
            std::shared_ptr<AudioActions> audioActions;
            std::shared_ptr<ViewActions> viewActions;
            std::shared_ptr<WindowActions> windowActions;
            std::shared_ptr<ColorActions> colorActions;
            std::shared_ptr<ToolsActions> toolsActions;
            std::shared_ptr<HelpActions> helpActions;
            std::shared_ptr<FileMenu> fileMenu;
            std::shared_ptr<CompareMenu> compareMenu;
            std::shared_ptr<PlaybackMenu> playbackMenu;
            std::shared_ptr<FrameMenu> frameMenu;
            std::shared_ptr<TimelineMenu> timelineMenu;
            std::shared_ptr<AudioMenu> audioMenu;
            std::shared_ptr<ViewMenu> viewMenu;
            std::shared_ptr<WindowMenu> windowMenu;
            std::shared_ptr<ColorMenu> colorMenu;
            std::shared_ptr<ToolsMenu> toolsMenu;
            std::shared_ptr<HelpMenu> helpMenu;
            std::shared_ptr<ftk::MenuBar> menuBar;
#if defined(_WIN32)
            std::shared_ptr<ftk::HorizontalLayout> titleBar;
            std::shared_ptr<ftk::HorizontalLayout> titleButtons;
            std::shared_ptr<WindowsTitleButton> minimizeButton;
            std::shared_ptr<WindowsTitleButton> maximizeButton;
            std::shared_ptr<WindowsTitleButton> closeButton;
#endif // _WIN32
            std::shared_ptr<FileToolBar> fileToolBar;
            std::shared_ptr<CompareToolBar> compareToolBar;
            std::shared_ptr<ViewToolBar> viewToolBar;
            std::shared_ptr<WindowToolBar> windowToolBar;
            std::shared_ptr<ToolsToolBar> toolsToolBar;
            std::shared_ptr<TabBar> tabBar;
            std::shared_ptr<BottomToolBar> bottomToolBar;
            std::shared_ptr<StatusBar> statusBar;
            std::shared_ptr<ToolsWidget> toolsWidget;
            std::shared_ptr<ui::SetupDialog> setupDialog;
            std::shared_ptr<ui::AboutDialog> aboutDialog;
            std::shared_ptr<ui::SysInfoDialog> sysInfoDialog;
            std::map<std::string, std::shared_ptr<ftk::Divider> > dividers;
            std::shared_ptr<ftk::Splitter> splitter;
            std::shared_ptr<ftk::Splitter> splitter2;
            std::shared_ptr<ftk::VerticalLayout> splitterLayout;
            std::shared_ptr<ftk::VerticalLayout> layout;

            std::shared_ptr<ftk::Observer<std::shared_ptr<tl::Player> > > playerObserver;
            std::shared_ptr<ftk::Observer<tl::CompareOptions> > compareOptionsObserver;
            std::shared_ptr<ftk::Observer<tl::OCIOOptions> > ocioOptionsObserver;
            std::shared_ptr<ftk::Observer<tl::LUTOptions> > lutOptionsObserver;
            std::shared_ptr<ftk::Observer<ftk::gl::TextureType> > colorBufferObserver;
            std::shared_ptr<ftk::ListObserver<std::string> > openToolsObserver;
            std::shared_ptr<ftk::Observer<models::MouseSettings> > mouseSettingsObserver;
            std::shared_ptr<ftk::Observer<models::TimelineSettings> > timelineSettingsObserver;
            std::shared_ptr<ftk::Observer<bool> > timelineFrameViewObserver;
            std::shared_ptr<ftk::Observer<models::WindowSettings> > windowSettingsObserver;
        };

        void MainWindow::_init(
            const std::shared_ptr<ftk::Context>& context,
            const std::shared_ptr<App>& app)
        {
            const models::WindowSettings& settings = app->getSettingsModel()->getWindow();
            Window::_init(
                context,
                app,
                _title(app->getAppInfoModel()),
                settings.size);
            FTK_P();

            auto iconSystem = context->getSystem<ftk::IconSystem>();
            iconSystem->add("DJV_Icon", djv_resource::DJV_Icon);
            setIcon(iconSystem->get("DJV_Icon", 1.0));

            p.app = app;
            p.settingsModel = app->getSettingsModel();
            p.presentMode = ftk::Observable<bool>::create(false);

            p.viewport = Viewport::create(context, app);
            ftk::setScreenshotTag(p.viewport, "MainWindow.Viewport");

            auto timeUnitsModel = app->getTimeUnitsModel();
            p.timelineWidget = tl::ui::TimelineWidget::create(context, timeUnitsModel);
            ftk::setScreenshotTag(p.timelineWidget, "MainWindow.Timeline");

            p.fileActions = FileActions::create(context, app);
            p.compareActions = CompareActions::create(context, app);
            p.playbackActions = PlaybackActions::create(context, app);
            p.frameActions = FrameActions::create(
                context,
                app,
                std::dynamic_pointer_cast<MainWindow>(shared_from_this()));
            p.timelineActions = TimelineActions::create(
                context,
                app,
                std::dynamic_pointer_cast<MainWindow>(shared_from_this()));
            p.audioActions = AudioActions::create(context, app);
            p.viewActions = ViewActions::create(
                context,
                app,
                std::dynamic_pointer_cast<MainWindow>(shared_from_this()));
            p.windowActions = WindowActions::create(
                context,
                app,
                std::dynamic_pointer_cast<MainWindow>(shared_from_this()));
            p.colorActions = ColorActions::create(context, app);
            p.toolsActions = ToolsActions::create(context, app);
            p.helpActions = HelpActions::create(
                context,
                app,
                std::dynamic_pointer_cast<MainWindow>(shared_from_this()));

            p.fileMenu = FileMenu::create(context, app, p.fileActions);
            p.compareMenu = CompareMenu::create(context, app, p.compareActions);
            p.playbackMenu = PlaybackMenu::create(context, p.playbackActions);
            p.frameMenu = FrameMenu::create(context, p.frameActions);
            p.timelineMenu = TimelineMenu::create(context, p.timelineActions);
            p.audioMenu = AudioMenu::create(context, p.audioActions);
            p.viewMenu = ViewMenu::create(context, app, p.viewActions);
            p.windowMenu = WindowMenu::create(
                context,
                std::dynamic_pointer_cast<MainWindow>(shared_from_this()),
                p.windowActions);
            p.colorMenu = ColorMenu::create(context, p.colorActions);
            p.toolsMenu = ToolsMenu::create(context, app, p.toolsActions);
            p.helpMenu = HelpMenu::create(context, p.helpActions);
            p.menuBar = ftk::MenuBar::create(context);
            ftk::setScreenshotTag(p.menuBar, "MainWindow.MenuBar");
            p.menuBar->addMenu("File", p.fileMenu);
            p.menuBar->addMenu("Compare", p.compareMenu);
            p.menuBar->addMenu("Playback", p.playbackMenu);
            p.menuBar->addMenu("Frame", p.frameMenu);
            p.menuBar->addMenu("Timeline", p.timelineMenu);
            p.menuBar->addMenu("Audio", p.audioMenu);
            p.menuBar->addMenu("View", p.viewMenu);
            p.menuBar->addMenu("Window", p.windowMenu);
            p.menuBar->addMenu("Color", p.colorMenu);
            p.menuBar->addMenu("Tools", p.toolsMenu);
            p.menuBar->addMenu("Help", p.helpMenu);

            p.fileToolBar = FileToolBar::create(
                context,
                p.fileActions->getActions());
            ftk::setScreenshotTag(p.fileToolBar, "MainWindow.FileToolBar");

            p.compareToolBar = CompareToolBar::create(
                context,
                p.compareActions->getActions());
            ftk::setScreenshotTag(p.compareToolBar, "MainWindow.CompareToolBar");

            p.viewToolBar = ViewToolBar::create(
                context,
                app,
                std::dynamic_pointer_cast<MainWindow>(shared_from_this()),
                p.viewActions);
            ftk::setScreenshotTag(p.viewToolBar, "MainWindow.ViewToolBar");

            p.windowToolBar = WindowToolBar::create(
                context,
                p.windowActions->getActions());
            ftk::setScreenshotTag(p.windowToolBar, "MainWindow.WindowToolBar");

            p.toolsToolBar = ToolsToolBar::create(
                context,
                app,
                p.toolsActions->getActions());
            ftk::setScreenshotTag(p.toolsToolBar, "MainWindow.ToolsToolBar");

            p.tabBar = TabBar::create(context, app);
            ftk::setScreenshotTag(p.tabBar, "MainWindow.TabBar");

            p.bottomToolBar = BottomToolBar::create(
                context,
                app,
                p.playbackActions,
                p.frameActions,
                p.audioActions);
            ftk::setScreenshotTag(p.bottomToolBar, "MainWindow.BottomToolBar");

            p.statusBar = StatusBar::create(context, app);
            ftk::setScreenshotTag(p.statusBar, "MainWindow.StatusBar");

            p.toolsWidget = ToolsWidget::create(
                context,
                app,
                std::dynamic_pointer_cast<MainWindow>(shared_from_this()));
            ftk::setScreenshotTag(p.toolsWidget, "MainWindow.Tools");

            p.layout = ftk::VerticalLayout::create(context, shared_from_this());
            p.layout->setSpacingRole(ftk::SizeRole::None);
#if defined(_WIN32)
            p.titleBar = ftk::HorizontalLayout::create(context, p.layout);
            p.titleBar->setSpacingRole(ftk::SizeRole::None);
            p.titleBar->setBackgroundRole(ftk::ColorRole::Button);
            p.menuBar->setParent(p.titleBar);
            p.menuBar->setHStretch(ftk::Stretch::Expanding);
            p.titleButtons = ftk::HorizontalLayout::create(context, p.titleBar);
            p.titleButtons->setSpacingRole(ftk::SizeRole::None);
            p.minimizeButton = WindowsTitleButton::create(
                context, WindowsTitleButtonType::Minimize, p.titleButtons);
            p.maximizeButton = WindowsTitleButton::create(
                context, WindowsTitleButtonType::Maximize, p.titleButtons);
            p.closeButton = WindowsTitleButton::create(
                context, WindowsTitleButtonType::Close, p.titleButtons);
            auto mainWindowWeak = std::weak_ptr<MainWindow>(
                std::dynamic_pointer_cast<MainWindow>(shared_from_this()));
            p.minimizeButton->setClickedCallback(
                [mainWindowWeak]
                {
                    if (auto mainWindow = mainWindowWeak.lock())
                    {
                        if (HWND hwnd = getWindowsWindowHandle(mainWindow)) ::ShowWindow(hwnd, SW_MINIMIZE);
                    }
                });
            p.maximizeButton->setClickedCallback(
                [mainWindowWeak]
                {
                    if (auto mainWindow = mainWindowWeak.lock())
                    {
                        if (HWND hwnd = getWindowsWindowHandle(mainWindow))
                        {
                            ::ShowWindow(hwnd, ::IsZoomed(hwnd) ? SW_RESTORE : SW_MAXIMIZE);
                            mainWindow->setDrawUpdate();
                        }
                    }
                });
            p.closeButton->setClickedCallback(
                [mainWindowWeak]
                {
                    if (auto mainWindow = mainWindowWeak.lock())
                    {
                        if (HWND hwnd = getWindowsWindowHandle(mainWindow)) ::PostMessageW(hwnd, WM_CLOSE, 0, 0);
                    }
                });
#else // _WIN32
            p.menuBar->setParent(p.layout);
#endif // _WIN32
            p.dividers["MenuBar"] = ftk::Divider::create(context, ftk::Orientation::Vertical, p.layout);
            auto hLayout = ftk::HorizontalLayout::create(context, p.layout);
            hLayout->setSpacingRole(ftk::SizeRole::Spacing);
            ftk::setScreenshotTag(hLayout, "MainWindow.ToolBar");
            p.fileToolBar->setParent(hLayout);
            p.dividers["File"] = ftk::Divider::create(context, ftk::Orientation::Horizontal, hLayout);
            p.compareToolBar->setParent(hLayout);
            p.dividers["Compare"] = ftk::Divider::create(context, ftk::Orientation::Horizontal, hLayout);
            p.windowToolBar->setParent(hLayout);
            p.dividers["Window"] = ftk::Divider::create(context, ftk::Orientation::Horizontal, hLayout);
            p.viewToolBar->setParent(hLayout);
            p.dividers["View"] = ftk::Divider::create(context, ftk::Orientation::Horizontal, hLayout);
            p.toolsToolBar->setParent(hLayout);
            p.dividers["ToolBars"] = ftk::Divider::create(context, ftk::Orientation::Vertical, p.layout);
            p.splitterLayout = ftk::VerticalLayout::create(context, p.layout);
            p.splitterLayout->setSpacingRole(ftk::SizeRole::None);
            p.splitterLayout->setVStretch(ftk::Stretch::Expanding);
            p.splitter = ftk::Splitter::create(context, ftk::Orientation::Vertical, p.splitterLayout);
            p.splitter->setSplit(settings.splitter);
            p.splitter2 = ftk::Splitter::create(context, ftk::Orientation::Horizontal, p.splitter);
            p.splitter2->setSplit(settings.splitter2);
            auto vLayout = ftk::VerticalLayout::create(context, p.splitter2);
            vLayout->setSpacingRole(ftk::SizeRole::None);
            p.tabBar->setParent(vLayout);
            p.viewport->setParent(vLayout);
            p.toolsWidget->setParent(p.splitter2);
            p.timelineWidget->setParent(p.splitter);
            p.dividers["Bottom"] = ftk::Divider::create(context, ftk::Orientation::Vertical, p.layout);
            p.bottomToolBar->setParent(p.layout);
            p.dividers["Status"] = ftk::Divider::create(context, ftk::Orientation::Vertical, p.layout);
            p.statusBar->setParent(p.layout);

            // Each context menu offers the band it belongs to, and only
            // that band; the Window menu remains the one place that lists
            // every piece of chrome together.
            hLayout->setContextMenuCallback(chromeMenuCallback(
                context,
                p.windowActions,
                {
                    "FileToolBar",
                    "CompareToolBar",
                    "WindowToolBar",
                    "ViewToolBar",
                    "ToolsToolBar"
                }));

            // The timeline, playback controls and status bar form one band
            // across the bottom of the window, so right clicking any of
            // them offers the whole band rather than only itself.
            const std::vector<std::string> bottomChrome =
            {
                "Timeline",
                "BottomToolBar",
                "StatusToolBar"
            };
            p.timelineWidget->setContextMenuCallback(
                chromeMenuCallback(context, p.windowActions, bottomChrome));
            p.bottomToolBar->setContextMenuCallback(
                chromeMenuCallback(context, p.windowActions, bottomChrome));
            p.statusBar->setContextMenuCallback(
                chromeMenuCallback(context, p.windowActions, bottomChrome));

            // Anywhere the click is not claimed, offer every toggle. The
            // window itself is consulted last, so a band keeps its own
            // menu; and because the viewport can never be hidden, this is
            // the one door that cannot be closed by hiding chrome.
            auto chromeMenu = chromeMenuCallback(
                context,
                p.windowActions,
                {
                    "FileToolBar",
                    "CompareToolBar",
                    "WindowToolBar",
                    "ViewToolBar",
                    "ToolsToolBar",
                    "TabBar",
                    "Timeline",
                    "BottomToolBar",
                    "StatusToolBar"
                });
            setContextMenuCallback(
                [this, chromeMenu]() -> std::shared_ptr<ftk::Menu>
                {
                    // Presentation mode hides every piece of chrome
                    // regardless of these settings, so the toggles would
                    // do nothing you could see. Escape leaves the mode.
                    return _p->presentMode->get() ? nullptr : chromeMenu();
                });

            auto miscSettings = app->getSettingsModel()->getMisc();
            if (miscSettings.showSetup && !app->getHideSetup())
            {
                miscSettings.showSetup = false;
                auto settingsModel = app->getSettingsModel();
                settingsModel->setMisc(miscSettings);
                p.setupDialog = ui::SetupDialog::create(
                    context,
                    app->getAppInfoModel(),
                    settingsModel,
                    app->getTimeUnitsModel());
                p.setupDialog->open(std::dynamic_pointer_cast<IWindow>(shared_from_this()));
                p.setupDialog->setCloseCallback(
                    [this]
                    {
                        _p->setupDialog.reset();
                    });
            }

            p.playerObserver = ftk::Observer<std::shared_ptr<tl::Player> >::create(
                app->observePlayer(),
                [this](const std::shared_ptr<tl::Player>& player)
                {
                    FTK_P();
                    p.viewport->setPlayer(player);
                    p.timelineWidget->setPlayer(player);
                });

            p.compareOptionsObserver = ftk::Observer<tl::CompareOptions>::create(
                p.viewport->observeCompareOptions(),
                [this](const tl::CompareOptions& value)
                {
                    auto app = _p->app.lock();
                    app->getFilesModel()->setCompareOptions(value);
                });

            p.ocioOptionsObserver = ftk::Observer<tl::OCIOOptions>::create(
                app->getColorModel()->observeOCIOOptions(),
                [this](const tl::OCIOOptions& value)
                {
                    auto options = _p->timelineWidget->getDisplayOptions();
                    options.ocio = value;
                    _p->timelineWidget->setDisplayOptions(options);
                });

            p.lutOptionsObserver = ftk::Observer<tl::LUTOptions>::create(
                app->getColorModel()->observeLUTOptions(),
                [this](const tl::LUTOptions& value)
                {
                    auto options = _p->timelineWidget->getDisplayOptions();
                    options.lut = value;
                    _p->timelineWidget->setDisplayOptions(options);
                });

            p.colorBufferObserver = ftk::Observer<ftk::gl::TextureType>::create(
                app->getViewportModel()->observeColorBuffer(),
                [this](ftk::gl::TextureType value)
                {
                    setBufferType(ftk::gl::TextureType::RGBA_U8 == value ?
                        ftk::WindowBufferType::U8 :
                        ftk::WindowBufferType::F16);
                });

            p.openToolsObserver = ftk::ListObserver<std::string>::create(
                app->getToolsModel()->observeOpenTools(),
                [this](const std::vector<std::string>&)
                {
                    _windowUpdate();
                });

            p.mouseSettingsObserver = ftk::Observer<models::MouseSettings>::create(
                p.settingsModel->observeMouse(),
                [this](const models::MouseSettings& value)
                {
                    _settingsUpdate(value);
                });

            p.timelineSettingsObserver = ftk::Observer<models::TimelineSettings>::create(
                p.settingsModel->observeTimeline(),
                [this](const models::TimelineSettings& value)
                {
                    _settingsUpdate(value);
                });

            p.timelineFrameViewObserver = ftk::Observer<bool>::create(
                p.timelineWidget->observeFrameView(),
                [this](bool value)
                {
                    auto app = _p->app.lock();
                    auto settings = app->getSettingsModel()->getTimeline();
                    settings.frameView = value;
                    app->getSettingsModel()->setTimeline(settings);
                });

            p.windowSettingsObserver = ftk::Observer<models::WindowSettings>::create(
                p.settingsModel->observeWindow(),
                [this](const models::WindowSettings&)
                {
                    _windowUpdate();
                });
        }

        MainWindow::MainWindow() :
            _p(new Private)
        {}

        MainWindow::~MainWindow()
        {
            FTK_P();

            _makeCurrent();
            p.viewport->setParent(nullptr);
            p.viewport.reset();
            p.timelineWidget->setParent(nullptr);
            p.timelineWidget.reset();

            if (p.shown)
            {
                models::WindowSettings settings = p.settingsModel->getWindow();
                settings.size = getSize();
                settings.splitter = p.splitter->getSplit();
                settings.splitter2 = p.splitter2->getSplit();
                p.settingsModel->setWindow(settings);
            }
        }

        std::shared_ptr<MainWindow> MainWindow::create(
            const std::shared_ptr<ftk::Context>& context,
            const std::shared_ptr<App>& app)
        {
            auto out = std::shared_ptr<MainWindow>(new MainWindow);
            out->_init(context, app);
            return out;
        }

        const std::shared_ptr<ftk::MenuBar> MainWindow::getMenuBar() const
        {
            return _p->menuBar;
        }

        void MainWindow::click(const ftk::V2I& pos, int modifiers)
        {
            _cursorEnter(true);
            _cursorPos(pos);
            _mouseButton(ftk::MouseButton::Left, true, modifiers);
            _mouseButton(ftk::MouseButton::Left, false, modifiers);
        }

        const std::shared_ptr<Viewport>& MainWindow::getViewport() const
        {
            return _p->viewport;
        }

        const std::shared_ptr<tl::ui::TimelineWidget>& MainWindow::getTimelineWidget() const
        {
            return _p->timelineWidget;
        }

        std::shared_ptr<IToolWidget> MainWindow::getToolWidget(
            const std::string& name) const
        {
            return _p->toolsWidget->getToolWidget(name);
        }

        void MainWindow::setSplitters(float splitter, float splitter2)
        {
            FTK_P();
            if (p.splitter)
            {
                p.splitter->setSplit(splitter);
            }
            if (p.splitter2)
            {
                p.splitter2->setSplit(splitter2);
            }
        }

        bool MainWindow::hasPresentMode() const
        {
            return _p->presentMode->get();
        }

        std::shared_ptr<ftk::IObservable<bool> > MainWindow::observePresentMode() const
        {
            return _p->presentMode;
        }

        void MainWindow::setPresentMode(bool value)
        {
            FTK_P();
            if (p.presentMode->setIfChanged(value))
            {
                setFullScreen(value);
                _windowUpdate();
            }
        }

        void MainWindow::focusCurrentFrame()
        {
            _p->bottomToolBar->focusCurrentFrame();
        }

        void MainWindow::showAboutDialog()
        {
            FTK_P();
            p.aboutDialog = ui::AboutDialog::create(
                getContext(),
                p.app.lock()->getAppInfoModel());
            p.aboutDialog->open(std::dynamic_pointer_cast<IWindow>(shared_from_this()));
            p.aboutDialog->setCloseCallback(
                [this]
                {
                    _p->aboutDialog.reset();
                });
        }

        void MainWindow::showSysInfoDialog()
        {
            FTK_P();
            p.sysInfoDialog = ui::SysInfoDialog::create(
                getContext(),
                p.app.lock()->getSysInfo(),
                std::dynamic_pointer_cast<MainWindow>(shared_from_this()));
            p.sysInfoDialog->open(std::dynamic_pointer_cast<IWindow>(shared_from_this()));
            p.sysInfoDialog->setCloseCallback(
                [this]
                {
                    _p->sysInfoDialog.reset();
                });
        }

        void MainWindow::setVisible(bool value)
        {
            Window::setVisible(value);
#if defined(_WIN32)
            if (value)
            {
                applyWindowsTitleBar(std::dynamic_pointer_cast<ftk::IWindow>(shared_from_this()));
            }
#endif // _WIN32
        }

        void MainWindow::setGeometry(const ftk::Box2I& value)
        {
            Window::setGeometry(value);
            FTK_P();
            p.shown = true;
            p.layout->setGeometry(value);
#if defined(_WIN32)
            if (p.titleBar && p.titleButtons)
            {
                const auto& titleBarGeometry = p.titleBar->getGeometry();
                if (titleBarGeometry.isValid())
                {
                    const int controlsWidth = std::max(1, static_cast<int>(
                        windowsTitleButtonWidth * windowsTitleButtonCount * getDisplayScale()));
                    p.titleButtons->setGeometry(ftk::Box2I(
                        titleBarGeometry.x() + std::max(0, titleBarGeometry.w() - controlsWidth),
                        titleBarGeometry.y(),
                        std::min(controlsWidth, titleBarGeometry.w()),
                        titleBarGeometry.h()));
                    const auto& controlsGeometry = p.titleButtons->getGeometry();
                    const int buttonWidth = std::max(1, controlsGeometry.w() / windowsTitleButtonCount);
                    p.minimizeButton->setGeometry(ftk::Box2I(
                        controlsGeometry.x(), controlsGeometry.y(), buttonWidth, controlsGeometry.h()));
                    p.maximizeButton->setGeometry(ftk::Box2I(
                        controlsGeometry.x() + buttonWidth, controlsGeometry.y(), buttonWidth, controlsGeometry.h()));
                    p.closeButton->setGeometry(ftk::Box2I(
                        controlsGeometry.x() + buttonWidth * 2, controlsGeometry.y(),
                        std::max(1, controlsGeometry.w() - buttonWidth * 2), controlsGeometry.h()));
                    p.titleBar->moveToFront(p.titleButtons);
                    updateWindowsTitleBarHitRegion(
                        std::dynamic_pointer_cast<ftk::IWindow>(shared_from_this()),
                        p.menuBar->getGeometry().x() + p.menuBar->getGeometry().w(),
                        controlsGeometry.x());
                }
            }
            if (isVisible(false))
            {
                applyWindowsTitleBar(std::dynamic_pointer_cast<ftk::IWindow>(shared_from_this()));
            }
#endif // _WIN32
        }

        void MainWindow::keyPressEvent(ftk::KeyEvent& event)
        {
            FTK_P();
            if (0 == event.modifiers &&
                ftk::Key::Escape == event.key &&
                p.presentMode->get())
            {
                event.accept = true;
                setPresentMode(false);
            }
            else
            {
                event.accept = p.menuBar->shortcut(event.key, event.modifiers);
            }
        }

        void MainWindow::keyReleaseEvent(ftk::KeyEvent& event)
        {
            event.accept = true;
        }

        void MainWindow::dropEvent(ftk::DragDropEvent& event)
        {
            FTK_P();
            event.accept = true;
            if (auto textData = std::dynamic_pointer_cast<ftk::DragDropTextData>(event.data))
            {
                if (auto app = p.app.lock())
                {
                    for (const auto& i : textData->getText())
                    {
                        app->open(ftk::Path(i));
                    }
                }
            }
        }

        void MainWindow::_settingsUpdate(const models::MouseSettings& settings)
        {
            FTK_P();
            p.timelineWidget->setMouseWheelScale(settings.wheelScale);
            p.viewport->setMouseWheelScale(settings.wheelScale);
        }

        void MainWindow::_settingsUpdate(const models::TimelineSettings& settings)
        {
            FTK_P();

            p.timelineWidget->setFrameView(settings.frameView);
            p.timelineWidget->setScrollBarsVisible(settings.scrollBars);
            p.timelineWidget->setAutoScroll(settings.autoScroll);
            p.timelineWidget->setStopOnScrub(settings.stopOnScrub);

            auto display = p.timelineWidget->getDisplayOptions();

            display.minimize = settings.minimize;
            // Track media gates the two rather than replacing them, so that
            // turning it off and on leaves the choice below it alone.
            display.thumbnails = settings.trackMedia && settings.thumbnails;
            display.thumbnailHeight = getTimelineThumbnailSize(settings.thumbnailSize);
            display.waveforms = settings.trackMedia && settings.waveforms;
            display.waveformHeight = getTimelineWaveformSize(settings.waveformSize);
            p.timelineWidget->setDisplayOptions(display);

            if (settings.minimize)
            {
                if (p.splitter->getParent())
                {
                    p.splitter->setParent(nullptr);
                    p.splitter2->setParent(p.splitterLayout);
                    p.timelineWidget->setParent(p.splitterLayout);
                }
            }
            else
            {
                if (!p.splitter->getParent())
                {
                    p.splitter->setParent(p.splitterLayout);
                    p.splitter2->setParent(p.splitter);
                    p.timelineWidget->setParent(p.splitter);
                }
            }
        }

        void MainWindow::_windowUpdate()
        {
            FTK_P();
            if (auto app = p.app.lock())
            {
                auto settings = p.settingsModel->getWindow();
                const bool presentMode = p.presentMode->get();

                p.menuBar->setVisible(!presentMode);
                p.dividers["MenuBar"]->setVisible(!presentMode);

                p.fileToolBar->setVisible(settings.fileToolBar && !presentMode);
                p.dividers["File"]->setVisible(settings.fileToolBar && !presentMode);

                p.compareToolBar->setVisible(settings.compareToolBar && !presentMode);
                p.dividers["Compare"]->setVisible(settings.compareToolBar && !presentMode);

                p.windowToolBar->setVisible(settings.windowToolBar && !presentMode);
                p.dividers["Window"]->setVisible(settings.windowToolBar && !presentMode);

                p.viewToolBar->setVisible(settings.viewToolBar && !presentMode);
                p.dividers["View"]->setVisible(settings.viewToolBar && !presentMode);

                p.toolsToolBar->setVisible(settings.toolsToolBar && !presentMode);

                p.dividers["ToolBars"]->setVisible(
                    (settings.fileToolBar ||
                    settings.compareToolBar ||
                    settings.windowToolBar ||
                    settings.viewToolBar ||
                    settings.toolsToolBar) && !presentMode);

                p.tabBar->setVisible(settings.tabBar && !presentMode);

                p.toolsWidget->setVisible(
                    settings.tools &&
                    !app->getToolsModel()->getOpenTools().empty() &&
                    !presentMode);

                p.timelineWidget->setVisible(settings.timeline && !presentMode);

                p.bottomToolBar->setVisible(settings.bottomToolBar && !presentMode);
                p.dividers["Bottom"]->setVisible(settings.bottomToolBar && !presentMode);

                p.statusBar->setVisible(settings.statusToolBar && !presentMode);

                // With no status bar to put them in, messages appear over the
                // viewport instead. Not in presentation mode: an error balloon
                // over someone else's review is worse than a missed message,
                // and the messages tool still has them.
                p.viewport->setToastActive(!settings.statusToolBar && !presentMode);
                // Hidden rather than turned off, so that what was being shown
                // is still being shown on the way back out.
                p.viewport->setHUDActive(!presentMode);
                p.dividers["Status"]->setVisible(settings.statusToolBar && !presentMode);
            }
        }
    }
}
