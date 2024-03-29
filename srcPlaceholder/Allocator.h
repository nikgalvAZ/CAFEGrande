/******************************************************************************/
/* Copyright (C) Teledyne Photometrics. All rights reserved.                  */
/******************************************************************************/
#pragma once
#ifndef PM_ALLOCATOR_H
#define PM_ALLOCATOR_H

/* Local */
#include "backend/AllocatorType.h"

/* System */
#include <cstdlib>

namespace pm {

class Allocator
{
protected:
    Allocator(AllocatorType type) : m_type(type) {}

public:
    virtual ~Allocator() {}

public:
    AllocatorType GetType() const
    { return m_type; }

public:
    virtual void* Allocate(size_t size) = 0;
    virtual void Free(void* ptr) = 0;

protected:
    AllocatorType m_type{ AllocatorType::Default };
};

} // namespace

#endif
