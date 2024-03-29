/******************************************************************************/
/* Copyright (C) Teledyne Photometrics. All rights reserved.                  */
/******************************************************************************/
#pragma once
#ifndef PM_ALLOCATOR_DEFAULT_H
#define PM_ALLOCATOR_DEFAULT_H

/* Local */
#include "backend/Allocator.h"

/* System */
#include <cstdlib>

namespace pm {

class AllocatorDefault : public Allocator
{
public:
    AllocatorDefault();

public: // From Allocator
    virtual void* Allocate(size_t size) override;
    virtual void Free(void* ptr) override;
};

} // namespace

#endif
