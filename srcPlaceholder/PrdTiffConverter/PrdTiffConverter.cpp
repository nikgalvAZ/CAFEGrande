/******************************************************************************/
/* Copyright (C) Teledyne Photometrics. All rights reserved.                  */
/******************************************************************************/

/* Local */
#include <backend/ColorRuntimeLoader.h>
#include <backend/ColorUtils.h>
#include "backend/ConsoleLogger.h"
#include "backend/Frame.h"
#include "backend/FrameProcessor.h"
#include "backend/Log.h"
#include "backend/OptionController.h"
#include "backend/PrdFileLoad.h"
#include "backend/PrdFileUtils.h"
#include <backend/PvcamRuntimeLoader.h>
#include "backend/TiffFileSave.h"
#include "backend/Utils.h"
#include "version.h"

/* PVCAM */
#include "master.h"
#include "pvcam.h"

/* System */
#include <algorithm>
#include <chrono>
#include <cstring>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <locale>
#include <map>
#include <sstream>
#include <string>
#include <thread>
#include <vector>

#if defined(_WIN32)
    #include <Windows.h>
#else
    #include <cstring>
    #include <signal.h>
#endif

const std::string prdExt(".prd");
const std::string tiffExt(".tiff");
const std::string csvExt(".csv");
const char csvDelim(',');

constexpr int APP_SUCCESS = 0;
constexpr int APP_ERR_HOOKS = 1;
constexpr int APP_ERR_CLI_ARGS = 2;
constexpr int APP_ERR_RUN = 3;
constexpr int APP_ERR_LIB_LOAD = 4;

// Custom CLI options
static constexpr uint32_t OptionId_Dir =
    static_cast<uint32_t>(pm::OptionId::CustomBase) + 0;
static constexpr uint32_t OptionId_Mode =
    static_cast<uint32_t>(pm::OptionId::CustomBase) + 1;
static constexpr uint32_t OptionId_Csv =
    static_cast<uint32_t>(pm::OptionId::CustomBase) + 2;

// Global flag saying if user wants to abort current operation
std::atomic<bool> g_userAbortFlag(false);

class Helper final
{
public:
    enum class TiffMode {
        None,
        Single,
        Stack,
        BigStack,
    };

public:
    Helper(int argc, char* argv[]);
    ~Helper();

public:
    bool InstallTerminationHandlers();

    bool ProcessCliOptions();
    void ShowHelp(); // Show help text if some set

    int RunConversion();

private: // CLI option handlers
    bool HandleHelp(const std::string& value);
    bool HandleFolder(const std::string& value);
    bool HandleTiffMode(const std::string& value);
    bool HandleTiffOptFull(const std::string& value);
    bool HandleCsvParticles(const std::string& value);

private:
    void SetHelpText(const std::vector<pm::Option>& options);

    bool UpdateHelperColorContext(const PrdHeader& header);
    bool UpdateHelperBitmap(const PrdHeader& header);

    bool ExportTiffs_Single(const std::string& prdFileName,
            const std::string& outFileBaseName);
    bool ExportTiffs_Stack(const std::string& prdFileName,
            const std::string& outFileBaseName, bool useBigTiff);

    bool ExportCsvs_Particles(const std::string& prdFileName,
            const std::string& outFileBaseName);
    bool ExportCsv_Particles(const std::string& outFileName, pm::Frame& frame);

private:
    int m_appArgC;
    char** m_appArgV;

    pm::OptionController m_optionController{};
    pm::Option m_helpOption;
    bool m_showFullHelp{ false };
    std::string m_helpText{};
    std::string m_folder{ "." };
    TiffMode m_tiffMode{ TiffMode::Single };
    bool m_tiffOptFull{ false };
    bool m_csvParticles{ false };

    pm::TiffFileSave::Helper m_tiffHelper{};
    pm::FrameProcessor m_frameProc{};
    ph_color_context* m_colorCtx{ nullptr };
};

Helper::Helper(int argc, char* argv[])
    : m_appArgC(argc),
    m_appArgV(argv),
    m_helpOption(
            { "-Help", "-help", "--help", "-h", "/?" },
            { "" },
            { "false" },
            "Shows description for all supported options.",
            static_cast<uint32_t>(pm::OptionId::Help),
            std::bind(&Helper::HandleHelp, this, std::placeholders::_1))
{
    m_tiffHelper.frameProc = &m_frameProc;

    // TODO: Move ImageDlg::FillMethod to backend and add CLI option
    //       to allow at least fill by mean value
    m_tiffHelper.fillValue = 0.0; // Black-fill
}

Helper::~Helper()
{
    delete m_tiffHelper.fullBmp;

    if (PH_COLOR && m_colorCtx)
    {
        PH_COLOR->context_release(&m_colorCtx);
    }
}

