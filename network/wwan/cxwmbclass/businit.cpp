//
//    Copyright (C) Microsoft.  All rights reserved.
//

#include <precomp.h>

#pragma pack(push, 1)

typedef struct _USB_DESCRIPTOR_HEADER
{

    BYTE bLength;
    BYTE bDescriptorType;

} USB_DESCRIPTOR_HEADER, * PUSB_DESCRIPTOR_HEADER;

#pragma pack(pop)

NTSTATUS
MbbParseConfigDescriptor(
    WDFUSBDEVICE WdfUsbDevice,
    PUSHORT MaxSegmentSize,
    PUCHAR NcmCaps,
    PUCHAR PowerFiltersSupported,
    PUCHAR MaxPowerFilterSize,
    PUCHAR CommunicationClassInterface,
    PUCHAR DataClassInterface,
    PUSHORT MaxControlMessage,
    PUCHAR AltCommunicationClassSetting,
    PUCHAR AltDataClassSetting,
    PUSHORT BulkInDataClassPacketSize,
    PUSHORT MTU,
    PUSHORT MbimVersion,
    PUSHORT MbimExtendedVersion);

NTSTATUS
GetNtbParameters(WDFUSBDEVICE WdfUsbDevice, PNCM_NTB_PARAMETER NcmNtb);

NTSTATUS
GetDeviceString(__in WDFUSBDEVICE UsbDevice, __in UCHAR Index, __out PWSTR* String);

VOID FreeBusObject(_In_ __drv_freesMem(Mem) PBUS_OBJECT BusObject);

NTSTATUS
SetActivityIdForRequest(__in WDFREQUEST Request, __in LPGUID ActivityId);

void ResetDataPipeWorkItem(_In_ WDFWORKITEM WorkItem);

EVT_WDF_USB_READER_COMPLETION_ROUTINE InterruptPipeReadComplete;
EVT_WDF_USB_READERS_FAILED InterruptPipeReadError;

