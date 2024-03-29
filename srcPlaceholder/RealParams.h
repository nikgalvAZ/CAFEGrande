/******************************************************************************/
/* Copyright (C) Teledyne Photometrics. All rights reserved.                  */
/******************************************************************************/
#pragma once
#ifndef PM_REAL_PARAMS_H
#define PM_REAL_PARAMS_H

/* Local */
#include "backend/Params.h"

namespace pm {

class RealCamera;

class RealParams : public Params
{
public:
    explicit RealParams(RealCamera* camera);
};

} // namespace pm

#endif /* PM_REAL_PARAMS_H */
