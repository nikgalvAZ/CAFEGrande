/******************************************************************************/
/* Copyright (C) Teledyne Photometrics. All rights reserved.                  */
/******************************************************************************/
#include "backend/Settings.h"

/* Local */
#include "backend/Log.h"
#include "backend/Utils.h"

using SettingsProto = bool(pm::Settings::*)(const std::string&);

// pm::Settings

pm::Settings::Settings()
    : SettingsReader()
{
}

pm::Settings::~Settings()
{
}

bool pm::Settings::AddOptions(OptionController& controller)
{
    const char valSep = Option::ValuesSeparator;
    const char grpSep = Option::ValueGroupsSeparator;

    if (!controller.AddOption(Option(
            { "--cam-index", "-c" },
            { "index" },
            { "0" },
            "Index of camera to be used for acquisition.",
            static_cast<uint32_t>(OptionId::CamIndex),
            std::bind(&Settings::HandleCamIndex,
                    this, std::placeholders::_1))))
        return false;

    if (!controller.AddOption(Option(
            { "--gen-data", "--fps" },
            { "FPS" },
            { "0" },
            "Generates random image at given frame rate.\n"
            "Does not use real camera.",
            static_cast<uint32_t>(OptionId::FakeCamFps),
            std::bind(&Settings::HandleFakeCamFps,
                    this, std::placeholders::_1))))
        return false;

    if (!controller.AddOption(Option(
            { "--exposure-mode", "--trigger-mode", "--trig-mode" },
            { "mode" },
            { "<camera default>" },
            "Trigger (or exposure) mode used for exposure triggering.\n"
            "It is related to expose out mode, see details in PVCAM manual.\n"
            "Supported values are : Classics modes 'timed', 'strobed', 'bulb',\n"
            "'trigger-first', 'flash', 'variable-timed', 'int-strobe'\n"
            "and extended modes 'ext-internal', 'ext-trig-first', 'ext-edge-raising',\n"
            "'ext-trig-sw-first' and 'ext-trig-sw-edge'.\n"
            "WARNING:\n"
            "  'variable-timed' mode works in time-lapse acquisition modes only!",
            PARAM_EXPOSURE_MODE,
            std::bind(&Settings::HandleTrigMode,
                    this, std::placeholders::_1))))
        return false;

    if (!controller.AddOption(Option(
            { "--expose-out-mode", "--exp-out-mode" },
            { "mode" },
            { "<camera default>" },
            "Expose mode used for exposure triggering.\n"
            "It is related to exposure mode, see details in PVCAM manual.\n"
            "Supported values are : 'first-row', 'all-rows', 'any-row', 'rolling-shutter'\n"
            "and 'line-trigger'.",
            PARAM_EXPOSE_OUT_MODE,
            std::bind(&Settings::HandleExpOutMode,
                    this, std::placeholders::_1))))
        return false;

    if (!controller.AddOption(Option(
            { "--acq-frames", "-f" },
            { "count" },
            { "1" },
            "Total number of frames to be captured in acquisition.\n"
            "In snap sequence mode (set via --acq-mode) the total number of frames\n"
            "is limited to value 65535.",
            static_cast<uint32_t>(OptionId::AcqFrameCount),
            std::bind(&Settings::HandleAcqFrameCount,
                    this, std::placeholders::_1))))
        return false;

    if (!controller.AddOption(Option(
            { "--buffer-frames" },
            { "count" },
            { "50" },
            "Number of frames in PVCAM circular buffer.",
            static_cast<uint32_t>(OptionId::BufferFrameCount),
            std::bind(&Settings::HandleBufferFrameCount,
                    this, std::placeholders::_1))))
        return false;

    if (!controller.AddOption(Option(
            { "--allocator" },
            { "type" },
            { "align4k" },
            "Changes how is buffer memory allocated and aligned.\n"
            "The 'align4k' allocator allows optimized streaming to disk in PRD\n"
            "format without additional buffering done by OS.\n"
            "Supported values are: 'default', 'align16', 'align32' and 'align4k'.",
            static_cast<uint32_t>(OptionId::AllocatorType),
            std::bind(&Settings::HandleAllocatorType,
                    this, std::placeholders::_1))))
        return false;

    if (!controller.AddOption(Option(
            { "--binning-serial", "--sbin" },
            { "factor" },
            { "<camera default> or 1" },
            "Serial binning factor.",
            PARAM_BINNING_SER,
            std::bind(&Settings::HandleBinningSerial,
                    this, std::placeholders::_1))))
        return false;

    if (!controller.AddOption(Option(
            { "--binning-parallel", "--pbin" },
            { "factor" },
            { "<camera default> or 1" },
            "Parallel binning factor.",
            PARAM_BINNING_PAR,
            std::bind(&Settings::HandleBinningParallel,
                    this, std::placeholders::_1))))
        return false;

    const std::string roiArgsDescs = std::string()
        + "sA1" + valSep + "sA2" + valSep + "pA1" + valSep + "pA2" + grpSep
        + "sB1" + valSep + "sB2" + valSep + "pB1" + valSep + "pB2" + grpSep
        + "...";
    if (!controller.AddOption(Option(
            { "--region", "--regions", "--roi", "--rois", "-r" },
            { roiArgsDescs },
            { "" },
            "Region of interest for serial (width) and parallel (height) dimension.\n"
            "'sA1' is the first pixel, 'sA2' is the last pixel of the first region\n"
            "included on row. The same applies to columns. Multiple regions are\n"
            "separated by semicolon.\n"
            "Example of two diagonal regions 10x10: '--rois=0,9,0,9;10,19,10,19'.\n"
            "Binning factors are configured separately (via --sbin and --pbin).\n"
            "The empty value causes the camera's full-frame will be used internally.",
            static_cast<uint32_t>(OptionId::Regions),
            std::bind(&Settings::HandleRegions,
                    this, std::placeholders::_1))))
        return false;

    if (!controller.AddOption(Option(
            { "--exposure", "--exposure-time", "-e" },
            { "units" },
            { "10" },
            "Exposure time for each frame in millisecond units by default.\n"
            "Use us, ms or s suffix to change exposure resolution, e.g. 100us or 10ms.",
            static_cast<uint32_t>(OptionId::Exposure),
            std::bind(&Settings::HandleExposure,
                    this, std::placeholders::_1))))
        return false;

    if (!controller.AddOption(Option(
            { "--vtm-exposures" },
            { "units" },
            { "10,20,30" },
            "A set of exposure times used with variable timed trigger mode.\n"
            "It should be a list of comma-separated values in range from 1 to 65535.\n"
            "The exposure resolution is the same as set by --exposure option.\n"
            "WARNING:\n"
            "  VTM works in time-lapse acquisition modes only!",
            static_cast<uint32_t>(OptionId::VtmExposures),
            std::bind(&Settings::HandleVtmExposures,
                    this, std::placeholders::_1))))
        return false;

    if (!controller.AddOption(Option(
            { "--acq-mode" },
            { "mode" },
            { "snap-seq" },
            "Specifies acquisition mode used for collecting images.\n"
            "Supported values are : 'snap-seq', 'snap-circ-buffer', 'snap-time-lapse',\n"
            "'live-circ-buffer' and 'live-time-lapse'.\n"
            "'snap-seq' mode:\n"
            "  Frames are captured in one sequence instead of continuous\n"
            "  acquisition with circular buffer.\n"
            "  Number of frames in buffer (set using --buffer-frames) has to\n"
            "  be equal or greater than number of frames in sequence\n"
            "  (set using --acq-frames).\n"
            "'snap-circ-buffer' mode:\n"
            "  Uses circular buffer to snap given number of frames in continuous\n"
            "  acquisition.\n"
            "  If the frame rate is high enough, it happens that number of\n"
            "  acquired frames is higher that requested, because new frames\n"
            "  can come between stop request and actual acq. interruption.\n"
            "'snap-time-lapse' mode:\n"
            "  Required number of frames is collected using multiple sequence\n"
            "  acquisitions where only one frame is captured at a time.\n"
            "  Delay between single frames can be set using --time-lapse-delay\n"
            "  option."
            "'live-circ-buffer' mode:\n"
            "  Uses circular buffer to snap frames in infinite continuous\n"
            "  acquisition.\n"
            "'live-time-lapse' mode:\n"
            "  The same as 'snap-time-lapse' but runs in infinite loop.",
            static_cast<uint32_t>(OptionId::AcqMode),
            std::bind(&Settings::HandleAcqMode,
                    this, std::placeholders::_1))))
        return false;

    if (!controller.AddOption(Option(
            { "--time-lapse-delay" },
            { "milliseconds" },
            { "0" },
            "A delay between single frames in time lapse mode.",
            static_cast<uint32_t>(OptionId::TimeLapseDelay),
            std::bind(&Settings::HandleTimeLapseDelay,
                    this, std::placeholders::_1))))
        return false;

    if (!controller.AddOption(Option(
            { "--save-as" },
            { "format" },
            { "none" },
            "Stores captured frames on disk in chosen format.\n"
            "Supported values are: 'none', 'prd', 'tiff' and 'big-tiff'.",
            static_cast<uint32_t>(OptionId::StorageType),
            std::bind(&Settings::HandleStorageType,
                    this, std::placeholders::_1))))
        return false;

    if (!controller.AddOption(Option(
            { "--save-dir" },
            { "folder" },
            { "" },
            "Stores captured frames on disk in given existing directory.\n"
            "If empty string is given (the default) current working directory is used.",
            static_cast<uint32_t>(OptionId::SaveDir),
            std::bind(&Settings::HandleSaveDir,
                    this, std::placeholders::_1))))
        return false;

    if (!controller.AddOption(Option(
            { "--save-tiff-opt-full" },
            { "" },
            { "false" },
            "If 'true', saves fully processed images if selected format is 'tiff' or 'big-tiff'.\n"
            "By default TIFF file contains unaltered raw pixel data that require additional\n"
            "processing like debayering or white-balancing.",
            static_cast<uint32_t>(OptionId::SaveTiffOptFull),
            std::bind(&Settings::HandleSaveTiffOptFull,
                    this, std::placeholders::_1))))
        return false;

    if (!controller.AddOption(Option(
            { "--save-digits" },
            { "count" },
            { "0" },
            "Uses a counter in file name with <count> fixed digits.\n"
            "If the counter value doesn't fill all digits leading zeros are applied.",
            static_cast<uint32_t>(OptionId::SaveDigits),
            std::bind(&Settings::HandleSaveDigits,
                    this, std::placeholders::_1))))
        return false;

    if (!controller.AddOption(Option(
            { "--save-first" },
            { "count" },
            { "0" },
            "Saves only first <count> frames.\n"
            "If both --save-first and --save-last are zero, all frames are stored unless\n"
            "an option --save-as is 'none'.",
            static_cast<uint32_t>(OptionId::SaveFirst),
            std::bind(&Settings::HandleSaveFirst,
                    this, std::placeholders::_1))))
        return false;

    if (!controller.AddOption(Option(
            { "--save-last" },
            { "count" },
            { "0" },
            "Saves only last <count> frames.\n"
            "If both --save-first and --save-last are zero, all frames are stored unless\n"
            "an option --save-as is 'none'.",
            static_cast<uint32_t>(OptionId::SaveLast),
            std::bind(&Settings::HandleSaveLast,
                    this, std::placeholders::_1))))
        return false;

    if (!controller.AddOption(Option(
            { "--save-stack-size", "--save-max-stack-size", "--max-stack-size" },
            { "size" },
            { "0" },
            "Stores multiple frames in one file up to given size.\n"
            "Another stack file with new index is created for more frames.\n"
            "Use k, M or G suffix to enter nicer values. (1k = 1024)\n"
            "Default value is 0 which means each frame is stored to its own file.\n"
            "WARNING:\n"
            "  Storing too many small frames into one TIFF file (using --max-stack-size)\n"
            "  might be significantly slower compared to PRD format!",
            static_cast<uint32_t>(OptionId::MaxStackSize),
            std::bind(&Settings::HandleMaxStackSize,
                    this, std::placeholders::_1))))
        return false;

    if (!controller.AddOption(Option(
            { "--track-link-frames" },
            { "count" },
            { "10" },
            "Tracks particles for given number of frames.",
            static_cast<uint32_t>(OptionId::TrackLinkFrames),
            std::bind(&Settings::HandleTrackLinkFrames,
                    this, std::placeholders::_1))))
        return false;

    if (!controller.AddOption(Option(
            { "--track-max-dist" },
            { "pixels" },
            { "25" },
            "Searches for same particles not further than given distance.",
            static_cast<uint32_t>(OptionId::TrackMaxDistance),
            std::bind(&Settings::HandleTrackMaxDistance,
                    this, std::placeholders::_1))))
        return false;

    if (!controller.AddOption(Option(
            { "--track-cpu-only" },
            { "" },
            { "false" },
            "Enforces linking on CPU, does not use CUDA on GPU even if available.",
            static_cast<uint32_t>(OptionId::TrackCpuOnly),
            std::bind(&Settings::HandleTrackCpuOnly,
                    this, std::placeholders::_1))))
        return false;

    if (!controller.AddOption(Option(
            { "--track-trajectory" },
            { "frames" },
            { "10" },
            "Draws a trajectory lines for each particle for given number of frames.\n"
            "Zero value means the trajectories won't be displayed.",
            static_cast<uint32_t>(OptionId::TrackTrajectoryDuration),
            std::bind(&Settings::HandleTrackTrajectory,
                    this, std::placeholders::_1))))
        return false;

    if (!controller.AddOption(Option(
            { "--color-wb-scale-red" },
            { "scale" },
            { "1.0" },
            "Red channel scale factor for white balance the image.\n"
            "The value must be zero or positive.",
            static_cast<uint32_t>(OptionId::ColorWbScaleRed),
            std::bind(&Settings::HandleColorWbScaleRed,
                    this, std::placeholders::_1))))
        return false;

    if (!controller.AddOption(Option(
            { "--color-wb-scale-green" },
            { "scale" },
            { "1.0" },
            "Green channel scale factor for white balance the image.\n"
            "The value must be zero or positive.",
            static_cast<uint32_t>(OptionId::ColorWbScaleGreen),
            std::bind(&Settings::HandleColorWbScaleGreen,
                    this, std::placeholders::_1))))
        return false;

    if (!controller.AddOption(Option(
            { "--color-wb-scale-blue" },
            { "scale" },
            { "1.0" },
            "Blue channel scale factor for white balance the image.\n"
            "The value must be zero or positive.",
            static_cast<uint32_t>(OptionId::ColorWbScaleBlue),
            std::bind(&Settings::HandleColorWbScaleBlue,
                    this, std::placeholders::_1))))
        return false;

    if (!controller.AddOption(Option(
            { "--color-debayer-alg" },
            { "algorithm" },
            { "nearest" },
            "Debayer algorithm used to demosaic mono buffer coming from color camera.\n"
            "Supported values are : 'nearest' and 'bilinear'.",
            static_cast<uint32_t>(OptionId::ColorDebayerAlg),
            std::bind(&Settings::HandleColorDebayerAlgorithm,
                    this, std::placeholders::_1))))
        return false;

    if (!controller.AddOption(Option(
            { "--color-cpu-only" },
            { "" },
            { "false" },
            "Enforces color image processing on CPU, does not use CUDA on GPU even if available.",
            static_cast<uint32_t>(OptionId::ColorCpuOnly),
            std::bind(&Settings::HandleColorCpuOnly,
                    this, std::placeholders::_1))))
        return false;

    return true;
}

