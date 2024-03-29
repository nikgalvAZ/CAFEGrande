/******************************************************************************/
/* Copyright (C) Teledyne Photometrics. All rights reserved.                  */
/******************************************************************************/
#include "backend/RealCamera.h"

/* Local */
#include "backend/exceptions/CameraException.h"
#include "backend/Frame.h"
#include "backend/Log.h"
#include "backend/PvcamRuntimeLoader.h"
#include "backend/RealParams.h"

/* System */
#include <cassert>
#include <chrono>
#include <limits>
#include <thread>

bool pm::RealCamera::s_isInitialized = false;

void PV_DECL pm::RealCamera::TimeLapseCallbackHandler(FRAME_INFO* frameInfo,
        void* RealCamera_pointer)
{
    RealCamera* cam = static_cast<RealCamera*>(RealCamera_pointer);
    cam->HandleTimeLapseEofCallback(frameInfo);
}

pm::RealCamera::RealCamera()
    : Camera()
{
    m_params = std::make_unique<RealParams>(this);
}

pm::RealCamera::~RealCamera()
{
}

bool pm::RealCamera::InitLibrary()
{
    if (s_isInitialized)
        return true;

    if (PV_OK != PVCAM->pl_pvcam_init())
    {
        Log::LogE("Failure initializing PVCAM (%s)",
                GetErrorMessage().c_str());
        return false;
    }

    uns16 version;
    if (PV_OK != PVCAM->pl_pvcam_get_ver(&version))
    {
        Log::LogE("Failure getting PVCAM version (%s)",
                GetErrorMessage().c_str());
        return false;
    }

    Log::LogI("Using PVCAM version %u.%u.%u\n",
            (version >> 8) & 0xFF,
            (version >> 4) & 0x0F,
            (version >> 0) & 0x0F);

    s_isInitialized = true;
    return true;
}

bool pm::RealCamera::UninitLibrary()
{
    if (!s_isInitialized)
        return true;

    if (PV_OK != PVCAM->pl_pvcam_uninit())
    {
        Log::LogE("Failure uninitializing PVCAM (%s)",
                GetErrorMessage().c_str());
        return false;
    }

    s_isInitialized = false;
    return true;
}

bool pm::RealCamera::GetCameraCount(int16& count) const
{
    if (PV_OK != PVCAM->pl_cam_get_total(&count))
    {
        Log::LogE("Failure getting camera count (%s)",
                GetErrorMessage().c_str());
        return false;
    }
    return true;
}

bool pm::RealCamera::GetName(int16 index, std::string& name) const
{
    name.clear();

    char camName[CAM_NAME_LEN];
    if (PV_OK != PVCAM->pl_cam_get_name(index, camName))
    {
        Log::LogE("Failed to get name for camera at index %d (%s)", index,
                GetErrorMessage().c_str());
        return false;
    }

    name = camName;
    return true;
}

std::string pm::RealCamera::GetErrorMessage() const
{
    std::string message;

    char errMsg[ERROR_MSG_LEN] = "\0";
    const int16 code = PVCAM->pl_error_code();
    if (PV_OK != PVCAM->pl_error_message(code, errMsg))
    {
        message = std::string("Unable to get error message for error code ")
            + std::to_string(code);
    }
    else
    {
        message = errMsg;
    }

    return message;
}

bool pm::RealCamera::Open(const std::string& name,
        CallbackEx3Fn removeCallbackHandler, void* removeCallbackContext)
{
    if (m_isOpen)
        return true;

    if (PV_OK != PVCAM->pl_create_frame_info_struct(&m_latestFrameInfo))
    {
        Log::LogE("Failure creating frame info structure (%s)",
                GetErrorMessage().c_str());
        return false;
    }

    if (PV_OK != PVCAM->pl_cam_open((char*)name.c_str(), &m_hCam, OPEN_EXCLUSIVE))
    {
        Log::LogE("Failure opening camera '%s' (%s)", name.c_str(),
                GetErrorMessage().c_str());
        m_hCam = -1;
        PVCAM->pl_release_frame_info_struct(m_latestFrameInfo); // Ignore errors
        m_latestFrameInfo = nullptr;
        return false;
    }

    if (!Base::Open(name, removeCallbackHandler, removeCallbackContext))
    {
        PVCAM->pl_cam_close(m_hCam); // Ignore errors
        m_hCam = -1;
        PVCAM->pl_release_frame_info_struct(m_latestFrameInfo); // Ignore errors
        m_latestFrameInfo = nullptr;
        return false;
    }

    if (m_removeCallbackHandler)
    {
        if (PV_OK != PVCAM->pl_cam_register_callback_ex3(m_hCam,
                    PL_CALLBACK_CAM_REMOVED, (void*)m_removeCallbackHandler,
                    m_removeCallbackContext))
        {
            Log::LogW("Unable to register camera removal callback (%s)",
                    GetErrorMessage().c_str());
        }
    }

    return true;
}

