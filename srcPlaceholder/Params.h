/******************************************************************************/
/* Copyright (C) Teledyne Photometrics. All rights reserved.                  */
/******************************************************************************/
#pragma once
#ifndef PM_PARAMS_H
#define PM_PARAMS_H

/* Local */
#include "backend/Param.h"
#include "backend/ParamDefinitions.h"

/* System */
#include <map>
#include <memory> // std::shared_ptr

namespace pm {

class Camera;

class Params
{
public:
    explicit Params(Camera* camera)
        : m_camera(camera)
    {}
    virtual ~Params()
    {}

public:
    const std::map<uint32_t, std::shared_ptr<ParamBase>>& GetParams() const
    { return m_params; }

    template<uint32_t id>
    std::shared_ptr<typename ParamT<id>::T> Get() const
    { return std::static_pointer_cast<typename ParamT<id>::T>(m_params.at(id)); }

protected:
    Camera* m_camera;

    // Map for parameter ID and its instance
    std::map<uint32_t, std::shared_ptr<ParamBase>> m_params{};
};

} // namespace pm

#endif /* PM_PARAMS_H */
