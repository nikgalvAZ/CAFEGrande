/******************************************************************************/
/* Copyright (C) Teledyne Photometrics. All rights reserved.                  */
/******************************************************************************/
#include "backend/ParamInfoMap.h"

/* Local */
#include "backend/exceptions/ParamGetException.h"
#include "backend/Log.h"
#include "backend/ParamDefinitions.h"

/* PVCAM */
#include "master.h"
#include "pvcam.h"

/* System */
#include <algorithm>
#include <cassert>
#include <functional>

static const std::string g_grpName_CamIdentity    = "A - Camera Identification";
static const std::string g_grpName_SensorProps    = "B - Sensor Properties";
static const std::string g_grpName_MetaCentroids  = "C - Metadata & Centroids";
static const std::string g_grpName_TriggerTable   = "D - Trigger Table";
static const std::string g_grpName_Timing         = "E - Timing Estimations";
static const std::string g_grpName_PostProcessing = "F - Post Processing";
static const std::string g_grpName_ScanMode       = "G - Scan Mode";
static const std::string g_grpName_IO             = "H - I/O Signals";
// Keep miscellaneous group as last one
static const std::string g_grpName_Misc           = "Z - Miscellaneous";

template<class T>
static inline std::string GetMappedName(const std::map<T, std::string>& map,
        T value, bool includeId = false)
{
    const auto it = map.find(value);
    const bool found = it != map.cend();
    std::string str = (found) ? it->second :  "<UNKNOWN>";
    if (includeId || !found)
        str += " (" + std::to_string(value) + ")";
    return str;
}

static const std::map<int16_t, std::string> g_attrIdNameMap {
    { (int16_t)ATTR_AVAIL    , "ATTR_AVAIL" },
    { (int16_t)ATTR_TYPE     , "ATTR_TYPE" },
    { (int16_t)ATTR_ACCESS   , "ATTR_ACCESS" },
    { (int16_t)ATTR_COUNT    , "ATTR_COUNT" },
    { (int16_t)ATTR_CURRENT  , "ATTR_CURRENT" },
    { (int16_t)ATTR_DEFAULT  , "ATTR_DEFAULT" },
    { (int16_t)ATTR_MIN      , "ATTR_MIN" },
    { (int16_t)ATTR_MAX      , "ATTR_MAX" },
    { (int16_t)ATTR_INCREMENT, "ATTR_INCREMENT" },
};

static const std::map<uint16_t, std::string> g_accessIdNameMap {
    { (uint16_t)ACC_READ_ONLY       , "ACC_READ_ONLY" },
    { (uint16_t)ACC_READ_WRITE      , "ACC_READ_WRITE" },
    { (uint16_t)ACC_EXIST_CHECK_ONLY, "ACC_EXIST_CHECK_ONLY" },
    { (uint16_t)ACC_WRITE_ONLY      , "ACC_WRITE_ONLY" },
};

static const std::map<uint16_t, std::string> g_typeIdNameMap {
    { (uint16_t)TYPE_ENUM                 , "TYPE_ENUM" },
    { (uint16_t)TYPE_BOOLEAN              , "TYPE_BOOLEAN" },
    { (uint16_t)TYPE_INT8                 , "TYPE_INT8" },
    { (uint16_t)TYPE_INT16                , "TYPE_INT16" },
    { (uint16_t)TYPE_INT32                , "TYPE_INT32" },
    { (uint16_t)TYPE_INT64                , "TYPE_INT64" },
    { (uint16_t)TYPE_UNS8                 , "TYPE_UNS8" },
    { (uint16_t)TYPE_UNS16                , "TYPE_UNS16" },
    { (uint16_t)TYPE_UNS32                , "TYPE_UNS32" },
    { (uint16_t)TYPE_UNS64                , "TYPE_UNS64" },
    { (uint16_t)TYPE_FLT32                , "TYPE_FLT32" },
    { (uint16_t)TYPE_FLT64                , "TYPE_FLT64" },
    { (uint16_t)TYPE_CHAR_PTR             , "TYPE_CHAR_PTR" },
    { (uint16_t)TYPE_SMART_STREAM_TYPE_PTR, "TYPE_SMART_STREAM_TYPE_PTR" },
    { (uint16_t)TYPE_SMART_STREAM_TYPE    , "TYPE_SMART_STREAM_TYPE" },
    { (uint16_t)TYPE_VOID_PTR             , "TYPE_VOID_PTR" },
    { (uint16_t)TYPE_VOID_PTR_PTR         , "TYPE_VOID_PTR_PTR" },
};


