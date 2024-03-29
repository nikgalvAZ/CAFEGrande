/******************************************************************************/
/* Copyright (C) Teledyne Photometrics. All rights reserved.                  */
/******************************************************************************/
#include "backend/PrdFileUtils.h"

/* Local */
#include "backend/BitmapFormat.h"

/* System */
#include <cassert>
#include <cstring>
#include <limits>
#include <sstream>

void pm::PrdFileUtils::ClearPrdHeaderStructure(PrdHeader& header)
{
    ::memset(&header, 0, sizeof(PrdHeader));
    // PRD v0.1 data
    header.signature = PRD_SIGNATURE;
}

void pm::PrdFileUtils::InitPrdHeaderStructure(PrdHeader& header, uint16_t version,
        const pm::Frame::AcqCfg& acqCfg, const PrdRegion& prdRegion,
        uint32_t prdExpTimeRes, uint16_t alignment)
{
    ClearPrdHeaderStructure(header);

    // PRD v0.1 data
    header.version = version;

    if (header.version >= PRD_VERSION_0_1)
    {
        header.bitDepth = static_cast<uint8_t>(acqCfg.GetBitmapFormat().GetBitDepth());
        header.frameCount = 1;
        ::memcpy(&header.region, &prdRegion, sizeof(PrdRegion));
        header.sizeOfPrdMetaDataStruct = sizeof(PrdMetaData);
        header.exposureResolution = prdExpTimeRes;
    }
    if (header.version >= PRD_VERSION_0_3)
    {
        header.colorMask = static_cast<uint8_t>(acqCfg.GetBitmapFormat().GetColorMask());
        header.flags |= (acqCfg.HasMetadata()) ? PRD_FLAG_HAS_METADATA : 0x00;
        header.frameSize = static_cast<uint32_t>(acqCfg.GetFrameBytes());
    }
    if (header.version >= PRD_VERSION_0_6)
    {
        header.imageFormat = static_cast<uint8_t>(acqCfg.GetBitmapFormat().GetImageFormat());
    }
    if (header.version >= PRD_VERSION_0_8)
    {
        if (alignment > 1) // Alignment equal to 1 is same like no alignment
        {
            // Alignment must be power of two
            assert((alignment & (alignment - 1)) == 0);
            // Alignment must be multiple of pointer size (relies on check above)
            assert(alignment >= sizeof(void*));

            header.flags |= PRD_FLAG_HAS_ALIGNMENT;
            header.alignment = alignment;
        }
        else
        {
            header.alignment = 0;
        }
    }
}

void pm::PrdFileUtils::InitPrdHeaderStructure(PrdHeader& header, uint16_t version,
        const pm::Frame::AcqCfg& acqCfg, const rgn_type& pvcamRegion,
        uint32_t pvcamExpTimeRes, uint16_t alignment)
{
    uint32_t prdExpTimeRes;
    switch (pvcamExpTimeRes)
    {
    case EXP_RES_ONE_MICROSEC:
        prdExpTimeRes = PRD_EXP_RES_US;
        break;
    case EXP_RES_ONE_MILLISEC:
    default: // Just in case, should never happen
        prdExpTimeRes = PRD_EXP_RES_MS;
        break;
    case EXP_RES_ONE_SEC:
        prdExpTimeRes = PRD_EXP_RES_S;
        break;
    }

    InitPrdHeaderStructure(header, version, acqCfg,
            *reinterpret_cast<const PrdRegion*>(&pvcamRegion),
            prdExpTimeRes, alignment);
}

size_t pm::PrdFileUtils::GetAlignedSize(const PrdHeader& header, size_t size)
{
    size_t alignedSize = size;
    if (header.version >= PRD_VERSION_0_8
            && (header.flags & PRD_FLAG_HAS_ALIGNMENT)
            && header.alignment > 1)
    {
        const size_t algn = header.alignment;
        // Because alignment can be power of 2 only, / and * can be avoided
        //alignedSize = ((size + algn - 1) / algn) * algn;
        alignedSize = (size + (algn - 1)) & ~(algn - 1);
    }
    return alignedSize;
}

