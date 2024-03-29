/******************************************************************************/
/* Copyright (C) Teledyne Photometrics. All rights reserved.                  */
/******************************************************************************/
#include "backend/FrameProcessor.h"

/* Local */
#include "backend/Bitmap.h"
#include "backend/ColorRuntimeLoader.h"
#include "backend/ColorUtils.h"
#include "backend/exceptions/Exception.h"
#include "backend/UniqueThreadPool.h"

/* System */
#include <algorithm>

pm::FrameProcessor::FrameProcessor()
{
}

pm::FrameProcessor::~FrameProcessor()
{
    Invalidate();
}

void pm::FrameProcessor::Invalidate()
{
    m_frame.reset();

    m_validRoiCount = 0;

    m_debayeredBitmaps.clear();
    m_rgb8bitBitmaps.clear();

    m_stats.Clear();
    m_roiStats.clear();
}

void pm::FrameProcessor::SetFrame(std::shared_ptr<Frame> frame)
{
    if (!frame || !frame->IsValid())
    {
        Invalidate();
        return;
    }

    if (!m_frame || m_frame->GetAcqCfg() != frame->GetAcqCfg())
    {
        Reconfigure(*frame);
    }

    m_frame = frame;

    m_validRoiCount = m_frame->GetRoiBitmapValidCount();
}

const std::vector<rgn_type>& pm::FrameProcessor::GetBitmapRegions() const
{
    static const auto empty = std::vector<rgn_type>();
    if (m_frame)
        return m_frame->GetRoiBitmapRegions();
    else
        return empty;
}

const std::vector<pm::Frame::Point>& pm::FrameProcessor::GetBitmapPositions() const
{
    static const auto empty = std::vector<Frame::Point>();
    if (m_frame)
        return m_frame->GetRoiBitmapPositions();
    else
        return empty;
}

size_t pm::FrameProcessor::GetValidBitmapCount() const
{
    return m_validRoiCount;
}

const std::vector<std::unique_ptr<pm::Bitmap>>&
    pm::FrameProcessor::GetBitmaps(UseBmp useBmp) const
{
    switch (useBmp)
    {
    case UseBmp::Raw:
        return GetRawBitmaps();
    case UseBmp::Debayered:
        return GetDebayeredBitmaps();
    case UseBmp::Rgb8bit:
        return GetRgb8bitBitmaps();
    }
    throw Exception("Unknown UseBmp value");
}

const std::vector<std::unique_ptr<pm::Bitmap>>&
    pm::FrameProcessor::GetRawBitmaps() const
{
    static const auto empty = std::vector<std::unique_ptr<Bitmap>>();
    if (m_frame)
        return m_frame->GetRoiBitmaps();
    else
        return empty;
}

void pm::FrameProcessor::Debayer(const ph_color_context* colorCtx)
{
    if (!m_frame || !m_frame->IsValid())
        return;

    const size_t count = m_validRoiCount;
    for (uint16_t roiIdx = 0; roiIdx < count; ++roiIdx)
    {
        DoDebayerRoi(roiIdx, colorCtx);
    }
}

void pm::FrameProcessor::DebayerRoi(uint16_t roiIdx,
        const ph_color_context* colorCtx)
{
    if (!m_frame || !m_frame->IsValid())
        return;

    DoDebayerRoi(roiIdx, colorCtx);
}

const std::vector<std::unique_ptr<pm::Bitmap>>&
    pm::FrameProcessor::GetDebayeredBitmaps() const
{
    return m_debayeredBitmaps;
}

