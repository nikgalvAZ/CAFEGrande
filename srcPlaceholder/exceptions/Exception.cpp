/******************************************************************************/
/* Copyright (C) Teledyne Photometrics. All rights reserved.                  */
/******************************************************************************/
#include "backend/exceptions/Exception.h"

pm::Exception::Exception(const std::string& what)
    : std::runtime_error(what)
{
}