size_t pm::PrdFileUtils::GetRawDataSize(const PrdHeader& header)
{
    const PrdRegion& region = header.region;
    if (region.sbin == 0 || region.pbin == 0)
        return 0;
    size_t bytes;
    if (header.version >= PRD_VERSION_0_3)
    {
        bytes = header.frameSize;
    }
    else
    {
        const uint32_t width = (region.s2 - region.s1 + 1) / region.sbin;
        const uint32_t height = (region.p2 - region.p1 + 1) / region.pbin;
        // Older PRD versions support 16 bit per pixel only
        bytes = sizeof(uint16_t) * width * height;
    }
    return bytes;
}

size_t pm::PrdFileUtils::GetPrdFileSizeOverhead(const PrdHeader& header)
{
    const auto prdHeaderBytesAligned = GetAlignedSize(header, sizeof(PrdHeader));
    const auto prdMetaDataBytesAligned = GetAlignedSize(header, header.sizeOfPrdMetaDataStruct);
    return prdHeaderBytesAligned + header.frameCount * prdMetaDataBytesAligned;
}

size_t pm::PrdFileUtils::GetPrdFileSize(const PrdHeader& header)
{
    const auto rawDataBytes = GetRawDataSize(header);
    if (rawDataBytes == 0)
        return 0;
    const auto rawDataBytesAligned = GetAlignedSize(header, rawDataBytes);
    return GetPrdFileSizeOverhead(header) + header.frameCount * rawDataBytesAligned;
}

uint32_t pm::PrdFileUtils::GetPrdFrameCountThatFitsIn(const PrdHeader& header,
        size_t maxSizeInBytes)
{
    const auto rawDataBytes = GetRawDataSize(header);
    const auto prdHeaderBytesAligned = GetAlignedSize(header, sizeof(PrdHeader));
    if (rawDataBytes == 0 || maxSizeInBytes <= prdHeaderBytesAligned)
        return 0;
    const auto prdMetaDataBytesAligned = GetAlignedSize(header, header.sizeOfPrdMetaDataStruct);
    const auto rawDataBytesAligned = GetAlignedSize(header, rawDataBytes);
    const size_t count = (maxSizeInBytes - prdHeaderBytesAligned)
        / (prdMetaDataBytesAligned + rawDataBytesAligned);
    if (count > std::numeric_limits<uint32_t>::max())
        return 0;
    return (uint32_t)count;
}

const void* pm::PrdFileUtils::GetExtMetadataAddress(const PrdHeader& header,
        const void* metadata, uint32_t extFlag)
{
    if (!metadata)
        return nullptr;

    // Extended metadata added in PRD_VERSION_0_5
    if (header.version < PRD_VERSION_0_5)
        return nullptr;

    auto prdMeta = static_cast<const PrdMetaData*>(metadata);

    const uint32_t extMetaOffset =
        header.sizeOfPrdMetaDataStruct - prdMeta->extMetaDataSize;
    auto extMeta = static_cast<const uint8_t*>(metadata) + extMetaOffset;

    const PrdTrajectoriesHeader* extMetaTrajectories = nullptr;
    if (header.version >= PRD_VERSION_0_5)
    {
        if (prdMeta->extFlags & PRD_EXT_FLAG_HAS_TRAJECTORIES)
        {
            extMetaTrajectories =
                reinterpret_cast<const PrdTrajectoriesHeader*>(extMeta);
            extMeta += GetTrajectoriesSize(extMetaTrajectories);
        }
    }

    //const PrdXyz* extMetaXyz = nullptr;
    //if (header.version >= PRD_VERSION_M_N)
    //{
    //    if (prdMeta->extFlags & PRD_EXT_FLAG_HAS_XYZ)
    //    {
    //        extMetaXyz = reinterpret_cast<const PrdXyz*>(extMeta);
    //        extMeta += GetXyzSizeInBytes(extMetaXyz);
    //    }
    //}

    switch (extFlag)
    {
    case PRD_EXT_FLAG_HAS_TRAJECTORIES:
        return extMetaTrajectories;
    //case PRD_EXT_FLAG_HAS_XYZ:
    //    return extMetaXyz;
    default:
        return nullptr;
    };
}

