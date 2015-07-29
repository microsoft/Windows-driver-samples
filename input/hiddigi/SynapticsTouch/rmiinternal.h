/*++
    Copyright (c) Microsoft Corporation. All Rights Reserved. 
    Sample code. Dealpoint ID #843729.

    Module Name:

        rmiinternal.h

    Abstract:

        Contains common types and defintions used internally
        by the multi touch screen driver.

    Environment:

        Kernel mode

    Revision History:

--*/

#pragma once

#include <wdm.h>
#include <wdf.h>
#include "controller.h"
#include "resolutions.h"


// Ignore warning C4152: nonstandard extension, function/data pointer conversion in expression
#pragma warning (disable : 4152)

// Ignore warning C4201: nonstandard extension used : nameless struct/union
#pragma warning (disable : 4201)

// Ignore warning C4201: nonstandard extension used : bit field types other than in
#pragma warning (disable : 4214)

// Ignore warning C4324: 'xxx' : structure was padded due to __declspec(align())
#pragma warning (disable : 4324)


//
// Defines from Synaptics RMI4 Data Sheet, please refer to
// the spec for details about the fields and values.
//

#define RMI4_FIRST_FUNCTION_ADDRESS       0xE9
#define RMI4_PAGE_SELECT_ADDRESS          0xFF

#define RMI4_F01_RMI_DEVICE_CONTROL       0x01
#define RMI4_F11_2D_TOUCHPAD_SENSOR       0x11
#define RMI4_F1A_0D_CAP_BUTTON_SENSOR     0x1A
#define RMI4_F34_FLASH_MEMORY_MANAGEMENT  0x34
#define RMI4_F54_TEST_REPORTING           0x54

#define RMI4_MAX_FUNCTIONS                10
#define RMI4_MAX_TOUCHES                  10

typedef struct _RMI4_FUNCTION_DESCRIPTOR
{
    BYTE QueryBase;
    BYTE CommandBase;
    BYTE ControlBase;
    BYTE DataBase;
    union 
    {
        BYTE All;
        struct
        {
            BYTE IrqCount  : 3;
            BYTE Reserved0 : 2;
            BYTE FuncVer   : 2;
            BYTE Reserved1 : 1;
        };
    } VersionIrq;
    BYTE Number;
} RMI4_FUNCTION_DESCRIPTOR;

//
// Function $01 - RMI Device Control
//

typedef struct _RMI4_F01_QUERY_REGISTERS
{
    BYTE ManufacturerID;
    union
    {
        BYTE All;
        struct
        {
            BYTE CustomMap      : 1;
            BYTE NonCompliant   : 1;
            BYTE Reserved0      : 1;
            BYTE HasSensorID    : 1;
            BYTE Reserved1      : 1;
            BYTE HasAdjDoze     : 1;
            BYTE HasAdJDozeHold : 1;
            BYTE Reserved2      : 1;
        };
    } ProductProperties;
    BYTE ProductInfo0;
    BYTE ProductInfo1;
    BYTE Date0;
    BYTE Date1;
    BYTE WaferLotId0Lo;
    BYTE WaferLotId0Hi;
    BYTE WaferLotId1Lo;
    BYTE WaferLotId1Hi;
    BYTE WaferLotId2Lo;
    BYTE ProductID1;
    BYTE ProductID2;
    BYTE ProductID3;
    BYTE ProductID4;
    BYTE ProductID5;
    BYTE ProductID6;
    BYTE ProductID7;
    BYTE ProductID8;
    BYTE ProductID9;
    BYTE ProductID10;
    BYTE Reserved21;
    BYTE SendorID;
    BYTE Reserved23;
    BYTE Reserved24;
    BYTE Reserved25;
    BYTE Reserved26;
    BYTE Reserved27;
    BYTE Reserved28;
    BYTE Reserved29;
    BYTE Reserved30;
    BYTE Reserved31;
} RMI4_F01_QUERY_REGISTERS;

typedef struct _RMI4_F01_CTRL_REGISTERS
{
    union
    {
        BYTE All;
        struct
        {
            BYTE SleepMode   :2;
            BYTE NoSleep     :1;
            BYTE Reserved0   :3;
            BYTE ReportRate  :1;
            BYTE Configured  :1;
        };
    } DeviceControl;
    BYTE InterruptEnable;
    BYTE DozeInterval;
    BYTE DozeThreshold;
    BYTE DozeHoldoff;
} RMI4_F01_CTRL_REGISTERS;

#define RMI4_F11_DEVICE_CONTROL_SLEEP_MODE_OPERATING  0
#define RMI4_F11_DEVICE_CONTROL_SLEEP_MODE_SLEEPING   1

