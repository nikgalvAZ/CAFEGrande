/******************************************************************************/
/* Copyright (C) Teledyne Photometrics. All rights reserved.                  */
/******************************************************************************/
#pragma once
#ifndef PM_FILE_SAVE_H
#define PM_FILE_SAVE_H

/* Local */
#include "backend/Allocator.h"
#include "backend/File.h"
#include "backend/Frame.h"

namespace pm {

class FileSave : public File
{
public:
    FileSave(const std::string& fileName, const PrdHeader& header,
            std::shared_ptr<Allocator> allocator = nullptr);
    virtual ~FileSave();

    FileSave() = delete;
    FileSave(const FileSave&) = delete;
    FileSave(FileSave&&) = delete;
    FileSave& operator=(const FileSave&) = delete;
    FileSave& operator=(FileSave&&) = delete;

public: // From File
    virtual void Close() override;

public:
    // New frame is added at the end of the file

    virtual bool WriteFrame(const void* metaData, const void* extDynMetaData,
            const void* rawData);
    virtual bool WriteFrame(std::shared_ptr<Frame> frame);

private:
    bool UpdateFrameExtMetaData(const Frame& frame);
    bool UpdateFrameExtDynMetaData(const Frame& frame);

    uint32_t GetExtMetaDataSizeInBytes(const Frame& frame);

protected:
    PrdHeader m_header{};

    const std::shared_ptr<Allocator> m_allocator;

    const uint32_t m_width;
    const uint32_t m_height;
    const size_t m_rawDataBytes;
    const size_t m_rawDataBytesAligned;

    void* m_framePrdMetaData{ nullptr };
    size_t m_framePrdMetaDataBytesAligned{ 0 };
    void* m_framePrdExtDynMetaData{ nullptr };
    size_t m_framePrdExtDynMetaDataBytesAligned{ 0 };

private:
    // Zero until first frame comes, then set to orig. size from header
    uint32_t m_frameOrigSizeOfPrdMetaDataStruct{ 0 };

    uint32_t m_framePrdMetaDataExtFlags{ 0 };
    uint32_t m_trajectoriesBytes{ 0 };
};

} // namespace

#endif
