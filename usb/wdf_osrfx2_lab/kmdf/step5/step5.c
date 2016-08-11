/*++

Step4: This steps shows:
       1) How to map KdPrint function to do WPP tracing
--*/

#include <stdarg.h> // To avoid build errors on Win2K due to WPP
#include "ntddk.h"
#include "wdf.h"
#include "prototypes.h"
#include "usbdi.h"
#include "usbdlib.h"
#include "wdfusb.h"
#include "initguid.h"

#include "step5.tmh"

DEFINE_GUID(GUID_DEVINTERFACE_OSRUSBFX2, // Generated using guidgen.exe
   0x573e8c73, 0xcb4, 0x4471, 0xa1, 0xbf, 0xfa, 0xb2, 0x6c, 0x31, 0xd3, 0x84);
// {573E8C73-0CB4-4471-A1BF-FAB26C31D384}

#define IOCTL_INDEX                     0x800
#define FILE_DEVICE_OSRUSBFX2           65500U
#define USBFX2LK_SET_BARGRAPH_DISPLAY 0xD8
#define BULK_OUT_ENDPOINT_INDEX        1
#define BULK_IN_ENDPOINT_INDEX         2
#define IOCTL_OSRUSBFX2_SET_BAR_GRAPH_DISPLAY CTL_CODE(FILE_DEVICE_OSRUSBFX2,\
                                                    IOCTL_INDEX + 5, \
                                                    METHOD_BUFFERED, \
                                                    FILE_WRITE_ACCESS)
typedef struct _DEVICE_CONTEXT {
  WDFUSBDEVICE      UsbDevice;
  WDFUSBINTERFACE   UsbInterface;
  WDFUSBPIPE        BulkReadPipe;
  WDFUSBPIPE        BulkWritePipe;
} DEVICE_CONTEXT, *PDEVICE_CONTEXT;

WDF_DECLARE_CONTEXT_TYPE_WITH_NAME(DEVICE_CONTEXT, GetDeviceContext)


NTSTATUS
DriverEntry(
    IN PDRIVER_OBJECT  DriverObject,
    IN PUNICODE_STRING RegistryPath
    )
{
    WDF_DRIVER_CONFIG       config;
    NTSTATUS                status;
    WDF_OBJECT_ATTRIBUTES   attributes;

    WPP_INIT_TRACING( DriverObject, RegistryPath );

    KdPrint(("DriverEntry of Step5\n"));

    WDF_DRIVER_CONFIG_INIT(&config, EvtDeviceAdd);

    //
    // Register a cleanup callback so that we can call WPP_CLEANUP when
    // the framework driver object is deleted during driver unload.
    //
    WDF_OBJECT_ATTRIBUTES_INIT(&attributes);
    attributes.EvtCleanupCallback = EvtDriverContextCleanup;

    status = WdfDriverCreate(DriverObject,
                        RegistryPath,
                        &attributes,
                        &config,
                        WDF_NO_HANDLE
                        );

    if (!NT_SUCCESS(status)) {
        KdPrint(("WdfDriverCreate failed %!STATUS!\n", status));
        //
        // Cleanup tracing here because DriverContextCleanup will not be called
        // as we have failed to create WDFDRIVER object itself.
        // Please note that if your return failure from DriverEntry after the
        // WDFDRIVER object is created successfully, you don't have to
        // call WPP cleanup because in those cases DriverContextCleanup
        // will be executed when the framework deletes the DriverObject.
        //
        WPP_CLEANUP(DriverObject);
    }

    return status;
}

VOID
EvtDriverContextCleanup(
    IN WDFOBJECT Driver
    )
{
    WPP_CLEANUP(WdfDriverWdmGetDriverObject((WDFDRIVER)Driver ));
}

