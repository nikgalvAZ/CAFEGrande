/******************************************************************************/
/* Copyright (C) Teledyne Photometrics. All rights reserved.                  */
/******************************************************************************/
#include "backend/exceptions/ParamGetException.h"

/* Local */
#include "backend/Camera.h"
#include "backend/ParamInfoMap.h"

pm::ParamGetException::ParamGetException(const std::string& what,
        Camera* camera, uint32_t paramId, int16_t attrId)
    : Base(what + " - GetParam(paramId="
                + ((ParamInfoMap::HasParamInfo(paramId))
                    ? ParamInfoMap::GetParamInfo(paramId).GetName()
                    : std::to_string(paramId))
            + ", attrId=" + ParamInfoMap::GetParamAttrIdName(attrId) + ")",
            camera),
    m_paramId(paramId),
    m_attrId(attrId)
{
}

uint32_t pm::ParamGetException::GetParamId() const
{
    return m_paramId;
}

int16_t pm::ParamGetException::GetAttrId() const
{
    return m_attrId;
}
