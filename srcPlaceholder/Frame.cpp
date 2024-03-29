/******************************************************************************/
/* Copyright (C) Teledyne Photometrics. All rights reserved.                  */
/******************************************************************************/
#include "backend/Frame.h"

/* Local */
#include "backend/AllocatorFactory.h"
#include "backend/Log.h"
#include "backend/PvcamRuntimeLoader.h"
#include "backend/TaskSet_CopyMemory.h"
#include "backend/UniqueThreadPool.h"

/* System */
#include <algorithm> // std::for_each, std::min
#include <cassert>
#include <iomanip> // std::setfill, std::setw
#include <new>
#include <sstream>

// Empty instances help eliminate frequent allocation

static const auto sEmptyFrameInfo = pm::Frame::Info();
static const auto sEmptyFrameTrajectories = pm::Frame::Trajectories();

// pm::Frame::AcqCfg

bool operator==(const rgn_type& lhs, const rgn_type& rhs)
{
    return lhs.s1   == rhs.s1
        && lhs.s2   == rhs.s2
        && lhs.sbin == rhs.sbin
        && lhs.p1   == rhs.p1
        && lhs.p2   == rhs.p2
        && lhs.pbin == rhs.pbin;
}

pm::Frame::AcqCfg::AcqCfg()
{
}

pm::Frame::AcqCfg::AcqCfg(size_t frameBytes, uint16_t roiCount, bool hasMetadata,
        const rgn_type& impliedRoi, const BitmapFormat& format,
        AllocatorType allocatorType)
    : m_frameBytes(frameBytes),
    m_roiCount(roiCount),
    m_hasMetadata(hasMetadata),
    m_impliedRoi(impliedRoi),
    m_format(format),
    m_allocatorType(allocatorType)
{
    if (!m_hasMetadata)
    {
        m_outputBmpRois.push_back(m_impliedRoi);
    }
}

pm::Frame::AcqCfg::AcqCfg(size_t frameBytes, uint16_t roiCount, bool hasMetadata,
        const rgn_type& impliedRoi, const BitmapFormat& format,
        const std::vector<rgn_type>& outputBmpRois, AllocatorType allocatorType)
    : m_frameBytes(frameBytes),
    m_roiCount(roiCount),
    m_hasMetadata(hasMetadata),
    m_impliedRoi(impliedRoi),
    m_format(format),
    m_outputBmpRois(outputBmpRois),
    m_allocatorType(allocatorType)
{
}

pm::Frame::AcqCfg::AcqCfg(const Frame::AcqCfg& other)
    : m_frameBytes(other.m_frameBytes),
    m_roiCount(other.m_roiCount),
    m_hasMetadata(other.m_hasMetadata),
    m_impliedRoi(other.m_impliedRoi),
    m_format(other.m_format),
    m_outputBmpRois(other.m_outputBmpRois),
    m_allocatorType(other.m_allocatorType)
{
}

pm::Frame::AcqCfg& pm::Frame::AcqCfg::operator=(const Frame::AcqCfg& other)
{
    if (&other != this)
    {
        m_frameBytes = other.m_frameBytes;
        m_roiCount = other.m_roiCount;
        m_hasMetadata = other.m_hasMetadata;
        m_impliedRoi = other.m_impliedRoi;
        m_format = other.m_format;
        m_outputBmpRois = other.m_outputBmpRois;
        m_allocatorType = other.m_allocatorType;
    }
    return *this;
}

bool pm::Frame::AcqCfg::operator==(const Frame::AcqCfg& other) const
{
    return m_frameBytes == other.m_frameBytes
        && m_roiCount == other.m_roiCount
        && m_hasMetadata == other.m_hasMetadata
        && m_impliedRoi == other.m_impliedRoi
        && m_format == other.m_format
        && m_outputBmpRois == other.m_outputBmpRois
        && m_allocatorType == other.m_allocatorType;
}

bool pm::Frame::AcqCfg::operator!=(const Frame::AcqCfg& other) const
{
    return !(*this == other);
}

