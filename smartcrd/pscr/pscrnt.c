/*++

Copyright (c) 1997  - 1999 SCM Microsystems, Inc.

Module Name:

   PscrNT.c

Abstract:

   Main Driver Module - NT Version

Author:

   Andreas Straub

Notes:

   The file pscrnt.c was reviewed by LCA in June 2011 and per license is
   acceptable for Microsoft use under Dealpoint ID 178449.

Revision History:


   Andreas Straub 1.00     8/18/1997      Initial Version
   Klaus Schuetz  1.01     9/20/1997      Timing changed
   Andreas Straub 1.02     9/24/1997      Low Level error handling,
                                    minor bugfixes, clanup
   Andreas Straub 1.03     10/8/1997      Timing changed, generic SCM
                                    interface changed
   Andreas Straub 1.04     10/18/1997     Interrupt handling changed
   Andreas Straub 1.05     10/19/1997     Generic IOCTL's added
   Andreas Straub 1.06     10/25/1997     Timeout limit for FW update variable
   Andreas Straub 1.07     11/7/1997      Version information added
   Klaus Schuetz  1.08     11/10/1997     PnP capabilities added
    Klaus Schuetz                               Cleanup added
    Calai Bhoopathi         6/12/2006       Changed retry count in Dpc routine
   Eliyas Yakub             10/6/2006      Ported to KMDF

--*/

#include <PscrNT.h>
#include <PscrCmd.h>
#include <PscrCB.h>
#include <PscrLog.h>
#include <PscrVers.h>

#pragma alloc_text(INIT, DriverEntry)
#pragma alloc_text(PAGE, PscrEvtDeviceAdd)
#pragma alloc_text(PAGE, PscrRegisterWithSmcLib)
#pragma alloc_text(PAGE, PscrEvtDevicePrepareHardware)
#pragma alloc_text(PAGE, PscrEvtDeviceD0Exit)
#pragma alloc_text(PAGE, PscrEvtFileCleanup)
#pragma alloc_text(PAGE, PscrGetRegistryValue)

NTSTATUS
DriverEntry(
           PDRIVER_OBJECT DriverObject,
           PUNICODE_STRING   RegistryPath
           )
/*++

DriverEntry:
   entry function of the driver. setup the callbacks for the OS and try to
   initialize a device object for every device in the system

Arguments:
   DriverObject   context of the driver
   RegistryPath   path to the registry entry for the driver

Return Value:
   STATUS_SUCCESS
   STATUS_UNSUCCESSFUL

--*/
{
    NTSTATUS status = STATUS_SUCCESS;
    WDF_DRIVER_CONFIG   config;

    SmartcardDebug(
                  DEBUG_TRACE,
                  ( "PSCR!DriverEntry: Enter - KMDF Version Built %s %s\n",
                  __DATE__, __TIME__ )
                  );

    WDF_DRIVER_CONFIG_INIT(&config, PscrEvtDeviceAdd);


    //
    // Create a framework driver object to represent our driver.
    //
    status = WdfDriverCreate(DriverObject,
                        RegistryPath,
                        WDF_NO_OBJECT_ATTRIBUTES, // Driver Attributes
                        &config,          // Driver Config Info
                        WDF_NO_HANDLE
                        );

    if (!NT_SUCCESS(status)) {
        KdPrint( ("WdfDriverCreate failed with status 0x%x\n", status));
    }

    SmartcardDebug(
                  DEBUG_TRACE,
                  ("PSCR!DriverEntry: Exit %x\n",
                   status)
                  );

    return status;
}

NTSTATUS
PscrEvtDeviceAdd(
    IN WDFDRIVER       Driver,
    IN PWDFDEVICE_INIT DeviceInit
    )
