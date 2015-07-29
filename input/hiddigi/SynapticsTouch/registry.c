/*++
    Copyright (c) Microsoft Corporation. All Rights Reserved. 
    Sample code. Dealpoint ID #843729.

    Module Name: 

        registry.c

    Abstract:

        This module retrieves platform-specific controller
        configuration from the registry, or assigns default
        values if no registry configuration is present.

    Environment:

        Kernel mode

    Revision History:

--*/

#include "rmiinternal.h"
#include "registry.tmh"

//
// Default RMI4 configuration values can be changed here. Please refer to the
// RMI4 specification for a full description of the fields and value meanings
//

static RMI4_CONFIGURATION gDefaultConfiguration =
{
    //
    // RMI4 F01 - Device control settings
    //
    {
        0,                                              // Sleep Mode (normal)
        1,                                              // No Sleep (do sleep)
        0,                                              // Report Rate (standard)
        1,                                              // Configured
        0xf,                                            // Interrupt Enable
        RMI4_MILLISECONDS_TO_TENTH_MILLISECONDS(20),    // Doze Interval
        10,                                             // Doze Threshold
        RMI4_SECONDS_TO_HALF_SECONDS(2)                 // Doze Holdoff
    },

    //
    // RMI4 F11 - 2D Touchpad sensor settings
    //
    {
        1,                                              // Reporting mode (throttle)
        1,                                              // Abs position filter
        0,                                              // Rel position filter
        0,                                              // Rel ballistics
        0,                                              // Dribble
        0xb,                                            // PalmDetectThreshold
        3,                                              // MotionSensitivity
        0,                                              // ManTrackEn
        0,                                              // ManTrackedFinger
        0,                                              // DeltaXPosThreshold
        0,                                              // DeltaYPosThreshold
        0,                                              // Velocity
        0,                                              // Acceleration
        TOUCH_DEVICE_RESOLUTION_X,                      // Sensor Max X Position
        TOUCH_DEVICE_RESOLUTION_Y,                      // Sensor Max Y Position
        0x1e,                                           // ZTouchThreshold
        0x05,                                           // ZHysteresis
        0x28,                                           // SmallZThreshold
        0x28f5,                                         // SmallZScaleFactor
        0x051e,                                         // LargeZScaleFactor
        0x1,                                            // AlgorithmSelection
        0x30,                                           // WxScaleFactor
        0x0,                                            // WxOffset
        0x30,                                           // WyScaleFactor
        0x0,                                            // WyOffset
        0x4800,                                         // XPitch
        0x4800,                                         // YPitch
        0xea4f,                                         // FingerWidthX
        0xdf6c,                                         // FingerWidthY
        0,                                              // ReportMeasuredSize
        0x70,                                           // SegmentationSensitivity
        0x0,                                            // XClipLo
        0x0,                                            // XClipHi
        0x0,                                            // YClipLo
        0x0,                                            // YClipHi
        0x0a,                                           // MinFingerSeparation
        0x04                                            // MaxFingerMovement
    },

    //
    // Internal driver settings
    //
    {
        0x0,                                            // Controller stays powered in D3
    },
};