size_t pm::Frame::AcqCfg::GetFrameBytes() const
{
    return m_frameBytes;
}

void pm::Frame::AcqCfg::SetFrameBytes(size_t frameBytes)
{
    m_frameBytes = frameBytes;
}

uint16_t pm::Frame::AcqCfg::GetRoiCount() const
{
    return m_roiCount;
}

void pm::Frame::AcqCfg::SetRoiCount(uint16_t roiCount)
{
    m_roiCount = roiCount;
}

bool pm::Frame::AcqCfg::HasMetadata() const
{
    return m_hasMetadata;
}

void pm::Frame::AcqCfg::SetMetadata(bool hasMetadata)
{
    m_hasMetadata = hasMetadata;
}

const rgn_type& pm::Frame::AcqCfg::GetImpliedRoi() const
{
    return m_impliedRoi;
}

void pm::Frame::AcqCfg::SetImpliedRoi(const rgn_type& impliedRoi)
{
    m_impliedRoi = impliedRoi;
}

const pm::BitmapFormat& pm::Frame::AcqCfg::GetBitmapFormat() const
{
    return m_format;
}

void pm::Frame::AcqCfg::SetBitmapFormat(const BitmapFormat& format)
{
    m_format = format;
}

const std::vector<rgn_type>& pm::Frame::AcqCfg::GetOutputBmpRois() const
{
    return m_outputBmpRois;
}

void pm::Frame::AcqCfg::SetOutputBmpRois(const std::vector<rgn_type>& rois)
{
    m_outputBmpRois = rois;
}

pm::AllocatorType pm::Frame::AcqCfg::GetAllocatorType() const
{
    return m_allocatorType;
}

void pm::Frame::AcqCfg::SetAllocatorType(AllocatorType type)
{
    m_allocatorType = type;
}

// pm::Frame::Info

pm::Frame::Info::Info()
{
}

pm::Frame::Info::Info(uint32_t frameNr, uint64_t timestampBOF, uint64_t timestampEOF)
    : m_frameNr(frameNr),
    m_timestampBOF(timestampBOF),
    m_timestampEOF(timestampEOF),
    m_readoutTime((uint32_t)(timestampEOF - timestampBOF))
{
}

pm::Frame::Info::Info(uint32_t frameNr, uint64_t timestampBOF, uint64_t timestampEOF,
        uint32_t expTime)
    : m_frameNr(frameNr),
    m_timestampBOF(timestampBOF),
    m_timestampEOF(timestampEOF),
    m_readoutTime((uint32_t)(timestampEOF - timestampBOF)),
    m_expTime(expTime)
{
}

pm::Frame::Info::Info(uint32_t frameNr, uint64_t timestampBOF, uint64_t timestampEOF,
        uint32_t expTime, float colorWbScaleRed, float colorWbScaleGreen,
        float colorWbScaleBlue)
    : m_frameNr(frameNr),
    m_timestampBOF(timestampBOF),
    m_timestampEOF(timestampEOF),
    m_readoutTime((uint32_t)(timestampEOF - timestampBOF)),
    m_expTime(expTime),
    m_colorWbScaleRed(colorWbScaleRed),
    m_colorWbScaleGreen(colorWbScaleGreen),
    m_colorWbScaleBlue(colorWbScaleBlue)
{
}

pm::Frame::Info::Info(const Frame::Info& other)
    : m_frameNr(other.m_frameNr),
    m_timestampBOF(other.m_timestampBOF),
    m_timestampEOF(other.m_timestampEOF),
    m_readoutTime(other.m_readoutTime),
    m_expTime(other.m_expTime),
    m_colorWbScaleRed(other.m_colorWbScaleRed),
    m_colorWbScaleGreen(other.m_colorWbScaleGreen),
    m_colorWbScaleBlue(other.m_colorWbScaleBlue)
{
}

