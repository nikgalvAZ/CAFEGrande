/******************************************************************************/
/* Copyright (C) Teledyne Photometrics. All rights reserved.                  */
/******************************************************************************/
#pragma once
#ifndef PM_TASK_SET_FILL_BITMAP_VALUE_H
#define PM_TASK_SET_FILL_BITMAP_VALUE_H

/* Local */
#include "backend/FrameStats.h"
#include "backend/Task.h"
#include "backend/TaskSet.h"

namespace pm {

class Bitmap;

class TaskSet_FillBitmapValue : public TaskSet
{
private:
    class ATask final : public Task
    {
    public:
        explicit ATask(std::shared_ptr<Semaphore> semDone, size_t taskIndex,
                size_t taskCount);

    public:
        void SetUp(Bitmap* const bmp, double value);

    public: // Task
        virtual void Execute() override;

    private:
        template<typename T>
        void ExecuteT();

    private:
        size_t m_maxTasks{ 0 };
        Bitmap* m_bmp{ nullptr }; // Cannot be const to auto-generate assignment operator
        double m_value;
    };

public:
    explicit TaskSet_FillBitmapValue(std::shared_ptr<ThreadPool> pool);

public:
    void SetUp(Bitmap* const bmp, double value);
};

} // namespace pm

#endif /* PM_TASK_SET_FILL_BITMAP_VALUE_H */