void pm::FrameProcessor::CovertToRgb8bit(UseBmp useBmp,
        double min, double max, bool autoConbright, int brightness, int contrast)
{
    if (!m_frame || !m_frame->IsValid())
        return;

    const size_t count = m_validRoiCount;
    if (count == 0)
        return;

    auto& srcBitmaps = GetBitmaps(useBmp);
    auto& srcBmp = srcBitmaps[0];
    TaskSet_ConvertToRgb8::UpdateLookupMap(m_convToRgb8bitLookupMap,
            srcBmp->GetFormat(), min, max, autoConbright, brightness, contrast);

    for (uint16_t roiIdx = 0; roiIdx < count; ++roiIdx)
    {
        DoConvertRoiToRgb8bit(roiIdx, useBmp,
                min, max, autoConbright, brightness, contrast);
    }

    for (uint16_t roiIdx = 0; roiIdx < count; ++roiIdx)
    {
        if (!m_tasksConvToRgb8Active[roiIdx])
            continue;
        m_tasksConvToRgb8[roiIdx]->Wait();
        m_tasksConvToRgb8Active[roiIdx] = false;
    }
}

void pm::FrameProcessor::CovertRoiToRgb8bit(uint16_t roiIdx, UseBmp useBmp,
        double min, double max, bool autoConbright, int brightness, int contrast)
{
    if (!m_frame || !m_frame->IsValid())
        return;

    auto& srcBitmaps = GetBitmaps(useBmp);
    auto& srcBmp = srcBitmaps[roiIdx];
    TaskSet_ConvertToRgb8::UpdateLookupMap(m_convToRgb8bitLookupMap,
            srcBmp->GetFormat(), min, max, autoConbright, brightness, contrast);

    DoConvertRoiToRgb8bit(roiIdx, useBmp,
            min, max, autoConbright, brightness, contrast);

    if (m_tasksConvToRgb8Active[roiIdx])
    {
        m_tasksConvToRgb8[roiIdx]->Wait();
        m_tasksConvToRgb8Active[roiIdx] = false;
    }
}

const std::vector<std::unique_ptr<pm::Bitmap>>&
    pm::FrameProcessor::GetRgb8bitBitmaps() const
{
    return m_rgb8bitBitmaps;
}

void pm::FrameProcessor::ComputeStats(UseBmp useBmp)
{
    if (!m_frame || !m_frame->IsValid())
        return;

    m_stats.Clear();

    const size_t count = m_validRoiCount;

    for (uint16_t roiIdx = 0; roiIdx < count; ++roiIdx)
    {
        DoComputeRoiStats(roiIdx, useBmp);
    }

    for (uint16_t roiIdx = 0; roiIdx < count; ++roiIdx)
    {
        if (!m_tasksRoiStatsActive[roiIdx])
            continue;
        m_tasksRoiStats[roiIdx]->Wait();
        m_tasksRoiStatsActive[roiIdx] = false;

        m_stats.Add(m_roiStats[roiIdx]);
    }
}

const pm::FrameStats& pm::FrameProcessor::GetStats() const
{
    return m_stats;
}

void pm::FrameProcessor::ComputeRoiStats(uint16_t roiIdx, UseBmp useBmp)
{
    if (!m_frame || !m_frame->IsValid())
        return;

    DoComputeRoiStats(roiIdx, useBmp);

    if (m_tasksRoiStatsActive[roiIdx])
    {
        m_tasksRoiStats[roiIdx]->Wait();
        m_tasksRoiStatsActive[roiIdx] = false;
    }
}

const std::vector<pm::FrameStats>& pm::FrameProcessor::GetRoiStats() const
{
    return m_roiStats;
}

void pm::FrameProcessor::Recompose(UseBmp useBmp,
        Bitmap* dstBmp, uint16_t dstOffX, uint16_t dstOffY)
{
    if (!m_frame || !m_frame->IsValid())
        return;

    const size_t count = m_validRoiCount;
    for (uint16_t roiIdx = 0; roiIdx < count; ++roiIdx)
    {
        DoRecomposeRoi(roiIdx, useBmp, dstBmp, dstOffX, dstOffY);
    }

    for (uint16_t roiIdx = 0; roiIdx < count; ++roiIdx)
    {
        if (!m_tasksFillBitmapActive[roiIdx])
            continue;
        m_tasksFillBitmap[roiIdx]->Wait();
        m_tasksFillBitmapActive[roiIdx] = false;
    }
}

