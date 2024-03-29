/******************************************************************************/
/* Copyright (C) Teledyne Photometrics. All rights reserved.                  */
/******************************************************************************/
#pragma once
#ifndef PM_FILE_LOAD_H
#define PM_FILE_LOAD_H

/* Local */
#include "backend/File.h"

namespace pm {

class FileLoad : public File
{
public:
    FileLoad(const std::string& fileName);
    virtual ~FileLoad();

    FileLoad() = delete;
    FileLoad(const FileLoad&) = delete;
    FileLoad(FileLoad&&) = delete;
    FileLoad& operator=(const FileLoad&) = delete;
    FileLoad& operator=(FileLoad&&) = delete;

public: // From File
    virtual void Close() override;

public:
    // Next frame is read out of the file

    // The metaData, extDynMetaData and rawData sizes are auto-detected during
    // reading from file, memory is allocated and filled. The memory is owned
    // by this class and can be released or reallocated in each ReadFrame call.
    virtual bool ReadFrame(const void** metaData, const void** extDynMetaData,
            const void** rawData);

protected:
    size_t m_rawDataBytes{ 0 };

    void* m_metaData{ nullptr };
    void* m_extDynMetaData{ nullptr };
    void* m_rawData{ nullptr };
};

} // namespace

#endif
