/******************************************************************************/
/* Copyright (C) Teledyne Photometrics. All rights reserved.                  */
/******************************************************************************/
#pragma once
#ifndef PM_FAKE_PARAM_H
#define PM_FAKE_PARAM_H

/* Local */
#include "backend/exceptions/ParamSetException.h"
#include "backend/FakeParamBase.h"
#include "backend/ParamDefinitions.h"
#include "backend/Utils.h"

/* System */
#include <cassert>
#include <cstring> // strlen
#include <limits>
#include <vector>

namespace pm {

class FakeCamera;

// 'this->' has to be used all around to satisfy strict gcc

template<typename T>
class FakeParam : public Param<T>, public FakeParamBase<T>
{
public:
    using Base = Param<T>;
    using FakeBase = FakeParamBase<T>;

public:
    explicit FakeParam(FakeCamera* camera, uint32_t id)
        : Base(camera, id),
        FakeBase(this)
    {
        this->m_type->SetValue(ParamTypeFromT<Base>::value);
    }

protected: // Base
    virtual void ReadValue(void* /*value*/, int16_t attrId) const override
    {
        this->CheckGetAccess(attrId);
        this->m_attrIdCacheSetMap[attrId] = true;
    }
    virtual void WriteValue(const void* value, const char* valueAsStr) override
    {
        WriteValueNoHandlers(value, valueAsStr);
        this->InvokeChangeHandlers();
    }

protected:
    friend class FakeCamera;

    virtual void SetCurNoHandlers(const T value, bool checkAccess = true)
    {
        WriteValueNoHandlers(&value, std::to_string(value).c_str(), checkAccess);
    }
    virtual void WriteValueNoHandlers(const void* value, const char* valueAsStr,
            bool checkAccess = true)
    {
        assert(value);

        if (checkAccess)
        {
            this->CheckSetAccess();
        }

        const T val = *static_cast<const T*>(value);
        const T min = static_cast<ParamValue<T>*>(this->m_min.get())->GetValue();
        const T max = static_cast<ParamValue<T>*>(this->m_max.get())->GetValue();
        const T inc = static_cast<ParamValue<T>*>(this->m_inc.get())->GetValue();
        if (val < min || val > max)
        {
            this->SetError(FakeCameraErrors::CannotSetValue);
            throw ParamSetException("Value " + std::string(valueAsStr)
                    + " out of range <min=" + this->m_min->ToString().c_str()
                    + ",max=" + this->m_max->ToString().c_str() + ">",
                    this->m_camera, this->m_id);
        }
        if (!IsValueIncAligned(val, min, inc))
        {
            this->SetError(FakeCameraErrors::CannotSetValue);
            throw ParamSetException("Value " + std::string(valueAsStr)
                    + " out of step alignment <min=" + this->m_min->ToString().c_str()
                    + ",step=" + this->m_inc->ToString().c_str()
                    + ",max=" + this->m_max->ToString().c_str() + ">",
                    this->m_camera, this->m_id);
        }
        this->m_cur->template SetValueT<T>(val);
        this->m_attrIdCacheSetMap[ATTR_CURRENT] = false;
    }