bool pm::RealCamera::Close()
{
    if (!m_isOpen)
        return true;

    if (m_removeCallbackHandler)
    {
        if (PV_OK != PVCAM->pl_cam_deregister_callback(m_hCam,
                    PL_CALLBACK_CAM_REMOVED))
        {
            Log::LogE("Failed to unregister camera removal callback (%s)",
                    GetErrorMessage().c_str());
            // Error ignored, need to uninitialize PVCAM anyway
            //return false;
        }
    }

    if (PV_OK != PVCAM->pl_cam_close(m_hCam))
    {
        Log::LogE("Failed to close camera, error ignored (%s)",
                GetErrorMessage().c_str());
        // Error ignored, need to uninitialize PVCAM anyway
        //return false;
    }

    if (PV_OK != PVCAM->pl_release_frame_info_struct(m_latestFrameInfo))
    {
        Log::LogE("Failure releasing frame info structure, error ignored (%s)",
                GetErrorMessage().c_str());
        // Error ignored, need to uninitialize PVCAM anyway
        //return false;
    }

    DeleteBuffers();

    m_hCam = -1;
    m_latestFrameInfo = nullptr;

    return Base::Close();
}

bool pm::RealCamera::SetupExp(const SettingsReader& settings)
{
    if (!Base::SetupExp(settings))
        return false;

    const uns32 acqFrameCount = m_settings.GetAcqFrameCount();
    const uns32 bufferFrameCount = m_settings.GetBufferFrameCount();
    const AcqMode acqMode = m_settings.GetAcqMode();

    const int32 trigMode = m_settings.GetTrigMode();
    const int32 expOutMode = m_settings.GetExpOutMode();
    const int16 expMode = (int16)trigMode | (int16)expOutMode;

    const uns16 rgn_total = (uns16)m_settings.GetRegions().size();
    const rgn_type* rgn_array = m_settings.GetRegions().data();

    // Get exposure time, in VTM & SS mode it must not be zero
    uns32 exposure = (trigMode == VARIABLE_TIMED_MODE || !m_smartExposures.empty())
        ? 1 // The value does not matter but it must not be zero
        : m_settings.GetExposure();

    uns32 frameBytes = 0;
    uns32 bufferBytes = 0;

    switch (acqMode)
    {
    case AcqMode::SnapSequence:
        if (acqFrameCount > std::numeric_limits<uns16>::max())
        {
            Log::LogE("Too many frames in sequence (%u does not fit in 16 bits)",
                    acqFrameCount);
            return false;
        }
        if (PV_OK != PVCAM->pl_exp_setup_seq(m_hCam, (uns16)acqFrameCount,
                    rgn_total, rgn_array, expMode, exposure, &bufferBytes))
        {
            Log::LogE("Failed to setup sequence acquisition (%s)",
                    GetErrorMessage().c_str());
            return false;
        }
        frameBytes = bufferBytes / acqFrameCount;
        break;

    case AcqMode::SnapCircBuffer:
    case AcqMode::LiveCircBuffer:
        if (PV_OK != PVCAM->pl_exp_setup_cont(m_hCam, rgn_total, rgn_array,
                    expMode, exposure, &frameBytes, CIRC_OVERWRITE))
        {
            Log::LogE("Failed to setup continuous acquisition (%s)",
                    GetErrorMessage().c_str());
            return false;
        }
        break;

    case AcqMode::SnapTimeLapse:
    case AcqMode::LiveTimeLapse:
        if (!m_smartExposures.empty())
        {
            // In time-lapse mode we have to simulate Smart Streaming behavior
            exposure = m_smartExposures.at(0);
            // SS is disabled here and re-enabled in StopExp
            auto paramSsEn = m_params->Get<PARAM_SMART_STREAM_MODE_ENABLED>();
            paramSsEn->SetCur(false);
        }
        if (PV_OK != PVCAM->pl_exp_setup_seq(m_hCam, 1, rgn_total, rgn_array,
                    expMode, exposure, &frameBytes))
        {
            Log::LogE("Failed to setup time-lapse acquisition (%s)",
                    GetErrorMessage().c_str());
            return false;
        }
        // This mode uses single frame acq. but we re-use all frames in our buffer
        break;
    }

    if (!AllocateBuffers(bufferFrameCount, frameBytes))
        return false;

    m_framesMap.clear();
    for (auto& frame : m_frames)
    {
        frame->Invalidate();
    }

    m_timeLapseFrameCount = 0;

    InvokeAfterSetupParamChangeHandlers();
    return true;
}

