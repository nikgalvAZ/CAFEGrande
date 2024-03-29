/******************************************************************************/
/* Copyright (C) Teledyne Photometrics. All rights reserved.                  */
/******************************************************************************/
#include "backend/Acquisition.h"

/* Local */
#include "backend/AllocatorFactory.h"
#include "backend/Camera.h"
#include "backend/ColorRuntimeLoader.h"
#include "backend/ColorUtils.h"
#include "backend/FakeCamera.h"
#include "backend/Log.h"
#include "backend/ParticleLinker.h"
#include "backend/PrdFileSave.h"
#include "backend/PrdFileUtils.h"
#include "backend/TiffFileSave.h"
#include "backend/TrackRuntimeLoader.h"
#include "backend/Utils.h"

/* System */
#include <cmath>
#include <cstring>
#include <fstream>
#include <iomanip>
#include <limits>
#include <sstream>
#include <string>
#include <thread>
#include <ctime>
#include <PVCamTest/MainDlg.h>
//#include <PVCamTest/MainDlg.h>

//CHANGE
// May need this header file for this
#include <PVCamTest/PVCamTest_Ui.h>

// TODO: Remove completely after testing
//#define PM_PRINT_WRITE_STATS
#ifdef PM_PRINT_WRITE_STATS
    size_t sWriteCount{ 0 };
    pm::Timer sWriteTimer{};
    double sWriteTimeSec{ 0.0 };
#endif

pm::Acquisition::Acquisition(std::shared_ptr<Camera> camera)
    : m_camera(camera)
{
    m_tiffHelper.frameProc = &m_tiffFrameProc;
}

pm::Acquisition::~Acquisition()
{
    RequestAbort();
    WaitForStop();

    ColorUtils::AssignContexts(&m_tiffHelper.colorCtx, nullptr);
    delete m_tiffHelper.fullBmp;
}

bool pm::Acquisition::Start(std::shared_ptr<FpsLimiter> fpsLimiter,
        double tiffFillValue, const ph_color_context* tiffColorCtx)
{
    if (!m_camera)
        return false;

    if (IsRunning())
        return true;

    m_fpsLimiter = fpsLimiter;

    m_tiffHelper.fillValue = tiffFillValue;
    const bool applyColorCtx = tiffColorCtx
        && !ColorUtils::CompareContexts(m_tiffHelper.colorCtx, tiffColorCtx);
    if (!ColorUtils::AssignContexts(&m_tiffHelper.colorCtx, tiffColorCtx))
        return false;
    if (m_tiffHelper.colorCtx && applyColorCtx)
    {
        if (PH_COLOR_ERROR_NONE
                != PH_COLOR->context_apply_changes(m_tiffHelper.colorCtx))
        {
            ColorUtils::LogError("Failure applying color helper context changes");
            return false;
        }
    }

    m_expTimeRes =
        m_camera->GetParams().Get<PARAM_EXP_RES_INDEX>()->IsAvail()
        ? m_camera->GetParams().Get<PARAM_EXP_RES_INDEX>()->GetCur()
        : static_cast<uint32_t>(EXP_RES_ONE_MILLISEC);

    /* The option below is used for testing purposes, but also for
        demonstration in terms of what places in the code would need to be
        altered to NOT save frames to disk */
    if (!ConfigureStorage())
        return false;

    const auto framePoolOps = FramePool::Ops::Shrink/* | FramePool::Ops::Prefetch*/;
    if (!PreallocateUnusedFrames(framePoolOps))
        return false;

    const bool centroidsCapable =
        m_camera->GetParams().Get<PARAM_CENTROIDS_ENABLED>()->IsAvail();
    const bool centroidsEnabled = centroidsCapable
        && m_camera->GetParams().Get<PARAM_CENTROIDS_ENABLED>()->GetCur();
    const bool centroidsCountCapable =
        m_camera->GetParams().Get<PARAM_CENTROIDS_COUNT>()->IsAvail();
    const bool centroidsRadiusCapable =
        m_camera->GetParams().Get<PARAM_CENTROIDS_RADIUS>()->IsAvail();
    const bool centroidsModeCapable =
        m_camera->GetParams().Get<PARAM_CENTROIDS_MODE>()->IsAvail();
    // Cache the tracking functionality status
    m_trackEnabled = PH_TRACK && centroidsEnabled && centroidsCountCapable
        && centroidsRadiusCapable && centroidsModeCapable
        && m_camera->GetParams().Get<PARAM_CENTROIDS_MODE>()->GetCur()
            == PL_CENTROIDS_MODE_TRACK;

    if (m_trackEnabled)
    {
        m_centroidsRadius =
            m_camera->GetParams().Get<PARAM_CENTROIDS_RADIUS>()->GetCur();

        const uint16_t maxFramesToLink = m_camera->GetSettings().GetTrackLinkFrames();
        const uint16_t maxDistPerFrame = m_camera->GetSettings().GetTrackMaxDistance();
        const uint16_t maxParticles =
            m_camera->GetParams().Get<PARAM_CENTROIDS_COUNT>()->GetCur();
        const bool useCpuOnly = m_camera->GetSettings().GetTrackCpuOnly();
        const int32_t trackErr =
            PH_TRACK->init(&m_trackContext, maxFramesToLink, maxDistPerFrame,
                    useCpuOnly, maxParticles, &m_trackMaxParticles);
        if (trackErr != PH_TRACK_ERROR_NONE)
        {
            char msg[PH_TRACK_MAX_ERROR_LEN] = "Unknown error";
            uint32_t size = PH_TRACK_MAX_ERROR_LEN;
            PH_TRACK->get_last_error_message(msg, &size);
            Log::LogE("Failed to initialize tracking context (%s)", msg);
            return false;
        }

        m_trackParticles = new(std::nothrow) ph_track_particle[m_trackMaxParticles];
        if (!m_trackParticles)
        {
            PH_TRACK->uninit(&m_trackContext);
            m_trackContext = nullptr;
            return false;
        }

        const uint32_t historyDepth =
            (uint32_t)m_camera->GetSettings().GetTrackTrajectoryDuration();
        m_trackLinker = new(std::nothrow) ParticleLinker(maxParticles, historyDepth);
        if (!m_trackLinker)
        {
            delete [] m_trackParticles;
            m_trackParticles = nullptr;
            PH_TRACK->uninit(&m_trackContext);
            m_trackContext = nullptr;
            return false;
        }
    }

    m_acqThreadReadyFlag = false;
    m_acqThreadAbortFlag = false;
    m_acqThreadDoneFlag = false;
    m_diskThreadReadyFlag = false;
    m_diskThreadAbortFlag = false;
    m_diskThreadDoneFlag = false;
    m_updateThreadReadyFlag = false;

    /* Start all threads but acquisition first to reduce the overall system
       load after starting the acquisition */
    m_diskThread =
        new(std::nothrow) std::thread(&Acquisition::DiskThreadLoop, this);
    if (m_diskThread)
    {
        {
            std::unique_lock<std::mutex> lock(m_diskThreadReadyMutex);
            if (!m_diskThreadReadyFlag)
            {
                m_diskThreadReadyCond.wait(lock, [this]() {
                    return !!m_diskThreadReadyFlag;
                });
            }
        }

        m_updateThread =
            new(std::nothrow) std::thread(&Acquisition::UpdateThreadLoop, this);
        if (m_updateThread)
        {
            {
                std::unique_lock<std::mutex> lock(m_updateThreadReadyMutex);
                if (!m_updateThreadReadyFlag)
                {
                    m_updateThreadReadyCond.wait(lock, [this]() {
                        return !!m_updateThreadReadyFlag;
                    });
                }
            }

            m_acqThread =
                new(std::nothrow) std::thread(&Acquisition::AcqThreadLoop, this);
            if (m_acqThread)
            {
                std::unique_lock<std::mutex> lock(m_acqThreadReadyMutex);
                if (!m_acqThreadReadyFlag)
                {
                    m_acqThreadReadyCond.wait(lock, [this]() {
                        return !!m_acqThreadReadyFlag;
                    });
                }
            }
        }

        // Acq thread could fail to start the acquisition on Camera class,
        // RequestAbort was already called and abort flag set, but that's OK
        if (!m_updateThread || !m_acqThread || m_acqThreadAbortFlag)
        {
            RequestAbort();
            WaitForStop(); // Returns true - aborted
        }
    }

    return IsRunning();
}

