/*++

Module Name:

    PortControllerInterface.h

Abstract:

    This file contains the declarations of functions to read to and write from the
    Type-C port controller hardware registers.

Environment:

    Kernel-mode Driver Framework

--*/

#pragma once

void
PostponeToWorkitem(
    _In_ WDFREQUEST Request,
    _In_ WDFDEVICE Device,
    _In_ WDFWORKITEM Workitem
);

EVT_WDF_WORKITEM
EvtWorkItemGetStatus;

EVT_WDF_WORKITEM
EvtWorkItemGetControl;

void
EvtSetControl(
    _In_ WDFREQUEST Request,
    _In_ WDFDEVICE Device
);

void
EvtSetCommand(
    _In_ WDFREQUEST Request,
    _In_ WDFDEVICE Device
);

void
EvtSetConfigStandardOutput(
    _In_ WDFREQUEST Request,
    _In_ WDFDEVICE Device
);

void
EvtSetMessageHeaderInfo(
    _In_ WDFREQUEST Request,
    _In_ WDFDEVICE Device
);

void
EvtSetReceiveDetect(
    _In_ WDFREQUEST Request,
    _In_ WDFDEVICE Device
);

void
EvtSetTransmit(
    _In_ WDFREQUEST Request,
    _In_ WDFDEVICE Device
);

void
EvtSetTransmitBuffer(
    _In_ WDFREQUEST Request,
    _In_ WDFDEVICE Device
);