std::map<uint32_t, pm::ParamInfo> pm::ParamInfoMap::m_map {
    #define NEW_PAR(id, ...) \
        { (id), pm::ParamInfo((id), #id, ParamTypeFromT<ParamT<id>::T>::value, ##__VA_ARGS__) }

    // Same order as PARAM_* definitions in pvcam.h
    // Some level of circular dependencies is allowed

    NEW_PAR(PARAM_DD_INFO_LENGTH           , false, g_grpName_CamIdentity),
    NEW_PAR(PARAM_DD_VERSION               , false, g_grpName_CamIdentity),
    NEW_PAR(PARAM_DD_RETRIES               , false, g_grpName_Misc),
    NEW_PAR(PARAM_DD_TIMEOUT               , false, g_grpName_Misc),
    NEW_PAR(PARAM_DD_INFO                  , false, g_grpName_CamIdentity),

    NEW_PAR(PARAM_CAM_INTERFACE_TYPE       , false, g_grpName_Misc),
    NEW_PAR(PARAM_CAM_INTERFACE_MODE       , false, g_grpName_Misc),

    NEW_PAR(PARAM_ADC_OFFSET               , false, g_grpName_Misc),
    NEW_PAR(PARAM_CHIP_NAME                , false, g_grpName_CamIdentity),
    NEW_PAR(PARAM_SYSTEM_NAME              , false, g_grpName_CamIdentity),
    NEW_PAR(PARAM_VENDOR_NAME              , false, g_grpName_CamIdentity),
    NEW_PAR(PARAM_PRODUCT_NAME             , false, g_grpName_CamIdentity),
    NEW_PAR(PARAM_CAMERA_PART_NUMBER       , false, g_grpName_CamIdentity),

    NEW_PAR(PARAM_COOLING_MODE             , false, g_grpName_Misc),
    NEW_PAR(PARAM_PREAMP_DELAY             , false, g_grpName_Misc),
    NEW_PAR(PARAM_COLOR_MODE               , false, g_grpName_Misc),
    NEW_PAR(PARAM_MPP_CAPABLE              , false, g_grpName_Misc),
    NEW_PAR(PARAM_PREAMP_OFF_CONTROL       , false, g_grpName_Misc),

    NEW_PAR(PARAM_PREMASK                  , false, g_grpName_SensorProps),
    NEW_PAR(PARAM_PRESCAN                  , false, g_grpName_SensorProps),
    NEW_PAR(PARAM_POSTMASK                 , false, g_grpName_SensorProps),
    NEW_PAR(PARAM_POSTSCAN                 , false, g_grpName_SensorProps),
    NEW_PAR(PARAM_PIX_PAR_DIST             , false, g_grpName_SensorProps),
    NEW_PAR(PARAM_PIX_PAR_SIZE             , false, g_grpName_SensorProps),
    NEW_PAR(PARAM_PIX_SER_DIST             , false, g_grpName_SensorProps),
    NEW_PAR(PARAM_PIX_SER_SIZE             , false, g_grpName_SensorProps),
    NEW_PAR(PARAM_SUMMING_WELL             , false, g_grpName_SensorProps),
    NEW_PAR(PARAM_FWELL_CAPACITY           , false, g_grpName_SensorProps),
    NEW_PAR(PARAM_PAR_SIZE                 , false, g_grpName_SensorProps),
    NEW_PAR(PARAM_SER_SIZE                 , false, g_grpName_SensorProps),

    NEW_PAR(PARAM_READOUT_TIME             , true , g_grpName_Timing),
    NEW_PAR(PARAM_CLEARING_TIME            , true , g_grpName_Timing),
    NEW_PAR(PARAM_POST_TRIGGER_DELAY       , true , g_grpName_Timing),
    NEW_PAR(PARAM_PRE_TRIGGER_DELAY        , true , g_grpName_Timing),

    NEW_PAR(PARAM_CLEAR_CYCLES             , false, g_grpName_Misc),
    NEW_PAR(PARAM_CLEAR_MODE               , false, g_grpName_Misc),
    NEW_PAR(PARAM_FRAME_CAPABLE            , false, g_grpName_Misc),
    NEW_PAR(PARAM_PMODE                    , false, g_grpName_Misc, { PARAM_TEMP_SETPOINT }),

    NEW_PAR(PARAM_TEMP                     , false, g_grpName_Misc),
    NEW_PAR(PARAM_TEMP_SETPOINT            , false, g_grpName_Misc),

    NEW_PAR(PARAM_CAM_FW_VERSION           , false, g_grpName_CamIdentity),
    NEW_PAR(PARAM_HEAD_SER_NUM_ALPHA       , false, g_grpName_CamIdentity),
    NEW_PAR(PARAM_PCI_FW_VERSION           , false, g_grpName_CamIdentity),

    NEW_PAR(PARAM_FAN_SPEED_SETPOINT       , false, g_grpName_Misc, { PARAM_TEMP_SETPOINT }),
#if 0 // Temporarily disabled due to USB issues
    NEW_PAR(PARAM_CAM_SYSTEMS_INFO         , false, g_grpName_Misc),
#endif

    NEW_PAR(PARAM_EXPOSURE_MODE            , true , g_grpName_Misc),
    NEW_PAR(PARAM_EXPOSE_OUT_MODE          , true , g_grpName_Misc),

    NEW_PAR(PARAM_BIT_DEPTH                , false, g_grpName_Misc),
    NEW_PAR(PARAM_IMAGE_FORMAT             , false, g_grpName_Misc),
    NEW_PAR(PARAM_IMAGE_COMPRESSION        , false, g_grpName_Misc),
    NEW_PAR(PARAM_SCAN_MODE                , false, g_grpName_ScanMode, { PARAM_SCAN_LINE_DELAY, PARAM_SCAN_DIRECTION, PARAM_SCAN_DIRECTION_RESET }),
    NEW_PAR(PARAM_SCAN_DIRECTION           , false, g_grpName_ScanMode),
    NEW_PAR(PARAM_SCAN_DIRECTION_RESET     , false, g_grpName_ScanMode),
    NEW_PAR(PARAM_SCAN_LINE_DELAY          , false, g_grpName_ScanMode, { PARAM_SCAN_WIDTH, PARAM_SCAN_LINE_TIME }),
    NEW_PAR(PARAM_SCAN_LINE_TIME           , true , g_grpName_ScanMode),
    NEW_PAR(PARAM_SCAN_WIDTH               , false, g_grpName_ScanMode, { PARAM_SCAN_LINE_DELAY, PARAM_SCAN_LINE_TIME }),
#if 0 // Temporarily disabled, both are read-write, but we don't support sensor size reconfiguration at runtime
    NEW_PAR(PARAM_FRAME_ROTATE             , false, g_grpName_Misc),
    NEW_PAR(PARAM_FRAME_FLIP               , false, g_grpName_Misc),
#endif
    NEW_PAR(PARAM_GAIN_INDEX               , false, g_grpName_Misc, { PARAM_BIT_DEPTH, PARAM_GAIN_NAME, PARAM_SCAN_MODE, PARAM_GAIN_MULT_FACTOR, PARAM_TEMP_SETPOINT }),
    NEW_PAR(PARAM_SPDTAB_INDEX             , false, g_grpName_Misc, { PARAM_PIX_TIME, PARAM_SPDTAB_NAME, PARAM_GAIN_INDEX, PARAM_COLOR_MODE, PARAM_IMAGE_COMPRESSION, PARAM_IMAGE_FORMAT, PARAM_PP_INDEX }),
    NEW_PAR(PARAM_GAIN_NAME                , false, g_grpName_Misc),
    NEW_PAR(PARAM_SPDTAB_NAME              , false, g_grpName_Misc),
    NEW_PAR(PARAM_READOUT_PORT             , false, g_grpName_Misc, { PARAM_SPDTAB_INDEX }),
    NEW_PAR(PARAM_PIX_TIME                 , false, g_grpName_Misc),

    NEW_PAR(PARAM_SHTR_CLOSE_DELAY         , false, g_grpName_Misc),
    NEW_PAR(PARAM_SHTR_OPEN_DELAY          , false, g_grpName_Misc),
    NEW_PAR(PARAM_SHTR_OPEN_MODE           , false, g_grpName_Misc),
    NEW_PAR(PARAM_SHTR_STATUS              , false, g_grpName_Misc),

    NEW_PAR(PARAM_IO_ADDR                  , false, g_grpName_IO, { PARAM_IO_BITDEPTH, PARAM_IO_DIRECTION, PARAM_IO_TYPE }),
    NEW_PAR(PARAM_IO_TYPE                  , false, g_grpName_IO),
    NEW_PAR(PARAM_IO_DIRECTION             , false, g_grpName_IO, { PARAM_IO_STATE }),
    NEW_PAR(PARAM_IO_STATE                 , false, g_grpName_IO),
    NEW_PAR(PARAM_IO_BITDEPTH              , false, g_grpName_IO),

    NEW_PAR(PARAM_GAIN_MULT_FACTOR         , false, g_grpName_Misc),
    NEW_PAR(PARAM_GAIN_MULT_ENABLE         , false, g_grpName_Misc),

    NEW_PAR(PARAM_PP_FEAT_NAME             , false, g_grpName_PostProcessing),
    NEW_PAR(PARAM_PP_INDEX                 , false, g_grpName_PostProcessing, { PARAM_PP_FEAT_ID, PARAM_PP_FEAT_NAME, PARAM_PP_PARAM_INDEX }),
    NEW_PAR(PARAM_ACTUAL_GAIN              , false, g_grpName_PostProcessing),
    NEW_PAR(PARAM_PP_PARAM_INDEX           , false, g_grpName_PostProcessing, { PARAM_PP_PARAM_ID, PARAM_PP_PARAM_NAME, PARAM_PP_PARAM }),
    NEW_PAR(PARAM_PP_PARAM_NAME            , false, g_grpName_PostProcessing),
    NEW_PAR(PARAM_PP_PARAM                 , false, g_grpName_PostProcessing, { PARAM_BIT_DEPTH, PARAM_IMAGE_FORMAT }),
    NEW_PAR(PARAM_READ_NOISE               , false, g_grpName_PostProcessing),
    NEW_PAR(PARAM_PP_FEAT_ID               , false, g_grpName_PostProcessing),
    NEW_PAR(PARAM_PP_PARAM_ID              , false, g_grpName_PostProcessing),

    NEW_PAR(PARAM_SMART_STREAM_MODE_ENABLED, false, g_grpName_Misc),
    NEW_PAR(PARAM_SMART_STREAM_MODE        , false, g_grpName_Misc),
    NEW_PAR(PARAM_SMART_STREAM_EXP_PARAMS  , false, g_grpName_Misc),
    NEW_PAR(PARAM_SMART_STREAM_DLY_PARAMS  , false, g_grpName_Misc),

    NEW_PAR(PARAM_EXP_TIME                 , false, g_grpName_Misc),
    NEW_PAR(PARAM_EXP_RES                  , false, g_grpName_Misc, { PARAM_EXP_RES_INDEX, PARAM_EXPOSURE_TIME }),
    NEW_PAR(PARAM_EXP_RES_INDEX            , false, g_grpName_Misc, { PARAM_EXP_RES, PARAM_EXPOSURE_TIME }),
    NEW_PAR(PARAM_EXPOSURE_TIME            , true , g_grpName_Misc),

    NEW_PAR(PARAM_BOF_EOF_ENABLE           , false, g_grpName_Misc),
    NEW_PAR(PARAM_BOF_EOF_COUNT            , false, g_grpName_Misc, { PARAM_BOF_EOF_CLR }),
    NEW_PAR(PARAM_BOF_EOF_CLR              , false, g_grpName_Misc),

    NEW_PAR(PARAM_CIRC_BUFFER              , false, g_grpName_Misc),
    NEW_PAR(PARAM_FRAME_BUFFER_SIZE        , true , g_grpName_Misc),

    NEW_PAR(PARAM_BINNING_SER              , true , g_grpName_Misc),
    NEW_PAR(PARAM_BINNING_PAR              , true , g_grpName_Misc),

    NEW_PAR(PARAM_METADATA_ENABLED         , false, g_grpName_MetaCentroids),
    NEW_PAR(PARAM_ROI_COUNT                , true , g_grpName_MetaCentroids),
    NEW_PAR(PARAM_CENTROIDS_ENABLED        , false, g_grpName_MetaCentroids),
    NEW_PAR(PARAM_CENTROIDS_RADIUS         , false, g_grpName_MetaCentroids),
    NEW_PAR(PARAM_CENTROIDS_COUNT          , false, g_grpName_MetaCentroids),
    NEW_PAR(PARAM_CENTROIDS_MODE           , false, g_grpName_MetaCentroids),
    NEW_PAR(PARAM_CENTROIDS_BG_COUNT       , false, g_grpName_MetaCentroids),
    NEW_PAR(PARAM_CENTROIDS_THRESHOLD      , false, g_grpName_MetaCentroids),

    NEW_PAR(PARAM_TRIGTAB_SIGNAL           , false, g_grpName_TriggerTable, { PARAM_LAST_MUXED_SIGNAL }),
    NEW_PAR(PARAM_LAST_MUXED_SIGNAL        , false, g_grpName_TriggerTable),
    NEW_PAR(PARAM_FRAME_DELIVERY_MODE      , false, g_grpName_Misc),

    #undef NEW_PAR
};

std::vector<uint32_t> pm::ParamInfoMap::m_paramIds = InitSortedIds();

const std::map<uint32_t, pm::ParamInfo>& pm::ParamInfoMap::GetMap()
{
    return m_map;
}

const std::vector<uint32_t>& pm::ParamInfoMap::GetSortedParamIds()
{
    return m_paramIds;
}

pm::ParamInfo pm::ParamInfoMap::GetParamInfo(uint32_t paramId)
{
    const auto it = m_map.find(paramId);
    if (it == m_map.cend())
        throw Exception("No definition found for param id '"
                + std::to_string(paramId) + "'");
    return it->second;
}

bool pm::ParamInfoMap::HasParamInfo(uint32_t paramId)
{
    const auto it = m_map.find(paramId);
    const bool hasInfo = (it != m_map.cend());
    return hasInfo;
}

bool pm::ParamInfoMap::FindParamInfo(uint32_t paramId, pm::ParamInfo& paramInfo)
{
    const auto it = m_map.find(paramId);
    if (it == m_map.cend())
        return false;
    paramInfo = it->second;
    return true;
}

std::string pm::ParamInfoMap::GetParamAttrIdName(int16_t value, bool includeId)
{
    return GetMappedName(g_attrIdNameMap, value, includeId);
}

std::string pm::ParamInfoMap::GetParamAccessIdName(uint16_t value, bool includeId)
{
    return GetMappedName(g_accessIdNameMap, value, includeId);
}

std::string pm::ParamInfoMap::GetParamTypeIdName(uint16_t value, bool includeId)
{
    return GetMappedName(g_typeIdNameMap, value, includeId);
}

std::string pm::ParamInfoMap::GetParamEnumItemName(const ParamEnumItem& item,
        bool includeId)
{
    std::string str = item.GetName();
    if (includeId)
        str += " (" + std::to_string(item.GetValue()) + ")";
    return str;
}

std::vector<uint32_t> pm::ParamInfoMap::InitSortedIds()
{
    std::vector<uint32_t> ids;
    ids.reserve(m_map.size());

    // Enforce PARAM_READOUT_PORT to be first
    ids.push_back(PARAM_READOUT_PORT);

    // Allow just a few sorting iterations to break possibly infinite recursion
    constexpr size_t maxTouches = 10;
    std::map<uint32_t, size_t> idTouchMap;

    // Set recursive dependencies for all parameters first
    for (auto& paramInfoPair : m_map)
    {
        const auto id = paramInfoPair.first;
        auto& info = paramInfoPair.second;

        std::vector<uint32_t> deps;

        // Lambdas cannot be called easily recursively, declare it first
        std::function<void(uint32_t)> AddRecurDeps;
        AddRecurDeps = [&](uint32_t id2) {
            for (auto depId : m_map.at(id2).GetDirectDeps())
            {
                if (depId == id)
                    continue;
                if (std::find(deps.cbegin(), deps.cend(), depId) != deps.cend())
                    continue;
                deps.push_back(depId);
                AddRecurDeps(depId);
            }
        };

        AddRecurDeps(id);
        info.SetRecursiveDeps(deps);
    }

    // Now sort parameters based on their dependencies
    // First put those with some dependencies
    for (const auto& paramInfoPair : m_map)
    {
        const auto id = paramInfoPair.first;
        const auto& paramInfo = paramInfoPair.second;
        if (paramInfo.GetRecursiveDeps().empty())
            continue;

        // Lambdas cannot be called easily recursively, declare it first
        std::function<void(uint32_t)> AddAndSortDeps;
        AddAndSortDeps = [&](uint32_t id2) {
            auto it = std::find(ids.cbegin(), ids.cend(), id2);
            assert(it != ids.cend());
            const auto& deps = m_map.at(id2).GetRecursiveDeps();
            size_t moveOffset = 0;
            for (auto depId : deps)
            {
                auto depIt = std::find(ids.cbegin(), ids.cend(), depId);
                if (depIt == ids.cend())
                {
                    ids.insert(it + 1 + moveOffset++, depId);
                    continue;
                }
                if (depIt < it + moveOffset)
                {
                    if (idTouchMap[depId] >= maxTouches)
                        continue;
                    idTouchMap[depId]++;

                    ids.erase(depIt);
                    it = std::find(ids.cbegin(), ids.cend(), id2);
                    assert(it != ids.cend());
                    ids.insert(it + 1 + moveOffset++, depId);
                }
            }
            for (auto depId : deps)
            {
                if (depId == id)
                    continue;
                if (idTouchMap[depId] >= maxTouches)
                    continue;
                AddAndSortDeps(depId);
            }
        };

        if (std::find(ids.cbegin(), ids.cend(), id) == ids.cend())
            ids.push_back(id);
        idTouchMap.clear();
        AddAndSortDeps(id);
    }
    // Then append all remaining
    for (const auto& paramInfoPair : m_map)
    {
        const auto id = paramInfoPair.first;
        if (std::find(ids.cbegin(), ids.cend(), id) == ids.cend())
            ids.push_back(id);
    }

#if 0 // Print sorted IDs with recursive dependencies
    for (auto id : ids) {
        const auto& info = m_map.at(id);
        const auto& deps = info.GetRecursiveDeps();
        std::string log = info.GetName();
        if (!deps.empty())
        {
            log += "  -";
            for (auto depId : deps)
                log += "  " + m_map.at(depId).GetName();
        }
        log += "\n";
        Log::LogD(log.c_str());
    }
#endif

    return ids;
}