pm::Frame::Info& pm::Frame::Info::operator=(const Frame::Info& other)
{
    if (&other != this)
    {
        m_frameNr = other.m_frameNr;
        m_timestampBOF = other.m_timestampBOF;
        m_timestampEOF = other.m_timestampEOF;
        m_readoutTime = other.m_readoutTime;
        m_expTime = other.m_expTime;
        m_colorWbScaleRed = other.m_colorWbScaleRed;
        m_colorWbScaleGreen = other.m_colorWbScaleGreen;
        m_colorWbScaleBlue = other.m_colorWbScaleBlue;
    }
    return *this;
}

bool pm::Frame::Info::operator==(const Frame::Info& other) const
{
    return m_frameNr == other.m_frameNr
        && m_timestampBOF == other.m_timestampBOF
        && m_timestampEOF == other.m_timestampEOF
        && m_expTime == other.m_expTime
        && m_colorWbScaleRed == other.m_colorWbScaleRed
        && m_colorWbScaleGreen == other.m_colorWbScaleGreen
        && m_colorWbScaleBlue == other.m_colorWbScaleBlue;
}

bool pm::Frame::Info::operator!=(const Frame::Info& other) const
{
    return !(*this == other);
}

uint32_t pm::Frame::Info::GetFrameNr() const
{
    return m_frameNr;
}

uint64_t pm::Frame::Info::GetTimestampBOF() const
{
    return m_timestampBOF;
}

uint64_t pm::Frame::Info::GetTimestampEOF() const
{
    return m_timestampEOF;
}

uint32_t pm::Frame::Info::GetReadoutTime() const
{
    return m_readoutTime;
}

uint32_t pm::Frame::Info::GetExpTime() const
{
    return m_expTime;
}

float pm::Frame::Info::GetColorWbScaleRed() const
{
    return m_colorWbScaleRed;
}

float pm::Frame::Info::GetColorWbScaleGreen() const
{
    return m_colorWbScaleGreen;
}

float pm::Frame::Info::GetColorWbScaleBlue() const
{
    return m_colorWbScaleBlue;
}

// pm::Frame

pm::Frame::Frame(const AcqCfg& acqCfg, bool deepCopy,
        std::shared_ptr<Allocator> allocator)
    : m_acqCfg(acqCfg),
    m_deepCopy(deepCopy),
    m_allocator((allocator)
        ? allocator
        : (m_deepCopy)
            ? AllocatorFactory::Create(m_acqCfg.GetAllocatorType())
            : nullptr),
    m_needsDecoding(m_acqCfg.HasMetadata()),
    m_tasksMemCopy(new(std::nothrow) TaskSet_CopyMemory(
            UniqueThreadPool::Get().GetPool()))
{
    assert(!m_deepCopy || m_allocator);
    assert(!m_deepCopy || m_allocator->GetType() == m_acqCfg.GetAllocatorType());

    if (m_deepCopy && m_acqCfg.GetFrameBytes() > 0)
    {
        m_data = m_allocator->Allocate(m_acqCfg.GetFrameBytes());
        if (!m_data)
        {
            Log::LogE("Failed to allocate frame data");
        }
    }

    const auto& outputBmpRois = m_acqCfg.GetOutputBmpRois();
    const size_t maxOutputBmpRoiSize =
        (outputBmpRois.empty()) ? m_acqCfg.GetRoiCount() : outputBmpRois.size();

    m_roiBitmaps.resize(maxOutputBmpRoiSize);
    m_roiBitmapRegions.resize(maxOutputBmpRoiSize);
    m_roiBitmapPositions.resize(maxOutputBmpRoiSize);

    if (m_acqCfg.HasMetadata())
    {
        if (!PvcamRuntimeLoader::Get()->hasMetadataFunctions())
        {
            Log::LogE("Failed to allocate frame metadata structure."
                " Loaded PVCAM does not support metadata");
        }
        else
        {
            if (PV_OK != PVCAM->pl_md_create_frame_struct_cont(
                        &m_metadata, m_acqCfg.GetRoiCount()))
            {
                Log::LogE("Failed to allocate frame metadata structure");
            }
        }
    }
}

