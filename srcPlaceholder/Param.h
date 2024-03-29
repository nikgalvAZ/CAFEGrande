/******************************************************************************/
/* Copyright (C) Teledyne Photometrics. All rights reserved.                  */
/******************************************************************************/
#pragma once
#ifndef PM_PARAM_H
#define PM_PARAM_H

/* Local */
#include "backend/ParamBase.h"
#include "backend/ParamEnumItem.h"

/* PVCAM */
#include "master.h"
#include "pvcam.h"

/* System */
#include <cassert>
#include <limits>
#include <map>
#include <string>
#include <type_traits>
#include <vector>

namespace pm {

template<typename T>
class Param : public ParamBase
{
public:
    using Base = ParamBase;

public:
    explicit Param(Camera* camera, uint32_t id)
        : Base(camera, id)
    {
        m_def    = std::make_unique<ParamValue<T>>();
        m_min    = std::make_unique<ParamValue<T>>();
        m_max    = std::make_unique<ParamValue<T>>();
        m_inc    = std::make_unique<ParamValue<T>>();
        m_cur    = std::make_unique<ParamValue<T>>();
        m_curTmp = std::make_unique<ParamValue<T>>();
        InitAttrValueMap();
    }

public: // Base
    virtual T GetDef() const
    {
        UpdateDefCache();
        return m_def->GetValueT<T>();
    }
    virtual T GetMin() const
    {
        UpdateMinCache();
        return m_min->GetValueT<T>();
    }
    virtual T GetMax() const
    {
        UpdateMaxCache();
        return m_max->GetValueT<T>();
    }
    virtual T GetInc() const
    {
        UpdateIncCache();
        return m_inc->GetValueT<T>();
    }

    virtual T GetCurNoCache() const
    {
        ReadValue(m_cur->GetPtr(), ATTR_CURRENT);
        return m_cur->GetValueT<T>();
    }
    virtual T GetCur() const
    {
        UpdateCurCache();
        return m_cur->GetValueT<T>();
    }
    virtual void SetCur(const T value)
    {
        WriteValue(&value, std::to_string(value).c_str());
    }

protected:
    virtual void UpdateDefCache() const override
    { ReadValueCached(m_def->GetPtr(), ATTR_DEFAULT); }
    virtual void UpdateMinCache() const override
    { ReadValueCached(m_min->GetPtr(), ATTR_MIN); }
    virtual void UpdateMaxCache() const override
    { ReadValueCached(m_max->GetPtr(), ATTR_MAX); }
    virtual void UpdateIncCache() const override
    { ReadValueCached(m_inc->GetPtr(), ATTR_INCREMENT); }
    virtual void UpdateCurCache() const override
    { ReadValueCached(m_cur->GetPtr(), ATTR_CURRENT); }
};

template<>
class Param<bool> : public ParamBase
{
public:
    using T = bool;
    using Base = ParamBase;

public:
    explicit Param(Camera* camera, uint32_t id)
        : Base(camera, id)
    {
        m_def    = std::make_unique<ParamValue<T>>();
        m_min    = std::make_unique<ParamValue<T>>();
        m_max    = std::make_unique<ParamValue<T>>();
        m_inc    = std::make_unique<ParamValue<T>>();
        m_cur    = std::make_unique<ParamValue<T>>();
        m_curTmp = std::make_unique<ParamValue<T>>();
        InitAttrValueMap();
    }

public: // Base
    virtual T GetDef() const
    {
        UpdateDefCache();
        return m_def->GetValueT<T>();
    }
    virtual T GetMin() const
    {
        UpdateMinCache();
        return m_min->GetValueT<T>();
    }
    virtual T GetMax() const
    {
        UpdateMaxCache();
        return m_max->GetValueT<T>();
    }
    virtual T GetInc() const
    {
        UpdateIncCache();
        return m_inc->GetValueT<T>();
    }

