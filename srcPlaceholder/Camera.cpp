/******************************************************************************/
/* Copyright (C) Teledyne Photometrics. All rights reserved.                  */
/******************************************************************************/
#include "backend/Camera.h"

/* Local */
#include "backend/AllocatorFactory.h"
#include "backend/ColorRuntimeLoader.h"
#include "backend/exceptions/CameraException.h"
#include "backend/Frame.h"
#include "backend/Log.h"
#include "backend/ParamInfoMap.h"
#include "backend/Params.h"
#include "backend/Utils.h"

/* System */
#include <algorithm>
#include <limits>
#include <sstream>

pm::Camera::Camera()
{
}

pm::Camera::~Camera()
{
}

bool pm::Camera::AddCliOptions(OptionController& controller, bool fixUserInput)
{
    if (!controller.AddOption(Option(
            { "--clear-cycles" },
            { "count" },
            { "<camera default>" },
            "Number of clear cycles.",
            PARAM_CLEAR_CYCLES,
            std::bind(&Camera::HandleClearCycles, this, std::placeholders::_1))))
        return false;

    if (!controller.AddOption(Option(
            { "--clear-mode" },
            { "mode" },
            { "<camera default>" },
            "Clear mode used for sensor clearing during acquisition.\n"
            "Supported values are : 'auto', 'never', 'pre-exp', 'pre-seq', 'post-seq',\n"
            "'pre-post-seq' and 'pre-exp-post-seq'.",
            PARAM_CLEAR_MODE,
            std::bind(&Camera::HandleClearMode, this, std::placeholders::_1))))
        return false;

    if (!controller.AddOption(Option(
            { "--p-mode", "--pmode" },
            { "mode" },
            { "<camera default>" },
            "Parallel clocking mode used for sensor.\n"
            "Supported values are : 'normal', 'ft', 'mpp', 'ft-mpp', 'alt-normal',\n"
            "'alt-ft', 'alt-mpp' and 'alt-ft-mpp'.\n"
            "Modes with 'ft' in name are supported on frame-transfer capable cameras only.\n"
            "Modes with 'mpp' in name are supported on MPP sensors only.\n"
            "Although the default value is 'normal', on frame-transfer cameras it should \n"
            "be 'ft' by default. Let's hope it won't cause problems.",
            PARAM_PMODE,
            std::bind(&Camera::HandlePMode, this, std::placeholders::_1))))
        return false;

    if (!controller.AddOption(Option(
            { "--port" },
            { "port" },
            { "<camera default>" },
            "Port value as reported by camera. The readout port is an enumeration\n"
            "thus the value of enum item at some index doesn't need to be equal to index.\n"
            "The default value is taken from the first enum item at index 0.",
            PARAM_READOUT_PORT,
            std::bind(&Camera::HandlePort, this, std::placeholders::_1))))
        return false;

    if (!controller.AddOption(Option(
            { "--speed-index" },
            { "index" },
            { "<camera default>" },
            "Speed index (first is 0).",
            PARAM_SPDTAB_INDEX,
            std::bind(&Camera::HandleSpeedIndex, this, std::placeholders::_1))))
        return false;

    if (!controller.AddOption(Option(
            { "--gain-index" },
            { "index" },
            { "<camera default>" },
            "Gain index (first is 1).",
            PARAM_GAIN_INDEX,
            std::bind(&Camera::HandleGainIndex, this, std::placeholders::_1))))
        return false;

    if (!controller.AddOption(Option(
            { "--gain-mult-factor", "--em-gain" },
            { "gain" },
            { "<camera default>" },
            "Gain multiplication factor for EM CCD cameras (lowest value is 1).",
            PARAM_GAIN_MULT_FACTOR,
            std::bind(&Camera::HandleEmGain, this, std::placeholders::_1))))
        return false;

    if (!controller.AddOption(Option(
            { "--metadata-enabled", "--use-metadata" },
            { "" },
            { "<camera default>" },
            "If camera supports frame metadata use it even if not needed.\n"
            "Application may silently override this value when metadata is needed,\n"
            "for instance multiple regions or centroids.",
            PARAM_METADATA_ENABLED,
            std::bind(&Camera::HandleMetadataEnabled,
                    this, std::placeholders::_1))))
        return false;

    if (!controller.AddOption(Option(
            { "--centroids-enabled", "--use-centroids" },
            { "" },
            { "<camera default>" },
            "Turns on the centroids feature.\n"
            "This feature can be used with up to one region only.",
            PARAM_CENTROIDS_ENABLED,
            std::bind(&Camera::HandleCentroidsEnabled,
                    this, std::placeholders::_1))))
        return false;

    if (!controller.AddOption(Option(
            { "--centroids-radius" },
            { "radius" },
            { "<camera default>" },
            "Specifies the radius of all centroids.",
            PARAM_CENTROIDS_RADIUS,
            std::bind(&Camera::HandleCentroidsRadius,
                    this, std::placeholders::_1))))
        return false;

    if (!controller.AddOption(Option(
            { "--centroids-count" },
            { "count" },
            { "<camera default>" },
            "Requests camera to find given number of centroids.\n"
            "Application may override this value if it is greater than max. number of\n"
            "supported centroids.",
            PARAM_CENTROIDS_COUNT,
            std::bind(&Camera::HandleCentroidsCount,
                    this, std::placeholders::_1))))
        return false;

    if (!controller.AddOption(Option(
            { "--centroids-mode" },
            { "mode" },
            { "<camera default>" },
            "Small objects can be either located only or tracked across frames.\n"
            "Supported values are : 'locate', 'track' and 'blob'.",
            PARAM_CENTROIDS_MODE,
            std::bind(&Camera::HandleCentroidsMode,
                    this, std::placeholders::_1))))
        return false;

    if (!controller.AddOption(Option(
            { "--centroids-bg-count" },
            { "frames" },
            { "<camera default>" },
            "Sets number of frames used for dynamic background removal.",
            PARAM_CENTROIDS_BG_COUNT,
            std::bind(&Camera::HandleCentroidsBgCount,
                    this, std::placeholders::_1))))
        return false;

    if (!controller.AddOption(Option(
            { "--centroids-threshold" },
            { "multiplier" },
            { "<camera default>" },
            "Sets a threshold multiplier. It is a fixed-point real number in format Q8.4.\n"
            "E.g. the value 1234 (0x4D2) means 77.2 (0x4D hex = 77 dec).",
            PARAM_CENTROIDS_THRESHOLD,
            std::bind(&Camera::HandleCentroidsThreshold,
                    this, std::placeholders::_1))))
        return false;

    if (!controller.AddOption(Option(
            { "--trigtab-signal" },
            { "signal" },
            { "<camera default>" },
            "The output signal with embedded multiplexer forwarding chosen signal\n"
            "to multiple output wires (set via --last-muxed-signal).\n"
            "Supported values are : 'expose-out'.",
            PARAM_TRIGTAB_SIGNAL,
            std::bind(&Camera::HandleTrigTabSignal,
                    this, std::placeholders::_1))))
        return false;

    if (!controller.AddOption(Option(
            { "--last-muxed-signal" },
            { "number" },
            { "<camera default>" },
            "Number of multiplexed output wires for chosen signal (set via --trigtab-signal).",
            PARAM_LAST_MUXED_SIGNAL,
            std::bind(&Camera::HandleLastMuxedSignal,
                    this, std::placeholders::_1))))
        return false;

    m_fixCliOptions = fixUserInput;

    return true;
}

