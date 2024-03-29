/******************************************************************************/
/* Copyright (C) Teledyne Photometrics. All rights reserved.                  */
/******************************************************************************/
#pragma once
#ifndef PM_ACQUISITION_H
#define PM_ACQUISITION_H

/* Local */
#include "backend/AcquisitionStats.h"
#include "backend/FpsLimiter.h"
#include "backend/Frame.h"
#include "backend/FramePool.h"
#include "backend/FrameProcessor.h"
#include "backend/ListStatistics.h"
#include "backend/PrdFileFormat.h"
#include "backend/TiffFileSave.h"
#include "backend/Timer.h"


/* pvcam_helper_track */
#include "pvcam_helper_track.h"

/* System */
#include <atomic>
#include <condition_variable>
#include <memory>
#include <mutex>
#include <queue>

namespace std
{
    class thread;
}

// Forward declaration for FRAME_INFO that satisfies compiler (taken from pvcam.h)
struct _TAG_FRAME_INFO;
typedef struct _TAG_FRAME_INFO FRAME_INFO;

// Forward declaration for color context (taken from pvcam_helper_color.h)
struct ph_color_context;

namespace pm {

class Camera;
class ParticleLinker;

class Acquisition
{
public:
    explicit Acquisition(std::shared_ptr<Camera> camera);
    ~Acquisition();

    Acquisition() = delete;
    Acquisition(const Acquisition&) = delete;
    Acquisition& operator=(const Acquisition&) = delete;

    // Starts the acquisition.
    // The non-null color context values are copied to local context so the
    // caller can use and change its context as needed. It is used in case the
    // streaming to disk is enabled and either tiff or big-tiff format selected.
    // See TiffFileSave::Helper for details.
    bool Start(std::shared_ptr<FpsLimiter> fpsLimiter = nullptr,
            double tiffFillValue = 0.0,
            const ph_color_context* tiffColorCtx = nullptr);
    // Returns true if acquisition is running, false otherwise
    bool IsRunning() const;
    // Forces correct acquisition interruption
    void RequestAbort(bool abortBufferedFramesProcessing = true);
    /* Blocks until the acquisition completes or reacts to abort request.
       Return true if stopped due to abort request. */
    bool WaitForStop(bool printStats = false);

    // Returns acquisition related statistics
    const AcquisitionStats& GetAcqStats() const;
    // Returns storage/processing related statistics
    const AcquisitionStats& GetDiskStats() const;

private:
    static void PV_DECL EofCallback(FRAME_INFO* frameInfo,
            void* Acquisition_pointer);

private:
    // Called from callback function to handle new frame
    bool HandleEofCallback(FRAME_INFO* frameInfo);
    // Called from AcqThreadLoop to handle new frame
    bool HandleNewFrame(std::shared_ptr<Frame> frame);
    // Tracks particles and updates trajectories points
    bool TrackNewFrame(std::shared_ptr<Frame> frame);

    // Updates max. allowed number of frames in queue to be saved
    void UpdateToBeSavedFramesMax();
    // Preallocate or release some ready-to-use frames at start/end
    bool PreallocateUnusedFrames(int framePoolOps = FramePool::Ops::None);
    // Configures how frames will be stored on disk
    bool ConfigureStorage();

    // The function performs in m_acqThread, caches frames from camera
    void AcqThreadLoop();
    // The function performs in m_diskThread, saves frames to disk
    void DiskThreadLoop();
    // Called from DiskThreadLoop, now for both, one frame per file and stacked frames
    void DiskThreadLoopWriter();
    // The function performs in m_updateThread, saves frames to disk
    void UpdateThreadLoop();

    void PrintAcqThreadStats() const;
    void PrintDiskThreadStats() const;

private:
    std::shared_ptr<Camera> m_camera{ nullptr };
    std::shared_ptr<FpsLimiter> m_fpsLimiter{ nullptr };

    uint32_t m_frameCountThatFitsStack{ 0 }; // Limited to 32 bit

    // Uncaught frames statistics
    ListStatistics<size_t> m_uncaughtFrames{};
    // Unsaved frames statistics
    ListStatistics<size_t> m_unsavedFrames{};

