/******************************************************************************/
/* Copyright (C) Teledyne Photometrics. All rights reserved.                  */
/******************************************************************************/
#include "backend/FakeCamera.h"

/* Local */
#include "backend/FakeParam.h"
#include "backend/FakeParams.h"
#include "backend/Frame.h"
#include "backend/Log.h"
#include "backend/Utils.h"

/* System */
#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstring>
#include <map>
#include <random>
#include <sstream>
#include <thread>

// Camera properties
// TODO: Read from some config file
#if 1 // Just collapsible block that doesn't need wide code changes

constexpr char cDdInfo[] = "PVCamTest Virtual Device Driver Version 177.12.3\n"
                           "Copyright (c) Teledyne Photometrics, Inc.";
constexpr int16_t cDdInfoLen = static_cast<int16_t>(sizeof(cDdInfo));
constexpr uint16_t cDdVersion = 0xB1C3; // 177.12.3
const std::vector<pm::ParamEnumItem> cCamIfcTypes{
    pm::ParamEnumItem(PL_CAM_IFC_TYPE_VIRTUAL, "FakeVirtual")
};
constexpr pm::ParamEnumItem::T cCamIfcTypeDef = PL_CAM_IFC_TYPE_VIRTUAL;
const std::vector<pm::ParamEnumItem> cCamIfcModes{
    pm::ParamEnumItem(PL_CAM_IFC_MODE_IMAGING, "FakeImaging")
};
constexpr pm::ParamEnumItem::T cCamIfcModeDef = PL_CAM_IFC_MODE_IMAGING;
constexpr int16_t cAdcOffsetDef = 100;
constexpr char cChipName[] = "FakeChipName";
constexpr char cSystemName[] = "FakeSystemName";
constexpr char cVendorName[] = "FakeVendorName";
constexpr char cProductName[] = "FakeProductName";
constexpr char cCamPartNumber[] = "FakePartNumber";
const std::vector<pm::ParamEnumItem> cCoolingModes{
    pm::ParamEnumItem(NORMAL_COOL, "FakeAirOrWater"),
    pm::ParamEnumItem(CRYO_COOL, "FakeCryogenic"),
    pm::ParamEnumItem(NO_COOL, "FakeNoCooling")
};
constexpr pm::ParamEnumItem::T cCoolingModeDef = NO_COOL;
const std::vector<pm::ParamEnumItem> cColorModes{
    pm::ParamEnumItem(COLOR_NONE, "FakeMono"),
    pm::ParamEnumItem(COLOR_RGGB, "FakeRGGB"),
    pm::ParamEnumItem(COLOR_GRBG, "FakeGRBG"),
    pm::ParamEnumItem(COLOR_GBRG, "FakeGBRG"),
    pm::ParamEnumItem(COLOR_BGGR, "FakeBGGR")
};
const std::vector<pm::ParamEnumItem> cMppModes{
    pm::ParamEnumItem(MPP_UNKNOWN, "FakeUnknown"),
    pm::ParamEnumItem(MPP_ALWAYS_OFF, "FakeAlwaysOff"),
    pm::ParamEnumItem(MPP_ALWAYS_ON, "FakeAlwaysOn"),
    pm::ParamEnumItem(MPP_SELECTABLE, "FakeSelectable")
};
constexpr pm::ParamEnumItem::T cMppModeDef = MPP_UNKNOWN;

constexpr uint16_t cPreMask = 19;
constexpr uint16_t cPreScan = 75;
constexpr uint16_t cPostMask = 11;
constexpr uint16_t cPostScan = 75;
constexpr uint16_t cPixParDist = 14540;
constexpr uint16_t cPixParSize = 14540;
constexpr uint16_t cPixSerDist = 14540;
constexpr uint16_t cPixSerSize = 14540;
constexpr uint32_t cFwellCapacity = 240000;

constexpr uint16_t cSensorWidth = 1024; // cParSize
constexpr uint16_t cSensorHeight = 512; // cSerSize

constexpr uint32_t cReadoutTime = 10;
constexpr int64_t cClearingTime = 0;
constexpr int64_t cPostTrigTime = 0;
constexpr int64_t cPreTrigTime = 0;

const std::vector<pm::ParamEnumItem> cClearModes{
    pm::ParamEnumItem(CLEAR_NEVER, "FakeNever"),
    pm::ParamEnumItem(CLEAR_PRE_EXPOSURE, "FakePreExposure"),
    pm::ParamEnumItem(CLEAR_PRE_SEQUENCE, "FakePreSequence"),
    pm::ParamEnumItem(CLEAR_POST_SEQUENCE, "FakePostSequence"),
    pm::ParamEnumItem(CLEAR_PRE_POST_SEQUENCE, "FakePrePostSequence"),
    pm::ParamEnumItem(CLEAR_PRE_EXPOSURE_POST_SEQ, "FakePreExpPostSeq")
};
constexpr pm::ParamEnumItem::T cClearModeDef = CLEAR_NEVER;
const std::vector<pm::ParamEnumItem> cPModes{
    pm::ParamEnumItem(PMODE_NORMAL, "FakeNormal"),
    pm::ParamEnumItem(PMODE_ALT_NORMAL, "FakeAltNormal")
};
constexpr pm::ParamEnumItem::T cPModeDef = PMODE_NORMAL;
constexpr char cSerialNumber[] = "FakeSerial"; // cHeadSerNumAlpha
const std::vector<pm::ParamEnumItem> cFanSpeeds{
    pm::ParamEnumItem(FAN_SPEED_HIGH, "FakeHigh"),
    pm::ParamEnumItem(FAN_SPEED_MEDIUM, "FakeMedium"),
    pm::ParamEnumItem(FAN_SPEED_LOW, "FakeLow"),
    pm::ParamEnumItem(FAN_SPEED_OFF, "FakeOff")
};
constexpr pm::ParamEnumItem::T cFanSpeedDef = FAN_SPEED_OFF;
constexpr char cCamSystemsInfo[] = "Camera System 0: PVCT_Cam00\n"
            "Node 0:\tPVCamTest Bridge(I/F) - 123.456.789 @ 12.34.56\n"
            "Node 1:\t999_XXX_ENU(CAM) - 987.654.321 @ 177.12.3\n";
const std::vector<pm::ParamEnumItem> cExposureModes{
    pm::ParamEnumItem(EXT_TRIG_INTERNAL, "FakeExtInternal"),
    pm::ParamEnumItem(VARIABLE_TIMED_MODE, "FakeVTM"),
    pm::ParamEnumItem(EXT_TRIG_SOFTWARE_EDGE, "FakeExtSwEdge"),
    pm::ParamEnumItem(EXT_TRIG_SOFTWARE_FIRST, "FakeExtSwFirst")
};
constexpr pm::ParamEnumItem::T cExposureModeDef = EXT_TRIG_INTERNAL;
const std::vector<pm::ParamEnumItem> cExposeOutModes{
    pm::ParamEnumItem(EXPOSE_OUT_FIRST_ROW, "FakeFirstRow"),
    pm::ParamEnumItem(EXPOSE_OUT_ANY_ROW, "FakeAnyRow"),
    pm::ParamEnumItem(EXPOSE_OUT_ALL_ROWS, "FakeAllRows"),
    pm::ParamEnumItem(EXPOSE_OUT_ROLLING_SHUTTER, "FakeRollingShutter"),
    pm::ParamEnumItem(EXPOSE_OUT_LINE_TRIGGER, "FakeLineTrigger")
};
constexpr pm::ParamEnumItem::T cExposeOutModeDef = EXPOSE_OUT_FIRST_ROW;
const std::vector<pm::ParamEnumItem> cImageFormats{
    pm::ParamEnumItem(PL_IMAGE_FORMAT_MONO8, "FakeMono8"),
    pm::ParamEnumItem(PL_IMAGE_FORMAT_MONO16, "FakeMono16"),
    pm::ParamEnumItem(PL_IMAGE_FORMAT_MONO24, "FakeMono24"),
    pm::ParamEnumItem(PL_IMAGE_FORMAT_MONO32, "FakeMono32"),
    pm::ParamEnumItem(PL_IMAGE_FORMAT_BAYER8, "FakeBayer8"),
    pm::ParamEnumItem(PL_IMAGE_FORMAT_BAYER16, "FakeBayer16"),
    pm::ParamEnumItem(PL_IMAGE_FORMAT_BAYER24, "FakeBayer24"),
    pm::ParamEnumItem(PL_IMAGE_FORMAT_BAYER32, "FakeBayer32"),
    pm::ParamEnumItem(PL_IMAGE_FORMAT_RGB24, "FakeRgb24"),
    pm::ParamEnumItem(PL_IMAGE_FORMAT_RGB48, "FakeRgb48"),
    pm::ParamEnumItem(PL_IMAGE_FORMAT_RGB48, "FakeRgb72")
};
const std::vector<pm::ParamEnumItem> cImageCompressions{
    pm::ParamEnumItem(PL_IMAGE_COMPRESSION_NONE, "FakeNone"),
    pm::ParamEnumItem(PL_IMAGE_COMPRESSION_BITPACK9, "FakeBitpack9"),
    pm::ParamEnumItem(PL_IMAGE_COMPRESSION_BITPACK10, "FakeBitpack10"),
    pm::ParamEnumItem(PL_IMAGE_COMPRESSION_BITPACK11, "FakeBitpack11"),
    pm::ParamEnumItem(PL_IMAGE_COMPRESSION_BITPACK12, "FakeBitpack12"),
    pm::ParamEnumItem(PL_IMAGE_COMPRESSION_BITPACK13, "FakeBitpack13"),
    pm::ParamEnumItem(PL_IMAGE_COMPRESSION_BITPACK14, "FakeBitpack14"),
    pm::ParamEnumItem(PL_IMAGE_COMPRESSION_BITPACK15, "FakeBitpack15"),
    pm::ParamEnumItem(PL_IMAGE_COMPRESSION_BITPACK17, "FakeBitpack17"),
    pm::ParamEnumItem(PL_IMAGE_COMPRESSION_BITPACK18, "FakeBitpack18")
};

const std::vector<pm::ParamEnumItem> cScanModes{
    pm::ParamEnumItem(PL_SCAN_MODE_AUTO, "FakeAuto"),
    pm::ParamEnumItem(PL_SCAN_MODE_PROGRAMMABLE_LINE_DELAY, "FakeProgLineDelay"),
    pm::ParamEnumItem(PL_SCAN_MODE_PROGRAMMABLE_SCAN_WIDTH, "FakeProgScanWidth")
};
constexpr pm::ParamEnumItem::T cScanModeDef = PL_SCAN_MODE_AUTO;
const std::vector<pm::ParamEnumItem> cScanDirections{
    pm::ParamEnumItem(PL_SCAN_DIRECTION_DOWN, "FakeDown"),
    pm::ParamEnumItem(PL_SCAN_DIRECTION_UP, "FakeUp"),
    pm::ParamEnumItem(PL_SCAN_DIRECTION_DOWN_UP, "FakeDownUp")
};
constexpr pm::ParamEnumItem::T cScanDirectionDef = PL_SCAN_DIRECTION_DOWN;

const std::vector<pm::ParamEnumItem> cReadoutPorts{
    pm::ParamEnumItem(5, "FakePort5"),
    pm::ParamEnumItem(3, "FakePort3")
};

constexpr int16_t                                cPortCount = 2;
constexpr int16_t                    cSpeedCount[cPortCount] = { 2, 3 };
constexpr int16_t                    cSpeedCountMax = 3; // TODO: With C++17 use constexpr std::max_element
constexpr uint16_t                      cPixTime[cPortCount][cSpeedCountMax] = {
    {   4,                                                              3                                                                          },
    {  10,                                                              5,                                               1                         }
};
constexpr char                        cSpeedName[cPortCount][cSpeedCountMax][MAX_SPDTAB_NAME_LEN] = {
    {   "FakeSpeed50",                                                  "FakeSpeed51"                                                              },
    {   "FakeSpeed30",                                                  "FakeSpeed31",                                   "FakeSpeed32"             }
};
constexpr pm::ParamEnumItem::T        cColorMode[cPortCount][cSpeedCountMax] = {
    {   COLOR_NONE,                                                     COLOR_NONE                                                                 },
    {   COLOR_NONE,                                                     COLOR_NONE,                                      COLOR_NONE                }
};
constexpr pm::ParamEnumItem::T      cImageFormat[cPortCount][cSpeedCountMax] = {
    {   PL_IMAGE_FORMAT_MONO16,                                         PL_IMAGE_FORMAT_MONO16                                                     },
    {   PL_IMAGE_FORMAT_MONO32,                                         PL_IMAGE_FORMAT_MONO16,                          PL_IMAGE_FORMAT_MONO8     }
};
constexpr pm::ParamEnumItem::T cImageCompression[cPortCount][cSpeedCountMax] = {
    {   PL_IMAGE_COMPRESSION_NONE,                                      PL_IMAGE_COMPRESSION_NONE                                                  },
    {   PL_IMAGE_COMPRESSION_NONE,                                      PL_IMAGE_COMPRESSION_NONE,                       PL_IMAGE_COMPRESSION_NONE }
};
constexpr int16_t                     cGainCount[cPortCount][cSpeedCountMax] = {
    {   4,                                                              3                                                                          },
    {   3,                                                              2,                                               1                         }
};
constexpr int16_t                     cGainCountMax = 4; // TODO: With C++17 use constexpr std::max_element
constexpr int16_t                      cBitDepth[cPortCount][cSpeedCountMax][cGainCountMax] = {
    { { 16,            15,            14,            13            }, { 12,            11,            10            }                              },
    { { 32,            24,            18                           }, { 16,             9                           }, { 8                       } }
};
constexpr char                         cGainName[cPortCount][cSpeedCountMax][cGainCountMax][MAX_GAIN_NAME_LEN] = {
    { { "FakeGain501", "FakeGain502", "FakeGain503", "FakeGain504" }, { "FakeGain511", "FakeGain512", "FakeGain513" }                              },
    { { "FakeGain301", "FakeGain302", "FakeGain303"                }, { "FakeGain311", "FakeGain312"                }, { "FakeGain321"           } },
};
// Helpers for switching groups of PP parameters
constexpr int16_t                  cPpGroupCount = 2;
constexpr int16_t                  cPpGroupIndex[cPortCount][cSpeedCountMax] = {
    {   0,                                                              1                                                                          },
    {   1,                                                              0,                                               1                         }
};

const std::vector<pm::ParamEnumItem> cShtrOpenModes{
    pm::ParamEnumItem(OPEN_NEVER, "FakeNever"),
    pm::ParamEnumItem(OPEN_PRE_EXPOSURE, "FakePreExposure"),
    pm::ParamEnumItem(OPEN_PRE_SEQUENCE, "FakePreSequence"),
    pm::ParamEnumItem(OPEN_PRE_TRIGGER, "FakePreTrigger"),
    pm::ParamEnumItem(OPEN_NO_CHANGE, "FakeNoChange")
};
constexpr pm::ParamEnumItem::T cShtrOpenModeDef = OPEN_NO_CHANGE;
const std::vector<pm::ParamEnumItem> cShtrModes{
    pm::ParamEnumItem(SHTR_FAULT, "FakeFault"),
    pm::ParamEnumItem(SHTR_OPENING, "FakeOpening"),
    pm::ParamEnumItem(SHTR_OPEN, "FakeOpen"),
    pm::ParamEnumItem(SHTR_CLOSING, "FakeClosing"),
    pm::ParamEnumItem(SHTR_CLOSED, "FakeClosed"),
    pm::ParamEnumItem(SHTR_UNKNOWN, "FakeUnknown")
};
constexpr pm::ParamEnumItem::T cShtrModeDef = SHTR_OPEN;
constexpr uint16_t cShtrCloseDelayDef = 0;
constexpr uint16_t cShtrOpenDelayDef = 0;

const std::vector<pm::ParamEnumItem> cIoTypes{
    pm::ParamEnumItem(IO_TYPE_TTL, "FakeTTL"),
    pm::ParamEnumItem(IO_TYPE_DAC, "FakeDAC")
};
const std::vector<pm::ParamEnumItem> cIoDirs{
    pm::ParamEnumItem(IO_DIR_INPUT, "FakeInput"),
    pm::ParamEnumItem(IO_DIR_OUTPUT, "FakeOutput"),
    pm::ParamEnumItem(IO_DIR_INPUT_OUTPUT, "FakeInputOutput")
};
constexpr uint16_t             cIoAddrCount               = 4;
constexpr uint16_t             cIoAddrDef                 = 0;
constexpr pm::ParamEnumItem::T cIoType     [cIoAddrCount] = {
    IO_TYPE_TTL  , IO_TYPE_DAC   , IO_TYPE_DAC   , IO_TYPE_TTL
};
constexpr pm::ParamEnumItem::T cIoDir      [cIoAddrCount] = {
    IO_DIR_INPUT , IO_DIR_OUTPUT , IO_DIR_OUTPUT , IO_DIR_INPUT_OUTPUT
};
constexpr uint16_t             cIoStateAcc [cIoAddrCount] = {
    ACC_READ_ONLY, ACC_READ_WRITE, ACC_WRITE_ONLY, ACC_READ_ONLY
};
constexpr double               cIoStateDef [cIoAddrCount] = {
    4            , -1.27         , +0.5          , 100
};
constexpr double               cIoStateMin [cIoAddrCount] = {
    0            , -12.7         , -1.0          , 0
};
constexpr double               cIoStateMax [cIoAddrCount] = {
    15           , +12.8         , +1.5          , 255
};
constexpr uint16_t             cIoBitDepth [cIoAddrCount] = {
    4            , 8             , 4             , 8
};