bool pm::Camera::ReviseSettings(Settings& settings,
        const OptionController& optionController, bool fixUserInput)
{
    if (!m_isOpen)
        return false;

    // Prepare lookup map for CLI options overridden by user
    std::map<uint32_t, const Option*> optionMap; // Key is Option::id
    for (const auto& option : optionController.GetAllProcessedOptions())
    {
        optionMap[option.GetId()] = &option;
    }

    auto IsOverriden = [&optionMap](uint32_t id) {
        return (optionMap.count(id) > 0);
    };

    auto ShouldFix = [&IsOverriden, &optionMap, fixUserInput](uint32_t id) {
        return fixUserInput || !IsOverriden(id);
    };

    // A bit different handling, due to legacy and new modes a fix is forced
    {
        const bool found = IsOverriden(PARAM_EXPOSURE_MODE)
            && m_params->Get<PARAM_EXPOSURE_MODE>()->HasValue(settings.GetTrigMode());
        if (!found)
        {
            const auto defVal = m_params->Get<PARAM_EXPOSURE_MODE>()->GetDef();
            if (IsOverriden(PARAM_EXPOSURE_MODE))
            {
                Log::LogW("Fixing triggering mode from %d to default %d",
                        settings.GetTrigMode(), defVal);
            }
            settings.SetTrigMode(defVal);
        }
    }

    const auto hasExpOutModes = m_params->Get<PARAM_EXPOSE_OUT_MODE>()->IsAvail();
    if (hasExpOutModes && ShouldFix(PARAM_EXPOSE_OUT_MODE))
    {
        const bool found = IsOverriden(PARAM_EXPOSE_OUT_MODE)
            && m_params->Get<PARAM_EXPOSE_OUT_MODE>()->HasValue(settings.GetExpOutMode());
        if (!found)
        {
            const auto defVal = m_params->Get<PARAM_EXPOSE_OUT_MODE>()->GetDef();
            if (IsOverriden(PARAM_EXPOSE_OUT_MODE))
            {
                Log::LogW("Fixing expose out mode from %d to default %d",
                        settings.GetExpOutMode(), defVal);
            }
            settings.SetExpOutMode(defVal);
        }
    }

    // Parameter can be overridden only together with exposure time
    if (ShouldFix(static_cast<uint32_t>(OptionId::Exposure)))
    {
        const auto val = (uint16_t)settings.GetExposureResolution();
        uint16_t validVal = EXP_RES_ONE_MILLISEC;
        auto param = m_params->Get<PARAM_EXP_RES_INDEX>();
        if (param->IsAvail())
        {
            validVal = (val >= param->GetMin() && val <= param->GetMax())
                ? val : param->GetDef();
        }
        if (IsOverriden(static_cast<uint32_t>(OptionId::Exposure))
                && val != validVal)
        {
            Log::LogW("Fixing exposure resolution from %d to default %d",
                    val, validVal);
        }
        settings.SetExposureResolution(validVal);
        if (param->IsAvail())
        {
            param->SetCur(validVal);
        }
        // TODO: Validate exposure time with min/max limits
    }

    if (m_params->Get<PARAM_BINNING_SER>()->IsAvail()
            && m_params->Get<PARAM_BINNING_PAR>()->IsAvail()
            && (fixUserInput
                || (!IsOverriden(PARAM_BINNING_SER)
                    && !IsOverriden(PARAM_BINNING_PAR))))
    {
        bool found = false;
        if (IsOverriden(PARAM_BINNING_SER) || IsOverriden(PARAM_BINNING_PAR))
        {
            const auto& binSerItems = m_params->Get<PARAM_BINNING_SER>()->GetItems();
            const auto& binParItems = m_params->Get<PARAM_BINNING_PAR>()->GetItems();
            if (binSerItems.size() != binParItems.size())
            {
                Log::LogE("Number of serial and parallel binning factors does not match");
                return false;
            }
            for (size_t n = 0; n < binSerItems.size(); n++)
            {
                if (binSerItems.at(n).GetValue() == settings.GetBinningSerial()
                        && binParItems.at(n).GetValue() == settings.GetBinningParallel())
                {
                    found = true;
                    break;
                }
            }
        }
        if (!found)
        {
            const auto defSerVal =
                static_cast<uint16_t>(m_params->Get<PARAM_BINNING_SER>()->GetDef());
            const auto defParVal =
                static_cast<uint16_t>(m_params->Get<PARAM_BINNING_PAR>()->GetDef());
            if (IsOverriden(PARAM_BINNING_SER) || IsOverriden(PARAM_BINNING_PAR))
            {
                Log::LogW("Fixing binning from %hux%hu to default %hux%hu",
                        settings.GetBinningSerial(), settings.GetBinningParallel(),
                        defSerVal, defParVal);
            }
            settings.SetBinningSerial(defSerVal);
            settings.SetBinningParallel(defParVal);
        }
    }

    // Older PVCAMs don't have this parameter yet, otherwise it's always available
    const uint16_t regionCountMax = (m_params->Get<PARAM_ROI_COUNT>()->IsAvail())
        ? m_params->Get<PARAM_ROI_COUNT>()->GetMax()
        : 1;

    std::vector<rgn_type> regions = settings.GetRegions();
    if (regions.size() > regionCountMax)
    {
        if (ShouldFix(static_cast<uint32_t>(OptionId::Regions)))
        {
            if (IsOverriden(static_cast<uint32_t>(OptionId::Regions)))
            {
                Log::LogW("Unable to use all %zu regions, camera supports only %hu",
                        regions.size(), regionCountMax);
            }
            regions.erase(regions.begin() + regionCountMax, regions.end());
            // Cannot fail, remaining regions already were in settings so are valid
            settings.SetRegions(regions);
        }
        else
        {
            Log::LogE("Unable to use %zu regions, camera supports only %hu",
                    regions.size(), regionCountMax);
            return false;
        }
    }

    // Enforcing frame metadata usage when needed, do not fail
    if (m_params->Get<PARAM_METADATA_ENABLED>()->IsAvail()
            && !m_params->Get<PARAM_METADATA_ENABLED>()->GetCur())
    {
        if (settings.GetRegions().size() > 1)
        {
            Log::LogW("Enforcing frame metadata usage with multiple regions");
            m_params->Get<PARAM_METADATA_ENABLED>()->SetCur(true);
        }
    }

    // Print some info about camera

    if (m_params->Get<PARAM_PRODUCT_NAME>()->IsAvail())
    {
        const auto name = m_params->Get<PARAM_PRODUCT_NAME>()->GetCur();
        Log::LogI("Product: '%s'", name);
    }

    const auto width = m_params->Get<PARAM_SER_SIZE>()->GetCur();
    const auto height = m_params->Get<PARAM_PAR_SIZE>()->GetCur();
    Log::LogI("Sensor resolution: %hux%hu px", width, height);

    if (m_params->Get<PARAM_CHIP_NAME>()->IsAvail())
    {
        const auto chipName = m_params->Get<PARAM_CHIP_NAME>()->GetCur();
        Log::LogI("Sensor name: '%s'", chipName);
    }
    else
    {
        Log::LogW("Sensor name: NOT SUPPORTED");
    }

    if (m_params->Get<PARAM_HEAD_SER_NUM_ALPHA>()->IsAvail())
    {
        const auto serNum = m_params->Get<PARAM_HEAD_SER_NUM_ALPHA>()->GetCur();
        Log::LogI("Serial number: '%s'", serNum);
    }
    else
    {
        Log::LogW("Serial number: NOT SUPPORTED");
    }

    if (m_params->Get<PARAM_CAM_INTERFACE_TYPE>()->IsAvail())
    {
        const auto curType = m_params->Get<PARAM_CAM_INTERFACE_TYPE>()->GetCur();
        const auto& curTypeName =
            m_params->Get<PARAM_CAM_INTERFACE_TYPE>()->GetValueName(curType);
        Log::LogI("Interface type: '%s'", curTypeName.c_str());
    }

    if (m_params->Get<PARAM_CAM_INTERFACE_MODE>()->IsAvail())
    {
        const auto curIfcMode =
            m_params->Get<PARAM_CAM_INTERFACE_MODE>()->GetCur();
        if (curIfcMode != PL_CAM_IFC_MODE_IMAGING)
        {
            Log::LogE("Current interface mode is not sufficient for imaging");
            return false;
        }
    }

    // For monochromatic cameras the parameter might not be available
    const auto colorMask = m_params->Get<PARAM_COLOR_MODE>()->IsAvail()
        ? m_params->Get<PARAM_COLOR_MODE>()->GetCur()
        : COLOR_NONE;
    static const std::map<int32, const char*> colorMaskNameMap({
        { COLOR_NONE, "None" },
        { COLOR_RGGB, "RGGB" },
        { COLOR_GRBG, "GRBG" },
        { COLOR_GBRG, "GBRG" },
        { COLOR_BGGR, "BGGR" },
    });
    if (colorMaskNameMap.count(colorMask))
    {
        Log::LogI("Color mask: %s", colorMaskNameMap.at(colorMask));

        if (colorMask != COLOR_NONE && !PH_COLOR)
        {
            Log::LogW("Color camera detected but pvcam_helper_color library not found, "
                    "debayering won't be possible");
        }
    }
    else
    {
        Log::LogW("Color mask: UNKNOWN");
    }

    // Add extra line to separate output
#if 0 // Using PARAM_CAM_SYSTEMS_INFO with other parameters hangs new Retiga cameras
    // We can fallback to show FW version only
    if (m_params->GetCamSystemsInfo()->IsAvail())
    {
        const auto sysInfo = m_params->GetCamSystemsInfo()->GetCur();
        Log::LogI("Camera systems info:\n%s\n", sysInfo);
    }
    else
#endif
    {
        const auto fwVer = m_params->Get<PARAM_CAM_FW_VERSION>()->GetCur();
        Log::LogI("Firmware version: %hu.%hu\n", (fwVer >> 8) & 0xFF, fwVer & 0xFF);
    }

    return true;
}