NTSTATUS
EvtDeviceAdd(
    IN WDFDRIVER        Driver,
    IN PWDFDEVICE_INIT  DeviceInit
    )
{
    WDF_OBJECT_ATTRIBUTES               attributes;
    NTSTATUS                            status;
    WDFDEVICE                           device;
    WDF_PNPPOWER_EVENT_CALLBACKS        pnpPowerCallbacks;
    WDF_IO_QUEUE_CONFIG                 ioQueueConfig;

    UNREFERENCED_PARAMETER(Driver);

    WDF_PNPPOWER_EVENT_CALLBACKS_INIT(&pnpPowerCallbacks);
    pnpPowerCallbacks.EvtDevicePrepareHardware = EvtDevicePrepareHardware;
    WdfDeviceInitSetPnpPowerEventCallbacks(DeviceInit, &pnpPowerCallbacks);

    WDF_OBJECT_ATTRIBUTES_INIT_CONTEXT_TYPE(&attributes, DEVICE_CONTEXT);

    status = WdfDeviceCreate(&DeviceInit, &attributes, &device);
    if (!NT_SUCCESS(status)) {
        KdPrint(("WdfDeviceCreate failed %!STATUS!\n", status));
        return status;
    }

    status = WdfDeviceCreateDeviceInterface(device,
                                (LPGUID) &GUID_DEVINTERFACE_OSRUSBFX2,
                                NULL);// Reference String
    if (!NT_SUCCESS(status)) {
        KdPrint(("WdfDeviceCreateDeviceInterface failed %!STATUS!\n", status));
        return status;
    }

    WDF_IO_QUEUE_CONFIG_INIT_DEFAULT_QUEUE(&ioQueueConfig,
                                    WdfIoQueueDispatchParallel);

    ioQueueConfig.EvtIoDeviceControl = EvtIoDeviceControl;
    ioQueueConfig.EvtIoRead = EvtIoRead;
    ioQueueConfig.EvtIoWrite = EvtIoWrite;

    //
    // By default, Static Driver Verifier (SDV) displays a warning if it 
    // doesn't find the EvtIoStop callback on a power-managed queue. 
    // The 'assume' below causes SDV to suppress this warning. 
    // Please see 'final' step for implementation of EvtIoStop.
    //
    __analysis_assume(ioQueueConfig.EvtIoStop != 0);
    status = WdfIoQueueCreate(device,
                         &ioQueueConfig,
                         WDF_NO_OBJECT_ATTRIBUTES,
                         WDF_NO_HANDLE);
    __analysis_assume(ioQueueConfig.EvtIoStop == 0);
    
    if (!NT_SUCCESS(status)) {
        KdPrint(("WdfIoQueueCreate failed  %!STATUS!\n", status));
        return status;
    }

    return status;
}


NTSTATUS
EvtDevicePrepareHardware(
    IN WDFDEVICE    Device,
    IN WDFCMRESLIST ResourceList,
    IN WDFCMRESLIST ResourceListTranslated
    )
{
    NTSTATUS                            status;
    PDEVICE_CONTEXT                     pDeviceContext;
    WDF_USB_DEVICE_SELECT_CONFIG_PARAMS configParams;

    UNREFERENCED_PARAMETER(ResourceList);
    UNREFERENCED_PARAMETER(ResourceListTranslated);

    pDeviceContext = GetDeviceContext(Device);
    
    //
    // Create the USB device if it is not already created.
    //
    if (pDeviceContext->UsbDevice == NULL) {
        WDF_USB_DEVICE_CREATE_CONFIG config;

        WDF_USB_DEVICE_CREATE_CONFIG_INIT(&config,
                                   USBD_CLIENT_CONTRACT_VERSION_602);

        status = WdfUsbTargetDeviceCreateWithParameters(Device,
                                               &config,
                                               WDF_NO_OBJECT_ATTRIBUTES,
                                               &pDeviceContext->UsbDevice);

        if (!NT_SUCCESS(status)) {
            KdPrint(("WdfUsbTargetDeviceCreateWithParameters failed 0x%x\n", status));        
            return status;
        }
    }

    WDF_USB_DEVICE_SELECT_CONFIG_PARAMS_INIT_SINGLE_INTERFACE(&configParams);

    status = WdfUsbTargetDeviceSelectConfig(pDeviceContext->UsbDevice,
                                        WDF_NO_OBJECT_ATTRIBUTES,
                                        &configParams);
    if(!NT_SUCCESS(status)) {
        KdPrint(("WdfUsbTargetDeviceSelectConfig failed %!STATUS!\n", status));
        return status;
    }

    pDeviceContext->UsbInterface =
                configParams.Types.SingleInterface.ConfiguredUsbInterface;


    pDeviceContext->BulkReadPipe = WdfUsbInterfaceGetConfiguredPipe(
                                                  pDeviceContext->UsbInterface,
                                                  BULK_IN_ENDPOINT_INDEX,
                                                  NULL);// pipeInfo

    WdfUsbTargetPipeSetNoMaximumPacketSizeCheck(pDeviceContext->BulkReadPipe);

    pDeviceContext->BulkWritePipe = WdfUsbInterfaceGetConfiguredPipe(
                                                  pDeviceContext->UsbInterface,
                                                  BULK_OUT_ENDPOINT_INDEX,
                                                  NULL);// pipeInfo

    WdfUsbTargetPipeSetNoMaximumPacketSizeCheck(pDeviceContext->BulkWritePipe);

    return status;
}

