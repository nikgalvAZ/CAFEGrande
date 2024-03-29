/******************************************************************************/
/* Copyright (C) Teledyne Photometrics. All rights reserved.                  */
/******************************************************************************/
#pragma once
#ifndef PM_FRAME_STATS_H
#define PM_FRAME_STATS_H

/* System */
#include <cstdint>

namespace pm {

class FrameStats
{
public:
    explicit FrameStats();

public:
    void Clear();

    // Stats are empty if there is no pixel
    bool IsEmpty() const;

    // Assigns given values and calculates remaining
    void SetViaSums(uint32_t pixelCount, double min, double max,
            double sum, double sumSq);
    // Assigns given values and calculates remaining
    void SetDirectly(uint32_t pixelCount, double min, double max,
            double mean, double secondMoment);

    // Combines particular results with algorithm described here:
    // https://en.wikipedia.org/wiki/Algorithms_for_calculating_variance#Parallel_algorithm
    void Add(const FrameStats& stats);

    uint32_t GetPixelCount() const;
    void SetPixelCount(uint32_t pixelCount);

    double GetMin() const;
    void SetMin(double min);

    double GetMax() const;
    void SetMax(double max);

    double GetMean() const;
    void SetMean(double mean);

    double GetSum() const;
    void SetSum(double sum);

    double GetSumSq() const;
    void SetSumSq(double sumSq);

    double GetSecondMoment() const;
    void SetSecondMoment(double m2);

    double GetVariance() const;
    void SetVariance(double variance);

    double GetStdDev() const;
    void SetStdDev(double stdDev);

private:
    uint32_t m_pixelCount;

    double m_min;
    double m_max;
    double m_mean;

    double m_sum;
    double m_sumSq;
    double m_secondMoment;

    double m_variance;
    double m_stdDev;
};

} // namespace

#endif
