/******************************************************************************/
/* Copyright (C) Teledyne Photometrics. All rights reserved.                  */
/******************************************************************************/
#include "backend/exceptions/ParamSetException.h"

/* Local */
#include "backend/Camera.h"
#include "backend/ParamInfoMap.h"

pm::ParamSetException::ParamSetException(const std::string& what,
        Camera* camera, uint32_t paramId)
    : Base(what + " - SetParam(paramId="
                + ((ParamInfoMap::HasParamInfo(paramId))
                    ? ParamInfoMap::GetParamInfo(paramId).GetName()
                    : std::to_string(paramId))
                + ")",
            camera),
    m_paramId(paramId)
{
}

uint32_t pm::ParamSetException::GetParamId() const
{
    return m_paramId;
}
