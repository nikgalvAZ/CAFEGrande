/******************************************************************************/
/* Copyright (C) Teledyne Photometrics. All rights reserved.                  */
/******************************************************************************/
#pragma once
#ifndef PM_FRAME_PROCESSOR_H
#define PM_FRAME_PROCESSOR_H

/* Local */
#include "backend/Frame.h"
#include "backend/FrameStats.h"
#include "backend/TaskSet_ComputeFrameStats.h"
#include "backend/TaskSet_ConvertToRgb8.h"
#include "backend/TaskSet_FillBitmap.h"
#include "backend/TaskSet_FillBitmapValue.h"

/* System */
#include <memory>
#include <vector>

struct ph_color_context;

namespace pm {

class FrameProcessor
{
public:
    enum class UseBmp
    {
        Raw,
        Debayered,
        Rgb8bit,
    };

public:
    FrameProcessor();
    ~FrameProcessor();

public:
    void Invalidate();

    // The frame with metadata must be already decoded.
    // - checks if frame configuration is same as previous processing
    // - (re)allocates internal resources to match configuration
    void SetFrame(std::shared_ptr<Frame> frame);

    // Returns bitmap regions, it is the same for all kind of bitmaps
    const std::vector<rgn_type>& GetBitmapRegions() const;
    // Returns bitmap positions, it is the same for all kind of bitmaps
    const std::vector<Frame::Point>& GetBitmapPositions() const;
    // The vectors with bitmaps are allocated to max. possible number of regions.
    // This method returns how many of them are valid. Bitmap pointers beyond
    // that count are null or contain invalid data.
    size_t GetValidBitmapCount() const;

    const std::vector<std::unique_ptr<Bitmap>>& GetBitmaps(UseBmp useBmp) const;

    const std::vector<std::unique_ptr<Bitmap>>& GetRawBitmaps() const;

    // Debayering is always done on Raw bitmaps
    void Debayer(const ph_color_context* colorCtx);
    void DebayerRoi(uint16_t roiIdx, const ph_color_context* colorCtx);
    const std::vector<std::unique_ptr<Bitmap>>& GetDebayeredBitmaps() const;

    // Can work either on Raw or Debayered bitmaps
    void CovertToRgb8bit(UseBmp useBmp,
            double min, double max, bool autoConbright = true,
            int brightness = 0, int contrast = 0);
    void CovertRoiToRgb8bit(uint16_t roiIdx, UseBmp useBmp,
            double min, double max, bool autoConbright = true,
            int brightness = 0, int contrast = 0);
    const std::vector<std::unique_ptr<Bitmap>>& GetRgb8bitBitmaps() const;

    // Computes stats from frame's raw (mono) bitmaps by default
    void ComputeStats(UseBmp useBmp = UseBmp::Raw);
    const FrameStats& GetStats() const;
    void ComputeRoiStats(uint16_t roiIdx, UseBmp useBmp = UseBmp::Raw);
    const std::vector<FrameStats>& GetRoiStats() const;

    // Can work on any bitmap type
    void Recompose(UseBmp useBmp,
            Bitmap* dstBmp, uint16_t dstOffX, uint16_t dstOffY);
    void RecomposeRoi(uint16_t roiIdx, UseBmp useBmp,
            Bitmap* dstBmp, uint16_t dstOffX, uint16_t dstOffY);

    // Doesn't change internal bitmaps, works with dstBmp only
    void Fill(Bitmap* dstBmp, double value);

private:
    void Reconfigure(const Frame& frame);

    void DoDebayerRoi(uint16_t roiIdx, const ph_color_context* colorCtx);

    void DoConvertRoiToRgb8bit(uint16_t roiIdx, UseBmp useBmp,
            double min, double max, bool autoConbright,
            int brightness, int contrast);

    void DoComputeRoiStats(uint16_t roiIdx, UseBmp useBmp);

    void DoRecomposeRoi(uint16_t roiIdx, UseBmp useBmp,
            Bitmap* dstBmp, uint16_t dstOffX, uint16_t dstOffY);

private:
    std::shared_ptr<Frame> m_frame{};

    size_t m_validRoiCount{ 0 };

    std::vector<std::unique_ptr<Bitmap>> m_debayeredBitmaps{};
    std::vector<std::unique_ptr<Bitmap>> m_rgb8bitBitmaps{};

    FrameStats m_stats{};
    std::vector<FrameStats> m_roiStats{};

    std::vector<std::unique_ptr<TaskSet_ComputeFrameStats>> m_tasksRoiStats{};
    std::vector<bool> m_tasksRoiStatsActive{};

    std::vector<std::unique_ptr<TaskSet_ConvertToRgb8>> m_tasksConvToRgb8{};
    std::vector<bool> m_tasksConvToRgb8Active{};
    std::vector<uint8_t> m_convToRgb8bitLookupMap{};

    std::vector<std::unique_ptr<TaskSet_FillBitmap>> m_tasksFillBitmap{};
    std::vector<bool> m_tasksFillBitmapActive{};

    std::unique_ptr<TaskSet_FillBitmapValue> m_taskFillBitmapValue{};
};

} // namespace

#endif
