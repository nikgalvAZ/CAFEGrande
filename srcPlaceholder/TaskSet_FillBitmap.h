/******************************************************************************/
/* Copyright (C) Teledyne Photometrics. All rights reserved.                  */
/******************************************************************************/
#pragma once
#ifndef PM_TASK_SET_FILL_BITMAP_H
#define PM_TASK_SET_FILL_BITMAP_H

/* Local */
#include "backend/FrameStats.h"
#include "backend/Task.h"
#include "backend/TaskSet.h"

namespace pm {

class Bitmap;

class TaskSet_FillBitmap : public TaskSet
{
private:
    class ATask final : public Task
    {
    public:
        explicit ATask(std::shared_ptr<Semaphore> semDone, size_t taskIndex,
                size_t taskCount);

    public:
        void SetUp(Bitmap* const dstBmp, const Bitmap* srcBmp, uint16_t srcOffX,
                uint16_t srcOffY);

    public: // Task
        virtual void Execute() override;

    private:
        template<typename Tdst>
        void ExecuteT();

        template<typename Tdst, typename Tsrc>
        void ExecuteTT();

    private:
        size_t m_maxTasks{ 0 };
        Bitmap* m_dstBmp{ nullptr }; // Cannot be const to auto-generate assignment operator
        Bitmap* m_srcBmp{ nullptr }; // Cannot be const to auto-generate assignment operator
        uint16_t m_srcOffX;
        uint16_t m_srcOffY;
    };

public:
    explicit TaskSet_FillBitmap(std::shared_ptr<ThreadPool> pool);

public:
    void SetUp(Bitmap* const dstBmp, const Bitmap* srcBmp, uint16_t srcOffX,
            uint16_t srcOffY);
};

} // namespace pm

#endif /* PM_TASK_SET_FILL_BITMAP_H */
