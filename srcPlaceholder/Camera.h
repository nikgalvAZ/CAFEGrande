/******************************************************************************/
/* Copyright (C) Teledyne Photometrics. All rights reserved.                  */
/******************************************************************************/
#pragma once
#ifndef PM_CAMERA_H
#define PM_CAMERA_H

/* Local */
#include "backend/Allocator.h"
#include "backend/Frame.h"
#include "backend/OptionController.h"
#include "backend/Params.h"
#include "backend/Settings.h"

/* PVCAM */
#include "master.h"
#include "pvcam.h"

/* System */
#include <map>
#include <memory> // std::shared_ptr
#include <string>
#include <vector>

// Function used as an interface between the queue and the callback
using CallbackEx3Fn = void (PV_DECL *)(FRAME_INFO* frameInfo, void* context);

namespace pm {

// The base class for all kind of cameras
class Camera
{
public:
    struct Port
    {
        struct Speed
        {
            struct Gain
            {
                int16_t index; // PARAM_GAIN_INDEX
                std::string name; // PARAM_GAIN_NAME or empty string
                uint16_t bitDepth; // PARAM_BIT_DEPTH
                std::string label; // Handy to UI labels, e.g. in combo box
            };
            int16_t index; // PARAM_SPDTAB_INDEX
            uint16_t pixTimeNs; // PARAM_PIX_TIME
            std::vector<Gain> gains;
            std::string label; // Handy to UI labels, e.g. in combo box
        };
        ParamEnumItem item; // PARAM_READOUT_PORT (index + name)
        std::vector<Speed> speeds;
        std::string label; // Handy to UI labels, e.g. in combo box
    };

    enum class AcqStatus
    {
        Inactive = 0,
        Active,
        Failure
    };

public:
    Camera();
    virtual ~Camera();

    // Library related methods

    // Initialize camera/library
    virtual bool InitLibrary() = 0;
    // Uninitialize camera/library
    virtual bool UninitLibrary() = 0;
    // Current init state
    virtual bool IsLibraryInitialized() const = 0;

    // Get number of cameras detected
    virtual bool GetCameraCount(int16& count) const = 0;
    // Get name of the camera on given index
    virtual bool GetName(int16 index, std::string& name) const = 0;

    // Camera related methods

    // Get error message
    virtual std::string GetErrorMessage() const = 0;

    // Open camera, has to be called from derived class upon successful open
    virtual bool Open(const std::string& name,
            CallbackEx3Fn removeCallbackHandler = nullptr,
            void* removeCallbackContext = nullptr);
    // Close camera, has to be called from derived class upon successful close
    virtual bool Close();
    // Current open state
    virtual bool IsOpen() const
    { return m_isOpen; }

    // Return camera handle, AKA PVCAM hcam
    virtual int16 GetHandle() const
    { return m_hCam; }

    // Add CLI options for writable parameters.
    // Options are added the same order as controller will process them later.
    // If fixUserInput is set CLI values, that are usually valid but are e.g.
    // not supported by current camera, are corrected.
    // Otherwise camera-default values are kept as set during open.
    virtual bool AddCliOptions(OptionController& controller, bool fixUserInput);

    // Update read-only settings and correct other values that are usually valid
    // but are e.g. not supported by this camera. The correction occurs in case
    // user overrides values by custom ones. Otherwise camera-default values
    // are used.
    virtual bool ReviseSettings(Settings& settings,
            const OptionController& optionController, bool fixUserInput);

    // Return settings set via SetupExp
    const SettingsReader& GetSettings() const
    { return m_settings; }

    // Return supported speeds that are obtained on camera open
    const std::vector<Port>& GetSpeedTable() const
    { return m_ports; }

    // Setup acquisition
    virtual bool SetupExp(const SettingsReader& settings);
    // Start acquisition
    virtual bool StartExp(CallbackEx3Fn eofCallbackHandler, void* eofCallbackContext) = 0;
    // Stop acquisition
    virtual bool StopExp() = 0;
    // Current acquisition state
    virtual bool IsImaging() const
    { return m_isImaging; }
    // Get acquisition status
    virtual AcqStatus GetAcqStatus() const = 0;

    // Reset post-processing features
    virtual bool PpReset() = 0;

    // Issue software trigger
    virtual bool Trigger() = 0;

    // Used to generically access camera parameters through PVCAM API
    virtual const Params& GetParams() const;