bool pm::RealCamera::StartExp(CallbackEx3Fn eofCallbackHandler,
        void* eofCallbackContext)
{
    if (!eofCallbackHandler || !eofCallbackContext)
        return false;

    m_eofCallbackHandler = eofCallbackHandler;
    m_eofCallbackContext = eofCallbackContext;

    const AcqMode acqMode = m_settings.GetAcqMode();

    if (acqMode == AcqMode::SnapTimeLapse || acqMode == AcqMode::LiveTimeLapse)
    {
        /* Register time lapse callback only at the beginning, that might
           increase performance a bit. */
        if (m_timeLapseFrameCount == 0)
        {
            if (PV_OK != PVCAM->pl_cam_register_callback_ex3(m_hCam, PL_CALLBACK_EOF,
                        (void*)&RealCamera::TimeLapseCallbackHandler, this))
            {
                Log::LogE("Failed to register EOF callback for time-lapse mode (%s)",
                        GetErrorMessage().c_str());
                return false;
            }

            m_timeLapseAbortFlag = false;
        }
    }
    else
    {
        if (PV_OK != PVCAM->pl_cam_register_callback_ex3(m_hCam, PL_CALLBACK_EOF,
                    (void*)m_eofCallbackHandler, m_eofCallbackContext))
        {
            Log::LogE("Failed to register EOF callback (%s)",
                    GetErrorMessage().c_str());
            return false;
        }
    }

    const auto frameBytes = m_frameAcqCfg.GetFrameBytes();

    // Tell the camera to start
    bool keepGoing = false;
    switch (acqMode)
    {
    case AcqMode::SnapCircBuffer:
    case AcqMode::LiveCircBuffer:
        keepGoing = (PV_OK == PVCAM->pl_exp_start_cont(m_hCam, m_buffer.get(),
                m_frameCount * (uns32)frameBytes));
        break;
    case AcqMode::SnapSequence:
        keepGoing = (PV_OK == PVCAM->pl_exp_start_seq(m_hCam, m_buffer.get()));
        break;
    case AcqMode::SnapTimeLapse:
    case AcqMode::LiveTimeLapse:
        int32 trigMode = m_settings.GetTrigMode();
        uns32 exposure = m_settings.GetExposure();
        bool needsNewSetup = false;

        // Emulate trigger-first mode in time-lapse mode
        if (m_timeLapseFrameCount > 0)
        {
            // Setup for changing trigger-first to internal/SW mode is needed
            // only after first frame is captured. Then setup remains unchanged,
            // unless we emulate Smart Streaming...
            if (m_timeLapseFrameCount == 1)
            {
                if (trigMode == TRIGGER_FIRST_MODE)
                {
                    trigMode = TIMED_MODE;
                    needsNewSetup = true;
                }
                else if (trigMode == EXT_TRIG_TRIG_FIRST)
                {
                    trigMode = EXT_TRIG_INTERNAL;
                    needsNewSetup = true;
                }
                else if (trigMode == EXT_TRIG_SOFTWARE_FIRST)
                {
                    trigMode = EXT_TRIG_INTERNAL;
                    needsNewSetup = true;
                }
            }

            // Update exposure time in emulated Smart Streaming mode.
            // First frame is setup in SetupExp, the others here
            if (!m_smartExposures.empty())
            {
                const uint32_t ssExpIndex =
                    m_timeLapseFrameCount % m_smartExposures.size();
                exposure = m_smartExposures.at(ssExpIndex);
                needsNewSetup = true;
            }
        }

        if (needsNewSetup)
        {
            const int32 expOutMode = m_settings.GetExpOutMode();
            const int16 expMode = (int16)trigMode | (int16)expOutMode;

            const uns16 rgn_total = (uns16)m_settings.GetRegions().size();
            const rgn_type* rgn_array = m_settings.GetRegions().data();

            uns32 newFrameBytes = 0;
            if (PV_OK != PVCAM->pl_exp_setup_seq(m_hCam, 1, rgn_total,
                        rgn_array, expMode, exposure, &newFrameBytes))
            {
                Log::LogE("Failed to setup time-lapse acquisition (%s)",
                        GetErrorMessage().c_str());
                return false;
            }
            assert(frameBytes == newFrameBytes);
        }

        if (trigMode == VARIABLE_TIMED_MODE)
        {
            // Update exposure time in VTM mode after setup and before start
            const auto& vtmExposures = m_settings.GetVtmExposures();
            const uns32 vtmExpIndex = m_timeLapseFrameCount % vtmExposures.size();
            const uns16 expTime = vtmExposures.at(vtmExpIndex);
            if (PV_OK != PVCAM->pl_set_param(m_hCam, PARAM_EXP_TIME, (void*)&expTime))
            {
                Log::LogE("Failed to set new VTM exposure to %u (%s)", expTime,
                        GetErrorMessage().c_str());
                return false;
            }
        }

        // Re-use internal buffer for buffering when sequence has one frame only
        const uns32 frameIndex =
            m_timeLapseFrameCount % m_settings.GetBufferFrameCount();
        void* frameBuffer = &m_buffer[frameBytes * frameIndex];
        keepGoing = (PV_OK == PVCAM->pl_exp_start_seq(m_hCam, frameBuffer));
        break;
    }
    if (!keepGoing)
    {
        Log::LogE("Failed to start the acquisition (%s)",
                GetErrorMessage().c_str());
        return false;
    }

    m_isImaging = true;

    return true;
}

