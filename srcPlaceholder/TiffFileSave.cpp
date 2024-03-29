/******************************************************************************/
/* Copyright (C) Teledyne Photometrics. All rights reserved.                  */
/******************************************************************************/
#include "backend/TiffFileSave.h"

/* Local */
#include "backend/Bitmap.h"
#include "backend/ColorRuntimeLoader.h"
#include "backend/ColorUtils.h"
#include "backend/FrameProcessor.h"
#include "backend/Log.h"
#include "backend/PrdFileUtils.h"
#include "backend/PvcamRuntimeLoader.h"

/* PVCAM */
#include "master.h"
#include "pvcam.h"

/* libtiff */
#include <tiffio.h>

/* System */
#include <cstring>
#include <limits>
#include <map>

pm::TiffFileSave::TiffFileSave(const std::string& fileName, PrdHeader& header,
        bool useBigTiff)
    : FileSave(fileName, header),
    m_helper(nullptr), // Created during open
    m_helperOwned(true),
    m_isBigTiff(useBigTiff)
{
    // Turn off alignment for TIFF
    m_header.flags &= ~PRD_FLAG_HAS_ALIGNMENT;
    m_header.alignment = 0;
}

pm::TiffFileSave::TiffFileSave(const std::string& fileName, PrdHeader& header,
        Helper* helper, bool useBigTiff)
    : FileSave(fileName, header),
    m_helper(helper),
    m_helperOwned(false),
    m_isBigTiff(useBigTiff)
{
    // Turn off alignment for TIFF
    m_header.flags &= ~PRD_FLAG_HAS_ALIGNMENT;
    m_header.alignment = 0;
}

pm::TiffFileSave::~TiffFileSave()
{
    if (IsOpen())
        Close();

    if (m_helper && m_helperOwned)
    {
        delete m_helper->fullBmp;
        delete m_helper->frameProc;
        delete m_helper;
    }
}

bool pm::TiffFileSave::Open()
{
    if (IsOpen())
        return true;

    if (m_helperOwned)
    {
        BitmapFormat bmpFormat;
        bmpFormat.SetBitDepth(m_header.bitDepth);
        bmpFormat.SetColorMask((m_header.version >= PRD_VERSION_0_3)
                ? static_cast<BayerPattern>(m_header.colorMask)
                : BayerPattern::None);
        try
        {
            bmpFormat.SetImageFormat(static_cast<ImageFormat>(m_header.imageFormat));
        }
        catch (...)
        {
            Log::LogE("Failed allocation of internal bitmap format");
            return false;
        }
        m_helper = new(std::nothrow) Helper();
        if (!m_helper)
        {
            Log::LogE("Failed allocation of internal helper structure");
            return false;
        }
        m_helper->frameProc = new(std::nothrow) FrameProcessor();
        if (!m_helper->frameProc)
        {
            Log::LogE("Failed allocation of internal frame processor");
            return false;
        }
        m_helper->fullBmp = new(std::nothrow) Bitmap(m_width, m_height, bmpFormat);
        if (!m_helper->fullBmp)
        {
            Log::LogE("Failed allocation of internal bitmap");
            return false;
        }
    }
    else
    {
        if (!m_helper || !m_helper->frameProc || !m_helper->fullBmp)
        {
            Log::LogE("Given frame processing helper not initialized");
            return false;
        }
        if (m_helper->fullBmp->GetWidth() != m_width
                || m_helper->fullBmp->GetHeight() != m_height)
        {
            Log::LogE("Given helper bitmap is too small");
            return false;
        }
    }

#if SIZE_MAX > UINT32_MAX
    if (!m_isBigTiff && m_rawDataBytes > std::numeric_limits<uint32_t>::max())
    {
        Log::LogE("TIFF format is unable to store more than 4GB raw data");
        return false;
    }
#endif

    // Image description without metadata has 200-220 bytes,
    // but including metadata it could be 1-105 kB!
    constexpr size_t estimatedOverheadBytes = 1500;
    const size_t estimatedFileBytes =
        m_header.frameCount * (estimatedOverheadBytes + m_rawDataBytes);
    if (!m_isBigTiff && estimatedFileBytes > std::numeric_limits<uint32_t>::max())
    {
        Log::LogE("The libtiff is unable to store Classic TIFF files bigger than 4GB,"
                " use Big TIFF instead");
        return false;
    }

    if (m_header.frameCount > std::numeric_limits<uint16_t>::max())
    {
        Log::LogE("TIFF format is unable to store more than 65536 pages");
        return false;
    }

    const char* mode = (m_isBigTiff) ? "w8" : "w";
    m_file = TIFFOpen(m_fileName.c_str(), mode);
    if (!m_file)
        return false;

    m_frameIndex = 0;

    return IsOpen();
}

