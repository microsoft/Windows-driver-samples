/*++

Copyright (c) Microsoft Corporation All Rights Reserved

Module Name:

    UsbHsDevice.cpp

Abstract:

    Implementation of the UsbHsDevice class. 

--*/

#pragma warning (disable : 4127)

#include <initguid.h>
#include <sysvad.h>
#include "hw.h"
#include "savedata.h"
#include "IHVPrivatePropertySet.h"
#include "simple.h"

#ifdef SYSVAD_USB_SIDEBAND
#include <limits.h>
#include <usbspec.h>
#include <usb.h>
#include <SidebandAudio.h>
#include <USBSidebandAudio.h>
#include <wdmguid.h>    // guild-arrival/removal
#include <devpkey.h>
#include "usbhsminipairs.h"
#include "UsbHsDevice.h"
#include "UsbHsDeviceFormats.h"

//
// UsbHsDevice implementation.
//

// # of sec before sync request is cancelled.
#define USB_SIDEBAND_SYNC_REQ_TIMEOUT_IN_SEC         60 
#define USB_SIDEBAND_NOTIFICATION_MAX_ERROR_COUNT    5

//=============================================================================
#pragma code_seg("PAGE")
STDMETHODIMP
UsbHsDevice::NonDelegatingQueryInterface
( 
    _In_ REFIID                 Interface,
    _COM_Outptr_ PVOID *        Object 
)
/*++

Routine Description:

  QueryInterface routine for UsbHsDevice

Arguments:

  Interface - 

  Object -

Return Value:

  NT status code.

--*/
{
    PAGED_CODE();

    ASSERT(Object);

    if (IsEqualGUIDAligned(Interface, IID_IUnknown))
    {
        *Object = PVOID(PUNKNOWN(PSIDEBANDDEVICECOMMON(this)));
    }
    else if (IsEqualGUIDAligned(Interface, IID_IUsbHsDeviceCommon))
    {
        *Object = PVOID(PSIDEBANDDEVICECOMMON(this));
    }
    else
    {
        *Object = NULL;
    }

    if (*Object)
    {
        PUNKNOWN(*Object)->AddRef();
        return STATUS_SUCCESS;
    }

    return STATUS_INVALID_PARAMETER;
} // NonDelegatingQueryInterface

//=============================================================================
// Dummy stubs to override the default WDF behavior of closing the target 
// on query remove. This driver closes and deletes the supporting objects
// when the target removes the USB Sideband interface.
//

#pragma code_seg("PAGE")
NTSTATUS
UsbHsDevice::EvtUsbHsTargetQueryRemove
( 
    _In_    WDFIOTARGET     IoTarget
)
{
    PAGED_CODE();
    DPF_ENTER(("[%!FUNC!]"));

    UNREFERENCED_PARAMETER(IoTarget);
    return STATUS_SUCCESS;
}

#pragma code_seg("PAGE")
VOID
UsbHsDevice::EvtUsbHsTargetRemoveCanceled
( 
    _In_    WDFIOTARGET     IoTarget
)
{
    PAGED_CODE();
    DPF_ENTER(("[%!FUNC!]"));

    UNREFERENCED_PARAMETER(IoTarget);
}
#pragma code_seg("PAGE")
VOID
UsbHsDevice::EvtUsbHsTargetRemoveComplete
( 
    _In_    WDFIOTARGET     IoTarget
)
{
    PAGED_CODE();
    DPF_ENTER(("[%!FUNC!]"));

    UNREFERENCED_PARAMETER(IoTarget);
}

//=============================================================================
#pragma code_seg("PAGE")
NTSTATUS
UsbHsDevice::Init
(
    _In_ IAdapterCommon     * Adapter, 
    _In_ PUNICODE_STRING      SymbolicLinkName
)
{
    PAGED_CODE();
    DPF_ENTER(("[%!FUNC!]"));

    NTSTATUS                                ntStatus        = STATUS_SUCCESS;
    UsbHsDeviceNotificationWorkItemContext *wiCtx          = NULL;
    UsbHsDeviceNotificationReqContext      *reqCtx         = NULL;
    WDF_OBJECT_ATTRIBUTES                   attributes;
    WDF_IO_TARGET_OPEN_PARAMS               openParams;
    WDF_WORKITEM_CONFIG                     wiConfig;

    AddRef(); // first ref.

    //
    // Basic init of all the class' members.
    //
    m_State                         = eUsbHsStateInitializing;
    m_Adapter                       = Adapter;

    // Static config.
    m_WdfIoTarget                   = NULL;
    m_SpeakerMiniports              = NULL;
    m_MicMiniports                  = NULL;
    m_UnknownSpeakerTopology        = NULL;
    m_UnknownSpeakerWave            = NULL;
    m_UnknownMicTopology            = NULL;
    m_UnknownMicWave                = NULL;
    m_Descriptor                    = NULL;
    m_SpeakerVolumePropValues       = NULL;
    m_MicVolumePropValues           = NULL;
    m_SpeakerMutePropValues         = NULL;
    m_MicMutePropValues             = NULL;

    // Notification updates. 
    m_SpeakerVolumeLevel            = 0;
    m_MicVolumeLevel                = 0;
    m_SpeakerMute                   = 0;
    m_MicMute                       = 0;
    m_SpeakerStreamStatusLong       = STATUS_SUCCESS; // USB stream is not open.
    m_MicStreamStatusLong           = STATUS_SUCCESS; // USB stream is not open.

    m_SpeakerStreamReq              = NULL;
    m_MicStreamReq                  = NULL;
    m_SpeakerVolumeReq              = NULL;
    m_MicVolumeReq                  = NULL;
    m_SpeakerMuteReq                = NULL;
    m_MicMuteReq                    = NULL;

    m_WorkItem                      = NULL;
    m_ReqCollection                 = NULL;

    m_nSpeakerStreamsOpen           = 0;
    m_nMicStreamsOpen               = 0;

    m_nSpeakerStreamsStart          = 0;
    m_nMicStreamsStart              = 0;

    m_pSpeakerSupportedFormatsIntersection  = NULL;
    m_pMicSupportedFormatsIntersection      = NULL;

    KeInitializeEvent(&m_SpeakerStreamStatusEvent, NotificationEvent, TRUE);
    KeInitializeEvent(&m_MicStreamStatusEvent, NotificationEvent, TRUE);

    InitializeListHead(&m_ListEntry);
    KeInitializeSpinLock(&m_Lock);

    RtlZeroMemory(&m_SymbolicLinkName, sizeof(m_SymbolicLinkName));

    RtlZeroMemory(&m_SpeakerVolumeCallback, sizeof(m_SpeakerVolumeCallback));
    RtlZeroMemory(&m_MicVolumeCallback, sizeof(m_MicVolumeCallback));

    RtlZeroMemory(&m_SpeakerMuteCallback, sizeof(m_SpeakerMuteCallback));
    RtlZeroMemory(&m_MicMuteCallback, sizeof(m_MicMuteCallback));

    RtlZeroMemory(&m_SpeakerConnectionStatusCallback, sizeof(m_SpeakerConnectionStatusCallback));
    RtlZeroMemory(&m_MicConnectionStatusCallback, sizeof(m_MicConnectionStatusCallback));

    RtlZeroMemory(&m_SpeakerFormatChangeCallback, sizeof(m_SpeakerFormatChangeCallback));
    RtlZeroMemory(&m_MicFormatChangeCallback, sizeof(m_MicFormatChangeCallback));

    RtlZeroMemory(&m_SpeakerTransportResources, sizeof(USBHSDEVICE_EP_TRANSPORT_RESOURCES));
    RtlZeroMemory(&m_MicTransportResources , sizeof(USBHSDEVICE_EP_TRANSPORT_RESOURCES));

    //
    // Allocate a notification WDF work-item.
    //
    WDF_WORKITEM_CONFIG_INIT(&wiConfig, EvtUsbHsDeviceNotificationStatusWorkItem);
    wiConfig.AutomaticSerialization = FALSE;

    WDF_OBJECT_ATTRIBUTES_INIT_CONTEXT_TYPE(&attributes, UsbHsDeviceNotificationWorkItemContext);
    attributes.ParentObject = Adapter->GetWdfDevice();
    ntStatus = WdfWorkItemCreate( &wiConfig,
                                  &attributes,
                                  &m_WorkItem);
    IF_FAILED_ACTION_JUMP(
        ntStatus,
        DPF(D_ERROR, ("%!FUNC!: WdfWorkItemCreate failed: 0x%x", ntStatus)),
        Done);

    wiCtx = GetUsbHsDeviceNotificationWorkItemContext(m_WorkItem);
    wiCtx->UsbHsDevice = this; // weak ref.

    //
    // Allocate a collection to hold notification requests for the notification work-item.
    //
    WDF_OBJECT_ATTRIBUTES_INIT(&attributes);
    attributes.ParentObject = m_Adapter->GetWdfDevice();
    ntStatus = WdfCollectionCreate(
        &attributes,
        &m_ReqCollection);

    IF_FAILED_ACTION_JUMP(
        ntStatus,
        DPF(D_ERROR, ("%!FUNC!: WdfCollectionCreate failed: 0x%x", ntStatus)),
        Done);

    //
    // Open the target interface.
    //
    WDF_OBJECT_ATTRIBUTES_INIT(&attributes);
    attributes.ParentObject = m_Adapter->GetWdfDevice();
    ntStatus = WdfIoTargetCreate(m_Adapter->GetWdfDevice(),
                                 &attributes,
                                 &m_WdfIoTarget);
    IF_FAILED_ACTION_JUMP(
        ntStatus,
        DPF(D_ERROR, ("%!FUNC!: WdfIoTargetCreate failed: 0x%x", ntStatus)),
        Done);

    WDF_IO_TARGET_OPEN_PARAMS_INIT_OPEN_BY_NAME(
        &openParams,
        SymbolicLinkName,
        STANDARD_RIGHTS_ALL);     

    openParams.EvtIoTargetQueryRemove = EvtUsbHsTargetQueryRemove;
    openParams.EvtIoTargetRemoveCanceled = EvtUsbHsTargetRemoveCanceled;
    openParams.EvtIoTargetRemoveComplete = EvtUsbHsTargetRemoveComplete;

    ntStatus = WdfIoTargetOpen(m_WdfIoTarget, &openParams);
    IF_FAILED_ACTION_JUMP(
        ntStatus,
        DPF(D_ERROR, ("%!FUNC!: WdfIoTargetOpen(%wZ) failed: 0x%x", SymbolicLinkName, ntStatus)),
        Done);

    PDEVICE_OBJECT pdo = WdfIoTargetWdmGetTargetPhysicalDevice(m_WdfIoTarget);
    m_Adapter->AddDeviceAsPowerDependency(pdo);

    //
    // Make a copy of the symbolic link name.
    //
    m_SymbolicLinkName.MaximumLength = SymbolicLinkName->MaximumLength;
    m_SymbolicLinkName.Length = SymbolicLinkName->Length;
    m_SymbolicLinkName.Buffer = (PWSTR) ExAllocatePool2(POOL_FLAG_NON_PAGED,
                                                        SymbolicLinkName->MaximumLength,
                                                        MINADAPTER_POOLTAG);
    if (m_SymbolicLinkName.Buffer == NULL)
    {
        ntStatus = STATUS_INSUFFICIENT_RESOURCES;
    }

    IF_FAILED_ACTION_JUMP(
        ntStatus,
        DPF(D_ERROR, ("%!FUNC!: ExAllocatePool2 failed, out of memory")),
        Done);

    RtlCopyUnicodeString(&m_SymbolicLinkName, SymbolicLinkName);

    //
    // Allocate the WDF requests for status notifications.
    //

    //
    // IOCTL_SBAUD_GET_VOLUME_STATUS_UPDATE 
    // SPEAKER
    //
    WDF_OBJECT_ATTRIBUTES_INIT_CONTEXT_TYPE(&attributes, UsbHsDeviceNotificationReqContext);
    attributes.ParentObject = m_Adapter->GetWdfDevice();
    ntStatus = WdfRequestCreate(
        &attributes,
        m_WdfIoTarget,
        &m_SpeakerVolumeReq);
    IF_FAILED_ACTION_JUMP(
        ntStatus,
        DPF(D_ERROR, ("%!FUNC!: WdfRequestCreate(Speaker-Volume) failed, 0x%x", ntStatus)),
        Done);

    // Init context.
    reqCtx = GetUsbHsDeviceNotificationReqContext(m_SpeakerVolumeReq);
    reqCtx->UsbHsDevice = this; // weak ref.

    ntStatus = WdfMemoryCreatePreallocated(
        WDF_NO_OBJECT_ATTRIBUTES,
        &reqCtx->Buffer,
        sizeof(reqCtx->Buffer.Volume),
        &reqCtx->MemIn);
    IF_FAILED_ACTION_JUMP(
        ntStatus,
        DPF(D_ERROR, ("%!FUNC!: WdfMemoryCreatePreallocated failed, 0x%x", ntStatus)),
        Done);

    ntStatus = WdfMemoryCreatePreallocated(
        WDF_NO_OBJECT_ATTRIBUTES,
        &reqCtx->Buffer,
        sizeof(reqCtx->Buffer.Volume),
        &reqCtx->MemOut);
    IF_FAILED_ACTION_JUMP(
        ntStatus,
        DPF(D_ERROR, ("%!FUNC!: WdfMemoryCreatePreallocated failed, 0x%x", ntStatus)),
        Done);

    //
    // IOCTL_SBAUD_GET_VOLUME_STATUS_UPDATE
    // MIC
    //
    WDF_OBJECT_ATTRIBUTES_INIT_CONTEXT_TYPE(&attributes, UsbHsDeviceNotificationReqContext);
    attributes.ParentObject = m_Adapter->GetWdfDevice();
    ntStatus = WdfRequestCreate(
        &attributes,
        m_WdfIoTarget,
        &m_MicVolumeReq);
    IF_FAILED_ACTION_JUMP(
        ntStatus,
        DPF(D_ERROR, ("%!FUNC!: WdfRequestCreate(Mic-Volume) failed, 0x%x", ntStatus)),
        Done);

    // Init context.
    reqCtx = GetUsbHsDeviceNotificationReqContext(m_MicVolumeReq);
    reqCtx->UsbHsDevice = this; // weak ref.

    ntStatus = WdfMemoryCreatePreallocated(
        WDF_NO_OBJECT_ATTRIBUTES,
        &reqCtx->Buffer,
        sizeof(reqCtx->Buffer.Volume),
        &reqCtx->MemIn);
    IF_FAILED_ACTION_JUMP(
        ntStatus,
        DPF(D_ERROR, ("%!FUNC!: WdfMemoryCreatePreallocated failed, 0x%x", ntStatus)),
        Done);

    ntStatus = WdfMemoryCreatePreallocated(
        WDF_NO_OBJECT_ATTRIBUTES,
        &reqCtx->Buffer,
        sizeof(reqCtx->Buffer.Volume),
        &reqCtx->MemOut);
    IF_FAILED_ACTION_JUMP(
        ntStatus,
        DPF(D_ERROR, ("%!FUNC!: WdfMemoryCreatePreallocated failed, 0x%x", ntStatus)),
        Done);

    //
    // IOCTL_SBAUD_GET_MUTE_STATUS_UPDATE 
    // SPEAKER
    //
    WDF_OBJECT_ATTRIBUTES_INIT_CONTEXT_TYPE(&attributes, UsbHsDeviceNotificationReqContext);
    attributes.ParentObject = m_Adapter->GetWdfDevice();
    ntStatus = WdfRequestCreate(
        &attributes,
        m_WdfIoTarget,
        &m_SpeakerMuteReq);
    IF_FAILED_ACTION_JUMP(
        ntStatus,
        DPF(D_ERROR, ("%!FUNC!: WdfRequestCreate(Speaker-Mute) failed, 0x%x", ntStatus)),
        Done);

    // Init context.
    reqCtx = GetUsbHsDeviceNotificationReqContext(m_SpeakerMuteReq);
    reqCtx->UsbHsDevice = this; // weak ref.

    ntStatus = WdfMemoryCreatePreallocated(
        WDF_NO_OBJECT_ATTRIBUTES,
        &reqCtx->Buffer,
        sizeof(reqCtx->Buffer.Mute),
        &reqCtx->MemIn);
    IF_FAILED_ACTION_JUMP(
        ntStatus,
        DPF(D_ERROR, ("%!FUNC!: WdfMemoryCreatePreallocated failed, 0x%x", ntStatus)),
        Done);

    ntStatus = WdfMemoryCreatePreallocated(
        WDF_NO_OBJECT_ATTRIBUTES,
        &reqCtx->Buffer,
        sizeof(reqCtx->Buffer.Mute),
        &reqCtx->MemOut);
    IF_FAILED_ACTION_JUMP(
        ntStatus,
        DPF(D_ERROR, ("%!FUNC!: WdfMemoryCreatePreallocated failed, 0x%x", ntStatus)),
        Done);

    //
    // IOCTL_SBAUD_GET_MUTE_STATUS_UPDATE
    // MIC
    //
    WDF_OBJECT_ATTRIBUTES_INIT_CONTEXT_TYPE(&attributes, UsbHsDeviceNotificationReqContext);
    attributes.ParentObject = m_Adapter->GetWdfDevice();
    ntStatus = WdfRequestCreate(
        &attributes,
        m_WdfIoTarget,
        &m_MicMuteReq);
    IF_FAILED_ACTION_JUMP(
        ntStatus,
        DPF(D_ERROR, ("%!FUNC!: WdfRequestCreate(Mic-Mute) failed, 0x%x", ntStatus)),
        Done);

    // Init context.
    reqCtx = GetUsbHsDeviceNotificationReqContext(m_MicMuteReq);
    reqCtx->UsbHsDevice = this; // weak ref.

    ntStatus = WdfMemoryCreatePreallocated(
        WDF_NO_OBJECT_ATTRIBUTES,
        &reqCtx->Buffer,
        sizeof(reqCtx->Buffer.Mute),
        &reqCtx->MemIn);
    IF_FAILED_ACTION_JUMP(
        ntStatus,
        DPF(D_ERROR, ("%!FUNC!: WdfMemoryCreatePreallocated failed, 0x%x", ntStatus)),
        Done);

    ntStatus = WdfMemoryCreatePreallocated(
        WDF_NO_OBJECT_ATTRIBUTES,
        &reqCtx->Buffer,
        sizeof(reqCtx->Buffer.Mute),
        &reqCtx->MemOut);
    IF_FAILED_ACTION_JUMP(
        ntStatus,
        DPF(D_ERROR, ("%!FUNC!: WdfMemoryCreatePreallocated failed, 0x%x", ntStatus)),
        Done);

    //
    // IOCTL_SBAUD_GET_STREAM_STATUS_UPDATE
    //
    WDF_OBJECT_ATTRIBUTES_INIT_CONTEXT_TYPE(&attributes, UsbHsDeviceNotificationReqContext);
    attributes.ParentObject = m_Adapter->GetWdfDevice();
    ntStatus = WdfRequestCreate(
        &attributes,
        m_WdfIoTarget,
        &m_SpeakerStreamReq);
    IF_FAILED_ACTION_JUMP(
        ntStatus,
        DPF(D_ERROR, ("%!FUNC!: WdfRequestCreate(Stream-Status) failed, 0x%x", ntStatus)),
        Done);

    // Init context.
    reqCtx = GetUsbHsDeviceNotificationReqContext(m_SpeakerStreamReq);
    reqCtx->UsbHsDevice = this; // weak ref.

    ntStatus = WdfMemoryCreatePreallocated(
        WDF_NO_OBJECT_ATTRIBUTES,
        &reqCtx->Buffer,
        sizeof(reqCtx->Buffer.streamStatus),
        &reqCtx->MemIn);
    IF_FAILED_ACTION_JUMP(
        ntStatus,
        DPF(D_ERROR, ("%!FUNC!: WdfMemoryCreatePreallocated failed, 0x%x", ntStatus)),
        Done);

    ntStatus = WdfMemoryCreatePreallocated(
        WDF_NO_OBJECT_ATTRIBUTES,
        &reqCtx->Buffer,
        sizeof(reqCtx->Buffer.streamStatus),
        &reqCtx->MemOut);
    IF_FAILED_ACTION_JUMP(
        ntStatus,
        DPF(D_ERROR, ("%!FUNC!: WdfMemoryCreatePreallocated failed, 0x%x", ntStatus)),
        Done);

    //
    // IOCTL_SBAUD_GET_STREAM_STATUS_UPDATE
    //
    WDF_OBJECT_ATTRIBUTES_INIT_CONTEXT_TYPE(&attributes, UsbHsDeviceNotificationReqContext);
    attributes.ParentObject = m_Adapter->GetWdfDevice();
    ntStatus = WdfRequestCreate(
        &attributes,
        m_WdfIoTarget,
        &m_MicStreamReq);
    IF_FAILED_ACTION_JUMP(
        ntStatus,
        DPF(D_ERROR, ("%!FUNC!: WdfRequestCreate(Stream-Status) failed, 0x%x", ntStatus)),
        Done);

    // Init context.
    reqCtx = GetUsbHsDeviceNotificationReqContext(m_MicStreamReq);
    reqCtx->UsbHsDevice = this; // weak ref.

    ntStatus = WdfMemoryCreatePreallocated(
        WDF_NO_OBJECT_ATTRIBUTES,
        &reqCtx->Buffer,
        sizeof(reqCtx->Buffer.streamStatus),
        &reqCtx->MemIn);
    IF_FAILED_ACTION_JUMP(
        ntStatus,
        DPF(D_ERROR, ("%!FUNC!: WdfMemoryCreatePreallocated failed, 0x%x", ntStatus)),
        Done);

    ntStatus = WdfMemoryCreatePreallocated(
        WDF_NO_OBJECT_ATTRIBUTES,
        &reqCtx->Buffer,
        sizeof(reqCtx->Buffer.streamStatus),
        &reqCtx->MemOut);
    IF_FAILED_ACTION_JUMP(
        ntStatus,
        DPF(D_ERROR, ("%!FUNC!: WdfMemoryCreatePreallocated failed, 0x%x", ntStatus)),
        Done);

    //
    // Create wave and filter names unique to this USB device
    //
    ntStatus = CreateFilterNames(SymbolicLinkName);
    IF_FAILED_ACTION_JUMP(
        ntStatus,
        DPF(D_ERROR, ("%!FUNC!: Creating unqie filter names for the USB device failed, 0x%x", ntStatus)),
        Done);

   //
   // This remote device is now in running state. No need to use interlock operations
   // b/c at this time this is the only thread accessing this info.
   //
   m_State = eUsbHsStateRunning;

   //
   // Init successful.
   //
   ntStatus = STATUS_SUCCESS;

Done:
    return ntStatus;
}