bool pm::Acquisition::IsRunning() const
{
    return (m_acqThread || m_diskThread || m_updateThread);
}

void pm::Acquisition::RequestAbort(bool abortBufferedFramesProcessing)
{
    m_acqThreadAbortFlag = true;
    if (m_acqThread)
    {
        // Wake acq waiter
        m_toBeProcessedFramesCond.notify_one();
    }
    else
    {
        m_acqThreadDoneFlag = true;
    }

    if (abortBufferedFramesProcessing)
    {
        m_diskThreadAbortFlag = true;
        if (m_diskThread)
        {
            // Wake disk waiter
            m_toBeSavedFramesCond.notify_one();
        }
        else
        {
            m_diskThreadDoneFlag = true;

            // Wake update thread
            m_updateThreadCond.notify_one();
        }
    }
}

bool pm::Acquisition::WaitForStop(bool printStats)
{
    const bool printEndMessage = m_acqThread && m_diskThread && m_updateThread;

    if (m_acqThread)
    {
        if (m_acqThread->joinable())
            m_acqThread->join();
        delete m_acqThread;
        m_acqThread = nullptr;
    }
    if (m_diskThread)
    {
        if (m_diskThread->joinable())
            m_diskThread->join();
        delete m_diskThread;
        m_diskThread = nullptr;
    }
    if (m_updateThread)
    {
        if (m_updateThread->joinable())
            m_updateThread->join();
        delete m_updateThread;
        m_updateThread = nullptr;
    }

    if (m_trackContext != PH_TRACK_CONTEXT_INVALID)
    {
        PH_TRACK->uninit(&m_trackContext);
        m_trackContext = PH_TRACK_CONTEXT_INVALID;
    }
    delete [] m_trackParticles;
    m_trackParticles = nullptr;
    delete m_trackLinker;
    m_trackLinker = nullptr;

    if (printStats)
    {
        PrintAcqThreadStats();
        PrintDiskThreadStats();
    }

    const bool wasAborted = m_acqThreadAbortFlag || m_diskThreadAbortFlag;

    if (printEndMessage)
    {
        if (wasAborted)
            Log::LogI("Acquisition stopped\n");
        else
            Log::LogI("Acquisition finished\n");
    }

    // After full stop release most of the frames to free RAM. It is done
    // anyway at next acq start. Frame cfg is unchanged so it cannot fail.
    const auto framePoolOps = FramePool::Ops::Shrink;
    PreallocateUnusedFrames(framePoolOps);

    return wasAborted;
}

const pm::AcquisitionStats& pm::Acquisition::GetAcqStats() const
{
    return m_toBeProcessedFramesStats;
}

const pm::AcquisitionStats& pm::Acquisition::GetDiskStats() const
{
    return m_toBeSavedFramesStats;
}

void PV_DECL pm::Acquisition::EofCallback(FRAME_INFO* frameInfo,
        void* Acquisition_pointer)
{
    Acquisition* thiz = static_cast<Acquisition*>(Acquisition_pointer);
    if (!frameInfo)
    {
        thiz->RequestAbort();
    }
    if (!thiz->HandleEofCallback(frameInfo))
    {
        thiz->RequestAbort(false); // Let queued frames to be processed
    }
}

bool pm::Acquisition::HandleEofCallback(FRAME_INFO* frameInfo)
{
    if (m_acqThreadAbortFlag)
        return true; // Return value doesn't matter, abort is already in progress

    auto CheckLostFrames = [&](uint32_t frameNr) {
        if (frameNr > m_lastFrameNumberInCallback + 1)
        {
            m_toBeProcessedFramesStats.ReportFrameLost(
                    frameNr - m_lastFrameNumberInCallback - 1);

            // Log all the frame numbers we missed
            for (uint32_t nr = m_lastFrameNumberInCallback + 1; nr < frameNr; nr++)
            {
                m_uncaughtFrames.AddItem(nr);
            }
        }
        m_lastFrameNumberInCallback = frameNr;
    };

    const uint32_t cbFrameNr = frameInfo->FrameNr;

    // Check to make sure we didn't skip any frame
    CheckLostFrames(cbFrameNr);

    std::shared_ptr<Frame> frame = m_unusedFramesPool.TakeFrame();
    if (!frame)
    {
        // No RAM for new frame, this should happen rarely as we reuse frames
        m_toBeProcessedFramesStats.ReportFrameLost();
        m_uncaughtFrames.AddItem(cbFrameNr);
        return false;
    }

    if (!m_camera->GetLatestFrame(*frame))
    {
        // Abort, could happen e.g. if frame number is 0
        m_toBeProcessedFramesStats.ReportFrameLost();
        m_uncaughtFrames.AddItem(cbFrameNr);
        return false;
    }

    // Put frame to queue for processing
    {
        std::lock_guard<std::mutex> lock(m_toBeProcessedFramesMutex);

        if (m_toBeProcessedFramesStats.GetQueueSize()
                < m_toBeProcessedFramesStats.GetQueueCapacity())
        {
            m_toBeProcessedFrames.push(frame);
            m_toBeProcessedFramesStats.SetQueueSize(m_toBeProcessedFrames.size());
        }
        else
        {
            // No RAM for frame processing

            // frameNr from GetLatestFrame could be newer than in frameInfo
            // passed to callback function
            const uint32_t frameNr = frame->GetInfo().GetFrameNr();
            if (cbFrameNr < frameNr)
            {
                CheckLostFrames(frameNr);
            }
        }
    }
    // Notify acq thread about new captured frame
    m_toBeProcessedFramesCond.notify_one();

    return true;
}