//
// Logical structure for getting registry config settings
//
typedef struct _RM4_F01_CTRL_REGISTERS_LOGICAL
{
    UINT32 SleepMode;
    UINT32 NoSleep;
    UINT32 ReportRate;
    UINT32 Configured;
    UINT32 InterruptEnable;
    UINT32 DozeInterval;
    UINT32 DozeThreshold;
    UINT32 DozeHoldoff;
} RMI4_F01_CTRL_REGISTERS_LOGICAL;

#define RMI4_MILLISECONDS_TO_TENTH_MILLISECONDS(n) n/10
#define RMI4_SECONDS_TO_HALF_SECONDS(n) 2*n

typedef struct _RMI4_F01_DATA_REGISTERS
{
    union {
        BYTE All;
        struct
        {
            BYTE Status       : 4;
            BYTE Reserved0    : 2;
            BYTE FlashProg    : 1;
            BYTE Unconfigured : 1;
        };
    } DeviceStatus;
    BYTE InterruptStatus[1];
} RMI4_F01_DATA_REGISTERS;

#define RMI4_INTERRUPT_BIT_2D_TOUCH               0x04
#define RMI4_INTERRUPT_BIT_0D_CAP_BUTTON          0x20

#define RMI4_F01_DATA_STATUS_NO_ERROR             0
#define RMI4_F01_DATA_STATUS_RESET_OCCURRED       1
#define RMI4_F01_DATA_STATUS_INVALID_CONFIG       2
#define RMI4_F01_DATA_STATUS_DEVICE_FAILURE       3
#define RMI4_F01_DATA_STATUS_CONFIG_CRC_FAILURE   4
#define RMI4_F01_DATA_STATUS_FW_CRC_FAILURE       5
#define RMI4_F01_DATA_STATUS_CRC_IN_PROGRESS      6

typedef struct _RMI4_F01_COMMAND_REGISTERS
{
    BYTE Reset;
} RMI4_F01_COMMAND_REGISTERS;

//
// Function $11 - 2-D Touch Sensor
//

typedef struct _RMI4_F11_QUERY_REGISTERS
{
    struct
    {
        BYTE NumberOfSensors : 3;
        BYTE Reserved0       : 1;
        BYTE HasQuery11      : 1;
        BYTE Reserved1       : 3;
    };
    struct
    {
        BYTE NumberOfFingers : 3;
        BYTE HasRelative     : 1;
        BYTE HasAbsolute     : 1;
        BYTE HasGestures     : 1;
        BYTE HasSensitivity  : 1;
        BYTE Configurable    : 1;
    };
    BYTE NumXElectrodes;
    BYTE NumYElectrodes;
    BYTE MaxElectrodes;
    struct
    {
        BYTE AbsDataSize     : 2;
        BYTE HasAnchoredFin  : 1;
        BYTE HasAdjHyst      : 1;
        BYTE HasDribble      : 1;
        BYTE Reserved2       : 3;
    };
    struct
    {
        BYTE HasZTuning      : 1;
        BYTE HasAlgoSelect   : 1;
        BYTE HasWTuning      : 1;
        BYTE HasPitchInfo    : 1;
        BYTE HasFingerSize   : 1;
        BYTE HasObjSensAdj   : 1;
        BYTE HasXYClip       : 1;
        BYTE HasDrummingAdj  : 1;
    };
} RMI4_F11_QUERY_REGISTERS;

typedef struct _RMI4_F11_CTRL_REGISTERS
{
    struct
    {
        BYTE ReportingMode   : 3;
        BYTE AbsPosFilt      : 1;
        BYTE RelPosFilt      : 1;
        BYTE RelBallistics   : 1;
        BYTE Dribble         : 1;
        BYTE Reserved0       : 1;
    };
    struct
    {
        BYTE PalmDetectThreshold : 4;
        BYTE MotionSensitivity   : 2;
        BYTE ManTrackEn          : 1;
        BYTE ManTrackedFinger    : 1;
    };
    BYTE DeltaXPosThreshold;
    BYTE DeltaYPosThreshold;
    BYTE Velocity;
    BYTE Acceleration;
    BYTE SensorMaxXPosLo;
    struct 
    {
        BYTE SensorMaxXPosHi : 4;
        BYTE Reserved1       : 4;
    };
    BYTE SensorMaxYPosLo;
    struct 
    {
        BYTE SensorMaxYPosHi : 4;
        BYTE Reserved2       : 4;
    };
    //
    // Note gap in registers on 3200
    //
    BYTE ZTouchThreshold;
    BYTE ZHysteresis;
    BYTE SmallZThreshold;
    BYTE SmallZScaleFactor[2];
    BYTE LargeZScaleFactor[2];
    BYTE AlgorithmSelection;
    BYTE WxScaleFactor;
    BYTE WxOffset;
    BYTE WyScaleFactor;
    BYTE WyOffset;    
    BYTE XPitch[2];
    BYTE YPitch[2];
    BYTE FingerWidthX[2];
    BYTE FingerWidthY[2];
    BYTE ReportMeasuredSize;
    BYTE SegmentationSensitivity;
    BYTE XClipLo;
    BYTE XClipHi;
    BYTE YClipLo;
    BYTE YClipHi;
    BYTE MinFingerSeparation;
    BYTE MaxFingerMovement;
} RMI4_F11_CTRL_REGISTERS;

