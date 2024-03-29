/******************************************************************************/
/* Copyright (C) Teledyne Photometrics. All rights reserved.                  */
/******************************************************************************/
#include "backend/PrdFileLoad.h"

/* Local */
#include "backend/PrdFileUtils.h"

/* System */
#include <cstring>

pm::PrdFileLoad::PrdFileLoad(const std::string& fileName)
    : FileLoad(fileName)
{
}

pm::PrdFileLoad::~PrdFileLoad()
{
    if (IsOpen())
        Close();
}

bool pm::PrdFileLoad::Open()
{
    if (IsOpen())
        return true;

    m_file.open(m_fileName, std::ios_base::in | std::ios_base::binary);
    if (!m_file.is_open())
        return false;

    PrdHeader header;
    constexpr size_t headerBytes = sizeof(PrdHeader);
    m_file.read(reinterpret_cast<char*>(&header), headerBytes);
    if (m_file.good() && header.signature == PRD_SIGNATURE)
    {
        m_header = header;

        m_rawDataBytes = PrdFileUtils::GetRawDataSize(m_header);
        m_frameIndex = 0;

        const auto headerBytesAligned = PrdFileUtils::GetAlignedSize(m_header, headerBytes);
        if (headerBytesAligned > headerBytes)
        {
            const auto skipBytes = headerBytesAligned - headerBytes;
            m_file.seekg(skipBytes, std::ios_base::cur);
            if (!m_file.good())
            {
                m_file.close();
            }
        }
    }
    else
    {
        m_file.close();
    }

    return IsOpen();
}

bool pm::PrdFileLoad::IsOpen() const
{
    return m_file.is_open();
}

void pm::PrdFileLoad::Close()
{
    if (!IsOpen())
        return;

    m_file.close();

    FileLoad::Close();
}

bool pm::PrdFileLoad::ReadFrame(const void** metaData, const void** extDynMetaData,
        const void** rawData)
{
    if (!FileLoad::ReadFrame(metaData, extDynMetaData, rawData))
        return false;

    auto ReallocAndRead = [&](void** data, size_t bytes) -> bool
    {
        const auto bytesAligned = PrdFileUtils::GetAlignedSize(m_header, bytes);

        void* newMem = std::realloc(*data, bytesAligned);
        if (!newMem)
            return false;
        *data = newMem;

        m_file.read(static_cast<char*>(*data), bytesAligned);
        if (!m_file.good())
            return false;

        return true;
    };

    if (!ReallocAndRead(&m_metaData, m_header.sizeOfPrdMetaDataStruct))
        return false;

    auto prdMetaData = static_cast<PrdMetaData*>(m_metaData);
    if (prdMetaData->extDynMetaDataSize > 0)
    {
        if (!ReallocAndRead(&m_extDynMetaData, prdMetaData->extDynMetaDataSize))
            return false;
    }

    if (!ReallocAndRead(&m_rawData, m_rawDataBytes))
        return false;

    *metaData = m_metaData;
    *extDynMetaData = m_extDynMetaData;
    *rawData = m_rawData;

    return true;
}