bool pm::Settings::SetCamIndex(int16_t value)
{
    if (value < 0)
        return false;

    m_camIndex = value;
    return true;
}

bool pm::Settings::SetFakeCamFps(unsigned int value)
{
    m_fakeCamFps = value;
    return true;
}

bool pm::Settings::SetTrigMode(int32_t value)
{
    m_trigMode = value;
    return true;
}

bool pm::Settings::SetExpOutMode(int32_t value)
{
    m_expOutMode = value;
    return true;
}

bool pm::Settings::SetExposureResolution(int32_t value)
{
    switch (value)
    {
    case EXP_RES_ONE_MICROSEC:
    case EXP_RES_ONE_MILLISEC:
    case EXP_RES_ONE_SEC:
        m_expTimeRes = value;
        return true;
    default:
        return false;
    }
}

bool pm::Settings::SetAcqFrameCount(uint32_t value)
{
    m_acqFrameCount = value;
    return true;
}

bool pm::Settings::SetBufferFrameCount(uint32_t value)
{
    m_bufferFrameCount = value;
    return true;
}

bool pm::Settings::SetAllocatorType(AllocatorType value)
{
    m_allocatorType = value;
    return true;
}

bool pm::Settings::SetBinningSerial(uint16_t value)
{
    if (value == 0)
        return false;

    m_binSer = value;

    for (size_t n = 0; n < m_regions.size(); n++)
    {
        m_regions[n].sbin = m_binSer;
        m_regions[n].pbin = m_binPar;
    }

    return true;
}

