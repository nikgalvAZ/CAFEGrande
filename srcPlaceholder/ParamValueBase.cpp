/******************************************************************************/
/* Copyright (C) Teledyne Photometrics. All rights reserved.                  */
/******************************************************************************/
#include "backend/ParamValueBase.h"

/* Local */
#include "backend/exceptions/Exception.h"
#include "backend/Utils.h"

/* System */
#include <cstring> // memcpy

pm::ParamValueBase::ParamValueBase(uint16_t type)
    : m_type(type)
{
}

pm::ParamValueBase::~ParamValueBase()
{
}

uint16_t pm::ParamValueBase::GetType() const
{
    return m_type;
}

void pm::ParamValueBase::Set(const ParamValueBase& value)
{
    if (value.GetType() != GetType())
        throw Exception("Failed to set value from different type");
}

void pm::ParamValueBase::SmartStreamFromString(const std::string& str,
        smart_stream_type& value, uint16_t* valueCapacity)
{
    uint16_t capacity = (valueCapacity) ? *valueCapacity : 0;

    if (str == "<null>")
    {
        delete [] value.params;
        value.params = nullptr;
        value.entries = 0;
        capacity = 0;
    }
    else
    {
        std::vector<uint32_t> params;
        if (!Utils::StrToArray(params, str, ','))
            throw Exception("Failed to convert string to list of numbers");
        const size_t maxSize = std::numeric_limits<uint16_t>::max();
        if (params.size() > maxSize)
            throw Exception("Too many numbers, max. is " + std::to_string(maxSize));

        value.entries = static_cast<uint16_t>(params.size());
        if (value.entries > capacity || !value.params)
        {
            delete [] value.params;
            value.params = new uint32_t[value.entries]();
            capacity = value.entries;
        }
        std::memcpy(value.params, params.data(), value.entries * sizeof(uint32_t));
    }

    if (valueCapacity)
    {
        *valueCapacity = capacity;
    }
}

std::string pm::ParamValueBase::SmartStreamToString(
        const smart_stream_type* value)
{
    if (!value || !value->params)
        return "<null>";

    return Utils::ArrayToStr(value->params, value->entries, ',');
}
