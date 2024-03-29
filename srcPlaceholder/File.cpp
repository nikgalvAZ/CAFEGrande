/******************************************************************************/
/* Copyright (C) Teledyne Photometrics. All rights reserved.                  */
/******************************************************************************/
#include "backend/File.h"

/* System */
#include <cstring> // memset

pm::File::File(const std::string& fileName)
    : m_fileName(fileName)
{
    // Do not use std::memset, spec. says behavior might be undefined for C structs
    ::memset(&m_header, 0, sizeof(PrdHeader));
}

pm::File::~File()
{
}

const std::string& pm::File::GetFileName() const
{
    return m_fileName;
}

const PrdHeader& pm::File::GetHeader() const
{
    return m_header;
}