/*++

Routine Description:

    PscrEvtDeviceAdd is called by the framework in response to AddDevice
    call from the PnP manager. We create and initialize a WDF device object to
    represent a new instance of toaster device.

Arguments:

    Driver - Handle to a framework driver object created in DriverEntry

    DeviceInit - Pointer to a framework-allocated WDFDEVICE_INIT structure.

Return Value:

    NTSTATUS
--*/
{
    NTSTATUS status;
    WDF_PNPPOWER_EVENT_CALLBACKS    pnpPowerCallbacks;
    WDF_OBJECT_ATTRIBUTES   attributes;
    WDFDEVICE   device;
    PDEVICE_EXTENSION   DeviceExtension;
    WDF_IO_QUEUE_CONFIG ioQueueConfig;
    WDF_INTERRUPT_CONFIG    interruptConfig;
    WDF_FILEOBJECT_CONFIG         fileobjectConfig;

    UNREFERENCED_PARAMETER(Driver);

    PAGED_CODE();

    SmartcardDebug(
                  DEBUG_TRACE,
                  ( "PSCR!PscrAddDevice: Enter\n" )
                  );

    WdfDeviceInitSetDeviceType(DeviceInit, FILE_DEVICE_SMARTCARD);

    WdfDeviceInitSetExclusive(DeviceInit, TRUE);

    WdfDeviceInitSetIoType(DeviceInit, WdfDeviceIoBuffered);

    WDF_PNPPOWER_EVENT_CALLBACKS_INIT(&pnpPowerCallbacks);

    pnpPowerCallbacks.EvtDevicePrepareHardware = PscrEvtDevicePrepareHardware;

    pnpPowerCallbacks.EvtDeviceD0Entry = PscrEvtDeviceD0Entry;
    pnpPowerCallbacks.EvtDeviceD0Exit  = PscrEvtDeviceD0Exit;

    WdfDeviceInitSetPnpPowerEventCallbacks(DeviceInit, &pnpPowerCallbacks);

    //
    // Register close callback so that we can clear the notification irp.
    //
    WDF_FILEOBJECT_CONFIG_INIT(
                        &fileobjectConfig,
                        WDF_NO_EVENT_CALLBACK, // Create
                        WDF_NO_EVENT_CALLBACK, // Close
                        PscrEvtFileCleanup // Cleanup
                        );

    WdfDeviceInitSetFileObjectConfig(
                    DeviceInit,
                    &fileobjectConfig,
                    WDF_NO_OBJECT_ATTRIBUTES
                    );
    //
    // Initialize attributes and a context area for the device object.
    //
    //
    WDF_OBJECT_ATTRIBUTES_INIT_CONTEXT_TYPE(&attributes, DEVICE_EXTENSION);
    attributes.EvtCleanupCallback = PscrEvtDeviceContextCleanup;

    //
    // Create a framework device object.This call will in turn create
    // a WDM device object, attach to the lower stack, and set the
    // appropriate flags and attributes.
    //
    status = WdfDeviceCreate(&DeviceInit, &attributes, &device);
    if (!NT_SUCCESS(status)) {
        SmartcardLogError(
                         WdfDriverWdmGetDriverObject(WdfGetDriver()),
                         PSCR_INSUFFICIENT_RESOURCES,
                         NULL,
                         0
                         );
        return status;
    }

    //
    // Get the device context by using the accessor function specified in
    // the WDF_DECLARE_CONTEXT_TYPE_WITH_NAME macro for DEVICE_EXTENSION.
    //
    DeviceExtension = GetDeviceExtension(device);

    DeviceExtension->Device = device;

    InterlockedIncrement((PLONG)&DeviceExtension->DeviceInstanceNo);

    //
    // Add an extra stack location to the IRP disaptched to our device so
    // that we can use that to workaround the problem of scmlib completing
    // ioctl IRPs.
    //
    WdfDeviceWdmGetDeviceObject(DeviceExtension->Device)->StackSize++;

    //
    // Tell the Framework that this device will need an interface
    //
    status = WdfDeviceCreateDeviceInterface(
                 device,
                 &SmartCardReaderGuid,
                 NULL // ReferenceString
             );

    if (!NT_SUCCESS (status)) {
        SmartcardLogError(
                         WdfDriverWdmGetDriverObject(WdfGetDriver()),
                         PSCR_INSUFFICIENT_RESOURCES,
                         NULL,
                         0
                         );

        return status;
    }

    //
    // Create a parallel queue for dispatching ioctl requests.
    //

    WDF_IO_QUEUE_CONFIG_INIT(
        &ioQueueConfig,
        WdfIoQueueDispatchParallel
        );

    ioQueueConfig.EvtIoDeviceControl = PscrEvtIoDeviceControl;


    status = WdfIoQueueCreate (
                   device,
                   &ioQueueConfig,
                   WDF_NO_OBJECT_ATTRIBUTES,
                   &DeviceExtension->IoctlQueue
                   );

    if(!NT_SUCCESS (status)){
        return status;
    }

    status = WdfDeviceConfigureRequestDispatching(
                    device,
                    DeviceExtension->IoctlQueue,
                    WdfRequestTypeDeviceControl);

    if(!NT_SUCCESS (status)){
        return status;
    }

    //
    // Manual queue for parking smartcard notification request.
    //
    WDF_IO_QUEUE_CONFIG_INIT(
        &ioQueueConfig,
        WdfIoQueueDispatchManual
        );

    //
    // Since this request dispatch from this queue doesn't touch the 
    // hardware, it doesn't have to be power managed.
    //
    ioQueueConfig.PowerManaged = WdfFalse;

    //
    // Smartcard library keeps track of last outstanding notification
    // request provided to the client driver. When the client driver
    // completes the request, it has to clear the field in the data-
    // structure maintained by the library. Since we keep the request
    // in a queue, we need a notification from the framework when the 
    // request is cancelled so that we can clear the field.
    //
    ioQueueConfig.EvtIoCanceledOnQueue = PscrEvtIoCanceledOnQueue;

    status = WdfIoQueueCreate (
                   device,
                   &ioQueueConfig,
                   WDF_NO_OBJECT_ATTRIBUTES,
                   &DeviceExtension->NotificationQueue
                   );

    if(!NT_SUCCESS (status)){
        return status;
    }

    //
    // Create WDFINTERRUPT object. Framework will take care of parsing
    // the interrupt resource and connecting and disconnecting the ISR
    // at the appropriate PNP/POWER state changes.
    //

    WDF_INTERRUPT_CONFIG_INIT(&interruptConfig,
                              PscrEvtInterruptServiceRoutine,
                              PscrEvtInterruptDpc);

    status = WdfInterruptCreate(device,
                        &interruptConfig,
                        WDF_NO_OBJECT_ATTRIBUTES,
                        &DeviceExtension->Interrupt);
    if (!NT_SUCCESS (status)) {
        return status;
    }

    //
    // Register with smartcard libarary.
    //
    status = PscrRegisterWithSmcLib(DeviceExtension);
    if (!NT_SUCCESS (status)) {
        return status;
    }

    SmartcardDebug(
                  DEBUG_TRACE,
                  ( "PSCR!PscrAddDevice: Exit %x\n",
                    status)
                  );

    return status;
}

VOID
PscrEvtDeviceContextCleanup (
    WDFOBJECT       Object
    )
/*++

Routine Description:

    Called when the device object is deleted by the framework in response
    to REMOVE_DEVICE request. Here, free up all the resources that wouldn't
    cleaned up by the framework automatically due to parent child relationship.

Arguments:

    Device - Handle to a framework device object.

Return Value:

    VOID

--*/
{
    WDFDEVICE Device;
    PDEVICE_EXTENSION DeviceExtension;

    Device = (WDFDEVICE)Object;
    DeviceExtension = GetDeviceExtension(Device);

    InterlockedDecrement((PLONG)&DeviceExtension->DeviceInstanceNo);

    if (DeviceExtension->SmartcardExtension.ReaderExtension != NULL) {

        ExFreePoolWithTag(DeviceExtension->SmartcardExtension.ReaderExtension,
                            SMARTCARD_POOL_TAG);

        DeviceExtension->SmartcardExtension.ReaderExtension = NULL;
    }
}