bool pm::TiffFileSave::IsOpen() const
{
    return !!m_file;
}

void pm::TiffFileSave::Close()
{
    if (!IsOpen())
        return;

    if (m_header.frameCount != m_frameIndex)
    {
        m_header.frameCount = m_frameIndex;

        const auto tiffFrameCount = (uint16_t)m_frameIndex;
        for (uint16_t n = 0; n < tiffFrameCount; ++n)
        {
            if (!TIFFSetDirectory(m_file, n)
                    || !TIFFSetField(m_file, TIFFTAG_PAGENUMBER, n, tiffFrameCount)
                    || !TIFFWriteDirectory(m_file))
            {
                Log::LogE("Failed to fix frame count in multi-page tiff");
                break;
            }
        }
    }

    TIFFFlush(m_file);
    TIFFClose(m_file);
    m_file = nullptr;

    FileSave::Close();
}

bool pm::TiffFileSave::WriteFrame(const void* metaData,
        const void* extDynMetaData, const void* rawData)
{
    auto frame = PrdFileUtils::ReconstructFrame(m_header, metaData, extDynMetaData, rawData);
    if (!frame || !frame->IsValid())
    {
        pm::Log::LogE("Failed to reconstruct frame");
        return false;
    }

    return WriteFrame(frame);
}

bool pm::TiffFileSave::WriteFrame(std::shared_ptr<Frame> frame)
{
    if (!FileSave::WriteFrame(frame))
        return false;

    if (m_frameIndex >= m_header.frameCount)
    {
        Log::LogE("Cannot write more pages to TIFF file than declared during open");
        return false;
    }

    return DoWriteFrame(frame);
}

bool pm::TiffFileSave::DoWriteFrame(std::shared_ptr<Frame> frame)
{
    if (!frame->DecodeMetadata())
        return false;

    const md_frame* frameMeta = frame->GetMetadata();

    try
    {
        m_helper->frameProc->SetFrame(frame);

        FrameProcessor::UseBmp fullBmpType;
        if (m_helper->colorCtx)
        {
            if (m_header.version >= PRD_VERSION_0_7)
            {
                // Override possibly different RGB scales per frame
                auto metaData = static_cast<PrdMetaData*>(m_framePrdMetaData);
                if (       m_helper->colorCtx->redScale   != metaData->colorWbScaleRed
                        || m_helper->colorCtx->greenScale != metaData->colorWbScaleGreen
                        || m_helper->colorCtx->blueScale  != metaData->colorWbScaleBlue)
                {
                    m_helper->colorCtx->redScale   = metaData->colorWbScaleRed;
                    m_helper->colorCtx->greenScale = metaData->colorWbScaleGreen;
                    m_helper->colorCtx->blueScale  = metaData->colorWbScaleBlue;

                    if (PH_COLOR_ERROR_NONE
                            != PH_COLOR->context_apply_changes(m_helper->colorCtx))
                    {
                        pm::ColorUtils::LogError("Failure applying color context changes");
                        return false;
                    }
                }
            }
            m_helper->frameProc->Debayer(m_helper->colorCtx);
            fullBmpType = FrameProcessor::UseBmp::Debayered;
        }
        else
        {
            fullBmpType = FrameProcessor::UseBmp::Raw;
        }

        rgn_type rgn;
        uint16_t roiCount;
        if (frame->GetAcqCfg().HasMetadata())
        {
            rgn = frameMeta->impliedRoi;
            roiCount = frameMeta->roiCount;
        }
        else
        {
            rgn = frame->GetAcqCfg().GetImpliedRoi();
            roiCount = 1;
        }

        const uint32_t rgnW = ((uint32_t)rgn.s2 + 1 - rgn.s1) / rgn.sbin;
        const uint32_t rgnH = ((uint32_t)rgn.p2 + 1 - rgn.p1) / rgn.pbin;

        const bool hasNoData = roiCount == 0
            || rgn.s1 > rgn.s2 || rgnW > m_width  || rgn.sbin == 0
            || rgn.p1 > rgn.p2 || rgnH > m_height || rgn.pbin == 0;

        // Fill image with some value only if metadata has more or no regions,
        // or the only valid region doesn't cover whole area
        const bool isFillNeeded = (hasNoData)
            ? true
            : roiCount > 1 || rgnW != m_width || rgnH != m_height;
        if (isFillNeeded)
        {
            auto fillValue = m_helper->fillValue;
            if (fillValue < -0.5)
            {
                m_helper->frameProc->ComputeStats();
                fillValue = m_helper->frameProc->GetStats().GetMean();
            }
            m_helper->frameProc->Fill(m_helper->fullBmp, fillValue);
        }

        if (!hasNoData)
        {
            const uint16_t rgnX = rgn.s1 / rgn.sbin;
            const uint16_t rgnY = rgn.p1 / rgn.pbin;
            m_helper->frameProc->Recompose(fullBmpType, m_helper->fullBmp, rgnX, rgnY);
        }
    }
    catch (const std::exception& ex)
    {
        Log::LogE("Failed to process frame - %s", ex.what());
        return false;
    }

    const auto imageDesc =
        PrdFileUtils::GetImageDescription(m_header, m_framePrdMetaData, frameMeta);

    return DoWriteTiff(m_helper->fullBmp, imageDesc);
}

