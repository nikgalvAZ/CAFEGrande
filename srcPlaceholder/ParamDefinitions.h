/******************************************************************************/
/* Copyright (C) Teledyne Photometrics. All rights reserved.                  */
/******************************************************************************/
#pragma once
#ifndef PM_PARAM_DEFINITIONS_H
#define PM_PARAM_DEFINITIONS_H

/* PVCAM */
#include "master.h"
#include "pvcam.h"

/* System */
#include <cstdint>

namespace pm {

template<typename T> class Param;
class ParamEnum;

template<typename T> class FakeParam;
class FakeParamEnum;

// Compile-time conversion from template type to parameter type
template<typename T> struct ParamTypeFromT;
template<> struct ParamTypeFromT<ParamEnum                > { static constexpr uint16_t value = TYPE_ENUM                 ; };
template<> struct ParamTypeFromT<Param<bool>              > { static constexpr uint16_t value = TYPE_BOOLEAN              ; };
template<> struct ParamTypeFromT<Param<int8_t>            > { static constexpr uint16_t value = TYPE_INT8                 ; };
template<> struct ParamTypeFromT<Param<int16_t>           > { static constexpr uint16_t value = TYPE_INT16                ; };
template<> struct ParamTypeFromT<Param<int32_t>           > { static constexpr uint16_t value = TYPE_INT32                ; };
template<> struct ParamTypeFromT<Param<int64_t>           > { static constexpr uint16_t value = TYPE_INT64                ; };
template<> struct ParamTypeFromT<Param<uint8_t>           > { static constexpr uint16_t value = TYPE_UNS8                 ; };
template<> struct ParamTypeFromT<Param<uint16_t>          > { static constexpr uint16_t value = TYPE_UNS16                ; };
template<> struct ParamTypeFromT<Param<uint32_t>          > { static constexpr uint16_t value = TYPE_UNS32                ; };
template<> struct ParamTypeFromT<Param<uint64_t>          > { static constexpr uint16_t value = TYPE_UNS64                ; };
template<> struct ParamTypeFromT<Param<float>             > { static constexpr uint16_t value = TYPE_FLT32                ; };
template<> struct ParamTypeFromT<Param<double>            > { static constexpr uint16_t value = TYPE_FLT64                ; };
template<> struct ParamTypeFromT<Param<char*>             > { static constexpr uint16_t value = TYPE_CHAR_PTR             ; };
template<> struct ParamTypeFromT<Param<smart_stream_type*>> { static constexpr uint16_t value = TYPE_SMART_STREAM_TYPE_PTR; };
template<> struct ParamTypeFromT<Param<smart_stream_type> > {}; // Not used in PVCAM
template<> struct ParamTypeFromT<Param<void*>             > {}; // Not used in PVCAM
template<> struct ParamTypeFromT<Param<void**>            > {}; // Not used in PVCAM

// Compile-time conversion from parameter type to template type
template<uint16_t paramAttrType> struct ParamTypeToT;
template<> struct ParamTypeToT<TYPE_ENUM                 > { using T = ParamEnum                ; };
template<> struct ParamTypeToT<TYPE_BOOLEAN              > { using T = Param<bool>              ; };
template<> struct ParamTypeToT<TYPE_INT8                 > { using T = Param<int8_t>            ; };
template<> struct ParamTypeToT<TYPE_INT16                > { using T = Param<int16_t>           ; };
template<> struct ParamTypeToT<TYPE_INT32                > { using T = Param<int32_t>           ; };
template<> struct ParamTypeToT<TYPE_INT64                > { using T = Param<int64_t>           ; };
template<> struct ParamTypeToT<TYPE_UNS8                 > { using T = Param<uint8_t>           ; };
template<> struct ParamTypeToT<TYPE_UNS16                > { using T = Param<uint16_t>          ; };
template<> struct ParamTypeToT<TYPE_UNS32                > { using T = Param<uint32_t>          ; };
template<> struct ParamTypeToT<TYPE_UNS64                > { using T = Param<uint64_t>          ; };
template<> struct ParamTypeToT<TYPE_FLT32                > { using T = Param<float>             ; };
template<> struct ParamTypeToT<TYPE_FLT64                > { using T = Param<double>            ; };
template<> struct ParamTypeToT<TYPE_CHAR_PTR             > { using T = Param<char*>             ; };
template<> struct ParamTypeToT<TYPE_SMART_STREAM_TYPE_PTR> { using T = Param<smart_stream_type*>; };
template<> struct ParamTypeToT<TYPE_SMART_STREAM_TYPE    > {}; // Not used in PVCAM
template<> struct ParamTypeToT<TYPE_VOID_PTR             > {}; // Not used in PVCAM
template<> struct ParamTypeToT<TYPE_VOID_PTR_PTR         > {}; // Not used in PVCAM

// Compile-time conversion from parameter type to template type
template<uint16_t paramAttrType> struct ParamTypeToFakeT;
template<> struct ParamTypeToFakeT<TYPE_ENUM                 > { using T = FakeParamEnum                ; };
template<> struct ParamTypeToFakeT<TYPE_BOOLEAN              > { using T = FakeParam<bool>              ; };
template<> struct ParamTypeToFakeT<TYPE_INT8                 > { using T = FakeParam<int8_t>            ; };
template<> struct ParamTypeToFakeT<TYPE_INT16                > { using T = FakeParam<int16_t>           ; };
template<> struct ParamTypeToFakeT<TYPE_INT32                > { using T = FakeParam<int32_t>           ; };
template<> struct ParamTypeToFakeT<TYPE_INT64                > { using T = FakeParam<int64_t>           ; };
template<> struct ParamTypeToFakeT<TYPE_UNS8                 > { using T = FakeParam<uint8_t>           ; };
template<> struct ParamTypeToFakeT<TYPE_UNS16                > { using T = FakeParam<uint16_t>          ; };
template<> struct ParamTypeToFakeT<TYPE_UNS32                > { using T = FakeParam<uint32_t>          ; };
template<> struct ParamTypeToFakeT<TYPE_UNS64                > { using T = FakeParam<uint64_t>          ; };
template<> struct ParamTypeToFakeT<TYPE_FLT32                > { using T = FakeParam<float>             ; };
template<> struct ParamTypeToFakeT<TYPE_FLT64                > { using T = FakeParam<double>            ; };
template<> struct ParamTypeToFakeT<TYPE_CHAR_PTR             > { using T = FakeParam<char*>             ; };
template<> struct ParamTypeToFakeT<TYPE_SMART_STREAM_TYPE_PTR> { using T = FakeParam<smart_stream_type*>; };
template<> struct ParamTypeToFakeT<TYPE_SMART_STREAM_TYPE    > {}; // Not used in PVCAM
template<> struct ParamTypeToFakeT<TYPE_VOID_PTR             > {}; // Not used in PVCAM
template<> struct ParamTypeToFakeT<TYPE_VOID_PTR_PTR         > {}; // Not used in PVCAM

#if 1 // Always enabled, just to allow code folding for ParamT
// Compile-time conversion from parameter ID to template type
template<uint32_t paramId> struct ParamT;

// Same order as PARAM_* definitions in pvcam.h

template<> struct ParamT<PARAM_DD_INFO_LENGTH           > { using T = Param<int16_t>           ; };
template<> struct ParamT<PARAM_DD_VERSION               > { using T = Param<uint16_t>          ; };
template<> struct ParamT<PARAM_DD_RETRIES               > { using T = Param<uint16_t>          ; };
template<> struct ParamT<PARAM_DD_TIMEOUT               > { using T = Param<uint16_t>          ; };
template<> struct ParamT<PARAM_DD_INFO                  > { using T = Param<char*>             ; };

template<> struct ParamT<PARAM_CAM_INTERFACE_TYPE       > { using T = ParamEnum                ; };
template<> struct ParamT<PARAM_CAM_INTERFACE_MODE       > { using T = ParamEnum                ; };

template<> struct ParamT<PARAM_ADC_OFFSET               > { using T = Param<int16_t>           ; };
template<> struct ParamT<PARAM_CHIP_NAME                > { using T = Param<char*>             ; };
template<> struct ParamT<PARAM_SYSTEM_NAME              > { using T = Param<char*>             ; };
template<> struct ParamT<PARAM_VENDOR_NAME              > { using T = Param<char*>             ; };
template<> struct ParamT<PARAM_PRODUCT_NAME             > { using T = Param<char*>             ; };
template<> struct ParamT<PARAM_CAMERA_PART_NUMBER       > { using T = Param<char*>             ; };

template<> struct ParamT<PARAM_COOLING_MODE             > { using T = ParamEnum                ; };
template<> struct ParamT<PARAM_PREAMP_DELAY             > { using T = Param<uint16_t>          ; };
template<> struct ParamT<PARAM_COLOR_MODE               > { using T = ParamEnum                ; };
template<> struct ParamT<PARAM_MPP_CAPABLE              > { using T = ParamEnum                ; };
template<> struct ParamT<PARAM_PREAMP_OFF_CONTROL       > { using T = Param<uint32_t>          ; };

template<> struct ParamT<PARAM_PREMASK                  > { using T = Param<uint16_t>          ; };
template<> struct ParamT<PARAM_PRESCAN                  > { using T = Param<uint16_t>          ; };
template<> struct ParamT<PARAM_POSTMASK                 > { using T = Param<uint16_t>          ; };
template<> struct ParamT<PARAM_POSTSCAN                 > { using T = Param<uint16_t>          ; };
template<> struct ParamT<PARAM_PIX_PAR_DIST             > { using T = Param<uint16_t>          ; };
template<> struct ParamT<PARAM_PIX_PAR_SIZE             > { using T = Param<uint16_t>          ; };
template<> struct ParamT<PARAM_PIX_SER_DIST             > { using T = Param<uint16_t>          ; };
template<> struct ParamT<PARAM_PIX_SER_SIZE             > { using T = Param<uint16_t>          ; };
template<> struct ParamT<PARAM_SUMMING_WELL             > { using T = Param<bool>              ; };
template<> struct ParamT<PARAM_FWELL_CAPACITY           > { using T = Param<uint32_t>          ; };
template<> struct ParamT<PARAM_PAR_SIZE                 > { using T = Param<uint16_t>          ; };
template<> struct ParamT<PARAM_SER_SIZE                 > { using T = Param<uint16_t>          ; };
// PARAM_ACCUM_CAPABLE and PARAM_FLASH_DWNLD_CAPABLE intentionally omitted

template<> struct ParamT<PARAM_READOUT_TIME             > { using T = Param<uint32_t>          ; };
template<> struct ParamT<PARAM_CLEARING_TIME            > { using T = Param<int64_t>           ; };
template<> struct ParamT<PARAM_POST_TRIGGER_DELAY       > { using T = Param<int64_t>           ; };
template<> struct ParamT<PARAM_PRE_TRIGGER_DELAY        > { using T = Param<int64_t>           ; };

template<> struct ParamT<PARAM_CLEAR_CYCLES             > { using T = Param<uint16_t>          ; };
template<> struct ParamT<PARAM_CLEAR_MODE               > { using T = ParamEnum                ; };
template<> struct ParamT<PARAM_FRAME_CAPABLE            > { using T = Param<bool>              ; };
template<> struct ParamT<PARAM_PMODE                    > { using T = ParamEnum                ; };

template<> struct ParamT<PARAM_TEMP                     > { using T = Param<int16_t>           ; };
template<> struct ParamT<PARAM_TEMP_SETPOINT            > { using T = Param<int16_t>           ; };

template<> struct ParamT<PARAM_CAM_FW_VERSION           > { using T = Param<uint16_t>          ; };
template<> struct ParamT<PARAM_HEAD_SER_NUM_ALPHA       > { using T = Param<char*>             ; };
template<> struct ParamT<PARAM_PCI_FW_VERSION           > { using T = Param<uint16_t>          ; };

template<> struct ParamT<PARAM_FAN_SPEED_SETPOINT       > { using T = ParamEnum                ; };
template<> struct ParamT<PARAM_CAM_SYSTEMS_INFO         > { using T = Param<char*>             ; };

template<> struct ParamT<PARAM_EXPOSURE_MODE            > { using T = ParamEnum                ; };
template<> struct ParamT<PARAM_EXPOSE_OUT_MODE          > { using T = ParamEnum                ; };

template<> struct ParamT<PARAM_BIT_DEPTH                > { using T = Param<int16_t>           ; };
template<> struct ParamT<PARAM_IMAGE_FORMAT             > { using T = ParamEnum                ; };
template<> struct ParamT<PARAM_IMAGE_COMPRESSION        > { using T = ParamEnum                ; };
template<> struct ParamT<PARAM_SCAN_MODE                > { using T = ParamEnum                ; };
template<> struct ParamT<PARAM_SCAN_DIRECTION           > { using T = ParamEnum                ; };
template<> struct ParamT<PARAM_SCAN_DIRECTION_RESET     > { using T = Param<bool>              ; };
template<> struct ParamT<PARAM_SCAN_LINE_DELAY          > { using T = Param<uint16_t>          ; };
template<> struct ParamT<PARAM_SCAN_LINE_TIME           > { using T = Param<int64_t>           ; };
template<> struct ParamT<PARAM_SCAN_WIDTH               > { using T = Param<uint16_t>          ; };
template<> struct ParamT<PARAM_FRAME_ROTATE             > { using T = ParamEnum                ; };
template<> struct ParamT<PARAM_FRAME_FLIP               > { using T = ParamEnum                ; };
template<> struct ParamT<PARAM_GAIN_INDEX               > { using T = Param<int16_t>           ; };
template<> struct ParamT<PARAM_SPDTAB_INDEX             > { using T = Param<int16_t>           ; };
template<> struct ParamT<PARAM_GAIN_NAME                > { using T = Param<char*>             ; };
template<> struct ParamT<PARAM_SPDTAB_NAME              > { using T = Param<char*>             ; };
template<> struct ParamT<PARAM_READOUT_PORT             > { using T = ParamEnum                ; };
template<> struct ParamT<PARAM_PIX_TIME                 > { using T = Param<uint16_t>          ; };

template<> struct ParamT<PARAM_SHTR_CLOSE_DELAY         > { using T = Param<uint16_t>          ; };
template<> struct ParamT<PARAM_SHTR_OPEN_DELAY          > { using T = Param<uint16_t>          ; };
template<> struct ParamT<PARAM_SHTR_OPEN_MODE           > { using T = ParamEnum                ; };
template<> struct ParamT<PARAM_SHTR_STATUS              > { using T = ParamEnum                ; };

template<> struct ParamT<PARAM_IO_ADDR                  > { using T = Param<uint16_t>          ; };
template<> struct ParamT<PARAM_IO_TYPE                  > { using T = ParamEnum                ; };
template<> struct ParamT<PARAM_IO_DIRECTION             > { using T = ParamEnum                ; };
template<> struct ParamT<PARAM_IO_STATE                 > { using T = Param<double>            ; };
template<> struct ParamT<PARAM_IO_BITDEPTH              > { using T = Param<uint16_t>          ; };

template<> struct ParamT<PARAM_GAIN_MULT_FACTOR         > { using T = Param<uint16_t>          ; };
template<> struct ParamT<PARAM_GAIN_MULT_ENABLE         > { using T = Param<bool>              ; };

template<> struct ParamT<PARAM_PP_FEAT_NAME             > { using T = Param<char*>             ; };
template<> struct ParamT<PARAM_PP_INDEX                 > { using T = Param<int16_t>           ; };
template<> struct ParamT<PARAM_ACTUAL_GAIN              > { using T = Param<uint16_t>          ; };
template<> struct ParamT<PARAM_PP_PARAM_INDEX           > { using T = Param<int16_t>           ; };
template<> struct ParamT<PARAM_PP_PARAM_NAME            > { using T = Param<char*>             ; };
template<> struct ParamT<PARAM_PP_PARAM                 > { using T = Param<uint32_t>          ; };
template<> struct ParamT<PARAM_READ_NOISE               > { using T = Param<uint16_t>          ; };
template<> struct ParamT<PARAM_PP_FEAT_ID               > { using T = Param<uint32_t>          ; };
template<> struct ParamT<PARAM_PP_PARAM_ID              > { using T = Param<uint32_t>          ; };

template<> struct ParamT<PARAM_SMART_STREAM_MODE_ENABLED> { using T = Param<bool>              ; };
template<> struct ParamT<PARAM_SMART_STREAM_MODE        > { using T = Param<uint16_t>          ; };
template<> struct ParamT<PARAM_SMART_STREAM_EXP_PARAMS  > { using T = Param<smart_stream_type*>; };
template<> struct ParamT<PARAM_SMART_STREAM_DLY_PARAMS  > { using T = Param<smart_stream_type*>; };

template<> struct ParamT<PARAM_EXP_TIME                 > { using T = Param<uint16_t>          ; };
template<> struct ParamT<PARAM_EXP_RES                  > { using T = ParamEnum                ; };
template<> struct ParamT<PARAM_EXP_RES_INDEX            > { using T = Param<uint16_t>          ; };
template<> struct ParamT<PARAM_EXPOSURE_TIME            > { using T = Param<uint64_t>          ; };

template<> struct ParamT<PARAM_BOF_EOF_ENABLE           > { using T = ParamEnum                ; };
template<> struct ParamT<PARAM_BOF_EOF_COUNT            > { using T = Param<uint32_t>          ; };
template<> struct ParamT<PARAM_BOF_EOF_CLR              > { using T = Param<bool>              ; };

template<> struct ParamT<PARAM_CIRC_BUFFER              > { using T = Param<bool>              ; };
template<> struct ParamT<PARAM_FRAME_BUFFER_SIZE        > { using T = Param<uint64_t>          ; };

template<> struct ParamT<PARAM_BINNING_SER              > { using T = ParamEnum                ; };
template<> struct ParamT<PARAM_BINNING_PAR              > { using T = ParamEnum                ; };

template<> struct ParamT<PARAM_METADATA_ENABLED         > { using T = Param<bool>              ; };
template<> struct ParamT<PARAM_ROI_COUNT                > { using T = Param<uint16_t>          ; };
template<> struct ParamT<PARAM_CENTROIDS_ENABLED        > { using T = Param<bool>              ; };
template<> struct ParamT<PARAM_CENTROIDS_RADIUS         > { using T = Param<uint16_t>          ; };
template<> struct ParamT<PARAM_CENTROIDS_COUNT          > { using T = Param<uint16_t>          ; };
template<> struct ParamT<PARAM_CENTROIDS_MODE           > { using T = ParamEnum                ; };
template<> struct ParamT<PARAM_CENTROIDS_BG_COUNT       > { using T = ParamEnum                ; };
template<> struct ParamT<PARAM_CENTROIDS_THRESHOLD      > { using T = Param<uint32_t>          ; };

template<> struct ParamT<PARAM_TRIGTAB_SIGNAL           > { using T = ParamEnum                ; };
template<> struct ParamT<PARAM_LAST_MUXED_SIGNAL        > { using T = Param<uint8_t>           ; };
template<> struct ParamT<PARAM_FRAME_DELIVERY_MODE      > { using T = ParamEnum                ; };
#endif

} // namespace pm

#endif /* PM_PARAM_DEFINITIONS_H */
