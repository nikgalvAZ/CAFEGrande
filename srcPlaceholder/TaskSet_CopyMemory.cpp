/******************************************************************************/
/* Copyright (C) Teledyne Photometrics. All rights reserved.                  */
/******************************************************************************/
#include "backend/TaskSet_CopyMemory.h"

/* System */
#include <cassert>
#include <cstring> // std::memcpy

// TaskSet_CopyMemory::Task

pm::TaskSet_CopyMemory::ATask::ATask(
        std::shared_ptr<Semaphore> semDone, size_t taskIndex, size_t taskCount)
    : pm::Task(semDone, taskIndex, taskCount),
    m_maxTasks(taskCount)
{
}

void pm::TaskSet_CopyMemory::ATask::SetUp(void* dst, const void* src, size_t bytes)
{
    assert(dst != nullptr);
    assert(src != nullptr);
    assert(bytes != 0);

    m_maxTasks = (bytes < 4096)
        ? 1
        : GetTaskCount();

    m_dst = dst;
    m_src = src;
    m_bytes = bytes;
}

void pm::TaskSet_CopyMemory::ATask::Execute()
{
    if (GetTaskIndex() >= m_maxTasks)
        return;

    size_t chunkBytes = m_bytes / m_maxTasks;
    const size_t chunkOffset = GetTaskIndex() * chunkBytes;
    if (GetTaskIndex() == m_maxTasks - 1)
    {
        chunkBytes += m_bytes % m_maxTasks;
    }

    void* dst = static_cast<uint8_t*>(m_dst) + chunkOffset;
    const void* src = static_cast<const uint8_t*>(m_src) + chunkOffset;

    std::memcpy(dst, src, chunkBytes);
}

// TaskSet_CopyMemory

pm::TaskSet_CopyMemory::TaskSet_CopyMemory(
        std::shared_ptr<ThreadPool> pool)
    : TaskSet(pool)
{
    CreateTasks<ATask>();
}

void pm::TaskSet_CopyMemory::SetUp(void* dst, const void* src, size_t bytes)
{
    const auto& tasks = GetTasks();
    for (auto task : tasks)
    {
        static_cast<ATask*>(task)->SetUp(dst, src, bytes);
    }
}