bool pm::TiffFileSave::DoWriteTiff(const Bitmap* bmp, const std::string& imageDesc)
{
    const auto& bmpFormat = bmp->GetFormat();
    TIFFSetField(m_file, TIFFTAG_IMAGEWIDTH, m_width);
    TIFFSetField(m_file, TIFFTAG_IMAGELENGTH, m_height);
    TIFFSetField(m_file, TIFFTAG_SAMPLEFORMAT, SAMPLEFORMAT_UINT);
    TIFFSetField(m_file, TIFFTAG_BITSPERSAMPLE, 8 * bmpFormat.GetBytesPerSample());
    TIFFSetField(m_file, TIFFTAG_ORIENTATION, ORIENTATION_TOPLEFT);
    TIFFSetField(m_file, TIFFTAG_PLANARCONFIG, PLANARCONFIG_CONTIG);
    //TIFFSetField(m_file, TIFFTAG_ROWSPERSTRIP, 8); // Keep commented out!
    TIFFSetField(m_file, TIFFTAG_SAMPLESPERPIXEL, bmpFormat.GetSamplesPerPixel());
    TIFFSetField(m_file, TIFFTAG_PHOTOMETRIC, PHOTOMETRIC_MINISBLACK);
    if (bmpFormat.GetBitDepth() <= 16)
    {
        TIFFSetField(m_file, TIFFTAG_MAXSAMPLEVALUE, (1u << bmpFormat.GetBitDepth()) - 1);
    }

    if (m_header.frameCount > 1)
    {
        // We are writing single page of the multi-page file
        TIFFSetField(m_file, TIFFTAG_SUBFILETYPE, FILETYPE_PAGE);
        // Set the page number
        TIFFSetField(m_file, TIFFTAG_PAGENUMBER, m_frameIndex, m_header.frameCount);
    }

    // Put the PVCAM metadata into the image description
    TIFFSetField(m_file, TIFFTAG_IMAGEDESCRIPTION, imageDesc.c_str());

    auto tiffData = bmp->GetData();
    const auto tiffDataBytes = bmp->GetDataBytes();
    // This is fastest streaming option, but it requires the TIFFTAG_ROWSPERSTRIP
    // tag is not set (or maybe requires well calculated value)
    if (tiffDataBytes != (size_t)TIFFWriteRawStrip(m_file, 0, tiffData, tiffDataBytes))
        return false;

    if (m_header.frameCount > 1)
    {
        TIFFWriteDirectory(m_file);
    }

    m_frameIndex++;
    return true;
}
