/******************************************************************************/
/* Copyright (C) Teledyne Photometrics. All rights reserved.                  */
/******************************************************************************/
#pragma once
#ifndef PM_EXCEPTION_H
#define PM_EXCEPTION_H

/* System */
#include <stdexcept>
#include <string>

namespace pm {

class Exception : public std::runtime_error
{
public:
    explicit Exception(const std::string& what);
};

} // namespace pm

#endif /* PM_EXCEPTION_H */