#if defined(_WIN32)
static BOOL WINAPI ConsoleCtrlHandler(DWORD dwCtrlType)
{
    /* Return TRUE if handled this message, further handler functions won't be called.
       Return FALSE to pass this message to further handlers until default handler
       calls ExitProcess(). */

    switch (dwCtrlType)
    {
    case CTRL_C_EVENT: // Ctrl+C
    case CTRL_BREAK_EVENT: // Ctrl+Break
    case CTRL_CLOSE_EVENT: // Closing the console window
    case CTRL_LOGOFF_EVENT: // User logs off. Passed only to services!
    case CTRL_SHUTDOWN_EVENT: // System is shutting down. Passed only to services!
        break;
    default:
        pm::Log::LogE("Unknown console control type!");
        return FALSE;
    }

    pm::Log::LogI("\n>>> Processing abort requested\n");
    g_userAbortFlag = true;

    return TRUE;
}
#else
static void TerminalSignalHandler(int sigNum)
{
    pm::Log::LogI("\n>>> Processing abort requested\n");
    g_userAbortFlag = true;
}
#endif

// Sets handlers that properly end acquisition on Ctrl+C, Ctrl+Break, Log-off, etc.
bool Helper::InstallTerminationHandlers()
{
    bool retVal;

#if defined(_WIN32)
    retVal = (TRUE == SetConsoleCtrlHandler(ConsoleCtrlHandler, TRUE));
#else
    struct sigaction newAction;
    memset(&newAction, 0, sizeof(newAction));
    newAction.sa_handler = TerminalSignalHandler;
    retVal = true;
    if (0 != sigaction(SIGINT, &newAction, NULL)
            || 0 != sigaction(SIGHUP, &newAction, NULL)
            || 0 != sigaction(SIGTERM, &newAction, NULL))
        retVal = false;
#endif

    if (!retVal)
        pm::Log::LogE("Unable to install termination handler(s)!");

    return retVal;
}

bool Helper::ProcessCliOptions()
{
    // Add Help option first (the variable is needed later)
    if (!m_optionController.AddOption(m_helpOption))
        return false;

    if (!m_optionController.AddOption(pm::Option(
            { "-d", "--dir", "--folder" },
            { "directory" },
            { m_folder },
            "Processes PRD files on disk in given directory.\n"
            "If empty string is given current working directory is used\n"
            "(as if user would enter relative path '.').",
            OptionId_Dir,
            std::bind(&Helper::HandleFolder, this, std::placeholders::_1))))
        return false;

    if (!m_optionController.AddOption(pm::Option(
            { "-m", "--mode", "--tiff-mode" },
            { "mode" },
            { "single" },
            "States how TIFF files should be generated.\n"
            "Supported values are : 'single', 'stack', 'big-stack' and 'none'.\n"
            "'single' mode:\n"
            "  For PRD files with multiple frames in it are generated multiple\n"
            "  TIFF files, i.e. each frame is extracted to separate file.\n"
            "'stack' mode:\n"
            "  For PRD files with multiple frames in it are Classic TIFF files\n"
            "  generated with multiple pages, i.e. 1 to 1 mapping.\n"
            "'big-stack' mode:\n"
            "  For PRD files with multiple frames in it are BIG TIFF files\n"
            "  generated with multiple pages, i.e. 1 to 1 mapping.\n"
            "'none' mode:\n"
            "  No TIFF files are generated.",
            OptionId_Mode,
            std::bind(&Helper::HandleTiffMode, this, std::placeholders::_1))))
        return false;

    if (!m_optionController.AddOption(pm::Option(
            { "--tiff-opt-full" },
            { "" },
            { "false" },
            "If 'true', saves fully processed images if selected format is 'tiff' or 'big-tiff'.\n"
            "By default TIFF file contains unaltered raw pixel data that require additional\n"
            "processing like debayering or white-balancing.",
            static_cast<uint32_t>(pm::OptionId::SaveTiffOptFull),
            std::bind(&Helper::HandleTiffOptFull, this, std::placeholders::_1))))
        return false;

    if (!m_optionController.AddOption(pm::Option(
            { "--csv-particles" },
            { "" },
            { "false" },
            "Exports metadata related to particles to separate CSV file.\n"
            "For PRD files with multiple frames in it are generated multiple\n"
            "CSV files, i.e. data for each frame goes to separate file.",
            OptionId_Csv,
            std::bind(&Helper::HandleCsvParticles, this, std::placeholders::_1))))
        return false;

    const auto& cliAllOptions = m_optionController.GetOptions();
    const bool cliParseOk = m_optionController.ProcessOptions(
            m_appArgC, m_appArgV, cliAllOptions);
    if (!cliParseOk || m_showFullHelp)
    {
        SetHelpText((m_showFullHelp)
                ? cliAllOptions
                : m_optionController.GetFailedProcessedOptions());
        return cliParseOk;
    }

    return true;
}

