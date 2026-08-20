// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the DJV project.

#pragma once

#include <djv/Models/Export.h>

#include <tlRender/Timeline/CompareOptions.h>

#include <ftk/Core/ObservableList.h>
#include <ftk/Core/Observable.h>
#include <ftk/Core/Path.h>

namespace ftk
{
    class Settings;
}

namespace djv
{
    namespace models
    {
        //! Files model item.
        struct DJV_API_TYPE FilesModelItem
        {
            ftk::Path                path;
            ftk::Path                audioPath;

            std::vector<std::string> videoLayers;
            size_t                   videoLayer  = 0;

            double                   speed       = -1.0;

            //! Where playback had got to, and the in/out points, when the
            //! file last lost focus. Unset until it has been played.
            std::optional<OTIO_NS::RationalTime> currentTime;
            std::optional<OTIO_NS::TimeRange>    inOutRange;

            //! The range the file turned out to have when it was opened. For
            //! an image sequence this is the frames that were found, which
            //! the path does not carry when it names a single file. Unset
            //! until the file has been opened.
            std::optional<OTIO_NS::TimeRange>    timeRange;

            //! Whether the range on the path was asked for rather than found.
            //! A stated range is used as it is; an unstated one is looked for
            //! on disk each time the file is opened, so that frames rendered
            //! since last time are picked up.
            bool                     framesStated = false;

            bool                     newFile = true;
        };

        //! Files model.
        class DJV_API_TYPE FilesModel : public std::enable_shared_from_this<FilesModel>
        {
            FTK_NON_COPYABLE(FilesModel);

        protected:
            void _init(const std::shared_ptr<ftk::Settings>&);

            FilesModel();

        public:
            DJV_API ~FilesModel();

            //! Create a new model.
            DJV_API static std::shared_ptr<FilesModel> create(
                const std::shared_ptr<ftk::Settings>&);

            //! Get the files.
            DJV_API const std::vector<std::shared_ptr<FilesModelItem> >& getFiles() const;

            //! Observe the files.
            DJV_API std::shared_ptr<ftk::IObservableList<std::shared_ptr<FilesModelItem> > > observeFiles() const;

            //! Get the "A" file.
            DJV_API const std::shared_ptr<FilesModelItem>& getA() const;

            //! Observe the "A" file.
            DJV_API std::shared_ptr<ftk::IObservable<std::shared_ptr<FilesModelItem> > > observeA() const;

            //! Get the "A" file index.
            DJV_API int getAIndex() const;

            //! Observe the "A" file index.
            DJV_API std::shared_ptr<ftk::IObservable<int> > observeAIndex() const;

            //! Get the "B" files.
            DJV_API const std::vector<std::shared_ptr<FilesModelItem> >& getB() const;

            //! Observe the "B" files.
            DJV_API std::shared_ptr<ftk::IObservableList<std::shared_ptr<FilesModelItem> > > observeB() const;

            //! Get the "B" file indexes.
            DJV_API const std::vector<int>& getBIndexes() const;

            //! Observe the "B" file indexes.
            DJV_API std::shared_ptr<ftk::IObservableList<int> > observeBIndexes() const;

            //! Get the active files. The active files are the "A" file and
            //! "B" files.
            DJV_API const std::vector<std::shared_ptr<FilesModelItem> >& getActive() const;

            //! Observe the active files. The active files are the "A" file
            //! and "B" files.
            DJV_API std::shared_ptr<ftk::IObservableList<std::shared_ptr<FilesModelItem> > > observeActive() const;

            //! Add a file.
            DJV_API void add(const std::shared_ptr<FilesModelItem>&);

            //! Close the current "A" file.
            DJV_API void close();

            //! Close the given file.
            DJV_API void close(int);

            //! Close all the files.
            DJV_API void closeAll();

            //! Set the "A" file.
            DJV_API void setA(int index);

            //! Set the "B" files.
            DJV_API void setB(int index, bool);

            //! Toggle a "B" file.
            DJV_API void toggleB(int index);

            //! Clear the "B" files.
            DJV_API void clearB();

            //! Set the "A" file to the first file.
            DJV_API void first();

            //! Set the "A" file to the last file.
            DJV_API void last();

            //! Set the "A" file to the next file.
            DJV_API void next();

            //! Set the "A" file to the previous file.
            DJV_API void prev();

            //! Set the "B" file to the first file.
            DJV_API void firstB();

            //! Set the "B" file to the last file.
            DJV_API void lastB();

            //! Set the "B" file to the next file.
            DJV_API void nextB();

            //! Set the "B" file to the previous file.
            DJV_API void prevB();

            //! Observe the layers.
            DJV_API std::shared_ptr<ftk::IObservableList<int> > observeLayers() const;

            //! Set a layer.
            DJV_API void setLayer(const std::shared_ptr<FilesModelItem>&, int layer);

            //! Set the frame range of an image sequence. This is the range the
            //! sequence is meant to cover, which need not be the frames that
            //! are on disk: a render in progress is watched over the range it
            //! will have when it finishes. The file is reopened, since
            //! everything about a timeline is derived from its path.
            DJV_API void setFrames(
                const std::shared_ptr<FilesModelItem>&,
                const ftk::RangeI64&);

            //! Observe files that have to be reopened.
            DJV_API std::shared_ptr<ftk::IObservable<std::shared_ptr<FilesModelItem> > > observeReload() const;

            //! Announce the files again, for a change to what an item holds
            //! rather than to which items there are. The items are shared and
            //! written to in place -- the frame range and the layers are
            //! filled in once the file has been opened -- and the list itself
            //! does not change when they are, so nothing else says so.
            DJV_API void refresh();

            //! Set the "A" file to the next layer.
            DJV_API void nextLayer();

            //! Set the "A" file to the previous layer.
            DJV_API void prevLayer();

            //! Get the compare options.
            DJV_API const tl::CompareOptions& getCompareOptions() const;

            //! Observe the compare options.
            DJV_API std::shared_ptr<ftk::IObservable<tl::CompareOptions> > observeCompareOptions() const;

            //! Set the compare options.
            DJV_API void setCompareOptions(const tl::CompareOptions&);

            //! Get the compare time mode.
            DJV_API tl::CompareTime getCompareTime() const;

            //! Observe the compare time mode.
            DJV_API std::shared_ptr<ftk::IObservable<tl::CompareTime> > observeCompareTime() const;

            //! Set the compare time mode.
            DJV_API void setCompareTime(tl::CompareTime);

        private:
            int _getIndex(const std::shared_ptr<FilesModelItem>&) const;
            std::vector<int> _getBIndexes() const;
            std::vector<std::shared_ptr<FilesModelItem> > _getActive() const;
            std::vector<int> _getLayers() const;

            FTK_PRIVATE();
        };

        //! Labels for the comparison time, as shown to the user. The
        //! enumeration's own labels name the setting where it is stored and in
        //! the keyboard shortcuts, so they say what the values are rather than
        //! what they do, and are not these.
        DJV_API std::vector<std::string> getCompareTimeLabels();

    }
}