//=============================================================================
#pragma code_seg("PAGE")
UsbHsDevice::~UsbHsDevice
( 
    void 
)
/*++

Routine Description:

  Destructor for UsbHsDevice.

Arguments:

Return Value:

  void

--*/
{
    PAGED_CODE();
    DPF_ENTER(("[%!FUNC!]"));

    ASSERT(m_State != eUsbHsStateRunning);
    ASSERT(IsListEmpty(&m_ListEntry));

    //
    // Release ref to remote stack.
    //
    if (m_WdfIoTarget != NULL)
    {
        PDEVICE_OBJECT pdo = WdfIoTargetWdmGetTargetPhysicalDevice(m_WdfIoTarget);
        m_Adapter->RemoveDeviceAsPowerDependency(pdo);

        WdfObjectDelete(m_WdfIoTarget);
        m_WdfIoTarget = NULL;
    }

    //
    // Free symbolic links.
    //
    if (m_SymbolicLinkName.Buffer != NULL)
    {
        ExFreePoolWithTag(m_SymbolicLinkName.Buffer, MINADAPTER_POOLTAG);
        RtlZeroMemory(&m_SymbolicLinkName, sizeof(m_SymbolicLinkName));
    }

    DeleteCustomEndpointMinipair(m_SpeakerMiniports);
    m_SpeakerMiniports = NULL;

    DeleteCustomEndpointMinipair(m_MicMiniports);
    m_MicMiniports = NULL;

    if (m_Descriptor != NULL)
    {
        ExFreePoolWithTag(m_Descriptor, MINADAPTER_POOLTAG);
        m_Descriptor = NULL;
    }

    SAFE_DELETE_PTR_WITH_TAG(m_pSpeakerDescriptor, USBSIDEBANDTEST_POOLTAG02);
    SAFE_DELETE_PTR_WITH_TAG(m_pMicDescriptor, USBSIDEBANDTEST_POOLTAG02);

    SAFE_DELETE_PTR_WITH_TAG(m_pSpeakerSupportedFormatsIntersection, USBSIDEBANDTEST_POOLTAG03);
    SAFE_DELETE_PTR_WITH_TAG(m_pMicSupportedFormatsIntersection, USBSIDEBANDTEST_POOLTAG03);

    FreeTransportResources(&m_SpeakerTransportResources);
    FreeTransportResources(&m_MicTransportResources);

    if (m_SpeakerVolumePropValues != NULL)
    {
        ExFreePoolWithTag(m_SpeakerVolumePropValues, USBSIDEBANDTEST_POOLTAG02);
        m_SpeakerVolumePropValues = NULL;
    }

    if (m_MicVolumePropValues != NULL)
    {
        ExFreePoolWithTag(m_MicVolumePropValues, USBSIDEBANDTEST_POOLTAG02);
        m_MicVolumePropValues = NULL;
    }

    if (m_SpeakerMutePropValues != NULL)
    {
        ExFreePoolWithTag(m_SpeakerMutePropValues, USBSIDEBANDTEST_POOLTAG06);
        m_SpeakerMutePropValues = NULL;
    }

    if (m_MicMutePropValues != NULL)
    {
        ExFreePoolWithTag(m_MicMutePropValues, USBSIDEBANDTEST_POOLTAG06);
        m_MicMutePropValues = NULL;
    }

    //
    // Free Irps.
    //
    if (m_SpeakerVolumeReq != NULL)
    {
        UsbHsDeviceNotificationReqContext * ctx;

        // Delete the associated memory objects.
        ctx = GetUsbHsDeviceNotificationReqContext(m_SpeakerVolumeReq);
        if (ctx->MemIn != NULL)
        {
            WdfObjectDelete(ctx->MemIn);
            ctx->MemIn = NULL;
        }

        if (ctx->MemOut != NULL)
        {
            WdfObjectDelete(ctx->MemOut);
            ctx->MemOut = NULL;
        }

        // Delete the request.
        WdfObjectDelete(m_SpeakerVolumeReq);
        m_SpeakerVolumeReq = NULL;
    }

    if (m_MicVolumeReq != NULL)
    {
        UsbHsDeviceNotificationReqContext * ctx;

        // Delete the associated memory objects.
        ctx = GetUsbHsDeviceNotificationReqContext(m_MicVolumeReq);
        if (ctx->MemIn != NULL)
        {
            WdfObjectDelete(ctx->MemIn);
            ctx->MemIn = NULL;
        }

        if (ctx->MemOut != NULL)
        {
            WdfObjectDelete(ctx->MemOut);
            ctx->MemOut = NULL;
        }

        // Delete the request.
        WdfObjectDelete(m_MicVolumeReq);
        m_MicVolumeReq = NULL;
    }

    if (m_SpeakerMuteReq != NULL)
    {
        UsbHsDeviceNotificationReqContext * ctx;

        // Delete the associated memory objects.
        ctx = GetUsbHsDeviceNotificationReqContext(m_SpeakerMuteReq);
        if (ctx->MemIn != NULL)
        {
            WdfObjectDelete(ctx->MemIn);
            ctx->MemIn = NULL;
        }

        if (ctx->MemOut != NULL)
        {
            WdfObjectDelete(ctx->MemOut);
            ctx->MemOut = NULL;
        }

        // Delete the request.
        WdfObjectDelete(m_SpeakerMuteReq);
        m_SpeakerMuteReq = NULL;
    }

    if (m_MicMuteReq != NULL)
    {
        UsbHsDeviceNotificationReqContext * ctx;

        // Delete the associated memory objects.
        ctx = GetUsbHsDeviceNotificationReqContext(m_MicMuteReq);
        if (ctx->MemIn != NULL)
        {
            WdfObjectDelete(ctx->MemIn);
            ctx->MemIn = NULL;
        }

        if (ctx->MemOut != NULL)
        {
            WdfObjectDelete(ctx->MemOut);
            ctx->MemOut = NULL;
        }

        // Delete the request.
        WdfObjectDelete(m_MicMuteReq);
        m_MicMuteReq = NULL;
    }

    if (m_SpeakerStreamReq != NULL)
    {
        UsbHsDeviceNotificationReqContext * ctx;

        // Delete the associated memory objects.
        ctx = GetUsbHsDeviceNotificationReqContext(m_SpeakerStreamReq);
        if (ctx->MemIn != NULL)
        {
            WdfObjectDelete(ctx->MemIn);
            ctx->MemIn = NULL;
        }

        if (ctx->MemOut != NULL)
        {
            WdfObjectDelete(ctx->MemOut);
            ctx->MemOut = NULL;
        }

        // Delete the request.
        WdfObjectDelete(m_SpeakerStreamReq);
        m_SpeakerStreamReq = NULL;
    }

    if (m_MicStreamReq != NULL)
    {
        UsbHsDeviceNotificationReqContext * ctx;

        // Delete the associated memory objects.
        ctx = GetUsbHsDeviceNotificationReqContext(m_MicStreamReq);
        if (ctx->MemIn != NULL)
        {
            WdfObjectDelete(ctx->MemIn);
            ctx->MemIn = NULL;
        }

        if (ctx->MemOut != NULL)
        {
            WdfObjectDelete(ctx->MemOut);
            ctx->MemOut = NULL;
        }

        // Delete the request.
        WdfObjectDelete(m_MicStreamReq);
        m_MicStreamReq = NULL;
    }

    //
    // Notification work-item.
    //
    if (m_WorkItem != NULL)
    {
        WdfObjectDelete(m_WorkItem);
        m_WorkItem = NULL;
    }

    //
    // Notification req. collection.
    //
    if (m_ReqCollection != NULL)
    {
        WdfObjectDelete(m_ReqCollection);
        m_ReqCollection = NULL;
    }

    ASSERT(m_UnknownSpeakerTopology == NULL);
    SAFE_RELEASE(m_UnknownSpeakerTopology);

    ASSERT(m_UnknownSpeakerWave == NULL);
    SAFE_RELEASE(m_UnknownSpeakerWave);

    ASSERT(m_UnknownMicTopology == NULL);
    SAFE_RELEASE(m_UnknownMicTopology);

    ASSERT(m_UnknownMicWave == NULL);
    SAFE_RELEASE(m_UnknownMicWave);

    ASSERT(m_nSpeakerStreamsOpen == 0);
    ASSERT(m_nMicStreamsOpen == 0);

    ASSERT(m_nSpeakerStreamsStart == 0);
    ASSERT(m_nMicStreamsStart == 0);

    ASSERT(m_SpeakerVolumeCallback.Handler == NULL);
    ASSERT(m_MicVolumeCallback.Handler == NULL);

    ASSERT(m_SpeakerMuteCallback.Handler == NULL);
    ASSERT(m_MicMuteCallback.Handler == NULL);

    ASSERT(m_SpeakerConnectionStatusCallback.Handler == NULL);
    ASSERT(m_MicConnectionStatusCallback.Handler == NULL);

    ASSERT(m_SpeakerFormatChangeCallback.Handler == NULL);
    ASSERT(m_MicFormatChangeCallback.Handler == NULL);
} // ~CAdapterCommon  

//
// ISidebandDeviceCommon implementation. 
//

//=============================================================================
#pragma code_seg("PAGE")
BOOL
UsbHsDevice::IsVolumeSupported
(
    _In_ eDeviceType deviceType
)
{
    PAGED_CODE();
    DPF_ENTER(("[%!FUNC!]"));

    if (deviceType == eUsbHsSpeakerDevice)
    {
        return m_pSpeakerDescriptor->Capabilities.Volume;
    }
    else if (deviceType == eUsbHsMicDevice)
    {
        return m_pMicDescriptor->Capabilities.Volume;
    }

    return FALSE;
}

//=============================================================================
#pragma code_seg("PAGE")
PVOID
UsbHsDevice::GetVolumeSettings
(
    _In_  eDeviceType deviceType,
    _Out_ PULONG    Size 
)
{
    PAGED_CODE();
    DPF_ENTER(("[%!FUNC!]"));

    ASSERT(Size != NULL);

    if (deviceType == eUsbHsSpeakerDevice)
    {
        *Size = m_pSpeakerDescriptor->VolumePropertyValuesSize;
        return m_SpeakerVolumePropValues;
    }
    else if (deviceType == eUsbHsMicDevice)
    {
        *Size = m_pMicDescriptor->VolumePropertyValuesSize;
        return m_MicVolumePropValues;
    }

    return NULL;
}

//=============================================================================
#pragma code_seg("PAGE")
NTSTATUS
UsbHsDevice::GetVolume
(
    _In_    eDeviceType     deviceType,
    _In_    LONG            Channel,
    _Out_   LONG            *pVolume
)
{
    PAGED_CODE();
    UNREFERENCED_PARAMETER(Channel);
    DPF_ENTER(("[%!FUNC!]"));

    NTSTATUS status = STATUS_SUCCESS;

    ASSERT(pVolume);

    if (eUsbHsSpeakerDevice == deviceType)
    {
        status = GetUsbHsSpeakerVolume(Channel, pVolume);
        IF_FAILED_ACTION_JUMP(
            status,
            DPF(D_ERROR, ("Start: GetUsbHsSpeakerVolume: failed, 0x%x", status)),
            Done);

        InterlockedExchange(&m_SpeakerVolumeLevel, *pVolume);
    }
    else if (eUsbHsMicDevice == deviceType)
    {
        status = GetUsbHsMicVolume(Channel, pVolume);
        IF_FAILED_ACTION_JUMP(
            status,
            DPF(D_ERROR, ("Start: GetUsbHsMicVolume: failed, 0x%x", status)),
            Done);

        InterlockedExchange(&m_MicVolumeLevel, *pVolume);
    }
    else
    {
        status = STATUS_INVALID_PARAMETER;
    }

Done:
    return status;
}

//=============================================================================
#pragma code_seg("PAGE")
NTSTATUS
UsbHsDevice::SetVolume
(
    _In_    eDeviceType     deviceType,
    _In_    LONG            Channel,
    _In_    LONG            Volume
)
{
    PAGED_CODE();
    DPF_ENTER(("[%!FUNC!]"));

    NTSTATUS status = STATUS_SUCCESS;

    if (eUsbHsSpeakerDevice == deviceType)
    {
        status = SetUsbHsSpeakerVolume(Channel, Volume);
    }
    else if(eUsbHsMicDevice == deviceType)
    {
        status = SetUsbHsMicVolume(Channel, Volume);
    }
    else
    {
        status = STATUS_INVALID_PARAMETER;
    }

    return status;
}

//=============================================================================
#pragma code_seg("PAGE")
BOOL
UsbHsDevice::IsMuteSupported
(
    _In_ eDeviceType deviceType
)
{
    PAGED_CODE();
    DPF_ENTER(("[%!FUNC!]"));

    if (deviceType == eUsbHsSpeakerDevice)
    {
        return m_pSpeakerDescriptor->Capabilities.Mute;
    }
    else if (deviceType == eUsbHsMicDevice)
    {
        return m_pMicDescriptor->Capabilities.Mute;
    }

    return FALSE;
}

//=============================================================================
#pragma code_seg("PAGE")
PVOID
UsbHsDevice::GetMuteSettings
(
    _In_  eDeviceType deviceType,
    _Out_ PULONG    Size 
)
{
    PAGED_CODE();
    DPF_ENTER(("[%!FUNC!]"));

    ASSERT(Size != NULL);

    if (deviceType == eUsbHsSpeakerDevice)
    {
        *Size = m_pSpeakerDescriptor->MutePropertyValuesSize;
        return m_SpeakerMutePropValues;
    }
    else if (deviceType == eUsbHsMicDevice)
    {
        *Size = m_pMicDescriptor->MutePropertyValuesSize;
        return m_MicMutePropValues;
    }

    return NULL;
}

//=============================================================================
#pragma code_seg("PAGE")
LONG
UsbHsDevice::GetMute
(
    _In_    eDeviceType     deviceType,
    _In_    LONG            Channel
)
{
    PAGED_CODE();
    UNREFERENCED_PARAMETER(Channel);
    DPF_ENTER(("[%!FUNC!]"));

    LONG ulMute = 0;
    NTSTATUS status = STATUS_SUCCESS;

    if (eUsbHsSpeakerDevice == deviceType)
    {
        status = GetUsbHsSpeakerMute(Channel, &ulMute);
    }
    else if (eUsbHsMicDevice == deviceType)
    {
        status = GetUsbHsMicVolume(Channel, &ulMute);
    }
    else
    {
        status = STATUS_INVALID_PARAMETER;
    }

    if (NT_SUCCESS(status))
    {
        return ulMute;
    }

    return 0;
}

//=============================================================================
#pragma code_seg("PAGE")
NTSTATUS
UsbHsDevice::SetMute
(
    _In_    eDeviceType     deviceType,
    _In_    LONG            Channel,
    _In_    LONG            Mute
)
{
    PAGED_CODE();
    DPF_ENTER(("[%!FUNC!]"));

    NTSTATUS status = STATUS_SUCCESS;

    if (eUsbHsSpeakerDevice == deviceType)
    {
        status = SetUsbHsSpeakerMute(Channel, Mute);
    }
    else if(eUsbHsMicDevice == deviceType)
    {
        status = SetUsbHsMicMute(Channel, Mute);
    }
    else
    {
        status = STATUS_INVALID_PARAMETER;
    }

    return status;
}

//=============================================================================
#pragma code_seg("PAGE")
BOOL
UsbHsDevice::GetConnectionStatus()
{
    PAGED_CODE();
    DPF_ENTER(("[%!FUNC!]"));

    return TRUE;
}

//=============================================================================
#pragma code_seg("PAGE")
NTSTATUS
UsbHsDevice::Connect()
{
    PAGED_CODE();
    DPF_ENTER(("[%!FUNC!]"));

    return STATUS_DEVICE_FEATURE_NOT_SUPPORTED;
}

//=============================================================================
#pragma code_seg("PAGE")
NTSTATUS
UsbHsDevice::Disconnect()
{
    PAGED_CODE();
    DPF_ENTER(("[%!FUNC!]"));

    return STATUS_DEVICE_FEATURE_NOT_SUPPORTED;
}

//=============================================================================
#pragma code_seg()
BOOL
UsbHsDevice::GetStreamStatus(_In_ eDeviceType deviceType)
{
    DPF_ENTER(("[%!FUNC!]"));

    NTSTATUS ntStatus = STATUS_SUCCESS;

    if (deviceType == eUsbHsSpeakerDevice)
    {
        ntStatus = (NTSTATUS)InterlockedCompareExchange(&m_SpeakerStreamStatusLong, 0, 0);
    }
    else if (deviceType == eUsbHsMicDevice)
    {
        ntStatus = (NTSTATUS)InterlockedCompareExchange(&m_MicStreamStatusLong, 0, 0);
    }

    return NT_SUCCESS(ntStatus) ? TRUE : FALSE;
}

//=============================================================================
#pragma code_seg("PAGE")
NTSTATUS
UsbHsDevice::SpeakerStreamOpen()
{
    PAGED_CODE();
    DPF_ENTER(("[%!FUNC!]"));
    NTSTATUS    ntStatus    = STATUS_SUCCESS;
    LONG        nStreams    = 0;

    ASSERT(m_nSpeakerStreamsOpen >= 0);

    nStreams = InterlockedIncrement(&m_nSpeakerStreamsOpen);
    if (nStreams == 1)
    {
        BOOLEAN  streamOpen = FALSE;

        ntStatus = SetUsbHsStreamOpen(m_SpeakerEpIndex,
                                      &UsbHsSpeakerSupportedDeviceFormats[m_SpeakerSelectedFormat],
                                      m_pSpeakerDescriptor);

        if (NT_SUCCESS(ntStatus))
        {
            streamOpen = TRUE;
            m_SpeakerStreamStatus = STATUS_SUCCESS;
            ntStatus = EnableUsbHsSpeakerStreamStatusNotification();
        }

        //
        // Cleanup if any error.
        //
        if (!NT_SUCCESS(ntStatus))
        {
            nStreams = InterlockedDecrement(&m_nSpeakerStreamsOpen);
            ASSERT(nStreams == 0);
            UNREFERENCED_VAR(nStreams);

            if (streamOpen)
            {
                SetUsbHsStreamClose(m_SpeakerEpIndex);
            }

            m_SpeakerStreamStatus = STATUS_INVALID_DEVICE_STATE;
        }
    }

    return ntStatus;
}

//=============================================================================
#pragma code_seg("PAGE")
NTSTATUS
UsbHsDevice::MicStreamOpen()
{
    PAGED_CODE();
    DPF_ENTER(("[%!FUNC!]"));
    NTSTATUS    ntStatus    = STATUS_SUCCESS;
    LONG        nStreams    = 0;

    ASSERT(m_nMicStreamsOpen >= 0);

    nStreams = InterlockedIncrement(&m_nMicStreamsOpen);
    if (nStreams == 1)
    {
        BOOLEAN  streamOpen = FALSE;

        ntStatus = SetUsbHsStreamOpen(m_MicEpIndex,
                                      &UsbHsMicSupportedDeviceFormats[m_MicSelectedFormat],
                                      m_pMicDescriptor);

        if (NT_SUCCESS(ntStatus))
        {
            streamOpen = TRUE;
            m_MicStreamStatus = STATUS_SUCCESS;
            ntStatus = EnableUsbHsMicStreamStatusNotification();
        }

        //
        // Cleanup if any error.
        //
        if (!NT_SUCCESS(ntStatus))
        {
            nStreams = InterlockedDecrement(&m_nMicStreamsOpen);
            ASSERT(nStreams == 0);
            UNREFERENCED_VAR(nStreams);

            if (streamOpen)
            {
                SetUsbHsStreamClose(m_MicEpIndex);
            }

            m_MicStreamStatus = STATUS_INVALID_DEVICE_STATE;
        }
    }

    return ntStatus;
}