uint32_t pm::PrdFileUtils::GetTrajectoriesSize(
        const PrdTrajectoriesHeader* trajectoriesHeader)
{
    if (!trajectoriesHeader)
        return 0;

    if (trajectoriesHeader->maxTrajectories == 0
            && trajectoriesHeader->maxTrajectoryPoints == 0)
        return 0;

    const uint32_t onePointSize = sizeof(PrdTrajectoryPoint);
    const uint32_t allPointsSize =
        trajectoriesHeader->maxTrajectoryPoints * onePointSize;

    const uint32_t oneTrajectorySize =
        sizeof(PrdTrajectoryHeader) + allPointsSize;
    const uint32_t allTrajectoriesSize =
        trajectoriesHeader->maxTrajectories * oneTrajectorySize;

    const uint32_t totalTrajectoriesSize =
        sizeof(PrdTrajectoriesHeader) + allTrajectoriesSize;

    return totalTrajectoriesSize;
}

bool pm::PrdFileUtils::ConvertTrajectoriesFromPrd(
        const PrdTrajectoriesHeader* from, pm::Frame::Trajectories& to)
{
    if (!from)
        return false;

    if (from->maxTrajectories < from->trajectoryCount)
        return false;

    if (from->maxTrajectories == 0 && from->maxTrajectoryPoints == 0)
        return true;

    void* dst;
    // The src is non-const just to be able to move the pointer to next data
    auto src = const_cast<uint8_t*>(reinterpret_cast<const uint8_t*>(from));
    uint32_t size;

    // Add trajectories header
    dst = &to.header;
    size = sizeof(PrdTrajectoriesHeader);
    std::memcpy(dst, src, size);
    src += size;

    for (uint32_t n = 0; n < from->trajectoryCount; ++n)
    {
        pm::Frame::Trajectory trajectory;

        // Add trajectory header
        dst = &trajectory.header;
        size = sizeof(PrdTrajectoryHeader);
        std::memcpy(dst, src, size);
        src += size;

        if (from->maxTrajectoryPoints < trajectory.header.pointCount)
            return false;

        // Resize vector to right size
        trajectory.data.resize(trajectory.header.pointCount);

        // Add valid trajectory points
        dst = trajectory.data.data();
        size = sizeof(PrdTrajectoryPoint) * trajectory.header.pointCount;
        std::memcpy(dst, src, size);
        // Move over all points including unused space up to capacity
        src += sizeof(PrdTrajectoryPoint) * from->maxTrajectoryPoints;

        // Add trajectory to trajectories vector
        to.data.push_back(trajectory);
    }

    return true;
}

bool pm::PrdFileUtils::ConvertTrajectoriesToPrd(
        const pm::Frame::Trajectories& from, PrdTrajectoriesHeader* to)
{
    if (!to)
        return false;

    if (from.header.maxTrajectories < from.header.trajectoryCount)
        return false;

    if (from.data.size() != from.header.trajectoryCount)
        return false;

    if (from.header.maxTrajectories == 0 && from.header.maxTrajectoryPoints == 0)
        return true;

    auto dst = reinterpret_cast<uint8_t*>(to);
    const void* src;
    uint32_t size;

    // Add trajectories header
    src = &from.header;
    size = sizeof(PrdTrajectoriesHeader);
    std::memcpy(dst, src, size);
    dst += size;

    for (uint32_t n = 0; n < from.header.trajectoryCount; ++n)
    {
        const pm::Frame::Trajectory& trajectory = from.data.at(n);

        if (from.header.maxTrajectoryPoints < trajectory.header.pointCount)
            return false;

        if (trajectory.data.size() != trajectory.header.pointCount)
            return false;

        // Add trajectory header
        src = &trajectory.header;
        size = sizeof(PrdTrajectoryHeader);
        std::memcpy(dst, src, size);
        dst += size;

        // Add valid trajectory points
        src = trajectory.data.data();
        size = sizeof(PrdTrajectoryPoint) * trajectory.header.pointCount;
        std::memcpy(dst, src, size);
        // Move over all points including unused space up to capacity
        dst += sizeof(PrdTrajectoryPoint) * from.header.maxTrajectoryPoints;
    }

    return true;
}

