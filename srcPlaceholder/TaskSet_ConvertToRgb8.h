/******************************************************************************/
/* Copyright (C) Teledyne Photometrics. All rights reserved.                  */
/******************************************************************************/
#pragma once
#ifndef PM_TASK_SET_CONVERT_TO_RGB8_H
#define PM_TASK_SET_CONVERT_TO_RGB8_H

/* Local */
#include "backend/BitmapFormat.h"
#include "backend/Task.h"
#include "backend/TaskSet.h"

/* System */
#include <memory>

namespace pm {

class Bitmap;

class TaskSet_ConvertToRgb8 : public TaskSet
{
private:
    class ATask : public Task
    {
    public:
        explicit ATask(std::shared_ptr<Semaphore> semDone,
                size_t taskIndex, size_t taskCount);

    public:
        void SetUp(const Bitmap* dstBmp, const Bitmap* srcBmp,
            double srcMin, double srcMax, bool autoConbright, int brightness,
            int contrast, const std::vector<uint8_t>* pixLookupMap);

    public: // Task
        virtual void Execute() override;

    private:
        template<typename T>
        void ExecuteT();
        template<typename T>
        void ExecuteT_Lookup();

    private:
        size_t m_maxTasks{ 0 };
        // Cannot be const to auto-generate assignment operator
        Bitmap* m_dstBmp{ nullptr };
        // Cannot be const to auto-generate assignment operator
        Bitmap* m_srcBmp{ nullptr };
        double m_srcMin{ 0.0 };
        double m_srcMax{ 0.0 };
        bool m_autoConbright{ true };
        int m_brightness{ 0 };
        int m_contrast{ 0 };
        // Cannot be const to auto-generate assignment operator
        std::vector<uint8_t>* m_pixLookupMap{ nullptr };
    };

public:
    explicit TaskSet_ConvertToRgb8(std::shared_ptr<ThreadPool> pool);

public:
    // Does either automatic or manual contrast and brightness adjustment.
    // brightness value is from -255 to +255, 0 means no change.
    // contrast value is from -255 to +255, 0 means no change.
    // Lookup map can be used to speed up conversion from 8 and 16 bit pixels only.
    void SetUp(const Bitmap* dstBmp,
            const Bitmap* srcBmp, double srcMin, double srcMax,
            const std::vector<uint8_t>* pixLookupMap = nullptr,
            bool autoConbright = true, int brightness = 0, int contrast = 0);

public:
    static uint8_t ConvertOnePixel(double srcValue, double srcMin, double srcMax,
            bool autoConbright = true, int brightness = 0, int contrast = 0);

    static void UpdateLookupMap(std::vector<uint8_t>& lookupMap,
            const BitmapFormat& srcBmpFormat, double srcMin, double srcMax,
            bool autoConbright, int brightness, int contrast);

private:
    BitmapFormat m_dstFormat{};
    BitmapFormat m_srcFormat{};
    uint16_t m_srcMin{ 0 };
    uint16_t m_srcMax{ 0 };
    bool m_autoConbright{ true };
    int m_brightness{ 0 };
    int m_contrast{ 0 };
};

} // namespace pm

#endif /* PM_TASK_SET_CONVERT_TO_RGB8_H */
