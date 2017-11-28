// THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
// ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
// THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
// PARTICULAR PURPOSE.
//
// Copyright (c) Microsoft Corporation. All rights reserved
//
// Module Name:
//
//     elamsample.c
//
// Abstract:
//
//     This driver demonstrates how to use the Boot Driver Callback APIs
//     IoRegisterBootDriverCallback and IoUnRegisterBootDriverCallback and
//     the callback type PBOOT_DRIVER_CALLBACK_FUNCTION.
//
// Environment:
//
//     Kernel mode only.
//

#include "elamsample.h"

#ifdef ALLOC_PRAGMA
#pragma alloc_text(INIT, DriverEntry)
#endif // ALLOC_PRAGMA

//
// Trace level to use for kernel debugger DbgPrintEx output.
//

#define ELAMSAMPLE_TRACE_LEVEL DPFLTR_TRACE_LEVEL

//
// Callback handle returned by IoRegisterBootDriverCallback.
//

static PVOID g_IoRegisterBootDriverCallbackHandle = NULL;

//
// Current status update type from the callback.
//

static BDCB_STATUS_UPDATE_TYPE g_CurrentBcdCallbackContextType =
                                    BdCbStatusPrepareForDependencyLoad;

NTSTATUS
DriverEntry(
    _In_ PDRIVER_OBJECT  DriverObject,
    _In_ PUNICODE_STRING RegistryPath
    )
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
{
    WDF_OBJECT_ATTRIBUTES Attributes;
    WDF_DRIVER_CONFIG Config;
    WDFDRIVER Driver;
    NTSTATUS Status;

    DbgPrintEx(DPFLTR_IHVDRIVER_ID,
               ELAMSAMPLE_TRACE_LEVEL,
               "ElamSample is being initialized.\r\n");

    //
    // Initialize a non-PnP driver with the framework.
    //

    WDF_DRIVER_CONFIG_INIT(&Config, WDF_NO_EVENT_CALLBACK);

    Config.DriverInitFlags |= WdfDriverInitNonPnpDriver;

    //
    // Non-PnP drivers must register an unload routine.
    //

    Config.EvtDriverUnload = ElamSampleEvtDriverUnload;

    //
    // Create a framework driver object.
    //

    WDF_OBJECT_ATTRIBUTES_INIT(&Attributes);

    Status = WdfDriverCreate(DriverObject,
                             RegistryPath,
                             &Attributes,
                             &Config,
                             &Driver);

    if (!NT_SUCCESS(Status))
    {
        return Status;
    }

    //
    // Register for the boot driver callback.
    //

    g_IoRegisterBootDriverCallbackHandle = IoRegisterBootDriverCallback(
                                                ElamSampleBootDriverCallback,
                                                NULL);

    if (g_IoRegisterBootDriverCallbackHandle == NULL)
    {
        return STATUS_UNSUCCESSFUL;
    }

    return STATUS_SUCCESS;
}

VOID
ElamSampleEvtDriverUnload(
    _In_ WDFDRIVER Driver
    )
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
{
    UNREFERENCED_PARAMETER(Driver);

    if (g_IoRegisterBootDriverCallbackHandle != NULL)
    {
        IoUnregisterBootDriverCallback(g_IoRegisterBootDriverCallbackHandle);
        g_IoRegisterBootDriverCallbackHandle = NULL;
    }

    DbgPrintEx(DPFLTR_IHVDRIVER_ID,
               ELAMSAMPLE_TRACE_LEVEL,
               "ElamSample is being unloaded.\r\n");
}

VOID
ElamSampleBootDriverCallback(
    _In_opt_ PVOID CallbackContext,
    _In_ BDCB_CALLBACK_TYPE Classification,
    _Inout_ PBDCB_IMAGE_INFORMATION ImageInformation
    )
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
{
    PBDCB_STATUS_UPDATE_CONTEXT StatusUpdate;

    //
    // IoRegisterBootDriverCallback was called with a null context. Ensure that
    // is passed here.
    //

    if (CallbackContext != NULL)
    {
        DbgPrintEx(DPFLTR_IHVDRIVER_ID,
                   ELAMSAMPLE_TRACE_LEVEL,
                   "ElamSample has been passed an unexpected callback context.\r\n");
    }

    switch (Classification)
    {
    case BdCbStatusUpdate:
        StatusUpdate = (PBDCB_STATUS_UPDATE_CONTEXT)ImageInformation;

        ElamSampleProcessStatusUpdate(StatusUpdate->StatusType);
        break;

    case BdCbInitializeImage:
        ElamSampleProcessInitializeImage(ImageInformation);
        break;

    default:
        //
        // Do nothing. If new classifications are supported we should just
        // ignore them.
        //
        break;
    }
}

