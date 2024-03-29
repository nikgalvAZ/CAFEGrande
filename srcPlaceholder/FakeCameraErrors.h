/******************************************************************************/
/* Copyright (C) Teledyne Photometrics. All rights reserved.                  */
/******************************************************************************/
#pragma once
#ifndef PM_FAKE_CAMERA_ERRORS_H
#define PM_FAKE_CAMERA_ERRORS_H

/* System */
#include <cstdint>

namespace pm {

enum class FakeCameraErrors : int16_t
{
    None = 0,
    Unknown,
    NotInitialized,
    CannotGetResource,
    IndexOutOfRange,
    CamNameNotFound,
    InvalidRoi,
    NotAvailable,
    CannotSetValue,
    CannotGetValue,
};

} // namespace pm

#endif /* PM_FAKE_CAMERA_ERRORS_H */
