/******************************************************************************/
/* Copyright (C) Teledyne Photometrics. All rights reserved.                  */
/******************************************************************************/
#include "backend/FramePool.h"

/* System */
#include <functional>

#ifdef _WIN32
    #include <Windows.h>
#else
    #include <unistd.h>
#endif

const static auto sPageSize = []() -> size_t
{
#ifdef _WIN32
    SYSTEM_INFO sysInfo;
    ::GetSystemInfo(&sysInfo);
    return sysInfo.dwPageSize;
#else
    return ::sysconf(_SC_PAGESIZE);
#endif
}();

pm::FramePool::FramePool()
{
}

void pm::FramePool::Setup(Frame::AcqCfg acqCfg, bool deepCopy,
        std::shared_ptr<Allocator> allocator)
{
    std::lock_guard<std::mutex> lock(m_mutex);

    if (!m_queue.empty() && !MatchesSetup(acqCfg, deepCopy))
    {
        // Release all frames, frame configuration has changed
        std::queue<std::unique_ptr<Frame>>().swap(m_queue);
    }

    m_acqCfg = acqCfg;
    m_deepCopy = deepCopy;
    m_allocator = allocator;
}

bool pm::FramePool::MatchesSetup(const Frame& frame) const
{
    std::lock_guard<std::mutex> lock(m_mutex);

    return MatchesSetup(frame.GetAcqCfg(), frame.UsesDeepCopy());
}

bool pm::FramePool::IsEmpty() const
{
    std::lock_guard<std::mutex> lock(m_mutex);

    return m_queue.empty();
}

void pm::FramePool::Clear()
{
    std::lock_guard<std::mutex> lock(m_mutex);

    if (m_queue.empty())
        return;

    std::queue<std::unique_ptr<Frame>>().swap(m_queue);
}

size_t pm::FramePool::GetSize() const
{
    std::lock_guard<std::mutex> lock(m_mutex);

    return m_queue.size();
}

std::shared_ptr<pm::Frame> pm::FramePool::TakeFrame()
{
    std::lock_guard<std::mutex> lock(m_mutex);

    std::unique_ptr<Frame> frame = nullptr;

    if (m_queue.empty())
    {
        frame = std::move(AllocateNewFrame());
    }
    else
    {
        frame = std::move(m_queue.front());
        m_queue.pop();
    }

    if (!frame)
        return nullptr;

    // Transform unique_ptr to shared_ptr with custom deleter
    return std::shared_ptr<Frame>(frame.release(),
            std::bind(&FramePool::PushBack, this, std::placeholders::_1));
}

bool pm::FramePool::EnsureReadyFrames(size_t count, int ops)
{
    std::lock_guard<std::mutex> lock(m_mutex);

    if (ops & FramePool::Ops::Shrink)
    {
        // Release surplus frames
        while (m_queue.size() > count)
        {
            m_queue.pop();
        }
    }

    const bool doPrefetch = m_deepCopy && (ops & FramePool::Ops::Prefetch);

    // Allocate missing ready-to-use frames
    while (m_queue.size() < count)
    {
        std::unique_ptr<Frame> frame = std::move(AllocateNewFrame());
        if (!frame)
            return false;

        if (doPrefetch)
        {
            // HACK: The const casted out for now
            uint8_t* data = (uint8_t*)frame->GetData();
            const size_t frameBytes = m_acqCfg.GetFrameBytes();
            for (size_t n = 0; n < frameBytes; n += sPageSize)
            {
                data[n] = 0xA5;
            }
        }

        m_queue.push(std::move(frame));
    }

    return true;
}

bool pm::FramePool::MatchesSetup(Frame::AcqCfg acqCfg, bool deepCopy) const
{
    return (m_acqCfg == acqCfg && m_deepCopy == deepCopy);
}

std::unique_ptr<pm::Frame> pm::FramePool::AllocateNewFrame()
{
    std::unique_ptr<Frame> frame = nullptr;

    if (m_acqCfg.GetFrameBytes() == 0)
        return nullptr;

    try
    {
        frame = std::make_unique<Frame>(m_acqCfg, m_deepCopy, m_allocator);
    }
    catch (...)
    {
    }

    return frame;
}

void pm::FramePool::PushBack(Frame* frame)
{
    if (!frame)
        return;

    std::lock_guard<std::mutex> lock(m_mutex);

    if (!MatchesSetup(frame->GetAcqCfg(), frame->UsesDeepCopy()))
    {
        delete frame;
        return;
    }

    frame->Invalidate();
    m_queue.push(std::move(std::unique_ptr<Frame>(frame)));
}