bool pm::Settings::SetBinningParallel(uint16_t value)
{
    if (value == 0)
        return false;

    m_binPar = value;

    for (size_t n = 0; n < m_regions.size(); n++)
    {
        m_regions[n].sbin = m_binSer;
        m_regions[n].pbin = m_binPar;
    }

    return true;
}

bool pm::Settings::SetRegions(const std::vector<rgn_type>& value)
{
    for (auto& roi : value)
    {
        if (roi.sbin != m_binSer || roi.pbin != m_binPar)
        {
            Log::LogE("Region binning factors do not match");
            return false;
        }
    }

    m_regions = value;
    return true;
}

bool pm::Settings::SetExposure(uint32_t value)
{
    m_expTime = value;
    return true;
}

bool pm::Settings::SetVtmExposures(const std::vector<uint16_t>& value)
{
    m_vtmExposures = value;
    return true;
}

bool pm::Settings::SetAcqMode(AcqMode value)
{
    m_acqMode = value;
    return true;
}

bool pm::Settings::SetTimeLapseDelay(unsigned int value)
{
    m_timeLapseDelay = value;
    return true;
}

bool pm::Settings::SetStorageType(StorageType value)
{
    m_storageType = value;
    return true;
}

bool pm::Settings::SetSaveDir(const std::string& value)
{
    m_saveDir = value;
    return true;
}

