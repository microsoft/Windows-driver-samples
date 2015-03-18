/*++

Copyright (c) Microsoft Corporation.  All rights reserved.

Module Name:

    Mars.c

Abstract:
Environment:

    kernel

Notes:


Revision History:


--*/
#include "mars.h"

#ifdef ALLOC_PRAGMA
    #pragma alloc_text (INIT, DriverEntry)
    #pragma alloc_text (PAGE, MarsEvtDeviceAdd)
    #pragma alloc_text (PAGE, MarsEvtIoDeviceControl)
#endif


BOOLEAN NoisyMode = FALSE;


NTSTATUS
DriverEntry(
           IN PDRIVER_OBJECT  DriverObject,
           IN PUNICODE_STRING RegistryPath
           )
/*++

Routine Description:

    Installable driver initialization entry point.
    This entry point is called directly by the I/O system.

Arguments:

    DriverObject - pointer to the driver object

    RegistryPath - pointer to a unicode string representing the path,
                   to driver-specific key in the registry.

Return Value:

    STATUS_SUCCESS if successful,
    STATUS_UNSUCCESSFUL otherwise.

--*/
{

    NTSTATUS            status = STATUS_SUCCESS;
    WDF_DRIVER_CONFIG    config;

    WDF_DRIVER_CONFIG_INIT(&config,
                           MarsEvtDeviceAdd
                          );

    status = WdfDriverCreate(DriverObject,
                             RegistryPath,
                             WDF_NO_OBJECT_ATTRIBUTES,
                             &config,
                             WDF_NO_HANDLE
                            );

    return status;
}


NTSTATUS
MarsEvtDeviceAdd(
                IN WDFDRIVER Driver,
                IN PWDFDEVICE_INIT DeviceInit
                )
/*++
Routine Description:

    EvtDeviceAdd is called by the framework in response to AddDevice
    call from the PnP manager.

Arguments:

    Driver - Handle to a framework driver object created in DriverEntry

    DeviceInit - Pointer to a framework-allocated WDFDEVICE_INIT structure.

Return Value:

    NTSTATUS

--*/
{
    NTSTATUS                status = STATUS_SUCCESS;
    PFDO_DATA                fdoData;
    WDF_IO_QUEUE_CONFIG        queueConfig;
    WDF_OBJECT_ATTRIBUTES    fdoAttributes;
    WDFDEVICE                device;
    WDF_PNPPOWER_EVENT_CALLBACKS          pnpPowerCallbacks;
    SDBUS_INTERFACE_PARAMETERS interfaceParameters = {0};

    UNREFERENCED_PARAMETER(Driver);

    PAGED_CODE();

    WdfDeviceInitSetPowerPageable(DeviceInit);

    WdfDeviceInitSetIoType(DeviceInit, WdfDeviceIoBuffered);

    WDF_PNPPOWER_EVENT_CALLBACKS_INIT(&pnpPowerCallbacks);

    //
    // Register PNP callbacks.
    //
    pnpPowerCallbacks.EvtDevicePrepareHardware = MarsEvtDevicePrepareHardware;
    pnpPowerCallbacks.EvtDeviceReleaseHardware = MarsEvtDeviceReleaseHardware;

    WdfDeviceInitSetPnpPowerEventCallbacks(DeviceInit, &pnpPowerCallbacks);

    WDF_OBJECT_ATTRIBUTES_INIT(&fdoAttributes);
    WDF_OBJECT_ATTRIBUTES_SET_CONTEXT_TYPE(&fdoAttributes, FDO_DATA);

    status = WdfDeviceCreate(&DeviceInit, &fdoAttributes, &device);
    if (!NT_SUCCESS(status)) {
        return status;
    }

    fdoData = MarsFdoGetData(device); //Gets device context

    //
    // Open an interface to the SD bus driver
    //
    status = SdBusOpenInterface(WdfDeviceWdmGetPhysicalDevice (device),
                                &fdoData->BusInterface,
                                sizeof(SDBUS_INTERFACE_STANDARD),
                                SDBUS_INTERFACE_VERSION);

    if (!NT_SUCCESS(status)) {
        return status;
    }

    interfaceParameters.Size                        = sizeof(SDBUS_INTERFACE_PARAMETERS);
    interfaceParameters.TargetObject                = WdfDeviceWdmGetAttachedDevice(device);
    interfaceParameters.DeviceGeneratesInterrupts   = TRUE;    //change to true eventually
    interfaceParameters.CallbackRoutine             = MarsEventCallback;
    interfaceParameters.CallbackRoutineContext      = fdoData;

    status = STATUS_UNSUCCESSFUL;
    if (fdoData->BusInterface.InitializeInterface) {
        status = (fdoData->BusInterface.InitializeInterface)(fdoData->BusInterface.Context,
                                                             &interfaceParameters);
    }

    if (!NT_SUCCESS(status)) {
        return status;
    }

    //
    //    Register New device
    //

    status = WdfDeviceCreateDeviceInterface(device,
                                            (LPGUID) &GUID_DEVINTERFACE_MARS,
                                            NULL
                                           );
    if (!NT_SUCCESS(status)) {
        return status;
    }

    fdoData->WdfDevice = device;

    WDF_IO_QUEUE_CONFIG_INIT_DEFAULT_QUEUE(&queueConfig,  WdfIoQueueDispatchSequential);

    queueConfig.EvtIoRead = MarsEvtIoRead;
    queueConfig.EvtIoWrite = MarsEvtIoWrite;
    queueConfig.EvtIoDeviceControl = MarsEvtIoDeviceControl;

    status = WdfIoQueueCreate(device,
                              &queueConfig,
                              WDF_NO_OBJECT_ATTRIBUTES,
                              &fdoData->IoctlQueue
                             );

    return status;
}

