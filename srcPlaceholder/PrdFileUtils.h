/******************************************************************************/
/* Copyright (C) Teledyne Photometrics. All rights reserved.                  */
/******************************************************************************/
#pragma once
#ifndef PM_PRD_FILE_UTILS_H
#define PM_PRD_FILE_UTILS_H

/* Local */
#include "backend/Frame.h"
#include "backend/PrdFileFormat.h"

/* System */
#include <cstdlib>

namespace pm {

/// Provides various helper functions related to PRD file format.
class PrdFileUtils
{
public:
    /// Function initializes PrdHeader structure with zeros and sets its type member.
    static void ClearPrdHeaderStructure(PrdHeader& header);

    /// Initializes PrdHeader structure with given data and 1 frame.
    /** The prdExpTimeRes is a PRD_EXP_RES_ value.
        The alignment shall be a power of two or zero for default behavior. Also for
        alignment equal to 1 this function changes it to zero and disables it. */
    static void InitPrdHeaderStructure(PrdHeader& header, uint16_t version,
            const pm::Frame::AcqCfg& acqCfg, const PrdRegion& prdRegion,
            uint32_t prdExpTimeRes, uint16_t alignment = 0);

    /// Initializes PrdHeader structure with given data and 1 frame.
    /** The pvcamExpTimeRes is a EXP_RES_ONE_ value.
        The alignment shall be a power of two or zero for default behavior. Also for
        alignment equal to 1 this function changes it to zero and disables it. */
    static void InitPrdHeaderStructure(PrdHeader& header, uint16_t version,
            const pm::Frame::AcqCfg& acqCfg, const rgn_type& pvcamRegion,
            uint32_t pvcamExpTimeRes, uint16_t alignment = 0);

    /// Calculates aligned size in bytes for given @a size.
    /** It requires only following header members: flags and alignment. */
    static size_t GetAlignedSize(const PrdHeader& header, size_t size);

    /// Calculates RAW data size in bytes.
    /** It requires only following header members: region and frameSize. */
    static size_t GetRawDataSize(const PrdHeader& header);

    /// Calculates PRD file data overhead in bytes from its header.
    /** It requires only following header members: frameCount,
        sizeOfPrdMetaDataStruct and alignment.
        Returned size does not include possible extended dynamic metadata. */
    static size_t GetPrdFileSizeOverhead(const PrdHeader& header);

    /// Calculates size in bytes of whole PRD file from its header.
    /** It requires only following header members: region, frameSize, frameCount,
        sizeOfPrdMetaDataStruct and alignment.
        Returned size does not include possible extended dynamic metadata. */
    static size_t GetPrdFileSize(const PrdHeader& header);

    /// Calculates max. number of frames in PRD file that fits into given limit.
    /** It requires only following header members: region, frameSize,
        sizeOfPrdMetaDataStruct and alignment.
        Returned value is restricted to uint32_t by PrdHeader.frameCount type.
        It returns 0 if in given size fits more frames than can be stored in uint32_t. */
    static uint32_t GetPrdFrameCountThatFitsIn(const PrdHeader& header,
            size_t maxSizeInBytes);

    /// Returns beginning of extended metadata block for given flag.
    /** The given flag has to be one of PRD_EXT_FLAG_* values, not the combined
        value as stored in PrdMetaData structure. If more flags are combined, or if
        there is no extended metadata, function returns null. */
    static const void* GetExtMetadataAddress(const PrdHeader& header,
            const void* metadata, uint32_t extFlag);

    /// Calculates number of bytes required to store given trajectories.
    /** The size includes also all headers as described in PRD file format.
        The size is calculated for the given capacity, not from current number of
        trajectories and points in each trajectory. */
    static uint32_t GetTrajectoriesSize(
            const PrdTrajectoriesHeader* trajectoriesHeader);

    /// Converts trajectories from raw data as stored in PRD file to C++ containers.
    /** If #GetTrajectoriesSize(@a from) returns zero, the @a to argument is
        not touched. Thus it should be zeroed or default-initialized before call. */
    static bool ConvertTrajectoriesFromPrd(const PrdTrajectoriesHeader* from,
            pm::Frame::Trajectories& to);

    /// Converts trajectories from C++ containers to raw data as stored in PRD file.
    /** If the capacity of trajectories and points in each trajectory are zero, the
        @a to argument is not touched. Thus it should be zeroed before call. */
    static bool ConvertTrajectoriesToPrd(const pm::Frame::Trajectories& from,
            PrdTrajectoriesHeader* to);

    /// Reconstructs whole frame from file.
    /** If everything goes well, new Frame instance is allocated, filled with data,
        trajectories, etc. On error a null is returned. */
    static std::shared_ptr<pm::Frame> ReconstructFrame(const PrdHeader& header,
            const void* metaData, const void* extDynMetaData, const void* rawData);

    /// Generates description for single image.
    /** The description is multi-line and contains names and values of all
        members of given structures. It is esp. useful for TIFF metadata. */
    static std::string GetImageDescription(const PrdHeader& header,
            const void* prdMeta, const md_frame* pvcamMeta);

    /// Prepares instructions how to import PRD file to ImageJ.
    /** The import is possible only if PRD file doesn't contain extended
        dynamic data (due to possibly variable size per frame), and if raw
        data either doesn't contain PVCAM metadata, or the metadata describes
        one ROI only (neither multiple regions nor centroids are supported). */
    static std::string GetPrdImportHints_ImageJ(const PrdHeader& header);
};

} // namespace

#endif
