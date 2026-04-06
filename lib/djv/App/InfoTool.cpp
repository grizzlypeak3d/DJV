// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the DJV project.

#include <djv/App/InfoTool.h>

#include <djv/App/App.h>

#include <ftk/UI/ClipboardSystem.h>
#include <ftk/UI/Divider.h>
#include <ftk/UI/TextEdit.h>
#include <ftk/UI/ToolButton.h>
#include <ftk/UI/RowLayout.h>
#include <ftk/UI/ScrollWidget.h>
#include <ftk/UI/SearchBox.h>
#include <ftk/Core/String.h>

namespace djv
{
    namespace app
    {
        struct InfoTool::Private
        {
            tl::IOInfo info;
            std::string search;

            std::shared_ptr<ftk::TextEdit> textEdit;
            std::shared_ptr<ftk::SearchBox> searchBox;

            std::shared_ptr<ftk::Observer<std::shared_ptr<tl::Player> > > playerObserver;
        };

        void InfoTool::_init(
            const std::shared_ptr<ftk::Context>& context,
            const std::shared_ptr<App>& app,
            const std::shared_ptr<IWidget>& parent)
        {
            IToolWidget::_init(
                context,
                app,
                models::Tool::Info,
                "djv::app::InfoTool",
                parent);
            FTK_P();

            p.textEdit = ftk::TextEdit::create(context);
            p.textEdit->setReadOnly(true);
            ftk::TextEditOptions textEditOptions;
            textEditOptions.fontInfo.name = ftk::getDefaultFont(ftk::FontType::Mono);
            p.textEdit->setOptions(textEditOptions);
            p.textEdit->setVStretch(ftk::Stretch::Expanding);

            auto copyButton = ftk::ToolButton::create(context, "Copy");

            p.searchBox = ftk::SearchBox::create(context);
            p.searchBox->setHStretch(ftk::Stretch::Expanding);

            auto layout = ftk::VerticalLayout::create(context);
            layout->setMarginRole(ftk::SizeRole::MarginSmall);
            layout->setSpacingRole(ftk::SizeRole::SpacingSmall);
            p.textEdit->setParent(layout);
            auto hLayout = ftk::HorizontalLayout::create(context, layout);
            hLayout->setSpacingRole(ftk::SizeRole::SpacingSmall);
            copyButton->setParent(hLayout);
            p.searchBox->setParent(hLayout);
            _setWidget(layout);

            p.playerObserver = ftk::Observer<std::shared_ptr<tl::Player> >::create(
                app->observePlayer(),
                [this](const std::shared_ptr<tl::Player>& value)
                {
                    _p->info = value ? value->getIOInfo() : tl::IOInfo();
                    _widgetUpdate();
                });

            copyButton->setClickedCallback(
                [this]
                {
                    FTK_P();
                    auto context = getContext();
                    auto clipboardSystem = context->getSystem<ftk::ClipboardSystem>();
                    clipboardSystem->setText(ftk::join(p.textEdit->getText(), '\n'));
                });

            p.searchBox->setCallback(
                [this](const std::string& value)
                {
                    _p->search = value;
                    _widgetUpdate();
                });
        }

        InfoTool::InfoTool() :
            _p(new Private)
        {}

        InfoTool::~InfoTool()
        {}

        std::shared_ptr<InfoTool> InfoTool::create(
            const std::shared_ptr<ftk::Context>& context,
            const std::shared_ptr<App>& app,
            const std::shared_ptr<IWidget>& parent)
        {
            auto out = std::shared_ptr<InfoTool>(new InfoTool);
            out->_init(context, app, parent);
            return out;
        }

        void InfoTool::_widgetUpdate()
        {
            FTK_P();
            std::vector<std::pair<std::string, std::string> > pairs;
            size_t maxSize = 0;
            for (const auto& tag : p.info.tags)
            {
                bool filter = false;
                if (!p.search.empty() &&
                    !ftk::contains(
                        tag.first,
                        p.search,
                        ftk::CaseCompare::Insensitive) &&
                    !ftk::contains(
                        tag.second,
                        p.search,
                        ftk::CaseCompare::Insensitive))
                {
                    filter = true;
                }
                if (!filter)
                {
                    const std::string first = tag.first + ": ";
                    pairs.push_back(std::make_pair(first, tag.second));
                    maxSize = std::max(maxSize, first.size());
                }
            }
            std::vector<std::string> text;
            for (auto& i : pairs)
            {
                i.first.resize(maxSize, ' ');
                text.emplace_back(i.first + i.second);
            }
            p.textEdit->setText(text);
        }
    }
}