NTSTATUS
MarsEvtDevicePrepareHardware(
                            WDFDEVICE Device,
                            WDFCMRESLIST Resources,
                            WDFCMRESLIST ResourcesTranslated
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
    NTSTATUS                status = STATUS_SUCCESS;
    USHORT maxBlockLength;
    USHORT hostBlockLength;
    PFDO_DATA fdoData;

    UNREFERENCED_PARAMETER(Resources);
    UNREFERENCED_PARAMETER(ResourcesTranslated);

    fdoData = MarsFdoGetData(Device);

    //
    // Get the function number
    //

    status = SdioGetProperty(Device,
                             SDP_FUNCTION_NUMBER,
                             &fdoData->FunctionNumber,
                             sizeof(fdoData->FunctionNumber));
    
    if (!NT_SUCCESS(status)) {
        return status;
    }

    fdoData->FunctionFocus = fdoData->FunctionNumber;
    

    //
    // Get the SD bus driver version
    //

    fdoData->DriverVersion = SDBUS_DRIVER_VERSION_1;

    SdioGetProperty(Device,
                    SDP_BUS_DRIVER_VERSION,
                    &fdoData->DriverVersion,
                    sizeof(fdoData->DriverVersion));

    if (fdoData->DriverVersion < SDBUS_DRIVER_VERSION_2) {
        fdoData->BlockMode = 0;
    } else {
        fdoData->BlockMode = 1;
    }


    //
    // Get the host buffer block size
    //

    status = SdioGetProperty(Device,
                             SDP_HOST_BLOCK_LENGTH,
                             &hostBlockLength,
                             sizeof(hostBlockLength));

    if (!NT_SUCCESS(status)) {
        return status;
    }

    maxBlockLength = 128;  // just a value for testing

    if (hostBlockLength < maxBlockLength) {
        maxBlockLength = hostBlockLength;

    }

    //
    // set the block count since the MARS board may not have any
    // tuples in its FLASH
    //

    status = SdioSetProperty(Device,
                             SDP_FUNCTION_BLOCK_LENGTH,
                             &maxBlockLength,
                             sizeof(maxBlockLength));

    return status;
}


