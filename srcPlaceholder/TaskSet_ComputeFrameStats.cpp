/******************************************************************************/
/* Copyright (C) Teledyne Photometrics. All rights reserved.                  */
/******************************************************************************/
#include "backend/TaskSet_ComputeFrameStats.h"

/* Local */
#include "backend/Bitmap.h"
#include "backend/exceptions/Exception.h"

/* System */
#include <cassert>
#include <cmath>
#include <numeric>

// TaskSet_ComputeFrameStats::Task

pm::TaskSet_ComputeFrameStats::ATask::ATask(
        std::shared_ptr<Semaphore> semDone, size_t taskIndex, size_t taskCount)
    : pm::Task(semDone, taskIndex, taskCount),
    m_maxTasks(taskCount)
{
}

void pm::TaskSet_ComputeFrameStats::ATask::SetUp(const Bitmap* bmp,
        FrameStats* stats)
{
    const size_t dataBytes = bmp->GetDataBytes();

    m_maxTasks = (dataBytes < 4096)
        ? (dataBytes == 0) ? 0 : 1
        : GetTaskCount();

    m_bmp = const_cast<Bitmap*>(bmp);
    m_stats = stats;
}

void pm::TaskSet_ComputeFrameStats::ATask::Execute()
{
    assert(m_bmp != nullptr);
    assert(m_stats != nullptr);

    m_stats->Clear();

    const size_t taskId = GetTaskIndex();
    if (taskId >= m_maxTasks)
        return;

    const size_t pixels = (size_t)m_bmp->GetWidth() * m_bmp->GetHeight();

    size_t chunkPixels = pixels / m_maxTasks;
    const size_t chunkOffset = taskId * chunkPixels;
    if (taskId == m_maxTasks - 1)
    {
        chunkPixels += pixels % m_maxTasks;
    }

    switch (m_bmp->GetFormat().GetDataType())
    {
    case BitmapDataType::UInt8:
        ExecuteT_UpTo16b<uint8_t>(chunkOffset, chunkPixels);
        break;
    case BitmapDataType::UInt16:
        ExecuteT_UpTo16b<uint16_t>(chunkOffset, chunkPixels);
        break;
    case BitmapDataType::UInt32:
        if (m_bmp->GetFormat().GetBitDepth() <= 16)
            ExecuteT_UpTo16b<uint32_t>(chunkOffset, chunkPixels);
        else
            ExecuteT<uint32_t>(chunkOffset, chunkPixels);
        break;
    default:
        throw Exception("Unsupported bitmap data type");
    }
}

template<typename T>
void pm::TaskSet_ComputeFrameStats::ATask::ExecuteT(
        size_t chunkOffset, size_t chunkPixels)
{
    const T* dataStart = static_cast<T*>(m_bmp->GetData()) + chunkOffset;
    const T* dataEnd = dataStart + chunkPixels;

    T min = *dataStart;
    T max = *dataStart;
    double M1 = *dataStart; // mean
    double M2 = 0.0;
    uint32_t n = 1;

    for (const T* data = dataStart + 1; data < dataEnd; data++)
    {
        if (min > *data)
            min = *data;
        else if (max < *data)
            max = *data;
        n++;
        const double delta = *data - M1;
        const double delta_n = delta / n;
        M2 = M2 + delta_n * (n - 1) * delta;
        M1 = M1 + delta_n;
    }

    m_stats->SetDirectly(static_cast<uint32_t>(chunkPixels),
            static_cast<double>(min), static_cast<double>(max), M1, M2);
}

template<typename T>
void pm::TaskSet_ComputeFrameStats::ATask::ExecuteT_UpTo16b(
        size_t chunkOffset, size_t chunkPixels)
{
    const T* dataStart = static_cast<T*>(m_bmp->GetData()) + chunkOffset;
    const T* dataEnd = dataStart + chunkPixels;

    T min = *dataStart;
    T max = *dataStart;
    // For 16b utilizes max. 48 bits - W(16b)*H(16b)*P(16b)
    uint64_t sum = *dataStart;
    // For 16b utilizes all  64 bits - W(16b)*H(16b)*P(16b)*P(16b)
    uint64_t sumSq = uint64_t(*dataStart) * *dataStart;

    for (const T* data = dataStart + 1; data < dataEnd; data++)
    {
        if (min > *data)
            min = *data;
        else if (max < *data)
            max = *data;
        sum += *data;
        sumSq += uint64_t(*data) * *data;
    }

    m_stats->SetViaSums(static_cast<uint32_t>(chunkPixels),
            static_cast<double>(min), static_cast<double>(max),
            static_cast<double>(sum), static_cast<double>(sumSq));
}

// TaskSet_ComputeFrameStats

pm::TaskSet_ComputeFrameStats::TaskSet_ComputeFrameStats(
        std::shared_ptr<ThreadPool> pool)
    : TaskSet(pool)
{
    CreateTasks<ATask>();

    const size_t taskCount = GetTasks().size();
    m_taskStats.resize(taskCount);
}

void pm::TaskSet_ComputeFrameStats::SetUp(const Bitmap* bmp, FrameStats* stats)
{
    assert(bmp != nullptr);
    assert(stats != nullptr);

    // TODO: Implement
    if (bmp->GetFormat().GetPixelType() != BitmapPixelType::Mono)
        throw Exception("Unsupported bitmap pixel type");

    m_stats = stats;

    const auto& tasks = GetTasks();
    const size_t taskCount = tasks.size();
    for (size_t n = 0; n < taskCount; ++n)
    {
        static_cast<ATask*>(tasks[n])->SetUp(bmp, &m_taskStats[n]);
    }
}

void pm::TaskSet_ComputeFrameStats::Wait()
{
    TaskSet::Wait();
    CollectResults();
}

template<typename Rep, typename Period>
bool pm::TaskSet_ComputeFrameStats::Wait(
        const std::chrono::duration<Rep, Period>& timeout)
{
    const bool retVal = TaskSet::Wait(timeout);
    CollectResults();
    return retVal;
}

void pm::TaskSet_ComputeFrameStats::CollectResults()
{
    m_stats->Clear();

    const size_t taskCount = GetTasks().size();
    for (size_t n = 0; n < taskCount; ++n)
    {
        m_stats->Add(m_taskStats[n]);
    }
}