bool pm::Acquisition::HandleNewFrame(std::shared_ptr<Frame> frame)
{
    // Do deep copy
    if (!frame->CopyData())
        return false;

    const uint32_t frameNr = frame->GetInfo().GetFrameNr();

    if (frameNr <= m_lastFrameNumberInHandling)
    {
        m_outOfOrderFrameCount++;

        Log::LogE("Frame number out of order: %u, last frame number was %u, ignoring",
                frameNr, m_lastFrameNumberInHandling);

        // Drop frame for invalid frame number
        // Number out of order, cannot add it to m_unsavedFrames stats
        return true;
    }

    // Check to make sure we didn't skip a frame
    const uint32_t lostFrameCount = frameNr - m_lastFrameNumberInHandling - 1;
    if (lostFrameCount > 0)
    {
        m_toBeProcessedFramesStats.ReportFrameLost(lostFrameCount);

        // Log all the frame numbers we missed
        for (uint32_t nr = m_lastFrameNumberInHandling + 1; nr < frameNr; nr++)
        {
            m_uncaughtFrames.AddItem(nr);
        }
    }
    m_lastFrameNumberInHandling = frameNr;

    m_toBeProcessedFramesStats.ReportFrameAcquired();

    // If we don't need to track particles, send frame to GUI here so displaying
    // is not slowed down by saving images.
    if (!m_trackEnabled && m_fpsLimiter)
    {
        m_fpsLimiter->InputNewFrame(frame);
    }

    {
        std::unique_lock<std::mutex> lock(m_toBeSavedFramesMutex);

        if (m_toBeSavedFramesStats.GetQueueSize()
                < m_toBeSavedFramesStats.GetQueueCapacity())
        {
            m_toBeSavedFrames.push(frame);
            m_toBeSavedFramesStats.SetQueueSize(m_toBeSavedFrames.size());
        }
        else
        {
            // Not enough RAM to queue it for saving
            m_toBeSavedFramesStats.ReportFrameLost();
            m_unsavedFrames.AddItem(frameNr);
        }
    }
    // Notify disk waiter about new queued frame
    m_toBeSavedFramesCond.notify_one();

    return true;
}

