/******************************************************************************/
/* Copyright (C) Teledyne Photometrics. All rights reserved.                  */
/******************************************************************************/
#pragma once
#ifndef PM_BITMAP_FORMAT_H
#define PM_BITMAP_FORMAT_H

/* System */
#include <cstddef>
#include <cstdint>

namespace pm {

/**
Image format specifies the buffer format in which the pixels are transferred.
The format should be used together with bit depth because it specifies only the
format of the pixel container, not the actual bit depth of the pixel it
contains.
*/
enum class ImageFormat : int32_t
{
    Mono16 = 0, ///< 16bit mono, 2 bytes per pixel.
    Bayer16 = 1, ///< 16bit bayer masked image, 2 bytes per pixel.
    Mono8 = 2, ///< 8bit mono, 1 byte per pixel.
    Bayer8 = 3, ///< 8bit bayer masked image, 1 byte per pixel.
    //Mono24 = 4, ///< 24bit mono, 3 bytes per pixel.
    //Bayer24 = 5, ///< 24bit bayer masked image, 3 bytes per pixel.
    RGB24 = 6, ///< 8bit RGB, 1 byte per sample, 3 bytes per pixel.
    RGB48 = 7, ///< 16bit RGB, 2 bytes per sample, 6 bytes per pixel.
    //RGB72 = 8, ///< 24bit RGB, 3 bytes per sample, 9 bytes per pixel.
    Mono32 = 9, ///< 32bit mono, 4 bytes per pixel.
    Bayer32 = 10, ///< 32bit bayer masked image, 4 bytes per pixel.
    // TODO: Fix the value once defined in pvcam.h
    RGB96 = 11, ///< 32bit RGB, 4 bytes per sample, 12 bytes per pixel.
};

/**
Pixel type. A pixel may consists of several samples. For monochrome bitmap the
pixel is a simple single value, for RGB bitmaps the pixel contains 3 samples.
*/
enum class BitmapPixelType : int32_t
{
    Mono = 0, ///< Each pixel contains only one sample
    RGB, ///< Each pixel consists of 3 samples, Red, Green, Blue, in this order
};

/**
The data type of a bitmap sample. The bitmap can have a pixel type (Mono, RGB)
and that type can have different data type, e.g. Mono+UInt16, or RGB+UInt8.
*/
enum class BitmapDataType : int32_t
{
    UInt8 = 0, ///< Each sample takes 1 byte, 8-bit unsigned value
    UInt16, ///< Each sample takes 2 bytes, 16-bit unsigned value
    UInt32, ///< Each sample takes 4 bytes, 32-bit unsigned value
};

/**
Bayer pattern for the Mono bitmap format. The pattern is named with 4 letters
where the first two are first line and another two are the next line.
For example RGGB means the pixels have following mask:
@code
R G R G R G ...
G B G B G B ...
R G R G R G ...
G B G B G B ...
...
@endcode
The values correspond to PL_COLOR_MODES from PVCAM.
*/
enum class BayerPattern : int32_t
{
    None = 0, ///< No Bayer pattern, the image is plain monochrome bitmap
    RGGB = 2, ///< R-G-G-B pattern
    GRBG = 3, ///< G-R-B-G pattern
    GBRG = 4, ///< G-B-R-G pattern
    BGGR = 5, ///< B-G-G-R pattern
};

/**
A class that describes the format of a Bitmap.

Data type:         A type of the sample, usually UInt16 for 16bit camera images.
Samples-per-pixel: Technically, one pixel may consist of several samples, e.g.
                   RGB bitmaps have 3 samples per pixel, RGBA or CMYK bitmap
                   would have 4 samples per pixel. Gray-scale bitmaps have 1
                   sample per pixel.
Bits per sample:   Image bit depth, a 16-bit data type may have only 14, 10 or
                   even 8 bits valid.
                   Bits-per-sample is the actual bit depth. Data-type is the
                   sample carrier. An UInt32-type bitmap may have bit-depth of
                   12 only if it was, for example, up-converted from UInt16.

Example of a 16bit RGB frame:
          _______________________________________
SAMPLES: |  R  |  G  |  B  |  R  |  G  |  B  |...|
         |__0__|__1__|__2__|__3__|__4__|__5__|   |
PIXELS:  |        0        |        1        |   |
         |_________________|_________________|   |
BYTES:   |0 |1 |2 |3 |4 |5 |6 |7 |8 |9 |10|11|...|
*/
class BitmapFormat
{
public:
    /**
    Creates an empty, undefined bitmap data type.
    @remarks The format will be set to default values that are not guaranteed to
    not change in between library versions.
    */
    BitmapFormat();
    /**
    Creates a new bitmap format instance.
    @param imageFormat Image format.
    @param bitDepth Bitmap bit depth.
    The bitmap color mask will be defaulted to None.
    */
    BitmapFormat(ImageFormat imageFormat, uint16_t bitDepth);
    /**
    Creates a new bitmap format instance.
    @param pixelType Bitmap pixel type.
    @param dataType Bitmap data type.
    @param bitDepth Bitmap bit depth.
    The bitmap color mask will be defaulted to None.
    */
    BitmapFormat(BitmapPixelType pixelType, BitmapDataType dataType,
            uint16_t bitDepth);

