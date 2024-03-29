/******************************************************************************/
/* Copyright (C) Teledyne Photometrics. All rights reserved.                  */
/******************************************************************************/
#pragma once
#ifndef PM_FAKE_CAMERA_H
#define PM_FAKE_CAMERA_H

/* Local */
#include "backend/Camera.h"
#include "backend/FakeCameraErrors.h"
#include "backend/RandomPixelCache.h"
#include "backend/Timer.h"

/* System */
#include <atomic>
#include <condition_variable>
#include <map>
#include <memory> // std::unique_ptr
#include <mutex>

namespace std
{
    class thread;
}

namespace pm {

template<typename T> class FakeParamBase;
class Frame;

// The class in charge of PVCAM calls
class FakeCamera final : public Camera
{
public:
    using Base = Camera;

public:
    explicit FakeCamera(unsigned int targetFps);
    virtual ~FakeCamera();

    FakeCamera() = delete;
    FakeCamera(const FakeCamera&) = delete;
    FakeCamera& operator=(const FakeCamera&) = delete;

public:
    unsigned int GetTargetFps() const;

protected:
    template<typename T> friend class FakeParamBase;
    // Const so it can be set also from const methods
    void SetError(FakeCameraErrors error) const;

public: // From Camera
    virtual bool InitLibrary() override;
    virtual bool UninitLibrary() override;
    virtual bool IsLibraryInitialized() const override
    { return s_isInitialized; }

    virtual bool GetCameraCount(int16& count) const override;
    virtual bool GetName(int16 index, std::string& name) const override;

    virtual std::string GetErrorMessage() const override;

    virtual bool Open(const std::string& name,
            CallbackEx3Fn removeCallbackHandler = nullptr,
            void* removeCallbackContext = nullptr) override;
    virtual bool Close() override;

    virtual bool SetupExp(const SettingsReader& settings) override;
    virtual bool StartExp(CallbackEx3Fn eofCallbackHandler,
            void* eofCallbackContext) override;
    virtual bool StopExp() override;
    virtual AcqStatus GetAcqStatus() const override;

    virtual bool PpReset() override;

    virtual bool Trigger() override;

    virtual bool GetLatestFrame(Frame& frame) const override;
    virtual bool GetLatestFrameIndex(size_t& index, bool suppressCamErrMsg = false)
        const override;

protected: // From Camera
    virtual bool AllocateBuffers(uint32_t frameCount, uint32_t frameBytes) override;
    virtual void DeleteBuffers() override;

private:
    void OnParamIoAddrChanged(ParamBase& param, bool allAttrsChanged);
    void OnParamIoStateChanged(ParamBase& param, bool allAttrsChanged);
    void OnParamScanModeChanged(ParamBase& param, bool allAttrsChanged);
    void OnParamScanLineDelayChanged(ParamBase& param, bool allAttrsChanged);
    void OnParamScanWidthChanged(ParamBase& param, bool allAttrsChanged);
    void OnParamGainIndexChanged(ParamBase& param, bool allAttrsChanged);
    void OnParamSpdtabIndexChanged(ParamBase& param, bool allAttrsChanged);
    void OnParamReadoutPortChanged(ParamBase& param, bool allAttrsChanged);
    void OnParamPpIndexChanged(ParamBase& param, bool allAttrsChanged);
    void OnParamPpParamIndexChanged(ParamBase& param, bool allAttrsChanged);
    void OnParamPpParamChanged(ParamBase& param, bool allAttrsChanged);
    void OnParamExpResChanged(ParamBase& param, bool allAttrsChanged);
    void OnParamExpResIndexChanged(ParamBase& param, bool allAttrsChanged);

private:
    // Calculate frame size including metadata if any
    size_t CalculateFrameBytes() const;

    uint16_t GetExtMdBytes(PL_MD_EXT_TAGS tagId) const;
    void SetExtMdData(PL_MD_EXT_TAGS tagId, uint8_t** buffer, const void* data);

    uint32_t GetRandomNumber();

    // Generate ROI "background noise" pixels
    void GenerateRoiData(void* const dstBuffer, size_t dstBytes);
    // Copy particle "background noise" pixels to right location
    void AppendParticleData(void* const dstBuffer, const rgn_type& dstRgn,
            const void* srcBuffer, const rgn_type& srcRgn);
    // Draw fake particles into given buffer
    template <typename T>
    void InjectParticlesT(T* const dstBuffer, const rgn_type& dstRgn,
            const std::vector<std::pair<uint16_t, uint16_t>>& particleCoordinates);
    void InjectParticles(void* const dstBuffer, const rgn_type& dstRgn);
    // Generate frame metadata header
    md_frame_header_v3 GenerateFrameHeader();
    // Get ROI metadata header
    md_frame_roi_header GenerateRoiHeader(uint16_t roiIndex, const rgn_type& rgn);

