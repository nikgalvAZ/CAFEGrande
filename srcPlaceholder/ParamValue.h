/******************************************************************************/
/* Copyright (C) Teledyne Photometrics. All rights reserved.                  */
/******************************************************************************/
#pragma once
#ifndef PM_PARAM_VALUE_H
#define PM_PARAM_VALUE_H

/* Local */
#include "backend/exceptions/Exception.h"
#include "backend/ParamDefinitions.h"
#include "backend/ParamValueBase.h"
#include "backend/Utils.h"

/* System */
#include <cstring> // strlen, memcpy
#include <string>

namespace pm {

template<typename T>
class ParamValue : public ParamValueBase
{
public:
    using Base = ParamValueBase;

public:
    ParamValue()
        : Base(ParamTypeFromT<Param<T>>::value)
    {}
    explicit ParamValue(T value)
        : Base(ParamTypeFromT<Param<T>>::value)
    { SetValue(value); }

    ParamValue(const ParamValue<T>& other) = delete;
    ParamValue<T>& operator=(const ParamValue<T>& other) = delete;

public:
    T GetValue() const
    { return m_value; }
    void SetValue(const T value)
    { m_value = value; }

    virtual void Set(const ParamValueBase& value) override
    {
        if (this == &value)
            return;
        Base::Set(value);
        const auto& other = static_cast<const ParamValue<T>&>(value);
        SetValue(other.GetValue());
    }

    virtual void FromString(const std::string& str) override
    {
        if (!Utils::StrToNumber<T>(str, m_value))
            throw Exception("Failed to convert '" + str + "' to PVCAM type");
    }
    virtual std::string ToString() const override
    { return std::to_string(m_value); }
    virtual void* GetPtr() const override
    { return const_cast<T*>(&m_value); }

protected:
    T m_value{};
};

template<>
class ParamValue<bool> : public ParamValueBase
{
public:
    using T = bool;
    using Base = ParamValueBase;

public:
    ParamValue()
        : Base(ParamTypeFromT<Param<T>>::value)
    {}
    explicit ParamValue(T value)
        : Base(ParamTypeFromT<Param<T>>::value)
    { SetValue(value); }

    ParamValue(const ParamValue<T>& other) = delete;
    ParamValue<T>& operator=(const ParamValue<T>& other) = delete;

public:
    T GetValue() const
    { return m_rsValue != FALSE; }
    void SetValue(const T value)
    { m_rsValue = static_cast<rs_bool>((value) ? TRUE : FALSE); }

    virtual void Set(const ParamValueBase& value) override
    {
        if (this == &value)
            return;
        Base::Set(value);
        const auto& other = static_cast<const ParamValue<T>&>(value);
        SetValue(other.GetValue());
    }

    virtual void FromString(const std::string& str) override
    {
        T value;
        if (!Utils::StrToBool(str, value))
            throw Exception("Failed to convert '" + str + "' to PVCAM type");
        SetValue(value);
    }
    virtual std::string ToString() const override
    { return (m_rsValue == FALSE) ? "false" : "true"; }
    virtual void* GetPtr() const override
    { return const_cast<rs_bool*>(&m_rsValue); }

protected:
    rs_bool m_rsValue{ static_cast<rs_bool>(FALSE) };
};

template<>
class ParamValue<char*> : public ParamValueBase
{
public:
    using T = char*;
    using TB = std::remove_pointer<T>::type;
    using Base = ParamValueBase;

public:
    ParamValue()
        : Base(ParamTypeFromT<Param<T>>::value)
    {}
    explicit ParamValue(const TB* value)
        : Base(ParamTypeFromT<Param<T>>::value)
    { SetValue(value); }
    virtual ~ParamValue()
    { delete [] m_value; }

    ParamValue(const ParamValue<T>& other) = delete;
    ParamValue<T>& operator=(const ParamValue<T>& other) = delete;

public:
    T GetValue() const
    { return m_value; }
    void SetValue(const TB* value)
    {
        if (m_value == value)
            return;
        if (!value)
        {
            m_size = 0;
            delete m_value;
            m_value = nullptr;
            return;
        }
        Enlarge(strlen(value) + 1);
        std::memcpy(m_value, value, m_size * sizeof(TB));
    }

    virtual void Set(const ParamValueBase& value) override
    {
        if (this == &value)
            return;
        Base::Set(value);
        const auto& other = static_cast<const ParamValue<T>&>(value);
        SetValue(other.GetValue());
    }

    virtual void FromString(const std::string& str) override
    {
        Enlarge(str.length() + 1);
        std::memcpy(m_value, str.c_str(), m_size * sizeof(TB));
    }
    virtual std::string ToString() const override
    { return std::string((m_value) ? m_value : "<null>"); }
    virtual void* GetPtr() const override
    { return m_value; }

protected:
    virtual bool Enlarge(size_t size) override
    {
        if (size <= m_size)
            return false;
        TB* newValue = new TB[size]();
        if (m_value)
        {
            std::memcpy(newValue, m_value, m_size);
            delete [] m_value;
        }
        m_value = newValue;
        m_size = size;
        return true;
    }

protected:
    T m_value{ nullptr };
};

template <>
class ParamValue<smart_stream_type*> : public ParamValueBase
{
public:
    using T = smart_stream_type*;
    using TB = std::remove_pointer<T>::type;
    using Base = ParamValueBase;

public:
    ParamValue()
        : Base(ParamTypeFromT<Param<T>>::value)
    {}
    explicit ParamValue(const TB* value)
        : Base(ParamTypeFromT<Param<T>>::value)
    { SetValue(value); }
    virtual ~ParamValue()
    {
        delete [] m_value->params;
    }

    ParamValue(const ParamValue<T>& other) = delete;
    ParamValue<T>& operator=(const ParamValue<T>& other) = delete;

public:
    T GetValue() const
    { return m_value; }
    void SetValue(const TB* value)
    {
        if (m_value == value)
            return;
        if (!value || !value->params)
        {
            m_size = 0;
            delete [] m_value->params;
            m_value->entries = 0;
            m_value->params = nullptr;
            return;
        }
        Enlarge(value->entries);
        m_value->entries = value->entries;
        std::memcpy(m_value->params, value->params,
                m_value->entries * sizeof(uint32_t));
    }

    virtual void Set(const ParamValueBase& value) override
    {
        if (this == &value)
            return;
        Base::Set(value);
        const auto& other = static_cast<const ParamValue<T>&>(value);
        SetValue(other.GetValue());
    }

    virtual void FromString(const std::string& str) override
    {
        uint16_t capacity = static_cast<uint16_t>(m_size);
        ParamValueBase::SmartStreamFromString(str, *m_value, &capacity);
        m_size = capacity;
    }
    virtual std::string ToString() const override
    { return ParamValueBase::SmartStreamToString(m_value); }
    virtual void* GetPtr() const override
    { return m_value; }

protected:
    virtual bool Enlarge(size_t size) override
    {
        if (size <= m_size)
            return false;
        uint32_t* newParams = new uint32_t[size]();
        if (m_value->params)
        {
            std::memcpy(newParams, m_value->params, m_size * sizeof(uint32_t));
            delete [] m_value->params;
        }
        m_value->params = newParams;
        m_size = size;
        return true;
    }

protected:
    TB m_valueBuffer{ 0, nullptr };
    T m_value{ &m_valueBuffer };
};

} // namespace pm

#endif /* PM_PARAM_VALUE_H */
