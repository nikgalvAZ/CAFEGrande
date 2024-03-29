/******************************************************************************/
/* Copyright (C) Teledyne Photometrics. All rights reserved.                  */
/******************************************************************************/
#pragma once
#ifndef PM_ALLOCATOR_ALIGNED_H
#define PM_ALLOCATOR_ALIGNED_H

/* Local */
#include "backend/Allocator.h"

namespace pm {

class AllocatorAligned : public Allocator
{
protected:
    AllocatorAligned(AllocatorType type);

public:
    size_t GetAlignment() const;

public: // From Allocator
    virtual void* Allocate(size_t size) override;
    virtual void Free(void* ptr) override;

private:
    const size_t m_alignment;
};

class AllocatorAligned16 : public AllocatorAligned
{
public:
    AllocatorAligned16() : AllocatorAligned(AllocatorType::Align16) {}
};

class AllocatorAligned32 : public AllocatorAligned
{
public:
    AllocatorAligned32() : AllocatorAligned(AllocatorType::Align32) {}
};

class AllocatorAligned4k : public AllocatorAligned
{
public:
    AllocatorAligned4k() : AllocatorAligned(AllocatorType::Align4k) {}
};

} // namespace

#endif
