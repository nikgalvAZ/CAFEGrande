/******************************************************************************/
/* Copyright (C) Teledyne Photometrics. All rights reserved.                  */
/******************************************************************************/
#include "backend/FakeParams.h"

/* Local */
#include "backend/FakeCamera.h"
#include "backend/FakeParam.h"
#include "backend/FakeParams.h"
#include "backend/ParamInfoMap.h"

pm::FakeParams::FakeParams(FakeCamera* camera)
    : Params(camera)
{

    // The 'type' has to be TYPE_* value, not a variable holding the value
    #define ADD_PAR(id, type) { \
        std::shared_ptr<ParamBase> p = \
            std::make_shared<ParamTypeToFakeT<(type)>::T>(camera, (id)); \
        m_params[p->GetId()] = p; \
    }

    const auto& paramInfoMap = ParamInfoMap::GetMap();
    for (const auto& paramInfoPair : paramInfoMap)
    {
        const auto id = paramInfoPair.first;
        const auto& paramInfo = paramInfoPair.second;

        const auto type = paramInfo.GetType();
        switch (type)
        {
        case TYPE_ENUM                 : ADD_PAR(id, TYPE_ENUM                 ); break;
        case TYPE_BOOLEAN              : ADD_PAR(id, TYPE_BOOLEAN              ); break;
        case TYPE_INT8                 : ADD_PAR(id, TYPE_INT8                 ); break;
        case TYPE_INT16                : ADD_PAR(id, TYPE_INT16                ); break;
        case TYPE_INT32                : ADD_PAR(id, TYPE_INT32                ); break;
        case TYPE_INT64                : ADD_PAR(id, TYPE_INT64                ); break;
        case TYPE_UNS8                 : ADD_PAR(id, TYPE_UNS8                 ); break;
        case TYPE_UNS16                : ADD_PAR(id, TYPE_UNS16                ); break;
        case TYPE_UNS32                : ADD_PAR(id, TYPE_UNS32                ); break;
        case TYPE_UNS64                : ADD_PAR(id, TYPE_UNS64                ); break;
        case TYPE_FLT32                : ADD_PAR(id, TYPE_FLT32                ); break;
        case TYPE_FLT64                : ADD_PAR(id, TYPE_FLT64                ); break;
        case TYPE_CHAR_PTR             : ADD_PAR(id, TYPE_CHAR_PTR             ); break;
        case TYPE_SMART_STREAM_TYPE_PTR: ADD_PAR(id, TYPE_SMART_STREAM_TYPE_PTR); break;
        case TYPE_SMART_STREAM_TYPE    : // Not used in PVCAM
        case TYPE_VOID_PTR             : // Not used in PVCAM
        case TYPE_VOID_PTR_PTR         : // Not used in PVCAM
        default                        : assert(false);
        }
    }

    #undef ADD_PAR
}
