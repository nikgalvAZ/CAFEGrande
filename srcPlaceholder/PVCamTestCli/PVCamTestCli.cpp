/******************************************************************************/
/* Copyright (C) Teledyne Photometrics. All rights reserved.                  */
/******************************************************************************/

/* Local */
#include "backend/Acquisition.h"
#include "backend/Camera.h"
#include <backend/ColorRuntimeLoader.h>
#include <backend/ColorUtils.h>
#include "backend/ConsoleLogger.h"
#include <backend/exceptions/Exception.h>
#include "backend/FakeCamera.h"
#include "backend/Frame.h"
#include "backend/Log.h"
#include "backend/OptionController.h"
#include "backend/PrdFileFormat.h"
#include <backend/PvcamRuntimeLoader.h>
#include "backend/RealCamera.h"
#include "backend/Settings.h"
#include "backend/TrackRuntimeLoader.h"
#include "backend/Utils.h"
#include "version.h"

/* PVCAM */
#include "master.h"
#include "pvcam.h"

/* System */
#include <algorithm>
#include <chrono>
#include <iostream>
#include <limits>
#include <string>
#include <thread>

#if defined(_WIN32)
    #include <Windows.h>
#else
    #include <cstring>
    #include <signal.h>
#endif

constexpr int APP_SUCCESS = 0;
constexpr int APP_ERR_HOOKS = 1;
constexpr int APP_ERR_CLI_ARGS = 2;
constexpr int APP_ERR_RUN = 3;
constexpr int APP_ERR_LIB_LOAD = 4;

// Global variables, used only for termination handlers!

// Global copy of Acquisition pointer
std::shared_ptr<pm::Acquisition> g_acquisition(nullptr);
// Global flag saying if user wants to abort current operation
std::atomic<bool> g_userAbortFlag(false);

class Helper final
{
public:
    Helper(int argc, char* argv[]);
    ~Helper();

public:
    bool InstallTerminationHandlers();

    bool AddCliOptions();
    bool ProcessCliOptions();
    void ShowHelp(); // Show help text if some set

    int RunAcquisition();

private: // CLI option handlers
    bool HandleHelp(const std::string& value);

private:
    void SetHelpText(const std::vector<pm::Option>& options);

    bool CreateColorContext(ph_color_context** colorCtx);

    bool InitAcquisition();
    void UninitAcquisition();

private:
    static void PV_DECL CameraRemovalCallback(FRAME_INFO* frameInfo,
            void* Helper_pointer);

private:
    int m_appArgC;
    char** m_appArgV;

    pm::Settings m_settings{};
    pm::OptionController m_optionController{};
    pm::Option m_helpOption;
    bool m_showFullHelp{ false };
    std::string m_helpText{};
    std::shared_ptr<pm::Camera> m_camera{ nullptr };
    std::shared_ptr<pm::Acquisition> m_acquisition{ nullptr };
    std::atomic<bool> m_cameraRemovedFlag{ false };
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
}