//
// Logical structure for getting registry config settings
//
typedef struct _RMI4_F11_CTRL_REGISTERS_LOGICAL
{
    UINT32 ReportingMode;
    UINT32 AbsPosFilt;  
    UINT32 RelPosFilt;
    UINT32 RelBallistics;
    UINT32 Dribble;
    UINT32 PalmDetectThreshold;
    UINT32 MotionSensitivity;  
    UINT32 ManTrackEn;  
    UINT32 ManTrackedFinger;
    UINT32 DeltaXPosThreshold;
    UINT32 DeltaYPosThreshold;
    UINT32 Velocity;
    UINT32 Acceleration;
    UINT32 SensorMaxXPos;
    UINT32 SensorMaxYPos;
    UINT32 ZTouchThreshold;
    UINT32 ZHysteresis;
    UINT32 SmallZThreshold;
    UINT32 SmallZScaleFactor;
    UINT32 LargeZScaleFactor;
    UINT32 AlgorithmSelection;
    UINT32 WxScaleFactor;
    UINT32 WxOffset;
    UINT32 WyScaleFactor;
    UINT32 WyOffset;    
    UINT32 XPitch;
    UINT32 YPitch;
    UINT32 FingerWidthX;
    UINT32 FingerWidthY;
    UINT32 ReportMeasuredSize;
    UINT32 SegmentationSensitivity;
    UINT32 XClipLo;
    UINT32 XClipHi;
    UINT32 YClipLo;
    UINT32 YClipHi;
    UINT32 MinFingerSeparation;
    UINT32 MaxFingerMovement;
} RMI4_F11_CTRL_REGISTERS_LOGICAL;

typedef struct _RMI4_F11_DATA_POSITION
{
    BYTE XPosHi;
    BYTE YPosHi;
    struct
    {
        BYTE XPosLo : 4;
        BYTE YPosLo : 4;
    };
    struct
    {
        BYTE XWidth : 4;
        BYTE YWidth : 4;
    };
    BYTE ZAmplitude;
} RMI4_F11_DATA_POSITION;

typedef struct _RMI4_F11_DATA_REGISTERS_STATUS_BLOCK
{
    struct 
    {
        BYTE FingerState0 : 2;
        BYTE FingerState1 : 2;
        BYTE FingerState2 : 2;
        BYTE FingerState3 : 2;
    };
    struct 
    {
        BYTE FingerState4 : 2;
        BYTE FingerState5 : 2;
        BYTE FingerState6 : 2;
        BYTE FingerState7 : 2;
    };
    struct 
    {
        BYTE FingerState8 : 2;
        BYTE FingerState9 : 2;
        BYTE Reserved0    : 4;
    };
} RMI4_F11_DATA_REGISTERS_STATUS_BLOCK;

typedef struct _RMI4_F11_DATA_REGISTERS
{
    RMI4_F11_DATA_REGISTERS_STATUS_BLOCK Status;
    RMI4_F11_DATA_POSITION Finger[RMI4_MAX_TOUCHES];
} RMI4_F11_DATA_REGISTERS;

#define RMI4_FINGER_STATE_NOT_PRESENT                  0
#define RMI4_FINGER_STATE_PRESENT_WITH_ACCURATE_POS    1
#define RMI4_FINGER_STATE_PRESENT_WITH_INACCURATE_POS  2
#define RMI4_FINGER_STATE_RESERVED                     3

//
// Function $1A - 0-D Capacitive Button Sensors
//

typedef struct _RMI4_F1A_QUERY_REGISTERS
{
    struct
    {
        BYTE MaxButtonCount  : 2;
        BYTE Reserved0       : 5;
    };
    struct
    {
        BYTE HasGenControl   : 1;
        BYTE HasIntEnable    : 1;
        BYTE HasMultiButSel  : 1;
        BYTE HasTxRxMapping  : 1;
        BYTE HasPerButThresh : 1;
        BYTE HasRelThresh    : 1;
        BYTE HasStrongButHyst: 1;
        BYTE HasFiltStrength : 1;
    };
} RMI4_F1A_QUERY_REGISTERS;