void Helper::ShowHelp()
{
    if (m_helpText.empty())
        return;

    pm::Log::LogI("\n%s", m_helpText.c_str());
}

int Helper::RunConversion()
{
    if (m_showFullHelp)
        return APP_SUCCESS;

    if (m_tiffMode == TiffMode::None && !m_csvParticles)
    {
        pm::Log::LogW("No actions specified.");
        return APP_SUCCESS;
    }

    const std::vector<std::string> fileNames = pm::Utils::GetFiles(m_folder, prdExt);
    if (fileNames.empty())
    {
        pm::Log::LogI("No files match '%s/*'", m_folder.c_str());
        return APP_SUCCESS;
    }

    pm::Log::LogI("Processing files in folder '%s'", m_folder.c_str());

    for (auto& inFileName : fileNames)
    {
        // Prepare output file base name - remove PRD extension
        std::string outFileBaseName(inFileName);
        const std::size_t prdExtPos = outFileBaseName.rfind(prdExt);
        if (prdExtPos != std::string::npos)
            outFileBaseName.erase(prdExtPos);

        pm::Log::LogI("Processing '%s'", inFileName.c_str());

        switch (m_tiffMode)
        {
        case TiffMode::Single:
            if (!ExportTiffs_Single(inFileName, outFileBaseName))
                return APP_ERR_RUN;
            break;
        case TiffMode::Stack:
        case TiffMode::BigStack:
            if (!ExportTiffs_Stack(inFileName, outFileBaseName,
                        m_tiffMode == TiffMode::BigStack))
                return APP_ERR_RUN;
            break;
        case TiffMode::None:
            break; // Just to silent GCC warning
        }

        if (m_csvParticles)
        {
            if (!ExportCsvs_Particles(inFileName, outFileBaseName))
                return APP_ERR_RUN;
        }
    }

    return APP_SUCCESS;
}

bool Helper::HandleHelp(const std::string& value)
{
    if (value.empty())
    {
        m_showFullHelp = true;
    }
    else
    {
        if (!pm::Utils::StrToBool(value, m_showFullHelp))
            return false;
    }
    return true;
}

bool Helper::HandleFolder(const std::string& value)
{
    m_folder = (value.empty()) ? "." : value;
    return true;
}

bool Helper::HandleTiffMode(const std::string& value)
{
    TiffMode tiffMode;
    if (value == "none")
        tiffMode = TiffMode::None;
    else if (value == "single")
        tiffMode = TiffMode::Single;
    else if (value == "stack")
        tiffMode = TiffMode::Stack;
    else if (value == "big-stack")
        tiffMode = TiffMode::BigStack;
    else
        return false;

    m_tiffMode = tiffMode;
    return true;
}

bool Helper::HandleTiffOptFull(const std::string& value)
{
    if (value.empty())
    {
        m_tiffOptFull = true;
    }
    else
    {
        if (!pm::Utils::StrToBool(value, m_tiffOptFull))
            return false;
    }

    return true;
}

bool Helper::HandleCsvParticles(const std::string& value)
{
    if (value.empty())
    {
        m_csvParticles = true;
    }
    else
    {
        if (!pm::Utils::StrToBool(value, m_csvParticles))
            return false;
    }

    return true;
}

void Helper::SetHelpText(const std::vector<pm::Option>& options)
{
    m_helpText  = "Usage\n";
    m_helpText += "=====\n";
    m_helpText += "\n";
    m_helpText += "This CLI application converts PRD files in given folder to TIFF\n";
    m_helpText += "files and can export additional metadata to separate files.\n";
    m_helpText += "Run without or with almost any combination of options listed below.\n";
    m_helpText += "\n";
    m_helpText += "Return value\n";
    m_helpText += "------------\n";
    m_helpText += "\n";
    m_helpText += "  " + std::to_string(APP_SUCCESS)      + " - Application exited without any error.\n";
    m_helpText += "  " + std::to_string(APP_ERR_HOOKS)    + " - Error while setting termination hooks (e.g. for ctrl+c).\n";
    m_helpText += "  " + std::to_string(APP_ERR_CLI_ARGS) + " - Error while parsing CLI options.\n";
    m_helpText += "  " + std::to_string(APP_ERR_RUN)      + " - Failure while processing.\n";
    m_helpText += "  " + std::to_string(APP_ERR_LIB_LOAD) + " - Mandatory library not loaded at run-time.\n";
    m_helpText += "\n";

    m_helpText += m_optionController.GetOptionsDescription(options);

    if (std::find_if(options.cbegin(), options.cend(),
                [](const pm::Option& o) {
                    return o.GetId() == static_cast<uint32_t>(pm::OptionId::Help);
                })
            == options.cend())
    {
        m_helpText += m_optionController.GetOptionsDescription(
                { m_helpOption }, false);
    }
}

