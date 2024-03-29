/******************************************************************************/
/* Copyright (C) Teledyne Photometrics. All rights reserved.                  */
/******************************************************************************/
#pragma once
#ifndef PM_PARAM_INFO_H
#define PM_PARAM_INFO_H

/* System */
#include <string>
#include <vector>

namespace pm {

class ParamInfo
{
public:
    explicit ParamInfo(uint32_t id, const std::string& name,
            uint16_t type, bool needsSetup, const std::string& groupName);
    explicit ParamInfo(uint32_t id, const std::string& name,
            uint16_t type, bool needsSetup, const std::string& groupName,
            const std::vector<uint32_t>& dependants);

public:
    uint32_t GetId() const;
    const std::string& GetName() const;
    uint16_t GetType() const;
    bool NeedsSetup() const;
    const std::string& GetGroupName() const;
    const std::vector<uint32_t>& GetDirectDeps() const;

    const std::vector<uint32_t>& GetRecursiveDeps() const;
    void SetRecursiveDeps(const std::vector<uint32_t>& deps);

private:
    uint32_t m_id;
    std::string m_name;
    uint16_t m_type;
    bool m_needsSetup;
    std::string m_groupName;
    std::vector<uint32_t> m_directDeps{};
    std::vector<uint32_t> m_recursiveDeps{};
};

} // namespace pm

#endif /* PM_PARAM_INFO_H */