bool pm::RealCamera::StopExp()
{
    bool ok = true;

    if (!m_isImaging)
        return true;

    // Unconditionally stop the acquisition

    const AcqMode acqMode = m_settings.GetAcqMode();
    const bool isTimeLapse =
        acqMode == AcqMode::SnapTimeLapse || acqMode == AcqMode::LiveTimeLapse;

    if (isTimeLapse)
    {
        {
            std::unique_lock<std::mutex> lock(m_timeLapseMutex);
            m_timeLapseAbortFlag = true;
        }
        m_timeLapseCond.notify_one();

        if (m_timeLapseFuture.valid())
        {
            m_timeLapseFuture.get(); // Calls wait() and changes valid() to false
        }
    }

    if (PV_OK != PVCAM->pl_exp_abort(m_hCam, CCS_HALT))
    {
        Log::LogE("Failed to abort acquisition, error ignored (%s)",
                GetErrorMessage().c_str());
        // Error ignored, need to abort as much as possible
        //return false;
        ok = false;
    }
    if (PV_OK != PVCAM->pl_exp_finish_seq(m_hCam, m_buffer.get(), 0))
    {
        Log::LogE("Failed to finish sequence, error ignored (%s)",
                GetErrorMessage().c_str());
        // Error ignored, need to abort as much as possible
        //return false;
        ok = false;
    }

    m_isImaging = false;

    // Do not deregister callbacks before pl_exp_abort, abort could freeze then
    if (PV_OK != PVCAM->pl_cam_deregister_callback(m_hCam, PL_CALLBACK_EOF))
    {
        Log::LogE("Failed to deregister EOF callback, error ignored (%s)",
                GetErrorMessage().c_str());
        // Error ignored, need to abort as much as possible
        //return false;
        ok = false;
    }

    m_eofCallbackHandler = nullptr;
    m_eofCallbackContext = nullptr;

    if (isTimeLapse)
    {
        // Re-enable Smart Streaming if it was emulated
        if (!m_smartExposures.empty())
        {
            auto paramSsEn = m_params->Get<PARAM_SMART_STREAM_MODE_ENABLED>();
            paramSsEn->SetCur(true);
        }
        // TODO: Restore m_settings.GetTrigMode() via pl_exp_setup if changed
    }

    return ok;
}

