/******************************************************************************/
/* Copyright (C) Teledyne Photometrics. All rights reserved.                  */
/******************************************************************************/
#pragma once
#ifndef PM_PARAM_ENUM_ITEM_H
#define PM_PARAM_ENUM_ITEM_H

/* System */
#include <string>

namespace pm {

class ParamEnumItem
{
public:
    using T = int32_t;

public:
    ParamEnumItem();
    ParamEnumItem(T value, const std::string& name);

public:
    T GetValue() const;
    const std::string& GetName() const;

private:
    T m_value{ 0 };
    std::string m_name{};
};

} // namespace pm

#endif /* PM_PARAM_ENUM_ITEM_H */