NTSTATUS
MbbBusInitializeByWdf(
    _In_ WDFDEVICE WdfDevice,
    _In_ MBB_BUS_RESPONSE_AVAILABLE_CALLBACK ResponseAvailableCallback,
    _In_ MBB_BUS_DATA_RECEIVE_CALLBACK ReceiveDataCallback,
    _In_ MBB_BUS_SS_IDLE_CONFIRM_CALLBACK IdleConfirmCallback,
    _In_ MBB_BUS_SS_IDLE_NOTIFICATION_COMPLETE_CALLBACK IdleNotificationComplete,
    _In_ MBB_PROTOCOL_HANDLE ProtocolHandle,
    _Outptr_ MBB_BUS_HANDLE* BusHandle,
    _Inout_opt_ MBB_BUS_HANDLE preAllocatedBusObject)
{
    NTSTATUS Status = STATUS_SUCCESS;
    WDF_OBJECT_ATTRIBUTES attributes;

    WDF_USB_DEVICE_SELECT_CONFIG_PARAMS configParams;
    PWDF_USB_INTERFACE_SETTING_PAIR settingPairs = NULL;
    WDF_USB_INTERFACE_SELECT_SETTING_PARAMS SettingParams;
    WDFUSBPIPE pipe;
    WDF_USB_PIPE_INFORMATION pipeInfo;
    WDFUSBINTERFACE UsbInterface = NULL;
    PUSB_DEVICE_CONTEXT usbDeviceContext = NULL;
    USB_DEVICE_DESCRIPTOR DeviceDescriptor;
    USB_INTERFACE_DESCRIPTOR InterfaceDescriptor;
    WDF_USB_DEVICE_CREATE_CONFIG Config;

    WDF_WORKITEM_CONFIG workitemConfig;
    WDF_OBJECT_ATTRIBUTES workitemAttributes;

    UCHAR numInterfaces;
    UCHAR interfaceIndex;
    UCHAR i;
    USHORT DescriptorSize = 0;
    UCHAR CurrentSetting;
    USHORT BulkInPacketSize = 0;
    WDF_USB_CONTINUOUS_READER_CONFIG ReaderConfig;

    PIRP UsbSsIrp = NULL;

    PBUS_OBJECT BusObject = NULL;

    USB_CAP_DEVICE_INFO usbCapDeviceInfo = { USB_CAP_DEVICE_TYPE_MAXIMUM, 0, 0, 0 };
    ULONG capResultLength = 0;

    WDF_USB_DEVICE_INFORMATION deviceInformation;

    *BusHandle = NULL;

    if (preAllocatedBusObject == NULL)
    {

        BusObject = (PBUS_OBJECT)ALLOCATE_NONPAGED_POOL(sizeof(BUS_OBJECT));

        if (BusObject == NULL)
        {
            Status = STATUS_NO_MEMORY;
            goto Cleanup;
        }

        RtlZeroMemory(BusObject, sizeof(*BusObject));

        BusObject->Fdo = WdfDeviceWdmGetDeviceObject(WdfDevice);

        INITIALIZE_PASSIVE_LOCK(&BusObject->Lock);
        BusObject->State = BUS_STATE_CLOSED;

        BusObject->ProtocolHandle = ProtocolHandle;
        BusObject->ResponseAvailableCallback = ResponseAvailableCallback;
        BusObject->ReceiveDataCallback = ReceiveDataCallback;
        BusObject->IdleConfirmCallback = IdleConfirmCallback;
        BusObject->IdleNotificationComplete = IdleNotificationComplete;

    }
    else
    {
        BusObject = (PBUS_OBJECT)preAllocatedBusObject;
    }

    WDF_OBJECT_ATTRIBUTES_INIT_CONTEXT_TYPE(&attributes, USB_DEVICE_CONTEXT);

    WDF_USB_DEVICE_CREATE_CONFIG_INIT(&Config, USBD_CLIENT_CONTRACT_VERSION_602);

    Status = WdfUsbTargetDeviceCreateWithParameters(WdfDevice, &Config, &attributes, &BusObject->WdfUsbDevice);

    if (!NT_SUCCESS(Status))
    {
        goto Cleanup;
    }

    usbDeviceContext = GetUsbDeviceContext(BusObject->WdfUsbDevice);

    usbDeviceContext->BulkPipeResetFlag = FALSE;
    usbDeviceContext->BulkPipeResetWorkitem = NULL;

    //
    //  create a work item to reset data pipe
    //
    ExInitializeRundownProtection(&usbDeviceContext->BulkPipeResetRundown);

    WDF_OBJECT_ATTRIBUTES_INIT(&workitemAttributes);
    workitemAttributes.ParentObject = BusObject->WdfUsbDevice;
    WDF_WORKITEM_CONFIG_INIT(&workitemConfig, ResetDataPipeWorkItem);

    Status = WdfWorkItemCreate(&workitemConfig, &workitemAttributes, &usbDeviceContext->BulkPipeResetWorkitem);
    if (!NT_SUCCESS(Status))
    {
        goto Cleanup;
    }

    //
    //  create a lock
    //
    WDF_OBJECT_ATTRIBUTES_INIT(&attributes);
    attributes.ParentObject = BusObject->WdfUsbDevice;

    Status = WdfWaitLockCreate(&attributes, &usbDeviceContext->PipeStateLock);

    if (!NT_SUCCESS(Status))
    {
        return Status;
    }

    WDF_OBJECT_ATTRIBUTES_INIT(&attributes);
    attributes.ParentObject = BusObject->WdfUsbDevice;

    Status = WdfCollectionCreate(&attributes, &usbDeviceContext->WriteRequestCollection);

    if (!NT_SUCCESS(Status))
    {
        return Status;
    }

    //
    WDF_OBJECT_ATTRIBUTES_INIT(&attributes);
    attributes.ParentObject = BusObject->WdfUsbDevice;

    Status = WdfSpinLockCreate(&attributes, &usbDeviceContext->WriteCollectionLock);

    if (!NT_SUCCESS(Status))
    {
        return Status;
    }

    //
    //  see if the bus driver supports chained mdl's
    //
    Status = WdfUsbTargetDeviceQueryUsbCapability(BusObject->WdfUsbDevice, (PGUID)&GUID_USB_CAPABILITY_CHAINED_MDLS, 0, NULL, NULL);

    if (NT_SUCCESS(Status))
    {
        //
        //  request work, the stack supports chained mdl's
        //
        BusObject->ChainedMdlsSupported = TRUE;
    }
    else
    {
    }

    //  see what USB device type the bus driver is supporting
    //
    Status = WdfUsbTargetDeviceQueryUsbCapability(
        BusObject->WdfUsbDevice, (PGUID)&GUID_USB_CAPABILITY_DEVICE_TYPE, sizeof(USB_CAP_DEVICE_INFO), (PVOID)&usbCapDeviceInfo, &capResultLength);

    if (NT_SUCCESS(Status))
    {
        //
        //  request work, the stack returns USB device type
        //
        if (IsUsbCapDeviceInfoValid(usbCapDeviceInfo, capResultLength))
        {
            BusObject->UsbCapDeviceInfo = usbCapDeviceInfo;
        }
    }

    WdfUsbTargetDeviceGetDeviceDescriptor(BusObject->WdfUsbDevice, &DeviceDescriptor);

    Status = GetDeviceString(BusObject->WdfUsbDevice, DeviceDescriptor.iManufacturer, &BusObject->Manufacturer);

    Status = GetDeviceString(BusObject->WdfUsbDevice, DeviceDescriptor.iProduct, &BusObject->Model);

    WDF_USB_DEVICE_INFORMATION_INIT(&deviceInformation);
    Status = WdfUsbTargetDeviceRetrieveInformation(BusObject->WdfUsbDevice, &deviceInformation);
    if (NT_SUCCESS(Status))
    {
        BusObject->RemoteWakeCapable = IS_USB_DEVICE_REMOTE_WAKE_CAPABLE(deviceInformation);
    }

    Status = MbbParseConfigDescriptor(
        BusObject->WdfUsbDevice,
        &BusObject->MaxSegmentSize,
        &BusObject->NcmParams,
        &BusObject->PowerFiltersSupported,
        &BusObject->MaxPowerFilterSize,
        &usbDeviceContext->UsbCommunicationInterfaceIndex,
        &usbDeviceContext->UsbDataInterfaceIndex,
        &BusObject->MaxControlChannelSize,
        &usbDeviceContext->UsbCommunicationInterfaceSetting,
        &usbDeviceContext->UsbDataInterfaceSetting,
        &BulkInPacketSize,
        &BusObject->MTU,
        &BusObject->MbimVersion,
        &BusObject->MbimExtendedVersion);

    if (!NT_SUCCESS(Status))
    {
        goto Cleanup;
    }

    if (BusObject->MaxControlChannelSize < MIN_CONTROL_MESSAGE_SIZE)
    {
        Status = STATUS_INFO_LENGTH_MISMATCH;
        goto Cleanup;
    }

    if (BulkInPacketSize == 0)
    {
        Status = STATUS_INFO_LENGTH_MISMATCH;
        goto Cleanup;
    }

    numInterfaces = WdfUsbTargetDeviceGetNumInterfaces(BusObject->WdfUsbDevice);

    settingPairs = (PWDF_USB_INTERFACE_SETTING_PAIR)ALLOCATE_NONPAGED_POOL(sizeof(WDF_USB_INTERFACE_SETTING_PAIR) * numInterfaces);

    if (settingPairs == NULL)
    {
        Status = STATUS_INSUFFICIENT_RESOURCES;
        goto Cleanup;
    }

    for (interfaceIndex = 0; interfaceIndex < numInterfaces; interfaceIndex++)
    {
        settingPairs[interfaceIndex].UsbInterface = WdfUsbTargetDeviceGetInterface(BusObject->WdfUsbDevice, interfaceIndex);

        //
        //  Select alternate setting zero on all interfaces.
        //
        settingPairs[interfaceIndex].SettingIndex = 0;
    }

    WDF_USB_DEVICE_SELECT_CONFIG_PARAMS_INIT_MULTIPLE_INTERFACES(&configParams, numInterfaces, settingPairs);

    Status = WdfUsbTargetDeviceSelectConfig(BusObject->WdfUsbDevice, NULL, &configParams);

    if (!NT_SUCCESS(Status))
    {
        goto Cleanup;
    }

    if (configParams.Types.MultiInterface.NumberOfConfiguredInterfaces < 2)
    {
        Status = STATUS_NO_SUCH_DEVICE;
        goto Cleanup;
    }


    usbDeviceContext->WdfCommunicationInterfaceIndex = 0xff;
    usbDeviceContext->WdfDataInterfaceIndex = 0xff;

    for (i = 0; i < configParams.Types.MultiInterface.NumberOfConfiguredInterfaces; i++)
    {
        UsbInterface = WdfUsbTargetDeviceGetInterface(BusObject->WdfUsbDevice, i);
        CurrentSetting = WdfUsbInterfaceGetConfiguredSettingIndex(UsbInterface);
        WdfUsbInterfaceGetDescriptor(UsbInterface, CurrentSetting, &InterfaceDescriptor);
        if ((InterfaceDescriptor.bInterfaceNumber == usbDeviceContext->UsbCommunicationInterfaceIndex))
        {
            if (usbDeviceContext->WdfCommunicationInterfaceIndex == 0xff)
            {
                usbDeviceContext->WdfCommunicationInterfaceIndex = i;
            }
        }

        if ((InterfaceDescriptor.bInterfaceNumber == usbDeviceContext->UsbDataInterfaceIndex))
        {
            if (usbDeviceContext->WdfDataInterfaceIndex == 0xff)
            {
                usbDeviceContext->WdfDataInterfaceIndex = i;
            }
        }
    }

    if ((usbDeviceContext->WdfCommunicationInterfaceIndex == 0xff) || (usbDeviceContext->WdfDataInterfaceIndex == 0xff))
    {
        Status = STATUS_NO_SUCH_DEVICE;
        goto Cleanup;
    }

    //
    // get interrupt pipe handles
    //
    {
        BYTE ConfiguredPipes = 0;
        UsbInterface = WdfUsbTargetDeviceGetInterface(BusObject->WdfUsbDevice, usbDeviceContext->WdfCommunicationInterfaceIndex);

        WDF_USB_INTERFACE_SELECT_SETTING_PARAMS_INIT_SETTING(&SettingParams, usbDeviceContext->UsbCommunicationInterfaceSetting);

        Status = WdfUsbInterfaceSelectSetting(UsbInterface, WDF_NO_OBJECT_ATTRIBUTES, &SettingParams);

        if (!NT_SUCCESS(Status))
        {
            goto Cleanup;
        }

        ConfiguredPipes = WdfUsbInterfaceGetNumConfiguredPipes(UsbInterface);

        if (ConfiguredPipes != 1)
        {
            Status = STATUS_NO_SUCH_DEVICE;
            goto Cleanup;
        }

        WDF_USB_PIPE_INFORMATION_INIT(&pipeInfo);

        pipe = WdfUsbInterfaceGetConfiguredPipe(
            UsbInterface,
            0, // index 0
            &pipeInfo);

        if (WdfUsbPipeTypeInterrupt == pipeInfo.PipeType)
        {
            usbDeviceContext->InterruptPipe = pipe;
            usbDeviceContext->InterruptPipeIoTarget = WdfUsbTargetPipeGetIoTarget(pipe);

            usbDeviceContext->InterruptPipeMaxPacket = pipeInfo.MaximumPacketSize;
        }
        else
        {
            Status = STATUS_NO_SUCH_DEVICE;
            goto Cleanup;
        }
    }

    BusObject->SyncInterruptReadBuffer = (PUCHAR)ALLOCATE_NONPAGED_POOL(usbDeviceContext->InterruptPipeMaxPacket);

    if (BusObject->SyncInterruptReadBuffer == NULL)
    {
        Status = STATUS_NO_MEMORY;
        goto Cleanup;
    }

    //
    //   Configure the continous reader now
    //

    WDF_USB_CONTINUOUS_READER_CONFIG_INIT(&ReaderConfig, InterruptPipeReadComplete, BusObject, usbDeviceContext->InterruptPipeMaxPacket);

    ReaderConfig.NumPendingReads = 1;
    ReaderConfig.EvtUsbTargetPipeReadersFailed = InterruptPipeReadError;

    Status = WdfUsbTargetPipeConfigContinuousReader(usbDeviceContext->InterruptPipe, &ReaderConfig);

    if (!NT_SUCCESS(Status))
    {
        goto Cleanup;
    }

    Status = GetNtbParameters(BusObject->WdfUsbDevice, &BusObject->NtbParam);

    if (!NT_SUCCESS(Status))
    {
        goto Cleanup;
    }

    if (!BusObject->ChainedMdlsSupported)
    {
        //
        //  chained mdl's not supported, create a lookaside list to hold the buffers
        //
        WDF_OBJECT_ATTRIBUTES_INIT(&attributes);
        attributes.ParentObject = BusObject->WdfUsbDevice;
        //
        // TODO : For testing
        //
        // BusObject->NtbParam.dwNtbOutMaxSize = 512;

        Status = WdfLookasideListCreate(
            &attributes, BusObject->NtbParam.dwNtbOutMaxSize, NonPagedPoolNx, WDF_NO_OBJECT_ATTRIBUTES, 'CBMW', &usbDeviceContext->LookasideList);

        if (!NT_SUCCESS(Status))
        {
            goto Cleanup;
        }
    }

    if (BusObject->UsbCapDeviceInfo.DeviceInfoHeader.DeviceType == USB_CAP_DEVICE_TYPE_UDE_MBIM)
    {
        BusObject->MaxBulkInTransfer = BusObject->NtbParam.dwNtbInMaxSize < MAX_HOST_NTB_SIZE_FOR_UDE_MBIM
            ? BusObject->NtbParam.dwNtbInMaxSize
            : MAX_HOST_NTB_SIZE_FOR_UDE_MBIM;
    }
    else
    {
        BusObject->MaxBulkInTransfer =
            BusObject->NtbParam.dwNtbInMaxSize < MAX_HOST_NTB_SIZE ? BusObject->NtbParam.dwNtbInMaxSize : MAX_HOST_NTB_SIZE;
    }

    //
    //  round down the transfer size so it is a multiple of maxpacket size for the builk in pipe
    //
    BusObject->MaxBulkInTransfer = (BusObject->MaxBulkInTransfer / BulkInPacketSize) * BulkInPacketSize;

    BusObject->BulkInHeaderSize =
        ROUND_UP_COUNT((ULONG)MmSizeOfMdl(NULL, ROUND_UP_COUNT(BusObject->MaxBulkInTransfer + PAGE_SIZE, ALIGN_QUAD)), ALIGN_QUAD);

    if ((BusObject->NtbParam.bmNtbFormatSupported & NCM_NTB_FORMAT_32_BIT) == NCM_NTB_FORMAT_32_BIT)
    {
        //
        //  device supports 32 bit mode
        //
        if ((BusObject->NtbParam.dwNtbInMaxSize > 0xffff) || (BusObject->NtbParam.dwNtbOutMaxSize > 0xffff))
        {
            //
            //  the device can actually handle a transfer larger than 16bit, change to 32 bit
            //
            BusObject->NtbFormat32Bit = TRUE;
        }
    }

    //
    //  put the device back into a known state
    //
    Status = MbbBusResetDeviceAndSetParms(BusObject);

    if (!NT_SUCCESS(Status))
    {
        goto Cleanup;
    }

    *BusHandle = BusObject;
    usbDeviceContext->BusObject = BusObject;
    BusObject = NULL;

Cleanup:

    if (settingPairs != NULL)
    {
        FREE_POOL(settingPairs);
        settingPairs = NULL;
    }

    if (BusObject != NULL && preAllocatedBusObject == NULL)
    {
        FreeBusObject(BusObject);
        BusObject = NULL;
    }

    return Status;
}