bool pm::Camera::Open(const std::string& /*name*/,
        CallbackEx3Fn removeCallbackHandler,
        void* removeCallbackContext)
{
    try
    {
        BuildSpeedTable();
    }
    catch (const Exception& ex)
    {
        Log::LogE("Failure building speed table - %s", ex.what());
        return false;
    }

#if 0
    static const std::vector<uint32_t> skipIds{
        // Rather don't touch driver-related params
        PARAM_DD_RETRIES,
        PARAM_DD_TIMEOUT,
        // Forbidden and PVCAM has hard-coded default value that differs from current
        PARAM_ADC_OFFSET,
        // Rather skip, PVCAM has hard-coded default value that differs from current
        PARAM_PREAMP_OFF_CONTROL,
        // Keep what user set in other applications
        PARAM_TEMP_SETPOINT,
        PARAM_FAN_SPEED_SETPOINT,
        // There is no def. value for Smart Streaming params
        PARAM_SMART_STREAM_EXP_PARAMS,
        PARAM_SMART_STREAM_DLY_PARAMS,
        // PARAM_EXP_RES_INDEX takes care
        PARAM_EXP_RES,
        // Skip all post-processing params, there is pl_pp_reset() for that
        PARAM_PP_INDEX,
        PARAM_PP_PARAM_INDEX,
        PARAM_PP_PARAM,
    };
    const auto& params = m_params->GetParams();
    for (auto id : ParamInfoMap::GetSortedParamIds())
    {
        if (skipIds.cend() != std::find(skipIds.cbegin(), skipIds.cend(), id))
            continue;
        try
        {
            auto param = params.at(id).get();
            try
            {
                if (!param->IsAvail() || param->GetAccess() != ACC_READ_WRITE)
                    continue;
            }
            catch (const Exception&)
            {
                continue;
            }
            auto def = param->GetDefValue();
            param->SetCurValue(def);
        }
        catch (const Exception& ex)
        {
            Log::LogW("Failure setting %s to default value, ignoring (%s)",
                    ParamInfoMap::GetParamInfo(id).GetName().c_str(),
                    ex.what());
        }
    }
#endif

    m_removeCallbackHandler = removeCallbackHandler;
    m_removeCallbackContext = removeCallbackContext;

    m_isOpen = true;
    return true;
}

