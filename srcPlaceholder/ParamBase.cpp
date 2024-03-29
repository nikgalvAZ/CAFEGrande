/******************************************************************************/
/* Copyright (C) Teledyne Photometrics. All rights reserved.                  */
/******************************************************************************/
#include "backend/ParamBase.h"

/* Local */
#include "backend/Camera.h"
#include "backend/exceptions/ParamGetException.h"
#include "backend/exceptions/ParamSetException.h"
#include "backend/ParamInfoMap.h"
#include "backend/PvcamRuntimeLoader.h"

/* PVCAM */
#include "master.h"
#include "pvcam.h"

/* System */
#include <cassert>

pm::ParamBase::ParamBase(Camera* camera, uint32_t id)
    : m_camera(camera),
    m_id(id),
    m_changeHandlersNextHandle(static_cast<uint64_t>(id) << 32)
{
    assert(camera);
    m_avail = std::make_unique<ParamValue<bool>>(false);
    m_access = std::make_unique<ParamValue<uint16_t>>((uint16_t)0); // Unknown access
    m_type = std::make_unique<ParamValue<uint16_t>>((uint16_t)0); // Unknown type
    m_count = std::make_unique<ParamValue<uint32_t>>(0);
}

pm::ParamBase::~ParamBase()
{
}

pm::Camera* pm::ParamBase::GetCamera() const
{
    return m_camera;
}

uint32_t pm::ParamBase::GetId() const
{
    return m_id;
}

const pm::ParamValueBase* pm::ParamBase::GetValue(int16_t attrId) const
{
    // Invoke mapped virtual member function to update cache
    (this->*m_attrUpdateFnMap.at(attrId))();
    return m_attrValueMap.at(attrId);
}

bool pm::ParamBase::IsAvail() const
{
    return GetIsAvailValue()->GetValue();
}

uint16_t pm::ParamBase::GetAccess() const
{
    return GetAccessValue()->GetValue();
}

uint16_t pm::ParamBase::GetType() const
{
    return GetTypeValue()->GetValue();
}

const pm::ParamValue<bool>* pm::ParamBase::GetIsAvailValue() const
{
    UpdateIsAvailCache();
    return m_avail.get();
}

const pm::ParamValue<uint16_t>* pm::ParamBase::GetAccessValue() const
{
    UpdateAccessCache();
    return m_access.get();
}

const pm::ParamValue<uint16_t>* pm::ParamBase::GetTypeValue() const
{
    UpdateTypeCache();
    return m_type.get();
}

uint32_t pm::ParamBase::GetCount() const
{
    return GetCountValue()->GetValue();
}

const pm::ParamValue<uint32_t>* pm::ParamBase::GetCountValue() const
{
    UpdateCountCache();
    return m_count.get();
}

const pm::ParamValueBase* pm::ParamBase::GetDefValue() const
{
    UpdateDefCache();
    return m_def.get();
}

const pm::ParamValueBase* pm::ParamBase::GetMinValue() const
{
    UpdateMinCache();
    return m_min.get();
}

const pm::ParamValueBase* pm::ParamBase::GetMaxValue() const
{
    UpdateMaxCache();
    return m_max.get();
}

const pm::ParamValueBase* pm::ParamBase::GetIncValue() const
{
    UpdateIncCache();
    return m_inc.get();
}

const pm::ParamValueBase* pm::ParamBase::GetCurValue() const
{
    UpdateCurCache();
    return m_cur.get();
}

void pm::ParamBase::SetCurValue(const pm::ParamValueBase* value)
{
    if (!value)
        throw Exception("Failed to set value from null pointer");

    WriteValue(value->GetPtr(), value->ToString().c_str());
}

void pm::ParamBase::SetFromString(const std::string& str)
{
    // Used temporary value instead of m_cur for parsing from string.
    // m_cur shouldn't be changed by Set function
    // (only for FakeParam via overridden WriteValue method)
    m_curTmp->FromString(str);
    WriteValue(m_curTmp->GetPtr(), str.c_str());
}

void pm::ParamBase::ResetCacheAllFlags()
{
    m_attrIdCacheSetMap[ATTR_AVAIL ] = false;
    m_attrIdCacheSetMap[ATTR_ACCESS] = false;
    m_attrIdCacheSetMap[ATTR_TYPE  ] = false;

    ResetCacheRangeFlags();
}

void pm::ParamBase::ResetCacheRangeFlags()
{
    m_attrIdCacheSetMap[ATTR_COUNT    ] = false;
    m_attrIdCacheSetMap[ATTR_DEFAULT  ] = false;
    m_attrIdCacheSetMap[ATTR_MIN      ] = false;
    m_attrIdCacheSetMap[ATTR_MAX      ] = false;
    m_attrIdCacheSetMap[ATTR_INCREMENT] = false;

    m_attrIdCacheSetMap[ATTR_CURRENT  ] = false;
}