NTSTATUS
PscrRegisterWithSmcLib(
    PDEVICE_EXTENSION DeviceExtension
    )
{
    PREADER_EXTENSION ReaderExtension;
    PSMARTCARD_EXTENSION SmartcardExtension;
    UNICODE_STRING vendorNameU, ifdTypeU;
    ANSI_STRING vendorNameA, ifdTypeA;
    WDFSTRING vendorNameStr, ifdTypeStr;
    NTSTATUS status;

    PAGED_CODE();

    vendorNameStr = NULL;
    ifdTypeStr = NULL;

    RtlZeroMemory(&vendorNameA, sizeof(vendorNameA));
    RtlZeroMemory(&ifdTypeA, sizeof(ifdTypeA));

   //   set up the device extension.
    SmartcardExtension = &DeviceExtension->SmartcardExtension;

   //   allocate the reader extension
    ReaderExtension = ExAllocatePool2(
                                    POOL_FLAG_NON_PAGED,
                                    sizeof( READER_EXTENSION ),
                                    SMARTCARD_POOL_TAG
                                    );

    if ( ReaderExtension == NULL ) {
        SmartcardLogError(
                         WdfDriverWdmGetDriverObject(WdfGetDriver()),
                         PSCR_INSUFFICIENT_RESOURCES,
                         NULL,
                         0
                         );
        status = STATUS_INSUFFICIENT_RESOURCES;
        return status;
    }

    RtlZeroMemory( ReaderExtension, sizeof( READER_EXTENSION ));

    SmartcardExtension->ReaderExtension = ReaderExtension;

    ReaderExtension->CompletionRoutine  = CBUpdateCardState;
    ReaderExtension->TrackingContext    = SmartcardExtension;

    ReaderExtension->dataRatesSupported[0]  = (ULONG) 9600;
    ReaderExtension->dataRatesSupported[1]    = (ULONG) 38400;
    ReaderExtension->dataRatesSupported[2]    = (ULONG) 57600;
    ReaderExtension->dataRatesSupported[3]    = (ULONG) 115200;

    // Initialize the RequestInterrupt flag
    ReaderExtension->RequestInterrupt = FALSE;

   //   setup smartcard extension - callback's
    SmartcardExtension->ReaderFunction[RDF_CARD_POWER] = CBCardPower;
    SmartcardExtension->ReaderFunction[RDF_TRANSMIT] = CBTransmit;
    SmartcardExtension->ReaderFunction[RDF_CARD_TRACKING] = CBCardTracking;
    SmartcardExtension->ReaderFunction[RDF_SET_PROTOCOL] = CBSetProtocol;
    SmartcardExtension->ReaderFunction[RDF_IOCTL_VENDOR] = PscrGenericIOCTL;

  // setup smartcard extension - vendor attribute
    RtlCopyMemory(
                 SmartcardExtension->VendorAttr.VendorName.Buffer,
                 PSCR_VENDOR_NAME,
                 sizeof( PSCR_VENDOR_NAME )
                 );
    SmartcardExtension->VendorAttr.VendorName.Length =
    sizeof( PSCR_VENDOR_NAME );

    RtlCopyMemory(
                 SmartcardExtension->VendorAttr.IfdType.Buffer,
                 PSCR_IFD_TYPE,
                 sizeof( PSCR_IFD_TYPE )
                 );
    SmartcardExtension->VendorAttr.IfdType.Length =
    sizeof( PSCR_IFD_TYPE );

    SmartcardExtension->VendorAttr.UnitNo = DeviceExtension->DeviceInstanceNo;

    SmartcardExtension->VendorAttr.IfdVersion.BuildNumber = 0;

   //   store firmware revision in ifd version
    SmartcardExtension->VendorAttr.IfdVersion.VersionMajor =
    ReaderExtension->FirmwareMajor;
    SmartcardExtension->VendorAttr.IfdVersion.VersionMinor =
    ReaderExtension->FirmwareMinor;
    SmartcardExtension->VendorAttr.IfdSerialNo.Length = 0;

   //   setup smartcard extension - reader capabilities
    SmartcardExtension->ReaderCapabilities.SupportedProtocols =
    SCARD_PROTOCOL_T0 | SCARD_PROTOCOL_T1;

    SmartcardExtension->ReaderCapabilities.ReaderType =
    SCARD_READER_TYPE_PCMCIA;
    SmartcardExtension->ReaderCapabilities.MechProperties = 0;
    SmartcardExtension->ReaderCapabilities.Channel = 0;

    SmartcardExtension->ReaderCapabilities.CLKFrequency.Default = 4000;
    SmartcardExtension->ReaderCapabilities.CLKFrequency.Max = 4000;

    // reader could support higher data rates
    SmartcardExtension->ReaderCapabilities.DataRatesSupported.List =
        ReaderExtension->dataRatesSupported;
    SmartcardExtension->ReaderCapabilities.DataRatesSupported.Entries = MAX_DATARATES;
    SmartcardExtension->ReaderCapabilities.DataRate.Default = ReaderExtension->dataRatesSupported[0];
    SmartcardExtension->ReaderCapabilities.DataRate.Max =
        ReaderExtension->dataRatesSupported[ MAX_DATARATES-1 ];

   //   enter correct version of the lib
    SmartcardExtension->Version = SMCLIB_VERSION;
    SmartcardExtension->SmartcardRequest.BufferSize   = MIN_BUFFER_SIZE;
    SmartcardExtension->SmartcardReply.BufferSize  = MIN_BUFFER_SIZE;

    SmartcardExtension->ReaderExtension->ReaderPowerState = PowerReaderWorking;

    status = SmartcardInitialize(SmartcardExtension);

    if (status != STATUS_SUCCESS) {

        SmartcardLogError(
                         WdfDriverWdmGetDriverObject(WdfGetDriver()),
                         PSCR_INSUFFICIENT_RESOURCES,
                         NULL,
                         0
                         );
        //
        // No need to worry about freeing the pool we allocated earlier.
        // It will be freed in the cleanup callback when the device is 
        // deleted by the framework as a result of failing this call.
        //
        return status;
    }

    SmartcardExtension->OsData->DeviceObject =
                        WdfDeviceWdmGetDeviceObject(DeviceExtension->Device);

    vendorNameStr = PscrGetRegistryValue(DeviceExtension->Device, L"VendorName");
    if (vendorNameStr != NULL) {

        WdfStringGetUnicodeString(vendorNameStr, &vendorNameU);

        status = RtlUnicodeStringToAnsiString(
                                        &vendorNameA,
                                        &vendorNameU,
                                        TRUE
                                        );
        if (NT_SUCCESS(status)) {
            if (vendorNameA.Length > 0 && vendorNameA.Length < MAXIMUM_ATTR_STRING_LENGTH) {
                RtlCopyMemory(
                             SmartcardExtension->VendorAttr.VendorName.Buffer,
                             vendorNameA.Buffer,
                             vendorNameA.Length
                             );
                SmartcardExtension->VendorAttr.VendorName.Length = vendorNameA.Length;    
            }
            RtlFreeAnsiString(&vendorNameA);
        }

        //
        // We have copied the string to the extension. No need to keep the object
        // around and waste memory.
        //
        WdfObjectDelete(vendorNameStr);
        vendorNameStr = NULL;
    } else {
        status = STATUS_UNSUCCESSFUL;
    }

    if (!NT_SUCCESS(status)) {
        return status;
    }

    ifdTypeStr = PscrGetRegistryValue(DeviceExtension->Device, L"IfdType");
    if (ifdTypeStr != NULL) {

        WdfStringGetUnicodeString(ifdTypeStr, &ifdTypeU);

        status = RtlUnicodeStringToAnsiString(
                                    &ifdTypeA,
                                    &ifdTypeU,
                                    TRUE
                                    );
        if (NT_SUCCESS(status)) {
            if (ifdTypeA.Length > 0 && ifdTypeA.Length < MAXIMUM_ATTR_STRING_LENGTH) {
                RtlCopyMemory(
                                SmartcardExtension->VendorAttr.IfdType.Buffer,
                                ifdTypeA.Buffer,
                                ifdTypeA.Length
                                );
                SmartcardExtension->VendorAttr.IfdType.Length = ifdTypeA.Length;
            }
            RtlFreeAnsiString(&ifdTypeA);
        }

        WdfObjectDelete(ifdTypeStr);
        ifdTypeStr = NULL;
    } else {
        status = STATUS_UNSUCCESSFUL;
    }

    return status;
}


