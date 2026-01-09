// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the DJV project.

#include <djvApp/Models/FilesModel.h>

#include <ftk/UI/Settings.h>
#include <ftk/Core/Math.h>

namespace djv
{
    namespace app
    {
        struct FilesModel::Private
        {
            std::shared_ptr<ftk::Settings> settings;

            std::shared_ptr<ftk::ObservableList<std::shared_ptr<FilesModelItem> > > files;
            std::shared_ptr<ftk::Observable<std::shared_ptr<FilesModelItem> > > a;
            std::shared_ptr<ftk::Observable<int> > aIndex;
            std::shared_ptr<ftk::ObservableList<std::shared_ptr<FilesModelItem> > > b;
            std::shared_ptr<ftk::ObservableList<int> > bIndexes;
            std::shared_ptr<ftk::ObservableList<std::shared_ptr<FilesModelItem> > > active;
            std::shared_ptr<ftk::ObservableList<int> > layers;
            std::shared_ptr<ftk::Observable<tl::CompareOptions> > compareOptions;
            std::shared_ptr<ftk::Observable<tl::CompareTime> > compareTime;
        };

        void FilesModel::_init(const std::shared_ptr<ftk::Settings>& settings)
        {
            FTK_P();

            p.settings = settings;

            p.files = ftk::ObservableList<std::shared_ptr<FilesModelItem> >::create();
            p.a = ftk::Observable<std::shared_ptr<FilesModelItem> >::create();
            p.aIndex = ftk::Observable<int>::create();
            p.b = ftk::ObservableList<std::shared_ptr<FilesModelItem> >::create();
            p.bIndexes = ftk::ObservableList<int>::create();
            p.active = ftk::ObservableList<std::shared_ptr<FilesModelItem> >::create();
            p.layers = ftk::ObservableList<int>::create();
            tl::CompareOptions compareOptions;
            p.settings->getT("/Files/CompareOptions", compareOptions);
            p.compareOptions = ftk::Observable<tl::CompareOptions>::create(compareOptions);
            std::string s;
            p.settings->get("/Files/CompareTime", s);
            tl::CompareTime compareTime = tl::CompareTime::First;
            from_string(s, compareTime);
            p.compareTime = ftk::Observable<tl::CompareTime>::create(compareTime);
        }

        FilesModel::FilesModel() :
            _p(new Private)
        {}

        FilesModel::~FilesModel()
        {
            FTK_P();
            p.settings->setT("/Files/CompareOptions", p.compareOptions->get());
            p.settings->set("/Files/CompareTime", to_string(p.compareTime->get()));
        }

        std::shared_ptr<FilesModel> FilesModel::create(const std::shared_ptr<ftk::Settings>& settings)
        {
            auto out = std::shared_ptr<FilesModel>(new FilesModel);
            out->_init(settings);
            return out;
        }

        const std::vector<std::shared_ptr<FilesModelItem> >& FilesModel::getFiles() const
        {
            return _p->files->get();
        }

        std::shared_ptr<ftk::IObservableList<std::shared_ptr<FilesModelItem> > > FilesModel::observeFiles() const
        {
            return _p->files;
        }

        const std::shared_ptr<FilesModelItem>& FilesModel::getA() const
        {
            return _p->a->get();
        }

        std::shared_ptr<ftk::IObservable<std::shared_ptr<FilesModelItem> > > FilesModel::observeA() const
        {
            return _p->a;
        }

        int FilesModel::getAIndex() const
        {
            return _p->aIndex->get();
        }

        std::shared_ptr<ftk::IObservable<int> > FilesModel::observeAIndex() const
        {
            return _p->aIndex;
        }

        const std::vector<std::shared_ptr<FilesModelItem> >& FilesModel::getB() const
        {
            return _p->b->get();
        }

        std::shared_ptr<ftk::IObservableList<std::shared_ptr<FilesModelItem> > > FilesModel::observeB() const
        {
            return _p->b;
        }

        const std::vector<int>& FilesModel::getBIndexes() const
        {
            return _p->bIndexes->get();
        }

        std::shared_ptr<ftk::IObservableList<int> > FilesModel::observeBIndexes() const
        {
            return _p->bIndexes;
        }

        const std::vector<std::shared_ptr<FilesModelItem> >& FilesModel::getActive() const
        {
            return _p->active->get();
        }

        std::shared_ptr<ftk::IObservableList<std::shared_ptr<FilesModelItem> > > FilesModel::observeActive() const
        {
            return _p->active;
        }