bool pm::Camera::Close()
{
    m_ports.clear();

    m_removeCallbackHandler = nullptr;
    m_removeCallbackContext = nullptr;

    m_isOpen = false;
    return true;
}

bool pm::Camera::SetupExp(const SettingsReader& settings)
{
    m_settings = settings;

    // Update cached params
    m_usesMetadata = m_params->Get<PARAM_METADATA_ENABLED>()->IsAvail()
        && m_params->Get<PARAM_METADATA_ENABLED>()->GetCur();
    m_usesCentroids = m_usesMetadata
        && m_params->Get<PARAM_CENTROIDS_ENABLED>()->IsAvail()
        && m_params->Get<PARAM_CENTROIDS_ENABLED>()->GetCur();
    m_centroidsMode = m_params->Get<PARAM_CENTROIDS_MODE>()->IsAvail()
        ? m_params->Get<PARAM_CENTROIDS_MODE>()->GetCur()
        : PL_CENTROIDS_MODE_LOCATE;
    m_centroidsCount = m_params->Get<PARAM_CENTROIDS_COUNT>()->IsAvail()
        ? m_params->Get<PARAM_CENTROIDS_COUNT>()->GetCur()
        : 0;
    m_centroidsRadius = m_params->Get<PARAM_CENTROIDS_RADIUS>()->IsAvail()
        ? m_params->Get<PARAM_CENTROIDS_RADIUS>()->GetCur()
        : 0;

    auto paramSsEn = m_params->Get<PARAM_SMART_STREAM_MODE_ENABLED>();
    auto paramSsExps = m_params->Get<PARAM_SMART_STREAM_EXP_PARAMS>();
    const bool usesSmartStreaming =
        paramSsEn->IsAvail() && paramSsEn->GetCur() && paramSsExps->IsAvail();
    if (usesSmartStreaming)
    {
        const auto ssExps = paramSsExps->GetCur();
        m_smartExposures.assign(ssExps->params, ssExps->params + ssExps->entries);
    }
    else
    {
        m_smartExposures.clear();
    }

    // Older PVCAMs don't have this parameter yet, otherwise it's always available
    const uint16_t regionCountMax = (m_params->Get<PARAM_ROI_COUNT>()->IsAvail())
        ? m_params->Get<PARAM_ROI_COUNT>()->GetMax()
        : 1;

    if (m_settings.GetRegions().size() > regionCountMax
            || m_settings.GetRegions().empty())
    {
        Log::LogE("Invalid number of regions (%zu)",
               m_settings.GetRegions().size());
        return false;
    }

    const uns32 acqFrameCount = m_settings.GetAcqFrameCount();
    const uns32 bufferFrameCount = m_settings.GetBufferFrameCount();
    const AcqMode acqMode = m_settings.GetAcqMode();
    const int32 trigMode = m_settings.GetTrigMode();

    if (acqMode == AcqMode::SnapSequence && acqFrameCount > bufferFrameCount)
    {
        Log::LogE("When in snap sequence mode, "
                "we cannot acquire more frames than the buffer size (%u)",
                bufferFrameCount);
        return false;
    }

    if ((acqMode == AcqMode::LiveCircBuffer || acqMode == AcqMode::LiveTimeLapse)
            && m_settings.GetStorageType() != StorageType::None
            && m_settings.GetSaveLast() > 0)
    {
        Log::LogE("When in live mode, we cannot save last N frames");
        return false;
    }

    if (acqMode != AcqMode::SnapTimeLapse && acqMode != AcqMode::LiveTimeLapse
            && trigMode == VARIABLE_TIMED_MODE)
    {
        Log::LogE("'Variable Timed' mode works in time-lapse modes only");
        return false;
    }

    const ParamEnum::T colorMask =
        m_params->Get<PARAM_COLOR_MODE>()->IsAvail()
        ? m_params->Get<PARAM_COLOR_MODE>()->GetCur()
        : COLOR_NONE;
    const ParamEnum::T imageFormat =
        m_params->Get<PARAM_IMAGE_FORMAT>()->IsAvail()
        ? m_params->Get<PARAM_IMAGE_FORMAT>()->GetCur()
        : PL_IMAGE_FORMAT_MONO16;
    const auto bitDepth = m_params->Get<PARAM_BIT_DEPTH>()->GetCur();

    m_bmpFormat.SetColorMask(static_cast<BayerPattern>(colorMask));
    m_bmpFormat.SetImageFormat(static_cast<ImageFormat>(imageFormat));
    m_bmpFormat.SetBitDepth(static_cast<uint16_t>(bitDepth));

    // Setup the acquisition and call AllocateBuffer in derived class

    return true;
}

std::shared_ptr<pm::Frame> pm::Camera::GetFrameAt(size_t index) const
{
    if (index >= m_frames.size())
    {
        Log::LogD("Frame index out of buffer boundaries");
        return nullptr;
    }

    return m_frames.at(index);
}

bool pm::Camera::GetFrameIndex(const Frame& frame, size_t& index) const
{
    const uint32_t frameNr = frame.GetInfo().GetFrameNr();

    auto it = m_framesMap.find(frameNr);
    if (it != m_framesMap.end())
    {
        index = it->second;
        return true;
    }

    return false;
}

uint32_t pm::Camera::GetFrameExpTime(uint32_t frameNr) const
{
    if (m_settings.GetTrigMode() == VARIABLE_TIMED_MODE)
    {
        const auto& vtmExposures = m_settings.GetVtmExposures();
        // frameNr is 1-based, not 0-based
        const uint32_t vtmExpIndex = (frameNr - 1) % vtmExposures.size();
        const uint16_t vtmExpTime = vtmExposures.at(vtmExpIndex);
        return static_cast<uint32_t>(vtmExpTime);
    }
    else if (!m_smartExposures.empty())
    {
        const uint32_t ssExpIndex = (frameNr - 1) % m_smartExposures.size();
        return m_smartExposures.at(ssExpIndex);
    }

    return m_settings.GetExposure();
}

void pm::Camera::UpdateFrameIndexMap(uint32_t oldFrameNr, size_t index) const
{
    m_framesMap.erase(oldFrameNr);

    if (index >= m_frames.size())
        return;
    const uint32_t frameNr = m_frames.at(index)->GetInfo().GetFrameNr();
    m_framesMap[frameNr] = index;
}

