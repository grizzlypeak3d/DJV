// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the DJV project.

#include <djv/UI/InfoWidget.h>
#include <djv/Models/Info.h>
#include <djv/Models/SettingsKeys.h>

#include <tlRender/Timeline/Player.h>

#include <ftk/UI/Bellows.h>
#include <ftk/UI/ClipboardSystem.h>
#include <ftk/UI/Settings.h>
#include <ftk/UI/TextEdit.h>
#include <ftk/UI/ToolButton.h>
#include <ftk/UI/RowLayout.h>
#include <ftk/UI/ScreenshotTag.h>
#include <ftk/UI/SearchBox.h>
#include <ftk/Core/Format.h>
#include <ftk/Core/Path.h>
#include <ftk/Core/String.h>

namespace djv
{
    namespace ui
    {
        struct InfoWidget::Private
        {
            std::shared_ptr<ftk::Settings> settings;

            tl::IOInfo info;
            int videoLayer = 0;
            ftk::Path path;
            std::string search;

            //! One per section, in the order they are shown.
            std::vector<std::string> sectionNames;
            std::map<std::string, std::shared_ptr<ftk::Bellows> > bellows;
            std::map<std::string, std::shared_ptr<ftk::TextEdit> > textEdits;
            std::shared_ptr<ftk::SearchBox> searchBox;

            std::shared_ptr<tl::Player> player;

            std::shared_ptr<ftk::Observer<std::string> > mediaReferenceKeyObserver;
            std::shared_ptr<ftk::Observer<int> > videoLayerObserver;
        };

        void InfoWidget::_init(
            const std::shared_ptr<ftk::Context>& context,
            const std::shared_ptr<ftk::Settings>& settings,
            const std::shared_ptr<IWidget>& parent)
        {
            IContainer::_init(context, "djv::ui::InfoWidget", parent);
            FTK_P();

            p.settings = settings;

            auto copyButton = ftk::ToolButton::create(context, "Copy");
            ftk::setScreenshotTag(copyButton, "Info.Copy");

            p.searchBox = ftk::SearchBox::create(context);
            p.searchBox->setHStretch(ftk::Stretch::Expanding);
            ftk::setScreenshotTag(p.searchBox, "Info.Search");

            auto layout = ftk::VerticalLayout::create(context);
            layout->setSpacingRole(ftk::SizeRole::Border);
            // Above the sections, so that it stays in reach however much
            // there is below it.
            auto hLayout = ftk::HorizontalLayout::create(context, layout);
            hLayout->setMarginRole(ftk::SizeRole::MarginSmall);
            hLayout->setSpacingRole(ftk::SizeRole::SpacingSmall);
            p.searchBox->setParent(hLayout);
            copyButton->setParent(hLayout);
            ftk::TextEditOptions textEditOptions;
            textEditOptions.fontInfo.name = ftk::getDefaultFont(ftk::FontType::Mono);
            p.sectionNames = models::getInfoSections();
            for (const auto& name : p.sectionNames)
            {
                auto textEdit = ftk::TextEdit::create(context);
                textEdit->setReadOnly(true);
                textEdit->setOptions(textEditOptions);
                // The bellows are stacked in the tools panel's scroll area,
                // so each one takes the height of what is in it rather than
                // scrolling inside itself.
                textEdit->setSizeHintRole(ftk::SizeRole::None);
                p.textEdits[name] = textEdit;
                p.bellows[name] = ftk::Bellows::create(context, name, layout);
                p.bellows[name]->setWidget(textEdit);
                // Open until the settings say otherwise, so that a tool
                // opened for the first time shows what it has.
                p.bellows[name]->setOpen(true);
                // The state is written when it changes; the write in the
                // destructor is a backstop.
                p.bellows[name]->setOpenCallback(
                    [this](bool)
                    {
                        _saveSettings();
                    });
                ftk::setScreenshotTag(p.bellows[name], "Info." + name);
            }
            _setWidget(layout);

            // The same keys the tool has always used, so existing settings
            // carry over.
            nlohmann::json json;
            p.settings->get(models::settingsKeys::informationBellows, json);
            for (auto i = json.begin(); i != json.end(); ++i)
            {
                auto j = p.bellows.find(i.key());
                if (j != p.bellows.end() && i.value().is_boolean())
                {
                    j->second->setOpen(i.value().get<bool>());
                }
            }

            copyButton->setClickedCallback(
                [this]
                {
                    FTK_P();
                    auto context = getContext();
                    auto clipboardSystem = context->getSystem<ftk::ClipboardSystem>();
                    std::vector<std::string> text;
                    for (const auto& name : p.sectionNames)
                    {
                        const auto& lines = p.textEdits[name]->getText();
                        if (lines.empty())
                            continue;
                        if (!text.empty())
                        {
                            text.push_back(std::string());
                        }
                        text.push_back(name);
                        text.insert(text.end(), lines.begin(), lines.end());
                    }
                    clipboardSystem->setText(ftk::join(text, '\n'));
                });

            p.searchBox->setCallback(
                [this](const std::string& value)
                {
                    _p->search = value;
                    _widgetUpdate();
                });
        }