//=============================================================================
#pragma code_seg("PAGE")
NTSTATUS
UsbHsDevice::StreamOpen(_In_ eDeviceType deviceType)
{
    PAGED_CODE();
    DPF_ENTER(("[%!FUNC!]"));

    if (deviceType == eUsbHsSpeakerDevice)
    {
        return SpeakerStreamOpen();
    }
    else if (deviceType == eUsbHsMicDevice)
    {
        return MicStreamOpen();
    }

    return STATUS_INVALID_PARAMETER;
}

//=============================================================================
#pragma code_seg("PAGE")
NTSTATUS
UsbHsDevice::SpeakerStreamStart()
{
    PAGED_CODE();
    DPF_ENTER(("[%!FUNC!]"));
    NTSTATUS    ntStatus    = STATUS_SUCCESS;
    LONG        nStreams    = 0;

    ASSERT(m_nSpeakerStreamsStart >= 0);

    nStreams = InterlockedIncrement(&m_nSpeakerStreamsStart);
    if (nStreams == 1)
    {
        BOOLEAN  streamStart = FALSE;

        ntStatus = SetTransportResources(m_pSpeakerDescriptor, m_SpeakerEpIndex);

        if (NT_SUCCESS(ntStatus))
        {
            ntStatus = SetUsbHsStreamStart(m_SpeakerEpIndex);
        }

        if (NT_SUCCESS(ntStatus))
        {
            streamStart = TRUE;
        }

        if (NT_SUCCESS(ntStatus))
        {
            ntStatus = GetTransportResources(m_SpeakerEpIndex, m_pSpeakerDescriptor, &m_SpeakerTransportResources);
        }

        //
        // Cleanup if any error.
        //
        if (!NT_SUCCESS(ntStatus))
        {
            nStreams = InterlockedDecrement(&m_nSpeakerStreamsStart);
            ASSERT(nStreams == 0);
            UNREFERENCED_VAR(nStreams);

            if (streamStart)
            {
                SetUsbHsStreamSuspend(m_SpeakerEpIndex);
            }
        }
    }

    return ntStatus;
}

//=============================================================================
#pragma code_seg("PAGE")
NTSTATUS
UsbHsDevice::MicStreamStart()
{
    PAGED_CODE();
    DPF_ENTER(("[%!FUNC!]"));
    NTSTATUS    ntStatus    = STATUS_SUCCESS;
    LONG        nStreams    = 0;

    ASSERT(m_nMicStreamsStart >= 0);

    nStreams = InterlockedIncrement(&m_nMicStreamsStart);
    if (nStreams == 1)
    {
        BOOLEAN  streamStart = FALSE;

        ntStatus = SetTransportResources(m_pMicDescriptor, m_MicEpIndex);

        if (NT_SUCCESS(ntStatus))
        {
            ntStatus = SetUsbHsStreamStart(m_MicEpIndex);
        }

        if (NT_SUCCESS(ntStatus))
        {
            streamStart = TRUE;
        }

        if (NT_SUCCESS(ntStatus))
        {
            ntStatus = GetTransportResources(m_MicEpIndex, m_pMicDescriptor, &m_MicTransportResources);
        }

        //
        // Cleanup if any error.
        //
        if (!NT_SUCCESS(ntStatus))
        {
            nStreams = InterlockedDecrement(&m_nMicStreamsStart);
            ASSERT(nStreams == 0);
            UNREFERENCED_VAR(nStreams);

            if (streamStart)
            {
                SetUsbHsStreamSuspend(m_MicEpIndex);
            }
        }
    }

    return ntStatus;
}

//=============================================================================
#pragma code_seg("PAGE")
NTSTATUS
UsbHsDevice::StreamStart(_In_ eDeviceType deviceType)
{
    PAGED_CODE();
    DPF_ENTER(("[%!FUNC!]"));

    if (deviceType == eUsbHsSpeakerDevice)
    {
        return SpeakerStreamStart();
    }
    else if (deviceType == eUsbHsMicDevice)
    {
        return MicStreamStart();
    }

    return STATUS_INVALID_PARAMETER;
}

//=============================================================================
#pragma code_seg("PAGE")
NTSTATUS
UsbHsDevice::SpeakerStreamSuspend()
{
    PAGED_CODE();
    DPF_ENTER(("[%!FUNC!]"));

    NTSTATUS    ntStatus    = STATUS_SUCCESS;
    LONG        nStreams    = 0;

    ASSERT(m_nSpeakerStreamsStart > 0);

    nStreams = InterlockedDecrement(&m_nSpeakerStreamsStart);
    if (nStreams == 0)
    {
        ntStatus = SetUsbHsStreamSuspend(m_SpeakerEpIndex);

        FreeTransportResources(&m_SpeakerTransportResources);
    }

    return ntStatus;
}

//=============================================================================
#pragma code_seg("PAGE")
NTSTATUS
UsbHsDevice::MicStreamSuspend()
{
    PAGED_CODE();
    DPF_ENTER(("[%!FUNC!]"));

    NTSTATUS    ntStatus    = STATUS_SUCCESS;
    LONG        nStreams    = 0;

    ASSERT(m_nMicStreamsStart > 0);

    nStreams = InterlockedDecrement(&m_nMicStreamsStart);
    if (nStreams == 0)
    {
        ntStatus = SetUsbHsStreamSuspend(m_MicEpIndex);

        FreeTransportResources(&m_MicTransportResources);
    }

    return ntStatus;
}

//=============================================================================
#pragma code_seg("PAGE")
NTSTATUS
UsbHsDevice::StreamSuspend(_In_ eDeviceType deviceType)
{
    PAGED_CODE();
    DPF_ENTER(("[%!FUNC!]"));

    if (deviceType == eUsbHsSpeakerDevice)
    {
        return SpeakerStreamSuspend();
    }
    else if (deviceType == eUsbHsMicDevice)
    {
        return MicStreamSuspend();
    }

    return STATUS_INVALID_PARAMETER;
}

//=============================================================================
#pragma code_seg("PAGE")
NTSTATUS
UsbHsDevice::SpeakerStreamClose()
{
    PAGED_CODE();
    DPF_ENTER(("[%!FUNC!]"));

    NTSTATUS    ntStatus    = STATUS_SUCCESS;
    LONG        nStreams    = 0;

    ASSERT(m_nSpeakerStreamsOpen > 0);

    nStreams = InterlockedDecrement(&m_nSpeakerStreamsOpen);
    if (nStreams == 0)
    {
        ntStatus = SetUsbHsStreamClose(m_SpeakerEpIndex);

        StopUsbHsSpeakerStreamStatusNotification();

        m_SpeakerStreamStatus = STATUS_INVALID_DEVICE_STATE;
    }

    return ntStatus;
}

//=============================================================================
#pragma code_seg("PAGE")
NTSTATUS
UsbHsDevice::MicStreamClose()
{
    PAGED_CODE();
    DPF_ENTER(("[%!FUNC!]"));

    NTSTATUS    ntStatus    = STATUS_SUCCESS;
    LONG        nStreams    = 0;

    ASSERT(m_nMicStreamsOpen > 0);

    nStreams = InterlockedDecrement(&m_nMicStreamsOpen);
    if (nStreams == 0)
    {
        ntStatus = SetUsbHsStreamClose(m_MicEpIndex);

        StopUsbHsMicStreamStatusNotification();

        m_MicStreamStatus = STATUS_INVALID_DEVICE_STATE;
    }

    return ntStatus;
}

//=============================================================================
#pragma code_seg("PAGE")
NTSTATUS
UsbHsDevice::StreamClose(_In_ eDeviceType deviceType)
{
    PAGED_CODE();
    DPF_ENTER(("[%!FUNC!]"));

    if (deviceType == eUsbHsSpeakerDevice)
    {
        return SpeakerStreamClose();
    }
    else if (deviceType == eUsbHsMicDevice)
    {
        return MicStreamClose();
    }

    return STATUS_INVALID_PARAMETER;
}

//=============================================================================
#pragma code_seg("PAGE")
GUID
UsbHsDevice::GetContainerId(_In_ eDeviceType deviceType)
{
    PAGED_CODE();
    DPF_ENTER(("[%!FUNC!]"));

    if (deviceType == eUsbHsSpeakerDevice)
    {
        return m_pSpeakerDescriptor->ContainerId;
    }
    else if (deviceType == eUsbHsMicDevice)
    {
        return m_pMicDescriptor->ContainerId;
    }
    else
        return { 0 };
}

//=============================================================================
#pragma code_seg("PAGE")
VOID
UsbHsDevice::SetVolumeHandler
(
    _In_        eDeviceType             deviceType,
    _In_opt_    PFNEVENTNOTIFICATION    EventHandler,
    _In_opt_    PVOID                   EventHandlerContext
)
{
    PAGED_CODE();
    DPF_ENTER(("[%!FUNC!]"));

    if (deviceType == eUsbHsSpeakerDevice)
    {
        ASSERT(EventHandler == NULL || m_SpeakerVolumeCallback.Handler == NULL);

        m_SpeakerVolumeCallback.Handler = EventHandler; // weak ref.
        m_SpeakerVolumeCallback.Context = EventHandlerContext;
    }
    else if (deviceType == eUsbHsMicDevice)
    {
        ASSERT(EventHandler == NULL || m_MicVolumeCallback.Handler == NULL);

        m_MicVolumeCallback.Handler = EventHandler; // weak ref.
        m_MicVolumeCallback.Context = EventHandlerContext;
    }
}

//=============================================================================
#pragma code_seg("PAGE")
VOID
UsbHsDevice::SetMuteHandler
(
    _In_        eDeviceType             deviceType,
    _In_opt_    PFNEVENTNOTIFICATION    EventHandler,
    _In_opt_    PVOID                   EventHandlerContext
)
{
    PAGED_CODE();
    DPF_ENTER(("[%!FUNC!]"));

    if (deviceType == eUsbHsSpeakerDevice)
    {
        ASSERT(EventHandler == NULL || m_SpeakerMuteCallback.Handler == NULL);

        m_SpeakerMuteCallback.Handler = EventHandler; // weak ref.
        m_SpeakerMuteCallback.Context = EventHandlerContext;
    }
    else if (deviceType == eUsbHsMicDevice)
    {
        ASSERT(EventHandler == NULL || m_MicMuteCallback.Handler == NULL);

        m_MicMuteCallback.Handler = EventHandler; // weak ref.
        m_MicMuteCallback.Context = EventHandlerContext;
    }
}

//=============================================================================
#pragma code_seg("PAGE")
VOID
UsbHsDevice::SetConnectionStatusHandler
(
    _In_        eDeviceType             deviceType,
    _In_opt_    PFNEVENTNOTIFICATION    EventHandler,
    _In_opt_    PVOID                   EventHandlerContext
)
{
    PAGED_CODE();
    DPF_ENTER(("[%!FUNC!]"));

    if (deviceType == eUsbHsSpeakerDevice)
    {
        ASSERT(EventHandler == NULL || m_SpeakerConnectionStatusCallback.Handler == NULL);

        m_SpeakerConnectionStatusCallback.Handler = EventHandler; // weak ref.
        m_SpeakerConnectionStatusCallback.Context = EventHandlerContext;
    }
    else if (deviceType == eUsbHsMicDevice)
    {
        ASSERT(EventHandler == NULL || m_MicConnectionStatusCallback.Handler == NULL);

        m_MicConnectionStatusCallback.Handler = EventHandler; // weak ref.
        m_MicConnectionStatusCallback.Context = EventHandlerContext;
    }

}

//=============================================================================
#pragma code_seg("PAGE")
VOID
UsbHsDevice::SetFormatChangeHandler
(
    _In_        eDeviceType             deviceType,
    _In_opt_    PFNEVENTNOTIFICATION    EventHandler,
    _In_opt_    PVOID                   EventHandlerContext
)
{
    PAGED_CODE();
    DPF_ENTER(("[%!FUNC!]"));

    if (deviceType == eUsbHsSpeakerDevice)
    {
        ASSERT(EventHandler == NULL || m_SpeakerFormatChangeCallback.Handler == NULL);

        m_SpeakerFormatChangeCallback.Handler = EventHandler; // weak ref.
        m_SpeakerFormatChangeCallback.Context = EventHandlerContext;
    }
    else if (deviceType == eUsbHsMicDevice)
    {
        ASSERT(EventHandler == NULL || m_MicFormatChangeCallback.Handler == NULL);

        m_MicFormatChangeCallback.Handler = EventHandler; // weak ref.
        m_MicFormatChangeCallback.Context = EventHandlerContext;
    }

}

//=============================================================================
#pragma code_seg("PAGE")
PPIN_DEVICE_FORMATS_AND_MODES
UsbHsDevice::GetFormatsAndModes
(
    _In_        eDeviceType             deviceType
)
{
    PAGED_CODE();

    UNREFERENCED_PARAMETER(deviceType);
    PPIN_DEVICE_FORMATS_AND_MODES pFormatsAndModes = NULL;
    return pFormatsAndModes;
}

//=============================================================================
#pragma code_seg()
_IRQL_requires_max_(DISPATCH_LEVEL)
BOOL
UsbHsDevice::IsNRECSupported()
{
    DPF_ENTER(("[%!FUNC!]"));

    return FALSE;
}

//=============================================================================
#pragma code_seg("PAGE")
BOOL
UsbHsDevice::GetNRECDisableStatus()
{
    PAGED_CODE();
    DPF_ENTER(("[%!FUNC!]"));

    // Return TRUE if HF wants to disable the NREC on the AG.
    return FALSE;
}

//
// Helper functions.
//

//=============================================================================
#pragma code_seg()
NTSTATUS 
UsbHsDevice::SendIoCtrlAsynchronously
(
    _In_        WDFREQUEST      Request,
    _In_        ULONG           IoControlCode,
    _In_opt_    WDFMEMORY       MemIn,
    _In_opt_    WDFMEMORY       MemOut,
    _In_        PFN_WDF_REQUEST_COMPLETION_ROUTINE CompletionRoutine,
    _In_        WDFCONTEXT      Context
)
/*++

Routine Description:

  This function aynchronously sends an I/O Ctrl request to the USB Headset
  device.

--*/
{
    DPF_ENTER(("[%!FUNC!]"));

    NTSTATUS    ntStatus    = STATUS_SUCCESS;
    BOOLEAN     fSent       = FALSE;

    //
    // Format and send the request.
    //
    ntStatus = WdfIoTargetFormatRequestForIoctl(
        m_WdfIoTarget,
        Request,
        IoControlCode,
        MemIn,
        NULL,
        MemOut,
        NULL);

    IF_FAILED_ACTION_JUMP(
        ntStatus,
        DPF(D_ERROR, ("%!FUNC!: WdfIoTargetFormatRequestForIoctl(0x%x) failed, 0x%x", IoControlCode, ntStatus)),
        Done);

    WdfRequestSetCompletionRoutine(
        Request,
        CompletionRoutine,
        Context);

    fSent = WdfRequestSend(Request, m_WdfIoTarget, NULL);  // no options.
    if (fSent == FALSE)
    {
        ntStatus = WdfRequestGetStatus(Request);
        if (NT_SUCCESS(ntStatus))
        {
            ntStatus = STATUS_INVALID_DEVICE_STATE;
        }

        DPF(D_ERROR, ("%!FUNC!: WdfRequestSend(0x%x) failed, 0x%x", IoControlCode, ntStatus));
        goto Done;
    }

    //
    // All Done.
    //
    ntStatus = STATUS_SUCCESS;

Done:

    return ntStatus;
}

//=============================================================================
#pragma code_seg("PAGE")
NTSTATUS 
UsbHsDevice::SendIoCtrlSynchronously
(
    _In_opt_    WDFREQUEST  Request,
    _In_        ULONG       IoControlCode,
    _In_        ULONG       InLength,
    _In_        ULONG       OutLength,
    _When_(InLength > 0 || OutLength > 0, _In_)
    _When_(InLength == 0 && OutLength == 0, _In_opt_)
                PVOID       Buffer
)
/*++

Routine Description:

  This function inits and synchronously sends an I/O Ctrl request to the USB headset
  device.

--*/
{
    PAGED_CODE();
    DPF_ENTER(("[%!FUNC!]"));

    NTSTATUS                    ntStatus    = STATUS_SUCCESS;
    PWDF_MEMORY_DESCRIPTOR      memInPtr    = NULL;
    PWDF_MEMORY_DESCRIPTOR      memOutPtr   = NULL;
    WDF_MEMORY_DESCRIPTOR       memIn;
    WDF_MEMORY_DESCRIPTOR       memOut;
    WDF_REQUEST_SEND_OPTIONS    reqOpts;

    //
    // Format and send the request.
    //
    if (InLength)
    {
        WDF_MEMORY_DESCRIPTOR_INIT_BUFFER(&memIn,  Buffer, InLength);
        memInPtr = &memIn;
    }

    if (OutLength)
    {
        WDF_MEMORY_DESCRIPTOR_INIT_BUFFER(&memOut, Buffer, OutLength);
        memOutPtr = &memOut;
    }

    WDF_REQUEST_SEND_OPTIONS_INIT(
        &reqOpts, 
        WDF_REQUEST_SEND_OPTION_TIMEOUT | 
         WDF_REQUEST_SEND_OPTION_SYNCHRONOUS);

    reqOpts.Timeout = WDF_REL_TIMEOUT_IN_SEC(USB_SIDEBAND_SYNC_REQ_TIMEOUT_IN_SEC);

    ntStatus = WdfIoTargetSendIoctlSynchronously(
        m_WdfIoTarget,
        Request,
        IoControlCode,
        memInPtr,
        memOutPtr,
        &reqOpts,
        NULL);      // bytes returned.

    IF_FAILED_ACTION_JUMP(
        ntStatus,
        DPF(D_VERBOSE, ("%!FUNC!: WdfIoTargetSendIoctlSynchronously(0x%x) failed, 0x%x", IoControlCode, ntStatus)),
        Done);

Done:

    return ntStatus;
}

//=============================================================================
#pragma code_seg("PAGE")
NTSTATUS 
UsbHsDevice::SendIoCtrlSynchronously
(
    _In_opt_    WDFREQUEST  Request,
    _In_        ULONG       IoControlCode,
    _In_        ULONG       InLength,
    _In_        ULONG       OutLength,
    _When_(InLength > 0, _In_)
    _When_(InLength == 0, _In_opt_)
                PVOID       InBuffer,
    _When_(OutLength > 0, _In_)
    _When_(OutLength == 0, _In_opt_)
                PVOID       OutBuffer
)
/*++

Routine Description:

  This function inits and synchronously sends an I/O Ctrl request to the USB headset
  device.

--*/
{
    PAGED_CODE();
    DPF_ENTER(("[%!FUNC!]"));

    NTSTATUS                    ntStatus    = STATUS_SUCCESS;
    PWDF_MEMORY_DESCRIPTOR      memInPtr    = NULL;
    PWDF_MEMORY_DESCRIPTOR      memOutPtr   = NULL;
    WDF_MEMORY_DESCRIPTOR       memIn;
    WDF_MEMORY_DESCRIPTOR       memOut;
    WDF_REQUEST_SEND_OPTIONS    reqOpts;

    //
    // Format and send the request.
    //
    if (InLength)
    {
        WDF_MEMORY_DESCRIPTOR_INIT_BUFFER(&memIn,  InBuffer, InLength);
        memInPtr = &memIn;
    }

    if (OutLength)
    {
        WDF_MEMORY_DESCRIPTOR_INIT_BUFFER(&memOut, OutBuffer, OutLength);
        memOutPtr = &memOut;
    }

    WDF_REQUEST_SEND_OPTIONS_INIT(
        &reqOpts, 
        WDF_REQUEST_SEND_OPTION_TIMEOUT | 
         WDF_REQUEST_SEND_OPTION_SYNCHRONOUS);

    reqOpts.Timeout = WDF_REL_TIMEOUT_IN_SEC(USB_SIDEBAND_SYNC_REQ_TIMEOUT_IN_SEC);

    ntStatus = WdfIoTargetSendIoctlSynchronously(
        m_WdfIoTarget,
        Request,
        IoControlCode,
        memInPtr,
        memOutPtr,
        &reqOpts,
        NULL);      // bytes returned.

    IF_FAILED_ACTION_JUMP(
        ntStatus,
        DPF(D_VERBOSE, ("%!FUNC!: WdfIoTargetSendIoctlSynchronously(0x%x) failed, 0x%x", IoControlCode, ntStatus)),
        Done);

Done:

    return ntStatus;
}