RTL_QUERY_REGISTRY_TABLE gRegistryTable[] =
{
    //
    // RMI4 F01 - Device control settings
    //
    {
        NULL, RTL_QUERY_REGISTRY_DIRECT,
        L"SleepMode",
        (PVOID) (FIELD_OFFSET(RMI4_CONFIGURATION, DeviceSettings) + 
            FIELD_OFFSET(RMI4_F01_CTRL_REGISTERS_LOGICAL, SleepMode)),
        REG_DWORD,
        &gDefaultConfiguration.DeviceSettings.SleepMode,
        sizeof(UINT32)
    },
    {
        NULL, RTL_QUERY_REGISTRY_DIRECT,
        L"NoSleep",
        (PVOID) (FIELD_OFFSET(RMI4_CONFIGURATION, DeviceSettings) + 
            FIELD_OFFSET(RMI4_F01_CTRL_REGISTERS_LOGICAL, NoSleep)),
        REG_DWORD,
        &gDefaultConfiguration.DeviceSettings.NoSleep,
        sizeof(UINT32)
    },
    {
        NULL, RTL_QUERY_REGISTRY_DIRECT,
        L"ReportRate",
        (PVOID) (FIELD_OFFSET(RMI4_CONFIGURATION, DeviceSettings) + 
            FIELD_OFFSET(RMI4_F01_CTRL_REGISTERS_LOGICAL, ReportRate)),
        REG_DWORD,
        &gDefaultConfiguration.DeviceSettings.ReportRate,
        sizeof(UINT32)
    },
    {
        NULL, RTL_QUERY_REGISTRY_DIRECT,
        L"Configured",
        (PVOID) (FIELD_OFFSET(RMI4_CONFIGURATION, DeviceSettings) + 
            FIELD_OFFSET(RMI4_F01_CTRL_REGISTERS_LOGICAL, Configured)),
        REG_DWORD,
        &gDefaultConfiguration.DeviceSettings.Configured,
        sizeof(UINT32)
    },
    {
        NULL, RTL_QUERY_REGISTRY_DIRECT,
        L"InterruptEnable",
        (PVOID) (FIELD_OFFSET(RMI4_CONFIGURATION, DeviceSettings) + 
            FIELD_OFFSET(RMI4_F01_CTRL_REGISTERS_LOGICAL, InterruptEnable)),
        REG_DWORD,
        &gDefaultConfiguration.DeviceSettings.InterruptEnable,
        sizeof(UINT32)
    },
    {
        NULL, RTL_QUERY_REGISTRY_DIRECT,
        L"DozeInterval",
        (PVOID) (FIELD_OFFSET(RMI4_CONFIGURATION, DeviceSettings) + 
            FIELD_OFFSET(RMI4_F01_CTRL_REGISTERS_LOGICAL, DozeInterval)),
        REG_DWORD,
        &gDefaultConfiguration.DeviceSettings.DozeInterval,
        sizeof(UINT32)
    },
    {
        NULL, RTL_QUERY_REGISTRY_DIRECT,
        L"DozeThreshold",
        (PVOID) (FIELD_OFFSET(RMI4_CONFIGURATION, DeviceSettings) + 
            FIELD_OFFSET(RMI4_F01_CTRL_REGISTERS_LOGICAL, DozeThreshold)),
        REG_DWORD,
        &gDefaultConfiguration.DeviceSettings.DozeThreshold,
        sizeof(UINT32)
    },
    {
        NULL, RTL_QUERY_REGISTRY_DIRECT,
        L"DozeHoldoff",
        (PVOID) (FIELD_OFFSET(RMI4_CONFIGURATION, DeviceSettings) + 
            FIELD_OFFSET(RMI4_F01_CTRL_REGISTERS_LOGICAL, DozeHoldoff)),
        REG_DWORD,
        &gDefaultConfiguration.DeviceSettings.DozeHoldoff,
        sizeof(UINT32)
    },

    //
    // RMI4 F11 - 2D Touchpad sensor settings
    //
    {
        NULL, RTL_QUERY_REGISTRY_DIRECT,
        L"ReportingMode",
        (PVOID) (FIELD_OFFSET(RMI4_CONFIGURATION, TouchSettings) + 
            FIELD_OFFSET(RMI4_F11_CTRL_REGISTERS_LOGICAL, ReportingMode)),
        REG_DWORD,
        &gDefaultConfiguration.TouchSettings.ReportingMode,
        sizeof(UINT32)
    },
    {
        NULL, RTL_QUERY_REGISTRY_DIRECT,
        L"AbsPosFilt",
        (PVOID) (FIELD_OFFSET(RMI4_CONFIGURATION, TouchSettings) + 
            FIELD_OFFSET(RMI4_F11_CTRL_REGISTERS_LOGICAL, AbsPosFilt)),
        REG_DWORD,
        &gDefaultConfiguration.TouchSettings.AbsPosFilt,
        sizeof(UINT32)
    },
    {
        NULL, RTL_QUERY_REGISTRY_DIRECT,
        L"RelPosFilt",
        (PVOID) (FIELD_OFFSET(RMI4_CONFIGURATION, TouchSettings) + 
            FIELD_OFFSET(RMI4_F11_CTRL_REGISTERS_LOGICAL, RelPosFilt)),
        REG_DWORD,
        &gDefaultConfiguration.TouchSettings.RelPosFilt,
        sizeof(UINT32)
    },
    {
        NULL, RTL_QUERY_REGISTRY_DIRECT,
        L"RelBallistics",
        (PVOID) (FIELD_OFFSET(RMI4_CONFIGURATION, TouchSettings) + 
            FIELD_OFFSET(RMI4_F11_CTRL_REGISTERS_LOGICAL, RelBallistics)),
        REG_DWORD,
        &gDefaultConfiguration.TouchSettings.RelBallistics,
        sizeof(UINT32)
    },
    {
        NULL, RTL_QUERY_REGISTRY_DIRECT,
        L"Dribble",
        (PVOID) (FIELD_OFFSET(RMI4_CONFIGURATION, TouchSettings) + 
            FIELD_OFFSET(RMI4_F11_CTRL_REGISTERS_LOGICAL, Dribble)),
        REG_DWORD,
        &gDefaultConfiguration.TouchSettings.Dribble,
        sizeof(UINT32)
    },
    {
        NULL, RTL_QUERY_REGISTRY_DIRECT,
        L"PalmDetectThreshold",
        (PVOID) (FIELD_OFFSET(RMI4_CONFIGURATION, TouchSettings) + 
            FIELD_OFFSET(RMI4_F11_CTRL_REGISTERS_LOGICAL, PalmDetectThreshold)),
        REG_DWORD,
        &gDefaultConfiguration.TouchSettings.PalmDetectThreshold,
        sizeof(UINT32)
    },
    {
        NULL, RTL_QUERY_REGISTRY_DIRECT,
        L"MotionSensitivity",
        (PVOID) (FIELD_OFFSET(RMI4_CONFIGURATION, TouchSettings) + 
            FIELD_OFFSET(RMI4_F11_CTRL_REGISTERS_LOGICAL, MotionSensitivity)),
        REG_DWORD,
        &gDefaultConfiguration.TouchSettings.MotionSensitivity,
        sizeof(UINT32)
    },
    {
        NULL, RTL_QUERY_REGISTRY_DIRECT,
        L"ManTrackEn",
        (PVOID) (FIELD_OFFSET(RMI4_CONFIGURATION, TouchSettings) + 
            FIELD_OFFSET(RMI4_F11_CTRL_REGISTERS_LOGICAL, ManTrackEn)),
        REG_DWORD,
        &gDefaultConfiguration.TouchSettings.ManTrackEn,
        sizeof(UINT32)
    },
    {
        NULL, RTL_QUERY_REGISTRY_DIRECT,
        L"ManTrackedFinger",
        (PVOID) (FIELD_OFFSET(RMI4_CONFIGURATION, TouchSettings) + 
            FIELD_OFFSET(RMI4_F11_CTRL_REGISTERS_LOGICAL, ManTrackedFinger)),
        REG_DWORD,
        &gDefaultConfiguration.TouchSettings.ManTrackedFinger,
        sizeof(UINT32)
    },
    {
        NULL, RTL_QUERY_REGISTRY_DIRECT,
        L"DeltaXPosThreshold",
        (PVOID) (FIELD_OFFSET(RMI4_CONFIGURATION, TouchSettings) + 
            FIELD_OFFSET(RMI4_F11_CTRL_REGISTERS_LOGICAL, DeltaXPosThreshold)),
        REG_DWORD,
        &gDefaultConfiguration.TouchSettings.DeltaXPosThreshold,
        sizeof(UINT32)
    },
    {
        NULL, RTL_QUERY_REGISTRY_DIRECT,
        L"DeltaYPosThreshold",
        (PVOID) (FIELD_OFFSET(RMI4_CONFIGURATION, TouchSettings) + 
            FIELD_OFFSET(RMI4_F11_CTRL_REGISTERS_LOGICAL, DeltaYPosThreshold)),
        REG_DWORD,
        &gDefaultConfiguration.TouchSettings.DeltaYPosThreshold,
        sizeof(UINT32)
    },
    {
        NULL, RTL_QUERY_REGISTRY_DIRECT,
        L"Velocity",
        (PVOID) (FIELD_OFFSET(RMI4_CONFIGURATION, TouchSettings) + 
            FIELD_OFFSET(RMI4_F11_CTRL_REGISTERS_LOGICAL, Velocity)),
        REG_DWORD,
        &gDefaultConfiguration.TouchSettings.Velocity,
        sizeof(UINT32)
    },
    {
        NULL, RTL_QUERY_REGISTRY_DIRECT,
        L"Acceleration",
        (PVOID) (FIELD_OFFSET(RMI4_CONFIGURATION, TouchSettings) + 
            FIELD_OFFSET(RMI4_F11_CTRL_REGISTERS_LOGICAL, Acceleration)),
        REG_DWORD,
        &gDefaultConfiguration.TouchSettings.Acceleration,
        sizeof(UINT32)
    },
    {
        NULL, RTL_QUERY_REGISTRY_DIRECT,
        L"SensorMaxXPos",
        (PVOID) (FIELD_OFFSET(RMI4_CONFIGURATION, TouchSettings) + 
            FIELD_OFFSET(RMI4_F11_CTRL_REGISTERS_LOGICAL, SensorMaxXPos)),
        REG_DWORD,
        &gDefaultConfiguration.TouchSettings.SensorMaxXPos,
        sizeof(UINT32)
    },
    {
        NULL, RTL_QUERY_REGISTRY_DIRECT,
        L"SensorMaxYPos",
        (PVOID) (FIELD_OFFSET(RMI4_CONFIGURATION, TouchSettings) + 
            FIELD_OFFSET(RMI4_F11_CTRL_REGISTERS_LOGICAL, SensorMaxYPos)),
        REG_DWORD,
        &gDefaultConfiguration.TouchSettings.SensorMaxYPos,
        sizeof(UINT32)
    },
        {
        NULL, RTL_QUERY_REGISTRY_DIRECT,
        L"ZTouchThreshold",
        (PVOID) (FIELD_OFFSET(RMI4_CONFIGURATION, TouchSettings) + 
            FIELD_OFFSET(RMI4_F11_CTRL_REGISTERS_LOGICAL, ZTouchThreshold)),
        REG_DWORD,
        &gDefaultConfiguration.TouchSettings.ZTouchThreshold,
        sizeof(UINT32)
    },
    {
        NULL, RTL_QUERY_REGISTRY_DIRECT,
        L"ZHysteresis",
        (PVOID) (FIELD_OFFSET(RMI4_CONFIGURATION, TouchSettings) + 
            FIELD_OFFSET(RMI4_F11_CTRL_REGISTERS_LOGICAL, ZHysteresis)),
        REG_DWORD,
        &gDefaultConfiguration.TouchSettings.ZHysteresis,
        sizeof(UINT32)
    },
    {
        NULL, RTL_QUERY_REGISTRY_DIRECT,
        L"SmallZThreshold",
        (PVOID) (FIELD_OFFSET(RMI4_CONFIGURATION, TouchSettings) + 
            FIELD_OFFSET(RMI4_F11_CTRL_REGISTERS_LOGICAL, SmallZThreshold)),
        REG_DWORD,
        &gDefaultConfiguration.TouchSettings.SmallZThreshold,
        sizeof(UINT32)
    },
    {
        NULL, RTL_QUERY_REGISTRY_DIRECT,
        L"SmallZScaleFactor",
        (PVOID) (FIELD_OFFSET(RMI4_CONFIGURATION, TouchSettings) + 
            FIELD_OFFSET(RMI4_F11_CTRL_REGISTERS_LOGICAL, SmallZScaleFactor)),
        REG_DWORD,
        &gDefaultConfiguration.TouchSettings.SmallZScaleFactor,
        sizeof(UINT32)
    },
    {
        NULL, RTL_QUERY_REGISTRY_DIRECT,
        L"LargeZScaleFactor",
        (PVOID) (FIELD_OFFSET(RMI4_CONFIGURATION, TouchSettings) + 
            FIELD_OFFSET(RMI4_F11_CTRL_REGISTERS_LOGICAL, LargeZScaleFactor)),
        REG_DWORD,
        &gDefaultConfiguration.TouchSettings.LargeZScaleFactor,
        sizeof(UINT32)
    },
    {
        NULL, RTL_QUERY_REGISTRY_DIRECT,
        L"AlgorithmSelection",
        (PVOID) (FIELD_OFFSET(RMI4_CONFIGURATION, TouchSettings) + 
            FIELD_OFFSET(RMI4_F11_CTRL_REGISTERS_LOGICAL, AlgorithmSelection)),
        REG_DWORD,
        &gDefaultConfiguration.TouchSettings.AlgorithmSelection,
        sizeof(UINT32)
    },
    {
        NULL, RTL_QUERY_REGISTRY_DIRECT,
        L"WxScaleFactor",
        (PVOID) (FIELD_OFFSET(RMI4_CONFIGURATION, TouchSettings) + 
            FIELD_OFFSET(RMI4_F11_CTRL_REGISTERS_LOGICAL, WxScaleFactor)),
        REG_DWORD,
        &gDefaultConfiguration.TouchSettings.WxScaleFactor,
        sizeof(UINT32)
    },
    {
        NULL, RTL_QUERY_REGISTRY_DIRECT,
        L"WxOffset",
        (PVOID) (FIELD_OFFSET(RMI4_CONFIGURATION, TouchSettings) + 
            FIELD_OFFSET(RMI4_F11_CTRL_REGISTERS_LOGICAL, WxOffset)),
        REG_DWORD,
        &gDefaultConfiguration.TouchSettings.WxOffset,
        sizeof(UINT32)
    },
    {
        NULL, RTL_QUERY_REGISTRY_DIRECT,
        L"WyScaleFactor",
        (PVOID) (FIELD_OFFSET(RMI4_CONFIGURATION, TouchSettings) + 
            FIELD_OFFSET(RMI4_F11_CTRL_REGISTERS_LOGICAL, WyScaleFactor)),
        REG_DWORD,
        &gDefaultConfiguration.TouchSettings.WyScaleFactor,
        sizeof(UINT32)
    },
    {
        NULL, RTL_QUERY_REGISTRY_DIRECT,
        L"WyOffset",
        (PVOID) (FIELD_OFFSET(RMI4_CONFIGURATION, TouchSettings) + 
            FIELD_OFFSET(RMI4_F11_CTRL_REGISTERS_LOGICAL, WyOffset)),
        REG_DWORD,
        &gDefaultConfiguration.TouchSettings.WyOffset,
        sizeof(UINT32)
    },
    {
        NULL, RTL_QUERY_REGISTRY_DIRECT,
        L"XPitch",
        (PVOID) (FIELD_OFFSET(RMI4_CONFIGURATION, TouchSettings) + 
            FIELD_OFFSET(RMI4_F11_CTRL_REGISTERS_LOGICAL, XPitch)),
        REG_DWORD,
        &gDefaultConfiguration.TouchSettings.XPitch,
        sizeof(UINT32)
    },
    {
        NULL, RTL_QUERY_REGISTRY_DIRECT,
        L"YPitch",
        (PVOID) (FIELD_OFFSET(RMI4_CONFIGURATION, TouchSettings) + 
            FIELD_OFFSET(RMI4_F11_CTRL_REGISTERS_LOGICAL, YPitch)),
        REG_DWORD,
        &gDefaultConfiguration.TouchSettings.YPitch,
        sizeof(UINT32)
    },
    {
        NULL, RTL_QUERY_REGISTRY_DIRECT,
        L"FingerWidthX",
        (PVOID) (FIELD_OFFSET(RMI4_CONFIGURATION, TouchSettings) + 
            FIELD_OFFSET(RMI4_F11_CTRL_REGISTERS_LOGICAL, FingerWidthX)),
        REG_DWORD,
        &gDefaultConfiguration.TouchSettings.FingerWidthX,
        sizeof(UINT32)
    },
    {
        NULL, RTL_QUERY_REGISTRY_DIRECT,
        L"FingerWidthY",
        (PVOID) (FIELD_OFFSET(RMI4_CONFIGURATION, TouchSettings) + 
            FIELD_OFFSET(RMI4_F11_CTRL_REGISTERS_LOGICAL, FingerWidthY)),
        REG_DWORD,
        &gDefaultConfiguration.TouchSettings.FingerWidthY,
        sizeof(UINT32)
    },
    {
        NULL, RTL_QUERY_REGISTRY_DIRECT,
        L"ReportMeasuredSize",
        (PVOID) (FIELD_OFFSET(RMI4_CONFIGURATION, TouchSettings) + 
            FIELD_OFFSET(RMI4_F11_CTRL_REGISTERS_LOGICAL, ReportMeasuredSize)),
        REG_DWORD,
        &gDefaultConfiguration.TouchSettings.ReportMeasuredSize,
        sizeof(UINT32)
    },
    {
        NULL, RTL_QUERY_REGISTRY_DIRECT,
        L"SegmentationSensitivity",
        (PVOID) (FIELD_OFFSET(RMI4_CONFIGURATION, TouchSettings) + 
            FIELD_OFFSET(RMI4_F11_CTRL_REGISTERS_LOGICAL, SegmentationSensitivity)),
        REG_DWORD,
        &gDefaultConfiguration.TouchSettings.SegmentationSensitivity,
        sizeof(UINT32)
    },
    {
        NULL, RTL_QUERY_REGISTRY_DIRECT,
        L"XClipLo",
        (PVOID) (FIELD_OFFSET(RMI4_CONFIGURATION, TouchSettings) + 
            FIELD_OFFSET(RMI4_F11_CTRL_REGISTERS_LOGICAL, XClipLo)),
        REG_DWORD,
        &gDefaultConfiguration.TouchSettings.XClipLo,
        sizeof(UINT32)
    },
    {
        NULL, RTL_QUERY_REGISTRY_DIRECT,
        L"XClipHi",
        (PVOID) (FIELD_OFFSET(RMI4_CONFIGURATION, TouchSettings) + 
            FIELD_OFFSET(RMI4_F11_CTRL_REGISTERS_LOGICAL, XClipHi)),
        REG_DWORD,
        &gDefaultConfiguration.TouchSettings.XClipHi,
        sizeof(UINT32)
    },
    {
        NULL, RTL_QUERY_REGISTRY_DIRECT,
        L"YClipLo",
        (PVOID) (FIELD_OFFSET(RMI4_CONFIGURATION, TouchSettings) + 
            FIELD_OFFSET(RMI4_F11_CTRL_REGISTERS_LOGICAL, YClipLo)),
        REG_DWORD,
        &gDefaultConfiguration.TouchSettings.YClipLo,
        sizeof(UINT32)
    },
    {
        NULL, RTL_QUERY_REGISTRY_DIRECT,
        L"YClipHi",
        (PVOID) (FIELD_OFFSET(RMI4_CONFIGURATION, TouchSettings) + 
            FIELD_OFFSET(RMI4_F11_CTRL_REGISTERS_LOGICAL, YClipHi)),
        REG_DWORD,
        &gDefaultConfiguration.TouchSettings.YClipHi,
        sizeof(UINT32)
    },
    {
        NULL, RTL_QUERY_REGISTRY_DIRECT,
        L"MinFingerSeparation",
        (PVOID) (FIELD_OFFSET(RMI4_CONFIGURATION, TouchSettings) + 
            FIELD_OFFSET(RMI4_F11_CTRL_REGISTERS_LOGICAL, MinFingerSeparation)),
        REG_DWORD,
        &gDefaultConfiguration.TouchSettings.MinFingerSeparation,
        sizeof(UINT32)
    },
    {
        NULL, RTL_QUERY_REGISTRY_DIRECT,
        L"MaxFingerMovement",
        (PVOID) (FIELD_OFFSET(RMI4_CONFIGURATION, TouchSettings) + 
            FIELD_OFFSET(RMI4_F11_CTRL_REGISTERS_LOGICAL, MaxFingerMovement)),
        REG_DWORD,
        &gDefaultConfiguration.TouchSettings.MaxFingerMovement,
        sizeof(UINT32)
    },

    //
    // Add new supported functions here
    //

    //
    // Internal driver settings
    //
    {
        NULL, RTL_QUERY_REGISTRY_DIRECT,
        L"PepRemovesVoltageInD3",
        (PVOID) (FIELD_OFFSET(RMI4_CONFIGURATION, PepRemovesVoltageInD3)),
        REG_DWORD,
        &gDefaultConfiguration.PepRemovesVoltageInD3,
        sizeof(UINT32)
    },

    //
    // List Terminator
    //
    {
        NULL, 0,
        NULL,
        0,
        REG_DWORD,
        NULL,
        0
    }
};
static const ULONG gcbRegistryTable = sizeof(gRegistryTable);
static const ULONG gcRegistryTable = 
    sizeof(gRegistryTable) / sizeof(gRegistryTable[0]);


