/******************************************************************************/
/* Copyright (C) Teledyne Photometrics. All rights reserved.                  */
/******************************************************************************/
#pragma once
#ifndef PM_FRAME_H
#define PM_FRAME_H

/* Local */
#include "backend/Allocator.h"
#include "backend/Bitmap.h"
#include "backend/BitmapFormat.h"
#include "backend/PrdFileFormat.h"

/* PVCAM */
#include "master.h"
#include "pvcam.h" // rgn_type, md_frame

/* System */
#include <cstdint>
#include <cstring> // memset
#include <map>
#include <memory> // std::shared_ptr, std::unique_ptr
#include <mutex> // std::lock_guard, std::unique_lock
#include <shared_mutex> // std::shared_(timed_)mutex, std::shared_lock
#include <utility> // std::pair
#include <vector>

namespace pm {

class TaskSet_CopyMemory;

class Frame
{
public:
    struct Point
    {
        Point() {}
        Point(uint16_t x, uint16_t y) : x(x), y(y) {}
        uint16_t x{ 0 };
        uint16_t y{ 0 };
    };

    struct Trajectory
    {
        Trajectory() : header(), data() { memset(&header, 0, sizeof(header)); }
        PrdTrajectoryHeader header;
        std::vector<PrdTrajectoryPoint> data;
    };

    struct Trajectories
    {
        Trajectories() : header(), data() { memset(&header, 0, sizeof(header)); }
        PrdTrajectoriesHeader header;
        std::vector<Trajectory> data;
    };

public:
    class AcqCfg
    {
    public:
        AcqCfg();
        AcqCfg(size_t frameBytes, uint16_t roiCount, bool hasMetadata,
                const rgn_type& impliedRoi, const BitmapFormat& format,
                AllocatorType allocatorType = AllocatorType::Default);
        AcqCfg(size_t frameBytes, uint16_t roiCount, bool hasMetadata,
                const rgn_type& impliedRoi, const BitmapFormat& format,
                const std::vector<rgn_type>& outputBmpRois,
                AllocatorType allocatorType = AllocatorType::Default);
        AcqCfg(const AcqCfg& other);
        AcqCfg& operator=(const AcqCfg& other);
        bool operator==(const AcqCfg& other) const;
        bool operator!=(const AcqCfg& other) const;

        size_t GetFrameBytes() const;
        void SetFrameBytes(size_t frameBytes);

        uint16_t GetRoiCount() const;
        void SetRoiCount(uint16_t roiCount);

        bool HasMetadata() const;
        void SetMetadata(bool hasMetadata);

        const rgn_type& GetImpliedRoi() const;
        void SetImpliedRoi(const rgn_type& impliedRoi);

        const BitmapFormat& GetBitmapFormat() const;
        void SetBitmapFormat(const BitmapFormat& format);

        const std::vector<rgn_type>& GetOutputBmpRois() const;
        void SetOutputBmpRois(const std::vector<rgn_type>& rois);

        AllocatorType GetAllocatorType() const;
        void SetAllocatorType(AllocatorType type);

    private:
        size_t m_frameBytes{ 0 };
        uint16_t m_roiCount{ 0 };
        bool m_hasMetadata{ false };
        rgn_type m_impliedRoi{ 0, 0, 0, 0, 0, 0 };
        BitmapFormat m_format{};
        std::vector<rgn_type> m_outputBmpRois{};
        AllocatorType m_allocatorType{ AllocatorType::Default };
    };

public:
    class Info
    {
    public:
        Info();
        Info(uint32_t frameNr, uint64_t timestampBOF, uint64_t timestampEOF);
        Info(uint32_t frameNr, uint64_t timestampBOF, uint64_t timestampEOF,
                uint32_t expTime);
        Info(uint32_t frameNr, uint64_t timestampBOF, uint64_t timestampEOF,
                uint32_t expTime, float colorWbScaleRed,
                float colorWbScaleGreen, float colorWbScaleBlue);
        Info(const Info& other);
        Info& operator=(const Info& other);
        bool operator==(const Info& other) const;
        bool operator!=(const Info& other) const;

        uint32_t GetFrameNr() const;
        uint64_t GetTimestampBOF() const;
        uint64_t GetTimestampEOF() const;
        uint32_t GetReadoutTime() const;

        uint32_t GetExpTime() const;
        float GetColorWbScaleRed() const;
        float GetColorWbScaleGreen() const;
        float GetColorWbScaleBlue() const;

    private:
        uint32_t m_frameNr{ 0 };
        uint64_t m_timestampBOF{ 0 };
        uint64_t m_timestampEOF{ 0 };
        uint32_t m_readoutTime{ 0 };

        uint32_t m_expTime{ 0 };
        float m_colorWbScaleRed{ 1.0 };
        float m_colorWbScaleGreen{ 1.0 };
        float m_colorWbScaleBlue{ 1.0 };
    };

public:
    explicit Frame(const AcqCfg& acqCfg, bool deepCopy,
            std::shared_ptr<Allocator> allocator = nullptr);
    ~Frame();

    Frame() = delete;
    Frame(const Frame&) = delete;
    Frame(Frame&&) = delete;
    Frame& operator=(const Frame&) = delete;
    Frame& operator=(Frame&&) = delete;

