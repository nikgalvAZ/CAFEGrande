/******************************************************************************/
/* Copyright (C) Teledyne Photometrics. All rights reserved.                  */
/******************************************************************************/
#pragma once
#ifndef PM_BITMAP_H
#define PM_BITMAP_H

/* Local */
#include "backend/BitmapFormat.h"

namespace pm {

/**
This class represents the most simple bitmap representation in memory.
The bitmap simply has a width, height and pixel format. No other metadata.
*/
class Bitmap final
{
public:
    /**
    Creates a bitmap that uses existing data. The data will not be owned
    by the new bitmap and must be deleted by the creator of the Bitmap.
    @param data A pointer to existing buffer. The buffer must have size defined
    by width, height and format.
    @param w Width of the new bitmap
    @param h Height of the new bitmap
    @param f Format of the new bitmap
    @param lineAlign Alignment of scan line width in bytes. For example 4 for
        32bit alignment.
    */
    Bitmap(void* data, uint32_t w, uint32_t h, const BitmapFormat& f,
            uint16_t lineAlign = 1);
    /**
    Creates a new bitmap that will allocate its own buffer based on bitmap format.
    @param w Width of the new bitmap
    @param h Height of the new bitmap
    @param f Format of the new bitmap
    @param lineAlign Alignment of scan line width in bytes. For example 4 for
        32bit alignment.
    @throw std::bad_alloc Throws if the memory buffer cannot be allocated
    */
    Bitmap(uint32_t w, uint32_t h, const BitmapFormat& f, uint16_t lineAlign = 1);
    /**
    Deletes the bitmap instance. Deletes the buffer if the instance owns it.
    */
    ~Bitmap();

public:
    /**
    Calculates the bitmap line width in bytes based on bitmap format, width and
    scan line alignment.
    @param w Bitmap width
    @param f Bitmap format
    @param lineAlign Desired scan line byte alignment
    */
    static size_t CalculateStrideBytes(uint32_t w, const pm::BitmapFormat& f,
            uint16_t lineAlign = 1);

    /**
    Calculates the bitmap data size in bytes based on bitmap format, width,
    height and scan line alignment.
    @param w Bitmap width
    @param h Bitmap width
    @param f Bitmap format
    @param lineAlign Desired scan line byte alignment
    */
    static size_t CalculateDataBytes(uint32_t w, uint32_t h,
            const pm::BitmapFormat& f, uint16_t lineAlign = 1);

public:
    /**
    Returns direct pointer to bitmap data.
    @return Bitmap data pointer.
    */
    void* GetData() const;
    /**
    Returns size in bytes of the bitmap.
    @returns Bitmap size in bytes.
    */
    size_t GetDataBytes() const;

    /**
    Returns width of the bitmap in number of pixels.
    @return Bitmap width.
    */
    uint32_t GetWidth() const;
    /**
    Returns height of the bitmap in number of pixels.
    @return Bitmap height.
    */
    uint32_t GetHeight() const;

    /**
    Returns format of the bitmap
    @return Bitmap format.
    */
    const BitmapFormat& GetFormat() const;
    /**
    Overrides bayer pattern in current bitmap format.
    It doesn't change memory layout and buffer size so prevents useless
    reallocation of whole Bitmap object.
    @param colorMask New bayer pattern.
    */
    void ChangeColorMask(BayerPattern colorMask);

    /**
    Returns bitmap's scan line alignment in bytes.
    @return Scan line alignment.
    */
    uint16_t GetLineAlign() const;
    /**
    Returns number of bytes per aligned scan line.
    @return Full line width in bytes.
    */
    size_t GetStride() const;

    /**
    Returns a pointer to a given scan line.
    Some bitmaps may have the scan line aligned to a particular number of bytes,
    for example the 'stride' may be aligned to 32-bits (4 bytes).
    Use this function instead of calculating the pointer manually.
    @param y Row index, this must be a number from 0 to #GetHeight() - 1.
    @return A pointer to the scan line bytes.
    */
    void* GetScanLine(uint16_t y) const;

    /**
    Retrieves a value of a pixel at specific location
    @param x X pixel position
    @param y Y pixel position
    @param sIdx Index of the sample to retrieve (For Mono frames it's 0 always)
    @return Sample value in double type
    */
    double GetSample(uint16_t x, uint16_t y, uint8_t sIdx = 0) const;

    /**
    Creates a deep copy of the bitmap.
    @return Pointer to a new bitmap object. Caller becomes owner of the object.
    @throw std::bad_alloc Throws if the memory buffer cannot be allocated
    */
    Bitmap* Clone() const;

    /**
    Performs per-pixel FILL operation, i.e. A[i+off] = B[i]
    @param srcBmp Source Bitmap which pixels will be used to replace current bitmap
    @param dstOffX Column offset in destination bitmap
    @param dstOffY Row offset in destination bitmap
    */
    void Fill(const Bitmap* srcBmp, uint16_t dstOffX, uint16_t dstOffY);
    /**
    Performs per-pixel FILL operation like function Fill(srcBmp, 0, 0)
    @param srcBmp Source Bitmap which pixels will be used to replace current bitmap
    */
    void Fill(const Bitmap* srcBmp);

    /**
    Performs per-pixel FILL operation with value, i.e. A[i] = val
    @param val Value that will be used to fill every pixel of the current bitmap
    */
    void Fill(double val);

    /**
    Clears the bitmap, i.e. sets all pixels to 0
    */
    void Clear();

private:
    uint8_t* m_data{ nullptr }; ///< A pointer to the raw bitmap data
    size_t m_dataBytes{ 0 }; ///< Total data size in bytes
    const uint32_t m_width{ 0 }; ///< Bitmap width
    const uint32_t m_height{ 0 }; ///< Bitmap height
    // Format not const just to allow override of color mask
    BitmapFormat m_format{}; ///< Bitmap format (pixel type, data type, ...)
    const uint16_t m_lineAlign{ 1 }; ///< Stride alignment in bytes (1 = no alignment)
    const bool m_deleteData{ false }; ///< If true the buffer will be deleted by destructor
    const size_t m_stride{ 0 }; ///< Line width in bytes, including padding (if applied)
};

} // namespace pm

#endif /* PM_BITMAP_H */
