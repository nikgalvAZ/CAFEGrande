/******************************************************************************/
/* Copyright (C) Teledyne Photometrics. All rights reserved.                  */
/******************************************************************************/
#pragma once
#ifndef PM_RANDOM_PIXEL_CACHE_H
#define PM_RANDOM_PIXEL_CACHE_H

/* Local */
#include "backend/XoShiRo128Plus.h"

/* System */
#include <cstddef>
#include <cstdint>

namespace pm {

template <typename T>
class RandomPixelCache final
{
public:
    explicit RandomPixelCache(size_t size);
    ~RandomPixelCache();

    RandomPixelCache(const RandomPixelCache&) = delete;
    RandomPixelCache(RandomPixelCache&&) = delete;
    RandomPixelCache& operator=(const RandomPixelCache&) = delete;
    RandomPixelCache& operator=(RandomPixelCache&&) = delete;

public:
    // Regenerates the cache.
    // Bit spread means how many bits are used to vary the pixel value.
    // Generated value is added to offset.
    void Update(T offset, uint8_t bitSpread = 4);

    // Fills the buffer with values from cache
    void Fill(void* const dstBuffer, size_t dstBytes);

private:
    const size_t m_size;
    XoShiRo128Plus m_rand{};
    T* m_data{ nullptr }; // Not using std::unique_ptr due to performance in debug
    size_t m_index{ 0 }; // Next index to read from
    uint8_t m_bitSpread{ 0 }; // Last bit spread the data is generated for
    T m_offset{ 0 }; // Last offset the data is generated for
};

} // namespace

#endif