    const Frame::AcqCfg& GetAcqCfg() const;
    bool UsesDeepCopy() const;
    std::shared_ptr<Allocator> GetAllocator() const;

    /* Stores only pointer to data without copying it.
       To copy the data itself call CopyData method. */
    void SetDataPointer(void* data);
    /* Invalidates the frame and makes a deep copy with data pointer stored by
       SetDataPointers.
       This is usually done by another thread and it is *not* responsibility
       of this class to watch that stored data pointer still points to valid
       data.
       If frame was created with deepCopy set to false, the deep copy is not
       needed but you still should call this function.
       The metadata if available has to be decoded again.
       Upon successful data copy, either shallow or deep, the frame is set to
       valid. This is the only method that can set frame to be valid. */
    bool CopyData();

    const void* GetData() const;

    bool IsValid() const;
    /* Invalidates frame, clears frame info, trajectories, metadata, etc. */
    void Invalidate();
    /* Should be used in very rare cases where you know what you're doing */
    void OverrideValidity(bool isValid);

    const Frame::Info& GetInfo() const;
    void SetInfo(const Frame::Info& frameInfo);

    /* Returns previously set list of <X, Y> coordinates in sensor area for each
       particle detected in frame data. For each particle first item in array
       are coordinates of given particle on this frame, next item is location of
       the same particle on previous frame, and so on up to max. number of
       frames configured by user.*/
    const Frame::Trajectories& GetTrajectories() const;
    void SetTrajectories(const Frame::Trajectories& trajectories);

    /* Decodes frame metadata if AcqCfg::HasMetadata is set. The method returns
       immediately if metadata has already been decoded.
       Method returns without error if frame has no metadata. */
    bool DecodeMetadata();
    /* Returns decoded metadata or null if frame has no metadata.
       DecodeMetadata should be always called before this method to ensure the
       returned value is valid. */
    const md_frame* GetMetadata() const;
    /* Returns decoded extended metadata, e.g. with m0/m2 moments for particle
       tracking. */
    const std::map<uint16_t, md_ext_item_collection>& GetExtMetadata() const;

    // Provides a Bitmap wrapper for each ROI in frame.
    // If AcqCfg::m_outputBmpRois is empty, Frame's c-tor allocates
    // AcqCfg::m_roiCount bitmaps and updates them in DecodeMetadata.
    const std::vector<std::unique_ptr<Bitmap>>& GetRoiBitmaps() const;
    // Returns bitmap regions
    const std::vector<rgn_type>& GetRoiBitmapRegions() const;
    // Returns bitmap positions
    const std::vector<Frame::Point>& GetRoiBitmapPositions() const;
    // The vector with bitmaps is allocated to max. possible number of regions.
    // This method returns how many of them are valid. Bitmap pointers beyond
    // that count are null or contain invalid data.
    size_t GetRoiBitmapValidCount() const;

    /* Copies all from other to this frame. Utilizes private Copy method.
       If deepCopy is false, this method only stores frame info and calls
       SetDataPointer method. CopyData method has to be called separately.
       Unlike the Clone method, this one does not change the value returned by
       UsesDeepCopy method. */
    bool Copy(const Frame& other, bool deepCopy = true);
    /* Returns new copy of this frame. Utilizes private Copy method. */
    std::shared_ptr<Frame> Clone(bool deepCopy = true) const;

private:
    void DoSetDataPointer(void* data);
    bool DoCopyData();
    void DoInvalidate();
    bool DoOverrideValidity(bool isValid);
    void DoSetInfo(const Frame::Info& frameInfo);
    void DoSetTrajectories(const Frame::Trajectories& trajectories);
    /* Uses DoSetDataPointer and DoCopyData methods to make the copy. It also
       copies frame info and trajectories. Metadata if available has to be
       decoded by calling DecodeMetadata method as we cannot safely do a deep copy. */
    bool DoCopy(const Frame& from, Frame& to, bool deepCopy = true) const;

private:
    const Frame::AcqCfg m_acqCfg;
    const bool m_deepCopy;
    const std::shared_ptr<Allocator> m_allocator;

    // Using shared mutex for simple implementation or reader-writer lock.
    // With C++17 change to std::shared_mutex.
    mutable std::shared_timed_mutex m_mutex{};

    void* m_data{ nullptr };
    void* m_dataSrc{ nullptr };

    bool m_isValid{ false };

    Frame::Info m_info{};
    Frame::Info m_shallowInfo{};
    Frame::Trajectories m_trajectories{};

    bool m_needsDecoding{ false };
    md_frame* m_metadata{ nullptr };
    // Returns map<roiNr, md_ext_item_collection>
    std::map<uint16_t, md_ext_item_collection> m_extMetadata{};

    std::vector<std::unique_ptr<Bitmap>> m_roiBitmaps{};
    std::vector<rgn_type> m_roiBitmapRegions{};
    std::vector<Point> m_roiBitmapPositions{};
    size_t m_roiBitmapValidCount{ 0 };

    TaskSet_CopyMemory* m_tasksMemCopy{ nullptr };
};

} // namespace

#endif