NTSTATUS
PscrEvtDevicePrepareHardware (
    WDFDEVICE      Device,
    WDFCMRESLIST   Resources,
    WDFCMRESLIST   ResourcesTranslated
    )
/*++

Routine Description:

    EvtDevicePrepareHardware event callback performs operations that are necessary
    to make the driver's device operational. The framework calls the driver's
    EvtDeviceStart callback when the PnP manager sends an IRP_MN_START_DEVICE
    request to the driver stack.

Arguments:

    Device - Handle to a framework device object.

    Resources - Handle to a collection of framework resource objects.
                This collection identifies the raw (bus-relative) hardware
                resources that have been assigned to the device.

    ResourcesTranslated - Handle to a collection of framework resource objects.
                This collection identifies the translated (system-physical)
                hardware resources that have been assigned to the device.
                The resources appear from the CPU's point of view.
                Use this list of resources to map I/O space and
                device-accessible memory into virtual address space

Return Value:

    WDF status code

--*/
{
    PCM_PARTIAL_RESOURCE_DESCRIPTOR  descriptor;
    PDEVICE_EXTENSION DeviceExtension = GetDeviceExtension(Device);
    PSMARTCARD_EXTENSION SmartcardExtension = &DeviceExtension->SmartcardExtension;
    PREADER_EXTENSION ReaderExtension = SmartcardExtension->ReaderExtension;
    NTSTATUS status;
    ULONG i;

    UNREFERENCED_PARAMETER(Resources);
    UNREFERENCED_PARAMETER(ResourcesTranslated);

    PAGED_CODE();

    for (i=0; i < WdfCmResourceListGetCount(ResourcesTranslated); i++) {

        descriptor = WdfCmResourceListGetDescriptor(ResourcesTranslated, i);

        switch(descriptor->Type) {

        case CmResourceTypePort:

            KdPrint(("I/O Port: (%x) Length: (%d)\n",
                    descriptor->u.Port.Start.LowPart,
                    descriptor->u.Port.Length));

            ReaderExtension->IOBase =
                (PPSCR_REGISTERS) UlongToPtr(descriptor->u.Port.Start.LowPart);

            NT_ASSERT(descriptor->u.Port.Length >= 4);

            break;

        case CmResourceTypeMemory:

            KdPrint(("Memory: (%x) Length: (%d)\n",
                    descriptor->u.Memory.Start.LowPart,
                    descriptor->u.Memory.Length));
            break;
        case CmResourceTypeInterrupt:
            //
            // Framework will parse the interrupt resource and connect and
            // disconnect interrupt. We don't have to do anything here.
            //
            KdPrint(("Interrupt level: 0x%0x, Vector: 0x%0x, Affinity: 0x%0x\n",
                descriptor->u.Interrupt.Level,
                descriptor->u.Interrupt.Vector,
                descriptor->u.Interrupt.Affinity));

             break;

        default:
            break;
        }

    }


    if ( ReaderExtension->IOBase == NULL ) {
        SmartcardLogError(
                         WdfDriverWdmGetDriverObject(WdfGetDriver()),
                         PSCR_ERROR_IO_PORT,
                         NULL,
                         0
                         );

        status = STATUS_INSUFFICIENT_RESOURCES;
        return status;
    }


    ReaderExtension->Device    = DEVICE_ICC1;
    ReaderExtension->MaxRetries = PSCR_MAX_RETRIES;
    status = CmdResetInterface( ReaderExtension );

    SmartcardExtension->ReaderCapabilities.MaxIFSD =
                            ReaderExtension->MaxIFSD;

    if (status != STATUS_SUCCESS) {

        SmartcardLogError(
                         WdfDriverWdmGetDriverObject(WdfGetDriver()),
                         PSCR_CANT_INITIALIZE_READER,
                         NULL,
                         0
                         );

        return status;
    }

    status = CmdReset(
                     ReaderExtension,
                     0x00,          // reader
                     FALSE,            // cold reset
                     NULL,          // no atr
                     NULL
                     );

    if (status != STATUS_SUCCESS) {

        SmartcardLogError(
                         WdfDriverWdmGetDriverObject(WdfGetDriver()),
                         PSCR_CANT_INITIALIZE_READER,
                         NULL,
                         0
                         );

        return status;
    }

    PscrFlushInterface(DeviceExtension->SmartcardExtension.ReaderExtension);

    CmdGetFirmwareRevision(
                          DeviceExtension->SmartcardExtension.ReaderExtension
                          );

    // If you change the min. firmware version here, please update
    // the .mc file for the correct error message, too
    if (SmartcardExtension->ReaderExtension->FirmwareMajor < 2 ||
        SmartcardExtension->ReaderExtension->FirmwareMajor == 2 &&
        SmartcardExtension->ReaderExtension->FirmwareMinor < 0x34) {

        SmartcardLogError(
                         WdfDriverWdmGetDriverObject(WdfGetDriver()),
                         PSCR_WRONG_FIRMWARE,
                         NULL,
                         0
                         );
    }

    //  store firmware revision in ifd version
    SmartcardExtension->VendorAttr.IfdVersion.VersionMajor =
        ReaderExtension->FirmwareMajor;
    SmartcardExtension->VendorAttr.IfdVersion.VersionMinor =
        ReaderExtension->FirmwareMinor;

    // reader could support higher data rates
    SmartcardExtension->ReaderCapabilities.DataRatesSupported.List =
        ReaderExtension->dataRatesSupported;
    SmartcardExtension->ReaderCapabilities.DataRate.Default = ReaderExtension->dataRatesSupported[0];

    SmartcardExtension->ReaderCapabilities.DataRatesSupported.Entries = 2;
    SmartcardExtension->ReaderCapabilities.DataRate.Max =
        ReaderExtension->dataRatesSupported[ 1 ];

    NT_ASSERT(status == STATUS_SUCCESS);

    return status;
}

NTSTATUS
PscrEvtDeviceD0Entry(
    IN  WDFDEVICE Device,
    IN  WDF_POWER_DEVICE_STATE PreviousState
    )
