/******************************************************************************/
/* Copyright (C) Teledyne Photometrics. All rights reserved.                  */
/******************************************************************************/
#include "TaskSet_ConvertToRgb8.h"

/* Local */
#include "backend/Bitmap.h"
#include "backend/exceptions/Exception.h"
#include "backend/Utils.h"

/* System */
#include <algorithm>
#include <cassert>

// TaskSet_ConvertToRgb8::Task

pm::TaskSet_ConvertToRgb8::ATask::ATask(
        std::shared_ptr<Semaphore> semDone, size_t taskIndex, size_t taskCount)
    : pm::Task(semDone, taskIndex, taskCount),
    m_maxTasks(taskCount)
{
}

void pm::TaskSet_ConvertToRgb8::ATask::SetUp(const Bitmap* dstBmp,
        const Bitmap* srcBmp, double srcMin, double srcMax, bool autoConbright,
        int brightness, int contrast, const std::vector<uint8_t>* pixLookupMap)
{
    const size_t rows = srcBmp->GetHeight();
    const size_t tasks = GetTaskCount();

    m_maxTasks = std::min(rows, tasks);

    m_dstBmp = const_cast<Bitmap*>(dstBmp);
    m_srcBmp = const_cast<Bitmap*>(srcBmp);
    m_srcMin = srcMin;
    m_srcMax = srcMax;
    m_autoConbright = autoConbright;
    m_brightness = brightness;
    m_contrast = contrast;
    m_pixLookupMap = const_cast<std::vector<uint8_t>*>(pixLookupMap);
}

void pm::TaskSet_ConvertToRgb8::ATask::Execute()
{
    assert(m_dstBmp != nullptr);
    assert(m_srcBmp != nullptr);
    assert(m_pixLookupMap != nullptr);

    if (GetTaskIndex() >= m_maxTasks)
        return;

    switch (m_srcBmp->GetFormat().GetDataType())
    {
    case BitmapDataType::UInt8:
        if (!m_pixLookupMap->empty())
            ExecuteT_Lookup<uint8_t>();
        else
            ExecuteT<uint8_t>();
        break;
    case BitmapDataType::UInt16:
        if (!m_pixLookupMap->empty())
            ExecuteT_Lookup<uint16_t>();
        else
            ExecuteT<uint16_t>();
        break;
    case BitmapDataType::UInt32:
        ExecuteT<uint32_t>();
        break;
    default:
        throw Exception("Unsupported bitmap data type");
    }
}