void pm::FrameProcessor::RecomposeRoi(uint16_t roiIdx, UseBmp useBmp,
        Bitmap* dstBmp, uint16_t dstOffX, uint16_t dstOffY)
{
    if (!m_frame || !m_frame->IsValid())
        return;

    DoRecomposeRoi(roiIdx, useBmp, dstBmp, dstOffX, dstOffY);

    if (m_tasksFillBitmapActive[roiIdx])
    {
        m_tasksFillBitmap[roiIdx]->Wait();
        m_tasksFillBitmapActive[roiIdx] = false;
    }
}

void pm::FrameProcessor::Fill(Bitmap* dstBmp, double value)
{
    if (!m_taskFillBitmapValue)
    {
        m_taskFillBitmapValue = std::make_unique<TaskSet_FillBitmapValue>(
                UniqueThreadPool::Get().GetPool());
    }

    const auto sizeLimit =
        std::max<size_t>(50, 5 * m_taskFillBitmapValue->GetThreadPool()->GetSize());
    const auto srcW = dstBmp->GetWidth();
    const auto srcH = dstBmp->GetHeight();
    const bool doNotParallelize = srcH < sizeLimit || srcW < sizeLimit;
    if (doNotParallelize)
    {
        dstBmp->Fill(value);
    }
    else
    {
        m_taskFillBitmapValue->SetUp(dstBmp, value);
        m_taskFillBitmapValue->Execute();
        m_taskFillBitmapValue->Wait();
    }
}

void pm::FrameProcessor::Reconfigure(const Frame& frame)
{
    Invalidate(); // Clears all vectors except those task-related

    const auto& rawBitmaps = frame.GetRoiBitmaps();
    const auto size = rawBitmaps.size();

    m_debayeredBitmaps.resize(size); // nullptr
    m_rgb8bitBitmaps.resize(size); //nullptr
    m_roiStats.resize(size); // FrameStats()

    if (m_tasksRoiStats.size() < size)
    {
        m_tasksRoiStats.reserve(size);
        m_tasksConvToRgb8.reserve(size);
        m_tasksFillBitmap.reserve(size);

        while (m_tasksRoiStats.size() < size)
        {
            m_tasksRoiStats.push_back(std::make_unique<TaskSet_ComputeFrameStats>(
                        UniqueThreadPool::Get().GetPool()));
            m_tasksConvToRgb8.push_back(std::make_unique<TaskSet_ConvertToRgb8>(
                        UniqueThreadPool::Get().GetPool()));
            m_tasksFillBitmap.push_back(std::make_unique<TaskSet_FillBitmap>(
                        UniqueThreadPool::Get().GetPool()));
        }

        m_tasksRoiStatsActive.resize(size, false);
        m_tasksConvToRgb8Active.resize(size, false);
        m_tasksFillBitmapActive.resize(size, false);
    }

    const auto count = m_tasksRoiStats.size();
    for (size_t n = 0; n < count; ++n)
    {
        m_tasksRoiStatsActive[n] = false;
        m_tasksConvToRgb8Active[n] = false;
        m_tasksFillBitmapActive[n] = false;
    }
}

void pm::FrameProcessor::DoDebayerRoi(uint16_t roiIdx,
        const ph_color_context* colorCtx)
{
    const auto& rawBitmaps = m_frame->GetRoiBitmaps();
    auto& rawBitmap = rawBitmaps[roiIdx];

    auto& debayeredBitmap = m_debayeredBitmaps[roiIdx];
    if (!debayeredBitmap)
    {
        const auto& rawFormat = rawBitmap->GetFormat();
        if (rawFormat.GetPixelType() != BitmapPixelType::Mono)
            throw Exception("Unable to debayer non-mono bitmaps");
        auto rgbFormat = rawFormat;
        rgbFormat.SetPixelType(BitmapPixelType::RGB);
        const auto width = rawBitmap->GetWidth();
        const auto height = rawBitmap->GetHeight();
        const auto rgbBufferBytes =
            Bitmap::CalculateDataBytes(width, height, rgbFormat);
        void* rgbBuffer = ColorUtils::AllocBuffer(rgbBufferBytes);
        debayeredBitmap = std::move(std::make_unique<Bitmap>(
                    rgbBuffer, width, height, rgbFormat));
    }

    const auto& rgns = m_frame->GetRoiBitmapRegions();
    auto& rgn = rgns[roiIdx];

    if (PH_COLOR_ERROR_NONE != PH_COLOR->debayer_and_white_balance(
                colorCtx, rawBitmap->GetData(), rgn, debayeredBitmap->GetData()))
    {
        char errMsg[PH_COLOR_MAX_ERROR_LEN];
        uint32_t errMsgSize = PH_COLOR_MAX_ERROR_LEN;
        PH_COLOR->get_last_error_message(errMsg, &errMsgSize);
        const auto what =
            std::string("Unable to debayer and white-balance frame (")
            +  errMsg + ")";
        throw Exception(what);
    }
}