std::shared_ptr<pm::Frame> pm::PrdFileUtils::ReconstructFrame(const PrdHeader& header,
        const void* metaData, const void* /*extDynMetaData*/, const void* rawData)
{
    if (!rawData || !metaData)
        return nullptr;

    auto prdMeta = static_cast<const PrdMetaData*>(metaData);

    const size_t rawDataSize = GetRawDataSize(header);
    const uint16_t roiCount = prdMeta->roiCount;
    const bool hasMetadata = ((header.flags & PRD_FLAG_HAS_METADATA) != 0);

    rgn_type rgn;
    std::memcpy(&rgn, &header.region, sizeof(rgn));

    pm::BitmapFormat bmpFormat;
    bmpFormat.SetBitDepth(header.bitDepth);
    if (header.version >= PRD_VERSION_0_3)
    {
        bmpFormat.SetColorMask(static_cast<pm::BayerPattern>(header.colorMask));
    }
    if (header.version >= PRD_VERSION_0_6)
    {
        try
        {
            bmpFormat.SetImageFormat(static_cast<pm::ImageFormat>(header.imageFormat));
        }
        catch (...)
        {
            return nullptr;
        }
    }

    const pm::Frame::AcqCfg acqCfg(rawDataSize, roiCount, hasMetadata, rgn, bmpFormat);

    std::shared_ptr<pm::Frame> frame;
    try
    {
        frame = std::make_shared<pm::Frame>(acqCfg, true);
    }
    catch (...)
    {
        return nullptr;
    }

    frame->SetDataPointer(const_cast<void*>(rawData));
    if (!frame->CopyData())
        return nullptr;

    const uint32_t frameNr = prdMeta->frameNumber;
    uint64_t timestampBOF = 0;
    uint64_t timestampEOF = 0;
    if (header.version >= PRD_VERSION_0_2)
    {
        timestampBOF = prdMeta->bofTime;
        timestampEOF = prdMeta->eofTime;
        if (header.version >= PRD_VERSION_0_4)
        {
            timestampBOF |= ((uint64_t)prdMeta->bofTimeHigh << 32);
            timestampEOF |= ((uint64_t)prdMeta->eofTimeHigh << 32);
        }
    }
    if (header.version >= PRD_VERSION_0_7)
    {
        const pm::Frame::Info info(frameNr, timestampBOF, timestampEOF,
                prdMeta->exposureTime, prdMeta->colorWbScaleRed,
                prdMeta->colorWbScaleGreen, prdMeta->colorWbScaleBlue);
        frame->SetInfo(info);
    }
    else
    {
        const pm::Frame::Info info(frameNr, timestampBOF, timestampEOF,
                prdMeta->exposureTime);
        frame->SetInfo(info);
    }

    auto trajectoriesAddress =
        GetExtMetadataAddress(header, metaData, PRD_EXT_FLAG_HAS_TRAJECTORIES);
    if (trajectoriesAddress)
    {
        auto prdTrajectories =
            static_cast<const PrdTrajectoriesHeader*>(trajectoriesAddress);
        pm::Frame::Trajectories trajectories;

        if (!ConvertTrajectoriesFromPrd(prdTrajectories, trajectories))
            return nullptr;

        frame->SetTrajectories(trajectories);
    }

    return frame;
}