pm::Camera::AcqStatus pm::RealCamera::GetAcqStatus() const
{
    if (!m_isImaging)
        return Camera::AcqStatus::Inactive;

    const AcqMode acqMode = m_settings.GetAcqMode();
    const bool isLive =
        acqMode == AcqMode::SnapCircBuffer || acqMode == AcqMode::LiveCircBuffer;
    const bool isTimeLapse =
        acqMode == AcqMode::SnapTimeLapse || acqMode == AcqMode::LiveTimeLapse;

    int16 status;
    uns32 bytes_arrived;
    uns32 buffer_cnt;

    const rs_bool res = (isLive)
            ? PVCAM->pl_exp_check_cont_status(m_hCam, &status, &bytes_arrived, &buffer_cnt)
            : PVCAM->pl_exp_check_status(m_hCam, &status, &bytes_arrived);

    if (res == PV_FAIL)
        return Camera::AcqStatus::Failure;

    Camera::AcqStatus acqStatus;

    switch (status)
    {
    case READOUT_NOT_ACTIVE:
        acqStatus = Camera::AcqStatus::Inactive;
        break;
    case EXPOSURE_IN_PROGRESS:
    case READOUT_IN_PROGRESS:
        acqStatus = Camera::AcqStatus::Active;
        break;
    case FRAME_AVAILABLE: // READOUT_COMPLETE
        acqStatus = (isLive)
            ? Camera::AcqStatus::Active // FRAME_AVAILABLE
            : Camera::AcqStatus::Inactive; // READOUT_COMPLETE
        break;
    case READOUT_FAILED:
    default:
        acqStatus = Camera::AcqStatus::Failure;
        break;
    }

    // Override status for active time-lapse loop
    if (acqStatus == Camera::AcqStatus::Inactive
            && isTimeLapse && m_timeLapseFuture.valid())
        acqStatus = Camera::AcqStatus::Active;

    return acqStatus;
}

bool pm::RealCamera::PpReset()
{
    if (PV_OK != PVCAM->pl_pp_reset(m_hCam))
    {
        Log::LogE("Failure resetting PP features (%s)", GetErrorMessage().c_str());
        return false;
    }

    return true;
}

bool pm::RealCamera::Trigger()
{
    if (!PVCAM->pl_exp_trigger)
    {
        Log::LogE("Failure sending software trigger, PVCAM library is too old");
        return false;
    }

    uns32 flags = 0;
    const uns32 value = 0;
    if (PV_OK != PVCAM->pl_exp_trigger(m_hCam, &flags, value))
    {
        Log::LogE("Failure sending software trigger (%s)", GetErrorMessage().c_str());
        return false;
    }
    if (flags != PL_SW_TRIG_STATUS_TRIGGERED)
    {
        Log::LogE("Camera didn't accept the trigger");
        return false;
    }

    return true;
}

bool pm::RealCamera::GetLatestFrame(Frame& frame) const
{
    size_t index;
    if (!GetLatestFrameIndex(index))
        return false;

    frame.Invalidate();
    return frame.Copy(*m_frames[index], false);
}

