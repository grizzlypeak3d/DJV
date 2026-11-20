// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the DJV project.

#include <djv/UI/ExportQueueWidget.h>

#include <djv/Models/ExportQueue.h>

#include <ftk/UI/DrawUtil.h>
#include <ftk/UI/Label.h>
#include <ftk/UI/RowLayout.h>
#include <ftk/UI/ScreenshotTag.h>
#include <ftk/UI/Spacer.h>
#include <ftk/UI/ToolButton.h>
#include <ftk/Core/Format.h>

namespace djv
{
    namespace ui
    {
        namespace
        {
            //! A thin bar of how much is done, flat, without a border.
            class ProgressBar : public ftk::IWidget
            {
            protected:
                void _init(
                    const std::shared_ptr<ftk::Context>& context,
                    const std::shared_ptr<ftk::IWidget>& parent)
                {
                    ftk::IWidget::_init(context, "djv::ui::ProgressBar", parent);
                    setHStretch(ftk::Stretch::Expanding);
                }

            public:
                static std::shared_ptr<ProgressBar> create(
                    const std::shared_ptr<ftk::Context>& context,
                    const std::shared_ptr<ftk::IWidget>& parent = nullptr)
                {
                    auto out = std::shared_ptr<ProgressBar>(new ProgressBar);
                    out->_init(context, parent);
                    return out;
                }

                void setValue(float value)
                {
                    if (value == _value)
                        return;
                    _value = value;
                    setDrawUpdate();
                }

                ftk::Size2I getSizeHint() const override
                {
                    return ftk::Size2I(_height, _height);
                }

                void sizeHintEvent(const ftk::SizeHintEvent& event) override
                {
                    ftk::IWidget::sizeHintEvent(event);
                    _height = event.style->getSizeRole(ftk::SizeRole::Border, event.displayScale) * 3;
                }

                void drawEvent(const ftk::Box2I& drawRect, const ftk::DrawEvent& event) override
                {
                    ftk::IWidget::drawEvent(drawRect, event);
                    const ftk::Box2I& g = getGeometry();
                    event.render->drawRect(g, event.style->getColorRole(ftk::ColorRole::Window));
                    const int w = static_cast<int>(std::clamp(_value, 0.F, 1.F) * g.w());
                    if (w > 0)
                    {
                        event.render->drawRect(
                            ftk::Box2I(g.min.x, g.min.y, w, g.h()),
                            event.style->getColorRole(ftk::ColorRole::Checked));
                    }
                }

            private:
                float _value = 0.F;
                int _height = 0;
            };

