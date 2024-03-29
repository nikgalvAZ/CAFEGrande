/******************************************************************************/
/* Copyright (C) Teledyne Photometrics. All rights reserved.                  */
/******************************************************************************/
#pragma once
#ifndef PM_ACQUISITION_STATS_H
#define PM_ACQUISITION_STATS_H

/* Local */
#include "backend/Timer.h"

namespace pm {

class AcquisitionStats final
{
public:
    // Resets acquired & lost frames, a queue peak size and
    // internal timer and members related to frame periods only
    void Reset();

public:
    // Auto-corrected value to be min. 1
    void SetQueueCapacity(size_t capacity);
    size_t GetQueueCapacity() const;

    void SetQueueSize(size_t size);
    size_t GetQueueSize() const;

    size_t GetQueueSizePeak() const;

public:
    void ReportFrameAcquired();
    size_t GetFramesAcquired() const;

    void ReportFrameLost(size_t count = 1);
    size_t GetFramesLost() const;

    size_t GetFramesTotal() const;

    // For two consecutive frames, in seconds
    double GetFramePeriod() const;
    double GetFrameRate() const;

    // Average value for all frames reported within at least 500ms, in seconds
    double GetAvgFramePeriod() const;
    double GetAvgFrameRate() const;

    // Average value for all frames since last reset, in seconds
    double GetOverallFramePeriod() const;
    double GetOverallFrameRate() const;

private:
    void UpdateValues(size_t frameDiff);

private:
    // Maximal size of a processing queue
    size_t m_queueCapacity{ 1 };
    // Current size of a processing queue
    size_t m_queueSize{ 0 };
    // Maximal size of a queue that was set since last reset
    size_t m_queueSizePeak{ 0 };

    // Holds how many frames have been processed since last reset
    size_t m_framesAcquired{ 0 };
    // Holds how many frames have been lost since last reset
    size_t m_framesLost{ 0 };

    Timer m_timer{};

    double m_firstFrameTime{ 0.0 };
    size_t m_firstFrameCount{ 0 };

    double m_lastFrameTime{ 0.0 };
    double m_framePeriod{ 0.0 };

    double m_lastAvgFrameTime{ 0.0 };
    size_t m_lastAvgFrameCount{ 0 };
    double m_avgFramePeriod{ 0.0 };

    double m_overallFramePeriod{ 0.0 };
};

}; // namespace pm

#endif /* PM_ACQUISITION_STATS_H */