    std::thread*            m_acqThread{ nullptr };
    bool                    m_acqThreadReadyFlag{ false };
    std::mutex              m_acqThreadReadyMutex{};
    std::condition_variable m_acqThreadReadyCond{};
    std::atomic<bool>       m_acqThreadAbortFlag{ false };
    std::atomic<bool>       m_acqThreadDoneFlag{ false };
    std::thread*            m_diskThread{ nullptr };
    bool                    m_diskThreadReadyFlag{ false };
    std::mutex              m_diskThreadReadyMutex{};
    std::condition_variable m_diskThreadReadyCond{};
    std::atomic<bool>       m_diskThreadAbortFlag{ false };
    std::atomic<bool>       m_diskThreadDoneFlag{ false };
    std::thread*            m_updateThread{ nullptr };
    bool                    m_updateThreadReadyFlag{ false };
    std::mutex              m_updateThreadReadyMutex{};
    std::condition_variable m_updateThreadReadyCond{};

    // The Timer used for m_acqTime
    Timer m_acqTimer{};
    // Time taken to finish acquisition, zero if in progress
    double m_acqTime{ 0.0 };
    // The Timer used for m_diskTime
    Timer m_diskTimer{};
    // Time taken to finish saving, zero if in progress
    double m_diskTime{ 0.0 };

    uint32_t m_lastFrameNumberInCallback{ 0 };
    uint32_t m_lastFrameNumberInHandling{ 0 };

    // Cached value so we don't check settings with every frame
    bool m_trackEnabled{ false };
    PH_TRACK_CONTEXT m_trackContext{ PH_TRACK_CONTEXT_INVALID };
    uns32 m_trackMaxParticles{ 0 };
    ph_track_particle* m_trackParticles{ nullptr };
    ParticleLinker* m_trackLinker{ nullptr };

    uint32_t m_expTimeRes{ EXP_RES_ONE_MILLISEC };
    uint16_t m_centroidsRadius{ 1 };

    TiffFileSave::Helper m_tiffHelper{};
    FrameProcessor m_tiffFrameProc{};

    std::atomic<size_t> m_outOfOrderFrameCount{ 0 };

    // Mutex that guards all non-atomic m_updateThread* variables
    std::mutex              m_updateThreadMutex{};
    // Condition the update thread waits on for new update iteration
    std::condition_variable m_updateThreadCond{};

    /*
       Data flow is like this:
       1. In callback handler thread is:
          - taken one frame from m_unusedFramesPool
          - stored frame info and pointer to data (shallow copy only) in frame
          - frame put to m_toBeProcessedFrames queue
       2. In acquisition thread is:
          - made deep copy of frame's data
          - done check for lost frames
          - frame moved to m_toBeSavedFrames queue
       3. In disk thread is:
          - tracked frame trajectory
          - frame stored to disk in chosen format
          - frame moved back to m_unusedFramesPool
    */

    // Frames captured in callback thread to be processed in acquisition thread
    std::queue<std::shared_ptr<Frame>>  m_toBeProcessedFrames{};
    // Mutex that guards all non-atomic m_toBeProcessed* variables
    std::mutex                          m_toBeProcessedFramesMutex{};
    // Condition the acquisition thread waits on for new frame
    std::condition_variable             m_toBeProcessedFramesCond{};
    // Acquisition statistics with captured & lost frames and queue usage
    AcquisitionStats                    m_toBeProcessedFramesStats{};

    // Frames queued in acquisition thread to be saved to disk
    std::queue<std::shared_ptr<Frame>>  m_toBeSavedFrames{};
    // Mutex that guards all non-atomic m_toBeSavedFrames* variables
    std::mutex                          m_toBeSavedFramesMutex{};
    // Condition the frame saving thread waits on for new frame
    std::condition_variable             m_toBeSavedFramesCond{};
    // Acquisition statistics with queued & dropped frames and queue usage
    AcquisitionStats                    m_toBeSavedFramesStats{};

    // Holds how many queued frames have been saved to disk
    std::atomic<size_t>                 m_toBeSavedFramesSaved{ 0 };

    // Unused but allocated frames to be re-used
    FramePool m_unusedFramesPool{};
};

} // namespace

#endif
