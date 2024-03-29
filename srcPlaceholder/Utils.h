/******************************************************************************/
/* Copyright (C) Teledyne Photometrics. All rights reserved.                  */
/******************************************************************************/
#pragma once
#ifndef _UTILS_H
#define _UTILS_H

/* System */
#include <cassert>
#include <cstdint>
#include <cstring>
#include <limits>
#include <sstream>
#include <string>
#include <vector>

/* Helper macro that makes multi-line function-like macros more safe
   Usage example:
    #define MULTI_LINE_FUNC_LIKE_MACRO(a,b,c) do {\
        <code>
    } ONCE
*/
#if defined(_WIN32)
  #define ONCE\
    __pragma(warning(push))\
    __pragma(warning(disable:4127))\
    while (0)\
    __pragma(warning(pop))
#else
  #define ONCE while (0)
#endif

/* Platform independent macro to avoid compiler warnings for unreferenced
   function parameters. It has the same effect as Win32 UNREFERENCED_PARAMETER
   Usage example:
       void do_magic(int count)
       {
           UNUSED(count);
       }
*/
#define UNUSED(expr) do { (void)(expr); } ONCE

namespace pm {

class Utils
{
public:
    // Converts string to signed integral number of given type
    template<typename T>
    static bool StrToNumber(const std::string& str,
            typename std::enable_if_t<
                std::is_integral<T>::value && std::is_signed<T>::value,
                T>& number)
    {
        try {
            size_t idx;
            const long long nr = std::stoll(str, &idx);
            if (idx == str.length()
                    && nr >= (long long)(std::numeric_limits<T>::min)()
                    && nr <= (long long)(std::numeric_limits<T>::max)())
            {
                number = (T)nr;
                return true;
            }
        }
        catch(...) {};
        return false;
    }

    // Converts string to unsigned integral number of given type
    template<typename T>
    static bool StrToNumber(const std::string& str,
            typename std::enable_if_t<
                std::is_integral<T>::value && std::is_unsigned<T>::value,
                T>& number)
    {
        try {
            size_t idx;
            const unsigned long long nr = std::stoull(str, &idx);
            if (idx == str.length()
                    && nr >= (unsigned long long)(std::numeric_limits<T>::min)()
                    && nr <= (unsigned long long)(std::numeric_limits<T>::max)())
            {
                number = (T)nr;
                return true;
            }
        }
        catch(...) {};
        return false;
    }

    // Converts string to float number of given type
    template<typename T>
    static bool StrToNumber(const std::string& str,
            typename std::enable_if_t<std::is_same<T, float>::value, T>& number)
    {
        try {
            size_t idx;
            const T nr = std::stof(str, &idx);
            if (idx == str.length())
            {
                number = nr;
                return true;
            }
        }
        catch(...) {};
        return false;
    }

    // Converts string to double number of given type
    template<typename T>
    static bool StrToNumber(const std::string& str,
            typename std::enable_if_t<std::is_same<T, double>::value, T>& number)
    {
        try {
            size_t idx;
            const T nr = std::stod(str, &idx);
            if (idx == str.length())
            {
                number = nr;
                return true;
            }
        }
        catch(...) {};
        return false;
    }

    // Converts string to boolean value
    static bool StrToBool(const std::string& str, bool& value);

    // Remove leading whitespaces in-place
    static std::string& TrimLeft(std::string& str);

    // Remove trailing whitespaces in-place
    static std::string& TrimRight(std::string& str);

    // Remove leading and trailing whitespaces in-place
    static std::string& Trim(std::string& str);

    // Splits string by given delimiter and converts to numbers
    template <typename T>
    static typename std::enable_if_t<std::is_arithmetic<T>::value, bool>
        StrToArray(std::vector<T>& arr, const std::string& string, char delimiter)
    {
        std::istringstream ss(string);
        std::vector<T> items;
        std::string item;
        while (std::getline(ss, item, delimiter))
        {
            auto str = Trim(item);
            T number;
            if (!StrToNumber<T>(str, number))
                return false;
            items.push_back(number);
        }
        arr.swap(items);
        return true;
    }

