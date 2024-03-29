#ifdef _MSC_VER
// Suppress warning C4996 on copy_n functions
#define _SCL_SECURE_NO_WARNINGS
#endif

/******************************************************************************/
/* Copyright (C) Teledyne Photometrics. All rights reserved.                  */
/******************************************************************************/
#include "backend/RandomPixelCache.h"

/* Local */
#include "backend/Log.h"
#include "backend/Timer.h"
#include "backend/Utils.h"

/* System */
#include <algorithm>
#include <cassert>
#include <cmath>

template <typename T>
pm::RandomPixelCache<T>::RandomPixelCache(size_t size)
    : m_size(size)
{
}

template <typename T>
pm::RandomPixelCache<T>::~RandomPixelCache()
{
    delete [] m_data;
}

template <typename T>
void pm::RandomPixelCache<T>::Update(T offset, uint8_t bitSpread)
{
    constexpr uint8_t maxBitDepth = 8 * sizeof(T);

    if (!m_data)
    {
        m_data = new T[m_size];
    }

    if (m_bitSpread == bitSpread && m_offset == offset)
        return;

    m_bitSpread = Utils::Clamp<uint8_t>(bitSpread, 1, maxBitDepth);
    m_offset = offset;

    //Timer t;
#ifdef _DEBUG
    // Use uniform distribution in debug build.
    // Single thread timing to generate 50M of 16bit pixels:
    // | OS                            | Compiler | Release  | Debug    |
    // |-------------------------------|----------|----------|----------|
    // | Windows 10   64bit (i7-7920X) | VS  2015 |   157 ms |  1050 ms |
    // | Ubuntu 18.04 64bit (i7-4770 ) | GCC  7.5 |   160 ms |   450 ms |
    // | Ubuntu 18.04 64bit (i5-8265U) | GCC  7.5 |   163 ms |   444 ms |
    //t.Reset();
    const auto shift = 32 - m_bitSpread;
    for (size_t n = 0; n < m_size; ++n)
    {
        m_data[n] = m_offset + static_cast<T>(m_rand.GetNext() >> shift);
    }
    //Log::LogW("%ub pixel cache update took %f ms", maxBitDepth, t.Milliseconds());
#else
    // Use poisson distribution in release build.
    // Single thread timing to generate 50M of 16bit pixels, bitSpread=4:
    // | OS                            | Compiler | Release  | Debug    |
    // |-------------------------------|----------|----------|----------|
    // | Windows 10   64bit (i7-7920X) | VS  2015 |  1630 ms | 11300 ms |
    // | Ubuntu 18.04 64bit (i7-4770 ) | GCC  7.5 |  1810 ms |  4400 ms |
    // | Ubuntu 18.04 64bit (i5-8265U) | GCC  7.5 |  1795 ms |  4330 ms |
    //t.Reset();
    const int lambda = 1 << (m_bitSpread - 1);
    const float target = std::exp((float)-lambda);
    for (size_t n = 0; n < m_size; ++n)
    {
        T spread = 0;
        float rand = m_rand.GetNext() / (float)0xFFFFFFFF;
        while (rand > target)
        {
            rand *= m_rand.GetNext() / (float)0xFFFFFFFF;
            spread++;
        }
        m_data[n] = m_offset + spread;
    }
    //Log::LogW("%ub pixel cache update took %f ms", maxBitDepth, t.Milliseconds());
#endif
}

template <typename T>
void pm::RandomPixelCache<T>::Fill(void* const dstBuffer, size_t dstBytes)
{
    assert(dstBytes % sizeof(T) == 0);
    const size_t dstSize = dstBytes / sizeof(T);

    //const uint8_t spp = m_frameAcqCfg.GetBitmapFormat().GetSamplesPerPixel();
    //assert(dstSize % spp == 0);

    auto buffer = static_cast<T* const>(dstBuffer);
    size_t copied = 0;
    while (copied < dstSize)
    {
        const auto toEnd = m_size - m_index;
        const auto size = std::min(dstSize - copied, toEnd);
        std::copy_n(m_data + m_index, size, buffer + copied);
        copied += size;
        m_index = (m_index + size) % m_size;
    }
}

// Implementation of the only allowed types
template class pm::RandomPixelCache<uint8_t >;
template class pm::RandomPixelCache<uint16_t>;
template class pm::RandomPixelCache<uint32_t>;
