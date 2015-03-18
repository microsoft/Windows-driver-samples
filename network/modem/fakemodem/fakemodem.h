/*++

Copyright (c) 1990-2000 Microsoft Corporation All Rights Reserved

Module Name:

    fakemodem.h

Abstract:

    Header file for the toaster driver modules.

Environment:

    Kernel mode

--*/


#if !defined(_FAKEMODEM_H_)
#define _FAKEMODEM_H_

#include <NTDDK.h>
#include <wdf.h>
#include <ntddser.h>
#include <initguid.h>
#define NTSTRSAFE_LIB
#include <ntstrsafe.h>
#include <ntintsafe.h>

#ifdef DEFINE_GUID

DEFINE_GUID(GUID_DEVINTERFACE_MODEM,0x2c7089aa, 0x2e0e,0x11d1,0xb1, 0x14, 0x00, 0xc0, 0x4f, 0xc2, 0xaa, 0xe4);

#endif //DEFINE_GUID


#define OBJECT_DIRECTORY L"\\DosDevices\\"

#define READ_BUFFER_SIZE  128

#define COMMAND_MATCH_STATE_IDLE   0
#define COMMAND_MATCH_STATE_GOT_A  1
#define COMMAND_MATCH_STATE_GOT_T  2

//
// This defines the bit used to control whether the device is sending
// a break.  When this bit is set the device is sending a space (logic 0).
//
// Most protocols will assume that this is a hangup.


#define SERIAL_LCR_BREAK    0x40

//
// These defines are used to define the line control register
//

#define SERIAL_5_DATA       ((UCHAR)0x00)
#define SERIAL_6_DATA       ((UCHAR)0x01)
#define SERIAL_7_DATA       ((UCHAR)0x02)
#define SERIAL_8_DATA       ((UCHAR)0x03)
#define SERIAL_DATA_MASK    ((UCHAR)0x03)

#define SERIAL_1_STOP       ((UCHAR)0x00)
#define SERIAL_1_5_STOP     ((UCHAR)0x04) // Only valid for 5 data bits
#define SERIAL_2_STOP       ((UCHAR)0x04) // Not valid for 5 data bits
#define SERIAL_STOP_MASK    ((UCHAR)0x04)

#define SERIAL_NONE_PARITY  ((UCHAR)0x00)
#define SERIAL_ODD_PARITY   ((UCHAR)0x08)
#define SERIAL_EVEN_PARITY  ((UCHAR)0x18)
#define SERIAL_MARK_PARITY  ((UCHAR)0x28)
#define SERIAL_SPACE_PARITY ((UCHAR)0x38)
#define SERIAL_PARITY_MASK  ((UCHAR)0x38)

#define REG_VALUE_CREATED_FLAG 0x1
//
// The device extension for the device object
//
typedef struct _FM_DEVICE_DATA
{

    UNICODE_STRING   PdoName;  //save this so that we can use it to delete the registry value later
    WDFQUEUE         FmReadQueue;    // Staging area for pending Read requests
    WDFQUEUE         FmMaskWaitQueue;
    ULONG            CurrentMask;
    SERIAL_TIMEOUTS  CurrentTimeouts;
    ULONG            ReadBufferBegin;
    ULONG            ReadBufferEnd;
    ULONG            BytesInReadBuffer;
    UCHAR            CommandMatchState;
    BOOLEAN          ConnectCommand;
    BOOLEAN          IgnoreNextChar;
    BOOLEAN          CapsQueried;
    ULONG            ModemStatus;
    BOOLEAN          CurrentlyConnected;
    BOOLEAN          ConnectionStateChanged;
    UCHAR            ReadBuffer[READ_BUFFER_SIZE];
    ULONG            BaudRate;
    UCHAR            LineControl;
    UCHAR            ValidDataMask;
    UCHAR            Flags;

}  FM_DEVICE_DATA, *PFM_DEVICE_DATA;

WDF_DECLARE_CONTEXT_TYPE_WITH_NAME(FM_DEVICE_DATA, FmDeviceDataGet)

#define FM_COM_PORT_STRING_LENGTH  80

DRIVER_INITIALIZE DriverEntry;

EVT_WDF_DRIVER_DEVICE_ADD FmEvtDeviceAdd;
EVT_WDF_DEVICE_CONTEXT_CLEANUP FmDeviceCleanup;

EVT_WDF_IO_QUEUE_IO_READ FmEvtIoRead;
EVT_WDF_IO_QUEUE_IO_WRITE FmEvtIoWrite;
EVT_WDF_IO_QUEUE_IO_DEVICE_CONTROL FmEvtIoDeviceControl;

NTSTATUS
FmCreateDosDevicesSymbolicLink(
    WDFDEVICE       Device,
    PFM_DEVICE_DATA FmDeviceData
    );

VOID
ProcessConnectionStateChange(
    IN PFM_DEVICE_DATA  FmDeviceData
    );
VOID
ProcessWriteBytes(
    PFM_DEVICE_DATA   FmDeviceData,
    PUCHAR            Characters,
    ULONG             Length
    );


VOID
PutCharInReadBuffer(
    PFM_DEVICE_DATA   FmDeviceData,
    UCHAR             Character
    );



VOID
ProcessReadBuffer(
    IN  PFM_DEVICE_DATA FmDeviceData,
    IN  PUCHAR  SystemBuffer,
    IN  ULONG   Length,
    OUT PULONG  ByesToMove
    );

#endif  // _FAKEMODEM_H


