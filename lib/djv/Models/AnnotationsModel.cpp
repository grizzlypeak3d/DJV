// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the DJV project.

#include <djv/Models/AnnotationsModel.h>

#include <ftk/Core/Command.h>

#include <algorithm>
#include <cmath>

namespace djv
{
    namespace models
    {
        namespace
        {
            //! A command that swaps the whole annotation list.
            //!
            //! Annotations are small -- a few hundred strokes at most -- so
            //! snapshotting the list is simpler and safer than tracking
            //! incremental edits, and it makes every operation undoable the same
            //! way.
            class SnapshotCommand : public ftk::ICommand
            {
            public:
                SnapshotCommand(
                    const std::function<void(const std::vector<ReviewAnnotation>&)>& apply,
                    const std::vector<ReviewAnnotation>& before,
                    const std::vector<ReviewAnnotation>& after) :
                    _apply(apply),
                    _before(before),
                    _after(after)
                {}

                void exec() override { _apply(_after); }
                void undo() override { _apply(_before); }

            private:
                std::function<void(const std::vector<ReviewAnnotation>&)> _apply;
                std::vector<ReviewAnnotation> _before;
                std::vector<ReviewAnnotation> _after;
            };

            //! The distance from a point to a segment.
            float distanceToSegment(
                const ftk::V2F& p,
                const ftk::V2F& a,
                const ftk::V2F& b)
            {
                const float dx = b.x - a.x;
                const float dy = b.y - a.y;
                const float lengthSquared = dx * dx + dy * dy;
                float t = 0.F;
                if (lengthSquared > 0.F)
                {
                    t = ((p.x - a.x) * dx + (p.y - a.y) * dy) / lengthSquared;
                    t = std::max(0.F, std::min(1.F, t));
                }
                const float x = a.x + t * dx;
                const float y = a.y + t * dy;
                return std::sqrt((p.x - x) * (p.x - x) + (p.y - y) * (p.y - y));
            }

            //! Does the stroke pass within the radius of the position?
            bool strokeHit(const ReviewStroke& stroke, const ftk::V2F& pos, float radius)
            {
                // Include the stroke's own width, so a thick stroke is as easy to
                // hit as it looks.
                const float threshold = radius + stroke.width / 2.F;
                if (stroke.points.empty())
                {
                    return false;
                }
                if (1 == stroke.points.size())
                {
                    return distanceToSegment(pos, stroke.points[0], stroke.points[0]) <= threshold;
                }
                for (size_t i = 0; i + 1 < stroke.points.size(); ++i)
                {
                    if (distanceToSegment(pos, stroke.points[i], stroke.points[i + 1]) <= threshold)
                    {
                        return true;
                    }
                }
                return false;
            }
        }

        struct AnnotationsModel::Private
        {
            std::shared_ptr<ftk::ObservableList<ReviewAnnotation> > annotations;
            std::shared_ptr<ftk::CommandStack> commandStack;
        };

        void AnnotationsModel::_init()
        {
            FTK_P();
            p.annotations = ftk::ObservableList<ReviewAnnotation>::create();
            p.commandStack = ftk::CommandStack::create();
        }

        AnnotationsModel::AnnotationsModel() :
            _p(new Private)
        {}

        AnnotationsModel::~AnnotationsModel()
        {}

        std::shared_ptr<AnnotationsModel> AnnotationsModel::create()
        {
            auto out = std::shared_ptr<AnnotationsModel>(new AnnotationsModel);
            out->_init();
            return out;
        }

        const std::vector<ReviewAnnotation>& AnnotationsModel::getAnnotations() const
        {
            return _p->annotations->get();
        }

        std::shared_ptr<ftk::IObservableList<ReviewAnnotation> > AnnotationsModel::observeAnnotations() const
        {
            return _p->annotations;
        }

        std::vector<ReviewStroke> AnnotationsModel::getStrokes(
            const std::string& sourceId,
            const OTIO_NS::RationalTime& time) const
        {
            FTK_P();
            for (const auto& annotation : p.annotations->get())
            {
                if (annotation.sourceId == sourceId &&
                    sameTime(annotation.time, time))
                {
                    return annotation.strokes;
                }
            }
            return {};
        }

