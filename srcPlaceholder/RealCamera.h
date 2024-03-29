/******************************************************************************/
/* Copyright (C) Teledyne Photometrics. All rights reserved.                  */
/******************************************************************************/
#pragma once
#ifndef PM_REAL_CAMERA_H
#define PM_REAL_CAMERA_H

/* Local */
#include "backend/Camera.h"

/* System */
#include <atomic>
#include <future>

namespace pm {

class Frame;

// The class in charge of PVCAM calls
class RealCamera final : public Camera
{
public:
    using Base = Camera;

public:
    static void PV_DECL TimeLapseCallbackHandler(FRAME_INFO* frameInfo,
            void* RealCamera_pointer);

public:
    RealCamera();
    virtual ~RealCamera();

    RealCamera(const RealCamera&) = delete;
    RealCamera& operator=(const RealCamera&) = delete;

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

private:
    // Called from callback function to handle new frame
    void HandleTimeLapseEofCallback(FRAME_INFO* frameInfo);

private:
    static bool s_isInitialized; // PVCAM init state is common for all cameras

private:
    std::atomic<unsigned int> m_timeLapseFrameCount{ 0 };
    std::future<void>         m_timeLapseFuture{};
    std::mutex                m_timeLapseMutex{};
    std::condition_variable   m_timeLapseCond{};
    std::atomic<bool>         m_timeLapseAbortFlag{ true };

    // Used for an outside entity to receive EOF callbacks from PVCAM
    CallbackEx3Fn m_eofCallbackHandler{ nullptr };
    // Used for an outside entity to receive EOF callbacks from PVCAM
    void* m_eofCallbackContext{ nullptr };

    FRAME_INFO* m_latestFrameInfo{ nullptr };
};

} // namespace pm

#endif /* PM_REAL_CAMERA_H */