/*++

Routine Description:

   EvtDeviceD0Entry event callback must perform any operations that are
   necessary before the specified device is used.  It will be called every
   time the hardware needs to be (re-)initialized.  This includes after
   IRP_MN_START_DEVICE, IRP_MN_CANCEL_STOP_DEVICE, IRP_MN_CANCEL_REMOVE_DEVICE,
   IRP_MN_SET_POWER-D0.

   This function runs at PASSIVE_LEVEL, though it is generally not paged.  A
   driver can optionally make this function pageable if DO_POWER_PAGABLE is set.

   Even if DO_POWER_PAGABLE isn't set, this function still runs at
   PASSIVE_LEVEL.  In this case, though, the function absolutely must not do
   anything that will cause a page fault.

Arguments:

    Device - Handle to a framework device object.

    PreviousState - Device power state which the device was in most recently.
        If the device is being newly started, this will be
        PowerDeviceUnspecified.

Return Value:

    NTSTATUS

--*/
{
    NTSTATUS status;
    PDEVICE_EXTENSION DeviceExtension;
    PSMARTCARD_EXTENSION SmartcardExtension;
    UCHAR state;
    WDF_INTERRUPT_INFO  interruptInfo;

    //
    //MikeTs(10/15/2008): http://msdn.microsoft.com/en-us/library/bb725862.aspx
    //  explains although the callback function is called at PASSIVE_LEVEL,
    //  callback functions that leave a lower device state returning to
    //  working state should not be pageable.
    //
    DeviceExtension = GetDeviceExtension(Device);

    //
    // Find out if the interrupt is shared.
    //
    WDF_INTERRUPT_INFO_INIT(&interruptInfo);
    WdfInterruptGetInfo(DeviceExtension->Interrupt, &interruptInfo);
    DeviceExtension->bSharedIRQ =
        (interruptInfo.ShareDisposition ==  CmResourceShareShared);

    SmartcardExtension = (PSMARTCARD_EXTENSION) &DeviceExtension->SmartcardExtension;

    status = CmdResetInterface(SmartcardExtension->ReaderExtension);
    NT_ASSERT(status == STATUS_SUCCESS);

    SmartcardExtension->ReaderExtension->StatusFileSelected = FALSE;
    state = CBGetCardState(SmartcardExtension);


    // save the current power state of the reader
    SmartcardExtension->ReaderExtension->ReaderPowerState = PowerReaderWorking;

    //
    // We will use PreviousState to differetiate whether EvtDeviceD0Entry is
    // called due to implicit power when the device is first started or when
    // the device is resume power a low power state (due to explicit power up).
    //
    if (PreviousState != WdfPowerDeviceD3Final) {
        //
        // The device is resuming from a suspended state.
        //
        CBUpdateCardState(SmartcardExtension, state, TRUE);
    } else {
        //
        // The device is starting up for the first time.
        //
        CBUpdateCardState(SmartcardExtension, state, FALSE);
    }

    return STATUS_SUCCESS;
}


NTSTATUS
PscrEvtDeviceD0Exit(
    IN  WDFDEVICE Device,
    IN  WDF_POWER_DEVICE_STATE TargetState
    )
/*++

Routine Description:

    This routine undoes anything done in EvtDeviceD0Entry.  It is called
    whenever the device leaves the D0 state, which happens when the device is
    stopped, when it is removed, and when it is powered off.

    The device is still in D0 when this callback is invoked, which means that
    the driver can still touch hardware in this routine.

    Note that interrupts have already been disabled by the time that this
    callback is invoked.

   EvtDeviceD0Exit event callback must perform any operations that are
   necessary before the specified device is moved out of the D0 state.  If the
   driver needs to save hardware state before the device is powered down, then
   that should be done here.

   This function runs at PASSIVE_LEVEL, though it is generally not paged.  A
   driver can optionally make this function pageable if DO_POWER_PAGABLE is set.

   Even if DO_POWER_PAGABLE isn't set, this function still runs at
   PASSIVE_LEVEL.  In this case, though, the function absolutely must not do
   anything that will cause a page fault.

Arguments:

    Device - Handle to a framework device object.

    TargetState - Device power state which the device will be put in once this
        callback is complete.

Return Value:

    Success implies that the device can be used.  Failure will result in the
    device stack being torn down.

--*/
{
    NTSTATUS status;
    PDEVICE_EXTENSION DeviceExtension;
    PSMARTCARD_EXTENSION smartcardExtension;

    PAGED_CODE();

    DeviceExtension = GetDeviceExtension(Device);
    smartcardExtension = (PSMARTCARD_EXTENSION) &DeviceExtension->SmartcardExtension;

    switch (TargetState) {
    case WdfPowerDeviceD1:
    case WdfPowerDeviceD2:
    case WdfPowerDeviceD3:

        smartcardExtension->ReaderExtension->CardPresent =
        smartcardExtension->ReaderCapabilities.CurrentState > SCARD_ABSENT ? TRUE: FALSE;

        if (smartcardExtension->ReaderExtension->CardPresent) {
            smartcardExtension->MinorIoControlCode = SCARD_POWER_DOWN;
            status = CBCardPower(smartcardExtension);
            NT_ASSERT(NT_SUCCESS(status));
        }
        // save the current power state of the reader
        smartcardExtension->ReaderExtension->ReaderPowerState = PowerReaderOff;
        break;

    case WdfPowerDevicePrepareForHibernation:

        //
        // Fill in any code to save hardware state here.  Do not put in any
        // code to shut the device off.  If this device cannot support being
        // in the paging path (or being a parent or grandparent of a paging
        // path device) then this whole case can be deleted.
        //
        NT_ASSERT(FALSE); // This driver shouldn't get this.
        break;

    case WdfPowerDeviceD3Final:
        //
        // Reset and put the device into a known initial state we're shutting
        // down for the last time.
        //
        break;

    default:
        break;
    }

    return STATUS_SUCCESS;
}


VOID
PscrEvtFileCleanup(
    IN WDFFILEOBJECT FileObject
    )
/*++

   EvtFileClose is called when all the handles represented by the FileObject
   is closed and all the references to FileObject is removed. This callback
   may get called in an arbitrary thread context instead of the thread that
   called CloseHandle. If you want to delete any per FileObject context that
   must be done in the context of the user thread that made the Create call,
   you should do that in the EvtDeviceCleanp callback.

Arguments:

    FileObject - Pointer to fileobject that represents the open handle.

Return Value:

   VOID

--*/

{
    PDEVICE_EXTENSION DeviceExtension;
    PSMARTCARD_EXTENSION smartcardExtension;
    NTSTATUS status;
    WDFREQUEST request;

    PAGED_CODE();

    //KdPrint(("PscrEvtFileCleanup called\n"));

    //
    // After cleanup callback is invoked, framework flushes all the queues
    // by fileobject to cancel irps belonging to the file handle being closed.
    // Since framework version 1.5 and less have a bug in that on cleanup-
    // flush the EvtIoCanceledOnQueue callback is not invoked, we will
    // retrive the request by fileobject ourselves and complete the request
    // and clear the NotificaitonIrp field.
    //
    DeviceExtension = GetDeviceExtension(WdfFileObjectGetDevice(FileObject));
    smartcardExtension = (PSMARTCARD_EXTENSION) &DeviceExtension->SmartcardExtension;

    do {
        status = WdfIoQueueRetrieveRequestByFileObject(
                                    DeviceExtension->NotificationQueue,
                                    FileObject,
                                    &request);

        if (NT_SUCCESS(status)) {

            NT_ASSERT(smartcardExtension->OsData->NotificationIrp ==
                    WdfRequestWdmGetIrp(request));

            InterlockedExchangePointer(
                                 &(smartcardExtension->OsData->NotificationIrp),
                                 NULL
                                 );

            WdfRequestComplete(request, STATUS_CANCELLED);
        }
    }while (NT_SUCCESS(status));

    NT_ASSERT(status == STATUS_NO_MORE_ENTRIES);

    return;
}

