// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the DJV project.

#include <djv/App/ExportIndicator.h>

#include <djv/App/App.h>
#include <djv/Models/ExportQueue.h>
#include <djv/Models/SettingsModel.h>
#include <djv/Models/ToolsModel.h>

#include <ftk/UI/DrawUtil.h>
#include <ftk/Core/Format.h>
#include <ftk/Core/String.h>

#include <algorithm>
#include <cmath>

namespace djv
{
    namespace app
    {
        namespace
        {
            //! A ring, or the part of one from the top clockwise to a
            //! fraction of the way round.
            ftk::TriMesh2F arc(
                const ftk::V2F& c,
                float outer,
                float inner,
                float to)
            {
                ftk::TriMesh2F out;
                const int segments = std::max(1, static_cast<int>(std::ceil(to * 48)));
                for (int i = 0; i <= segments; ++i)
                {
                    // From the top, clockwise: y is down.
                    const float a = to * i / segments * 2.F * M_PI - M_PI / 2.F;
                    out.v.emplace_back(c.x + std::cos(a) * outer, c.y + std::sin(a) * outer);
                    out.v.emplace_back(c.x + std::cos(a) * inner, c.y + std::sin(a) * inner);
                }
                for (int i = 0; i < segments; ++i)
                {
                    // The indices count from one, and the corners go round
                    // the way ftk::circle() has them.
                    const size_t o0 = i * 2 + 1;
                    const size_t i0 = o0 + 1;
                    const size_t o1 = o0 + 2;
                    const size_t i1 = o0 + 3;
                    out.triangles.emplace_back(o0, i0, o1);
                    out.triangles.emplace_back(i0, i1, o1);
                }
                return out;
            }
        }

        struct ExportIndicator::Private
        {
            std::weak_ptr<App> app;
            std::function<void(const std::string&)> finishedCallback;

            //! The exports of the run going on, or of the last one: the
            //! queue keeps the ones from before until they are cleared.
            std::vector<std::shared_ptr<models::ExportJob> > run;
            bool busy = false;
            //! A run has stopped that the export tool has not shown.
            bool done = false;
            float value = 0.F;
            bool failed = false;

            int margin = 0;
            int keyFocus = 0;
            int cornerRadius = 0;
            int size = 0;

            std::shared_ptr<ftk::ListObserver<std::shared_ptr<models::ExportJob> > > jobsObserver;
            std::vector<std::shared_ptr<ftk::Observer<models::ExportProgress> > > progressObservers;
            std::shared_ptr<ftk::Observer<bool> > busyObserver;
            std::shared_ptr<ftk::ListObserver<std::string> > openToolsObserver;
        };