    bool operator==(const BitmapFormat& other) const;
    bool operator!=(const BitmapFormat& other) const;

public:
    /**
    Returns the image format. For example Mono16 or RGB24.
    @return Image format.
    */
    ImageFormat GetImageFormat() const;
    /**
    Sets the image format.
    @param imageFormat Image format.
    */
    void SetImageFormat(ImageFormat imageFormat);

    /**
    Returns the bitmap pixel type. For example Mono or RGB.
    This value is directly related to #SamplesPerPixel(), i.e. a Mono bitmap has
    one sample per pixel, an RGB bitmap has 3 samples per pixel.
    @return Bitmap pixel type.
    */
    BitmapPixelType GetPixelType() const;
    /**
    Sets the bitmap pixel type.
    @param pixelType Bitmap pixel type.
    */
    void SetPixelType(BitmapPixelType pixelType);

    /**
    Returns the sample data type, for example, UInt8, UInt16, etc.
    @returns Bitmap sample data type.
    */
    BitmapDataType GetDataType() const;
    /**
    Sets the bitmap data type.
    @param dataType Data type.
    */
    void SetDataType(BitmapDataType dataType);

    /**
    Returns the bitmap bit-depth, or in other words number of valid bits-per-sample.
    Please note that the #BitmapDataType defines the sample 'carrier' or 'container',
    an UInt16 type bitmap may have bit-depth of 14, 12, 10, or even 8.
    However the bit depth should not be higher than is the 'container' size.
    */
    uint16_t GetBitDepth() const;
    /**
    Sets the bitmap bit depth.
    @param bitDepth Bit depth.
    */
    void SetBitDepth(uint16_t bitDepth);

    /**
    Returns the sensor, or bitmap mask used for this bitmap, for example RGGB or
    simply None. This value does not (and should not) have any effect on the
    bitmap data or its size. The value only tells the caller how to interpret
    the pixels. For example a monochromatic bitmap may have been acquired on a
    sensor with RGGB bayer mask. The bitmap configuration (and size) is no
    different from normal gray-scale bitmap but in order to properly display the
    bitmap it has to be demosaiced. The BayerPattern value is used to properly
    represent the bitmap on the screen.
    @return Bitmap color mask.
    */
    BayerPattern GetColorMask() const;
    /**
    Sets the bitmap color mask.
    @param mask Bitmap mask.
    */
    void SetColorMask(BayerPattern colorMask);

public:
    /**
    @brief Returns number of bytes required for one pixel.
    @details The bytes-per-pixel value is usually calculated as:
    @code
        bytes-per-sample x samples-per-pixel
    @endcode
    For example, an UInt16 RGB bitmap would need 2 * 3 = 6 bytes per pixel.
    Generally, size of a bitmap can be calculated as width * height * bytes per pixel.
    @return Number of bytes per pixel.
    */
    size_t GetBytesPerPixel() const;
    /**
    @brief Returns number of bytes required for one sample.
    @details The bytes-per-sample value is usually calculated as:
    @code
        sizeof(data-type)
    @endcode
    For example, an UInt16 RGB bitmap would need 2 bytes per sample.
    @returns Number of bytes per sample.
    */
    size_t GetBytesPerSample() const;
    /**
    @brief Returns number of samples per pixel.
    @details For example an RGB bitmap has 3 samples per pixel, an RGBA or CMYK
    bitmap would consists of 4 samples per pixel. Monochrome bitmaps use 1 sample
    per pixel.
    @return Number of samples per pixel.
    */
    uint8_t GetSamplesPerPixel() const;

private:
    void SetupPixelAndDataType(ImageFormat imageFormat);
    void SetupImageFormat(BitmapPixelType pixelType, BitmapDataType dataType);

private:
    ImageFormat m_imageFormat{ ImageFormat::Mono16 };
    BitmapPixelType m_pixelType{ BitmapPixelType::Mono };
    BitmapDataType m_dataType{ BitmapDataType::UInt16 };
    uint16_t m_bitDepth{ 16 };
    BayerPattern m_colorMask{ BayerPattern::None };
};

} // namespace pm

#endif /* PM_BITMAP_FORMAT_H */