bool pm::RealCamera::GetLatestFrameIndex(size_t& index, bool suppressCamErrMsg) const
{
    // Set to an error state before PVCAM tries to reset pointer to valid frame location
    void* data = nullptr;

    // Get the latest frame
    if (PV_OK != PVCAM->pl_exp_get_latest_frame_ex(m_hCam, &data, m_latestFrameInfo))
    {
        if (!suppressCamErrMsg)
        {
            Log::LogE("Failed to get latest frame from PVCAM (%s)",
                    GetErrorMessage().c_str());
        }
        return false;
    }

    if (!data)
    {
        Log::LogE("Invalid latest frame pointer");
        return false;
    }

    // Fix the frame number which is always 1 in time lapse mode
    const AcqMode acqMode = m_settings.GetAcqMode();
    if (acqMode == AcqMode::SnapTimeLapse || acqMode == AcqMode::LiveTimeLapse)
    {
        m_latestFrameInfo->FrameNr = (int32)m_timeLapseFrameCount;
    }

    const size_t frameBytes = m_frameAcqCfg.GetFrameBytes();
    if (frameBytes == 0)
    {
        Log::LogE("Invalid acquisition configuration");
        return false;
    }
    const size_t offset = static_cast<uint8_t*>(data) - m_buffer.get();
    const size_t idx = offset / frameBytes;
    if (idx * frameBytes != offset)
    {
        Log::LogE("Invalid frame data offset");
        return false;
    }

    if (m_frames[idx]->GetData() != data)
    {
        Log::LogE("Frame data address does not match");
        return false;
    }
    index = idx;

    m_frames[index]->Invalidate(); // Does some cleanup
    m_frames[index]->OverrideValidity(true);

    const uint32_t oldFrameNr = m_frames[index]->GetInfo().GetFrameNr();
    const Frame::Info fi(
            (uint32_t)m_latestFrameInfo->FrameNr,
            (uint64_t)m_latestFrameInfo->TimeStampBOF,
            (uint64_t)m_latestFrameInfo->TimeStamp,
            GetFrameExpTime((uint32_t)m_latestFrameInfo->FrameNr),
            m_settings.GetColorWbScaleRed(),
            m_settings.GetColorWbScaleGreen(),
            m_settings.GetColorWbScaleBlue());
    m_frames[index]->SetInfo(fi);
    UpdateFrameIndexMap(oldFrameNr, index);

    return true;
}

void pm::RealCamera::HandleTimeLapseEofCallback(FRAME_INFO* frameInfo)
{
    m_timeLapseFrameCount++;

    // Fix the frame number which is always 1 in time lapse mode
    frameInfo->FrameNr = (int32)m_timeLapseFrameCount;

    // Call registered callback
    m_eofCallbackHandler(frameInfo, m_eofCallbackContext);

    // Do not start acquisition for next frame if done
    if (m_timeLapseFrameCount >= m_settings.GetAcqFrameCount()
            && m_settings.GetAcqMode() == AcqMode::SnapTimeLapse)
        return;

    m_timeLapseFuture = std::async(std::launch::async, [this]() -> void {
        // Backup callback data, StopExp will clear these members
        CallbackEx3Fn eofCallbackHandler = m_eofCallbackHandler;
        void* eofCallbackContext = m_eofCallbackContext;

        // This can happen on high FPS when user aborts the acquisition
        if (!eofCallbackHandler || !eofCallbackContext)
            return;

        /* No need to stop explicitly, acquisition has already finished and
           we don't want to release allocated buffer. */
        //StopExp();
        if (PV_OK != PVCAM->pl_exp_finish_seq(m_hCam, m_buffer.get(), 0))
        {
            Log::LogE("Failed to finish sequence, error ignored (%s)",
                    GetErrorMessage().c_str());
            /* There is no direct way how to let Acquisition know about error.
               Call the callback handler with null pointer again. */
            eofCallbackHandler(nullptr, eofCallbackContext);
            return;
        }

        // Put delay between frames if configured
        const auto timeLapseDelayMs = m_settings.GetTimeLapseDelay();
        if (timeLapseDelayMs > 0)
        {
            std::unique_lock<std::mutex> lock(m_timeLapseMutex);
            m_timeLapseCond.wait_for(lock, std::chrono::milliseconds(timeLapseDelayMs),
                    [this]() { return !!m_timeLapseAbortFlag; });
        }
        if (m_timeLapseAbortFlag)
            return;

        if (!StartExp(eofCallbackHandler, eofCallbackContext))
        {
            /* There is no direct way how to let Acquisition know about error.
               Call the callback handler with null pointer again. */
            eofCallbackHandler(nullptr, eofCallbackContext);
        }
    });
}
