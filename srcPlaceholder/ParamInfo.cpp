/******************************************************************************/
/* Copyright (C) Teledyne Photometrics. All rights reserved.                  */
/******************************************************************************/
#include "backend/ParamInfo.h"

pm::ParamInfo::ParamInfo(uint32_t id, const std::string& name,
        uint16_t type, bool needsSetup, const std::string& groupName)
    : m_id(id),
    m_name(name),
    m_type(type),
    m_needsSetup(needsSetup),
    m_groupName(groupName)
{
}

pm::ParamInfo::ParamInfo(uint32_t id, const std::string& name,
        uint16_t type, bool needsSetup, const std::string& groupName,
        const std::vector<uint32_t>& dependants)
    : m_id(id),
    m_name(name),
    m_type(type),
    m_needsSetup(needsSetup),
    m_groupName(groupName),
    m_directDeps(dependants)
{
}

uint32_t pm::ParamInfo::GetId() const
{
    return m_id;
}

const std::string& pm::ParamInfo::GetName() const
{
    return m_name;
}

uint16_t pm::ParamInfo::GetType() const
{
    return m_type;
}

bool pm::ParamInfo::NeedsSetup() const
{
    return m_needsSetup;
}

const std::string& pm::ParamInfo::GetGroupName() const
{
    return m_groupName;
}

const std::vector<uint32_t>& pm::ParamInfo::GetDirectDeps() const
{
    return m_directDeps;
}

const std::vector<uint32_t>& pm::ParamInfo::GetRecursiveDeps() const
{
    return m_recursiveDeps;
}

void pm::ParamInfo::SetRecursiveDeps(const std::vector<uint32_t>& deps)
{
    m_recursiveDeps = deps;
}
