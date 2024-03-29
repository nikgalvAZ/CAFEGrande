/******************************************************************************/
/* Copyright (C) Teledyne Photometrics. All rights reserved.                  */
/******************************************************************************/
#pragma once
#ifndef PM_PARAM_GET_EXCEPTION_H
#define PM_PARAM_GET_EXCEPTION_H

/* Local */
#include "backend/exceptions/CameraException.h"

/* System */
#include <cstdint>

namespace pm {

class ParamGetException : public pm::CameraException
{
public:
    using Base = CameraException;

public:
    explicit ParamGetException(const std::string& what, Camera* camera,
            uint32_t paramId, int16_t attrId);

public:
    uint32_t GetParamId() const;
    int16_t GetAttrId() const;

private:
    uint32_t m_paramId;
    int16_t m_attrId;
};

} // namespace pm

#endif /* PM_PARAM_GET_EXCEPTION_H */