Helper::~Helper()
{
    UninitAcquisition();
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

    if (g_acquisition)
    {
        // On first abort it gives a chance to finish processing.
        // On second abort it forces full stop.
        g_acquisition->RequestAbort(g_userAbortFlag);
        pm::Log::LogI((!g_userAbortFlag)
                ? "\n>>> Acquisition stop requested\n"
                : "\n>>> Acquisition interruption forced\n");
        g_userAbortFlag = true;
    }

    return TRUE;
}
#else
static void TerminalSignalHandler(int sigNum)
{
    if (g_acquisition)
    {
        // On first abort it gives a chance to finish processing.
        // On second abort it forces full stop.
        g_acquisition->RequestAbort(g_userAbortFlag);
        pm::Log::LogI((!g_userAbortFlag)
                ? "\n>>> Acquisition stop requested\n"
                : "\n>>> Acquisition interruption forced\n");
        g_userAbortFlag = true;
    }
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

bool Helper::AddCliOptions()
{
    // Add Help option first (the variable is needed later)
    if (!m_optionController.AddOption(m_helpOption))
        return false;

    return true;
}

bool Helper::ProcessCliOptions()
{
    // Add options specific for this application
    if (!AddCliOptions())
        return false;

    // Add all generic options
    if (!m_settings.AddOptions(m_optionController))
        return false;

    const auto& cliOptions = m_optionController.GetOptions();
    const auto& fpsOption = *std::find_if(cliOptions.cbegin(), cliOptions.cend(),
            [](const pm::Option& o) {
                return o.GetId() == static_cast<uint32_t>(pm::OptionId::FakeCamFps);
            });
    const auto& camIdxOption = *std::find_if(cliOptions.cbegin(), cliOptions.cend(),
            [](const pm::Option& o) {
                return o.GetId() == static_cast<uint32_t>(pm::OptionId::CamIndex);
            });

    const std::vector<pm::Option> initOptions{ m_helpOption, fpsOption, camIdxOption };
    const bool cliParseOk = m_optionController.ProcessOptions(
            m_appArgC, m_appArgV, initOptions, true);
    if (!cliParseOk || m_showFullHelp)
    {
        SetHelpText((m_showFullHelp)
                ? cliOptions
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

int Helper::RunAcquisition()
{
    if (!InitAcquisition())
        return APP_ERR_RUN;

    int16_t totalCams;
    if (!m_camera->GetCameraCount(totalCams))
    {
        totalCams = 0;
    }
    pm::Log::LogI("We have %d camera(s)", totalCams);

    const int16_t camIndex = m_settings.GetCamIndex();
    if (camIndex >= totalCams)
    {
        pm::Log::LogE("There is not so many cameras to select from index %d",
                camIndex);
        return APP_ERR_RUN;
    }

    std::string camName;
    if (!m_camera->GetName(camIndex, camName))
        return APP_ERR_RUN;

    if (!m_camera->Open(camName, &Helper::CameraRemovalCallback, this))
        return APP_ERR_RUN;

    if (!m_camera->AddCliOptions(m_optionController, false))
        return APP_ERR_CLI_ARGS;

    const auto& cliAllOptions = m_optionController.GetOptions();
    const bool cliParseOk = m_optionController.ProcessOptions(
            m_appArgC, m_appArgV, cliAllOptions);
    if (!cliParseOk || m_showFullHelp)
    {
        SetHelpText((m_showFullHelp)
                ? cliAllOptions
                : m_optionController.GetFailedProcessedOptions());

        return (cliParseOk) ? APP_SUCCESS : APP_ERR_CLI_ARGS;
    }

    if (!m_camera->ReviseSettings(m_settings, m_optionController, false))
        return APP_ERR_RUN;

    const auto width = m_camera->GetParams().Get<PARAM_SER_SIZE>()->GetCur();
    const auto height = m_camera->GetParams().Get<PARAM_PAR_SIZE>()->GetCur();

    // With no region specified use full sensor size
    if (m_settings.GetRegions().empty())
    {
        std::vector<rgn_type> regions;
        rgn_type rgn;
        rgn.s1 = 0;
        rgn.s2 = width - 1;
        rgn.sbin = m_settings.GetBinningSerial();
        rgn.p1 = 0;
        rgn.p2 = height - 1;
        rgn.pbin = m_settings.GetBinningParallel();
        regions.push_back(rgn);
        // Cannot fail, the only region uses correct binning factors
        m_settings.SetRegions(regions);
    }

    /* One additional note, is that the print statements in this code
       are for demonstration only, and it is not normally recommended
       to print this verbosely during an acquisition, because it may
       affect the performance of the system. */
    if (!m_camera->SetupExp(m_settings))
    {
        pm::Log::LogE("Please review your command line parameters "
                "and ensure they are supported by this camera");
        return APP_ERR_RUN;
    }

    // TODO: Move ImageDlg::FillMethod to backend and add CLI option
    //       to allow at least fill by mean value
    double tiffFillValue = 0.0; // Black-fill

    ph_color_context* tiffColorCtx;
    if (!CreateColorContext(&tiffColorCtx))
        return APP_ERR_RUN;

    g_userAbortFlag = false;
    if (m_acquisition->Start(nullptr, tiffFillValue, tiffColorCtx))
    {
        g_acquisition = m_acquisition;
        m_acquisition->WaitForStop(true);
        g_acquisition = nullptr;
    }

    if (PH_COLOR && tiffColorCtx)
    {
        PH_COLOR->context_release(&tiffColorCtx);
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

void Helper::SetHelpText(const std::vector<pm::Option>& options)
{
    m_helpText  = "Usage\n";
    m_helpText += "=====\n";
    m_helpText += "\n";
    m_helpText += "This CLI application is helpful for automated camera testing.\n";
    m_helpText += "Run without or with almost any combination of options listed below.\n";
    m_helpText += "\n";
    m_helpText += "Return value\n";
    m_helpText += "------------\n";
    m_helpText += "\n";
    m_helpText += "  " + std::to_string(APP_SUCCESS)      + " - Application exited without any error.\n";
    m_helpText += "  " + std::to_string(APP_ERR_HOOKS)    + " - Error while setting termination hooks (e.g. for ctrl+c).\n";
    m_helpText += "  " + std::to_string(APP_ERR_CLI_ARGS) + " - Error while parsing CLI options.\n";
    m_helpText += "  " + std::to_string(APP_ERR_RUN)      + " - Failure during acquisition setup or run.\n";
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

bool Helper::CreateColorContext(ph_color_context** colorCtx)
{
    *colorCtx = nullptr;

    if (!m_settings.GetSaveTiffOptFull())
        return true;

    const auto colorCapable = PH_COLOR
            && m_settings.GetBinningSerial() == 1
            && m_settings.GetBinningParallel() == 1;
    if (!colorCapable)
        return true;

    const auto colorMask =
        m_camera->GetParams().Get<PARAM_COLOR_MODE>()->IsAvail()
        ? m_camera->GetParams().Get<PARAM_COLOR_MODE>()->GetCur()
        : COLOR_NONE;
    if (colorMask == COLOR_NONE)
        return true;

    const int32_t imageFormat =
        m_camera->GetParams().Get<PARAM_IMAGE_FORMAT>()->IsAvail()
        ? m_camera->GetParams().Get<PARAM_IMAGE_FORMAT>()->GetCur()
        : PL_IMAGE_FORMAT_BAYER16;
    int32_t rgbFormat;
    switch (imageFormat)
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

    if (PH_COLOR_ERROR_NONE != PH_COLOR->context_create(colorCtx))
    {
        pm::ColorUtils::LogError("Failure initializing color helper context");
        return false;
    }

    (*colorCtx)->algorithm = m_settings.GetColorDebayerAlgorithm();
    (*colorCtx)->pattern = colorMask;
    (*colorCtx)->bitDepth = m_camera->GetParams().Get<PARAM_BIT_DEPTH>()->GetCur();
    (*colorCtx)->rgbFormat = rgbFormat;
    (*colorCtx)->redScale = m_settings.GetColorWbScaleRed();
    (*colorCtx)->greenScale = m_settings.GetColorWbScaleGreen();
    (*colorCtx)->blueScale = m_settings.GetColorWbScaleBlue();
    (*colorCtx)->forceCpu = (rs_bool)(m_settings.GetColorCpuOnly() ? TRUE : FALSE);
    (*colorCtx)->sensorWidth = m_camera->GetParams().Get<PARAM_SER_SIZE>()->GetCur();
    (*colorCtx)->sensorHeight = m_camera->GetParams().Get<PARAM_PAR_SIZE>()->GetCur();

    if (PH_COLOR_ERROR_NONE != PH_COLOR->context_apply_changes(*colorCtx))
    {
        pm::ColorUtils::LogError("Failure applying color context changes");
        PH_COLOR->context_release(colorCtx);
        return false;
    }

    return true;
}

bool Helper::InitAcquisition()
{
    // Get Camera instance
    try
    {
        auto fakeCamFps = m_settings.GetFakeCamFps();
        if (fakeCamFps != 0)
        {
            m_camera = std::make_shared<pm::FakeCamera>(fakeCamFps);
        }
        else
        {
            m_camera = std::make_shared<pm::RealCamera>();
        }
    }
    catch (...)
    {
        pm::Log::LogE("Failure getting Camera instance!!!");
        return false;
    }

    if (!m_camera->InitLibrary())
        return false;

    // Get Acquisition instance
    try
    {
        m_acquisition = std::make_shared<pm::Acquisition>(m_camera);
    }
    catch (...)
    {
        pm::Log::LogE("Failure getting Acquisition instance!!!");
        return false;
    }

    return true;
}

void Helper::UninitAcquisition()
{
    if (m_acquisition)
    {
        // Ignore errors
        m_acquisition->RequestAbort();
        m_acquisition->WaitForStop(false);
    }

    if (m_camera)
    {
        // Ignore errors
        if (m_camera->IsOpen())
        {
            if (!m_camera->Close())
            {
                pm::Log::LogE("Failure closing camera");
            }
        }
        if (!m_camera->UninitLibrary())
        {
            pm::Log::LogE("Failure uninitializing PVCAM");
        }
    }

    m_acquisition = nullptr;
    m_camera = nullptr;
}

void PV_DECL Helper::CameraRemovalCallback(FRAME_INFO* /*frameInfo*/,
        void* Helper_pointer)
{
    Helper* thiz = static_cast<Helper*>(Helper_pointer);
    thiz->m_cameraRemovedFlag = true;
    thiz->m_acquisition->RequestAbort();
    pm::Log::LogW("Camera has been disconnected\n");
}

int main(int argc, char* argv[])
{
    int retVal = APP_SUCCESS;

    {
        // Initiate the Log instance as the very first before any logging starts
        std::shared_ptr<pm::ConsoleLogger> consoleLogger;
        try
        {
            consoleLogger = std::make_shared<pm::ConsoleLogger>();
        }
        catch (...)
        {
            retVal = APP_ERR_LIB_LOAD;
            std::cerr << "Failed to initialize console logger" << std::endl;
            std::cerr << "\nExiting with error " << retVal << std::endl;
            return retVal;
        }

        pm::Log::LogI("PVCamTestCli (formerly Stream Saving Tool)");
        pm::Log::LogI("Version %s", VERSION_NUMBER_STR);

        uns16 verMajor;
        uns16 verMinor;
        uns16 verBuild;

        auto pvcam = pm::PvcamRuntimeLoader::Get();
        try
        {
            pvcam->Load();

            pm::Log::LogI("-------------------");
            pm::Log::LogI("Found %s", pvcam->GetFileName().c_str());
            pm::Log::LogI("Path '%s'", pvcam->GetFilePath().c_str());

            pvcam->LoadSymbols();

            // All symbols loaded properly

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

            auto ph_track = pm::TrackRuntimeLoader::Get();
            try
            {
                ph_track->Load();

                pm::Log::LogI("-------------------");
                pm::Log::LogI("Found %s", ph_track->GetFileName().c_str());
                pm::Log::LogI("Path '%s'", ph_track->GetFilePath().c_str());

                ph_track->LoadSymbols();

                // All symbols loaded properly

                if (PH_TRACK_ERROR_NONE != PH_TRACK->get_lib_version(&verMajor,
                            &verMinor, &verBuild))
                {
                    pm::Log::LogE("Version UNKNOWN, library unloaded");
                    ph_track->Unload();
                }
                else
                {
                    pm::Log::LogI("Version %u.%u.%u", verMajor, verMinor, verBuild);

                    if (PH_TRACK_VERSION_MAJOR != verMajor)
                    {
                        pm::Log::LogE("Required major version %u.x.x, library unloaded",
                                PH_TRACK_VERSION_MAJOR);
                        ph_track->Unload();
                    }
                    else if (PH_TRACK_VERSION_MINOR > verMinor)
                    {
                        pm::Log::LogE("Required minor version x.%u.x or newer, library unloaded",
                                PH_TRACK_VERSION_MINOR);
                        ph_track->Unload();
                    }
                }
            }
            catch (const pm::RuntimeLoader::Exception& ex)
            {
                if (ph_track->IsLoaded())
                {
                    pm::Log::LogW("Failed to load some symbols from track helper library, "
                            "library unloaded (%s)",
                            ex.what());
                    ph_track->Unload();
                }
            }

            pm::Log::LogI("==================\n");

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
                try
                {
                    // ATM can fail either by throwing an exception or returning error
                    retVal = helper->RunAcquisition();
                }
                catch (const pm::Exception& ex)
                {
                    pm::Log::LogE(ex.what());
                    retVal = APP_ERR_RUN;
                }
            }

            helper->ShowHelp();

            try
            {
                if (ph_track->IsLoaded())
                {
                    ph_track->Unload();
                }
            }
            catch (const pm::RuntimeLoader::Exception& ex)
            {
                pm::Log::LogE(ex.what());
            }
            ph_track->Release();

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
        std::cout << "\nExiting with error " << retVal << std::endl;
    }
    else
    {
        std::cout << "\nFinished successfully" << std::endl;
    }

    return retVal;
}