// Driver by speed table and groups index from cPpGroupIndex
constexpr int16_t  cPpIndexCount     [cPpGroupCount]                = {
    2, // Group 0
    2  // Group 1
};
constexpr int16_t  cPpIndexCountMax = 2; // TODO: With C++17 use constexpr std::max_element
constexpr int16_t  cPpIndexDef       [cPpGroupCount]                = {
    0, // Group 0
    0  // Group 1
};
constexpr uint32_t cPpFeatId         [cPpGroupCount][cPpIndexCountMax] = {
    { PP_FEATURE_RING_FUNCTION, PP_FEATURE_FRAME_SUMMING }, // Group 0
    { PP_FEATURE_RING_FUNCTION, PP_FEATURE_FRAME_SUMMING }  // Group 1
};
constexpr char     cPpFeatName       [cPpGroupCount][cPpIndexCountMax][MAX_PP_NAME_LEN] = {
    { "FakeRingFunction", "FakeFrameSumming" }, // Group 0
    { "FakeRingFunction", "FakeFrameSumming" }  // Group 1
};
constexpr int16_t  cPpParamIndexCount[cPpGroupCount][cPpIndexCountMax] = {
    { 1, 2 }, // Group 0
    { 1, 2 }  // Group 1
};
constexpr int16_t  cPpParamIndexDef  [cPpGroupCount][cPpIndexCountMax] = {
    { 0, 0 }, // Group 0
    { 0, 0 }  // Group 1
};
constexpr uint32_t cPpParamId        [cPpGroupCount][cPpIndexCountMax][PP_MAX_PARAMETERS_PER_FEATURE] = {
    {    // Group 0
        { PP_PARAMETER_RF_FUNCTION },
        { PP_FEATURE_FRAME_SUMMING_ENABLED, PP_FEATURE_FRAME_SUMMING_COUNT }
    }, { // Group 1
        { PP_PARAMETER_RF_FUNCTION },
        { PP_FEATURE_FRAME_SUMMING_ENABLED, PP_FEATURE_FRAME_SUMMING_COUNT }
    }
};
constexpr char     cPpParamName      [cPpGroupCount][cPpIndexCountMax][PP_MAX_PARAMETERS_PER_FEATURE][MAX_PP_NAME_LEN] = {
    {    // Group 0
        { "FakeRfFunction" },
        { "FakeFrameSummingEnabled", "FakeFrameSummingCount" }
    }, { // Group 1
        { "FakeRfFunction" },
        { "FakeFrameSummingEnabled", "FakeFrameSummingCount" }
    }
};
constexpr uint32_t cPpParamDef       [cPpGroupCount][cPpIndexCountMax][PP_MAX_PARAMETERS_PER_FEATURE] = {
    {    // Group 0
        { 100 },
        { 0, 2 }
    }, { // Group 1
        { 110 },
        { 1, 12 }
    }
};
constexpr uint32_t cPpParamMin       [cPpGroupCount][cPpIndexCountMax][PP_MAX_PARAMETERS_PER_FEATURE] = {
    {    // Group 0
        {  0 },
        { 0,  1 }
    }, { // Group 1
        { 50 },
        { 0, 10 }
    }
};
constexpr uint32_t cPpParamMax       [cPpGroupCount][cPpIndexCountMax][PP_MAX_PARAMETERS_PER_FEATURE] = {
    {    // Group 0
        { 150 },
        { 1,  9 }
    }, { // Group 1
        { 200 },
        { 1, 20 }
    }
};
constexpr uint32_t cPpParamInc       [cPpGroupCount][cPpIndexCountMax][PP_MAX_PARAMETERS_PER_FEATURE] = {
    {    // Group 0
        { 1 /*10*/ }, // PVCAM doesn't read increment from camera, it's always 1
        { 1, 1 }
    }, { // Group 1
        { 1 /*10*/ }, // PVCAM doesn't read increment from camera, it's always 1
        { 1, 1 }
    }
};
constexpr uint32_t cPpParamCount     [cPpGroupCount][cPpIndexCountMax][PP_MAX_PARAMETERS_PER_FEATURE] = {
    {    // Group 0
        { (cPpParamMax[0][0][0] - cPpParamMin[0][0][0]) / cPpParamInc[0][0][0] + 1 },
        { (cPpParamMax[0][1][0] - cPpParamMin[0][1][0]) / cPpParamInc[0][1][0] + 1, (cPpParamMax[0][1][1] - cPpParamMin[0][1][1]) / cPpParamInc[0][1][1] + 1 }
    }, { // Group 1
        { (cPpParamMax[1][0][0] - cPpParamMin[1][0][0]) / cPpParamInc[1][0][0] + 1 },
        { (cPpParamMax[1][1][0] - cPpParamMin[1][1][0]) / cPpParamInc[1][1][0] + 1, (cPpParamMax[1][1][1] - cPpParamMin[1][1][1]) / cPpParamInc[1][1][1] + 1 }
    }
};
constexpr uint16_t cActualGain = 10;
constexpr uint16_t cReadNoise = 590;

constexpr uint16_t cSmartCount = 10;
constexpr uint16_t cSmartMode = SMTMODE_ARBITRARY_ALL;

constexpr uint16_t cExpTime = 10;

const std::vector<pm::ParamEnumItem> cExpRess{
    pm::ParamEnumItem(EXP_RES_ONE_MILLISEC, "FakeMilliSec"),
    pm::ParamEnumItem(EXP_RES_ONE_MICROSEC, "FakeMicroSec"),
    pm::ParamEnumItem(EXP_RES_ONE_SEC, "FakeSec")
};
constexpr pm::ParamEnumItem::T cExpResDef = EXP_RES_ONE_MILLISEC;
constexpr uint16_t cExpResIndexDef = static_cast<uint16_t>(cExpResDef);
constexpr uint64_t cExposureTimeDef = 10; // 0-100

const std::vector<pm::ParamEnumItem> cIrqModes{
    pm::ParamEnumItem(NO_FRAME_IRQS, "FakeNone"),
    pm::ParamEnumItem(BEGIN_FRAME_IRQS, "FakeBof"),
    pm::ParamEnumItem(END_FRAME_IRQS, "FakeEof"),
    pm::ParamEnumItem(BEGIN_END_FRAME_IRQS, "FakeBofEof")
};
constexpr pm::ParamEnumItem::T cIrqModeDef = END_FRAME_IRQS;

const std::vector<pm::ParamEnumItem> cBinSerModes{
    pm::ParamEnumItem(1, "1x1"),
    pm::ParamEnumItem(2, "2x2")
};
constexpr pm::ParamEnumItem::T cBinSerDef = 1;
const std::vector<pm::ParamEnumItem> cBinParModes{
    pm::ParamEnumItem(1, "1x1"),
    pm::ParamEnumItem(2, "2x2")
};
constexpr pm::ParamEnumItem::T cBinParDef = 1;

constexpr uint16_t cRoiCountMax = 15;
constexpr uint16_t cCentroidRadiusMax = 50;
constexpr uint16_t cCentroidCountMax = 500;
const std::vector<pm::ParamEnumItem> cCentroidModes{
    pm::ParamEnumItem(PL_CENTROIDS_MODE_LOCATE, "FakeLocate"),
    pm::ParamEnumItem(PL_CENTROIDS_MODE_TRACK, "FakeTrack"),
    pm::ParamEnumItem(PL_CENTROIDS_MODE_BLOB, "FakeBlob")
};
constexpr pm::ParamEnumItem::T cCentroidModeDef = PL_CENTROIDS_MODE_LOCATE;
const std::vector<pm::ParamEnumItem> cCentroidBgCountModes{
    pm::ParamEnumItem(0, "10"),
    pm::ParamEnumItem(1, "50")
};
constexpr pm::ParamEnumItem::T cCentroidBgCountDef = 0;

// TODO: Change it to array similar to IO parameters
const std::vector<pm::ParamEnumItem> cTrigtabSignals{
    pm::ParamEnumItem(PL_TRIGTAB_SIGNAL_EXPOSE_OUT, "FakeExposeOut")
};
constexpr pm::ParamEnumItem::T cTrigtabSignalDef = PL_TRIGTAB_SIGNAL_EXPOSE_OUT;
constexpr uint8_t cLastMuxedSignalDef = 1;
constexpr uint8_t cLastMuxedSignalMin = 1;
constexpr uint8_t cLastMuxedSignalMax = 4;

const std::vector<pm::ParamEnumItem> cFrameDeliveryModes{
    pm::ParamEnumItem(PL_FRAME_DELIVERY_MODE_MAX_FPS, "FakeMaxFPS"),
    pm::ParamEnumItem(PL_FRAME_DELIVERY_MODE_CONSTANT_INTERVALS, "FakeConstIntervals")
};
constexpr pm::ParamEnumItem::T cFrameDeliveryModeDef = PL_FRAME_DELIVERY_MODE_MAX_FPS;

constexpr char cCameraName[] = "FakeCamera";
constexpr uint32_t cMaxGenFrameCount = 10;
// Prime number around 50MB that should help to non-repeating patterns
constexpr size_t cRandomNumberCacheSize = 50000017;

const std::map<PL_MD_EXT_TAGS, md_ext_item_info> g_pl_ext_md_map = {
    //{ PL_MD_EXT_TAG_PARTICLE_ID, { (PL_MD_EXT_TAGS)PL_MD_EXT_TAG_PARTICLE_ID, TYPE_UNS32, sizeof(uns32), "Particle ID" } },
    { PL_MD_EXT_TAG_PARTICLE_M0, { (PL_MD_EXT_TAGS)PL_MD_EXT_TAG_PARTICLE_M0, TYPE_UNS32, sizeof(uns32), "Particle M0" } },
    { PL_MD_EXT_TAG_PARTICLE_M2, { (PL_MD_EXT_TAGS)PL_MD_EXT_TAG_PARTICLE_M2, TYPE_UNS32, sizeof(uns32), "Particle M2" } },
};

#endif // Collapsible block

bool pm::FakeCamera::s_isInitialized = false;