NTSTATUS
MarsEvtDeviceReleaseHardware(
                            IN  WDFDEVICE Device,
                            IN  WDFCMRESLIST ResourcesTranslated
                            )
/*++

Routine Description:

    EvtDeviceReleaseHardware is called by the framework whenever the PnP manager
    is revoking ownership of our resources.  This may be in response to either
    IRP_MN_STOP_DEVICE or IRP_MN_REMOVE_DEVICE.  The callback is made before
    passing down the IRP to the lower driver.

    In this callback, do anything necessary to free those resources.

Arguments:

    Device - Handle to a framework device object.

    ResourcesTranslated - Handle to a collection of framework resource objects.
                This collection identifies the translated (system-physical)
                hardware resources that have been assigned to the device.
                The resources appear from the CPU's point of view.
                Use this list of resources to map I/O space and
                device-accessible memory into virtual address space

Return Value:

    NTSTATUS - Failures will be logged, but not acted on.

--*/
{
    PFDO_DATA   fdoData;

    UNREFERENCED_PARAMETER(ResourcesTranslated);

    fdoData = MarsFdoGetData(Device);

    if (fdoData->BusInterface.InterfaceDereference) {

        (fdoData->BusInterface.InterfaceDereference)(fdoData->BusInterface.Context);
        fdoData->BusInterface.InterfaceDereference = NULL;

    }

    return STATUS_SUCCESS;
}