bool pm::Settings::SetSaveTiffOptFull(bool value)
{
    m_saveTiffOptFull = value;
    return true;
}

bool pm::Settings::SetSaveDigits(uint8_t value)
{
    m_saveDigits = value;
    return true;
}

bool pm::Settings::SetSaveFirst(size_t value)
{
    m_saveFirst = value;
    return true;
}

bool pm::Settings::SetSaveLast(size_t value)
{
    m_saveLast = value;
    return true;
}

bool pm::Settings::SetMaxStackSize(size_t value)
{
    m_maxStackSize = value;
    return true;
}

bool pm::Settings::SetTrackLinkFrames(uint16_t value)
{
    m_trackLinkFrames = value;
    return true;
}

bool pm::Settings::SetTrackMaxDistance(uint16_t value)
{
    m_trackMaxDistance = value;
    return true;
}

bool pm::Settings::SetTrackCpuOnly(bool value)
{
    m_trackCpuOnly = value;
    return true;
}

bool pm::Settings::SetTrackTrajectoryDuration(uint16_t value)
{
    m_trackTrajectoryDuration = value;
    return true;
}

bool pm::Settings::SetColorWbScaleRed(float value)
{
    m_colorWbScaleRed = value;
    return true;
}

bool pm::Settings::SetColorWbScaleGreen(float value)
{
    m_colorWbScaleGreen = value;
    return true;
}