void pm::ParamBase::ResetCacheFlag(int16_t attrId)
{
    switch (attrId)
    {
    case ATTR_AVAIL:
    case ATTR_TYPE:
    case ATTR_ACCESS:
    case ATTR_COUNT:
    case ATTR_CURRENT:
    case ATTR_DEFAULT:
    case ATTR_MIN:
    case ATTR_MAX:
    case ATTR_INCREMENT:
        m_attrIdCacheSetMap[attrId] = false;
        break;
    default:
        throw Exception("Failure resetting cache flag (paramId="
                + ((ParamInfoMap::HasParamInfo(m_id))
                    ? ParamInfoMap::GetParamInfo(m_id).GetName()
                    : std::to_string(m_id))
                + ", attrId=" + ParamInfoMap::GetParamAttrIdName(attrId) + ")");
    }
}

void pm::ParamBase::ReadValueCached(void* value, int16_t attrId) const
{
    if (!m_attrIdCacheSetMap[attrId])
    {
        ReadValue(value, attrId);
    }
}

void pm::ParamBase::ReadValue(void* value, int16_t attrId) const
{
    if (PV_OK != PVCAM->pl_get_param(m_camera->GetHandle(), m_id, attrId, value))
    {
        throw ParamGetException("Failure getting value", m_camera, m_id, attrId);
    }
    m_attrIdCacheSetMap[attrId] = true;
}

void pm::ParamBase::WriteValue(const void* value, const char* valueAsStr)
{
    if (PV_OK != PVCAM->pl_set_param(m_camera->GetHandle(), m_id,
                const_cast<void*>(value)))
    {
        throw ParamSetException(
                std::string("Failure setting new value to ") + valueAsStr,
                m_camera, m_id);
    }
    m_attrIdCacheSetMap[ATTR_CURRENT] = false;
    InvokeChangeHandlers();
}

void pm::ParamBase::InitAttrValueMap()
{
    m_attrValueMap[ATTR_AVAIL    ] = m_avail.get();
    m_attrValueMap[ATTR_ACCESS   ] = m_access.get();
    m_attrValueMap[ATTR_TYPE     ] = m_type.get();
    m_attrValueMap[ATTR_COUNT    ] = m_count.get();
    m_attrValueMap[ATTR_DEFAULT  ] = m_def.get();
    m_attrValueMap[ATTR_MIN      ] = m_min.get();
    m_attrValueMap[ATTR_MAX      ] = m_max.get();
    m_attrValueMap[ATTR_INCREMENT] = m_inc.get();
    m_attrValueMap[ATTR_CURRENT  ] = m_cur.get();
}

uint64_t pm::ParamBase::RegisterChangeHandler(const ChangeHandler& handler)
{
    std::lock_guard<std::mutex> lock(m_changeHandlersMutex);

    ChangeHandlerStorage storage{ handler, m_changeHandlersNextHandle++ };
    m_changeHandlers.push_back(storage);
    return storage.handle;
}

void pm::ParamBase::UnregisterChangeHandler(uint64_t handle)
{
    std::lock_guard<std::mutex> lock(m_changeHandlersMutex);

    for (size_t n = 0; n < m_changeHandlers.size(); ++n)
    {
        const auto& storage = m_changeHandlers.at(n);
        if (storage.handle == handle)
        {
            m_changeHandlers.erase(m_changeHandlers.cbegin() + n);
            break;
        }
    }
}

void pm::ParamBase::InvokeChangeHandlers(bool allAttrsChanged)
{
    std::lock_guard<std::mutex> lock(m_changeHandlersMutex);

    // Invoke direct handlers with given argument
    if (m_avail)
    {
        for (const auto& storage : m_changeHandlers)
        {
            storage.handler(*this, allAttrsChanged);
        }
    }

    // Continue only if this method was called from Set method
    if (allAttrsChanged)
        return;

    // Take care of all recursive dependencies
    const auto& params = m_camera->GetParams().GetParams();
    const auto& deps = ParamInfoMap::GetMap().at(m_id).GetRecursiveDeps();
    for (auto id : deps)
    {
        auto& param = params.at(id);
        param->ResetCacheFlag(ATTR_ACCESS);
        param->ResetCacheRangeFlags();
        param->InvokeChangeHandlers(true);
    }
}

void pm::ParamBase::UpdateIsAvailCache() const
{
    ReadValueCached(m_avail->GetPtr(), ATTR_AVAIL);
}

void pm::ParamBase::UpdateAccessCache() const
{
    ReadValueCached(m_access->GetPtr(), ATTR_ACCESS);
}

void pm::ParamBase::UpdateTypeCache() const
{
    ReadValueCached(m_type->GetPtr(), ATTR_TYPE);
}

void pm::ParamBase::UpdateCountCache() const
{
    ReadValueCached(m_count->GetPtr(), ATTR_COUNT);
}