//
// Io events callbacks.
//
VOID
MarsEvtIoDeviceControl(
                      IN WDFQUEUE Queue,
                      IN WDFREQUEST Request,
                      IN size_t OutputBufferLength,
                      IN size_t InputBufferLength,
                      IN ULONG IoControlCode
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
    PFDO_DATA               fdoData = NULL;
    WDFDEVICE               device;
    NTSTATUS                status = STATUS_SUCCESS;
    PVOID                   inBuffer = NULL, outBuffer = NULL;
    size_t                  bytesReturned = 0;
    size_t                  size = 0;

    PAGED_CODE();

    //DbgPrint(("Started MarsEvtIODeviceControl\n"));

    device = WdfIoQueueGetDevice(Queue);
    fdoData = MarsFdoGetData(device);            //Gets device context

    //
    // Get the ioctl input & output buffers
    //
    if (InputBufferLength != 0) {
        status = WdfRequestRetrieveInputBuffer(Request,
                                               InputBufferLength,
                                               &inBuffer,
                                               &size);

        if (!NT_SUCCESS(status)) {
            WdfRequestComplete(Request, status);
            return;
        }
    }

    if (OutputBufferLength != 0) {
        status = WdfRequestRetrieveOutputBuffer(Request,
                                                OutputBufferLength,
                                                &outBuffer,
                                                &size);
        if (!NT_SUCCESS(status)) {
            WdfRequestComplete(Request, status);
            return;
        }
    }

    switch (IoControlCode) {
    
    case IOCTL_MARS_GET_DRIVER_VERSION:

        if (OutputBufferLength < sizeof(USHORT)) {
            status = STATUS_BUFFER_TOO_SMALL;
            break;
        }

        *(PUSHORT)outBuffer = fdoData->DriverVersion;
        bytesReturned = sizeof(USHORT);
        break;


    case IOCTL_MARS_GET_FUNCTION_NUMBER:

        if (OutputBufferLength < sizeof(UCHAR)) {
            status = STATUS_BUFFER_TOO_SMALL;
            break;
        }

        *(PUCHAR)outBuffer = fdoData->FunctionNumber;
        bytesReturned  = sizeof(UCHAR);
        break;


    case IOCTL_MARS_GET_FUNCTION_FOCUS:

        if (OutputBufferLength < sizeof(UCHAR)) {
            status = STATUS_BUFFER_TOO_SMALL;
            break;
        }

        *(PUCHAR)outBuffer = fdoData->FunctionFocus;
        bytesReturned  = sizeof(UCHAR);
        break;


    case IOCTL_MARS_SET_FUNCTION_FOCUS:

        if (InputBufferLength < sizeof(UCHAR)) {
            status = STATUS_BUFFER_TOO_SMALL;
            break;
        }

        fdoData->FunctionFocus = *(PUCHAR)inBuffer;
        break;


        //---------------------------------------------------
        //
        // SDP_BUS_WIDTH
        //
        //---------------------------------------------------

    case IOCTL_MARS_GET_BUS_WIDTH:

        if (OutputBufferLength < sizeof(UCHAR)) {
            status = STATUS_BUFFER_TOO_SMALL;
            break;
        }

        status = SdioGetProperty(device, SDP_BUS_WIDTH,
                                 outBuffer, sizeof(UCHAR));

        if (NT_SUCCESS(status)) {
            bytesReturned  = sizeof(UCHAR);
        }
        break;


    case IOCTL_MARS_SET_BUS_WIDTH:

        if (InputBufferLength < sizeof(UCHAR)) {
            status = STATUS_BUFFER_TOO_SMALL;
            break;
        }

        status = SdioSetProperty(device, SDP_BUS_WIDTH,
                                 inBuffer, sizeof(UCHAR));
        break;


        //---------------------------------------------------
        //
        // SDP_BUS_CLOCK
        //
        //---------------------------------------------------

    case IOCTL_MARS_GET_BUS_CLOCK:

        if (OutputBufferLength < sizeof(ULONG)) {
            status = STATUS_BUFFER_TOO_SMALL;
            break;
        }

        status = SdioGetProperty(device, SDP_BUS_CLOCK,
                                 outBuffer, sizeof(ULONG));

        if (NT_SUCCESS(status)) {
            bytesReturned  = sizeof(ULONG);
        }
        break;


    case IOCTL_MARS_SET_BUS_CLOCK:

        if (InputBufferLength < sizeof(ULONG)) {
            status = STATUS_BUFFER_TOO_SMALL;
            break;
        }

        status = SdioSetProperty(device, SDP_BUS_CLOCK,
                                 inBuffer, sizeof(ULONG));
        break;


        //---------------------------------------------------
        //
        // SDP_FUNCTION_BLOCK_LENGTH
        //
        //---------------------------------------------------


    case IOCTL_MARS_GET_BLOCKLEN:

        if (OutputBufferLength < sizeof(USHORT)) {
            status = STATUS_BUFFER_TOO_SMALL;
            break;
        }
        status = SdioGetProperty(device, SDP_FUNCTION_BLOCK_LENGTH,
                                 outBuffer, sizeof(SHORT));

        if (NT_SUCCESS(status)) {
            bytesReturned  = sizeof(USHORT);
        }
        break;


    case IOCTL_MARS_SET_BLOCKLEN:

        if (InputBufferLength < sizeof(USHORT)) {
            status = STATUS_BUFFER_TOO_SMALL;
            break;
        }
        status = SdioSetProperty(device, SDP_FUNCTION_BLOCK_LENGTH,
                                 inBuffer, sizeof(SHORT));
        break;


        //---------------------------------------------------
        //
        // SDP_FN0_BLOCK_LENGTH
        //
        //---------------------------------------------------


    case IOCTL_MARS_GET_FN0_BLOCKLEN:

        if (OutputBufferLength < sizeof(USHORT)) {
            status = STATUS_BUFFER_TOO_SMALL;
            break;
        }
        status = SdioGetProperty(device, SDP_FN0_BLOCK_LENGTH,
                                 outBuffer, sizeof(SHORT));

        if (NT_SUCCESS(status)) {
            bytesReturned  = sizeof(USHORT);
        }
        break;


    case IOCTL_MARS_SET_FN0_BLOCKLEN:

        if (InputBufferLength < sizeof(USHORT)) {
            status = STATUS_BUFFER_TOO_SMALL;
            break;
        }
        status = SdioSetProperty(device, SDP_FN0_BLOCK_LENGTH,
                                 inBuffer, sizeof(SHORT));
        break;


        //---------------------------------------------------
        //
        // SDP_BUS_INTERFACE_CONTROL
        //
        //---------------------------------------------------


    case IOCTL_MARS_GET_BUS_INTERFACE_CONTROL:

        if (OutputBufferLength < sizeof(UCHAR)) {
            status = STATUS_BUFFER_TOO_SMALL;
            break;
        }
        status = SdioGetProperty(device, SDP_BUS_INTERFACE_CONTROL,
                                 outBuffer, sizeof(UCHAR));

        if (NT_SUCCESS(status)) {
            bytesReturned  = sizeof(UCHAR);
        }
        break;


    case IOCTL_MARS_SET_BUS_INTERFACE_CONTROL:

        if (InputBufferLength < sizeof(UCHAR)) {
            status = STATUS_BUFFER_TOO_SMALL;
            break;
        }
        status = SdioSetProperty(device, SDP_BUS_INTERFACE_CONTROL,
                                 inBuffer, sizeof(UCHAR));
        break;


        //---------------------------------------------------
        //
        // SDP_FUNCTION_INT_ENABLE
        //
        //---------------------------------------------------


    case IOCTL_MARS_GET_INT_ENABLE:

        if (OutputBufferLength < sizeof(UCHAR)) {
            status = STATUS_BUFFER_TOO_SMALL;
            break;
        }
        status = SdioGetProperty(device, SDP_FUNCTION_INT_ENABLE,
                                 outBuffer, sizeof(UCHAR));

        if (NT_SUCCESS(status)) {
            bytesReturned  = sizeof(UCHAR);
        }
        break;


    case IOCTL_MARS_SET_INT_ENABLE:

        if (InputBufferLength < sizeof(UCHAR)) {
            status = STATUS_BUFFER_TOO_SMALL;
            break;
        }
        status = SdioSetProperty(device, SDP_FUNCTION_INT_ENABLE,
                                 inBuffer, sizeof(UCHAR));
        break;


        //---------------------------------------------------
        //
        // READ/WRITE BYTE
        //
        //---------------------------------------------------


    case IOCTL_MARS_READ_BYTE:

        if ((InputBufferLength < sizeof(ULONG)) || (OutputBufferLength < sizeof(UCHAR))) {
            status = STATUS_BUFFER_TOO_SMALL;
            break;
        }

        status = SdioReadWriteByte(device,
                                   fdoData->FunctionFocus,
                                   (PUCHAR)outBuffer,
                                   *(PULONG)inBuffer,
                                   FALSE);

        if (NT_SUCCESS(status)) {
            bytesReturned  = sizeof(UCHAR);
        }

        break;


    case IOCTL_MARS_WRITE_BYTE:

        if ((InputBufferLength < sizeof(ULONG)*2)) {//||(OutputBufferLength < sizeof(UCHAR))) {
            status = STATUS_BUFFER_TOO_SMALL;
            break;
        }

        // BUGBUG: check for output buffer length

        status = SdioReadWriteByte(device,
                                   fdoData->FunctionFocus,
                                   (PUCHAR)(&((PULONG)inBuffer)[1]),
                                   *(PULONG)inBuffer,
                                   TRUE);

        // BUGBUG: Return the right size

        if (NT_SUCCESS(status)) {
            bytesReturned  = sizeof(UCHAR);
        }


        break;


        //---------------------------------------------------
        //
        // Mode settings
        //
        //---------------------------------------------------

    case IOCTL_MARS_SET_TRANSFER_MODE:

        bytesReturned  = 0;

        break;

    case IOCTL_MARS_TOGGLE_MODE:

        if (OutputBufferLength < sizeof(UCHAR)) {
            status = STATUS_BUFFER_TOO_SMALL;
            break;
        }

        if (fdoData->DriverVersion < SDBUS_DRIVER_VERSION_2) {
            status = STATUS_INVALID_DEVICE_REQUEST;
            break;
        }

        fdoData->BlockMode = fdoData->BlockMode ? 0 : 1;
        *(PUCHAR)outBuffer = fdoData->BlockMode;
        bytesReturned  = sizeof(UCHAR);
        break;


    case IOCTL_MARS_TOGGLE_NOISY:

        if (OutputBufferLength < sizeof(BOOLEAN)) {
            status = STATUS_BUFFER_TOO_SMALL;
            break;
        }

        NoisyMode = NoisyMode ? 0 : 1;
        *(PBOOLEAN)outBuffer = NoisyMode;
        bytesReturned  = sizeof(BOOLEAN);

        if (NoisyMode) {
            KdPrintEx((DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "MARS: Noisy mode\n"));
        } else {
            KdPrintEx((DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "MARS: Quiet mode\n"));
        }
        break;


    default:
        NT_ASSERTMSG("Invalid IOCTL request\n", FALSE);

        status = STATUS_INVALID_DEVICE_REQUEST;
    }

    WdfRequestCompleteWithInformation(Request, status, bytesReturned);
    return;
}