pm::Frame::~Frame()
{
    delete m_tasksMemCopy;

    if (m_deepCopy)
    {
        // m_data must not be deleted on shallow copy
        m_allocator->Free(m_data);
    }

    if (m_metadata)
    {
        // Ignore errors
        PVCAM->pl_md_release_frame_struct(m_metadata);
    }
}

const pm::Frame::AcqCfg& pm::Frame::GetAcqCfg() const
{
    // Locking mutex not needed, it's const
    return m_acqCfg;
}

bool pm::Frame::UsesDeepCopy() const
{
    // Locking mutex not needed, it's const
    return m_deepCopy;
}

std::shared_ptr<pm::Allocator> pm::Frame::GetAllocator() const
{
    // Locking mutex not needed, it's const
    return m_allocator;
}

void pm::Frame::SetDataPointer(void* data)
{
    std::lock_guard<std::shared_timed_mutex> lock(m_mutex);

    DoSetDataPointer(data);
}

bool pm::Frame::CopyData()
{
    std::lock_guard<std::shared_timed_mutex> lock(m_mutex);

    return DoCopyData();
}

const void* pm::Frame::GetData() const
{
    std::shared_lock<std::shared_timed_mutex> lock(m_mutex);

    return m_data;
}

bool pm::Frame::IsValid() const
{
    std::shared_lock<std::shared_timed_mutex> lock(m_mutex);

    return m_isValid;
}

void pm::Frame::Invalidate()
{
    std::lock_guard<std::shared_timed_mutex> lock(m_mutex);

    DoInvalidate();
}

void pm::Frame::OverrideValidity(bool isValid)
{
    std::lock_guard<std::shared_timed_mutex> lock(m_mutex);

    DoOverrideValidity(isValid);
}

const pm::Frame::Info& pm::Frame::GetInfo() const
{
    std::shared_lock<std::shared_timed_mutex> lock(m_mutex);

    return m_info;
}

void pm::Frame::SetInfo(const Frame::Info& frameInfo)
{
    std::lock_guard<std::shared_timed_mutex> lock(m_mutex);

    DoSetInfo(frameInfo);
}

const pm::Frame::Trajectories& pm::Frame::GetTrajectories() const
{
    std::shared_lock<std::shared_timed_mutex> lock(m_mutex);

    return m_trajectories;
}

void pm::Frame::SetTrajectories(const Frame::Trajectories& trajectories)
{
    std::lock_guard<std::shared_timed_mutex> lock(m_mutex);

    DoSetTrajectories(trajectories);
}