//=============================================================================
#pragma code_seg()
VOID
UsbHsDevice::EvtUsbHsDeviceNotificationStatusWorkItem
(
    _In_    WDFWORKITEM WorkItem
)
/*++

Routine Description:

  The function processes status notification updates.

Arguments:

  WorkItem    - WDF work-item object.

--*/
{
    DPF_ENTER(("[%!FUNC!]"));

    NTSTATUS            ntStatus = STATUS_SUCCESS;
    UsbHsDevice      * This;
    KIRQL               oldIrql;

    if (WorkItem == NULL) 
    {
        return;
    }

    This = GetUsbHsDeviceNotificationWorkItemContext(WorkItem)->UsbHsDevice;
    ASSERT(This != NULL);

    for (;;)
    {
        BOOL                                    resend  = TRUE; 
        WDFREQUEST                              req     = NULL;
        UsbHsDeviceNotificationReqContext    * reqCtx;
        WDF_REQUEST_COMPLETION_PARAMS           params;

        //
        // Retrieve a task.
        //
        KeAcquireSpinLock(&This->m_Lock, &oldIrql);
        req = (WDFREQUEST) WdfCollectionGetFirstItem(This->m_ReqCollection);
        if (req != NULL)
        {
            WdfCollectionRemove(This->m_ReqCollection, req);
        }
        KeReleaseSpinLock(&This->m_Lock, oldIrql);

        if (req == NULL)
        {
            break;
        }

        //
        // Get request parameters and context.
        //
        WDF_REQUEST_COMPLETION_PARAMS_INIT(&params);
        WdfRequestGetCompletionParams(req, &params);

        reqCtx = GetUsbHsDeviceNotificationReqContext(req);
        ASSERT(reqCtx != NULL);

        //
        // Handle this notification.
        //
        if (NT_SUCCESS(params.IoStatus.Status))
        {
            switch(params.Parameters.Ioctl.IoControlCode)
            {
            case IOCTL_SBAUD_GET_VOLUME_STATUS_UPDATE:
                {
                    LONG oldVolume;

                    // Prepare for request resent
                    reqCtx->Buffer.Volume.Immediate = FALSE;

                    if (reqCtx->Buffer.Volume.EpIndex == This->m_SpeakerEpIndex)
                    {
                        oldVolume = InterlockedExchange(&This->m_SpeakerVolumeLevel, reqCtx->Buffer.Volume.Value);
                        if (reqCtx->Buffer.Volume.Value != oldVolume)
                        {
                            // Notify audio miniport about this change.
                            if (This->m_SpeakerVolumeCallback.Handler != NULL)
                            {
                                This->m_SpeakerVolumeCallback.Handler(
                                    This->m_SpeakerVolumeCallback.Context);
                            }
                        }
                    }
                    else if (reqCtx->Buffer.Volume.EpIndex == This->m_MicEpIndex)
                    {
                        oldVolume = InterlockedExchange(&This->m_MicVolumeLevel, reqCtx->Buffer.Volume.Value);
                        if (reqCtx->Buffer.Volume.Value != oldVolume)
                        {
                            // Notify audio miniport about this change.
                            if (This->m_MicVolumeCallback.Handler != NULL)
                            {
                                This->m_MicVolumeCallback.Handler(
                                    This->m_MicVolumeCallback.Context);
                            }
                        }
                    }

                }
                break;

            case IOCTL_SBAUD_GET_MUTE_STATUS_UPDATE:
                {
                    LONG oldMute;

                    // Prepare for request resent
                    reqCtx->Buffer.Mute.Immediate = FALSE;

                    if (reqCtx->Buffer.Mute.EpIndex == This->m_SpeakerEpIndex)
                    {
                        oldMute = InterlockedExchange(&This->m_SpeakerMute, reqCtx->Buffer.Mute.Value);
                        if (reqCtx->Buffer.Mute.Value != oldMute)
                        {
                            // Notify audio miniport about this change.
                            if (This->m_SpeakerMuteCallback.Handler != NULL)
                            {
                                This->m_SpeakerMuteCallback.Handler(
                                    This->m_SpeakerMuteCallback.Context);
                            }
                        }
                    }
                    else if (reqCtx->Buffer.Mute.EpIndex == This->m_MicEpIndex)
                    {
                        oldMute = InterlockedExchange(&This->m_MicMute, reqCtx->Buffer.Mute.Value);
                        if (reqCtx->Buffer.Mute.Value != oldMute)
                        {
                            // Notify audio miniport about this change.
                            if (This->m_MicMuteCallback.Handler != NULL)
                            {
                                This->m_MicMuteCallback.Handler(
                                    This->m_MicMuteCallback.Context);
                            }
                        }
                    }

                }
                break;

            default:
                // This should never happen.
                resend = FALSE;
                DPF(D_ERROR, ("%!FUNC!: invalid request ctrl 0x%x", 
                    params.Parameters.Ioctl.IoControlCode));
                break;
            }
        }

        if (resend)
        {
            WDF_REQUEST_REUSE_PARAMS    reuseParams;

            WDF_REQUEST_REUSE_PARAMS_INIT(
                &reuseParams,   
                WDF_REQUEST_REUSE_NO_FLAGS,
                STATUS_SUCCESS);

            ntStatus = WdfRequestReuse(req, &reuseParams);
            if (!NT_SUCCESS(ntStatus))
            {
                DPF(D_ERROR, ("%!FUNC!: WdfRequestReuse failed, 0x%x", ntStatus));
                break;
            }

            // Resend status notification request.
            ntStatus = This->SendIoCtrlAsynchronously(
                req,
                params.Parameters.Ioctl.IoControlCode,
                reqCtx->MemIn,
                reqCtx->MemOut,
                EvtUsbHsDeviceNotificationStatusCompletion,
                This);

            if (!NT_SUCCESS(ntStatus))
            {
                DPF(D_ERROR, ("%!FUNC!: SendIoCtrlAsynchronously"
                              "(0x%x) failed, 0x%x", 
                              params.Parameters.Ioctl.IoControlCode, ntStatus));
                break;
            }
        }
    }
}

//=============================================================================
#pragma code_seg()
void 
UsbHsDevice::EvtUsbHsDeviceNotificationStatusCompletion
(
  _In_  WDFREQUEST Request,
  _In_  WDFIOTARGET Target,
  _In_  PWDF_REQUEST_COMPLETION_PARAMS Params,
  _In_  WDFCONTEXT Context
)
{
    DPF_ENTER(("[%!FUNC!]"));

    NTSTATUS                            ntStatus    = STATUS_SUCCESS;
    UsbHsDeviceNotificationReqContext   *ctx        = NULL;
    UsbHsDevice                         *This       = NULL;
    KIRQL                               oldIrql;

    UNREFERENCED_PARAMETER(Target);
    UNREFERENCED_PARAMETER(Context);

    ctx = GetUsbHsDeviceNotificationReqContext(Request);
    This = ctx->UsbHsDevice;
    ASSERT(This != NULL);

    ntStatus = Params->IoStatus.Status;
    if (ntStatus == STATUS_CANCELLED)
    {
        // USB device is shutting down. Do not re-send this request.
        goto Done;
    }

    //
    // If something is wrong with the USB interface, do not loop forever.
    //
    if (!NT_SUCCESS(ntStatus))
    {
        if (++ctx->Errors > USB_SIDEBAND_NOTIFICATION_MAX_ERROR_COUNT)
        {
            // Too many errors. Do not re-send this request.
            goto Done;
        }
    }
    else
    {
        // reset the # of errors.
        ctx->Errors = 0;
    }

    // 
    // Let the work-item thread process this request.
    //
    KeAcquireSpinLock(&This->m_Lock, &oldIrql);

    ntStatus = WdfCollectionAdd(This->m_ReqCollection, Request);
    if (NT_SUCCESS(ntStatus))
    {
        WdfWorkItemEnqueue(This->m_WorkItem);
    }

    KeReleaseSpinLock(&This->m_Lock, oldIrql);

Done:;
}

//=============================================================================
#pragma code_seg("PAGE")
NTSTATUS 
UsbHsDevice::GetUsbHsDescriptor
(
    _Out_ PSIDEBANDAUDIO_DEVICE_DESCRIPTOR *Descriptor
)
/*++

Routine Description:

  This function synchronously gets the remote USB Headset Device descriptor.

--*/
{
    PAGED_CODE();
    DPF_ENTER(("[%!FUNC!]"));

    NTSTATUS                                ntStatus    = STATUS_SUCCESS;
    WDFREQUEST                              req         = NULL;
    PSIDEBANDAUDIO_DEVICE_DESCRIPTOR     descriptor  = NULL;
    ULONG                                   length      = 0;
    ULONG_PTR                               information = 0;
    WDF_REQUEST_REUSE_PARAMS                reuseParams;   
    WDF_OBJECT_ATTRIBUTES                   attributes;

    *Descriptor = NULL;

    //
    // Allocate and format a WDF request.
    //
    WDF_OBJECT_ATTRIBUTES_INIT(&attributes);
    attributes.ParentObject = m_Adapter->GetWdfDevice();
    ntStatus = WdfRequestCreate(
        &attributes,
        m_WdfIoTarget,
        &req);
    IF_FAILED_ACTION_JUMP(
        ntStatus,
        DPF(D_ERROR, ("%!FUNC!: WdfRequestCreate failed, 0x%x", ntStatus)),
        Done);

    //
    // Get the size of the buffer.
    //
    ntStatus = SendIoCtrlSynchronously(
        req,
        IOCTL_SBAUD_GET_DEVICE_DESCRIPTOR,
        NULL,
        NULL,
        NULL);

    if (ntStatus != STATUS_BUFFER_TOO_SMALL)
    {
        if (NT_SUCCESS(ntStatus))
        {
            ntStatus = STATUS_INVALID_DEVICE_STATE;
        }

        DPF(D_ERROR, ("%!FUNC!: SendIoCtrlSynchronously(IOCTL_SBAUD_GET_DEVICE_DESCRIPTOR): failed, 0x%x", ntStatus));
        goto Done;
    }

    ntStatus = STATUS_SUCCESS;

    information = WdfRequestGetInformation(req);
    if (information == 0 || information > ULONG_MAX)
    {
        ntStatus = STATUS_INVALID_DEVICE_STATE;        
        DPF(D_ERROR, ("%!FUNC!: IOCTL_SBAUD_GET_DEVICE_DESCRIPTOR buffer too big (%Id): 0x%x", information, ntStatus));
        goto Done;
    }

    length = (ULONG)information;

    //
    // Allocate memory needed to hold the info.
    //
    descriptor  = (PSIDEBANDAUDIO_DEVICE_DESCRIPTOR) ExAllocatePool2(POOL_FLAG_NON_PAGED, length, MINADAPTER_POOLTAG);
    if (descriptor == NULL)
    {
        ntStatus = STATUS_INSUFFICIENT_RESOURCES;
    }
    IF_FAILED_ACTION_JUMP(
        ntStatus,
        DPF(D_ERROR, ("%!FUNC!: ExAllocatePool2 failed, out of memory")),
        Done);

    //
    // Get the USB Headset descriptor.
    //
    WDF_REQUEST_REUSE_PARAMS_INIT(
        &reuseParams,   
        WDF_REQUEST_REUSE_NO_FLAGS,
        STATUS_SUCCESS);

    ntStatus = WdfRequestReuse(req, &reuseParams);
    IF_FAILED_ACTION_JUMP(
        ntStatus,
        DPF(D_ERROR, ("%!FUNC!: WdfRequestReuse failed, 0x%x", ntStatus)),
        Done);

    ntStatus = SendIoCtrlSynchronously(
        req,
        IOCTL_SBAUD_GET_DEVICE_DESCRIPTOR,
        NULL,
        length,
        descriptor);
    IF_FAILED_ACTION_JUMP(
        ntStatus,
        DPF(D_ERROR, ("%!FUNC!: SendIoCtrlSynchronously(IOCTL_SBAUD_GET_DEVICE_DESCRIPTOR) failed, 0x%x", ntStatus)),
        Done);

    //
    // All done.
    //
    *Descriptor = descriptor;
    ntStatus = STATUS_SUCCESS;

Done:
    if (!NT_SUCCESS(ntStatus))
    {
        if (descriptor != NULL)
        {
            ExFreePoolWithTag(descriptor, MINADAPTER_POOLTAG);
        }   
    }

    if (req != NULL)
    {
        WdfObjectDelete(req);
        req = NULL;
    }

    return ntStatus;
}

//=============================================================================
#pragma code_seg("PAGE")
NTSTATUS 
UsbHsDevice::GetUsbHsEndpointDescriptor
(
    _In_    ULONG                               EpIndex,
    _Out_   PSIDEBANDAUDIO_ENDPOINT_DESCRIPTOR2 *EpDescriptor
)
/*++

Routine Description:

  This function synchronously gets the USB Headset Endpoint Descriptor.

--*/
{
    PAGED_CODE();
    DPF_ENTER(("[%!FUNC!]"));

    NTSTATUS                                ntStatus    = STATUS_SUCCESS;
    WDFREQUEST                              req         = NULL;
    PSIDEBANDAUDIO_ENDPOINT_DESCRIPTOR2     descriptor  = NULL;
    ULONG                                   length      = 0;
    ULONG_PTR                               information = 0;
    WDF_REQUEST_REUSE_PARAMS                reuseParams;   
    WDF_OBJECT_ATTRIBUTES                   attributes;

    *EpDescriptor = NULL;

    //
    // Allocate and format a WDF request.
    //
    WDF_OBJECT_ATTRIBUTES_INIT(&attributes);
    attributes.ParentObject = m_Adapter->GetWdfDevice();
    ntStatus = WdfRequestCreate(
        &attributes,
        m_WdfIoTarget,
        &req);
    IF_FAILED_ACTION_JUMP(
        ntStatus,
        DPF(D_ERROR, ("%!FUNC!: WdfRequestCreate failed, 0x%x", ntStatus)),
        Done);

    //
    // Get the size of the buffer.
    //
    ntStatus = SendIoCtrlSynchronously(
        req,
        IOCTL_SBAUD_GET_ENDPOINT_DESCRIPTOR2,
        sizeof(EpIndex),
        NULL,
        &EpIndex);

    if (ntStatus != STATUS_BUFFER_TOO_SMALL)
    {
        if (NT_SUCCESS(ntStatus))
        {
            ntStatus = STATUS_INVALID_DEVICE_STATE;
        }

        DPF(D_ERROR, ("%!FUNC!: SendIoCtrlSynchronously(IOCTL_SBAUD_GET_ENDPOINT_DESCRIPTOR): failed, 0x%x", ntStatus));
        goto Done;
    }

    ntStatus = STATUS_SUCCESS;

    information = WdfRequestGetInformation(req);
    if (information == 0 || information > ULONG_MAX)
    {
        ntStatus = STATUS_INVALID_DEVICE_STATE;        
        DPF(D_ERROR, ("%!FUNC!: IOCTL_SBAUD_GET_ENDPOINT_DESCRIPTOR buffer too big (%Id): 0x%x", information, ntStatus));
        goto Done;
    }

    length = (ULONG)information;

    //
    // Allocate memory needed to hold the info.
    //
    descriptor = (PSIDEBANDAUDIO_ENDPOINT_DESCRIPTOR2) ExAllocatePool2(POOL_FLAG_NON_PAGED, length, MINADAPTER_POOLTAG);
    if (descriptor == NULL)
    {
        ntStatus = STATUS_INSUFFICIENT_RESOURCES;
    }
    IF_FAILED_ACTION_JUMP(
        ntStatus,
        DPF(D_ERROR, ("%!FUNC!: ExAllocatePool2 failed, out of memory")),
        Done);

    //
    // Get the USB Headset descriptor.
    //
    WDF_REQUEST_REUSE_PARAMS_INIT(
        &reuseParams,   
        WDF_REQUEST_REUSE_NO_FLAGS,
        STATUS_SUCCESS);

    ntStatus = WdfRequestReuse(req, &reuseParams);
    IF_FAILED_ACTION_JUMP(
        ntStatus,
        DPF(D_ERROR, ("%!FUNC!: WdfRequestReuse failed, 0x%x", ntStatus)),
        Done);

    // Same buffer used for input and output
    // Cast into ULONG to provide EpIndex
    *((ULONG *)descriptor) = EpIndex;

    ntStatus = SendIoCtrlSynchronously(
        req,
        IOCTL_SBAUD_GET_ENDPOINT_DESCRIPTOR2,
        sizeof(EpIndex),
        length,
        descriptor);
    IF_FAILED_ACTION_JUMP(
        ntStatus,
        DPF(D_ERROR, ("%!FUNC!: SendIoCtrlSynchronously(IOCTL_SBAUD_GET_ENDPOINT_DESCRIPTOR) failed, 0x%x", ntStatus)),
        Done);

    //
    // All done.
    //
    *EpDescriptor = descriptor;
    ntStatus = STATUS_SUCCESS;

Done:
    if (!NT_SUCCESS(ntStatus))
    {
        if (descriptor != NULL)
        {
            ExFreePoolWithTag(descriptor, MINADAPTER_POOLTAG);
        }   
    }

    if (req != NULL)
    {
        WdfObjectDelete(req);
        req = NULL;
    }

    return ntStatus;
}

//=============================================================================
#pragma code_seg("PAGE")
NTSTATUS
UsbHsDevice::VerifyEndpointFormatCompatibility
(
    _In_    PSIDEBANDAUDIO_SUPPORTED_FORMATS    EpSupportedFormats,
    _In_    PKSDATAFORMAT_WAVEFORMATEXTENSIBLE  pHardwareSupportedFormats,
    _In_    ULONG                               NumHardwareSupportedFormats,
    _Out_   PULONG                              pHardwareFormatIndex
)
{
    PAGED_CODE();

    NTSTATUS status = STATUS_NOT_SUPPORTED;
    ULONG iHardwareFormats = 0;
    ULONG iEpDataFormats = 0;
    BOOL bFound = FALSE;

    // Verify here whether hardware/dsp mixer can support the USB device formats
    // This format will be later used to open the ISOCH channel

    for (iHardwareFormats = 0; iHardwareFormats < NumHardwareSupportedFormats; iHardwareFormats++)
    {
        // for each format supported by hardware, search in EP data formats
        for (iEpDataFormats = 0; iEpDataFormats < EpSupportedFormats->NumFormats; iEpDataFormats++)
        {
            PKSDATAFORMAT pEpDataFormat = (PKSDATAFORMAT)EpSupportedFormats->Formats[iEpDataFormats];

            // Verify KSDATAFORMAT
            if (pEpDataFormat->FormatSize >= sizeof(KSDATAFORMAT))
            {
                if (pEpDataFormat->FormatSize >= sizeof(KSDATAFORMAT_WAVEFORMATEX))
                {
                    // Check other parameters as well
                    // Ignoring for simplicity
                    if ((((PKSDATAFORMAT_WAVEFORMATEX)pEpDataFormat)->WaveFormatEx.nSamplesPerSec == pHardwareSupportedFormats[iHardwareFormats].WaveFormatExt.Format.nSamplesPerSec) &&
                        (((PKSDATAFORMAT_WAVEFORMATEX)pEpDataFormat)->WaveFormatEx.nChannels == pHardwareSupportedFormats[iHardwareFormats].WaveFormatExt.Format.nChannels) &&
                        (((PKSDATAFORMAT_WAVEFORMATEX)pEpDataFormat)->WaveFormatEx.wBitsPerSample == pHardwareSupportedFormats[iHardwareFormats].WaveFormatExt.Format.wBitsPerSample))
                    {
                        bFound = TRUE;
                        *pHardwareFormatIndex = iHardwareFormats;
                        status = STATUS_SUCCESS;
                        break;
                    }
                }
            }
        }
    }

    // Iterate through Sideband Device data range to figure out
    // intersection with hardware supported formats

    return status;
}

#pragma code_seg("PAGE")
NTSTATUS UsbHsDevice::FreeTransportResources
(
    _In_    PUSBHSDEVICE_EP_TRANSPORT_RESOURCES     pTransportResources
)
{
    PAGED_CODE();

    SAFE_DELETE_PTR_WITH_TAG(pTransportResources->pUsbInterfaceDescriptor, USBSIDEBANDTEST_POOLTAG07);
    SAFE_DELETE_PTR_WITH_TAG(pTransportResources->pUsbEndpointDescriptor, USBSIDEBANDTEST_POOLTAG08);
    SAFE_DELETE_PTR_WITH_TAG(pTransportResources->pUsbOffloadInformation, USBSIDEBANDTEST_POOLTAG09);
    SAFE_DELETE_PTR_WITH_TAG(pTransportResources->pUsbAudioTransportResources, USBSIDEBANDTEST_POOLTAG010);
    SAFE_DELETE_PTR_WITH_TAG(pTransportResources->pSyncUsbEndpointDescriptor, USBSIDEBANDTEST_POOLTAG011);
    SAFE_DELETE_PTR_WITH_TAG(pTransportResources->pSyncUsbOffloadInformation, USBSIDEBANDTEST_POOLTAG012);
    SAFE_DELETE_PTR_WITH_TAG(pTransportResources->pSyncUsbAudioTransportResources, USBSIDEBANDTEST_POOLTAG013);
    RtlZeroMemory(pTransportResources, sizeof(USBHSDEVICE_EP_TRANSPORT_RESOURCES));
    return STATUS_SUCCESS;
}