        void AnnotationsModel::setAnnotations(const std::vector<ReviewAnnotation>& value)
        {
            FTK_P();
            p.commandStack->clear();
            p.annotations->setIfChanged(value);
        }

        void AnnotationsModel::addStroke(
            const std::string& sourceId,
            const OTIO_NS::RationalTime& time,
            const ReviewStroke& stroke)
        {
            FTK_P();
            if (stroke.points.empty())
            {
                return;
            }
            auto annotations = p.annotations->get();
            const auto i = std::find_if(
                annotations.begin(),
                annotations.end(),
                [&sourceId, &time](const ReviewAnnotation& value)
                {
                    return value.sourceId == sourceId && sameTime(value.time, time);
                });
            if (i != annotations.end())
            {
                i->strokes.push_back(stroke);
            }
            else
            {
                ReviewAnnotation annotation;
                annotation.id = generateId();
                annotation.sourceId = sourceId;
                annotation.time = time;
                annotation.strokes.push_back(stroke);
                annotations.push_back(annotation);
            }
            _push(annotations);
        }

        void AnnotationsModel::eraseStrokes(
            const std::string& sourceId,
            const OTIO_NS::RationalTime& time,
            const ftk::V2F& pos,
            float radius)
        {
            FTK_P();
            auto annotations = p.annotations->get();
            bool changed = false;
            for (auto i = annotations.begin(); i != annotations.end(); )
            {
                if (i->sourceId == sourceId && sameTime(i->time, time))
                {
                    auto& strokes = i->strokes;
                    const size_t before = strokes.size();
                    strokes.erase(
                        std::remove_if(
                            strokes.begin(),
                            strokes.end(),
                            [&pos, radius](const ReviewStroke& stroke)
                            {
                                return strokeHit(stroke, pos, radius);
                            }),
                        strokes.end());
                    changed = changed || strokes.size() != before;
                }
                // Drop annotations that no longer hold anything, so they do not
                // leave a phantom timeline marker.
                if (i->strokes.empty())
                {
                    i = annotations.erase(i);
                }
                else
                {
                    ++i;
                }
            }
            if (changed)
            {
                _push(annotations);
            }
        }

        void AnnotationsModel::clearFrame(
            const std::string& sourceId,
            const OTIO_NS::RationalTime& time)
        {
            FTK_P();
            auto annotations = p.annotations->get();
            const size_t before = annotations.size();
            annotations.erase(
                std::remove_if(
                    annotations.begin(),
                    annotations.end(),
                    [&sourceId, &time](const ReviewAnnotation& value)
                    {
                        return value.sourceId == sourceId && sameTime(value.time, time);
                    }),
                annotations.end());
            if (annotations.size() != before)
            {
                _push(annotations);
            }
        }

        void AnnotationsModel::clear()
        {
            FTK_P();
            p.commandStack->clear();
            p.annotations->setIfChanged({});
        }

        std::shared_ptr<ftk::IObservable<bool> > AnnotationsModel::observeHasUndo() const
        {
            return _p->commandStack->observeHasUndo();
        }

        std::shared_ptr<ftk::IObservable<bool> > AnnotationsModel::observeHasRedo() const
        {
            return _p->commandStack->observeHasRedo();
        }

        void AnnotationsModel::undo()
        {
            _p->commandStack->undo();
        }

        void AnnotationsModel::redo()
        {
            _p->commandStack->redo();
        }

        void AnnotationsModel::_set(const std::vector<ReviewAnnotation>& value)
        {
            _p->annotations->setIfChanged(value);
        }

        void AnnotationsModel::_push(const std::vector<ReviewAnnotation>& value)
        {
            FTK_P();
            p.commandStack->push(std::make_shared<SnapshotCommand>(
                [this](const std::vector<ReviewAnnotation>& v) { _set(v); },
                p.annotations->get(),
                value));
        }
    }
}