_Function_class_(IO_COMPLETION_ROUTINE)
NTSTATUS
PscrSmcLibComplete (
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN PVOID Context
    )
/*++

Routine Description:

    Completion routine is called when the IRP is completed by the smclib.
    We will interrupt the completion of the IRP, and inturn complete the
    WDFREQUEST given to us by the framework.

--*/
{
    UNREFERENCED_PARAMETER(DeviceObject);

    //KdPrint(("<-- PscrSmcLibComplete called %x Request %x\n",
    //        Irp->IoStatus.Status, Context));

    WdfRequestComplete((WDFREQUEST)Context, Irp->IoStatus.Status);

    return STATUS_MORE_PROCESSING_REQUIRED;
}


VOID
PscrEvtIoDeviceControl(
    IN WDFQUEUE    Queue,
    IN WDFREQUEST  Request,
    IN size_t      OutputBufferLength,
    IN size_t      InputBufferLength,
    IN ULONG       IoControlCode
    )
/*++

Routine Description:

    This event is called when the framework receives IRP_MJ_DEVICE_CONTROL
    requests from the system.

Arguments:

    Queue - Handle to the framework queue object that is associated
            with the I/O request.
    Request - Handle to a framework request object.

    OutputBufferLength - length of the request's output buffer,
                        if an output buffer is available.
    InputBufferLength - length of the request's input buffer,
                        if an input buffer is available.

    IoControlCode - the driver-defined or system-defined I/O control code
                    (IOCTL) that is associated with the request.
Return Value:

    VOID

--*/
{
    PDEVICE_EXTENSION deviceExtension;
    PIRP irp;

    UNREFERENCED_PARAMETER(OutputBufferLength);
    UNREFERENCED_PARAMETER(InputBufferLength);
    UNREFERENCED_PARAMETER(IoControlCode);

    //KdPrint(("--> PscrEvtIoDeviceControl called %s Request 0x%x\n",
    //            GetIoctlName(IoControlCode), Request));

    deviceExtension = GetDeviceExtension(WdfIoQueueGetDevice(Queue));

    //
    // Since smartcard library expects an IRP, we will get the underlying
    // irp.
    //
    irp = WdfRequestWdmGetIrp(Request);

    //
    // We will store the Request handle in the IRP DriverContext field so
    // that we can get the handle back in vendor-ioctl callback routine.
    //
    SET_REQUEST_IN_IRP(irp, Request);

    //
    // To workaround the problem of smartcard completing the IRP we provided.
    // we will use the extra stack location to set a completion routine so that
    // when it completes the IRP, our completion routine will be called, where
    // we can interrupt the completion and complete the actual WDFREQUEST.
    //
    IoCopyCurrentIrpStackLocationToNext(irp);

    IoSetCompletionRoutine (irp,
                           PscrSmcLibComplete,
                           Request,
                           TRUE,
                           TRUE,
                           TRUE
                           );

    IoSetNextIrpStackLocation(irp);

    //
    // Ignore the return value because framework has marked the irp pending
    // and will return status-pending when we return from this routine.
    //
    (VOID) SmartcardDeviceControl(&(deviceExtension->SmartcardExtension), irp);

    return;
}

NTSTATUS
PscrGenericIOCTL(
    PSMARTCARD_EXTENSION SmartcardExtension
    )
/*++

PscrGenericIOCTL:
   Performs generic callbacks to the reader

Arguments:
   SmartcardExtension   context of the call

Return Value:
   STATUS_SUCCESS

--*/
{
    NTSTATUS          status;
    PIRP              Irp;
    WDFREQUEST request;
    ULONG bytesTransfered;
    size_t inBufLen, outBufLen;
    PVOID inBuf, outBuf;
    PIO_STACK_LOCATION      IrpStack;

    SmartcardDebug(
                  DEBUG_TRACE,
                  ( "PSCR!PscrGenericIOCTL: Enter\n" )
                  );

    //
    // get pointer to current IRP stack location
    //
    Irp = SmartcardExtension->OsData->CurrentIrp;
    IrpStack = IoGetCurrentIrpStackLocation( Irp );

    //
    // Get the request from the IRP so that we can call framework functions
    // to retreive the buffers.
    //
    request = GET_WDFREQUEST_FROM_IRP(SmartcardExtension->OsData->CurrentIrp);

    status = WdfRequestRetrieveInputBuffer(request, 0, &inBuf, &inBufLen);
    if (!NT_SUCCESS(status)) {
        return status;
    }

    status = WdfRequestRetrieveOutputBuffer(request, 0, &outBuf, &outBufLen);
    if (!NT_SUCCESS(status)) {
        return status;
    }

    //
    // assume error
    //
    status = STATUS_INVALID_DEVICE_REQUEST;
    bytesTransfered = 0;

    //
    // dispatch IOCTL
    //
    switch ( IrpStack->Parameters.DeviceIoControl.IoControlCode ) {
    case IOCTL_PSCR_COMMAND:

        status = CmdPscrCommand(
                                 SmartcardExtension->ReaderExtension,
                                 (PUCHAR) inBuf,
                                 (ULONG) inBufLen,
                                 (PUCHAR) outBuf,
                                 (ULONG) outBufLen,
                                 (PULONG) &bytesTransfered
                                 );
         //
         // the command could change the active file in the reader file
         // system, so make sure that the status file will be selected
         // before the next read
         //
        SmartcardExtension->ReaderExtension->StatusFileSelected = FALSE;
        break;

    case IOCTL_GET_VERSIONS:

        if ( outBufLen < SIZEOF_VERSION_CONTROL ) {
            status = STATUS_BUFFER_TOO_SMALL;
        } else {
            PVERSION_CONTROL  VersionControl;

            VersionControl = (PVERSION_CONTROL)outBufLen;

            VersionControl->SmclibVersion = SmartcardExtension->Version;
            VersionControl->DriverMajor      = PSCR_MAJOR_VERSION;
            VersionControl->DriverMinor      = PSCR_MINOR_VERSION;

            // update firmware version (changed after update)
            CmdGetFirmwareRevision(
                                  SmartcardExtension->ReaderExtension
                                  );
            VersionControl->FirmwareMajor =
            SmartcardExtension->ReaderExtension->FirmwareMajor;

            VersionControl->FirmwareMinor =
            SmartcardExtension->ReaderExtension->FirmwareMinor;

            VersionControl->UpdateKey =
            SmartcardExtension->ReaderExtension->UpdateKey;

            bytesTransfered = SIZEOF_VERSION_CONTROL;
            status = STATUS_SUCCESS;
        }
        break;

    case IOCTL_SET_TIMEOUT:
        {
            ULONG NewLimit;
            //
            // get new timeout limit
            //
            if ( inBufLen == sizeof( ULONG )) {
                NewLimit = *(PULONG)inBuf;
            } else {
                NewLimit = 0;
            }

            //
            // report actual timeout limit
            //
            if (outBufLen == sizeof( ULONG )) {

                *(PULONG)outBuf = SmartcardExtension->ReaderExtension->MaxRetries
                                    * DELAY_PSCR_WAIT;
                bytesTransfered = sizeof( ULONG );
            }

            //
            // set new timeout limit
            //
            if ( (NewLimit != 0) ||
                 (NewLimit == MAXULONG-DELAY_PSCR_WAIT+2 )) {
                SmartcardExtension->ReaderExtension->MaxRetries =
                (NewLimit + DELAY_PSCR_WAIT - 1) / DELAY_PSCR_WAIT;
            }
        }
        status = STATUS_SUCCESS;
        break;

    case IOCTL_GET_CONFIGURATION:
         //
         // return IOBase and IRQ
         //
        if (outBufLen < SIZEOF_PSCR_CONFIGURATION ) {
            status = STATUS_BUFFER_TOO_SMALL;
        } else {
            PPSCR_CONFIGURATION  PSCRConfiguration;

            PSCRConfiguration = (PPSCR_CONFIGURATION) outBuf;
            PSCRConfiguration->IOBase =
                SmartcardExtension->ReaderExtension->IOBase;
            PSCRConfiguration->IRQ =
                SmartcardExtension->ReaderExtension->CurrentIRQ;

            bytesTransfered = SIZEOF_PSCR_CONFIGURATION;
            status = STATUS_SUCCESS;
        }
        break;

    default:
        break;
    }

    //
    // set status of the packet
    //
    Irp->IoStatus.Status = status;
    Irp->IoStatus.Information = bytesTransfered;

    //
    // The request will be completed the smartcard library.
    //
    SmartcardDebug(
                  DEBUG_TRACE,
                  ( "PSCR!PscrGenericIOCTL: Exit\n" )
                  );

    return( status );
}


