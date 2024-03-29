/******************************************************************************/
/* Copyright (C) Teledyne Photometrics. All rights reserved.                  */
/******************************************************************************/
#pragma once
#ifndef PM_FAKE_PARAMS_H
#define PM_FAKE_PARAMS_H

/* Local */
#include "backend/Params.h"

namespace pm {

class FakeCamera;

class FakeParams : public Params
{
public:
    explicit FakeParams(FakeCamera* camera);
};

} // namespace pm

#endif /* PM_FAKE_PARAMS_H */
