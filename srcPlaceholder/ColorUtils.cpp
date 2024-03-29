/******************************************************************************/
/* Copyright (C) Teledyne Photometrics. All rights reserved.                  */
/******************************************************************************/
#include "backend/ColorUtils.h"

/* Local */
#include "backend/ColorRuntimeLoader.h"
#include "backend/Log.h"

/* System */
#include <cassert>

void pm::ColorUtils::LogError(const char* message)
{
    if (PH_COLOR)
    {
        char errMsg[PH_COLOR_MAX_ERROR_LEN];
        uint32_t errMsgSize = PH_COLOR_MAX_ERROR_LEN;
        PH_COLOR->get_last_error_message(errMsg, &errMsgSize);
        Log::LogE("%s (%s)", message, errMsg);
    }
    else
    {
        Log::LogE("%s", message);
    }
}

bool pm::ColorUtils::AssignContexts(ph_color_context** dst,
        const ph_color_context* src)
{
    if (!dst)
        return false;
    if (!(*dst) && !src)
        return true;
    if (!PH_COLOR)
        return false;
    if (!src)
    {
        PH_COLOR->context_release(dst);
        return true;
    }
    if (!(*dst))
    {
        if (PH_COLOR_ERROR_NONE != PH_COLOR->context_create(dst))
        {
            LogError("Failure initializing color helper context");
            return false;
        }
    }
    (*dst)->algorithm        = src->algorithm;
    (*dst)->pattern          = src->pattern;
    (*dst)->bitDepth         = src->bitDepth;
    (*dst)->rgbFormat        = src->rgbFormat;
    (*dst)->redScale         = src->redScale;
    (*dst)->greenScale       = src->greenScale;
    (*dst)->blueScale        = src->blueScale;
    (*dst)->autoExpAlgorithm = src->autoExpAlgorithm;
    (*dst)->forceCpu         = src->forceCpu;
    (*dst)->sensorWidth      = src->sensorWidth;
    (*dst)->sensorHeight     = src->sensorHeight;
    (*dst)->alphaValue       = src->alphaValue;
    return true;
}

bool pm::ColorUtils::CompareContexts(const ph_color_context* lhs,
        const ph_color_context* rhs)
{
    if (!lhs || !rhs)
        return lhs == rhs;
    return lhs->algorithm        == rhs->algorithm
        && lhs->pattern          == rhs->pattern
        && lhs->bitDepth         == rhs->bitDepth
        && lhs->rgbFormat        == rhs->rgbFormat
        && lhs->redScale         == rhs->redScale
        && lhs->greenScale       == rhs->greenScale
        && lhs->blueScale        == rhs->blueScale
        && lhs->autoExpAlgorithm == rhs->autoExpAlgorithm
        && lhs->forceCpu         == rhs->forceCpu
        && lhs->sensorWidth      == rhs->sensorWidth
        && lhs->sensorHeight     == rhs->sensorHeight
        && lhs->alphaValue       == rhs->alphaValue;
}

void* pm::ColorUtils::AllocBuffer(size_t bufferBytes)
{
    assert(bufferBytes > 0);
    assert(bufferBytes <= std::numeric_limits<uint32_t>::max());

    void* buffer = nullptr;
    if (PH_COLOR)
    {
        if (PH_COLOR_ERROR_NONE != PH_COLOR->buffer_alloc(&buffer,
                    static_cast<uint32_t>(bufferBytes)))
        {
            LogError("Unable to allocate RGB buffer");
            throw std::bad_alloc();
        }
    }
    else
    {
        buffer = new uint8_t[bufferBytes];
    }
    return buffer;
}

void pm::ColorUtils::FreeBuffer(void** buffer)
{
    if (!buffer)
        return;

    if (PH_COLOR)
    {
        PH_COLOR->buffer_free(buffer);
    }
    else
    {
        delete [] static_cast<uint8_t*>(*buffer); // Cast back to original type
        *buffer = nullptr;
    }
}
