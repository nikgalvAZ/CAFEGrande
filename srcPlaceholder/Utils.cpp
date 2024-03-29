#ifdef _MSC_VER
// Suppress warning C4996 on strncpy functions
#define _CRT_SECURE_NO_WARNINGS
#endif

/******************************************************************************/
/* Copyright (C) Teledyne Photometrics. All rights reserved.                  */
/******************************************************************************/
#include "backend/Utils.h"

/* System */
#include <algorithm>
#include <cctype>
#include <functional>
#include <map>

#if defined(_WIN32)
    #include <Windows.h>
#elif defined(__linux__)
    #include <dirent.h> // opendir, closedir, readdir
    #include <fstream> // std::ifstream
    #include <limits> // std::numeric_limits
    #include <unistd.h> // stat
    #include <sys/stat.h> // stat
    #include <sys/types.h> // opendir, closedir, stat
#else
    #error Unsupported OS
#endif

bool pm::Utils::StrToBool(const std::string& str, bool& value)
{
    if (str.empty())
        return false;

    std::string tmpStr = str;
    // Without ICU library we can only assume the string contains ASCII chars only
    std::transform(tmpStr.begin(), tmpStr.end(), tmpStr.begin(),
            [](char c) { return static_cast<char>(::tolower(c)); });

    static const std::map<std::string, bool> validValues = {
        { "0",      false },
        { "false",  false },
        { "off",    false },
        { "no",     false },
        { "1",      true },
        { "true",   true },
        { "on",     true },
        { "yes",    true }
    };

    if (validValues.count(tmpStr) == 0)
        return false;

    value = validValues.at(tmpStr);
    return true;
}

std::string& pm::Utils::TrimLeft(std::string& str)
{
    str.erase(str.begin(), std::find_if(str.begin(), str.end(),
            std::ptr_fun<int, int>(std::isgraph)));
    return str;
}

std::string& pm::Utils::TrimRight(std::string& str)
{
    str.erase(std::find_if(str.rbegin(), str.rend(),
            std::ptr_fun<int, int>(std::isgraph)).base(), str.end());
    return str;
}

std::string& pm::Utils::Trim(std::string& str)
{
    return TrimLeft(TrimRight(str));
}

char* pm::Utils::CopyString(char* dst, const char* src, size_t len)
{
    return strncpy(dst, src, len);
}

#if defined(__linux__)
namespace pm {
bool GetProcMemInfo(size_t* total, size_t* avail)
{
    std::ifstream file("/proc/meminfo");
    if (!file)
        return false;

    // Both function arguments are optional, we don't search those that are null
    bool foundTotal = (total == nullptr);
    bool foundAvail = (avail == nullptr);

    size_t tmpAvail = 0; // Used in case MemAvailable not supported
    bool foundAvail1 = false;
    bool foundAvail2 = false;
    bool foundAvail3 = false;
    bool foundAvail4 = false;

    std::string token;
    size_t mem;

    while (!foundTotal || !foundAvail) {
        if (!(file >> token))
            return false;
        if (!(file >> mem))
            return false;
        // Ignore rest of the line
        file.ignore(std::numeric_limits<std::streamsize>::max(), '\n');

        if (!foundTotal) {
            if (token == "MemTotal:") {
                *total = mem * 1024; // Convert kB to bytes
                foundTotal = true;
                continue;
            }
        }
        if (!foundAvail) {
            if (token == "MemAvailable:") {
                // MemAvailable is available with newer kernels only.
                *avail = mem * 1024; // Convert kB to bytes
                foundAvail = true;
                continue;
            }
            else {
                // For older kernels we sum 1-MemFree, 2-Active(file),
                // 3-Inactive(file) and 4-SReclaimable.
                // Value of "low" watermarks from /proc/zoneinfo is ignored.
                if (token == "MemFree:") {
                    tmpAvail += mem * 1024; // Convert kB to bytes
                    foundAvail1 = true;
                }
                else if (token == "Active(file):") {
                    tmpAvail += mem * 1024; // Convert kB to bytes
                    foundAvail2 = true;
                }
                else if (token == "Inactive(file):") {
                    tmpAvail += mem * 1024; // Convert kB to bytes
                    foundAvail3 = true;
                }
                else if (token == "SReclaimable:") {
                    tmpAvail += mem * 1024; // Convert kB to bytes
                    foundAvail4 = true;
                }
                foundAvail = foundAvail1 && foundAvail2 && foundAvail3 && foundAvail4;
                if (foundAvail) {
                    *avail = tmpAvail;
                    continue;
                }
            }
        }
    }
    return foundTotal && foundAvail;
}
} // namespace pm
#endif

size_t pm::Utils::GetTotalRamMB()
{
#if defined(_WIN32)

    MEMORYSTATUSEX status;
    status.dwLength = sizeof(status);
    GlobalMemoryStatusEx(&status);
    // Right shift by 20 bytes converts bytes to megabytes
    return (size_t)(status.ullTotalPhys >> 20);

#elif defined(__linux__)

    size_t total;
    const bool ok = pm::GetProcMemInfo(&total, nullptr);
    return (ok) ? (total >> 20) : 0;

#endif
}

size_t pm::Utils::GetAvailRamMB()
{
#if defined(_WIN32)

    MEMORYSTATUSEX status;
    status.dwLength = sizeof(status);
    GlobalMemoryStatusEx(&status);
    // Right shift by 20 bytes converts bytes to megabytes
    return (size_t)(status.ullAvailPhys >> 20);

#elif defined(__linux__)

    size_t avail;
    const bool ok = pm::GetProcMemInfo(nullptr, &avail);
    return (ok) ? (avail >> 20) : 0;

#endif
}

std::vector<std::string> pm::Utils::GetFiles(const std::string& dir, const std::string& ext)
{
    std::vector<std::string> files;

#if defined(_WIN32)

    WIN32_FIND_DATAA fdFile;
    const std::string filter = dir + "/*" + ext;
    HANDLE hFind = FindFirstFileA(filter.c_str(), &fdFile);
    if (hFind != INVALID_HANDLE_VALUE)
    {
        do
        {
            const std::string fullFileName = dir + "/" + fdFile.cFileName;
            /* Ignore folders */
            if ((fdFile.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY))
                continue;
            /* Check file name ends with extension */
            const size_t extPos = fullFileName.rfind(ext);
            if (extPos == std::string::npos
                || extPos + ext.length() != fullFileName.length())
                continue;
            files.push_back(fullFileName);
        }
        while (FindNextFileA(hFind, &fdFile));

        FindClose(hFind);
    }

#elif defined(__linux__)

    DIR* dr = opendir(dir.c_str());
    if (dr)
    {
        struct dirent* ent;
        struct stat st;
        while ((ent = readdir(dr)) != NULL)
        {
            const std::string fileName = ent->d_name;
            const std::string fullFileName = dir + "/" + fileName;
            /* Ignore folders */
            if (stat(fullFileName.c_str(), &st) != 0)
                continue;
            if (st.st_mode & S_IFDIR)
                continue;
            /* Check file extension */
            const size_t extPos = fileName.rfind(ext);
            if (extPos == std::string::npos
                    || extPos + ext.length() != fileName.length())
                continue;
            files.push_back(fullFileName);
        }
        closedir(dr);
    }

#endif

    return files;
}
