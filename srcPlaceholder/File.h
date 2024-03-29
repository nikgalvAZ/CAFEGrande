/******************************************************************************/
/* Copyright (C) Teledyne Photometrics. All rights reserved.                  */
/******************************************************************************/
#pragma once
#ifndef PM_FILE_H
#define PM_FILE_H

/* Local */
#include "backend/PrdFileFormat.h"

/* System */
#include <cstdint>
#include <string>

namespace pm {

class File
{
public:
    File(const std::string& fileName);
    virtual ~File();

    File() = delete;
    File(const File&) = delete;
    File(File&&) = delete;
    File& operator=(const File&) = delete;
    File& operator=(File&&) = delete;

public:
    const std::string& GetFileName() const;
    const PrdHeader& GetHeader() const;

public:
    virtual bool Open() = 0;
    virtual bool IsOpen() const = 0;
    virtual void Close() = 0;

protected:
    const std::string m_fileName{};

    PrdHeader m_header{};
    uint32_t m_frameIndex{ 0 };
};

} // namespace

#endif