std::string pm::PrdFileUtils::GetImageDescription(const PrdHeader& header,
        const void* prdMeta, const md_frame* pvcamMeta)
{
    if (!prdMeta)
        return "";

    std::ostringstream dsc;

    auto prdMetaData = static_cast<const PrdMetaData*>(prdMeta);

    dsc << "version="
        << ((header.version >> 8) & 0xFF) << "."
        << ((header.version >> 0) & 0xFF);

    if (header.version >= PRD_VERSION_0_1)
    {
        static std::map<uint32_t, const char*> expResUnit =
        {
            { PRD_EXP_RES_US, "us" },
            { PRD_EXP_RES_MS, "ms" },
            { PRD_EXP_RES_S, "s" }
        };
        const PrdRegion& rgn = header.region;
        dsc << "\nbitDepth=" << header.bitDepth
            << "\nregion=["
                << rgn.s1 << "," << rgn.s2 << "," << rgn.sbin << ","
                << rgn.p1 << "," << rgn.p2 << "," << rgn.pbin << "]"
            << "\nframeNr=" << prdMetaData->frameNumber
            << "\nreadoutTime=" << prdMetaData->readoutTime << "us"
            << "\nexpTime=" << prdMetaData->exposureTime
                << ((expResUnit.count(header.exposureResolution) > 0)
                        ? expResUnit[header.exposureResolution]
                        : "<unknown unit>");
    }

    if (header.version >= PRD_VERSION_0_2)
    {
        uint64_t bofTime = prdMetaData->bofTime;
        uint64_t eofTime = prdMetaData->eofTime;
        if (header.version >= PRD_VERSION_0_4)
        {
            bofTime |= (uint64_t)prdMetaData->bofTimeHigh << 32;
            eofTime |= (uint64_t)prdMetaData->eofTimeHigh << 32;
        }
        dsc << "\nbofTime=" << bofTime << "us"
            << "\neofTime=" << eofTime << "us";
    }

    if (header.version >= PRD_VERSION_0_3)
    {
        // Cast 8bit values to 16bit, otherwise ostream processes it as char
        dsc << "\nroiCount=" << prdMetaData->roiCount
            << "\ncolorMask=" << (uint16_t)header.colorMask
            << "\nflags=0x" << std::hex << (uint16_t)header.flags << std::dec;
    }

    if (header.version >= PRD_VERSION_0_6)
    {
        // Cast 8bit values to 16bit, otherwise ostream processes it as char
        dsc << "\nimageFormat=" << (uint16_t)header.imageFormat;
    }

    if (header.version >= PRD_VERSION_0_7)
    {
        dsc << "\ncolorWbScaleRed=" << prdMetaData->colorWbScaleRed
            << "\ncolorWbScaleGreen=" << prdMetaData->colorWbScaleGreen
            << "\ncolorWbScaleBlue" << prdMetaData->colorWbScaleBlue;
    }

    if (header.version >= PRD_VERSION_0_8)
    {
        dsc << "\nalignment=" << header.alignment;
    }

    if (pvcamMeta && header.version >= PRD_VERSION_0_3
            && (header.flags & PRD_FLAG_HAS_METADATA))
    {
        // uns8 type is handled as underlying char by stream, cast it to uint16_t
        dsc << "\nmeta.header.version=" << (uint16_t)pvcamMeta->header->version
            << "\nmeta.header.frameNr=" << pvcamMeta->header->frameNr
            << "\nmeta.header.roiCount=" << pvcamMeta->header->roiCount;
        if (pvcamMeta->header->version < 3)
        {
            const md_frame_header* pvcamHdr = pvcamMeta->header;
            const uint64_t rdfTimeNs = (uint64_t)pvcamHdr->timestampResNs
                * (pvcamHdr->timestampEOF - pvcamHdr->timestampBOF);
            const uint64_t expTimeNs = (uint64_t)pvcamHdr->exposureTimeResNs
                * pvcamHdr->exposureTime;
            dsc << "\nmeta.header.timeBof=" << pvcamHdr->timestampBOF
                << "\nmeta.header.timeEof=" << pvcamHdr->timestampEOF
                << "\n  (diff=" << rdfTimeNs << "ns)"
                << "\nmeta.header.timeResNs=" << pvcamHdr->timestampResNs
                << "\nmeta.header.expTime=" << pvcamHdr->exposureTime
                << "\n  (" << expTimeNs << "ns)"
                << "\nmeta.header.expTimeResNs=" << pvcamHdr->exposureTimeResNs
                << "\nmeta.header.roiTimeResNs=" << pvcamHdr->roiTimestampResNs;
        }
        else
        {
            const auto* pvcamHdr = reinterpret_cast<md_frame_header_v3*>(pvcamMeta->header);
            const uint64_t rdfTimePs = pvcamHdr->timestampEOF - pvcamHdr->timestampBOF;
            dsc << "\nmeta.header.timeBof=" << pvcamHdr->timestampBOF << "ps"
                << "\nmeta.header.timeEof=" << pvcamHdr->timestampEOF << "ps"
                << "\n  (diff=" << rdfTimePs << "ps)"
                << "\nmeta.header.expTime=" << pvcamHdr->exposureTime << "ps";
        }
        dsc << "\nmeta.header.bitDepth=" << (uint16_t)pvcamMeta->header->bitDepth
            << "\nmeta.header.colorMask=" << (uint16_t)pvcamMeta->header->colorMask
            << "\nmeta.header.flags=0x"
                << std::hex << (uint16_t)pvcamMeta->header->flags << std::dec
            << "\nmeta.header.extMdSize=" << pvcamMeta->header->extendedMdSize;
        if (pvcamMeta->header->version >= 2)
        {
            dsc << "\nmeta.header.imageFormat=" << (uint16_t)pvcamMeta->header->imageFormat
                << "\nmeta.header.imageCompression=" << (uint16_t)pvcamMeta->header->imageCompression;
        }
        const rgn_type& irgn = pvcamMeta->impliedRoi;
        dsc << "\nmeta.extMdSize=" << pvcamMeta->extMdDataSize
            << "\nmeta.impliedRoi=["
                << irgn.s1 << "," << irgn.s2 << "," << irgn.sbin << ","
                << irgn.p1 << "," << irgn.p2 << "," << irgn.pbin << "]"
            << "\nmeta.roiCapacity=" << pvcamMeta->roiCapacity
            << "\nmeta.roiCount=" << pvcamMeta->roiCount;
        for (int n = 0; n < pvcamMeta->roiCount; ++n)
        {
            const md_frame_roi& roi = pvcamMeta->roiArray[n];
            const auto roiHdr = roi.header;
            if (roiHdr->flags & PL_MD_ROI_FLAG_INVALID)
                continue; // Skip invalid regions
            const rgn_type& rgn = roiHdr->roi;
            dsc << "\nmeta.roi[" << n << "].header.roiNr=" << roiHdr->roiNr;
            if (pvcamMeta->header->flags & PL_MD_FRAME_FLAG_ROI_TS_SUPPORTED)
            {
                // TODO: Never used in real camera yet, the meaning not clear
                dsc << "\nmeta.roi[" << n << "].header.timeBor=" << roiHdr->timestampBOR
                    << "\nmeta.roi[" << n << "].header.timeEor=" << roiHdr->timestampEOR;
                if (pvcamMeta->header->version < 3)
                {
                    const uint64_t rdrTimeNs =
                        (uint64_t)pvcamMeta->header->roiTimestampResNs
                        * (roiHdr->timestampEOR - roiHdr->timestampBOR);
                    dsc << "\n  (diff=" << rdrTimeNs << "ns)";
                }
                else
                {
                    // TODO: Fix later, for now limited to max 4.2ms readout time
                    const uint32_t rdrTimePs =
                        roiHdr->timestampEOR - roiHdr->timestampBOR;
                    dsc << "\n  (diff=" << rdrTimePs << "ps)";
                }
            }
            dsc << "\nmeta.roi[" << n << "].header.roi=["
                    << rgn.s1 << "," << rgn.s2 << "," << rgn.sbin << ","
                    << rgn.p1 << "," << rgn.p2 << "," << rgn.pbin << "]";
            dsc << "\nmeta.roi[" << n << "].header.flags=0x"
                    << std::hex << (uint16_t)roiHdr->flags << std::dec
                << "\nmeta.roi[" << n << "].header.extMdSize=" << roiHdr->extendedMdSize
                << "\nmeta.roi[" << n << "].dataSize=" << roi.dataSize
                << "\nmeta.roi[" << n << "].extMdSize=" << roi.extMdDataSize;
        }
    }

    return dsc.str();
}