    // Get the latest frame and deliver it to the frame being pushed into the queue.
    // It has to call UpdateFrameIndexMap to keep GetFrameIndex work.
    // It has to be called from within EOF callback handler for each frame as
    // there is no other way how to detect the data in raw buffer has changed.
    // The given frame as well as internal frame around raw buffer are invalidated.
    virtual bool GetLatestFrame(Frame& frame) const = 0;
    // Does exactly the same as GetLatestFrame but returns frame index only.
    // It's useful mainly at the acquisition end to update UI.
    virtual bool GetLatestFrameIndex(size_t& index, bool suppressCamErrMsg = false) const = 0;
    // Get the frame at index or null (should be used for displaying only)
    virtual std::shared_ptr<Frame> GetFrameAt(size_t index) const;
    // Get index of the frame from circular buffer
    virtual bool GetFrameIndex(const Frame& frame, size_t& index) const;

    // Get current acquisition configuration for frame
    virtual Frame::AcqCfg GetFrameAcqCfg() const
    { return m_frameAcqCfg; }
    // Get current allocator
    virtual std::shared_ptr<pm::Allocator> GetAllocator() const
    { return m_allocator; }

    // Returns exposure time for given frame based on configuration (VTM, etc.)
    virtual uint32_t GetFrameExpTime(uint32_t frameNr) const;

protected:
    // Updates m_framesMap[oldFrameNr] to m_frame[index].
    // It is const with mutable map to allow usage from GetLatestFrame method.
    virtual void UpdateFrameIndexMap(uint32_t oldFrameNr, size_t index) const;
    // Collects supported speeds
    virtual void BuildSpeedTable();
    // Allocate internal buffers
    virtual bool AllocateBuffers(uns32 frameCount, uns32 frameBytes);
    // Make sure the buffer is freed and the head pointer is chained at NULL
    virtual void DeleteBuffers();
    // Invoke ChangeHandlers for parameters requiring a PVCAM setup
    virtual void InvokeAfterSetupParamChangeHandlers();

private:
    bool HandleClearCycles(const std::string& value);
    bool HandleClearMode(const std::string& value);
    bool HandlePMode(const std::string& value);

    bool HandlePort(const std::string& value);
    bool HandleSpeedIndex(const std::string& value);
    bool HandleGainIndex(const std::string& value);

    bool HandleEmGain(const std::string& value);

    bool HandleMetadataEnabled(const std::string& value);
    bool HandleCentroidsEnabled(const std::string& value);
    bool HandleCentroidsRadius(const std::string& value);
    bool HandleCentroidsCount(const std::string& value);
    bool HandleCentroidsMode(const std::string& value);
    bool HandleCentroidsBgCount(const std::string& value);
    bool HandleCentroidsThreshold(const std::string& value);

    bool HandleTrigTabSignal(const std::string& value);
    bool HandleLastMuxedSignal(const std::string& value);

protected:
    int16 m_hCam{ -1 }; // Invalid handle by default
    bool m_isOpen{ false };
    bool m_isImaging{ false };
    SettingsReader m_settings{};
    std::vector<Port> m_ports{};

    std::unique_ptr<Params> m_params{ nullptr };

    bool m_fixCliOptions{ true };

    // Used for an outside entity to receive camera removal callbacks from PVCAM
    CallbackEx3Fn m_removeCallbackHandler{ nullptr };
    // Used for an outside entity to receive camera removal callbacks from PVCAM
    void* m_removeCallbackContext{ nullptr };

    // Cached parameter values since last setup
    bool m_usesMetadata{ false };
    bool m_usesCentroids{ false };
    ParamEnum::T m_centroidsMode{ PL_CENTROIDS_MODE_LOCATE };
    uint16_t m_centroidsCount{ 0 };
    uint16_t m_centroidsRadius{ 0 };
    std::vector<uint32_t> m_smartExposures{}; // Empty if n/a or disabled

    // Frame format, needed sooner than m_frameAcqCfg is set
    BitmapFormat m_bmpFormat{}; // Updated in Camera::SetupExp
    // Number of bytes in one frame in buffer, etc.
    Frame::AcqCfg m_frameAcqCfg{}; // Updated in Camera::AllocateBuffers
    // Allocator for buffers and frames
    std::shared_ptr<Allocator> m_allocator{}; // Updated in Camera::AllocateBuffers
    // Number of frames in buffer (circ/sequence)
    uns32 m_frameCount{ 0 };
    // PVCAM buffer (raw bytes)
    std::unique_ptr<uns8[]> m_buffer{ nullptr };

    std::vector<std::shared_ptr<Frame>> m_frames{};
    // Lookup map - frameNr is the key, index to m_frames vector is the value
    mutable std::map<uint32_t, size_t> m_framesMap{};
};

} // namespace

#endif