bool Helper::UpdateHelperColorContext(const PrdHeader& header)
{
    // Turn off debayering
    m_tiffHelper.colorCtx = nullptr;

    if (!m_tiffOptFull)
        return true; // No debayering requested
    if (header.version < PRD_VERSION_0_3)
        return true; // No debayering possible
    if (header.colorMask == COLOR_NONE) // Since v0.3
        return true; // No debayering needed
    if (header.region.sbin != 1 || header.region.pbin != 1)
        return true; // No debayering needed

    int32_t rgbFormat = PH_COLOR_RGB_FORMAT_RGB48;
    if (header.version >= PRD_VERSION_0_6)
    {
        switch (header.imageFormat) // Since v0.6
        {
        case PL_IMAGE_FORMAT_BAYER8:
            rgbFormat = PH_COLOR_RGB_FORMAT_RGB24;
            break;
        case PL_IMAGE_FORMAT_BAYER16:
            rgbFormat = PH_COLOR_RGB_FORMAT_RGB48;
            break;
        default:
            pm::Log::LogE("Color processing not supported for current image format");
            return false;
        }
    }

    if (!PH_COLOR)
    {
        pm::Log::LogE("Color helper library not loaded to process color frame. "
                "Remove --tiff-opt-full option to save it non-debayered.");
        return false;
    }

    if (!m_colorCtx)
    {
        if (PH_COLOR_ERROR_NONE != PH_COLOR->context_create(&m_colorCtx))
        {
            pm::ColorUtils::LogError("Failure initializing color helper context");
            return false;
        }
    }

    const auto rgn = header.region;
    const uint16_t rgnW = (rgn.s2 + 1 - rgn.s1) / rgn.sbin;
    const uint16_t rgnH = (rgn.p2 + 1 - rgn.p1) / rgn.pbin;

    if (m_colorCtx->pattern != header.colorMask
            || m_colorCtx->bitDepth != header.bitDepth
            || m_colorCtx->rgbFormat != rgbFormat
            || m_colorCtx->sensorWidth != rgnW
            || m_colorCtx->sensorHeight != rgnH)
    {
        m_colorCtx->pattern = header.colorMask;
        m_colorCtx->bitDepth = header.bitDepth;
        m_colorCtx->rgbFormat = rgbFormat;
        m_colorCtx->sensorWidth = rgnW;
        m_colorCtx->sensorHeight = rgnH;

        if (PH_COLOR_ERROR_NONE != PH_COLOR->context_apply_changes(m_colorCtx))
        {
            pm::ColorUtils::LogError("Failure applying color context changes");
            return false;
        }
    }

    // Turn on debayering
    m_tiffHelper.colorCtx = m_colorCtx;
    return true;
}

bool Helper::UpdateHelperBitmap(const PrdHeader& header)
{
    const auto rgn = header.region;
    const uint32_t bmpW = ((uint32_t)rgn.s2 + 1 - rgn.s1) / rgn.sbin;
    const uint32_t bmpH = ((uint32_t)rgn.p2 + 1 - rgn.p1) / rgn.pbin;

    pm::BitmapFormat bmpFormat;
    bmpFormat.SetBitDepth(header.bitDepth);
    if (header.version >= PRD_VERSION_0_3)
    {
        bmpFormat.SetColorMask(static_cast<pm::BayerPattern>(header.colorMask));
    }
    if (header.version >= PRD_VERSION_0_6)
    {
        try
        {
            bmpFormat.SetImageFormat(static_cast<pm::ImageFormat>(header.imageFormat));
        }
        catch (...)
        {
            pm::Log::LogE("Failed allocation of internal bitmap format");
            return false;
        }
    }

    if (m_tiffHelper.colorCtx)
    {
        // TODO: Remove this restriction
        switch (bmpFormat.GetDataType())
        {
        case pm::BitmapDataType::UInt8:
        case pm::BitmapDataType::UInt16:
            break; // Supported types for color helper library
        default:
            pm::Log::LogE("Bitmap data type not supported by Color Helper library");
            return false;
        }

        bmpFormat.SetPixelType(pm::BitmapPixelType::RGB);
        bmpFormat.SetColorMask(
                static_cast<pm::BayerPattern>(m_tiffHelper.colorCtx->pattern));
    }
    else
    {
        bmpFormat.SetPixelType(pm::BitmapPixelType::Mono);
        bmpFormat.SetColorMask(pm::BayerPattern::None);
    }

    bool reallocateBmp = true;
    if (m_tiffHelper.fullBmp)
    {
        reallocateBmp = m_tiffHelper.fullBmp->GetFormat() != bmpFormat
            || m_tiffHelper.fullBmp->GetWidth() != bmpW
            || m_tiffHelper.fullBmp->GetHeight() != bmpH;
    }
    if (reallocateBmp)
    {
        delete m_tiffHelper.fullBmp;
        m_tiffHelper.fullBmp = new(std::nothrow) pm::Bitmap(bmpW, bmpH, bmpFormat);
        if (!m_tiffHelper.fullBmp)
        {
            pm::Log::LogE("Failure allocating internal bitmap");
            return false;
        }
    }

    return true;
}

