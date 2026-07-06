// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the DJV project.

#include <djv/ModelsTest/AudioModelTest.h>

#include <djv/ModelsTest/ModelsTestUtil.h>

#include <djv/Models/AudioModel.h>

#include <ftk/UI/Settings.h>

#include <ftk/Core/Assert.h>
#include <ftk/Core/Math.h>
#include <ftk/Core/Observable.h>

#include <filesystem>

namespace djv
{
    namespace models_tests
    {
        AudioModelTest::AudioModelTest(const std::shared_ptr<ftk::Context>& context) :
            ITest(context, "models_tests::AudioModelTest")
        {}

        std::shared_ptr<AudioModelTest> AudioModelTest::create(
            const std::shared_ptr<ftk::Context>& context)
        {
            return std::shared_ptr<AudioModelTest>(new AudioModelTest(context));
        }

        void AudioModelTest::run()
        {
            _behavior();
            _persistence();
        }

        void AudioModelTest::_behavior()
        {
            // In-memory settings keep this test isolated (no file I/O).
            auto settings = createTestSettings(_context);
            auto model = models::AudioModel::create(_context, settings);

            // Observe the volume and mute. With ObserverAction::Trigger (the
            // default) the callbacks fire immediately with the current values.
            float volume = 0.F;
            auto volumeObserver = ftk::Observer<float>::create(
                model->observeVolume(),
                [&volume](const float& value) { volume = value; });
            bool mute = true;
            auto muteObserver = ftk::Observer<bool>::create(
                model->observeMute(),
                [&mute](const bool& value) { mute = value; });

            // Defaults.
            FTK_ASSERT(1.F == model->getVolume());
            FTK_ASSERT(1.F == volume);
            FTK_ASSERT(!model->isMuted());
            FTK_ASSERT(!mute);

            // Set the volume; the observer should see the change.
            model->setVolume(.5F);
            FTK_ASSERT(.5F == model->getVolume());
            FTK_ASSERT(.5F == volume);

            // The volume is clamped to [0, 1].
            model->setVolume(2.F);
            FTK_ASSERT(1.F == model->getVolume());
            model->setVolume(-1.F);
            FTK_ASSERT(0.F == model->getVolume());

            // Step the volume up and down (+/- .01).
            model->setVolume(.5F);
            model->volumeUp();
            FTK_ASSERT(ftk::fuzzyCompare(.51F, model->getVolume()));
            model->volumeDown();
            FTK_ASSERT(ftk::fuzzyCompare(.5F, model->getVolume()));

            // Set the mute.
            model->setMute(true);
            FTK_ASSERT(model->isMuted());
            FTK_ASSERT(mute);
            model->setMute(false);
            FTK_ASSERT(!model->isMuted());
        }

        void AudioModelTest::_persistence()
        {
            // Unlike the behavior test, this needs a real (temporary) file so the
            // state can survive across model instances.
            const std::filesystem::path path =
                std::filesystem::temp_directory_path() / "djv-AudioModelTest.json";
            std::filesystem::remove(path);

            // Change the values. AudioModel writes them into the settings in its
            // destructor, and ftk::Settings saves to disk in its destructor. The
            // model is declared after the settings, so it is destroyed first --
            // writing the values before the settings save. reset=true starts from
            // a clean slate.
            {
                auto settings = ftk::Settings::create(_context, path, true);
                auto model = models::AudioModel::create(_context, settings);
                model->setVolume(.5F);
                model->setMute(true);
            }

            // Recreate from the same file (reset=false loads it) and verify the
            // values were restored.
            {
                auto settings = ftk::Settings::create(_context, path, false);
                auto model = models::AudioModel::create(_context, settings);
                FTK_ASSERT(ftk::fuzzyCompare(.5F, model->getVolume()));
                FTK_ASSERT(model->isMuted());
            }

            std::filesystem::remove(path);
        }
    }
}