NTSTATUS
TchRegistryGetControllerSettings(
    IN VOID* ControllerContext,
    IN WDFDEVICE FxDevice
    )
/*++
 
  Routine Description:

    This routine retrieves controller wide settings
    from the registry.

  Arguments:

    FxDevice - a handle to the framework device object
    Settings - A pointer to the chip settings structure

  Return Value:

    NTSTATUS indicating success or failure

--*/
{
    RMI4_CONTROLLER_CONTEXT* controller;
    HANDLE hKey;
    ULONG i;
    WDFKEY key;
    PRTL_QUERY_REGISTRY_TABLE regTable;
    NTSTATUS status;
    WDFKEY subkey;
    DECLARE_CONST_UNICODE_STRING(subkeyName, L"Settings");

    controller = (RMI4_CONTROLLER_CONTEXT*) ControllerContext;

    hKey = NULL;
    key = NULL;
    regTable = NULL;
    subkey = NULL;

    //
    // Obtain a WDM hkey for RtlQueryRegistryValues
    //

    status = WdfDeviceOpenRegistryKey(
        FxDevice,
        PLUGPLAY_REGKEY_DEVICE,
        KEY_READ,
        WDF_NO_OBJECT_ATTRIBUTES,
        &key);
  
    if (!NT_SUCCESS(status))
    {
        Trace(
            TRACE_LEVEL_ERROR,
            TRACE_FLAG_REGISTRY,
            "Error opening device registry key - %!STATUS!",
            status);

        goto exit;
    }

    status = WdfRegistryOpenKey(
        key,
        &subkeyName,
        KEY_READ,
        WDF_NO_OBJECT_ATTRIBUTES,
        &subkey);

    if (!NT_SUCCESS(status))
    {
        Trace(
            TRACE_LEVEL_ERROR,
            TRACE_FLAG_REGISTRY,
            "Error opening device registry subkey - %!STATUS!",
            status);

        goto exit;
    }

    hKey = WdfRegistryWdmGetHandle(subkey);

    if (NULL == hKey)
    {
        Trace(
            TRACE_LEVEL_ERROR,
            TRACE_FLAG_REGISTRY,
            "Error getting WDM handle to WDF subkey");

        goto exit;
    }
    
    //
    // RtlQueryRegistryValues table must be allocated from NonPagedPool
    //

    regTable = ExAllocatePoolWithTag(
        NonPagedPool,
        gcbRegistryTable,
        TOUCH_POOL_TAG);

    RtlCopyMemory(
        regTable,
        gRegistryTable,
        gcbRegistryTable);

    //
    // Update offset values with base pointer
    // 

    for (i=0; i < gcRegistryTable-1; i++)
    {
        regTable[i].EntryContext = (PVOID) (
            ((SIZE_T) regTable[i].EntryContext) +
            ((ULONG_PTR) &controller->Config));
    }

    //
    // Populate device context with registry or default configurations
    //

    status = RtlQueryRegistryValues(
        RTL_REGISTRY_HANDLE,
        (PCWSTR) hKey,
        regTable,
        NULL,
        NULL);

    if (!NT_SUCCESS(status))
    {
        Trace(
            TRACE_LEVEL_ERROR,
            TRACE_FLAG_REGISTRY,
            "Error retrieving registry configuration - %!STATUS!",
            status);

        goto exit;
    }

exit:

    if (!NT_SUCCESS(status))
    {
        //
        // Revert to default configuration values if there was an
        // issue reading configuration data from the registry
        //
        RtlCopyMemory(
            &controller->Config,
            &gDefaultConfiguration,
            sizeof(RMI4_CONFIGURATION));

        Trace(
            TRACE_LEVEL_WARNING,
            TRACE_FLAG_REGISTRY,
            "Error reading registry config, using defaults! - %!STATUS!",
            status);

        status = STATUS_SUCCESS;
    }

    if (subkey != NULL)
    {
        WdfRegistryClose(subkey);
    }

    if (key != NULL)
    {
        WdfRegistryClose(key);
    }

    if (regTable != NULL)
    {
        ExFreePoolWithTag(regTable, TOUCH_POOL_TAG);
    }

    return status;
}