bool Helper::ExportTiffs_Single(const std::string& prdFileName,
        const std::string& outFileBaseName)
{
    bool retVal = true;

    const void* rawData;
    const void* metaData;
    const void* extDynMetaData;

    pm::PrdFileLoad prdFile(prdFileName);
    if (!prdFile.Open())
    {
        pm::Log::LogE("Cannot open input file '%s', skipping",
                prdFileName.c_str());
        return false;
    }
    const PrdHeader& prdHeader = prdFile.GetHeader();

    if (!UpdateHelperColorContext(prdHeader))
        return false;

    if (!UpdateHelperBitmap(prdHeader))
        return false;

    for (uint32_t frameIndexInStack = 0;
            frameIndexInStack < prdHeader.frameCount; frameIndexInStack++)
    {
        if (!prdFile.ReadFrame(&metaData, &extDynMetaData, &rawData))
        {
            pm::Log::LogE("Cannot read frame for stack index %u, "
                    "skipping whole file", frameIndexInStack);
            retVal = false;
            break;
        }

        auto prdMetaData = static_cast<const PrdMetaData*>(metaData);

        if (prdMetaData->frameNumber == 0)
        {
            pm::Log::LogE("Invalid frame number for stack index %u, "
                    "skipping this frame", frameIndexInStack);
            retVal = false;
            continue;
        }

        // Complete TIFF file name
        const std::string outFileName = outFileBaseName + "_"
            + std::to_string(prdMetaData->frameNumber) + tiffExt;

        bool keepFile = true;

        PrdHeader tiffHeader = prdHeader;
        tiffHeader.frameCount = 1;
        pm::TiffFileSave tiffFile(outFileName, tiffHeader, &m_tiffHelper);
        if (!tiffFile.Open())
        {
            pm::Log::LogE("Cannot open output file '%s', "
                    "skipping this frame", outFileName.c_str());
            retVal = false;
            continue;
        }

        if (!tiffFile.WriteFrame(metaData, extDynMetaData, rawData))
        {
            pm::Log::LogE("Cannot write frame for stack index %u, "
                    "frame number %u, skipping this frame",
                    frameIndexInStack, prdMetaData->frameNumber);
            retVal = false;
            keepFile = false;
        }

        tiffFile.Close();

        if (keepFile)
        {
            pm::Log::LogI("Successfully created file '%s' for stack index %u, "
                    "frame number %u",
                    outFileName.c_str(), frameIndexInStack,
                    prdMetaData->frameNumber);
        }
        else
        {
            if (0 == std::remove(outFileName.c_str()))
            {
                pm::Log::LogI("Removed output file '%s'", outFileName.c_str());
            }
            else
            {
                pm::Log::LogE("Cannot remove output file '%s'", outFileName.c_str());
            }
        }
    }

    prdFile.Close();

    return retVal;
}

