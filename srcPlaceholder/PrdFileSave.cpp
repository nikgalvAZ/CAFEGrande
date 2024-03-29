/******************************************************************************/
/* Copyright (C) Teledyne Photometrics. All rights reserved.                  */
/******************************************************************************/
#include "backend/PrdFileSave.h"

/* Local */
#include "backend/AllocatorFactory.h"
#include "backend/PrdFileUtils.h"

/* System */
#include <cassert>
#include <cstring>
#include <limits>

#ifdef _WIN32
    #include <Windows.h>
#else
    #include <sys/types.h>
    #include <sys/stat.h>
    #include <fcntl.h> // open
    #include <unistd.h> // sysconf, write
#endif

const static auto sPageSize = []() -> size_t
{
#ifdef _WIN32
    SYSTEM_INFO sysInfo;
    ::GetSystemInfo(&sysInfo);
    return sysInfo.dwPageSize;
#else
    return ::sysconf(_SC_PAGESIZE);
#endif
}();

pm::PrdFileSave::PrdFileSave(const std::string& fileName, const PrdHeader& header,
        std::shared_ptr<Allocator> allocator)
    : FileSave(fileName, header, allocator),
    m_headerBytesAligned(PrdFileUtils::GetAlignedSize(header, sizeof(PrdHeader)))
{
    // TODO: The sector size should be read from underlying device, i.e.
    //       on existing file or path takes its fill file/path name and:
    //       - on Windows - use GetVolumePathName and pass the volume name as
    //         root path to GetDiskFreeSpace, then use lpBytesPerSector returned.
    //       - on Linux - call stat and use st_blksize from returned struct stat.
    // For now we simplify it and assume the block size is always equal page size
    const bool isSectorAligned = m_header.alignment > 0
        && (m_header.alignment % sPageSize) == 0;

    const bool canWriteAligned = isSectorAligned
        && AllocatorFactory::GetAlignment(*m_allocator) >= m_header.alignment;

#ifdef _WIN32
    m_fileFlags = (canWriteAligned) ? FILE_FLAG_NO_BUFFERING : FILE_ATTRIBUTE_NORMAL;
#else
    m_fileFlags = (canWriteAligned) ? O_DIRECT : 0;
#endif

    if (m_headerBytesAligned != sizeof(PrdHeader))
    {
        m_headerAlignedBuffer = m_allocator->Allocate(m_headerBytesAligned);
        m_headerDataPtr = m_headerAlignedBuffer;
    }
    else
    {
        m_headerDataPtr = &m_header;
    }
}

pm::PrdFileSave::~PrdFileSave()
{
    if (IsOpen())
        Close();

    m_allocator->Free(m_headerAlignedBuffer);
}

bool pm::PrdFileSave::Open()
{
    if (IsOpen())
        return true;

#ifdef _WIN32
    HANDLE file = ::CreateFileA(m_fileName.c_str(), GENERIC_WRITE, 0, NULL,
            CREATE_ALWAYS, m_fileFlags, NULL);
    if (file == INVALID_HANDLE_VALUE)
        return false;
#else
    const int flags = O_WRONLY | O_CREAT | O_TRUNC | m_fileFlags;
    const mode_t mode = S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH;
    const int file = ::open(m_fileName.c_str(), flags, mode);
    if (file == -1)
        return false;
#endif
    m_file = file;

    m_frameIndex = 0;

    return IsOpen();
}

bool pm::PrdFileSave::IsOpen() const
{
#ifdef _WIN32
    return (m_file != INVALID_HANDLE_VALUE);
#else
    return (m_file > -1);
#endif
}

void pm::PrdFileSave::Close()
{
    if (!IsOpen())
        return;

    if (m_header.frameCount != m_frameIndex)
    {
        m_header.frameCount = m_frameIndex;

        auto OsSeekToZeroOffset = [&](bool fromEnd) -> bool
        {
#ifdef _WIN32
            const DWORD whence = (fromEnd) ? FILE_END : FILE_BEGIN;
            return (::SetFilePointer(m_file, 0, NULL, whence) != INVALID_SET_FILE_POINTER);
#else
            const int whence = (fromEnd) ? SEEK_END : SEEK_SET;
            return (::lseek(m_file, 0, whence) >= 0);
#endif
        };

        OsSeekToZeroOffset(false);
        OsWrite(m_headerDataPtr, m_headerBytesAligned);
        OsSeekToZeroOffset(true);
    }

#ifdef _WIN32
    HANDLE file = static_cast<HANDLE>(m_file);
    ::CloseHandle(file);
    m_file = INVALID_HANDLE_VALUE;
#else
    ::close(m_file);
    m_file = -1;
#endif

    FileSave::Close();
}

bool pm::PrdFileSave::WriteFrame(const void* metaData,
        const void* extDynMetaData, const void* rawData)
{
    if (!FileSave::WriteFrame(metaData, extDynMetaData, rawData))
        return false;

    // Write PRD header to file only once at the beginning
    if (m_frameIndex == 0)
    {
        // Copy header data to aligned memory
        if (m_headerAlignedBuffer)
        {
            std::memcpy(m_headerAlignedBuffer, &m_header, sizeof(PrdHeader));
        }

        if (!OsWrite(m_headerDataPtr, m_headerBytesAligned))
            return false;
    }

    if (!OsWrite(metaData, m_framePrdMetaDataBytesAligned))
        return false;

    if (m_header.version >= PRD_VERSION_0_5)
    {
        if (m_framePrdExtDynMetaDataBytesAligned > 0 && extDynMetaData)
        {
            if (!OsWrite(extDynMetaData, m_framePrdExtDynMetaDataBytesAligned))
                return false;
        }
    }

    if (!OsWrite(rawData, m_rawDataBytesAligned))
        return false;

    m_frameIndex++;
    return true;
}

bool pm::PrdFileSave::WriteFrame(std::shared_ptr<Frame> frame)
{
    if (!FileSave::WriteFrame(frame))
        return false;

    return WriteFrame(m_framePrdMetaData, m_framePrdExtDynMetaData, frame->GetData());
}

bool pm::PrdFileSave::OsWrite(const void* data, size_t bytes)
{
#ifdef _WIN32
    if (bytes > (std::numeric_limits<DWORD>::max)())
        return false;
    DWORD bytesWritten = 0;
    HANDLE file = static_cast<HANDLE>(m_file);
    return (::WriteFile(file, data, (DWORD)bytes, &bytesWritten, NULL) == TRUE
            && bytes == bytesWritten);
#else
    if (bytes > (std::numeric_limits<ssize_t>::max)())
        return false;
    return (::write(m_file, data, bytes) == (ssize_t)bytes);
#endif
}