bool pm::Acquisition::TrackNewFrame(std::shared_ptr<Frame> frame)
{
    const uint32_t frameNr = frame->GetInfo().GetFrameNr();

    // If all ROIs have particle ID set (non-zero) by camera,
    // linking is not needed.
    bool isLinkingNeeded = false;

    // 1. Decode
    if (!frame->DecodeMetadata())
        return false;
    auto frameMeta = frame->GetMetadata();
    // Format is: map<roiNr, md_ext_item_collection>
    auto frameExtMeta = frame->GetExtMetadata();

    // 2. Verify extended metadata before using it
    for (uint16_t n = 0; n < frameMeta->roiCount; ++n)
    {
        const md_frame_roi& mdRoi = frameMeta->roiArray[n];

        // Do not work with background image ROI
        if (!(mdRoi.header->flags & PL_MD_ROI_FLAG_HEADER_ONLY))
            continue;

        const uint16_t roiNr = mdRoi.header->roiNr;
        md_ext_item_collection* collection = &frameExtMeta[roiNr];

        // Extract particle ID from extended metadata
        const md_ext_item* item_id = collection->map[PL_MD_EXT_TAG_PARTICLE_ID];
        if (!item_id)
        {
            // Particle ID is usually missing, we get it after linking
            isLinkingNeeded = true;
        }
        else
        {
            if (!item_id->value || !item_id->tagInfo
                || item_id->tagInfo->type != TYPE_UNS32
                || item_id->tagInfo->size != 4)
            {
                Log::LogE("Invalid particle ID in ext. metadata, frameNr %u, roiNr=%u",
                        frameNr, roiNr);
                return false;
            }

            if (*((uint32_t*)item_id->value) == 0)
            {
                // Particle ID sent by camera is invalid, we get it after linking
                isLinkingNeeded = true;
            }
        }
        // Extract M0 from extended metadata
        const md_ext_item* item_m0 = collection->map[PL_MD_EXT_TAG_PARTICLE_M0];
        if (!item_m0 || !item_m0->value || !item_m0->tagInfo
                || item_m0->tagInfo->type != TYPE_UNS32
                || item_m0->tagInfo->size != 4)
        {
            Log::LogE("Missing M0 moment in ext. metadata, frameNr %u, roiNr=%u",
                    frameNr, roiNr);
            return false;
        }
        // Extract M2 from extended metadata
        const md_ext_item* item_m2 = collection->map[PL_MD_EXT_TAG_PARTICLE_M2];
        if (!item_m2 || !item_m2->value || !item_m2->tagInfo
                || item_m2->tagInfo->type != TYPE_UNS32
                || item_m2->tagInfo->size != 4)
        {
            Log::LogE("Missing M2 moment in ext. metadata, frameNr %u, roiNr=%u",
                    frameNr, roiNr);
            return false;
        }
    }

    // 3. Link particles
    std::vector<ph_track_particle_event> events;
    // Copy to separate variable that get overwritten after linking
    uint32_t particlesCount = m_trackMaxParticles;

    if (!isLinkingNeeded)
    {
        // Camera sent valid ID yet, linking not needed

        // Just convert data to the same format as goes from track library
        for (uint16_t n = 0; n < frameMeta->roiCount; ++n)
        {
            const md_frame_roi& mdRoi = frameMeta->roiArray[n];

            // Do not work with background image ROI
            if (!(mdRoi.header->flags & PL_MD_ROI_FLAG_HEADER_ONLY))
                continue;

            const uint16_t roiNr = mdRoi.header->roiNr;

            // Extract particle ID from extended metadata
            const md_ext_item* item_id =
                frameExtMeta[roiNr].map[PL_MD_EXT_TAG_PARTICLE_ID];
            const uint32_t id = *((uint32_t*)item_id->value);

            ph_track_particle particle;
            particle.event = events[n - 1];
            particle.id = id;
            particle.lifetime = 10;
            particle.state = PH_TRACK_PARTICLE_STATE_CONTINUATION;
            m_trackParticles[n - 1] = particle;
        }

        // Update count the same way as ph_track_link_particles does
        particlesCount = frameMeta->roiCount - 1;
    }
    else
    {
        // Linking is needed

        // 3a. Prepare input data for linking
        const uint16_t radius = m_centroidsRadius;

        for (uint16_t n = 0; n < frameMeta->roiCount; ++n)
        {
            const md_frame_roi& mdRoi = frameMeta->roiArray[n];

            // Do not work with background image ROI
            if (!(mdRoi.header->flags & PL_MD_ROI_FLAG_HEADER_ONLY))
                continue;

            const uint16_t roiNr = mdRoi.header->roiNr;

            const rgn_type& rgn = mdRoi.header->roi;
            const uint16_t roiX = rgn.s1 / rgn.sbin;
            const uint16_t roiY = rgn.p1 / rgn.pbin;

            const uint16_t x = roiX + radius;
            const uint16_t y = roiY + radius;

            // Extract M0 from extended metadata
            const md_ext_item* item_m0 =
                frameExtMeta[roiNr].map[PL_MD_EXT_TAG_PARTICLE_M0];
            const uint32_t m0 = *((uint32_t*)item_m0->value);

            // Extract M2 from extended metadata
            const md_ext_item* item_m2 =
                frameExtMeta[roiNr].map[PL_MD_EXT_TAG_PARTICLE_M2];
            const uint32_t m2 = *((uint32_t*)item_m2->value);

            ph_track_particle_event event;
            event.roiNr = mdRoi.header->roiNr;
            event.center = ph_track_particle_coord{(double)x, (double)y};
            // Unsigned fixed-point real number in format Q22.0
            event.m0 = Utils::FixedPointToReal<double, uint32_t>(22, 0, m0);
            // Unsigned fixed-point real number in format Q3.19
            event.m2 = Utils::FixedPointToReal<double, uint32_t>(3, 19, m2);

            events.push_back(event);
        }

        // 3b. Link particles
        const int32_t trackErr =
            PH_TRACK->link_particles(m_trackContext,
                    events.data(), (uint32_t)events.size(),
                    m_trackParticles, &particlesCount);
        if (trackErr != PH_TRACK_ERROR_NONE)
        {
            char msg[PH_TRACK_MAX_ERROR_LEN] = "Unknown error";
            uint32_t size = PH_TRACK_MAX_ERROR_LEN;
            PH_TRACK->get_last_error_message(msg, &size);
            Log::LogE("Failed to link particles for frame nr. %u (%s)",
                    frameNr, msg);
            return false;
        }
    }

    // 4. "Convert" particles to trajectories
    m_trackLinker->AddParticles(m_trackParticles, particlesCount);

    // 5. Store them in frame
    frame->SetTrajectories(m_trackLinker->GetTrajectories());

    // 6. Update trajectories in camera's circular buffer
    size_t index;
    if (m_camera->GetFrameIndex(*frame, index))
    {
        auto camFrame = m_camera->GetFrameAt(index);
        if (camFrame)
        {
            camFrame->SetTrajectories(m_trackLinker->GetTrajectories());
        }
    }
    if (m_fpsLimiter)
    {
        m_fpsLimiter->InputNewFrame(frame);
    }

    return true;
}

void pm::Acquisition::UpdateToBeSavedFramesMax()
{
    static const size_t totalRamMB = Utils::GetTotalRamMB();
    const size_t availRamMB = Utils::GetAvailRamMB();
    /* We allow allocation of memory up to bigger value from these:
       - 90% of total RAM
       - whole available RAM reduced by 2048MB (former 1GB limit seemed to
         activate Windows swapping and caused huge performance glitches) */
    const size_t dontTouchRamMB = std::min<size_t>(totalRamMB * (100 - 90) / 100, 2048);
    const size_t maxFreeRamMB =
        (availRamMB >= dontTouchRamMB) ? availRamMB - dontTouchRamMB : 0;
    // Left shift by 20 bits "converts" megabytes to bytes
    const size_t maxFreeRamBytes = maxFreeRamMB << 20;

    const size_t frameBytes = m_camera->GetFrameAcqCfg().GetFrameBytes();
    const size_t maxNewFrameCount =
        (frameBytes == 0) ? 0 : maxFreeRamBytes / frameBytes;

    m_toBeSavedFramesStats.SetQueueCapacity(
            m_toBeSavedFramesStats.GetQueueSize() + maxNewFrameCount);
}

bool pm::Acquisition::PreallocateUnusedFrames(int framePoolOps)
{
    // Limit the queue with captured frames to half of the circular buffer size
    m_toBeProcessedFramesStats.SetQueueCapacity(
            (m_camera->GetSettings().GetBufferFrameCount() / 2) + 1);

    UpdateToBeSavedFramesMax();

    const Frame::AcqCfg frameAcqCfg = m_camera->GetFrameAcqCfg();
    const auto allocator = m_camera->GetAllocator();
    const size_t frameCount = m_camera->GetSettings().GetAcqFrameCount();
    const size_t frameBytes = frameAcqCfg.GetFrameBytes();
    const size_t frameCountIn100MB =
        (frameBytes == 0) ? 0 : ((100 << 20) / frameBytes);
    const size_t recommendedFrameCount = std::min<size_t>(
            10 + std::min(frameCount, frameCountIn100MB),
            m_toBeSavedFramesStats.GetQueueCapacity());
    const bool deepCopy =
        m_camera->GetSettings().GetAcqMode() != AcqMode::SnapSequence;

    // Moved unprocessed frames to unused frames queue
    std::queue<std::shared_ptr<Frame>>().swap(m_toBeProcessedFrames);
    m_toBeProcessedFramesStats.SetQueueSize(0);

    // Moved unsaved frames to unused frames queue
    std::queue<std::shared_ptr<Frame>>().swap(m_toBeSavedFrames);
    m_toBeSavedFramesStats.SetQueueSize(0);

    m_unusedFramesPool.Setup(frameAcqCfg, deepCopy, allocator);
    if (!m_unusedFramesPool.EnsureReadyFrames(recommendedFrameCount, framePoolOps))
        return false;

    return true;
}

