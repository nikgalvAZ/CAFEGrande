/******************************************************************************/
/* Copyright (C) Teledyne Photometrics. All rights reserved.                  */
/******************************************************************************/
#include "backend/FrameStats.h"

/* System */
#include <cmath>
#include <limits>

pm::FrameStats::FrameStats()
{
    Clear();
}

void pm::FrameStats::Clear()
{
    m_pixelCount = 0;

    m_min = 0.0;
    m_max = 0.0;
    m_mean = 0.0;

    m_sum = 0.0;
    m_sumSq = 0.0;
    m_secondMoment = 0.0;

    m_variance = std::numeric_limits<double>::quiet_NaN();
    m_stdDev = std::numeric_limits<double>::quiet_NaN();
}

bool pm::FrameStats::IsEmpty() const
{
    return m_pixelCount == 0;
}

void pm::FrameStats::SetViaSums(uint32_t pixelCount, double min, double max,
        double sum, double sumSq)
{
    Clear();

    if (pixelCount == 0)
        return;

    m_pixelCount = pixelCount;

    m_min = min;
    m_max = max;
    m_mean = sum / pixelCount;

    m_sum = sum;
    m_sumSq = sumSq;
    m_secondMoment = sumSq - m_mean * sum;

    if (m_secondMoment > 0.0 && pixelCount >= 2)
    {
        m_variance = m_secondMoment / pixelCount;
        m_stdDev = ::sqrt(m_variance);
    }
}

void pm::FrameStats::SetDirectly(uint32_t pixelCount, double min, double max,
        double mean, double secondMoment)
{
    Clear();

    if (pixelCount == 0)
        return;

    m_pixelCount = pixelCount;

    m_min = min;
    m_max = max;
    m_mean = mean;

    m_sum = pixelCount * mean;
    m_sumSq = pixelCount * mean * mean;
    m_secondMoment = secondMoment;

    if (secondMoment > 0.0 && pixelCount >= 2)
    {
        m_variance = secondMoment / pixelCount;
        m_stdDev = ::sqrt(m_variance);
    }
}

void pm::FrameStats::Add(const FrameStats& stats)
{
    if (stats.IsEmpty())
        return;

    if (IsEmpty())
    {
        *this = stats;
        return;
    }

    if (m_min > stats.GetMin())
        m_min = stats.GetMin();
    if (m_max < stats.GetMax())
        m_max = stats.GetMax();
    m_sum += stats.GetSum();
    m_sumSq += stats.GetSumSq();

    const uint32_t n_a = m_pixelCount;
    const double avg_a = m_mean; // M1_a
    const double  M2_a = m_secondMoment;

    const uint32_t n_b = stats.GetPixelCount();
    const double avg_b = stats.GetMean(); // M1_b
    const double  M2_b = stats.GetSecondMoment();

    const uint32_t n_ab = n_a + n_b;
    const double delta = avg_b - avg_a;
    const double avg_ab = (n_a * avg_a + n_b * avg_b) / n_ab; // M1_ab
    const double  M2_ab = M2_a + M2_b + (delta * delta * n_a * n_b) / n_ab;
    const double var_ab = M2_ab / n_ab;
    const double stdD_ab = ::sqrt(var_ab);

    m_pixelCount = n_ab;
    m_mean = avg_ab;
    m_secondMoment = M2_ab;
    m_variance = var_ab;
    m_stdDev = stdD_ab;
}

uint32_t pm::FrameStats::GetPixelCount() const
{
    return m_pixelCount;
}

void pm::FrameStats::SetPixelCount(uint32_t pixelCount)
{
    m_pixelCount = pixelCount;
}

double pm::FrameStats::GetMin() const
{
    return m_min;
}

void pm::FrameStats::SetMin(double min)
{
    m_min = min;
}

double pm::FrameStats::GetMax() const
{
    return m_max;
}

void pm::FrameStats::SetMax(double max)
{
    m_max = max;
}

double pm::FrameStats::GetMean() const
{
    return m_mean;
}

void pm::FrameStats::SetMean(double mean)
{
    m_mean = mean;
}

double pm::FrameStats::GetSum() const
{
    return m_sum;
}

void pm::FrameStats::SetSum(double sum)
{
    m_sum = sum;
}

double pm::FrameStats::GetSumSq() const
{
    return m_sumSq;
}

void pm::FrameStats::SetSumSq(double sumSq)
{
    m_sumSq = sumSq;
}

double pm::FrameStats::GetSecondMoment() const
{
    return m_secondMoment;
}

void pm::FrameStats::SetSecondMoment(double m2)
{
    m_secondMoment = m2;
}

double pm::FrameStats::GetVariance() const
{
    return m_variance;
}

void pm::FrameStats::SetVariance(double variance)
{
    m_variance = variance;
}

double pm::FrameStats::GetStdDev() const
{
    return m_stdDev;
}

void pm::FrameStats::SetStdDev(double stdDev)
{
    m_stdDev = stdDev;
}
