/*++

Copyright (c) Microsoft Corporation All Rights Reserved

Module Name:

    A2dpHpDevice.cpp

Abstract:

    Implementation of the A2dpHpDevice class.

--*/

#pragma warning (disable : 4127)

#include <initguid.h>
#include <sysvad.h>
#include "hw.h"
#include "savedata.h"
#include "IHVPrivatePropertySet.h"
#include "simple.h"

#ifdef SYSVAD_A2DP_SIDEBAND
#include <limits.h>
#include <SidebandAudio.h>
#include <A2DPSidebandAudio.h>
#include <wdmguid.h>    // guild-arrival/removal
#include <devpkey.h>
#include "a2dphpminipairs.h"
#include "A2dpHpDevice.h"
#include "A2dpHpDeviceFormats.h"

//
// A2dpHpDevice implementation.
//

// # of sec before sync request is cancelled.
#define A2DP_SIDEBAND_SYNC_REQ_TIMEOUT_IN_SEC         60 
#define A2DP_SIDEBAND_NOTIFICATION_MAX_ERROR_COUNT    5

//=============================================================================
#pragma code_seg("PAGE")
STDMETHODIMP
A2dpHpDevice::NonDelegatingQueryInterface
( 
    _In_ REFIID                 Interface,
    _COM_Outptr_ PVOID *        Object 
)
/*++

Routine Description:

  QueryInterface routine for A2dpHpDevice

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
    else if (IsEqualGUIDAligned(Interface, IID_IA2dpHpDeviceCommon))
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
#pragma code_seg("PAGE")
NTSTATUS
A2dpHpDevice::Init
(
    _In_ IAdapterCommon     * Adapter, 
    _In_ PUNICODE_STRING      SymbolicLinkName
)
{
    PAGED_CODE();
    DPF_ENTER(("[%!FUNC!]"));

    NTSTATUS                                ntStatus        = STATUS_SUCCESS;
    A2dpHpDeviceNotificationWorkItemContext *wiCtx          = NULL;
    A2dpHpDeviceNotificationReqContext      *reqCtx         = NULL;
    WDF_OBJECT_ATTRIBUTES                   attributes;
    WDF_IO_TARGET_OPEN_PARAMS               openParams;
    WDF_WORKITEM_CONFIG                     wiConfig;

    AddRef(); // first ref.
    
    //
    // Basic init of all the class' members.
    //
    m_State                         = eA2dpHpStateInitializing;
    m_Adapter                       = Adapter;

    // Static config.
    m_WdfIoTarget                   = NULL;
    m_SpeakerMiniports              = NULL;
    m_UnknownSpeakerTopology        = NULL;
    m_UnknownSpeakerWave            = NULL;
    m_Descriptor                    = NULL;
    m_SpeakerVolumePropValues       = NULL;
    m_SpeakerMutePropValues         = NULL;

    // Notification updates. 
    m_SpeakerVolumeLevel            = 0;
    m_SpeakerMute                   = 0;
    RtlZeroMemory(&m_ConnectedCodecCaps, sizeof(A2DPHPDEVICE_CODEC_CAPABILITIES));
    ExInitializeFastMutex(&m_ConnectedCodecLock);
    m_ConnectionStatusLong          = FALSE;
    m_SpeakerStreamStatusLong       = STATUS_SUCCESS; // A2DP stream is not open.
        
    m_SpeakerStreamReq              = NULL;
    m_SpeakerVolumeReq              = NULL;
    m_SpeakerMuteReq                = NULL;
    m_ConnectionReq                 = NULL;
    m_SiopUpdateReq                 = NULL;

    m_WorkItem                      = NULL;
    m_ReqCollection                 = NULL;
    
    m_nSpeakerStreams               = 0;
    m_nSpeakerStartedStreams        = 0;

    m_pSpeakerSupportedFormatsIntersection  = NULL;

    KeInitializeEvent(&m_SpeakerStreamStatusEvent, NotificationEvent, TRUE);

    InitializeListHead(&m_ListEntry);
    KeInitializeSpinLock(&m_Lock);
    
    RtlZeroMemory(&m_SymbolicLinkName, sizeof(m_SymbolicLinkName));
    
    RtlZeroMemory(&m_SpeakerVolumeCallback, sizeof(m_SpeakerVolumeCallback));
    RtlZeroMemory(&m_SpeakerMuteCallback, sizeof(m_SpeakerMuteCallback));
    RtlZeroMemory(&m_SpeakerConnectionStatusCallback, sizeof(m_SpeakerConnectionStatusCallback));
    RtlZeroMemory(&m_SpeakerFormatChangeCallback, sizeof(m_SpeakerFormatChangeCallback));

    RtlZeroMemory(&m_SpeakerTransportResources, sizeof(A2DPHPDEVICE_EP_TRANSPORT_RESOURCES));
    
    //
    // Allocate a notification WDF work-item.
    //
    WDF_WORKITEM_CONFIG_INIT(&wiConfig, EvtA2dpHpDeviceNotificationStatusWorkItem);
    wiConfig.AutomaticSerialization = FALSE;

    WDF_OBJECT_ATTRIBUTES_INIT_CONTEXT_TYPE(&attributes, A2dpHpDeviceNotificationWorkItemContext);
    attributes.ParentObject = Adapter->GetWdfDevice();
    ntStatus = WdfWorkItemCreate( &wiConfig,
                                  &attributes,
                                  &m_WorkItem);
    IF_FAILED_ACTION_JUMP(
        ntStatus,
        DPF(D_ERROR, ("%!FUNC!: WdfWorkItemCreate failed: 0x%x", ntStatus)),
        Done);

    wiCtx = GetA2dpHpDeviceNotificationWorkItemContext(m_WorkItem);
    wiCtx->A2dpHpDevice = this; // weak ref.

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

    ntStatus = WdfIoTargetOpen(m_WdfIoTarget, &openParams);
    IF_FAILED_ACTION_JUMP(
        ntStatus,
        DPF(D_ERROR, ("%!FUNC!: WdfIoTargetOpen(%wZ) failed: 0x%x", SymbolicLinkName, ntStatus)),
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
    WDF_OBJECT_ATTRIBUTES_INIT_CONTEXT_TYPE(&attributes, A2dpHpDeviceNotificationReqContext);
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
    reqCtx = GetA2dpHpDeviceNotificationReqContext(m_SpeakerVolumeReq);
    reqCtx->A2dpHpDevice = this; // weak ref.
    
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
    WDF_OBJECT_ATTRIBUTES_INIT_CONTEXT_TYPE(&attributes, A2dpHpDeviceNotificationReqContext);
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
    reqCtx = GetA2dpHpDeviceNotificationReqContext(m_SpeakerMuteReq);
    reqCtx->A2dpHpDevice = this; // weak ref.
    
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
    // IOCTL_SBAUD_GET_CONNECTION_STATUS_UPDATE
    //
    WDF_OBJECT_ATTRIBUTES_INIT_CONTEXT_TYPE(&attributes, A2dpHpDeviceNotificationReqContext);
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
    reqCtx = GetA2dpHpDeviceNotificationReqContext(m_ConnectionReq);
    reqCtx->A2dpHpDevice = this; // weak ref.

    ntStatus = WdfMemoryCreatePreallocated(
        WDF_NO_OBJECT_ATTRIBUTES,
        &reqCtx->Buffer,
        sizeof(reqCtx->Buffer.ConnectionStatus),
        &reqCtx->MemIn);
    IF_FAILED_ACTION_JUMP(
        ntStatus,
        DPF(D_ERROR, ("Init: WdfMemoryCreatePreallocated failed, 0x%x", ntStatus)),
        Done);

    ntStatus = WdfMemoryCreatePreallocated(
        WDF_NO_OBJECT_ATTRIBUTES,
        &reqCtx->Buffer,
        sizeof(reqCtx->Buffer.ConnectionStatus),
        &reqCtx->MemOut);
    IF_FAILED_ACTION_JUMP(
        ntStatus,
        DPF(D_ERROR, ("Init: WdfMemoryCreatePreallocated failed, 0x%x", ntStatus)),
        Done);
    
    //
    // IOCTL_SBAUD_GET_STREAM_STATUS_UPDATE
    //
    WDF_OBJECT_ATTRIBUTES_INIT_CONTEXT_TYPE(&attributes, A2dpHpDeviceNotificationReqContext);
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
    reqCtx = GetA2dpHpDeviceNotificationReqContext(m_SpeakerStreamReq);
    reqCtx->A2dpHpDevice = this; // weak ref.
    
    ntStatus = WdfMemoryCreatePreallocated(
        WDF_NO_OBJECT_ATTRIBUTES,
        &reqCtx->Buffer,
        sizeof(reqCtx->Buffer.StreamStatus),
        &reqCtx->MemIn);
    IF_FAILED_ACTION_JUMP(
        ntStatus,
        DPF(D_ERROR, ("%!FUNC!: WdfMemoryCreatePreallocated failed, 0x%x", ntStatus)),
        Done);
    
    ntStatus = WdfMemoryCreatePreallocated(
        WDF_NO_OBJECT_ATTRIBUTES,
        &reqCtx->Buffer,
        sizeof(reqCtx->Buffer.StreamStatus),
        &reqCtx->MemOut);
    IF_FAILED_ACTION_JUMP(
        ntStatus,
        DPF(D_ERROR, ("%!FUNC!: WdfMemoryCreatePreallocated failed, 0x%x", ntStatus)),
        Done);

    //
    // IOCTL_SBAUD_GET_SIOP_UPDATE
    //
    WDF_OBJECT_ATTRIBUTES_INIT_CONTEXT_TYPE(&attributes, A2dpHpDeviceNotificationReqContext);
    attributes.ParentObject = m_Adapter->GetWdfDevice();
    ntStatus = WdfRequestCreate(
        &attributes,
        m_WdfIoTarget,
        &m_SiopUpdateReq);
    IF_FAILED_ACTION_JUMP(
        ntStatus,
        DPF(D_ERROR, ("%!FUNC!: WdfRequestCreate(SIOP Update) failed, 0x%x", ntStatus)),
        Done);

    // Init context.
    reqCtx = GetA2dpHpDeviceNotificationReqContext(m_SiopUpdateReq);
    reqCtx->A2dpHpDevice = this;

    ntStatus = WdfMemoryCreatePreallocated(
        WDF_NO_OBJECT_ATTRIBUTES,
        &reqCtx->Buffer,
        sizeof(reqCtx->Buffer.SiopUpdate),
        &reqCtx->MemIn);
    IF_FAILED_ACTION_JUMP(
        ntStatus,
        DPF(D_ERROR, ("%!FUNC! WdfMemoryCreatePreallocated failed, 0x%x", ntStatus)),
        Done);

    ntStatus = WdfMemoryCreatePreallocated(
        WDF_NO_OBJECT_ATTRIBUTES,
        &reqCtx->Buffer,
        sizeof(reqCtx->Buffer.SiopUpdate),
        &reqCtx->MemOut);
    IF_FAILED_ACTION_JUMP(
        ntStatus,
        DPF(D_ERROR, ("%!FUNC! WdfMemoryCreatePreallocated failed, 0x%x", ntStatus)),
        Done);

    //
    // Create wave and filter names unique to this HFP device
    //
    ntStatus = CreateFilterNames(SymbolicLinkName);
    IF_FAILED_ACTION_JUMP(
        ntStatus,
        DPF(D_ERROR, ("%!FUNC!: Creating unique filter names for the A2DP device failed, 0x%x", ntStatus)),
        Done);

   //
   // This remote device is now in running state. No need to use interlock operations
   // b/c at this time this is the only thread accessing this info.
   //
   m_State = eA2dpHpStateRunning;
   
   //
   // Init successful.
   //
   ntStatus = STATUS_SUCCESS;

Done:
    return ntStatus;
}

//=============================================================================
#pragma code_seg("PAGE")
A2dpHpDevice::~A2dpHpDevice
( 
    void 
)
/*++

Routine Description:

  Destructor for A2dpHpDevice.

Arguments:

Return Value:

  void

--*/
{
    PAGED_CODE();
    DPF_ENTER(("[%!FUNC!]"));

    ASSERT(m_State != eA2dpHpStateRunning);
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

    if (m_Descriptor != NULL)
    {
        ExFreePoolWithTag(m_Descriptor, MINADAPTER_POOLTAG);
        m_Descriptor = NULL;
    }

    SAFE_DELETE_PTR_WITH_TAG(m_pSpeakerDescriptor, A2DPSIDEBANDTEST_POOLTAG02);

    SAFE_DELETE_PTR_WITH_TAG(m_pSpeakerSupportedFormatsIntersection, A2DPSIDEBANDTEST_POOLTAG03);

    RtlZeroMemory(&m_SpeakerTransportResources, sizeof(A2DPHPDEVICE_EP_TRANSPORT_RESOURCES));

    if (m_SpeakerVolumePropValues != NULL)
    {
        ExFreePoolWithTag(m_SpeakerVolumePropValues, A2DPSIDEBANDTEST_POOLTAG02);
        m_SpeakerVolumePropValues = NULL;
    }

    if (m_SpeakerMutePropValues != NULL)
    {
        ExFreePoolWithTag(m_SpeakerMutePropValues, A2DPSIDEBANDTEST_POOLTAG06);
        m_SpeakerMutePropValues = NULL;
    }

    if (m_ConnectedCodecCaps)
    {
        ExFreePoolWithTag(m_ConnectedCodecCaps, A2DPSIDEBANDTEST_POOLTAG07);
        m_ConnectedCodecCaps = NULL;
    }

    //
    // Free Irps.
    //
    if (m_SpeakerVolumeReq != NULL)
    {
        A2dpHpDeviceNotificationReqContext * ctx;

        // Delete the associated memory objects.
        ctx = GetA2dpHpDeviceNotificationReqContext(m_SpeakerVolumeReq);
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

    if (m_SpeakerMuteReq != NULL)
    {
        A2dpHpDeviceNotificationReqContext * ctx;

        // Delete the associated memory objects.
        ctx = GetA2dpHpDeviceNotificationReqContext(m_SpeakerMuteReq);
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

    if (m_ConnectionReq != NULL)
    {
        A2dpHpDeviceNotificationReqContext * ctx;

        // Delete the associated memory objects.
        ctx = GetA2dpHpDeviceNotificationReqContext(m_ConnectionReq);
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
    
    if (m_SpeakerStreamReq != NULL)
    {
        A2dpHpDeviceNotificationReqContext * ctx;

        // Delete the associated memory objects.
        ctx = GetA2dpHpDeviceNotificationReqContext(m_SpeakerStreamReq);
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

    if (m_SiopUpdateReq != NULL)
    {
        A2dpHpDeviceNotificationReqContext* ctx;

        // Delete the associated memory objects.
        ctx = GetA2dpHpDeviceNotificationReqContext(m_SiopUpdateReq);
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
        WdfObjectDelete(m_SiopUpdateReq);
        m_SiopUpdateReq = NULL;
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
    
    ASSERT(m_nSpeakerStreams == 0);

    ASSERT(m_SpeakerVolumeCallback.Handler == NULL);
    ASSERT(m_SpeakerMuteCallback.Handler == NULL);
    ASSERT(m_SpeakerConnectionStatusCallback.Handler == NULL);
    ASSERT(m_SpeakerFormatChangeCallback.Handler == NULL);
} // ~A2dpHpDevice  

//
// ISidebandDeviceCommon implementation. 
//

//=============================================================================
#pragma code_seg("PAGE")
BOOL
A2dpHpDevice::IsVolumeSupported
(
    _In_ eDeviceType deviceType
)
{
    PAGED_CODE();
    DPF_ENTER(("[%!FUNC!]"));
    
    if (deviceType == eA2dpHpSpeakerDevice)
    {
        return m_pSpeakerDescriptor->Capabilities.Volume;
    }

    return FALSE;
}

//=============================================================================
#pragma code_seg("PAGE")
PVOID
A2dpHpDevice::GetVolumeSettings
(
    _In_  eDeviceType deviceType,
    _Out_ PULONG    Size 
)
{
    PAGED_CODE();
    DPF_ENTER(("[%!FUNC!]"));

    ASSERT(Size != NULL);
    
    if (deviceType == eA2dpHpSpeakerDevice)
    {
        *Size = m_pSpeakerDescriptor->VolumePropertyValuesSize;
        return m_SpeakerVolumePropValues;
    }

    return NULL;
}

//=============================================================================
#pragma code_seg("PAGE")
NTSTATUS
A2dpHpDevice::GetVolume
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

    if (eA2dpHpSpeakerDevice == deviceType)
    {
        status = GetA2dpHpSpeakerVolume(Channel, pVolume);
        IF_FAILED_ACTION_JUMP(
            status,
            DPF(D_ERROR, ("Start: GetA2dpHpSpeakerVolume: failed, 0x%x", status)),
            Done);

        InterlockedExchange(&m_SpeakerVolumeLevel, *pVolume);
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
A2dpHpDevice::SetVolume
(
    _In_    eDeviceType     deviceType,
    _In_    LONG            Channel,
    _In_    LONG            Volume
)
{
    PAGED_CODE();
    DPF_ENTER(("[%!FUNC!]"));

    NTSTATUS status = STATUS_SUCCESS;

    if (eA2dpHpSpeakerDevice == deviceType)
    {
        status = SetA2dpHpSpeakerVolume(Channel, Volume);
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
A2dpHpDevice::IsMuteSupported
(
    _In_ eDeviceType deviceType
)
{
    PAGED_CODE();
    DPF_ENTER(("[%!FUNC!]"));
    
    if (deviceType == eA2dpHpSpeakerDevice)
    {
        return m_pSpeakerDescriptor->Capabilities.Mute;
    }

    return FALSE;
}

//=============================================================================
#pragma code_seg("PAGE")
PVOID
A2dpHpDevice::GetMuteSettings
(
    _In_  eDeviceType deviceType,
    _Out_ PULONG    Size 
)
{
    PAGED_CODE();
    DPF_ENTER(("[%!FUNC!]"));

    ASSERT(Size != NULL);
    
    if (deviceType == eA2dpHpSpeakerDevice)
    {
        *Size = m_pSpeakerDescriptor->MutePropertyValuesSize;
        return m_SpeakerMutePropValues;
    }

    return NULL;
}

//=============================================================================
#pragma code_seg("PAGE")
LONG
A2dpHpDevice::GetMute
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

    if (eA2dpHpSpeakerDevice == deviceType)
    {
        status = GetA2dpHpSpeakerMute(Channel, &ulMute);
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
A2dpHpDevice::SetMute
(
    _In_    eDeviceType     deviceType,
    _In_    LONG            Channel,
    _In_    LONG            Mute
)
{
    PAGED_CODE();
    DPF_ENTER(("[%!FUNC!]"));

    NTSTATUS status = STATUS_SUCCESS;

    if (eA2dpHpSpeakerDevice == deviceType)
    {
        status = SetA2dpHpSpeakerMute(Channel, Mute);
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
A2dpHpDevice::GetConnectionStatus()
{
    PAGED_CODE();
    DPF_ENTER(("[%!FUNC!]"));

    return (BOOL)InterlockedCompareExchange(&m_ConnectionStatusLong, 0, 0);
}

//=============================================================================
#pragma code_seg("PAGE")
NTSTATUS
A2dpHpDevice::GetA2dpHpCodecCaps
(
    _Outptr_ PA2DPHPDEVICE_CODEC_CAPABILITIES* ppCodecCaps
)
{
    PAGED_CODE();
    DPF_ENTER(("[A2dpHpDevice::GetA2dpHpCodecCaps]"));

    ASSERT(ppCodecCaps != NULL);

    NTSTATUS ntStatus = STATUS_SUCCESS;
    WDF_OBJECT_ATTRIBUTES attributes;
    WDF_REQUEST_REUSE_PARAMS reuseParams;
    WDFREQUEST req = NULL;
    ULONG_PTR information = 0;
    PBYTE pOutput = NULL;
    SIDEBANDAUDIO_SIOP_REQUEST_PARAM rp = { 0 };

    *ppCodecCaps = NULL;    
    rp.EpIndex = m_SpeakerEpIndex;
    rp.RequestedSiop.ParamSet   = SIDEBANDAUDIO_PARAMS_SET_A2DP;
    rp.RequestedSiop.TypeId     = SIDEANDAUDIO_PARAM_A2DP_CONFIGURED_CODEC;
    rp.RequestedSiop.Size       = 0;

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
    // Get the SIOP buffer size.
    //
    ntStatus = SendIoCtrlSynchronously(
        req,
        IOCTL_SBAUD_GET_SIOP,
        sizeof(rp),
        0,
        &rp,
        NULL);

    if (ntStatus == STATUS_BUFFER_TOO_SMALL)
    {
        information = WdfRequestGetInformation(req);
        if (information == 0 || information > ULONG_MAX)
        {
            ntStatus = STATUS_INVALID_DEVICE_STATE;
            DPF(D_ERROR, ("%!FUNC!: IOCTL_SBAUD_GET_SIOP invalid buffer size (%Id): 0x%x", information, ntStatus));
            goto Done;
        }

        rp.RequestedSiop.Size = (ULONG)information;
        ntStatus = STATUS_SUCCESS;
    }
    IF_FAILED_ACTION_JUMP(
        ntStatus,
        DPF(D_ERROR, ("GetA2dpHpCodecCaps: SendIoCtrlSynchronously(IOCTL_SBAUD_GET_SIOP, SIDEANDAUDIO_PARAM_A2DP_CONFIGURED_CODEC) failed, 0x%x", ntStatus)),
        Done);

    pOutput = (PBYTE)ExAllocatePool2(POOL_FLAG_NON_PAGED, rp.RequestedSiop.Size, A2DPSIDEBANDTEST_POOLTAG07);
    if (pOutput == NULL)
    {
        ntStatus = STATUS_INSUFFICIENT_RESOURCES;
    }
    IF_FAILED_ACTION_JUMP(
        ntStatus,
        DPF(D_ERROR, ("GetA2dpHpCodecCaps: out of memory")),
        Done);

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
        IOCTL_SBAUD_GET_SIOP,
        sizeof(rp),
        rp.RequestedSiop.Size,
        &rp,
        pOutput);
    IF_FAILED_ACTION_JUMP(
        ntStatus,
        DPF(D_ERROR, ("GetA2dpHpCodecCaps: SendIoCtrlSynchronously(IOCTL_SBAUD_GET_SIOP, SIDEANDAUDIO_PARAM_A2DP_CONFIGURED_CODEC) failed, 0x%x", ntStatus)),
        Done);

    //
    // All done.
    //
    *ppCodecCaps = (PA2DPHPDEVICE_CODEC_CAPABILITIES)pOutput;
Done:
    if (req != NULL)
    {
        WdfObjectDelete(req);
        req = NULL;
    }

    if (!NT_SUCCESS(ntStatus))
    {
        if (pOutput)
        {
            ExFreePoolWithTag(pOutput, A2DPSIDEBANDTEST_POOLTAG07);
            pOutput = NULL;
        }
    }

    return ntStatus;
}

#pragma code_seg("PAGE")
NTSTATUS
A2dpHpDevice::UpdateA2dpHpCodecCaps
(
    _In_ PA2DPHPDEVICE_CODEC_CAPABILITIES pCodecCaps
)
{
    PAGED_CODE();
    PA2DPHPDEVICE_CODEC_CAPABILITIES oldCodecCaps = NULL;
    BOOL capabilitiesChanged = FALSE;

    ExAcquireFastMutex(&m_ConnectedCodecLock);
    oldCodecCaps = m_ConnectedCodecCaps;
    m_ConnectedCodecCaps = pCodecCaps;
    //
    // Check if any codec configuration parameters changed.
    //
    if ((m_ConnectedCodecCaps != NULL) && (oldCodecCaps != NULL))
    {
        capabilitiesChanged = (RtlCompareMemory(oldCodecCaps, m_ConnectedCodecCaps, sizeof(*oldCodecCaps)) != sizeof(*oldCodecCaps));
    }
    ExReleaseFastMutex(&m_ConnectedCodecLock);

    if (capabilitiesChanged)
    {
        // Notify audio miniport about this change.
        if (m_SpeakerFormatChangeCallback.Handler != NULL)
        {
            m_SpeakerFormatChangeCallback.Handler(m_SpeakerFormatChangeCallback.Context);
        }
    }

    if (oldCodecCaps != NULL)
    {
        ExFreePoolWithTag(oldCodecCaps, A2DPSIDEBANDTEST_POOLTAG07);
        oldCodecCaps = NULL;
    }

    return STATUS_SUCCESS;
}

//=============================================================================
#pragma code_seg("PAGE")
NTSTATUS
A2dpHpDevice::Connect()
{
    PAGED_CODE();
    DPF_ENTER(("[%!FUNC!]"));

    return STATUS_DEVICE_FEATURE_NOT_SUPPORTED;
}

//=============================================================================
#pragma code_seg("PAGE")
NTSTATUS
A2dpHpDevice::Disconnect()
{
    PAGED_CODE();
    DPF_ENTER(("[%!FUNC!]"));

    return STATUS_DEVICE_FEATURE_NOT_SUPPORTED;
}

//=============================================================================
#pragma code_seg()
BOOL
A2dpHpDevice::GetStreamStatus(_In_ eDeviceType deviceType)
{
    DPF_ENTER(("[%!FUNC!]"));

    NTSTATUS ntStatus = STATUS_SUCCESS;

    if (deviceType == eA2dpHpSpeakerDevice)
    {
        ntStatus = (NTSTATUS)InterlockedCompareExchange(&m_SpeakerStreamStatusLong, 0, 0);
    }

    return NT_SUCCESS(ntStatus) ? TRUE : FALSE;
}

//=============================================================================
#pragma code_seg("PAGE")
NTSTATUS
A2dpHpDevice::SpeakerStreamOpen()
{
    PAGED_CODE();
    DPF_ENTER(("[%!FUNC!]"));
    NTSTATUS    ntStatus    = STATUS_SUCCESS;
    LONG        nStreams    = 0;

    ASSERT(m_nSpeakerStreams >= 0);
    
    nStreams = InterlockedIncrement(&m_nSpeakerStreams);
    if (nStreams == 1)
    {
        BOOLEAN  streamOpen = FALSE;
        
        ntStatus = SetA2dpHpStreamOpen(m_SpeakerEpIndex,
                                      &A2dpHpSpeakerSupportedDeviceFormats[m_SpeakerSelectedFormat],
                                      m_pSpeakerDescriptor);

        if (NT_SUCCESS(ntStatus))
        {
            streamOpen = TRUE;
            m_SpeakerStreamStatus = STATUS_SUCCESS;
            ntStatus = EnableA2dpHpSpeakerStreamStatusNotification();
        }

        //
        // Cleanup if any error.
        //
        if (!NT_SUCCESS(ntStatus))
        {
            nStreams = InterlockedDecrement(&m_nSpeakerStreams);
            ASSERT(nStreams == 0);
            UNREFERENCED_VAR(nStreams);

            RtlZeroMemory(&m_SpeakerTransportResources, sizeof(A2DPHPDEVICE_EP_TRANSPORT_RESOURCES));

            if (streamOpen)
            {
                SetA2dpHpStreamClose();
            }

            m_SpeakerStreamStatus = STATUS_INVALID_DEVICE_STATE;
        }
    }

    return ntStatus;
}

//=============================================================================
#pragma code_seg("PAGE")
NTSTATUS
A2dpHpDevice::StreamOpen(_In_ eDeviceType deviceType)
{
    PAGED_CODE();
    DPF_ENTER(("[%!FUNC!]"));

    if (deviceType == eA2dpHpSpeakerDevice)
    {
        return SpeakerStreamOpen();
    }

    return STATUS_INVALID_PARAMETER;
}

//=============================================================================
#pragma code_seg("PAGE")
NTSTATUS A2dpHpDevice::SpeakerStreamStart()
{
    PAGED_CODE();
    DPF_ENTER(("[%!FUNC!]"));

    NTSTATUS ntStatus = STATUS_SUCCESS;
    LONG nStreams = 0;

    ASSERT(m_nSpeakerStartedStreams >= 0);
    nStreams = InterlockedIncrement(&m_nSpeakerStartedStreams);

    if (nStreams == 1)
    {
        ntStatus = SetA2dpHpStreamStart();
    }

    return ntStatus;
}

//=============================================================================
#pragma code_seg("PAGE")
NTSTATUS
A2dpHpDevice::StreamStart(_In_ eDeviceType deviceType)
{
    PAGED_CODE();
    DPF_ENTER(("[%!FUNC!]"));

    if (deviceType == eA2dpHpSpeakerDevice)
    {
        return SpeakerStreamStart();
    }

    return STATUS_INVALID_PARAMETER;
}

//=============================================================================
#pragma code_seg("PAGE")
NTSTATUS
A2dpHpDevice::StreamSuspend(_In_ eDeviceType deviceType)
{
    PAGED_CODE();
    DPF_ENTER(("[%!FUNC!]"));
    
    if (deviceType == eA2dpHpSpeakerDevice)
    {
        return SpeakerStreamSuspend();
    }

    return STATUS_INVALID_PARAMETER;
}

//=============================================================================
#pragma code_seg("PAGE")
NTSTATUS A2dpHpDevice::SpeakerStreamSuspend()
{
    PAGED_CODE();
    NTSTATUS ntStatus = STATUS_SUCCESS;
    LONG nStreams = 0;
    ASSERT(m_nSpeakerStartedStreams > 0);
    nStreams = InterlockedDecrement(&m_nSpeakerStartedStreams);

    if (m_nSpeakerStartedStreams == 0)
    {
        ntStatus = SetA2dpHpStreamSuspend();
    }

    return ntStatus;
}

//=============================================================================
#pragma code_seg("PAGE")
NTSTATUS
A2dpHpDevice::SpeakerStreamClose()
{
    PAGED_CODE();
    DPF_ENTER(("[%!FUNC!]"));
    
    NTSTATUS    ntStatus    = STATUS_SUCCESS;
    LONG        nStreams    = 0;

    ASSERT(m_nSpeakerStreams > 0);
    
    nStreams = InterlockedDecrement(&m_nSpeakerStreams);
    if (nStreams == 0)
    {
        ntStatus = SetA2dpHpStreamClose();

        RtlZeroMemory(&m_SpeakerTransportResources, sizeof(A2DPHPDEVICE_EP_TRANSPORT_RESOURCES));
        
        StopA2dpHpSpeakerStreamStatusNotification();
        
        m_SpeakerStreamStatus = STATUS_INVALID_DEVICE_STATE;
    }

    return ntStatus;
}

//=============================================================================
#pragma code_seg("PAGE")
NTSTATUS
A2dpHpDevice::StreamClose(_In_ eDeviceType deviceType)
{
    PAGED_CODE();
    DPF_ENTER(("[%!FUNC!]"));
    
    if (deviceType == eA2dpHpSpeakerDevice)
    {
        return SpeakerStreamClose();
    }

    return STATUS_INVALID_PARAMETER;
}

//=============================================================================
#pragma code_seg("PAGE")
GUID
A2dpHpDevice::GetContainerId(_In_ eDeviceType deviceType)
{
    PAGED_CODE();
    DPF_ENTER(("[%!FUNC!]"));

    if (deviceType == eA2dpHpSpeakerDevice)
    {
        return m_pSpeakerDescriptor->ContainerId;
    }
    else
        return { 0 };
}

//=============================================================================
#pragma code_seg("PAGE")
VOID
A2dpHpDevice::SetVolumeHandler
(
    _In_        eDeviceType             deviceType,
    _In_opt_    PFNEVENTNOTIFICATION    EventHandler,
    _In_opt_    PVOID                   EventHandlerContext
)
{
    PAGED_CODE();
    DPF_ENTER(("[%!FUNC!]"));

    if (deviceType == eA2dpHpSpeakerDevice)
    {
        ASSERT(EventHandler == NULL || m_SpeakerVolumeCallback.Handler == NULL);

        m_SpeakerVolumeCallback.Handler = EventHandler; // weak ref.
        m_SpeakerVolumeCallback.Context = EventHandlerContext;
    }
}

//=============================================================================
#pragma code_seg("PAGE")
VOID
A2dpHpDevice::SetMuteHandler
(
    _In_        eDeviceType             deviceType,
    _In_opt_    PFNEVENTNOTIFICATION    EventHandler,
    _In_opt_    PVOID                   EventHandlerContext
)
{
    PAGED_CODE();
    DPF_ENTER(("[%!FUNC!]"));

    if (deviceType == eA2dpHpSpeakerDevice)
    {
        ASSERT(EventHandler == NULL || m_SpeakerMuteCallback.Handler == NULL);

        m_SpeakerMuteCallback.Handler = EventHandler; // weak ref.
        m_SpeakerMuteCallback.Context = EventHandlerContext;
    }
}

//=============================================================================
#pragma code_seg("PAGE")
VOID
A2dpHpDevice::SetConnectionStatusHandler
(
    _In_        eDeviceType             deviceType,
    _In_opt_    PFNEVENTNOTIFICATION    EventHandler,
    _In_opt_    PVOID                   EventHandlerContext
)
{
    PAGED_CODE();
    DPF_ENTER(("[%!FUNC!]"));

    if (deviceType == eA2dpHpSpeakerDevice)
    {
        ASSERT(EventHandler == NULL || m_SpeakerConnectionStatusCallback.Handler == NULL);

        m_SpeakerConnectionStatusCallback.Handler = EventHandler; // weak ref.
        m_SpeakerConnectionStatusCallback.Context = EventHandlerContext;
    }

}

//=============================================================================
#pragma code_seg("PAGE")
VOID
A2dpHpDevice::SetFormatChangeHandler
(
    _In_        eDeviceType             deviceType,
    _In_opt_    PFNEVENTNOTIFICATION    EventHandler,
    _In_opt_    PVOID                   EventHandlerContext
)
{
    PAGED_CODE();
    DPF_ENTER(("[%!FUNC!]"));

    //m_SpeakerFormatChangeCallback
    //m_MicFormatChangeCallback
    if (deviceType == eA2dpHpSpeakerDevice)
    {
        ASSERT(EventHandler == NULL || m_SpeakerFormatChangeCallback.Handler == NULL);

        m_SpeakerFormatChangeCallback.Handler = EventHandler; // weak ref.
        m_SpeakerFormatChangeCallback.Context = EventHandlerContext;
    }

}

//=============================================================================
#pragma code_seg("PAGE")
PPIN_DEVICE_FORMATS_AND_MODES
A2dpHpDevice::GetFormatsAndModes
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
A2dpHpDevice::IsNRECSupported()
{
    DPF_ENTER(("[%!FUNC!]"));

    return FALSE;
}

//=============================================================================
#pragma code_seg("PAGE")
BOOL
A2dpHpDevice::GetNRECDisableStatus()
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
A2dpHpDevice::SendIoCtrlAsynchronously
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

  This function aynchronously sends an I/O Ctrl request to the A2DP Headphone
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
A2dpHpDevice::SendIoCtrlSynchronously
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

  This function inits and synchronously sends an I/O Ctrl request to the A2DP Headphone
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

    reqOpts.Timeout = WDF_REL_TIMEOUT_IN_SEC(A2DP_SIDEBAND_SYNC_REQ_TIMEOUT_IN_SEC);
        
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
A2dpHpDevice::SendIoCtrlSynchronously
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

  This function inits and synchronously sends an I/O Ctrl request to the A2DP Headphone
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

    reqOpts.Timeout = WDF_REL_TIMEOUT_IN_SEC(A2DP_SIDEBAND_SYNC_REQ_TIMEOUT_IN_SEC);
        
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
A2dpHpDevice::EvtA2dpHpDeviceNotificationStatusWorkItem
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
    A2dpHpDevice      * This;
    KIRQL               oldIrql;
    
    if (WorkItem == NULL) 
    {
        return;
    }

    This = GetA2dpHpDeviceNotificationWorkItemContext(WorkItem)->A2dpHpDevice;
    ASSERT(This != NULL);

    for (;;)
    {
        BOOL                                    resend  = TRUE; 
        WDFREQUEST                              req     = NULL;
        A2dpHpDeviceNotificationReqContext    * reqCtx;
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
        
        reqCtx = GetA2dpHpDeviceNotificationReqContext(req);
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

                }
                break;

            case IOCTL_SBAUD_GET_CONNECTION_STATUS_UPDATE:
            {
                BOOL oldStatus = (BOOL)InterlockedExchange(&This->m_ConnectionStatusLong, (LONG)reqCtx->Buffer.ConnectionStatus.Connected);
                if (reqCtx->Buffer.ConnectionStatus.Connected != oldStatus)
                {
                    // Notify audio miniport about this change.
                    if (This->m_SpeakerConnectionStatusCallback.Handler != NULL)
                    {
                        This->m_SpeakerConnectionStatusCallback.Handler(
                            This->m_SpeakerConnectionStatusCallback.Context);
                    }
                }
            }
            break;

            case IOCTL_SBAUD_GET_SIOP_UPDATE:
            {
                if ((reqCtx->Buffer.SiopUpdate.EpIndex == This->m_SpeakerEpIndex) &&
                    (reqCtx->Buffer.SiopUpdate.RequestedSiop.ParamSet == SIDEBANDAUDIO_PARAMS_SET_A2DP) &&
                    (reqCtx->Buffer.SiopUpdate.RequestedSiop.TypeId == SIDEANDAUDIO_PARAM_A2DP_CONFIGURED_CODEC))
                {
                    PA2DPHPDEVICE_CODEC_CAPABILITIES codecCaps = NULL;
                    ntStatus = This->GetA2dpHpCodecCaps(&codecCaps);
                    if (NT_SUCCESS(ntStatus))
                    {
                        ntStatus = This->UpdateA2dpHpCodecCaps(codecCaps);
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
                EvtA2dpHpDeviceNotificationStatusCompletion,
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
A2dpHpDevice::EvtA2dpHpDeviceNotificationStatusCompletion
(
  _In_  WDFREQUEST Request,
  _In_  WDFIOTARGET Target,
  _In_  PWDF_REQUEST_COMPLETION_PARAMS Params,
  _In_  WDFCONTEXT Context
)
{
    DPF_ENTER(("[%!FUNC!]"));
   
    NTSTATUS                            ntStatus    = STATUS_SUCCESS;
    A2dpHpDeviceNotificationReqContext   *ctx        = NULL;
    A2dpHpDevice                         *This       = NULL;
    KIRQL                               oldIrql;
    
    UNREFERENCED_PARAMETER(Target);
    UNREFERENCED_PARAMETER(Context);
    
    ctx = GetA2dpHpDeviceNotificationReqContext(Request);
    This = ctx->A2dpHpDevice;
    ASSERT(This != NULL);

    ntStatus = Params->IoStatus.Status;
    if (ntStatus == STATUS_CANCELLED)
    {
        // A2DP device is shutting down. Do not re-send this request.
        goto Done;
    }

    //
    // If something is wrong with the A2DP interface, do not loop forever.
    //
    if (!NT_SUCCESS(ntStatus))
    {
        if (++ctx->Errors > A2DP_SIDEBAND_NOTIFICATION_MAX_ERROR_COUNT)
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
A2dpHpDevice::SetA2dpHpDeviceCapabilities
(
)
/*++

Routine Description:

This function synchronously informs the Audio Driver capabilities to A2DP stack.

--*/
{
    PAGED_CODE();
    DPF_ENTER(("[A2dpHpDevice::SetA2dpHpDeviceCapabilities]"));

    NTSTATUS                            ntStatus = STATUS_SUCCESS;

    typedef struct _SidebandDeviceSupportedCodecs
    {
        A2DP_SBC_MEDIA_HEADER SupportedSbcCodec;
        A2DP_AAC_MEDIA_HEADER SupportedAacCodec;
    } SidebandDeviceSupportedCodecs, * PSidebandDeviceSupportedCodecs;

    typedef struct _SidebandDeviceCapsInputParams
    {
        SIDEBANDAUDIO_SIOP_REQUEST_PARAM DeviceCapsHeader;
        SidebandDeviceSupportedCodecs SupportedCodecs;
    }SidebandDeviceCapsInputParams, * PSidebandDeviceCapsInputParams;

    SidebandDeviceCapsInputParams deviceCapsParams = { 0 };
    deviceCapsParams.DeviceCapsHeader.RequestedSiop = { SIDEBANDAUDIO_PARAMS_SET_A2DP, SIDEBANDAUDIO_PARAM_A2DP_CODECS, sizeof(deviceCapsParams.SupportedCodecs) };
    deviceCapsParams.DeviceCapsHeader.EpIndex = 0;
    //
    // Set SBC codec settings.
    //
    deviceCapsParams.SupportedCodecs.SupportedSbcCodec.CategoryCodecHeader.SvcCategory = 0x7; // Service category is media codec.
    deviceCapsParams.SupportedCodecs.SupportedSbcCodec.CategoryCodecHeader.Losc = A2DP_SBC_MEDIA_HEADER::CorrectLosc();
    deviceCapsParams.SupportedCodecs.SupportedSbcCodec.CodecType = 0x0; // Codec type is SBC
    deviceCapsParams.SupportedCodecs.SupportedSbcCodec.MediaType = 0x0; // Media type is audio
    deviceCapsParams.SupportedCodecs.SupportedSbcCodec.ChannelMode = 0x1; // Channel Mode is joint stereo
    deviceCapsParams.SupportedCodecs.SupportedSbcCodec.SamplingFrequency = 0x3; // 48 and 44.1 kHz
    deviceCapsParams.SupportedCodecs.SupportedSbcCodec.AllocMethod = 0x1; // Allocation method is loudness
    deviceCapsParams.SupportedCodecs.SupportedSbcCodec.Subbands = 0x1; // 8 subbands
    deviceCapsParams.SupportedCodecs.SupportedSbcCodec.BlockLength = 0x1; 
    deviceCapsParams.SupportedCodecs.SupportedSbcCodec.MinBitpoolVal = 0x2;
    deviceCapsParams.SupportedCodecs.SupportedSbcCodec.MaxBitpoolVal = 0x53;

    //
    // Set AAC codec settings.
    //
    deviceCapsParams.SupportedCodecs.SupportedAacCodec.CategoryCodecHeader.SvcCategory = 0x7; // Service category is media codec.
    deviceCapsParams.SupportedCodecs.SupportedAacCodec.CategoryCodecHeader.Losc = A2DP_AAC_MEDIA_HEADER::CorrectLosc();
    deviceCapsParams.SupportedCodecs.SupportedAacCodec.MediaType = 0x0;     // Media type is audio.
    deviceCapsParams.SupportedCodecs.SupportedAacCodec.CodecType = 0x2;     // Codec type is MPEG 2 AAC
    deviceCapsParams.SupportedCodecs.SupportedAacCodec.ObjectType = 0x80;   // Object type is AAC MPEG 2 LC
    deviceCapsParams.SupportedCodecs.SupportedAacCodec.SamplingFrequencyLower4Bits = 0x8; // Sampling frequency is 48kHz.
    deviceCapsParams.SupportedCodecs.SupportedAacCodec.Channels = 0x1; // Dual channel.
    deviceCapsParams.SupportedCodecs.SupportedAacCodec.VariableBitRate = 1; // Vbr is enabled.
    // Set bitrate to 256kbps
    deviceCapsParams.SupportedCodecs.SupportedAacCodec.BitRateUpper7Bits = 0x3;
    deviceCapsParams.SupportedCodecs.SupportedAacCodec.BitRateMiddle8Bits = 0xe8;
    deviceCapsParams.SupportedCodecs.SupportedAacCodec.BitRateLower8Bits = 0;

    //
    // Set the audio codec capabilities SIOP.
    //
    ntStatus = SendIoCtrlSynchronously(
        NULL,
        IOCTL_SBAUD_SET_SIOP,
        sizeof(deviceCapsParams),
        0,
        &deviceCapsParams);

    IF_FAILED_ACTION_JUMP(
        ntStatus,
        DPF(D_ERROR, ("SetA2dpHpDeviceCapabilities: SendIoCtrlSynchronously(IOCTL_SBAUD_SET_SIOP) failed, 0x%x", ntStatus)),
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
A2dpHpDevice::GetA2dpHpDescriptor
(
    _Out_ PSIDEBANDAUDIO_DEVICE_DESCRIPTOR *Descriptor
)
/*++

Routine Description:

  This function synchronously gets the remote A2DP Headphone Device descriptor.

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
    // Get the A2DP Headphone descriptor.
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
A2dpHpDevice::GetA2dpHpEndpointDescriptor
(
    _In_    ULONG                               EpIndex,
    _Out_ PSIDEBANDAUDIO_ENDPOINT_DESCRIPTOR2  *EpDescriptor
)
/*++

Routine Description:

  This function synchronously gets the A2DP Headphone Endpoint Descriptor.

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
    descriptor  = (PSIDEBANDAUDIO_ENDPOINT_DESCRIPTOR2) ExAllocatePool2(POOL_FLAG_NON_PAGED, length, MINADAPTER_POOLTAG);
    if (descriptor == NULL)
    {
        ntStatus = STATUS_INSUFFICIENT_RESOURCES;
    }
    IF_FAILED_ACTION_JUMP(
        ntStatus,
        DPF(D_ERROR, ("%!FUNC!: ExAllocatePool2 failed, out of memory")),
        Done);

    //
    // Get the A2DP Headphone descriptor.
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
A2dpHpDevice::VerifyEndpointFormatCompatibility
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

    // Verify here whether hardware/dsp mixer can support the A2DP device formats
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


//=============================================================================
#pragma code_seg("PAGE")
NTSTATUS A2dpHpDevice::EnableA2dpHpConnectionStatusNotification()
{
    DPF_ENTER(("[%!FUNC!]"));

    NTSTATUS                                ntStatus = STATUS_SUCCESS;
    A2dpHpDeviceNotificationReqContext* ctx = NULL;

    ctx = GetA2dpHpDeviceNotificationReqContext(m_ConnectionReq);

    //
    // This is a notification request.
    //
    ctx->Buffer.ConnectionStatus.Immediate = FALSE;
    ctx->Buffer.ConnectionStatus.EpIndex = m_SpeakerEpIndex;
    ctx->Buffer.ConnectionStatus.Connected = FALSE;

    //
    // Register for connection status updates.
    //
    ntStatus = SendIoCtrlAsynchronously(
        m_ConnectionReq,
        IOCTL_SBAUD_GET_CONNECTION_STATUS_UPDATE,
        ctx->MemIn,
        ctx->MemOut,
        EvtA2dpHpDeviceNotificationStatusCompletion,
        this);

    IF_FAILED_ACTION_JUMP(
        ntStatus,
        DPF(D_ERROR, ("%!FUNC!: SendIoCtrlAsynchronously(IOCTL_SBAUD_GET_CONNECTION_STATUS_UPDATE) failed, 0x%x", ntStatus)),
        Done);

    //
    // All done.
    //
    ntStatus = STATUS_SUCCESS;

Done:

    return ntStatus;
}

NTSTATUS
A2dpHpDevice::GetA2dpHpEndpointFormatsIntersection
(
    _In_    PSIDEBANDAUDIO_SUPPORTED_FORMATS    pDeviceFormats,
    _Out_   PSIDEBANDAUDIO_SUPPORTED_FORMATS    *ppSupportedFormatsIntersection
)
/*++

Routine Description:

  This function synchronously gets the A2DP Headphone supported formats for
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
    formats  = (PSIDEBANDAUDIO_SUPPORTED_FORMATS) ExAllocatePool2(POOL_FLAG_NON_PAGED, length, A2DPSIDEBANDTEST_POOLTAG03);
    if (formats == NULL)
    {
        ntStatus = STATUS_INSUFFICIENT_RESOURCES;
    }
    IF_FAILED_ACTION_JUMP(
        ntStatus,
        DPF(D_ERROR, ("%!FUNC!: ExAllocatePool2 failed, out of memory")),
        Done);

    //
    // Get the A2DP Headphone descriptor.
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
            ExFreePoolWithTag(formats, A2DPSIDEBANDTEST_POOLTAG03);
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
A2dpHpDevice::GetA2dpHpVolumePropertyValues
(
    _In_  ULONG                     EpIndex,
    _In_  ULONG                     Length,
    _Out_ PKSPROPERTY_DESCRIPTION   *PropValues
)
/*++

Routine Description:

  This function synchronously gets the remote A2DP Headphone volume values.

--*/
{
    PAGED_CODE();
    DPF_ENTER(("[%!FUNC!]"));
    
    NTSTATUS                    ntStatus    = STATUS_SUCCESS;
    PKSPROPERTY_DESCRIPTION     propValues  = NULL;
    
    *PropValues = NULL;

    //
    // Allocate memory.
    //
    propValues  = (PKSPROPERTY_DESCRIPTION) ExAllocatePool2(POOL_FLAG_NON_PAGED, Length, A2DPSIDEBANDTEST_POOLTAG02);
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
    *((ULONG *)propValues) = EpIndex;

    //
    // Get the A2DP Headphone volume property values.
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
A2dpHpDevice::SetA2dpHpSpeakerVolume
(
    _In_ LONG   Channel,
    _In_ LONG   Volume  
)
/*++

Routine Description:

  This function synchronously sets the remote A2DP Headphone speaker volume.

--*/
{
    PAGED_CODE();
    DPF_ENTER(("[%!FUNC!]"));
    
    NTSTATUS                        ntStatus    = STATUS_SUCCESS;
    SIDEBANDAUDIO_VOLUME_PARAMS  vp;
    
    //
    // Set the A2DP Headphone speaker volume.
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
A2dpHpDevice::GetA2dpHpSpeakerVolume
(
    _In_  LONG  Channel,
    _Out_ LONG  * Volume    
)
/*++

Routine Description:

  This function synchronously gets the remote A2DP Headphone speaker volume.

--*/
{
    PAGED_CODE();
    DPF_ENTER(("[%!FUNC!]"));
    
    NTSTATUS                        ntStatus    = STATUS_SUCCESS;
    A2dpHpDeviceNotificationBuffer   buffer      = {0};

    *Volume = 0;
    
    buffer.Volume.Immediate = TRUE;
    buffer.Volume.EpIndex = m_SpeakerEpIndex;
    buffer.Volume.Channel = Channel;
    buffer.Volume.Value = 0;
    
    //
    // Get the A2DP Headphone speaker volume.
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
A2dpHpDevice::EnableA2dpHpSpeakerVolumeStatusNotification()
/*++

Routine Description:

  This function registers for A2DP Headphone speaker 
  volume change notification. 

--*/
{
    DPF_ENTER(("[%!FUNC!]"));
    
    NTSTATUS                                ntStatus    = STATUS_SUCCESS;
    A2dpHpDeviceNotificationReqContext       *ctx         = NULL;

    ctx = GetA2dpHpDeviceNotificationReqContext(m_SpeakerVolumeReq);

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
        EvtA2dpHpDeviceNotificationStatusCompletion,
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
A2dpHpDevice::GetA2dpHpMutePropertyValues
(
    _In_  ULONG                     EpIndex,
    _In_  ULONG                     Length,
    _Out_ PKSPROPERTY_DESCRIPTION   *PropValues
)
/*++

Routine Description:

  This function synchronously gets the remote A2DP Headphone mute values.

--*/
{
    PAGED_CODE();
    DPF_ENTER(("[%!FUNC!]"));

    NTSTATUS                    ntStatus = STATUS_SUCCESS;
    PKSPROPERTY_DESCRIPTION     propValues = NULL;
    *PropValues = NULL;

    //
    // Allocate memory
    //
    propValues = (PKSPROPERTY_DESCRIPTION) ExAllocatePool2(POOL_FLAG_NON_PAGED, Length, A2DPSIDEBANDTEST_POOLTAG06);
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
    // Get the A2DP Headphone volume property values.
    //
    ntStatus = SendIoCtrlSynchronously(
        NULL,
        IOCTL_SBAUD_GET_MUTEPROPERTYVALUES,
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
            ExFreePoolWithTag(propValues, A2DPSIDEBANDTEST_POOLTAG06);
        }   
    }
    
    return ntStatus;
}

//=============================================================================
#pragma code_seg("PAGE")
NTSTATUS 
A2dpHpDevice::SetA2dpHpSpeakerMute
(
    _In_ LONG   Channel,
    _In_ LONG   Mute
)
/*++

Routine Description:

  This function synchronously sets the remote A2DP Headphone speaker mute.

--*/
{
    PAGED_CODE();
    DPF_ENTER(("[%!FUNC!]"));
    
    NTSTATUS                        ntStatus    = STATUS_SUCCESS;
    SIDEBANDAUDIO_MUTE_PARAMS       mp;
    
    //
    // Set the A2DP Headphone speaker mute.
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
A2dpHpDevice::GetA2dpHpSpeakerMute
(
    _In_  LONG  Channel,
    _Out_ LONG  *Mute
)
/*++

Routine Description:

  This function synchronously gets the remote A2DP Headphone speaker mute.

--*/
{
    PAGED_CODE();
    DPF_ENTER(("[%!FUNC!]"));
    
    NTSTATUS                        ntStatus    = STATUS_SUCCESS;
    A2dpHpDeviceNotificationBuffer   buffer      = {0};

    *Mute = 0;
    
    buffer.Mute.Immediate = TRUE;
    buffer.Mute.EpIndex = m_SpeakerEpIndex;
    buffer.Mute.Channel = Channel;
    buffer.Mute.Value = 0;
    
    //
    // Get the A2DP Headphone speaker mute.
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
A2dpHpDevice::EnableA2dpHpSpeakerMuteStatusNotification()
/*++

Routine Description:

  This function registers for A2DP Headphone speaker 
  mute change notification. 

--*/
{
    DPF_ENTER(("[%!FUNC!]"));
    
    NTSTATUS                                ntStatus    = STATUS_SUCCESS;
    A2dpHpDeviceNotificationReqContext       *ctx         = NULL;

    ctx = GetA2dpHpDeviceNotificationReqContext(m_SpeakerMuteReq);

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
        EvtA2dpHpDeviceNotificationStatusCompletion,
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

#pragma code_seg("PAGE")
NTSTATUS A2dpHpDevice::GetA2dpHpConnectionStatus(BOOL* ConnectionStatus)
{
    PAGED_CODE();
    DPF_ENTER(("[%!FUNC!]"));

    NTSTATUS                        ntStatus = STATUS_SUCCESS;
    A2dpHpDeviceNotificationBuffer   buffer = { 0 };

    *ConnectionStatus = FALSE;

    buffer.ConnectionStatus.Immediate = TRUE;
    buffer.ConnectionStatus.EpIndex = m_SpeakerEpIndex;

    //
    // Get the A2DP connection status.
    //
    ntStatus = SendIoCtrlSynchronously(
        NULL,
        IOCTL_SBAUD_GET_CONNECTION_STATUS_UPDATE,
        sizeof(buffer.ConnectionStatus),
        sizeof(buffer.ConnectionStatus),
        &buffer);

    IF_FAILED_ACTION_JUMP(
        ntStatus,
        DPF(D_ERROR, ("%!FUNC!: SendIoCtrlSynchronously(IOCTL_SBAUD_GET_CONNECTION_STATUS_UPDATE) failed, 0x%x", ntStatus)),
        Done);

    //
    // All done.
    //
    *ConnectionStatus = buffer.ConnectionStatus.Connected;

Done:

    return ntStatus;
}

//=============================================================================
#pragma code_seg("PAGE")
NTSTATUS 
A2dpHpDevice::SetSidebandClaimed(_In_ BOOL bClaimed)
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
void 
A2dpHpDevice::EvtA2dpHpDeviceStreamStatusCompletion
(
  _In_  WDFREQUEST Request,
  _In_  WDFIOTARGET Target,
  _In_  PWDF_REQUEST_COMPLETION_PARAMS Params,
  _In_  WDFCONTEXT Context
)
/*++

Routine Description:

  Completion callback for the A2DP Headphone stream status notification. 

--*/
{
    DPF_ENTER(("[%!FUNC!]"));
   
    NTSTATUS                             ntStatus   = STATUS_SUCCESS;
    A2dpHpDeviceNotificationReqContext    *reqCtx    = NULL;
    A2dpHpDevice                          *This      = NULL;
    NTSTATUS                             ntResult   = STATUS_SUCCESS;
    
    UNREFERENCED_PARAMETER(Target);
    UNREFERENCED_PARAMETER(Context);

    //
    // Get the A2DP stream status.
    //
    reqCtx = GetA2dpHpDeviceNotificationReqContext(Request);
    This = reqCtx->A2dpHpDevice;
    ASSERT(This != NULL);

    ntStatus = Params->IoStatus.Status;
    if (!NT_SUCCESS(ntStatus))
    {
        ntResult = STATUS_INVALID_DEVICE_STATE;
    }
    else 
    {
        ntResult = reqCtx->Buffer.StreamStatus.Status;
    }

    if (reqCtx->Buffer.StreamStatus.EpIndex == This->m_SpeakerEpIndex)
    {
        InterlockedExchange(&This->m_SpeakerStreamStatusLong, (LONG)ntResult);

        //
        // Let the stop routine know we are done. Stop routine will 
        // re-init the request.
        //
        KeSetEvent(&This->m_SpeakerStreamStatusEvent, IO_NO_INCREMENT, FALSE);
    }
}

//=============================================================================
#pragma code_seg()
NTSTATUS 
A2dpHpDevice::EnableA2dpHpSpeakerStreamStatusNotification()
/*++

Routine Description:

  This function registers for A2DP Headphone stream status notification. 

--*/
{
    DPF_ENTER(("[%!FUNC!]"));
    
    NTSTATUS                            ntStatus    = STATUS_SUCCESS;
    A2dpHpDeviceNotificationReqContext   *ctx        = NULL;

    ASSERT(m_nSpeakerStreams > 0);
    
    ctx = GetA2dpHpDeviceNotificationReqContext(m_SpeakerStreamReq);
    ctx->Buffer.StreamStatus.Immediate = FALSE; 
    ctx->Buffer.StreamStatus.EpIndex = m_SpeakerEpIndex;

    KeClearEvent(&m_SpeakerStreamStatusEvent);
    
    //
    // Get the A2DP Headphone speaker stream status.
    //
    ntStatus = SendIoCtrlAsynchronously(
        m_SpeakerStreamReq,
        IOCTL_SBAUD_GET_STREAM_STATUS_UPDATE,
        ctx->MemIn,
        ctx->MemOut,
        EvtA2dpHpDeviceStreamStatusCompletion,
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

NTSTATUS
A2dpHpDevice::EnableA2dpHpSpeakerSiopNotification()
/*++

Routine Description:

    This function registers for A2DP Headphone updates for the configured codec SIOP.

--*/
{
    DPF_ENTER(("[%!FUNC!]"));

    NTSTATUS                            ntStatus    = STATUS_SUCCESS;
    A2dpHpDeviceNotificationReqContext* ctx         = NULL;

    ctx = GetA2dpHpDeviceNotificationReqContext(m_SiopUpdateReq);
    ctx->Buffer.SiopUpdate.EpIndex = m_SpeakerEpIndex;
    ctx->Buffer.SiopUpdate.RequestedSiop.ParamSet = SIDEBANDAUDIO_PARAMS_SET_A2DP;
    ctx->Buffer.SiopUpdate.RequestedSiop.Size = 0;
    ctx->Buffer.SiopUpdate.RequestedSiop.TypeId = SIDEANDAUDIO_PARAM_A2DP_CONFIGURED_CODEC;

    //
    // Register for updates to the A2DP configured codec SIOP.
    //
    ntStatus = SendIoCtrlAsynchronously(
        m_SiopUpdateReq,
        IOCTL_SBAUD_GET_SIOP_UPDATE,
        ctx->MemIn,
        ctx->MemOut,
        EvtA2dpHpDeviceNotificationStatusCompletion,
        this);

    IF_FAILED_ACTION_JUMP(
        ntStatus,
        DPF(D_ERROR, ("%!FUNC!: SendIoCtrlAsynchronously(IOCTL_SBAUD_GET_SIOP_UPDATE) failed, 0x%x", ntStatus)),
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
A2dpHpDevice::StopA2dpHpSpeakerStreamStatusNotification()
/*++

Routine Description:

  This function stops the A2DP Headphone speaker stream status notification.
  The function waits for the request to be done before returning.

--*/
{
    PAGED_CODE();
    DPF_ENTER(("[%!FUNC!]"));
    
    NTSTATUS                            ntStatus        = STATUS_SUCCESS;
    A2dpHpDeviceNotificationReqContext   *reqCtx         = NULL;
    WDF_REQUEST_REUSE_PARAMS            reuseParams;  
   
    WdfRequestCancelSentRequest(m_SpeakerStreamReq);
    KeWaitForSingleObject(&m_SpeakerStreamStatusEvent, Executive, KernelMode, FALSE, NULL);

    reqCtx = GetA2dpHpDeviceNotificationReqContext(m_SpeakerStreamReq);
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
A2dpHpDevice::SetA2dpHpStreamOpen
(
    ULONG EpIndex,
    PKSDATAFORMAT_WAVEFORMATEXTENSIBLE  pStreamFormat,
    PSIDEBANDAUDIO_ENDPOINT_DESCRIPTOR2 pEndpointDescriptor
)
/*++

Routine Description:

This function synchronously requests an open ISOCH channel to transmit audio
data over transport.

--*/
{
    PAGED_CODE();
    DPF_ENTER(("[A2dpHpDevice::SetA2dpHpStreamOpen]"));

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
        DPF(D_VERBOSE, ("SetA2dpHpStreamOpen: the stream channel is already open"));
        ntStatus = STATUS_SUCCESS;
    }

    IF_FAILED_ACTION_JUMP(
        ntStatus,
        DPF(D_ERROR, ("SetA2dpHpStreamOpen: SendIoCtrlSynchronously(IOCTL_SBAUD_STREAM_OPEN) failed, 0x%x", ntStatus)),
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
NTSTATUS A2dpHpDevice::SetA2dpHpStreamStart()
{
    PAGED_CODE();
    DPF_ENTER(("[%!FUNC!]"));
    NTSTATUS ntStatus = STATUS_SUCCESS;

    ntStatus = SendIoCtrlSynchronously(
        NULL,
        IOCTL_SBAUD_STREAM_START,
        sizeof(m_SpeakerEpIndex),
        0,
        &m_SpeakerEpIndex);

    if (ntStatus == STATUS_DEVICE_BUSY)
    {
        DPF(D_VERBOSE, ("%!FUNC!: the stream channel is already started"));
        ntStatus = STATUS_SUCCESS;
    }

    IF_FAILED_ACTION_JUMP(
        ntStatus,
        DPF(D_ERROR, ("%!FUNC!: SendIoCtrlSynchronously(IOCTL_SBAUD_STREAM_START) failes, 0x%x", ntStatus)),
        Done);

    ntStatus = STATUS_SUCCESS;

Done:
    return ntStatus;
}

//=============================================================================
#pragma code_seg("PAGE")
NTSTATUS A2dpHpDevice::SetA2dpHpStreamSuspend()
{
    PAGED_CODE();

    DPF_ENTER(("[%!FUNC!]"));
    NTSTATUS ntStatus = STATUS_SUCCESS;

    ntStatus = SendIoCtrlSynchronously(
        NULL,
        IOCTL_SBAUD_STREAM_SUSPEND,
        sizeof(m_SpeakerEpIndex),
        0,
        &m_SpeakerEpIndex);

    if (ntStatus == STATUS_DEVICE_BUSY)
    {
        DPF(D_VERBOSE, ("%!FUNC!: the stream channel is already suspended"));
        ntStatus = STATUS_SUCCESS;
    }

    IF_FAILED_ACTION_JUMP(
        ntStatus,
        DPF(D_ERROR, ("%!FUNC!: SendIoCtrlSynchronously(IOCTL_SBAUD_STREAM_SUSPEND) failes, 0x%x", ntStatus)),
        Done);

    ntStatus = STATUS_SUCCESS;

Done:
    return ntStatus;
}

//=============================================================================
#pragma code_seg("PAGE")
NTSTATUS 
A2dpHpDevice::SetA2dpHpStreamClose()
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
        sizeof(m_SpeakerEpIndex),
        0,
        &m_SpeakerEpIndex);

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
A2dpHpDevice::Start()
/*++

Routine Description:

  Asynchronously called to start the audio device.

--*/
{
    PAGED_CODE();
    DPF_ENTER(("[%!FUNC!]"));

    NTSTATUS            ntStatus            = STATUS_SUCCESS;
    ULONG               speakerEndpoints    = 0;

    // Indicate audio driver capabilities indicating support for 16KHz sampling rate
    ntStatus = SetA2dpHpDeviceCapabilities();
    IF_FAILED_ACTION_JUMP(
        ntStatus,
        DPF(D_ERROR, ("Start: SetA2dpHpDeviceCapabilities: failed to set Wideband Support, 0x%x", ntStatus)),
        Done);

    //
    // Get A2DP descriptor
    //
    ntStatus = GetA2dpHpDescriptor(&m_Descriptor);
    IF_FAILED_ACTION_JUMP(
        ntStatus,
        DPF(D_ERROR, ("%!FUNC!: GetA2dpHpDescriptor: failed to retrieve A2DP Headphone Descriptor, 0x%x", ntStatus)),
        Done);

    for (ULONG i = 0; i < m_Descriptor->NumberOfEndpoints; i++)
    {
        PSIDEBANDAUDIO_ENDPOINT_DESCRIPTOR2 ed;
        ntStatus = GetA2dpHpEndpointDescriptor(i, &ed);
        IF_FAILED_ACTION_JUMP(
            ntStatus,
            DPF(D_ERROR, ("%!FUNC!: GetA2dpHpEndpointDescriptor: failed to retrieve A2DP Headphone Endpoint %d Descriptor, 0x%x", i, ntStatus)),
            Done);

        if (ed->Direction == KSPIN_DATAFLOW_OUT)
        {
            speakerEndpoints++;
            m_SpeakerEpIndex = i;
            m_pSpeakerDescriptor = ed;
        }
    }

    if (speakerEndpoints > 1)
    {
        ntStatus = STATUS_DEVICE_FEATURE_NOT_SUPPORTED;
        m_SpeakerEpIndex = 0;
    }
    IF_FAILED_ACTION_JUMP(
        ntStatus,
        DPF(D_ERROR, ("%!FUNC!: Number of endpoints on A2DP device not supported, 0x%x", ntStatus)),
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
            ntStatus = GetA2dpHpVolumePropertyValues(
                m_SpeakerEpIndex,
                m_pSpeakerDescriptor->VolumePropertyValuesSize,
                &volumePropValues);

            IF_FAILED_ACTION_JUMP(
                ntStatus,
                DPF(D_ERROR, ("%!FUNC!: GetA2dpHpVolumePropertyValues: failed to retrieve KSPROPERTY_VALUES, 0x%x", ntStatus)),
                Done);

            m_SpeakerVolumePropValues = volumePropValues;

            // Speaker volume.
            ntStatus = GetA2dpHpSpeakerVolume(0, &volume);
            IF_FAILED_ACTION_JUMP(
                ntStatus,
                DPF(D_ERROR, ("%!FUNC!: GetA2dpHpSpeakerVolume: failed, 0x%x", ntStatus)),
                Done);

            m_SpeakerVolumeLevel = volume;
        }

        if (m_pSpeakerDescriptor->Capabilities.Mute)
        {
            PKSPROPERTY_DESCRIPTION     mutePropValues = NULL;
            LONG                        mute = 0;

            // Mute settings.
            ntStatus = GetA2dpHpMutePropertyValues(
                m_SpeakerEpIndex,
                m_pSpeakerDescriptor->MutePropertyValuesSize,
                &mutePropValues);

            IF_FAILED_ACTION_JUMP(
                ntStatus,
                DPF(D_ERROR, ("%!FUNC!: GetA2dpHpMutePropertyValues: failed to retrieve KSPROPERTY_DESCRIPTION, 0x%x", ntStatus)),
                Done);

            m_SpeakerMutePropValues         = mutePropValues;

            // Speaker mute.
            ntStatus = GetA2dpHpSpeakerMute(0, &mute);
            IF_FAILED_ACTION_JUMP(
                ntStatus,
                DPF(D_ERROR, ("%!FUNC!: GetA2dpHpSpeakerMute: failed, 0x%x", ntStatus)),
                Done);

            m_SpeakerMute = mute;
        }

        // Speaker Formats
        PKSDATAFORMAT speakerFormatsArray[SIZEOF_ARRAY(A2dpHpSpeakerSupportedDeviceFormats)];
        for (ULONG i = 0; i < SIZEOF_ARRAY(A2dpHpSpeakerSupportedDeviceFormats); i++)
        {
            speakerFormatsArray[i] = (PKSDATAFORMAT)(&A2dpHpSpeakerSupportedDeviceFormats[i]);
        }
        SIDEBANDAUDIO_SUPPORTED_FORMATS speakerDeviceFormats =
        {
            sizeof(SIDEBANDAUDIO_SUPPORTED_FORMATS) + SIZEOF_ARRAY(speakerFormatsArray)*sizeof(*speakerFormatsArray),
            m_SpeakerEpIndex,
            SIZEOF_ARRAY(speakerFormatsArray),
            speakerFormatsArray
        };

        m_pSpeakerSupportedFormatsIntersection = (PSIDEBANDAUDIO_SUPPORTED_FORMATS)ExAllocatePool2(POOL_FLAG_NON_PAGED, speakerDeviceFormats.CbSize, SYSVAD_POOLTAG);
        RtlCopyMemory(m_pSpeakerSupportedFormatsIntersection, &speakerDeviceFormats, sizeof(speakerDeviceFormats));
        RtlCopyMemory(m_pSpeakerSupportedFormatsIntersection->Formats, speakerFormatsArray, SIZEOF_ARRAY(speakerFormatsArray)*sizeof(*speakerFormatsArray));

        //
        // Customize the topology/wave descriptors for this instance
        //
        ntStatus = CreateCustomEndpointMinipair(
            g_A2dpHpRenderEndpoints[0],
            &m_pSpeakerDescriptor->FriendlyName,
            &m_pSpeakerDescriptor->Category,
            m_pSpeakerDescriptor->FilterInterfacePropertyCount,
            m_pSpeakerDescriptor->FilterInterfaceProperties,
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
            A2dpHpSpeakerSupportedDeviceFormats,
            SIZEOF_ARRAY(A2dpHpSpeakerSupportedDeviceFormats),
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
            DPF(D_ERROR, ("%!FUNC!: InstallEndpointRenderFilters (A2DP Headphone): failed, 0x%x", ntStatus)),
            Done);

        //
        // Pend status notifications.
        //

        ntStatus = EnableA2dpHpConnectionStatusNotification();
        IF_FAILED_ACTION_JUMP(
            ntStatus,
            DPF(D_ERROR, ("%!FUNC!: EnableA2dpHpConnectionStatusNotification: failed, 0x%x", ntStatus)),
            Done);

        ntStatus = EnableA2dpHpSpeakerSiopNotification();
        IF_FAILED_ACTION_JUMP(
            ntStatus,
            DPF(D_ERROR, ("%!FUNC!: EnableA2dpHpSpeakerSiopNotification: failed, 0x%x", ntStatus)),
            Done);

        if (m_pSpeakerDescriptor->Capabilities.Volume)
        {
            // Volume speaker status.
            ntStatus = EnableA2dpHpSpeakerVolumeStatusNotification();
            IF_FAILED_ACTION_JUMP(
                ntStatus,
                DPF(D_ERROR, ("%!FUNC!: EnableA2dpHpSpeakerVolumeStatusNotification: failed, 0x%x", ntStatus)),
                Done);
        }
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
        InterlockedExchange((PLONG)&m_State, eA2dpHpStateFailed);
    }
}

//=============================================================================
#pragma code_seg("PAGE")
NTSTATUS 
A2dpHpDevice::CreateCustomEndpointMinipair
(
    _In_                                           PENDPOINT_MINIPAIR pBaseMinipair,
    _In_                                           PUNICODE_STRING FriendlyName,
    _In_                                           PGUID pCategory,
    _In_                                           ULONG customFilterInterfacePropertyCount,
    _In_count_(customFilterInterfacePropertyCount) DEVPROPERTY* customFilterInterfaceProperties,
    _Outptr_                                       PENDPOINT_MINIPAIR *ppCustomMinipair
)
{
    NTSTATUS ntStatus;
    PENDPOINT_MINIPAIR  pNewMinipair = NULL;
    SYSVAD_DEVPROPERTY* pTopoProperties = NULL;
    PPCFILTER_DESCRIPTOR pNewTopoFilterDesc = NULL;
    PPCPIN_DESCRIPTOR   pNewTopoPins = NULL;
    ULONG cTopoProperties;
    ULONG cTopoPins;
    ULONG cWaveProperties;
    SYSVAD_DEVPROPERTY* pWaveProperties = NULL;

    PAGED_CODE();

    //
    // This routine will add one more property to whatever the base minipair describes for the topo filter interface properties for the friendly name.
    // Additional properties will be added to the topo filter interface for custom properties set by the profile driver.
    // It will also allocate and set up custom filter and pin descriptors to allow changing the KSNODETYPE for the a2dp device
    //
    cTopoPins = pBaseMinipair->TopoDescriptor->PinCount;
    // Additional topo properties consist of the friendly name and custom filter properties set by the profile driver.
    cTopoProperties = pBaseMinipair->TopoInterfacePropertyCount + 1 + customFilterInterfacePropertyCount;
    pTopoProperties = (SYSVAD_DEVPROPERTY*)ExAllocatePool2(POOL_FLAG_NON_PAGED, cTopoProperties * sizeof(SYSVAD_DEVPROPERTY), SYSVAD_POOLTAG);
    pNewMinipair = (ENDPOINT_MINIPAIR*)ExAllocatePool2(POOL_FLAG_NON_PAGED, sizeof(ENDPOINT_MINIPAIR), SYSVAD_POOLTAG);
    pNewTopoFilterDesc = (PCFILTER_DESCRIPTOR*)ExAllocatePool2(POOL_FLAG_NON_PAGED, sizeof(PCFILTER_DESCRIPTOR), SYSVAD_POOLTAG);
    pNewTopoPins = (PCPIN_DESCRIPTOR*)ExAllocatePool2(POOL_FLAG_NON_PAGED, cTopoPins * sizeof(PCPIN_DESCRIPTOR), SYSVAD_POOLTAG);
    cWaveProperties = pBaseMinipair->WaveInterfacePropertyCount;
    pWaveProperties = (SYSVAD_DEVPROPERTY*)ExAllocatePool2(POOL_FLAG_NON_PAGED, cWaveProperties * sizeof(SYSVAD_DEVPROPERTY), SYSVAD_POOLTAG);

    if ((pTopoProperties != NULL) && (pNewMinipair != NULL) && (pNewTopoFilterDesc != NULL) && (pNewTopoPins != NULL) && (pWaveProperties != NULL))
    {
        SYSVAD_DEVPROPERTY *pLastProperty;

        // Copy base minipair properties to new property list
        if (pBaseMinipair->TopoInterfacePropertyCount > 0)
        {
            RtlCopyMemory(pTopoProperties, pBaseMinipair->TopoInterfaceProperties, pBaseMinipair->TopoInterfacePropertyCount * sizeof(SYSVAD_DEVPROPERTY));
        }

        // Add friendly name property to the topo property list
        NT_ASSERT(FriendlyName->Length + sizeof(UNICODE_NULL) <= FriendlyName->MaximumLength);  // Assuming NULL terminated string
        pLastProperty = pTopoProperties + pBaseMinipair->TopoInterfacePropertyCount;
        pLastProperty->PropertyKey = &DEVPKEY_DeviceInterface_FriendlyName;
        pLastProperty->Type = DEVPROP_TYPE_STRING_INDIRECT;
        pLastProperty->BufferSize = FriendlyName->Length + sizeof(UNICODE_NULL);
        pLastProperty->Buffer = FriendlyName->Buffer;
        pLastProperty++;

        // Add custom filter interface properties to the topo property list.
        for (ULONG i = 0; i < customFilterInterfacePropertyCount; i++)
        {
            pLastProperty->PropertyKey = &customFilterInterfaceProperties[i].CompKey.Key;
            pLastProperty->Type = customFilterInterfaceProperties[i].Type;
            pLastProperty->BufferSize = customFilterInterfaceProperties[i].BufferSize;
            pLastProperty->Buffer = customFilterInterfaceProperties[i].Buffer;
            pLastProperty++;
        }

        if (pBaseMinipair->WaveInterfacePropertyCount > 0)
        {
            RtlCopyMemory(pWaveProperties, pBaseMinipair->WaveInterfaceProperties, pBaseMinipair->WaveInterfacePropertyCount * sizeof(SYSVAD_DEVPROPERTY));
        }

        // Copy base minipair structure
        RtlCopyMemory(pNewMinipair, pBaseMinipair, sizeof(ENDPOINT_MINIPAIR));

        RtlCopyMemory(pNewTopoFilterDesc, pBaseMinipair->TopoDescriptor, sizeof(PCFILTER_DESCRIPTOR));
        RtlCopyMemory(pNewTopoPins, pBaseMinipair->TopoDescriptor->Pins, cTopoPins * sizeof(PCPIN_DESCRIPTOR));

        pNewTopoFilterDesc->Pins = pNewTopoPins;
        pNewMinipair->TopoDescriptor = pNewTopoFilterDesc;

        // Update it to point to new property list
        pNewMinipair->TopoInterfacePropertyCount = cTopoProperties;
        pNewMinipair->TopoInterfaceProperties = pTopoProperties;
        pNewMinipair->WaveInterfacePropertyCount = cWaveProperties;
        pNewMinipair->WaveInterfaceProperties = pWaveProperties;

        ntStatus = UpdateCustomEndpointCategory(pNewTopoFilterDesc, pNewTopoPins, pCategory);
        if (!NT_SUCCESS(ntStatus))
        {
            DPF(D_ERROR, ("%!FUNC!: UpdateCustomEndpointCategory failed, 0x%x", ntStatus));
        }
        else
        {
            *ppCustomMinipair = pNewMinipair;

            pTopoProperties = NULL;
            pWaveProperties = NULL;
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

    if (pTopoProperties != NULL)
    {
        ExFreePoolWithTag(pTopoProperties, SYSVAD_POOLTAG);
    }
    if (pWaveProperties != NULL)
    {
        ExFreePoolWithTag(pWaveProperties, SYSVAD_POOLTAG);
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
A2dpHpDevice::UpdateCustomEndpointCategory
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
A2dpHpDevice::DeleteCustomEndpointMinipair
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
A2dpHpDevice::Stop()
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
    eA2dpHpState    state       = eA2dpHpStateInvalid;

    state = (eA2dpHpState) InterlockedExchange((PLONG)&m_State, eA2dpHpStateStopping);
    ASSERT(state == eA2dpHpStateRunning || state == eA2dpHpStateFailed);
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
            DPF(D_ERROR, ("%!FUNC!: RemoveEndpointFilters (A2DP Headphone Speaker): failed, 0x%x", ntStatus));
        }
    }

    //
    // Release port/miniport pointers.
    //
    SAFE_RELEASE(m_UnknownSpeakerTopology);
    SAFE_RELEASE(m_UnknownSpeakerWave);

    //
    // The device is in the stopped state.
    //
    InterlockedExchange((PLONG)&m_State, eA2dpHpStateStopped);

}

#pragma code_seg("PAGE")
NTSTATUS A2dpHpDevice::CreateFilterNames(
    PUNICODE_STRING A2dpHpDeviceSymbolicLinkName
)
/*++

Routine Description:

Creates unique wave and topology filter reference names.

Since each subdevice representing a A2DP sideband interface arrival needs to have a unique
reference string, this function generates a hash on 'A2dpHpDeviceSymbolicLinkName' parameter
and appends to static interface strings.

--*/
{
    PAGED_CODE();
    DPF_ENTER(("[%!FUNC!]"));

    NTSTATUS status = STATUS_SUCCESS;
    ULONG hashValue = 0;

    status = RtlHashUnicodeString(A2dpHpDeviceSymbolicLinkName, TRUE, HASH_STRING_ALGORITHM_X65599, &hashValue);
    IF_FAILED_ACTION_JUMP(
        status,
        DPF(D_ERROR, ("%!FUNC!: Failed to create KSCATEGORY_REALTIME wave interface: 0x%x", status)),
        End);

    // create wave interfaces for speaker
    RtlStringCbPrintfW(m_SpeakerWaveNameBuffer, sizeof(m_SpeakerWaveNameBuffer), L"%s-%lu", A2DPHP_SPEAKER_WAVE_NAME, hashValue);
    RtlUnicodeStringInit(&m_SpeakerWaveRefString, m_SpeakerWaveNameBuffer);

    // create topology interfaces for speaker
    RtlStringCbPrintfW(m_SpeakerTopologyNameBuffer, sizeof(m_SpeakerTopologyNameBuffer), L"%s-%lu", A2DPHP_SPEAKER_TOPO_NAME, hashValue);
    RtlUnicodeStringInit(&m_SpeakerTopologyRefString, m_SpeakerTopologyNameBuffer);

End:
    return status;
}
#endif // SYSVAD_A2DP_SIDEBAND
