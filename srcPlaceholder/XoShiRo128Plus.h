/******************************************************************************/
/* Copyright (C) Teledyne Photometrics. All rights reserved.                  */
/******************************************************************************/
#pragma once
#ifndef PM_XOSHIRO128PLUS_H
#define PM_XOSHIRO128PLUS_H

// Based on implementation taken from http://prng.di.unimi.it/

/* System */
#include <cstdint>

namespace pm {

class XoShiRo128Plus final
{
public:
    using State = uint32_t[4];

public:
    explicit XoShiRo128Plus();
    XoShiRo128Plus(const XoShiRo128Plus::State& state);

    XoShiRo128Plus(const XoShiRo128Plus&) = delete;
    XoShiRo128Plus(XoShiRo128Plus&&) = delete;
    XoShiRo128Plus& operator=(const XoShiRo128Plus&) = delete;
    XoShiRo128Plus& operator=(XoShiRo128Plus&&) = delete;

public:
    const XoShiRo128Plus::State& GetState() const;

    uint32_t GetNext();

private:
    State m_state;
};

} // namespace

#endif