        void FilesModel::add(const std::shared_ptr<FilesModelItem>& item)
        {
            FTK_P();

            p.files->pushBack(item);

            p.a->setIfChanged(p.files->getItem(p.files->getSize() - 1));
            p.aIndex->setIfChanged(_getIndex(p.a->get()));

            p.active->setIfChanged(_getActive());
            p.layers->setIfChanged(_getLayers());
        }

        void FilesModel::close()
        {
            FTK_P();
            if (p.a->get())
            {
                close(_getIndex(p.a->get()));
            }
        }

        void FilesModel::close(int index)
        {
            FTK_P();
            auto files = p.files->get();
            if (index >= 0 && index < files.size())
            {
                const int aPrevIndex = _getIndex(p.a->get());

                files.erase(files.begin() + index);
                p.files->setIfChanged(files);

                if (aPrevIndex == index)
                {
                    const int aNewIndex = ftk::clamp(aPrevIndex, 0, static_cast<int>(files.size()) - 1);
                    p.a->setIfChanged(aNewIndex != -1 ? files[aNewIndex] : nullptr);
                }
                p.aIndex->setIfChanged(_getIndex(p.a->get()));

                auto b = p.b->get();
                auto j = b.begin();
                while (j != b.end())
                {
                    const auto k = std::find(files.begin(), files.end(), *j);
                    if (k == files.end())
                    {
                        j = b.erase(j);
                    }
                    else
                    {
                        ++j;
                    }
                }
                p.b->setIfChanged(b);
                p.bIndexes->setIfChanged(_getBIndexes());

                p.active->setIfChanged(_getActive());
                p.layers->setIfChanged(_getLayers());
            }
        }

        void FilesModel::closeAll()
        {
            FTK_P();

            p.files->clear();

            p.a->setIfChanged(nullptr);
            p.aIndex->setIfChanged(-1);

            p.b->clear();
            p.bIndexes->setIfChanged(_getBIndexes());

            p.active->setIfChanged(_getActive());
            p.layers->setIfChanged(_getLayers());
        }

        void FilesModel::setA(int index)
        {
            FTK_P();
            const int prevIndex = _getIndex(p.a->get());
            if (index >= 0 && index < p.files->getSize() && index != prevIndex)
            {
                p.a->setIfChanged(p.files->getItem(index));
                p.aIndex->setIfChanged(_getIndex(p.a->get()));

                p.active->setIfChanged(_getActive());
                p.layers->setIfChanged(_getLayers());
            }
        }

        void FilesModel::setB(int index, bool value)
        {
            FTK_P();
            if (index >= 0 && index < p.files->getSize())
            {
                auto b = p.b->get();
                int removedIndex = -1;
                const auto bIndexes = _getBIndexes();
                const auto i = std::find(bIndexes.begin(), bIndexes.end(), index);
                if (value && i == bIndexes.end())
                {
                    b.push_back(p.files->getItem(index));
                    switch (p.compareOptions->get().compare)
                    {
                    case tl::Compare::A:
                    case tl::Compare::B:
                    case tl::Compare::Wipe:
                    case tl::Compare::Overlay:
                    case tl::Compare::Difference:
                    case tl::Compare::Horizontal:
                    case tl::Compare::Vertical:
                        if (b.size() > 1)
                        {
                            removedIndex = _getIndex(b.front());
                            b.erase(b.begin());
                        }
                        break;
                    default: break;
                    }
                }
                else if (!value && i != bIndexes.end())
                {
                    b.erase(b.begin() + (i - bIndexes.begin()));
                }
                p.b->setIfChanged(b);
                p.bIndexes->setIfChanged(_getBIndexes());

                p.active->setIfChanged(_getActive());
                p.layers->setIfChanged(_getLayers());
            }
        }

        void FilesModel::toggleB(int index)
        {
            FTK_P();
            if (index >= 0 && index < p.files->getSize())
            {
                const auto& item = p.files->getItem(index);
                setB(index, p.b->indexOf(item) == ftk::ObservableListInvalidIndex);
            }
        }

        void FilesModel::clearB()
        {
            FTK_P();
            if (!p.b->isEmpty())
            {
                p.b->clear();
                p.bIndexes->setIfChanged(_getBIndexes());

                p.active->setIfChanged(_getActive());
                p.layers->setIfChanged(_getLayers());
            }
        }

