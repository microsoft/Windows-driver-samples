/*++

Step5: This step shows:
       1) How to map KdPrint function to do WPP tracing
       2) Demonstrates additional USB DDI coverage for usbdlib.h and wdfusb.h
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

#define POOL_TAG 'RsoU'  // 'UsoR' — OSRUSBFX2 pool tag

DEFINE_GUID(GUID_DEVINTERFACE_OSRUSBFX2, // Generated using guidgen.exe
   0x573e8c73, 0xcb4, 0x4471, 0xa1, 0xbf, 0xfa, 0xb2, 0x6c, 0x31, 0xd3, 0x84);
// {573E8C73-0CB4-4471-A1BF-FAB26C31D384}

#define IOCTL_INDEX                     0x800
#define FILE_DEVICE_OSRUSBFX2           65500U
#define USBFX2LK_READ_7SEGMENT_DISPLAY  0xD4
#define USBFX2LK_READ_SWITCHES          0xD6
#define USBFX2LK_READ_BARGRAPH_DISPLAY  0xD7
#define USBFX2LK_SET_BARGRAPH_DISPLAY   0xD8
#define USBFX2LK_REENUMERATE            0xDA
#define USBFX2LK_SET_7SEGMENT_DISPLAY   0xDB
#define BULK_OUT_ENDPOINT_INDEX        1
#define BULK_IN_ENDPOINT_INDEX         2

#include <pshpack1.h>
typedef struct _BAR_GRAPH_STATE {
    union {
        struct {
            UCHAR Bar1 : 1;
            UCHAR Bar2 : 1;
            UCHAR Bar3 : 1;
            UCHAR Bar4 : 1;
            UCHAR Bar5 : 1;
            UCHAR Bar6 : 1;
            UCHAR Bar7 : 1;
            UCHAR Bar8 : 1;
        };
        UCHAR BarsAsUChar;
    };
} BAR_GRAPH_STATE, *PBAR_GRAPH_STATE;

typedef struct _SWITCH_STATE {
    union {
        struct {
            UCHAR Switch1 : 1;
            UCHAR Switch2 : 1;
            UCHAR Switch3 : 1;
            UCHAR Switch4 : 1;
            UCHAR Switch5 : 1;
            UCHAR Switch6 : 1;
            UCHAR Switch7 : 1;
            UCHAR Switch8 : 1;
        };
        UCHAR SwitchesAsUChar;
    };
} SWITCH_STATE, *PSWITCH_STATE;
#include <poppack.h>

#define IOCTL_OSRUSBFX2_GET_CONFIG_DESCRIPTOR CTL_CODE(FILE_DEVICE_OSRUSBFX2,\
                                                    IOCTL_INDEX, \
                                                    METHOD_BUFFERED, \
                                                    FILE_READ_ACCESS)

#define IOCTL_OSRUSBFX2_RESET_DEVICE CTL_CODE(FILE_DEVICE_OSRUSBFX2,\
                                                    IOCTL_INDEX + 1, \
                                                    METHOD_BUFFERED, \
                                                    FILE_WRITE_ACCESS)

#define IOCTL_OSRUSBFX2_REENUMERATE_DEVICE CTL_CODE(FILE_DEVICE_OSRUSBFX2,\
                                                    IOCTL_INDEX + 3, \
                                                    METHOD_BUFFERED, \
                                                    FILE_WRITE_ACCESS)

#define IOCTL_OSRUSBFX2_GET_BAR_GRAPH_DISPLAY CTL_CODE(FILE_DEVICE_OSRUSBFX2,\
                                                    IOCTL_INDEX + 4, \
                                                    METHOD_BUFFERED, \
                                                    FILE_READ_ACCESS)

#define IOCTL_OSRUSBFX2_SET_BAR_GRAPH_DISPLAY CTL_CODE(FILE_DEVICE_OSRUSBFX2,\
                                                    IOCTL_INDEX + 5, \
                                                    METHOD_BUFFERED, \
                                                    FILE_WRITE_ACCESS)

#define IOCTL_OSRUSBFX2_READ_SWITCHES CTL_CODE(FILE_DEVICE_OSRUSBFX2,\
                                                    IOCTL_INDEX + 6, \
                                                    METHOD_BUFFERED, \
                                                    FILE_READ_ACCESS)

#define IOCTL_OSRUSBFX2_GET_7_SEGMENT_DISPLAY CTL_CODE(FILE_DEVICE_OSRUSBFX2,\
                                                    IOCTL_INDEX + 7, \
                                                    METHOD_BUFFERED, \
                                                    FILE_READ_ACCESS)

#define IOCTL_OSRUSBFX2_SET_7_SEGMENT_DISPLAY CTL_CODE(FILE_DEVICE_OSRUSBFX2,\
                                                    IOCTL_INDEX + 8, \
                                                    METHOD_BUFFERED, \
                                                    FILE_WRITE_ACCESS)

#define IOCTL_OSRUSBFX2_GET_INTERRUPT_MESSAGE CTL_CODE(FILE_DEVICE_OSRUSBFX2,\
                                                    IOCTL_INDEX + 9, \
                                                    METHOD_OUT_DIRECT, \
                                                    FILE_READ_ACCESS)

//
// Additional IOCTLs to demonstrate uncovered USB DDI usage.
//
#define IOCTL_OSRUSBFX2_GET_USB_STATUS CTL_CODE(FILE_DEVICE_OSRUSBFX2,\
                                                    IOCTL_INDEX + 20, \
                                                    METHOD_BUFFERED, \
                                                    FILE_READ_ACCESS)

#define IOCTL_OSRUSBFX2_SET_FEATURE CTL_CODE(FILE_DEVICE_OSRUSBFX2,\
                                                    IOCTL_INDEX + 21, \
                                                    METHOD_BUFFERED, \
                                                    FILE_WRITE_ACCESS)

#define IOCTL_OSRUSBFX2_CLEAR_FEATURE CTL_CODE(FILE_DEVICE_OSRUSBFX2,\
                                                    IOCTL_INDEX + 22, \
                                                    METHOD_BUFFERED, \
                                                    FILE_WRITE_ACCESS)

#define IOCTL_OSRUSBFX2_GET_STRING CTL_CODE(FILE_DEVICE_OSRUSBFX2,\
                                                    IOCTL_INDEX + 23, \
                                                    METHOD_BUFFERED, \
                                                    FILE_READ_ACCESS)

#define IOCTL_OSRUSBFX2_CYCLE_PORT CTL_CODE(FILE_DEVICE_OSRUSBFX2,\
                                                    IOCTL_INDEX + 24, \
                                                    METHOD_BUFFERED, \
                                                    FILE_WRITE_ACCESS)

#define IOCTL_OSRUSBFX2_RESET_PIPE CTL_CODE(FILE_DEVICE_OSRUSBFX2,\
                                                    IOCTL_INDEX + 25, \
                                                    METHOD_BUFFERED, \
                                                    FILE_WRITE_ACCESS)

#define IOCTL_OSRUSBFX2_ABORT_PIPE CTL_CODE(FILE_DEVICE_OSRUSBFX2,\
                                                    IOCTL_INDEX + 26, \
                                                    METHOD_BUFFERED, \
                                                    FILE_WRITE_ACCESS)

#define IOCTL_OSRUSBFX2_WRITE_SYNC CTL_CODE(FILE_DEVICE_OSRUSBFX2,\
                                                    IOCTL_INDEX + 27, \
                                                    METHOD_BUFFERED, \
                                                    FILE_WRITE_ACCESS)

typedef struct _DEVICE_CONTEXT {
  WDFUSBDEVICE      UsbDevice;
  WDFUSBINTERFACE   UsbInterface;
  WDFUSBPIPE        BulkReadPipe;
  WDFUSBPIPE        BulkWritePipe;
  USBD_HANDLE       UsbdHandle;
} DEVICE_CONTEXT, *PDEVICE_CONTEXT;

WDF_DECLARE_CONTEXT_TYPE_WITH_NAME(DEVICE_CONTEXT, GetDeviceContext)

//
// Forward declaration for helper routine defined later in this file.
//
VOID
OsrFxDemonstrateUrbDdis(
    IN WDFDEVICE        Device,
    IN PDEVICE_CONTEXT  pDeviceContext
    );


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
    pnpPowerCallbacks.EvtDeviceReleaseHardware = EvtDeviceReleaseHardware;
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

    //
    // ---- wdfusb.h DDI coverage: WdfUsbInterfaceGetNumEndpoints ----
    // Query the number of endpoints on the current alternate setting.
    //
    {
        BYTE numEndpoints;
        numEndpoints = WdfUsbInterfaceGetNumEndpoints(
                            pDeviceContext->UsbInterface,
                            0); // setting index 0
        KdPrint(("Number of endpoints: %d\n", numEndpoints));
    }

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

    //
    // ---- wdfusb.h DDI coverage: WdfUsbInterfaceGetEndpointInformation ----
    // Retrieve endpoint information for each configured endpoint.
    //
    {
        BYTE i;
        BYTE numEp;
        numEp = WdfUsbInterfaceGetNumEndpoints(
                    pDeviceContext->UsbInterface, 0);
        for (i = 0; i < numEp; i++) {
            WDF_USB_PIPE_INFORMATION pipeInfo;
            WDF_USB_PIPE_INFORMATION_INIT(&pipeInfo);
            WdfUsbInterfaceGetEndpointInformation(
                pDeviceContext->UsbInterface,
                0, // setting index
                i, // endpoint index
                &pipeInfo);
            KdPrint(("Endpoint %d: MaxPacketSize=%d, type=%d\n",
                     i, pipeInfo.MaximumPacketSize, pipeInfo.PipeType));
        }
    }

    //
    // ---- wdfusb.h DDI coverage: WdfUsbTargetPipeGetType ----
    // Demonstrate querying pipe type for the bulk read and write pipes.
    //
    {
        WDF_USB_PIPE_TYPE readPipeType;
        WDF_USB_PIPE_TYPE writePipeType;
        readPipeType = WdfUsbTargetPipeGetType(pDeviceContext->BulkReadPipe);
        writePipeType = WdfUsbTargetPipeGetType(pDeviceContext->BulkWritePipe);
        KdPrint(("Read pipe type: %d, Write pipe type: %d\n",
                 readPipeType, writePipeType));
    }

    //
    // ---- wdfusb.h DDI coverage: WDF_USB_PIPE_DIRECTION_IN / _OUT ----
    // Verify pipe directions using the direction helper macros.
    //
    {
        WDF_USB_PIPE_INFORMATION readPipeInfo, writePipeInfo;
        WDF_USB_PIPE_INFORMATION_INIT(&readPipeInfo);
        WDF_USB_PIPE_INFORMATION_INIT(&writePipeInfo);
        WdfUsbInterfaceGetEndpointInformation(
            pDeviceContext->UsbInterface, 0, BULK_IN_ENDPOINT_INDEX, &readPipeInfo);
        WdfUsbInterfaceGetEndpointInformation(
            pDeviceContext->UsbInterface, 0, BULK_OUT_ENDPOINT_INDEX, &writePipeInfo);

        if (WDF_USB_PIPE_DIRECTION_IN(readPipeInfo.EndpointAddress)) {
            KdPrint(("Read pipe confirmed as IN direction\n"));
        }
        if (WDF_USB_PIPE_DIRECTION_OUT(writePipeInfo.EndpointAddress)) {
            KdPrint(("Write pipe confirmed as OUT direction\n"));
        }
    }

    //
    // ---- usbdlib.h DDI coverage: USBD_IsInterfaceVersionSupported ----
    // Check if the USB driver stack supports USBD interface version 602.
    //
    {
        USBD_HANDLE usbdHandle;
        usbdHandle = NULL;
        status = USBD_CreateHandle(WdfDeviceWdmGetDeviceObject(Device),
                                   WdfDeviceWdmGetAttachedDevice(Device),
                                   USBD_CLIENT_CONTRACT_VERSION_602,
                                   POOL_TAG,
                                   &usbdHandle);
        if (NT_SUCCESS(status)) {
            BOOLEAN supported;
            supported = USBD_IsInterfaceVersionSupported(usbdHandle,
                                        USBD_INTERFACE_VERSION_602);
            KdPrint(("USBD interface version 602 supported: %d\n", supported));
            pDeviceContext->UsbdHandle = usbdHandle;
        } else {
            KdPrint(("USBD_CreateHandle failed %!STATUS!\n", status));
            pDeviceContext->UsbdHandle = NULL;
            //
            // Non-fatal: continue even without a USBD handle.
            //
            status = STATUS_SUCCESS;
        }
    }

    //
    // ---- usbdlib.h DDI coverage: USBD_GetPdoRegistryParameter ----
    // Read a sample registry parameter from the device's hardware key.
    //
    {
        ULONG regValue = 0;
        NTSTATUS regStatus;
        DECLARE_CONST_UNICODE_STRING(regKeyName, L"SampleValue");
        regStatus = USBD_GetPdoRegistryParameter(
                        WdfDeviceWdmGetPhysicalDevice(Device),
                        &regValue,
                        sizeof(regValue),
                        (PWSTR)regKeyName.Buffer,
                        regKeyName.Length);
        if (NT_SUCCESS(regStatus)) {
            KdPrint(("PDO registry value: %lu\n", regValue));
        } else {
            KdPrint(("USBD_GetPdoRegistryParameter returned %!STATUS! (expected)\n",
                     regStatus));
        }
    }

    //
    // ---- usbdlib.h DDI coverage: WdfUsbTargetDeviceWdmGetConfigurationHandle ----
    // Retrieve the WDM USBD configuration handle.
    //
    {
        USBD_CONFIGURATION_HANDLE configHandle;
        configHandle = WdfUsbTargetDeviceWdmGetConfigurationHandle(
                           pDeviceContext->UsbDevice);
        KdPrint(("WDM Configuration handle: %p\n", configHandle));
    }

    //
    // ---- wdfusb.h DDI coverage: WdfUsbTargetDeviceAllocAndQueryString ----
    // Query the product string descriptor from the device.
    //
    {
        WDFMEMORY stringMem = NULL;
        USHORT numChars = 0;
        NTSTATUS strStatus;
        strStatus = WdfUsbTargetDeviceAllocAndQueryString(
                        pDeviceContext->UsbDevice,
                        WDF_NO_OBJECT_ATTRIBUTES,
                        &stringMem,
                        &numChars,
                        2, // String descriptor index (typically product string)
                        0x0409); // LANGID: English (US)
        if (NT_SUCCESS(strStatus) && stringMem != NULL) {
            KdPrint(("Product string length: %d chars\n", numChars));
            WdfObjectDelete(stringMem);
        } else {
            KdPrint(("WdfUsbTargetDeviceAllocAndQueryString returned %!STATUS!\n",
                     strStatus));
        }
    }

    //
    // ---- usbdlib.h DDI coverage: USBD_GetInterfaceLength ----
    // Demonstrate parsing an interface descriptor to get its total length.
    //
    {
        PUSB_CONFIGURATION_DESCRIPTOR configDesc;
        USHORT configDescSize;
        NTSTATUS descStatus;
        WDF_USB_DEVICE_INFORMATION deviceInfo;

        WDF_USB_DEVICE_INFORMATION_INIT(&deviceInfo);
        descStatus = WdfUsbTargetDeviceRetrieveInformation(
                         pDeviceContext->UsbDevice, &deviceInfo);
        if (NT_SUCCESS(descStatus)) {
            descStatus = WdfUsbTargetDeviceRetrieveConfigDescriptor(
                             pDeviceContext->UsbDevice, NULL, &configDescSize);
            if (descStatus == STATUS_BUFFER_TOO_SMALL && configDescSize > 0) {
                configDesc = (PUSB_CONFIGURATION_DESCRIPTOR)
                    ExAllocatePool2(POOL_FLAG_NON_PAGED, configDescSize, POOL_TAG);
                if (configDesc != NULL) {
                    descStatus = WdfUsbTargetDeviceRetrieveConfigDescriptor(
                                     pDeviceContext->UsbDevice,
                                     configDesc,
                                     &configDescSize);
                    if (NT_SUCCESS(descStatus)) {
                        //
                        // Find the first interface descriptor and get its length.
                        //
                        PUSB_INTERFACE_DESCRIPTOR ifaceDesc;
                        ifaceDesc = USBD_ParseConfigurationDescriptorEx(
                                        configDesc,
                                        configDesc,
                                        -1, // InterfaceNumber (any)
                                        -1, // AlternateSetting (any)
                                        -1, -1, -1); // Class, SubClass, Protocol
                        if (ifaceDesc != NULL) {
                            ULONG ifaceLength;
                            ifaceLength = USBD_GetInterfaceLength(
                                              ifaceDesc,
                                              (PUCHAR)configDesc + configDescSize);
                            KdPrint(("Interface descriptor length: %lu\n",
                                     ifaceLength));

                            //
                            // ---- usbdlib.h DDI coverage: USBD_CreateConfigurationRequestEx ----
                            // Build a configuration request URB from the parsed descriptor.
                            //
                            {
                                PUSBD_INTERFACE_LIST_ENTRY ifaceList;
                                PURB configUrb;
                                ifaceList = (PUSBD_INTERFACE_LIST_ENTRY)
                                    ExAllocatePool2(POOL_FLAG_NON_PAGED,
                                        sizeof(USBD_INTERFACE_LIST_ENTRY) * 2,
                                        POOL_TAG);
                                if (ifaceList != NULL) {
                                    ifaceList[0].InterfaceDescriptor = ifaceDesc;
                                    ifaceList[0].Interface = NULL;
                                    ifaceList[1].InterfaceDescriptor = NULL;
                                    ifaceList[1].Interface = NULL;

                                    configUrb = USBD_CreateConfigurationRequestEx(
                                                    configDesc, ifaceList);
                                    if (configUrb != NULL) {
                                        KdPrint(("USBD_CreateConfigurationRequestEx "
                                                 "URB created, length=%lu\n",
                                                 configUrb->UrbHeader.Length));
                                        ExFreePoolWithTag(configUrb, 0);
                                    }
                                    ExFreePoolWithTag(ifaceList, POOL_TAG);
                                }
                            }
                        }
                    }
                    ExFreePoolWithTag(configDesc, POOL_TAG);
                }
            }
        }
    }

    //
    // ---- usbdlib.h DDI coverage: GET_ISO_URB_SIZE, UsbBuildGetStatusRequest,
    //      UsbBuildInterruptOrBulkTransferRequest ----
    // Demonstrate the macro-based URB builders (build URBs but do not submit).
    //
    {
        //
        // GET_ISO_URB_SIZE: Calculate the size needed for an isochronous URB.
        //
        ULONG isoUrbSize;
        isoUrbSize = GET_ISO_URB_SIZE(8); // 8 packets
        KdPrint(("ISO URB size for 8 packets: %lu bytes\n", isoUrbSize));
    }

    {
        //
        // UsbBuildGetStatusRequest: Format a URB to get device status.
        //
        URB statusUrb;
        USHORT usbStatus = 0;
        RtlZeroMemory(&statusUrb, sizeof(URB));
        UsbBuildGetStatusRequest(
            &statusUrb,
            URB_FUNCTION_GET_STATUS_FROM_DEVICE,
            0, // Index
            &usbStatus,
            NULL, // Link
            NULL); // TransferFlags not used by this macro
        KdPrint(("UsbBuildGetStatusRequest: URB function=0x%x, length=%d\n",
                 statusUrb.UrbHeader.Function,
                 statusUrb.UrbHeader.Length));
    }

    {
        //
        // UsbBuildInterruptOrBulkTransferRequest: Format a bulk transfer URB.
        //
        URB bulkUrb;
        UCHAR dummyBuffer[64];
        RtlZeroMemory(&bulkUrb, sizeof(URB));
        RtlZeroMemory(dummyBuffer, sizeof(dummyBuffer));
        UsbBuildInterruptOrBulkTransferRequest(
            &bulkUrb,
            sizeof(struct _URB_BULK_OR_INTERRUPT_TRANSFER),
            NULL, // PipeHandle — NULL since we're just demonstrating the build
            dummyBuffer,
            NULL, // TransferBufferMDL
            sizeof(dummyBuffer),
            USBD_TRANSFER_DIRECTION_IN | USBD_SHORT_TRANSFER_OK,
            NULL); // Link
        KdPrint(("UsbBuildInterruptOrBulkTransferRequest: URB function=0x%x\n",
                 bulkUrb.UrbHeader.Function));
    }

    //
    // Call helper to demonstrate URB-based DDIs from usbdlib.h and wdfusb.h.
    //
    OsrFxDemonstrateUrbDdis(Device, pDeviceContext);

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

    //
    // ---- wdfusb.h DDI coverage: WDF_USB_CONTROL_SETUP_PACKET_INIT_GET_STATUS ----
    // Retrieve the USB device status (2 bytes per USB spec).
    //
    case IOCTL_OSRUSBFX2_GET_USB_STATUS:
    {
        WDF_USB_CONTROL_SETUP_PACKET statusPacket;
        USHORT usbDeviceStatus = 0;
        WDF_MEMORY_DESCRIPTOR statusMemDesc;

        if (OutputBufferLength < sizeof(USHORT)) {
            status = STATUS_BUFFER_TOO_SMALL;
            break;
        }

        WDF_USB_CONTROL_SETUP_PACKET_INIT_GET_STATUS(
            &statusPacket,
            BmRequestToDevice,
            0); // Index

        WDF_MEMORY_DESCRIPTOR_INIT_BUFFER(&statusMemDesc,
                                          &usbDeviceStatus,
                                          sizeof(usbDeviceStatus));

        WDF_REQUEST_SEND_OPTIONS_INIT(&sendOptions,
                                      WDF_REQUEST_SEND_OPTION_TIMEOUT);
        WDF_REQUEST_SEND_OPTIONS_SET_TIMEOUT(&sendOptions,
                                             WDF_REL_TIMEOUT_IN_MS(500));

        status = WdfUsbTargetDeviceSendControlTransferSynchronously(
                     pDevContext->UsbDevice, NULL, &sendOptions,
                     &statusPacket, &statusMemDesc,
                     (PULONG)&bytesTransferred);
        if (NT_SUCCESS(status)) {
            //
            // Copy the status back to the output buffer.
            //
            PUSHORT outputBuf;
            NTSTATUS bufStatus;
            bufStatus = WdfRequestRetrieveOutputBuffer(
                            Request, sizeof(USHORT), (PVOID*)&outputBuf, NULL);
            if (NT_SUCCESS(bufStatus)) {
                *outputBuf = usbDeviceStatus;
                bytesTransferred = sizeof(USHORT);
            }
            KdPrint(("USB device status: 0x%04X\n", usbDeviceStatus));
        }
        break;
    }

    //
    // ---- wdfusb.h DDI coverage: WDF_USB_CONTROL_SETUP_PACKET_INIT_FEATURE ----
    // Send a SET_FEATURE request to the device.
    //
    case IOCTL_OSRUSBFX2_SET_FEATURE:
    {
        WDF_USB_CONTROL_SETUP_PACKET featurePacket;
        USHORT featureSelector = 0;
        PUSHORT inputBuf;

        if (InputBufferLength < sizeof(USHORT)) {
            status = STATUS_BUFFER_TOO_SMALL;
            break;
        }

        status = WdfRequestRetrieveInputBuffer(
                     Request, sizeof(USHORT), (PVOID*)&inputBuf, NULL);
        if (!NT_SUCCESS(status)) break;
        featureSelector = *inputBuf;

        WDF_USB_CONTROL_SETUP_PACKET_INIT_FEATURE(
            &featurePacket,
            BmRequestToDevice,
            featureSelector,
            0,      // Index
            TRUE);  // SetFeature = TRUE

        WDF_REQUEST_SEND_OPTIONS_INIT(&sendOptions,
                                      WDF_REQUEST_SEND_OPTION_TIMEOUT);
        WDF_REQUEST_SEND_OPTIONS_SET_TIMEOUT(&sendOptions,
                                             WDF_REL_TIMEOUT_IN_MS(500));

        status = WdfUsbTargetDeviceSendControlTransferSynchronously(
                     pDevContext->UsbDevice, NULL, &sendOptions,
                     &featurePacket, NULL, NULL);
        KdPrint(("SET_FEATURE(%d) %!STATUS!\n", featureSelector, status));
        break;
    }

    //
    // ---- wdfusb.h DDI coverage: WDF_USB_CONTROL_SETUP_PACKET_INIT_FEATURE ----
    // Send a CLEAR_FEATURE request (SetFeature = FALSE).
    //
    case IOCTL_OSRUSBFX2_CLEAR_FEATURE:
    {
        WDF_USB_CONTROL_SETUP_PACKET clearPacket;
        USHORT featureSelector = 0;
        PUSHORT inputBuf;

        if (InputBufferLength < sizeof(USHORT)) {
            status = STATUS_BUFFER_TOO_SMALL;
            break;
        }

        status = WdfRequestRetrieveInputBuffer(
                     Request, sizeof(USHORT), (PVOID*)&inputBuf, NULL);
        if (!NT_SUCCESS(status)) break;
        featureSelector = *inputBuf;

        WDF_USB_CONTROL_SETUP_PACKET_INIT_FEATURE(
            &clearPacket,
            BmRequestToDevice,
            featureSelector,
            0,       // Index
            FALSE);  // SetFeature = FALSE => CLEAR_FEATURE

        WDF_REQUEST_SEND_OPTIONS_INIT(&sendOptions,
                                      WDF_REQUEST_SEND_OPTION_TIMEOUT);
        WDF_REQUEST_SEND_OPTIONS_SET_TIMEOUT(&sendOptions,
                                             WDF_REL_TIMEOUT_IN_MS(500));

        status = WdfUsbTargetDeviceSendControlTransferSynchronously(
                     pDevContext->UsbDevice, NULL, &sendOptions,
                     &clearPacket, NULL, NULL);
        KdPrint(("CLEAR_FEATURE(%d) %!STATUS!\n", featureSelector, status));
        break;
    }

    //
    // ---- wdfusb.h DDI coverage: WdfUsbTargetDeviceFormatRequestForString ----
    // Asynchronously retrieve a USB string descriptor.
    //
    case IOCTL_OSRUSBFX2_GET_STRING:
    {
        WDFMEMORY outputMem;
        WDFREQUEST stringRequest;
        WDF_OBJECT_ATTRIBUTES reqAttributes;

        if (OutputBufferLength < sizeof(WCHAR)) {
            status = STATUS_BUFFER_TOO_SMALL;
            break;
        }

        status = WdfRequestRetrieveOutputMemory(Request, &outputMem);
        if (!NT_SUCCESS(status)) break;

        WDF_OBJECT_ATTRIBUTES_INIT(&reqAttributes);
        reqAttributes.ParentObject = device;
        status = WdfRequestCreate(&reqAttributes, 
                     WdfUsbTargetDeviceGetIoTarget(pDevContext->UsbDevice),
                     &stringRequest);
        if (!NT_SUCCESS(status)) break;

        status = WdfUsbTargetDeviceFormatRequestForString(
                     pDevContext->UsbDevice,
                     stringRequest,
                     outputMem,
                     NULL,  // Offset
                     2,     // StringIndex (product string)
                     0x0409); // LANGID: English (US)
        if (NT_SUCCESS(status)) {
            //
            // Send synchronously for simplicity in this demo.
            //
            WDF_REQUEST_SEND_OPTIONS_INIT(&sendOptions,
                                          WDF_REQUEST_SEND_OPTION_SYNCHRONOUS |
                                          WDF_REQUEST_SEND_OPTION_TIMEOUT);
            WDF_REQUEST_SEND_OPTIONS_SET_TIMEOUT(&sendOptions,
                                                 WDF_REL_TIMEOUT_IN_MS(1000));
            if (WdfRequestSend(stringRequest,
                    WdfUsbTargetDeviceGetIoTarget(pDevContext->UsbDevice),
                    &sendOptions)) {
                status = WdfRequestGetStatus(stringRequest);
                if (NT_SUCCESS(status)) {
                    bytesTransferred = WdfRequestGetInformation(stringRequest);
                }
            } else {
                status = WdfRequestGetStatus(stringRequest);
            }
            KdPrint(("FormatRequestForString: %!STATUS!, %Iu bytes\n",
                     status, bytesTransferred));
        }
        WdfObjectDelete(stringRequest);
        break;
    }

    //
    // ---- wdfusb.h DDI coverage: WdfUsbTargetDeviceFormatRequestForCyclePort ----
    // Cycle the USB port (simulates unplug/replug).
    //
    case IOCTL_OSRUSBFX2_CYCLE_PORT:
    {
        WDFREQUEST cycleRequest;
        WDF_OBJECT_ATTRIBUTES reqAttributes;

        WDF_OBJECT_ATTRIBUTES_INIT(&reqAttributes);
        reqAttributes.ParentObject = device;
        status = WdfRequestCreate(&reqAttributes,
                     WdfUsbTargetDeviceGetIoTarget(pDevContext->UsbDevice),
                     &cycleRequest);
        if (!NT_SUCCESS(status)) break;

        status = WdfUsbTargetDeviceFormatRequestForCyclePort(
                     pDevContext->UsbDevice, cycleRequest);
        if (NT_SUCCESS(status)) {
            WDF_REQUEST_SEND_OPTIONS_INIT(&sendOptions,
                                          WDF_REQUEST_SEND_OPTION_SYNCHRONOUS |
                                          WDF_REQUEST_SEND_OPTION_TIMEOUT);
            WDF_REQUEST_SEND_OPTIONS_SET_TIMEOUT(&sendOptions,
                                                 WDF_REL_TIMEOUT_IN_MS(5000));
            if (WdfRequestSend(cycleRequest,
                    WdfUsbTargetDeviceGetIoTarget(pDevContext->UsbDevice),
                    &sendOptions)) {
                status = WdfRequestGetStatus(cycleRequest);
            } else {
                status = WdfRequestGetStatus(cycleRequest);
            }
        }
        KdPrint(("CyclePort: %!STATUS!\n", status));
        WdfObjectDelete(cycleRequest);
        break;
    }

    //
    // ---- wdfusb.h DDI coverage: WdfUsbTargetPipeFormatRequestForReset ----
    // Reset the bulk read pipe.
    //
    case IOCTL_OSRUSBFX2_RESET_PIPE:
    {
        WDFREQUEST resetRequest;
        WDF_OBJECT_ATTRIBUTES reqAttributes;

        WDF_OBJECT_ATTRIBUTES_INIT(&reqAttributes);
        reqAttributes.ParentObject = device;
        status = WdfRequestCreate(&reqAttributes,
                     WdfUsbTargetPipeGetIoTarget(pDevContext->BulkReadPipe),
                     &resetRequest);
        if (!NT_SUCCESS(status)) break;

        status = WdfUsbTargetPipeFormatRequestForReset(
                     pDevContext->BulkReadPipe, resetRequest);
        if (NT_SUCCESS(status)) {
            WDF_REQUEST_SEND_OPTIONS_INIT(&sendOptions,
                                          WDF_REQUEST_SEND_OPTION_SYNCHRONOUS |
                                          WDF_REQUEST_SEND_OPTION_TIMEOUT);
            WDF_REQUEST_SEND_OPTIONS_SET_TIMEOUT(&sendOptions,
                                                 WDF_REL_TIMEOUT_IN_MS(2000));
            if (WdfRequestSend(resetRequest,
                    WdfUsbTargetPipeGetIoTarget(pDevContext->BulkReadPipe),
                    &sendOptions)) {
                status = WdfRequestGetStatus(resetRequest);
            } else {
                status = WdfRequestGetStatus(resetRequest);
            }
        }
        KdPrint(("ResetPipe: %!STATUS!\n", status));
        WdfObjectDelete(resetRequest);
        break;
    }

    //
    // ---- wdfusb.h DDI coverage: WdfUsbTargetPipeFormatRequestForAbort ----
    // Abort pending transfers on the bulk read pipe.
    //
    case IOCTL_OSRUSBFX2_ABORT_PIPE:
    {
        WDFREQUEST abortRequest;
        WDF_OBJECT_ATTRIBUTES reqAttributes;

        WDF_OBJECT_ATTRIBUTES_INIT(&reqAttributes);
        reqAttributes.ParentObject = device;
        status = WdfRequestCreate(&reqAttributes,
                     WdfUsbTargetPipeGetIoTarget(pDevContext->BulkReadPipe),
                     &abortRequest);
        if (!NT_SUCCESS(status)) break;

        status = WdfUsbTargetPipeFormatRequestForAbort(
                     pDevContext->BulkReadPipe, abortRequest);
        if (NT_SUCCESS(status)) {
            WDF_REQUEST_SEND_OPTIONS_INIT(&sendOptions,
                                          WDF_REQUEST_SEND_OPTION_SYNCHRONOUS |
                                          WDF_REQUEST_SEND_OPTION_TIMEOUT);
            WDF_REQUEST_SEND_OPTIONS_SET_TIMEOUT(&sendOptions,
                                                 WDF_REL_TIMEOUT_IN_MS(2000));
            if (WdfRequestSend(abortRequest,
                    WdfUsbTargetPipeGetIoTarget(pDevContext->BulkReadPipe),
                    &sendOptions)) {
                status = WdfRequestGetStatus(abortRequest);
            } else {
                status = WdfRequestGetStatus(abortRequest);
            }
        }
        KdPrint(("AbortPipe: %!STATUS!\n", status));
        WdfObjectDelete(abortRequest);
        break;
    }

    //
    // ---- wdfusb.h DDI coverage: WdfUsbTargetPipeWriteSynchronously ----
    // Synchronous write to the bulk write pipe with a timeout.
    //
    case IOCTL_OSRUSBFX2_WRITE_SYNC:
    {
        WDF_MEMORY_DESCRIPTOR writeMemDesc;

        if (InputBufferLength == 0) {
            status = STATUS_BUFFER_TOO_SMALL;
            break;
        }

        status = WdfRequestRetrieveInputMemory(Request, &memory);
        if (!NT_SUCCESS(status)) break;

        WDF_MEMORY_DESCRIPTOR_INIT_HANDLE(&writeMemDesc, memory, NULL);
        WDF_REQUEST_SEND_OPTIONS_INIT(&sendOptions,
                                      WDF_REQUEST_SEND_OPTION_TIMEOUT);
        WDF_REQUEST_SEND_OPTIONS_SET_TIMEOUT(&sendOptions,
                                             WDF_REL_TIMEOUT_IN_MS(1000));

        status = WdfUsbTargetPipeWriteSynchronously(
                     pDevContext->BulkWritePipe,
                     NULL,         // Optional WDFREQUEST
                     &sendOptions,
                     &writeMemDesc,
                     (PULONG)&bytesTransferred);
        KdPrint(("WriteSynchronously: %!STATUS!, %Iu bytes\n",
                 status, bytesTransferred));
        break;
    }

    //
    // ---- Standard test app IOCTLs ----
    //

    case IOCTL_OSRUSBFX2_GET_CONFIG_DESCRIPTOR:
    {
        PUSB_CONFIGURATION_DESCRIPTOR configDesc = NULL;
        USHORT requiredSize = 0;

        status = WdfUsbTargetDeviceRetrieveConfigDescriptor(
                     pDevContext->UsbDevice, NULL, &requiredSize);
        if (status != STATUS_BUFFER_TOO_SMALL) {
            KdPrint(("RetrieveConfigDescriptor failed %!STATUS!\n", status));
            break;
        }

        status = WdfRequestRetrieveOutputBuffer(Request,
                     (size_t)requiredSize, (PVOID*)&configDesc, NULL);
        if (!NT_SUCCESS(status)) {
            KdPrint(("Output buffer too small for config descriptor\n"));
            break;
        }

        status = WdfUsbTargetDeviceRetrieveConfigDescriptor(
                     pDevContext->UsbDevice, configDesc, &requiredSize);
        if (NT_SUCCESS(status)) {
            bytesTransferred = requiredSize;
        }
        break;
    }

    case IOCTL_OSRUSBFX2_RESET_DEVICE:
        status = WdfUsbTargetDeviceResetPortSynchronously(pDevContext->UsbDevice);
        if (!NT_SUCCESS(status)) {
            KdPrint(("ResetDevice failed %!STATUS!\n", status));
        }
        break;

    case IOCTL_OSRUSBFX2_REENUMERATE_DEVICE:
    {
        WDF_USB_CONTROL_SETUP_PACKET reenumPacket;

        WDF_USB_CONTROL_SETUP_PACKET_INIT_VENDOR(&reenumPacket,
                                        BmRequestHostToDevice,
                                        BmRequestToDevice,
                                        USBFX2LK_REENUMERATE,
                                        0, 0);

        WDF_REQUEST_SEND_OPTIONS_INIT(&sendOptions,
                                      WDF_REQUEST_SEND_OPTION_TIMEOUT);
        WDF_REQUEST_SEND_OPTIONS_SET_TIMEOUT(&sendOptions,
                                             WDF_REL_TIMEOUT_IN_MS(5000));

        status = WdfUsbTargetDeviceSendControlTransferSynchronously(
                     pDevContext->UsbDevice, NULL, &sendOptions,
                     &reenumPacket, NULL, NULL);
        KdPrint(("Reenumerate: %!STATUS!\n", status));
        break;
    }

    case IOCTL_OSRUSBFX2_GET_BAR_GRAPH_DISPLAY:
    {
        PBAR_GRAPH_STATE barGraphState;
        WDF_MEMORY_DESCRIPTOR barMemDesc;
        ULONG transferred;

        if (OutputBufferLength < sizeof(BAR_GRAPH_STATE)) {
            status = STATUS_BUFFER_TOO_SMALL;
            break;
        }

        status = WdfRequestRetrieveOutputBuffer(Request,
                     sizeof(BAR_GRAPH_STATE), (PVOID*)&barGraphState, NULL);
        if (!NT_SUCCESS(status)) break;

        barGraphState->BarsAsUChar = 0;

        WDF_USB_CONTROL_SETUP_PACKET_INIT_VENDOR(&controlSetupPacket,
                                        BmRequestDeviceToHost,
                                        BmRequestToDevice,
                                        USBFX2LK_READ_BARGRAPH_DISPLAY,
                                        0, 0);

        WDF_MEMORY_DESCRIPTOR_INIT_BUFFER(&barMemDesc,
                     barGraphState, sizeof(BAR_GRAPH_STATE));

        WDF_REQUEST_SEND_OPTIONS_INIT(&sendOptions,
                                      WDF_REQUEST_SEND_OPTION_TIMEOUT);
        WDF_REQUEST_SEND_OPTIONS_SET_TIMEOUT(&sendOptions,
                                             WDF_REL_TIMEOUT_IN_MS(500));

        status = WdfUsbTargetDeviceSendControlTransferSynchronously(
                     pDevContext->UsbDevice, NULL, &sendOptions,
                     &controlSetupPacket, &barMemDesc, &transferred);
        if (NT_SUCCESS(status)) {
            bytesTransferred = sizeof(BAR_GRAPH_STATE);
            KdPrint(("GetBarGraph: 0x%x\n", barGraphState->BarsAsUChar));
        }
        break;
    }

    case IOCTL_OSRUSBFX2_READ_SWITCHES:
    {
        PSWITCH_STATE switchState;
        WDF_MEMORY_DESCRIPTOR switchMemDesc;
        ULONG transferred;

        if (OutputBufferLength < sizeof(SWITCH_STATE)) {
            status = STATUS_BUFFER_TOO_SMALL;
            break;
        }

        status = WdfRequestRetrieveOutputBuffer(Request,
                     sizeof(SWITCH_STATE), (PVOID*)&switchState, NULL);
        if (!NT_SUCCESS(status)) break;

        switchState->SwitchesAsUChar = 0;

        WDF_USB_CONTROL_SETUP_PACKET_INIT_VENDOR(&controlSetupPacket,
                                        BmRequestDeviceToHost,
                                        BmRequestToDevice,
                                        USBFX2LK_READ_SWITCHES,
                                        0, 0);

        WDF_MEMORY_DESCRIPTOR_INIT_BUFFER(&switchMemDesc,
                     switchState, sizeof(SWITCH_STATE));

        WDF_REQUEST_SEND_OPTIONS_INIT(&sendOptions,
                                      WDF_REQUEST_SEND_OPTION_TIMEOUT);
        WDF_REQUEST_SEND_OPTIONS_SET_TIMEOUT(&sendOptions,
                                             WDF_REL_TIMEOUT_IN_MS(500));

        status = WdfUsbTargetDeviceSendControlTransferSynchronously(
                     pDevContext->UsbDevice, NULL, &sendOptions,
                     &controlSetupPacket, &switchMemDesc, &transferred);
        if (NT_SUCCESS(status)) {
            bytesTransferred = sizeof(SWITCH_STATE);
            KdPrint(("ReadSwitches: 0x%x\n", switchState->SwitchesAsUChar));
        }
        break;
    }

    case IOCTL_OSRUSBFX2_GET_7_SEGMENT_DISPLAY:
    {
        PUCHAR sevenSegment;
        WDF_MEMORY_DESCRIPTOR segMemDesc;
        ULONG transferred;

        if (OutputBufferLength < sizeof(UCHAR)) {
            status = STATUS_BUFFER_TOO_SMALL;
            break;
        }

        status = WdfRequestRetrieveOutputBuffer(Request,
                     sizeof(UCHAR), (PVOID*)&sevenSegment, NULL);
        if (!NT_SUCCESS(status)) break;

        *sevenSegment = 0;

        WDF_USB_CONTROL_SETUP_PACKET_INIT_VENDOR(&controlSetupPacket,
                                        BmRequestDeviceToHost,
                                        BmRequestToDevice,
                                        USBFX2LK_READ_7SEGMENT_DISPLAY,
                                        0, 0);

        WDF_MEMORY_DESCRIPTOR_INIT_BUFFER(&segMemDesc,
                     sevenSegment, sizeof(UCHAR));

        WDF_REQUEST_SEND_OPTIONS_INIT(&sendOptions,
                                      WDF_REQUEST_SEND_OPTION_TIMEOUT);
        WDF_REQUEST_SEND_OPTIONS_SET_TIMEOUT(&sendOptions,
                                             WDF_REL_TIMEOUT_IN_MS(500));

        status = WdfUsbTargetDeviceSendControlTransferSynchronously(
                     pDevContext->UsbDevice, NULL, &sendOptions,
                     &controlSetupPacket, &segMemDesc, &transferred);
        if (NT_SUCCESS(status)) {
            bytesTransferred = sizeof(UCHAR);
            KdPrint(("Get7Segment: 0x%x\n", *sevenSegment));
        }
        break;
    }

    case IOCTL_OSRUSBFX2_SET_7_SEGMENT_DISPLAY:
    {
        PUCHAR sevenSegment;
        WDF_MEMORY_DESCRIPTOR segMemDesc;
        ULONG transferred;

        if (InputBufferLength < sizeof(UCHAR)) {
            status = STATUS_BUFFER_TOO_SMALL;
            break;
        }

        status = WdfRequestRetrieveInputBuffer(Request,
                     sizeof(UCHAR), (PVOID*)&sevenSegment, NULL);
        if (!NT_SUCCESS(status)) break;

        WDF_USB_CONTROL_SETUP_PACKET_INIT_VENDOR(&controlSetupPacket,
                                        BmRequestHostToDevice,
                                        BmRequestToDevice,
                                        USBFX2LK_SET_7SEGMENT_DISPLAY,
                                        0, 0);

        WDF_MEMORY_DESCRIPTOR_INIT_BUFFER(&segMemDesc,
                     sevenSegment, sizeof(UCHAR));

        WDF_REQUEST_SEND_OPTIONS_INIT(&sendOptions,
                                      WDF_REQUEST_SEND_OPTION_TIMEOUT);
        WDF_REQUEST_SEND_OPTIONS_SET_TIMEOUT(&sendOptions,
                                             WDF_REL_TIMEOUT_IN_MS(500));

        status = WdfUsbTargetDeviceSendControlTransferSynchronously(
                     pDevContext->UsbDevice, NULL, &sendOptions,
                     &controlSetupPacket, &segMemDesc, &transferred);
        KdPrint(("Set7Segment: %!STATUS!\n", status));
        break;
    }

    case IOCTL_OSRUSBFX2_GET_INTERRUPT_MESSAGE:
        //
        // The interrupt message IOCTL requires an interrupt endpoint queue.
        // For simplicity, complete with STATUS_NOT_SUPPORTED.
        //
        status = STATUS_NOT_SUPPORTED;
        KdPrint(("GET_INTERRUPT_MESSAGE: not implemented in step5\n"));
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

//
// EvtDeviceReleaseHardware: Clean up USBD handle and demonstrate remaining DDIs.
//
NTSTATUS
EvtDeviceReleaseHardware(
    IN WDFDEVICE    Device,
    IN WDFCMRESLIST ResourceListTranslated
    )
{
    PDEVICE_CONTEXT pDeviceContext;

    UNREFERENCED_PARAMETER(ResourceListTranslated);

    pDeviceContext = GetDeviceContext(Device);

    //
    // ---- usbdlib.h DDI coverage: USBD_CloseHandle ----
    // Close the USBD handle obtained during PrepareHardware.
    //
    if (pDeviceContext->UsbdHandle != NULL) {
        USBD_CloseHandle(pDeviceContext->UsbdHandle);
        pDeviceContext->UsbdHandle = NULL;
        KdPrint(("USBD_CloseHandle completed\n"));
    }

    return STATUS_SUCCESS;
}

//
// OsrFxDemonstrateUrbDdis: Shows usbdlib.h and wdfusb.h URB-based DDI usage.
// Called from EvtDevicePrepareHardware context but factored out for readability.
// These DDIs build and format URBs but do not submit them to hardware.
//
VOID
OsrFxDemonstrateUrbDdis(
    IN WDFDEVICE    Device,
    IN PDEVICE_CONTEXT pDeviceContext
    )
{
    NTSTATUS status;

    UNREFERENCED_PARAMETER(Device);

    //
    // ---- wdfusb.h DDI coverage: WDF_USB_CONTROL_SETUP_PACKET_INIT ----
    // Initialize a raw USB control setup packet (generic form).
    //
    {
        WDF_USB_CONTROL_SETUP_PACKET rawPacket;
        WDF_USB_CONTROL_SETUP_PACKET_INIT(
            &rawPacket,
            BmRequestDeviceToHost,   // Direction
            BmRequestToDevice,       // Recipient
            USB_REQUEST_GET_STATUS,  // Request
            0,                       // Value
            0);                      // Index
        KdPrint(("WDF_USB_CONTROL_SETUP_PACKET_INIT: bmRequestType=0x%02x\n",
                 rawPacket.Packet.bm.Request.Type));
    }

    //
    // ---- wdfusb.h DDI coverage: WDF_USB_DEVICE_SELECT_CONFIG_PARAMS_INIT_DECONFIG ----
    // Demonstrate building a deconfiguration request params structure.
    //
    {
        WDF_USB_DEVICE_SELECT_CONFIG_PARAMS deconfigParams;
        WDF_USB_DEVICE_SELECT_CONFIG_PARAMS_INIT_DECONFIG(&deconfigParams);
        KdPrint(("INIT_DECONFIG: Type=%d\n", deconfigParams.Type));
    }

    //
    // ---- wdfusb.h DDI coverage: WDF_USB_DEVICE_SELECT_CONFIG_PARAMS_INIT_URB ----
    // Demonstrate building a select-config-by-URB params structure.
    //
    {
        WDF_USB_DEVICE_SELECT_CONFIG_PARAMS urbConfigParams;
        URB dummyUrb;
        RtlZeroMemory(&dummyUrb, sizeof(URB));
        WDF_USB_DEVICE_SELECT_CONFIG_PARAMS_INIT_URB(
            &urbConfigParams, &dummyUrb);
        KdPrint(("INIT_URB: Type=%d\n", urbConfigParams.Type));
    }

    //
    // ---- wdfusb.h DDI coverage: WDF_USB_INTERFACE_SELECT_SETTING_PARAMS_INIT_URB ----
    // Demonstrate building a select-interface-setting-by-URB params structure.
    //
    {
        WDF_USB_INTERFACE_SELECT_SETTING_PARAMS settingParams;
        URB dummyUrb;
        RtlZeroMemory(&dummyUrb, sizeof(URB));
        WDF_USB_INTERFACE_SELECT_SETTING_PARAMS_INIT_URB(
            &settingParams, &dummyUrb);
        KdPrint(("SELECT_SETTING_INIT_URB: Type=%d\n", settingParams.Type));
    }

    //
    // ---- wdfusb.h DDI coverage: WdfUsbTargetDeviceSendUrbSynchronously ----
    // Send a GET_STATUS URB synchronously to the USB device.
    // Note: USBD_CLIENT_CONTRACT_VERSION_602 requires URBs allocated via USBD_UrbAllocate.
    //
    if (pDeviceContext->UsbdHandle != NULL) {
        PURB statusUrb = NULL;
        USHORT deviceStatus = 0;
        WDF_REQUEST_SEND_OPTIONS sendOptions;

        status = USBD_UrbAllocate(pDeviceContext->UsbdHandle, &statusUrb);
        if (NT_SUCCESS(status) && statusUrb != NULL) {
            UsbBuildGetStatusRequest(
                statusUrb,
                URB_FUNCTION_GET_STATUS_FROM_DEVICE,
                0,
                &deviceStatus,
                NULL,
                NULL);

            WDF_REQUEST_SEND_OPTIONS_INIT(&sendOptions,
                                          WDF_REQUEST_SEND_OPTION_TIMEOUT);
            WDF_REQUEST_SEND_OPTIONS_SET_TIMEOUT(&sendOptions,
                                                 WDF_REL_TIMEOUT_IN_MS(1000));

            status = WdfUsbTargetDeviceSendUrbSynchronously(
                         pDeviceContext->UsbDevice,
                         NULL,
                         &sendOptions,
                         statusUrb);
            KdPrint(("SendUrbSynchronously (GET_STATUS): %!STATUS!, status=0x%04X\n",
                     status, deviceStatus));
            USBD_UrbFree(pDeviceContext->UsbdHandle, statusUrb);
        }
    }

    //
    // ---- wdfusb.h DDI coverage: WdfUsbTargetDeviceFormatRequestForUrb ----
    // Format a request for a raw URB submission (async pattern demo).
    // Note: USBD_CLIENT_CONTRACT_VERSION_602 requires URBs allocated via USBD_UrbAllocate.
    //
    if (pDeviceContext->UsbdHandle != NULL) {
        PURB getStatusUrb = NULL;
        USHORT devStatus = 0;
        WDFREQUEST urbRequest;
        WDFMEMORY urbMemory;
        WDF_OBJECT_ATTRIBUTES reqAttributes;
        WDF_REQUEST_SEND_OPTIONS sendOptions;

        status = USBD_UrbAllocate(pDeviceContext->UsbdHandle, &getStatusUrb);
        if (NT_SUCCESS(status) && getStatusUrb != NULL) {
            UsbBuildGetStatusRequest(
                getStatusUrb,
                URB_FUNCTION_GET_STATUS_FROM_DEVICE,
                0,
                &devStatus,
                NULL,
                NULL);

            WDF_OBJECT_ATTRIBUTES_INIT(&reqAttributes);
            status = WdfRequestCreate(&reqAttributes,
                         WdfUsbTargetDeviceGetIoTarget(pDeviceContext->UsbDevice),
                         &urbRequest);
            if (NT_SUCCESS(status)) {
                WDF_OBJECT_ATTRIBUTES memAttributes;
                WDF_OBJECT_ATTRIBUTES_INIT(&memAttributes);
                status = WdfMemoryCreatePreallocated(
                             &memAttributes, getStatusUrb, sizeof(URB), &urbMemory);
                if (NT_SUCCESS(status)) {
                    status = WdfUsbTargetDeviceFormatRequestForUrb(
                                 pDeviceContext->UsbDevice,
                                 urbRequest,
                                 urbMemory,
                                 NULL); // Offset
                    if (NT_SUCCESS(status)) {
                        WDF_REQUEST_SEND_OPTIONS_INIT(&sendOptions,
                                                      WDF_REQUEST_SEND_OPTION_SYNCHRONOUS |
                                                      WDF_REQUEST_SEND_OPTION_TIMEOUT);
                        WDF_REQUEST_SEND_OPTIONS_SET_TIMEOUT(&sendOptions,
                                                             WDF_REL_TIMEOUT_IN_MS(1000));
                        if (WdfRequestSend(urbRequest,
                                WdfUsbTargetDeviceGetIoTarget(pDeviceContext->UsbDevice),
                                &sendOptions)) {
                            status = WdfRequestGetStatus(urbRequest);
                        } else {
                            status = WdfRequestGetStatus(urbRequest);
                        }
                        KdPrint(("FormatRequestForUrb: %!STATUS!\n", status));
                    }
                    WdfObjectDelete(urbMemory);
                }
                WdfObjectDelete(urbRequest);
            }
            USBD_UrbFree(pDeviceContext->UsbdHandle, getStatusUrb);
        }
    }

    //
    // ---- usbdlib.h DDI coverage: USBD_AssignUrbToIoStackLocation ----
    // Demonstrate assigning a URB to an IRP's next stack location.
    // We build an IRP, assign the URB, then cancel/free (no actual submission).
    //
    {
        PIRP irp;
        PIO_STACK_LOCATION nextStack;
        URB dummyUrb;
        PDEVICE_OBJECT targetDevObj;

        targetDevObj = WdfDeviceWdmGetAttachedDevice(Device);
        RtlZeroMemory(&dummyUrb, sizeof(URB));
        dummyUrb.UrbHeader.Length = sizeof(struct _URB_CONTROL_GET_STATUS_REQUEST);
        dummyUrb.UrbHeader.Function = URB_FUNCTION_GET_STATUS_FROM_DEVICE;

        irp = IoAllocateIrp(targetDevObj->StackSize, FALSE);
        if (irp != NULL) {
            nextStack = IoGetNextIrpStackLocation(irp);
            nextStack->MajorFunction = IRP_MJ_INTERNAL_DEVICE_CONTROL;
            //
            // USBD_AssignUrbToIoStackLocation sets the URB pointer in the
            // stack location. This is the recommended way for USBD_CLIENT_CONTRACT_VERSION_602.
            //
            USBD_AssignUrbToIoStackLocation(
                pDeviceContext->UsbdHandle, nextStack, &dummyUrb);
            KdPrint(("USBD_AssignUrbToIoStackLocation: URB assigned to IRP stack\n"));
            IoFreeIrp(irp);
        }
    }

    //
    // ---- usbdlib.h DDI coverage: USBD_SelectConfigUrbAllocateAndBuild,
    //      USBD_SelectInterfaceUrbAllocateAndBuild, USBD_UrbAllocate, USBD_UrbFree ----
    // Build select-config and select-interface URBs using the modern allocation APIs.
    //
    if (pDeviceContext->UsbdHandle != NULL) {
        USHORT configDescSize = 0;
        NTSTATUS descStatus;

        descStatus = WdfUsbTargetDeviceRetrieveConfigDescriptor(
                         pDeviceContext->UsbDevice, NULL, &configDescSize);
        if (descStatus == STATUS_BUFFER_TOO_SMALL && configDescSize > 0) {
            PUSB_CONFIGURATION_DESCRIPTOR configDesc;
            configDesc = (PUSB_CONFIGURATION_DESCRIPTOR)
                ExAllocatePool2(POOL_FLAG_NON_PAGED, configDescSize, POOL_TAG);
            if (configDesc != NULL) {
                descStatus = WdfUsbTargetDeviceRetrieveConfigDescriptor(
                                 pDeviceContext->UsbDevice, configDesc,
                                 &configDescSize);
                if (NT_SUCCESS(descStatus)) {
                    PUSB_INTERFACE_DESCRIPTOR ifaceDesc;
                    ifaceDesc = USBD_ParseConfigurationDescriptorEx(
                                    configDesc, configDesc,
                                    -1, -1, -1, -1, -1);
                    if (ifaceDesc != NULL) {
                        PUSBD_INTERFACE_LIST_ENTRY ifaceList;
                        PURB selectConfigUrb = NULL;

                        ifaceList = (PUSBD_INTERFACE_LIST_ENTRY)
                            ExAllocatePool2(POOL_FLAG_NON_PAGED,
                                sizeof(USBD_INTERFACE_LIST_ENTRY) * 2, POOL_TAG);
                        if (ifaceList != NULL) {
                            ifaceList[0].InterfaceDescriptor = ifaceDesc;
                            ifaceList[0].Interface = NULL;
                            ifaceList[1].InterfaceDescriptor = NULL;
                            ifaceList[1].Interface = NULL;

                            status = USBD_SelectConfigUrbAllocateAndBuild(
                                         pDeviceContext->UsbdHandle,
                                         configDesc,
                                         ifaceList,
                                         &selectConfigUrb);
                            if (NT_SUCCESS(status) && selectConfigUrb != NULL) {
                                KdPrint(("USBD_SelectConfigUrbAllocateAndBuild: "
                                         "URB length=%lu\n",
                                         selectConfigUrb->UrbHeader.Length));
                                //
                                // Also demonstrate USBD_SelectInterfaceUrbAllocateAndBuild.
                                //
                                if (ifaceList[0].Interface != NULL) {
                                    PURB selectIfaceUrb = NULL;
                                    NTSTATUS ifaceStatus;
                                    ifaceStatus = USBD_SelectInterfaceUrbAllocateAndBuild(
                                                      pDeviceContext->UsbdHandle,
                                                      selectConfigUrb->UrbSelectConfiguration.ConfigurationHandle,
                                                      ifaceList,
                                                      &selectIfaceUrb);
                                    if (NT_SUCCESS(ifaceStatus) && selectIfaceUrb != NULL) {
                                        KdPrint(("USBD_SelectInterfaceUrbAllocateAndBuild: "
                                                 "URB length=%lu\n",
                                                 selectIfaceUrb->UrbHeader.Length));
                                        USBD_UrbFree(pDeviceContext->UsbdHandle,
                                                     selectIfaceUrb);
                                    }
                                }
                                USBD_UrbFree(pDeviceContext->UsbdHandle, selectConfigUrb);
                            }
                            ExFreePoolWithTag(ifaceList, POOL_TAG);
                        }
                    }
                }
                ExFreePoolWithTag(configDesc, POOL_TAG);
            }
        }

        //
        // ---- usbdlib.h DDI coverage: USBD_UrbAllocate / USBD_UrbFree ----
        // Allocate a generic URB using the modern USBD APIs.
        //
        {
            PURB genericUrb = NULL;
            status = USBD_UrbAllocate(pDeviceContext->UsbdHandle, &genericUrb);
            if (NT_SUCCESS(status) && genericUrb != NULL) {
                KdPrint(("USBD_UrbAllocate: allocated URB at %p\n", genericUrb));
                USBD_UrbFree(pDeviceContext->UsbdHandle, genericUrb);
            }
        }

        //
        // ---- usbdlib.h DDI coverage: USBD_IsochUrbAllocate ----
        // Allocate an isochronous URB with 8 packets.
        //
        {
            PURB isoUrb = NULL;
            status = USBD_IsochUrbAllocate(pDeviceContext->UsbdHandle,
                                           8, // NumberOfIsochPackets
                                           &isoUrb);
            if (NT_SUCCESS(status) && isoUrb != NULL) {
                KdPrint(("USBD_IsochUrbAllocate: allocated ISO URB at %p\n", isoUrb));
                USBD_UrbFree(pDeviceContext->UsbdHandle, isoUrb);
            }
        }
    }

    //
    // ---- usbdlib.h DDI coverage: USBD_ValidateConfigurationDescriptor ----
    // Validate the configuration descriptor retrieved from the device.
    //
    {
        USHORT configDescSize = 0;
        NTSTATUS descStatus;

        descStatus = WdfUsbTargetDeviceRetrieveConfigDescriptor(
                         pDeviceContext->UsbDevice, NULL, &configDescSize);
        if (descStatus == STATUS_BUFFER_TOO_SMALL && configDescSize > 0) {
            PUSB_CONFIGURATION_DESCRIPTOR configDesc;
            configDesc = (PUSB_CONFIGURATION_DESCRIPTOR)
                ExAllocatePool2(POOL_FLAG_NON_PAGED, configDescSize, POOL_TAG);
            if (configDesc != NULL) {
                descStatus = WdfUsbTargetDeviceRetrieveConfigDescriptor(
                                 pDeviceContext->UsbDevice, configDesc,
                                 &configDescSize);
                if (NT_SUCCESS(descStatus)) {
                    PUCHAR validationOffset = NULL;
                    descStatus = USBD_ValidateConfigurationDescriptor(
                                     configDesc,
                                     configDescSize,
                                     1, // level
                                     &validationOffset,
                                     POOL_TAG);
                    KdPrint(("USBD_ValidateConfigurationDescriptor: %!STATUS!, offset=%p\n",
                             descStatus, validationOffset));
                }
                ExFreePoolWithTag(configDesc, POOL_TAG);
            }
        }
    }
}