pm::FakeCamera::FakeCamera(unsigned int targetFps)
    : Camera(),
    m_targetFps(targetFps),
    m_readoutTimeUs(1000000.0 / targetFps),
    m_trackRoiExtMdBytes(
        GetExtMdBytes(PL_MD_EXT_TAG_PARTICLE_ID)
        + GetExtMdBytes(PL_MD_EXT_TAG_PARTICLE_M0)
        + GetExtMdBytes(PL_MD_EXT_TAG_PARTICLE_M2))
{
    m_portIndex = 0;
    m_speedIndex = 0;
    m_gainIndex = 0;

    for (int16_t ppGrpIdx = 0; ppGrpIdx < cPpGroupCount; ++ppGrpIdx)
    {
        const auto& gPpIndexCount      = cPpIndexCount     [ppGrpIdx];
        const auto& gPpParamIndexCount = cPpParamIndexCount[ppGrpIdx];
        const auto& gPpParamDef        = cPpParamDef       [ppGrpIdx];
        auto&       gPpParam           = m_ppParam         [ppGrpIdx];

        for (int16_t ppFeatIdx = 0; ppFeatIdx < gPpIndexCount; ++ppFeatIdx)
        {
            for (int16_t ppParamIdx = 0; ppParamIdx < gPpParamIndexCount[ppFeatIdx]; ++ppParamIdx)
            {
                gPpParam[ppFeatIdx][ppParamIdx] = gPpParamDef[ppFeatIdx][ppParamIdx];
            }
        }
    }

    for (int16_t ioAddrIdx = 0; ioAddrIdx < cIoAddrCount; ++ioAddrIdx)
    {
        m_ioState[ioAddrIdx] = cIoStateDef[ioAddrIdx];
    }

    memset(&m_frameGenFrameInfo, 0, sizeof(m_frameGenFrameInfo));

    m_randomPixelCache8  = std::make_unique<RandomPixelCache<uint8_t >>(cRandomNumberCacheSize);
    m_randomPixelCache16 = std::make_unique<RandomPixelCache<uint16_t>>(cRandomNumberCacheSize);
    m_randomPixelCache32 = std::make_unique<RandomPixelCache<uint32_t>>(cRandomNumberCacheSize);

    m_params = std::make_unique<FakeParams>(this);

    // Same order as PARAM_* definitions in pvcam.h

    std::static_pointer_cast<FakeParam<int16_t>>(m_params->Get<PARAM_DD_INFO_LENGTH>())
        ->ChangeRangeAttrs(1, cDdInfoLen, cDdInfoLen, cDdInfoLen, 0)
        .ChangeBaseAttrs(true, ACC_READ_ONLY);
    std::static_pointer_cast<FakeParam<uint16_t>>(m_params->Get<PARAM_DD_VERSION>())
        ->ChangeRangeAttrs(1, cDdVersion, cDdVersion, cDdVersion, 0)
        .ChangeBaseAttrs(true, ACC_READ_ONLY);
    std::static_pointer_cast<FakeParam<uint16_t>>(m_params->Get<PARAM_DD_RETRIES>())
        ->ChangeRangeAttrs(1, 0, 0, 0, 0)
        .ChangeBaseAttrs(false, ACC_READ_ONLY);
    std::static_pointer_cast<FakeParam<uint16_t>>(m_params->Get<PARAM_DD_TIMEOUT>())
        ->ChangeRangeAttrs(1, 0, 0, 0, 0)
        .ChangeBaseAttrs(false, ACC_READ_ONLY);
    std::static_pointer_cast<FakeParam<char*>>(m_params->Get<PARAM_DD_INFO>())
        ->ChangeRangeAttrs(cDdInfo)
        .ChangeBaseAttrs(true, ACC_READ_ONLY);

    std::static_pointer_cast<FakeParamEnum>(m_params->Get<PARAM_CAM_INTERFACE_TYPE>())
        ->ChangeRangeAttrs(cCamIfcTypeDef, cCamIfcTypes)
        .ChangeBaseAttrs(true, ACC_READ_ONLY);
    std::static_pointer_cast<FakeParamEnum>(m_params->Get<PARAM_CAM_INTERFACE_MODE>())
        ->ChangeRangeAttrs(cCamIfcModeDef, cCamIfcModes)
        .ChangeBaseAttrs(true, ACC_READ_ONLY);

    std::static_pointer_cast<FakeParam<int16_t>>(m_params->Get<PARAM_ADC_OFFSET>())
        ->ChangeRangeAttrs(0, cAdcOffsetDef, -32768, 32767, 1)
        .ChangeBaseAttrs(true, ACC_READ_WRITE);
    std::static_pointer_cast<FakeParam<char*>>(m_params->Get<PARAM_CHIP_NAME>())
        ->ChangeRangeAttrs(cChipName)
        .ChangeBaseAttrs(true, ACC_READ_ONLY);
    std::static_pointer_cast<FakeParam<char*>>(m_params->Get<PARAM_SYSTEM_NAME>())
        ->ChangeRangeAttrs(cSystemName)
        .ChangeBaseAttrs(true, ACC_READ_ONLY);
    std::static_pointer_cast<FakeParam<char*>>(m_params->Get<PARAM_VENDOR_NAME>())
        ->ChangeRangeAttrs(cVendorName)
        .ChangeBaseAttrs(true, ACC_READ_ONLY);
    std::static_pointer_cast<FakeParam<char*>>(m_params->Get<PARAM_PRODUCT_NAME>())
        ->ChangeRangeAttrs(cProductName)
        .ChangeBaseAttrs(true, ACC_READ_ONLY);
    std::static_pointer_cast<FakeParam<char*>>(m_params->Get<PARAM_CAMERA_PART_NUMBER>())
        ->ChangeRangeAttrs(cCamPartNumber)
        .ChangeBaseAttrs(true, ACC_READ_ONLY);

    std::static_pointer_cast<FakeParamEnum>(m_params->Get<PARAM_COOLING_MODE>())
        ->ChangeRangeAttrs(cCoolingModeDef, cCoolingModes)
        .ChangeBaseAttrs(false, ACC_READ_ONLY);
    std::static_pointer_cast<FakeParam<uint16_t>>(m_params->Get<PARAM_PREAMP_DELAY>())
        ->ChangeRangeAttrs(1, 5, 5, 5, 0)
        .ChangeBaseAttrs(true, ACC_READ_ONLY);
    const auto colorMode = cColorMode[m_portIndex][m_speedIndex];
    std::static_pointer_cast<FakeParamEnum>(m_params->Get<PARAM_COLOR_MODE>())
        ->ChangeRangeAttrs(colorMode, cColorModes)
        .ChangeBaseAttrs(colorMode != COLOR_NONE, ACC_READ_ONLY);
    std::static_pointer_cast<FakeParamEnum>(m_params->Get<PARAM_MPP_CAPABLE>())
        ->ChangeRangeAttrs(cMppModeDef, cMppModes)
        .ChangeBaseAttrs(false, ACC_READ_ONLY);
    std::static_pointer_cast<FakeParam<uint32_t>>(m_params->Get<PARAM_PREAMP_OFF_CONTROL>())
        ->ChangeRangeAttrs(0, 10000, 0, 0xFFFFFFFF, 0)
        .ChangeBaseAttrs(true, ACC_READ_WRITE);

    std::static_pointer_cast<FakeParam<uint16_t>>(m_params->Get<PARAM_PREMASK>())
        ->ChangeRangeAttrs(1, cPreMask, cPreMask, cPreMask, 0)
        .ChangeBaseAttrs(true, ACC_READ_ONLY);
    std::static_pointer_cast<FakeParam<uint16_t>>(m_params->Get<PARAM_PRESCAN>())
        ->ChangeRangeAttrs(1, cPreScan, cPreScan, cPreScan, 0)
        .ChangeBaseAttrs(true, ACC_READ_ONLY);
    std::static_pointer_cast<FakeParam<uint16_t>>(m_params->Get<PARAM_POSTMASK>())
        ->ChangeRangeAttrs(1, cPostMask, cPostMask, cPostMask, 0)
        .ChangeBaseAttrs(true, ACC_READ_ONLY);
    std::static_pointer_cast<FakeParam<uint16_t>>(m_params->Get<PARAM_POSTSCAN>())
        ->ChangeRangeAttrs(1, cPostScan, cPostScan, cPostScan, 0)
        .ChangeBaseAttrs(true, ACC_READ_ONLY);
    std::static_pointer_cast<FakeParam<uint16_t>>(m_params->Get<PARAM_PIX_PAR_DIST>())
        ->ChangeRangeAttrs(1, cPixParDist, cPixParDist, cPixParDist, 0)
        .ChangeBaseAttrs(true, ACC_READ_ONLY);
    std::static_pointer_cast<FakeParam<uint16_t>>(m_params->Get<PARAM_PIX_PAR_SIZE>())
        ->ChangeRangeAttrs(1, cPixParSize, cPixParSize, cPixParSize, 0)
        .ChangeBaseAttrs(true, ACC_READ_ONLY);
    std::static_pointer_cast<FakeParam<uint16_t>>(m_params->Get<PARAM_PIX_SER_DIST>())
        ->ChangeRangeAttrs(1, cPixSerDist, cPixSerDist, cPixSerDist, 0)
        .ChangeBaseAttrs(true, ACC_READ_ONLY);
    std::static_pointer_cast<FakeParam<uint16_t>>(m_params->Get<PARAM_PIX_SER_SIZE>())
        ->ChangeRangeAttrs(1, cPixSerSize, cPixSerSize, cPixSerSize, 0)
        .ChangeBaseAttrs(true, ACC_READ_ONLY);
    std::static_pointer_cast<FakeParam<bool>>(m_params->Get<PARAM_SUMMING_WELL>())
        ->ChangeRangeAttrs(true)
        .ChangeBaseAttrs(false, ACC_EXIST_CHECK_ONLY);
    std::static_pointer_cast<FakeParam<uint32_t>>(m_params->Get<PARAM_FWELL_CAPACITY>())
        ->ChangeRangeAttrs(1, cFwellCapacity, cFwellCapacity, cFwellCapacity, 0)
        .ChangeBaseAttrs(true, ACC_READ_ONLY);
    std::static_pointer_cast<FakeParam<uint16_t>>(m_params->Get<PARAM_PAR_SIZE>())
        ->ChangeRangeAttrs(1, cSensorHeight, cSensorHeight, cSensorHeight, 0)
        .ChangeBaseAttrs(true, ACC_READ_ONLY);
    std::static_pointer_cast<FakeParam<uint16_t>>(m_params->Get<PARAM_SER_SIZE>())
        ->ChangeRangeAttrs(1, cSensorWidth, cSensorWidth, cSensorWidth, 0)
        .ChangeBaseAttrs(true, ACC_READ_ONLY);
    // PARAM_ACCUM_CAPABLE
    // PARAM_FLASH_DWNLD_CAPABLE

    std::static_pointer_cast<FakeParam<uint32_t>>(m_params->Get<PARAM_READOUT_TIME>())
        ->ChangeRangeAttrs(1, cReadoutTime, cReadoutTime, cReadoutTime, 0)
        .ChangeBaseAttrs(true, ACC_READ_ONLY);
    std::static_pointer_cast<FakeParam<int64_t>>(m_params->Get<PARAM_CLEARING_TIME>())
        ->ChangeRangeAttrs(1, cClearingTime, cClearingTime, cClearingTime, 0)
        .ChangeBaseAttrs(false, ACC_READ_ONLY);
    std::static_pointer_cast<FakeParam<int64_t>>(m_params->Get<PARAM_POST_TRIGGER_DELAY>())
        ->ChangeRangeAttrs(1, cPostTrigTime, cPostTrigTime, cPostTrigTime, 0)
        .ChangeBaseAttrs(false, ACC_READ_ONLY);
    std::static_pointer_cast<FakeParam<int64_t>>(m_params->Get<PARAM_PRE_TRIGGER_DELAY>())
        ->ChangeRangeAttrs(1, cPreTrigTime, cPreTrigTime, cPreTrigTime, 0)
        .ChangeBaseAttrs(false, ACC_READ_ONLY);

    std::static_pointer_cast<FakeParam<uint16_t>>(m_params->Get<PARAM_CLEAR_CYCLES>())
        ->ChangeRangeAttrs(11, 2, 0, 10, 1)
        .ChangeBaseAttrs(true, ACC_READ_WRITE);
    std::static_pointer_cast<FakeParamEnum>(m_params->Get<PARAM_CLEAR_MODE>())
        ->ChangeRangeAttrs(cClearModeDef, cClearModes)
        .ChangeBaseAttrs(true, ACC_READ_WRITE);
    std::static_pointer_cast<FakeParam<bool>>(m_params->Get<PARAM_FRAME_CAPABLE>())
        ->ChangeRangeAttrs(false)
        .ChangeBaseAttrs(false, ACC_EXIST_CHECK_ONLY);
    std::static_pointer_cast<FakeParamEnum>(m_params->Get<PARAM_PMODE>())
        ->ChangeRangeAttrs(cPModeDef, cPModes)
        .ChangeBaseAttrs(true, ACC_READ_WRITE);

    std::static_pointer_cast<FakeParam<int16_t>>(m_params->Get<PARAM_TEMP>())
        ->ChangeRangeAttrs(60083, 0, -27315, 32767, 1)
        .ChangeBaseAttrs(true, ACC_READ_ONLY);
    std::static_pointer_cast<FakeParam<int16_t>>(m_params->Get<PARAM_TEMP_SETPOINT>())
        ->ChangeRangeAttrs(60083, -1000, -27315, 32767, 1)
        .ChangeBaseAttrs(true, ACC_READ_WRITE);

    std::static_pointer_cast<FakeParam<uint16_t>>(m_params->Get<PARAM_CAM_FW_VERSION>())
        ->ChangeRangeAttrs(1, 0x0814, 0x0814, 0x0814, 0)
        .ChangeBaseAttrs(true, ACC_READ_ONLY);
    std::static_pointer_cast<FakeParam<char*>>(m_params->Get<PARAM_HEAD_SER_NUM_ALPHA>())
        ->ChangeRangeAttrs(cSerialNumber)
        .ChangeBaseAttrs(true, ACC_READ_ONLY);
    std::static_pointer_cast<FakeParam<uint16_t>>(m_params->Get<PARAM_PCI_FW_VERSION>())
        ->ChangeRangeAttrs(1, 1, 1, 1, 0)
        .ChangeBaseAttrs(true, ACC_READ_ONLY);

    std::static_pointer_cast<FakeParamEnum>(m_params->Get<PARAM_FAN_SPEED_SETPOINT>())
        ->ChangeRangeAttrs(cFanSpeedDef, cFanSpeeds)
        .ChangeBaseAttrs(true, ACC_READ_WRITE);
#if 0 // Temporarily disabled due to USB issues
    std::static_pointer_cast<FakeParam<char*>>(m_params->Get<PARAM_CAM_SYSTEMS_INFO>())
        ->ChangeRangeAttrs(cCamSystemsInfo)
        .ChangeBaseAttrs(true, ACC_READ_ONLY);
#endif

    std::static_pointer_cast<FakeParamEnum>(m_params->Get<PARAM_EXPOSURE_MODE>())
        ->ChangeRangeAttrs(cExposureModeDef, cExposureModes)
        .ChangeBaseAttrs(true, ACC_READ_ONLY);
    std::static_pointer_cast<FakeParamEnum>(m_params->Get<PARAM_EXPOSE_OUT_MODE>())
        ->ChangeRangeAttrs(cExposeOutModeDef, cExposeOutModes)
        .ChangeBaseAttrs(true, ACC_READ_ONLY);

    const auto bitDepth = cBitDepth[m_portIndex][m_speedIndex][m_gainIndex];
    std::static_pointer_cast<FakeParam<int16_t>>(m_params->Get<PARAM_BIT_DEPTH>())
        ->ChangeRangeAttrs(1, bitDepth, bitDepth, bitDepth, 0)
        .ChangeBaseAttrs(true, ACC_READ_ONLY);
    std::static_pointer_cast<FakeParamEnum>(m_params->Get<PARAM_IMAGE_FORMAT>())
        ->ChangeRangeAttrs(cImageFormat[m_portIndex][m_speedIndex], cImageFormats)
        .ChangeBaseAttrs(true, ACC_READ_ONLY);
    std::static_pointer_cast<FakeParamEnum>(m_params->Get<PARAM_IMAGE_COMPRESSION>())
        ->ChangeRangeAttrs(cImageCompression[m_portIndex][m_speedIndex], cImageCompressions)
        .ChangeBaseAttrs(true, ACC_READ_ONLY);
    std::static_pointer_cast<FakeParamEnum>(m_params->Get<PARAM_SCAN_MODE>())
        ->ChangeRangeAttrs(cScanModeDef, cScanModes)
        .ChangeBaseAttrs(true, ACC_READ_WRITE);
    std::static_pointer_cast<FakeParamEnum>(m_params->Get<PARAM_SCAN_DIRECTION>())
        ->ChangeRangeAttrs(cScanDirectionDef, cScanDirections)
        .ChangeBaseAttrs(true, ACC_READ_WRITE);
    std::static_pointer_cast<FakeParam<bool>>(m_params->Get<PARAM_SCAN_DIRECTION_RESET>())
        ->ChangeRangeAttrs(true)
        .ChangeBaseAttrs(true, ACC_READ_WRITE);
    std::static_pointer_cast<FakeParam<uint16_t>>(m_params->Get<PARAM_SCAN_LINE_DELAY>())
        ->ChangeRangeAttrs(1, 0, 0, 0, 0)
        .ChangeBaseAttrs(true, ACC_READ_ONLY);
    const auto scanOneLineTime = (int64_t)cPixTime[m_portIndex][m_speedIndex] * cSensorWidth;
    std::static_pointer_cast<FakeParam<int64_t>>(m_params->Get<PARAM_SCAN_LINE_TIME>())
        ->ChangeRangeAttrs(1, scanOneLineTime, scanOneLineTime, scanOneLineTime, 0)
        .ChangeBaseAttrs(true, ACC_READ_ONLY);
    std::static_pointer_cast<FakeParam<uint16_t>>(m_params->Get<PARAM_SCAN_WIDTH>())
        ->ChangeRangeAttrs(1, 0, 0, 0, 0)
        .ChangeBaseAttrs(true, ACC_READ_ONLY);
    const auto gainCount = cGainCount[m_portIndex][m_speedIndex];
    std::static_pointer_cast<FakeParam<int16_t>>(m_params->Get<PARAM_GAIN_INDEX>())
        ->ChangeRangeAttrs(gainCount, 1, 1, gainCount, 1)
        .ChangeBaseAttrs(true, ACC_READ_WRITE);
    std::static_pointer_cast<FakeParam<int16_t>>(m_params->Get<PARAM_SPDTAB_INDEX>())
        ->ChangeRangeAttrs(cSpeedCount[m_portIndex], 0, 0, cSpeedCount[m_portIndex] - 1, 1)
        .ChangeBaseAttrs(true, ACC_READ_WRITE);
    std::static_pointer_cast<FakeParam<char*>>(m_params->Get<PARAM_GAIN_NAME>())
        ->ChangeRangeAttrs(cGainName[m_portIndex][m_speedIndex][m_gainIndex])
        .ChangeBaseAttrs(true, ACC_READ_ONLY);
    std::static_pointer_cast<FakeParam<char*>>(m_params->Get<PARAM_SPDTAB_NAME>())
        ->ChangeRangeAttrs(cSpeedName[m_portIndex][m_speedIndex])
        .ChangeBaseAttrs(true, ACC_READ_ONLY);
    std::static_pointer_cast<FakeParamEnum>(m_params->Get<PARAM_READOUT_PORT>())
        ->ChangeRangeAttrs(cReadoutPorts[m_portIndex].GetValue(), cReadoutPorts)
        .ChangeBaseAttrs(true, ACC_READ_WRITE);
    const auto pixTime = cPixTime[m_portIndex][m_speedIndex];
    std::static_pointer_cast<FakeParam<uint16_t>>(m_params->Get<PARAM_PIX_TIME>())
        ->ChangeRangeAttrs(1, pixTime, pixTime, pixTime, 0)
        .ChangeBaseAttrs(true, ACC_READ_ONLY);

    std::static_pointer_cast<FakeParam<uint16_t>>(m_params->Get<PARAM_SHTR_CLOSE_DELAY>())
        ->ChangeRangeAttrs(0, cShtrCloseDelayDef, 0, 65535, 1)
        .ChangeBaseAttrs(true, ACC_READ_WRITE);
    std::static_pointer_cast<FakeParam<uint16_t>>(m_params->Get<PARAM_SHTR_OPEN_DELAY>())
        ->ChangeRangeAttrs(0, cShtrOpenDelayDef, 0, 65535, 1)
        .ChangeBaseAttrs(true, ACC_READ_WRITE);
    std::static_pointer_cast<FakeParamEnum>(m_params->Get<PARAM_SHTR_OPEN_MODE>())
        ->ChangeRangeAttrs(cShtrOpenModeDef, cShtrOpenModes)
        .ChangeBaseAttrs(true, ACC_READ_WRITE);
    std::static_pointer_cast<FakeParamEnum>(m_params->Get<PARAM_SHTR_STATUS>())
        ->ChangeRangeAttrs(cShtrModeDef, cShtrModes)
        .ChangeBaseAttrs(true, ACC_READ_ONLY);

    std::static_pointer_cast<FakeParam<uint16_t>>(m_params->Get<PARAM_IO_ADDR>())
        ->ChangeRangeAttrs(cIoAddrCount, cIoAddrDef, 0, cIoAddrCount - 1, 1)
        .ChangeBaseAttrs(true, ACC_READ_WRITE);
    std::static_pointer_cast<FakeParamEnum>(m_params->Get<PARAM_IO_TYPE>())
        ->ChangeRangeAttrs(cIoType[cIoAddrDef], cIoTypes)
        .ChangeBaseAttrs(true, ACC_READ_ONLY);
    std::static_pointer_cast<FakeParamEnum>(m_params->Get<PARAM_IO_DIRECTION>())
        ->ChangeRangeAttrs(cIoDir[cIoAddrDef], cIoDirs)
        .ChangeBaseAttrs(true, ACC_READ_ONLY);
    std::static_pointer_cast<FakeParam<double>>(m_params->Get<PARAM_IO_STATE>())
        ->ChangeRangeAttrs(0,
                           cIoStateDef[cIoAddrDef],
                           cIoStateMin[cIoAddrDef],
                           cIoStateMax[cIoAddrDef],
                           0.0)
        .ChangeBaseAttrs(true, cIoStateAcc[cIoAddrDef]);
    std::static_pointer_cast<FakeParam<uint16_t>>(m_params->Get<PARAM_IO_BITDEPTH>())
        ->ChangeRangeAttrs(1,
                           cIoBitDepth[cIoAddrDef],
                           cIoBitDepth[cIoAddrDef],
                           cIoBitDepth[cIoAddrDef],
                           0)
        .ChangeBaseAttrs(true, ACC_READ_ONLY);

    std::static_pointer_cast<FakeParam<uint16_t>>(m_params->Get<PARAM_GAIN_MULT_FACTOR>())
        ->ChangeRangeAttrs(1000, 1, 1, 1000, 1)
        .ChangeBaseAttrs(false, ACC_READ_WRITE);
    std::static_pointer_cast<FakeParam<bool>>(m_params->Get<PARAM_GAIN_MULT_ENABLE>())
        ->ChangeRangeAttrs(true)
        .ChangeBaseAttrs(false, ACC_READ_ONLY);

    const auto& ppGrpIdx = cPpGroupIndex[m_portIndex][m_speedIndex];
    const auto& gPpIndexCount      = cPpIndexCount     [ppGrpIdx];
    const auto& gPpIndexDef        = cPpIndexDef       [ppGrpIdx];
    const auto& gPpFeatId          = cPpFeatId         [ppGrpIdx];
    const auto& gPpFeatName        = cPpFeatName       [ppGrpIdx];
    const auto& gPpParamIndexCount = cPpParamIndexCount[ppGrpIdx];
    const auto& gPpParamIndexDef   = cPpParamIndexDef  [ppGrpIdx];
    const auto& gPpParamId         = cPpParamId        [ppGrpIdx];
    const auto& gPpParamName       = cPpParamName      [ppGrpIdx];
    const auto& gPpParamDef        = cPpParamDef       [ppGrpIdx];
    const auto& gPpParamMin        = cPpParamMin       [ppGrpIdx];
    const auto& gPpParamMax        = cPpParamMax       [ppGrpIdx];
    const auto& gPpParamInc        = cPpParamInc       [ppGrpIdx];
    const auto& gPpParamCount      = cPpParamCount     [ppGrpIdx];
    std::static_pointer_cast<FakeParam<char*>>(m_params->Get<PARAM_PP_FEAT_NAME>())
        ->ChangeRangeAttrs(gPpFeatName[gPpIndexDef])
        .ChangeBaseAttrs(true, ACC_READ_ONLY);
    std::static_pointer_cast<FakeParam<int16_t>>(m_params->Get<PARAM_PP_INDEX>())
        ->ChangeRangeAttrs(gPpIndexCount, gPpIndexDef, 0, gPpIndexCount - 1, 1)
        .ChangeBaseAttrs(true, ACC_READ_WRITE);
    std::static_pointer_cast<FakeParam<uint16_t>>(m_params->Get<PARAM_ACTUAL_GAIN>())
        ->ChangeRangeAttrs(1, cActualGain, cActualGain, cActualGain, 0)
        .ChangeBaseAttrs(true, ACC_READ_ONLY);
    std::static_pointer_cast<FakeParam<int16_t>>(m_params->Get<PARAM_PP_PARAM_INDEX>())
        ->ChangeRangeAttrs(gPpParamIndexCount[gPpIndexDef],
                           gPpParamIndexDef  [gPpIndexDef],
                           0,
                           gPpParamIndexCount[gPpIndexDef] - 1,
                           1)
        .ChangeBaseAttrs(true, ACC_READ_WRITE);
    std::static_pointer_cast<FakeParam<char*>>(m_params->Get<PARAM_PP_PARAM_NAME>())
        ->ChangeRangeAttrs(gPpParamName[gPpIndexDef][gPpParamIndexDef[gPpIndexDef]])
        .ChangeBaseAttrs(true, ACC_READ_ONLY);
    std::static_pointer_cast<FakeParam<uint32_t>>(m_params->Get<PARAM_PP_PARAM>())
        ->ChangeRangeAttrs(gPpParamCount[gPpIndexDef][gPpParamIndexDef[gPpIndexDef]],
                           gPpParamDef  [gPpIndexDef][gPpParamIndexDef[gPpIndexDef]],
                           gPpParamMin  [gPpIndexDef][gPpParamIndexDef[gPpIndexDef]],
                           gPpParamMax  [gPpIndexDef][gPpParamIndexDef[gPpIndexDef]],
                           gPpParamInc  [gPpIndexDef][gPpParamIndexDef[gPpIndexDef]])
        .ChangeBaseAttrs(true, ACC_READ_WRITE);
    std::static_pointer_cast<FakeParam<uint16_t>>(m_params->Get<PARAM_READ_NOISE>())
        ->ChangeRangeAttrs(1, cReadNoise, cReadNoise, cReadNoise, 0)
        .ChangeBaseAttrs(true, ACC_READ_ONLY);
    std::static_pointer_cast<FakeParam<uint32_t>>(m_params->Get<PARAM_PP_FEAT_ID>())
        ->ChangeRangeAttrs(1, gPpFeatId[gPpIndexDef], gPpFeatId[gPpIndexDef], gPpFeatId[gPpIndexDef], 0)
        .ChangeBaseAttrs(true, ACC_READ_ONLY);
    std::static_pointer_cast<FakeParam<uint32_t>>(m_params->Get<PARAM_PP_PARAM_ID>())
        ->ChangeRangeAttrs(1,
                           gPpParamId[gPpIndexDef][gPpParamIndexDef[gPpIndexDef]],
                           gPpParamId[gPpIndexDef][gPpParamIndexDef[gPpIndexDef]],
                           gPpParamId[gPpIndexDef][gPpParamIndexDef[gPpIndexDef]],
                           0)
        .ChangeBaseAttrs(true, ACC_READ_ONLY);

    std::static_pointer_cast<FakeParam<bool>>(m_params->Get<PARAM_SMART_STREAM_MODE_ENABLED>())
        ->ChangeRangeAttrs(false)
        .ChangeBaseAttrs(true, ACC_READ_WRITE);
    std::static_pointer_cast<FakeParam<uint16_t>>(m_params->Get<PARAM_SMART_STREAM_MODE>())
        ->ChangeRangeAttrs(1, cSmartMode, cSmartMode, cSmartMode, 0)
        .ChangeBaseAttrs(true, ACC_READ_WRITE);
    std::static_pointer_cast<FakeParam<smart_stream_type*>>(m_params->Get<PARAM_SMART_STREAM_EXP_PARAMS>())
        ->ChangeRangeAttrs(cSmartCount, std::vector<uint32_t>{ 10, 20, 30 })
        .ChangeBaseAttrs(true, ACC_READ_WRITE);
    std::static_pointer_cast<FakeParam<smart_stream_type*>>(m_params->Get<PARAM_SMART_STREAM_DLY_PARAMS>())
        ->ChangeRangeAttrs(cSmartCount, std::vector<uint32_t>{ 100, 200, 300 })
        .ChangeBaseAttrs(true, ACC_READ_WRITE);

    std::static_pointer_cast<FakeParam<uint16_t>>(m_params->Get<PARAM_EXP_TIME>())
        ->ChangeRangeAttrs(0, cExpTime, 0, 65535, 1)
        .ChangeBaseAttrs(true, ACC_READ_WRITE);
    std::static_pointer_cast<FakeParamEnum>(m_params->Get<PARAM_EXP_RES>())
        ->ChangeRangeAttrs(cExpResDef, cExpRess)
        .ChangeBaseAttrs(true, ACC_READ_WRITE);
    std::static_pointer_cast<FakeParam<uint16_t>>(m_params->Get<PARAM_EXP_RES_INDEX>())
        ->ChangeRangeAttrs(3, cExpResIndexDef, EXP_RES_ONE_MILLISEC, EXP_RES_ONE_SEC, 1)
        .ChangeBaseAttrs(true, ACC_READ_WRITE);
    std::static_pointer_cast<FakeParam<uint64_t>>(m_params->Get<PARAM_EXPOSURE_TIME>())
        ->ChangeRangeAttrs(101, cExposureTimeDef, 0, 100, 1)
        .ChangeBaseAttrs(true, ACC_READ_ONLY);

    std::static_pointer_cast<FakeParamEnum>(m_params->Get<PARAM_BOF_EOF_ENABLE>())
        ->ChangeRangeAttrs(cIrqModeDef, cIrqModes)
        .ChangeBaseAttrs(true, ACC_READ_WRITE);
    std::static_pointer_cast<FakeParam<uint32_t>>(m_params->Get<PARAM_BOF_EOF_COUNT>())
        ->ChangeRangeAttrs(0, 0, 0, 0, 0)
        .ChangeBaseAttrs(true, ACC_READ_ONLY);
    std::static_pointer_cast<FakeParam<bool>>(m_params->Get<PARAM_BOF_EOF_CLR>())
        ->ChangeRangeAttrs(true)
        .ChangeBaseAttrs(true, ACC_WRITE_ONLY);

    std::static_pointer_cast<FakeParam<bool>>(m_params->Get<PARAM_CIRC_BUFFER>())
        ->ChangeRangeAttrs(true)
        .ChangeBaseAttrs(true, ACC_EXIST_CHECK_ONLY);
    std::static_pointer_cast<FakeParam<uint64_t>>(m_params->Get<PARAM_FRAME_BUFFER_SIZE>())
        ->ChangeRangeAttrs(0, 0, 0, 0, 0)
        .ChangeBaseAttrs(false, ACC_READ_ONLY);

    std::static_pointer_cast<FakeParamEnum>(m_params->Get<PARAM_BINNING_SER>())
        ->ChangeRangeAttrs(cBinSerDef, cBinSerModes)
        .ChangeBaseAttrs(true, ACC_READ_ONLY);
    std::static_pointer_cast<FakeParamEnum>(m_params->Get<PARAM_BINNING_PAR>())
        ->ChangeRangeAttrs(cBinParDef, cBinParModes)
        .ChangeBaseAttrs(true, ACC_READ_ONLY);

    std::static_pointer_cast<FakeParam<bool>>(m_params->Get<PARAM_METADATA_ENABLED>())
        ->ChangeRangeAttrs(true)
        .ChangeBaseAttrs(true, ACC_READ_WRITE);
    std::static_pointer_cast<FakeParam<uint16_t>>(m_params->Get<PARAM_ROI_COUNT>())
        ->ChangeRangeAttrs(cRoiCountMax, 1, 1, cRoiCountMax, 1)
        .ChangeBaseAttrs(true, ACC_READ_ONLY);
    std::static_pointer_cast<FakeParam<bool>>(m_params->Get<PARAM_CENTROIDS_ENABLED>())
        ->ChangeRangeAttrs(false)
        .ChangeBaseAttrs(true, ACC_READ_WRITE);
    std::static_pointer_cast<FakeParam<uint16_t>>(m_params->Get<PARAM_CENTROIDS_RADIUS>())
        ->ChangeRangeAttrs(cCentroidRadiusMax, cCentroidRadiusMax, 1, cCentroidRadiusMax, 1)
        .ChangeBaseAttrs(true, ACC_READ_WRITE);
    std::static_pointer_cast<FakeParam<uint16_t>>(m_params->Get<PARAM_CENTROIDS_COUNT>())
        ->ChangeRangeAttrs(cCentroidCountMax, cCentroidCountMax, 1, cCentroidCountMax, 1)
        .ChangeBaseAttrs(true, ACC_READ_WRITE);
    std::static_pointer_cast<FakeParamEnum>(m_params->Get<PARAM_CENTROIDS_MODE>())
        ->ChangeRangeAttrs(cCentroidModeDef, cCentroidModes)
        .ChangeBaseAttrs(true, ACC_READ_WRITE);
    std::static_pointer_cast<FakeParamEnum>(m_params->Get<PARAM_CENTROIDS_BG_COUNT>())
        ->ChangeRangeAttrs(cCentroidBgCountDef, cCentroidBgCountModes)
        .ChangeBaseAttrs(true, ACC_READ_WRITE);
    std::static_pointer_cast<FakeParam<uint32_t>>(m_params->Get<PARAM_CENTROIDS_THRESHOLD>())
        ->ChangeRangeAttrs(4080, 160, 16, 4095, 1)
        .ChangeBaseAttrs(true, ACC_READ_WRITE);

    std::static_pointer_cast<FakeParamEnum>(m_params->Get<PARAM_TRIGTAB_SIGNAL>())
        ->ChangeRangeAttrs(cTrigtabSignalDef, cTrigtabSignals)
        .ChangeBaseAttrs(true, ACC_READ_WRITE);
    std::static_pointer_cast<FakeParam<uint8_t>>(m_params->Get<PARAM_LAST_MUXED_SIGNAL>())
        ->ChangeRangeAttrs(cLastMuxedSignalMax - cLastMuxedSignalMin + 1,
                           cLastMuxedSignalDef,
                           cLastMuxedSignalMin,
                           cLastMuxedSignalMax,
                           1)
        .ChangeBaseAttrs(true, ACC_READ_WRITE);
    std::static_pointer_cast<FakeParamEnum>(m_params->Get<PARAM_FRAME_DELIVERY_MODE>())
        ->ChangeRangeAttrs(cFrameDeliveryModeDef, cFrameDeliveryModes)
        .ChangeBaseAttrs(true, ACC_READ_WRITE);

    // Connect change listeners for selected parameters

    std::shared_ptr<ParamBase> param;

    #define BIND_PARAM(id, listener)                                        \
        param = m_params->Get<id>();                                        \
        m_paramChangeHandleMap[param.get()] = param->RegisterChangeHandler( \
                std::bind(&FakeCamera::listener,                            \
                    this, std::placeholders::_1, std::placeholders::_2))

    BIND_PARAM(PARAM_IO_ADDR        , OnParamIoAddrChanged       );
    BIND_PARAM(PARAM_IO_STATE       , OnParamIoStateChanged      );
    BIND_PARAM(PARAM_SCAN_MODE      , OnParamScanModeChanged     );
    BIND_PARAM(PARAM_SCAN_LINE_DELAY, OnParamScanLineDelayChanged);
    BIND_PARAM(PARAM_SCAN_WIDTH     , OnParamScanWidthChanged    );
    BIND_PARAM(PARAM_GAIN_INDEX     , OnParamGainIndexChanged    );
    BIND_PARAM(PARAM_SPDTAB_INDEX   , OnParamSpdtabIndexChanged  );
    BIND_PARAM(PARAM_READOUT_PORT   , OnParamReadoutPortChanged  );
    BIND_PARAM(PARAM_PP_INDEX       , OnParamPpIndexChanged      );
    BIND_PARAM(PARAM_PP_PARAM_INDEX , OnParamPpParamIndexChanged );
    BIND_PARAM(PARAM_PP_PARAM       , OnParamPpParamChanged      );
    BIND_PARAM(PARAM_EXP_RES        , OnParamExpResChanged       );
    BIND_PARAM(PARAM_EXP_RES_INDEX  , OnParamExpResIndexChanged  );

    #undef BIND_PARAM
}