VOID
MarsEvtIoRead (
              WDFQUEUE      Queue,
              WDFREQUEST    Request,
              size_t         Length
              )
/*++

Routine Description:

    This event is called when the framework receives IRP_MJ_READ requests.

Arguments:

    Queue -  Handle to the framework queue object that is associated with the
            I/O request.
    Request - Handle to a framework request object.

    Lenght - Length of the data buffer associated with the request.
             The default property of the queue is to not dispatch
             zero lenght read & write requests to the driver and
             complete is with status success. So we will never get
             a zero length request.

Return Value:

  None.

--*/
{
    WDFDEVICE        device;
    PFDO_DATA        fdoData;
    PMDL            mdlAddress;
    WDF_REQUEST_PARAMETERS  parameters;
    NTSTATUS        status;
    ULONG            bytesRead;

    device = WdfIoQueueGetDevice(Queue);
    fdoData = MarsFdoGetData(device);

    status = WdfRequestRetrieveOutputWdmMdl(Request,&mdlAddress);
    if (!NT_SUCCESS(status)) {
        WdfRequestComplete(Request, status);
        return;
    }

    WDF_REQUEST_PARAMETERS_INIT(&parameters);
    WdfRequestGetParameters(Request, &parameters);

    status = SdioReadWriteBuffer(device,
                                 fdoData->FunctionFocus,
                                 mdlAddress,
                                 (ULONG)parameters.Parameters.Read.DeviceOffset,
                                 (ULONG)parameters.Parameters.Read.Length,
                                 FALSE,
                                 &bytesRead);

    WdfRequestCompleteWithInformation(Request, status, bytesRead);

    return;
}