VOID FreeBusObject(_In_ __drv_freesMem(Mem) PBUS_OBJECT BusObject)

{
    PUSB_DEVICE_CONTEXT usbDeviceContext = NULL;

    if (BusObject->WdfUsbDevice != NULL)
    {

        usbDeviceContext = GetUsbDeviceContext(BusObject->WdfUsbDevice);

        if (usbDeviceContext->InterruptPipeIoTarget != NULL)
        {
            WdfIoTargetStop(usbDeviceContext->InterruptPipeIoTarget, WdfIoTargetCancelSentIo);
        }

        usbDeviceContext->BusObject = NULL;
    }

    if (BusObject->Manufacturer != NULL)
    {
        FREE_POOL(BusObject->Manufacturer);
        BusObject->Manufacturer = NULL;
    }

    if (BusObject->Model != NULL)
    {
        FREE_POOL(BusObject->Model);
        BusObject->Model = NULL;
    }

    if (BusObject->WdfDevice != NULL)
    {
        WdfObjectDelete(BusObject->WdfDevice);
    }

    if (BusObject->UsbSsIrp != NULL)
    {
        IoCancelIrp(BusObject->UsbSsIrp);


        KeWaitForSingleObject(&BusObject->UsbSsIrpComplete, Executive, KernelMode, FALSE, NULL);


        IoFreeIrp(BusObject->UsbSsIrp);
        BusObject->UsbSsIrp = NULL;
    }

    if (BusObject->SyncInterruptReadBuffer != NULL)
    {
        FREE_POOL(BusObject->SyncInterruptReadBuffer);
        BusObject->SyncInterruptReadBuffer = NULL;
    }

    FREE_POOL(BusObject);
    BusObject = NULL;

    return;
}

VOID MbbBusCleanup(__in MBB_BUS_HANDLE BusHandle)

{
    PBUS_OBJECT BusObject = (PBUS_OBJECT)BusHandle;

    FreeBusObject(BusObject);
    BusObject = NULL;

    return;
}

NTSTATUS
MbbParseConfigDescriptor(
    WDFUSBDEVICE WdfUsbDevice,
    PUSHORT MaxSegmentSize,
    PUCHAR NcmCaps,
    PUCHAR PowerFiltersSupported,
    PUCHAR MaxPowerFilterSize,
    PUCHAR CommunicationClassInterface,
    PUCHAR DataClassInterface,
    PUSHORT MaxControlMessage,
    PUCHAR AltCommunicationClassSetting,
    PUCHAR AltDataClassSetting,
    PUSHORT BulkInDataClassPacketSize,
    PUSHORT MTU,
    PUSHORT MbimVersion,
    PUSHORT MbimExtendedVersion)