pm::FakeCamera::~FakeCamera()
{
    for (const auto& paramHandlePair : m_paramChangeHandleMap)
    {
        auto param = paramHandlePair.first;
        auto handle = paramHandlePair.second;
        param->UnregisterChangeHandler(handle);
    }
    m_paramChangeHandleMap.clear();
}

unsigned int pm::FakeCamera::GetTargetFps() const
{
    return m_targetFps;
}

void pm::FakeCamera::SetError(FakeCameraErrors error) const
{
    m_error = error;
}

bool pm::FakeCamera::InitLibrary()
{
    m_error = FakeCameraErrors::None;

    if (s_isInitialized)
        return true;

    Log::LogI("Using fake camera set to %u FPS\n", m_targetFps);

    s_isInitialized = true;
    return true;
}

bool pm::FakeCamera::UninitLibrary()
{
    m_error = FakeCameraErrors::None;

    if (!s_isInitialized)
        return true;

    s_isInitialized = false;
    return true;
}

bool pm::FakeCamera::GetCameraCount(int16& count) const
{
    m_error = FakeCameraErrors::None;

    if (!s_isInitialized)
    {
        Log::LogE("Fake camera not initialized");
        m_error = FakeCameraErrors::NotInitialized;
        return false;
    }

    count = 1;
    return true;
}

bool pm::FakeCamera::GetName(int16 index, std::string& name) const
{
    m_error = FakeCameraErrors::None;

    if (!s_isInitialized)
    {
        Log::LogE("Fake camera not initialized");
        m_error = FakeCameraErrors::NotInitialized;
        return false;
    }

    name.clear();

    if (index != 0)
    {
        Log::LogE("Failed to get name for camera at index %d", index);
        m_error = FakeCameraErrors::IndexOutOfRange;
        return false;
    }

    name = cCameraName;
    return true;
}

std::string pm::FakeCamera::GetErrorMessage() const
{
    switch (m_error)
    {
    case FakeCameraErrors::None:
        return "No errors";
    case FakeCameraErrors::Unknown:
        return "Unknown error";
    case FakeCameraErrors::NotInitialized:
        return "Camera not initialized";
    case FakeCameraErrors::CannotGetResource:
        return "Cannot get resource";
    case FakeCameraErrors::IndexOutOfRange:
        return "Index out of range";
    case FakeCameraErrors::CamNameNotFound:
        return "Camera name not found";
    case FakeCameraErrors::InvalidRoi:
        return "Invalid region(s)";
    case FakeCameraErrors::NotAvailable:
        return "Not available";
    case FakeCameraErrors::CannotSetValue:
        return "Cannot set parameter value";
    case FakeCameraErrors::CannotGetValue:
        return "Cannot get parameter value";
    }
    return "!UNKNOWN!";
}

bool pm::FakeCamera::Open(const std::string& name,
        CallbackEx3Fn removeCallbackHandler, void* removeCallbackContext)
{
    m_error = FakeCameraErrors::None;

    if (!s_isInitialized)
    {
        Log::LogE("Fake camera not initialized");
        m_error = FakeCameraErrors::NotInitialized;
        return false;
    }

    if (m_isOpen)
        return true;

    if (name != cCameraName)
    {
        Log::LogE("Failure opening camera '%s'", name.c_str());
        m_error = FakeCameraErrors::CamNameNotFound;
        return false;
    }

    m_hCam = 0;

    if (!Base::Open(name, removeCallbackHandler, removeCallbackContext))
    {
        m_error = FakeCameraErrors::CannotGetValue;
        return false;
    }

    return true;
}

bool pm::FakeCamera::Close()
{
    m_error = FakeCameraErrors::None;

    if (!s_isInitialized)
    {
        Log::LogE("Fake camera not initialized");
        m_error = FakeCameraErrors::NotInitialized;
        return false;
    }

    if (!m_isOpen)
        return true;

    DeleteBuffers();

    m_hCam = -1;

    return Base::Close();
}