//=============================================================================
#pragma code_seg("PAGE")
NTSTATUS UsbHsDevice::GetSiop
(
    _In_        const GUID  *ParamSet,
    _In_        ULONG       SiopTypeId,
    _In_        ULONG       EpIndex,
    _In_        ULONG       PoolTag,
    _Outptr_    PBYTE       *ppOutputBuffer
)
{
    PAGED_CODE();
    DPF_ENTER(("[%!FUNC!]"));

    NTSTATUS                                status      = STATUS_SUCCESS;
    WDFREQUEST                              req         = NULL;
    PBYTE                                   pOutput     = NULL;
    ULONG                                   length      = 0;
    ULONG_PTR                               information = 0;
    WDF_REQUEST_REUSE_PARAMS                reuseParams;   
    WDF_OBJECT_ATTRIBUTES                   attributes;
    SIDEBANDAUDIO_SIOP_REQUEST_PARAM siop = { 0 };

    *ppOutputBuffer = NULL;

    //
    // Allocate and format a WDF request.
    //
    WDF_OBJECT_ATTRIBUTES_INIT(&attributes);
    attributes.ParentObject = m_Adapter->GetWdfDevice();
    status = WdfRequestCreate(
        &attributes,
        m_WdfIoTarget,
        &req);
    IF_FAILED_ACTION_JUMP(
        status,
        DPF(D_ERROR, ("%!FUNC!: WdfRequestCreate failed, 0x%x", status)),
        Done);

    siop.EpIndex = EpIndex;
    siop.RequestedSiop.ParamSet = *ParamSet;
    siop.RequestedSiop.TypeId = SiopTypeId;
    siop.RequestedSiop.Size = 0;

    //
    // Get the size of the buffer.
    //
    status = SendIoCtrlSynchronously(
        req,
        IOCTL_SBAUD_GET_SIOP,
        sizeof(siop),
        0,
        &siop,
        NULL);

    if (status != STATUS_BUFFER_TOO_SMALL)
    {
        if (NT_SUCCESS(status))
        {
            status = STATUS_INVALID_DEVICE_STATE;
        }

        DPF(D_ERROR, ("%!FUNC!: SendIoCtrlSynchronously(IOCTL_SBAUD_GET_SIOP): failed, 0x%x", status));
        goto Done;
    }

    status = STATUS_SUCCESS;

    information = WdfRequestGetInformation(req);
    if (information == 0 || information > ULONG_MAX)
    {
        status = STATUS_INVALID_DEVICE_STATE;        
        DPF(D_ERROR, ("%!FUNC!: IOCTL_SBAUD_GET_SIOP buffer too big (%Id): 0x%x", information, status));
        goto Done;
    }

    length = (ULONG)information;

    //
    // Allocate memory needed to hold the info.
    //
    pOutput = (PBYTE)ExAllocatePool2(POOL_FLAG_NON_PAGED, length, PoolTag);
    if (pOutput == NULL)
    {
        status = STATUS_INSUFFICIENT_RESOURCES;
    }
    IF_FAILED_ACTION_JUMP(
        status,
        DPF(D_ERROR, ("%!FUNC!: ExAllocatePool2 failed, out of memory")),
        Done);

    //
    // Get the Siop
    //
    WDF_REQUEST_REUSE_PARAMS_INIT(
        &reuseParams,   
        WDF_REQUEST_REUSE_NO_FLAGS,
        STATUS_SUCCESS);

    status = WdfRequestReuse(req, &reuseParams);
    IF_FAILED_ACTION_JUMP(
        status,
        DPF(D_ERROR, ("%!FUNC!: WdfRequestReuse failed, 0x%x", status)),
        Done);

    // Same buffer used for input and output
    // Cast into ULONG to provide EpIndex

    status = SendIoCtrlSynchronously(
        req,
        IOCTL_SBAUD_GET_SIOP,
        sizeof(siop),
        length,
        &siop,
        pOutput);
    IF_FAILED_ACTION_JUMP(
        status,
        DPF(D_ERROR, ("%!FUNC!: SendIoCtrlSynchronously(IOCTL_SBAUD_GET_SIOP) failed, 0x%x", status)),
        Done);

    //
    // All done.
    //
    *ppOutputBuffer = pOutput;
    status = STATUS_SUCCESS;

Done:
    if (!NT_SUCCESS(status))
    {
        if (pOutput != NULL)
        {
            ExFreePoolWithTag(pOutput, PoolTag);
        }   
    }

    if (req != NULL)
    {
        WdfObjectDelete(req);
        req = NULL;
    }

    return status;
}

//=============================================================================
#pragma code_seg("PAGE")
NTSTATUS
UsbHsDevice::GetTransportResources
(
    _In_    ULONG                                   EpIndex,
    _In_    PSIDEBANDAUDIO_ENDPOINT_DESCRIPTOR2     pEndpointDescriptor,
    _Out_   PUSBHSDEVICE_EP_TRANSPORT_RESOURCES     pTransportResources
)
{
    PAGED_CODE();
    DPF_ENTER(("[%!FUNC!]"));

    NTSTATUS status = STATUS_SUCCESS;

    // SIOP_TYPE_USBAUD_USBD_INTERFACE_DESCRIPTOR
    status = GetSiop(&SIDEBANDAUDIO_PARAMS_SET_USBAUDIO, SIOP_TYPE_USBAUD_USBD_INTERFACE_DESCRIPTOR, EpIndex, USBSIDEBANDTEST_POOLTAG07, (PBYTE *)&pTransportResources->pUsbInterfaceDescriptor);
    if (!NT_SUCCESS(status))
    {
        DPF(D_ERROR, ("%!FUNC!: GetSiop(SIOP_TYPE_USBAUD_USBD_INTERFACE_DESCRIPTOR) failed, 0x%x", status));
        goto Done;
    }

    // SIOP_TYPE_USBAUD_EP_USBD_ENDPOINT_DESCRIPTOR
    status = GetSiop(&SIDEBANDAUDIO_PARAMS_SET_USBAUDIO, SIOP_TYPE_USBAUD_EP_USBD_ENDPOINT_DESCRIPTOR, EpIndex, USBSIDEBANDTEST_POOLTAG08, (PBYTE *)&pTransportResources->pUsbEndpointDescriptor);
    if (!NT_SUCCESS(status))
    {
        DPF(D_ERROR, ("%!FUNC!: GetSiop(SIOP_TYPE_USBAUD_EP_USBD_ENDPOINT_DESCRIPTOR) failed, 0x%x", status));
        goto Done;
    }

    // SIOP_TYPE_USBAUD_EP_USBD_ENDPOINT_OFFLOAD_INFORMATION
    status = GetSiop(&SIDEBANDAUDIO_PARAMS_SET_USBAUDIO, SIOP_TYPE_USBAUD_EP_USBD_ENDPOINT_OFFLOAD_INFORMATION, EpIndex, USBSIDEBANDTEST_POOLTAG09, (PBYTE *)&pTransportResources->pUsbOffloadInformation);
    if (!NT_SUCCESS(status))
    {
        DPF(D_ERROR, ("%!FUNC!: GetSiop(SIOP_TYPE_USBAUD_EP_USBD_ENDPOINT_OFFLOAD_INFORMATION) failed, 0x%x", status));
        goto Done;
    }

    // SIOP_TYPE_USBAUD_EP_USBAUDIO_TRANSPORT_RESOURCES
    status = GetSiop(&SIDEBANDAUDIO_PARAMS_SET_USBAUDIO, SIOP_TYPE_USBAUD_EP_USBAUDIO_TRANSPORT_RESOURCES, EpIndex, USBSIDEBANDTEST_POOLTAG010, (PBYTE *)&pTransportResources->pUsbAudioTransportResources);
    if (!NT_SUCCESS(status))
    {
        DPF(D_ERROR, ("%!FUNC!: GetSiop(SIOP_TYPE_USBAUD_EP_USBAUDIO_TRANSPORT_RESOURCES) failed, 0x%x", status));
        goto Done;
    }

    if (pEndpointDescriptor->Capabilities.Feedback)
    {
        // SIOP_TYPE_USBAUD_FB_EP_USBD_ENDPOINT_DESCRIPTOR
        status = GetSiop(&SIDEBANDAUDIO_PARAMS_SET_USBAUDIO, SIOP_TYPE_USBAUD_FB_EP_USBD_ENDPOINT_DESCRIPTOR, EpIndex, USBSIDEBANDTEST_POOLTAG011, (PBYTE *)&pTransportResources->pSyncUsbEndpointDescriptor);
        if (!NT_SUCCESS(status))
        {
            DPF(D_ERROR, ("%!FUNC!: GetSiop(SIOP_TYPE_USBAUD_FB_EP_USBD_ENDPOINT_DESCRIPTOR) failed, 0x%x", status));
            goto Done;
        }

        // SIOP_TYPE_USBAUD_FB_EP_USBD_ENDPOINT_OFFLOAD_INFORMATION
        status = GetSiop(&SIDEBANDAUDIO_PARAMS_SET_USBAUDIO, SIOP_TYPE_USBAUD_FB_EP_USBD_ENDPOINT_OFFLOAD_INFORMATION, EpIndex, USBSIDEBANDTEST_POOLTAG012, (PBYTE *)&pTransportResources->pSyncUsbOffloadInformation);
        if (!NT_SUCCESS(status))
        {
            DPF(D_ERROR, ("%!FUNC!: GetSiop(SIOP_TYPE_USBAUD_FB_EP_USBD_ENDPOINT_OFFLOAD_INFORMATION) failed, 0x%x", status));
            goto Done;
        }

        // SIOP_TYPE_USBAUD_FB_EP_USBAUDIO_TRANSPORT_RESOURCES
        status = GetSiop(&SIDEBANDAUDIO_PARAMS_SET_USBAUDIO, SIOP_TYPE_USBAUD_FB_EP_USBAUDIO_TRANSPORT_RESOURCES, EpIndex, USBSIDEBANDTEST_POOLTAG013, (PBYTE *)&pTransportResources->pSyncUsbAudioTransportResources);
        if (!NT_SUCCESS(status))
        {
            DPF(D_ERROR, ("%!FUNC!: GetSiop(SIOP_TYPE_USBAUD_FB_EP_USBAUDIO_TRANSPORT_RESOURCES) failed, 0x%x", status));
            goto Done;
        }
    }

Done:
    if (!NT_SUCCESS(status))
    {
        FreeTransportResources(pTransportResources);
    }
    return status;
}

//=============================================================================
#pragma code_seg("PAGE")
NTSTATUS
UsbHsDevice::SetTransportResources
(
    _In_    PSIDEBANDAUDIO_ENDPOINT_DESCRIPTOR2     pEndpointDescriptor,
    _In_    ULONG                                   EpIndex
)
{
    PAGED_CODE();
    DPF_ENTER(("[%!FUNC!]"));

    NTSTATUS status = STATUS_SUCCESS;

    typedef struct _SYSVAD_USBSBAUD_SIOP_RESOURCE_ID
    {
        SIDEBANDAUDIO_SIOP_REQUEST_PARAM    srp;
        ULONG                               resourceId;
    }SYSVAD_USBSBAUD_SIOP_RESOURCE_ID, *PSYSVAD_USBSBAUD_SIOP_RESOURCE_ID;

    //
    // Set the USB Resource ID for Endpoint
    //
    SYSVAD_USBSBAUD_SIOP_RESOURCE_ID    siopResourceId = { 0 };
    siopResourceId.srp.EpIndex = EpIndex;
    siopResourceId.srp.RequestedSiop.ParamSet = SIDEBANDAUDIO_PARAMS_SET_USBAUDIO;
    siopResourceId.srp.RequestedSiop.TypeId = SIOP_TYPE_USBAUD_EP_OFFLOAD_RESOURCE_ID;
    siopResourceId.srp.RequestedSiop.Size = sizeof(ULONG);
    siopResourceId.resourceId = EpIndex + 7;// Just for testing. Real hardware will populate this with the DSP resource ID

    status = SendIoCtrlSynchronously(
        NULL,
        IOCTL_SBAUD_SET_SIOP,
        sizeof(siopResourceId),
        0,
        &siopResourceId);

    IF_FAILED_ACTION_JUMP(
        status,
        DPF(D_ERROR, ("%!FUNC!: SendIoCtrlSynchronously(IOCTL_SBAUD_SET_SIOP) failed, 0x%x", status)),
        Done);

    if (pEndpointDescriptor->Capabilities.Feedback)
    {
        //
        // Set the USB Resource ID for Feedback Endpoint
        //
        siopResourceId.srp.RequestedSiop.TypeId = SIOP_TYPE_USBAUD_FB_EP_OFFLOAD_RESOURCE_ID;
        siopResourceId.resourceId = EpIndex + 10;// Just for testing. Real hardware will populate this with the DSP resource ID

        status = SendIoCtrlSynchronously(
            NULL,
            IOCTL_SBAUD_SET_SIOP,
            sizeof(siopResourceId),
            0,
            &siopResourceId);

        IF_FAILED_ACTION_JUMP(
            status,
            DPF(D_ERROR, ("%!FUNC!: SendIoCtrlSynchronously(IOCTL_SBAUD_SET_SIOP) Feedback failed, 0x%x", status)),
            Done);
    }

    //
    // All done.
    //
    status = STATUS_SUCCESS;

Done:

    return status;
}

//=============================================================================
#pragma code_seg("PAGE")
NTSTATUS 
UsbHsDevice::GetUsbHsEndpointFormatsIntersection
(
    _In_    PSIDEBANDAUDIO_SUPPORTED_FORMATS    pDeviceFormats,
    _Out_   PSIDEBANDAUDIO_SUPPORTED_FORMATS    *ppSupportedFormatsIntersection
)
/*++

Routine Description:

  This function synchronously gets the USB Headset supported formats for
  a given endpoint.

--*/
{
    PAGED_CODE();
    DPF_ENTER(("[%!FUNC!]"));

    NTSTATUS                                ntStatus    = STATUS_SUCCESS;
    WDFREQUEST                              req         = NULL;
    PSIDEBANDAUDIO_SUPPORTED_FORMATS        formats     = NULL;
    ULONG                                   length      = 0;
    ULONG_PTR                               information = 0;
    WDF_REQUEST_REUSE_PARAMS                reuseParams;   
    WDF_OBJECT_ATTRIBUTES                   attributes;

    *ppSupportedFormatsIntersection = NULL;

    //
    // Allocate and format a WDF request.
    //
    WDF_OBJECT_ATTRIBUTES_INIT(&attributes);
    attributes.ParentObject = m_Adapter->GetWdfDevice();
    ntStatus = WdfRequestCreate(
        &attributes,
        m_WdfIoTarget,
        &req);
    IF_FAILED_ACTION_JUMP(
        ntStatus,
        DPF(D_ERROR, ("%!FUNC!: WdfRequestCreate failed, 0x%x", ntStatus)),
        Done);

    //
    // Get the size of the buffer.
    //
    ntStatus = SendIoCtrlSynchronously(
        req,
        IOCTL_SBAUD_GET_SUPPORTED_FORMATS,
        pDeviceFormats->CbSize,
        0,
        pDeviceFormats,
        NULL);

    if (ntStatus != STATUS_BUFFER_TOO_SMALL)
    {
        if (NT_SUCCESS(ntStatus))
        {
            ntStatus = STATUS_INVALID_DEVICE_STATE;
        }

        DPF(D_ERROR, ("%!FUNC!: SendIoCtrlSynchronously(IOCTL_SBAUD_GET_SUPPORTED_FORMATS): failed, 0x%x", ntStatus));
        goto Done;
    }

    ntStatus = STATUS_SUCCESS;

    information = WdfRequestGetInformation(req);
    if (information == 0 || information > ULONG_MAX)
    {
        ntStatus = STATUS_INVALID_DEVICE_STATE;        
        DPF(D_ERROR, ("%!FUNC!: IOCTL_SBAUD_GET_SUPPORTED_FORMATS buffer too big (%Id): 0x%x", information, ntStatus));
        goto Done;
    }

    length = (ULONG)information;

    //
    // Allocate memory needed to hold the info.
    //
    formats  = (PSIDEBANDAUDIO_SUPPORTED_FORMATS) ExAllocatePool2(POOL_FLAG_NON_PAGED, length, USBSIDEBANDTEST_POOLTAG03);
    if (formats == NULL)
    {
        ntStatus = STATUS_INSUFFICIENT_RESOURCES;
    }
    IF_FAILED_ACTION_JUMP(
        ntStatus,
        DPF(D_ERROR, ("%!FUNC!: ExAllocatePool2 failed, out of memory")),
        Done);

    //
    // Get the USB Headset descriptor.
    //
    WDF_REQUEST_REUSE_PARAMS_INIT(
        &reuseParams,   
        WDF_REQUEST_REUSE_NO_FLAGS,
        STATUS_SUCCESS);

    ntStatus = WdfRequestReuse(req, &reuseParams);
    IF_FAILED_ACTION_JUMP(
        ntStatus,
        DPF(D_ERROR, ("%!FUNC!: WdfRequestReuse failed, 0x%x", ntStatus)),
        Done);

    // Same buffer used for input and output
    // Cast into ULONG to provide EpIndex

    ntStatus = SendIoCtrlSynchronously(
        req,
        IOCTL_SBAUD_GET_SUPPORTED_FORMATS,
        pDeviceFormats->CbSize,
        length,
        pDeviceFormats,
        formats);
    IF_FAILED_ACTION_JUMP(
        ntStatus,
        DPF(D_ERROR, ("%!FUNC!: SendIoCtrlSynchronously(IOCTL_SBAUD_GET_SUPPORTED_FORMATS) failed, 0x%x", ntStatus)),
        Done);

    //
    // All done.
    //
    *ppSupportedFormatsIntersection = formats;
    ntStatus = STATUS_SUCCESS;

Done:
    if (!NT_SUCCESS(ntStatus))
    {
        if (formats != NULL)
        {
            ExFreePoolWithTag(formats, USBSIDEBANDTEST_POOLTAG03);
        }   
    }

    if (req != NULL)
    {
        WdfObjectDelete(req);
        req = NULL;
    }

    return ntStatus;
}

//=============================================================================
#pragma code_seg("PAGE")
NTSTATUS
UsbHsDevice::GetUsbHsVolumePropertyValues
(
    _In_  ULONG                     EpIndex,
    _In_  ULONG                     Length,
    _Out_ PKSPROPERTY_DESCRIPTION*  PropValues
)
/*++

Routine Description:

  This function synchronously gets the remote USB Headset volume values.

--*/
{
    PAGED_CODE();
    DPF_ENTER(("[%!FUNC!]"));

    NTSTATUS                    ntStatus = STATUS_SUCCESS;
    PKSPROPERTY_DESCRIPTION     propValues = NULL;

    *PropValues = NULL;

    //
    // Allocate memory.
    //
    propValues  = (PKSPROPERTY_DESCRIPTION) ExAllocatePool2(POOL_FLAG_NON_PAGED, Length, USBSIDEBANDTEST_POOLTAG02);
    if (propValues == NULL)
    {
        ntStatus = STATUS_INSUFFICIENT_RESOURCES;
    }
    IF_FAILED_ACTION_JUMP(
        ntStatus,
        DPF(D_ERROR, ("%!FUNC!: ExAllocatePool2 failed, out of memory")),
        Done);

    //
    // Send EpIndex in same buffer as input param
    //
    *((ULONG*)propValues) = EpIndex;

    //
    // Get the USB Headset volume property values.
    //
    ntStatus = SendIoCtrlSynchronously(
        NULL,
        IOCTL_SBAUD_GET_VOLUMEPROPERTYVALUES,
        sizeof(EpIndex),
        Length,
        propValues);

    IF_FAILED_ACTION_JUMP(
        ntStatus,
        DPF(D_ERROR, ("%!FUNC!: SendIoCtrlSynchronously(IOCTL_SBAUD_GET_VOLUMEPROPERTYVALUES) failed, 0x%x", ntStatus)),
        Done);

    //
    // All done.
    //
    *PropValues = propValues;
    ntStatus = STATUS_SUCCESS;

Done:
    if (!NT_SUCCESS(ntStatus))
    {
        if (propValues != NULL)
        {
            ExFreePoolWithTag(propValues, MINADAPTER_POOLTAG);
        }
    }

    return ntStatus;
}

