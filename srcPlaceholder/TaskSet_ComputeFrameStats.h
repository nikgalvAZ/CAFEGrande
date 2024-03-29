/******************************************************************************/
/* Copyright (C) Teledyne Photometrics. All rights reserved.                  */
/******************************************************************************/
#pragma once
#ifndef PM_TASK_SET_COMPUTE_FRAME_STATS_H
#define PM_TASK_SET_COMPUTE_FRAME_STATS_H

/* Local */
#include "backend/FrameStats.h"
#include "backend/Task.h"
#include "backend/TaskSet.h"

/* System */
#include <vector>

namespace pm {

class Bitmap;

// See https://en.wikipedia.org/wiki/Algorithms_for_calculating_variance
class TaskSet_ComputeFrameStats : public TaskSet
{
private:
    class ATask final : public Task
    {
    public:
        explicit ATask(std::shared_ptr<Semaphore> semDone, size_t taskIndex,
                size_t taskCount);

    public:
        void SetUp(const Bitmap* bmp, FrameStats* stats);

    public: // Task
        virtual void Execute() override;

    private:
        // Uses generic one-pass algorithm with real numbers as described in
        // Higher-order statistics chapter on wikipedia.
        template<typename T>
        void ExecuteT(size_t chunkOffset, size_t chunkPixels);

        // Uses faster one-pass Naive algorithm with small integral numbers.
        // With greater numbers it is unusable due to catastrophic cancellation.
        template<typename T>
        void ExecuteT_UpTo16b(size_t chunkOffset, size_t chunkPixels);

    private:
        size_t m_maxTasks{ 0 };
        Bitmap* m_bmp{ nullptr }; // Cannot be const to auto-generate assignment operator
        FrameStats* m_stats{ nullptr };
    };

public:
    explicit TaskSet_ComputeFrameStats(std::shared_ptr<ThreadPool> pool);

public:
    void SetUp(const Bitmap* bmp, FrameStats* stats);

public: // TaskSet
    virtual void Wait() override;
    template<typename Rep, typename Period>
    bool Wait(const std::chrono::duration<Rep, Period>& timeout);

private:
    void CollectResults();

private:
    FrameStats* m_stats{ nullptr };
    std::vector<FrameStats> m_taskStats{};
};

} // namespace pm

#endif /* PM_TASK_SET_COMPUTE_FRAME_STATS_H */