    // Get particle header based on coordinates
    md_frame_roi_header GenerateParticleHeader(uint16_t roiIndex,
            uint16_t centerX, uint16_t centerY);
    // Generate random particles
    void GenerateParticles(const rgn_type& rgn);
    // Move each particle in random way
    void MoveParticles(const rgn_type& rgn);
    // Generate frame data including metadata if any
    bool GenerateFrameData();

    // This is the function used to generate frames.
    void FrameGeneratorLoop(); // Routine launched by m_framegenThread

private:
    static bool s_isInitialized; // Init state is common for all cameras

private:
    const unsigned int m_targetFps;
    const double m_readoutTimeUs; // Calculated from FPS, in microseconds

    std::map<ParamBase*, uint64_t> m_paramChangeHandleMap{};

    // Mutable so it can be set also from const methods
    mutable FakeCameraErrors m_error{ FakeCameraErrors::None };

    // Indexes to internal arrays, not matching PVCAM parameters value
    int16_t m_portIndex{ 0 };
    int16_t m_speedIndex{ 0 };
    int16_t m_gainIndex{ 0 };

    uint64_t m_expTimeResPs{ 0 };

    // TODO: Write some FakeParamProxy that allows swapping parameter storage
    //       under the hood. We cannot replace Param instance because it can be
    //       stored in various lookup maps across the application.

    // Local storage for PP values so we can reuse one parameter for all indexes.
    // Size equals to cPpGroupCount=2 from .cpp
    uint32_t m_ppParam[2][PP_FEATURE_MAX][PP_MAX_PARAMETERS_PER_FEATURE];
    // Local storage for I/O states so we can reuse one parameter for all indexes.
    // Size equals to cIoAddrCount=4 from .cpp
    double m_ioState[4];

    const uns16 m_trackRoiExtMdBytes;

    // Coordinates - first X, second Y
    std::vector<std::pair<uint16_t, uint16_t>> m_particleCoordinates{};
    // Moments - first M0, second M2
    std::vector<std::pair<uint32_t, uint32_t>> m_particleMoments{};

    // Used for an outside entity to receive EOF callbacks from PVCAM
    CallbackEx3Fn m_eofCallbackHandler{ nullptr };
    // Used for an outside entity to receive EOF callbacks from PVCAM
    void* m_eofCallbackContext{ nullptr };

    Timer m_startStopTimer{};

    // Buffer of generated data used for each frame with centroids Locate mode
    std::unique_ptr<uint8_t[]> m_frameGenRoi0Buffer{ nullptr };

    // Buffer of generated data used for each frame
    std::unique_ptr<uint8_t[]> m_frameGenBuffer{ nullptr };
    /* Used for artificially constructed buffer to label where we currently are,
       this is the substitution for get_latest_frame. */
    size_t                  m_frameGenBufferPos{ 0 };
    // Used to track frame index of generated frame in whole acquisition
    size_t                  m_frameGenFrameIndex{ 0 };
    // Generated frame info
    FRAME_INFO              m_frameGenFrameInfo{};
    // Flag to tell frame generation method when it should unblock next frame
    std::atomic<bool>       m_frameGenSwTriggerFlag{ false };
    // Flag to tell frame generation method when it should stop
    std::atomic<bool>       m_frameGenStopFlag{ true };
    // Condition to wait on that can be interrupted sooner than timeout
    std::condition_variable m_frameGenCond{};
    // Actual thread object used to run FrameGenerator
    std::thread*            m_frameGenThread{ nullptr };
    // Mutex that guards all m_frameGen* variables
    mutable std::mutex      m_frameGenMutex{};

    // Lazy initialized on first access to let GUI show first
    std::unique_ptr<RandomPixelCache<uint8_t >> m_randomPixelCache8 { nullptr };
    std::unique_ptr<RandomPixelCache<uint16_t>> m_randomPixelCache16{ nullptr };
    std::unique_ptr<RandomPixelCache<uint32_t>> m_randomPixelCache32{ nullptr };
};

} // namespace pm

#endif /* PM_FAKE_CAMERA_H */