{

    NTSTATUS Status;

    BYTE* ConfigDescriptorBuffer = NULL;
    PUSB_CONFIGURATION_DESCRIPTOR ConfigDescriptor = NULL;
    PUSB_INTERFACE_DESCRIPTOR InterfaceDescriptor = NULL;
    PUSB_ENDPOINT_DESCRIPTOR EndpointDescriptor = NULL;
    PUSB_INTERFACE_ASSOCIATION_DESCRIPTOR IadDescriptor = NULL;
    PUSB_CS_DESCRIPTOR CsDescriptor = NULL;
    PUSB_CS_ECM_DESCRIPTOR EcmDescriptor = NULL;
    PUSB_CS_NCM_DESCRIPTOR NcmDescriptor = NULL;
    PUSB_CS_MBB_DESCRIPTOR MbbDescriptor = NULL;
    PUSB_CS_MBB_DESCRIPTOR_EXTENDED MbbDescriptorExtended = NULL;
    PUSB_CS_CDC_DESCRIPTOR CdcDescriptor = NULL;

    BYTE* Current = NULL;
    USHORT DescriptorSize = 0;

    BOOLEAN GotEcmDesc = FALSE;
    BOOLEAN GotNcmDesc = FALSE;
    BOOLEAN GotMbbDesc = FALSE;
    BYTE CurrentInterface = 0;
    PUSB_DEVICE_CONTEXT usbDeviceContext = NULL;

    *CommunicationClassInterface = 0xff;
    *DataClassInterface = 0xff;
    *AltCommunicationClassSetting = 0;
    *AltDataClassSetting = 0;
    *BulkInDataClassPacketSize = 0;
    *MTU = 0;
    *MbimVersion = 0x0100; // Default to MBIM 1.0 Version
    *MbimExtendedVersion = 0x0100;

    usbDeviceContext = GetUsbDeviceContext(WdfUsbDevice);

    Status = WdfUsbTargetDeviceRetrieveConfigDescriptor(WdfUsbDevice, NULL, &DescriptorSize);

    if (Status != STATUS_BUFFER_TOO_SMALL)
    {

        goto Cleanup;
    }

    ConfigDescriptorBuffer = (BYTE*)ALLOCATE_NONPAGED_POOL(DescriptorSize);

    if (ConfigDescriptorBuffer == NULL)
    {
        Status = STATUS_NO_MEMORY;
        goto Cleanup;
    }

    Status = WdfUsbTargetDeviceRetrieveConfigDescriptor(WdfUsbDevice, ConfigDescriptorBuffer, &DescriptorSize);

    if (!NT_SUCCESS(Status))
    {

        goto Cleanup;
    }

    Current = ConfigDescriptorBuffer;

    while (Current + sizeof(USB_DESCRIPTOR_HEADER) <= ConfigDescriptorBuffer + DescriptorSize)
    {
        PUSB_DESCRIPTOR_HEADER Header = (PUSB_DESCRIPTOR_HEADER)Current;

        if (Current + Header->bLength > ConfigDescriptorBuffer + DescriptorSize)
        {
            Status = STATUS_INFO_LENGTH_MISMATCH;

            goto Cleanup;
        }

        switch (Header->bDescriptorType)
        {
        case USB_CONFIGURATION_DESCRIPTOR_TYPE:

            ConfigDescriptor = (PUSB_CONFIGURATION_DESCRIPTOR)Current;

            break;

        case USB_INTERFACE_DESCRIPTOR_TYPE:

            InterfaceDescriptor = (PUSB_INTERFACE_DESCRIPTOR)Current;

            CurrentInterface = InterfaceDescriptor->bInterfaceNumber;

            if ((InterfaceDescriptor->bInterfaceClass == MBIM_CC_INTERFACE_CLASS) &&
                ((InterfaceDescriptor->bInterfaceSubClass == MBIM_CC_INTERFACE_SUBCLASS)) &&
                (InterfaceDescriptor->bInterfaceProtocol == MBIM_CC_INTERFACE_PROTOCOL ||
                    InterfaceDescriptor->bInterfaceProtocol == MBIM_CC_INTERFACE_NBL_PROTOCOL ||
                    InterfaceDescriptor->bInterfaceProtocol == MBIM_CC_INTERFACE_NETPACKET_PROTOCOL))
            {

                *CommunicationClassInterface = InterfaceDescriptor->bInterfaceNumber;
                *AltCommunicationClassSetting = InterfaceDescriptor->bAlternateSetting;
            }

            if ((InterfaceDescriptor->bInterfaceClass == MBIM_DC_INTERFACE_CLASS) &&
                (InterfaceDescriptor->bInterfaceSubClass == MBIM_DC_INTERFACE_SUBCLASS) &&
                (InterfaceDescriptor->bInterfaceProtocol == MBIM_DC_INTERFACE_PROTOCOL))
            {
                *DataClassInterface = InterfaceDescriptor->bInterfaceNumber;
                *AltDataClassSetting = InterfaceDescriptor->bAlternateSetting;
            }

            break;

        case USB_ENDPOINT_DESCRIPTOR_TYPE:

            if ((Header->bLength != sizeof(*EndpointDescriptor)))
            {

                Status = STATUS_INFO_LENGTH_MISMATCH;

                goto Cleanup;
            }

            EndpointDescriptor = (PUSB_ENDPOINT_DESCRIPTOR)Current;


            if (CurrentInterface == *DataClassInterface)
            {
                //
                //  this endpoint is for the data interface
                //
                if (USB_ENDPOINT_DIRECTION_IN(EndpointDescriptor->bEndpointAddress) && (EndpointDescriptor->bmAttributes & USB_ENDPOINT_TYPE_BULK))
                {
                    //
                    //  bulk in endpoint
                    //
                    *BulkInDataClassPacketSize = EndpointDescriptor->wMaxPacketSize & 0x7ff;

                }
            }

            break;

        case USB_INTERFACE_ASSOCIATION_DESCRIPTOR_TYPE:


            break;

        case USB_CDC_CS_DESCRIPTOR_TYPE:

            CsDescriptor = (PUSB_CS_DESCRIPTOR)Current;

            switch (CsDescriptor->bSubType)
            {
            case USB_CDC_CS_ECM_DESCRIPTOR_SUBTYPE:

                EcmDescriptor = (PUSB_CS_ECM_DESCRIPTOR)Current;


                break;

            case USB_CDC_CS_NCM_DESCRIPTOR_SUBTYPE:

                NcmDescriptor = (PUSB_CS_NCM_DESCRIPTOR)Current;

                break;

            case USB_CDC_CS_DESCRIPTOR_SUBTYPE:

                if ((CsDescriptor->bLength != sizeof(*CdcDescriptor)))
                {
                    Status = STATUS_INFO_LENGTH_MISMATCH;

                    goto Cleanup;
                }

                CdcDescriptor = (PUSB_CS_CDC_DESCRIPTOR)Current;

                break;

            case USB_CDC_CS_MBB_DESCRIPTOR_SUBTYPE:

                if ((CsDescriptor->bLength != sizeof(*MbbDescriptor)))
                {
                    Status = STATUS_INFO_LENGTH_MISMATCH;

                    goto Cleanup;
                }

                MbbDescriptor = (PUSB_CS_MBB_DESCRIPTOR)Current;

                if (MbbDescriptor->wMbbVersion < MBIM_MBB_FUNCDESC_MIN_VERSION)
                {

                    Status = STATUS_INFO_LENGTH_MISMATCH;
                    goto Cleanup;
                }

                if (MbbDescriptor->wMaxSegmentSize < MBIM_MIN_SEGMENT_SIZE)
                {
                    Status = STATUS_INFO_LENGTH_MISMATCH;
                    goto Cleanup;
                }

                if (MbbDescriptor->bMaxFilterSize > MBIM_MAX_PACKET_FILTER_SIZE)
                {
                    Status = STATUS_INFO_LENGTH_MISMATCH;

                    goto Cleanup;
                }

                if (MbbDescriptor->bNumberPowerFilters < MBIM_MIN_NUMBER_OF_PACKET_FILTERS)
                {
                    Status = STATUS_INFO_LENGTH_MISMATCH;

                    goto Cleanup;
                }

                *MaxSegmentSize = MbbDescriptor->wMaxSegmentSize;
                *PowerFiltersSupported = MbbDescriptor->bNumberPowerFilters;
                *MaxPowerFilterSize = MbbDescriptor->bMaxFilterSize;
                *NcmCaps = MbbDescriptor->bmNetworkCapabilities;
                *MaxControlMessage = MbbDescriptor->wMaxControlMessage;
                GotMbbDesc = TRUE;
                *MbimVersion = MbbDescriptor->wMbbVersion;

                break;

            case USB_CDC_CS_MBB_DESCRIPTOR_EXTENDED_SUBTYPE:

                // MBIM 1.0 - 6.5: If MBIM Extended Functional Descriptor is provided, it must appear after MBIM Functional Descriptor.
                if (!GotMbbDesc)
                {
                    Status = STATUS_UNRECOGNIZED_MEDIA;

                    goto Cleanup;
                }

                if ((CsDescriptor->bLength != sizeof(*MbbDescriptorExtended)))
                {
                    Status = STATUS_INFO_LENGTH_MISMATCH;

                    goto Cleanup;
                }

                MbbDescriptorExtended = (PUSB_CS_MBB_DESCRIPTOR_EXTENDED)Current;

                if (MbbDescriptorExtended->wMbbVersion < MBIM_MBB_FUNCDESC_EXTENDED_MIN_VERSION)
                {

                    Status = STATUS_INFO_LENGTH_MISMATCH;
                    goto Cleanup;
                }

                if (MbbDescriptorExtended->wMTU < MBIM_MIN_MTU_SIZE)
                {
                    Status = STATUS_INFO_LENGTH_MISMATCH;
                    goto Cleanup;
                }

                // If the Extended MTU comes in as larger than the MaxSegmentSize, we set the MTU to MaxSegmentSize
                if (MbbDescriptorExtended->wMTU > *MaxSegmentSize)
                {
                    // Check if the MaxSegmentSize is not less than the Minimum MTU size, if it is then bail
                    if (*MaxSegmentSize < MBIM_MIN_MTU_SIZE)
                    {
                        Status = STATUS_INFO_LENGTH_MISMATCH;
                        goto Cleanup;
                    }
                    else
                    {
                        MbbDescriptorExtended->wMTU = *MaxSegmentSize;
                    }
                }

                *MTU = MbbDescriptorExtended->wMTU;
                *MbimExtendedVersion = MbbDescriptorExtended->wMbbVersion;

                break;

            default:


                break;
            }

            break;

        default:


            break;
        }

        Current += Header->bLength;
    }

    Status = STATUS_SUCCESS;

Cleanup:

    if (ConfigDescriptorBuffer != NULL)
    {
        FREE_POOL(ConfigDescriptorBuffer);
        ConfigDescriptorBuffer = NULL;
    }

    if (!(GotMbbDesc))
    {

        Status = STATUS_UNRECOGNIZED_MEDIA;
    }

    if ((*CommunicationClassInterface == 0xff) || (*DataClassInterface == 0xff))
    {

        Status = STATUS_UNRECOGNIZED_MEDIA;
    }

    return Status;
}

NTSTATUS
GetNtbParameters(WDFUSBDEVICE WdfUsbDevice, PNCM_NTB_PARAMETER NcmNtb)

