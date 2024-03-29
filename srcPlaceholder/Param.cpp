/******************************************************************************/
/* Copyright (C) Teledyne Photometrics. All rights reserved.                  */
/******************************************************************************/
#include "backend/Param.h"

/* Local */
#include "backend/Camera.h"
#include "backend/exceptions/CameraException.h"
#include "backend/ParamInfoMap.h"
#include "backend/PvcamRuntimeLoader.h"

/* System */
#include <algorithm>

pm::ParamEnum::ParamEnum(Camera* camera, uint32_t id)
    : Base(camera, id)
{
}

void pm::ParamEnum::ResetCacheRangeFlags()
{
    Base::ResetCacheRangeFlags();
    UpdateEnumItems();
}

void pm::ParamEnum::UpdateEnumItems()
{
    m_itemsCacheSet = false;
}

const std::vector<pm::ParamEnumItem>& pm::ParamEnum::GetItems() const
{
    ReadItemsCached();
    return m_items;
}

const std::vector<std::string>& pm::ParamEnum::GetNames() const
{
    ReadItemsCached();
    return m_names;
}

const std::vector<pm::ParamEnum::T>& pm::ParamEnum::GetValues() const
{
    ReadItemsCached();
    return m_values;
}

bool pm::ParamEnum::HasValue(T value) const
{
    ReadItemsCached();
    return HasValue(m_items, value);
}

const std::string& pm::ParamEnum::GetValueName(T value) const
{
    ReadItemsCached();
    try
    {
        return m_valueNameMap.at(value);
    }
    catch (...)
    {
        throw CameraException(
                "Enum has no items with value " + std::to_string(value)
                + " (paramId="
                + ((ParamInfoMap::HasParamInfo(m_id))
                    ? ParamInfoMap::GetParamInfo(m_id).GetName()
                    : std::to_string(m_id))
                + ")",
                m_camera);
    }
}

const pm::ParamEnumItem& pm::ParamEnum::GetItem(T value) const
{
    ReadItemsCached();
    try
    {
        return m_valueItemMap.at(value);
    }
    catch (...)
    {
        throw CameraException(
                "Enum has no items with value " + std::to_string(value)
                + " (paramId="
                + ((ParamInfoMap::HasParamInfo(m_id))
                    ? ParamInfoMap::GetParamInfo(m_id).GetName()
                    : std::to_string(m_id))
                + ")",
                m_camera);
    }
}

void pm::ParamEnum::ReadItemsCached() const
{
    if (!m_itemsCacheSet)
    {
        ReadItems();
    }
}

void pm::ParamEnum::ReadItems() const
{
    m_items.clear();
    m_names.clear();
    m_values.clear();
    m_valueNameMap.clear();
    m_valueItemMap.clear();

    if (!IsAvail())
        return;

    std::vector<ParamEnumItem> items;

    const uint32_t count = GetCount();
    for (uint32_t n = 0; n < count; ++n)
    {
        uint32_t enumStrLen;
        if (PV_OK != PVCAM->pl_enum_str_length(m_camera->GetHandle(), m_id, n,
                    &enumStrLen))
        {
            throw CameraException("Failure getting enum item length GetParam(paramId="
                    + ((ParamInfoMap::HasParamInfo(m_id))
                        ? ParamInfoMap::GetParamInfo(m_id).GetName()
                        : std::to_string(m_id))
                    + ", n=" + std::to_string(n) + ")",
                    m_camera);
        }

        T enumValue;
        auto enumString = std::make_unique<char[]>(enumStrLen);
        if (PV_OK != PVCAM->pl_get_enum_param(m_camera->GetHandle(), m_id, n,
                    &enumValue, enumString.get(), enumStrLen))
        {
            throw CameraException("Failure getting enum item GetParam(paramId="
                    + ((ParamInfoMap::HasParamInfo(m_id))
                        ? ParamInfoMap::GetParamInfo(m_id).GetName()
                        : std::to_string(m_id))
                    + ", n=" + std::to_string(n) + ")",
                    m_camera);
        }

        items.emplace_back(enumValue, enumString.get());
    }

    if (items.empty())
    {
        throw CameraException("Enum has no items (paramId="
                + ((ParamInfoMap::HasParamInfo(m_id))
                    ? ParamInfoMap::GetParamInfo(m_id).GetName()
                    : std::to_string(m_id))
                + ")",
                m_camera);
    }

    for (auto& item : items)
    {
        m_items.push_back(item);
        m_values.push_back(item.GetValue());
        m_names.push_back(item.GetName());
        m_valueNameMap[item.GetValue()] = item.GetName();
        m_valueItemMap[item.GetValue()] = item;
    }
    m_itemsCacheSet = true;
}

bool pm::ParamEnum::HasValue(const std::vector<ParamEnumItem>& items, T value)
{
    const auto it = std::find_if(items.cbegin(), items.cend(),
            [value](const auto& item) { return value == item.GetValue(); });
    return it != items.cend();
}
