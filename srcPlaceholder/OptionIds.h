/******************************************************************************/
/* Copyright (C) Teledyne Photometrics. All rights reserved.                  */
/******************************************************************************/
#pragma once
#ifndef PM_OPTION_IDS_H
#define PM_OPTION_IDS_H

/* System */
#include <cstdint>

namespace pm {

/// Related to #pm::Option::GetId but for non-PVCAM options only.
enum class OptionId : uint32_t
{
    /** @showinitializer */
    Unknown = 0,
    /** @showinitializer */
    Help = 1,
    CamIndex,
    FakeCamFps,
    AcqFrameCount,
    BufferFrameCount,
    AllocatorType,
    Regions,
    Exposure,
    VtmExposures,
    AcqMode,
    TimeLapseDelay,
    StorageType,
    SaveDir,
    SaveTiffOptFull,
    SaveDigits,
    SaveFirst,
    SaveLast,
    MaxStackSize,
    TrackLinkFrames,
    TrackMaxDistance,
    TrackCpuOnly,
    TrackTrajectoryDuration,
    ColorWbScaleRed,
    ColorWbScaleGreen,
    ColorWbScaleBlue,
    ColorDebayerAlg,
    ColorCpuOnly,

    /// Has to be last one, the app can use CustomBase+N for custom options.
    /** @showinitializer */
    CustomBase = 0x80000
};

} // namespace

#endif
