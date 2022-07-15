/*++

Copyright (c) Microsoft Corporation All Rights Reserved

Module Name:

    common.cpp

Abstract:

    Implementation of the BthHfpDevice class.

--*/

#pragma warning (disable : 4127)

#include <initguid.h>
#include <sysvad.h>
#include "hw.h"
#include "savedata.h"
#include "IHVPrivatePropertySet.h"
#include "simple.h"

#ifdef SYSVAD_BTH_BYPASS
#include <limits.h>
#include <bthhfpddi.h>
#include <wdmguid.h>    // guild-arrival/removal
#include <devpkey.h>
#include "bthhfpminipairs.h"
#include "BthhfpDevice.h"

//
// BthHfpDevice implementation.
//

// # of sec before sync request is cancelled.
#define BTH_HFP_SYNC_REQ_TIMEOUT_IN_SEC         60 
#define BTH_HFP_NOTIFICATION_MAX_ERROR_COUNT    5

//=============================================================================
#pragma code_seg("PAGE")
STDMETHODIMP
BthHfpDevice::NonDelegatingQueryInterface
(
    _In_ REFIID                 Interface,
    _COM_Outptr_ PVOID *        Object
)
/*++

Routine Description:

QueryInterface routine for BthHfpDevice

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
    else if (IsEqualGUIDAligned(Interface, IID_IBthHfpDeviceCommon))
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
  // when the target removes the BTH HFP SCO Bypass interface.
  //

#pragma code_seg("PAGE")
NTSTATUS
BthHfpDevice::EvtBthHfpTargetQueryRemove
(
    _In_    WDFIOTARGET     IoTarget
)
{
    PAGED_CODE();
    DPF_ENTER(("[BthHfpDevice::EvtBthHfpTargetQueryRemove]"));

    UNREFERENCED_PARAMETER(IoTarget);
    return STATUS_SUCCESS;
}

#pragma code_seg("PAGE")
VOID
BthHfpDevice::EvtBthHfpTargetRemoveCanceled
(
    _In_    WDFIOTARGET     IoTarget
)
{
    PAGED_CODE();
    DPF_ENTER(("[BthHfpDevice::EvtBthHfpTargetRemoveCanceled]"));

    UNREFERENCED_PARAMETER(IoTarget);
}
#pragma code_seg("PAGE")
VOID
BthHfpDevice::EvtBthHfpTargetRemoveComplete
(
    _In_    WDFIOTARGET     IoTarget
)
{
    PAGED_CODE();
    DPF_ENTER(("[BthHfpDevice::EvtBthHfpTargetRemoveComplete]"));

    UNREFERENCED_PARAMETER(IoTarget);
}

//=============================================================================
#pragma code_seg("PAGE")
NTSTATUS
BthHfpDevice::Init
(
    _In_ IAdapterCommon     * Adapter,
    _In_ PUNICODE_STRING      SymbolicLinkName
)
{
    PAGED_CODE();
    DPF_ENTER(("[BthHfpDevice::Init]"));

    NTSTATUS                                ntStatus        = STATUS_SUCCESS;
    BthHfpDeviceNotificationWorkItemContext *wiCtx          = NULL;
    BthHfpDeviceNotificationReqContext      *reqCtx         = NULL;
    WDF_OBJECT_ATTRIBUTES                   attributes;
    WDF_IO_TARGET_OPEN_PARAMS               openParams;
    WDF_WORKITEM_CONFIG                     wiConfig;

    AddRef(); // first ref.

     //
     // Basic init of all the class' members.
     //
    m_State = eBthHfpStateInitializing;
    m_Adapter = Adapter;

    // Static config.
    m_WdfIoTarget               = NULL;
    m_SpeakerMiniports          = NULL;
    m_MicMiniports              = NULL;
    m_UnknownSpeakerTopology    = NULL;
    m_UnknownSpeakerWave        = NULL;
    m_UnknownMicTopology        = NULL;
    m_UnknownMicWave            = NULL;
    m_Descriptor                = NULL;
    m_VolumePropValues          = NULL;

    // Notification updates. 
    m_SpeakerVolumeLevel        = 0;
    m_MicVolumeLevel            = 0;
    m_ConnectionStatusLong      = FALSE;
    m_StreamStatusLong          = STATUS_INVALID_DEVICE_STATE; // Sco stream is not open.
    m_NRECDisableStatusLong     = FALSE;
        
    m_StreamReq                 = NULL;
    m_SpeakerVolumeReq          = NULL;
    m_MicVolumeReq              = NULL;
    m_ConnectionReq             = NULL;
    m_NRECDisableStatusReq      = NULL;

    m_WorkItem                  = NULL;
    m_ReqCollection             = NULL;
    m_CodecId                   = 0;
    
    m_nStreams                  = 0;

    KeInitializeEvent(&m_StreamStatusEvent, NotificationEvent, TRUE);

    InitializeListHead(&m_ListEntry);
    KeInitializeSpinLock(&m_Lock);

    RtlZeroMemory(&m_SymbolicLinkName, sizeof(m_SymbolicLinkName));

    RtlZeroMemory(&m_SpeakerVolumeCallback, sizeof(m_SpeakerVolumeCallback));
    RtlZeroMemory(&m_SpeakerConnectionStatusCallback, sizeof(m_SpeakerConnectionStatusCallback));
    RtlZeroMemory(&m_MicVolumeCallback, sizeof(m_MicVolumeCallback));
    RtlZeroMemory(&m_MicConnectionStatusCallback, sizeof(m_MicConnectionStatusCallback));
    RtlZeroMemory(&m_SpeakerFormatChangeCallback, sizeof(m_SpeakerFormatChangeCallback));
    RtlZeroMemory(&m_MicFormatChangeCallback, sizeof(m_MicFormatChangeCallback));

    //
    // Allocate a notification WDF work-item.
    //
    WDF_WORKITEM_CONFIG_INIT(&wiConfig, EvtBthHfpDeviceNotificationStatusWorkItem);
    wiConfig.AutomaticSerialization = FALSE;

    WDF_OBJECT_ATTRIBUTES_INIT_CONTEXT_TYPE(&attributes, BthHfpDeviceNotificationWorkItemContext);
    attributes.ParentObject = Adapter->GetWdfDevice();
    ntStatus = WdfWorkItemCreate(&wiConfig,
                                 &attributes,
                                 &m_WorkItem);
    IF_FAILED_ACTION_JUMP(
        ntStatus,
        DPF(D_ERROR, ("Init: WdfWorkItemCreate failed: 0x%x", ntStatus)),
        Done);

    wiCtx = GetBthHfpDeviceNotificationWorkItemContext(m_WorkItem);
    wiCtx->BthHfpDevice = this; // weak ref.

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
        DPF(D_ERROR, ("Init: WdfCollectionCreate failed: 0x%x", ntStatus)),
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
        DPF(D_ERROR, ("Init: WdfIoTargetCreate failed: 0x%x", ntStatus)),
        Done);

    WDF_IO_TARGET_OPEN_PARAMS_INIT_OPEN_BY_NAME(
        &openParams,
        SymbolicLinkName,
        STANDARD_RIGHTS_ALL);

    openParams.EvtIoTargetQueryRemove = EvtBthHfpTargetQueryRemove;
    openParams.EvtIoTargetRemoveCanceled = EvtBthHfpTargetRemoveCanceled;
    openParams.EvtIoTargetRemoveComplete = EvtBthHfpTargetRemoveComplete;

    ntStatus = WdfIoTargetOpen(m_WdfIoTarget, &openParams);
    IF_FAILED_ACTION_JUMP(
        ntStatus,
        DPF(D_ERROR, ("Init: WdfIoTargetOpen(%wZ) failed: 0x%x", SymbolicLinkName, ntStatus)),
        Done);

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
        DPF(D_ERROR, ("Init: ExAllocatePool2 failed, out of memory")),
        Done);

    RtlCopyUnicodeString(&m_SymbolicLinkName, SymbolicLinkName);

    //
    // Allocate the WDF requests for status notifications.
    //

    //
    // IOCTL_BTHHFP_DEVICE_GET_NRECDISABLE_STATUS_UPDATE 
    //
    WDF_OBJECT_ATTRIBUTES_INIT_CONTEXT_TYPE(&attributes, BthHfpDeviceNotificationReqContext);
    attributes.ParentObject = m_Adapter->GetWdfDevice();
    ntStatus = WdfRequestCreate(
        &attributes,
        m_WdfIoTarget,
        &m_NRECDisableStatusReq);
    IF_FAILED_ACTION_JUMP(
        ntStatus,
        DPF(D_ERROR, ("Init: WdfRequestCreate(Nrec-disable status) failed, 0x%x", ntStatus)),
        Done);

    // Init context.
    reqCtx = GetBthHfpDeviceNotificationReqContext(m_NRECDisableStatusReq);
    reqCtx->BthHfpDevice = this; // weak ref.

    ntStatus = WdfMemoryCreatePreallocated(
        WDF_NO_OBJECT_ATTRIBUTES,
        &reqCtx->Buffer,
        sizeof(reqCtx->Buffer.bImmediate),
        &reqCtx->MemIn);
    IF_FAILED_ACTION_JUMP(
        ntStatus,
        DPF(D_ERROR, ("Init: WdfMemoryCreatePreallocated failed, 0x%x", ntStatus)),
        Done);

    ntStatus = WdfMemoryCreatePreallocated(
        WDF_NO_OBJECT_ATTRIBUTES,
        &reqCtx->Buffer,
        sizeof(reqCtx->Buffer.BoolStatus),
        &reqCtx->MemOut);
    IF_FAILED_ACTION_JUMP(
        ntStatus,
        DPF(D_ERROR, ("Init: WdfMemoryCreatePreallocated failed, 0x%x", ntStatus)),
        Done);

    //
    // IOCTL_BTHHFP_SPEAKER_GET_VOLUME_STATUS_UPDATE 
    //
    WDF_OBJECT_ATTRIBUTES_INIT_CONTEXT_TYPE(&attributes, BthHfpDeviceNotificationReqContext);
    attributes.ParentObject = m_Adapter->GetWdfDevice();
    ntStatus = WdfRequestCreate(
        &attributes,
        m_WdfIoTarget,
        &m_SpeakerVolumeReq);
    IF_FAILED_ACTION_JUMP(
        ntStatus,
        DPF(D_ERROR, ("Init: WdfRequestCreate(Speaker-Volume) failed, 0x%x", ntStatus)),
        Done);

    // Init context.
    reqCtx = GetBthHfpDeviceNotificationReqContext(m_SpeakerVolumeReq);
    reqCtx->BthHfpDevice = this; // weak ref.

    ntStatus = WdfMemoryCreatePreallocated(
        WDF_NO_OBJECT_ATTRIBUTES,
        &reqCtx->Buffer,
        sizeof(reqCtx->Buffer.bImmediate),
        &reqCtx->MemIn);
    IF_FAILED_ACTION_JUMP(
        ntStatus,
        DPF(D_ERROR, ("Init: WdfMemoryCreatePreallocated failed, 0x%x", ntStatus)),
        Done);

    ntStatus = WdfMemoryCreatePreallocated(
        WDF_NO_OBJECT_ATTRIBUTES,
        &reqCtx->Buffer,
        sizeof(reqCtx->Buffer.Volume),
        &reqCtx->MemOut);
    IF_FAILED_ACTION_JUMP(
        ntStatus,
        DPF(D_ERROR, ("Init: WdfMemoryCreatePreallocated failed, 0x%x", ntStatus)),
        Done);

    //
    // IOCTL_BTHHFP_MIC_GET_VOLUME_STATUS_UPDATE
    //
    WDF_OBJECT_ATTRIBUTES_INIT_CONTEXT_TYPE(&attributes, BthHfpDeviceNotificationReqContext);
    attributes.ParentObject = m_Adapter->GetWdfDevice();
    ntStatus = WdfRequestCreate(
        &attributes,
        m_WdfIoTarget,
        &m_MicVolumeReq);
    IF_FAILED_ACTION_JUMP(
        ntStatus,
        DPF(D_ERROR, ("Init: WdfRequestCreate(Mic-Volume) failed, 0x%x", ntStatus)),
        Done);

    // Init context.
    reqCtx = GetBthHfpDeviceNotificationReqContext(m_MicVolumeReq);
    reqCtx->BthHfpDevice = this; // weak ref.

    ntStatus = WdfMemoryCreatePreallocated(
        WDF_NO_OBJECT_ATTRIBUTES,
        &reqCtx->Buffer,
        sizeof(reqCtx->Buffer.bImmediate),
        &reqCtx->MemIn);
    IF_FAILED_ACTION_JUMP(
        ntStatus,
        DPF(D_ERROR, ("Init: WdfMemoryCreatePreallocated failed, 0x%x", ntStatus)),
        Done);

    ntStatus = WdfMemoryCreatePreallocated(
        WDF_NO_OBJECT_ATTRIBUTES,
        &reqCtx->Buffer,
        sizeof(reqCtx->Buffer.Volume),
        &reqCtx->MemOut);
    IF_FAILED_ACTION_JUMP(
        ntStatus,
        DPF(D_ERROR, ("Init: WdfMemoryCreatePreallocated failed, 0x%x", ntStatus)),
        Done);

    //
    // IOCTL_BTHHFP_DEVICE_GET_CONNECTION_STATUS_UPDATE
    //
    WDF_OBJECT_ATTRIBUTES_INIT_CONTEXT_TYPE(&attributes, BthHfpDeviceNotificationReqContext);
    attributes.ParentObject = m_Adapter->GetWdfDevice();
    ntStatus = WdfRequestCreate(
        &attributes,
        m_WdfIoTarget,
        &m_ConnectionReq);
    IF_FAILED_ACTION_JUMP(
        ntStatus,
        DPF(D_ERROR, ("Init: WdfRequestCreate(Connection-Status) failed, 0x%x", ntStatus)),
        Done);

    // Init context.
    reqCtx = GetBthHfpDeviceNotificationReqContext(m_ConnectionReq);
    reqCtx->BthHfpDevice = this; // weak ref.

    ntStatus = WdfMemoryCreatePreallocated(
        WDF_NO_OBJECT_ATTRIBUTES,
        &reqCtx->Buffer,
        sizeof(reqCtx->Buffer.bImmediate),
        &reqCtx->MemIn);
    IF_FAILED_ACTION_JUMP(
        ntStatus,
        DPF(D_ERROR, ("Init: WdfMemoryCreatePreallocated failed, 0x%x", ntStatus)),
        Done);

    ntStatus = WdfMemoryCreatePreallocated(
        WDF_NO_OBJECT_ATTRIBUTES,
        &reqCtx->Buffer,
        sizeof(reqCtx->Buffer.BoolStatus),
        &reqCtx->MemOut);
    IF_FAILED_ACTION_JUMP(
        ntStatus,
        DPF(D_ERROR, ("Init: WdfMemoryCreatePreallocated failed, 0x%x", ntStatus)),
        Done);

    //
    // IOCTL_BTHHFP_STREAM_GET_STATUS_UPDATE
    //
    WDF_OBJECT_ATTRIBUTES_INIT_CONTEXT_TYPE(&attributes, BthHfpDeviceNotificationReqContext);
    attributes.ParentObject = m_Adapter->GetWdfDevice();
    ntStatus = WdfRequestCreate(
        &attributes,
        m_WdfIoTarget,
        &m_StreamReq);
    IF_FAILED_ACTION_JUMP(
        ntStatus,
        DPF(D_ERROR, ("Init: WdfRequestCreate(Stream-Status) failed, 0x%x", ntStatus)),
        Done);

    // Init context.
    reqCtx = GetBthHfpDeviceNotificationReqContext(m_StreamReq);
    reqCtx->BthHfpDevice = this; // weak ref.

    ntStatus = WdfMemoryCreatePreallocated(
        WDF_NO_OBJECT_ATTRIBUTES,
        &reqCtx->Buffer,
        sizeof(reqCtx->Buffer.bImmediate),
        &reqCtx->MemIn);
    IF_FAILED_ACTION_JUMP(
        ntStatus,
        DPF(D_ERROR, ("Init: WdfMemoryCreatePreallocated failed, 0x%x", ntStatus)),
        Done);

    ntStatus = WdfMemoryCreatePreallocated(
        WDF_NO_OBJECT_ATTRIBUTES,
        &reqCtx->Buffer,
        sizeof(reqCtx->Buffer.NtStatus),
        &reqCtx->MemOut);
    IF_FAILED_ACTION_JUMP(
        ntStatus,
        DPF(D_ERROR, ("Init: WdfMemoryCreatePreallocated failed, 0x%x", ntStatus)),
        Done);

    //
    // Create wave and filter names unique to this HFP device
    //
    ntStatus = CreateFilterNames(SymbolicLinkName);
    IF_FAILED_ACTION_JUMP(
        ntStatus,
        DPF(D_ERROR, ("Init: Creating unqie filter names for the hfp device failed, 0x%x", ntStatus)),
        Done);

    //
    // This remote device is now in running state. No need to use interlock operations
    // b/c at this time this is the only thread accessing this info.
    //
    m_State = eBthHfpStateRunning;

    //
    // Init successful.
    //
    ntStatus = STATUS_SUCCESS;

Done:
    return ntStatus;
}

//=============================================================================
#pragma code_seg("PAGE")
BthHfpDevice::~BthHfpDevice
(
    void
)
/*++

Routine Description:

Destructor for BthHfpDevice.

Arguments:

Return Value:

void

--*/
{
    PAGED_CODE();
    DPF_ENTER(("[BthHfpDevice::~BthHfpDevice]"));

    ASSERT(m_State != eBthHfpStateRunning);
    ASSERT(IsListEmpty(&m_ListEntry));

    //
    // Release ref to remote stack.
    //
    if (m_WdfIoTarget != NULL)
    {
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

    if (m_VolumePropValues != NULL)
    {
        ExFreePoolWithTag(m_VolumePropValues, MINADAPTER_POOLTAG);
        m_VolumePropValues = NULL;
    }

    //
    // Free Irps.
    //
    if (m_SpeakerVolumeReq != NULL)
    {
        BthHfpDeviceNotificationReqContext * ctx;

        // Delete the associated memory objects.
        ctx = GetBthHfpDeviceNotificationReqContext(m_SpeakerVolumeReq);
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
        BthHfpDeviceNotificationReqContext * ctx;

        // Delete the associated memory objects.
        ctx = GetBthHfpDeviceNotificationReqContext(m_MicVolumeReq);
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

    if (m_ConnectionReq != NULL)
    {
        BthHfpDeviceNotificationReqContext * ctx;

        // Delete the associated memory objects.
        ctx = GetBthHfpDeviceNotificationReqContext(m_ConnectionReq);
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
        WdfObjectDelete(m_ConnectionReq);
        m_ConnectionReq = NULL;
    }

    if (m_StreamReq != NULL)
    {
        BthHfpDeviceNotificationReqContext * ctx;

        // Delete the associated memory objects.
        ctx = GetBthHfpDeviceNotificationReqContext(m_StreamReq);
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
        WdfObjectDelete(m_StreamReq);
        m_StreamReq = NULL;
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

    ASSERT(m_nStreams == 0);

    ASSERT(m_SpeakerVolumeCallback.Handler == NULL);
    ASSERT(m_SpeakerConnectionStatusCallback.Handler == NULL);
    ASSERT(m_MicVolumeCallback.Handler == NULL);
    ASSERT(m_MicConnectionStatusCallback.Handler == NULL);
    ASSERT(m_SpeakerFormatChangeCallback.Handler == NULL);
    ASSERT(m_MicFormatChangeCallback.Handler == NULL);
} // ~BthHfpDevice

//
// ISidebandDeviceCommon implementation.
//

//=============================================================================
#pragma code_seg("PAGE")
BOOL
BthHfpDevice::IsVolumeSupported
(
    _In_ eDeviceType deviceType
)
{
    UNREFERENCED_PARAMETER(deviceType);
    PAGED_CODE();
    DPF_ENTER(("[BthHfpDevice::IsVolumeSupported]"));

    return m_Descriptor->SupportsVolume;
}

//=============================================================================
#pragma code_seg("PAGE")
PVOID
BthHfpDevice::GetVolumeSettings
(
    _In_ eDeviceType deviceType,
    _Out_ PULONG    Size
)
{
    UNREFERENCED_PARAMETER(deviceType);
    PAGED_CODE();
    DPF_ENTER(("[BthHfpDevice::GetVolumeSettings]"));

    ASSERT(Size != NULL);

    *Size = m_Descriptor->VolumePropertyValuesSize;

    return m_VolumePropValues;
}

//=============================================================================
#pragma code_seg("PAGE")
NTSTATUS
BthHfpDevice::GetVolume
(
    _In_    eDeviceType     DeviceType,
    _In_    LONG            Channel,
    _Out_   LONG            *pVolume
)
{
    PAGED_CODE();
    UNREFERENCED_PARAMETER(Channel);
    DPF_ENTER(("[BthHfpDevice::GetSpeakerVolume]"));

    NTSTATUS status = STATUS_SUCCESS;

    ASSERT(pVolume);

    if (eBthHfpSpeakerDevice == DeviceType)
    {
        status = GetBthHfpSpeakerVolume(pVolume);
        IF_FAILED_ACTION_JUMP(
            status,
            DPF(D_ERROR, ("Start: GetBthHfpSpeakerVolume: failed, 0x%x", status)),
            Done);

        InterlockedExchange(&m_SpeakerVolumeLevel, *pVolume);
    }
    else if(eBthHfpMicDevice == DeviceType)
    {
        status = GetBthHfpMicVolume(pVolume);
        IF_FAILED_ACTION_JUMP(
            status,
            DPF(D_ERROR, ("Start: GetBthHfpMicVolume: failed, 0x%x", status)),
            Done);

        InterlockedExchange(&m_MicVolumeLevel, *pVolume);
    }
    else
    {
        ASSERTMSG("Invalid device type", FALSE);
        status = STATUS_INVALID_PARAMETER;
    }

Done:
    return status;
}

//=============================================================================
#pragma code_seg("PAGE")
NTSTATUS
BthHfpDevice::SetVolume
(
    _In_    eDeviceType     DeviceType,
    _In_    LONG            Channel,
    _In_    LONG            Volume
)
{
    PAGED_CODE();
    UNREFERENCED_PARAMETER(Channel);
    DPF_ENTER(("[BthHfpDevice::SetSpeakerVolume]"));

    if (eBthHfpSpeakerDevice == DeviceType)
    {
        return SetBthHfpSpeakerVolume(Volume);
    }
    else if(eBthHfpMicDevice == DeviceType)
    {
        return SetBthHfpMicVolume(Volume);
    }
    else
    {
        ASSERTMSG("Invalid device type", FALSE);
        return STATUS_INVALID_PARAMETER;
    }
}

//=============================================================================
#pragma code_seg("PAGE")
BOOL
BthHfpDevice::IsMuteSupported
(
    _In_    eDeviceType     DeviceType
)
{
    PAGED_CODE();
    UNREFERENCED_PARAMETER(DeviceType);
    DPF_ENTER(("[BthHfpDevice::IsMuteSupported]"));

    return FALSE;
}

//=============================================================================
#pragma code_seg("PAGE")
PVOID
BthHfpDevice::GetMuteSettings
(
    _In_    eDeviceType     DeviceType,
    _Out_   PULONG          Size
)
{
    PAGED_CODE();
    UNREFERENCED_PARAMETER(DeviceType);
    UNREFERENCED_PARAMETER(Size);
    DPF_ENTER(("[BthHfpDevice::GetMuteSettings]"));

    return NULL;
}

//=============================================================================
#pragma code_seg("PAGE")
LONG
BthHfpDevice::GetMute
(
    _In_    eDeviceType     DeviceType,
    _In_    LONG            Channel
)
{
    PAGED_CODE();
    UNREFERENCED_PARAMETER(DeviceType);
    UNREFERENCED_PARAMETER(Channel);
    DPF_ENTER(("[BthHfpDevice::GetMute]"));

    return 0;
}

//=============================================================================
#pragma code_seg("PAGE")
NTSTATUS
BthHfpDevice::SetMute
(
    _In_    eDeviceType     DeviceType,
    _In_    LONG            Channel,
    _In_    LONG            Mute
)
{
    PAGED_CODE();
    UNREFERENCED_PARAMETER(DeviceType);
    UNREFERENCED_PARAMETER(Channel);
    UNREFERENCED_PARAMETER(Mute);
    DPF_ENTER(("[BthHfpDevice::SetMute]"));

    return STATUS_NOT_IMPLEMENTED;
}

//=============================================================================
#pragma code_seg("PAGE")
BOOL
BthHfpDevice::GetConnectionStatus()
{
    PAGED_CODE();
    DPF_ENTER(("[BthHfpDevice::GetConnectionStatus]"));

    return (BOOL)InterlockedCompareExchange(&m_ConnectionStatusLong, 0, 0);
}

//=============================================================================
#pragma code_seg("PAGE")
NTSTATUS
BthHfpDevice::GetBthHfpCodecId(_Out_ UCHAR * CodecId)
{
    PAGED_CODE();
    DPF_ENTER(("[BthHfpDevice::GetBthHfpCodecId]"));

    ASSERT(CodecId != NULL);

    NTSTATUS ntStatus = STATUS_SUCCESS;

    union {
        HFP_BYPASS_CODEC_ID_V1 CodecIdV1;
        HFP_BYPASS_CODEC_ID_VERSION Version;
    } value;

    *CodecId = 0;

    value.Version = REQ_HFP_BYPASS_CODEC_ID_V1;

    //
    // Get the Bth HFP SCO Codec ID.
    //
    ntStatus = SendIoCtrlSynchronously(
        NULL,
        IOCTL_BTHHFP_DEVICE_GET_CODEC_ID,
        sizeof(value.Version),
        sizeof(value.CodecIdV1),
        &value);

    IF_FAILED_ACTION_JUMP(
        ntStatus,
        DPF(D_ERROR, ("GetBthHfpCodecId: SendIoCtrlSynchronously(IOCTL_BTHHFP_DEVICE_GET_CODEC_ID) failed, 0x%x", ntStatus)),
        Done);

    //
    // All done.
    //
    *CodecId = value.CodecIdV1.CodecId;
    ntStatus = STATUS_SUCCESS;

Done:

    return ntStatus;
}

//=============================================================================
#pragma code_seg("PAGE")
NTSTATUS
BthHfpDevice::Connect()
{
    PAGED_CODE();
    DPF_ENTER(("[BthHfpDevice::Connect]"));

    return SetBthHfpConnect();
}

//=============================================================================
#pragma code_seg("PAGE")
NTSTATUS
BthHfpDevice::Disconnect()
{
    PAGED_CODE();
    DPF_ENTER(("[BthHfpDevice::Disconnect]"));

    return SetBthHfpDisconnect();
}

//=============================================================================
#pragma code_seg()
BOOL
BthHfpDevice::GetStreamStatus(_In_ eDeviceType deviceType)
{
    UNREFERENCED_PARAMETER(deviceType);
    DPF_ENTER(("[BthHfpDevice::GetStreamStatus]"));

    NTSTATUS ntStatus;

    ntStatus = (NTSTATUS)InterlockedCompareExchange(&m_StreamStatusLong, 0, 0);

    return NT_SUCCESS(ntStatus) ? TRUE : FALSE;
}

//=============================================================================
#pragma code_seg("PAGE")
NTSTATUS
BthHfpDevice::StreamOpen(_In_ eDeviceType deviceType)
{
    PAGED_CODE();
    UNREFERENCED_PARAMETER(deviceType);
    DPF_ENTER(("[BthHfpDevice::StreamOpen]"));

    // BTH HFP DDI has stream open and close.
    // No start and suspend commands
    // START = BTH HFP STREAM OPEN
    // SUSPEND = BTH HFP STREAM CLOSE
    return STATUS_SUCCESS;
}

//=============================================================================
#pragma code_seg("PAGE")
NTSTATUS
BthHfpDevice::StreamStart
(
    _In_ eDeviceType deviceType
)
{
    PAGED_CODE();
    UNREFERENCED_PARAMETER(deviceType);
    DPF_ENTER(("[BthHfpDevice::StreamStart]"));

    NTSTATUS    ntStatus = STATUS_SUCCESS;
    LONG        nStreams = 0;

    ASSERT(m_nStreams >= 0);

    nStreams = InterlockedIncrement(&m_nStreams);
    if (nStreams == 1)
    {
        BOOLEAN  streamStart = FALSE;

        ntStatus = SetBthHfpStreamOpen();
        if (NT_SUCCESS(ntStatus))
        {
            streamStart = TRUE;
            m_StreamStatus = STATUS_SUCCESS;
            ntStatus = EnableBthHfpStreamStatusNotification();
        }

        //
        // Cleanup if any error.
        //
        if (!NT_SUCCESS(ntStatus))
        {
            nStreams = InterlockedDecrement(&m_nStreams);
            ASSERT(nStreams == 0);
            UNREFERENCED_VAR(nStreams);

            if (streamStart)
            {
                SetBthHfpStreamClose();
            }

            m_StreamStatus = STATUS_INVALID_DEVICE_STATE;
        }
    }

    return ntStatus;
}

//=============================================================================
#pragma code_seg("PAGE")
NTSTATUS
BthHfpDevice::StreamSuspend
(
    _In_ eDeviceType deviceType
)
{
    UNREFERENCED_PARAMETER(deviceType);
    PAGED_CODE();
    DPF_ENTER(("[BthHfpDevice::StreamSuspend]"));

    NTSTATUS    ntStatus = STATUS_SUCCESS;
    LONG        nStreams = 0;

    ASSERT(m_nStreams > 0);

    nStreams = InterlockedDecrement(&m_nStreams);
    if (nStreams == 0)
    {
        ntStatus = SetBthHfpStreamClose();

        StopBthHfpStreamStatusNotification();

        m_StreamStatus = STATUS_INVALID_DEVICE_STATE;
    }

    return ntStatus;
}

//=============================================================================
#pragma code_seg("PAGE")
NTSTATUS
BthHfpDevice::StreamClose
(
    _In_ eDeviceType deviceType
)
{
    UNREFERENCED_PARAMETER(deviceType);
    PAGED_CODE();
    DPF_ENTER(("[BthHfpDevice::StreamClose]"));

    // BTH HFP DDI has stream open and close.
    // No start and suspend commands
    // START = BTH HFP STREAM OPEN
    // SUSPEND = BTH HFP STREAM CLOSE
    return STATUS_SUCCESS;
}

//=============================================================================
#pragma code_seg("PAGE")
GUID
BthHfpDevice::GetContainerId(_In_ eDeviceType deviceType)
{
    PAGED_CODE();
    DPF_ENTER(("[BthHfpDevice::GetContainerId]"));

    UNREFERENCED_PARAMETER(deviceType);

    return m_Descriptor->ContainerId;
}

//=============================================================================
#pragma code_seg("PAGE")
VOID
BthHfpDevice::SetVolumeHandler
(
    _In_        eDeviceType             deviceType,
    _In_opt_    PFNEVENTNOTIFICATION    EventHandler,
    _In_opt_    PVOID                   EventHandlerContext
)
{
    PAGED_CODE();
    DPF_ENTER(("[BthHfpDevice::SetVolumeHandler]"));

    if (deviceType == eBthHfpSpeakerDevice)
    {
        ASSERT(EventHandler == NULL || m_SpeakerVolumeCallback.Handler == NULL);

        m_SpeakerVolumeCallback.Handler = EventHandler; // weak ref.
        m_SpeakerVolumeCallback.Context = EventHandlerContext;
    }
    else if (deviceType == eBthHfpMicDevice)
    {
        ASSERT(EventHandler == NULL || m_MicVolumeCallback.Handler == NULL);

        m_MicVolumeCallback.Handler = EventHandler; // weak ref.
        m_MicVolumeCallback.Context = EventHandlerContext;
    }
}

//=============================================================================
#pragma code_seg("PAGE")
VOID
BthHfpDevice::SetMuteHandler
(
    _In_        eDeviceType             deviceType,
    _In_opt_    PFNEVENTNOTIFICATION    EventHandler,
    _In_opt_    PVOID                   EventHandlerContext
)
{
    PAGED_CODE();
    DPF_ENTER(("[BthHfpDevice::SetMuteHandler]"));

    if (deviceType == eBthHfpSpeakerDevice)
    {
        ASSERT(EventHandler == NULL || m_SpeakerMuteCallback.Handler == NULL);

        m_SpeakerMuteCallback.Handler = EventHandler; // weak ref.
        m_SpeakerMuteCallback.Context = EventHandlerContext;
    }
    else if (deviceType == eBthHfpMicDevice)
    {
        ASSERT(EventHandler == NULL || m_MicMuteCallback.Handler == NULL);

        m_MicMuteCallback.Handler = EventHandler; // weak ref.
        m_MicMuteCallback.Context = EventHandlerContext;
    }
}

//=============================================================================
#pragma code_seg("PAGE")
VOID
BthHfpDevice::SetConnectionStatusHandler
(
    _In_        eDeviceType             deviceType,
    _In_opt_    PFNEVENTNOTIFICATION    EventHandler,
    _In_opt_    PVOID                   EventHandlerContext
)
{
    PAGED_CODE();
    DPF_ENTER(("[BthHfpDevice::SetConnectionStatusHandler]"));

    if (deviceType == eBthHfpSpeakerDevice)
    {
        ASSERT(EventHandler == NULL || m_SpeakerConnectionStatusCallback.Handler == NULL);

        m_SpeakerConnectionStatusCallback.Handler = EventHandler; // weak ref.
        m_SpeakerConnectionStatusCallback.Context = EventHandlerContext;
    }
    else if (deviceType == eBthHfpMicDevice)
    {
        ASSERT(EventHandler == NULL || m_MicConnectionStatusCallback.Handler == NULL);

        m_MicConnectionStatusCallback.Handler = EventHandler; // weak ref.
        m_MicConnectionStatusCallback.Context = EventHandlerContext;
    }

}

//=============================================================================
#pragma code_seg("PAGE")
VOID
BthHfpDevice::SetFormatChangeHandler
(
    _In_        eDeviceType             deviceType,
    _In_opt_    PFNEVENTNOTIFICATION    EventHandler,
    _In_opt_    PVOID                   EventHandlerContext
)
{
    PAGED_CODE();
    DPF_ENTER(("[BthHfpDevice::SetFormatChangeHandler]"));

    if (deviceType == eBthHfpSpeakerDevice)
    {
        ASSERT(EventHandler == NULL || m_SpeakerFormatChangeCallback.Handler == NULL);

        m_SpeakerFormatChangeCallback.Handler = EventHandler; // weak ref.
        m_SpeakerFormatChangeCallback.Context = EventHandlerContext;
    }
    else if (deviceType == eBthHfpMicDevice)
    {
        ASSERT(EventHandler == NULL || m_MicFormatChangeCallback.Handler == NULL);

        m_MicFormatChangeCallback.Handler = EventHandler; // weak ref.
        m_MicFormatChangeCallback.Context = EventHandlerContext;
    }

}

//=============================================================================
#pragma code_seg("PAGE")
PPIN_DEVICE_FORMATS_AND_MODES
BthHfpDevice::GetFormatsAndModes
(
    _In_        eDeviceType             deviceType
)
{
    PAGED_CODE();

    PPIN_DEVICE_FORMATS_AND_MODES pFormatsAndModes = NULL;
    ULONG endpointIndex = 0;


    // CVSD is the narrow band codec for SCO. Wideband codec IDs are any number higher than 1.
    // mSBC is the only required wideband codec, though the controller+headset combination may
    // support other wideband codecs.

    enum
    {
        BTHHFP_CODEC_CVSD = 0,
        BTHHFP_CODEC_mSBC,

        BTHHFP_CODEC_INVALID
    };

    if (m_CodecId > BTHHFP_CODEC_CVSD)
    {
        // Use the miniport tables that support 16kHz
        endpointIndex = 1;
    }

    if (deviceType == eBthHfpSpeakerDevice)
    {
        pFormatsAndModes = g_BthHfpRenderEndpoints[endpointIndex]->PinDeviceFormatsAndModes;
    }
    else if (deviceType == eBthHfpMicDevice)
    {
        pFormatsAndModes = g_BthHfpCaptureEndpoints[endpointIndex]->PinDeviceFormatsAndModes;
    }

    return pFormatsAndModes;
}

//=============================================================================
#pragma code_seg()
_IRQL_requires_max_(DISPATCH_LEVEL)
BOOL
BthHfpDevice::IsNRECSupported()
{
    DPF_ENTER(("[BthHfpDevice::IsNRECSupported]"));

    return m_Descriptor->SupportsNREC;
}

//=============================================================================
#pragma code_seg("PAGE")
BOOL
BthHfpDevice::GetNRECDisableStatus()
{
    PAGED_CODE();
    DPF_ENTER(("[BthHfpDevice::GetNRECDisableStatus]"));

    // Return TRUE if HF wants to disable the NREC on the AG.
    return (BOOL)InterlockedCompareExchange(&m_NRECDisableStatusLong, 0, 0);
}

//
// Helper functions.
//

//=============================================================================
#pragma code_seg()
NTSTATUS
BthHfpDevice::SendIoCtrlAsynchronously
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

This function aynchronously sends an I/O Ctrl request to the BTH HFP
SCO Bypass device.

--*/
{
    DPF_ENTER(("[BthHfpDevice::SendIoCtrlAsynchronously]"));

    NTSTATUS    ntStatus = STATUS_SUCCESS;
    BOOLEAN     fSent = FALSE;

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
        DPF(D_ERROR, ("SendIoCtrlAsynchronously: WdfIoTargetFormatRequestForIoctl(0x%x) failed, 0x%x", IoControlCode, ntStatus)),
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

        DPF(D_ERROR, ("SendIoCtrlAsynchronously: WdfRequestSend(0x%x) failed, 0x%x", IoControlCode, ntStatus));
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
BthHfpDevice::SendIoCtrlSynchronously
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

This function inits and synchronously sends an I/O Ctrl request to the BTH HFP
SCO Bypass device.

--*/
{
    PAGED_CODE();
    DPF_ENTER(("[BthHfpDevice::SendIoCtrlSynchronously]"));

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
        WDF_MEMORY_DESCRIPTOR_INIT_BUFFER(&memIn, Buffer, InLength);
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

    reqOpts.Timeout = WDF_REL_TIMEOUT_IN_SEC(BTH_HFP_SYNC_REQ_TIMEOUT_IN_SEC);

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
        DPF(D_VERBOSE, ("SendIoCtrlSynchronously: WdfIoTargetSendIoctlSynchronously(0x%x) failed, 0x%x", IoControlCode, ntStatus)),
        Done);

Done:

    return ntStatus;
}

//=============================================================================
#pragma code_seg()
VOID
BthHfpDevice::EvtBthHfpDeviceNotificationStatusWorkItem
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
    DPF_ENTER(("[BthHfpDevice::EvtBthHfpDeviceNotificationStatusWorkItem]"));

    NTSTATUS            ntStatus = STATUS_SUCCESS;
    BthHfpDevice      * This;
    KIRQL               oldIrql;

    if (WorkItem == NULL)
    {
        return;
    }

    This = GetBthHfpDeviceNotificationWorkItemContext(WorkItem)->BthHfpDevice;
    ASSERT(This != NULL);

    for (;;)
    {
        BOOL                                    resend  = TRUE;
        WDFREQUEST                              req     = NULL;
        BthHfpDeviceNotificationReqContext    * reqCtx;
        WDF_REQUEST_COMPLETION_PARAMS           params;

        //
        // Retrieve a task.
        //
        KeAcquireSpinLock(&This->m_Lock, &oldIrql);
        req = (WDFREQUEST)WdfCollectionGetFirstItem(This->m_ReqCollection);
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

        reqCtx = GetBthHfpDeviceNotificationReqContext(req);
        ASSERT(reqCtx != NULL);

        //
        // Handle this notification.
        //
        if (NT_SUCCESS(params.IoStatus.Status))
        {
            switch (params.Parameters.Ioctl.IoControlCode)
            {
            case IOCTL_BTHHFP_DEVICE_GET_NRECDISABLE_STATUS_UPDATE:
            {
                InterlockedExchange(&This->m_NRECDisableStatusLong, (LONG)reqCtx->Buffer.BoolStatus);
            }
            break;

            case IOCTL_BTHHFP_SPEAKER_GET_VOLUME_STATUS_UPDATE:
            {
                LONG oldVolume;

                oldVolume = InterlockedExchange(&This->m_SpeakerVolumeLevel, reqCtx->Buffer.Volume);
                if (reqCtx->Buffer.Volume != oldVolume)
                {
                    // Notify audio miniport about this change.
                    if (This->m_SpeakerVolumeCallback.Handler != NULL)
                    {
                        This->m_SpeakerVolumeCallback.Handler(
                            This->m_SpeakerVolumeCallback.Context);
                    }
                }
            }
            break;

            case IOCTL_BTHHFP_MIC_GET_VOLUME_STATUS_UPDATE:
            {
                LONG oldVolume;

                oldVolume = InterlockedExchange(&This->m_MicVolumeLevel, reqCtx->Buffer.Volume);
                if (reqCtx->Buffer.Volume != oldVolume)
                {
                    // Notify audio miniport about this change.
                    if (This->m_MicVolumeCallback.Handler != NULL)
                    {
                        This->m_MicVolumeCallback.Handler(
                            This->m_MicVolumeCallback.Context);
                    }
                }
            }
            break;

            case IOCTL_BTHHFP_DEVICE_GET_CONNECTION_STATUS_UPDATE:
            {
                BOOL oldStatus;
                //

                //
                // Get codec id (if non-zero, connection supports Wideband Speech)
                //
                UCHAR codecId = 0;
                UCHAR oldCodecId = 0;
                if (reqCtx->Buffer.BoolStatus)
                {
                    ntStatus = This->GetBthHfpCodecId(&codecId);
                    if (ntStatus == STATUS_INVALID_DEVICE_REQUEST)
                    {
                        // GetBthHfpCodecId fails with STATUS_INVALID_DEVICE_REQUEST if the system doesn't
                        // support Wideband Speech (currently only Mobile supports this call)
                        ntStatus = STATUS_SUCCESS;
                    }

                    if (NT_SUCCESS(ntStatus))
                    {
                        oldCodecId = (UCHAR)InterlockedExchange(&This->m_CodecId, codecId);
                        if (codecId != oldCodecId)
                        {
                            // Notify audio miniport about this change.
                            if (This->m_SpeakerFormatChangeCallback.Handler != NULL)
                            {
                                This->m_SpeakerFormatChangeCallback.Handler(
                                    This->m_SpeakerFormatChangeCallback.Context);
                            }

                            if (This->m_MicFormatChangeCallback.Handler != NULL)
                            {
                                This->m_MicFormatChangeCallback.Handler(
                                    This->m_MicFormatChangeCallback.Context);
                            }
                        }
                    }
                }

                oldStatus = (BOOL)InterlockedExchange(&This->m_ConnectionStatusLong, (LONG)reqCtx->Buffer.BoolStatus);
                if (reqCtx->Buffer.BoolStatus != oldStatus)
                {
                    // Notify audio miniport about this change.
                    if (This->m_SpeakerConnectionStatusCallback.Handler != NULL)
                    {
                        This->m_SpeakerConnectionStatusCallback.Handler(
                            This->m_SpeakerConnectionStatusCallback.Context);
                    }

                    if (This->m_MicConnectionStatusCallback.Handler != NULL)
                    {
                        This->m_MicConnectionStatusCallback.Handler(
                            This->m_MicConnectionStatusCallback.Context);
                    }
                }
            }
            break;

            default:
                // This should never happen.
                resend = FALSE;
                DPF(D_ERROR, ("EvtBthHfpDeviceNotificationStatusWorkItem: invalid request ctrl 0x%x",
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
                DPF(D_ERROR, ("EvtBthHfpDeviceNotificationStatusWorkItem: WdfRequestReuse failed, 0x%x", ntStatus));
                break;
            }

            // Resend status notification request.
            reqCtx->Buffer.bImmediate = FALSE;

            ntStatus = This->SendIoCtrlAsynchronously(
                req,
                params.Parameters.Ioctl.IoControlCode,
                reqCtx->MemIn,
                reqCtx->MemOut,
                EvtBthHfpDeviceNotificationStatusCompletion,
                This);

            if (!NT_SUCCESS(ntStatus))
            {
                DPF(D_ERROR, ("EvtBthHfpDeviceNotificationStatusWorkItem: SendIoCtrlAsynchronously"
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
BthHfpDevice::EvtBthHfpDeviceNotificationStatusCompletion
(
    _In_  WDFREQUEST Request,
    _In_  WDFIOTARGET Target,
    _In_  PWDF_REQUEST_COMPLETION_PARAMS Params,
    _In_  WDFCONTEXT Context
)
{
    DPF_ENTER(("[BthHfpDevice::EvtBthHfpDeviceNotificationStatusCompletion]"));

    NTSTATUS                                ntStatus    = STATUS_SUCCESS;
    BthHfpDeviceNotificationReqContext    * ctx         = NULL;
    BthHfpDevice                          * This        = NULL;
    KIRQL                                   oldIrql;

    UNREFERENCED_PARAMETER(Target);
    UNREFERENCED_PARAMETER(Context);

    ctx = GetBthHfpDeviceNotificationReqContext(Request);
    This = ctx->BthHfpDevice;
    ASSERT(This != NULL);

    ntStatus = Params->IoStatus.Status;
    if (ntStatus == STATUS_CANCELLED)
    {
        // BTH HFP device is shutting down. Do not re-send this request.
        goto Done;
    }

    //
    // If something is wrong with the HFP interface, do not loop forever.
    //
    if (!NT_SUCCESS(ntStatus))
    {
        if (++ctx->Errors > BTH_HFP_NOTIFICATION_MAX_ERROR_COUNT)
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
BthHfpDevice::SetBthHfpDeviceCapabilities
(
    _In_    BOOL bSupports16kHzSampling

)
/*++

Routine Description:

This function synchronously informs the Audio Driver capabilities to BthHfp stack.

--*/
{
    PAGED_CODE();
    DPF_ENTER(("[BthHfpDevice::SetBthHfpDeviceCapabilities]"));

    NTSTATUS                            ntStatus = STATUS_SUCCESS;
    BTHHFP_AUDIO_DEVICE_CAPABILTIES     deviceCaps = { 0 };

    BTHHFP_AUDIO_DEVICE_CAPABILTIES_INIT(&deviceCaps);
    deviceCaps.Supports16kHzSampling = bSupports16kHzSampling;

    //
    // Get the Bth HFP SCO Bypass speaker volume.
    //
    ntStatus = SendIoCtrlSynchronously(
        NULL,
        IOCTL_BTHHFP_DEVICE_INDICATE_AUDIO_DEVICE_CAPABILITIES,
        sizeof(BTHHFP_AUDIO_DEVICE_CAPABILTIES),
        0,
        &deviceCaps);

    IF_FAILED_ACTION_JUMP(
        ntStatus,
        DPF(D_ERROR, ("SetBthHfpDeviceCapabilities: SendIoCtrlSynchronously(IOCTL_BTHHFP_DEVICE_INDICATE_AUDIO_DEVICE_CAPABILITIES) failed, 0x%x", ntStatus)),
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
BthHfpDevice::GetBthHfpDescriptor
(
    _Out_ PBTHHFP_DESCRIPTOR2 * Descriptor
)
/*++

Routine Description:

This function synchronously gets the remote Bluetooth Hands-Free Profile SCO
Bypass descriptor.

--*/
{
    PAGED_CODE();
    DPF_ENTER(("[BthHfpDevice::GetBthHfpDescriptor]"));

    NTSTATUS                    ntStatus    = STATUS_SUCCESS;
    WDFREQUEST                  req         = NULL;
    PBTHHFP_DESCRIPTOR2         descriptor  = NULL;
    ULONG                       length      = 0;
    ULONG_PTR                   information = 0;
    WDF_REQUEST_REUSE_PARAMS    reuseParams;
    WDF_OBJECT_ATTRIBUTES       attributes;

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
        DPF(D_ERROR, ("GetBthHfpDescriptor: WdfRequestCreate failed, 0x%x", ntStatus)),
        Done);

    //
    // Get the size of the buffer.
    //
    ntStatus = SendIoCtrlSynchronously(
        req,
        IOCTL_BTHHFP_DEVICE_GET_DESCRIPTOR2,
        NULL,
        NULL,
        NULL);

    if (ntStatus != STATUS_BUFFER_TOO_SMALL)
    {
        if (NT_SUCCESS(ntStatus))
        {
            ntStatus = STATUS_INVALID_DEVICE_STATE;
        }

        DPF(D_ERROR, ("GetBthHfpDescriptor: SendIoCtrlSynchronously(IOCTL_BTHHFP_DEVICE_GET_DESCRIPTOR2): failed, 0x%x", ntStatus));
        goto Done;
    }

    ntStatus = STATUS_SUCCESS;

    information = WdfRequestGetInformation(req);
    if (information == 0 || information > ULONG_MAX)
    {
        ntStatus = STATUS_INVALID_DEVICE_STATE;
        DPF(D_ERROR, ("GetBthHfpDescriptor: IOCTL_BTHHFP_DEVICE_GET_DESCRIPTOR2 buffer too big (%Id): 0x%x", information, ntStatus));
        goto Done;
    }

    length = (ULONG)information;

    //
    // Allocate memory needed to hold the info.
    //
    descriptor = (PBTHHFP_DESCRIPTOR2)ExAllocatePool2(POOL_FLAG_NON_PAGED, length, MINADAPTER_POOLTAG);
    if (descriptor == NULL)
    {
        ntStatus = STATUS_INSUFFICIENT_RESOURCES;
    }
    IF_FAILED_ACTION_JUMP(
        ntStatus,
        DPF(D_ERROR, ("GetBthHfpDescriptor: ExAllocatePool2 failed, out of memory")),
        Done);

    //
    // Get the Bth HFP SCO Bypass descriptor.
    //
    WDF_REQUEST_REUSE_PARAMS_INIT(
        &reuseParams,
        WDF_REQUEST_REUSE_NO_FLAGS,
        STATUS_SUCCESS);

    ntStatus = WdfRequestReuse(req, &reuseParams);
    IF_FAILED_ACTION_JUMP(
        ntStatus,
        DPF(D_ERROR, ("GetBthHfpDescriptor: WdfRequestReuse failed, 0x%x", ntStatus)),
        Done);

    ntStatus = SendIoCtrlSynchronously(
        req,
        IOCTL_BTHHFP_DEVICE_GET_DESCRIPTOR2,
        NULL,
        length,
        descriptor);
    IF_FAILED_ACTION_JUMP(
        ntStatus,
        DPF(D_ERROR, ("GetBthHfpDescriptor: SendIoCtrlSynchronously(IOCTL_BTHHFP_DEVICE_GET_DESCRIPTOR2) failed, 0x%x", ntStatus)),
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
#pragma code_seg()
NTSTATUS
BthHfpDevice::EnableBthHfpNrecDisableStatusNotification()
/*++

Routine Description:

This function registers for Bluetooth Hands-Free Profile SCO Bypass NREC-Disable
status change notification.

--*/
{
    DPF_ENTER(("[BthHfpDevice::EnableBthHfpNrecDisableStatusNotification]"));

    NTSTATUS                                ntStatus    = STATUS_SUCCESS;
    BthHfpDeviceNotificationReqContext    * ctx         = NULL;

    ctx = GetBthHfpDeviceNotificationReqContext(m_NRECDisableStatusReq);

    //
    // This is a notification request.
    //
    ctx->Buffer.bImmediate = FALSE;

    //
    // Get the Bth HFP SCO Bypass NREC-Disable status (async).
    //
    ntStatus = SendIoCtrlAsynchronously(
        m_NRECDisableStatusReq,
        IOCTL_BTHHFP_DEVICE_GET_NRECDISABLE_STATUS_UPDATE,
        ctx->MemIn,
        ctx->MemOut,
        EvtBthHfpDeviceNotificationStatusCompletion,
        this);

    IF_FAILED_ACTION_JUMP(
        ntStatus,
        DPF(D_ERROR, ("EnableBthHfpNrecDisableStatusNotification: SendIoCtrlAsynchronously(IOCTL_BTHHFP_DEVICE_GET_NRECDISABLE_STATUS_UPDATE) failed, 0x%x", ntStatus)),
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
BthHfpDevice::GetBthHfpVolumePropertyValues
(
    _In_  ULONG                 Length,
    _Out_ PKSPROPERTY_VALUES  * PropValues
)
/*++

Routine Description:

This function synchronously gets the remote Bluetooth Hands-Free Profile SCO
Bypass volume values.

--*/
{
    PAGED_CODE();
    DPF_ENTER(("[BthHfpDevice::GetBthHfpVolumePropertyValues]"));

    NTSTATUS            ntStatus    = STATUS_SUCCESS;
    PKSPROPERTY_VALUES  propValues  = NULL;

    *PropValues = NULL;

    //
    // Allocate memory.
    //
    propValues = (PKSPROPERTY_VALUES)ExAllocatePool2(POOL_FLAG_NON_PAGED, Length, MINADAPTER_POOLTAG);
    if (propValues == NULL)
    {
        ntStatus = STATUS_INSUFFICIENT_RESOURCES;
    }
    IF_FAILED_ACTION_JUMP(
        ntStatus,
        DPF(D_ERROR, ("GetBthHfpVolumePropertyValues: ExAllocatePool2 failed, out of memory")),
        Done);

    //
    // Get the Bth HFP SCO Bypass descriptor.
    //
    ntStatus = SendIoCtrlSynchronously(
        NULL,
        IOCTL_BTHHFP_DEVICE_GET_VOLUMEPROPERTYVALUES,
        0,
        Length,
        propValues);

    IF_FAILED_ACTION_JUMP(
        ntStatus,
        DPF(D_ERROR, ("GetBthHfpVolumePropertyValues: SendIoCtrlSynchronously(IOCTL_BTHHFP_DEVICE_GET_VOLUMEPROPERTYVALUES) failed, 0x%x", ntStatus)),
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
BthHfpDevice::SetBthHfpSpeakerVolume
(
    _In_ LONG  Volume
)
/*++

Routine Description:

This function synchronously sets the remote Bluetooth Hands-Free Profile SCO
Bypass speaker volume.

--*/
{
    PAGED_CODE();
    DPF_ENTER(("[BthHfpDevice::SetBthHfpSpeakerVolume]"));

    NTSTATUS        ntStatus = STATUS_SUCCESS;

    //
    // Get the Bth HFP SCO Bypass speaker volume.
    //
    ntStatus = SendIoCtrlSynchronously(
        NULL,
        IOCTL_BTHHFP_SPEAKER_SET_VOLUME,
        sizeof(Volume),
        0,
        &Volume);

    IF_FAILED_ACTION_JUMP(
        ntStatus,
        DPF(D_ERROR, ("SetBthHfpSpeakerVolume: SendIoCtrlSynchronously(IOCTL_BTHHFP_SPEAKER_SET_VOLUME) failed, 0x%x", ntStatus)),
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
BthHfpDevice::GetBthHfpSpeakerVolume
(
    _Out_ LONG  * Volume
)
/*++

Routine Description:

This function synchronously gets the remote Bluetooth Hands-Free Profile SCO
Bypass speaker volume.

--*/
{
    PAGED_CODE();
    DPF_ENTER(("[BthHfpDevice::GetBthHfpSpeakerVolume]"));

    NTSTATUS                        ntStatus    = STATUS_SUCCESS;
    BthHfpDeviceNotificationBuffer  buffer      = { 0 };

    *Volume = 0;

    buffer.bImmediate = TRUE;

    //
    // Get the Bth HFP SCO Bypass speaker volume.
    //
    ntStatus = SendIoCtrlSynchronously(
        NULL,
        IOCTL_BTHHFP_SPEAKER_GET_VOLUME_STATUS_UPDATE,
        sizeof(buffer.bImmediate),
        sizeof(buffer.Volume),
        &buffer);

    IF_FAILED_ACTION_JUMP(
        ntStatus,
        DPF(D_ERROR, ("GetBthHfpSpeakerVolume: SendIoCtrlSynchronously(IOCTL_BTHHFP_SPEAKER_GET_VOLUME_STATUS_UPDATE) failed, 0x%x", ntStatus)),
        Done);

    //
    // All done.
    //
    *Volume = buffer.Volume;
    ntStatus = STATUS_SUCCESS;

Done:

    return ntStatus;
}

//=============================================================================
#pragma code_seg()
NTSTATUS
BthHfpDevice::EnableBthHfpSpeakerVolumeStatusNotification()
/*++

Routine Description:

This function registers for Bluetooth Hands-Free Profile SCO Bypass speaker
volume change notification.

--*/
{
    DPF_ENTER(("[BthHfpDevice::EnableBthHfpSpeakerVolumeStatusNotification]"));

    NTSTATUS                                ntStatus    = STATUS_SUCCESS;
    BthHfpDeviceNotificationReqContext    * ctx         = NULL;

    ctx = GetBthHfpDeviceNotificationReqContext(m_SpeakerVolumeReq);

    //
    // This is a notification request.
    //
    ctx->Buffer.bImmediate = FALSE;

    //
    // Register for speaker volume updates.
    //
    ntStatus = SendIoCtrlAsynchronously(
        m_SpeakerVolumeReq,
        IOCTL_BTHHFP_SPEAKER_GET_VOLUME_STATUS_UPDATE,
        ctx->MemIn,
        ctx->MemOut,
        EvtBthHfpDeviceNotificationStatusCompletion,
        this);

    IF_FAILED_ACTION_JUMP(
        ntStatus,
        DPF(D_ERROR, ("EnableBthHfpSpeakerVolumeStatusNotification: SendIoCtrlAsynchronously(IOCTL_BTHHFP_SPEAKER_GET_VOLUME_STATUS_UPDATE) failed, 0x%x", ntStatus)),
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
BthHfpDevice::SetBthHfpMicVolume
(
    _In_ LONG  Volume
)
/*++

Routine Description:

This function synchronously sets the remote Bluetooth Hands-Free Profile SCO
Bypass mic volume.

--*/
{
    PAGED_CODE();
    DPF_ENTER(("[BthHfpDevice::SetBthHfpMicVolume]"));

    NTSTATUS        ntStatus = STATUS_SUCCESS;

    //
    // Get the Bth HFP SCO Bypass mic volume.
    //
    ntStatus = SendIoCtrlSynchronously(
        NULL,
        IOCTL_BTHHFP_MIC_SET_VOLUME,
        sizeof(Volume),
        0,
        &Volume);

    IF_FAILED_ACTION_JUMP(
        ntStatus,
        DPF(D_ERROR, ("SetBthHfpMicVolume: SendIoCtrlSynchronously(IOCTL_BTHHFP_MIC_SET_VOLUME) failed, 0x%x", ntStatus)),
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
BthHfpDevice::GetBthHfpMicVolume
(
    _Out_ LONG  * Volume
)
/*++

Routine Description:

This function synchronously gets the remote Bluetooth Hands-Free Profile SCO
Bypass mic volume.

--*/
{
    PAGED_CODE();
    DPF_ENTER(("[BthHfpDevice::GetBthHfpMicVolume]"));

    NTSTATUS                        ntStatus    = STATUS_SUCCESS;
    BthHfpDeviceNotificationBuffer  buffer      = { 0 };

    *Volume = 0;

    buffer.bImmediate = TRUE;

    //
    // Get the Bth HFP SCO Bypass mic volume.
    //
    ntStatus = SendIoCtrlSynchronously(
        NULL,
        IOCTL_BTHHFP_MIC_GET_VOLUME_STATUS_UPDATE,
        sizeof(buffer.bImmediate),
        sizeof(buffer.Volume),
        &buffer);

    IF_FAILED_ACTION_JUMP(
        ntStatus,
        DPF(D_ERROR, ("GetBthHfpMicVolume: SendIoCtrlSynchronously(IOCTL_BTHHFP_MIC_GET_VOLUME_STATUS_UPDATE) failed, 0x%x", ntStatus)),
        Done);

    //
    // All done.
    //
    *Volume = buffer.Volume;
    ntStatus = STATUS_SUCCESS;

Done:

    return ntStatus;
}

//=============================================================================
#pragma code_seg()
NTSTATUS
BthHfpDevice::EnableBthHfpMicVolumeStatusNotification()
/*++

Routine Description:

This function registers for Bluetooth Hands-Free Profile SCO Bypass mic
volume change notification.

--*/
{
    DPF_ENTER(("[BthHfpDevice::EnableBthHfpMicVolumeStatusNotification]"));

    NTSTATUS                               ntStatus = STATUS_SUCCESS;
    BthHfpDeviceNotificationReqContext   * ctx      = NULL;

    ctx = GetBthHfpDeviceNotificationReqContext(m_MicVolumeReq);

    //
    // This is a notification request.
    //
    ctx->Buffer.bImmediate = FALSE;

    //
    // Register for mic volume updates.
    //
    ntStatus = SendIoCtrlAsynchronously(
        m_MicVolumeReq,
        IOCTL_BTHHFP_MIC_GET_VOLUME_STATUS_UPDATE,
        ctx->MemIn,
        ctx->MemOut,
        EvtBthHfpDeviceNotificationStatusCompletion,
        this);

    IF_FAILED_ACTION_JUMP(
        ntStatus,
        DPF(D_ERROR, ("EnableBthHfpMicVolumeStatusNotification: SendIoCtrlAsynchronously(IOCTL_BTHHFP_MIC_GET_VOLUME_STATUS_UPDATE) failed, 0x%x", ntStatus)),
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
BthHfpDevice::GetBthHfpConnectionStatus
(
    _Out_ BOOL * ConnectionStatus
)
/*++

Routine Description:

This function synchronously gets the remote Bluetooth Hands-Free Profile SCO
Bypass connection status.

--*/
{
    PAGED_CODE();
    DPF_ENTER(("[BthHfpDevice::GetBthHfpConnectionStatus]"));

    NTSTATUS            ntStatus    = STATUS_SUCCESS;
    BOOL                bValue      = TRUE; // In: bImmediate, Out: value.

    *ConnectionStatus = 0;

    //
    // Get the Bth HFP SCO Bypass connection status.
    //
    ntStatus = SendIoCtrlSynchronously(
        NULL,
        IOCTL_BTHHFP_DEVICE_GET_CONNECTION_STATUS_UPDATE,
        sizeof(bValue),
        sizeof(bValue),
        &bValue);

    IF_FAILED_ACTION_JUMP(
        ntStatus,
        DPF(D_ERROR, ("GetBthHfpConnectionStatus: SendIoCtrlSynchronously(IOCTL_BTHHFP_DEVICE_GET_CONNECTION_STATUS_UPDATE) failed, 0x%x", ntStatus)),
        Done);

    //
    // All done.
    //
    *ConnectionStatus = bValue;
    ntStatus = STATUS_SUCCESS;

Done:

    return ntStatus;
}

//=============================================================================
#pragma code_seg()
NTSTATUS
BthHfpDevice::EnableBthHfpConnectionStatusNotification()
/*++

Routine Description:

This function registers for Bluetooth Hands-Free Profile SCO Bypass
connection status notification.

--*/
{
    DPF_ENTER(("[BthHfpDevice::EnableBthHfpConnectionStatusNotification]"));

    NTSTATUS                               ntStatus = STATUS_SUCCESS;
    BthHfpDeviceNotificationReqContext   * ctx      = NULL;

    ctx = GetBthHfpDeviceNotificationReqContext(m_ConnectionReq);

    //
    // Make sure this obj is alive while the IRP is active.
    //
    ctx->Buffer.bImmediate = FALSE;

    //
    // Get the Bth HFP SCO Bypass connection status.
    //
    ntStatus = SendIoCtrlAsynchronously(
        m_ConnectionReq,
        IOCTL_BTHHFP_DEVICE_GET_CONNECTION_STATUS_UPDATE,
        ctx->MemIn,
        ctx->MemOut,
        EvtBthHfpDeviceNotificationStatusCompletion,
        this);

    IF_FAILED_ACTION_JUMP(
        ntStatus,
        DPF(D_ERROR, ("EnableBthHfpConnectionStatusNotification: SendIoCtrlAsynchronously(IOCTL_BTHHFP_DEVICE_GET_CONNECTION_STATUS_UPDATE) failed, 0x%x", ntStatus)),
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
BthHfpDevice::SetBthHfpConnect()
/*++

Routine Description:

This function synchronously requests a Bluetooth Hands-Free Profile level
connection to the paired Bluetooth device.

This request initiates the Service Level Connection establishment procedure
and completes without waiting for the connection procedure to complete.
Connection status can be determined using IOCTL_BTHHFP_GET_CONNECTION_STATUS_UPDATE.

--*/
{
    PAGED_CODE();
    DPF_ENTER(("[BthHfpDevice::SetBthHfpConnect]"));

    NTSTATUS        ntStatus = STATUS_SUCCESS;

    ntStatus = SendIoCtrlSynchronously(
        NULL,
        IOCTL_BTHHFP_DEVICE_REQUEST_CONNECT,
        0,
        0,
        NULL);

    IF_FAILED_ACTION_JUMP(
        ntStatus,
        DPF(D_ERROR, ("SetBthHfpConnect: SendIoCtrlSynchronously(IOCTL_BTHHFP_DEVICE_REQUEST_CONNECT) failed, 0x%x", ntStatus)),
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
BthHfpDevice::SetBthHfpDisconnect()
/*++

Routine Description:

This function synchronously requests a Bluetooth Hands-Free Profile level
connection to the paired Bluetooth device.

This request initiates disconnection of the Service Level Connection and
completes without waiting for the disconnection to complete. Connection
status can be determined using IOCTL_BTHHFP_GET_CONNECTION_STATUS_UPDATE.

--*/
{
    PAGED_CODE();
    DPF_ENTER(("[BthHfpDevice::SetBthHfpDisconnect]"));

    NTSTATUS        ntStatus = STATUS_SUCCESS;

    ntStatus = SendIoCtrlSynchronously(
        NULL,
        IOCTL_BTHHFP_DEVICE_REQUEST_DISCONNECT,
        0,
        0,
        NULL);

    IF_FAILED_ACTION_JUMP(
        ntStatus,
        DPF(D_ERROR, ("SetBthHfpDisconnect: SendIoCtrlSynchronously(IOCTL_BTHHFP_DEVICE_REQUEST_DISCONNECT) failed, 0x%x", ntStatus)),
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
BthHfpDevice::EvtBthHfpDeviceStreamStatusCompletion
(
    _In_  WDFREQUEST Request,
    _In_  WDFIOTARGET Target,
    _In_  PWDF_REQUEST_COMPLETION_PARAMS Params,
    _In_  WDFCONTEXT Context
)
/*++

Routine Description:

Completion callback for the Bluetooth Hands-Free Profile SCO Bypass
stream status notification.

--*/
{
    DPF_ENTER(("[BthHfpDevice::EvtBthHfpDeviceStreamStatusCompletion]"));

    NTSTATUS                                ntStatus    = STATUS_SUCCESS;
    BthHfpDeviceNotificationReqContext    * reqCtx      = NULL;
    BthHfpDevice                          * This        = NULL;
    NTSTATUS                                ntResult    = STATUS_SUCCESS;

    UNREFERENCED_PARAMETER(Target);
    UNREFERENCED_PARAMETER(Context);

    //
    // Get the SCO stream status.
    //
    reqCtx = GetBthHfpDeviceNotificationReqContext(Request);
    This = reqCtx->BthHfpDevice;
    ASSERT(This != NULL);

    ntStatus = Params->IoStatus.Status;
    if (!NT_SUCCESS(ntStatus))
    {
        ntResult = STATUS_INVALID_DEVICE_STATE;
    }
    else
    {
        ntResult = reqCtx->Buffer.NtStatus;
    }

    InterlockedExchange(&This->m_StreamStatusLong, (LONG)ntResult);

    //
    // Let the stop routine know we are done. Stop routine will 
    // re-init the request.
    //
    KeSetEvent(&This->m_StreamStatusEvent, IO_NO_INCREMENT, FALSE);
}

//=============================================================================
#pragma code_seg()
NTSTATUS
BthHfpDevice::EnableBthHfpStreamStatusNotification()
/*++

Routine Description:

This function registers for Bluetooth Hands-Free Profile SCO Bypass
stream status notification.

--*/
{
    DPF_ENTER(("[BthHfpDevice::EnableBthHfpStreamStatusNotification]"));

    NTSTATUS                               ntStatus = STATUS_SUCCESS;
    BthHfpDeviceNotificationReqContext   * ctx      = NULL;

    ASSERT(m_nStreams > 0);

    ctx = GetBthHfpDeviceNotificationReqContext(m_StreamReq);
    ctx->Buffer.bImmediate = FALSE;

    KeClearEvent(&m_StreamStatusEvent);

    //
    // Get the Bth HFP SCO Bypass connection status.
    //
    ntStatus = SendIoCtrlAsynchronously(
        m_StreamReq,
        IOCTL_BTHHFP_STREAM_GET_STATUS_UPDATE,
        ctx->MemIn,
        ctx->MemOut,
        EvtBthHfpDeviceStreamStatusCompletion,
        this);

    if (!NT_SUCCESS(ntStatus))
    {
        KeSetEvent(&m_StreamStatusEvent, IO_NO_INCREMENT, FALSE);
        DPF(D_ERROR, ("EnableBthHfpStreamStatusNotification: SendIoCtrlAsynchronously(IOCTL_BTHHFP_STREAM_GET_STATUS_UPDATE) failed, 0x%x", ntStatus));
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
BthHfpDevice::StopBthHfpStreamStatusNotification()
/*++

Routine Description:

This function stops the Bluetooth Hands-Free Profile SCO Bypass
connection status notification.
The function waits for the request to be done before returning.

--*/
{
    PAGED_CODE();
    DPF_ENTER(("[BthHfpDevice::StopBthHfpStreamStatusNotification]"));

    NTSTATUS                                ntStatus    = STATUS_SUCCESS;
    BthHfpDeviceNotificationReqContext    * reqCtx      = NULL;
    WDF_REQUEST_REUSE_PARAMS                reuseParams;

    WdfRequestCancelSentRequest(m_StreamReq);
    KeWaitForSingleObject(&m_StreamStatusEvent, Executive, KernelMode, FALSE, NULL);

    reqCtx = GetBthHfpDeviceNotificationReqContext(m_StreamReq);
    ASSERT(reqCtx != NULL);
    UNREFERENCED_VAR(reqCtx);

    // 
    // Re-init the request for later.
    //
    WDF_REQUEST_REUSE_PARAMS_INIT(
        &reuseParams,
        WDF_REQUEST_REUSE_NO_FLAGS,
        STATUS_SUCCESS);

    ntStatus = WdfRequestReuse(m_StreamReq, &reuseParams);
    if (!NT_SUCCESS(ntStatus))
    {
        DPF(D_ERROR, ("StopBthHfpStreamStatusNotification: WdfRequestReuse failed, 0x%x", ntStatus));
    }

    return STATUS_SUCCESS;
}

//=============================================================================
#pragma code_seg("PAGE")
NTSTATUS
BthHfpDevice::SetBthHfpStreamOpen()
/*++

Routine Description:

This function synchronously requests an open SCO channel to transmit audio
data over the air.

--*/
{
    PAGED_CODE();
    DPF_ENTER(("[BthHfpDevice::SetBthHfpStreamOpen]"));

    NTSTATUS        ntStatus = STATUS_SUCCESS;

    ntStatus = SendIoCtrlSynchronously(
        NULL,
        IOCTL_BTHHFP_STREAM_OPEN,
        0,
        0,
        NULL);

    if (ntStatus == STATUS_DEVICE_BUSY)
    {
        // The stream channel is already open.
        DPF(D_VERBOSE, ("SetBthHfpStreamOpen: the stream channel is already open"));
        ntStatus = STATUS_SUCCESS;
    }

    IF_FAILED_ACTION_JUMP(
        ntStatus,
        DPF(D_ERROR, ("SetBthHfpStreamOpen: SendIoCtrlSynchronously(IOCTL_BTHHFP_STREAM_OPEN) failed, 0x%x", ntStatus)),
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
BthHfpDevice::SetBthHfpStreamClose()
/*++

Routine Description:

This function synchronously requests to close the SCO channel.

--*/
{
    PAGED_CODE();
    DPF_ENTER(("[BthHfpDevice::SetBthHfpStreamClose]"));

    NTSTATUS        ntStatus = STATUS_SUCCESS;

    ntStatus = SendIoCtrlSynchronously(
        NULL,
        IOCTL_BTHHFP_STREAM_CLOSE,
        0,
        0,
        NULL);

    IF_FAILED_ACTION_JUMP(
        ntStatus,
        DPF(D_ERROR, ("SetBthHfpStreamClose: SendIoCtrlSynchronously(IOCTL_BTHHFP_STREAM_CLOSE) failed, 0x%x", ntStatus)),
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
BthHfpDevice::Start()
/*++

Routine Description:

Asynchronously called to start the audio device.

--*/
{
    PAGED_CODE();
    DPF_ENTER(("[BthHfpDevice::Start]"));

    NTSTATUS                            ntStatus            = STATUS_SUCCESS;
    BOOL                                connStatus          = FALSE;
    UINT                                bthMiniportsIndex   = 0;

    // Indicate audio driver capabilities indicating support for 16KHz sampling rate
    ntStatus = SetBthHfpDeviceCapabilities(TRUE);
    IF_FAILED_ACTION_JUMP(
        ntStatus,
        DPF(D_ERROR, ("Start: SetBthHfpDeviceCapabilities: failed to set Wideband Support, 0x%x", ntStatus)),
        Done);

    //
    // Get bth hfp descriptor
    //
    ntStatus = GetBthHfpDescriptor(&m_Descriptor);
    IF_FAILED_ACTION_JUMP(
        ntStatus,
        DPF(D_ERROR, ("Start: GetBthHfpDescriptor: failed to retrieve BTHHFP_DESCRIPTOR2, 0x%x", ntStatus)),
        Done);

    //
    // Get valume settings.
    //
    if (m_Descriptor->SupportsVolume)
    {
        PKSPROPERTY_VALUES  volumePropValues    = NULL;
        LONG                volume              = 0;

        // Volume settings.
        ntStatus = GetBthHfpVolumePropertyValues(
            m_Descriptor->VolumePropertyValuesSize,
            &volumePropValues);

        IF_FAILED_ACTION_JUMP(
            ntStatus,
            DPF(D_ERROR, ("Start: GetBthHfpVolumePropertyValues: failed to retrieve KSPROPERTY_VALUES, 0x%x", ntStatus)),
            Done);

        m_VolumePropValues = volumePropValues;

        // Speaker volume.
        ntStatus = GetBthHfpSpeakerVolume(&volume);
        IF_FAILED_ACTION_JUMP(
            ntStatus,
            DPF(D_ERROR, ("Start: GetBthHfpSpeakerVolume: failed, 0x%x", ntStatus)),
            Done);

        m_SpeakerVolumeLevel = volume;

        // Mic volume.
        ntStatus = GetBthHfpMicVolume(&volume);
        IF_FAILED_ACTION_JUMP(
            ntStatus,
            DPF(D_ERROR, ("Start: GetBthHfpMicVolume: failed, 0x%x", ntStatus)),
            Done);

        m_MicVolumeLevel = volume;
    }

    //
    // Get connection status.
    //
    ntStatus = GetBthHfpConnectionStatus(&connStatus);
    IF_FAILED_ACTION_JUMP(
        ntStatus,
        DPF(D_ERROR, ("Start: GetBthHfpConnectionStatus: failed, 0x%x", ntStatus)),
        Done);

    m_ConnectionStatus = connStatus;

    //
    // Get codec id (if non-zero, connection supports Wideband Speech)
    //
    UCHAR codecId = 0;
    if (connStatus)
    {
        ntStatus = GetBthHfpCodecId(&codecId);
        if (ntStatus == STATUS_INVALID_DEVICE_REQUEST)
        {
            // GetBthHfpCodecId fails with STATUS_INVALID_DEVICE_REQUEST if the system doesn't
            // support Wideband Speech (currently only Mobile supports this call)
            ntStatus = STATUS_SUCCESS;
        }
        IF_FAILED_ACTION_JUMP(
            ntStatus,
            DPF(D_ERROR, ("Start: GetBthHfpCodecId: failed, 0x%x", ntStatus)),
            Done);
    }

    m_CodecId = codecId;

    enum
    {
        BTHHFP_CODEC_CVSD = 0,
        BTHHFP_CODEC_mSBC,

        BTHHFP_CODEC_INVALID
    };

    if (codecId > BTHHFP_CODEC_CVSD)
    {
        // Use the miniport tables that support 16kHz
        bthMiniportsIndex = 1;
    }


    //
    // Customize the topology/wave descriptors for this instance
    //
    ntStatus = CreateCustomEndpointMinipair(
        g_BthHfpRenderEndpoints[bthMiniportsIndex],
        &m_Descriptor->FriendlyName,
        &m_Descriptor->OutputPinCategory,
        &m_SpeakerMiniports);
    IF_FAILED_ACTION_JUMP(
        ntStatus,
        DPF(D_ERROR, ("Start: CreateCustomEndpointMinipair for Render: failed, 0x%x", ntStatus)),
        Done);
    m_SpeakerMiniports->TopoName = m_SpeakerTopologyNameBuffer;
    m_SpeakerMiniports->WaveName = m_SpeakerWaveNameBuffer;

    ntStatus = CreateCustomEndpointMinipair(
        g_BthHfpCaptureEndpoints[bthMiniportsIndex],
        &m_Descriptor->FriendlyName,
        &m_Descriptor->InputPinCategory,
        &m_MicMiniports);
    IF_FAILED_ACTION_JUMP(
        ntStatus,
        DPF(D_ERROR, ("Start: CreateCustomEndpointMinipair for Capture: failed, 0x%x", ntStatus)),
        Done);
    m_MicMiniports->TopoName = m_MicTopologyNameBuffer;
    m_MicMiniports->WaveName = m_MicWaveNameBuffer;


    ASSERT(m_SpeakerMiniports != NULL);
    ASSERT(m_MicMiniports != NULL);
    _Analysis_assume_(m_SpeakerMiniports != NULL);
    _Analysis_assume_(m_MicMiniports != NULL);

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
        DPF(D_ERROR, ("Start: InstallEndpointRenderFilters (Bth HFP SCO-Bypass): failed, 0x%x", ntStatus)),
        Done);

    ntStatus = m_Adapter->InstallEndpointFilters(
        NULL,
        m_MicMiniports,
        PSIDEBANDDEVICECOMMON(this),
        &m_UnknownMicTopology,
        &m_UnknownMicWave, NULL, NULL
    );

    IF_FAILED_ACTION_JUMP(
        ntStatus,
        DPF(D_ERROR, ("Start: InstallEndpointCaptureFilters (Bth HFP SCO-Bypass): failed, 0x%x", ntStatus)),
        Done);

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
        DPF(D_ERROR, ("Start: NotifyEndpointPair (Bth HFP SCO-Bypass): failed, 0x%x", ntStatus)),
        Done);

    //
    // Pend status notifications.
    //

    // NREC disable AudioGateway (AG) status.
    ntStatus = EnableBthHfpNrecDisableStatusNotification();
    IF_FAILED_ACTION_JUMP(
        ntStatus,
        DPF(D_ERROR, ("Start: EnableBthHfpNrecDisableStatusNotification: failed, 0x%x", ntStatus)),
        Done);

    // Volume speaker status.
    ntStatus = EnableBthHfpSpeakerVolumeStatusNotification();
    IF_FAILED_ACTION_JUMP(
        ntStatus,
        DPF(D_ERROR, ("Start: EnableBthHfpSpeakerVolumeStatusNotification: failed, 0x%x", ntStatus)),
        Done);

    // Volume mic status.
    ntStatus = EnableBthHfpMicVolumeStatusNotification();
    IF_FAILED_ACTION_JUMP(
        ntStatus,
        DPF(D_ERROR, ("Start: EnableBthHfpMicVolumeStatusNotification: failed, 0x%x", ntStatus)),
        Done);

    // Connection status.
    ntStatus = EnableBthHfpConnectionStatusNotification();
    IF_FAILED_ACTION_JUMP(
        ntStatus,
        DPF(D_ERROR, ("Start: EnableBthHfpConnectionStatusNotification: failed, 0x%x", ntStatus)),
        Done);

    //
    // All done.
    //
    ntStatus = STATUS_SUCCESS;

Done:
    if (!NT_SUCCESS(ntStatus))
    {
        InterlockedExchange((PLONG)&m_State, eBthHfpStateFailed);
    }
}

//=============================================================================
#pragma code_seg("PAGE")
NTSTATUS
BthHfpDevice::CreateCustomEndpointMinipair
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
    // It will also allocate and set up custom filter and pin descriptors to allow changing the KSNODETYPE for the hfp device
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
            DPF(D_ERROR, ("UpdateCustomEndpointCategory: failed, 0x%x", ntStatus));
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
BthHfpDevice::UpdateCustomEndpointCategory
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
                DPF(D_ERROR, ("UpdateCustomEndpointCategory: KSCATEGORY_AUDIO found more than once, 0x%x", ntStatus));
                break;
            }

            FoundCategoryAudio = TRUE;
            continue;
        }

        ASSERT(FoundNodeType == FALSE);
        if (FoundNodeType)
        {
            ntStatus = STATUS_INVALID_DEVICE_STATE;
            DPF(D_ERROR, ("UpdateCustomEndpointCategory: Found more than one applicable Pin, 0x%x", ntStatus));
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
BthHfpDevice::DeleteCustomEndpointMinipair
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
BthHfpDevice::Stop()
/*++

Routine Description:

Asynchronously called to stop the audio device.
After returning from this function, there are no more async notifications
pending (volume, connection, etc.).

--*/
{
    PAGED_CODE();
    DPF_ENTER(("[BthHfpDevice::Stop]"));

    NTSTATUS        ntStatus    = STATUS_SUCCESS;
    eBthHfpState    state       = eBthHfpStateInvalid;

    state = (eBthHfpState)InterlockedExchange((PLONG)&m_State, eBthHfpStateStopping);
    ASSERT(state == eBthHfpStateRunning || state == eBthHfpStateFailed);
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
            DPF(D_ERROR, ("RemoveEndpointFilters (Bth HFP SCO-Bypass Speaker): failed, 0x%x", ntStatus));
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
            DPF(D_ERROR, ("RemoveEndpointFilters (Bth HFP SCO-Bypass Capture): failed, 0x%x", ntStatus));
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
    InterlockedExchange((PLONG)&m_State, eBthHfpStateStopped);

}

#pragma code_seg("PAGE")
NTSTATUS BthHfpDevice::CreateFilterNames(
    PUNICODE_STRING HfpDeviceSymbolicLinkName
)
/*++

Routine Description:

Creates unique wave and topology filter reference names.

Since each subdevice representing a HFP sideband interface arrival needs to have a unique
reference string, this function generates a hash on 'HfpDeviceSymbolicLinkName' parameter
and appends to static interface strings.

--*/
{
    PAGED_CODE();
    DPF_ENTER(("[BthHfpDevice::CreateAudioInterfaces]"));

    NTSTATUS status = STATUS_SUCCESS;
    ULONG hashValue = 0;

    status = RtlHashUnicodeString(HfpDeviceSymbolicLinkName, TRUE, HASH_STRING_ALGORITHM_X65599, &hashValue);
    IF_FAILED_ACTION_JUMP(
        status,
        DPF(D_ERROR, ("Failed to create KSCATEGORY_REALTIME wave interface: 0x%x", status)),
        End);

    // create wave interfaces for speaker
    RtlStringCbPrintfW(m_SpeakerWaveNameBuffer, sizeof(m_SpeakerWaveNameBuffer), L"%s-%lu", BTHHFP_SPEAKER_WAVE_NAME, hashValue);
    RtlUnicodeStringInit(&m_SpeakerWaveRefString, m_SpeakerWaveNameBuffer);

    // create topology interfaces for speaker
    RtlStringCbPrintfW(m_SpeakerTopologyNameBuffer, sizeof(m_SpeakerTopologyNameBuffer), L"%s-%lu", BTHHFP_SPEAKER_TOPO_NAME, hashValue);
    RtlUnicodeStringInit(&m_SpeakerTopologyRefString, m_SpeakerTopologyNameBuffer);

    // create wave interfaces for mic
    RtlStringCbPrintfW(m_MicWaveNameBuffer, sizeof(m_MicWaveNameBuffer), L"%s-%lu", BTHHFP_MIC_WAVE_NAME, hashValue);
    RtlUnicodeStringInit(&m_MicWaveRefString, m_MicWaveNameBuffer);

    // create topology interfaces for mic
    RtlStringCbPrintfW(m_MicTopologyNameBuffer, sizeof(m_MicTopologyNameBuffer), L"%s-%lu", BTHHFP_MIC_TOPO_NAME, hashValue);
    RtlUnicodeStringInit(&m_MicTopologyRefString, m_MicTopologyNameBuffer);
End:
    return status;
}

#endif // SYSVAD_BTH_BYPASS