bool Helper::ExportTiffs_Stack(const std::string& prdFileName,
        const std::string& outFileBaseName, bool useBigTiff)
{
    bool retVal = true;
    bool keepFile = true;

    const void* rawData;
    const void* metaData;
    const void* extDynMetaData;

    pm::PrdFileLoad prdFile(prdFileName);
    if (!prdFile.Open())
    {
        pm::Log::LogE("Cannot open input file '%s', skipping",
                prdFileName.c_str());
        return false;
    }
    const PrdHeader& prdHeader = prdFile.GetHeader();

    if (!UpdateHelperColorContext(prdHeader))
        return false;

    if (!UpdateHelperBitmap(prdHeader))
        return false;

    // Complete TIFF file name
    const std::string outFileName = outFileBaseName + tiffExt;

    PrdHeader tiffHeader = prdHeader;
    pm::TiffFileSave tiffFile(outFileName, tiffHeader, &m_tiffHelper, useBigTiff);
    if (!tiffFile.Open())
    {
        pm::Log::LogE("Cannot open output file '%s', skipping",
                outFileName.c_str());
        retVal = false;
    }
    else
    {
        for (uint32_t frameIndexInStack = 0;
                frameIndexInStack < prdHeader.frameCount; frameIndexInStack++)
        {
            if (!prdFile.ReadFrame(&metaData, &extDynMetaData, &rawData))
            {
                pm::Log::LogE("Cannot read frame for stack index %u, "
                        "skipping whole file", frameIndexInStack);
                retVal = false;
                keepFile = false;
                break;
            }

            auto prdMetaData = static_cast<const PrdMetaData*>(metaData);

            if (prdMetaData->frameNumber == 0)
            {
                pm::Log::LogE("Invalid frame number for stack index %u, "
                        "skipping this frame", frameIndexInStack);
                retVal = false;
                continue;
            }

            if (!tiffFile.WriteFrame(metaData, extDynMetaData, rawData))
            {
                pm::Log::LogE("Cannot write frame for stack index %u, "
                        "frame number %u, skipping whole file",
                        frameIndexInStack, prdMetaData->frameNumber);
                retVal = true;
                keepFile = false;
                break;
            }
        }

        tiffFile.Close();

        if (keepFile)
        {
            pm::Log::LogI("Successfully created file '%s' with %u frame(s)",
                    outFileName.c_str(), tiffHeader.frameCount);
        }
        else
        {
            if (0 == std::remove(outFileName.c_str()))
            {
                pm::Log::LogI("Removed output file '%s'", outFileName.c_str());
            }
            else
            {
                pm::Log::LogE("Cannot remove output file '%s'", outFileName.c_str());
            }
        }
    }

    prdFile.Close();

    return retVal;
}

bool Helper::ExportCsvs_Particles(const std::string& prdFileName,
        const std::string& outFileBaseName)
{
    bool retVal = true;

    const void* rawData;
    const void* metaData;
    const void* extDynMetaData;

    pm::PrdFileLoad prdFile(prdFileName);
    if (!prdFile.Open())
    {
        pm::Log::LogE("Cannot open input file '%s', skipping",
                prdFileName.c_str());
        return false;
    }
    const PrdHeader& prdHeader = prdFile.GetHeader();

    if (prdHeader.version < PRD_VERSION_0_5)
    {
        pm::Log::LogI("Old PRD file version (%04x) without trajectory data, "
                "skipping whole file.", prdHeader.version);
        retVal = false;
    }
    else
    {
        for (uint32_t frameIndexInStack = 0;
                frameIndexInStack < prdHeader.frameCount; frameIndexInStack++)
        {
            if (!prdFile.ReadFrame(&metaData, &extDynMetaData, &rawData))
            {
                pm::Log::LogE("Cannot read frame for stack index %u, "
                        "skipping whole file", frameIndexInStack);
                retVal = false;
                break;
            }

            auto prdMetaData = static_cast<const PrdMetaData*>(metaData);

            if (prdMetaData->frameNumber == 0)
            {
                pm::Log::LogE("Invalid frame number for stack index %u, "
                        "skipping this frame", frameIndexInStack);
                retVal = false;
                continue;
            }

            if (!(prdMetaData->extFlags & PRD_EXT_FLAG_HAS_TRAJECTORIES))
            {
                pm::Log::LogI("No trajectory data in frame for stack index %u, "
                        "frame number %u, skipping this frame",
                        frameIndexInStack, prdMetaData->frameNumber);
                continue;
            }

            // Complete CSV file name
            const std::string outFileName = outFileBaseName + "_"
                + std::to_string(prdMetaData->frameNumber) + ".particles" + csvExt;

            auto frame = pm::PrdFileUtils::ReconstructFrame(prdHeader,
                    metaData, extDynMetaData, rawData);
            if (!frame)
            {
                pm::Log::LogE("Cannot reconstruct frame for stack index %u, "
                        "frame number %u, skipping this frame",
                        frameIndexInStack, prdMetaData->frameNumber);
                retVal = false;
                continue;
            }

            if (!ExportCsv_Particles(outFileName, *frame))
            {
                // All errors already logged
                retVal = false;
            }
        }
    }

    prdFile.Close();

    return retVal;
}