    virtual T GetCurNoCache() const
    {
        ReadValue(m_cur->GetPtr(), ATTR_CURRENT);
        return m_cur->GetValueT<T>();
    }
    virtual T GetCur() const
    {
        UpdateCurCache();
        return m_cur->GetValueT<T>();
    }
    virtual void SetCur(const T value)
    {
        auto val = static_cast<rs_bool>((value) ? TRUE : FALSE);
        WriteValue(&val, (value) ? "true" : "false");
    }

protected:
    virtual void UpdateDefCache() const override
    { ReadValueCached(m_def->GetPtr(), ATTR_DEFAULT); }
    virtual void UpdateMinCache() const override
    { ReadValueCached(m_min->GetPtr(), ATTR_MIN); }
    virtual void UpdateMaxCache() const override
    { ReadValueCached(m_max->GetPtr(), ATTR_MAX); }
    virtual void UpdateIncCache() const override
    { ReadValueCached(m_inc->GetPtr(), ATTR_INCREMENT); }
    virtual void UpdateCurCache() const override
    { ReadValueCached(m_cur->GetPtr(), ATTR_CURRENT); }
};

template<>
class Param<char*> : public ParamBase
{
public:
    using T = char*;
    using TB = std::remove_pointer<T>::type;
    using Base = ParamBase;

public:
    explicit Param(Camera* camera, uint32_t id)
        : Base(camera, id)
    {
        m_def    = std::make_unique<ParamValue<T>>();
        m_min    = std::make_unique<ParamValue<T>>();
        m_max    = std::make_unique<ParamValue<T>>();
        m_inc    = std::make_unique<ParamValue<T>>();
        m_cur    = std::make_unique<ParamValue<T>>();
        m_curTmp = std::make_unique<ParamValue<T>>();
        InitAttrValueMap();
    }

public: // Base
    virtual T GetDef() const
    {
        UpdateDefCache();
        return m_def->GetValueT<T>();
    }
    virtual T GetMin() const
    {
        UpdateMinCache();
        return m_min->GetValueT<T>();
    }
    virtual T GetMax() const
    {
        UpdateMaxCache();
        return m_max->GetValueT<T>();
    }
    virtual T GetInc() const
    {
        UpdateIncCache();
        return m_inc->GetValueT<T>();
    }

    virtual T GetCurNoCache() const
    {
        m_cur->Enlarge(GetCount());
        ReadValue(m_cur->GetPtr(), ATTR_CURRENT);
        return m_cur->GetValueT<T>();
    }
    virtual T GetCur() const
    {
        UpdateCurCache();
        return m_cur->GetValueT<T>();
    }
    virtual void SetCur(const TB* value)
    {
        assert(value);
        WriteValue(value, value);
    }

protected:
    virtual void UpdateDefCache() const override
    {
        if (m_def->Enlarge(GetCount()))
            m_attrIdCacheSetMap[ATTR_DEFAULT] = false;
        ReadValueCached(m_def->GetPtr(), ATTR_DEFAULT);
    }
    virtual void UpdateMinCache() const override
    {
        if (m_min->Enlarge(GetCount()))
            m_attrIdCacheSetMap[ATTR_MIN] = false;
        ReadValueCached(m_min->GetPtr(), ATTR_MIN);
    }
    virtual void UpdateMaxCache() const override
    {
        if (m_max->Enlarge(GetCount()))
            m_attrIdCacheSetMap[ATTR_MAX] = false;
        ReadValueCached(m_max->GetPtr(), ATTR_MAX);
    }
    virtual void UpdateIncCache() const override
    {
        if (m_inc->Enlarge(GetCount()))
            m_attrIdCacheSetMap[ATTR_INCREMENT] = false;
        ReadValueCached(m_inc->GetPtr(), ATTR_INCREMENT);
    }
    virtual void UpdateCurCache() const override
    {
        if (m_cur->Enlarge(GetCount()))
            m_attrIdCacheSetMap[ATTR_CURRENT] = false;
        ReadValueCached(m_cur->GetPtr(), ATTR_CURRENT);
    }
};

template<>
class Param<smart_stream_type*> : public ParamBase
{
public:
    using T = smart_stream_type*;
    using TB = std::remove_pointer<T>::type;
    using Base = ParamBase;

public:
    explicit Param(Camera* camera, uint32_t id)
        : Base(camera, id)
    {
        m_def    = std::make_unique<ParamValue<T>>();
        m_min    = std::make_unique<ParamValue<T>>();
        m_max    = std::make_unique<ParamValue<T>>();
        m_inc    = std::make_unique<ParamValue<T>>();
        m_cur    = std::make_unique<ParamValue<T>>();
        m_curTmp = std::make_unique<ParamValue<T>>();
        InitAttrValueMap();
    }

public: // Base
    virtual T GetDef() const
    {
        UpdateDefCache();
        return m_def->GetValueT<T>();
    }
    virtual T GetMin() const
    {
        UpdateMinCache();
        return m_min->GetValueT<T>();
    }
    virtual T GetMax() const
    {
        UpdateMaxCache();
        return m_max->GetValueT<T>();
    }
    virtual T GetInc() const
    {
        UpdateIncCache();
        return m_inc->GetValueT<T>();
    }