    virtual FakeBase& ChangeRangeAttrs(uint32_t count, T def, T min, T max, T inc)
    {
        //assert(min <= max); // Implied by next two asserts
        assert(min <= def);
        assert(def <= max);
#ifdef _DEBUG
        CheckRange(count, min, max, inc);
#endif

        this->m_count->SetValue(count);
        this->m_def->template SetValueT<T>(def);
        this->m_min->template SetValueT<T>(min);
        this->m_max->template SetValueT<T>(max);
        this->m_inc->template SetValueT<T>(inc);
        this->m_cur->template SetValueT<T>(def);

        this->m_rangeAttrsSet = true;

        return *this;
    }

private:
    template<typename TT>
    typename std::enable_if<!std::numeric_limits<TT>::is_integer, bool>::type
        IsValueIncAligned(TT /*val*/, TT /*min*/, TT /*inc*/)
    {
        return true; // Not checking floating point numbers
    }
    template<typename TT>
    typename std::enable_if<std::numeric_limits<TT>::is_integer, bool>::type
        IsValueIncAligned(TT val, TT min, TT inc)
    {
        return inc == 0 || (val - min) % inc == 0;
    }

#ifdef _DEBUG
private:
    template<typename TT, std::enable_if_t<!std::is_integral<TT>::value, int> = 0>
    void CheckRange(uint32_t, TT, TT, TT)
    {}
    template<typename TT, std::enable_if_t<std::is_integral<TT>::value, int> = 0>
    void CheckRange(uint32_t count, TT min, TT max, TT inc)
    {
        using UTT = typename std::make_unsigned<TT>::type;
        if (count > 0 && inc != 0)
        {
            // Check number of steps
            assert((((static_cast<UTT>(max) - min) / inc) + 1)
                    == static_cast<UTT>(count));
        }
    }
#endif
};

template<>
class FakeParam<bool> : public Param<bool>, public FakeParamBase<bool>
{
public:
    using T = Param<bool>::T;
    using Base = Param<T>;
    using FakeBase = FakeParamBase<T>;

public:
    explicit FakeParam(FakeCamera* camera, uint32_t id)
        : Base(camera, id),
        FakeBase(this)
    {
        this->m_type->SetValue(ParamTypeFromT<Base>::value);
    }

protected: // Base
    virtual void ReadValue(void* /*value*/, int16_t attrId) const override
    {
        this->CheckGetAccess(attrId);
        m_attrIdCacheSetMap[attrId] = true;
    }
    virtual void WriteValue(const void* value, const char* valueAsStr) override
    {
        WriteValueNoHandlers(value, valueAsStr);
        this->InvokeChangeHandlers();
    }

protected:
    friend class FakeCamera;

    virtual void SetCurNoHandlers(const T value, bool checkAccess = true)
    {
        WriteValueNoHandlers(&value, std::to_string(value).c_str(), checkAccess);
    }
    virtual void WriteValueNoHandlers(const void* value, const char* /*valueAsStr*/,
            bool checkAccess = true)
    {
        assert(value);

        if (checkAccess)
        {
            this->CheckSetAccess();
        }

        const T val = *static_cast<const T*>(value);
        this->m_cur->SetValueT<T>(val);
        this->m_attrIdCacheSetMap[ATTR_CURRENT] = false;
    }

    virtual FakeBase& ChangeRangeAttrs(T def)
    {
        this->m_count->SetValue(0);
        this->m_def->SetValueT<T>(def);
        this->m_min->SetValueT<T>(false);
        this->m_max->SetValueT<T>(true);
        this->m_inc->SetValueT<T>(false);
        this->m_cur->SetValueT<T>(def);

        this->m_rangeAttrsSet = true;

        return *this;
    }
};

template<>
class FakeParam<char*> : public Param<char*>, public FakeParamBase<char*>
{
public:
    using T = Param<char*>::T;
    using TB = std::remove_pointer<T>::type;
    using Base = Param<T>;
    using FakeBase = FakeParamBase<T>;

public:
    explicit FakeParam(FakeCamera* camera, uint32_t id)
        : Base(camera, id),
        FakeBase(this)
    {
        this->m_type->SetValue(ParamTypeFromT<Base>::value);
    }

protected: // Base
    virtual void ReadValue(void* /*value*/, int16_t attrId) const override
    {
        this->CheckGetAccess(attrId);
        m_attrIdCacheSetMap[attrId] = true;
    }
    virtual void WriteValue(const void* value, const char* valueAsStr) override
    {
        WriteValueNoHandlers(value, valueAsStr);
        this->InvokeChangeHandlers();
    }

protected:
    friend class FakeCamera;