bool pm::FakeCamera::SetupExp(const SettingsReader& settings)
{
    m_error = FakeCameraErrors::None;

    if (!s_isInitialized)
    {
        Log::LogE("Fake camera not initialized");
        m_error = FakeCameraErrors::NotInitialized;
        return false;
    }

    if (!Base::SetupExp(settings))
    {
        m_error = FakeCameraErrors::Unknown;
        return false;
    }

    if (m_settings.GetRegions().empty())
    {
        Log::LogE("No regions Specified");
        m_error = FakeCameraErrors::InvalidRoi;
        return false;
    }

    const auto trigMode = m_settings.GetTrigMode();
    switch (trigMode)
    {
    case VARIABLE_TIMED_MODE:
    case TIMED_MODE:
    case EXT_TRIG_INTERNAL:
    case EXT_TRIG_SOFTWARE_EDGE:
    case EXT_TRIG_SOFTWARE_FIRST:
        break; // OK, supported modes
    default:
        Log::LogE("Fake camera does not support HW trigger modes");
        m_error = FakeCameraErrors::NotAvailable;
        return false;
    }

    // Update non-writable fake parameters

    auto paramExposureTime = std::static_pointer_cast<FakeParam<uint64_t>>(
            m_params->Get<PARAM_EXPOSURE_TIME>());
    if ((paramExposureTime->IsAvail()))
    {
        const auto exposureTime = settings.GetExposure();
        paramExposureTime->SetCurNoHandlers(exposureTime, false);
    }

    auto paramRoiCount = std::static_pointer_cast<FakeParam<uint16_t>>(
            m_params->Get<PARAM_ROI_COUNT>());
    if ((paramRoiCount->IsAvail()))
    {
        const auto roiCount = static_cast<uint16_t>(settings.GetRegions().size());
        paramRoiCount->SetCurNoHandlers(roiCount, false);
    }

    // The binning is same for all ROIs
    auto paramBinSer = std::static_pointer_cast<FakeParamEnum>(
            m_params->Get<PARAM_BINNING_SER>());
    if ((paramBinSer->IsAvail()))
    {
        const auto sbin = static_cast<uint16_t>(settings.GetRegions().at(0).sbin);
        paramBinSer->SetCurNoHandlers(sbin, false);
    }
    auto paramBinPar = std::static_pointer_cast<FakeParamEnum>(
            m_params->Get<PARAM_BINNING_PAR>());
    if ((paramBinPar->IsAvail()))
    {
        const auto pbin = static_cast<uint16_t>(settings.GetRegions().at(0).sbin);
        paramBinPar->SetCurNoHandlers(pbin, false);
    }

    auto paramExposureMode = std::static_pointer_cast<FakeParamEnum>(
            m_params->Get<PARAM_EXPOSURE_MODE>());
    if ((paramExposureMode->IsAvail()))
    {
        const auto exposureMode = settings.GetTrigMode();
        paramExposureMode->SetCurNoHandlers(exposureMode, false);
    }
    auto paramExposeOutMode = std::static_pointer_cast<FakeParamEnum>(
            m_params->Get<PARAM_EXPOSE_OUT_MODE>());
    if ((paramExposeOutMode->IsAvail()))
    {
        const auto exposeOutMode = settings.GetExpOutMode();
        paramExposeOutMode->SetCurNoHandlers(exposeOutMode, false);
    }

    // TODO: Update PARAM_FRAME_BUFFER_SIZE

    // TODO: Update timing parameters:
    //       PARAM_READOUT_TIME, PARAM_CLEARING_TIME,
    //       PARAM_POST_TRIGGER_DELAY, PARAM_PRE_TRIGGER_DELAY

    // Prepare buffers

    const uint32_t frameCount = m_settings.GetBufferFrameCount();
    const size_t frameBytes = CalculateFrameBytes();

    if (frameBytes > std::numeric_limits<uint32_t>::max())
    {
        Log::LogE("Frame size over 4GiB not supported (%zu bytes)", frameBytes);
        m_error = FakeCameraErrors::Unknown;
        return false;
    }

    if (!AllocateBuffers(frameCount, static_cast<uint32_t>(frameBytes)))
        return false;

    m_framesMap.clear();
    for (auto& frame : m_frames)
    {
        frame->Invalidate();
    }

    InvokeAfterSetupParamChangeHandlers();
    return true;
}

bool pm::FakeCamera::StartExp(CallbackEx3Fn eofCallbackHandler,
        void* eofCallbackContext)
{
    assert(eofCallbackHandler);
    assert(eofCallbackContext);

    m_error = FakeCameraErrors::None;

    if (!s_isInitialized)
    {
        Log::LogE("Fake camera not initialized");
        m_error = FakeCameraErrors::NotInitialized;
        return false;
    }

    m_eofCallbackHandler = eofCallbackHandler;
    m_eofCallbackContext = eofCallbackContext;

    m_frameGenBufferPos = 0;
    m_frameGenFrameIndex = 0;

    m_startStopTimer.Reset();

    /* Start the frame generation thread, which is tasked with creating frames
       of the specified type and artificially calling the proper callback routine */
    m_frameGenStopFlag = false;
    m_frameGenThread =
        new(std::nothrow) std::thread(&FakeCamera::FrameGeneratorLoop, this);
    if (!m_frameGenThread)
    {
        Log::LogE("Failed to start the acquisition");
        m_error = FakeCameraErrors::CannotGetResource;
        return false;
    }

    m_isImaging = true;

    return true;
}

bool pm::FakeCamera::StopExp()
{
    m_error = FakeCameraErrors::None;

    if (!s_isInitialized)
    {
        Log::LogE("Fake camera not initialized");
        m_error = FakeCameraErrors::NotInitialized;
        return false;
    }

    if (m_isImaging)
    {
        if (m_frameGenThread)
        {
            // Tell frame generation thread we're done
            m_frameGenStopFlag = true;
            m_frameGenCond.notify_one();
            m_frameGenThread->join();
            delete m_frameGenThread;
            m_frameGenThread = nullptr;
        }

        m_isImaging = false;

        m_eofCallbackHandler = nullptr;
        m_eofCallbackContext = nullptr;
    }

    return true;
}

pm::Camera::AcqStatus pm::FakeCamera::GetAcqStatus() const
{
    return (m_isImaging)
        ? Camera::AcqStatus::Active
        : Camera::AcqStatus::Inactive;
}

bool pm::FakeCamera::PpReset()
{
    m_error = FakeCameraErrors::None;

    if (!s_isInitialized)
    {
        Log::LogE("Fake camera not initialized");
        m_error = FakeCameraErrors::NotInitialized;
        return false;
    }

    if (m_isImaging)
    {
        Log::LogE("Cannot reset PP features during running acquisition");
        m_error = FakeCameraErrors::NotAvailable;
        return false;
    }

    auto paramPpFeatIndex = std::static_pointer_cast<FakeParam<int16_t>>(
            m_params->Get<PARAM_PP_INDEX>());
    if (!paramPpFeatIndex || !paramPpFeatIndex->IsAvail())
    {
        Log::LogE("PP feature index parameter not available");
        m_error = FakeCameraErrors::NotAvailable;
        return false;
    }

    for (int16_t ppGrpIdx = 0; ppGrpIdx < cPpGroupCount; ++ppGrpIdx)
    {
        const auto& gPpIndexCount      = cPpIndexCount     [ppGrpIdx];
        const auto& gPpParamIndexCount = cPpParamIndexCount[ppGrpIdx];
        const auto& gPpParamDef        = cPpParamDef       [ppGrpIdx];
        auto&       gPpParam           = m_ppParam         [ppGrpIdx];

        for (int16_t ppFeatIdx = 0; ppFeatIdx < gPpIndexCount; ++ppFeatIdx)
        {
            for (int16_t ppParamIdx = 0; ppParamIdx < gPpParamIndexCount[ppFeatIdx]; ++ppParamIdx)
            {
                gPpParam[ppFeatIdx][ppParamIdx] = gPpParamDef[ppFeatIdx][ppParamIdx];
            }
        }
    }

    const auto& ppGrpIdx = cPpGroupIndex[m_portIndex][m_speedIndex];
    const auto& gPpIndexDef = cPpIndexDef[ppGrpIdx];
    paramPpFeatIndex->SetCurNoHandlers(gPpIndexDef, false);
    paramPpFeatIndex->InvokeChangeHandlers();

    return true;
}

bool pm::FakeCamera::Trigger()
{
    m_error = FakeCameraErrors::None;

    if (!s_isInitialized)
    {
        Log::LogE("Fake camera not initialized");
        m_error = FakeCameraErrors::NotInitialized;
        return false;
    }

    if (!m_isImaging)
    {
        Log::LogE("Cannot issue fake SW trigger without running acquisition");
        m_error = FakeCameraErrors::NotAvailable;
        return false;
    }

    const auto trigMode = m_settings.GetTrigMode();
    if (trigMode != EXT_TRIG_SOFTWARE_EDGE && trigMode != EXT_TRIG_SOFTWARE_FIRST)
    {
        Log::LogE("Cannot issue fake SW trigger without proper setup");
        m_error = FakeCameraErrors::NotAvailable;
        return false;
    }

    if (trigMode == EXT_TRIG_SOFTWARE_FIRST && m_frameGenFrameIndex > 0)
    {
        Log::LogE("Cannot issue fake 'SW trigger first' for other than first frame");
        m_error = FakeCameraErrors::NotAvailable;
        return false;
    }

    // Atomically sets m_frameGenSwTriggerFlag to true only if it was false
    bool expected = false;
    if (!m_frameGenSwTriggerFlag.compare_exchange_strong(expected, true))
    {
        // Not exchanged -> flag already set (not processed yet)
        Log::LogE("Fake camera didn't accept the trigger");
        m_error = FakeCameraErrors::NotAvailable;
        return false;
    }
    m_frameGenCond.notify_one();

    return true;
}

bool pm::FakeCamera::GetLatestFrame(Frame& frame) const
{
    size_t index;
    if (!GetLatestFrameIndex(index))
        return false;

    frame.Invalidate();
    return frame.Copy(*m_frames[index], false);
}

bool pm::FakeCamera::GetLatestFrameIndex(size_t& index, bool /*suppressCamErrMsg*/) const
{
    m_error = FakeCameraErrors::None;

    if (!s_isInitialized)
    {
        Log::LogE("Fake camera not initialized");
        m_error = FakeCameraErrors::NotInitialized;
        return false;
    }

    std::unique_lock<std::mutex> lock(m_frameGenMutex);

    index = m_frameGenBufferPos;

    m_frames[index]->Invalidate(); // Does some cleanup
    m_frames[index]->OverrideValidity(true);

    const uint32_t oldFrameNr = m_frames[index]->GetInfo().GetFrameNr();
    const Frame::Info fi(
            (uint32_t)m_frameGenFrameInfo.FrameNr,
            (uint64_t)m_frameGenFrameInfo.TimeStampBOF,
            (uint64_t)m_frameGenFrameInfo.TimeStamp,
            GetFrameExpTime((uint32_t)m_frameGenFrameInfo.FrameNr),
            m_settings.GetColorWbScaleRed(),
            m_settings.GetColorWbScaleGreen(),
            m_settings.GetColorWbScaleBlue());
    m_frames[index]->SetInfo(fi);
    UpdateFrameIndexMap(oldFrameNr, index);

    return true;
}

bool pm::FakeCamera::AllocateBuffers(uint32_t frameCount, uint32_t frameBytes)
{
    if (!Base::AllocateBuffers(frameCount, frameBytes))
    {
        m_error = FakeCameraErrors::CannotGetResource;
        return false;
    }

    const size_t bufferBytes = cMaxGenFrameCount * m_frameAcqCfg.GetFrameBytes();
    try
    {
        m_frameGenBuffer = std::make_unique<uint8_t[]>(bufferBytes);
    }
    catch (...)
    {
        m_frameGenBuffer = nullptr;
        Log::LogE("Failure allocating fake image buffer with %zu bytes", bufferBytes);
        m_error = FakeCameraErrors::CannotGetResource;
        return false;
    }

    if (m_usesCentroids && m_centroidsMode == PL_CENTROIDS_MODE_LOCATE)
    {
        const size_t bpp = m_frameAcqCfg.GetBitmapFormat().GetBytesPerPixel();

        const auto& regions = m_settings.GetRegions();
        const rgn_type rgn0 = regions.at(0);
        const uint32_t rgn0W = (rgn0.s2 + 1 - rgn0.s1) / rgn0.sbin;
        const uint32_t rgn0H = (rgn0.p2 + 1 - rgn0.p1) / rgn0.pbin;
        const size_t rgn0Bytes = bpp * rgn0W * rgn0H;

        try
        {
            m_frameGenRoi0Buffer = std::make_unique<uint8_t[]>(rgn0Bytes);
        }
        catch (...)
        {
            m_frameGenRoi0Buffer = nullptr;
            Log::LogE("Failure allocating fake buffer for bounding rectangle with %zu bytes",
                    rgn0Bytes);
            m_error = FakeCameraErrors::CannotGetResource;
            return false;
        }
    }

    if (!GenerateFrameData())
        return false;

    return true;
}

void pm::FakeCamera::DeleteBuffers()
{
    Base::DeleteBuffers();

    m_frameGenBuffer = nullptr;
    m_frameGenRoi0Buffer = nullptr;
}

void pm::FakeCamera::OnParamIoAddrChanged(ParamBase& /*param*/,
        bool /*allAttrsChanged*/)
{
    auto paramIoAddr = std::static_pointer_cast<FakeParam<uint16_t>>(
            m_params->Get<PARAM_IO_ADDR>());
    if (!paramIoAddr || !paramIoAddr->IsAvail())
        return;
    auto paramIoType = std::static_pointer_cast<FakeParamEnum>(
            m_params->Get<PARAM_IO_TYPE>());
    if (!paramIoType || !paramIoType->IsAvail())
        return;
    auto paramIoDir = std::static_pointer_cast<FakeParamEnum>(
            m_params->Get<PARAM_IO_DIRECTION>());
    if (!paramIoDir || !paramIoDir->IsAvail())
        return;
    auto paramIoState = std::static_pointer_cast<FakeParam<double>>(
            m_params->Get<PARAM_IO_STATE>());
    if (!paramIoState || !paramIoState->IsAvail())
        return;
    auto paramIoBitDepth = std::static_pointer_cast<FakeParam<uint16_t>>(
            m_params->Get<PARAM_IO_BITDEPTH>());
    if (!paramIoBitDepth || !paramIoBitDepth->IsAvail())
        return;

    const auto ioAddr = paramIoAddr->GetCur();

    paramIoType->SetCurNoHandlers(cIoType[ioAddr], false);
    paramIoDir->SetCurNoHandlers(cIoDir[ioAddr], false);

    paramIoState
        ->ChangeRangeAttrs(0,
                           cIoStateDef[ioAddr],
                           cIoStateMin[ioAddr],
                           cIoStateMax[ioAddr],
                           0.0)
        .ChangeBaseAttrs(true, cIoStateAcc[ioAddr]);
    // Restore the value from local storage for current indexes
    paramIoState->SetCurNoHandlers(m_ioState[ioAddr], false);

    paramIoBitDepth->ChangeRangeAttrs(
            1, cIoBitDepth[ioAddr], cIoBitDepth[ioAddr], cIoBitDepth[ioAddr], 0);
}

void pm::FakeCamera::OnParamIoStateChanged(ParamBase& /*param*/,
        bool /*allAttrsChanged*/)
{
    auto paramIoAddr = std::static_pointer_cast<FakeParam<uint16_t>>(
            m_params->Get<PARAM_IO_ADDR>());
    if (!paramIoAddr || !paramIoAddr->IsAvail())
        return;
    auto paramIoState = std::static_pointer_cast<FakeParam<double>>(
            m_params->Get<PARAM_IO_STATE>());
    if (!paramIoState || !paramIoState->IsAvail())
        return;

    const auto ioAddr = paramIoAddr->GetCur();

    // Update local storage to not loose the values when address change
    m_ioState[ioAddr] = paramIoState->GetCur();
}

void pm::FakeCamera::OnParamScanModeChanged(ParamBase & /*param*/,
        bool /*allAttrsChanged*/)
{
    auto paramScanMode = std::static_pointer_cast<FakeParamEnum>(
            m_params->Get<PARAM_SCAN_MODE>());
    if (!paramScanMode || !paramScanMode->IsAvail())
        return;
    auto paramScanLineDelay = std::static_pointer_cast<FakeParam<uint16_t>>(
            m_params->Get<PARAM_SCAN_LINE_DELAY>());
    if (!paramScanLineDelay || !paramScanLineDelay->IsAvail())
        return;
    auto paramScanWidth = std::static_pointer_cast<FakeParam<uint16_t>>(
            m_params->Get<PARAM_SCAN_WIDTH>());
    if (!paramScanWidth || !paramScanWidth->IsAvail())
        return;
    auto paramScanLineTime = std::static_pointer_cast<FakeParam<int64_t>>(
            m_params->Get<PARAM_SCAN_LINE_TIME>());
    if (!paramScanLineTime || !paramScanLineTime->IsAvail())
        return;

    const auto scanMode = paramScanMode->GetCur();
    switch (scanMode)
    {
    case PL_SCAN_MODE_PROGRAMMABLE_LINE_DELAY:
        paramScanLineDelay->ChangeBaseAttrs(true, ACC_READ_WRITE);
        paramScanLineDelay->ChangeRangeAttrs(65535, 1, 1, 65535, 1);
        paramScanWidth->ChangeBaseAttrs(true, ACC_READ_ONLY);
        paramScanWidth->ChangeRangeAttrs(cSensorHeight - 1, 1, 1, cSensorHeight - 1, 1);
        paramScanLineDelay->InvokeChangeHandlers();
        break;
    case PL_SCAN_MODE_PROGRAMMABLE_SCAN_WIDTH:
        paramScanLineDelay->ChangeBaseAttrs(true, ACC_READ_ONLY);
        paramScanLineDelay->ChangeRangeAttrs(65535, 1, 1, 65535, 1);
        paramScanWidth->ChangeBaseAttrs(true, ACC_READ_WRITE);
        paramScanWidth->ChangeRangeAttrs(cSensorHeight - 1, 1, 1, cSensorHeight - 1, 1);
        paramScanWidth->InvokeChangeHandlers();
        break;
    default:
        paramScanLineDelay->ChangeBaseAttrs(true, ACC_READ_ONLY);
        paramScanLineDelay->ChangeRangeAttrs(1, 0, 0, 0, 0);
        paramScanWidth->ChangeBaseAttrs(true, ACC_READ_ONLY);
        paramScanWidth->ChangeRangeAttrs(1, 0, 0, 0, 0);
        paramScanLineDelay->InvokeChangeHandlers(true);
        paramScanWidth->InvokeChangeHandlers(true);
        paramScanLineTime->InvokeChangeHandlers(true);
        break;
    }
}

