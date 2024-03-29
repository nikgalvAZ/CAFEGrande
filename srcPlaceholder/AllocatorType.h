/******************************************************************************/
/* Copyright (C) Teledyne Photometrics. All rights reserved.                  */
/******************************************************************************/
#pragma once
#ifndef PM_ALLOCATOR_TYPE_H
#define PM_ALLOCATOR_TYPE_H

/* System */
#include <cstdlib>

namespace pm {

enum class AllocatorType
{
    Default,
    Align16,
    Align32,
    Align4k,
};

} // namespace

#endif
