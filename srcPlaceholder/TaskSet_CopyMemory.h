/******************************************************************************/
/* Copyright (C) Teledyne Photometrics. All rights reserved.                  */
/******************************************************************************/
#pragma once
#ifndef PM_TASK_SET_COPY_MEMORY_H
#define PM_TASK_SET_COPY_MEMORY_H

/* Local */
#include "backend/FrameStats.h"
#include "backend/Task.h"
#include "backend/TaskSet.h"

/* System */
#include <vector>

namespace pm {

class TaskSet_CopyMemory : public TaskSet
{
private:
    class ATask : public Task
    {
    public:
        explicit ATask(std::shared_ptr<Semaphore> semDone, size_t taskIndex,
                size_t taskCount);

    public:
        void SetUp(void* dst, const void* src, size_t bytes);

    public: // Task
        virtual void Execute() override;

    private:
        size_t m_maxTasks{ 0 };
        void* m_dst{ nullptr };
        const void* m_src{ nullptr };
        size_t m_bytes{ 0 };
    };

public:
    explicit TaskSet_CopyMemory(std::shared_ptr<ThreadPool> pool);

public:
    void SetUp(void* dst, const void* src, size_t bytes);
};

} // namespace pm

#endif /* PM_TASK_SET_COPY_MEMORY_H */