void pm::FakeCamera::OnParamScanLineDelayChanged(ParamBase& /*param*/,
        bool allAttrsChanged)
{
    if (allAttrsChanged)
        return;

    auto paramScanLineDelay = std::static_pointer_cast<FakeParam<uint16_t>>(
            m_params->Get<PARAM_SCAN_LINE_DELAY>());
    if (!paramScanLineDelay || !paramScanLineDelay->IsAvail())
        return;
    auto paramScanWidth = std::static_pointer_cast<FakeParam<uint16_t>>(
            m_params->Get<PARAM_SCAN_WIDTH>());
    if (!paramScanWidth || !paramScanWidth->IsAvail())
        return;
    auto paramScanLineTime = std::static_pointer_cast<FakeParam<int64_t>>(
            m_params->Get<PARAM_SCAN_LINE_TIME>());
    if (!paramScanLineTime || !paramScanLineTime->IsAvail())
        return;

    auto paramExposureTime = m_params->Get<PARAM_EXPOSURE_TIME>();
    const auto exposureTime = (paramExposureTime->IsAvail())
        ? paramExposureTime->GetCur() : 0;
    auto paramExpRes = m_params->Get<PARAM_EXP_RES>();
    const auto expRes = (paramExpRes->IsAvail())
        ? paramExpRes->GetCur() : EXP_RES_ONE_MILLISEC;
    uint64_t exposureTimeNs;
    switch (expRes)
    {
    case EXP_RES_ONE_MICROSEC:
        exposureTimeNs = exposureTime * 1000;
        break;
    case EXP_RES_ONE_MILLISEC:
    default:
        exposureTimeNs = exposureTime * 1000 * 1000;
        break;
    case EXP_RES_ONE_SEC:
        exposureTimeNs = exposureTime * 1000 * 1000 * 1000;
        break;
    }

    const int64_t scanOneLineTime =
        (int64_t)cPixTime[m_portIndex][m_speedIndex] * cSensorWidth;

    const auto lineDelay = paramScanLineDelay->GetCur();
    const int64_t lineTime = scanOneLineTime * (1 + (int64_t)lineDelay);
    const auto scanWidth64 = (int64_t)(exposureTimeNs / lineTime);
    const auto scanWidth =
        (uint16_t)Utils::Clamp<int64_t>(scanWidth64, 1, cSensorHeight - 1);

    paramScanWidth->SetCurNoHandlers(scanWidth, false);
    paramScanLineTime->ChangeRangeAttrs(1, lineTime, lineTime, lineTime, 0);
}

void pm::FakeCamera::OnParamScanWidthChanged(ParamBase& /*param*/,
        bool allAttrsChanged)
{
    if (allAttrsChanged)
        return;

    auto paramScanLineDelay = std::static_pointer_cast<FakeParam<uint16_t>>(
            m_params->Get<PARAM_SCAN_LINE_DELAY>());
    if (!paramScanLineDelay || !paramScanLineDelay->IsAvail())
        return;
    auto paramScanWidth = std::static_pointer_cast<FakeParam<uint16_t>>(
            m_params->Get<PARAM_SCAN_WIDTH>());
    if (!paramScanWidth || !paramScanWidth->IsAvail())
        return;
    auto paramScanLineTime = std::static_pointer_cast<FakeParam<int64_t>>(
            m_params->Get<PARAM_SCAN_LINE_TIME>());
    if (!paramScanLineTime || !paramScanLineTime->IsAvail())
        return;

    auto paramExposureTime = m_params->Get<PARAM_EXPOSURE_TIME>();
    const auto exposureTime = (paramExposureTime->IsAvail())
        ? paramExposureTime->GetCur() : 0;
    auto paramExpRes = m_params->Get<PARAM_EXP_RES>();
    const auto expRes = (paramExpRes->IsAvail())
        ? paramExpRes->GetCur() : EXP_RES_ONE_MILLISEC;
    uint64_t exposureTimeNs;
    switch (expRes)
    {
    case EXP_RES_ONE_MICROSEC:
        exposureTimeNs = exposureTime * 1000;
        break;
    case EXP_RES_ONE_MILLISEC:
    default:
        exposureTimeNs = exposureTime * 1000 * 1000;
        break;
    case EXP_RES_ONE_SEC:
        exposureTimeNs = exposureTime * 1000 * 1000 * 1000;
        break;
    }

    const int64_t scanOneLineTime =
        (int64_t)cPixTime[m_portIndex][m_speedIndex] * cSensorWidth;

    const auto scanWidth = paramScanWidth->GetCur();
    const int64_t lineTime = (int64_t)(exposureTimeNs / scanWidth);
    const auto lineDelay64 = (lineTime / scanOneLineTime) - 1;
    const auto lineDelay = (uint16_t)Utils::Clamp<int64_t>(lineDelay64, 0, 65535);

    paramScanLineDelay->SetCurNoHandlers(lineDelay, false);
    paramScanLineTime->ChangeRangeAttrs(1, lineTime, lineTime, lineTime, 0);
}

void pm::FakeCamera::OnParamGainIndexChanged(ParamBase& /*param*/,
        bool /*allAttrsChanged*/)
{
    auto paramGainIndex = std::static_pointer_cast<FakeParam<int16_t>>(
            m_params->Get<PARAM_GAIN_INDEX>());
    if (!paramGainIndex || !paramGainIndex->IsAvail())
        return;
    auto paramBitDepth = std::static_pointer_cast<FakeParam<int16_t>>(
            m_params->Get<PARAM_BIT_DEPTH>());
    if (!paramBitDepth || !paramBitDepth->IsAvail())
        return;
    // Can be unavailable
    auto paramGainName = std::static_pointer_cast<FakeParam<char*>>(
            m_params->Get<PARAM_GAIN_NAME>());

    m_gainIndex = paramGainIndex->GetCur() - 1;

    const auto bitDepth = cBitDepth[m_portIndex][m_speedIndex][m_gainIndex];
    paramBitDepth->ChangeRangeAttrs(1, bitDepth, bitDepth, bitDepth, 0);

    if (paramGainName && paramGainName->IsAvail())
    {
        const char* gainName = cGainName[m_portIndex][m_speedIndex][m_gainIndex];
        paramGainName->ChangeRangeAttrs(gainName);
    }
}

void pm::FakeCamera::OnParamSpdtabIndexChanged(ParamBase& /*param*/,
        bool /*allAttrsChanged*/)
{
    auto paramSpdtabIndex = std::static_pointer_cast<FakeParam<int16_t>>(
            m_params->Get<PARAM_SPDTAB_INDEX>());
    if (!paramSpdtabIndex || !paramSpdtabIndex->IsAvail())
        return;
    auto paramPixTime = std::static_pointer_cast<FakeParam<uint16_t>>(
            m_params->Get<PARAM_PIX_TIME>());
    if (!paramPixTime || !paramPixTime->IsAvail())
        return;
    auto paramGainIndex = std::static_pointer_cast<FakeParam<int16_t>>(
            m_params->Get<PARAM_GAIN_INDEX>());
    if (!paramGainIndex || !paramGainIndex->IsAvail())
        return;
    // Can be unavailable
    auto paramSpeedName = std::static_pointer_cast<FakeParam<char*>>(
            m_params->Get<PARAM_SPDTAB_NAME>());
    auto paramImageFormat = std::static_pointer_cast<FakeParamEnum>(
            m_params->Get<PARAM_IMAGE_FORMAT>());
    auto paramImageCompression = std::static_pointer_cast<FakeParamEnum>(
            m_params->Get<PARAM_IMAGE_COMPRESSION>());

    m_speedIndex = paramSpdtabIndex->GetCur();
    m_gainIndex = 0; // Reset on speed change

    const auto pixTime = cPixTime[m_portIndex][m_speedIndex];
    paramPixTime->ChangeRangeAttrs(1, pixTime, pixTime, pixTime, 0);

    if (paramSpeedName && paramSpeedName->IsAvail())
    {
        const char* speedName = cSpeedName[m_portIndex][m_speedIndex];
        paramSpeedName->ChangeRangeAttrs(speedName);
    }

    if (paramImageFormat && paramImageFormat->IsAvail())
    {
        const auto imageFormat = cImageFormat[m_portIndex][m_speedIndex];
        paramImageFormat->ChangeRangeAttrs(imageFormat, cImageFormats);
    }

    if (paramImageCompression && paramImageCompression->IsAvail())
    {
        const auto imageCompression = cImageCompression[m_portIndex][m_speedIndex];
        paramImageCompression->ChangeRangeAttrs(imageCompression, cImageCompressions);
    }

    const auto gainCount = cGainCount[m_portIndex][m_speedIndex];
    paramGainIndex->ChangeRangeAttrs(gainCount, 1, 1, gainCount, 1);

    OnParamGainIndexChanged(*paramGainIndex, true);
}

void pm::FakeCamera::OnParamReadoutPortChanged(ParamBase& /*param*/,
        bool /*allAttrsChanged*/)
{
    auto paramReadoutPort = std::static_pointer_cast<FakeParamEnum>(
            m_params->Get<PARAM_READOUT_PORT>());
    if (!paramReadoutPort || !paramReadoutPort->IsAvail())
        return;
    auto paramSpdtabIndex = std::static_pointer_cast<FakeParam<int16_t>>(
            m_params->Get<PARAM_SPDTAB_INDEX>());
    if (!paramSpdtabIndex || !paramSpdtabIndex->IsAvail())
        return;

    int16_t portIndex = -1;
    const auto portValue = paramReadoutPort->GetCur();
    for (int16_t n = 0; (size_t)n < cReadoutPorts.size(); n++)
    {
        if (cReadoutPorts.at(n).GetValue() == portValue)
        {
            portIndex = n;
            break;
        }
    }
    if (portIndex < 0)
        return;

    m_portIndex = portIndex;
    m_speedIndex = 0; // Reset on port change

    const auto speedCount = cSpeedCount[m_portIndex];
    paramSpdtabIndex->ChangeRangeAttrs(speedCount, 0, 0, speedCount - 1, 1);

    OnParamSpdtabIndexChanged(*paramSpdtabIndex, true);
}

void pm::FakeCamera::OnParamPpIndexChanged(ParamBase& /*param*/,
        bool /*allAttrsChanged*/)
{
    auto paramPpFeatIndex = std::static_pointer_cast<FakeParam<int16_t>>(
            m_params->Get<PARAM_PP_INDEX>());
    if (!paramPpFeatIndex || !paramPpFeatIndex->IsAvail())
        return;
    auto paramPpParamIndex = std::static_pointer_cast<FakeParam<int16_t>>(
            m_params->Get<PARAM_PP_PARAM_INDEX>());
    if (!paramPpParamIndex || !paramPpParamIndex->IsAvail())
        return;

    const auto& ppGrpIdx = cPpGroupIndex[m_portIndex][m_speedIndex];
    const auto& gPpFeatId          = cPpFeatId         [ppGrpIdx];
    const auto& gPpFeatName        = cPpFeatName       [ppGrpIdx];
    const auto& gPpParamIndexCount = cPpParamIndexCount[ppGrpIdx];
    const auto& gPpParamIndexDef   = cPpParamIndexDef  [ppGrpIdx];

    const auto ppFeatIndex = paramPpFeatIndex->GetCur();

    auto paramPpFeatId = std::static_pointer_cast<FakeParam<uint32_t>>(
            m_params->Get<PARAM_PP_FEAT_ID>());
    if (paramPpFeatId && paramPpFeatId->IsAvail())
    {
        const uint32_t ppFeatId = gPpFeatId[ppFeatIndex];
        paramPpFeatId->ChangeRangeAttrs(1, ppFeatId, ppFeatId, ppFeatId, 0);
    }

    auto paramPpFeatName = std::static_pointer_cast<FakeParam<char*>>(
            m_params->Get<PARAM_PP_FEAT_NAME>());
    if (paramPpFeatName && paramPpFeatName->IsAvail())
    {
        const char* ppFeatName = gPpFeatName[ppFeatIndex];
        paramPpFeatName->ChangeRangeAttrs(ppFeatName);
    }

    const int16_t ppParamIndex = gPpParamIndexDef[ppFeatIndex]; // Reset on feature change
    const int16_t ppParamIndexCount = gPpParamIndexCount[ppFeatIndex];
    paramPpParamIndex->ChangeRangeAttrs(
            ppParamIndexCount, ppParamIndex, 0, ppParamIndexCount - 1, 1);

    OnParamPpParamIndexChanged(*paramPpParamIndex, true);
}

void pm::FakeCamera::OnParamPpParamIndexChanged(ParamBase& /*param*/,
        bool /*allAttrsChanged*/)
{
    auto paramPpFeatIndex = std::static_pointer_cast<FakeParam<int16_t>>(
            m_params->Get<PARAM_PP_INDEX>());
    if (!paramPpFeatIndex || !paramPpFeatIndex->IsAvail())
        return;
    auto paramPpParamIndex = std::static_pointer_cast<FakeParam<int16_t>>(
            m_params->Get<PARAM_PP_PARAM_INDEX>());
    if (!paramPpParamIndex || !paramPpParamIndex->IsAvail())
        return;
    auto paramPpParamValue = std::static_pointer_cast<FakeParam<uint32_t>>(
            m_params->Get<PARAM_PP_PARAM>());
    if (!paramPpParamValue || !paramPpParamValue->IsAvail())
        return;

    const auto ppFeatIndex = paramPpFeatIndex->GetCur();
    const auto ppParamIndex = paramPpParamIndex->GetCur();

    const auto& ppGrpIdx = cPpGroupIndex[m_portIndex][m_speedIndex];
    const auto& gPpParamId         = cPpParamId        [ppGrpIdx];
    const auto& gPpParamName       = cPpParamName      [ppGrpIdx];
    const auto& gPpParamDef        = cPpParamDef       [ppGrpIdx];
    const auto& gPpParamMin        = cPpParamMin       [ppGrpIdx];
    const auto& gPpParamMax        = cPpParamMax       [ppGrpIdx];
    const auto& gPpParamInc        = cPpParamInc       [ppGrpIdx];
    const auto& gPpParamCount      = cPpParamCount     [ppGrpIdx];
    auto&       gPpParam           = m_ppParam         [ppGrpIdx];

    paramPpParamValue->ChangeRangeAttrs(
            gPpParamCount[ppFeatIndex][ppParamIndex],
            gPpParamDef  [ppFeatIndex][ppParamIndex],
            gPpParamMin  [ppFeatIndex][ppParamIndex],
            gPpParamMax  [ppFeatIndex][ppParamIndex],
            gPpParamInc  [ppFeatIndex][ppParamIndex]);
    // Restore the value from local storage for current indexes
    paramPpParamValue->SetCurNoHandlers(
            gPpParam     [ppFeatIndex][ppParamIndex]);

    auto paramPpParamId = std::static_pointer_cast<FakeParam<uint32_t>>(
            m_params->Get<PARAM_PP_PARAM_ID>());
    if (paramPpParamId && paramPpParamId->IsAvail())
    {
        const uint32_t ppParamId = gPpParamId[ppFeatIndex][ppParamIndex];
        paramPpParamId->ChangeRangeAttrs(1, ppParamId, ppParamId, ppParamId, 0);
    }

    auto paramPpParamName = std::static_pointer_cast<FakeParam<char*>>(
            m_params->Get<PARAM_PP_PARAM_NAME>());
    if (paramPpParamName && paramPpParamName->IsAvail())
    {
        const char* ppParamName = gPpParamName[ppFeatIndex][ppParamIndex];
        paramPpParamName->ChangeRangeAttrs(ppParamName);
    }
}

void pm::FakeCamera::OnParamPpParamChanged(ParamBase& /*param*/,
        bool /*allAttrsChanged*/)
{
    auto paramPpFeatIndex = std::static_pointer_cast<FakeParam<int16_t>>(
            m_params->Get<PARAM_PP_INDEX>());
    if (!paramPpFeatIndex || !paramPpFeatIndex->IsAvail())
        return;
    auto paramPpParamIndex = std::static_pointer_cast<FakeParam<int16_t>>(
            m_params->Get<PARAM_PP_PARAM_INDEX>());
    if (!paramPpParamIndex || !paramPpParamIndex->IsAvail())
        return;
    auto paramPpParamValue = std::static_pointer_cast<FakeParam<uint32_t>>(
            m_params->Get<PARAM_PP_PARAM>());
    if (!paramPpParamValue || !paramPpParamValue->IsAvail())
        return;

    const auto ppFeatIndex = paramPpFeatIndex->GetCur();
    const auto ppParamIndex = paramPpParamIndex->GetCur();

    const auto& ppGrpIdx = cPpGroupIndex[m_portIndex][m_speedIndex];
    auto& gPpParam = m_ppParam[ppGrpIdx];

    // Update local storage to not loose the values when indexes change
    gPpParam[ppFeatIndex][ppParamIndex] = paramPpParamValue->GetCur();

    // TODO: Handle e.g. bit depth change when turned on frame summing feature
}