//=============================================================================
#pragma code_seg("PAGE")
NTSTATUS 
UsbHsDevice::SetUsbHsSpeakerVolume
(
    _In_ LONG   Channel,
    _In_ LONG   Volume  
)
/*++

Routine Description:

  This function synchronously sets the remote USB Headset speaker volume.

--*/
{
    PAGED_CODE();
    DPF_ENTER(("[%!FUNC!]"));

    NTSTATUS                        ntStatus    = STATUS_SUCCESS;
    SIDEBANDAUDIO_VOLUME_PARAMS  vp;

    //
    // Set the USB Headset speaker volume.
    //

    vp.EpIndex = m_SpeakerEpIndex;
    vp.Value = Volume;
    vp.Channel = Channel;
    vp.Immediate = TRUE; // not required for SET
    ntStatus = SendIoCtrlSynchronously(
        NULL,
        IOCTL_SBAUD_SET_VOLUME,
        sizeof(vp),
        0,
        &vp);

    IF_FAILED_ACTION_JUMP(
        ntStatus,
        DPF(D_ERROR, ("%!FUNC!: SendIoCtrlSynchronously(IOCTL_SBAUD_SET_VOLUME) failed, 0x%x", ntStatus)),
        Done);

    //
    // All done.
    //
    ntStatus = STATUS_SUCCESS;

Done:

    return ntStatus;
}

//=============================================================================
#pragma code_seg("PAGE")
NTSTATUS 
UsbHsDevice::GetUsbHsSpeakerVolume
(
    _In_  LONG  Channel,
    _Out_ LONG  * Volume    
)
/*++

Routine Description:

  This function synchronously gets the remote USB Headset speaker volume.

--*/
{
    PAGED_CODE();
    DPF_ENTER(("[%!FUNC!]"));

    NTSTATUS                        ntStatus    = STATUS_SUCCESS;
    UsbHsDeviceNotificationBuffer   buffer      = {0};

    *Volume = 0;

    buffer.Volume.Immediate = TRUE;
    buffer.Volume.EpIndex = m_SpeakerEpIndex;
    buffer.Volume.Channel = Channel;
    buffer.Volume.Value = 0;

    //
    // Get the USB headset speaker volume.
    //
    ntStatus = SendIoCtrlSynchronously(
        NULL,
        IOCTL_SBAUD_GET_VOLUME_STATUS_UPDATE,
        sizeof(buffer.Volume),
        sizeof(buffer.Volume),
        &buffer);

    IF_FAILED_ACTION_JUMP(
        ntStatus,
        DPF(D_ERROR, ("%!FUNC!: SendIoCtrlSynchronously(IOCTL_SBAUD_GET_VOLUME_STATUS_UPDATE) failed, 0x%x", ntStatus)),
        Done);

    //
    // All done.
    //
    *Volume = buffer.Volume.Value;
    ntStatus = STATUS_SUCCESS;

Done:

    return ntStatus;
}

//=============================================================================
#pragma code_seg()
NTSTATUS 
UsbHsDevice::EnableUsbHsSpeakerVolumeStatusNotification()
/*++

Routine Description:

  This function registers for USB Headset speaker 
  volume change notification. 

--*/
{
    DPF_ENTER(("[%!FUNC!]"));

    NTSTATUS                                ntStatus    = STATUS_SUCCESS;
    UsbHsDeviceNotificationReqContext       *ctx         = NULL;

    ctx = GetUsbHsDeviceNotificationReqContext(m_SpeakerVolumeReq);

    //
    // This is a notification request.
    //
    ctx->Buffer.Volume.Immediate    = FALSE; 
    ctx->Buffer.Volume.EpIndex      = m_SpeakerEpIndex;
    ctx->Buffer.Volume.Value        = 0;

    //
    // Register for speaker volume updates.
    //
    ntStatus = SendIoCtrlAsynchronously(
        m_SpeakerVolumeReq,
        IOCTL_SBAUD_GET_VOLUME_STATUS_UPDATE,
        ctx->MemIn,
        ctx->MemOut,
        EvtUsbHsDeviceNotificationStatusCompletion,
        this);

    IF_FAILED_ACTION_JUMP(
        ntStatus,
        DPF(D_ERROR, ("%!FUNC!: SendIoCtrlAsynchronously(IOCTL_SBAUD_GET_VOLUME_STATUS_UPDATE) failed, 0x%x", ntStatus)),
        Done);

    //
    // All done.
    //
    ntStatus = STATUS_SUCCESS;

Done:

    return ntStatus;
}

//=============================================================================
#pragma code_seg("PAGE")
NTSTATUS
UsbHsDevice::SetUsbHsMicVolume
(
    _In_ LONG   Channel,
    _In_ LONG   Volume
)
/*++

Routine Description:

This function synchronously sets the remote USB Headset mic volume.

--*/
{
    PAGED_CODE();
    DPF_ENTER(("[%!FUNC!]"));

    NTSTATUS                        ntStatus = STATUS_SUCCESS;
    SIDEBANDAUDIO_VOLUME_PARAMS  vp;

    //
    // Get the USB Headset mic volume.
    //

    vp.EpIndex = m_MicEpIndex;
    vp.Value = Volume;
    vp.Channel = Channel;
    vp.Immediate = TRUE; // not required for SET
    ntStatus = SendIoCtrlSynchronously(
        NULL,
        IOCTL_SBAUD_SET_VOLUME,
        sizeof(vp),
        0,
        &vp);

    IF_FAILED_ACTION_JUMP(
        ntStatus,
        DPF(D_ERROR, ("%!FUNC!: SendIoCtrlSynchronously(IOCTL_SBAUD_SET_VOLUME) failed, 0x%x", ntStatus)),
        Done);

    //
    // All done.
    //
    ntStatus = STATUS_SUCCESS;

Done:

    return ntStatus;
}

//=============================================================================
#pragma code_seg("PAGE")
NTSTATUS
UsbHsDevice::GetUsbHsMicVolume
(
    _In_  LONG  Channel,
    _Out_ LONG  * Volume
)
/*++

Routine Description:

This function synchronously gets the remote USB Headset mic volume.

--*/
{
    PAGED_CODE();
    DPF_ENTER(("[%!FUNC!]"));

    NTSTATUS                        ntStatus = STATUS_SUCCESS;
    UsbHsDeviceNotificationBuffer   buffer = { 0 };

    *Volume = 0;

    buffer.Volume.Immediate = TRUE;
    buffer.Volume.EpIndex   = m_MicEpIndex;
    buffer.Volume.Channel   = Channel;
    buffer.Volume.Value     = 0;

    //
    // Get the USB Headset mic volume.
    //
    ntStatus = SendIoCtrlSynchronously(
        NULL,
        IOCTL_SBAUD_GET_VOLUME_STATUS_UPDATE,
        sizeof(buffer.Volume),
        sizeof(buffer.Volume),
        &buffer);

    IF_FAILED_ACTION_JUMP(
        ntStatus,
        DPF(D_ERROR, ("%!FUNC!: SendIoCtrlSynchronously(IOCTL_SBAUD_GET_VOLUME_STATUS_UPDATE) failed, 0x%x", ntStatus)),
        Done);

    //
    // All done.
    //
    *Volume = buffer.Volume.Value;
    ntStatus = STATUS_SUCCESS;

Done:

    return ntStatus;
}

//=============================================================================
#pragma code_seg("PAGE")
NTSTATUS
UsbHsDevice::GetUsbHsMutePropertyValues
(
    _In_  ULONG                     EpIndex,
    _In_  ULONG                     Length,
    _Out_ PKSPROPERTY_DESCRIPTION* PropValues
)
/*++

Routine Description:

  This function synchronously gets the remote USB Headset mute values.

--*/
{
    PAGED_CODE();
    DPF_ENTER(("[%!FUNC!]"));

    NTSTATUS                    ntStatus = STATUS_SUCCESS;
    PKSPROPERTY_DESCRIPTION     propValues = NULL;

    *PropValues = NULL;

    //
    // Allocate memory.
    //
    propValues = (PKSPROPERTY_DESCRIPTION) ExAllocatePool2(POOL_FLAG_NON_PAGED, Length, USBSIDEBANDTEST_POOLTAG06);
    if (propValues == NULL)
    {
        ntStatus = STATUS_INSUFFICIENT_RESOURCES;
    }
    IF_FAILED_ACTION_JUMP(
        ntStatus,
        DPF(D_ERROR, ("%!FUNC!: ExAllocatePool2 failed, out of memory")),
        Done);

    //
    // Send EpIndex in same buffer as input param
    //
    *((ULONG*)propValues) = EpIndex;

    //
    // Get the USB Headset mute property values.
    //
    ntStatus = SendIoCtrlSynchronously(
        NULL,
        IOCTL_SBAUD_GET_MUTEPROPERTYVALUES,
        sizeof(EpIndex),
        Length,
        propValues);
    IF_FAILED_ACTION_JUMP(
        ntStatus,
        DPF(D_ERROR, ("%!FUNC!: SendIoCtrlSynchronously(IOCTL_SBAUD_GET_MUTEPROPERTYVALUES) failed, 0x%x", ntStatus)),
        Done);

    //
    // All done.
    //
    *PropValues = propValues;
    ntStatus = STATUS_SUCCESS;

Done:
    if (!NT_SUCCESS(ntStatus))
    {
        if (propValues != NULL)
        {
            ExFreePoolWithTag(propValues, USBSIDEBANDTEST_POOLTAG06);
        }
    }

    return ntStatus;
}

//=============================================================================
#pragma code_seg("PAGE")
NTSTATUS 
UsbHsDevice::SetUsbHsSpeakerMute
(
    _In_ LONG   Channel,
    _In_ LONG   Mute
)
/*++

Routine Description:

  This function synchronously sets the remote USB Headset speaker mute.

--*/
{
    PAGED_CODE();
    DPF_ENTER(("[%!FUNC!]"));

    NTSTATUS                        ntStatus    = STATUS_SUCCESS;
    SIDEBANDAUDIO_MUTE_PARAMS       mp;

    //
    // Set the USB Headset speaker mute.
    //

    mp.EpIndex      = m_SpeakerEpIndex;
    mp.Value        = Mute;
    mp.Channel      = Channel;
    mp.Immediate    = TRUE; // not required for SET
    ntStatus = SendIoCtrlSynchronously(
        NULL,
        IOCTL_SBAUD_SET_MUTE,
        sizeof(mp),
        0,
        &mp);

    IF_FAILED_ACTION_JUMP(
        ntStatus,
        DPF(D_ERROR, ("%!FUNC!: SendIoCtrlSynchronously(IOCTL_SBAUD_SET_MUTE) failed, 0x%x", ntStatus)),
        Done);

    //
    // All done.
    //
    ntStatus = STATUS_SUCCESS;

Done:

    return ntStatus;
}

//=============================================================================
#pragma code_seg("PAGE")
NTSTATUS 
UsbHsDevice::GetUsbHsSpeakerMute
(
    _In_  LONG  Channel,
    _Out_ LONG  *Mute
)
/*++

Routine Description:

  This function synchronously gets the remote USB Headset speaker mute.

--*/
{
    PAGED_CODE();
    DPF_ENTER(("[%!FUNC!]"));

    NTSTATUS                        ntStatus    = STATUS_SUCCESS;
    UsbHsDeviceNotificationBuffer   buffer      = {0};

    *Mute = 0;

    buffer.Mute.Immediate = TRUE;
    buffer.Mute.EpIndex = m_SpeakerEpIndex;
    buffer.Mute.Channel = Channel;
    buffer.Mute.Value = 0;

    //
    // Get the USB headset speaker mute.
    //
    ntStatus = SendIoCtrlSynchronously(
        NULL,
        IOCTL_SBAUD_GET_MUTE_STATUS_UPDATE,
        sizeof(buffer.Mute),
        sizeof(buffer.Mute),
        &buffer);

    IF_FAILED_ACTION_JUMP(
        ntStatus,
        DPF(D_ERROR, ("%!FUNC!: SendIoCtrlSynchronously(IOCTL_SBAUD_GET_MUTE_STATUS_UPDATE) failed, 0x%x", ntStatus)),
        Done);

    //
    // All done.
    //
    *Mute = buffer.Mute.Value;
    ntStatus = STATUS_SUCCESS;

Done:

    return ntStatus;
}

//=============================================================================
#pragma code_seg()
NTSTATUS 
UsbHsDevice::EnableUsbHsSpeakerMuteStatusNotification()
/*++

Routine Description:

  This function registers for USB Headset speaker 
  mute change notification. 

--*/
{
    DPF_ENTER(("[%!FUNC!]"));

    NTSTATUS                                ntStatus    = STATUS_SUCCESS;
    UsbHsDeviceNotificationReqContext       *ctx         = NULL;

    ctx = GetUsbHsDeviceNotificationReqContext(m_SpeakerMuteReq);

    //
    // This is a notification request.
    //
    ctx->Buffer.Mute.Immediate    = FALSE; 
    ctx->Buffer.Mute.EpIndex      = m_SpeakerEpIndex;
    ctx->Buffer.Mute.Value        = 0;

    //
    // Register for speaker mute updates.
    //
    ntStatus = SendIoCtrlAsynchronously(
        m_SpeakerMuteReq,
        IOCTL_SBAUD_GET_MUTE_STATUS_UPDATE,
        ctx->MemIn,
        ctx->MemOut,
        EvtUsbHsDeviceNotificationStatusCompletion,
        this);

    IF_FAILED_ACTION_JUMP(
        ntStatus,
        DPF(D_ERROR, ("%!FUNC!: SendIoCtrlAsynchronously(IOCTL_SBAUD_GET_MUTE_STATUS_UPDATE) failed, 0x%x", ntStatus)),
        Done);

    //
    // All done.
    //
    ntStatus = STATUS_SUCCESS;

Done:

    return ntStatus;
}

//=============================================================================
#pragma code_seg("PAGE")
NTSTATUS
UsbHsDevice::SetUsbHsMicMute
(
    _In_ LONG   Channel,
    _In_ LONG   Mute
)
/*++

Routine Description:

This function synchronously sets the remote USB Headset mic mute.

--*/
{
    PAGED_CODE();
    DPF_ENTER(("[%!FUNC!]"));

    NTSTATUS                        ntStatus = STATUS_SUCCESS;
    SIDEBANDAUDIO_MUTE_PARAMS  mp;

    //
    // Get the USB Headset mic mute.
    //

    mp.EpIndex = m_MicEpIndex;
    mp.Value = Mute;
    mp.Channel = Channel;
    mp.Immediate = TRUE; // not required for SET
    ntStatus = SendIoCtrlSynchronously(
        NULL,
        IOCTL_SBAUD_SET_MUTE,
        sizeof(mp),
        0,
        &mp);

    IF_FAILED_ACTION_JUMP(
        ntStatus,
        DPF(D_ERROR, ("%!FUNC!: SendIoCtrlSynchronously(IOCTL_SBAUD_SET_MUTE) failed, 0x%x", ntStatus)),
        Done);

    //
    // All done.
    //
    ntStatus = STATUS_SUCCESS;

Done:

    return ntStatus;
}

//=============================================================================
#pragma code_seg("PAGE")
NTSTATUS
UsbHsDevice::GetUsbHsMicMute
(
    _In_  LONG  Channel,
    _Out_ LONG  *Mute
)
/*++

Routine Description:

This function synchronously gets the remote USB Headset mic mute.

--*/
{
    PAGED_CODE();
    DPF_ENTER(("[%!FUNC!]"));

    NTSTATUS                        ntStatus = STATUS_SUCCESS;
    UsbHsDeviceNotificationBuffer   buffer = { 0 };

    *Mute = 0;

    buffer.Mute.Immediate = TRUE;
    buffer.Mute.EpIndex   = m_MicEpIndex;
    buffer.Mute.Channel   = Channel;
    buffer.Mute.Value     = 0;

    //
    // Get the USB Headset mic mute.
    //
    ntStatus = SendIoCtrlSynchronously(
        NULL,
        IOCTL_SBAUD_GET_MUTE_STATUS_UPDATE,
        sizeof(buffer.Mute),
        sizeof(buffer.Mute),
        &buffer);

    IF_FAILED_ACTION_JUMP(
        ntStatus,
        DPF(D_ERROR, ("%!FUNC!: SendIoCtrlSynchronously(IOCTL_SBAUD_GET_MUTE_STATUS_UPDATE) failed, 0x%x", ntStatus)),
        Done);

    //
    // All done.
    //
    *Mute = buffer.Mute.Value;
    ntStatus = STATUS_SUCCESS;

Done:

    return ntStatus;
}

//=============================================================================
#pragma code_seg()
NTSTATUS 
UsbHsDevice::EnableUsbHsMicMuteStatusNotification()
/*++

Routine Description:

  This function registers for USB Headset mic 
  mute change notification. 

--*/
{
    DPF_ENTER(("[%!FUNC!]"));

    NTSTATUS                                ntStatus    = STATUS_SUCCESS;
    UsbHsDeviceNotificationReqContext       *ctx         = NULL;

    ctx = GetUsbHsDeviceNotificationReqContext(m_MicMuteReq);

    //
    // This is a notification request.
    //
    ctx->Buffer.Mute.Immediate    = FALSE; 
    ctx->Buffer.Mute.EpIndex      = m_MicEpIndex;
    ctx->Buffer.Mute.Value        = 0;

    //
    // Register for mic mute updates.
    //
    ntStatus = SendIoCtrlAsynchronously(
        m_MicMuteReq,
        IOCTL_SBAUD_GET_MUTE_STATUS_UPDATE,
        ctx->MemIn,
        ctx->MemOut,
        EvtUsbHsDeviceNotificationStatusCompletion,
        this);

    IF_FAILED_ACTION_JUMP(
        ntStatus,
        DPF(D_ERROR, ("%!FUNC!: SendIoCtrlAsynchronously(IOCTL_SBAUD_GET_MUTE_STATUS_UPDATE) failed, 0x%x", ntStatus)),
        Done);

    //
    // All done.
    //
    ntStatus = STATUS_SUCCESS;

Done:

    return ntStatus;
}

//=============================================================================
#pragma code_seg("PAGE")
NTSTATUS 
UsbHsDevice::SetSidebandClaimed(_In_ BOOL bClaimed)
/*++

Routine Description:

  This function synchronously requests an open ISOCH channel to transmit audio
  data over transport.

--*/
{
    PAGED_CODE();
    DPF_ENTER(("[%!FUNC!]"));

    NTSTATUS    ntStatus    = STATUS_SUCCESS;

    ntStatus = SendIoCtrlSynchronously(
        NULL,
        IOCTL_SBAUD_SET_DEVICE_CLAIMED,
        sizeof(BOOL),
        0,
        &bClaimed);

    IF_FAILED_ACTION_JUMP(
        ntStatus,
        DPF(D_ERROR, ("%!FUNC!: SendIoCtrlSynchronously(IOCTL_SBAUD_SET_DEVICE_CLAIMED) failed, 0x%x", ntStatus)),
        Done);

Done:

    return ntStatus;
}

//=============================================================================
#pragma code_seg()
NTSTATUS
UsbHsDevice::EnableUsbHsMicVolumeStatusNotification()
/*++

Routine Description:

This function registers for USB Headset mic
volume change notification.

--*/
{
    DPF_ENTER(("[%!FUNC!]"));

    NTSTATUS                            ntStatus    = STATUS_SUCCESS;
    UsbHsDeviceNotificationReqContext   *ctx        = NULL;

    ctx = GetUsbHsDeviceNotificationReqContext(m_MicVolumeReq);

    //
    // This is a notification request.
    //
    ctx->Buffer.Volume.Immediate    = FALSE;
    ctx->Buffer.Volume.EpIndex      = m_MicEpIndex;
    ctx->Buffer.Volume.Value        = 0;

    //
    // Register for mic volume updates.
    //
    ntStatus = SendIoCtrlAsynchronously(
        m_MicVolumeReq,
        IOCTL_SBAUD_GET_VOLUME_STATUS_UPDATE,
        ctx->MemIn,
        ctx->MemOut,
        EvtUsbHsDeviceNotificationStatusCompletion,
        this);

    IF_FAILED_ACTION_JUMP(
        ntStatus,
        DPF(D_ERROR, ("%!FUNC!: SendIoCtrlAsynchronously(IOCTL_SBAUD_GET_VOLUME_STATUS_UPDATE) failed, 0x%x", ntStatus)),
        Done);

    //
    // All done.
    //
    ntStatus = STATUS_SUCCESS;

Done:

    return ntStatus;
}

