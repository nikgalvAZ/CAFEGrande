/******************************************************************************/
/* Copyright (C) Teledyne Photometrics. All rights reserved.                  */
/******************************************************************************/
#include "backend/AcquisitionStats.h"

/* System */
#include <algorithm>
#include <cassert>

void pm::AcquisitionStats::Reset()
{
    m_queueSizePeak = 0;
    m_framesAcquired = 0;
    m_framesLost = 0;

    m_firstFrameTime = 0.0;
    m_firstFrameCount = 0;

    m_lastFrameTime = 0.0;
    m_framePeriod = 0.0;

    m_lastAvgFrameTime = 0.0;
    m_lastAvgFrameCount = 0;
    m_avgFramePeriod = 0.0;

    m_overallFramePeriod = 0.0;

    m_timer.Reset();
}

void pm::AcquisitionStats::SetQueueCapacity(size_t capacity)
{
    m_queueCapacity = std::max<size_t>(1, capacity);
}

size_t pm::AcquisitionStats::GetQueueCapacity() const
{
    return m_queueCapacity;
}

void pm::AcquisitionStats::SetQueueSize(size_t size)
{
    m_queueSize = size;
    if (m_queueSizePeak < size)
    {
        m_queueSizePeak = size;
    }
}

size_t pm::AcquisitionStats::GetQueueSize() const
{
    return m_queueSize;
}

size_t pm::AcquisitionStats::GetQueueSizePeak() const
{
    return m_queueSizePeak;
}

void pm::AcquisitionStats::ReportFrameAcquired()
{
    m_framesAcquired += 1;
    UpdateValues(1);
}

size_t pm::AcquisitionStats::GetFramesAcquired() const
{
    return m_framesAcquired;
}

void pm::AcquisitionStats::ReportFrameLost(size_t count)
{
    m_framesLost += count;
    UpdateValues(count);
}

size_t pm::AcquisitionStats::GetFramesLost() const
{
    return m_framesLost;
}

size_t pm::AcquisitionStats::GetFramesTotal() const
{
    return m_framesAcquired + m_framesLost;
}

double pm::AcquisitionStats::GetFramePeriod() const
{
    return m_framePeriod;
}

double pm::AcquisitionStats::GetFrameRate() const
{
    return (m_framePeriod > 0.0) ? (1.0 / m_framePeriod) : 0.0;
}

double pm::AcquisitionStats::GetAvgFramePeriod() const
{
    return m_avgFramePeriod;
}

double pm::AcquisitionStats::GetAvgFrameRate() const
{
    return (m_avgFramePeriod > 0.0) ? (1.0 / m_avgFramePeriod) : 0.0;
}

double pm::AcquisitionStats::GetOverallFramePeriod() const
{
    return m_overallFramePeriod;
}

double pm::AcquisitionStats::GetOverallFrameRate() const
{
    return (m_overallFramePeriod > 0.0) ? (1.0 / m_overallFramePeriod) : 0.0;
}

void pm::AcquisitionStats::UpdateValues(size_t frameDiff)
{
    assert(frameDiff > 0);

    const auto timeNow = m_timer.Seconds();

    const auto lastFrameTime = m_lastFrameTime;
    m_lastFrameTime = timeNow;
    if (m_firstFrameCount == 0)
    {
        m_firstFrameTime = timeNow;
        m_firstFrameCount = frameDiff;
        m_lastAvgFrameTime = m_firstFrameTime;
        m_lastAvgFrameCount = m_firstFrameCount;
        m_overallFramePeriod = m_firstFrameTime / m_firstFrameCount;
        return; // We don't have two frames after reset yet
    }

    const auto timeDiff = timeNow - lastFrameTime;
    // Use average value if more than one frames lost
    m_framePeriod = timeDiff / frameDiff;

    const auto totalFrameCount = m_framesAcquired + m_framesLost;

    const auto overallTimeDiff = timeNow - m_firstFrameTime;
    const auto overallFrameDiff = totalFrameCount - m_firstFrameCount;
    m_overallFramePeriod = overallTimeDiff / overallFrameDiff;

    const auto avgTimeDiff = timeNow - m_lastAvgFrameTime;
    if (avgTimeDiff < 0.5)
        return; // 500ms didn't elapsed yet to calculate average value

    const auto avgFrameDiff = totalFrameCount - m_lastAvgFrameCount;
    m_lastAvgFrameTime = timeNow;
    m_lastAvgFrameCount = totalFrameCount;
    m_avgFramePeriod = avgTimeDiff / avgFrameDiff;
}
