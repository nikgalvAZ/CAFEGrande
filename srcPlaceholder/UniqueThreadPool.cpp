/******************************************************************************/
/* Copyright (C) Teledyne Photometrics. All rights reserved.                  */
/******************************************************************************/
#include "backend/UniqueThreadPool.h"

pm::UniqueThreadPool& pm::UniqueThreadPool::Get()
{
    // This is thread-safe in C++11, but on Windows only since MSVS 2015
    static UniqueThreadPool instance;
    return instance;
}

std::shared_ptr<pm::ThreadPool> pm::UniqueThreadPool::GetPool() const
{
    return m_threadPool;
}

pm::UniqueThreadPool::UniqueThreadPool()
    : m_threadPool(std::make_shared<ThreadPool>(
            std::max(1u, std::thread::hardware_concurrency())))
{
}

pm::UniqueThreadPool::~UniqueThreadPool()
{
    m_threadPool = nullptr;
}
