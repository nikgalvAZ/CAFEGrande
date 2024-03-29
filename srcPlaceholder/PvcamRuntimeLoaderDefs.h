/******************************************************************************/
/* Copyright (C) Teledyne Photometrics. All rights reserved.                  */
/******************************************************************************/
#pragma once
#ifndef PM_PVCAM_RUNTIME_LOADER_DEFS_H
#define PM_PVCAM_RUNTIME_LOADER_DEFS_H

/* PVCAM */
#include "master.h"
#include "pvcam.h"

typedef rs_bool(PV_DECL *PL_PVCAM_GET_VER)
    (uns16* pvcam_version);
typedef rs_bool(PV_DECL *PL_PVCAM_INIT)
    (void);
typedef rs_bool(PV_DECL *PL_PVCAM_UNINIT)
    (void);

typedef rs_bool(PV_DECL *PL_CAM_CLOSE)
    (int16 hcam);
typedef rs_bool(PV_DECL *PL_CAM_GET_NAME)
    (int16 cam_num, char* camera_name);
typedef rs_bool(PV_DECL *PL_CAM_GET_TOTAL)
    (int16* totl_cams);
typedef rs_bool(PV_DECL *PL_CAM_OPEN)
    (char* camera_name, int16* hcam, int16 o_mode);

typedef rs_bool(PV_DECL *PL_CAM_REGISTER_CALLBACK_EX3)
    (int16 hcam, int32 callback_event, void* callback, void* context);
typedef rs_bool(PV_DECL *PL_CAM_DEREGISTER_CALLBACK)
    (int16 hcam, int32 callback_event);

typedef   int16(PV_DECL *PL_ERROR_CODE)
    (void);
typedef rs_bool(PV_DECL *PL_ERROR_MESSAGE)
    (int16 err_code, char* msg);

typedef rs_bool(PV_DECL *PL_GET_PARAM)
    (int16 hcam, uns32 param_id, int16 param_attribute, void* param_value);
typedef rs_bool(PV_DECL *PL_SET_PARAM)
    (int16 hcam, uns32 param_id, void* param_value);
typedef rs_bool(PV_DECL *PL_GET_ENUM_PARAM)
    (int16 hcam, uns32 param_id, uns32 index, int32* value, char* desc, uns32 length);
typedef rs_bool(PV_DECL *PL_ENUM_STR_LENGTH)
    (int16 hcam, uns32 param_id, uns32 index, uns32* length);

typedef rs_bool(PV_DECL *PL_PP_RESET)
    (int16 hcam);

typedef rs_bool(PV_DECL *PL_CREATE_SMART_STREAM_STRUCT)
    (smart_stream_type** array, uns16 entries);
typedef rs_bool(PV_DECL *PL_RELEASE_SMART_STREAM_STRUCT)
    (smart_stream_type** array);

typedef rs_bool(PV_DECL *PL_CREATE_FRAME_INFO_STRUCT)
    (FRAME_INFO** new_frame);
typedef rs_bool(PV_DECL *PL_RELEASE_FRAME_INFO_STRUCT)
    (FRAME_INFO* frame_to_delete);

typedef rs_bool(PV_DECL *PL_EXP_SETUP_SEQ)
    (int16 hcam, uns16 exp_total, uns16 rgn_total, const rgn_type* rgn_array, int16 exp_mode, uns32 exposure_time, uns32* exp_bytes);
typedef rs_bool(PV_DECL *PL_EXP_START_SEQ)
    (int16 hcam, void* pixel_stream);
typedef rs_bool(PV_DECL *PL_EXP_SETUP_CONT)
    (int16 hcam, uns16 rgn_total, const rgn_type* rgn_array, int16 exp_mode, uns32 exposure_time, uns32* exp_bytes, int16 buffer_mode);
typedef rs_bool(PV_DECL *PL_EXP_START_CONT)
    (int16 hcam, void* pixel_stream, uns32 size);

// See PL_EXP_TRIGGER typedef below

typedef rs_bool(PV_DECL *PL_EXP_CHECK_STATUS)
    (int16 hcam, int16* status, uns32* bytes_arrived);
typedef rs_bool(PV_DECL *PL_EXP_CHECK_CONT_STATUS)
    (int16 hcam, int16* status, uns32* bytes_arrived, uns32* buffer_cnt);
typedef rs_bool(PV_DECL *PL_EXP_CHECK_CONT_STATUS_EX)
    (int16 hcam, int16* status, uns32* byte_cnt, uns32* buffer_cnt, FRAME_INFO* pFrameInfo);
typedef rs_bool(PV_DECL *PL_EXP_GET_LATEST_FRAME)
    (int16 hcam, void** frame);
typedef rs_bool(PV_DECL *PL_EXP_GET_LATEST_FRAME_EX)
    (int16 hcam, void** frame, FRAME_INFO* pFrameInfo);

typedef rs_bool(PV_DECL *PL_EXP_STOP_CONT)
    (int16 hcam, int16 cam_state);
typedef rs_bool(PV_DECL *PL_EXP_ABORT)
    (int16 hcam, int16 cam_state);
typedef rs_bool(PV_DECL *PL_EXP_FINISH_SEQ)
    (int16 hcam, void* pixel_stream, int16 hbuf);

// Added in 3.1.5
typedef rs_bool(PV_DECL *PL_MD_FRAME_DECODE)
    (md_frame* pDstFrame, void* pSrcBuf, uns32 srcBufSize);
typedef rs_bool(PV_DECL *PL_MD_FRAME_RECOMPOSE)
    (void* pDstBuf, uns16 offX, uns16 offY, uns16 dstWidth, uns16 dstHeight, md_frame* pSrcFrame);
typedef rs_bool(PV_DECL *PL_MD_CREATE_FRAME_STRUCT_CONT)
    (md_frame** pFrame, uns16 roiCount);
typedef rs_bool(PV_DECL *PL_MD_CREATE_FRAME_STRUCT)
    (md_frame** pFrame, void* pSrcBuf, uns32 srcBufSize);
typedef rs_bool(PV_DECL *PL_MD_RELEASE_FRAME_STRUCT)
    (md_frame* pFrame);
typedef rs_bool(PV_DECL *PL_MD_READ_EXTENDED)
    (md_ext_item_collection* pOutput, void* pExtMdPtr, uns32 extMdSize);

// Added in 3.8.0
typedef rs_bool(PV_DECL *PL_EXP_TRIGGER)
    (int16 hcam, uns32* flags, uns32 value);

#endif /* PM_PVCAM_RUNTIME_LOADER_DEFS_H */