void pm::FakeCamera::OnParamExpResChanged(ParamBase& /*param*/,
        bool /*allAttrsChanged*/)
{
    auto paramExpResIndex = std::static_pointer_cast<FakeParam<uint16_t>>(
            m_params->Get<PARAM_EXP_RES_INDEX>());
    if (!paramExpResIndex || !paramExpResIndex->IsAvail())
        return;

    auto paramExpRes = m_params->Get<PARAM_EXP_RES>();
    const auto value = static_cast<uint16_t>(paramExpRes->GetCur());

    // Mimic real camera - ExpRes and ExpResIndex work on same data, sync values
    paramExpResIndex->SetCurNoHandlers(value);
}

void pm::FakeCamera::OnParamExpResIndexChanged(ParamBase& /*param*/,
        bool /*allAttrsChanged*/)
{
    auto paramExpRes = std::static_pointer_cast<FakeParamEnum>(
            m_params->Get<PARAM_EXP_RES>());
    if (!paramExpRes || !paramExpRes->IsAvail())
        return;
    // Older PVCAM had ExpRes param read-only
    if (paramExpRes->GetAccess() != ACC_READ_WRITE)
        return;

    auto paramExpResIndex = m_params->Get<PARAM_EXP_RES_INDEX>();
    const auto value = static_cast<int32_t>(paramExpResIndex->GetCur());

    // Mimic real camera - ExpRes and ExpResIndex work on same data, sync values
    paramExpRes->SetCurNoHandlers(value);
}

size_t pm::FakeCamera::CalculateFrameBytes() const
{
    const size_t bpp = m_bmpFormat.GetBytesPerPixel();

    const auto& regions = m_settings.GetRegions();
    const rgn_type rgn0 = regions.at(0);
    const uint32_t rgn0W = (rgn0.s2 + 1 - rgn0.s1) / rgn0.sbin;
    const uint32_t rgn0H = (rgn0.p2 + 1 - rgn0.p1) / rgn0.pbin;
    const size_t rgn0Bytes = bpp * rgn0W * rgn0H;

    const uint32_t centroidEdge = 2 * m_centroidsRadius + 1;
    const size_t centroidBytes = bpp * centroidEdge * centroidEdge;

    // 1. case: Single ROI acquisition without metadata
    if (!m_usesMetadata)
    {
        return rgn0Bytes;
    }

    // Frame with metadata
    size_t frameBytes = sizeof(md_frame_header_v3);

    // 2. case: Single- or Multi-ROI acquisition with metadata
    if (!m_usesCentroids)
    {
        for (auto& rgn : regions)
        {
            const uint32_t rgnW = (rgn.s2 + 1 - rgn.s1) / rgn.sbin;
            const uint32_t rgnH = (rgn.p2 + 1 - rgn.p1) / rgn.pbin;
            const size_t rgnBytes = bpp * rgnW * rgnH;
            frameBytes += sizeof(md_frame_roi_header) + rgnBytes;
        }
        return frameBytes;
    }

    // Single ROI acquisition with some centroids

    // 3. case: Single ROI acquisition with Locate data
    if (m_centroidsMode == PL_CENTROIDS_MODE_LOCATE)
    {
        const uint16_t count = m_centroidsCount;

        // Each centroid has its own header, data and no ext. metadata
        frameBytes += count * (sizeof(md_frame_roi_header) + centroidBytes);
        return frameBytes;
    }

    // 4. case: Single ROI acquisition with Track data
    if (m_centroidsMode == PL_CENTROIDS_MODE_TRACK)
    {
        const uint16_t count = m_centroidsCount + 1;

        // Background ROI size (full ROI image, includes invalid ext. metadata)
        const size_t bgRoiBytes = sizeof(md_frame_roi_header) + rgn0Bytes
            + m_trackRoiExtMdBytes;
        // Patch ROI size (header only, no image data)
        const size_t patchTotalBytes = sizeof(md_frame_roi_header)
            + m_trackRoiExtMdBytes;

        // One full background ROI and each centroid has only its own header,
        // data and ext. metadata
        frameBytes += bgRoiBytes + count * patchTotalBytes;
        return frameBytes;
    }

    // 5. case: Single ROI acquisition with Blob data
    if (m_centroidsMode == PL_CENTROIDS_MODE_BLOB)
    {
        const uint16_t count = m_centroidsCount + 1;

        // Background ROI size (full ROI image, includes invalid ext. metadata)
        const size_t bgRoiBytes = sizeof(md_frame_roi_header) + rgn0Bytes;
        // Patch ROI size (header only, no image data)
        const size_t patchTotalBytes = sizeof(md_frame_roi_header);

        // One full background ROI and each centroid has only its own header,
        // data and ext. metadata
        frameBytes += bgRoiBytes + count * patchTotalBytes;
        return frameBytes;
    }

    return 0;
}

uint16_t pm::FakeCamera::GetExtMdBytes(PL_MD_EXT_TAGS tagId) const
{
    uint16_t bytes = 0;
    if (g_pl_ext_md_map.count(tagId) > 0)
    {
        bytes += sizeof(uint8_t); // 1 byte for tagId
        bytes += g_pl_ext_md_map.at(tagId).size;
    }
    return bytes;
}

void pm::FakeCamera::SetExtMdData(PL_MD_EXT_TAGS tagId, uint8_t** dstBuffer,
        const void* data)
{
    if (g_pl_ext_md_map.count(tagId) > 0)
    {
        **dstBuffer = (uint8_t)tagId;
        *dstBuffer += sizeof(uint8_t);
        const uint16_t dataBytes = g_pl_ext_md_map.at(tagId).size;
        memcpy(*dstBuffer, data, dataBytes);
        *dstBuffer += dataBytes;
    }
}

uint32_t pm::FakeCamera::GetRandomNumber()
{
    return std::random_device()();
}

void pm::FakeCamera::GenerateRoiData(void* const dstBuffer, size_t dstBytes)
{
    // Normally, the background noise would be shifted to ADC offset
    //const auto adcOffset = m_params->Get<PARAM_ADC_OFFSET>()->GetCur();

    // But we will rather generate the noise based on current bit depth
    // so the pixel values change with various settings
    const auto bitDepth = cBitDepth[m_portIndex][m_speedIndex][m_gainIndex];
    const auto offset = static_cast<uint32_t>(::pow(2, bitDepth) * 1 / 8);

    // TODO: Call RandomPixelCache::Update on bit depth change only

    switch (m_frameAcqCfg.GetBitmapFormat().GetDataType())
    {
    case BitmapDataType::UInt8:
        m_randomPixelCache8->Update(static_cast<uint8_t>(offset));
        m_randomPixelCache8->Fill(dstBuffer, dstBytes);
        break;
    case BitmapDataType::UInt16:
        m_randomPixelCache16->Update(static_cast<uint16_t>(offset));
        m_randomPixelCache16->Fill(dstBuffer, dstBytes);
        break;
    case BitmapDataType::UInt32:
        m_randomPixelCache32->Update(static_cast<uint32_t>(offset));
        m_randomPixelCache32->Fill(dstBuffer, dstBytes);
        break;
    default:
        throw Exception("Unsupported bitmap data type");
    }
}

void pm::FakeCamera::AppendParticleData(void* const dstBuffer,
        const rgn_type& dstRgn, const void* srcBuffer, const rgn_type& srcRgn)
{
    assert(dstRgn.sbin == 1);
    assert(dstRgn.pbin == 1);
    assert(srcRgn.sbin == 1);
    assert(srcRgn.pbin == 1);

    uint8_t* const dst = static_cast<uint8_t* /*const*/>(dstBuffer);
    const uint8_t* src = static_cast<const uint8_t*    >(srcBuffer);

    const uint16_t dstX = dstRgn.s1 / dstRgn.sbin;
    const uint16_t dstY = dstRgn.p1 / dstRgn.pbin;
    const uint32_t dstW = (dstRgn.s2 + 1 - dstRgn.s1) / dstRgn.sbin;
    const uint32_t dstH = (dstRgn.p2 + 1 - dstRgn.p1) / dstRgn.pbin;

    const uint16_t srcX = srcRgn.s1 / srcRgn.sbin;
    const uint16_t srcY = srcRgn.p1 / srcRgn.pbin;
    const uint32_t srcW = (srcRgn.s2 + 1 - srcRgn.s1) / srcRgn.sbin;
    //const uint32_t srcH = (srcRgn.p2 + 1 - srcRgn.p1) / srcRgn.pbin;

    const uint16_t srcOffX = dstX - srcX;
    const uint16_t srcOffY = dstY - srcY;

    // Copy each row one by one to the destination buffer
    const size_t bpp = m_frameAcqCfg.GetBitmapFormat().GetBytesPerPixel();
    const size_t dstBprl = bpp * dstW; ///< Bytes per rect line
    const size_t srcBprl = bpp * srcW; ///< Bytes per rect line
    const size_t srcBpoX = bpp * srcOffX; ///< Bytes per rect horiz. offset

    for (uint32_t dY = 0, sY = srcOffY; dY < dstH; ++dY, ++sY)
    {
        // Calculate address of the target row in the destination buffer
        uint8_t* const dstLine = dst + dY * dstBprl;
        // Calculate address of the source row in the ROI data buffer
        const uint8_t* srcLine = src + sY * srcBprl + srcBpoX;
        memcpy(dstLine, srcLine, dstBprl);
    }
}

template <typename T>
void pm::FakeCamera::InjectParticlesT(T* const dstBuffer, const rgn_type& dstRgn,
        const std::vector<std::pair<uint16_t, uint16_t>>& particleCoordinates)
{
    assert(dstRgn.sbin == 1);
    assert(dstRgn.pbin == 1);

    const uint16_t dstX = dstRgn.s1 / dstRgn.sbin;
    const uint16_t dstY = dstRgn.p1 / dstRgn.pbin;
    const uint32_t dstW = (dstRgn.s2 + 1 - dstRgn.s1) / dstRgn.sbin;
    const uint32_t dstH = (dstRgn.p2 + 1 - dstRgn.p1) / dstRgn.pbin;

    // Min. radius is 1 thus min. size is 3 (2*r+1)
    assert(dstW >= 3);
    assert(dstH >= 3);

    // A particle is a brighter pixel inside the given ROI
    const auto bitDepth = cBitDepth[m_portIndex][m_speedIndex][m_gainIndex];
    const T fgSampleValue = static_cast<T>(::pow(2, bitDepth) * 3 / 4);

    const uint8_t spp = m_frameAcqCfg.GetBitmapFormat().GetSamplesPerPixel();

    for (const auto& coordinate : particleCoordinates)
    {
        // Convert absolute sensor coordinates to be relative to ROI origin
        const uint16_t centerX = coordinate.first - dstX;
        const uint16_t centerY = coordinate.second - dstY;

        // Calculate pixel offsets
        const size_t indexC = (size_t)spp * dstW * centerY + centerX;
        const size_t indexL = (size_t)spp * dstW * centerY + centerX - 1;
        const size_t indexR = (size_t)spp * dstW * centerY + centerX + 1;
        const size_t indexT = (size_t)spp * dstW * ((size_t)centerY - 1) + centerX;
        const size_t indexB = (size_t)spp * dstW * ((size_t)centerY + 1) + centerX;

        // "Draw" a plus sign, all samples in pixel have same value, are gray
        for (uint8_t n = 0; n < spp; ++n)
        {
            dstBuffer[indexC + n] = fgSampleValue;
            dstBuffer[indexL + n] = fgSampleValue;
            dstBuffer[indexR + n] = fgSampleValue;
            dstBuffer[indexT + n] = fgSampleValue;
            dstBuffer[indexB + n] = fgSampleValue;
        }
    }
}

void pm::FakeCamera::InjectParticles(void* const dstBuffer, const rgn_type& dstRgn)
{
    switch (m_frameAcqCfg.GetBitmapFormat().GetDataType())
    {
    case BitmapDataType::UInt8:
        InjectParticlesT(static_cast<uint8_t* /*const*/>(dstBuffer), dstRgn,
                m_particleCoordinates);
        break;
    case BitmapDataType::UInt16:
        InjectParticlesT(static_cast<uint16_t* /*const*/>(dstBuffer), dstRgn,
                m_particleCoordinates);
        break;
    case BitmapDataType::UInt32:
        InjectParticlesT(static_cast<uint32_t* /*const*/>(dstBuffer), dstRgn,
                m_particleCoordinates);
        break;
    default:
        throw Exception("Unsupported bitmap data type");
    }
}

md_frame_header_v3 pm::FakeCamera::GenerateFrameHeader()
{
    md_frame_header_v3 header{};

    header.signature = PL_MD_FRAME_SIGNATURE;
    // Version 2 and above reports ROI data size in the ROI header
    // Version 3 reports precise timestamps in picoseconds
    header.version = 3;

    // For acquisition with any kind of centroids, set to centroids count
    header.roiCount = static_cast<uint16_t>(m_settings.GetRegions().size());

    header.bitDepth = static_cast<uint8_t>(cBitDepth[m_portIndex][m_speedIndex][m_gainIndex]);
    header.colorMask = static_cast<uint8_t>(cColorMode[m_portIndex][m_speedIndex]);
    header.flags = 0x00;
    header.extendedMdSize = 0;
    header.imageFormat = static_cast<uint8_t>(cImageFormat[m_portIndex][m_speedIndex]);
    header.imageCompression = static_cast<uint8_t>(cImageCompression[m_portIndex][m_speedIndex]);

    // Following members are set once frame is copied to circular buffer
    header.frameNr = 0; // Frame numbers are 1-based
    header.timestampBOF = 0;
    header.timestampEOF = 0;
    header.exposureTime = 0;

    return header;
}

md_frame_roi_header pm::FakeCamera::GenerateRoiHeader(uint16_t roiIndex,
        const rgn_type& rgn)
{
    md_frame_roi_header roiHdr{};

    const size_t bpp = m_frameAcqCfg.GetBitmapFormat().GetBytesPerPixel();
    const uint32_t rgnW = (rgn.s2 + 1 - rgn.s1) / rgn.sbin;
    const uint32_t rgnH = (rgn.p2 + 1 - rgn.p1) / rgn.pbin;
    const size_t rgnBytes = bpp * rgnW * rgnH;

    // Fill-in all ROI header data
    roiHdr.roiNr = roiIndex + 1; // ROI numbers are 1-based
    roiHdr.roi = rgn;
    roiHdr.extendedMdSize = 0;
    roiHdr.flags = 0;
    roiHdr.roiDataSize = static_cast<uint32_t>(rgnBytes); // Since version 2

    // Following members are set once frame is copied to circular buffer
    roiHdr.timestampBOR = 0;
    roiHdr.timestampEOR = 0;

    return roiHdr;
}

md_frame_roi_header pm::FakeCamera::GenerateParticleHeader(uint16_t roiIndex,
        uint16_t centerX, uint16_t centerY)
{
    const uint16_t radius = m_centroidsRadius;
    const uint16_t s1 = centerX - radius;
    const uint16_t s2 = centerX + radius;
    const uint16_t sbin = m_settings.GetBinningSerial();
    const uint16_t p1 = centerY - radius;
    const uint16_t p2 = centerY + radius;
    const uint16_t pbin = m_settings.GetBinningParallel();
    const rgn_type rgn = { s1, s2, sbin, p1, p2, pbin };

    return GenerateRoiHeader(roiIndex, rgn);
}

void pm::FakeCamera::GenerateParticles(const rgn_type& rgn)
{
    assert(rgn.sbin == 1);
    assert(rgn.pbin == 1);

    const uint16_t radius = m_centroidsRadius;
    const uint16_t rgnX = rgn.s1 / rgn.sbin;
    const uint16_t rgnY = rgn.p1 / rgn.pbin;
    const uint32_t rgnW = (rgn.s2 + 1 - rgn.s1) / rgn.sbin;
    const uint32_t rgnH = (rgn.p2 + 1 - rgn.p1) / rgn.pbin;

    m_particleCoordinates.clear();
    m_particleCoordinates.reserve(m_centroidsCount);
    for (uint16_t n = 0; n < m_centroidsCount; n++)
    {
        const uint16_t centerX = rgnX + radius
            + static_cast<uint16_t>(GetRandomNumber() % (rgnW - 2 * radius));
        const uint16_t centerY = rgnY + radius
            + static_cast<uint16_t>(GetRandomNumber() % (rgnH - 2 * radius));
        m_particleCoordinates.emplace_back(centerX, centerY);
    }

    if (m_centroidsMode == PL_CENTROIDS_MODE_TRACK)
    {
        m_particleMoments.clear();
        m_particleMoments.reserve(m_centroidsCount);
        for (uint16_t n = 0; n < m_centroidsCount; n++)
        {
            const uint32_t m0 = (GetRandomNumber() % ((1u << 22) - 1)); // Fixed-point float in format Q22.0
            const uint32_t m2 = (GetRandomNumber() % ((1u << 22) - 1)); // Fixed-point float in format Q3.19
            m_particleMoments.emplace_back(m0, m2);
        }
    }
}

