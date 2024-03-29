/******************************************************************************/
/* Copyright (C) Teledyne Photometrics. All rights reserved.                  */
/******************************************************************************/
#include "backend/ParamEnumItem.h"

pm::ParamEnumItem::ParamEnumItem()
{
}

pm::ParamEnumItem::ParamEnumItem(T value, const std::string& name)
    : m_value(value),
    m_name(name)
{
}

pm::ParamEnumItem::T pm::ParamEnumItem::GetValue() const
{
    return m_value;
}

const std::string& pm::ParamEnumItem::GetName() const
{
    return m_name;
}