bool pm::Frame::DecodeMetadata()
{
    std::lock_guard<std::shared_timed_mutex> lock(m_mutex);

    if (!m_needsDecoding)
        return true;

    if (!m_isValid)
    {
        Log::LogE("Invalid frame");
        return false;
    }

    if (!PvcamRuntimeLoader::Get()->hasMetadataFunctions())
    {
        Log::LogE("Unable to decode frame. Loaded PVCAM does not support metadata");
        return false;
    }

    const uns32 frameBytes = (uns32)m_acqCfg.GetFrameBytes();

    if (PV_OK != PVCAM->pl_md_frame_decode(m_metadata, m_data, frameBytes))
    {
        const int16 errId = PVCAM->pl_error_code();
        char errMsg[ERROR_MSG_LEN] = "<unknown>";
        PVCAM->pl_error_message(errId, errMsg);

        const uint8_t* dumpData = static_cast<uint8_t*>(m_data);
        const uint32_t dumpDataBytes = std::min<uint32_t>(frameBytes, 32u);
        std::ostringstream dumpStream;
        dumpStream << std::hex << std::uppercase << std::setfill('0');
        std::for_each(dumpData, dumpData + dumpDataBytes, [&dumpStream](uint8_t byte) {
            dumpStream << ' ' << std::setw(2) << (unsigned int)byte;
        });

        Log::LogE("Unable to decode frame %u (%s), addr: 0x%p, data: %s",
                m_info.GetFrameNr(), errMsg, m_data, dumpStream.str().c_str());

        DoInvalidate();
        return false;
    }

    m_roiBitmapValidCount = 0;
    for (uint16_t roiIdx = 0; roiIdx < m_metadata->roiCount; ++roiIdx)
    {
        const md_frame_roi& mdRoi = m_metadata->roiArray[roiIdx];

        if (!(mdRoi.header->flags & PL_MD_ROI_FLAG_HEADER_ONLY))
        {
            const rgn_type& rgn = mdRoi.header->roi;
            const uint16_t roiX = rgn.s1 / rgn.sbin;
            const uint16_t roiY = rgn.p1 / rgn.pbin;
            const uint32_t roiW = (rgn.s2 - rgn.s1 + 1) / rgn.sbin;
            const uint32_t roiH = (rgn.p2 - rgn.p1 + 1) / rgn.pbin;
            try
            {
                m_roiBitmaps[roiIdx] = std::move(std::make_unique<Bitmap>(
                            mdRoi.data, roiW, roiH, m_acqCfg.GetBitmapFormat()));
                m_roiBitmapRegions[roiIdx] = rgn;
                m_roiBitmapPositions[roiIdx] = Point(roiX, roiY);
                m_roiBitmapValidCount++;
            }
            catch (...)
            {
                Log::LogE("Failed to allocate Bitmap wrapper for ROI index %u", roiIdx);
                DoInvalidate();
                return false;
            }
        }

        if (mdRoi.extMdDataSize > 0)
        {
            md_ext_item_collection* collection = &m_extMetadata[mdRoi.header->roiNr];

            // Extract extended metadata from ROI
            if (PV_OK != PVCAM->pl_md_read_extended(collection, mdRoi.extMdData,
                        mdRoi.extMdDataSize))
            {
                const int16 errId = PVCAM->pl_error_code();
                char errMsg[ERROR_MSG_LEN] = "<unknown>";
                PVCAM->pl_error_message(errId, errMsg);

                Log::LogE("Failed to read ext. metadata for frame nr. %u (%s)",
                        m_info.GetFrameNr(), errMsg);

                DoInvalidate();
                return false;
            }
        }
    }

    m_needsDecoding = false;

    return true;
}

const md_frame* pm::Frame::GetMetadata() const
{
    std::shared_lock<std::shared_timed_mutex> lock(m_mutex);

    return m_metadata;
}

const std::map<uint16_t, md_ext_item_collection>& pm::Frame::GetExtMetadata() const
{
    std::shared_lock<std::shared_timed_mutex> lock(m_mutex);

    return m_extMetadata;
}

const std::vector<std::unique_ptr<pm::Bitmap>>& pm::Frame::GetRoiBitmaps() const
{
    std::shared_lock<std::shared_timed_mutex> lock(m_mutex);

    return m_roiBitmaps;
}

const std::vector<rgn_type>& pm::Frame::GetRoiBitmapRegions() const
{
    return m_roiBitmapRegions;
}

const std::vector<pm::Frame::Point>& pm::Frame::GetRoiBitmapPositions() const
{
    return m_roiBitmapPositions;
}

size_t pm::Frame::GetRoiBitmapValidCount() const
{
    return m_roiBitmapValidCount;
}

bool pm::Frame::Copy(const Frame& other, bool deepCopy)
{
    std::unique_lock<std::shared_timed_mutex> wrLock(m_mutex, std::defer_lock);
    std::shared_lock<std::shared_timed_mutex> rdLock(other.m_mutex, std::defer_lock);
    std::lock(wrLock, rdLock);

    return DoCopy(other, *this, deepCopy);
}

std::shared_ptr<pm::Frame> pm::Frame::Clone(bool deepCopy) const
{
    std::shared_lock<std::shared_timed_mutex> lock(m_mutex);

    std::shared_ptr<pm::Frame> frame;

    try
    {
        frame = std::make_shared<Frame>(m_acqCfg, deepCopy, m_allocator);
    }
    catch (...)
    {
        return nullptr;
    }

    if (!DoCopy(*this, *frame, true))
    {
        // Probably never happens
        return nullptr;
    }

    return frame;
}

