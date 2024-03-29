/******************************************************************************/
/* Copyright (C) Teledyne Photometrics. All rights reserved.                  */
/******************************************************************************/
#pragma once
#ifndef PM_PRD_FILE_LOAD_H
#define PM_PRD_FILE_LOAD_H

/* Local */
#include "backend/FileLoad.h"

/* System */
#include <fstream>

namespace pm {

class PrdFileLoad final : public FileLoad
{
public:
    PrdFileLoad(const std::string& fileName);
    virtual ~PrdFileLoad();

    PrdFileLoad() = delete;
    PrdFileLoad(const PrdFileLoad&) = delete;
    PrdFileLoad(PrdFileLoad&&) = delete;
    PrdFileLoad& operator=(const PrdFileLoad&) = delete;
    PrdFileLoad& operator=(PrdFileLoad&&) = delete;

public: // From File
    // Fills the PrdHeader structure
    virtual bool Open() override;
    virtual bool IsOpen() const override;
    virtual void Close() override;

public: // From FileLoad
    virtual bool ReadFrame(const void** metaData, const void** extDynMetaData,
            const void** rawData) override;

private:
    std::ifstream m_file{};
};

} // namespace

#endif