            //! One export, on a card of its own as a marker is in the
            //! Review tool, though not a button: there is nothing to do with
            //! an export by clicking it.
            class JobWidget : public ftk::IContainer
            {
            protected:
                void _init(
                    const std::shared_ptr<ftk::Context>& context,
                    const std::shared_ptr<models::ExportQueue>& queue,
                    const std::shared_ptr<models::ExportJob>& job,
                    const std::function<void(const ftk::Path&)>& openCallback,
                    const std::function<void(bool)>& hoveredCallback,
                    const std::shared_ptr<ftk::IWidget>& parent)
                {
                    ftk::IContainer::_init(context, "djv::ui::ExportJobWidget", parent);

                    const auto& options = job->getOptions();
                    _nameLabel = ftk::Label::create(context, options.name);
                    _nameLabel->setFont(ftk::FontType::Bold);
                    // Elided rather than clipped: a clipped label still asks
                    // for its whole width, and a long name or error would
                    // widen the tool.
                    _nameLabel->setHStretch(ftk::Stretch::Expanding);
                    _nameLabel->setElide(true);
                    _nameLabel->setTooltip(
                        ftk::Format("{0}\nFrom: {1}").
                            arg(options.path.get()).
                            arg(options.sourceName));
                    ftk::setScreenshotTag(_nameLabel, "Export.Queue.Name");
                    _statusLabel = ftk::Label::create(context);
                    _statusLabel->setHStretch(ftk::Stretch::Expanding);
                    _statusLabel->setElide(true);
                    _statusLabel->setTextRole(ftk::ColorRole::TextDisabled);
                    ftk::setScreenshotTag(_statusLabel, "Export.Queue.Status");
                    _progressBar = ProgressBar::create(context);
                    _openButton = ftk::ToolButton::create(context);
                    _openButton->setIcon("FileOpen");
                    _openButton->setTooltip("Open the exported file.");
                    _openButton->setVisible(false);
                    ftk::setScreenshotTag(_openButton, "Export.Queue.Open");
                    // Stop while the export is queued or running, a cross
                    // once it has stopped: cancelling leaves the row, and
                    // a second click takes it off.
                    _button = ftk::ToolButton::create(context);
                    ftk::setScreenshotTag(_button, "Export.Queue.Cancel");
                    _openButton->setHoveredCallback(hoveredCallback);
                    _button->setHoveredCallback(hoveredCallback);

                    auto layout = ftk::HorizontalLayout::create(context);
                    layout->setMarginRole(ftk::SizeRole::MarginSmall);
                    layout->setSpacingRole(ftk::SizeRole::SpacingSmall);
                    _setWidget(layout);
                    auto vLayout = ftk::VerticalLayout::create(context, layout);
                    vLayout->setSpacingRole(ftk::SizeRole::SpacingTool);
                    vLayout->setHStretch(ftk::Stretch::Expanding);
                    _nameLabel->setParent(vLayout);
                    _statusLabel->setParent(vLayout);
                    _progressBar->setParent(vLayout);
                    _openButton->setParent(layout);
                    _openButton->setVAlign(ftk::VAlign::Top);
                    _button->setParent(layout);
                    _button->setVAlign(ftk::VAlign::Top);

                    const ftk::Path path = options.path;
                    _openButton->setClickedCallback(
                        [openCallback, path]
                        {
                            if (openCallback)
                            {
                                openCallback(path);
                            }
                        });

                    std::weak_ptr<models::ExportQueue> queueWeak(queue);
                    std::weak_ptr<models::ExportJob> jobWeak(job);
                    _button->setClickedCallback(
                        [queueWeak, jobWeak]
                        {
                            auto queue = queueWeak.lock();
                            auto job = jobWeak.lock();
                            if (queue && job)
                            {
                                if (models::isDone(job->getProgress().status))
                                {
                                    queue->remove(job);
                                }
                                else
                                {
                                    queue->cancel(job);
                                }
                            }
                        });

                    _progressObserver = ftk::Observer<models::ExportProgress>::create(
                        job->observeProgress(),
                        [this, openCallback](const models::ExportProgress& value)
                        {
                            const std::string label = models::getLabel(value);
                            _statusLabel->setText(label);
                            // The whole of an error, which is elided.
                            _statusLabel->setTooltip(
                                models::ExportStatus::Failed == value.status ?
                                label :
                                std::string());
                            _statusLabel->setTextRole(
                                models::ExportStatus::Failed == value.status ?
                                ftk::ColorRole::Red :
                                ftk::ColorRole::TextDisabled);
                            _progressBar->setValue(
                                value.frames > 0 ?
                                value.framesDone / static_cast<float>(value.frames) :
                                0.F);
                            _progressBar->setVisible(models::ExportStatus::Running == value.status);
                            _openButton->setVisible(
                                openCallback &&
                                models::ExportStatus::Finished == value.status);
                            const bool done = models::isDone(value.status);
                            _button->setIcon(done ? "CloseSmall" : "StopSmall");
                            _button->setTooltip(done ?
                                "Remove the export from the list." :
                                "Cancel the export. What has been written is left.");
                        });
                }

