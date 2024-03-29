/******************************************************************************/
/* Copyright (C) Teledyne Photometrics. All rights reserved.                  */
/******************************************************************************/
#include "backend/XoShiRo128Plus.h"

/* System */
#include <random>

pm::XoShiRo128Plus::XoShiRo128Plus()
{
    std::random_device rd;
    m_state[0] = rd();
    m_state[1] = rd();
    m_state[2] = rd();
    m_state[3] = rd();
}

pm::XoShiRo128Plus::XoShiRo128Plus(const pm::XoShiRo128Plus::State& state)
{
    m_state[0] = state[0];
    m_state[1] = state[1];
    m_state[2] = state[2];
    m_state[3] = state[3];
}

const pm::XoShiRo128Plus::State& pm::XoShiRo128Plus::GetState() const
{
    return m_state;
}

uint32_t pm::XoShiRo128Plus::GetNext()
{
    const uint32_t result = m_state[0] + m_state[3];

    const uint32_t tmp = m_state[1] << 9;

    m_state[2] ^= m_state[0];
    m_state[3] ^= m_state[1];
    m_state[1] ^= m_state[2];
    m_state[0] ^= m_state[3];

    m_state[2] ^= tmp;

    //m_state[3] = rotl32(m_state[3], 11); // x, k => (x << k) | (x >> (32 - k))
    m_state[3] = (m_state[3] << 11) | (m_state[3] >> 21);

    return result;
}