void pm::Camera::BuildSpeedTable()
{
    m_ports.clear();

    if (!m_params->Get<PARAM_READOUT_PORT>()->IsAvail())
    {
        throw CameraException("Readout ports not available", this);
    }
    if (!m_params->Get<PARAM_SPDTAB_INDEX>()->IsAvail())
    {
        throw CameraException("Speed indexes not available", this);
    }
    if (!m_params->Get<PARAM_GAIN_INDEX>()->IsAvail())
    {
        throw CameraException("Gain indexes not available", this);
    }
    if (!m_params->Get<PARAM_BIT_DEPTH>()->IsAvail())
    {
        throw CameraException("Bit depth not available", this);
    }
    if (!m_params->Get<PARAM_PIX_TIME>()->IsAvail())
    {
        throw CameraException("Pixel time not available", this);
    }

    const auto& portItems = m_params->Get<PARAM_READOUT_PORT>()->GetItems();
    for (auto& portItem : portItems)
    {
        m_params->Get<PARAM_READOUT_PORT>()->SetCur(portItem.GetValue());

        std::vector<Port::Speed> speeds;

        const auto speedIndexMin = m_params->Get<PARAM_SPDTAB_INDEX>()->GetMin();
        const auto speedIndexMax = m_params->Get<PARAM_SPDTAB_INDEX>()->GetMax();
        auto speedIndexInc = m_params->Get<PARAM_SPDTAB_INDEX>()->GetInc();
        if (speedIndexInc == 0) // Just in case
            speedIndexInc = 1;
        for (auto speedIndex = speedIndexMin; speedIndex <= speedIndexMax;
                speedIndex += speedIndexInc)
        {
            m_params->Get<PARAM_SPDTAB_INDEX>()->SetCur(speedIndex);

            std::vector<Port::Speed::Gain> gains;

            const auto pixTimeNs = m_params->Get<PARAM_PIX_TIME>()->GetCur();

            std::string speedName = std::to_string(speedIndex) + ": ";
            if (m_params->Get<PARAM_SPDTAB_NAME>()->IsAvail())
            {
                speedName += m_params->Get<PARAM_SPDTAB_NAME>()->GetCur();
            }
            else
            {
                const double mhz = (pixTimeNs != 0) ? 1000.0 / pixTimeNs : 0.0;
                // Use stream to format double without trailing zeros
                std::ostringstream ss;
                ss << mhz;
                speedName += ss.str() + " MHz";
            }

            const auto gainIndexMin = m_params->Get<PARAM_GAIN_INDEX>()->GetMin();
            const auto gainIndexMax = m_params->Get<PARAM_GAIN_INDEX>()->GetMax();
            auto gainIndexInc = m_params->Get<PARAM_GAIN_INDEX>()->GetInc();
            if (gainIndexInc == 0) // Happens with S477 on PVCAM 2.9.3.4
                gainIndexInc = 1;
            for (auto gainIndex = gainIndexMin; gainIndex <= gainIndexMax;
                    gainIndex += gainIndexInc)
            {
                m_params->Get<PARAM_GAIN_INDEX>()->SetCur(gainIndex);

                Port::Speed::Gain gain;
                gain.index = gainIndex;
                gain.name = (m_params->Get<PARAM_GAIN_NAME>()->IsAvail())
                    ? m_params->Get<PARAM_GAIN_NAME>()->GetCur() : "";
                gain.bitDepth =
                    static_cast<uint16_t>(m_params->Get<PARAM_BIT_DEPTH>()->GetCur());
                gain.label = std::to_string(gainIndex) + ": "
                    + ((!gain.name.empty()) ? gain.name : "<unnamed>")
                    + " (" + std::to_string(gain.bitDepth) + "bit)";
                gains.push_back(gain);
            }

            Port::Speed speed;
            speed.index = speedIndex;
            speed.pixTimeNs = pixTimeNs;
            speed.gains = gains;
            speed.label = speedName;
            speeds.push_back(speed);
        }

        Port port;
        port.item = portItem;
        port.speeds = speeds;
        port.label = std::to_string(port.item.GetValue()) + ": " + port.item.GetName();
        m_ports.push_back(port);
    }

    // Set camera-default port, speed and gain
    // It could be overridden by CLI options (processed later)
    const auto portDef = m_params->Get<PARAM_READOUT_PORT>()->GetDef();
    m_params->Get<PARAM_READOUT_PORT>()->SetCur(portDef);
    const auto speedIndexDef = m_params->Get<PARAM_SPDTAB_INDEX>()->GetDef();
    m_params->Get<PARAM_SPDTAB_INDEX>()->SetCur(speedIndexDef);
    const auto gainIndexDef = m_params->Get<PARAM_GAIN_INDEX>()->GetDef();
    m_params->Get<PARAM_GAIN_INDEX>()->SetCur(gainIndexDef);
}