template<typename T>
void pm::TaskSet_ConvertToRgb8::ATask::ExecuteT()
{
    const uint32_t lineStep = static_cast<uint32_t>(m_maxTasks);
    const uint32_t lineOff = static_cast<uint32_t>(GetTaskIndex());

    const uint32_t h = m_srcBmp->GetHeight();
    const uint32_t w = m_srcBmp->GetWidth();

    const double mp = (m_srcMax == m_srcMin) ? 255.0 : 255.0 / (m_srcMax - m_srcMin);

    const T srcMin = static_cast<T>(m_srcMin);
    const T srcMax = static_cast<T>(m_srcMax);

    const auto srcSpp = m_srcBmp->GetFormat().GetSamplesPerPixel();
    const auto dstSpp = m_dstBmp->GetFormat().GetSamplesPerPixel();
    assert(dstSpp == 3);

    // brightness value is from -255 to +255
    // contrast value is from -255 to +255
    // factors for contrasts [-255, 0, +255] are [0, 1, 129.5]
    const double factor =
        (259.0 * (m_contrast + 255))
        / (255.0 * (259 - m_contrast));

    switch (m_srcBmp->GetFormat().GetPixelType())
    {
    case BitmapPixelType::Mono:
        assert(srcSpp == 1);
        if (m_autoConbright)
        {
            for (uint32_t y = lineOff; y < h; y += lineStep)
            {
                const T* const srcLine =
                    static_cast<const T*>(m_srcBmp->GetScanLine((uint16_t)y));
                uint8_t* const dstLine =
                    static_cast<uint8_t*>(m_dstBmp->GetScanLine((uint16_t)y));
                for (uint32_t x = 0; x < w; ++x)
                {
                    const uint32_t srcPixOff = srcSpp * x;
                    const uint32_t dstPixOff = dstSpp * x;
                    const T pix = Utils::Clamp(srcLine[srcPixOff], srcMin, srcMax);
                    const auto pix8 = static_cast<uint8_t>(mp * (pix - srcMin));
                    dstLine[dstPixOff + 0] = pix8;
                    dstLine[dstPixOff + 1] = pix8;
                    dstLine[dstPixOff + 2] = pix8;
                }
            }
        }
        else
        {
            for (uint32_t y = lineOff; y < h; y += lineStep)
            {
                const T* const srcLine =
                    static_cast<const T*>(m_srcBmp->GetScanLine((uint16_t)y));
                uint8_t* const dstLine =
                    static_cast<uint8_t*>(m_dstBmp->GetScanLine((uint16_t)y));
                for (uint32_t x = 0; x < w; ++x)
                {
                    const uint32_t srcPixOff = srcSpp * x;
                    const uint32_t dstPixOff = dstSpp * x;
                    const double pixF = mp * (srcLine[srcPixOff] - srcMin);
                    const auto pixI = static_cast<int>(
                            m_brightness + factor * (pixF - 128) + 128);
                    // Truncate the pixel value to range [0, 255]
                    const auto pix8 = static_cast<uint8_t>(Utils::Clamp(pixI, 0, 255));
                    dstLine[dstPixOff + 0] = pix8;
                    dstLine[dstPixOff + 1] = pix8;
                    dstLine[dstPixOff + 2] = pix8;
                }
            }
        }
        break;

    case BitmapPixelType::RGB:
        assert(srcSpp == 3);
        if (m_autoConbright)
        {
            for (uint32_t y = lineOff; y < h; y += lineStep)
            {
                const T* const srcLine =
                    static_cast<const T*>(m_srcBmp->GetScanLine((uint16_t)y));
                uint8_t* const dstLine =
                    static_cast<uint8_t*>(m_dstBmp->GetScanLine((uint16_t)y));
                for (uint32_t x = 0; x < w; ++x)
                {
                    const uint32_t srcPixOff = srcSpp * x;
                    const uint32_t dstPixOff = dstSpp * x;
                    const T pixR = Utils::Clamp(srcLine[srcPixOff + 0], srcMin, srcMax);
                    const T pixG = Utils::Clamp(srcLine[srcPixOff + 1], srcMin, srcMax);
                    const T pixB = Utils::Clamp(srcLine[srcPixOff + 2], srcMin, srcMax);
                    dstLine[dstPixOff + 0] = static_cast<uint8_t>(mp * (pixR - srcMin));
                    dstLine[dstPixOff + 1] = static_cast<uint8_t>(mp * (pixG - srcMin));
                    dstLine[dstPixOff + 2] = static_cast<uint8_t>(mp * (pixB - srcMin));
                }
            }
        }
        else
        {
            for (uint32_t y = lineOff; y < h; y += lineStep)
            {
                const T* const srcLine =
                    static_cast<const T*>(m_srcBmp->GetScanLine((uint16_t)y));
                uint8_t* const dstLine =
                    static_cast<uint8_t*>(m_dstBmp->GetScanLine((uint16_t)y));
                for (uint32_t x = 0; x < w; ++x)
                {
                    const uint32_t srcPixOff = srcSpp * x;
                    const uint32_t dstPixOff = dstSpp * x;
                    const double pixFR = mp * (srcLine[srcPixOff + 0] - srcMin);
                    const double pixFG = mp * (srcLine[srcPixOff + 1] - srcMin);
                    const double pixFB = mp * (srcLine[srcPixOff + 2] - srcMin);
                    const auto pixIR = static_cast<int>(
                            m_brightness + factor * (pixFR - 128) + 128);
                    const auto pixIG = static_cast<int>(
                            m_brightness + factor * (pixFG - 128) + 128);
                    const auto pixIB = static_cast<int>(
                            m_brightness + factor * (pixFB - 128) + 128);
                    // Truncate the pixel value to range [0, 255]
                    dstLine[dstPixOff + 0] = static_cast<uint8_t>(Utils::Clamp(pixIR, 0, 255));
                    dstLine[dstPixOff + 1] = static_cast<uint8_t>(Utils::Clamp(pixIG, 0, 255));
                    dstLine[dstPixOff + 2] = static_cast<uint8_t>(Utils::Clamp(pixIB, 0, 255));
                }
            }
        }
        break;
    }
}