//=============================================================================
#pragma code_seg()
void 
UsbHsDevice::EvtUsbHsDeviceStreamStatusCompletion
(
  _In_  WDFREQUEST Request,
  _In_  WDFIOTARGET Target,
  _In_  PWDF_REQUEST_COMPLETION_PARAMS Params,
  _In_  WDFCONTEXT Context
)
/*++

Routine Description:

  Completion callback for the USB Headset stream status notification. 

--*/
{
    DPF_ENTER(("[%!FUNC!]"));

    NTSTATUS                             ntStatus   = STATUS_SUCCESS;
    UsbHsDeviceNotificationReqContext    *reqCtx    = NULL;
    UsbHsDevice                          *This      = NULL;
    NTSTATUS                             ntResult   = STATUS_SUCCESS;

    UNREFERENCED_PARAMETER(Target);
    UNREFERENCED_PARAMETER(Context);

    //
    // Get the USB stream status.
    //
    reqCtx = GetUsbHsDeviceNotificationReqContext(Request);
    This = reqCtx->UsbHsDevice;
    ASSERT(This != NULL);

    ntStatus = Params->IoStatus.Status;
    if (!NT_SUCCESS(ntStatus))
    {
        ntResult = STATUS_INVALID_DEVICE_STATE;
    }
    else 
    {
        ntResult = reqCtx->Buffer.streamStatus.Status;
    }

    if (reqCtx->Buffer.streamStatus.EpIndex == This->m_SpeakerEpIndex)
    {
        InterlockedExchange(&This->m_SpeakerStreamStatusLong, (LONG)ntResult);

        //
        // Let the stop routine know we are done. Stop routine will 
        // re-init the request.
        //
        KeSetEvent(&This->m_SpeakerStreamStatusEvent, IO_NO_INCREMENT, FALSE);
    }
    else if (reqCtx->Buffer.streamStatus.EpIndex == This->m_MicEpIndex)
    {
        InterlockedExchange(&This->m_MicStreamStatusLong, (LONG)ntResult);

        //
        // Let the stop routine know we are done. Stop routine will 
        // re-init the request.
        //
        KeSetEvent(&This->m_MicStreamStatusEvent, IO_NO_INCREMENT, FALSE);
    }
}

//=============================================================================
#pragma code_seg()
NTSTATUS 
UsbHsDevice::EnableUsbHsSpeakerStreamStatusNotification()
/*++

Routine Description:

  This function registers for USB Headset stream status notification. 

--*/
{
    DPF_ENTER(("[%!FUNC!]"));

    NTSTATUS                            ntStatus    = STATUS_SUCCESS;
    UsbHsDeviceNotificationReqContext   *ctx        = NULL;

    ASSERT(m_nSpeakerStreamsOpen > 0);

    ctx = GetUsbHsDeviceNotificationReqContext(m_SpeakerStreamReq);
    ctx->Buffer.streamStatus.Immediate = FALSE; 
    ctx->Buffer.streamStatus.EpIndex = m_SpeakerEpIndex;

    KeClearEvent(&m_SpeakerStreamStatusEvent);

    //
    // Get the USB Headset speaker stream status.
    //
    ntStatus = SendIoCtrlAsynchronously(
        m_SpeakerStreamReq,
        IOCTL_SBAUD_GET_STREAM_STATUS_UPDATE,
        ctx->MemIn,
        ctx->MemOut,
        EvtUsbHsDeviceStreamStatusCompletion,
        this);

    if (!NT_SUCCESS(ntStatus))
    {
        KeSetEvent(&m_SpeakerStreamStatusEvent, IO_NO_INCREMENT, FALSE);
        DPF(D_ERROR, ("%!FUNC!: SendIoCtrlAsynchronously(IOCTL_SBAUD_GET_STREAM_STATUS_UPDATE) failed, 0x%x", ntStatus));
        goto Done;
    }

    //
    // All done.
    //
    ntStatus = STATUS_SUCCESS;

Done:

    return ntStatus;
}

//=============================================================================
#pragma code_seg()
NTSTATUS 
UsbHsDevice::EnableUsbHsMicStreamStatusNotification()
/*++

Routine Description:

  This function registers for USB Headset stream status notification. 

--*/
{
    DPF_ENTER(("[%!FUNC!]"));

    NTSTATUS                            ntStatus    = STATUS_SUCCESS;
    UsbHsDeviceNotificationReqContext   *ctx        = NULL;

    ASSERT(m_nMicStreamsOpen > 0);

    ctx = GetUsbHsDeviceNotificationReqContext(m_MicStreamReq);
    ctx->Buffer.streamStatus.Immediate = FALSE; 
    ctx->Buffer.streamStatus.EpIndex = m_MicEpIndex;

    KeClearEvent(&m_MicStreamStatusEvent);

    //
    // Get the USB Headset mic stream status.
    //
    ntStatus = SendIoCtrlAsynchronously(
        m_MicStreamReq,
        IOCTL_SBAUD_GET_STREAM_STATUS_UPDATE,
        ctx->MemIn,
        ctx->MemOut,
        EvtUsbHsDeviceStreamStatusCompletion,
        this);

    if (!NT_SUCCESS(ntStatus))
    {
        KeSetEvent(&m_MicStreamStatusEvent, IO_NO_INCREMENT, FALSE);
        DPF(D_ERROR, ("%!FUNC!: SendIoCtrlAsynchronously(IOCTL_SBAUD_GET_STREAM_STATUS_UPDATE) failed, 0x%x", ntStatus));
        goto Done;
    }

    //
    // All done.
    //
    ntStatus = STATUS_SUCCESS;

Done:

    return ntStatus;
}

//=============================================================================
#pragma code_seg("PAGE")
NTSTATUS 
UsbHsDevice::StopUsbHsSpeakerStreamStatusNotification()
/*++

Routine Description:

  This function stops the USB Headset speaker stream status notification.
  The function waits for the request to be done before returning.

--*/
{
    PAGED_CODE();
    DPF_ENTER(("[%!FUNC!]"));

    NTSTATUS                            ntStatus        = STATUS_SUCCESS;
    UsbHsDeviceNotificationReqContext   *reqCtx         = NULL;
    WDF_REQUEST_REUSE_PARAMS            reuseParams;  

    WdfRequestCancelSentRequest(m_SpeakerStreamReq);
    KeWaitForSingleObject(&m_SpeakerStreamStatusEvent, Executive, KernelMode, FALSE, NULL);

    reqCtx = GetUsbHsDeviceNotificationReqContext(m_SpeakerStreamReq);
    ASSERT(reqCtx != NULL);
    UNREFERENCED_VAR(reqCtx);

    // 
    // Re-init the request for later.
    //
    WDF_REQUEST_REUSE_PARAMS_INIT(
        &reuseParams,   
        WDF_REQUEST_REUSE_NO_FLAGS,
        STATUS_SUCCESS);

    ntStatus = WdfRequestReuse(m_SpeakerStreamReq, &reuseParams);
    if (!NT_SUCCESS(ntStatus))
    {
        DPF(D_ERROR, ("%!FUNC!: WdfRequestReuse failed, 0x%x", ntStatus));
    }

    return STATUS_SUCCESS;
}

//=============================================================================
#pragma code_seg("PAGE")
NTSTATUS 
UsbHsDevice::StopUsbHsMicStreamStatusNotification()
/*++

Routine Description:

  This function stops the USB Headset mic stream status notification.
  The function waits for the request to be done before returning.

--*/
{
    PAGED_CODE();
    DPF_ENTER(("[%!FUNC!]"));

    NTSTATUS                            ntStatus        = STATUS_SUCCESS;
    UsbHsDeviceNotificationReqContext   *reqCtx         = NULL;
    WDF_REQUEST_REUSE_PARAMS            reuseParams;  

    WdfRequestCancelSentRequest(m_MicStreamReq);
    KeWaitForSingleObject(&m_MicStreamStatusEvent, Executive, KernelMode, FALSE, NULL);

    reqCtx = GetUsbHsDeviceNotificationReqContext(m_MicStreamReq);
    ASSERT(reqCtx != NULL);
    UNREFERENCED_VAR(reqCtx);

    // 
    // Re-init the request for later.
    //
    WDF_REQUEST_REUSE_PARAMS_INIT(
        &reuseParams,   
        WDF_REQUEST_REUSE_NO_FLAGS,
        STATUS_SUCCESS);

    ntStatus = WdfRequestReuse(m_MicStreamReq, &reuseParams);
    if (!NT_SUCCESS(ntStatus))
    {
        DPF(D_ERROR, ("%!FUNC!: WdfRequestReuse failed, 0x%x", ntStatus));
    }

    return STATUS_SUCCESS;
}

//=============================================================================
#pragma code_seg("PAGE")
NTSTATUS 
UsbHsDevice::SetUsbHsStreamOpen
(
    ULONG EpIndex,
    PKSDATAFORMAT_WAVEFORMATEXTENSIBLE pStreamFormat,
    PSIDEBANDAUDIO_ENDPOINT_DESCRIPTOR2 pEndpointDescriptor
)
/*++

Routine Description:

  This function synchronously requests an open ISOCH channel to transmit audio
  data over transport.

--*/
{
    PAGED_CODE();
    DPF_ENTER(("[UsbHsDevice::SetUsbHsStreamOpen]"));

    NTSTATUS        ntStatus = STATUS_SUCCESS;
    UNREFERENCED_PARAMETER(pEndpointDescriptor);

    SIDEBANDAUDIO_STREAM_OPEN_PARAMS sp = { 0 };
    sp.EpIndex = EpIndex;
    sp.Format = (PKSDATAFORMAT)pStreamFormat;
    sp.SiopCount = 0;

    ntStatus = SendIoCtrlSynchronously(
        NULL,
        IOCTL_SBAUD_STREAM_OPEN,
        sizeof(sp),
        0,
        &sp);

    if (ntStatus == STATUS_DEVICE_BUSY)
    {
        // The stream channel is already open.
        DPF(D_VERBOSE, ("SetUsbHsStreamOpen: the stream channel is already open"));
        ntStatus = STATUS_SUCCESS;
    }

    IF_FAILED_ACTION_JUMP(
        ntStatus,
        DPF(D_ERROR, ("SetUsbHsStreamOpen: SendIoCtrlSynchronously(IOCTL_SBAUD_STREAM_OPEN) failed, 0x%x", ntStatus)),
        Done);

    //
    // All done.
    //
    ntStatus = STATUS_SUCCESS;

Done:

    return ntStatus;
}

//=============================================================================
#pragma code_seg("PAGE")
NTSTATUS 
UsbHsDevice::SetUsbHsStreamStart
(
    ULONG EpIndex
)
/*++

Routine Description:

  This function synchronously requests an Start ISOCH channel to transmit audio
  data over transport.

--*/
{
    PAGED_CODE();
    DPF_ENTER(("[UsbHsDevice::SetUsbHsStreamStart]"));

    NTSTATUS        ntStatus = STATUS_SUCCESS;

    ntStatus = SendIoCtrlSynchronously(
        NULL,
        IOCTL_SBAUD_STREAM_START,
        sizeof(EpIndex),
        0,
        &EpIndex);

    if (ntStatus == STATUS_DEVICE_BUSY)
    {
        // The stream channel is already Start.
        DPF(D_VERBOSE, ("SetUsbHsStreamStart: the stream channel is already Start"));
        ntStatus = STATUS_SUCCESS;
    }

    IF_FAILED_ACTION_JUMP(
        ntStatus,
        DPF(D_ERROR, ("SetUsbHsStreamStart: SendIoCtrlSynchronously(IOCTL_SBAUD_STREAM_START) failed, 0x%x", ntStatus)),
        Done);

    //
    // All done.
    //
    ntStatus = STATUS_SUCCESS;

Done:

    return ntStatus;
}

//=============================================================================
#pragma code_seg("PAGE")
NTSTATUS 
UsbHsDevice::SetUsbHsStreamSuspend
(
    _In_    ULONG       EpIndex
)
/*++

Routine Description:

  This function synchronously requests to Suspend the ISOCH channel.

--*/
{
    PAGED_CODE();
    DPF_ENTER(("[%!FUNC!]"));

    NTSTATUS        ntStatus    = STATUS_SUCCESS;

    ntStatus = SendIoCtrlSynchronously(
        NULL,
        IOCTL_SBAUD_STREAM_SUSPEND,
        sizeof(EpIndex),
        0,
        &EpIndex);

    IF_FAILED_ACTION_JUMP(
        ntStatus,
        DPF(D_ERROR, ("%!FUNC!: SendIoCtrlSynchronously(IOCTL_SBAUD_STREAM_SUSPEND) failed, 0x%x", ntStatus)),
        Done);

    //
    // All done.
    //
    ntStatus = STATUS_SUCCESS;

Done:

    return ntStatus;
}

//=============================================================================
#pragma code_seg("PAGE")
NTSTATUS 
UsbHsDevice::SetUsbHsStreamClose
(
    _In_    ULONG       EpIndex
)
/*++

Routine Description:

  This function synchronously requests to close the ISOCH channel.

--*/
{
    PAGED_CODE();
    DPF_ENTER(("[%!FUNC!]"));

    NTSTATUS        ntStatus    = STATUS_SUCCESS;

    ntStatus = SendIoCtrlSynchronously(
        NULL,
        IOCTL_SBAUD_STREAM_CLOSE,
        sizeof(EpIndex),
        0,
        &EpIndex);

    IF_FAILED_ACTION_JUMP(
        ntStatus,
        DPF(D_ERROR, ("%!FUNC!: SendIoCtrlSynchronously(IOCTL_SBAUD_STREAM_CLOSE) failed, 0x%x", ntStatus)),
        Done);

    //
    // All done.
    //
    ntStatus = STATUS_SUCCESS;

Done:

    return ntStatus;
}

//=============================================================================
#pragma code_seg("PAGE")
VOID 
UsbHsDevice::Start()
/*++

Routine Description:

  Asynchronously called to start the audio device.

--*/
{
    PAGED_CODE();
    DPF_ENTER(("[%!FUNC!]"));

    NTSTATUS                   ntStatus            = STATUS_SUCCESS;
    ULONG                      speakerEndpoints    = 0;
    ULONG                      micEndpoints        = 0;

    //
    // Get USB headset descriptor
    //
    ntStatus = GetUsbHsDescriptor(&m_Descriptor);
    IF_FAILED_ACTION_JUMP(
        ntStatus,
        DPF(D_ERROR, ("%!FUNC!: GetUsbHsDescriptor: failed to retrieve USB Headset Descriptor, 0x%x", ntStatus)),
        Done);

    // SIDEBANDAUDIO_PARAMS_SET_USB_CONTROLLER
    PBOOL pbIsBehindHub = NULL;
    ntStatus = GetSiop(
        &SIDEBANDAUDIO_PARAMS_SET_USB_CONTROLLER,
        SIOP_TYPE_USBAUD_CONTROLLER_CONFIG_INFO_DEVICE_BEHIND_HUB,
        (ULONG)-1,
        USBSIDEBANDTEST_POOLTAG016,
        (PBYTE *)&pbIsBehindHub);
    if (!NT_SUCCESS(ntStatus))
    {
        DPF(D_ERROR, ("%!FUNC!: GetSiop(SIOP_TYPE_USBAUD_CONTROLLER_CONFIG_INFO_DEVICE_BEHIND_HUB) failed, %!STATUS!", ntStatus));
        goto Done;
    }
    SAFE_DELETE_PTR_WITH_TAG(pbIsBehindHub, USBSIDEBANDTEST_POOLTAG016);

    for (ULONG i = 0; i < m_Descriptor->NumberOfEndpoints; i++)
    {
        PSIDEBANDAUDIO_ENDPOINT_DESCRIPTOR2 ed;
        ntStatus = GetUsbHsEndpointDescriptor(i, &ed);
        IF_FAILED_ACTION_JUMP(
            ntStatus,
            DPF(D_ERROR, ("%!FUNC!: GetUsbHsEndpointDescriptor: failed to retrieve USB Headset Endpoint %d Descriptor, 0x%x", i, ntStatus)),
            Done);

        if (ed->Direction == KSPIN_DATAFLOW_OUT)
        {
            speakerEndpoints++;
            m_SpeakerEpIndex = i;
            m_pSpeakerDescriptor = ed;
        }
        else if (ed->Direction == KSPIN_DATAFLOW_IN)
        {
            micEndpoints++;
            m_MicEpIndex = i;
            m_pMicDescriptor = ed;
        }
    }

    if (speakerEndpoints > 1 ||
        micEndpoints > 1)
    {
        ntStatus = STATUS_DEVICE_FEATURE_NOT_SUPPORTED;
        m_SpeakerEpIndex = 0;
        m_MicEpIndex = 0;
    }
    IF_FAILED_ACTION_JUMP(
        ntStatus,
        DPF(D_ERROR, ("%!FUNC!: Number of endpoints on USB device not supported, 0x%x", ntStatus)),
        Done);

    //
    // Get volume settings.
    //
    if (m_pSpeakerDescriptor)
    {
        if (m_pSpeakerDescriptor->Capabilities.Volume)
        {
            PKSPROPERTY_DESCRIPTION     volumePropValues = NULL;
            LONG                        volume = 0;

            // Volume settings.
            ntStatus = GetUsbHsVolumePropertyValues(
                m_SpeakerEpIndex,
                m_pSpeakerDescriptor->VolumePropertyValuesSize,
                &volumePropValues);

            IF_FAILED_ACTION_JUMP(
                ntStatus,
                DPF(D_ERROR, ("%!FUNC!: GetUsbHsVolumePropertyValues: failed to retrieve KSPROPERTY_VALUES, 0x%x", ntStatus)),
                Done);

            m_SpeakerVolumePropValues = volumePropValues;

            // Speaker volume.
            ntStatus = GetUsbHsSpeakerVolume(0, &volume);
            IF_FAILED_ACTION_JUMP(
                ntStatus,
                DPF(D_ERROR, ("%!FUNC!: GetUsbHsSpeakerVolume: failed, 0x%x", ntStatus)),
                Done);

            m_SpeakerVolumeLevel = volume;
        }

        if (m_pSpeakerDescriptor->Capabilities.Mute)
        {
            PKSPROPERTY_DESCRIPTION     mutePropValues = NULL;
            LONG                        mute = 0;

            // Mute settings.
            ntStatus = GetUsbHsMutePropertyValues(
                m_SpeakerEpIndex,
                m_pSpeakerDescriptor->MutePropertyValuesSize,
                &mutePropValues);

            IF_FAILED_ACTION_JUMP(
                ntStatus,
                DPF(D_ERROR, ("%!FUNC!: GetUsbHsMutePropertyValues: failed to retrieve KSPROPERTY_DESCRIPTION, 0x%x", ntStatus)),
                Done);

            m_SpeakerMutePropValues         = mutePropValues;

            // Speaker mute.
            ntStatus = GetUsbHsSpeakerMute(0, &mute);
            IF_FAILED_ACTION_JUMP(
                ntStatus,
                DPF(D_ERROR, ("%!FUNC!: GetUsbHsSpeakerMute: failed, 0x%x", ntStatus)),
                Done);

            m_SpeakerMute = mute;
        }

        // Speaker Formats
        PKSDATAFORMAT speakerFormatsArray[SIZEOF_ARRAY(UsbHsSpeakerSupportedDeviceFormats)];
        for (ULONG i = 0; i < SIZEOF_ARRAY(UsbHsSpeakerSupportedDeviceFormats); i++)
        {
            speakerFormatsArray[i] = (PKSDATAFORMAT)(&UsbHsSpeakerSupportedDeviceFormats[i]);
        }
        SIDEBANDAUDIO_SUPPORTED_FORMATS speakerDeviceFormats =
        {
            sizeof(SIDEBANDAUDIO_SUPPORTED_FORMATS),
            m_SpeakerEpIndex,
            SIZEOF_ARRAY(UsbHsSpeakerSupportedDeviceFormats),
            speakerFormatsArray
        };

        ntStatus = GetUsbHsEndpointFormatsIntersection(&speakerDeviceFormats, &m_pSpeakerSupportedFormatsIntersection);
        IF_FAILED_ACTION_JUMP(
            ntStatus,
            DPF(D_ERROR, ("%!FUNC!: GetUsbHsEndpointFormats: failed for speaker, 0x%x", ntStatus)),
            Done);

        //
        // Customize the topology/wave descriptors for this instance
        //
        ntStatus = CreateCustomEndpointMinipair(
            g_UsbHsRenderEndpoints[0],
            &m_pSpeakerDescriptor->FriendlyName,
            &m_pSpeakerDescriptor->Category,
            &m_SpeakerMiniports);
        IF_FAILED_ACTION_JUMP(
            ntStatus,
            DPF(D_ERROR, ("%!FUNC!: CreateCustomEndpointMinipair for Render: failed, 0x%x", ntStatus)),
            Done);
        m_SpeakerMiniports->TopoName = m_SpeakerTopologyNameBuffer;
        m_SpeakerMiniports->WaveName = m_SpeakerWaveNameBuffer;

        // Verify format compat
        ntStatus = VerifyEndpointFormatCompatibility(
            m_pSpeakerSupportedFormatsIntersection,
            UsbHsSpeakerSupportedDeviceFormats,
            SIZEOF_ARRAY(UsbHsSpeakerSupportedDeviceFormats),
            &m_SpeakerSelectedFormat);
        IF_FAILED_ACTION_JUMP(
            ntStatus,
            DPF(D_ERROR, ("%!FUNC!: VerifyEndpointFormatCompatibility : failed for speaker, 0x%x", ntStatus)),
            Done);

        ASSERT(m_SpeakerMiniports != NULL);
        _Analysis_assume_(m_SpeakerMiniports != NULL);

        //
        // Register topology and wave filters.
        //
        ntStatus = m_Adapter->InstallEndpointFilters(
            NULL,
            m_SpeakerMiniports,
            PSIDEBANDDEVICECOMMON(this),
            &m_UnknownSpeakerTopology,
            &m_UnknownSpeakerWave, NULL, NULL
        );

        IF_FAILED_ACTION_JUMP(
            ntStatus,
            DPF(D_ERROR, ("%!FUNC!: InstallEndpointRenderFilters (USB Headset): failed, 0x%x", ntStatus)),
            Done);

        //
        // Pend status notifications.
        //

        if (m_pSpeakerDescriptor->Capabilities.Volume)
        {
            // Volume speaker status.
            ntStatus = EnableUsbHsSpeakerVolumeStatusNotification();
            IF_FAILED_ACTION_JUMP(
                ntStatus,
                DPF(D_ERROR, ("%!FUNC!: EnableUsbHsSpeakerVolumeStatusNotification: failed, 0x%x", ntStatus)),
                Done);
        }
    }

    if (m_pMicDescriptor)
    {
        if (m_pMicDescriptor->Capabilities.Volume)
        {
            PKSPROPERTY_DESCRIPTION     volumePropValues = NULL;
            LONG                        volume = 0;

            // Volume settings.
            ntStatus = GetUsbHsVolumePropertyValues(
                m_MicEpIndex,
                m_pMicDescriptor->VolumePropertyValuesSize,
                &volumePropValues);

            IF_FAILED_ACTION_JUMP(
                ntStatus,
                DPF(D_ERROR, ("%!FUNC!: GetUsbHsVolumePropertyValues: failed to retrieve KSPROPERTY_VALUES, 0x%x", ntStatus)),
                Done);

            m_MicVolumePropValues = volumePropValues;

            // Mic volume.
            ntStatus = GetUsbHsMicVolume(0, &volume);
            IF_FAILED_ACTION_JUMP(
                ntStatus,
                DPF(D_ERROR, ("%!FUNC!: GetUsbHsMicVolume: failed, 0x%x", ntStatus)),
                Done);

            m_MicVolumeLevel = volume;
        }

        if (m_pMicDescriptor->Capabilities.Mute)
        {
            PKSPROPERTY_DESCRIPTION     mutePropValues = NULL;
            LONG                        mute = 0;

            // Mute settings.
            ntStatus = GetUsbHsMutePropertyValues(
                m_MicEpIndex,
                m_pMicDescriptor->MutePropertyValuesSize,
                &mutePropValues);

            IF_FAILED_ACTION_JUMP(
                ntStatus,
                DPF(D_ERROR, ("%!FUNC!: GetUsbHsMutePropertyValues: failed to retrieve KSPROPERTY_DESCRIPTION, 0x%x", ntStatus)),
                Done);

            m_MicMutePropValues         = mutePropValues;

            // Mic mute.
            ntStatus = GetUsbHsMicMute(0, &mute);
            IF_FAILED_ACTION_JUMP(
                ntStatus,
                DPF(D_ERROR, ("%!FUNC!: GetUsbHsMicMute: failed, 0x%x", ntStatus)),
                Done);

            m_MicMute = mute;
        }

        // Mic Formats
        PKSDATAFORMAT micFormatsArray[SIZEOF_ARRAY(UsbHsMicSupportedDeviceFormats)];
        for (ULONG i = 0; i < SIZEOF_ARRAY(UsbHsMicSupportedDeviceFormats); i++)
        {
            micFormatsArray[i] = (PKSDATAFORMAT)(&UsbHsMicSupportedDeviceFormats[i]);
        }
        SIDEBANDAUDIO_SUPPORTED_FORMATS micDeviceFormats =
        {
            sizeof(SIDEBANDAUDIO_SUPPORTED_FORMATS),
            m_MicEpIndex,
            SIZEOF_ARRAY(UsbHsMicSupportedDeviceFormats),
            micFormatsArray
        };

        ntStatus = GetUsbHsEndpointFormatsIntersection(&micDeviceFormats, &m_pMicSupportedFormatsIntersection);
        IF_FAILED_ACTION_JUMP(
            ntStatus,
            DPF(D_ERROR, ("%!FUNC!: GetUsbHsEndpointFormats: failed for mic, 0x%x", ntStatus)),
            Done);

        ntStatus = CreateCustomEndpointMinipair(
            g_UsbHsCaptureEndpoints[0],
            &m_pMicDescriptor->FriendlyName,
            &m_pMicDescriptor->Category,
            &m_MicMiniports);
        IF_FAILED_ACTION_JUMP(
            ntStatus,
            DPF(D_ERROR, ("%!FUNC!: CreateCustomEndpointMinipair for Capture: failed, 0x%x", ntStatus)),
            Done);
        m_MicMiniports->TopoName = m_MicTopologyNameBuffer;
        m_MicMiniports->WaveName = m_MicWaveNameBuffer;

        // Verify format compat
        ntStatus = VerifyEndpointFormatCompatibility(
            m_pMicSupportedFormatsIntersection,
            UsbHsMicSupportedDeviceFormats,
            SIZEOF_ARRAY(UsbHsMicSupportedDeviceFormats),
            &m_MicSelectedFormat);
        IF_FAILED_ACTION_JUMP(
            ntStatus,
            DPF(D_ERROR, ("%!FUNC!: VerifyEndpointFormatCompatibility : failed for mic, 0x%x", ntStatus)),
            Done);

        ASSERT(m_MicMiniports != NULL);
        _Analysis_assume_(m_MicMiniports != NULL);

        ntStatus = m_Adapter->InstallEndpointFilters(
            NULL,
            m_MicMiniports,
            PSIDEBANDDEVICECOMMON(this),
            &m_UnknownMicTopology,
            &m_UnknownMicWave, NULL, NULL
        );

        IF_FAILED_ACTION_JUMP(
            ntStatus,
            DPF(D_ERROR, ("%!FUNC!: InstallEndpointCaptureFilters (USB Headset): failed, 0x%x", ntStatus)),
            Done);

        //
        // Pend status notifications.
        //

        if (m_pMicDescriptor->Capabilities.Volume)
        {
            // Volume mic status.
            ntStatus = EnableUsbHsMicVolumeStatusNotification();
            IF_FAILED_ACTION_JUMP(
                ntStatus,
                DPF(D_ERROR, ("%!FUNC!: EnableUsbHsMicVolumeStatusNotification: failed, 0x%x", ntStatus)),
                Done);
        }
    }

    if (speakerEndpoints > 0 && micEndpoints > 0)
    {
        ntStatus = m_Adapter->NotifyEndpointPair(
            m_SpeakerTopologyNameBuffer,
            SIZEOF_ARRAY(m_SpeakerTopologyNameBuffer),
            KSPIN_TOPO_LINEOUT_DEST,
            m_MicTopologyNameBuffer,
            SIZEOF_ARRAY(m_MicTopologyNameBuffer),
            KSPIN_TOPO_MIC_ELEMENTS
        );

        IF_FAILED_ACTION_JUMP(
            ntStatus,
            DPF(D_ERROR, ("%!FUNC!: NotifyEndpointPair: failed, 0x%x", ntStatus)),
            Done);
    }

    // Set Sideband claimed
    ntStatus = SetSidebandClaimed(TRUE);
    IF_FAILED_ACTION_JUMP(
        ntStatus,
        DPF(D_ERROR, ("%!FUNC!: SetSidebandClaimed TRUE: failed, 0x%x", ntStatus)),
        Done);

    //
    // All done.
    //
    ntStatus = STATUS_SUCCESS;

Done:
    if (!NT_SUCCESS(ntStatus))
    {
        ntStatus = SetSidebandClaimed(FALSE);
        InterlockedExchange((PLONG)&m_State, eUsbHsStateFailed);
    }
}