VOID
EvtIoDeviceControl(
    IN WDFQUEUE   Queue,
    IN WDFREQUEST Request,
    IN size_t     OutputBufferLength,
    IN size_t     InputBufferLength,
    IN ULONG      IoControlCode
    )
{
    WDFDEVICE                           device;
    PDEVICE_CONTEXT                     pDevContext;
    size_t                              bytesTransferred = 0;
    NTSTATUS                            status;
    WDF_USB_CONTROL_SETUP_PACKET        controlSetupPacket;
    WDF_MEMORY_DESCRIPTOR               memDesc;
    WDFMEMORY                           memory;
    WDF_REQUEST_SEND_OPTIONS            sendOptions;

    UNREFERENCED_PARAMETER(InputBufferLength);
    UNREFERENCED_PARAMETER(OutputBufferLength);

    device = WdfIoQueueGetDevice(Queue);
    pDevContext = GetDeviceContext(device);

    switch(IoControlCode) {

    case IOCTL_OSRUSBFX2_SET_BAR_GRAPH_DISPLAY:

        if(InputBufferLength < sizeof(UCHAR)) {
            status = STATUS_BUFFER_OVERFLOW;
            bytesTransferred = sizeof(UCHAR);
            break;
        }

        status = WdfRequestRetrieveInputMemory(Request, &memory);
        if (!NT_SUCCESS(status)) {
            KdPrint(("WdfRequestRetrieveMemory failed %!STATUS!", status));
            break;
        }

        WDF_USB_CONTROL_SETUP_PACKET_INIT_VENDOR(&controlSetupPacket,
                                        BmRequestHostToDevice,
                                        BmRequestToDevice,
                                        USBFX2LK_SET_BARGRAPH_DISPLAY, // Request
                                        0, // Value
                                        0); // Index

        WDF_MEMORY_DESCRIPTOR_INIT_HANDLE(&memDesc, memory, NULL);

       //
       // Send the I/O with a timeout to avoid hanging the calling 
       // thread indefinitely.
       //
        WDF_REQUEST_SEND_OPTIONS_INIT(&sendOptions,
                                  WDF_REQUEST_SEND_OPTION_TIMEOUT);

        WDF_REQUEST_SEND_OPTIONS_SET_TIMEOUT(&sendOptions,
                                         WDF_REL_TIMEOUT_IN_MS(100));

        status = WdfUsbTargetDeviceSendControlTransferSynchronously(
                                        pDevContext->UsbDevice,
                                        NULL, // Optional WDFREQUEST
                                        &sendOptions, // PWDF_REQUEST_SEND_OPTIONS
                                        &controlSetupPacket,
                                        &memDesc,
                                        (PULONG)&bytesTransferred);
        if (!NT_SUCCESS(status)) {
            KdPrint(("SendControlTransfer failed %!STATUS!", status));
            break;
        }
        break;

    default:
        status = STATUS_INVALID_DEVICE_REQUEST;
        break;
    }

    WdfRequestCompleteWithInformation(Request, status, bytesTransferred);

    return;
}

VOID
EvtIoRead(
    IN WDFQUEUE         Queue,
    IN WDFREQUEST       Request,
    IN size_t           Length
    )
{
    WDFUSBPIPE                  pipe;
    NTSTATUS                    status;
    WDFMEMORY                   reqMemory;
    PDEVICE_CONTEXT             pDeviceContext;
    BOOLEAN                     ret;

    UNREFERENCED_PARAMETER(Length);

    pDeviceContext = GetDeviceContext(WdfIoQueueGetDevice(Queue));

    pipe = pDeviceContext->BulkReadPipe;

    status = WdfRequestRetrieveOutputMemory(Request, &reqMemory);
    if(!NT_SUCCESS(status)){
        goto Exit;
    }

    status = WdfUsbTargetPipeFormatRequestForRead(pipe,
                                        Request,
                                        reqMemory,
                                        NULL // Offsets
                                        );
    if (!NT_SUCCESS(status)) {
        goto Exit;
    }

    WdfRequestSetCompletionRoutine(
                            Request,
                            EvtRequestReadCompletionRoutine,
                            pipe);

    ret = WdfRequestSend(Request,
                    WdfUsbTargetPipeGetIoTarget(pipe),
                    WDF_NO_SEND_OPTIONS);
    if (ret == FALSE) {
        status = WdfRequestGetStatus(Request);
        goto Exit;
    } else {
        return;
    }

Exit:
    WdfRequestCompleteWithInformation(Request, status, 0);

    return;
}

