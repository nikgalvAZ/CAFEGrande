/******************************************************************************/
/* Copyright (C) Teledyne Photometrics. All rights reserved.                  */
/******************************************************************************/
#pragma once
#ifndef PM_FRAME_POOL_H
#define PM_FRAME_POOL_H

/* Local */
#include "backend/Frame.h"

/* System */
#include <memory>
#include <mutex>
#include <queue>

namespace pm {

class FramePool
{
public:
    enum Ops : int
    {
        None     = 0,
        Shrink   = 1 << 0,
        Prefetch = 1 << 1,
    };

public:
    explicit FramePool();

public:
    void Setup(Frame::AcqCfg acqCfg, bool deepCopy,
            std::shared_ptr<Allocator> allocator = nullptr);
    bool MatchesSetup(const Frame& frame) const;

    bool IsEmpty() const;
    void Clear();
    
    size_t GetSize() const;

    /* Returns one Frame, either from pool or newly allocated.
    PushBack is intentionally private. Once the returned shared_ptr gets
    out of scope, custom deleter will return it automatically to the pool. */
    std::shared_ptr<Frame> TakeFrame();

    bool EnsureReadyFrames(size_t count, int ops = FramePool::Ops::None);

private:
    bool MatchesSetup(Frame::AcqCfg acqCfg, bool deepCopy) const;
    std::unique_ptr<Frame> AllocateNewFrame();
    // Custom deleter for std::shared_ptr<Frame>
    void PushBack(Frame* frame);

private:
    mutable std::mutex m_mutex{};
    std::queue<std::unique_ptr<Frame>> m_queue{};

    pm::Frame::AcqCfg m_acqCfg{};
    bool m_deepCopy{ true };
    std::shared_ptr<Allocator> m_allocator{};
};

} // namespace

#endif
