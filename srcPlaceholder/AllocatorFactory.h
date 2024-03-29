/******************************************************************************/
/* Copyright (C) Teledyne Photometrics. All rights reserved.                  */
/******************************************************************************/
#pragma once
#ifndef PM_ALLOCATOR_FACTORY_H
#define PM_ALLOCATOR_FACTORY_H

/* Local */
#include "backend/Allocator.h"

/* System */
#include <memory>

namespace pm {

class AllocatorFactory
{
public:
    static std::shared_ptr<Allocator> Create(AllocatorType type);

public:
    static size_t GetAlignment(AllocatorType type);
    static size_t GetAlignment(const Allocator& allocator);

    static size_t GetAlignedSize(size_t size, AllocatorType type);
    static size_t GetAlignedSize(size_t size, const Allocator& allocator);
};

} // namespace

#endif