VOID
MarsEvtIoWrite (
               WDFQUEUE      Queue,
               WDFREQUEST    Request,
               size_t         Length
               )
/*++

Routine Description:

    Called by the framework as soon as it receive a write IRP.
    If the device is not ready, fail the request. Otherwise
    get scatter-gather list for this request and send the
    packet to the hardware for DMA.

Arguments:

    Queue - Handle to the framework queue object that is associated
            with the I/O request.
    Request - Handle to a framework request object.

    Length - Length of the IO operation
                 The default property of the queue is to not dispatch
                 zero lenght read & write requests to the driver and
                 complete is with status success. So we will never get
                 a zero length request.

Return Value:


--*/
{
    WDFDEVICE        device;
    PFDO_DATA        fdoData;
    PMDL            mdlAddress;
    WDF_REQUEST_PARAMETERS  parameters;
    NTSTATUS        status;
    ULONG            bytesRead;

    device = WdfIoQueueGetDevice(Queue);
    fdoData = MarsFdoGetData(device);

    status = WdfRequestRetrieveInputWdmMdl(Request,&mdlAddress);
    if (!NT_SUCCESS(status)) {
        WdfRequestComplete(Request, status);
        return;
    }

    WDF_REQUEST_PARAMETERS_INIT(&parameters);
    WdfRequestGetParameters(Request, &parameters);

    status = SdioReadWriteBuffer(device,
                                 fdoData->FunctionFocus,
                                 mdlAddress,
                                 (ULONG)parameters.Parameters.Read.DeviceOffset,
                                 (ULONG)parameters.Parameters.Read.Length,
                                 TRUE,
                                 &bytesRead);

    WdfRequestCompleteWithInformation(Request, status, bytesRead);
    return;
}

