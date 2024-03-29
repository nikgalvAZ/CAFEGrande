/******************************************************************************/
/* Copyright (C) Teledyne Photometrics. All rights reserved.                  */
/******************************************************************************/
#include "backend/FileSave.h"

/* Local */
#include "backend/AllocatorDefault.h"
#include "backend/PrdFileUtils.h"

/* System */
#include <cstring> // memcpy

pm::FileSave::FileSave(const std::string& fileName, const PrdHeader& header,
        std::shared_ptr<Allocator> allocator)
    : File(fileName),
    m_header(header),
    m_allocator((allocator) ? allocator : std::make_shared<AllocatorDefault>()),
    m_width((header.region.sbin == 0)
        ? 0
        : ((uint32_t)header.region.s2 + 1 - header.region.s1) / header.region.sbin),
    m_height((header.region.pbin == 0)
        ? 0
        : ((uint32_t)header.region.p2 + 1 - header.region.p1) / header.region.pbin),
    m_rawDataBytes(PrdFileUtils::GetRawDataSize(header)),
    m_rawDataBytesAligned(PrdFileUtils::GetAlignedSize(header, m_rawDataBytes))
{
}

pm::FileSave::~FileSave()
{
}

void pm::FileSave::Close()
{
    m_frameOrigSizeOfPrdMetaDataStruct = 0;
    m_trajectoriesBytes = 0;

    m_allocator->Free(m_framePrdMetaData);
    m_framePrdMetaData = nullptr;
    m_framePrdMetaDataBytesAligned = 0;

    m_allocator->Free(m_framePrdExtDynMetaData);
    m_framePrdExtDynMetaData = nullptr;
    m_framePrdExtDynMetaDataBytesAligned = 0;
}

bool pm::FileSave::WriteFrame(const void* metaData, const void* extDynMetaData,
        const void* rawData)
{
    if (!IsOpen())
        return false;

    if (!metaData || !rawData)
        return false;

    if (m_width == 0 || m_height == 0 || m_rawDataBytes == 0
            || m_header.sizeOfPrdMetaDataStruct == 0)
        return false;

    if (m_header.version >= PRD_VERSION_0_5)
    {
        if (m_header.flags & PRD_FLAG_FRAME_SIZE_VARY)
        {
            auto prdMetaData = static_cast<const PrdMetaData*>(metaData);
            if (prdMetaData->extDynMetaDataSize > 0)
            {
                if (!extDynMetaData)
                    return false;

                m_framePrdExtDynMetaDataBytesAligned =
                    PrdFileUtils::GetAlignedSize(m_header, prdMetaData->extDynMetaDataSize);
            }
        }
    }

    if (m_frameIndex == 0)
    {
        m_framePrdMetaDataBytesAligned =
            PrdFileUtils::GetAlignedSize(m_header, m_header.sizeOfPrdMetaDataStruct);
    }

    return true;
}