bool pm::Camera::AllocateBuffers(uns32 frameCount, uns32 frameBytes)
{
    const auto& regions = m_settings.GetRegions();
    const rgn_type implRoi = SettingsReader::GetImpliedRegion(regions);
    uint16_t roiCount;
    std::vector<rgn_type> outputBmpRois;
    if (!m_usesCentroids)
    {
        roiCount = static_cast<uint16_t>(regions.size());
        outputBmpRois = regions;
    }
    else
    {
        roiCount = m_centroidsCount;

        const bool hasFullBgImage =
            m_centroidsMode == PL_CENTROIDS_MODE_TRACK
            || m_centroidsMode == PL_CENTROIDS_MODE_BLOB;
        if (hasFullBgImage)
        {
            roiCount++; // One more ROI for background image sent with particles
            outputBmpRois = regions; // PVCAM allows one region only
        }
        else
        {
            const uint16_t last = m_centroidsRadius * 2/* + 1*/;
            const uint16_t sbin = m_settings.GetBinningSerial();
            const uint16_t pbin = m_settings.GetBinningParallel();
            for (uint16_t n = 0; n < m_centroidsCount; ++n)
            {
                outputBmpRois.push_back({ 0, last, sbin, 0, last, pbin });
            }
        }
    }

    const auto allocatorType = m_settings.GetAllocatorType();
    const Frame::AcqCfg frameAcqCfg(frameBytes, roiCount, m_usesMetadata,
            implRoi, m_bmpFormat, outputBmpRois, allocatorType);

    if (m_frameCount == frameCount
            && m_frameAcqCfg == frameAcqCfg
            && m_buffer)
        return true;

    DeleteBuffers();

    auto allocator = AllocatorFactory::Create(allocatorType);
    if (!allocator)
    {
        Log::LogE("Failure allocating memory allocator");
        return false;
    }

    const size_t bufferBytes = frameCount * frameBytes;

    if (bufferBytes == 0)
    {
        Log::LogE("Invalid buffer size (0 bytes)");
        return false;
    }

    try
    {
        /*
        HACK: THIS IS VERY DIRTY HACK!!!
        Because of heap corruption that occurs at least with PCIe cameras
        and ROI having position and size with odd numbers, we allocate here
        additional 16 bytes.
        Example rgn_type could be [123,881,1,135,491,1].
        During long investigation I've seen 2, 4 or 6 bytes behind m_buffer
        are always filled with value 0x1c coming probably from PCIe
        driver.
        */
        const size_t bufferBytesSafe = bufferBytes + 16;

        /*
        We will allocate whole camera buffer aligned according to current
        allocator. Remember, whole buffer is aligned, not each frame.
        In case the real size of one frame is less than its aligned size,
        the frame padding contains possibly yet invalid data from beginning
        of the next frame. That's not a problem but it helps to keep code
        related to optimized/non-buffered streaming relatively simple,
        especially when it comes to last frame in the buffer.
        */
        const auto bufferBytesSafeAligned =
            AllocatorFactory::GetAlignedSize(bufferBytesSafe, allocatorType);

        m_buffer = std::make_unique<uns8[]>(bufferBytesSafeAligned);
    }
    catch (...)
    {
        m_buffer = nullptr;
        Log::LogE("Failure allocating buffer with %zu bytes", bufferBytes);
        return false;
    }

    m_frames.reserve(frameCount);
    for (uns32 n = 0; n < frameCount; n++)
    {
        std::shared_ptr<pm::Frame> frame;

        try
        {
            // Always shallow copy on top of circular buffer
            frame = std::make_shared<Frame>(frameAcqCfg, false, allocator);
        }
        catch (...)
        {
            DeleteBuffers();
            Log::LogE("Failure allocating shallow frame %u copy", n);
            return false;
        }

        // Bind frames to internal buffer
        void* data = m_buffer.get() + n * frameBytes;
        frame->SetDataPointer(data);
        // On shallow copy it does some checks only, not deep copy
        if (!frame->CopyData())
        {
            DeleteBuffers();
            return false;
        }
        frame->OverrideValidity(false); // Force frame to be invalid on start

        m_frames.push_back(frame);
    }
    m_frames.shrink_to_fit();

    m_frameAcqCfg = frameAcqCfg;
    m_allocator = allocator;
    m_frameCount = frameCount;

    return true;
}

void pm::Camera::DeleteBuffers()
{
    m_frames.clear();
    m_framesMap.clear();

    m_buffer = nullptr;

    m_frameAcqCfg = Frame::AcqCfg();
    m_allocator = nullptr;
    m_frameCount = 0;
}

void pm::Camera::InvokeAfterSetupParamChangeHandlers()
{
    const auto& params = m_params->GetParams();
    const auto& paramInfoMap = ParamInfoMap::GetMap();
    for (const auto& paramInfoPair : paramInfoMap)
    {
        const auto id = paramInfoPair.first;
        const auto& paramInfo = paramInfoPair.second;

        if (!paramInfo.NeedsSetup())
            continue;

        params.at(id)->ResetCacheRangeFlags();
        params.at(id)->InvokeChangeHandlers();
    }
}

const pm::Params& pm::Camera::GetParams() const
{
    assert(m_params);
    return *m_params;
}

bool pm::Camera::HandleClearMode(const std::string& value)
{
    ParamEnum::T clearMode;
    if (!Utils::StrToNumber<ParamEnum::T>(value, clearMode))
    {
        if (value == "never")
            clearMode = CLEAR_NEVER;
        else if (value == "auto")
            clearMode = CLEAR_AUTO;
        else if (value == "pre-exp")
            clearMode = CLEAR_PRE_EXPOSURE;
        else if (value == "pre-seq")
            clearMode = CLEAR_PRE_SEQUENCE;
        else if (value == "post-seq")
            clearMode = CLEAR_POST_SEQUENCE;
        else if (value == "pre-post-seq")
            clearMode = CLEAR_PRE_POST_SEQUENCE;
        else if (value == "pre-exp-post-seq")
            clearMode = CLEAR_PRE_EXPOSURE_POST_SEQ;
        else
            return false;
    }

    auto param = m_params->Get<PARAM_CLEAR_MODE>();

    auto SetValue = [&](auto val) {
        try { param->SetCur(val); }
        catch (const Exception& ex) {
            Log::LogE("Failure setting clearing mode %d - %s", val, ex.what());
            return false;
        }
        return true;
    };

    if (!param->IsAvail())
        return true;
    if (param->HasValue(clearMode) && SetValue(clearMode))
        return true;
    if (!m_fixCliOptions)
        return false;
    const auto def = param->GetDef();
    Log::LogW("Fixing clearing mode from %d to default %d", clearMode, def);
    return SetValue(def);
}

bool pm::Camera::HandleClearCycles(const std::string& value)
{
    uint16_t clearCycles;
    if (!Utils::StrToNumber<uint16_t>(value, clearCycles))
        return false;

    auto param = m_params->Get<PARAM_CLEAR_CYCLES>();

    auto SetValue = [&](auto val) {
        try { param->SetCur(val); }
        catch (const Exception& ex) {
            Log::LogE("Failure setting clearing cycles %hu - %s", val, ex.what());
            return false;
        }
        return true;
    };

    if (!param->IsAvail())
        return true;
    const auto min = param->GetMin();
    const auto max = param->GetMax();
    if (clearCycles >= min && clearCycles <= max && SetValue(clearCycles))
        return true;
    if (!m_fixCliOptions)
        return false;
    const auto def = param->GetDef();
    Log::LogW("Fixing clearing cycles from %hu to default %hu", clearCycles, def);
    return SetValue(def);
}

