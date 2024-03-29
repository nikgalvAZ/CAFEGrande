/******************************************************************************/
/* Copyright (C) Teledyne Photometrics. All rights reserved.                  */
/******************************************************************************/
#pragma once
#ifndef PM_PARAM_VALUE_BASE_H
#define PM_PARAM_VALUE_BASE_H

/* Local */
#include "backend/ParamDefinitions.h"

/* System */
#include <string>
#include <type_traits>

namespace pm {

template<typename T>
class ParamValue;

class ParamValueBase
{
public:
    explicit ParamValueBase(uint16_t type);
    virtual ~ParamValueBase();

    ParamValueBase() = delete;
    ParamValueBase(const ParamValueBase& other) = delete;
    ParamValueBase& operator=(const ParamValueBase& other) = delete;

public:
    uint16_t GetType() const;

    template <typename T>
    T GetValueT() const
    { return dynamic_cast<const ParamValue<T>*>(this)->GetValue(); }
    template <typename T>
    void SetValueT(std::enable_if_t<
                !std::is_pointer<T>::value,
                T> value)
    { dynamic_cast<ParamValue<T>*>(this)->SetValue(value); }
    template <typename T>
    void SetValueT(std::enable_if_t<
                std::is_pointer<T>::value,
                const std::remove_pointer_t<T>*> value)
    { dynamic_cast<ParamValue<T>*>(this)->SetValue(value); }

    virtual void Set(const ParamValueBase& value);

    virtual void FromString(const std::string& str) = 0;
    virtual std::string ToString() const = 0;
    virtual void* GetPtr() const = 0;

public:
    static void SmartStreamFromString(const std::string& str,
            smart_stream_type& value, uint16_t* valueCapacity = nullptr);
    static std::string SmartStreamToString(const smart_stream_type* value);

protected:
    template<typename TT> friend class Param;
    template<typename TT> friend class FakeParam;

    // Returns true if resized
    virtual bool Enlarge(size_t /*size*/)
    { return false; }

protected:
    const uint16_t m_type;
    size_t m_size{ 0 };
};

} // namespace pm

#endif /* PM_PARAM_VALUE_BASE_H */
