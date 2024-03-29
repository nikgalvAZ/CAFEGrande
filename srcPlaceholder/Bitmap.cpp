/******************************************************************************/
/* Copyright (C) Teledyne Photometrics. All rights reserved.                  */
/******************************************************************************/
#include "backend/Bitmap.h"

/* Local */
#include "backend/exceptions/Exception.h"

/* System */
#include <algorithm>
#include <cassert>
#include <cstring> // memcpy

pm::Bitmap::Bitmap(void* data, uint32_t w, uint32_t h, const BitmapFormat& f,
        uint16_t lineAlign)
    : m_data(static_cast<uint8_t*>(data)),
    m_width(w),
    m_height(h),
    m_format(f),
    m_lineAlign(lineAlign),
    m_deleteData(false),
    m_stride(CalculateStrideBytes(w, f, lineAlign))
{
    m_dataBytes = m_stride * h;
}

pm::Bitmap::Bitmap(uint32_t w, uint32_t h, const BitmapFormat& f, uint16_t lineAlign)
    : m_width(w),
    m_height(h),
    m_format(f),
    m_lineAlign(lineAlign),
    m_deleteData(true),
    m_stride(CalculateStrideBytes(w, f, lineAlign))
{
    m_dataBytes = m_stride * h;
    m_data = new uint8_t[m_dataBytes]; // May throw with std::bad_alloc
}

pm::Bitmap::~Bitmap()
{
    if (m_deleteData)
    {
        delete [] m_data;
    }
}

size_t pm::Bitmap::CalculateStrideBytes(uint32_t w, const pm::BitmapFormat& f,
        uint16_t lineAlign)
{
    assert(lineAlign > 0);
    const size_t bytesPerPixel = f.GetBytesPerPixel();
    const size_t bytesPerLine = w * bytesPerPixel;
    const size_t bytesPerLineAligned =
        ((bytesPerLine + lineAlign - 1) / lineAlign) * lineAlign;
    return bytesPerLineAligned;
}

size_t pm::Bitmap::CalculateDataBytes(uint32_t w, uint32_t h,
        const pm::BitmapFormat& f, uint16_t lineAlign)
{
    const size_t stride = CalculateStrideBytes(w, f, lineAlign);
    const size_t dataBytes = stride * h;
    return dataBytes;
}

void* pm::Bitmap::GetData() const
{
    return m_data;
}

size_t pm::Bitmap::GetDataBytes() const
{
    return m_dataBytes;
}

uint32_t pm::Bitmap::GetWidth() const
{
    return m_width;
}

uint32_t pm::Bitmap::GetHeight() const
{
    return m_height;
}

const pm::BitmapFormat& pm::Bitmap::GetFormat() const
{
    return m_format;
}

void pm::Bitmap::ChangeColorMask(BayerPattern colorMask)
{
    m_format.SetColorMask(colorMask);
}

uint16_t pm::Bitmap::GetLineAlign() const
{
    return m_lineAlign;
}

size_t pm::Bitmap::GetStride() const
{
    return m_stride;
}

void* pm::Bitmap::GetScanLine(uint16_t y) const
{
    assert(m_data != nullptr);
    assert(y < m_height);
    uint8_t* const scanLine = m_data + y * m_stride;
    return scanLine;
}

double pm::Bitmap::GetSample(uint16_t x, uint16_t y, uint8_t sIdx) const
{
    assert(x < m_width);
    const uint8_t spp = m_format.GetSamplesPerPixel();
    const void* const scanLine = GetScanLine(y);
    const size_t pos = (size_t)x * spp + sIdx;
    switch (m_format.GetDataType())
    {
    case BitmapDataType::UInt8:
        return (static_cast<const uint8_t*>(scanLine))[pos];
    case BitmapDataType::UInt16:
        return (static_cast<const uint16_t*>(scanLine))[pos];
    case BitmapDataType::UInt32:
        return (static_cast<const uint32_t*>(scanLine))[pos];
    default:
        throw Exception("Unsupported bitmap data type");
    }
}

pm::Bitmap* pm::Bitmap::Clone() const
{
    assert(m_data != nullptr);
    // Allocate new bitmap
    Bitmap* bmpCopy = new Bitmap(m_width, m_height, m_format, m_lineAlign);
    // Copy current data to the new bitmap
    std::memcpy(bmpCopy->m_data, m_data, m_dataBytes);
    return bmpCopy;
}

