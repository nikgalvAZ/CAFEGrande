/******************************************************************************/
/* Copyright (C) Teledyne Photometrics. All rights reserved.                  */
/******************************************************************************/
#include "backend/TaskSet_FillBitmap.h"

/* Local */
#include "backend/Bitmap.h"
#include "backend/exceptions/Exception.h"

/* System */
#include <cassert>
#include <cstring> // memcpy

// TaskSet_FillBitmap::Task

pm::TaskSet_FillBitmap::ATask::ATask(
        std::shared_ptr<Semaphore> semDone, size_t taskIndex, size_t taskCount)
    : pm::Task(semDone, taskIndex, taskCount),
    m_maxTasks(taskCount)
{
}

void pm::TaskSet_FillBitmap::ATask::SetUp(Bitmap* const dstBmp,
        const Bitmap* srcBmp, uint16_t srcOffX, uint16_t srcOffY)
{
    assert(dstBmp != nullptr);
    assert(srcBmp != nullptr);

    // The images must have the same dimensions and must be the same pixel type.
    // E.g. we cannot copy from RGB to MONO frames
    if (dstBmp->GetFormat().GetPixelType() != srcBmp->GetFormat().GetPixelType())
        throw pm::Exception("Cannot process bitmaps with different pixel types");
    if (srcBmp->GetWidth() + srcOffX > dstBmp->GetWidth()
            || srcBmp->GetHeight() + srcOffY > dstBmp->GetHeight())
        throw pm::Exception(
                "Cannot process bitmaps, source doesn't fit the destination with given offset");

    const auto srcHeight = srcBmp->GetHeight();
    const auto taskCount = GetTaskCount();
    m_maxTasks = (srcHeight < taskCount) ? srcHeight : taskCount;

    m_dstBmp = const_cast<Bitmap*>(dstBmp);
    m_srcBmp = const_cast<Bitmap*>(srcBmp);
    m_srcOffX = srcOffX;
    m_srcOffY = srcOffY;
}

void pm::TaskSet_FillBitmap::ATask::Execute()
{
    if (GetTaskIndex() >= m_maxTasks)
        return;

    switch (m_dstBmp->GetFormat().GetDataType())
    {
    case BitmapDataType::UInt8:
        ExecuteT<uint8_t>();
        break;
    case BitmapDataType::UInt16:
        ExecuteT<uint16_t>();
        break;
    case BitmapDataType::UInt32:
        ExecuteT<uint32_t>();
        break;
    default:
        throw Exception("Unsupported destination bitmap data type");
    }
}

template<typename Tdst>
void pm::TaskSet_FillBitmap::ATask::ExecuteT()
{
    switch (m_srcBmp->GetFormat().GetDataType())
    {
    case BitmapDataType::UInt8:
        ExecuteTT<Tdst, uint8_t>();
        break;
    case BitmapDataType::UInt16:
        ExecuteTT<Tdst, uint16_t>();
        break;
    case BitmapDataType::UInt32:
        ExecuteTT<Tdst, uint32_t>();
        break;
    default:
        throw Exception("Unsupported source bitmap data type");
    }
}

template<typename Tdst, typename Tsrc>
void pm::TaskSet_FillBitmap::ATask::ExecuteTT()
{
    const auto step = (uint32_t)m_maxTasks;
    const auto offset = (uint32_t)GetTaskIndex();

    const auto w = m_srcBmp->GetWidth();
    const auto h = m_srcBmp->GetHeight();

    if (m_dstBmp->GetFormat().GetBytesPerPixel() != m_srcBmp->GetFormat().GetBytesPerPixel())
    {
        const auto spp = m_dstBmp->GetFormat().GetSamplesPerPixel();
        const uint32_t sprl = spp * w; // Samples per rect line
        const uint32_t dstSpoX = spp * m_srcOffX; // Samples per rect horiz. offset
        for (uint32_t y = offset; y < h; y += step)
        {
            Tdst* const dstLine = static_cast<Tdst* const>(
                    m_dstBmp->GetScanLine((uint16_t)y + m_srcOffY));
            const Tsrc* srcLine = static_cast<const Tsrc*>(
                    m_srcBmp->GetScanLine((uint16_t)y));
            // Do sample by sample as Tsrc and Tdst types have different size
            for (uint32_t x = 0; x < sprl; ++x)
            {
                dstLine[x + dstSpoX] = static_cast<Tdst>(srcLine[x]);
            }
        }
    }
    else
    {
        const auto bpp = m_dstBmp->GetFormat().GetBytesPerPixel();
        const size_t bprl = bpp * w; // Bytes per rect line
        const size_t dstBpoX = bpp * m_srcOffX; // Bytes per rect horiz. offset
        for (uint32_t y = offset; y < h; y += step)
        {
            uint8_t* const dstLine = static_cast<uint8_t* const>(
                    m_dstBmp->GetScanLine((uint16_t)y + m_srcOffY));
            const uint8_t* srcLine = static_cast<const uint8_t*>(
                    m_srcBmp->GetScanLine((uint16_t)y));
            ::memcpy(dstLine + dstBpoX, srcLine, bprl);
        }
    }
}

// TaskSet_FillBitmap

pm::TaskSet_FillBitmap::TaskSet_FillBitmap(std::shared_ptr<ThreadPool> pool)
    : TaskSet(pool)
{
    CreateTasks<ATask>();
}

void pm::TaskSet_FillBitmap::SetUp(Bitmap* const dstBmp, const Bitmap* srcBmp,
        uint16_t srcOffX, uint16_t srcOffY)
{
    const auto& tasks = GetTasks();
    for (auto task : tasks)
    {
        static_cast<ATask*>(task)->SetUp(dstBmp, srcBmp, srcOffX, srcOffY);
    }
}