            public:
                static std::shared_ptr<JobWidget> create(
                    const std::shared_ptr<ftk::Context>& context,
                    const std::shared_ptr<models::ExportQueue>& queue,
                    const std::shared_ptr<models::ExportJob>& job,
                    const std::function<void(const ftk::Path&)>& openCallback,
                    const std::function<void(bool)>& hoveredCallback,
                    const std::shared_ptr<ftk::IWidget>& parent = nullptr)
                {
                    auto out = std::shared_ptr<JobWidget>(new JobWidget);
                    out->_init(context, queue, job, openCallback, hoveredCallback, parent);
                    return out;
                }

                void sizeHintEvent(const ftk::SizeHintEvent& event) override
                {
                    ftk::IContainer::sizeHintEvent(event);
                    _cornerRadius = event.style->getSizeRole(ftk::SizeRole::CornerRadius, event.displayScale);
                }

                void drawEvent(const ftk::Box2I& drawRect, const ftk::DrawEvent& event) override
                {
                    ftk::IContainer::drawEvent(drawRect, event);
                    event.render->drawMesh(
                        ftk::rect(getGeometry(), _cornerRadius),
                        event.style->getColorRole(ftk::ColorRole::Base));
                }

            private:
                int _cornerRadius = 0;
                std::shared_ptr<ftk::Label> _nameLabel;
                std::shared_ptr<ftk::Label> _statusLabel;
                std::shared_ptr<ProgressBar> _progressBar;
                std::shared_ptr<ftk::ToolButton> _openButton;
                std::shared_ptr<ftk::ToolButton> _button;
                std::shared_ptr<ftk::Observer<models::ExportProgress> > _progressObserver;
            };
        }

        struct ExportQueueWidget::Private
        {
            std::weak_ptr<models::ExportQueue> queue;
            std::function<void(const ftk::Path&)> openCallback;
            std::shared_ptr<ftk::ToolButton> clearButton;
            std::shared_ptr<ftk::VerticalLayout> jobsLayout;
            std::shared_ptr<ftk::VerticalLayout> layout;

            //! The exports and their cards, in the order the queue runs
            //! them, with the status each was last seen at.
            struct Item
            {
                std::shared_ptr<models::ExportJob> job;
                std::shared_ptr<JobWidget> widget;
                models::ExportStatus status = models::ExportStatus::Queued;
            };
            std::vector<Item> items;
            std::vector<size_t> order;
            //! Whether the pointer is on the list, or on a button in it,
            //! which takes the hover from the list. The rows keep their
            //! places while it is, so that a second click lands on the row
            //! the first one did, not on the one moved under it, and a row
            //! does not move out from under the pointer.
            bool listHovered = false;
            bool buttonHovered = false;

            std::shared_ptr<ftk::ListObserver<std::shared_ptr<models::ExportJob> > > jobsObserver;
            std::vector<std::shared_ptr<ftk::Observer<models::ExportProgress> > > statusObservers;
        };

        void ExportQueueWidget::_init(
            const std::shared_ptr<ftk::Context>& context,
            const std::shared_ptr<models::ExportQueue>& queue,
            const std::shared_ptr<IWidget>& parent)
        {
            ftk::IContainer::_init(context, "djv::ui::ExportQueueWidget", parent);
            FTK_P();

            p.queue = queue;

            p.clearButton = ftk::ToolButton::create(context, "Clear");
            p.clearButton->setTooltip("Remove the exports that have stopped from the list.");
            ftk::setScreenshotTag(p.clearButton, "Export.Queue.Clear");

            // No title: the cards under the export button say what they
            // are, and a title row pushed them down. Clear goes under them,
            // where the list ends.
            // No margin: the export widget puts it in its own, under the
            // export button, which it goes with.
            p.layout = ftk::VerticalLayout::create(context);
            p.layout->setSpacingRole(ftk::SizeRole::SpacingSmall);
            _setWidget(p.layout);
            p.jobsLayout = ftk::VerticalLayout::create(context, p.layout);
            p.jobsLayout->setSpacingRole(ftk::SizeRole::SpacingSmall);
            ftk::setScreenshotTag(p.jobsLayout, "Export.Queue");
            auto hLayout = ftk::HorizontalLayout::create(context, p.layout);
            hLayout->setSpacingRole(ftk::SizeRole::SpacingSmall);
            auto spacer = ftk::Spacer::create(context, ftk::Orientation::Horizontal, hLayout);
            spacer->setHStretch(ftk::Stretch::Expanding);
            p.clearButton->setParent(hLayout);

            p.clearButton->setClickedCallback(
                [this]
                {
                    if (auto queue = _p->queue.lock())
                    {
                        queue->clearDone();
                    }
                });

            p.jobsObserver = ftk::ListObserver<std::shared_ptr<models::ExportJob> >::create(
                queue->observeJobs(),
                [this](const std::vector<std::shared_ptr<models::ExportJob> >&)
                {
                    _jobsUpdate();
                });
        }