{

    NTSTATUS Status;
    ULONG BytesTransfered = 0;

    Status = SendSyncControlCommand(
        WdfUsbDevice, BmRequestDeviceToHost, BmRequestToInterface, GET_NTB_PARAMETERS, 0, (PUCHAR)NcmNtb, sizeof(*NcmNtb), &BytesTransfered);

    if (NT_SUCCESS(Status))
    {
        if (BytesTransfered < sizeof(*NcmNtb))
        {
            Status = STATUS_INFO_LENGTH_MISMATCH;
            goto Cleanup;
        }
    }
    else
    {
        goto Cleanup;
    }

    if ((NcmNtb->bmNtbFormatSupported & NCM_NTB_FORMAT_16_BIT) != NCM_NTB_FORMAT_16_BIT)
    {
        Status = STATUS_INFO_LENGTH_MISMATCH;
        goto Cleanup;
    }

    if (NcmNtb->wNdpInPayloadRemainder >= NcmNtb->wNdpInDivisor)
    {
        Status = STATUS_INVALID_PARAMETER;
        goto Cleanup;
    }

    if ((NcmNtb->wNdpInPayloadRemainder > NcmNtb->dwNtbInMaxSize) || (NcmNtb->wNdpInDivisor > NcmNtb->dwNtbInMaxSize))
    {
        Status = STATUS_INVALID_PARAMETER;
        goto Cleanup;
    }

    if ((NcmNtb->wNdpInAlignment < 4) || (NcmNtb->wNdpInAlignment > NcmNtb->dwNtbInMaxSize) ||
        ((NcmNtb->wNdpInAlignment - 1) & NcmNtb->wNdpInAlignment) != 0)
    {
        Status = STATUS_INVALID_PARAMETER;
        goto Cleanup;
    }

    if (NcmNtb->wNdpOutPayloadRemainder >= NcmNtb->wNdpOutDivisor)
    {
        Status = STATUS_INVALID_PARAMETER;
        goto Cleanup;
    }

    if ((NcmNtb->wNdpOutPayloadRemainder > NcmNtb->dwNtbOutMaxSize) || (NcmNtb->wNdpOutDivisor > NcmNtb->dwNtbOutMaxSize))
    {
        Status = STATUS_INVALID_PARAMETER;
        goto Cleanup;
    }

    if ((NcmNtb->wNdpOutAlignment < 4) || (NcmNtb->wNdpOutAlignment > NcmNtb->dwNtbOutMaxSize) ||
        ((NcmNtb->wNdpOutAlignment - 1) & NcmNtb->wNdpOutAlignment) != 0)
    {
        Status = STATUS_INVALID_PARAMETER;
        goto Cleanup;
    }

Cleanup:

    return Status;
}

NTSTATUS
SetActivityIdForRequest(__in WDFREQUEST Request, __in LPGUID ActivityId)
{
    NTSTATUS Status = STATUS_SUCCESS;
    PIRP Irp = NULL;

    do
    {
        GUID zeroGuid = { 0 };

        if (ActivityId == NULL || IsEqualGUID(*ActivityId, zeroGuid))
        {
            Status = STATUS_INVALID_PARAMETER;
            break;
        }

        Irp = WdfRequestWdmGetIrp(Request);

        if (Irp == NULL)
        {
            Status = STATUS_INVALID_PARAMETER;
            break;
        }

        // set the activity id of the IRP
        Status = IoSetActivityIdIrp(Irp, ActivityId);
    } while (FALSE);

    return Status;
}

EVT_WDF_REQUEST_COMPLETION_ROUTINE SendCompletionRoutine;

VOID SendCompletionRoutine(__in WDFREQUEST Request, __in WDFIOTARGET Target, __in PWDF_REQUEST_COMPLETION_PARAMS Params, __in WDFCONTEXT Context)

{
    PREQUEST_CONTEXT ReqContext = NULL;

    ReqContext = GetRequestContext(Request);

    (ReqContext->Callback.Send)(ReqContext->ProtocolHandle, ReqContext->CallbackContext, Params->IoStatus.Status);

    WdfObjectDelete(Request);
}

NTSTATUS
MbbBusSendMessageFragment(
    __in MBB_BUS_HANDLE BusHandle,
    __in MBB_REQUEST_HANDLE RequestHandle,
    __in PVOID MessageFragment,
    __in ULONG FragmentLength,
    __in LPGUID ActivityId,
    __in MBB_BUS_SEND_COMPLETION_CALLBACK SendCompletionCallback)

    /*
        Description
            The protocol layer call this routine to request the bus layer to
            send a message fragment. Fragmentation / Reassembly is handled by
            the protocol layer and it will only handle fragments that are within
            the maximum transfer size of the bus.

            This routine is asynchronous and returns immediately after queueing
            the transfer. The caller is notified of the completion through the
            callback.

        Parameters
            __in MBB_BUS_HANDLE BusHandle,
                BusHandle identifies the instance of the bus layer.

            __in MBB_REQUEST_HANDLE RequestHandle,
                Identifies the request.

            __in PVOID MessageFragment,
                The data payload that needs to be sent.

            __in ULONG FragmentLength,
                Length of the data payload. This will not be greater than the
                maximum transfer size supported by the bus.

            __in LPGUID ActivityId,
                The activity Id to be associated with this fragment transfer.
                This activity Id will also be used by USB for logging USB events.

            __in MBB_BUS_SEND_COMPLETION_CALLBACK SendCompletionCallback
                The completion callback routine that will be called by the bus
                when the transfer is complete.

        Return Value

            NTSTATUS_SUCCESS
                The transfer has completed successfully. SendCompletionCallback will NOT be called.

            NTSTATUS_PENDING
                The transfer was queued. SendCompletionCallback will be called on completion.

            Other failure code
                The transfer could not be queued. SendCompletionCallback will NOT be called.
    */

{

    WDF_USB_CONTROL_SETUP_PACKET packet;
    NTSTATUS status;
    WDF_OBJECT_ATTRIBUTES attributes;
    WDFMEMORY memHandle = NULL;
    WDFREQUEST request = NULL;
    BOOLEAN SentToDevice = FALSE;
    PREQUEST_CONTEXT Context = NULL;
    WDF_REQUEST_SEND_OPTIONS SendOptions;

    PBUS_OBJECT BusObject = (PBUS_OBJECT)BusHandle;

    WDF_OBJECT_ATTRIBUTES_INIT_CONTEXT_TYPE(&attributes, REQUEST_CONTEXT);
    attributes.ParentObject = BusObject->WdfUsbDevice;

    status = WdfRequestCreate(&attributes, WdfUsbTargetDeviceGetIoTarget(BusObject->WdfUsbDevice), &request);

    if (!NT_SUCCESS(status))
    {

        goto Cleanup;
    }

    Context = GetRequestContext(request);

    Context->CallbackContext = RequestHandle;
    Context->Callback.Send = SendCompletionCallback;
    Context->ProtocolHandle = BusObject->ProtocolHandle;
    Context->UsbDeviceContext = GetUsbDeviceContext(BusObject->WdfUsbDevice);

    WDF_OBJECT_ATTRIBUTES_INIT(&attributes);
    attributes.ParentObject = request;

    status = WdfMemoryCreatePreallocated(&attributes, MessageFragment, FragmentLength, &memHandle);

    if (!NT_SUCCESS(status))
    {

        goto Cleanup;
    }

    WDF_USB_CONTROL_SETUP_PACKET_INIT_CLASS(
        &packet, BmRequestHostToDevice, BmRequestToInterface, SEND_ENCAPSULATE_COMMAND, 0, Context->UsbDeviceContext->UsbCommunicationInterfaceIndex);

    status = WdfUsbTargetDeviceFormatRequestForControlTransfer(BusObject->WdfUsbDevice, request, &packet, memHandle, NULL);

    if (!NT_SUCCESS(status))
    {

        goto Cleanup;
    }

    WdfRequestSetCompletionRoutine(request, SendCompletionRoutine, NULL);

    WDF_REQUEST_SEND_OPTIONS_INIT(&SendOptions, 0);

    WDF_REQUEST_SEND_OPTIONS_SET_TIMEOUT(&SendOptions, WDF_REL_TIMEOUT_IN_SEC(10));

    // Set the activity Id of the IRP associated with the request.
    // Ignore any failures
    SetActivityIdForRequest(request, ActivityId);

    if (FragmentLength >= sizeof(MBB_COMMAND_HEADER))
    {
        PMBB_COMMAND_HEADER CommandHeader = (PMBB_COMMAND_HEADER)MessageFragment;
    }

    SentToDevice = WdfRequestSend(request, WdfUsbTargetDeviceGetIoTarget(BusObject->WdfUsbDevice), &SendOptions);

    status = STATUS_PENDING;

    if (!SentToDevice)
    {

        status = WdfRequestGetStatus(request);

    }

Cleanup:

    if (!NT_SUCCESS(status))
    {
        if (request != NULL)
        {
            WdfObjectDelete(request);
            request = NULL;
        }
    }

    return status;
}

