/******************************************************************************/
/* Copyright (C) Teledyne Photometrics. All rights reserved.                  */
/******************************************************************************/
#pragma once
#ifndef PM_PVCAM_RUNTIME_LOADER_H
#define PM_PVCAM_RUNTIME_LOADER_H

/* Local */
#include "backend/PvcamRuntimeLoaderDefs.h"
#include "backend/RuntimeLoader.h"

namespace pm {

class PvcamRuntimeLoader final : public RuntimeLoader
{
public:
    struct Api
    {
        PL_PVCAM_GET_VER pl_pvcam_get_ver;
        PL_PVCAM_INIT pl_pvcam_init;
        PL_PVCAM_UNINIT pl_pvcam_uninit;

        PL_CAM_CLOSE pl_cam_close;
        PL_CAM_GET_NAME pl_cam_get_name;
        PL_CAM_GET_TOTAL pl_cam_get_total;
        PL_CAM_OPEN pl_cam_open;

        PL_CAM_REGISTER_CALLBACK_EX3 pl_cam_register_callback_ex3;
        PL_CAM_DEREGISTER_CALLBACK pl_cam_deregister_callback;

        PL_ERROR_CODE pl_error_code;
        PL_ERROR_MESSAGE pl_error_message;

        PL_GET_PARAM pl_get_param;
        PL_SET_PARAM pl_set_param;
        PL_GET_ENUM_PARAM pl_get_enum_param;
        PL_ENUM_STR_LENGTH pl_enum_str_length;

        PL_PP_RESET pl_pp_reset;

        PL_CREATE_SMART_STREAM_STRUCT pl_create_smart_stream_struct;
        PL_RELEASE_SMART_STREAM_STRUCT pl_release_smart_stream_struct;

        PL_CREATE_FRAME_INFO_STRUCT pl_create_frame_info_struct;
        PL_RELEASE_FRAME_INFO_STRUCT pl_release_frame_info_struct;

        PL_EXP_SETUP_SEQ pl_exp_setup_seq;
        PL_EXP_START_SEQ pl_exp_start_seq;
        PL_EXP_SETUP_CONT pl_exp_setup_cont;
        PL_EXP_START_CONT pl_exp_start_cont;

        // See pl_exp_trigger below

        PL_EXP_CHECK_STATUS pl_exp_check_status;
        PL_EXP_CHECK_CONT_STATUS pl_exp_check_cont_status;
        PL_EXP_CHECK_CONT_STATUS_EX pl_exp_check_cont_status_ex;

        PL_EXP_GET_LATEST_FRAME pl_exp_get_latest_frame;
        PL_EXP_GET_LATEST_FRAME_EX pl_exp_get_latest_frame_ex;

        PL_EXP_STOP_CONT pl_exp_stop_cont;
        PL_EXP_ABORT pl_exp_abort;
        PL_EXP_FINISH_SEQ pl_exp_finish_seq;

        // Added in 3.1.5
        PL_MD_FRAME_DECODE pl_md_frame_decode;
        PL_MD_FRAME_RECOMPOSE pl_md_frame_recompose;
        PL_MD_CREATE_FRAME_STRUCT_CONT pl_md_create_frame_struct_cont;
        PL_MD_CREATE_FRAME_STRUCT pl_md_create_frame_struct;
        PL_MD_RELEASE_FRAME_STRUCT pl_md_release_frame_struct;
        PL_MD_READ_EXTENDED pl_md_read_extended;

        // Added in 3.8.0
        PL_EXP_TRIGGER pl_exp_trigger;
    };

private:
    static PvcamRuntimeLoader* m_instance;
public:
    /// @brief Creates singleton instance
    static PvcamRuntimeLoader* Get();
    static void Release();

private:
    PvcamRuntimeLoader();
    virtual ~PvcamRuntimeLoader();

    PvcamRuntimeLoader(const PvcamRuntimeLoader&) = delete;
    PvcamRuntimeLoader(PvcamRuntimeLoader&&) = delete;
    PvcamRuntimeLoader& operator=(const PvcamRuntimeLoader&) = delete;
    PvcamRuntimeLoader& operator=(PvcamRuntimeLoader&&) = delete;

public: // From RuntimeLoader
    virtual void Unload();
    virtual bool LoadSymbols(bool silent = false) override;

public:
    /// @brief Calls RuntimeLoader::Load with deduced library name.
    /// @throw RuntimeLoader::Exception with error message for logging.
    void Load();
    /// @brief Returns loaded Api structure or null if not loaded.
    inline const Api* GetApi() const
    { return m_api; }

    bool hasMetadataFunctions() const;

private:
    Api* m_api{ nullptr };
    bool m_hasMetadataFunctions{ false };
};

} // namespace pm

#define PVCAM pm::PvcamRuntimeLoader::Get()->GetApi()

#endif /* PM_PVCAM_RUNTIME_LOADER_H */