bool pm::FileSave::WriteFrame(std::shared_ptr<Frame> frame)
{
    if (!IsOpen())
        return false;

    if (m_width == 0 || m_height == 0 || m_rawDataBytes == 0
            || m_header.sizeOfPrdMetaDataStruct == 0)
        return false;

    // Get the right metadata size including ext. metadata
    //if (m_frameIndex == 0)
    if (m_frameOrigSizeOfPrdMetaDataStruct == 0)
    {
        // Go this way only once
        m_frameOrigSizeOfPrdMetaDataStruct = m_header.sizeOfPrdMetaDataStruct;

        // Must be set before GetExtMetaDataSizeInBytes call
        m_trajectoriesBytes =
            PrdFileUtils::GetTrajectoriesSize(&frame->GetTrajectories().header);

        m_header.sizeOfPrdMetaDataStruct += GetExtMetaDataSizeInBytes(*frame);

        m_framePrdMetaDataBytesAligned =
            PrdFileUtils::GetAlignedSize(m_header, m_header.sizeOfPrdMetaDataStruct);
        m_framePrdMetaData = m_allocator->Allocate(m_framePrdMetaDataBytesAligned);
        if (!m_framePrdMetaData)
            return false;

        if (m_trajectoriesBytes > 0)
        {
            m_framePrdMetaDataExtFlags |= PRD_EXT_FLAG_HAS_TRAJECTORIES;
        }
    }

    // Do not use std::memset, spec. says behavior might be undefined for C structs
    ::memset(m_framePrdMetaData, 0, m_framePrdMetaDataBytesAligned);

    // Set basic metadata

    auto metaData = static_cast<PrdMetaData*>(m_framePrdMetaData);

    const Frame::Info& fi = frame->GetInfo();
    const uint64_t bof = fi.GetTimestampBOF() * 100;
    const uint64_t eof = fi.GetTimestampEOF() * 100;

    if (m_header.version >= PRD_VERSION_0_1)
    {
        metaData->frameNumber = fi.GetFrameNr();
        metaData->readoutTime = fi.GetReadoutTime() * 100;
        metaData->exposureTime = fi.GetExpTime();
    }
    if (m_header.version >= PRD_VERSION_0_2)
    {
        metaData->bofTime = (uint32_t)(bof & 0xFFFFFFFF);
        metaData->eofTime = (uint32_t)(eof & 0xFFFFFFFF);
    }
    if (m_header.version >= PRD_VERSION_0_3)
    {
        metaData->roiCount = frame->GetAcqCfg().GetRoiCount();
    }
    if (m_header.version >= PRD_VERSION_0_4)
    {
        metaData->bofTimeHigh = (uint32_t)((bof >> 32) & 0xFFFFFFFF);
        metaData->eofTimeHigh = (uint32_t)((eof >> 32) & 0xFFFFFFFF);
    }
    if (m_header.version >= PRD_VERSION_0_5)
    {
        metaData->extFlags = m_framePrdMetaDataExtFlags;
        metaData->extMetaDataSize =
            m_header.sizeOfPrdMetaDataStruct - m_frameOrigSizeOfPrdMetaDataStruct;
        metaData->extDynMetaDataSize = 0; // Updated in UpdateFrameExtDynMetaData

        if (!UpdateFrameExtMetaData(*frame))
            return false;

        if (!UpdateFrameExtDynMetaData(*frame))
            return false;
    }
    if (m_header.version >= PRD_VERSION_0_7)
    {
        metaData->colorWbScaleRed = fi.GetColorWbScaleRed();
        metaData->colorWbScaleGreen = fi.GetColorWbScaleGreen();
        metaData->colorWbScaleBlue = fi.GetColorWbScaleBlue();
    }

    return true;
}

bool pm::FileSave::UpdateFrameExtMetaData(const Frame& frame)
{
    auto dest = static_cast<uint8_t*>(m_framePrdMetaData);
    dest += m_frameOrigSizeOfPrdMetaDataStruct;

    if (m_header.version >= PRD_VERSION_0_5)
    {
        const Frame::Trajectories& from = frame.GetTrajectories();
        auto to = reinterpret_cast<PrdTrajectoriesHeader*>(dest);

        const uint32_t size = PrdFileUtils::GetTrajectoriesSize(&from.header);
        if (size != m_trajectoriesBytes)
            return false;

        if (m_trajectoriesBytes > 0)
        {
            if (!PrdFileUtils::ConvertTrajectoriesToPrd(from, to))
                return false;
            dest += m_trajectoriesBytes;
        }
    }

    return true;
}

bool pm::FileSave::UpdateFrameExtDynMetaData(const Frame& /*frame*/)
{
    if (!(m_header.flags & PRD_FLAG_FRAME_SIZE_VARY))
        return true;

    // No extended dynamic metadata so far.
    // In future it should be stored in given frame.
    const void* extDynMetaData = nullptr;
    uint32_t extDynMetaDataBytes = 0;

    if (!extDynMetaData || extDynMetaDataBytes == 0)
        return true;

    // Resize internal buffer to new size if not sufficient
    const size_t extDynMetaDataBytesAligned =
        PrdFileUtils::GetAlignedSize(m_header, extDynMetaDataBytes);
    if (m_framePrdExtDynMetaDataBytesAligned < extDynMetaDataBytesAligned)
    {
        m_allocator->Free(m_framePrdExtDynMetaData);
        m_framePrdExtDynMetaData = m_allocator->Allocate(extDynMetaDataBytesAligned);
        if (!m_framePrdExtDynMetaData)
            return false;
        m_framePrdExtDynMetaDataBytesAligned = extDynMetaDataBytesAligned;
    }
    auto metaData = static_cast<PrdMetaData*>(m_framePrdMetaData);
    metaData->extDynMetaDataSize = extDynMetaDataBytes;

    // Store the data
    std::memcpy(m_framePrdExtDynMetaData, extDynMetaData, extDynMetaDataBytes);

    return true;
}

uint32_t pm::FileSave::GetExtMetaDataSizeInBytes(const Frame& /*frame*/)
{
    if (m_header.version < PRD_VERSION_0_5)
        return 0;

    uint32_t size = 0;

    if (m_header.version >= PRD_VERSION_0_5)
    {
        size += m_trajectoriesBytes;
    }

    return size;
}