bool pm::Camera::HandlePMode(const std::string& value)
{
    ParamEnum::T pMode;
    if (!Utils::StrToNumber<ParamEnum::T>(value, pMode))
    {
        if (value == "normal")
            pMode = PMODE_NORMAL;
        else if (value == "ft")
            pMode = PMODE_FT;
        else if (value == "mpp")
            pMode = PMODE_MPP;
        else if (value == "ft-mpp")
            pMode = PMODE_FT_MPP;
        else if (value == "alt-normal")
            pMode = PMODE_ALT_NORMAL;
        else if (value == "alt-ft")
            pMode = PMODE_ALT_FT;
        else if (value == "alt-mpp")
            pMode = PMODE_ALT_MPP;
        else if (value == "alt-ft-mpp")
            pMode = PMODE_ALT_FT_MPP;
        else
            return false;
    }

    auto param = m_params->Get<PARAM_PMODE>();

    auto SetValue = [&](auto val) {
        try { param->SetCur(val); }
        catch (const Exception& ex) {
            Log::LogE("Failure setting parallel clocking mode %d - %s",
                    val, ex.what());
            return false;
        }
        return true;
    };

    if (!param->IsAvail())
        return true;
    if (param->HasValue(pMode) && SetValue(pMode))
        return true;
    if (!m_fixCliOptions)
        return false;
    const auto def = param->GetDef();
    Log::LogW("Fixing parallel clocking mode from %d to default %d", pMode, def);
    return SetValue(def);
}

bool pm::Camera::HandlePort(const std::string& value)
{
    ParamEnum::T port;
    if (!Utils::StrToNumber<ParamEnum::T>(value, port))
        return false;

    auto param = m_params->Get<PARAM_READOUT_PORT>();

    auto SetValue = [&](auto val) {
        try { param->SetCur(val); }
        catch (const Exception& ex) {
            Log::LogE("Failure setting port %d - %s", val, ex.what());
            return false;
        }
        return true;
    };

    if (!param->IsAvail())
        return true;
    const auto min = param->GetMin();
    const auto max = param->GetMax();
    if (param->HasValue(port) && SetValue(port))
        return true;
    if (!m_fixCliOptions)
        return false;
    const auto def = param->GetDef();
    Log::LogW("Fixing port from %d to default %d", port, def);
    return SetValue(def);
}

bool pm::Camera::HandleSpeedIndex(const std::string& value)
{
    int16_t speedIndex;
    if (!Utils::StrToNumber<int16_t>(value, speedIndex))
        return false;

    auto param = m_params->Get<PARAM_SPDTAB_INDEX>();

    auto SetValue = [&](auto val) {
        try { param->SetCur(val); }
        catch (const Exception& ex) {
            Log::LogE("Failure setting speed index %hd - %s", val, ex.what());
            return false;
        }
        return true;
    };

    if (!param->IsAvail())
        return true;
    const auto min = param->GetMin();
    const auto max = param->GetMax();
    if (speedIndex >= min && speedIndex <= max && SetValue(speedIndex))
        return true;
    if (!m_fixCliOptions)
        return false;
    const auto def = param->GetDef();
    Log::LogW("Fixing speed index from %hd to default %hd", speedIndex, def);
    return SetValue(def);
}

bool pm::Camera::HandleGainIndex(const std::string& value)
{
    int16_t gainIndex;
    if (!Utils::StrToNumber<int16_t>(value, gainIndex))
        return false;

    auto param = m_params->Get<PARAM_GAIN_INDEX>();

    auto SetValue = [&](auto val) {
        try { param->SetCur(val); }
        catch (const Exception& ex) {
            Log::LogE("Failure setting gain index %hd - %s", val, ex.what());
            return false;
        }
        return true;
    };

    if (!param->IsAvail())
        return true;
    const auto min = param->GetMin();
    const auto max = param->GetMax();
    if (gainIndex >= min && gainIndex <= max && SetValue(gainIndex))
        return true;
    if (!m_fixCliOptions)
        return false;
    const auto def = param->GetDef();
    Log::LogW("Fixing gain index from %hd to default %hd", gainIndex, def);
    return SetValue(def);
}

bool pm::Camera::HandleEmGain(const std::string& value)
{
    uint16_t emGain;
    if (!Utils::StrToNumber<uint16_t>(value, emGain))
        return false;

    auto param = m_params->Get<PARAM_GAIN_MULT_FACTOR>();

    auto SetValue = [&](auto val) {
        try { param->SetCur(val); }
        catch (const Exception& ex) {
            Log::LogE("Failure setting EM gain %hu - %s", val, ex.what());
            return false;
        }
        return true;
    };

    if (!param->IsAvail())
        return true;
    const auto min = param->GetMin();
    const auto max = param->GetMax();
    if (emGain >= min && emGain <= max && SetValue(emGain))
        return true;
    if (!m_fixCliOptions)
        return false;
    const auto def = param->GetDef();
    Log::LogW("Fixing EM gain from %hu to default %hu", emGain, def);
    return SetValue(def);
}

bool pm::Camera::HandleMetadataEnabled(const std::string& value)
{
    bool enabled;
    if (value.empty())
        enabled = true;
    else if (!Utils::StrToBool(value, enabled))
        return false;

    auto param = m_params->Get<PARAM_METADATA_ENABLED>();

    auto SetValue = [&](auto val) {
        try { param->SetCur(val); }
        catch (const Exception& ex) {
            Log::LogE("Failure %s frame metadata - %s",
                    (val) ? "enabling" : "disabling", ex.what());
            return false;
        }
        return true;
    };

    if (!param->IsAvail())
        return true;
    if (SetValue(enabled))
        return true;
    if (!m_fixCliOptions)
        return false;
    const auto def = param->GetDef();
    Log::LogW("Fixing metadata enabled state from %s to default %s",
            (enabled) ? "true" : "false", (def) ? "true" : "false");
    return SetValue(def);
}

bool pm::Camera::HandleCentroidsEnabled(const std::string& value)
{
    bool enabled;
    if (value.empty())
        enabled = true;
    else if (!Utils::StrToBool(value, enabled))
        return false;

    auto param = m_params->Get<PARAM_CENTROIDS_ENABLED>();

    auto SetValue = [&](auto val) {
        try { param->SetCur(val); }
        catch (const Exception& ex) {
            Log::LogE("Failure %s centroids - %s",
                    (val) ? "enabling" : "disabling", ex.what());
            return false;
        }
        return true;
    };

    if (!param->IsAvail())
        return true;
    if (enabled && !m_params->Get<PARAM_METADATA_ENABLED>()->GetCur())
    {
        Log::LogW("Enforcing frame metadata usage with centroids");
        m_params->Get<PARAM_METADATA_ENABLED>()->SetCur(true);
    }
    if (SetValue(enabled))
        return true;
    if (!m_fixCliOptions)
        return false;
    const auto def = param->GetDef();
    Log::LogW("Fixing centroids enabled state from %s to default %s",
            (enabled) ? "true" : "false", (def) ? "true" : "false");
    return SetValue(def);
}

