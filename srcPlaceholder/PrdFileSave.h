/******************************************************************************/
/* Copyright (C) Teledyne Photometrics. All rights reserved.                  */
/******************************************************************************/
#pragma once
#ifndef PM_PRD_FILE_SAVE_H
#define PM_PRD_FILE_SAVE_H

/* Local */
#include "backend/FileSave.h"

namespace pm {

class PrdFileSave final : public FileSave
{
public:
    PrdFileSave(const std::string& fileName, const PrdHeader& header,
            std::shared_ptr<Allocator> allocator = nullptr);
    virtual ~PrdFileSave();

    PrdFileSave() = delete;
    PrdFileSave(const PrdFileSave&) = delete;
    PrdFileSave(PrdFileSave&&) = delete;
    PrdFileSave& operator=(const PrdFileSave&) = delete;
    PrdFileSave& operator=(PrdFileSave&&) = delete;

public: // From File
    virtual bool Open() override;
    virtual bool IsOpen() const override;
    virtual void Close() override;

public: // From FileSave
    virtual bool WriteFrame(const void* metaData, const void* extDynMetaData,
            const void* rawData) override;
    virtual bool WriteFrame(std::shared_ptr<Frame> frame) override;

private:
    bool OsWrite(const void *data, size_t bytes);

private:
    const size_t m_headerBytesAligned;

    void* m_headerAlignedBuffer{ nullptr };
    const void* m_headerDataPtr{ nullptr };

#ifdef _WIN32
    void* m_file{ (void*)-1/*INVALID_HANDLE_VALUE*/ };
#else
    int m_file{ -1 };
#endif
    int m_fileFlags{ 0 };
};

} // namespace

#endif