bool pm::Settings::SetColorWbScaleBlue(float value)
{
    m_colorWbScaleBlue = value;
    return true;
}

bool pm::Settings::SetColorDebayerAlgorithm(int32_t value)
{
    m_colorDebayerAlg = value;
    return true;
}

bool pm::Settings::SetColorCpuOnly(bool value)
{
    m_colorCpuOnly = value;
    return true;
}

bool pm::Settings::HandleCamIndex(const std::string& value)
{
    int16_t index;
    if (!Utils::StrToNumber<int16_t>(value, index))
        return false;

    return SetCamIndex(index);
}

bool pm::Settings::HandleFakeCamFps(const std::string& value)
{
    unsigned int fps;
    if (!Utils::StrToNumber<unsigned int>(value, fps))
        return false;

    return SetFakeCamFps(fps);
}

bool pm::Settings::HandleTrigMode(const std::string& value)
{
    int32_t trigMode;
    if (!Utils::StrToNumber<int32_t>(value, trigMode))
    {
        // Classic modes
        if (value == "timed")
            trigMode = TIMED_MODE;
        else if (value == "strobed")
            trigMode = STROBED_MODE;
        else if (value == "bulb")
            trigMode = BULB_MODE;
        else if (value == "trigger-first")
            trigMode = TRIGGER_FIRST_MODE;
        else if (value == "flash")
            trigMode = FLASH_MODE;
        else if (value == "variable-timed")
            trigMode = VARIABLE_TIMED_MODE;
        else if (value == "int-strobe")
            trigMode = INT_STROBE_MODE;
        // Extended modes
        else if (value == "ext-internal")
            trigMode = EXT_TRIG_INTERNAL;
        else if (value == "ext-trig-first")
            trigMode = EXT_TRIG_TRIG_FIRST;
        else if (value == "ext-edge-raising")
            trigMode = EXT_TRIG_EDGE_RISING;
        else if (value == "ext-level")
            trigMode = EXT_TRIG_LEVEL;
        else if (value == "ext-trig-sw-first")
            trigMode = EXT_TRIG_SOFTWARE_FIRST;
        else if (value == "ext-trig-sw-edge")
            trigMode = EXT_TRIG_SOFTWARE_EDGE;
        else
            return false;
    }

    return SetTrigMode(trigMode);
}

