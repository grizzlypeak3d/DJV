// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the DJV project.

#include <djv/UI/InfoWidget.h>

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
            ftk::Path path;
            std::string search;

            //! One per section, in the order they are shown.
            std::vector<std::string> sectionNames;
            std::map<std::string, std::shared_ptr<ftk::Bellows> > bellows;
            std::map<std::string, std::shared_ptr<ftk::TextEdit> > textEdits;
            std::shared_ptr<ftk::SearchBox> searchBox;

            std::shared_ptr<tl::Player> player;

            std::shared_ptr<ftk::Observer<std::string> > mediaReferenceKeyObserver;
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
            p.sectionNames = { "File", "Video", "Audio", "Metadata" };
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
            p.settings->get("/Information/Bellows", json);
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
            p.settings->set("/Information/Bellows", json);
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
            }
            else
            {
                p.info = tl::IOInfo();
                _widgetUpdate();
            }
        }

        namespace
        {
            typedef std::vector<std::pair<std::string, std::string> > Pairs;

            std::string timecode(const OTIO_NS::RationalTime& value)
            {
                std::stringstream ss;
                ss << value.to_timecode();
                return ss.str();
            }

            template<typename T>
            std::string str(const T& value)
            {
                std::stringstream ss;
                ss << value;
                return ss.str();
            }

            //! Audio times are counted in samples, at a rate that is not a
            //! timecode rate, so they are given in seconds.
            std::string seconds(const OTIO_NS::RationalTime& value)
            {
                std::stringstream ss;
                ss.precision(2);
                ss << std::fixed << value.rescaled_to(1.0).value() << " seconds";
                return ss.str();
            }

            std::string rate(double value)
            {
                std::stringstream ss;
                ss.precision(1);
                ss << std::fixed << value / 1000.0 << "kHz";
                return ss.str();
            }

            //! Name and directory rather than one full path: the name is
            //! what somebody scanning the tool wants first, and the two
            //! compose the full path for sharing.
            Pairs filePairs(const ftk::Path& path)
            {
                Pairs out;
                if (path.isEmpty())
                    return out;
                out.push_back({ "Name", path.getFileName() });
                out.push_back({ "Directory", path.getDir() });
                if (path.isSeq())
                {
                    out.push_back({ "Frames", path.getFrameRange(true) });
                }
                return out;
            }

            //! The video the file holds, and the video it is decoded to. The
            //! two are not always the same and the difference is the point:
            //! what the rest of the application reports is the decoded one.
            Pairs videoPairs(const tl::IOInfo& info)
            {
                Pairs out;
                if (info.video.empty())
                    return out;
                const ftk::ImageInfo& video = info.video[0];
                if (!info.videoSource.codec.empty())
                {
                    out.push_back({ "Codec", info.videoSource.codec });
                }
                if (!info.videoSource.pixelFormat.empty())
                {
                    out.push_back({ "Source Format", info.videoSource.pixelFormat });
                }
                out.push_back({ "Resolution",
                    str(video.size.w) + " " + str(video.size.h) });
                out.push_back({ "Pixel Type", str(video.type) });
                {
                    std::stringstream ss;
                    ss.precision(2);
                    ss << std::fixed << video.pixelAspectRatio;
                    out.push_back({ "Pixel Aspect Ratio", ss.str() });
                }
                out.push_back({ "Levels", str(video.videoLevels) });
                if (info.videoTime.has_value())
                {
                    out.push_back({ "Start Time", timecode(info.videoTime->start_time()) });
                    out.push_back({ "Duration", timecode(info.videoTime->duration()) });
                    std::stringstream ss;
                    ss.precision(2);
                    ss << std::fixed << info.videoTime->start_time().rate() << " FPS";
                    out.push_back({ "Speed", ss.str() });
                }
                return out;
            }

            Pairs audioPairs(const tl::IOInfo& info)
            {
                Pairs out;
                if (!info.audio.isValid())
                    return out;
                const tl::AudioSourceInfo& source = info.audioSource;
                if (!source.codec.empty())
                {
                    out.push_back({ "Codec", source.codec });
                }
                out.push_back({ "Channels", str(info.audio.channelCount) });
                out.push_back({ "Type", str(info.audio.type) });
                out.push_back({ "Sample Rate", rate(info.audio.sampleRate) });
                // Only when the file is not what comes out of the reader,
                // which is the case worth showing.
                if (source.channelCount != 0 &&
                    static_cast<int>(source.channelCount) != info.audio.channelCount)
                {
                    out.push_back({ "Source Channels",
                        str(static_cast<int>(source.channelCount)) });
                }
                if (source.type != tl::AudioType::None &&
                    source.type != info.audio.type)
                {
                    out.push_back({ "Source Type", str(source.type) });
                }
                if (source.sampleRate != 0 &&
                    static_cast<int>(source.sampleRate) != info.audio.sampleRate)
                {
                    out.push_back({ "Source Sample Rate", rate(source.sampleRate) });
                }
                if (info.audioTime.has_value())
                {
                    out.push_back({ "Start Time", seconds(info.audioTime->start_time()) });
                    out.push_back({ "Duration", seconds(info.audioTime->duration()) });
                }
                return out;
            }
        }

        void InfoWidget::_widgetUpdate()
        {
            FTK_P();
            const std::map<std::string, Pairs> sections =
            {
                { "File", filePairs(p.path) },
                { "Video", videoPairs(p.info) },
                { "Audio", audioPairs(p.info) },
                { "Metadata", Pairs(p.info.tags.begin(), p.info.tags.end()) }
            };
            // The values line up in one column across the sections, so the
            // tool and the copied text read as one document (#37). Metadata
            // is left out of the shared width: its keys are foreign and can
            // be very long, and a single camera tag should not push every
            // value to the right margin.
            std::map<std::string, Pairs> kept;
            size_t sharedSize = 0;
            for (const auto& name : p.sectionNames)
            {
                for (const auto& tag : sections.at(name))
                {
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
                        continue;
                    }
                    kept[name].push_back(tag);
                    if (name != "Metadata")
                    {
                        sharedSize = std::max(sharedSize, tag.first.size() + 2);
                    }
                }
            }
            for (const auto& name : p.sectionNames)
            {
                size_t maxSize = sharedSize;
                if ("Metadata" == name)
                {
                    maxSize = 0;
                    for (const auto& i : kept[name])
                    {
                        maxSize = std::max(maxSize, i.first.size() + 2);
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