void pm::FakeCamera::MoveParticles(const rgn_type& rgn)
{
    assert(rgn.sbin == 1);
    assert(rgn.pbin == 1);

    const uns16 radius = m_centroidsRadius;
    const uns16 roiLeft = rgn.s1 / rgn.sbin;
    const uns16 roiTop = rgn.p1 / rgn.pbin;
    const uns16 roiRight = rgn.s2 / rgn.sbin;
    const uns16 roiBottom = rgn.p2 / rgn.pbin;

    // Generates new random position for particle
    for (size_t i = 0; i < m_centroidsCount; i++)
    {
        const uns16 oldX = m_particleCoordinates[i].first;
        const uns16 oldY = m_particleCoordinates[i].second;
        uns16 newX = 0;
        uns16 newY = 0;

        // Generates new coordinates till they fit the sensor
        do
        {
            const uns16 step = GetRandomNumber() % (m_settings.GetTrackMaxDistance() * 3 / 4);
            const int randomAngle = GetRandomNumber() % 360;
            const double radian = randomAngle * 3.14159265358979323846 / 180.0;

            newX = static_cast<uns16>(oldX + step * cos(radian));
            newY = static_cast<uns16>(oldY + step * sin(radian));
        }
        while (newX < roiLeft + radius || newX > roiRight - radius
            || newY < roiTop + radius || newY > roiBottom - radius);

        m_particleCoordinates[i].first = newX;
        m_particleCoordinates[i].second = newY;
    }
}

bool pm::FakeCamera::GenerateFrameData()
{
    const size_t bpp = m_frameAcqCfg.GetBitmapFormat().GetBytesPerPixel();

    const auto& regions = m_settings.GetRegions();
    const rgn_type& rgn0 = regions.at(0);
    const uint32_t rgn0W = (rgn0.s2 + 1 - rgn0.s1) / rgn0.sbin;
    const uint32_t rgn0H = (rgn0.p2 + 1 - rgn0.p1) / rgn0.pbin;
    const size_t rgn0Bytes = bpp * rgn0W * rgn0H;

    const uint32_t centroidEdge = 2 * m_centroidsRadius + 1;
    const size_t centroidBytes = bpp * centroidEdge * centroidEdge;

    switch (m_settings.GetExposureResolution())
    {
    case EXP_RES_ONE_MICROSEC:
        m_expTimeResPs = 1000 * 1000;
        break;
    case EXP_RES_ONE_MILLISEC:
    default: // Just in case, should never happen
        m_expTimeResPs = 1000 * 1000 * 1000;
        break;
    case EXP_RES_ONE_SEC:
        m_expTimeResPs = (uint64_t)1000 * 1000 * 1000 * 1000;
        break;
    }

    if (m_usesCentroids)
    {
        if (rgn0.sbin != 1 || rgn0.pbin != 1)
        {
            Log::LogE("Binning not supported with centroids");
            m_error = FakeCameraErrors::InvalidRoi;
            return false;
        }

        if (regions.size() > 1)
        {
            Log::LogE("Centroids not supported with more than one user region");
            m_error = FakeCameraErrors::InvalidRoi;
            return false;
        }

        if (centroidEdge > rgn0W || centroidEdge > rgn0H)
        {
            Log::LogE("Region size (%ux%u) is smaller than centroid size (%ux%u)",
                    rgn0W, rgn0H, centroidEdge, centroidEdge);
            m_error = FakeCameraErrors::InvalidRoi;
            return false;
        }

        // Generate centroids/particles/blobs only once so they can be moved around
        GenerateParticles(rgn0);
    }

    bool caseHandled = false;
    for (uint32_t fIdx = 0; fIdx < cMaxGenFrameCount; ++fIdx, caseHandled = true)
    {
        uint8_t* dstBuffer = &m_frameGenBuffer[fIdx * m_frameAcqCfg.GetFrameBytes()];

        // 1. case: Single ROI acquisition without metadata
        if (!m_usesMetadata)
        {
            // Add ROI data
            GenerateRoiData(dstBuffer, rgn0Bytes);
            //dstBuffer += rgn0Bytes;

            continue; // Generate next frame
        }

        // Frame with metadata
        md_frame_header_v3 frmHdr = GenerateFrameHeader();

        // 2. case: Single- or Multi-ROI acquisition with metadata
        if (!m_usesCentroids)
        {
            // Add frame metadata header
            memcpy(dstBuffer, &frmHdr, sizeof(frmHdr));
            dstBuffer += sizeof(frmHdr);

            for (uint16_t rIdx = 0; rIdx < frmHdr.roiCount; ++rIdx)
            {
                const rgn_type& rgn = regions.at(rIdx);
                const uint32_t rgnW = (rgn.s2 + 1 - rgn.s1) / rgn.sbin;
                const uint32_t rgnH = (rgn.p2 + 1 - rgn.p1) / rgn.pbin;
                const size_t rgnBytes = bpp * rgnW * rgnH;

                // Add ROI metadata header
                md_frame_roi_header roiHdr = GenerateRoiHeader(rIdx, rgn);
                memcpy(dstBuffer, &roiHdr, sizeof(roiHdr));
                dstBuffer += sizeof(roiHdr);

                // Add ROI data
                GenerateRoiData(dstBuffer, rgnBytes);
                dstBuffer += rgnBytes;
            }

            continue; // Generate next frame
        }

        // Single ROI acquisition with some centroids

        // 3. case: Single ROI acquisition with Locate data
        if (m_centroidsMode == PL_CENTROIDS_MODE_LOCATE)
        {
            frmHdr.roiCount = m_centroidsCount;

            // Add frame metadata header
            memcpy(dstBuffer, &frmHdr, sizeof(frmHdr));
            dstBuffer += sizeof(frmHdr);

            // Generate data for full rgn0, inject particles and later use it for
            // copying centroid ROIs line by line to right frame location.
            // This solves an issue where closely overlapping ROIs hide particles.
            GenerateRoiData(m_frameGenRoi0Buffer.get(), rgn0Bytes);
            InjectParticles(m_frameGenRoi0Buffer.get(), rgn0);

            for (uint16_t rIdx = 0; rIdx < frmHdr.roiCount; ++rIdx)
            {
                const uint16_t centerX = m_particleCoordinates[rIdx].first;
                const uint16_t centerY = m_particleCoordinates[rIdx].second;

                // Add ROI metadata header
                md_frame_roi_header roiHdr =
                    GenerateParticleHeader(rIdx, centerX, centerY);
                memcpy(dstBuffer, &roiHdr, sizeof(roiHdr));
                dstBuffer += sizeof(roiHdr);

                // Add ROI data
                AppendParticleData(dstBuffer, roiHdr.roi, m_frameGenRoi0Buffer.get(), rgn0);
                dstBuffer += centroidBytes;
            }

            MoveParticles(rgn0);
            continue; // Generate next frame
        }

        // 4. case: Single ROI acquisition with Track data
        if (m_centroidsMode == PL_CENTROIDS_MODE_TRACK)
        {
            frmHdr.roiCount = m_centroidsCount + 1; // One more for BG image

            // Add frame metadata header
            memcpy(dstBuffer, &frmHdr, sizeof(frmHdr));
            dstBuffer += sizeof(frmHdr);

            // Add background ROI metadata header
            md_frame_roi_header roi0Hdr = GenerateRoiHeader(0, rgn0);
            roi0Hdr.extendedMdSize = m_trackRoiExtMdBytes;
            memcpy(dstBuffer, &roi0Hdr, sizeof(roi0Hdr));
            dstBuffer += sizeof(roi0Hdr);
            // Add ext. metadata behind ROI header
            const uint32_t dummyValue = 0; // Value doesn't matter
            SetExtMdData(PL_MD_EXT_TAG_PARTICLE_ID, &dstBuffer, &dummyValue);
            SetExtMdData(PL_MD_EXT_TAG_PARTICLE_M0, &dstBuffer, &dummyValue);
            SetExtMdData(PL_MD_EXT_TAG_PARTICLE_M2, &dstBuffer, &dummyValue);
            // Add background ROI data
            GenerateRoiData(dstBuffer, rgn0Bytes);
            InjectParticles(dstBuffer, rgn0);
            dstBuffer += rgn0Bytes;

            for (uint16_t rIdx = 0; rIdx < m_centroidsCount; ++rIdx)
            {
                const uint16_t centerX = m_particleCoordinates[rIdx].first;
                const uint16_t centerY = m_particleCoordinates[rIdx].second;
                // Add ROI metadata header
                md_frame_roi_header roiHdr =
                    GenerateParticleHeader(rIdx + 1, centerX, centerY);
                roiHdr.flags = PL_MD_ROI_FLAG_HEADER_ONLY; // ROI with no data
                roiHdr.extendedMdSize = m_trackRoiExtMdBytes;
                memcpy(dstBuffer, &roiHdr, sizeof(roiHdr));
                dstBuffer += sizeof(roiHdr);
                // Add ext. metadata behind ROI header
                const uint32_t id = (uint32_t)rIdx; // Particle ID is zero-based
                SetExtMdData(PL_MD_EXT_TAG_PARTICLE_ID, &dstBuffer, &id);
                const uint32_t m0 = m_particleMoments[rIdx].first;
                SetExtMdData(PL_MD_EXT_TAG_PARTICLE_M0, &dstBuffer, &m0);
                const uint32_t m2 = m_particleMoments[rIdx].second;
                SetExtMdData(PL_MD_EXT_TAG_PARTICLE_M2, &dstBuffer, &m2);
            }

            MoveParticles(rgn0);
            continue; // Generate next frame
        }

        // 5. case: Single ROI acquisition with Blob data
        if (m_centroidsMode == PL_CENTROIDS_MODE_BLOB)
        {
            frmHdr.roiCount = m_centroidsCount + 1; // One more for BG image

            // Add frame metadata header
            memcpy(dstBuffer, &frmHdr, sizeof(frmHdr));
            dstBuffer += sizeof(frmHdr);

            // Add background ROI metadata header
            md_frame_roi_header roi0Hdr = GenerateRoiHeader(0, rgn0);
            memcpy(dstBuffer, &roi0Hdr, sizeof(roi0Hdr));
            dstBuffer += sizeof(roi0Hdr);
            // Add background ROI data
            GenerateRoiData(dstBuffer, rgn0Bytes);
            InjectParticles(dstBuffer, rgn0);
            dstBuffer += rgn0Bytes;

            for (uint16_t rIdx = 0; rIdx < m_centroidsCount; ++rIdx)
            {
                const uint16_t centerX = m_particleCoordinates[rIdx].first;
                const uint16_t centerY = m_particleCoordinates[rIdx].second;
                // Add ROI metadata header
                md_frame_roi_header roiHdr =
                    GenerateParticleHeader(rIdx + 1, centerX, centerY);
                roiHdr.flags = PL_MD_ROI_FLAG_HEADER_ONLY; // ROI with no data
                // Cripple the ROI size the same way as FW does
                roiHdr.roi.s1 = centerX;
                roiHdr.roi.s2 = centerX;
                roiHdr.roi.p1 = centerY;
                roiHdr.roi.p2 = centerY;
                memcpy(dstBuffer, &roiHdr, sizeof(roiHdr));
                dstBuffer += sizeof(roiHdr);
            }

            MoveParticles(rgn0);
            continue; // Generate next frame
        }

        break; // No case handled
    }

    if (!caseHandled)
    {
        Log::LogE("No frame data generated, configuration not supported");
        m_error = FakeCameraErrors::Unknown;
    }

    return caseHandled;
}

void pm::FakeCamera::FrameGeneratorLoop()
{
    /* Run in a loop giving one frame at a time to the callback
       routine that is normally called by PVCAM. */

    const auto acqMode = m_settings.GetAcqMode();
    const auto trigMode = m_settings.GetTrigMode();
    const auto bufferFrameCount = m_settings.GetBufferFrameCount();
    const auto acqFrameCount = m_settings.GetAcqFrameCount();
    const bool isSequence = // Let AcqMode::SnapCircBuffer run until explicit stop
        acqMode == AcqMode::SnapSequence || acqMode == AcqMode::SnapTimeLapse;
    const bool isTimeLapse =
        acqMode == AcqMode::SnapTimeLapse || acqMode == AcqMode::LiveTimeLapse;
    const long long delayBetweenFramesUs =
        (isTimeLapse) ? (long long)m_settings.GetTimeLapseDelay() * 1000 : 0;

    // TODO: Add exposure time with overlap-like effect based on clearing mode
    const auto readoutTimeUs = m_readoutTimeUs;

    // Sleep will be called only for times equal to or greater than this value
    constexpr long long sleepThresholdUs = 500;

    // A time when SW trigger arrived for first frame, the other are timed internally
    auto swTrigFirstFrameTimeUs = 0.0;

    while (!m_frameGenStopFlag)
    {
        double nowUs = m_startStopTimer.Microseconds();

        // Set frame info values
        {
            std::unique_lock<std::mutex> lock(m_frameGenMutex);

            long long sleepTimeUs;

            // Wait for SW trigger first
            const auto isSwTrigFirst =
                trigMode == EXT_TRIG_SOFTWARE_FIRST && m_frameGenFrameIndex == 0;
            if (trigMode == EXT_TRIG_SOFTWARE_EDGE || isSwTrigFirst)
            {
                m_frameGenCond.wait(lock,
                        [this]() { return m_frameGenStopFlag || m_frameGenSwTriggerFlag; });
                if (m_frameGenStopFlag)
                    break;

                const auto oldNowUs = nowUs;
                nowUs = m_startStopTimer.Microseconds();
                if (isSwTrigFirst)
                    swTrigFirstFrameTimeUs = nowUs - oldNowUs;

                sleepTimeUs = (long long)readoutTimeUs;
            }
            else
            {
                // Absolute delay (for high precision) from acq. start to next frame
                const auto totalDelayUs = swTrigFirstFrameTimeUs
                    + (readoutTimeUs + delayBetweenFramesUs) * m_frameGenFrameIndex;
                // A time elapsed for current frame
                const auto delayUs = nowUs - totalDelayUs;
                // A time to sleep before next frame is given
                sleepTimeUs = (long long)(readoutTimeUs - delayUs);
            }

            // Wait proper time matching specified frame rate
            if (sleepTimeUs > sleepThresholdUs)
            {
                m_frameGenCond.wait_for(lock, std::chrono::microseconds(sleepTimeUs),
                        [this]() { return !!m_frameGenStopFlag; });
                if (m_frameGenStopFlag)
                    break;
            }

            // Reset this flag after sleep so another trigger cannot be queued during "readout"
            m_frameGenSwTriggerFlag = false;

            // Prepare frame info
            m_frameGenFrameInfo.FrameNr =
                (int32_t)(m_frameGenFrameIndex & std::numeric_limits<int32_t>::max()) + 1;
            m_frameGenFrameInfo.hCam = m_hCam;
            m_frameGenFrameInfo.TimeStampBOF = (uint32_t)(nowUs / 100);
            m_frameGenFrameInfo.TimeStamp =
                m_frameGenFrameInfo.TimeStampBOF + (uint32_t)(readoutTimeUs / 100);
            m_frameGenFrameInfo.ReadoutTime = (int32_t)
                (m_frameGenFrameInfo.TimeStamp - m_frameGenFrameInfo.TimeStampBOF);
        }

        // Set frame data
        m_frameGenBufferPos = m_frameGenFrameIndex % bufferFrameCount;
        const size_t frameBytes = m_frameAcqCfg.GetFrameBytes();
        void* dst = &m_buffer[m_frameGenBufferPos * frameBytes];
        const void* src = &m_frameGenBuffer[
            (m_frameGenFrameIndex % cMaxGenFrameCount) * frameBytes];
        // TODO: Change to parallelized copy via TaskSet_CopyMemory.
        //       When tried the app crashed, probably some sync. issue...
        memcpy(dst, src, frameBytes);

        if (m_usesMetadata)
        {
            const auto nowPs = nowUs * 1000 * 1000;
            const auto readoutTimePs = readoutTimeUs * 1000 * 1000;

            // Update frame header in metadata
            auto frmHdr = static_cast<md_frame_header_v3*>(dst);
            frmHdr->frameNr =
                (int32_t)(m_frameGenFrameIndex & std::numeric_limits<int32_t>::max()) + 1;
            frmHdr->timestampBOF = (uint64_t)nowPs;
            frmHdr->timestampEOF = frmHdr->timestampBOF + (uint64_t)readoutTimePs;

            frmHdr->exposureTime = m_expTimeResPs * GetFrameExpTime(frmHdr->frameNr);

            // TODO: Update ROI headers in metadata if any
            //roiHdr->timestampBOR = (readoutTimePs / m_centroidsCount) * roiIndex;
            //roiHdr->timestampEOR = roiHdr->timestampBOR + readoutTimePs / m_centroidsCount;
        }

        // Invoke the registered callback function
        m_eofCallbackHandler(&m_frameGenFrameInfo, m_eofCallbackContext);

        // Stop sequence acquisition
        if (isSequence && m_frameGenFrameIndex + 1 >= acqFrameCount)
            break;

        // Update counters for frame
        m_frameGenFrameIndex++;
        // Updated in next iteration before first use, see "Set frame data" above
        //m_frameGenBufferPos = m_frameGenFrameIndex % bufferFrameCount;
    }
}
