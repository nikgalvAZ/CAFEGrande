/******************************************************************************/
/* Copyright (C) Teledyne Photometrics. All rights reserved.                  */
/******************************************************************************/
#include "backend/ColorRuntimeLoader.h"

/* System */
#include <cstring> // memset

pm::ColorRuntimeLoader* pm::ColorRuntimeLoader::m_instance = nullptr;

pm::ColorRuntimeLoader* pm::ColorRuntimeLoader::Get()
{
    if (!m_instance)
    {
        m_instance = new(std::nothrow) ColorRuntimeLoader();
    }

    return m_instance;
}

void pm::ColorRuntimeLoader::Release()
{
    delete m_instance;
    m_instance = nullptr;
}

pm::ColorRuntimeLoader::ColorRuntimeLoader()
    : RuntimeLoader()
{
}

pm::ColorRuntimeLoader::~ColorRuntimeLoader()
{
    delete m_api;
}

void pm::ColorRuntimeLoader::Unload()
{
    delete m_api;
    m_api = nullptr;

    RuntimeLoader::Unload();
}

bool pm::ColorRuntimeLoader::LoadSymbols(bool silent)
{
    if (m_api)
        return true;

    m_api = new(std::nothrow) Api();
    if (!m_api)
    {
        throw Exception("Failed to allocate resources for library loading");
    }

    memset(m_api, 0, sizeof(*m_api));

    bool status = true;

    try {
        m_api->get_lib_version = (ph_color_get_lib_version_fn)
            LoadSymbol(ph_color_get_lib_version_fn_name, silent);
    } catch (const RuntimeLoader::Exception& /*ex*/)
    { if (!silent) throw; }
    status = status && !!m_api->get_lib_version;

    try {
        m_api->get_last_error_message = (ph_color_get_last_error_message_fn)
            LoadSymbol(ph_color_get_last_error_message_fn_name, silent);
    } catch (const RuntimeLoader::Exception& /*ex*/)
    { if (!silent) throw; }
    status = status && !!m_api->get_last_error_message;

    try {
        m_api->get_error_message = (ph_color_get_error_message_fn)
            LoadSymbol(ph_color_get_error_message_fn_name, silent);
    } catch (const RuntimeLoader::Exception& /*ex*/)
    { if (!silent) throw; }
    status = status && !!m_api->get_error_message;

    try {
        m_api->context_create = (ph_color_context_create_fn)
            LoadSymbol(ph_color_context_create_fn_name, silent);
    } catch (const RuntimeLoader::Exception& /*ex*/)
    { if (!silent) throw; }
    status = status && !!m_api->context_create;

    try {
        m_api->context_release = (ph_color_context_release_fn)
            LoadSymbol(ph_color_context_release_fn_name, silent);
    } catch (const RuntimeLoader::Exception& /*ex*/)
    { if (!silent) throw; }
    status = status && !!m_api->context_release;

    try {
        m_api->context_apply_changes = (ph_color_context_apply_changes_fn)
            LoadSymbol(ph_color_context_apply_changes_fn_name, silent);
    } catch (const RuntimeLoader::Exception& /*ex*/)
    { if (!silent) throw; }
    status = status && !!m_api->context_apply_changes;

    try {
        m_api->debayer = (ph_color_debayer_fn)
            LoadSymbol(ph_color_debayer_fn_name, silent);
    } catch (const RuntimeLoader::Exception& /*ex*/)
    { if (!silent) throw; }
    status = status && !!m_api->debayer;

    try {
        m_api->debayer_and_white_balance = (ph_color_debayer_and_white_balance_fn)
            LoadSymbol(ph_color_debayer_and_white_balance_fn_name, silent);
    } catch (const RuntimeLoader::Exception& /*ex*/)
    { if (!silent) throw; }
    status = status && !!m_api->debayer_and_white_balance;

    try {
        m_api->white_balance = (ph_color_white_balance_fn)
            LoadSymbol(ph_color_white_balance_fn_name, silent);
    } catch (const RuntimeLoader::Exception& /*ex*/)
    { if (!silent) throw; }
    status = status && !!m_api->white_balance;

    try {
        m_api->auto_exposure = (ph_color_auto_exposure_fn)
            LoadSymbol(ph_color_auto_exposure_fn_name, silent);
    } catch (const RuntimeLoader::Exception& /*ex*/)
    { if (!silent) throw; }
    status = status && !!m_api->auto_exposure;

    try {
        m_api->auto_exposure_abort = (ph_color_auto_exposure_abort_fn)
            LoadSymbol(ph_color_auto_exposure_abort_fn_name, silent);
    } catch (const RuntimeLoader::Exception& /*ex*/)
    { if (!silent) throw; }
    status = status && !!m_api->auto_exposure_abort;

    try {
        m_api->auto_white_balance = (ph_color_auto_white_balance_fn)
            LoadSymbol(ph_color_auto_white_balance_fn_name, silent);
    } catch (const RuntimeLoader::Exception& /*ex*/)
    { if (!silent) throw; }
    status = status && !!m_api->auto_white_balance;

    try {
        m_api->auto_exposure_and_white_balance =
            (ph_color_auto_exposure_and_white_balance_fn)
            LoadSymbol(ph_color_auto_exposure_and_white_balance_fn_name, silent);
    } catch (const RuntimeLoader::Exception& /*ex*/)
    { if (!silent) throw; }
    status = status && !!m_api->auto_exposure_and_white_balance;

    try {
        m_api->convert_format = (ph_color_convert_format_fn)
            LoadSymbol(ph_color_convert_format_fn_name, silent);
    } catch (const RuntimeLoader::Exception& /*ex*/)
    { if (!silent) throw; }
    status = status && !!m_api->convert_format;

    try {
        m_api->buffer_alloc = (ph_color_buffer_alloc_fn)
            LoadSymbol(ph_color_buffer_alloc_fn_name, silent);
    } catch (const RuntimeLoader::Exception& /*ex*/)
    { if (!silent) throw; }
    status = status && !!m_api->buffer_alloc;

    try {
        m_api->buffer_free = (ph_color_buffer_free_fn)
            LoadSymbol(ph_color_buffer_free_fn_name, silent);
    } catch (const RuntimeLoader::Exception& /*ex*/)
    { if (!silent) throw; }
    status = status && !!m_api->buffer_free;

    return status;
}

void pm::ColorRuntimeLoader::Load()
{
    const std::string nameBase = "pvcam_helper_color";
    const std::string majorVer = std::to_string(PH_COLOR_VERSION_MAJOR);
#if defined(_WIN32)
    const std::string name = nameBase + "_v" + majorVer + ".dll";
#elif defined(__linux__)
    const std::string name = "lib" + nameBase + ".so." + majorVer;
#elif defined(__APPLE__)
    const std::string name = "lib" + nameBase + "." + majorVer + ".dylib";
#endif

    RuntimeLoader::Load(name.c_str());
}