        InfoWidget::InfoWidget() :
            _p(new Private)
        {}

        InfoWidget::~InfoWidget()
        {
            _saveSettings();
        }

        void InfoWidget::_saveSettings()
        {
            FTK_P();
            nlohmann::json json;
            for (const auto& i : p.bellows)
            {
                json[i.first] = i.second->isOpen();
            }
            p.settings->set(models::settingsKeys::informationBellows, json);
        }

        std::shared_ptr<InfoWidget> InfoWidget::create(
            const std::shared_ptr<ftk::Context>& context,
            const std::shared_ptr<ftk::Settings>& settings,
            const std::shared_ptr<IWidget>& parent)
        {
            auto out = std::shared_ptr<InfoWidget>(new InfoWidget);
            out->_init(context, settings, parent);
            return out;
        }

        void InfoWidget::setPlayer(const std::shared_ptr<tl::Player>& value)
        {
            FTK_P();
            p.player = value;
            p.path = value ? value->getPath() : ftk::Path();
            p.mediaReferenceKeyObserver.reset();
            p.videoLayerObserver.reset();
            if (value)
            {
                // The information describes the media reference being
                // read, so it is refreshed when the key changes. The
                // observer also reports the current key, which covers
                // the new player.
                p.mediaReferenceKeyObserver = ftk::Observer<std::string>::create(
                    value->observeMediaReferenceKey(),
                    [this](const std::string&)
                    {
                        FTK_P();
                        p.info = p.player->getIOInfo();
                        _widgetUpdate();
                    });

                // And on the layer: what is described is the layer being
                // shown, and a file can hold layers of different types.
                p.videoLayerObserver = ftk::Observer<int>::create(
                    value->observeVideoLayer(),
                    [this](int layer)
                    {
                        FTK_P();
                        p.videoLayer = layer;
                        _widgetUpdate();
                    });
            }
            else
            {
                p.videoLayer = 0;
                p.info = tl::IOInfo();
                _widgetUpdate();
            }
        }

        void InfoWidget::_widgetUpdate()
        {
            FTK_P();
            // The rows and the search are the models', shared with the
            // iPad; the layout of the text is this widget's.
            const std::vector<models::InfoItem> items = models::filterInfo(
                models::getInfo(p.path, p.info, p.videoLayer),
                p.search);
            // The values line up in one column across the sections, so the
            // tool and the copied text read as one document (#37). Metadata
            // joins the column while its keys fit, and pads by itself when
            // they do not: the keys are foreign and can be very long, and a
            // single camera tag should not push every value to the right
            // margin.
            typedef std::vector<std::pair<std::string, std::string> > Pairs;
            std::map<std::string, Pairs> kept;
            size_t sharedSize = 0;
            for (const auto& item : items)
            {
                kept[item.section].push_back({ item.name, item.value });
                if (item.section != "Metadata")
                {
                    sharedSize = std::max(sharedSize, item.name.size() + 2);
                }
            }
            for (const auto& name : p.sectionNames)
            {
                size_t maxSize = sharedSize;
                if ("Metadata" == name)
                {
                    size_t metadataSize = 0;
                    for (const auto& i : kept[name])
                    {
                        metadataSize = std::max(metadataSize, i.first.size() + 2);
                    }
                    if (metadataSize > sharedSize)
                    {
                        maxSize = metadataSize;
                    }
                }

                std::vector<std::string> text;
                for (const auto& i : kept[name])
                {
                    std::string first = i.first + ": ";
                    first.resize(maxSize, ' ');
                    // A value can have newlines in it -- descriptions and
                    // synopses do. The text edit takes one line per entry,
                    // so give it one, with the rest lined up under the first
                    // rather than running back to the margin and over the
                    // following name.
                    const std::vector<std::string> lines = ftk::splitLines(i.second);
                    if (lines.empty())
                    {
                        text.emplace_back(first);
                    }
                    else
                    {
                        text.emplace_back(first + lines.front());
                        for (size_t j = 1; j < lines.size(); ++j)
                        {
                            text.emplace_back(std::string(maxSize, ' ') + lines[j]);
                        }
                    }
                }
                p.textEdits[name]->setText(text);
            }
        }
    }
}
