/******************************************************************************/
/* Copyright (C) Teledyne Photometrics. All rights reserved.                  */
/******************************************************************************/
#include "backend/AllocatorAligned.h"

/* Local */
#include "backend/AllocatorFactory.h"

/* System */
#include <cassert>

#ifdef _WIN32
    #include <malloc.h> // _aligned_malloc
#else
    #include <stdlib.h> // aligned_alloc
#endif

pm::AllocatorAligned::AllocatorAligned(AllocatorType type)
    : Allocator(type),
    m_alignment(AllocatorFactory::GetAlignment(type))
{
    // Alignment must be power of two
    assert((m_alignment & (m_alignment - 1)) == 0);
    // Alignment must be multiple of pointer size (relies on check above)
    assert(m_alignment >= sizeof(void*));
}

size_t pm::AllocatorAligned::GetAlignment() const
{
    return m_alignment;
}

void* pm::AllocatorAligned::Allocate(size_t size)
{
    // The size argument should be multiple of alignment as per C11, otherwise
    // undefined behavior. This applies only to the non-Windows version.
#ifdef _WIN32
    return ::_aligned_malloc(size, m_alignment);
#else
    const size_t sizeAligned = (size + (m_alignment - 1)) & ~(m_alignment - 1);
    return ::aligned_alloc(m_alignment, sizeAligned);
#endif
}

void pm::AllocatorAligned::Free(void* ptr)
{
#ifdef _WIN32
    ::_aligned_free(ptr);
#else
    ::free(ptr);
#endif
}
