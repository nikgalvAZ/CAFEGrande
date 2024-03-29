/******************************************************************************/
/* Copyright (C) Teledyne Photometrics. All rights reserved.                  */
/******************************************************************************/
#include "backend/PvcamRuntimeLoader.h"

/* System */
#include <cstring> // memset

#if defined(_WIN32)
    #include <windows.h>
#endif

pm::PvcamRuntimeLoader* pm::PvcamRuntimeLoader::m_instance = nullptr;

pm::PvcamRuntimeLoader* pm::PvcamRuntimeLoader::Get()
{
    if (!m_instance)
    {
        m_instance = new(std::nothrow) PvcamRuntimeLoader();
    }

    return m_instance;
}

void pm::PvcamRuntimeLoader::Release()
{
    delete m_instance;
    m_instance = nullptr;
}

pm::PvcamRuntimeLoader::PvcamRuntimeLoader()
    : RuntimeLoader()
{
}

pm::PvcamRuntimeLoader::~PvcamRuntimeLoader()
{
    delete m_api;
}

void pm::PvcamRuntimeLoader::Unload()
{
    delete m_api;
    m_api = nullptr;

    RuntimeLoader::Unload();
}

bool pm::PvcamRuntimeLoader::LoadSymbols(bool silent)
{
    if (m_api)
        return true;

    m_api = new(std::nothrow) Api();
    memset(m_api, 0, sizeof(*m_api));

    bool status = true;

    try {
        m_api->pl_pvcam_get_ver = (PL_PVCAM_GET_VER)
            LoadSymbol("pl_pvcam_get_ver", silent);
    } catch (const RuntimeLoader::Exception& /*ex*/)
    { if (!silent) throw; }
    status = status && !!m_api->pl_pvcam_get_ver;

    try {
        m_api->pl_pvcam_init = (PL_PVCAM_INIT)
            LoadSymbol("pl_pvcam_init", silent);
    } catch (const RuntimeLoader::Exception& /*ex*/)
    { if (!silent) throw; }
    status = status && !!m_api->pl_pvcam_init;

    try {
        m_api->pl_pvcam_uninit = (PL_PVCAM_UNINIT)
            LoadSymbol("pl_pvcam_uninit", silent);
    } catch (const RuntimeLoader::Exception& /*ex*/)
    { if (!silent) throw; }
    status = status && !!m_api->pl_pvcam_uninit;

    try {
        m_api->pl_cam_close = (PL_CAM_CLOSE)
            LoadSymbol("pl_cam_close", silent);
    } catch (const RuntimeLoader::Exception& /*ex*/)
    { if (!silent) throw; }
    status = status && !!m_api->pl_cam_close;

    try {
        m_api->pl_cam_get_name = (PL_CAM_GET_NAME)
            LoadSymbol("pl_cam_get_name", silent);
    } catch (const RuntimeLoader::Exception& /*ex*/)
    { if (!silent) throw; }
    status = status && !!m_api->pl_cam_get_name;

    try {
        m_api->pl_cam_get_total = (PL_CAM_GET_TOTAL)
            LoadSymbol("pl_cam_get_total", silent);
    } catch (const RuntimeLoader::Exception& /*ex*/)
    { if (!silent) throw; }
    status = status && !!m_api->pl_cam_get_total;

    try {
        m_api->pl_cam_open = (PL_CAM_OPEN)
            LoadSymbol("pl_cam_open", silent);
    } catch (const RuntimeLoader::Exception& /*ex*/)
    { if (!silent) throw; }
    status = status && !!m_api->pl_cam_open;

    try {
        m_api->pl_cam_register_callback_ex3 = (PL_CAM_REGISTER_CALLBACK_EX3)
            LoadSymbol("pl_cam_register_callback_ex3", silent);
    } catch (const RuntimeLoader::Exception& /*ex*/)
    { if (!silent) throw; }
    status = status && !!m_api->pl_cam_register_callback_ex3;

    try {
        m_api->pl_cam_deregister_callback = (PL_CAM_DEREGISTER_CALLBACK)
            LoadSymbol("pl_cam_deregister_callback", silent);
    } catch (const RuntimeLoader::Exception& /*ex*/)
    { if (!silent) throw; }
    status = status && !!m_api->pl_cam_deregister_callback;

    try {
        m_api->pl_error_code = (PL_ERROR_CODE)
            LoadSymbol("pl_error_code", silent);
    } catch (const RuntimeLoader::Exception& /*ex*/)
    { if (!silent) throw; }
    status = status && !!m_api->pl_error_code;

    try {
        m_api->pl_error_message = (PL_ERROR_MESSAGE)
            LoadSymbol("pl_error_message", silent);
    } catch (const RuntimeLoader::Exception& /*ex*/)
    { if (!silent) throw; }
    status = status && !!m_api->pl_error_message;

    try {
        m_api->pl_get_param = (PL_GET_PARAM)
            LoadSymbol("pl_get_param", silent);
    } catch (const RuntimeLoader::Exception& /*ex*/)
    { if (!silent) throw; }
    status = status && !!m_api->pl_get_param;

    try {
        m_api->pl_set_param = (PL_SET_PARAM)
            LoadSymbol("pl_set_param", silent);
    } catch (const RuntimeLoader::Exception& /*ex*/)
    { if (!silent) throw; }
    status = status && !!m_api->pl_set_param;

    try {
        m_api->pl_get_enum_param = (PL_GET_ENUM_PARAM)
            LoadSymbol("pl_get_enum_param", silent);
    } catch (const RuntimeLoader::Exception& /*ex*/)
    { if (!silent) throw; }
    status = status && !!m_api->pl_get_enum_param;

    try {
        m_api->pl_enum_str_length = (PL_ENUM_STR_LENGTH)
            LoadSymbol("pl_enum_str_length", silent);
    } catch (const RuntimeLoader::Exception& /*ex*/)
    { if (!silent) throw; }
    status = status && !!m_api->pl_enum_str_length;

    try {
        m_api->pl_pp_reset = (PL_PP_RESET)
            LoadSymbol("pl_pp_reset", silent);
    } catch (const RuntimeLoader::Exception& /*ex*/)
    { if (!silent) throw; }
    status = status && !!m_api->pl_pp_reset;

    try {
        m_api->pl_create_smart_stream_struct = (PL_CREATE_SMART_STREAM_STRUCT)
            LoadSymbol("pl_create_smart_stream_struct", silent);
    } catch (const RuntimeLoader::Exception& /*ex*/)
    { if (!silent) throw; }
    status = status && !!m_api->pl_create_smart_stream_struct;

    try {
        m_api->pl_release_smart_stream_struct = (PL_RELEASE_SMART_STREAM_STRUCT)
            LoadSymbol("pl_release_smart_stream_struct", silent);
    } catch (const RuntimeLoader::Exception& /*ex*/)
    { if (!silent) throw; }
    status = status && !!m_api->pl_release_smart_stream_struct;

    try {
        m_api->pl_create_frame_info_struct = (PL_CREATE_FRAME_INFO_STRUCT)
            LoadSymbol("pl_create_frame_info_struct", silent);
    } catch (const RuntimeLoader::Exception& /*ex*/)
    { if (!silent) throw; }
    status = status && !!m_api->pl_create_frame_info_struct;

    try {
        m_api->pl_release_frame_info_struct = (PL_RELEASE_FRAME_INFO_STRUCT)
            LoadSymbol("pl_release_frame_info_struct", silent);
    } catch (const RuntimeLoader::Exception& /*ex*/)
    { if (!silent) throw; }
    status = status && !!m_api->pl_release_frame_info_struct;

    try {
        m_api->pl_exp_setup_seq = (PL_EXP_SETUP_SEQ)
            LoadSymbol("pl_exp_setup_seq", silent);
    } catch (const RuntimeLoader::Exception& /*ex*/)
    { if (!silent) throw; }
    status = status && !!m_api->pl_exp_setup_seq;

    try {
        m_api->pl_exp_start_seq = (PL_EXP_START_SEQ)
            LoadSymbol("pl_exp_start_seq", silent);
    } catch (const RuntimeLoader::Exception& /*ex*/)
    { if (!silent) throw; }
    status = status && !!m_api->pl_exp_start_seq;

    try {
        m_api->pl_exp_setup_cont = (PL_EXP_SETUP_CONT)
            LoadSymbol("pl_exp_setup_cont", silent);
    } catch (const RuntimeLoader::Exception& /*ex*/)
    { if (!silent) throw; }
    status = status && !!m_api->pl_exp_setup_cont;

    try {
        m_api->pl_exp_start_cont = (PL_EXP_START_CONT)
            LoadSymbol("pl_exp_start_cont", silent);
    } catch (const RuntimeLoader::Exception& /*ex*/)
    { if (!silent) throw; }
    status = status && !!m_api->pl_exp_start_cont;

    try {
        m_api->pl_exp_check_status = (PL_EXP_CHECK_STATUS)
            LoadSymbol("pl_exp_check_status", silent);
    } catch (const RuntimeLoader::Exception& /*ex*/)
    { if (!silent) throw; }
    status = status && !!m_api->pl_exp_check_status;

    try {
        m_api->pl_exp_check_cont_status = (PL_EXP_CHECK_CONT_STATUS)
            LoadSymbol("pl_exp_check_cont_status", silent);
    } catch (const RuntimeLoader::Exception& /*ex*/)
    { if (!silent) throw; }
    status = status && !!m_api->pl_exp_check_cont_status;

    try {
        m_api->pl_exp_check_cont_status_ex = (PL_EXP_CHECK_CONT_STATUS_EX)
            LoadSymbol("pl_exp_check_cont_status_ex", silent);
    } catch (const RuntimeLoader::Exception& /*ex*/)
    { if (!silent) throw; }
    status = status && !!m_api->pl_exp_check_cont_status_ex;

    try {
        m_api->pl_exp_get_latest_frame = (PL_EXP_GET_LATEST_FRAME)
            LoadSymbol("pl_exp_get_latest_frame", silent);
    } catch (const RuntimeLoader::Exception& /*ex*/)
    { if (!silent) throw; }
    status = status && !!m_api->pl_exp_get_latest_frame;

    try {
        m_api->pl_exp_get_latest_frame_ex = (PL_EXP_GET_LATEST_FRAME_EX)
            LoadSymbol("pl_exp_get_latest_frame_ex", silent);
    } catch (const RuntimeLoader::Exception& /*ex*/)
    { if (!silent) throw; }
    status = status && !!m_api->pl_exp_get_latest_frame_ex;

    try {
        m_api->pl_exp_stop_cont = (PL_EXP_STOP_CONT)
            LoadSymbol("pl_exp_stop_cont", silent);
    } catch (const RuntimeLoader::Exception& /*ex*/)
    { if (!silent) throw; }
    status = status && !!m_api->pl_exp_stop_cont;

    try {
        m_api->pl_exp_abort = (PL_EXP_ABORT)
            LoadSymbol("pl_exp_abort", silent);
    } catch (const RuntimeLoader::Exception& /*ex*/)
    { if (!silent) throw; }
    status = status && !!m_api->pl_exp_abort;

    try {
        m_api->pl_exp_finish_seq = (PL_EXP_FINISH_SEQ)
            LoadSymbol("pl_exp_finish_seq", silent);
    } catch (const RuntimeLoader::Exception& /*ex*/)
    { if (!silent) throw; }
    status = status && !!m_api->pl_exp_finish_seq;

    // Added in 3.1.5

    m_hasMetadataFunctions = true;

    try {
        m_api->pl_md_frame_decode = (PL_MD_FRAME_DECODE)
            LoadSymbol("pl_md_frame_decode", silent);
    } catch (const RuntimeLoader::Exception& /*ex*/)
    { /* Optional, do not throw */ }
    status = status && !!m_api->pl_md_frame_decode;
    m_hasMetadataFunctions = m_hasMetadataFunctions && !!m_api->pl_md_frame_decode;

    try {
        m_api->pl_md_frame_recompose = (PL_MD_FRAME_RECOMPOSE)
            LoadSymbol("pl_md_frame_recompose", silent);
    } catch (const RuntimeLoader::Exception& /*ex*/)
    { /* Optional, do not throw */ }
    status = status && !!m_api->pl_md_frame_recompose;
    m_hasMetadataFunctions = m_hasMetadataFunctions && !!m_api->pl_md_frame_recompose;

    try {
        m_api->pl_md_create_frame_struct_cont = (PL_MD_CREATE_FRAME_STRUCT_CONT)
            LoadSymbol("pl_md_create_frame_struct_cont", silent);
    } catch (const RuntimeLoader::Exception& /*ex*/)
    { /* Optional, do not throw */ }
    status = status && !!m_api->pl_md_create_frame_struct_cont;
    m_hasMetadataFunctions = m_hasMetadataFunctions && !!m_api->pl_md_create_frame_struct_cont;

    try {
        m_api->pl_md_create_frame_struct = (PL_MD_CREATE_FRAME_STRUCT)
            LoadSymbol("pl_md_create_frame_struct", silent);
    } catch (const RuntimeLoader::Exception& /*ex*/)
    { /* Optional, do not throw */ }
    status = status && !!m_api->pl_md_create_frame_struct;
    m_hasMetadataFunctions = m_hasMetadataFunctions && !!m_api->pl_md_create_frame_struct;

    try {
        m_api->pl_md_release_frame_struct = (PL_MD_RELEASE_FRAME_STRUCT)
            LoadSymbol("pl_md_release_frame_struct", silent);
    } catch (const RuntimeLoader::Exception& /*ex*/)
    { /* Optional, do not throw */ }
    status = status && !!m_api->pl_md_release_frame_struct;
    m_hasMetadataFunctions = m_hasMetadataFunctions && !!m_api->pl_md_release_frame_struct;

    try {
        m_api->pl_md_read_extended = (PL_MD_READ_EXTENDED)
            LoadSymbol("pl_md_read_extended", silent);
    } catch (const RuntimeLoader::Exception& /*ex*/)
    { /* Optional, do not throw */ }
    status = status && !!m_api->pl_md_read_extended;
    m_hasMetadataFunctions = m_hasMetadataFunctions && !!m_api->pl_md_read_extended;

    // Added in 3.8.0

    try {
        m_api->pl_exp_trigger = (PL_EXP_TRIGGER)
            LoadSymbol("pl_exp_trigger", silent);
    } catch (const RuntimeLoader::Exception& /*ex*/)
    { /* Optional, do not throw */ }
    status = status && !!m_api->pl_exp_trigger;

    return status;
}