    // Splits string into sub-strings separated by given delimiter.
    // This specialization never fails.
    template<typename T>
    static typename std::enable_if_t<std::is_same<T, std::string>::value, bool>
        StrToArray(std::vector<T>& arr, const std::string& string, char delimiter)
    {
        std::istringstream ss(string);
        std::vector<T> items;
        std::string item;
        while (std::getline(ss, item, delimiter))
            items.push_back(item);
        arr.swap(items);
        return true;
    }
    static inline std::vector<std::string> StrToArray(const std::string& string,
            char delimiter)
    {
        std::vector<std::string> arr;
        StrToArray(arr, string, delimiter);
        return arr;
    }

    // Joins numbers converted to strings into one string using given delimiter
    template<typename T>
    static typename std::enable_if_t<std::is_arithmetic<T>::value, std::string>
        ArrayToStr(const std::vector<T>& arr, char delimiter)
    {
        std::string str;
        for (const T& item : arr)
            str += std::to_string(item) + delimiter;
        if (!arr.empty())
            str.pop_back(); // Remove delimiter at the end
        return str;
    }
    // Joins strings from vector into one string using given delimiter
    template<typename T>
    static typename std::enable_if_t<std::is_same<T, std::string>::value, std::string>
        ArrayToStr(const std::vector<T>& arr, char delimiter)
    {
        std::string str;
        for (const T& item : arr)
            str += item + delimiter;
        if (!arr.empty())
            str.pop_back(); // Remove delimiter at the end
        return str;
    }

    template <typename T>
    static typename std::enable_if_t<std::is_arithmetic<T>::value, std::string>
        ArrayToStr(const T arr[], size_t count, char delimiter)
    {
        std::string str;
        if (!arr || count == 0)
            return str;
        for (size_t n = 0; n < count; ++n)
            str += std::to_string(arr[n]) + delimiter;
        str.pop_back(); // Remove delimiter at the end
        return str;
    }

    // Simply strncpy-like function with suppressed warnings
    static char* CopyString(char* dst, const char* src, size_t len);

    // Type size_t limits size of total memory to 4096TB
    static size_t GetTotalRamMB();

    // Type size_t limits size of available memory to 4096TB
    static size_t GetAvailRamMB();

    // Return a list of file names in given folder with given extension
    static std::vector<std::string> GetFiles(const std::string& dir, const std::string& ext);

    // Converts fixed-point to real number
    template<typename R, typename U>
    static typename std::enable_if_t<std::is_floating_point<R>::value, R>
        FixedPointToReal(uint8_t integralBits, uint8_t fractionBits,
            typename std::enable_if_t<
                std::is_integral<U>::value && std::is_unsigned<U>::value,
                U> value)
    {
        const uint64_t val64 = static_cast<uint64_t>(value);

        const uint64_t intSteps = 1ULL << integralBits;
        const uint64_t fractSteps = 1ULL << fractionBits;

        const uint64_t intMask = intSteps - 1ULL;
        const uint64_t fractMask = fractSteps - 1ULL;

        const R intPart = static_cast<R>((val64 >> fractionBits) & intMask);
        const R fractPart = (val64 & fractMask) / static_cast<R>(fractSteps);

        const R real = intPart + fractPart;
        return real;
    }

    // Converts real number to fixed-point
    template<typename R, typename U>
    static typename std::enable_if_t<
            std::is_integral<U>::value && std::is_unsigned<U>::value,
            U>
        RealToFixedPoint(uint8_t integralBits, uint8_t fractionBits,
            typename std::enable_if_t<std::is_floating_point<R>::value, R> value)
    {
        //const uint64_t intSteps = 1ULL << integralBits;
        const uint64_t fractSteps = 1ULL << fractionBits;
        const uint64_t steps = 1ULL << (integralBits + fractionBits);

        const uint64_t mask = steps - 1ULL;

        const U fixedPoint = static_cast<U>(value * fractSteps) & mask;
        return fixedPoint;
    }

    // TODO: With C++17 replace by std::clamp
    template<typename T>
    static const T& Clamp(const T& v, const T& lo, const T& hi)
    {
        return assert(lo <= hi), (v <= lo) ? lo : (v >= hi) ? hi : v;
    }
};

} // namespace

#endif