bool Helper::ExportCsv_Particles(const std::string& outFileName, pm::Frame& frame)
{
    if (!frame.GetAcqCfg().HasMetadata())
        return true;

    const uint32_t frameNr = frame.GetInfo().GetFrameNr();

    if (!frame.DecodeMetadata())
    {
        pm::Log::LogE("Cannot decode frame with number %u", frameNr);
        return false;
    }

    auto DoubleToString = [](double x) -> std::string
    {
        std::ostringstream sout;
        sout.imbue(std::locale("C"));
        sout << std::setprecision(std::numeric_limits<double>::max_digits10);
        sout << x;
        return sout.str();
    };

    std::string content;

    auto extFrameMeta = frame.GetExtMetadata();
    auto& trajectories = frame.GetTrajectories();

    // Add CSV header
    std::vector<std::string> columnNames;
    columnNames.push_back("Frame number");
    columnNames.push_back("ROI number");
    columnNames.push_back("Particle ID");
    columnNames.push_back("Center X");
    columnNames.push_back("Center Y");
    columnNames.push_back("M0");
    columnNames.push_back("M2");
    columnNames.push_back("Lifetime");
    columnNames.push_back("Trajectory length");
    content += pm::Utils::ArrayToStr(columnNames, csvDelim) + '\n';

    // Add CSV line for each particle
    for (const auto& trajectory : trajectories.data)
    {
        if (trajectory.header.pointCount == 0)
            continue; // No points

        const auto& point = trajectory.data.at(0);

        if (point.isValid == 0)
            continue; // First point not valid

        auto collection = &extFrameMeta[trajectory.header.roiNr];

        // Extract M0 from extended metadata
        const md_ext_item* item_m0 = collection->map[PL_MD_EXT_TAG_PARTICLE_M0];
        if (!item_m0 || !item_m0->value || !item_m0->tagInfo
                || item_m0->tagInfo->type != TYPE_UNS32
                || item_m0->tagInfo->size != 4)
        {
            pm::Log::LogE("Missing M0 moment in ext. metadata, frameNr %u, roiNr=%u",
                    frameNr, trajectory.header.roiNr);
            return false;
        }
        // Unsigned fixed-point real number in format Q22.0
        const double m0 =
            pm::Utils::FixedPointToReal<double, uint32_t>(22, 0, *((uint32_t*)item_m0->value));

        // Extract M2 from extended metadata
        const md_ext_item* item_m2 = collection->map[PL_MD_EXT_TAG_PARTICLE_M2];
        if (!item_m2 || !item_m2->value || !item_m2->tagInfo
                || item_m2->tagInfo->type != TYPE_UNS32
                || item_m2->tagInfo->size != 4)
        {
            pm::Log::LogE("Missing M2 moment in ext. metadata, frameNr %u, roiNr=%u",
                    frameNr, trajectory.header.roiNr);
            return false;
        }
        // Unsigned fixed-point real number in format Q3.19
        const double m2 =
            pm::Utils::FixedPointToReal<double, uint32_t>(3, 19, *((uint32_t*)item_m2->value));

        std::vector<std::string> values;
        values.push_back(std::to_string(frameNr));
        values.push_back(std::to_string(trajectory.header.roiNr));
        values.push_back(std::to_string(trajectory.header.particleId));
        values.push_back(DoubleToString(point.x));
        values.push_back(DoubleToString(point.y));
        values.push_back(DoubleToString(m0));
        values.push_back(DoubleToString(m2));
        values.push_back(std::to_string(trajectory.header.lifetime));
        values.push_back(std::to_string(trajectory.header.pointCount));
        content += pm::Utils::ArrayToStr(values, csvDelim) + '\n';
    }

    std::ofstream csv;
    csv.open(outFileName, std::ios::out);
    if (!csv.is_open())
    {
        pm::Log::LogE("Cannot open output file '%s'", outFileName.c_str());
        return false;
    }

    bool error = false;

    csv << content;
    if (!csv.good())
    {
        pm::Log::LogE("Cannot write data to file '%s' for frame number %u",
                outFileName.c_str(), frameNr);
        error = false;
    }

    csv.close();

    if (!error)
    {
        pm::Log::LogI("Successfully created file '%s' for frame number %u",
                outFileName.c_str(), frameNr);
    }
    else
    {
        if (0 == std::remove(outFileName.c_str()))
        {
            pm::Log::LogI("Removed output file '%s'", outFileName.c_str());
        }
        else
        {
            pm::Log::LogE("Cannot remove output file '%s'", outFileName.c_str());
        }
    }

    return !error;
}