VOID
ElamSampleProcessStatusUpdate(
    _In_ BDCB_STATUS_UPDATE_TYPE StatusType
    )
/*++

Routine Description:

    This routine processes the BdCbStatusUpdate callback type.

Arguments:

    StatusType - Supplies the type of status that is being reported.

Return Value:

    None.

--*/
{
    switch (StatusType)
    {
    case BdCbStatusPrepareForDependencyLoad:
        DbgPrintEx(DPFLTR_IHVDRIVER_ID,
                   ELAMSAMPLE_TRACE_LEVEL,
                   "ElamSample reports that Boot Start driver dependencies are being initialized.\r\n\r\n");
        break;

    case BdCbStatusPrepareForDriverLoad:
        DbgPrintEx(DPFLTR_IHVDRIVER_ID,
                   ELAMSAMPLE_TRACE_LEVEL,
                   "ElamSample reports that Boot Start drivers are about to be initialized.\r\n\r\n");
        break;

    case BdCbStatusPrepareForUnload:
        DbgPrintEx(DPFLTR_IHVDRIVER_ID,
                   ELAMSAMPLE_TRACE_LEVEL,
                   "ElamSample reports that all Boot Start drivers have been initialized "
                   "and that ElamSample is about to be unloaded\r\n\r\n");
        break;

    default:
        DbgPrintEx(DPFLTR_IHVDRIVER_ID,
                   ELAMSAMPLE_TRACE_LEVEL,
                   "ElamSample reports an unknown status type.\r\n\r\n");
        break;
    }

    g_CurrentBcdCallbackContextType = StatusType;
}

VOID
ElamSampleProcessInitializeImage(
    _Inout_ PBDCB_IMAGE_INFORMATION ImageInformation
    )