bool pm::Settings::HandleExpOutMode(const std::string& value)
{
    int32_t expOutMode;
    if (!Utils::StrToNumber<int32_t>(value, expOutMode))
    {
        if (value == "first-row")
            expOutMode = EXPOSE_OUT_FIRST_ROW;
        else if (value == "all-rows")
            expOutMode = EXPOSE_OUT_ALL_ROWS;
        else if (value == "any-row")
            expOutMode = EXPOSE_OUT_ANY_ROW;
        else if (value == "rolling-shutter")
            expOutMode = EXPOSE_OUT_ROLLING_SHUTTER;
        else if (value == "line-trigger")
            expOutMode = EXPOSE_OUT_LINE_TRIGGER;
        else
            return false;
    }

    return SetExpOutMode(expOutMode);
}

bool pm::Settings::HandleAcqFrameCount(const std::string& value)
{
    uint32_t acqFrameCount;
    if (!Utils::StrToNumber<uint32_t>(value, acqFrameCount))
        return false;

    return SetAcqFrameCount(acqFrameCount);
}

bool pm::Settings::HandleBufferFrameCount(const std::string& value)
{
    uint32_t bufferFrameCount;
    if (!Utils::StrToNumber<uint32_t>(value, bufferFrameCount))
        return false;

    return SetBufferFrameCount(bufferFrameCount);
}

bool pm::Settings::HandleAllocatorType(const std::string& value)
{
    AllocatorType allocatorType;
    if (value == "default" || value == "0")
        allocatorType = AllocatorType::Default;
    else if (value == "align16" || value == "16")
        allocatorType = AllocatorType::Align16;
    else if (value == "align32" || value == "32")
        allocatorType = AllocatorType::Align32;
    else if (value == "align4k" || value == "4096")
        allocatorType = AllocatorType::Align4k;
    else
        return false;

    return SetAllocatorType(allocatorType);
}

bool pm::Settings::HandleBinningSerial(const std::string& value)
{
    uint16_t binSer;
    if (!Utils::StrToNumber<uint16_t>(value, binSer))
        return false;

    return SetBinningSerial(binSer);
}

bool pm::Settings::HandleBinningParallel(const std::string& value)
{
    uint16_t binPar;
    if (!Utils::StrToNumber<uint16_t>(value, binPar))
        return false;

    return SetBinningParallel(binPar);
}

bool pm::Settings::HandleRegions(const std::string& value)
{
    std::vector<rgn_type> regions;

    if (value.empty())
    {
        // No regions means full sensor size will be used
        return SetRegions(regions);
    }

    const std::vector<std::string> rois =
        Utils::StrToArray(value, Option::ValueGroupsSeparator);

    if (rois.size() == 0)
    {
        Log::LogE("Incorrect number of values for ROI");
        return false;
    }

    for (std::string roi : rois)
    {
        std::vector<uint16_t> values;
        if (!Utils::StrToArray(values, roi, Option::ValuesSeparator))
        {
            Log::LogE("Incorrect ROI value(s) - '%s'", roi.c_str());
            return false;
        }

        if (values.size() != 4)
        {
            Log::LogE("Incorrect number of values for ROI");
            return false;
        }

        rgn_type rgn;
        rgn.s1 = values[0];
        rgn.s2 = values[1];
        rgn.sbin = m_binSer;
        rgn.p1 = values[2];
        rgn.p2 = values[3];
        rgn.pbin = m_binPar;

        regions.push_back(rgn);
    }

    return SetRegions(regions);
}