EVT_WDF_REQUEST_COMPLETION_ROUTINE ReceiveCompletionRoutine;

VOID ReceiveCompletionRoutine(__in WDFREQUEST Request, __in WDFIOTARGET Target, __in PWDF_REQUEST_COMPLETION_PARAMS Params, __in WDFCONTEXT Context)

{
    PREQUEST_CONTEXT ReqContext = NULL;
    GUID ActivityId = { 0 };
    PIRP Irp = NULL;
    NTSTATUS Status = STATUS_INVALID_PARAMETER;

    ReqContext = GetRequestContext(Request);

    // Get the IRP
    Irp = WdfRequestWdmGetIrp(Request);

    if (Irp != NULL)
    {
        // Get the activity Id
        Status = IoGetActivityIdIrp(Irp, &ActivityId);
    }

    if (NT_SUCCESS(Params->IoStatus.Status) && (Params->IoStatus.Information >= sizeof(MBB_COMMAND_DONE_HEADER)))
    {

        PMBB_COMMAND_DONE_HEADER CommandHeader = (PMBB_COMMAND_DONE_HEADER)ReqContext->Buffer;
    }

    (*ReqContext->Callback.Receive)(
        ReqContext->ProtocolHandle, ReqContext->CallbackContext, Params->IoStatus.Status, (ULONG)Params->IoStatus.Information);

    WdfObjectDelete(Request);
}

NTSTATUS
MbbBusReceiveMessageFragment(
    _In_ MBB_BUS_HANDLE BusHandle,
    _In_ MBB_REQUEST_HANDLE RequestHandle,
    _In_ __drv_aliasesMem PVOID MessageFragment,
    _In_ ULONG FragmentLength,
    _In_ LPGUID ActivityId,
    _In_ MBB_BUS_RECEIVE_COMPLETION_CALLBACK ReceiveCompletionCallback)
    /*
        Description

        Parameters


        Return Value

            NTSTATUS_SUCCESS
                Initialization was successful.

            Other failure code
    */
    /*
        Description
            The protocol layer call this routine to request the bus layer to
            receive data from the device. Reassembly is handled by the protocol layer.

            This routine is asynchronous and returns immediately after queueing
            the transfer. The caller is notified of the completion through the
            callback.

        Parameters
            _In_ MBB_BUS_HANDLE BusHandle,
                BusHandle identifies the instance of the bus layer.

            _In_ MBB_REQUEST_HANDLE RequestHandle,
                Identifies the request.

            _In_ __drv_aliasesMem PVOID MessageFragment,
                The data buffer that would be filled with the received data.

            _In_ ULONG FragmentLength,
                Length of the data requested from the device. This will not be
                greater than the maximum transfer size supported by the bus.

            _In_ LPGUID ActivityId,
                The activity Id to be associated with this fragment transfer.
                This activity Id will also be used by USB for logging USB events.

            _In_ MBB_BUS_RECEIVE_COMPLETION_CALLBACK ReceiveCompletionCallback
                The completion callback routine that will be called by the bus
                when the transfer is complete.

        Return Value

            NTSTATUS_SUCCESS
                The transfer has completed successfully. ReceiveCompletionCallback will NOT be called.

            NTSTATUS_PENDING
                The transfer was queued. ReceiveCompletionCallback will be called on completion.

            Other failure code
                The transfer could not be queued. ReceiveCompletionCallback will NOT be called.
    */

{
    WDF_USB_CONTROL_SETUP_PACKET packet;
    NTSTATUS status;
    WDF_OBJECT_ATTRIBUTES attributes;
    WDFMEMORY memHandle = NULL;
    WDFREQUEST request = NULL;
    BOOLEAN SentToDevice = FALSE;
    PREQUEST_CONTEXT Context = NULL;
    WDF_REQUEST_SEND_OPTIONS SendOptions;
    PIRP Irp = NULL;

    PBUS_OBJECT BusObject = (PBUS_OBJECT)BusHandle;


    WDF_OBJECT_ATTRIBUTES_INIT_CONTEXT_TYPE(&attributes, REQUEST_CONTEXT);

    attributes.ParentObject = BusObject->WdfUsbDevice;

    status = WdfRequestCreate(&attributes, WdfUsbTargetDeviceGetIoTarget(BusObject->WdfUsbDevice), &request);

    if (!NT_SUCCESS(status))
    {

        goto Cleanup;
    }

    Context = GetRequestContext(request);

    Context->CallbackContext = RequestHandle;
    Context->Callback.Receive = ReceiveCompletionCallback;
    Context->ProtocolHandle = BusObject->ProtocolHandle;
    Context->UsbDeviceContext = GetUsbDeviceContext(BusObject->WdfUsbDevice);

    Context->Buffer = MessageFragment;
    Context->BufferLength = FragmentLength;

    WDF_OBJECT_ATTRIBUTES_INIT(&attributes);
    attributes.ParentObject = request;

    status = WdfMemoryCreatePreallocated(&attributes, MessageFragment, FragmentLength, &memHandle);

    if (!NT_SUCCESS(status))
    {

        goto Cleanup;
    }

    WDF_USB_CONTROL_SETUP_PACKET_INIT_CLASS(
        &packet, BmRequestDeviceToHost, BmRequestToInterface, GET_ENCAPSULATE_RESPONSE, 0, Context->UsbDeviceContext->UsbCommunicationInterfaceIndex);

    status = WdfUsbTargetDeviceFormatRequestForControlTransfer(BusObject->WdfUsbDevice, request, &packet, memHandle, NULL);

    if (!NT_SUCCESS(status))
    {

        goto Cleanup;
    }

    WdfRequestSetCompletionRoutine(request, ReceiveCompletionRoutine, NULL);

    WDF_REQUEST_SEND_OPTIONS_INIT(&SendOptions, 0);

    WDF_REQUEST_SEND_OPTIONS_SET_TIMEOUT(&SendOptions, WDF_REL_TIMEOUT_IN_SEC(10));

    // Set the activity Id of the IRP associated with the request.
    // Ignore any failures
    SetActivityIdForRequest(request, ActivityId);

    SentToDevice = WdfRequestSend(request, WdfUsbTargetDeviceGetIoTarget(BusObject->WdfUsbDevice), &SendOptions);

    status = STATUS_PENDING;

    if (!SentToDevice)
    {

        status = WdfRequestGetStatus(request);

    }

Cleanup:

    if (!NT_SUCCESS(status))
    {
        if (request != NULL)
        {
            WdfObjectDelete(request);
            request = NULL;
        }
    }

    return status;
}

VOID InterruptPipeReadComplete(__in WDFUSBPIPE Pipe, __in WDFMEMORY Memory, __in size_t NumBytesTransfered, __in WDFCONTEXT Context)

{
    PBUS_OBJECT BusObject = (PBUS_OBJECT)Context;
    PVOID Buffer = NULL;
    PUSB_CDC_NOTIFICATION CdcNotification = NULL;
    PUSB_CDC_NOTIFICATION_SPEED_CHANGE CdcSpeedChangeNotification = NULL;
    ULONG TempBytesTransfered = (ULONG)NumBytesTransfered;
    UCHAR IndicateBuffer[INTERRUPT_REASSEMBLY_BUFFER_SIZE];
    ULONG BufferLength = 0;


    Buffer = WdfMemoryGetBuffer(Memory, NULL);

    //
    //  process this fragment, the function will return a non-zero value if a complete message is returned.
    //
    BufferLength = ProcessInterruptPipeRead(BusObject, (PCUCHAR)Buffer, NumBytesTransfered, IndicateBuffer, sizeof(IndicateBuffer));

    if (BufferLength >= sizeof(*CdcNotification))
    {
        CdcNotification = (PUSB_CDC_NOTIFICATION)&IndicateBuffer[0];

        switch (CdcNotification->bNotificationCode)
        {
        case USB_CDC_NOTIFICATION_RESPONSE_AVAILABLE: (BusObject->ResponseAvailableCallback)(BusObject->ProtocolHandle); break;

        case USB_CDC_NOTIFICATION_NETWORK_CONNECTION:
        case USB_CDC_NOTIFICATION_CONNECTION_SPEED_CHANGE:
        default: break;
        }
    }

    return;
}

BOOLEAN
InterruptPipeReadError(__in WDFUSBPIPE Pipe, __in NTSTATUS Status, __in USBD_STATUS UsbdStatus)

{
    return TRUE;
}

