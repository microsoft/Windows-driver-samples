/*++

Step3: This steps shows:
       1) How to create a default parallel queue to receive a IOCTL requests to 
          set bar graph display.
       2) How to retreive memory handle from the requests and use that to send
          a vendor command to the USB device.
--*/

#include "ntddk.h"
#include "wdf.h"
#include "prototypes.h"
#include "usbdi.h"
#include "usbdlib.h"
#include "wdfusb.h"
#include "initguid.h"

DEFINE_GUID(GUID_DEVINTERFACE_OSRUSBFX2, // Generated using guidgen.exe
   0x573e8c73, 0xcb4, 0x4471, 0xa1, 0xbf, 0xfa, 0xb2, 0x6c, 0x31, 0xd3, 0x84);
// {573E8C73-0CB4-4471-A1BF-FAB26C31D384}

#define IOCTL_INDEX                     0x800
#define FILE_DEVICE_OSRUSBFX2           65500U
#define USBFX2LK_SET_BARGRAPH_DISPLAY 0xD8
#define IOCTL_OSRUSBFX2_SET_BAR_GRAPH_DISPLAY CTL_CODE(FILE_DEVICE_OSRUSBFX2,\
                                                    IOCTL_INDEX + 5, \
                                                    METHOD_BUFFERED, \
                                                    FILE_WRITE_ACCESS)
typedef struct _DEVICE_CONTEXT {
  WDFUSBDEVICE      UsbDevice;
  WDFUSBINTERFACE   UsbInterface;
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

    KdPrint(("DriverEntry of Step3\n"));

    WDF_DRIVER_CONFIG_INIT(&config, EvtDeviceAdd);

    status = WdfDriverCreate(DriverObject,
                        RegistryPath,
                        WDF_NO_OBJECT_ATTRIBUTES, 
                        &config,     
                        WDF_NO_HANDLE 
                        );

    if (!NT_SUCCESS(status)) {
        KdPrint(("WdfDriverCreate failed 0x%x\n", status));
    }

    return status;
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
        KdPrint(("WdfDeviceCreate failed 0x%x\n", status));
        return status;
    }

    status = WdfDeviceCreateDeviceInterface(device,
                                (LPGUID) &GUID_DEVINTERFACE_OSRUSBFX2,
                                NULL);// Reference String
    if (!NT_SUCCESS(status)) {
        KdPrint(("WdfDeviceCreateDeviceInterface failed 0x%x\n", status));
        return status;
    }

    WDF_IO_QUEUE_CONFIG_INIT_DEFAULT_QUEUE(&ioQueueConfig,
                                    WdfIoQueueDispatchParallel);

    ioQueueConfig.EvtIoDeviceControl = EvtIoDeviceControl;

    //
    // By default, Static Driver Verifier (SDV) displays a warning if it 
    // doesn't find the EvtIoStop callback on a power-managed queue. 
    // The 'assume' below causes SDV to suppress this warning. If the driver 
    // has not explicitly set PowerManaged to WdfFalse, the framework creates
    // power-managed queues when the device is not a filter driver.  Normally 
    // the EvtIoStop is required for power-managed queues, but for this driver
    // it is not needed b/c the driver doesn't hold on to the requests for 
    // long time or forward them to other drivers. 
    // If the EvtIoStop callback is not implemented, the framework waits for
    // all driver-owned requests to be done before moving in the Dx/sleep 
    // states or before removing the device, which is the correct behavior 
    // for this type of driver. If the requests were taking an indeterminate
    // amount of time to complete, or if the driver forwarded the requests
    // to a lower driver/another stack, the queue should have an 
    // EvtIoStop/EvtIoResume.
    //
    __analysis_assume(ioQueueConfig.EvtIoStop != 0);
    status = WdfIoQueueCreate(device,
                         &ioQueueConfig,
                         WDF_NO_OBJECT_ATTRIBUTES,
                         WDF_NO_HANDLE);
    __analysis_assume(ioQueueConfig.EvtIoStop == 0);
    
    if (!NT_SUCCESS(status)) {
        KdPrint(("WdfIoQueueCreate failed 0x%x\n", status));
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
        KdPrint(("WdfUsbTargetDeviceSelectConfig failed 0x%x\n", status));
        return status;
    }

    pDeviceContext->UsbInterface =  
                configParams.Types.SingleInterface.ConfiguredUsbInterface;        
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
            KdPrint(("WdfRequestRetrieveMemory failed 0x%x", status));
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
            KdPrint(("SendControlTransfer failed 0x%x", status));
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
