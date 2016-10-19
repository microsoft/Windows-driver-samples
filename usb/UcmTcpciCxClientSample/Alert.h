/*++

Module Name:

    Alert.h

Abstract:

    This file contains the declarations for alert callbacks.

Environment:

    Kernel-mode Driver Framework

--*/

#pragma once

#pragma warning(push)
#pragma warning(disable:4201) // nonstandard extension used : nameless struct/union
#pragma warning(disable:4214) // nonstandard extension used : bit field types other than int

// Pack structure so that we can directly fill it with the contents of the alert register.
// Without this step, the structure may have extra padding that would cause the alert register data
// to not match up with the fields in the struct.
#include <pshpack1.h>

// Alert register as defined in the USB-Port Controller Specification R1.0.
typedef union _ALERT_REGISTER
{
    UINT16 AsUInt16;

    struct
    {
        UINT16 CCStatus : 1;
        UINT16 PowerStatus : 1;
        UINT16 ReceiveSOPMessageStatus : 1;
        UINT16 ReceivedHardReset : 1;
        UINT16 TransmitSOPMessageFailed : 1;
        UINT16 TransmitSOPMessageDiscarded : 1;
        UINT16 TransmitSOPMessageSuccessful : 1;
        UINT16 VbusVoltageAlarmHi : 1;
        UINT16 VbusVoltageAlarmLo : 1;
        UINT16 Fault : 1;
        UINT16 RxBufferOverflow : 1;
        UINT16 VbusSinkDisconnectDetected : 1;
        UINT16 : 4;
    };
} ALERT_REGISTER, *PALERT_REGISTER;

#include <poppack.h>
#pragma warning(pop)

#define MAX_ALERTS 12
#define MAX_ALERTS_TO_PROCESS 10

EXTERN_C_START

EVT_WDF_INTERRUPT_ISR
OnInterruptPassiveIsr;

void
ProcessAndSendAlerts(
    _In_ PALERT_REGISTER AlertRegister,
    _In_ PDEVICE_CONTEXT DeviceContext
);

EXTERN_C_END