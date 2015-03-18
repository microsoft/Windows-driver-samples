/*++

  THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
  ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
  THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
  PARTICULAR PURPOSE.

  Copyright (c) Microsoft Corporation. All rights reserved

Module Name:

    elamsample.h

Abstract:

    Contains function prototypes and includes other neccessary header files.

Environment:

    Kernel mode only.

--*/

//
// ------------------------------------------------------------------- Includes
//

#include <ntddk.h>
#include <wdf.h>

//
// -------------------------------------------------------- Function Prototypes
//

DRIVER_INITIALIZE
DriverEntry;
/*++

Routine Description:

    This routine is called by the Operating System to initialize the driver.

    It creates the device object, fills in the dispatch entry points and
    completes the initialization.

Arguments:

    DriverObject - Supplies a pointer to the object that represents this device
         driver.

    RegistryPath - Supplies a pointer to the Services key in the registry.

Return Value:

    STATUS_SUCCESS if initialized successfully.

    Error status if the driver could not be initialized.

--*/

EVT_WDF_DRIVER_UNLOAD
ElamSampleEvtDriverUnload;
/*++

Routine Description:

    This routine is called by the I/O subsystem before unloading the driver.

    It creates the device object, fills in the dispatch entry points and
    completes the initialization.

Arguments:

    Driver - Supplies a handle to a framework driver object.

Return Value:

    None.

--*/

VOID
ElamSampleBootDriverCallback(
    _In_opt_ PVOID CallbackContext,
    _In_ BDCB_CALLBACK_TYPE Classification,
    _Inout_ PBDCB_IMAGE_INFORMATION ImageInformation
    );
/*++

Routine Description:

    This routine is called by the Operating System when boot start drivers are
    being initialized.

Arguments:

    CallbackContext - Supplies the opaque context specified during callback
         registration.

    Classification - Supplies the type of the callback, including status update
         or image initialized.

    ImageInformation - Supplies a pointer to information about the next boot
         driver that is about to be initialized.

Return Value:

    None.

--*/

VOID
ElamSampleProcessStatusUpdate(
    _In_ BDCB_STATUS_UPDATE_TYPE StatusType
    );
/*++

Routine Description:

    This routine processes the BdCbStatusUpdate callback type.

Arguments:

    StatusType - Supplies the type of status that is being reported.

Return Value:

    None.

--*/

VOID
ElamSampleProcessInitializeImage(
    _Inout_ PBDCB_IMAGE_INFORMATION ImageInformation
    );
/*++

Routine Description:

    This routine processes the BdCbInitializeImage callback type.

Arguments:

    ImageInformation - Supplies a pointer to information about the next boot
         driver that is about to be initialized.

Return Value:

    None.

--*/

VOID
ElamSamplePrintHex(
    _In_reads_bytes_(DataSize) PVOID Data,
    _In_ ULONG DataSize
    );
/*++

Routine Description:

    This routine prints out the supplied data in hexadecimal form.

Arguments:

    Data - Supplies a pointer to the data to be printed.

    DataSize - Supplies the length in bytes of the data to be printed.

Return Value:

    None.

--*/