/*++

Routine Description:

    This routine processes the BdCbInitializeImage callback type.

Arguments:

    ImageInformation - Supplies a pointer to information about the next boot
         driver that is about to be initialized.

Return Value:

    None.

--*/
{
    //
    // Is this a dependency or a boot start driver?
    //

    if (g_CurrentBcdCallbackContextType == BdCbStatusPrepareForDependencyLoad)
    {
        DbgPrintEx(DPFLTR_IHVDRIVER_ID,
                   ELAMSAMPLE_TRACE_LEVEL,
                   "ElamSample reports the following dependency is about to be initialized:\r\n");
    }
    else if (g_CurrentBcdCallbackContextType == BdCbStatusPrepareForDriverLoad)
    {
        DbgPrintEx(DPFLTR_IHVDRIVER_ID,
                   ELAMSAMPLE_TRACE_LEVEL,
                   "ElamSample reports the following Boot Start driver is about to be initialized:\r\n");
    }
    else
    {
        NT_ASSERT(g_CurrentBcdCallbackContextType == BdCbStatusPrepareForDependencyLoad ||
                  g_CurrentBcdCallbackContextType == BdCbStatusPrepareForDriverLoad);

        DbgPrintEx(DPFLTR_IHVDRIVER_ID,
                   ELAMSAMPLE_TRACE_LEVEL,
                   "ElamSample reports an invalid status type for image initialization:\r\n");
    }

    //
    // Display the image name and any associated registry path.
    //

    DbgPrintEx(DPFLTR_IHVDRIVER_ID,
               ELAMSAMPLE_TRACE_LEVEL,
               "ElamSample:    Image name \"%wZ\"\r\n",
               &ImageInformation->ImageName);

    if (ImageInformation->RegistryPath.Buffer != NULL)
    {
        DbgPrintEx(DPFLTR_IHVDRIVER_ID,
                   ELAMSAMPLE_TRACE_LEVEL,
                   "ElamSample:    Registry path \"%wZ\"\r\n",
                   &ImageInformation->RegistryPath);
    }

    //
    // Did this image fail Code Integrity checks?
    //

    if ((ImageInformation->ImageFlags & BDCB_IMAGEFLAGS_FAILED_CODE_INTEGRITY) != 0)
    {
        DbgPrintEx(DPFLTR_IHVDRIVER_ID,
                   ELAMSAMPLE_TRACE_LEVEL,
                   "ElamSample:    FAILED Code Integrity checks but boot policy allowed it to be loaded.\r\n");
    }

    //
    // Display the image's hash.
    //

    if (ImageInformation->ImageHash != NULL &&
        ImageInformation->ImageHashLength != 0)
    {
        DbgPrintEx(DPFLTR_IHVDRIVER_ID,
                   ELAMSAMPLE_TRACE_LEVEL,
                   "ElamSample:    Image hash algorithm = 0x%08x.\r\n",
                   ImageInformation->ImageHashAlgorithm);

        DbgPrintEx(DPFLTR_IHVDRIVER_ID,
                   ELAMSAMPLE_TRACE_LEVEL,
                   "ElamSample:    Image hash:");

        ElamSamplePrintHex(ImageInformation->ImageHash,
                           ImageInformation->ImageHashLength);
    }

    //
    // Display who signed the image (if at all).
    //

    if (ImageInformation->CertificatePublisher.Buffer != NULL)
    {
        DbgPrintEx(DPFLTR_IHVDRIVER_ID,
                   ELAMSAMPLE_TRACE_LEVEL,
                   "ElamSample:    Image is signed by \"%wZ\".\r\n",
                   &ImageInformation->CertificatePublisher);

        if (ImageInformation->CertificateIssuer.Buffer != NULL)
        {
            DbgPrintEx(DPFLTR_IHVDRIVER_ID,
                       ELAMSAMPLE_TRACE_LEVEL,
                       "ElamSample:    Certificate issued by \"%wZ\".\r\n",
                       &ImageInformation->CertificateIssuer);
        }

        if (ImageInformation->CertificateThumbprint != NULL &&
            ImageInformation->CertificateThumbprintLength != 0)
        {
            DbgPrintEx(DPFLTR_IHVDRIVER_ID,
                       ELAMSAMPLE_TRACE_LEVEL,
                       "ElamSample:    Certificate thumb print algorithm = 0x%08x.\r\n",
                       ImageInformation->ThumbprintHashAlgorithm);

            DbgPrintEx(DPFLTR_IHVDRIVER_ID,
                       ELAMSAMPLE_TRACE_LEVEL,
                       "ElamSample:    Certificate thumb print:");

            ElamSamplePrintHex(ImageInformation->CertificateThumbprint,
                               ImageInformation->CertificateThumbprintLength);
        }
    }
    else
    {
        DbgPrintEx(DPFLTR_IHVDRIVER_ID,
                   ELAMSAMPLE_TRACE_LEVEL,
                   "ElamSample:    Not signed.\r\n");
    }

    DbgPrintEx(DPFLTR_IHVDRIVER_ID, ELAMSAMPLE_TRACE_LEVEL, "\r\n");

    //
    // Report that we don't know the authenticity of this image.
    //

    ImageInformation->Classification = BdCbClassificationUnknownImage;
}

VOID
ElamSamplePrintHex(
    _In_reads_bytes_(DataSize) PVOID Data,
    _In_ ULONG DataSize
    )
/*++

Routine Description:

    This routine prints out the supplied data in hexadecimal form.

Arguments:

    Data - Supplies a pointer to the data to be printed.

    DataSize - Supplies the length in bytes of the data to be printed.

Return Value:

    None.

--*/
{
    PCUCHAR Bytes;
    ULONG Index;

    for (Bytes = (PCUCHAR)Data, Index = 0; Index < DataSize; Index++)
    {
        if ((Index & 15) == 0)
        {
            DbgPrintEx(DPFLTR_IHVDRIVER_ID,
                       ELAMSAMPLE_TRACE_LEVEL,
                       "\r\nElamSample:    ");
        }

        DbgPrintEx(DPFLTR_IHVDRIVER_ID,
                   ELAMSAMPLE_TRACE_LEVEL,
                   "%02x ",
                   Bytes[Index]);
    }

    DbgPrintEx(DPFLTR_IHVDRIVER_ID, ELAMSAMPLE_TRACE_LEVEL, "\r\n");
}