        void ExportIndicator::_init(
            const std::shared_ptr<ftk::Context>& context,
            const std::shared_ptr<App>& app,
            const std::shared_ptr<IWidget>& parent)
        {
            IButton::_init(context, "djv::app::ExportIndicator", parent);
            FTK_P();
            _buttonRole = ftk::ColorRole::None;
            setVisible(false);

            p.app = app;

            std::weak_ptr<App> appWeak(app);
            setClickedCallback(
                [appWeak]
                {
                    if (auto app = appWeak.lock())
                    {
                        auto toolsModel = app->getToolsModel();
                        const bool open = !toolsModel->isToolOpen("Export");
                        toolsModel->setToolOpen("Export", open);
                        if (open)
                        {
                            // See the note in ToolsActions: a tool opened
                            // while the panel is hidden has to bring it back.
                            auto window = app->getSettingsModel()->getWindow();
                            if (!window.tools)
                            {
                                window.tools = true;
                                app->getSettingsModel()->setWindow(window);
                            }
                        }
                    }
                });

            auto queue = app->getExportQueue();
            p.jobsObserver = ftk::ListObserver<std::shared_ptr<models::ExportJob> >::create(
                queue->observeJobs(),
                [this](const std::vector<std::shared_ptr<models::ExportJob> >& value)
                {
                    FTK_P();
                    // A job joins the run when it is added; one taken off
                    // the list leaves it.
                    std::vector<std::shared_ptr<models::ExportJob> > run;
                    for (const auto& job : p.run)
                    {
                        if (std::find(value.begin(), value.end(), job) != value.end())
                        {
                            run.push_back(job);
                        }
                    }
                    for (const auto& job : value)
                    {
                        if (!models::isDone(job->getProgress().status) &&
                            std::find(run.begin(), run.end(), job) == run.end())
                        {
                            run.push_back(job);
                        }
                    }
                    p.run = run;
                    p.progressObservers.clear();
                    for (const auto& job : value)
                    {
                        p.progressObservers.push_back(
                            ftk::Observer<models::ExportProgress>::create(
                                job->observeProgress(),
                                [this](const models::ExportProgress&)
                                {
                                    _update();
                                },
                                ftk::ObserverAction::Suppress));
                    }
                    _update();
                });

            p.busyObserver = ftk::Observer<bool>::create(
                queue->observeBusy(),
                [this](bool value)
                {
                    FTK_P();
                    if (value && !p.busy)
                    {
                        // A new run: the one before is forgotten.
                        p.done = false;
                        std::vector<std::shared_ptr<models::ExportJob> > run;
                        for (const auto& job : p.run)
                        {
                            if (!models::isDone(job->getProgress().status))
                            {
                                run.push_back(job);
                            }
                        }
                        p.run = run;
                    }
                    else if (!value && p.busy && !p.run.empty())
                    {
                        // Kept until the export tool is opened, unless it
                        // is open already, and said once.
                        auto app = p.app.lock();
                        p.done = !(app && app->getToolsModel()->isToolOpen("Export"));
                        if (p.finishedCallback)
                        {
                            p.finishedCallback(_getSummary());
                        }
                    }
                    p.busy = value;
                    _update();
                });

            p.openToolsObserver = ftk::ListObserver<std::string>::create(
                app->getToolsModel()->observeOpenTools(),
                [this](const std::vector<std::string>& value)
                {
                    FTK_P();
                    if (p.done &&
                        std::find(value.begin(), value.end(), "Export") != value.end())
                    {
                        p.done = false;
                        _update();
                    }
                });
        }

        ExportIndicator::ExportIndicator() :
            _p(new Private)
        {}

        ExportIndicator::~ExportIndicator()
        {}

        std::shared_ptr<ExportIndicator> ExportIndicator::create(
            const std::shared_ptr<ftk::Context>& context,
            const std::shared_ptr<App>& app,
            const std::shared_ptr<IWidget>& parent)
        {
            auto out = std::shared_ptr<ExportIndicator>(new ExportIndicator);
            out->_init(context, app, parent);
            return out;
        }

        void ExportIndicator::setFinishedCallback(const std::function<void(const std::string&)>& value)
        {
            _p->finishedCallback = value;
        }

        ftk::Size2I ExportIndicator::getSizeHint() const
        {
            FTK_P();
            const int s = p.size + (p.margin + p.keyFocus) * 2;
            return ftk::Size2I(s, s);
        }

        void ExportIndicator::sizeHintEvent(const ftk::SizeHintEvent& event)
        {
            IButton::sizeHintEvent(event);
            FTK_P();
            p.margin = event.style->getSizeRole(ftk::SizeRole::MarginInside, event.displayScale);
            p.keyFocus = event.style->getSizeRole(ftk::SizeRole::KeyFocus, event.displayScale);
            p.cornerRadius = event.style->getSizeRole(ftk::SizeRole::CornerRadius, event.displayScale);
            // As tall as the status indicator's letters.
            const ftk::FontMetrics fontMetrics = event.fontSystem->getMetrics(
                event.style->getFont(ftk::FontType::Bold, event.displayScale));
            p.size = fontMetrics.ascender;
        }