bool pm::Camera::HandleCentroidsRadius(const std::string& value)
{
    uint16_t radius;
    if (!Utils::StrToNumber<uint16_t>(value, radius))
        return false;

    auto param = m_params->Get<PARAM_CENTROIDS_RADIUS>();

    auto SetValue = [&](auto val) {
        try { param->SetCur(val); }
        catch (const Exception& ex) {
            Log::LogE("Failure setting centroids radius %hu - %s", val, ex.what());
            return false;
        }
        return true;
    };

    if (!param->IsAvail())
        return true;
    const auto min = param->GetMin();
    const auto max = param->GetMax();
    if (radius >= min && radius <= max && SetValue(radius))
        return true;
    if (!m_fixCliOptions)
        return false;
    const auto def = param->GetDef();
    Log::LogW("Fixing centroids radius from %hu to default %hu", radius, def);
    return SetValue(def);
}

bool pm::Camera::HandleCentroidsCount(const std::string& value)
{
    uint16_t count;
    if (!Utils::StrToNumber<uint16_t>(value, count))
        return false;

    auto param = m_params->Get<PARAM_CENTROIDS_COUNT>();

    auto SetValue = [&](auto val) {
        try { param->SetCur(val); }
        catch (const Exception& ex) {
            Log::LogE("Failure setting centroids count %hu - %s", val, ex.what());
            return false;
        }
        return true;
    };

    if (!param->IsAvail())
        return true;
    const auto min = param->GetMin();
    const auto max = param->GetMax();
    if (count >= min && count <= max && SetValue(count))
        return true;
    if (!m_fixCliOptions)
        return false;
    const auto def = param->GetDef();
    Log::LogW("Fixing centroids count from %hu to default %hu", count, def);
    return SetValue(def);
}

bool pm::Camera::HandleCentroidsMode(const std::string& value)
{
    ParamEnum::T mode;
    if (!Utils::StrToNumber<ParamEnum::T>(value, mode))
    {
        if (value == "locate")
            mode = PL_CENTROIDS_MODE_LOCATE;
        else if (value == "track")
            mode = PL_CENTROIDS_MODE_TRACK;
        else if (value == "blob")
            mode = PL_CENTROIDS_MODE_BLOB;
        else
            return false;
    }

    auto param = m_params->Get<PARAM_CENTROIDS_MODE>();

    auto SetValue = [&](auto val) {
        try { param->SetCur(val); }
        catch (const Exception& ex) {
            Log::LogE("Failure setting centroids mode %d - %s", val, ex.what());
            return false;
        }
        return true;
    };

    if (!param->IsAvail())
        return true;
    if (param->HasValue(mode) && SetValue(mode))
        return true;
    if (!m_fixCliOptions)
        return false;
    const auto def = param->GetDef();
    Log::LogW("Fixing centroids mode from %d to default %d", mode, def);
    return SetValue(def);
}

bool pm::Camera::HandleCentroidsBgCount(const std::string& value)
{
    ParamEnum::T bgCount;
    if (!Utils::StrToNumber<ParamEnum::T>(value, bgCount))
        return false;
    // There are no pre-defined values for this enum parameter

    auto param = m_params->Get<PARAM_CENTROIDS_BG_COUNT>();

    auto SetValue = [&](auto val) {
        try { param->SetCur(val); }
        catch (const Exception& ex) {
            Log::LogE("Failure setting centroids background count %d - %s",
                    val, ex.what());
            return false;
        }
        return true;
    };

    if (!param->IsAvail())
        return true;
    if (param->HasValue(bgCount) && SetValue(bgCount))
        return true;
    if (!m_fixCliOptions)
        return false;
    const auto def = param->GetDef();
    Log::LogW("Fixing centroids background count from %d to default %d",
            bgCount, def);
    return SetValue(def);
}

bool pm::Camera::HandleCentroidsThreshold(const std::string& value)
{
    uint32_t threshold;
    if (!Utils::StrToNumber<uint32_t>(value, threshold))
        return false;

    auto param = m_params->Get<PARAM_CENTROIDS_THRESHOLD>();

    auto SetValue = [&](auto val) {
        try { param->SetCur(val); }
        catch (const Exception& ex) {
            Log::LogE("Failure setting centroids threshold %u - %s",
                    val, ex.what());
            return false;
        }
        return true;
    };

    if (!param->IsAvail())
        return true;
    const auto min = param->GetMin();
    const auto max = param->GetMax();
    if (threshold >= min && threshold <= max && SetValue(threshold))
        return true;
    if (!m_fixCliOptions)
        return false;
    const auto def = param->GetDef();
    Log::LogW("Fixing centroids threshold from %u to default %u", threshold, def);
    return SetValue(def);
}

bool pm::Camera::HandleTrigTabSignal(const std::string& value)
{
    ParamEnum::T trigTabSignal;
    if (!Utils::StrToNumber<ParamEnum::T>(value, trigTabSignal))
    {
        if (value == "expose-out")
            trigTabSignal = PL_TRIGTAB_SIGNAL_EXPOSE_OUT;
        else
            return false;
    }

    auto param = m_params->Get<PARAM_TRIGTAB_SIGNAL>();

    auto SetValue = [&](auto val) {
        try { param->SetCur(val); }
        catch (const Exception& ex) {
            Log::LogE("Failure setting trigger table signal %d - %s",
                    val, ex.what());
            return false;
        }
        return true;
    };

    if (!param->IsAvail())
        return true;
    if (param->HasValue(trigTabSignal) && SetValue(trigTabSignal))
        return true;
    if (!m_fixCliOptions)
        return false;
    const auto def = param->GetDef();
    Log::LogW("Fixing trigger table signal from %d to default %d",
            trigTabSignal, def);
    return SetValue(def);
}

bool pm::Camera::HandleLastMuxedSignal(const std::string& value)
{
    uint8_t lastSignal;
    if (!Utils::StrToNumber<uint8_t>(value, lastSignal))
        return false;

    auto param = m_params->Get<PARAM_LAST_MUXED_SIGNAL>();

    auto SetValue = [&](auto val) {
        try { param->SetCur(val); }
        catch (const Exception& ex) {
            Log::LogE("Failure setting last multiplexed signal %hu - %s",
                    val, ex.what());
            return false;
        }
        return true;
    };

    if (!param->IsAvail())
        return true;
    const auto min = param->GetMin();
    const auto max = param->GetMax();
    if (lastSignal >= min && lastSignal <= max && SetValue(lastSignal))
        return true;
    if (!m_fixCliOptions)
        return false;
    const auto def = param->GetDef();
    Log::LogW("Fixing last multiplexed signal from %hu to default %hu",
            lastSignal, def);
    return SetValue(def);
}