NTSTATUS
MbbBusQueryBusParameters(__in MBB_BUS_HANDLE BusHandle, __out PMBB_BUS_PARAMETERS BusParameters)

{
    PBUS_OBJECT BusObject = (PBUS_OBJECT)BusHandle;

    RtlZeroMemory(BusParameters, sizeof(*BusParameters));

    BusParameters->FragmentSize = BusObject->MaxControlChannelSize;

    BusParameters->MaxSegmentSize = BusObject->MaxSegmentSize;

    BusParameters->Ntb32BitSupported = (BusObject->NtbParam.bmNtbFormatSupported & NCM_NTB_FORMAT_32_BIT) == NCM_NTB_FORMAT_32_BIT;
    BusParameters->MaxOutNtb = BusObject->NtbParam.dwNtbOutMaxSize;

    if ((BusObject->NtbParam.wNtbOutMaxDatagrams == 0) || (BusObject->NtbParam.wNtbOutMaxDatagrams > MAX_OUT_DATAGRAMS))
    {
        //
        //  Zero means unlimited, impose a limit so the driver can preallocated the contexts it needs
        //
        BusParameters->MaxOutDatagrams = MAX_OUT_DATAGRAMS;
    }
    else
    {
        BusParameters->MaxOutDatagrams = BusObject->NtbParam.wNtbOutMaxDatagrams;
    }

    BusParameters->NdpOutDivisor = BusObject->NtbParam.wNdpOutDivisor;
    BusParameters->NdpOutRemainder = BusObject->NtbParam.wNdpOutPayloadRemainder;
    BusParameters->NdpOutAlignment = BusObject->NtbParam.wNdpOutAlignment;
    //
    // TODO: Read the current value from the bus object when SetNtb is implemented
    //
    BusParameters->CurrentMode32Bit = BusObject->NtbFormat32Bit;
    BusParameters->SelectiveSuspendSupported = TRUE;

    BusParameters->PowerFiltersSupported = BusObject->PowerFiltersSupported;
    BusParameters->MaxPowerFilterSize = BusObject->MaxPowerFilterSize;
    BusParameters->RemoteWakeCapable = BusObject->RemoteWakeCapable;

    if (BusObject->Manufacturer != NULL)
    {
        RtlStringCbCopyW(BusParameters->Manufacturer, sizeof(BusParameters->Manufacturer), BusObject->Manufacturer);
    }

    if (BusObject->Model != NULL)
    {
        RtlStringCbCopyW(BusParameters->Model, sizeof(BusParameters->Model), BusObject->Model);
    }

    BusParameters->MTU = BusObject->MTU;
    if (BusObject->MTU != 0)
    {
        BusParameters->IsErrataDevice = TRUE;
    }
    else
    {
        BusParameters->IsErrataDevice = FALSE;
    }

    BusParameters->MbimVersion = BusObject->MbimVersion;
    BusParameters->MbimExtendedVersion = BusObject->MbimExtendedVersion;

    return STATUS_SUCCESS;
}

NTSTATUS MbbBusHandshake(_In_ PBUS_OBJECT BusObject, _In_ ULONG TransactionId, _In_opt_ PVOID FastIOSendNetBufferListsComplete, _In_opt_ PVOID FastIOIndicateReceiveNetBufferLists)
{
    NTSTATUS Status;
    ULONG BytesTransferred = 0;
    UCHAR ReadBuffer[256];
    ULONG RetryCount = 0;
    MBB_OPEN_MESSAGE OpenMessage;
    PMBB_OPEN_DONE OpenDoneMessage = NULL;
    MBB_OPEN_MESSAGE_FASTIO OpenMessageFastIO;
    PMBB_OPEN_DONE_FASTIO OpenDoneMessageFastIO = NULL;

    RtlZeroMemory(&OpenMessage, sizeof(OpenMessage));

    OpenMessage.MessageHeader.MessageType = MBB_MESSAGE_TYPE_OPEN;
    OpenMessage.MessageHeader.MessageLength = sizeof(OpenMessage);
    OpenMessage.MessageHeader.MessageTransactionId = TransactionId;
    OpenMessage.MaximumControlTransfer =
        BusObject->MaxControlChannelSize < MAX_CONTROL_MESSAGE_SIZE ? BusObject->MaxControlChannelSize : MAX_CONTROL_MESSAGE_SIZE;

    do
    {
        RetryCount++;

        Status = TransactControlChannel(
            BusObject, RetryCount * INITIAL_OPEN_TIMEOUT, (PUCHAR)&OpenMessage, sizeof(OpenMessage), (PUCHAR)&ReadBuffer[0], sizeof(ReadBuffer), &BytesTransferred);
    } while ((Status == STATUS_IO_TIMEOUT) && (RetryCount < MAX_OPEN_RETRY_ATTEMPTS));

    if (NT_SUCCESS(Status))
    {
        if (BytesTransferred < sizeof(*OpenDoneMessage))
        {
            Status = STATUS_INFO_LENGTH_MISMATCH;
        }
    }

    if (NT_SUCCESS(Status))
    {
        OpenDoneMessage = (PMBB_OPEN_DONE)&ReadBuffer[0];

        if ((OpenDoneMessage->MessageHeader.MessageType != MBB_MESSAGE_TYPE_OPEN_DONE) ||
            (OpenDoneMessage->MessageHeader.MessageLength < sizeof(*OpenDoneMessage)) ||
            (OpenDoneMessage->MessageHeader.MessageTransactionId != OpenMessage.MessageHeader.MessageTransactionId))
        {
            Status = STATUS_NDIS_INVALID_DATA;
        }
        else if (OpenDoneMessage->MbbStatus != STATUS_SUCCESS)
        {
            Status = STATUS_OPEN_FAILED;
        }
    }
    return Status;
}

NTSTATUS
MbbBusStart(_In_ MBB_BUS_HANDLE BusHandle)
{
    PBUS_OBJECT BusObject = (PBUS_OBJECT)BusHandle;
    NTSTATUS Status;
    BOOLEAN FailOpen = FALSE;

    BOOLEAN AltSettingSet = FALSE;

    PUSB_DEVICE_CONTEXT usbDeviceContext = NULL;

    ACQUIRE_PASSIVE_LOCK(&BusObject->Lock);

    if (BusObject->State != BUS_STATE_CLOSED)
    {
        FailOpen = TRUE;
    }
    else
    {
        BusObject->State = BUS_STATE_OPENING;
    }

    RELEASE_PASSIVE_LOCK(&BusObject->Lock);

    if (FailOpen)
    {
        Status = STATUS_UNSUCCESSFUL;

        goto Cleanup;
    }

    usbDeviceContext = GetUsbDeviceContext(BusObject->WdfUsbDevice);

    Status = MbbBusResetDeviceAndSetParms(BusObject);

    if (!NT_SUCCESS(Status))
    {
        goto Cleanup;
    }

    Status = MbbBusSelectDataAltSetting(BusObject, ALT_DATA_SETTING_1);

    if (NT_ERROR(Status))
    {
        goto Cleanup;
    }

    AltSettingSet = TRUE;

    //
    //  start the continouse reader now
    //
    Status = WdfIoTargetStart(usbDeviceContext->InterruptPipeIoTarget);

    if (!NT_SUCCESS(Status))
    {

        goto Cleanup;
    }

    Status = STATUS_SUCCESS;

    ACQUIRE_PASSIVE_LOCK(&BusObject->Lock);

    BusObject->State = BUS_STATE_OPENED;

    RELEASE_PASSIVE_LOCK(&BusObject->Lock);

Cleanup:

    if (!NT_SUCCESS(Status))
    {
        //
        //  stop the pipe reader
        //
        WdfIoTargetStop(usbDeviceContext->InterruptPipeIoTarget, WdfIoTargetCancelSentIo);

        //
        //  set the data pipes back to alt seeting 0
        //
        if (AltSettingSet)
        {
            MbbBusSelectDataAltSetting(BusObject, ALT_DATA_SETTING_0);
        }

        ACQUIRE_PASSIVE_LOCK(&BusObject->Lock);

        BusObject->State = BUS_STATE_CLOSED;

        RELEASE_PASSIVE_LOCK(&BusObject->Lock);
    }

    return Status;
}