bool pm::Acquisition::ConfigureStorage()
{
    const rgn_type rgn = SettingsReader::GetImpliedRegion(
            m_camera->GetSettings().GetRegions());
    const uint32_t bmpW = ((uint32_t)rgn.s2 + 1 - rgn.s1) / rgn.sbin;
    const uint32_t bmpH = ((uint32_t)rgn.p2 + 1 - rgn.p1) / rgn.pbin;

    auto bmpFormat = m_camera->GetFrameAcqCfg().GetBitmapFormat();
    if (m_tiffHelper.colorCtx)
    {
        // TODO: Remove this restriction
        switch (bmpFormat.GetDataType())
        {
        case BitmapDataType::UInt8:
        case BitmapDataType::UInt16:
            break; // Supported types for color helper library
        default:
            Log::LogE("Bitmap data type not supported by Color Helper library");
            return false;
        }

        bmpFormat.SetPixelType(BitmapPixelType::RGB);
        bmpFormat.SetColorMask(
                static_cast<BayerPattern>(m_tiffHelper.colorCtx->pattern));
    }
    else
    {
        bmpFormat.SetPixelType(BitmapPixelType::Mono);
        bmpFormat.SetColorMask(BayerPattern::None);
    }

    bool reallocateBmp = true;
    if (m_tiffHelper.fullBmp)
    {
        reallocateBmp = m_tiffHelper.fullBmp->GetFormat() != bmpFormat
            || m_tiffHelper.fullBmp->GetWidth() != bmpW
            || m_tiffHelper.fullBmp->GetHeight() != bmpH;
    }
    if (reallocateBmp)
    {
        delete m_tiffHelper.fullBmp;
        m_tiffHelper.fullBmp = new(std::nothrow) Bitmap(bmpW, bmpH, bmpFormat);
        if (!m_tiffHelper.fullBmp)
        {
            Log::LogE("Failure allocating bitmap for streaming");
            return false;
        }
    }

    const auto allocator = m_camera->GetAllocator();
    const auto alignment = static_cast<uint16_t>(AllocatorFactory::GetAlignment(*allocator));

    PrdHeader prdHeader;
    PrdFileUtils::InitPrdHeaderStructure(prdHeader, PRD_VERSION_0_8,
            m_camera->GetFrameAcqCfg(), rgn, m_expTimeRes, alignment);

    const auto saveAs = m_camera->GetSettings().GetStorageType();
    const auto saveAsTiff =
        saveAs == StorageType::Tiff || saveAs == StorageType::BigTiff;

    // TODO: Think again and verify. The spp serves more like a ratio between
    //       raw PVCAM data size and size of final file format. E.g.:
    //       - Any format to PRD - ratio is 1:1
    //       - ImageFormat::Mono*  to TIFF                      - ratio is 1:1
    //       - ImageFormat::Bayer* to TIFF with color helper    - ratio is 1:3
    //       - ImageFormat::Bayer* to TIFF without color helper - ratio is 1:1 (stored as mono)
    //       - ImageFormat::RGB*   to TIFF                      - ratio is 1:1
    const auto fileTypeStr = (saveAsTiff)
        ? (m_tiffHelper.colorCtx)
            ? "color TIFF file (approx)"
            : "TIFF file (approx)"
        : "PRD file";
    const auto spp = (saveAsTiff && m_tiffHelper.colorCtx)
        ? bmpFormat.GetSamplesPerPixel()
        : 1;

    const auto maxStackSize = m_camera->GetSettings().GetMaxStackSize();
    const auto fileSingleBytes = PrdFileUtils::GetPrdFileSize(prdHeader) * spp;

    m_frameCountThatFitsStack =
        PrdFileUtils::GetPrdFrameCountThatFitsIn(prdHeader, maxStackSize) / spp;

    Log::LogI("Size of %s with single frame: %zu bytes",
            fileTypeStr, fileSingleBytes);

    if (maxStackSize > 0)
    {
        prdHeader.frameCount = m_frameCountThatFitsStack;
        const auto fileStackBytes = PrdFileUtils::GetPrdFileSize(prdHeader) * spp;

        Log::LogI("Max. size of %s with up to %u stacked frames: %zu bytes",
                fileTypeStr, m_frameCountThatFitsStack, fileStackBytes);

        if (m_frameCountThatFitsStack < 2)
        {
            Log::LogE("Stack size is too small");
            return false;
        }
    }

    UpdateToBeSavedFramesMax();

    return true;
}

void pm::Acquisition::AcqThreadLoop()
{
    m_acqTime = 0.0;

    m_toBeProcessedFramesStats.Reset();

    m_lastFrameNumberInCallback = 0;
    m_lastFrameNumberInHandling = 0;
    m_outOfOrderFrameCount = 0;
    m_uncaughtFrames.Clear();

    const AcqMode acqMode = m_camera->GetSettings().GetAcqMode();
    const bool isAcqModeLive =
        acqMode == AcqMode::LiveCircBuffer || acqMode == AcqMode::LiveTimeLapse;

    const size_t frameCount = (isAcqModeLive)
        ? 0
        : m_camera->GetSettings().GetAcqFrameCount();

    if (!m_camera->StartExp(&Acquisition::EofCallback, this))
    {
        RequestAbort();

        // Let Start method know that this thread has started regardless of
        // StartExp failure. It aborts everything based on m_acqThreadAbortFlag.
        {
            std::unique_lock<std::mutex> lock(m_acqThreadReadyMutex);
            m_acqThreadReadyFlag = true;
            m_acqThreadReadyCond.notify_one();
        }
    }
    else
    {
        m_acqTimer.Reset(); // Start up might take some time, ignore it

        Log::LogI("Acquisition has started successfully\n");

        {
            std::unique_lock<std::mutex> lock(m_acqThreadReadyMutex);
            m_acqThreadReadyFlag = true;
            m_acqThreadReadyCond.notify_one();
        }

        while ((isAcqModeLive
                    || m_toBeProcessedFramesStats.GetFramesTotal() < frameCount)
                && !m_acqThreadAbortFlag)
        {
            std::shared_ptr<Frame> frame = nullptr;
            {
                std::unique_lock<std::mutex> lock(m_toBeProcessedFramesMutex);

                const bool timedOut = !m_toBeProcessedFramesCond.wait_for(
                        lock, std::chrono::milliseconds(5000), [this]() {
                            return (!m_toBeProcessedFrames.empty() || m_acqThreadAbortFlag);
                        });
                if (timedOut)
                {
                    if (m_camera->GetAcqStatus() == Camera::AcqStatus::Active)
                        continue;

                    Log::LogE("Acquisition seems to be not active anymore");
                    RequestAbort(false); // Let queued frames to be processed
                    break;
                }
                if (m_acqThreadAbortFlag)
                    break;

                frame = m_toBeProcessedFrames.front();
                m_toBeProcessedFrames.pop();
                m_toBeProcessedFramesStats.SetQueueSize(m_toBeProcessedFrames.size());
            }
            // frame is always valid here
            if (!HandleNewFrame(frame))
            {
                RequestAbort(false); // Let queued frames to be processed
                break;
            }

            // Ensure there are some ready-to-use frame for HandleEofCallback
            m_unusedFramesPool.EnsureReadyFrames(3);
        }

        m_acqTime = m_acqTimer.Seconds();

        m_camera->StopExp();

        std::ostringstream ss;
        ss << (m_toBeProcessedFramesStats.GetFramesTotal())
            << " frames acquired from the camera and "
            << m_toBeProcessedFramesStats.GetFramesAcquired()
            << " of them queued for processing in " << m_acqTime << " seconds";
        Log::LogI(ss.str());
    }

    m_acqThreadDoneFlag = true;

    if (m_fpsLimiter)
    {
        m_fpsLimiter->SetAcqFinished();
    }

    // Wake disk waiter just in case it will abort right away
    m_toBeSavedFramesCond.notify_one();

    // Allow update thread to finish
    m_updateThreadCond.notify_one();
}

