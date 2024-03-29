/******************************************************************************/
/* Copyright (C) Teledyne Photometrics. All rights reserved.                  */
/******************************************************************************/
#include "backend/AllocatorDefault.h"

pm::AllocatorDefault::AllocatorDefault()
    : Allocator(AllocatorType::Default)
{
}

void* pm::AllocatorDefault::Allocate(size_t size)
{
    return std::malloc(size);
}

void pm::AllocatorDefault::Free(void* ptr)
{
    std::free(ptr);
}