bool pm::Settings::HandleExposure(const std::string& value)
{
    const size_t suffixPos = value.find_first_not_of("0123456789");

    std::string rawValue(value);
    std::string suffix;
    if (suffixPos != std::string::npos)
    {
        suffix = value.substr(suffixPos);
        // Remove the suffix from value
        rawValue = value.substr(0, suffixPos);
    }

    uint32_t expTime;
    if (!Utils::StrToNumber<uint32_t>(rawValue, expTime))
        return false;

    int32_t expTimeRes;
    if (suffix == "us")
        expTimeRes = EXP_RES_ONE_MICROSEC;
    else if (suffix == "ms" || suffix.empty()) // ms is default resolution
        expTimeRes = EXP_RES_ONE_MILLISEC;
    else if (suffix == "s")
        expTimeRes = EXP_RES_ONE_SEC;
    else
        return false;

    if (!SetExposure(expTime))
        return false;
    return SetExposureResolution(expTimeRes);
}

bool pm::Settings::HandleVtmExposures(const std::string& value)
{
    std::vector<uint16_t> exposures;
    if (!Utils::StrToArray(exposures, value, Option::ValuesSeparator))
    {
        Log::LogE("Incorrect VTM exposure value(s) '%s'", value.c_str());
        return false;
    }

    if (exposures.size() == 0)
    {
        Log::LogE("Incorrect number of values for VTM exposures");
        return false;
    }

    for (auto exposure : exposures)
    {
        if (exposure == 0)
        {
            Log::LogE("In VTM, zero exposure is not supported");
            return false;
        }
    }

    return SetVtmExposures(exposures);
}

bool pm::Settings::HandleAcqMode(const std::string& value)
{
    AcqMode acqMode;
    if (value == "snap-seq")
        acqMode = AcqMode::SnapSequence;
    else if (value == "snap-circ-buffer")
        acqMode = AcqMode::SnapCircBuffer;
    else if (value == "snap-time-lapse")
        acqMode = AcqMode::SnapTimeLapse;
    else if (value == "live-circ-buffer")
        acqMode = AcqMode::LiveCircBuffer;
    else if (value == "live-time-lapse")
        acqMode = AcqMode::LiveTimeLapse;
    else
        return false;

    return SetAcqMode(acqMode);
}

bool pm::Settings::HandleTimeLapseDelay(const std::string& value)
{
    unsigned int timeLapseDelay;
    if (!Utils::StrToNumber<unsigned int>(value, timeLapseDelay))
        return false;

    return SetTimeLapseDelay(timeLapseDelay);
}

bool pm::Settings::HandleStorageType(const std::string& value)
{
    StorageType storageType;
    if (value == "none")
        storageType = StorageType::None;
    else if (value == "prd")
        storageType = StorageType::Prd;
    else if (value == "tiff")
        storageType = StorageType::Tiff;
    else if (value == "big-tiff")
        storageType = StorageType::BigTiff;
    else
        return false;

    return SetStorageType(storageType);
}

bool pm::Settings::HandleSaveDir(const std::string& value)
{
    return SetSaveDir(value);
}

bool pm::Settings::HandleSaveTiffOptFull(const std::string& value)
{
    bool tiffOptFull;
    if (value.empty())
    {
        tiffOptFull = true;
    }
    else
    {
        if (!Utils::StrToBool(value, tiffOptFull))
            return false;
    }

    return SetSaveTiffOptFull(tiffOptFull);
}

bool pm::Settings::HandleSaveDigits(const std::string& value)
{
    uint8_t saveDigits;
    if (!Utils::StrToNumber<uint8_t>(value, saveDigits))
        return false;

    return SetSaveDigits(saveDigits);
}

bool pm::Settings::HandleSaveFirst(const std::string& value)
{
    size_t saveFirst;
    if (!Utils::StrToNumber<size_t>(value, saveFirst))
        return false;

    return SetSaveFirst(saveFirst);
}

