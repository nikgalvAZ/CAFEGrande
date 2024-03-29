/******************************************************************************/
/* Copyright (C) Teledyne Photometrics. All rights reserved.                  */
/******************************************************************************/
#include "backend/FileLoad.h"

/* System */
#include <cstring>

pm::FileLoad::FileLoad(const std::string& fileName)
    : File(fileName)
{
}

pm::FileLoad::~FileLoad()
{
}

void pm::FileLoad::Close()
{
    std::free(m_metaData);
    m_metaData = nullptr;

    std::free(m_extDynMetaData);
    m_extDynMetaData = nullptr;

    std::free(m_rawData);
    m_rawData = nullptr;
}

bool pm::FileLoad::ReadFrame(const void** metaData, const void** extDynMetaData,
            const void** rawData)
{
    if (!IsOpen())
        return false;

    if (!metaData || !extDynMetaData || !rawData)
        return false;

    if (m_rawDataBytes == 0 || m_frameIndex >= m_header.frameCount)
        return false;

    *metaData = nullptr;
    *extDynMetaData = nullptr;
    *rawData = nullptr;

    return true;
}