VOID
PscrEvtIoCanceledOnQueue(
    IN WDFQUEUE  Queue,
    IN WDFREQUEST  Request
    )
/*++

PscrEvtIoCanceledOnQueue:

  This routine is called when the request is cancelled on the queue.

  We will use the notificatio to clear the NotificationIrp field set by
  the smartcard library.

Arguments:

Return Value:
   VOID

--*/
{
    PDEVICE_EXTENSION DeviceExtension;
    PSMARTCARD_EXTENSION smartcardExtension;

    DeviceExtension = GetDeviceExtension(WdfIoQueueGetDevice(Queue));
    smartcardExtension = (PSMARTCARD_EXTENSION) &DeviceExtension->SmartcardExtension;

    //KdPrint(("Cancelled on queue 0x%x\n", WdfRequestWdmGetIrp(Request)));

    InterlockedExchangePointer(
                             &(smartcardExtension->OsData->NotificationIrp),
                             NULL
                             );

    WdfRequestComplete(Request, STATUS_CANCELLED);
}

BOOLEAN
PscrEvtInterruptServiceRoutine(
    IN WDFINTERRUPT Interrupt,
    IN ULONG        MessageID
    )
/*++

PscrIrqServiceRoutine:

   Because the device not supports shared interrupts, the call is passed
   to the DPC routine immediately and the IRQ is reported as served.

Arguments:

Return Value:
   STATUS_SUCCESS

--*/
{
    LARGE_INTEGER   Time;
    TIME_FIELDS     TimeFields;
    TIME_FIELDS     SaveTimeFields;
    BOOLEAN         Status = TRUE;
    PDEVICE_EXTENSION DeviceExtension;

    UNREFERENCED_PARAMETER(MessageID);

    DeviceExtension= GetDeviceExtension(WdfInterruptGetDevice(Interrupt));

    KeQuerySystemTime( &Time );
    RtlTimeToTimeFields( &Time, &TimeFields );
    RtlTimeToTimeFields( &DeviceExtension->SaveTime, &SaveTimeFields );

    SmartcardDebug(
                  DEBUG_TRACE,
                  ("PSCR!PscrIrqServiceRoutine: Enter\n")
                  );

    // if the interrupt is shared the PscrIrqServiceRoutine shall return always FALSE
    // since te driver can't detect if the interrupt was sending from the reader when
    // the firmware <2.50 is using
    // if the interrupt is used exclusiv and the PscrIrqServiceRoutine returns FALSE,
    // no more interrupts will be sended
    if(( DeviceExtension->bSharedIRQ ) &&
        ( TimeFields.Minute == SaveTimeFields.Minute ) &&
        ( TimeFields.Second == SaveTimeFields.Second ) &&
        ( TimeFields.Milliseconds < SaveTimeFields.Milliseconds+50 ))
    {
        KdPrint(( "****** Status FALSE ******\n" ));
        SysDelay( 5 );
        Status = FALSE;
    }
    else
    {
        //
        // When someone yanks out the card the interrupt handler gets called,
        // but since there is no card anymore when don't need to schedule a DPC
        //

        //
        // the interrupt is caused by a freeze event. the interface will be
        // cleared either by PscrRead() or the DPC routine (depending on
        // which is called first)
        //
        // DeviceExtension->SmartcardExtension.ReaderExtension->FreezePending = TRUE;

        InterlockedIncrement((PLONG)&DeviceExtension->PendingInterrupts);

        WdfInterruptQueueDpcForIsr( Interrupt );

        if( DeviceExtension->bSharedIRQ ) {
            Status = FALSE;
        }
    }

    DeviceExtension->SaveTime.QuadPart = Time.QuadPart;

    SmartcardDebug(
                  DEBUG_TRACE,
                  ( "PSCR!PscrIrqServiceRoutine: Exit\n" )
                  );

    return Status ;
}

VOID
PscrEvtInterruptDpc(
    IN WDFINTERRUPT Interrupt,
    IN WDFOBJECT    Device
    )