VOID
MarsEventCallback(
   IN PVOID Context,
   IN ULONG InterruptType
   )
/*++

Routine Description:

    This routine is called by the SD bus driver when a card interrupt is
    detected. It will launch the EventWorker routine which reads the data from
    the card.

Arguments:

    Context, interrupt type are defined in the SDBUS api

Return Values:

    none

--*/
{
    PFDO_DATA fdoData = (PFDO_DATA) Context;
    static ULONG intCount = 0;

    if (NoisyMode) {
        KdPrintEx((DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "MARS: got card interrupt %d\n",
                   intCount++));
    }


    (fdoData->BusInterface.AcknowledgeInterrupt)(fdoData->BusInterface.Context);

}

NTSTATUS
SdioReadWriteBuffer(
                   IN WDFDEVICE Device,
                   IN ULONG Function,
                   IN PMDL Mdl,
                   IN ULONG Address,
                   IN ULONG Length,
                   IN BOOLEAN WriteToDevice,
                   OUT PULONG BytesRead
                   )
{
    PFDO_DATA   fdoData;
    SDBUS_REQUEST_PACKET sdrp;
    SD_RW_EXTENDED_ARGUMENT extendedArgument;
    NTSTATUS                 status;
    const SDCMD_DESCRIPTOR ReadIoExtendedDesc =
    {SDCMD_IO_RW_EXTENDED, SDCC_STANDARD, SDTD_READ, SDTT_SINGLE_BLOCK, SDRT_5};

    const SDCMD_DESCRIPTOR WriteIoExtendedDesc =
    {SDCMD_IO_RW_EXTENDED, SDCC_STANDARD, SDTD_WRITE, SDTT_SINGLE_BLOCK, SDRT_5};

    //PAGED_CODE();

    fdoData = MarsFdoGetData(Device);

    RtlZeroMemory(&sdrp, sizeof(SDBUS_REQUEST_PACKET));

    sdrp.RequestFunction = SDRF_DEVICE_COMMAND;
    sdrp.Parameters.DeviceCommand.Mdl = Mdl;

    extendedArgument.u.AsULONG = 0;
    extendedArgument.u.bits.Function = Function;
    extendedArgument.u.bits.OpCode = 1;             // increment address
    extendedArgument.u.bits.BlockMode = fdoData->BlockMode;
    extendedArgument.u.bits.Address = Address;

    if (WriteToDevice) {
        extendedArgument.u.bits.WriteToDevice = 1;
        sdrp.Parameters.DeviceCommand.CmdDesc  = WriteIoExtendedDesc;
    } else {
        sdrp.Parameters.DeviceCommand.CmdDesc  = ReadIoExtendedDesc;
    }

    if (fdoData->BlockMode == 1) {
        sdrp.Parameters.DeviceCommand.CmdDesc.TransferType = SDTT_MULTI_BLOCK_NO_CMD12;
    }


    sdrp.Parameters.DeviceCommand.Argument = extendedArgument.u.AsULONG;
    sdrp.Parameters.DeviceCommand.Length   = Length;

    //
    // Send the IO request down to the bus driver
    //

    status = SdBusSubmitRequest(fdoData->BusInterface.Context, &sdrp);
    *BytesRead = (ULONG)sdrp.Information;
    return status;

}