void pm::Acquisition::DiskThreadLoop()
{
    m_diskTimer.Reset();
    m_diskTime = 0.0;

    m_toBeSavedFramesStats.Reset();
    m_toBeSavedFramesSaved = 0;
    m_unsavedFrames.Clear();

    const StorageType storageType = m_camera->GetSettings().GetStorageType();

    DiskThreadLoopWriter();

    m_diskTime = m_diskTimer.Seconds();

    if (m_trackEnabled && m_diskThreadAbortFlag)
    {
        // Moved unsaved frames to unused frames queue while invalidating
        // trajectories in camera's circular buffer.
        // No locking needed here.
        while (!m_toBeSavedFrames.empty())
        {
            auto frame = m_toBeSavedFrames.front();
            m_toBeSavedFrames.pop();

            size_t index;
            if (m_camera->GetFrameIndex(*frame, index))
            {
                auto camFrame = m_camera->GetFrameAt(index);
                if (camFrame)
                {
                    camFrame->SetTrajectories(Frame::Trajectories());

                    if (m_toBeSavedFrames.empty() && m_fpsLimiter)
                    {
                        m_fpsLimiter->InputNewFrame(camFrame);
                    }
                }
            }
        }
        m_toBeSavedFramesStats.SetQueueSize(0);
    }

    m_diskThreadDoneFlag = true;

    // Allow update thread to finish
    m_updateThreadCond.notify_one();

    // Wait for updateThread thread stop
    m_updateThread->join();

    if (m_diskTime > 0.0)
    {
        std::ostringstream ss;
        ss << m_toBeSavedFramesStats.GetFramesTotal() << " queued frames processed and ";
        switch (storageType)
        {
        case StorageType::Prd:
            ss << m_toBeSavedFramesSaved << " of them saved to PRD file(s)";
            break;
        case StorageType::Tiff:
            ss << m_toBeSavedFramesSaved << " of them saved to TIFF file(s)";
            break;
        case StorageType::BigTiff:
            ss << m_toBeSavedFramesSaved << " of them saved to BIG TIFF file(s)";
            break;
        case StorageType::None:
            ss << "none of them saved";
            break;
        // No default section, compiler will complain when new format added
        }
        ss << " in " << m_diskTime << " seconds\n";
        Log::LogI(ss.str());
    }
}