        void FilesModel::first()
        {
            FTK_P();
            const int prevIndex = _getIndex(p.a->get());
            if (!p.files->isEmpty() && prevIndex != 0)
            {
                p.a->setIfChanged(p.files->getItem(0));
                p.aIndex->setIfChanged(_getIndex(p.a->get()));

                p.active->setIfChanged(_getActive());
                p.layers->setIfChanged(_getLayers());
            }
        }

        void FilesModel::last()
        {
            FTK_P();
            const int index = static_cast<int>(p.files->getSize()) - 1;
            const int prevIndex = _getIndex(p.a->get());
            if (!p.files->isEmpty() && index != prevIndex)
            {
                p.a->setIfChanged(p.files->getItem(index));
                p.aIndex->setIfChanged(_getIndex(p.a->get()));

                p.active->setIfChanged(_getActive());
                p.layers->setIfChanged(_getLayers());
            }
        }

        void FilesModel::next()
        {
            FTK_P();
            if (!p.files->isEmpty())
            {
                const int prevIndex = _getIndex(p.a->get());
                int index = prevIndex + 1;
                if (index >= p.files->getSize())
                {
                    index = 0;
                }
                p.a->setIfChanged(p.files->getItem(index));
                p.aIndex->setIfChanged(_getIndex(p.a->get()));

                p.active->setIfChanged(_getActive());
                p.layers->setIfChanged(_getLayers());
            }
        }

        void FilesModel::prev()
        {
            FTK_P();
            if (!p.files->isEmpty())
            {
                const int prevIndex = _getIndex(p.a->get());
                int index = prevIndex - 1;
                if (index < 0)
                {
                    index = p.files->getSize() - 1;
                }
                p.a->setIfChanged(p.files->getItem(index));
                p.aIndex->setIfChanged(_getIndex(p.a->get()));

                p.active->setIfChanged(_getActive());
                p.layers->setIfChanged(_getLayers());
            }
        }

        void FilesModel::firstB()
        {
            FTK_P();

            p.b->clear();
            if (!p.files->isEmpty())
            {
                p.b->pushBack(p.files->getItem(0));
            }
            p.bIndexes->setIfChanged(_getBIndexes());

            p.active->setIfChanged(_getActive());
            p.layers->setIfChanged(_getLayers());
        }

        void FilesModel::lastB()
        {
            FTK_P();

            p.b->clear();
            if (!p.files->isEmpty())
            {
                p.b->pushBack(p.files->getItem(p.files->getSize() - 1));
            }
            p.bIndexes->setIfChanged(_getBIndexes());

            p.active->setIfChanged(_getActive());
            p.layers->setIfChanged(_getLayers());
        }

        void FilesModel::nextB()
        {
            FTK_P();

            int index = 0;
            const auto bIndexes = _getBIndexes();
            if (!bIndexes.empty())
            {
                index = bIndexes[bIndexes.size() - 1];
                ++index;
            }
            if (index >= p.files->getSize())
            {
                index = 0;
            }
            p.b->clear();
            if (index >= 0 && index <= p.files->getSize())
            {
                p.b->pushBack(p.files->getItem(index));
            }
            p.bIndexes->setIfChanged(_getBIndexes());

            p.active->setIfChanged(_getActive());
            p.layers->setIfChanged(_getLayers());
        }

        void FilesModel::prevB()
        {
            FTK_P();

            int index = 0;
            const auto bIndexes = _getBIndexes();
            if (!bIndexes.empty())
            {
                index = bIndexes[0];
                --index;
            }
            if (index < 0)
            {
                index = static_cast<int>(p.files->getSize()) - 1;
            }
            p.b->clear();
            if (index >= 0 && index <= p.files->getSize())
            {
                p.b->pushBack(p.files->getItem(index));
            }
            p.bIndexes->setIfChanged(_getBIndexes());

            p.active->setIfChanged(_getActive());
            p.layers->setIfChanged(_getLayers());
        }

        std::shared_ptr<ftk::IObservableList<int> > FilesModel::observeLayers() const
        {
            return _p->layers;
        }

        void FilesModel::setLayer(const std::shared_ptr<FilesModelItem>& item, int layer)
        {
            FTK_P();
            const int index = _getIndex(item);
            if (index != -1 &&
                layer < p.files->getItem(index)->videoLayers.size() &&
                layer != p.files->getItem(index)->videoLayer)
            {
                p.files->getItem(index)->videoLayer = layer;
                p.layers->setIfChanged(_getLayers());
            }
        }