template<typename Tdst, typename Tsrc>
void FillTT(pm::Bitmap* const dstBmp, const pm::Bitmap* srcBmp, uint16_t dstOffX,
        uint16_t dstOffY)
{
    // The images must have the same dimensions and must be the same pixel type.
    // E.g. we cannot copy from RGB to MONO frames
    if (dstBmp->GetFormat().GetPixelType() != srcBmp->GetFormat().GetPixelType())
        throw pm::Exception("Cannot process bitmaps with different pixel types");
    if (srcBmp->GetWidth() + dstOffX > dstBmp->GetWidth()
            || srcBmp->GetHeight() + dstOffY > dstBmp->GetHeight())
        throw pm::Exception(
                "Cannot process bitmaps, source doesn't fit the destination with given offset");

    const uint32_t w = srcBmp->GetWidth();
    const uint32_t h = srcBmp->GetHeight();

    if (dstBmp->GetFormat().GetBytesPerPixel() != srcBmp->GetFormat().GetBytesPerPixel())
    {
        const uint8_t spp = dstBmp->GetFormat().GetSamplesPerPixel();
        const uint32_t sprl = spp * w; ///< Samples per rect line
        const uint32_t dstSpoX = spp * dstOffX; ///< Samples per rect horiz. offset
        for (uint32_t y = 0; y < h; ++y)
        {
            Tdst* const dstLine = static_cast<Tdst* const>(
                    dstBmp->GetScanLine((uint16_t)y + dstOffY));
            const Tsrc* srcLine = static_cast<const Tsrc*>(
                    srcBmp->GetScanLine((uint16_t)y));
            // Do sample by sample as Tsrc and Tdst types have different size
            for (uint32_t x = 0; x < sprl; ++x)
            {
                dstLine[x + dstSpoX] = static_cast<Tdst>(srcLine[x]);
            }
        }
    }
    else
    {
        const size_t bpp = dstBmp->GetFormat().GetBytesPerPixel();
        const size_t bprl = bpp * w; ///< Bytes per rect line
        const size_t dstBpoX = bpp * dstOffX; ///< Bytes per rect horiz. offset
        for (uint32_t y = 0; y < h; ++y)
        {
            uint8_t* const dstLine = static_cast<uint8_t* const>(
                    dstBmp->GetScanLine((uint16_t)y + dstOffY));
            const uint8_t* srcLine = static_cast<const uint8_t*>(
                    srcBmp->GetScanLine((uint16_t)y));
            ::memcpy(dstLine + dstBpoX, srcLine, bprl);
        }
    }
}

template<typename Tdst>
void FillT(pm::Bitmap* const dstBmp, const pm::Bitmap* srcBmp, uint16_t dstOffX,
        uint16_t dstOffY)
{
    switch (srcBmp->GetFormat().GetDataType())
    {
    case pm::BitmapDataType::UInt8:
        FillTT<Tdst, uint8_t>(dstBmp, srcBmp, dstOffX, dstOffY);
        break;
    case pm::BitmapDataType::UInt16:
        FillTT<Tdst, uint16_t>(dstBmp, srcBmp, dstOffX, dstOffY);
        break;
    case pm::BitmapDataType::UInt32:
        FillTT<Tdst, uint32_t>(dstBmp, srcBmp, dstOffX, dstOffY);
        break;
    default:
        throw pm::Exception("Unsupported bitmap data type");
    }
}

void pm::Bitmap::Fill(const Bitmap* srcBmp, uint16_t dstOffX, uint16_t dstOffY)
{
    switch (this->m_format.GetDataType())
    {
    case BitmapDataType::UInt8:
        FillT<uint8_t>(this, srcBmp, dstOffX, dstOffY);
        break;
    case BitmapDataType::UInt16:
        FillT<uint16_t>(this, srcBmp, dstOffX, dstOffY);
        break;
    case BitmapDataType::UInt32:
        FillT<uint32_t>(this, srcBmp, dstOffX, dstOffY);
        break;
    default:
        throw Exception("Unsupported bitmap data type");
    }
}

void pm::Bitmap::Fill(const Bitmap* srcBmp)
{
    Fill(srcBmp, 0, 0);
}

template<typename Tdst>
void FillT(pm::Bitmap* const dstBmp, double val)
{
    std::fill_n(static_cast<Tdst*>(dstBmp->GetData()),
            dstBmp->GetDataBytes() / sizeof(Tdst), static_cast<Tdst>(val));
}

void pm::Bitmap::Fill(double val)
{
    assert(m_data != nullptr);
    switch (m_format.GetDataType())
    {
    case BitmapDataType::UInt8:
        FillT<uint8_t>(this, val);
        break;
    case BitmapDataType::UInt16:
        FillT<uint16_t>(this, val);
        break;
    case BitmapDataType::UInt32:
        FillT<uint32_t>(this, val);
        break;
    default:
        throw Exception("Unsupported bitmap data type");
    }
}

void pm::Bitmap::Clear()
{
    assert(m_data != nullptr);
    ::memset(m_data, 0, m_dataBytes);
}