    virtual T GetCurNoCache() const
    {
        const auto size = static_cast<uint16_t>(GetMax()->entries);
        m_cur->Enlarge(size);
        m_cur->GetValueT<T>()->entries = size;
        ReadValue(m_cur->GetPtr(), ATTR_CURRENT);
        return m_cur->GetValueT<T>();
    }
    virtual T GetCur() const
    {
        UpdateCurCache();
        return m_cur->GetValueT<T>();
    }
    virtual void SetCur(const TB* value)
    {
        assert(value);
        assert(value->params);
        WriteValue(value, ParamValueBase::SmartStreamToString(value).c_str());
    }

protected:
    virtual void UpdateDefCache() const override
    {
        if (m_def->Enlarge(GetMax()->entries))
            m_attrIdCacheSetMap[ATTR_DEFAULT] = false;
        ReadValueCached(m_def->GetPtr(), ATTR_DEFAULT);
    }
    virtual void UpdateMinCache() const override
    {
        if (m_min->Enlarge(GetMax()->entries))
            m_attrIdCacheSetMap[ATTR_MIN] = false;
        ReadValueCached(m_min->GetPtr(), ATTR_MIN);
    }
    virtual void UpdateMaxCache() const override
    {
        // PVCAM fills only first two bytes with size
        ReadValueCached(m_max->GetPtr(), ATTR_MAX);
        m_max->Enlarge(m_max->GetValueT<T>()->entries);
    }
    virtual void UpdateIncCache() const override
    {
        if (m_inc->Enlarge(GetMax()->entries))
            m_attrIdCacheSetMap[ATTR_INCREMENT] = false;
        ReadValueCached(m_inc->GetPtr(), ATTR_INCREMENT);
    }
    virtual void UpdateCurCache() const override
    {
        const auto size = static_cast<uint16_t>(GetMax()->entries);
        if (m_cur->Enlarge(size))
            m_attrIdCacheSetMap[ATTR_CURRENT] = false;
        if (!m_attrIdCacheSetMap[ATTR_CURRENT])
            m_cur->GetValueT<T>()->entries = size;
        ReadValueCached(m_cur->GetPtr(), ATTR_CURRENT);
    }
};

class ParamEnum : public Param<ParamEnumItem::T>
{
public:
    using T = ParamEnumItem::T;
    using Base = Param<T>;

public:
    explicit ParamEnum(Camera* camera, uint32_t id);

public: // Base
    virtual void ResetCacheRangeFlags() override;

public:
    virtual void UpdateEnumItems();

    virtual const std::vector<ParamEnumItem>& GetItems() const;
    virtual const std::vector<std::string>& GetNames() const;
    virtual const std::vector<T>& GetValues() const;

    virtual bool HasValue(T value) const;
    virtual const std::string& GetValueName(T value) const;
    virtual const ParamEnumItem& GetItem(T value) const;

protected:
    virtual void ReadItemsCached() const;
    virtual void ReadItems() const;

protected:
    static bool HasValue(const std::vector<ParamEnumItem>& items, T value);

protected:
    mutable std::vector<ParamEnumItem> m_items{};
    mutable std::vector<T> m_values{};
    mutable std::vector<std::string> m_names{};
    mutable std::map<T, std::string> m_valueNameMap{};
    mutable std::map<T, ParamEnumItem> m_valueItemMap{};
    mutable bool m_itemsCacheSet{ false };
};

} // namespace pm

#endif /* PM_PARAM_H */
