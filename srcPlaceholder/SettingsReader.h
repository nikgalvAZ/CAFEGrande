/******************************************************************************/
/* Copyright (C) Teledyne Photometrics. All rights reserved.                  */
/******************************************************************************/
#pragma once
#ifndef PM_SETTINGS_READER_H
#define PM_SETTINGS_READER_H

/* Local */
#include "backend/AllocatorType.h"

/* PVCAM */
#include "master.h"
#include "pvcam.h"

/* pvcam_helper_color */
#include "pvcam_helper_color.h"

/* System */
#include <cstdint>
#include <string>
#include <vector>

namespace pm {

enum class AcqMode : int32_t
{
    SnapSequence,
    SnapCircBuffer,
    SnapTimeLapse,
    LiveCircBuffer,
    LiveTimeLapse,
};

enum class StorageType : int32_t
{
    /** @showinitializer */
    None = 0,
    Prd,
    Tiff,
    BigTiff,
};

/**
@brief Read-only access point to whole application settings.

This class is designed to be passed to modules that are not supposed to change
the settings. It holds either current value of PVCAM parameters that are
writable or additional settings needed on multiple places in code.

To get a full read-write access to settings you need to get an instance of
#pm::Settings class.

Each settings is usually also possible to configure via command line.
*/
class SettingsReader
{
public:
    /**
    @brief The only place that initializes settings to some default values.

    Default values can be corrected or fully changed via #pm::Settings class
    once the camera is open and we can get supported values, value ranges etc.
    */
    SettingsReader();
    virtual ~SettingsReader();

public:
    /** @brief Calculates implied region from given vector.

    @param regions A list of regions.
    @return Returns either implied region or empty region (with all zeros)
        if @a regions is empty or regions don't have same binning factors.
    */
    static rgn_type GetImpliedRegion(const std::vector<rgn_type>& regions);

public:
    int16_t GetCamIndex() const
    { return m_camIndex; }
    unsigned int GetFakeCamFps() const
    { return m_fakeCamFps; }

    int32_t GetTrigMode() const
    { return m_trigMode; }
    int32_t GetExpOutMode() const
    { return m_expOutMode; }

    uint32_t GetAcqFrameCount() const
    { return m_acqFrameCount; }
    uint32_t GetBufferFrameCount() const
    { return m_bufferFrameCount; }
    AllocatorType GetAllocatorType() const
    { return m_allocatorType; }

    uint16_t GetBinningSerial() const
    { return m_binSer; }
    uint16_t GetBinningParallel() const
    { return m_binPar; }
    const std::vector<rgn_type>& GetRegions() const
    { return m_regions; }

    uint32_t GetExposure() const
    { return m_expTime; }
    const std::vector<uint16_t>& GetVtmExposures() const
    { return m_vtmExposures; }
    int32_t GetExposureResolution() const
    { return m_expTimeRes; }
    AcqMode GetAcqMode() const
    { return m_acqMode; }
    unsigned int GetTimeLapseDelay() const
    { return m_timeLapseDelay; }

    StorageType GetStorageType() const
    { return m_storageType; }
    const std::string& GetSaveDir() const
    { return m_saveDir; }
    bool GetSaveTiffOptFull() const
    { return m_saveTiffOptFull; }
    uint8_t GetSaveDigits() const
    { return m_saveDigits; }
    size_t GetSaveFirst() const
    { return m_saveFirst; }
    size_t GetSaveLast() const
    { return m_saveLast; }
    size_t GetMaxStackSize() const
    { return m_maxStackSize; }

    uint16_t GetTrackLinkFrames() const
    { return m_trackLinkFrames; }
    uint16_t GetTrackMaxDistance() const
    { return m_trackMaxDistance; }
    bool GetTrackCpuOnly() const
    { return m_trackCpuOnly; }
    uint16_t GetTrackTrajectoryDuration() const
    { return m_trackTrajectoryDuration; }

    float GetColorWbScaleRed() const
    { return m_colorWbScaleRed; }
    float GetColorWbScaleGreen() const
    { return m_colorWbScaleGreen; }
    float GetColorWbScaleBlue() const
    { return m_colorWbScaleBlue; }
    int32_t GetColorDebayerAlgorithm() const
    { return m_colorDebayerAlg; }
    bool GetColorCpuOnly() const
    { return m_colorCpuOnly; }

protected: // The only place where are specified default values

    int16_t m_camIndex{ 0 }; // First camera
    unsigned int m_fakeCamFps{ 0 }; // Zero for real cameras

    // Other PVCAM parameters
    int32_t m_trigMode{ TIMED_MODE }; // PL_EXPOSURE_MODES
    int32_t m_expOutMode{ EXPOSE_OUT_FIRST_ROW }; // PL_EXPOSE_OUT_MODES
    int32_t m_expTimeRes{ EXP_RES_ONE_MILLISEC }; // PL_EXP_RES_MODES

    // Other non-PVCAM and combined parameters
    uint32_t m_acqFrameCount{ 1 };
    uint32_t m_bufferFrameCount{ 50 };
    AllocatorType m_allocatorType{ AllocatorType::Align4k };

    uint16_t m_binSer{ 1 };
    uint16_t m_binPar{ 1 };
    std::vector<rgn_type> m_regions{};

    uint32_t m_expTime{ 10 };
    std::vector<uint16_t> m_vtmExposures{ 10, 20, 30 };
    AcqMode m_acqMode{ AcqMode::SnapSequence };
    unsigned int m_timeLapseDelay{ 0 };

    StorageType m_storageType{ StorageType::None };
    std::string m_saveDir{};
    bool m_saveTiffOptFull{ false };
    uint8_t m_saveDigits{ 0 };
    size_t m_saveFirst{ 0 };
    size_t m_saveLast{ 0 };
    size_t m_maxStackSize{ 0 };

    uint16_t m_trackLinkFrames{ 2 };
    uint16_t m_trackMaxDistance{ 25 };
    bool m_trackCpuOnly{ false };
    uint16_t m_trackTrajectoryDuration{ 10 };

    float m_colorWbScaleRed{ 1.0 };
    float m_colorWbScaleGreen{ 1.0 };
    float m_colorWbScaleBlue{ 1.0 };
    int32_t m_colorDebayerAlg{ PH_COLOR_DEBAYER_ALG_NEAREST }; // PH_COLOR_DEBAYER_ALG
    bool m_colorCpuOnly{ false };
};

} // namespace

#endif