void pm::Acquisition::DiskThreadLoopWriter()
{
#ifdef PM_PRINT_WRITE_STATS
    sWriteCount = 0;
    sWriteTimeSec = 0.0;
#endif

    const AcqMode acqMode = m_camera->GetSettings().GetAcqMode();
    const bool isAcqModeLive =
        acqMode == AcqMode::LiveCircBuffer || acqMode == AcqMode::LiveTimeLapse;

    const size_t frameCount = (isAcqModeLive)
        ? 0
        : m_camera->GetSettings().GetAcqFrameCount();
    const StorageType storageType = m_camera->GetSettings().GetStorageType();
    //CHANGE SAVE, line below allows user to set the save location in the GUI
    const std::string saveDir = m_camera->GetSettings().GetSaveDir(); 
    //const std::string saveDir = "C:\\Users\\localadmin\\Desktop\\CAFÉ Grande Images\\";
    const uns8 saveDigits = m_camera->GetSettings().GetSaveDigits();
    const size_t saveFirst = (isAcqModeLive)
        ? m_camera->GetSettings().GetSaveFirst()
        : std::min(frameCount, m_camera->GetSettings().GetSaveFirst());
    const size_t saveLast = (isAcqModeLive)
        ? 0
        : std::min(frameCount, m_camera->GetSettings().GetSaveLast());

    const rgn_type rgn = SettingsReader::GetImpliedRegion(
            m_camera->GetSettings().GetRegions());
    const auto allocator = m_camera->GetAllocator();
    const auto alignment = static_cast<uint16_t>(AllocatorFactory::GetAlignment(*allocator));

    PrdHeader prdHeader;
    PrdFileUtils::InitPrdHeaderStructure(prdHeader, PRD_VERSION_0_8,
            m_camera->GetFrameAcqCfg(), rgn, m_expTimeRes, alignment);

    const size_t maxStackSize = m_camera->GetSettings().GetMaxStackSize();
    const bool saveAsStack = maxStackSize > 0;
    const uint32_t maxFramesPerFile = (saveAsStack) ? m_frameCountThatFitsStack : 1;

    const std::string fileDir = ((saveDir.empty()) ? "." : saveDir) + "/";
    std::string fileName;
    FileSave* file = nullptr;
    
    // Absolute frame index in saving sequence
    size_t frameIndex = 0;

    // Store import instructions for PRD in 'saveDir' before first frame arrives
    if (storageType == StorageType::Prd)
    {
        std::string importFileName = fileDir + "0_import_imagej.txt";

        prdHeader.frameCount = maxFramesPerFile; // Set max. size
        try
        {
            std::ofstream fout(importFileName);
            fout << PrdFileUtils::GetPrdImportHints_ImageJ(prdHeader);
            fout.close();
        }
        catch(...)
        {
            RequestAbort(); // The main while loop below won't be entered
        }
        prdHeader.frameCount = 1; // Change back
    }

    {
        std::unique_lock<std::mutex> lock(m_diskThreadReadyMutex);
        m_diskThreadReadyFlag = true;
        m_diskThreadReadyCond.notify_one();
    }

    while ((isAcqModeLive || frameIndex < frameCount)
            && !m_diskThreadAbortFlag)
    {
        std::shared_ptr<Frame> frame = nullptr;
        {
            std::unique_lock<std::mutex> lock(m_toBeSavedFramesMutex);

            if (m_toBeSavedFrames.empty())
            {
                // There are no queued frames and acquisition has finished, stop this thread
                if (m_acqThreadDoneFlag)
                    break;

                m_toBeSavedFramesCond.wait(lock, [this]() {
                    const bool empty = m_toBeSavedFrames.empty();
                    return (!empty || m_diskThreadAbortFlag
                            || (m_acqThreadDoneFlag && empty));
                });
            }
            if (m_diskThreadAbortFlag)
                break;
            if (m_acqThreadDoneFlag && m_toBeSavedFrames.empty())
                break;

            frame = m_toBeSavedFrames.front();
            m_toBeSavedFrames.pop();
            m_toBeSavedFramesStats.SetQueueSize(m_toBeSavedFrames.size());
        }

        bool keepGoing = true;

        // If not tracking particles, frame is sent to GUI in acquisition thread
        if (m_trackEnabled)
        {
            if (!TrackNewFrame(frame))
            {
                RequestAbort();
                break;
            }
        }
        else
        {
            if (m_acqThreadDoneFlag && m_fpsLimiter)
            {
                // Pass null frame to FPS limiter for later processing in GUI
                // to let GUI know that disk thread is still working
                m_fpsLimiter->InputNewFrame(nullptr);
            }
        }
        m_toBeSavedFramesStats.ReportFrameAcquired();

        const bool doSaveFirst = saveFirst > 0 && frameIndex < saveFirst;
        const bool doSaveLast = saveLast > 0 && frameIndex >= frameCount - saveLast;
        const bool doSaveAll = (saveFirst == 0 && saveLast == 0)
            || (!isAcqModeLive && saveFirst >= frameCount - saveLast);
        const bool doSave = doSaveFirst || doSaveLast || doSaveAll;

        if (storageType != StorageType::None && doSave)
        {
            // Index for output file, relative either to sequence beginning
            // or to first frame for save-last option
            size_t fileIndex;
            // Relative frame index in file, first in file is 0
            size_t frameIndexInFile;

            if (saveAsStack)
            {
                if (doSaveFirst || doSaveAll)
                {
                    fileIndex = frameIndex / maxFramesPerFile;
                    frameIndexInFile = frameIndex % maxFramesPerFile;
                }
                else // doSaveLast
                {
                    fileIndex = (frameIndex - (frameCount - saveLast)) / maxFramesPerFile;
                    frameIndexInFile = (frameIndex - (frameCount - saveLast)) % maxFramesPerFile;
                }
            }
            else
            {
                // Used frame number instead of frameIndex for single frame files
                fileIndex = frame->GetInfo().GetFrameNr();
                frameIndexInFile = 0;
            }
            
            // First frame in new file, close previous file and open new one
            if (frameIndexInFile == 0)
            {
                // Close previous file if some open
                if (file)
                {
                    file->Close();
                    delete file;
                    file = nullptr;
                }

                fileName = fileDir;
                if (saveAsStack)
                {
                    size_t saveCount;
                    if (doSaveAll)
                    {
                        saveCount = frameCount;
                        fileName += "ss_stack_";
                    }
                    else if (doSaveFirst)
                    {
                        saveCount = saveFirst;
                        fileName += "ss_stack_first_";
                    }
                    else // doSaveLast
                    {
                        saveCount = saveLast;
                        fileName += "ss_stack_last_";
                    }
                    prdHeader.frameCount =
                        (fileIndex < (saveCount - 1) / maxFramesPerFile)
                        ? maxFramesPerFile
                        : ((saveCount - 1) % maxFramesPerFile) + 1;
                }
                else
                {
                    //CHANGE
                    extern std::string generatedFileName;
                    //prdHeader.frameCount = 1;
                    //CHANGE Check how to modify the names to be more useful
                    //std::string saveAs = "ss_single_Gain_" + std::string(pm::ui::MainDlg::m_cbGain->)
                    //fileName += "ss_single_" + generatedFileName;
                    fileName += generatedFileName;
                    //fileName += "ss_single_";
                }
                std::stringstream ss;
                ss << std::setfill('0') << std::setw(saveDigits) << fileIndex;
                fileName += ss.str();

                switch (storageType)
                {
                case StorageType::Prd:
                    fileName += ".prd";
                    file = new(std::nothrow) PrdFileSave(fileName, prdHeader, allocator);
                    break;
                case StorageType::Tiff:
                case StorageType::BigTiff:
                    fileName += ".tiff";
                    file = new(std::nothrow) TiffFileSave(fileName, prdHeader,
                            &m_tiffHelper, storageType == StorageType::BigTiff);
                    break;
                case StorageType::None:
                    break;
                // No default section, compiler will complain when new format added
                }

                // Open the file
                if (!file || !file->Open())
                {
                    Log::LogE("Error in opening file '%s' for frame with index %zu",
                            fileName.c_str(), frameIndex);
                    keepGoing = false;

                    delete file;
                    file = nullptr;
                }
            }

            // If file is open store current frame in it
            if (file)
            {
#ifdef PM_PRINT_WRITE_STATS
                sWriteTimer.Reset();
#endif

                if (!file->WriteFrame(frame))
                {
                    Log::LogE("Error in writing RAW data to '%s' for frame with index %zu",
                            fileName.c_str(), frameIndex);
                    keepGoing = false;
                }
                else
                {
                    m_toBeSavedFramesSaved++;
                }

#ifdef PM_PRINT_WRITE_STATS
                sWriteTimeSec += sWriteTimer.Seconds();
                sWriteCount++;
#endif
            }
        }

        if (!keepGoing)
            RequestAbort();

        frameIndex++;
    }

    // Just to be sure, close last file if remained open
    if (file)
    {
        file->Close();
        delete file;
    }

#ifdef PM_PRINT_WRITE_STATS
    if (sWriteCount > 0)
    {
        const auto headerCount = (sWriteCount + maxFramesPerFile - 1) / maxFramesPerFile;
        const auto prdHeaderBytesAligned =
            PrdFileUtils::GetAlignedSize(prdHeader, sizeof(PrdHeader));
        const auto prdMetaDataBytesAligned =
            PrdFileUtils::GetAlignedSize(prdHeader, prdHeader.sizeOfPrdMetaDataStruct);
        const auto rawDataBytes = PrdFileUtils::GetRawDataSize(prdHeader);
        const auto rawDataBytesAligned =
            PrdFileUtils::GetAlignedSize(prdHeader, rawDataBytes);

        const size_t bytes = headerCount * prdHeaderBytesAligned
            + sWriteCount * (prdMetaDataBytesAligned + rawDataBytesAligned);

        const double Bps = bytes / sWriteTimeSec;

        Log::LogI("Average file write speed is %.1f MiB/s", Bps / 1000000.0);
    }
#endif
}