template<typename T>
void pm::TaskSet_ConvertToRgb8::ATask::ExecuteT_Lookup()
{
    const uint32_t lineStep = static_cast<uint32_t>(m_maxTasks);
    const uint32_t lineOff = static_cast<uint32_t>(GetTaskIndex());

    const uint32_t h = m_srcBmp->GetHeight();
    const uint32_t w = m_srcBmp->GetWidth();

    const auto srcSpp = m_srcBmp->GetFormat().GetSamplesPerPixel();
    const auto dstSpp = m_dstBmp->GetFormat().GetSamplesPerPixel();
    assert(dstSpp == 3);

    const uint8_t* lookupMapData = m_pixLookupMap->data();

    switch (m_srcBmp->GetFormat().GetPixelType())
    {
    case BitmapPixelType::Mono:
        for (uint32_t y = lineOff; y < h; y += lineStep)
        {
            const T* const srcLine =
                static_cast<const T*>(m_srcBmp->GetScanLine((uint16_t)y));
            uint8_t* const dstLine =
                static_cast<uint8_t*>(m_dstBmp->GetScanLine((uint16_t)y));
            for (uint32_t x = 0; x < w; ++x)
            {
                const uint32_t srcPixOff = srcSpp * x;
                const uint32_t dstPixOff = dstSpp * x;
                const T pix = srcLine[srcPixOff];
                const uint8_t pix8 = lookupMapData[pix];
                dstLine[dstPixOff + 0] = pix8;
                dstLine[dstPixOff + 1] = pix8;
                dstLine[dstPixOff + 2] = pix8;
            }
        }
        break;

    case BitmapPixelType::RGB:
        for (uint32_t y = lineOff; y < h; y += lineStep)
        {
            const T* const srcLine =
                static_cast<const T*>(m_srcBmp->GetScanLine((uint16_t)y));
            uint8_t* const dstLine =
                static_cast<uint8_t*>(m_dstBmp->GetScanLine((uint16_t)y));
            for (uint32_t x = 0; x < w; ++x)
            {
                const uint32_t srcPixOff = srcSpp * x;
                const uint32_t dstPixOff = dstSpp * x;
                const T pixR = srcLine[srcPixOff + 0];
                const T pixG = srcLine[srcPixOff + 1];
                const T pixB = srcLine[srcPixOff + 2];
                dstLine[dstPixOff + 0] = lookupMapData[pixR];
                dstLine[dstPixOff + 1] = lookupMapData[pixG];
                dstLine[dstPixOff + 2] = lookupMapData[pixB];
            }
        }
        break;
    }
}

// TaskSet_ConvertToRgb8

pm::TaskSet_ConvertToRgb8::TaskSet_ConvertToRgb8(
        std::shared_ptr<ThreadPool> pool)
    : TaskSet(pool)
{
    CreateTasks<ATask>();
}

void pm::TaskSet_ConvertToRgb8::SetUp(const Bitmap* dstBmp,
        const Bitmap* srcBmp, double srcMin, double srcMax,
        const std::vector<uint8_t>* pixLookupMap,
        bool autoConbright, int brightness, int contrast)
{
    static const std::vector<uint8_t> emptyLookupMap{};

    const std::vector<uint8_t>* lookupMap =
        (pixLookupMap) ? pixLookupMap : &emptyLookupMap;

    const auto& tasks = GetTasks();
    for (auto task : tasks)
    {
        static_cast<ATask*>(task)->SetUp(dstBmp, srcBmp, srcMin, srcMax,
                autoConbright, brightness, contrast, lookupMap);
    }
}

