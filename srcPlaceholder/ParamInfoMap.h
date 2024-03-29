/******************************************************************************/
/* Copyright (C) Teledyne Photometrics. All rights reserved.                  */
/******************************************************************************/
#pragma once
#ifndef PM_PARAM_INFO_MAP_H
#define PM_PARAM_INFO_MAP_H

/* Local */
#include "backend/ParamEnumItem.h"
#include "backend/ParamInfo.h"

/* System */
#include <map>

namespace pm {

class ParamInfoMap
{
public:
    static const std::map<uint32_t, ParamInfo>& GetMap();

    // Sorted list of parameter IDs.
    // Sorted in a way the parameters should be set according to dependencies.
    static const std::vector<uint32_t>& GetSortedParamIds();

    static ParamInfo GetParamInfo(uint32_t paramId);
    static bool HasParamInfo(uint32_t paramId);
    static bool FindParamInfo(uint32_t paramId, ParamInfo& paramInfo);

    static std::string GetParamAttrIdName(int16_t value, bool includeId = false);
    static std::string GetParamAccessIdName(uint16_t value, bool includeId = false);
    static std::string GetParamTypeIdName(uint16_t value, bool includeId = false);
    static std::string GetParamEnumItemName(const ParamEnumItem& item,
            bool includeId = false);

private:
    static std::vector<uint32_t> InitSortedIds();

private:
    static std::map<uint32_t, ParamInfo> m_map;
    static std::vector<uint32_t> m_paramIds;
};

} // namespace pm

#endif /* PM_PARAM_INFO_MAP_H */