int main(int argc, char* argv[])
{
    int retVal = 0;

    {
        // Initiate the Log instance as the very first before any logging starts
        auto consoleLogger = std::make_shared<pm::ConsoleLogger>();

        pm::Log::LogI("PRD->TIFF Converter");
        pm::Log::LogI("Version %s", VERSION_NUMBER_STR);

        uns16 verMajor;
        uns16 verMinor;
        uns16 verBuild;

        auto pvcam = pm::PvcamRuntimeLoader::Get();
        try
        {
            pvcam->Load();
            pvcam->LoadSymbols();

            // All symbols loaded properly
            pm::Log::LogI("-------------------");
            pm::Log::LogI("Found %s", pvcam->GetFileName().c_str());
            pm::Log::LogI("Path '%s'", pvcam->GetFilePath().c_str());

            // TODO: There is no direct way to get PVCAM library version. We can get
            //       PVCAM version only that is not related to library version at all.
            uns16 version;
            if (PV_OK != PVCAM->pl_pvcam_get_ver(&version))
            {
                pm::Log::LogE("PVCAM version UNKNOWN, library unloaded");
                pvcam->Unload();
            }
            else
            {
                verMajor = (version >> 8) & 0xFF;
                verMinor = (version >> 4) & 0x0F;
                verBuild = (version >> 0) & 0x0F;
                pm::Log::LogI("PVCAM version %u.%u.%u", verMajor, verMinor, verBuild);
#if !defined(_WIN32)
                // On Linux loading PVCAM CORE library at run-time is supported
                // since PVCAM version 3.7.4.0 that has modified PVCAM<->driver API.
                // Min. PVCAM library versions: CORE >= 2.4.51, DDI >= 2.0.111.
                // Keep in mind here that PVCAM (i.e. installer) version differs
                // from PVCAM *library* version. Those are two unrelated things.
                if (verMajor < 3
                        || (verMajor == 3 && verMinor < 7)
                        || (verMajor == 3 && verMinor == 7 && verBuild < 4))
                {
                    pm::Log::LogE("Loading PVCAM library at run-time is supported since version 3.7.4.0");
                    pvcam->Unload();
                }
#endif
            }
        }
        catch (const pm::RuntimeLoader::Exception& ex)
        {
            if (pvcam->IsLoaded())
            {
                pm::Log::LogE("Failed to load some symbols from PVCAM library, "
                        "library unloaded (%s)",
                        ex.what());
                pvcam->Unload();
            }
            else
            {
                pm::Log::LogE("Failed to load PVCAM library (%s)", ex.what());
            }
        }
        if (!pvcam->IsLoaded())
        {
            pm::Log::LogE("Failure loading mandatory PVCAM library!!!");
            pm::Log::LogI("===================\n");
            retVal = APP_ERR_LIB_LOAD;
        }
        else
        {
            auto ph_color = pm::ColorRuntimeLoader::Get();
            try
            {
                ph_color->Load();

                pm::Log::LogI("-------------------");
                pm::Log::LogI("Found %s", ph_color->GetFileName().c_str());
                pm::Log::LogI("Path '%s'", ph_color->GetFilePath().c_str());

                ph_color->LoadSymbols();

                // All symbols loaded properly

                if (PH_COLOR_ERROR_NONE != PH_COLOR->get_lib_version(
                            &verMajor, &verMinor, &verBuild))
                {
                    pm::Log::LogE("Version UNKNOWN, library unloaded");
                    ph_color->Unload();
                }
                else
                {
                    pm::Log::LogI("Version %u.%u.%u", verMajor, verMinor, verBuild);

                    if (PH_COLOR_VERSION_MAJOR != verMajor)
                    {
                        pm::Log::LogE("Required major version %u.x.x, library unloaded",
                                PH_COLOR_VERSION_MAJOR);
                        ph_color->Unload();
                    }
                    else if (PH_COLOR_VERSION_MINOR > verMinor)
                    {
                        pm::Log::LogE("Required minor version x.%u.x or newer, library unloaded",
                                PH_COLOR_VERSION_MINOR);
                        ph_color->Unload();
                    }
                }
            }
            catch (const pm::RuntimeLoader::Exception& ex)
            {
                if (ph_color->IsLoaded())
                {
                    pm::Log::LogW("Failed to load some symbols from color helper library, "
                            "library unloaded (%s)",
                            ex.what());
                    ph_color->Unload();
                }
            }

            pm::Log::LogI("===================\n");

            auto helper = std::make_shared<Helper>(argc, argv);

            if (!helper->InstallTerminationHandlers())
            {
                retVal = APP_ERR_HOOKS;
            }
            else if (!helper->ProcessCliOptions())
            {
                retVal = APP_ERR_CLI_ARGS;
            }
            else
            {
                retVal = helper->RunConversion();
            }

            helper->ShowHelp();

            try
            {
                if (ph_color->IsLoaded())
                {
                    ph_color->Unload();
                }
            }
            catch (const pm::RuntimeLoader::Exception& ex)
            {
                pm::Log::LogE(ex.what());
            }
            ph_color->Release();
        }

        try
        {
            if (pvcam->IsLoaded())
            {
                pvcam->Unload();
            }
        }
        catch (const pm::RuntimeLoader::Exception& ex)
        {
            pm::Log::LogE(ex.what());
        }
        pvcam->Release();

        pm::Log::Flush();
    }

    if (retVal != APP_SUCCESS)
    {
        std::cout << "\n\nExiting with error " << retVal << std::endl;
    }
    else
    {
        std::cout << "\n\nFinished successfully" << std::endl;
    }

    return retVal;
}