        void FilesModel::nextLayer()
        {
            FTK_P();
            const int index = _getIndex(p.a->get());
            if (index != -1)
            {
                auto item = p.files->getItem(index);
                int layer = item->videoLayer + 1;
                if (layer >= item->videoLayers.size())
                {
                    layer = 0;
                }
                item->videoLayer = layer;
                p.layers->setIfChanged(_getLayers());
            }
        }

        void FilesModel::prevLayer()
        {
            FTK_P();
            const int index = _getIndex(p.a->get());
            if (index != -1)
            {
                auto item = p.files->getItem(index);
                int layer = item->videoLayer - 1;
                if (layer < 0)
                {
                    layer = static_cast<int>(item->videoLayers.size()) - 1;
                }
                item->videoLayer = std::max(layer, 0);
                p.layers->setIfChanged(_getLayers());
            }
        }

        const tl::CompareOptions& FilesModel::getCompareOptions() const
        {
            return _p->compareOptions->get();
        }

        std::shared_ptr<ftk::IObservable<tl::CompareOptions> > FilesModel::observeCompareOptions() const
        {
            return _p->compareOptions;
        }

        void FilesModel::setCompareOptions(const tl::CompareOptions& value)
        {
            FTK_P();
            if (p.compareOptions->setIfChanged(value))
            {
                auto b = p.b->get();
                switch (p.compareOptions->get().compare)
                {
                case tl::Compare::A:
                case tl::Compare::B:
                case tl::Compare::Wipe:
                case tl::Compare::Overlay:
                case tl::Compare::Difference:
                case tl::Compare::Horizontal:
                case tl::Compare::Vertical:
                {
                    while (b.size() > 1)
                    {
                        b.pop_back();
                    }
                    break;
                }
                default: break;
                }
                switch (p.compareOptions->get().compare)
                {
                case tl::Compare::B:
                case tl::Compare::Wipe:
                case tl::Compare::Overlay:
                case tl::Compare::Difference:
                case tl::Compare::Horizontal:
                case tl::Compare::Vertical:
                case tl::Compare::Tile:
                {
                    if (b.empty() && !p.files->isEmpty())
                    {
                        int index = _getIndex(p.a->get());
                        if (index != -1)
                        {
                            index = index - 1;
                            if (index < 0)
                            {
                                index = p.files->getSize() - 1;
                            }
                        }
                        b.push_back(p.files->getItem(index));
                    }
                    break;
                }
                default: break;
                }
                if (p.b->setIfChanged(b))
                {
                    p.bIndexes->setIfChanged(_getBIndexes());
                }

                p.active->setIfChanged(_getActive());
                p.layers->setIfChanged(_getLayers());
            }
        }

        tl::CompareTime FilesModel::getCompareTime() const
        {
            return _p->compareTime->get();
        }

        std::shared_ptr<ftk::IObservable<tl::CompareTime> > FilesModel::observeCompareTime() const
        {
            return _p->compareTime;
        }

        void FilesModel::setCompareTime(tl::CompareTime value)
        {
            FTK_P();
            p.compareTime->setIfChanged(value);
        }

        int FilesModel::_getIndex(const std::shared_ptr<FilesModelItem>& item) const
        {
            FTK_P();
            size_t index = p.files->indexOf(item);
            return index != ftk::ObservableListInvalidIndex ? index : -1;
        }

        std::vector<int> FilesModel::_getBIndexes() const
        {
            FTK_P();
            std::vector<int> out;
            for (const auto& b : p.b->get())
            {
                out.push_back(_getIndex(b));
            }
            return out;
        }

        std::vector<std::shared_ptr<FilesModelItem> > FilesModel::_getActive() const
        {
            FTK_P();
            std::vector<std::shared_ptr<FilesModelItem> > out;
            if (p.a->get())
            {
                out.push_back(p.a->get());
            }
            switch (p.compareOptions->get().compare)
            {
            case tl::Compare::A:
                if (!p.b->isEmpty())
                {
                    out.push_back(p.b->getItem(0));
                }
                break;
            case tl::Compare::B:
            case tl::Compare::Wipe:
            case tl::Compare::Overlay:
            case tl::Compare::Difference:
            case tl::Compare::Horizontal:
            case tl::Compare::Vertical:
            case tl::Compare::Tile:
                for (const auto& b : p.b->get())
                {
                    out.push_back(b);
                }
                break;
            default: break;
            }
            return out;
        }

        std::vector<int> FilesModel::_getLayers() const
        {
            FTK_P();
            std::vector<int> out;
            for (const auto& f : p.files->get())
            {
                out.push_back(f->videoLayer);
            }
            return out;
        }
    }
}