    virtual void SetCurNoHandlers(const TB* value, bool checkAccess = true)
    {
        WriteValueNoHandlers(value, value, checkAccess);
    }
    virtual void WriteValueNoHandlers(const void* value, const char* /*valueAsStr*/,
            bool checkAccess = true)
    {
        assert(value);

        if (checkAccess)
        {
            this->CheckSetAccess();
        }

        auto val = static_cast<const TB*>(value);
        const size_t count = strlen(val) + 1;
        if (count > this->m_count->GetValue())
        {
            this->SetError(FakeCameraErrors::CannotSetValue);
            throw ParamSetException("FakeParam::Set<char*> string longer than "
                    + std::to_string(this->m_count->GetValue() - 1) + " characters",
                    this->m_camera, this->m_id);
        }
        this->m_cur->SetValueT<T>(val);
        this->m_attrIdCacheSetMap[ATTR_CURRENT] = false;
    }

    virtual FakeBase& ChangeRangeAttrs(const TB* def)
    {
        assert(def);
        const size_t count = strlen(def) + 1;
        assert(count <= static_cast<size_t>(std::numeric_limits<uint32_t>::max()));

        this->m_count->SetValue(static_cast<uint32_t>(count));
        this->m_def->Enlarge(m_count->GetValue());
        this->m_min->Enlarge(m_count->GetValue());
        this->m_max->Enlarge(m_count->GetValue());
        this->m_inc->Enlarge(m_count->GetValue());
        this->m_cur->Enlarge(m_count->GetValue());
        this->m_def->SetValueT<T>("");
        this->m_min->SetValueT<T>("");
        this->m_max->SetValueT<T>("");
        this->m_inc->SetValueT<T>("");
        this->m_cur->SetValueT<T>(def);

        this->m_rangeAttrsSet = true;
        return *this;
    }
};

template<>
class FakeParam<smart_stream_type*> : public Param<smart_stream_type*>,
                                      public FakeParamBase<smart_stream_type*>
{
public:
    using T = Param<smart_stream_type*>::T;
    using TB = std::remove_pointer<T>::type;
    using Base = Param<T>;
    using FakeBase = FakeParamBase<T>;

public:
    explicit FakeParam(FakeCamera* camera, uint32_t id)
        : Base(camera, id),
        FakeBase(this)
    {
        this->m_type->SetValue(ParamTypeFromT<Base>::value);
    }

protected: // Base
    virtual T GetCurNoCache() const override
    {
        const auto size = static_cast<uint16_t>(GetMax()->entries);
        m_cur->Enlarge(size);
        ReadValue(m_cur->GetPtr(), ATTR_CURRENT);
        return m_cur->GetValueT<T>();
    }
    virtual void UpdateCurCache() const override
    {
        const auto size = static_cast<uint16_t>(GetMax()->entries);
        if (m_cur->Enlarge(size))
            m_attrIdCacheSetMap[ATTR_CURRENT] = false;
        ReadValueCached(m_cur->GetPtr(), ATTR_CURRENT);
    }
    virtual void ReadValue(void* value, int16_t attrId) const override
    {
        assert(value);
        this->CheckGetAccess(attrId);
        if (m_cur->GetPtr() == value)
        {
            auto val = static_cast<const TB*>(value);
            assert(val->params);
            if (val->entries < this->m_cur->GetValueT<T>()->entries)
            {
                this->SetError(FakeCameraErrors::CannotGetValue);
                throw ParamSetException(
                        "FakeParam::Get<smart_stream_type*> capacity less than "
                        + std::to_string(this->m_cur->GetValueT<T>()->entries),
                        this->m_camera, this->m_id);
            }
        }
        m_attrIdCacheSetMap[attrId] = true;
    }
    virtual void WriteValue(const void* value, const char* valueAsStr) override
    {
        WriteValueNoHandlers(value, valueAsStr);
        this->InvokeChangeHandlers();
    }

protected:
    friend class FakeCamera;

    virtual void SetCurNoHandlers(const TB* value, bool checkAccess = true)
    {
        WriteValueNoHandlers(value,
                ParamValueBase::SmartStreamToString(value).c_str(), checkAccess);
    }
    virtual void WriteValueNoHandlers(const void* value, const char* /*valueAsStr*/,
            bool checkAccess = true)
    {
        assert(value);

        if (checkAccess)
        {
            this->CheckSetAccess();
        }

        auto val = static_cast<const TB*>(value);
        assert(val->params);
        if (val->entries > this->m_max->GetValueT<T>()->entries)
        {
            this->SetError(FakeCameraErrors::CannotSetValue);
            throw ParamSetException(
                    "FakeParam::Set<smart_stream_type*> number of items greater than "
                    + std::to_string(this->m_max->GetValueT<T>()->entries),
                    this->m_camera, this->m_id);
        }
        this->m_cur->SetValueT<T>(val);
        this->m_attrIdCacheSetMap[ATTR_CURRENT] = false;
    }

    virtual FakeBase& ChangeRangeAttrs(uint16_t max,
            const std::vector<uint32_t>& items)
    {
        assert(max > 0);
        assert(!items.empty());
        assert(items.size() <= max);

        const auto size = static_cast<uint16_t>(items.size());

        this->m_count->SetValue(size);
        this->m_def->Enlarge(max);
        this->m_def->GetValueT<T>()->entries = 1;
        this->m_min->Enlarge(max);
        this->m_min->GetValueT<T>()->entries = 1;
        this->m_max->Enlarge(max);
        this->m_max->GetValueT<T>()->entries = max;
        this->m_inc->Enlarge(max);
        this->m_inc->GetValueT<T>()->entries = 1;
        this->m_cur->Enlarge(max);
        auto cur = this->m_cur->GetValueT<T>();
        cur->entries = size;
        std::memcpy(cur->params, items.data(), size * sizeof(uint32_t));

        this->m_rangeAttrsSet = true;
        return *this;
    }
};

class FakeParamEnum : public ParamEnum, public FakeParamBase<ParamEnum::T>
{
public:
    using T = ParamEnum::T;
    using Base = ParamEnum;
    using FakeBase = FakeParamBase<T>;

public:
    explicit FakeParamEnum(FakeCamera* camera, uint32_t id)
        : Base(camera, id),
        FakeBase(this)
    {
        this->m_type->SetValue(TYPE_ENUM);
    }

protected: // Base
    virtual void ReadValue(void* /*value*/, int16_t attrId) const override
    {
        this->CheckGetAccess(attrId);
        m_attrIdCacheSetMap[attrId] = true;
    }
    virtual void WriteValue(const void* value, const char* valueAsStr) override
    {
        WriteValueNoHandlers(value, valueAsStr);
        this->InvokeChangeHandlers();
    }

protected:
    friend class FakeCamera;

    virtual void SetCurNoHandlers(const T value, bool checkAccess = true)
    {
        WriteValueNoHandlers(&value, std::to_string(value).c_str(), checkAccess);
    }
    virtual void WriteValueNoHandlers(const void* value, const char* valueAsStr,
            bool checkAccess = true)
    {
        assert(value);

        if (checkAccess)
        {
            this->CheckSetAccess();
        }

        const T val = *static_cast<const T*>(value);
        const bool hasValInItems = HasValue(val);
        assert(hasValInItems);
        if (!hasValInItems)
        {
            this->SetError(FakeCameraErrors::CannotSetValue);
            throw ParamSetException("Value " + std::string(valueAsStr)
                    + " is not in items list",
                    this->m_camera, this->m_id);
        }
        this->m_cur->SetValueT<T>(val);
        this->m_attrIdCacheSetMap[ATTR_CURRENT] = false;
    }

    virtual void ReadItems() const {}

    virtual FakeBase& ChangeRangeAttrs(T def,
            const std::vector<ParamEnumItem>& items);
};

} // namespace pm

#endif /* PM_FAKE_PARAM_H */