/*++

PscrDpcRoutine:
   finishes interrupt requests. the freeze event data of the reader will be read
   & the card state will be updated if data valid

Arguments:

Return Value:
   void

--*/
{
    PDEVICE_EXTENSION DeviceExtension;
    PSMARTCARD_EXTENSION SmartcardExtension;

    UNREFERENCED_PARAMETER(Interrupt);

    DeviceExtension= GetDeviceExtension(Device);
    SmartcardExtension = (PSMARTCARD_EXTENSION) &DeviceExtension->SmartcardExtension;
    SmartcardDebug(
                  DEBUG_TRACE,
                  ( "PSCR!PscrInterruptEvent: IoBase %p\n",
                    SmartcardExtension->ReaderExtension->IOBase)
                  );

    //
    // In case of a card change the reader provides a TLV packet describing
    // the event ('freeze event'). If PscrRead was called before the DPC
    // routine is called, this event was cleared; in this case the card state
    // will be updated by reading the card status file
    //
    do {


        NT_ASSERT(DeviceExtension->PendingInterrupts < 10);

        SmartcardDebug(
                      DEBUG_TRACE,
                      ( "PSCR!PscrInterruptEvent: PendingInterrupts = %ld\n",
                        DeviceExtension->PendingInterrupts)
                      );

        if( !SmartcardExtension->ReaderExtension->RequestPending )
        {
            PscrFreeze( SmartcardExtension );
        }
        else
        {
            // sometimes the fw doesn't send the freeze data
            SmartcardExtension->ReaderExtension->RequestInterrupt = TRUE;
        }

    } while (InterlockedDecrement((PLONG)&DeviceExtension->PendingInterrupts) > 0);
}

void
PscrFreeze(
          PSMARTCARD_EXTENSION SmartcardExtension
          )

/*++
PscrFreeze:
   Read & evaluate freeze data

Arguments:
   ReaderExtension   context of call
   pDevice        device which causes the freeze event

Return Value:
   STATUS_SUCCESS
   STATUS_UNSUCCESSFUL

--*/
{
    NTSTATUS NTStatus = STATUS_UNSUCCESSFUL;
    UCHAR TLVList[9];
    ULONG NBytes;
    ULONG Retries;
    UCHAR ReadFreeze[] = { 0x12, 0x00, 0x05, 0x00, 0xB0, 0x00, 0x00, 0x01, 0xA6};

    for (Retries = 0; Retries < 2; Retries++)    //    CB - code sync - 7/13/2006
    {

        PscrWriteDirect(
            SmartcardExtension->ReaderExtension,
            (PUCHAR)ReadFreeze,
            sizeof( ReadFreeze ),
            &NBytes
            );

        SysDelay( 20 );

        NTStatus = PscrRead(
            SmartcardExtension->ReaderExtension,
            (PUCHAR) TLVList,
            sizeof( TLVList ),
            &NBytes
            );

        if( NT_SUCCESS( NTStatus ))
        {
            //
            //  get result
            //
            if( ( TLVList[ PSCR_NAD ] == 0x21 ) &&
                ( TLVList[ PSCR_INF ] == TAG_FREEZE_EVENTS ))
            {
                break;
            }
        }
    }
}

WDFSTRING
PscrGetRegistryValue(
    _In_  WDFDEVICE   Device,
    _In_  LPCWSTR     Name
    )
/*++

Routine Description:

    Get the registry values from the device parameters key.

Arguments:


Return Value:

--*/
{
    WDFKEY      hKey = NULL;
    NTSTATUS    status;
    UNICODE_STRING  valueName;
    WDFSTRING string = NULL;

    PAGED_CODE();

    status = WdfDeviceOpenRegistryKey(Device,
                                      PLUGPLAY_REGKEY_DEVICE,
                                      STANDARD_RIGHTS_ALL,
                                      WDF_NO_OBJECT_ATTRIBUTES,
                                      &hKey);
    if (NT_SUCCESS (status)) {

        status = WdfStringCreate(
                         NULL,
                         WDF_NO_OBJECT_ATTRIBUTES,
                         &string
                         );
        if (NT_SUCCESS (status)) {
            RtlInitUnicodeString(&valueName, Name);

            status = WdfRegistryQueryString(hKey, &valueName, string);
            if (!NT_SUCCESS (status)) {
                WdfObjectDelete(string);
                string = NULL;
            }
        }

        WdfRegistryClose(hKey);
    }

    return string;
}

void
SysDelay(
        ULONG Timeout
        )
/*++

SysDelay:
   performs a required delay. The usage of KeStallExecutionProcessor is
   very nasty, but it happends only if SysDelay is called in the context of
   our DPC routine (which is only called if a card change was detected).

   For 'normal' IO we have Irql < DISPATCH_LEVEL, so if the reader is polled
   while waiting for response we will not block the entire system

Arguments:
   Timeout     delay in milli seconds

Return Value:
   void

--*/
{
    LARGE_INTEGER  SysTimeout;

    if ( KeGetCurrentIrql() >= DISPATCH_LEVEL ) {
        ULONG Cnt = 20 * Timeout;

        while ( Cnt-- ) {
         // KeStallExecutionProcessor: counted in us
            KeStallExecutionProcessor( 50 );
        }
    } else {
        SysTimeout.QuadPart = (LONGLONG)-10 * 1000 * Timeout;

      // KeDelayExecutionThread: counted in 100 ns
        KeDelayExecutionThread( KernelMode, FALSE, &SysTimeout );
    }
    return;
}

PCHAR
GetIoctlName(
    ULONG IoControlCode
    )
{
    switch (IoControlCode) {
    case IOCTL_SMARTCARD_POWER: return "IOCTL_SMARTCARD_POWER";
    case IOCTL_SMARTCARD_GET_ATTRIBUTE: return "IOCTL_SMARTCARD_GET_ATTRIBUTE";
    case IOCTL_SMARTCARD_SET_ATTRIBUTE: return "IOCTL_SMARTCARD_SET_ATTRIBUTE";
    case IOCTL_SMARTCARD_CONFISCATE: return "IOCTL_SMARTCARD_CONFISCATE";
    case IOCTL_SMARTCARD_TRANSMIT: return "IOCTL_SMARTCARD_TRANSMIT";
    case IOCTL_SMARTCARD_EJECT: return "IOCTL_SMARTCARD_EJECT";
    case IOCTL_SMARTCARD_SWALLOW: return "IOCTL_SMARTCARD_SWALLOW";
    case IOCTL_SMARTCARD_IS_PRESENT: return "IOCTL_SMARTCARD_IS_PRESENT";
    case IOCTL_SMARTCARD_IS_ABSENT: return "IOCTL_SMARTCARD_IS_ABSENT";
    case IOCTL_SMARTCARD_SET_PROTOCOL: return "IOCTL_SMARTCARD_SET_PROTOCOL";
    case IOCTL_SMARTCARD_GET_STATE: return "IOCTL_SMARTCARD_GET_STATE";
    case IOCTL_SMARTCARD_GET_LAST_ERROR: return "IOCTL_SMARTCARD_GET_LAST_ERROR";
    default: return "Unknow_ioctl";
    }
}