void pm::PvcamRuntimeLoader::Load()
{
    const std::string nameBase = "pvcam";
#if defined(_WIN32)
    bool is64BitProcess; // Native 64-bit process
    bool isWow64BitProcess; // 32-bit process on 64-bit OS

    BOOL bIsWow64 = FALSE;
    if (!::IsWow64Process(GetCurrentProcess(), &bIsWow64))
    {
        // Failed for some reason
        isWow64BitProcess = false;
    }
    else
    {
        isWow64BitProcess = (bIsWow64 == TRUE);
    }

    if (isWow64BitProcess)
    {
        is64BitProcess = false; // 32-bit process on 64-bit OS
    }
    else
    {
#if defined(_WIN64)
        is64BitProcess = true; // Native 64-bit process
#else
        is64BitProcess = false; // Native 32-bit process
#endif
    }

    const unsigned int bits = (is64BitProcess) ? 64 : 32;
    const std::string name = nameBase + std::to_string(bits) + ".dll";
#elif defined(__linux__)
    // TODO: Get the major version from pvcam.h once added
    const std::string majorVer = std::to_string(2);
    const std::string name = "lib" + nameBase + ".so." + majorVer;
//#elif defined(__APPLE__)
//    const std::string majorVer = std::to_string(???);
//    const std::string name = "lib" + nameBase + "." + majorVer + ".dylib";
#endif

    RuntimeLoader::Load(name.c_str());
}

bool pm::PvcamRuntimeLoader::hasMetadataFunctions() const
{
    return m_hasMetadataFunctions;
}