NTSTATUS
MbbBusStop(__in MBB_BUS_HANDLE BusHandle)
{
    PBUS_OBJECT BusObject = (PBUS_OBJECT)BusHandle;
    NTSTATUS Status;

    PUSB_DEVICE_CONTEXT usbDeviceContext = NULL;


    usbDeviceContext = GetUsbDeviceContext(BusObject->WdfUsbDevice);

    //
    //  stop the interrupt pipe so the continuous reader will stop and we can do sync reads
    //
    WdfIoTargetStop(usbDeviceContext->InterruptPipeIoTarget, WdfIoTargetCancelSentIo);

    Status = STATUS_SUCCESS;

    MbbBusSelectDataAltSetting(BusObject, ALT_DATA_SETTING_0);

    ACQUIRE_PASSIVE_LOCK(&BusObject->Lock);

    BusObject->State = BUS_STATE_CLOSED;

    RELEASE_PASSIVE_LOCK(&BusObject->Lock);


    return Status;
}

BOOLEAN
MbbBusIsStoped(_In_ MBB_BUS_HANDLE BusHandle)
{
    PBUS_OBJECT BusObject = (PBUS_OBJECT)BusHandle;
    return BusObject->State == BUS_STATE_CLOSED;
}

NTSTATUS
MbbBusOpen(_In_ MBB_BUS_HANDLE BusHandle, _In_ ULONG TransactionId, _In_opt_ PVOID FastIOSendNetBufferListsComplete, _In_opt_ PVOID FastIOIndicateReceiveNetBufferLists)
/*
    Description

        Opens the session with the device

    Parameters
        __in    MBB_BUS_HANDLE      BusHandle,
            BusHandle identifies the instance of the bus layer.


    Return Value

        NTSTATUS_SUCCESS
            Information was successfully returned in the BusParameters structure.

        NTSTATUS_INVALID_PARAMETER
            One of the required parameters is missing or bad.

        Other failure code
*/
{
    PBUS_OBJECT BusObject = (PBUS_OBJECT)BusHandle;
    NTSTATUS Status;
    BOOLEAN FailOpen = FALSE;

    PUSB_CDC_NOTIFICATION Notification = NULL;
    BOOLEAN AltSettingSet = FALSE;

    PUSB_DEVICE_CONTEXT usbDeviceContext = NULL;

    ACQUIRE_PASSIVE_LOCK(&BusObject->Lock);

    if (BusObject->State != BUS_STATE_CLOSED)
    {
        FailOpen = TRUE;
    }
    else
    {
        BusObject->State = BUS_STATE_OPENING;
    }

    RELEASE_PASSIVE_LOCK(&BusObject->Lock);

    if (FailOpen)
    {
        Status = STATUS_UNSUCCESSFUL;


        goto Cleanup;
    }

    usbDeviceContext = GetUsbDeviceContext(BusObject->WdfUsbDevice);


    Status = MbbBusResetDeviceAndSetParms(BusObject);

    if (!NT_SUCCESS(Status))
    {
        goto Cleanup;
    }

    Status = MbbBusSelectDataAltSetting(BusObject, ALT_DATA_SETTING_1);

    if (NT_ERROR(Status))
    {
        goto Cleanup;
    }

    AltSettingSet = TRUE;

    Status = MbbBusHandshake(BusObject, TransactionId, FastIOSendNetBufferListsComplete, FastIOIndicateReceiveNetBufferLists);
    if (NT_ERROR(Status))
    {
        goto Cleanup;
    }

    //
    //  start the continouse reader now
    //
    Status = WdfIoTargetStart(usbDeviceContext->InterruptPipeIoTarget);

    if (!NT_SUCCESS(Status))
    {
        goto Cleanup;
    }

    Status = STATUS_SUCCESS;

    ACQUIRE_PASSIVE_LOCK(&BusObject->Lock);

    BusObject->State = BUS_STATE_OPENED;

    RELEASE_PASSIVE_LOCK(&BusObject->Lock);

Cleanup:

    if (!NT_SUCCESS(Status))
    {
        //
        //  stop the pipe reader
        //
        WdfIoTargetStop(usbDeviceContext->InterruptPipeIoTarget, WdfIoTargetCancelSentIo);

        //
        //  set the data pipes back to alt seeting 0
        //
        if (AltSettingSet)
        {
            MbbBusSelectDataAltSetting(BusObject, ALT_DATA_SETTING_0);
        }

        ACQUIRE_PASSIVE_LOCK(&BusObject->Lock);

        BusObject->State = BUS_STATE_CLOSED;

        RELEASE_PASSIVE_LOCK(&BusObject->Lock);
    }

    return Status;
}

NTSTATUS
MbbBusClose(__in MBB_BUS_HANDLE BusHandle, __in ULONG TransactionId, __in BOOLEAN ForceClose)
/*
    Description

        Opens the session with the device

    Parameters
        __in    MBB_BUS_HANDLE      BusHandle,
            BusHandle identifies the instance of the bus layer.


    Return Value

        NTSTATUS_SUCCESS
            Information was successfully returned in the BusParameters structure.

        NTSTATUS_INVALID_PARAMETER
            One of the required parameters is missing or bad.

        Other failure code
*/

{
    PBUS_OBJECT BusObject = (PBUS_OBJECT)BusHandle;
    NTSTATUS Status;
    BOOLEAN FailClose = FALSE;

    ULONG BytesTransfered = 0;
    PUSB_CDC_NOTIFICATION Notification = NULL;
    MBB_CLOSE_MESSAGE CloseMessage;
    PMBB_CLOSE_DONE CloseDoneMessage = NULL;
    UCHAR ReadBuffer[256];

    PUSB_DEVICE_CONTEXT usbDeviceContext = NULL;


    ACQUIRE_PASSIVE_LOCK(&BusObject->Lock);

    if (!ForceClose)
    {
        if (BusObject->State != BUS_STATE_OPENED)
        {
            FailClose = TRUE;
        }
    }

    RELEASE_PASSIVE_LOCK(&BusObject->Lock);

    if (FailClose)
    {
        Status = STATUS_UNSUCCESSFUL;

        goto Cleanup;
    }

    usbDeviceContext = GetUsbDeviceContext(BusObject->WdfUsbDevice);

    //
    //  stop the interrupt pipe so the continuous reader will stop and we can do sync reads
    //
    WdfIoTargetStop(usbDeviceContext->InterruptPipeIoTarget, WdfIoTargetCancelSentIo);

    RtlZeroMemory(&CloseMessage, sizeof(CloseMessage));

    CloseMessage.MessageHeader.MessageType = MBB_MESSAGE_TYPE_CLOSE;
    CloseMessage.MessageHeader.MessageLength = sizeof(CloseMessage);
    CloseMessage.MessageHeader.MessageTransactionId = TransactionId;

    Status = TransactControlChannel(
        BusObject, DEFAULT_IO_TIMEOUT, (PUCHAR)&CloseMessage, sizeof(CloseMessage), (PUCHAR)&ReadBuffer[0], sizeof(ReadBuffer), &BytesTransfered);

    if (NT_SUCCESS(Status))
    {
        if (BytesTransfered < sizeof(*CloseDoneMessage))
        {
            Status = STATUS_INFO_LENGTH_MISMATCH;
        }
    }

    if (!NT_SUCCESS(Status))
    {
        goto Cleanup;
    }

    CloseDoneMessage = (PMBB_CLOSE_DONE)&ReadBuffer[0];

    if ((CloseDoneMessage->MessageHeader.MessageType != MBB_MESSAGE_TYPE_CLOSE_DONE) ||
        (CloseDoneMessage->MessageHeader.MessageLength < sizeof(*CloseDoneMessage)) ||
        (CloseDoneMessage->MessageHeader.MessageTransactionId != CloseMessage.MessageHeader.MessageTransactionId))
    {
        Status = STATUS_INFO_LENGTH_MISMATCH;

        goto Cleanup;
    }

Cleanup:

    Status = STATUS_SUCCESS;

    MbbBusSelectDataAltSetting(BusObject, ALT_DATA_SETTING_0);

    ACQUIRE_PASSIVE_LOCK(&BusObject->Lock);

    BusObject->State = BUS_STATE_CLOSED;

    RELEASE_PASSIVE_LOCK(&BusObject->Lock);

    return Status;
}

VOID MbbBusSetNotificationState(__in MBB_BUS_HANDLE BusHandle, __in BOOLEAN Enabled)

{
    PBUS_OBJECT BusObject = (PBUS_OBJECT)BusHandle;
    PUSB_DEVICE_CONTEXT usbDeviceContext = NULL;

    usbDeviceContext = GetUsbDeviceContext(BusObject->WdfUsbDevice);

    if (!Enabled)
    {
        WdfIoTargetStop(usbDeviceContext->InterruptPipeIoTarget, WdfIoTargetCancelSentIo);
    }
    else
    {
        (void)WdfIoTargetStart(usbDeviceContext->InterruptPipeIoTarget);
    }

    return;
}