/******************************************************************************/
/* Copyright (C) Teledyne Photometrics. All rights reserved.                  */
/******************************************************************************/
#pragma once
#ifndef PM_PARAM_SET_EXCEPTION_H
#define PM_PARAM_SET_EXCEPTION_H

/* Local */
#include "backend/exceptions/CameraException.h"

/* System */
#include <cstdint>

namespace pm {

class ParamSetException : public pm::CameraException
{
public:
    using Base = CameraException;

public:
    explicit ParamSetException(const std::string& what, Camera* camera,
            uint32_t paramId);

public:
    uint32_t GetParamId() const;

private:
    uint32_t m_paramId;
};

} // namespace pm

#endif /* PM_PARAM_SET_EXCEPTION_H */
