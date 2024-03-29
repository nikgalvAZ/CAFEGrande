/******************************************************************************/
/* Copyright (C) Teledyne Photometrics. All rights reserved.                  */
/******************************************************************************/
#include "backend/BitmapFormat.h"

/* Local */
#include "backend/exceptions/Exception.h"

/* System */
#include <type_traits> // underlying_type

pm::BitmapFormat::BitmapFormat()
{
}

pm::BitmapFormat::BitmapFormat(ImageFormat imageFormat, uint16_t bitDepth)
    : m_imageFormat(imageFormat),
    m_bitDepth(bitDepth)
{
    SetupPixelAndDataType(imageFormat);
}

pm::BitmapFormat::BitmapFormat(BitmapPixelType pixelType, BitmapDataType dataType,
        uint16_t bitDepth)
    : m_pixelType(pixelType),
    m_dataType(dataType),
    m_bitDepth(bitDepth)
{
    SetupImageFormat(pixelType, dataType);
}

bool pm::BitmapFormat::operator==(const BitmapFormat& other) const
{
    return m_bitDepth == other.m_bitDepth
        && m_imageFormat == other.m_imageFormat // Covers m_dataType & m_pixelType
        && m_colorMask == other.m_colorMask;
}

bool pm::BitmapFormat::operator!=(const BitmapFormat& other) const
{
    return !(*this == other);
}

pm::ImageFormat pm::BitmapFormat::GetImageFormat() const
{
    return m_imageFormat;
}

void pm::BitmapFormat::SetImageFormat(ImageFormat imageFormat)
{
    SetupPixelAndDataType(imageFormat);
    m_imageFormat = imageFormat;
}

pm::BitmapPixelType pm::BitmapFormat::GetPixelType() const
{
    return m_pixelType;
}

void pm::BitmapFormat::SetPixelType(BitmapPixelType pixelType)
{
    SetupImageFormat(pixelType, m_dataType);
    m_pixelType = pixelType;
}

pm::BitmapDataType pm::BitmapFormat::GetDataType() const
{
    return m_dataType;
}

void pm::BitmapFormat::SetDataType(BitmapDataType dataType)
{
    SetupImageFormat(m_pixelType, dataType);
    m_dataType = dataType;
}

uint16_t pm::BitmapFormat::GetBitDepth() const
{
    return m_bitDepth;
}

void pm::BitmapFormat::SetBitDepth(uint16_t bitDepth)
{
    m_bitDepth = bitDepth;
}

pm::BayerPattern pm::BitmapFormat::GetColorMask() const
{
    return m_colorMask;
}

void pm::BitmapFormat::SetColorMask(BayerPattern colorMask)
{
    m_colorMask = colorMask;
}

size_t pm::BitmapFormat::GetBytesPerPixel() const
{
    const uint8_t samplesPerPixel = GetSamplesPerPixel();
    const size_t bytesPerSample = GetBytesPerSample();
    return samplesPerPixel * bytesPerSample;
}

size_t pm::BitmapFormat::GetBytesPerSample() const
{
    switch (m_dataType)
    {
    case pm::BitmapDataType::UInt8:
        return sizeof(uint8_t);
    case pm::BitmapDataType::UInt16:
        return sizeof(uint16_t);
    case pm::BitmapDataType::UInt32:
        return sizeof(uint32_t);
    }
    return 0; // Never happens
}

uint8_t pm::BitmapFormat::GetSamplesPerPixel() const
{
    switch (m_pixelType)
    {
    case pm::BitmapPixelType::Mono:
        return 1;
    case pm::BitmapPixelType::RGB:
        return 3;
    }
    return 0; // Never happens
}

void pm::BitmapFormat::SetupPixelAndDataType(ImageFormat imageFormat)
{
    switch (imageFormat)
    {
    case ImageFormat::Mono8:
    case ImageFormat::Bayer8:
        m_dataType = BitmapDataType::UInt8;
        m_pixelType = BitmapPixelType::Mono;
        return;
    case ImageFormat::Mono16:
    case ImageFormat::Bayer16:
        m_dataType = BitmapDataType::UInt16;
        m_pixelType = BitmapPixelType::Mono;
        return;
    case ImageFormat::Mono32:
    case ImageFormat::Bayer32:
        m_dataType = BitmapDataType::UInt32;
        m_pixelType = BitmapPixelType::Mono;
        return;
    case ImageFormat::RGB24:
        m_dataType = BitmapDataType::UInt8;
        m_pixelType = BitmapPixelType::RGB;
        return;
    case ImageFormat::RGB48:
        m_dataType = BitmapDataType::UInt16;
        m_pixelType = BitmapPixelType::RGB;
        return;
    case ImageFormat::RGB96:
        m_dataType = BitmapDataType::UInt32;
        m_pixelType = BitmapPixelType::RGB;
        return;
    }
    throw Exception("Unsupported image format " + std::to_string(
            static_cast<std::underlying_type<ImageFormat>::type>(imageFormat)));
}

void pm::BitmapFormat::SetupImageFormat(BitmapPixelType pixelType,
        BitmapDataType dataType)
{
    switch (dataType)
    {
    case pm::BitmapDataType::UInt8:
        switch (pixelType)
        {
        case pm::BitmapPixelType::Mono:
            m_imageFormat = ImageFormat::Mono8;
            return;
        case pm::BitmapPixelType::RGB:
            m_imageFormat = ImageFormat::RGB24;
            return;
        }
        break; // Throws below

    case pm::BitmapDataType::UInt16:
        switch (pixelType)
        {
        case pm::BitmapPixelType::Mono:
            m_imageFormat = ImageFormat::Mono16;
            return;
        case pm::BitmapPixelType::RGB:
            m_imageFormat = ImageFormat::RGB48;
            return;
        }
        break; // Throws below

    case pm::BitmapDataType::UInt32:
        switch (pixelType)
        {
        case pm::BitmapPixelType::Mono:
            m_imageFormat = ImageFormat::Mono32;
            return;
        case pm::BitmapPixelType::RGB:
            m_imageFormat = ImageFormat::RGB96;
            return;
        }
        break; // Throws below

    default:
        throw Exception("Unsupported bitmap data type " + std::to_string(
                static_cast<std::underlying_type<BitmapDataType>::type>(dataType)));
    }
    throw Exception("Unsupported pixel data type " + std::to_string(
            static_cast<std::underlying_type<BitmapPixelType>::type>(pixelType)));
}