std::string pm::PrdFileUtils::GetPrdImportHints_ImageJ(const PrdHeader& header)
{
    const std::string errorInfo =
        "This PRD file cannot be imported to ImageJ due to unsupported configuration:\n";

    if (header.version >= PRD_VERSION_0_3 && (header.flags & PRD_FLAG_FRAME_SIZE_VARY))
        return errorInfo + "Variable size of extended dynamic data";

    const auto& rgn = header.region;
    if (rgn.sbin == 0 || rgn.pbin == 0)
        return errorInfo + "Zero binning factor(s)";
    const uint32_t width  = ((uint32_t)rgn.s2 + 1 - rgn.s1) / rgn.sbin;
    const uint32_t height = ((uint32_t)rgn.p2 + 1 - rgn.p1) / rgn.pbin;

    pm::BitmapFormat bmpFormat;
    bmpFormat.SetBitDepth(header.bitDepth);
    if (header.version >= PRD_VERSION_0_3)
    {
        bmpFormat.SetColorMask(static_cast<pm::BayerPattern>(header.colorMask));
    }
    if (header.version >= PRD_VERSION_0_6)
    {
        try
        {
            bmpFormat.SetImageFormat(static_cast<pm::ImageFormat>(header.imageFormat));
        }
        catch (...)
        {
            return errorInfo + "Unsupported image format";
        }
    }

    const char* typeStr = "";
    switch (bmpFormat.GetDataType())
    {
    case pm::BitmapDataType::UInt8:
        typeStr = "8-bit";
        break;
    case pm::BitmapDataType::UInt16:
        typeStr = "16-bit Unsigned";
        break;
    case pm::BitmapDataType::UInt32:
        typeStr = "32-bit Unsigned";
        break;
    }
    if (typeStr[0] == '\0')
        return errorInfo + "Unsupported pixel data type";

    // ImageJ requires to know a "gap" between images, rather than offset or
    // something like that. ImageJ apparently calculates the next bitmap offset
    // from the given WxH definition, however, our frame size is not just WxH,
    // it may have padding and alignment. So we need to calculate the "gap"
    // using the WxH and count with any padding.

    const auto rawDataBytes = GetRawDataSize(header);
    if (rawDataBytes == 0)
        return errorInfo + "Zero size of raw data (metadata-only frame(s))";
    const auto prdHeaderBytesAligned = GetAlignedSize(header, sizeof(PrdHeader));
    const auto prdMetaDataBytesAligned = GetAlignedSize(header, header.sizeOfPrdMetaDataStruct);
    const auto rawDataBytesAligned = GetAlignedSize(header, rawDataBytes);

    const auto pvcamHasMetadata =
        header.version >= PRD_VERSION_0_3 && (header.flags & PRD_FLAG_HAS_METADATA);
    const auto pvcamMetadataBytes = pvcamHasMetadata
        ? sizeof(md_frame_header) + sizeof(md_frame_roi_header)
        : 0;

    const auto frameDataOffset = prdMetaDataBytesAligned + pvcamMetadataBytes;
    const auto firstFrameDataOffset = prdHeaderBytesAligned + frameDataOffset;
    const auto gap = rawDataBytesAligned - rawDataBytes + frameDataOffset;

    std::ostringstream nfo;

    nfo << "To import the stack in ImageJ, use following procedure.\n"
        << "\n"
        << "- Drag & drop the .prd file into the ImageJ window or select File -> Import -> Raw...\n"
        << "- In the 'Import' dialog, set the following:\n"
        << "-- Image type: '" << typeStr << "'\n"
        << "-- Width: " << width << " pixels\n"
        << "-- Height: " << height << " pixels\n"
        << "-- Offset to first image: " << firstFrameDataOffset << " bytes\n"
        << "-- Number of images: " << header.frameCount << "\n"
        << "   (it is max. configured value, ImageJ loads all available)\n"
        << "-- Gap between images: " << gap << " bytes\n"
        << "-- White is zero: Unchecked\n"
        << "-- Little-endian byte order: checked\n"
        << "-- Open all files in folder: possibly checked\n"
        << "   (caution with huge stacks, ImageJ loads all data to RAM)\n"
        << "-- Use virtual stack: keep unchecked\n"
        << "   (check only when importing a single huge file to avoid RAM caching)\n";

    return nfo.str();
}
