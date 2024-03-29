/******************************************************************************/
/* Copyright (C) Teledyne Photometrics. All rights reserved.                  */
/******************************************************************************/
#include "backend/AllocatorFactory.h"

/* Local */
#include "backend/AllocatorAligned.h"
#include "backend/AllocatorDefault.h"

std::shared_ptr<pm::Allocator> pm::AllocatorFactory::Create(AllocatorType type)
{
    try
    {
        switch (type)
        {
        case AllocatorType::Align16: return std::make_shared<AllocatorAligned16>();
        case AllocatorType::Align32: return std::make_shared<AllocatorAligned32>();
        case AllocatorType::Align4k: return std::make_shared<AllocatorAligned4k>();
        case AllocatorType::Default: return std::make_shared<AllocatorDefault  >();
        }
    }
    catch (...)
    {}

    return nullptr;
}

size_t pm::AllocatorFactory::GetAlignment(AllocatorType type)
{
    switch (type)
    {
    case AllocatorType::Align16: return 16;
    case AllocatorType::Align32: return 32;
    case AllocatorType::Align4k: return 4096;
    default:
    case AllocatorType::Default: return 0;
    }
}

size_t pm::AllocatorFactory::GetAlignment(const Allocator& allocator)
{
    return GetAlignment(allocator.GetType());
}

size_t pm::AllocatorFactory::GetAlignedSize(size_t size, AllocatorType type)
{
    const auto alignment = GetAlignment(type);
    if (alignment <= 0)
        return size;
    const size_t sizeAligned = (size + (alignment - 1)) & ~(alignment - 1);
    return sizeAligned;
}

size_t pm::AllocatorFactory::GetAlignedSize(size_t size, const Allocator& allocator)
{
    return GetAlignedSize(size, allocator.GetType());
}