        void ExportIndicator::drawEvent(const ftk::Box2I& drawRect, const ftk::DrawEvent& event)
        {
            IButton::drawEvent(drawRect, event);
            FTK_P();
            const ftk::Box2I& g = getGeometry();
            if (_isMousePressed())
            {
                event.render->drawMesh(
                    ftk::rect(g, p.cornerRadius),
                    event.style->getColorRole(ftk::ColorRole::Pressed));
            }
            else if (_isMouseInside())
            {
                event.render->drawMesh(
                    ftk::rect(g, p.cornerRadius),
                    event.style->getColorRole(ftk::ColorRole::Hover));
            }

            // The status indicator's colors: dim for what is not done, lit
            // for what is.
            const ftk::V2F c(g.min.x + g.w() / 2.F, g.min.y + g.h() / 2.F);
            const float outer = p.size / 2.F;
            const float inner = outer - std::max(2.F, p.size / 6.F);
            event.render->drawMesh(
                arc(c, outer, inner, 1.F),
                event.style->getColorRole(ftk::ColorRole::TextDisabled));
            if (p.value > 0.F)
            {
                event.render->drawMesh(
                    arc(c, outer, inner, std::min(p.value, 1.F)),
                    p.failed ?
                    event.style->getColorRole(ftk::ColorRole::Red) :
                    ftk::checkedHighlight(event.style->getColorRole(ftk::ColorRole::Checked)));
            }
        }

        void ExportIndicator::_update()
        {
            FTK_P();
            const bool visible = p.busy || p.done;
            setVisible(visible);
            if (!visible)
                return;

            // The whole run, each export counting the same: those that have
            // stopped in full, the running one by its frames.
            float value = 0.F;
            bool failed = false;
            std::shared_ptr<models::ExportJob> running;
            size_t runningIndex = 0;
            for (size_t i = 0; i < p.run.size(); ++i)
            {
                const models::ExportProgress& progress = p.run[i]->getProgress();
                if (models::isDone(progress.status))
                {
                    value += 1.F;
                    failed |= models::ExportStatus::Failed == progress.status;
                }
                else if (models::ExportStatus::Running == progress.status)
                {
                    if (progress.frames > 0)
                    {
                        value += progress.framesDone / static_cast<float>(progress.frames);
                    }
                    running = p.run[i];
                    runningIndex = i;
                }
            }
            const size_t count = p.run.size();
            value = count > 0 ? value / count : 0.F;
            if (value != p.value || failed != p.failed)
            {
                p.value = value;
                p.failed = failed;
                setDrawUpdate();
            }

            std::string tooltip;
            if (running)
            {
                tooltip = count > 1 ?
                    ftk::Format("Exporting {0} of {1}: {2}").
                        arg(runningIndex + 1).
                        arg(count).
                        arg(running->getOptions().name).str() :
                    ftk::Format("Exporting {0}").arg(running->getOptions().name).str();
                tooltip += "\n" + models::getLabel(running->getProgress());
            }
            else if (p.busy)
            {
                tooltip = "Exporting";
            }
            else
            {
                tooltip = _getSummary();
            }
            setTooltip(tooltip + "\n\nClick to show or hide the export tool.");
        }

        std::string ExportIndicator::_getSummary() const
        {
            FTK_P();
            size_t finished = 0;
            size_t failed = 0;
            size_t cancelled = 0;
            for (const auto& job : p.run)
            {
                switch (job->getProgress().status)
                {
                case models::ExportStatus::Finished: ++finished; break;
                case models::ExportStatus::Failed: ++failed; break;
                case models::ExportStatus::Cancelled: ++cancelled; break;
                default: break;
                }
            }
            std::string out;
            if (finished == p.run.size())
            {
                out = 1 == finished ?
                    ftk::Format("Export finished: {0}").
                        arg(p.run.front()->getOptions().name).str() :
                    ftk::Format("Exports finished: {0}").arg(finished).str();
            }
            else
            {
                std::vector<std::string> s;
                if (finished > 0)
                {
                    s.push_back(ftk::Format("{0} finished").arg(finished));
                }
                if (failed > 0)
                {
                    s.push_back(ftk::Format("{0} failed").arg(failed));
                }
                if (cancelled > 0)
                {
                    s.push_back(ftk::Format("{0} cancelled").arg(cancelled));
                }
                out = "Exports: " + ftk::join(s, ", ");
            }
            return out;
        }
    }
}