VOID
EvtRequestReadCompletionRoutine(
    IN WDFREQUEST                  Request,
    IN WDFIOTARGET                 Target,
    PWDF_REQUEST_COMPLETION_PARAMS CompletionParams,
    IN WDFCONTEXT                  Context
    )
{
    NTSTATUS    status;
    size_t      bytesRead = 0;
    PWDF_USB_REQUEST_COMPLETION_PARAMS usbCompletionParams;

    UNREFERENCED_PARAMETER(Target);
    UNREFERENCED_PARAMETER(Context);

    status = CompletionParams->IoStatus.Status;

    usbCompletionParams = CompletionParams->Parameters.Usb.Completion;

    bytesRead =  usbCompletionParams->Parameters.PipeRead.Length;

    if (NT_SUCCESS(status)){
        KdPrint(("Number of bytes read: %I64d\n", (INT64)bytesRead));
    } else {
        KdPrint(("Read failed - request status %!STATUS! UsbdStatus %!STATUS!\n",
                status, usbCompletionParams->UsbdStatus));

    }

    WdfRequestCompleteWithInformation(Request, status, bytesRead);

    return;
}

VOID
EvtIoWrite(
    IN WDFQUEUE         Queue,
    IN WDFREQUEST       Request,
    IN size_t           Length
    )
{
    NTSTATUS                    status;
    WDFUSBPIPE                  pipe;
    WDFMEMORY                   reqMemory;
    PDEVICE_CONTEXT             pDeviceContext;
    BOOLEAN                     ret;

    UNREFERENCED_PARAMETER(Length);

    pDeviceContext = GetDeviceContext(WdfIoQueueGetDevice(Queue));

    pipe = pDeviceContext->BulkWritePipe;

    status = WdfRequestRetrieveInputMemory(Request, &reqMemory);
    if(!NT_SUCCESS(status)){
        goto Exit;
    }

    status = WdfUsbTargetPipeFormatRequestForWrite(pipe,
                                              Request,
                                              reqMemory,
                                              NULL); // Offset
    if (!NT_SUCCESS(status)) {
        goto Exit;
    }

    WdfRequestSetCompletionRoutine(
                            Request,
                            EvtRequestWriteCompletionRoutine,
                            pipe);

    ret = WdfRequestSend(Request,
                    WdfUsbTargetPipeGetIoTarget(pipe),
                    WDF_NO_SEND_OPTIONS);
    if (ret == FALSE) {
        status = WdfRequestGetStatus(Request);
        goto Exit;
    } else {
        return;
    }

Exit:
    WdfRequestCompleteWithInformation(Request, status, 0);

    return;
}

VOID
EvtRequestWriteCompletionRoutine(
    IN WDFREQUEST                  Request,
    IN WDFIOTARGET                 Target,
    PWDF_REQUEST_COMPLETION_PARAMS CompletionParams,
    IN WDFCONTEXT                  Context
    )
{
    NTSTATUS    status;
    size_t      bytesWritten = 0;
    PWDF_USB_REQUEST_COMPLETION_PARAMS usbCompletionParams;

    UNREFERENCED_PARAMETER(Target);
    UNREFERENCED_PARAMETER(Context);

    status = CompletionParams->IoStatus.Status;

    usbCompletionParams = CompletionParams->Parameters.Usb.Completion;

    bytesWritten =  usbCompletionParams->Parameters.PipeWrite.Length;

    if (NT_SUCCESS(status)){
        KdPrint(("Number of bytes written: %I64d\n", (INT64)bytesWritten));
    } else {
        KdPrint(("Write failed: request Status %!STATUS! UsbdStatus %!STATUS!\n",
                status, usbCompletionParams->UsbdStatus));
    }

    WdfRequestCompleteWithInformation(Request, status, bytesWritten);

    return;
}