bool pm::Settings::HandleSaveLast(const std::string& value)
{
    size_t saveLast;
    if (!Utils::StrToNumber<size_t>(value, saveLast))
        return false;

    return SetSaveLast(saveLast);
}

bool pm::Settings::HandleMaxStackSize(const std::string& value)
{
    const size_t suffixPos = value.find_last_of("kMG");
    char suffix = ' ';
    std::string rawValue(value);
    if (suffixPos != std::string::npos && suffixPos == value.length() - 1)
    {
        suffix = value[suffixPos];
        // Remove the suffix from value
        rawValue.pop_back();
    }

    size_t bytes;
    if (!Utils::StrToNumber<size_t>(rawValue, bytes))
        return false;
    size_t maxStackSize = bytes;

    const unsigned int maxBits = sizeof(size_t) * 8;

    unsigned int shiftBits = 0;
    if (suffix == 'G')
        shiftBits = 30;
    else if (suffix == 'M')
        shiftBits = 20;
    else if (suffix == 'k')
        shiftBits = 10;

    unsigned int currentBits = 1; // At least one bit
    while ((bytes >> currentBits) > 0)
        currentBits++;

    if (currentBits + shiftBits > maxBits)
    {
        Log::LogE("Value '%s' is too big (maxBits: %u, currentBits: %u, "
                "shiftBits: %u", value.c_str(), maxBits, currentBits, shiftBits);
        return false;
    }

    maxStackSize <<= shiftBits;

    return SetMaxStackSize(maxStackSize);
}

bool pm::Settings::HandleTrackLinkFrames(const std::string& value)
{
    uint16_t frames;
    if (!Utils::StrToNumber<uint16_t>(value, frames))
        return false;

    return SetTrackLinkFrames(frames);
}

bool pm::Settings::HandleTrackMaxDistance(const std::string& value)
{
    uint16_t maxDist;
    if (!Utils::StrToNumber<uint16_t>(value, maxDist))
        return false;

    return SetTrackMaxDistance(maxDist);
}

bool pm::Settings::HandleTrackCpuOnly(const std::string& value)
{
    bool cpuOnly;
    if (value.empty())
    {
        cpuOnly = true;
    }
    else
    {
        if (!Utils::StrToBool(value, cpuOnly))
            return false;
    }

    return SetTrackCpuOnly(cpuOnly);
}

bool pm::Settings::HandleTrackTrajectory(const std::string& value)
{
    uint16_t duration;
    if (!Utils::StrToNumber<uint16_t>(value, duration))
        return false;

    return SetTrackTrajectoryDuration(duration);
}

bool pm::Settings::HandleColorWbScaleRed(const std::string& value)
{
    float scale;
    if (!Utils::StrToNumber<float>(value, scale))
        return false;

    return SetColorWbScaleRed(scale);
}

bool pm::Settings::HandleColorWbScaleGreen(const std::string& value)
{
    float scale;
    if (!Utils::StrToNumber<float>(value, scale))
        return false;

    return SetColorWbScaleGreen(scale);
}

bool pm::Settings::HandleColorWbScaleBlue(const std::string& value)
{
    float scale;
    if (!Utils::StrToNumber<float>(value, scale))
        return false;

    return SetColorWbScaleBlue(scale);
}

bool pm::Settings::HandleColorDebayerAlgorithm(const std::string& value)
{
    int32_t alg;
    if (!Utils::StrToNumber<int32_t>(value, alg))
    {
        if (value == "nearest")
            alg = PH_COLOR_DEBAYER_ALG_NEAREST;
        else if (value == "bilinear")
            alg = PH_COLOR_DEBAYER_ALG_BILINEAR;
        //else if (value == "bicubic")
        //    alg = PH_COLOR_DEBAYER_ALG_BICUBIC;
        else
            return false;
    }

    return SetColorDebayerAlgorithm(alg);
}

bool pm::Settings::HandleColorCpuOnly(const std::string& value)
{
    bool cpuOnly;
    if (value.empty())
    {
        cpuOnly = true;
    }
    else
    {
        if (!Utils::StrToBool(value, cpuOnly))
            return false;
    }

    return SetColorCpuOnly(cpuOnly);
}
