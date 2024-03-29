/******************************************************************************/
/* Copyright (C) Teledyne Photometrics. All rights reserved.                  */
/******************************************************************************/
#pragma once
#ifndef PM_SETTINGS_H
#define PM_SETTINGS_H

/* Local */
#include "backend/OptionController.h"
#include "backend/PrdFileFormat.h"
#include "backend/SettingsReader.h"

namespace pm {

/**
@brief Read-write access point to application settings.

This class provides setters for writable application settings as well as
handlers for parsing and validating related CLI option values.

It serves as simple container validating only input values with known range or
checking enumeration values against predefined list of items. If some camera
does not support e.g. some enum value, developer has to handle it explicitly
and ensure such value is not stored in this class. For that purpose can be used
e.g. #pm::Camera::ReviseSettings method for initial validation.
*/
class Settings : public SettingsReader
{
public:
    Settings();
    virtual ~Settings();

public:
    /**
    @brief Adds all supported CLI options to given controller.

    The Help option is intentionally not handled here so the application can
    add it separately and customized.

    @param controller CLI option controller that will process the options.
    */
    bool AddOptions(OptionController& controller);

public: // To be called by anyone
    /**
    @brief Sets an index to detected list of cameras.

    The list of cameras can be built with help of Camera::GetCameraCount and
    Camera::GetName methods.

    @param value PVCAM restricts camera index to @c int16_t type but this method
        enforces the value is not negative.
    */
    bool SetCamIndex(int16_t value);
    bool SetFakeCamFps(unsigned int value);

    bool SetTrigMode(int32_t value);
    bool SetExpOutMode(int32_t value);
    bool SetExposureResolution(int32_t value);

    bool SetAcqFrameCount(uint32_t value);
    bool SetBufferFrameCount(uint32_t value);
    bool SetAllocatorType(AllocatorType value);

    bool SetBinningSerial(uint16_t value);
    bool SetBinningParallel(uint16_t value);
    bool SetRegions(const std::vector<rgn_type>& value);

    bool SetExposure(uint32_t value);
    bool SetVtmExposures(const std::vector<uint16_t>& value);
    bool SetAcqMode(AcqMode value);
    bool SetTimeLapseDelay(unsigned int value);

    bool SetStorageType(StorageType value);
    bool SetSaveDir(const std::string& value);
    bool SetSaveTiffOptFull(bool value);
    bool SetSaveDigits(uint8_t value);
    bool SetSaveFirst(size_t value);
    bool SetSaveLast(size_t value);
    bool SetMaxStackSize(size_t value);

    bool SetTrackLinkFrames(uint16_t value);
    bool SetTrackMaxDistance(uint16_t value);
    bool SetTrackCpuOnly(bool value);
    bool SetTrackTrajectoryDuration(uint16_t value);

    bool SetColorWbScaleRed(float value);
    bool SetColorWbScaleGreen(float value);
    bool SetColorWbScaleBlue(float value);
    bool SetColorDebayerAlgorithm(int32_t value);
    bool SetColorCpuOnly(bool value);

private: // To be called indirectly by OptionController only (CLI options parsing)
    bool HandleCamIndex(const std::string& value);
    bool HandleFakeCamFps(const std::string& value);

    bool HandleTrigMode(const std::string& value);
    bool HandleExpOutMode(const std::string& value);

    bool HandleAcqFrameCount(const std::string& value);
    bool HandleBufferFrameCount(const std::string& value);
    bool HandleAllocatorType(const std::string& value);

    bool HandleBinningSerial(const std::string& value);
    bool HandleBinningParallel(const std::string& value);
    bool HandleRegions(const std::string& value);

    bool HandleExposure(const std::string& value);
    bool HandleVtmExposures(const std::string& value);
    bool HandleAcqMode(const std::string& value);
    bool HandleTimeLapseDelay(const std::string& value);

    bool HandleStorageType(const std::string& value);
    bool HandleSaveDir(const std::string& value);
    bool HandleSaveTiffOptFull(const std::string& value);
    bool HandleSaveDigits(const std::string& value);
    bool HandleSaveFirst(const std::string& value);
    bool HandleSaveLast(const std::string& value);
    bool HandleMaxStackSize(const std::string& value);

    bool HandleTrackLinkFrames(const std::string& value);
    bool HandleTrackMaxDistance(const std::string& value);
    bool HandleTrackCpuOnly(const std::string& value);
    bool HandleTrackTrajectory(const std::string& value);

    bool HandleColorWbScaleRed(const std::string& value);
    bool HandleColorWbScaleGreen(const std::string& value);
    bool HandleColorWbScaleBlue(const std::string& value);
    bool HandleColorDebayerAlgorithm(const std::string& value);
    bool HandleColorCpuOnly(const std::string& value);
};

} // namespace

#endif
