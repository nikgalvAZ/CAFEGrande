/******************************************************************************/
/* Copyright (C) Teledyne Photometrics. All rights reserved.                  */
/******************************************************************************/
#include "backend/TaskSet_FillBitmapValue.h"

/* Local */
#include "backend/Bitmap.h"
#include "backend/exceptions/Exception.h"

/* System */
#include <cassert>

// TaskSet_FillBitmapValue::Task

pm::TaskSet_FillBitmapValue::ATask::ATask(
        std::shared_ptr<Semaphore> semDone, size_t taskIndex, size_t taskCount)
    : pm::Task(semDone, taskIndex, taskCount),
    m_maxTasks(taskCount)
{
}

void pm::TaskSet_FillBitmapValue::ATask::SetUp(Bitmap* const bmp, double value)
{
    assert(bmp != nullptr);

    const auto srcHeight = bmp->GetHeight();
    const auto taskCount = GetTaskCount();
    m_maxTasks = (srcHeight < taskCount) ? srcHeight : taskCount;

    m_bmp = const_cast<Bitmap*>(bmp);
    m_value = value;
}

void pm::TaskSet_FillBitmapValue::ATask::Execute()
{
    if (GetTaskIndex() >= m_maxTasks)
        return;

    switch (m_bmp->GetFormat().GetDataType())
    {
    case BitmapDataType::UInt8:
        ExecuteT<uint8_t>();
        break;
    case BitmapDataType::UInt16:
        ExecuteT<uint16_t>();
        break;
    case BitmapDataType::UInt32:
        ExecuteT<uint32_t>();
        break;
    default:
        throw Exception("Unsupported destination bitmap data type");
    }
}

template<typename T>
void pm::TaskSet_FillBitmapValue::ATask::ExecuteT()
{
    const auto step = m_maxTasks;
    const auto offset = GetTaskIndex();

    const auto dest = static_cast<T*>(m_bmp->GetData());
    const auto totalCount = m_bmp->GetDataBytes() / sizeof(T);

    auto count = totalCount / step;
    const auto destOffset = offset * count;
    count += (offset == GetTaskCount() - 1) ? (totalCount % step) : 0;

    const T value = static_cast<T>(m_value);

    std::fill_n(dest + destOffset, count, value);
}

// TaskSet_FillBitmapValue

pm::TaskSet_FillBitmapValue::TaskSet_FillBitmapValue(
        std::shared_ptr<ThreadPool> pool)
    : TaskSet(pool)
{
    CreateTasks<ATask>();
}

void pm::TaskSet_FillBitmapValue::SetUp(Bitmap* const bmp, double value)
{
    const auto& tasks = GetTasks();
    for (auto task : tasks)
    {
        static_cast<ATask*>(task)->SetUp(bmp, value);
    }
}