        void ExportQueueWidget::setOpenCallback(const std::function<void(const ftk::Path&)>& value)
        {
            _p->openCallback = value;
            _jobsUpdate();
        }

        void ExportQueueWidget::_jobsUpdate()
        {
            FTK_P();
            p.statusObservers.clear();
            p.items.clear();
            p.order.clear();
            p.buttonHovered = false;
            p.jobsLayout->clear();
            std::vector<std::shared_ptr<models::ExportJob> > jobs;
            if (auto queue = p.queue.lock())
            {
                jobs = queue->observeJobs()->get();
                if (auto context = getContext())
                {
                    for (const auto& job : jobs)
                    {
                        p.items.push_back({
                            job,
                            JobWidget::create(
                                context,
                                queue,
                                job,
                                p.openCallback,
                                [this](bool value)
                                {
                                    _p->buttonHovered = value;
                                    if (!value)
                                    {
                                        _orderUpdate();
                                    }
                                },
                                p.jobsLayout),
                            job->getProgress().status });
                    }
                    for (size_t i = 0; i < p.items.size(); ++i)
                    {
                        p.statusObservers.push_back(ftk::Observer<models::ExportProgress>::create(
                            p.items[i].job->observeProgress(),
                            [this, i](const models::ExportProgress& value)
                            {
                                FTK_P();
                                if (i < p.items.size() && value.status != p.items[i].status)
                                {
                                    p.items[i].status = value.status;
                                    _orderUpdate();
                                }
                            }));
                    }
                }
            }
            _orderUpdate();
            setVisible(!jobs.empty());
        }

        void ExportQueueWidget::_orderUpdate()
        {
            FTK_P();
            std::vector<models::ExportStatus> statuses;
            for (const auto& item : p.items)
            {
                statuses.push_back(item.status);
            }
            const std::vector<size_t> order = models::getListOrder(statuses);
            const bool hold = p.listHovered || p.buttonHovered;
            if (order != p.order && (!hold || p.order.empty()))
            {
                p.order = order;
                for (size_t i = 0; i < order.size(); ++i)
                {
                    p.jobsLayout->moveToIndex(p.items[order[i]].widget, i);
                }
            }
        }

        void ExportQueueWidget::mouseEnterEvent(ftk::MouseEnterEvent& event)
        {
            IContainer::mouseEnterEvent(event);
            event.accept = true;
            _p->listHovered = true;
        }

        void ExportQueueWidget::mouseLeaveEvent()
        {
            IContainer::mouseLeaveEvent();
            _p->listHovered = false;
            _orderUpdate();
        }

        ExportQueueWidget::ExportQueueWidget() :
            _p(new Private)
        {}

        ExportQueueWidget::~ExportQueueWidget()
        {}

        std::shared_ptr<ExportQueueWidget> ExportQueueWidget::create(
            const std::shared_ptr<ftk::Context>& context,
            const std::shared_ptr<models::ExportQueue>& queue,
            const std::shared_ptr<IWidget>& parent)
        {
            auto out = std::shared_ptr<ExportQueueWidget>(new ExportQueueWidget);
            out->_init(context, queue, parent);
            return out;
        }
    }
}