void pm::FrameProcessor::DoConvertRoiToRgb8bit(uint16_t roiIdx, UseBmp useBmp,
        double min, double max, bool autoConbright, int brightness, int contrast)
{
    auto& srcBitmaps = GetBitmaps(useBmp);
    auto& srcBmp = srcBitmaps[roiIdx];

    auto& rgb8bitBitmap = m_rgb8bitBitmaps[roiIdx];
    if (!rgb8bitBitmap)
    {
        const auto rgbFormat =
            BitmapFormat(BitmapPixelType::RGB, BitmapDataType::UInt8, 8);
        const auto width = srcBmp->GetWidth();
        const auto height = srcBmp->GetHeight();
        rgb8bitBitmap =
            std::move(std::make_unique<Bitmap>(width, height, rgbFormat));
    }

    auto& taskConvToRgb8 = m_tasksConvToRgb8[roiIdx];
    taskConvToRgb8->SetUp(rgb8bitBitmap.get(), srcBmp.get(), min, max,
            &m_convToRgb8bitLookupMap, autoConbright, brightness, contrast);
    taskConvToRgb8->Execute();

    m_tasksConvToRgb8Active[roiIdx] = true;
}

void pm::FrameProcessor::DoComputeRoiStats(uint16_t roiIdx, UseBmp useBmp)
{
    auto& srcBitmaps = GetBitmaps(useBmp);
    auto& srcBmp = srcBitmaps[roiIdx];

    auto& roiStats = m_roiStats[roiIdx];
    roiStats.Clear();

    auto& taskRoiStats = m_tasksRoiStats[roiIdx];
    taskRoiStats->SetUp(srcBmp.get(), &roiStats);
    taskRoiStats->Execute();

    m_tasksRoiStatsActive[roiIdx] = true;
}

void pm::FrameProcessor::DoRecomposeRoi(uint16_t roiIdx, UseBmp useBmp,
        Bitmap* dstBmp, uint16_t dstOffX, uint16_t dstOffY)
{
    auto& srcBitmaps = GetBitmaps(useBmp);
    auto& srcBmp = srcBitmaps[roiIdx];

    auto& bitmapPositions = m_frame->GetRoiBitmapPositions();
    const auto& bitmapPosition = bitmapPositions[roiIdx];

    const uint16_t roiOffX = bitmapPosition.x - dstOffX;
    const uint16_t roiOffY = bitmapPosition.y - dstOffY;

    // Processing too many small ROIs (e.g. centroids) is significantly slower
    // than simply doing it one by one without thread synchronization
    auto& taskFillBitmap = m_tasksFillBitmap[roiIdx];
    const auto sizeLimit =
        std::max<size_t>(50, 5 * taskFillBitmap->GetThreadPool()->GetSize());
    const auto srcW = srcBmp->GetWidth();
    const auto srcH = srcBmp->GetHeight();
    const bool doNotParallelize = srcH < sizeLimit || srcW < sizeLimit;
    if (doNotParallelize)
    {
        dstBmp->Fill(srcBmp.get(), roiOffX, roiOffY);
        m_tasksFillBitmapActive[roiIdx] = false;
    }
    else
    {
        taskFillBitmap->SetUp(dstBmp, srcBmp.get(), roiOffX, roiOffY);
        taskFillBitmap->Execute();
        m_tasksFillBitmapActive[roiIdx] = true;
    }
}
