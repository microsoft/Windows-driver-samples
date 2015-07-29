/*++
    Copyright (c) Microsoft Corporation. All Rights Reserved. 
    Sample code. Dealpoint ID #843729.

    Module Name: 

        controller.h

    Abstract:

        This module contains touch driver public definitions.

    Environment:

        Kernel mode

    Revision History:

--*/

#pragma once

#include <wdm.h>
#include <wdf.h>
#include <hidport.h>
#define RESHUB_USE_HELPER_ROUTINES
#include <reshub.h>
#include <kbdmou.h>
#include "trace.h"
#include "spb.h"

#define TOUCH_POOL_TAG                  (ULONG)'cuoT'

//
// Device descriptions
//

#define OEM_MAX_TOUCHES    10

//
// Constants
//
#define MODE_MOUSE                      0x00
#define MODE_SINGLE_TOUCH               0x01
#define MODE_MULTI_TOUCH                0x02

#define MAX_MOUSE_COORD                 0x7FFF
#define MAX_TOUCH_COORD                 0x0FFF

#define MOUSE_LEFT_BUTTON_DOWN          0x0001
#define MOUSE_LEFT_BUTTON_UP            0x0002
#define MOUSE_MOVE_TOUCH_ABSOLUTE       0x8000

#define FINGER_STATUS                   0x01 // finger down
#define RANGE_STATUS                    0x02 // in range
#define RANGE_FINGER_STATUS             0x03 // finger down (range + finger)

#define KEY_DOWN_START                  (1 << 0)
#define KEY_DOWN_SEARCH                 (1 << 1)
#define KEY_DOWN_BACK                   (1 << 2)

#define REPORTID_MTOUCH                 1
#define REPORTID_MOUSE                  3
#define REPORTID_FEATURE                7
#define REPORTID_MAX_COUNT              8
#define REPORTID_CAPKEY                 9

// 
// Type defintions
//

#pragma warning(push)
#pragma warning(disable:4201)  // (nameless struct/union)
#include <pshpack1.h>

typedef struct _HID_TOUCH_REPORT
{
    union
    {
        struct
        {
            UCHAR  bStatus;
            UCHAR  ContactId;
            USHORT wXData;
            USHORT wYData;            
            UCHAR  bStatus2;
            UCHAR  ContactId2;
            USHORT wXData2;
            USHORT wYData2;
            UCHAR  ActualCount;
        } InputReport;
        UCHAR RawInput[13];
    };
} HID_TOUCH_REPORT, *PHID_TOUCH_REPORT;

typedef struct _HID_MOUSE_REPORT {
    union
    {
        struct
        {
            UCHAR  bButtons;
            USHORT wXData;
            USHORT wYData;
            USHORT wReserved;
        }InputReport;
        UCHAR RawInput[7];
    };
} HID_MOUSE_REPORT, *PHID_MOUSE_REPORT;

typedef struct _HID_KEY_REPORT {
    union
    {
        struct
        {
            UCHAR  bKeys;
            UCHAR  bReserved;
            USHORT wReserved;
        }InputReport;
        UCHAR RawInput[4];
    };
} HID_KEY_REPORT, *PHID_KEY_REPORT;


typedef struct _HID_FEATURE_REPORT
{
    UCHAR ReportID;
    UCHAR InputMode;
    UCHAR DeviceIndex;
} HID_FEATURE_REPORT, *PHID_FEATURE_REPORT;

typedef struct _HID_MAX_COUNT_REPORT
{
    UCHAR ReportID;
    UCHAR MaxCount;
}HID_MAX_COUNT_REPORT, *PHID_MAX_COUNT_REPORT;

typedef struct _HID_INPUT_REPORT
{
    UCHAR ReportID;
    union
    {
        HID_TOUCH_REPORT TouchReport;
        HID_MOUSE_REPORT MouseReport;
        HID_KEY_REPORT   KeyReport;
    };
#ifdef _TIMESTAMP_
    LARGE_INTEGER TimeStamp;
#endif
} HID_INPUT_REPORT, *PHID_INPUT_REPORT;

#include <poppack.h>
#pragma warning(pop)

NTSTATUS 
TchAllocateContext(
    OUT VOID **ControllerContext,
    IN WDFDEVICE FxDevice
    );

NTSTATUS 
TchFreeContext(
    IN VOID *ControllerContext
    );

NTSTATUS 
TchStartDevice(
    IN VOID *ControllerContext,
    IN SPB_CONTEXT *SpbContext
    );

NTSTATUS 
TchStopDevice(
    IN VOID *ControllerContext,
    IN SPB_CONTEXT *SpbContext
    );

NTSTATUS 
TchStandbyDevice(
    IN VOID *ControllerContext,
    IN SPB_CONTEXT *SpbContext
    );

NTSTATUS 
TchWakeDevice(
    IN VOID *ControllerContext,
    IN SPB_CONTEXT *SpbContext
    );

NTSTATUS
TchRegistryGetControllerSettings(
    IN VOID *ControllerContext,
    IN WDFDEVICE FxDevice
    );
   
NTSTATUS
TchServiceInterrupts(
    IN VOID *ControllerContext,
    IN SPB_CONTEXT *SpbContext,
    IN PHID_INPUT_REPORT HidReport,
    IN UCHAR InputMode,
    OUT BOOLEAN *ServicingComplete
    );