void pm::Acquisition::UpdateThreadLoop()
{
    const std::vector<std::string> progress{ "|", "/", "-", "\\" };
    size_t progressIndex = 0;
    size_t maxRefreshCounter = 0;

    {
        std::unique_lock<std::mutex> lock(m_updateThreadReadyMutex);
        m_updateThreadReadyFlag = true;
        m_updateThreadReadyCond.notify_one();
    }

    while (!(m_acqThreadDoneFlag && m_diskThreadDoneFlag))
    {
        // Use wait_for instead of sleep to stop immediately on request
        {
            std::unique_lock<std::mutex> lock(m_updateThreadMutex);
            m_updateThreadCond.wait_for(lock, std::chrono::milliseconds(500), [this]() {
                return (m_acqThreadDoneFlag && m_diskThreadDoneFlag);
            });
        }
        if (m_acqThreadDoneFlag && m_diskThreadDoneFlag)
            break;

        // Don't update limits too often
        maxRefreshCounter++;
        if ((maxRefreshCounter % 8 == 0) && !m_acqThreadDoneFlag)
        {
            UpdateToBeSavedFramesMax();
        }

        // Print info about progress
        std::ostringstream ss;

        // Get shorter numbers
        const auto frameBytes = m_camera->GetFrameAcqCfg().GetFrameBytes();
        const auto fps = m_toBeProcessedFramesStats.GetAvgFrameRate();
        const auto shortFps = round(fps);
        const auto shortMiBps = round(fps * frameBytes / 1024 / 1024);

        ss << progress[progressIndex] << " caught "
            << m_toBeProcessedFramesStats.GetFramesTotal() << " frames";
        if (m_toBeProcessedFramesStats.GetFramesLost() > 0)
            ss << " (" << m_toBeProcessedFramesStats.GetFramesLost() << " lost)";
        ss << ", " << shortFps << "fps " << shortMiBps << "MiB/s";

        ss << ", " << m_toBeSavedFramesStats.GetFramesTotal() << " queued";
        if (m_toBeSavedFramesStats.GetFramesLost() > 0)
            ss << " (" << m_toBeSavedFramesStats.GetFramesLost() << " dropped)";

        ss << ", " << m_toBeSavedFramesStats.GetFramesAcquired() << " processed";
        ss << ", " << m_toBeSavedFramesSaved << " saved";

        if (m_diskThreadAbortFlag)
            ss << ", aborting...";
        else if (m_acqThreadAbortFlag)
            ss << ", finishing...";

        Log::LogP(ss.str());

        progressIndex = (progressIndex + 1) % progress.size();
    }
}

void pm::Acquisition::PrintAcqThreadStats() const
{
    const size_t frameCount = m_toBeProcessedFramesStats.GetFramesTotal();
    const double frameDropsPercent = (frameCount > 0)
        ? ((double)m_uncaughtFrames.GetCount() / (double)frameCount) * 100
        : 0.0;
    const double fps = m_toBeProcessedFramesStats.GetOverallFrameRate();
    const double MiBps =
        round(fps * m_camera->GetFrameAcqCfg().GetFrameBytes() * 10 / 1024 / 1024) / 10.0;

    std::ostringstream ss;
    ss << "Acquisition thread queue stats:"
        << "\n  Frame count = " << frameCount
        << "\n  Frame drops = " << m_uncaughtFrames.GetCount()
            << " (" << frameDropsPercent << " %)"
        << "\n  Average # frames between drops = " << m_uncaughtFrames.GetAvgSpacing()
        << "\n  Longest series of dropped frames = " << m_uncaughtFrames.GetLargestCluster()
        << "\n  Max. used frames = " << m_toBeProcessedFramesStats.GetQueueSizePeak()
        << " out of " << m_toBeProcessedFramesStats.GetQueueCapacity()
        << "\n  Acquisition ran with " << fps << " fps (" << MiBps << " MiB/s)";
    if (m_outOfOrderFrameCount > 0)
    {
        ss << "\n  " << m_outOfOrderFrameCount
            << " frames with frame number <= last stored frame number";
    }
    ss << "\n";

    Log::LogI(ss.str());
}

void pm::Acquisition::PrintDiskThreadStats() const
{
    const size_t frameCount = m_toBeSavedFramesStats.GetFramesTotal();
    const double frameDropsPercent = (frameCount > 0)
        ? ((double)m_unsavedFrames.GetCount() / (double)frameCount) * 100
        : 0.0;
    const double fps = m_toBeSavedFramesStats.GetOverallFrameRate();
    const double MiBps =
        round(fps * m_camera->GetFrameAcqCfg().GetFrameBytes() * 10 / 1024 / 1024) / 10.0;

    std::ostringstream ss;
    ss << "Processing thread queue stats:"
        << "\n  Frame count = " << frameCount
        << "\n  Frame drops = " << m_unsavedFrames.GetCount()
            << " (" << frameDropsPercent << " %)"
        << "\n  Average # frames between drops = " << m_unsavedFrames.GetAvgSpacing()
        << "\n  Longest series of dropped frames = " << m_unsavedFrames.GetLargestCluster()
        << "\n  Max. used frames = " << m_toBeSavedFramesStats.GetQueueSizePeak()
        // Queue capacity could be less than current peak which would confuse users
        //<< " out of " << m_toBeSavedFramesStats.GetQueueCapacity()
        << "\n  Processing ran with " << fps << " fps (" << MiBps << " MiB/s)\n";

    Log::LogI(ss.str());
}
