/******************************************************************************/
/* Copyright (C) Teledyne Photometrics. All rights reserved.                  */
/******************************************************************************/
#pragma once
#ifndef PM_COLOR_UTILS_H
#define PM_COLOR_UTILS_H

/* System */
#include <cstddef> // size_t

struct ph_color_context;

namespace pm {

class ColorUtils final
{
public:
    // Logs last error message from color helper library
    static void LogError(const char* message);
    // Assigns one color context to another, allocates or releases it as needed
    static bool AssignContexts(ph_color_context** dst,
            const ph_color_context* src);
    // Compares all public members of color contexts, returns true if equal
    static bool CompareContexts(const ph_color_context* lhs,
            const ph_color_context* rhs);
    // Allocates buffer via color helper library if available,
    // otherwise uses new operator
    static void* AllocBuffer(size_t bufferBytes);
    // Releases buffer allocated by AllocColorBuffer
    static void FreeBuffer(void** buffer);
};

} // namespace

#endif
