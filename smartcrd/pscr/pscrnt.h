/*++

Copyright (c) 1997 - 1999 SCM Microsystems, Inc.

Module Name:

    PscrNT.h

Abstract:

    Driver header - NT Version

Author:

    Andreas Straub  (SCM Microsystems, Inc.)
    Klaus Schuetz   (Microsoft Corp.)

Notes:

    The file pscrnt.h was reviewed by LCA in June 2011 and per license is
    acceptable for Microsoft use under Dealpoint ID 178449.

Revision History:

    Andreas Straub  1.00        8/18/1997       Initial Version
    Klaus Schuetz   1.01        9/20/1997       Timing changed
    Andreas Straub  1.02        9/24/1997       Low Level error handling,
                                                minor bugfixes, clanup
    Andreas Straub  1.03        10/8/1997       Timing changed, generic SCM
                                                interface changed
    Andreas Straub  1.04        10/18/1997      Interrupt handling changed
    Andreas Straub  1.05        10/19/1997      Generic IOCTL's added
    Andreas Straub  1.06        10/25/1997      Timeout limit for FW update variable
    Andreas Straub  1.07        11/7/1997       Version information added
    Andreas Straub  1.08        11/10/1997      Generic IOCTL GET_CONFIGURATION
    Klaus Schuetz               1998            PnP and Power Management added

--*/

#if !defined ( __PSCR_NT_DRV_H__ )
#define __PSCR_NT_DRV_H__
#define SMARTCARD_POOL_TAG '4SCS'

#include <ntddk.h>
#include <wdf.h>
#include <DEVIOCTL.H>

#pragma warning(push)
#pragma warning(disable:4201)
#include "SMCLIB.h"
#pragma warning(pop)

#include "PscrRdWr.h"

_Analysis_mode_(_Analysis_code_type_kernel_driver_)

#if !defined( STATUS_DEVICE_REMOVED )
#define STATUS_DEVICE_REMOVED STATUS_UNSUCCESSFUL
#endif

#define SysCompareMemory( p1, p2, Len )         ( RtlCompareMemory( p1,p2, Len ) != Len )
#define SysCopyMemory( pDest, pSrc, Len )       RtlCopyMemory( pDest, pSrc, Len )
#define SysFillMemory( pDest, Value, Len )      RtlFillMemory( pDest, Len, Value )

#define DELAY_WRITE_PSCR_REG    1
#define DELAY_PSCR_WAIT         5

#define LOBYTE( any )   ((UCHAR)( any & 0xFF ) )
#define HIBYTE( any )   ((UCHAR)( ( any >> 8) & 0xFF ))

//
// These macros will store and retrieve the request handle from the IRP.
// We will use the first slot of DriverContext because framework reserves 
// the use of 3rd & 4rd slot of DriverContext for its own purpose.
//
#define GET_WDFREQUEST_FROM_IRP(Irp) (WDFREQUEST)Irp->Tail.Overlay.DriverContext[0]
#define SET_REQUEST_IN_IRP(Irp, Request) {Irp->Tail.Overlay.DriverContext[0] = (PVOID)Request;}

typedef struct _DEVICE_EXTENSION
{
    SMARTCARD_EXTENSION SmartcardExtension;

    WDFDEVICE Device;

    // Out interrupt resource
    WDFINTERRUPT Interrupt;

    WDFQUEUE IoctlQueue;

    WDFQUEUE NotificationQueue;

    // Flag that indicates if we need to unmap the port upon stop
    BOOLEAN UnMapPort;

    // Number of pending card tracking interrupts
    ULONG PendingInterrupts;

    // If the PCMCIA works in PCI mode, check the time between two interrupts
    BOOLEAN         bSharedIRQ;
    LARGE_INTEGER   SaveTime;

    ULONG DeviceInstanceNo;
} DEVICE_EXTENSION, *PDEVICE_EXTENSION;

//
// This macro defines an accessor function which will be used to get the context
// pointer from the device handle.
//
WDF_DECLARE_CONTEXT_TYPE_WITH_NAME(DEVICE_EXTENSION, GetDeviceExtension)

#define IOCTL_PSCR_COMMAND      SCARD_CTL_CODE( 0x8000 )
#define IOCTL_GET_VERSIONS      SCARD_CTL_CODE( 0x8001 )
#define IOCTL_SET_TIMEOUT       SCARD_CTL_CODE( 0x8002 )
#define IOCTL_GET_CONFIGURATION SCARD_CTL_CODE( 0x8003 )

typedef struct _VERSION_CONTROL
{
    ULONG   SmclibVersion;
    UCHAR   DriverMajor,
            DriverMinor,
            FirmwareMajor,
            FirmwareMinor,
            UpdateKey;
} VERSION_CONTROL, *PVERSION_CONTROL;

#define SIZEOF_VERSION_CONTROL  sizeof( VERSION_CONTROL )

typedef struct _PSCR_CONFIGURATION
{
    PPSCR_REGISTERS IOBase;
    ULONG           IRQ;

} PSCR_CONFIGURATION, *PPSCR_CONFIGURATION;

#define SIZEOF_PSCR_CONFIGURATION   sizeof( PSCR_CONFIGURATION )


DRIVER_INITIALIZE DriverEntry;

EVT_WDF_DRIVER_DEVICE_ADD PscrEvtDeviceAdd;

EVT_WDF_OBJECT_CONTEXT_CLEANUP PscrEvtDeviceContextCleanup;

EVT_WDF_DEVICE_D0_ENTRY PscrEvtDeviceD0Entry;
EVT_WDF_DEVICE_D0_EXIT PscrEvtDeviceD0Exit;
EVT_WDF_DEVICE_PREPARE_HARDWARE PscrEvtDevicePrepareHardware;

EVT_WDF_IO_QUEUE_IO_DEVICE_CONTROL PscrEvtIoDeviceControl;
EVT_WDF_IO_QUEUE_IO_CANCELED_ON_QUEUE PscrEvtIoCanceledOnQueue;

EVT_WDF_FILE_CLEANUP PscrEvtFileCleanup;

EVT_WDF_INTERRUPT_ISR PscrEvtInterruptServiceRoutine;
EVT_WDF_INTERRUPT_DPC PscrEvtInterruptDpc;

NTSTATUS
PscrGenericIOCTL(
    PSMARTCARD_EXTENSION SmartcardExtension
    );

VOID
PscrFreeze(
    PSMARTCARD_EXTENSION    SmartcardExtension
    );

VOID
SysDelay(
    ULONG Timeout
    );

NTSTATUS
PscrRegisterWithSmcLib(
    PDEVICE_EXTENSION DeviceExtension
    );

WDFSTRING
PscrGetRegistryValue(
    _In_  WDFDEVICE   Device,
    _In_  LPCWSTR     Name
    );

PCHAR
GetIoctlName(
    ULONG IoControlCode
    );

#endif  // __PSCR_NT_DRV_H__