NTSTATUS
SdioReadWriteByte(
                 IN WDFDEVICE Device,
                 IN ULONG Function,
                 IN PUCHAR Data,
                 IN ULONG Address,
                 IN BOOLEAN WriteToDevice
                 )
/*++


--*/

{
    PFDO_DATA                    fdoData;
    NTSTATUS                    status;
    SDBUS_REQUEST_PACKET        sdrp;
    SD_RW_DIRECT_ARGUMENT        directArgument;

    const SDCMD_DESCRIPTOR ReadIoDirectDesc =
    {SDCMD_IO_RW_DIRECT, SDCC_STANDARD, SDTD_READ, SDTT_CMD_ONLY, SDRT_5};

    const SDCMD_DESCRIPTOR WriteIoDirectDesc =
    {SDCMD_IO_RW_DIRECT, SDCC_STANDARD, SDTD_WRITE, SDTT_CMD_ONLY, SDRT_5};

    //
    // get an SD request packet
    //

    fdoData = MarsFdoGetData(Device);

    RtlZeroMemory(&sdrp, sizeof(SDBUS_REQUEST_PACKET));

    sdrp.RequestFunction = SDRF_DEVICE_COMMAND;

    directArgument.u.AsULONG = 0;
    directArgument.u.bits.Function = Function;
    directArgument.u.bits.Address = Address;


    if (WriteToDevice) {
        directArgument.u.bits.WriteToDevice = 1;
        directArgument.u.bits.Data = *Data;
        sdrp.Parameters.DeviceCommand.CmdDesc  = WriteIoDirectDesc;
    } else {
        sdrp.Parameters.DeviceCommand.CmdDesc  = ReadIoDirectDesc;
    }

    sdrp.Parameters.DeviceCommand.Argument = directArgument.u.AsULONG;

    //
    // Send the IO request down to the bus driver
    //

    status = SdBusSubmitRequest(fdoData->BusInterface.Context, &sdrp);

    if (NT_SUCCESS(status) && !WriteToDevice) {
        *Data = sdrp.ResponseData.AsUCHAR[0];
    }

    return status;
}

NTSTATUS
SdioGetProperty(
               IN WDFDEVICE Device,
               IN SDBUS_PROPERTY Property,
               IN PVOID Buffer,
               IN ULONG Length
               )
{
    PFDO_DATA fdoData;
    SDBUS_REQUEST_PACKET sdrp;

    fdoData = MarsFdoGetData(Device);
    RtlZeroMemory(&sdrp, sizeof(SDBUS_REQUEST_PACKET));

    sdrp.RequestFunction = SDRF_GET_PROPERTY;
    sdrp.Parameters.GetSetProperty.Property = Property;
    sdrp.Parameters.GetSetProperty.Buffer = Buffer;
    sdrp.Parameters.GetSetProperty.Length = Length;

    return SdBusSubmitRequest(fdoData->BusInterface.Context, &sdrp);
}


NTSTATUS
SdioSetProperty(
               IN WDFDEVICE Device,
               IN SDBUS_PROPERTY Property,
               IN PVOID Buffer,
               IN ULONG Length
               )
{
    PFDO_DATA fdoData;
    SDBUS_REQUEST_PACKET sdrp;

    fdoData = MarsFdoGetData(Device);

    RtlZeroMemory(&sdrp, sizeof(SDBUS_REQUEST_PACKET));

    sdrp.RequestFunction = SDRF_SET_PROPERTY;
    sdrp.Parameters.GetSetProperty.Property = Property;
    sdrp.Parameters.GetSetProperty.Buffer = Buffer;
    sdrp.Parameters.GetSetProperty.Length = Length;

    return SdBusSubmitRequest(fdoData->BusInterface.Context, &sdrp);
}