uint8_t pm::TaskSet_ConvertToRgb8::ConvertOnePixel(double srcValue, double srcMin,
        double srcMax, bool autoConbright, int brightness, int contrast)
{
    const double mp = (srcMax == srcMin) ? 255.0 : 255.0 / (srcMax - srcMin);

    if (autoConbright)
    {
        const auto pixF = Utils::Clamp(srcValue, srcMin, srcMax);
        const auto pix8 = static_cast<uint8_t>(mp * (pixF - srcMin));
        return pix8;
    }
    else
    {
        // brightness value is from -255 to +255
        // contrast value is from -255 to +255
        // factors for contrasts [-255, 0, +255] are [0, 1, 129.5]
        const double factor =
            (259.0 * (contrast + 255))
            / (255.0 * (259 - contrast));

        const double pixF = mp * (srcValue - srcMin);
        const auto pixI = static_cast<int>(brightness + factor * (pixF - 128) + 128);
        // Truncate the pixel value to range [0, 255]
        const auto pix8 = static_cast<uint8_t>(Utils::Clamp(pixI, 0, 255));
        return pix8;
    }
}

void pm::TaskSet_ConvertToRgb8::UpdateLookupMap(std::vector<uint8_t>& lookupMap,
        const BitmapFormat& srcBmpFormat, double srcMin, double srcMax,
        bool autoConbright, int brightness, int contrast)
{
    assert(brightness >= -255 && brightness <= +255);
    assert(contrast >= -255 && contrast <= +255);

    const auto bitDepth = srcBmpFormat.GetBitDepth();

    uint32_t mapSize = 0;
    switch (srcBmpFormat.GetDataType())
    {
    case pm::BitmapDataType::UInt8:
        mapSize = 256;
        assert(bitDepth <= 8);
        break; // OK, keep going
    case pm::BitmapDataType::UInt16:
        mapSize = 65536;
        assert(bitDepth <= 16);
        break; // OK, keep going
    default:
        lookupMap.clear();
        return;
    }

    const uint16_t maxPixelValue =
        static_cast<uint16_t>(((uint32_t)1 << bitDepth) - 1);
    const auto min = static_cast<uint16_t>(srcMin);
    const auto max = static_cast<uint16_t>(srcMax);
    assert(min <= max);

    lookupMap.resize(mapSize);

    // Calculate the conversion factor from N bits to 8 bits.
    // The min / max values are either real values taken from the image
    // (for auto adjustment) or theoretical minimum and maximum values
    // (for manual adjustment).
    const double mp = (max == min) ? 255.0 : 255.0 / (double)(max - min);

    if (autoConbright)
    {
        // [0] -> 0
        // [min] -> 0
        // [pixel] -> 8bit
        // [max] -> 255
        // [maxPixelValue] -> 255
        for (uint32_t n = 0; n <= min; ++n)
        {
            lookupMap[n] = 0;
        }
        if (min == max)
        {
            for (uint32_t n = min + 1; n <= maxPixelValue; ++n)
            {
                lookupMap[n] = 255;
            }
        }
        else
        {
            for (uint32_t n = min + 1; n < max; ++n)
            {
                const double pixF = mp * (n - min);
                lookupMap[n] = static_cast<uint8_t>(pixF);
            }
            for (uint32_t n = max; n <= maxPixelValue; ++n)
            {
                lookupMap[n] = 255;
            }
        }
    }
    else
    {
        // brightness value is from -255 to +255
        // contrast value is from -255 to +255
        // factors for contrasts [-255, 0, +255] are [0, 1, 129.5]
        const double factor =
            (259.0 * (contrast + 255))
            / (255.0 * (259 - contrast));
        if (min == max)
        {
            const int pixI = static_cast<int>(brightness + factor * 127 + 128);
            // Truncate the pixel value to range [0, 255]
            const auto pix8 = static_cast<uint8_t>(Utils::Clamp(pixI, 0, 255));
            for (uint32_t n = 0; n <= maxPixelValue; ++n)
            {
                lookupMap[n] = pix8;
            }
        }
        else
        {
            for (uint32_t n = 0; n <= maxPixelValue; ++n)
            {
                const double pixF = mp * (n - min);
                const int pixI = static_cast<int>(
                        brightness + factor * (pixF - 128) + 128);
                // Truncate the pixel value to range [0, 255]
                lookupMap[n] = static_cast<uint8_t>(Utils::Clamp(pixI, 0, 255));
            }
        }
    }

    // Just in case, add also values for remaining indexes up to max. data size
    for (uint32_t n = maxPixelValue + 1; n < mapSize; ++n)
    {
        lookupMap[n] = 255;
    }
}