typedef struct _RMI4_F1A_CTRL_REGISTERS
{
    struct
    {
        BYTE MultiButtonRpt  : 2;
        BYTE FilterMode      : 2;
        BYTE Reserved0       : 4;
    };
    struct
    {
        BYTE IntEnBtn0       : 1;
        BYTE IntEnBtn1       : 1;
        BYTE IntEnBtn2       : 1;
        BYTE IntEnBtn3       : 1;
        BYTE Reserved1       : 4;
    };
    struct
    {
        BYTE MultiBtn0       : 1;
        BYTE MultiBtn1       : 1;
        BYTE MultiBtn2       : 1;
        BYTE MultiBtn3       : 1;
        BYTE Reserved2       : 4;
    };
    BYTE PhysicalTx0;
    BYTE PhysicalRx0;
    BYTE PhysicalTx1;
    BYTE PhysicalRx1;
    BYTE PhysicalTx2;
    BYTE PhysicalRx2;
    BYTE PhysicalTx3;
    BYTE PhysicalRx3;
    BYTE Threshold0;
    BYTE Threshold1;
    BYTE Threshold2;
    BYTE Threshold3;
    BYTE ReleaseThreshold;
    BYTE StrongButtonHyst;
    BYTE FilterStrength;
} RMI4_F1A_CTRL_REGISTERS;

typedef struct _RMI4_F1A_DATA_REGISTERS
{
    struct
    {
        BYTE Button0         : 1;
        BYTE Button1         : 1;
        BYTE Button2         : 1;
        BYTE Button3         : 1;
        BYTE Reserved0       : 4;
    };
} RMI4_F1A_DATA_REGISTERS;

//
// Function $XX - Add new support functions here
//


//
// Driver structures
//

typedef struct _RMI4_CONFIGURATION
{
    RMI4_F01_CTRL_REGISTERS_LOGICAL DeviceSettings;
    RMI4_F11_CTRL_REGISTERS_LOGICAL TouchSettings;
    UINT32 PepRemovesVoltageInD3;
} RMI4_CONFIGURATION;

typedef struct _RMI4_FINGER_INFO
{
    int x;
    int y;
    UCHAR fingerStatus;
} RMI4_FINGER_INFO;

typedef struct _RMI4_FINGER_CACHE
{
    RMI4_FINGER_INFO FingerSlot[RMI4_MAX_TOUCHES];
    UINT32 FingerSlotValid;
    UINT32 FingerSlotDirty;
    int FingerDownOrder[RMI4_MAX_TOUCHES];
    int FingerDownCount;
} RMI4_FINGER_CACHE;

typedef struct _RMI4_CONTROLLER_CONTEXT
{
    WDFDEVICE FxDevice;    
    WDFWAITLOCK ControllerLock;

    //
    // Controller state
    //
    int FunctionCount;
    RMI4_FUNCTION_DESCRIPTOR Descriptors[RMI4_MAX_FUNCTIONS];
    int FunctionOnPage[RMI4_MAX_FUNCTIONS];
    int CurrentPage;

    ULONG InterruptStatus;
    BOOLEAN HasButtons;
    BOOLEAN ResetOccurred;
    BOOLEAN InvalidConfiguration;
    BOOLEAN DeviceFailure;
    BOOLEAN UnknownStatus;
    BYTE UnknownStatusMessage;
    RMI4_F01_QUERY_REGISTERS F01QueryRegisters;

    //
    // Power state
    //
    DEVICE_POWER_STATE DevicePowerState;

    //
    // Register configuration programmed to chip
    //
    TOUCH_SCREEN_PROPERTIES Props;
    RMI4_CONFIGURATION Config;

    //
    // Current touch state
    //
    int TouchesReported;
    int TouchesTotal;
    RMI4_FINGER_CACHE Cache;

} RMI4_CONTROLLER_CONTEXT;

NTSTATUS
RmiCheckInterrupts(
    IN RMI4_CONTROLLER_CONTEXT *ControllerContext,
    IN SPB_CONTEXT *SpbContext,
    IN ULONG* InterruptStatus
    );

int
RmiGetFunctionIndex(
    IN RMI4_FUNCTION_DESCRIPTOR* FunctionDescriptors,
    IN int FunctionCount,
    IN int FunctionDesired
    );

NTSTATUS
RmiChangePage(
    IN RMI4_CONTROLLER_CONTEXT* ControllerContext,
    IN SPB_CONTEXT* SpbContext,
    IN int DesiredPage
    );