void pm::Frame::DoSetDataPointer(void* data)
{
    m_dataSrc = data;
}

bool pm::Frame::DoCopyData()
{
    DoInvalidate();

    if (!m_dataSrc)
    {
        Log::LogE("Invalid source data pointer");
        return false;
    }

    if (m_acqCfg.HasMetadata() && !m_metadata)
    {
        Log::LogE("Invalid metadata pointer");
        return false;
    }

    if (m_deepCopy)
    {
        if (!m_data)
        {
            Log::LogE("Invalid data pointer");
            return false;
        }

        m_tasksMemCopy->SetUp(m_data, m_dataSrc, m_acqCfg.GetFrameBytes());
        m_tasksMemCopy->Execute();
        m_tasksMemCopy->Wait();
    }
    else
    {
        m_data = m_dataSrc;
    }

    if (m_shallowInfo != sEmptyFrameInfo)
    {
        m_info = m_shallowInfo;
        m_shallowInfo = sEmptyFrameInfo;
    }

    return DoOverrideValidity(true);
}

void pm::Frame::DoInvalidate()
{
    if (!m_isValid)
        return;

    m_isValid = false;

    m_info = sEmptyFrameInfo;
    m_trajectories = sEmptyFrameTrajectories;

    m_needsDecoding = m_acqCfg.HasMetadata();
    if (m_metadata)
    {
        // Invalidate metadata
        m_metadata->roiCount = 0;
    }
    m_extMetadata.clear();

    for (auto&& roiBitmap : m_roiBitmaps)
    {
        roiBitmap = nullptr;
    }
    m_roiBitmapValidCount = 0;
}

bool pm::Frame::DoOverrideValidity(bool isValid)
{
    m_isValid = isValid;

    // For meta-frame, bitmaps are updated during decode, for non-meta-frame here
    if (isValid && !m_acqCfg.HasMetadata())
    {
        const uint16_t roiIdx = 0;
        const rgn_type& rgn = m_acqCfg.GetImpliedRoi();
        const uint16_t roiX = rgn.s1 / rgn.sbin;
        const uint16_t roiY = rgn.p1 / rgn.pbin;
        const uint32_t roiW = (rgn.s2 - rgn.s1 + 1) / rgn.sbin;
        const uint32_t roiH = (rgn.p2 - rgn.p1 + 1) / rgn.pbin;
        try
        {
            m_roiBitmaps[roiIdx] = std::move(std::make_unique<Bitmap>(
                        m_data, roiW, roiH, m_acqCfg.GetBitmapFormat()));
            m_roiBitmapRegions[roiIdx] = rgn;
            m_roiBitmapPositions[roiIdx] = Point(roiX, roiY);
            m_roiBitmapValidCount = 1;
        }
        catch (...)
        {
            Log::LogE("Failed to allocate Bitmap wrapper for ROI index %u", roiIdx);
            return false;
        }
    }

    return true;
}

void pm::Frame::DoSetInfo(const Frame::Info& frameInfo)
{
    m_info = frameInfo;
}

void pm::Frame::DoSetTrajectories(const Frame::Trajectories& trajectories)
{
    m_trajectories = trajectories;
}

bool pm::Frame::DoCopy(const Frame& from, Frame& to, bool deepCopy) const
{
    if (from.m_acqCfg != to.m_acqCfg)
    {
        to.DoInvalidate();
        Log::LogE("Failed to copy frame due to configuration mismatch");
        return false;
    }

    to.DoSetDataPointer(from.m_data);

    if (deepCopy)
    {
        if (!to.DoCopyData())
            return false;

        to.DoSetInfo(from.m_info);
        to.DoSetTrajectories(from.m_trajectories);

        to.m_shallowInfo = sEmptyFrameInfo;
    }
    else
    {
        to.m_shallowInfo = from.m_info;
    }

    return true;
}
