/******************************************************************************/
/* Copyright (C) Teledyne Photometrics. All rights reserved.                  */
/******************************************************************************/
#ifndef PM_UNIQUE_THREAD_POOL_H
#define PM_UNIQUE_THREAD_POOL_H

/* Local */
#include "backend/ThreadPool.h"

namespace pm {

class UniqueThreadPool final
{
public:
    // Creates singleton instance
    static UniqueThreadPool& Get();

public:
    std::shared_ptr<ThreadPool> GetPool() const;

private:
    UniqueThreadPool();
    UniqueThreadPool(const UniqueThreadPool&) = delete;
    UniqueThreadPool(UniqueThreadPool&&) = delete;
    void operator=(const UniqueThreadPool&) = delete;
    void operator=(UniqueThreadPool&&) = delete;
    ~UniqueThreadPool();

private:
    std::shared_ptr<ThreadPool> m_threadPool{ nullptr };
};

} // namespace pm

#endif // PM_UNIQUE_THREAD_POOL_H