//=============================================================================
#pragma code_seg("PAGE")
NTSTATUS 
UsbHsDevice::CreateCustomEndpointMinipair
(
    _In_        PENDPOINT_MINIPAIR pBaseMinipair,
    _In_        PUNICODE_STRING FriendlyName,
    _In_        PGUID pCategory,
    _Outptr_    PENDPOINT_MINIPAIR *ppCustomMinipair
)
{
    NTSTATUS ntStatus;
    PENDPOINT_MINIPAIR  pNewMinipair = NULL;
    SYSVAD_DEVPROPERTY* pProperties = NULL;
    PPCFILTER_DESCRIPTOR pNewTopoFilterDesc = NULL;
    PPCPIN_DESCRIPTOR   pNewTopoPins = NULL;
    ULONG cProperties;
    ULONG cTopoPins;

    PAGED_CODE();

    //
    // This routine will add one more property to whatever the base minipair describes for the topo filter interface properties
    // It will also allocate and set up custom filter and pin descriptors to allow changing the KSNODETYPE for the usb device
    //
    cTopoPins = pBaseMinipair->TopoDescriptor->PinCount;
    cProperties = pBaseMinipair->TopoInterfacePropertyCount + 1;
    pProperties = (SYSVAD_DEVPROPERTY*)ExAllocatePool2(POOL_FLAG_NON_PAGED, cProperties * sizeof(SYSVAD_DEVPROPERTY), SYSVAD_POOLTAG);
    pNewMinipair = (ENDPOINT_MINIPAIR*)ExAllocatePool2(POOL_FLAG_NON_PAGED, sizeof(ENDPOINT_MINIPAIR), SYSVAD_POOLTAG);
    pNewTopoFilterDesc = (PCFILTER_DESCRIPTOR*)ExAllocatePool2(POOL_FLAG_NON_PAGED, sizeof(PCFILTER_DESCRIPTOR), SYSVAD_POOLTAG);
    pNewTopoPins = (PCPIN_DESCRIPTOR*)ExAllocatePool2(POOL_FLAG_NON_PAGED, cTopoPins * sizeof(PCPIN_DESCRIPTOR), SYSVAD_POOLTAG);

    if ((pProperties != NULL) && (pNewMinipair != NULL) && (pNewTopoFilterDesc != NULL) && (pNewTopoPins != NULL))
    {
        SYSVAD_DEVPROPERTY *pLastProperty;

        // Copy base minipair properties to new property list
        if (pBaseMinipair->TopoInterfacePropertyCount > 0)
        {
            RtlCopyMemory(pProperties, pBaseMinipair->TopoInterfaceProperties, (cProperties - 1) * sizeof(SYSVAD_DEVPROPERTY));
        }

        // Add friendly name property to the list
        NT_ASSERT(FriendlyName->Length + sizeof(UNICODE_NULL) <= FriendlyName->MaximumLength);  // Assuming NULL terminated string
        pLastProperty = &pProperties[cProperties - 1];
        pLastProperty->PropertyKey = &DEVPKEY_DeviceInterface_FriendlyName;
        pLastProperty->Type = DEVPROP_TYPE_STRING_INDIRECT;
        pLastProperty->BufferSize = FriendlyName->Length + sizeof(UNICODE_NULL);
        pLastProperty->Buffer = FriendlyName->Buffer;

        // Copy base minipair structure
        RtlCopyMemory(pNewMinipair, pBaseMinipair, sizeof(ENDPOINT_MINIPAIR));

        RtlCopyMemory(pNewTopoFilterDesc, pBaseMinipair->TopoDescriptor, sizeof(PCFILTER_DESCRIPTOR));
        RtlCopyMemory(pNewTopoPins, pBaseMinipair->TopoDescriptor->Pins, cTopoPins * sizeof(PCPIN_DESCRIPTOR));

        pNewTopoFilterDesc->Pins = pNewTopoPins;
        pNewMinipair->TopoDescriptor = pNewTopoFilterDesc;

        // Update it to point to new property list
        pNewMinipair->TopoInterfacePropertyCount = cProperties;
        pNewMinipair->TopoInterfaceProperties = pProperties;

        ntStatus = UpdateCustomEndpointCategory(pNewTopoFilterDesc, pNewTopoPins, pCategory);
        if (!NT_SUCCESS(ntStatus))
        {
            DPF(D_ERROR, ("%!FUNC!: UpdateCustomEndpointCategory failed, 0x%x", ntStatus));
        }
        else
        {
            *ppCustomMinipair = pNewMinipair;

            pProperties = NULL;
            pNewMinipair = NULL;
            pNewTopoFilterDesc = NULL;
            pNewTopoPins = NULL;

            ntStatus = STATUS_SUCCESS;
        }
    }
    else
    {
        ntStatus = STATUS_INSUFFICIENT_RESOURCES;
    }

    if (pProperties != NULL)
    {
        ExFreePoolWithTag(pProperties, SYSVAD_POOLTAG);
    }
    if (pNewMinipair != NULL)
    {
        ExFreePoolWithTag(pNewMinipair, SYSVAD_POOLTAG);
    }
    if (pNewTopoFilterDesc != NULL)
    {
        ExFreePoolWithTag(pNewTopoFilterDesc, SYSVAD_POOLTAG);
    }
    if (pNewTopoPins != NULL)
    {
        ExFreePoolWithTag(pNewTopoPins, SYSVAD_POOLTAG);
    }

    return ntStatus;
}
//=============================================================================
#pragma code_seg("PAGE")
NTSTATUS
UsbHsDevice::UpdateCustomEndpointCategory
(
    _In_        PPCFILTER_DESCRIPTOR pCustomMinipairTopoFilter,
    _In_        PPCPIN_DESCRIPTOR pCustomMinipairTopoPins,
    _In_        PGUID pCategory
)
{
    NTSTATUS ntStatus = STATUS_NOT_FOUND;
    ULONG cPinCount = 0;
    BOOL FoundCategoryAudio = FALSE;
    BOOL FoundNodeType = FALSE;

    PAGED_CODE();

    cPinCount = pCustomMinipairTopoFilter->PinCount;

    // Find the right pin: There should be two pins, one with Category KSCATEGORY_AUDIO,
    // and one with a KSNODETYPE_* Category. We need to modify the KSNODETYPE category.
    for (ULONG i = 0; i < cPinCount; ++i)
    {
        if (IsEqualGUID(*pCustomMinipairTopoPins[i].KsPinDescriptor.Category, KSCATEGORY_AUDIO))
        {
            ASSERT(FoundCategoryAudio == FALSE);
            if (FoundCategoryAudio)
            {
                ntStatus = STATUS_INVALID_DEVICE_STATE;
                DPF(D_ERROR, ("%!FUNC!: KSCATEGORY_AUDIO found more than once, 0x%x", ntStatus));
                break;
            }

            FoundCategoryAudio = TRUE;
            continue;
        }

        ASSERT(FoundNodeType == FALSE);
        if (FoundNodeType)
        {
            ntStatus = STATUS_INVALID_DEVICE_STATE;
            DPF(D_ERROR, ("%!FUNC!: Found more than one applicable Pin, 0x%x", ntStatus));
            break;
        }

        pCustomMinipairTopoPins[i].KsPinDescriptor.Category = pCategory;
        FoundNodeType = TRUE;
        ntStatus = STATUS_SUCCESS;
    }

    return ntStatus;
}
//=============================================================================
#pragma code_seg("PAGE")
VOID
UsbHsDevice::DeleteCustomEndpointMinipair
(
    _In_        PENDPOINT_MINIPAIR pCustomMinipair
)
{
    PAGED_CODE();

    if (pCustomMinipair != NULL)
    {
        if (pCustomMinipair->TopoInterfaceProperties != NULL)
        {
            ExFreePoolWithTag(const_cast<SYSVAD_DEVPROPERTY*>(pCustomMinipair->TopoInterfaceProperties), SYSVAD_POOLTAG);
            pCustomMinipair->TopoInterfaceProperties = NULL;
        }
        if (pCustomMinipair->TopoDescriptor != NULL)
        {
            if (pCustomMinipair->TopoDescriptor->Pins != NULL)
            {
                ExFreePoolWithTag((PVOID)pCustomMinipair->TopoDescriptor->Pins, SYSVAD_POOLTAG);
                pCustomMinipair->TopoDescriptor->Pins = NULL;
            }
            ExFreePoolWithTag(pCustomMinipair->TopoDescriptor, SYSVAD_POOLTAG);
            pCustomMinipair->TopoDescriptor = NULL;
        }
        ExFreePoolWithTag(pCustomMinipair, SYSVAD_POOLTAG);
    }
}

//=============================================================================
#pragma code_seg("PAGE")
VOID 
UsbHsDevice::Stop()
/*++

Routine Description:

  Asynchronously called to stop the audio device.
  After returning from this function, there are no more async notifications
  pending (volume, etc.).

--*/
{
    PAGED_CODE();
    DPF_ENTER(("[%!FUNC!]"));

    NTSTATUS        ntStatus    = STATUS_SUCCESS;
    eUsbHsState    state       = eUsbHsStateInvalid;

    state = (eUsbHsState) InterlockedExchange((PLONG)&m_State, eUsbHsStateStopping);
    ASSERT(state == eUsbHsStateRunning || state == eUsbHsStateFailed);
    UNREFERENCED_VAR(state);

    //
    // Stop async notifications.
    //
    WdfIoTargetPurge(m_WdfIoTarget, WdfIoTargetPurgeIoAndWait);

    //
    // Wait for work-item.
    //
    WdfWorkItemFlush(m_WorkItem);

    //
    // Remove the topology and wave render filters.
    //
    if (m_UnknownSpeakerTopology || m_UnknownSpeakerWave)
    {
        ntStatus = m_Adapter->RemoveEndpointFilters(
            m_SpeakerMiniports,
            m_UnknownSpeakerTopology,
            m_UnknownSpeakerWave);

        if (!NT_SUCCESS(ntStatus))
        {
            DPF(D_ERROR, ("%!FUNC!: RemoveEndpointFilters (USB Headset Speaker): failed, 0x%x", ntStatus));
        }
    }

    //
    // Remove the topology and wave capture filters.
    //
    if (m_UnknownMicTopology || m_UnknownMicWave)
    {
        ntStatus = m_Adapter->RemoveEndpointFilters(
            m_MicMiniports,
            m_UnknownMicTopology,
            m_UnknownMicWave);

        if (!NT_SUCCESS(ntStatus))
        {
            DPF(D_ERROR, ("%!FUNC!: RemoveEndpointFilters (USB Headset Capture): failed, 0x%x", ntStatus));
        }
    }

    //
    // Release port/miniport pointers.
    //
    SAFE_RELEASE(m_UnknownSpeakerTopology);
    SAFE_RELEASE(m_UnknownSpeakerWave);
    SAFE_RELEASE(m_UnknownMicTopology);
    SAFE_RELEASE(m_UnknownMicWave);

    //
    // The device is in the stopped state.
    //
    InterlockedExchange((PLONG)&m_State, eUsbHsStateStopped);

}

#pragma code_seg("PAGE")
NTSTATUS UsbHsDevice::CreateFilterNames(
    PUNICODE_STRING UsbHsDeviceSymbolicLinkName
)
/*++

Routine Description:

Creates unique wave and topology filter reference names.

Since each subdevice representing a USB sideband interface arrival needs to have a unique
reference string, this function generates a hash on 'UsbHsDeviceSymbolicLinkName' parameter
and appends to static interface strings.

--*/
{
    PAGED_CODE();
    DPF_ENTER(("[%!FUNC!]"));

    NTSTATUS status = STATUS_SUCCESS;
    ULONG hashValue = 0;

    status = RtlHashUnicodeString(UsbHsDeviceSymbolicLinkName, TRUE, HASH_STRING_ALGORITHM_X65599, &hashValue);
    IF_FAILED_ACTION_JUMP(
        status,
        DPF(D_ERROR, ("%!FUNC!: Failed to create KSCATEGORY_REALTIME wave interface: 0x%x", status)),
        End);

    // create wave interfaces for speaker
    RtlStringCbPrintfW(m_SpeakerWaveNameBuffer, sizeof(m_SpeakerWaveNameBuffer), L"%s-%lu", USBHS_SPEAKER_WAVE_NAME, hashValue);
    RtlUnicodeStringInit(&m_SpeakerWaveRefString, m_SpeakerWaveNameBuffer);

    // create topology interfaces for speaker
    RtlStringCbPrintfW(m_SpeakerTopologyNameBuffer, sizeof(m_SpeakerTopologyNameBuffer), L"%s-%lu", USBHS_SPEAKER_TOPO_NAME, hashValue);
    RtlUnicodeStringInit(&m_SpeakerTopologyRefString, m_SpeakerTopologyNameBuffer);

    // create wave interfaces for mic
    RtlStringCbPrintfW(m_MicWaveNameBuffer, sizeof(m_MicWaveNameBuffer), L"%s-%lu", USBHS_MIC_WAVE_NAME, hashValue);
    RtlUnicodeStringInit(&m_MicWaveRefString, m_MicWaveNameBuffer);

    // create topology interfaces for mic
    RtlStringCbPrintfW(m_MicTopologyNameBuffer, sizeof(m_MicTopologyNameBuffer), L"%s-%lu", USBHS_MIC_TOPO_NAME, hashValue);
    RtlUnicodeStringInit(&m_MicTopologyRefString, m_MicTopologyNameBuffer);
End:
    return status;
}
#endif // SYSVAD_USB_SIDEBAND
