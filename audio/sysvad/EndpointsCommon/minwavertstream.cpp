#include <sysvad.h>
#include <limits.h>
#include <ks.h>
#include "simple.h"
#include "minwavert.h"
#include "minwavertstream.h"
#include "UnittestData.h"
#define MINWAVERTSTREAM_POOLTAG 'SRWM'

#pragma warning (disable : 4127)

//=============================================================================
// CMiniportWaveRTStream
//=============================================================================

//=============================================================================
#pragma code_seg("PAGE")
CMiniportWaveRTStream::~CMiniportWaveRTStream
( 
    void 
)
/*++

Routine Description:

  Destructor for wavertstream 

Arguments:

Return Value:

  NT status code.

--*/
{
    PAGED_CODE();
    if (NULL != m_pMiniport)
    {
        if (m_bUnregisterStream)
        {
            m_pMiniport->StreamClosed(m_ulPin, this);
            m_bUnregisterStream = FALSE;
        }
        
        m_pMiniport->Release();
        m_pMiniport = NULL;
    }

    if (m_pDpc)
    {
        ExFreePoolWithTag( m_pDpc, MINWAVERTSTREAM_POOLTAG );
        m_pDpc = NULL;
    }

    if (m_pTimer)
    {
        ExFreePoolWithTag( m_pTimer, MINWAVERTSTREAM_POOLTAG );
        m_pTimer = NULL;
    }

    if (m_pbMuted)
    {
        ExFreePoolWithTag( m_pbMuted, MINWAVERTSTREAM_POOLTAG );
        m_pbMuted = NULL;
    }

    if (m_plVolumeLevel)
    {
        ExFreePoolWithTag( m_plVolumeLevel, MINWAVERTSTREAM_POOLTAG );
        m_plVolumeLevel = NULL;
    }

    if (m_plPeakMeter)
    {
        ExFreePoolWithTag( m_plPeakMeter, MINWAVERTSTREAM_POOLTAG );
        m_plPeakMeter = NULL;
    }

    if (m_pWfExt)
    {
        ExFreePoolWithTag( m_pWfExt, MINWAVERTSTREAM_POOLTAG );
        m_pWfExt = NULL;
    }
    if (m_pNotificationTimer)
    {
        KeCancelTimer(m_pNotificationTimer);
        ExFreePoolWithTag(m_pNotificationTimer, MINWAVERTSTREAM_POOLTAG);
    }

    // Since we just cancelled the notification timer, wait for all queued 
    // DPCs to complete before we free the notification DPC.
    //
    KeFlushQueuedDpcs();

    if (m_pNotificationDpc)
    {
        ExFreePoolWithTag( m_pNotificationDpc, MINWAVERTSTREAM_POOLTAG );
    }
    
#ifdef SYSVAD_BTH_BYPASS
    ASSERT(m_ScoOpen == FALSE);
#endif  // SYSVAD_BTH_BYPASS

    DPF_ENTER(("[CMiniportWaveRTStream::~CMiniportWaveRTStream]"));
} // ~CMiniportWaveRTStream

//=============================================================================
#pragma code_seg("PAGE")
NTSTATUS
CMiniportWaveRTStream::Init
( 
    _In_ PCMiniportWaveRT           Miniport_,
    _In_ PPORTWAVERTSTREAM          PortStream_,
    _In_ ULONG                      Pin_,
    _In_ BOOLEAN                    Capture_,
    _In_ PKSDATAFORMAT              DataFormat_,
    _In_ GUID                       SignalProcessingMode
)
/*++

Routine Description:

  Initializes the stream object.

Arguments:

  Miniport_ -

  Pin_ -

  Capture_ -

  DataFormat -

  SignalProcessingMode - The driver uses the signalProcessingMode to configure
    driver and/or hardware specific signal processing to be applied to this new
    stream.

Return Value:

  NT status code.

--*/
{
    PAGED_CODE();

    PWAVEFORMATEX pWfEx = NULL;
    NTSTATUS ntStatus = STATUS_SUCCESS;

    m_pMiniport = NULL;
    m_ulPin = 0;
    m_bUnregisterStream = FALSE;
    m_bCapture = FALSE;
    m_ulDmaBufferSize = 0;
    m_pDmaBuffer = NULL;
    m_ulNotificationsPerBuffer = 0;
    m_KsState = KSSTATE_STOP;
    m_pTimer = NULL;
    m_pDpc = NULL;
    m_llPacketCounter = 0;
    m_ullPlayPosition = 0;
    m_ullWritePosition = 0;
    m_ullDmaTimeStamp = 0;
    m_hnsElapsedTimeCarryForward = 0;
    m_ulDmaMovementRate = 0;
    m_bLfxEnabled = FALSE;
    m_pbMuted = NULL;
    m_plVolumeLevel = NULL;
    m_plPeakMeter = NULL;
    m_pWfExt = NULL;
    m_ullLinearPosition = 0;
    m_ulContentId = 0;
    m_ulCurrentWritePosition = 0;
    m_ulLastOsReadPacket = ULONG_MAX;
    m_ulLastOsWritePacket = ULONG_MAX;
    m_llEoSPosition = (-1);
    m_IsCurrentWritePositionUpdated = 0;
    m_SignalProcessingMode = SignalProcessingMode;

#ifdef SYSVAD_BTH_BYPASS
    m_ScoOpen = FALSE;
#endif  // SYSVAD_BTH_BYPASS

    m_pPortStream = PortStream_;
    InitializeListHead(&m_NotificationList);
    m_ulNotificationIntervalMs = 0;

    m_pNotificationDpc = (PRKDPC)ExAllocatePoolWithTag(
        NonPagedPoolNx,
        sizeof(KDPC),
        MINWAVERTSTREAM_POOLTAG);
    if (!m_pNotificationDpc)
    {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    m_pNotificationTimer = (PKTIMER)ExAllocatePoolWithTag(
        NonPagedPoolNx,
        sizeof(KTIMER),
        MINWAVERTSTREAM_POOLTAG);
    if (!m_pNotificationTimer)
    {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    KeInitializeDpc(m_pNotificationDpc, TimerNotifyRT, this);
    KeInitializeTimerEx(m_pNotificationTimer, NotificationTimer);

    pWfEx = GetWaveFormatEx(DataFormat_);
    if (NULL == pWfEx) 
    { 
        return STATUS_UNSUCCESSFUL; 
    }

    m_pMiniport = reinterpret_cast<CMiniportWaveRT*>(Miniport_);
    if (m_pMiniport == NULL)
    {
        return STATUS_INVALID_PARAMETER;
    }
    m_pMiniport->AddRef();
    if (!NT_SUCCESS(ntStatus))
    {
        return ntStatus;
    }
    m_ulPin = Pin_;
    m_bCapture = Capture_;
    m_ulDmaMovementRate = pWfEx->nAvgBytesPerSec;

    m_pDpc = (PRKDPC)ExAllocatePoolWithTag(NonPagedPoolNx, sizeof(KDPC), MINWAVERTSTREAM_POOLTAG);
    if (!m_pDpc)
    {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    m_pWfExt = (PWAVEFORMATEXTENSIBLE)ExAllocatePoolWithTag(NonPagedPoolNx, sizeof(WAVEFORMATEX) + pWfEx->cbSize, MINWAVERTSTREAM_POOLTAG);
    if (m_pWfExt == NULL)
    {
        return STATUS_INSUFFICIENT_RESOURCES;
    }
    RtlCopyMemory(m_pWfExt, pWfEx, sizeof(WAVEFORMATEX) + pWfEx->cbSize);

    m_pbMuted = (PBOOL)ExAllocatePoolWithTag(NonPagedPoolNx, m_pWfExt->Format.nChannels * sizeof(BOOL), MINWAVERTSTREAM_POOLTAG);
    if (m_pbMuted == NULL)
    {
        return STATUS_INSUFFICIENT_RESOURCES;
    }
    RtlZeroMemory(m_pbMuted, m_pWfExt->Format.nChannels * sizeof(BOOL));

    m_plVolumeLevel = (PLONG)ExAllocatePoolWithTag(NonPagedPoolNx, m_pWfExt->Format.nChannels * sizeof(LONG), MINWAVERTSTREAM_POOLTAG);
    if (m_plVolumeLevel == NULL)
    {
        return STATUS_INSUFFICIENT_RESOURCES;
    }
    RtlZeroMemory(m_plVolumeLevel, m_pWfExt->Format.nChannels * sizeof(LONG));

    m_plPeakMeter = (PLONG)ExAllocatePoolWithTag(NonPagedPoolNx, m_pWfExt->Format.nChannels * sizeof(LONG), MINWAVERTSTREAM_POOLTAG);
    if (m_plPeakMeter == NULL)
    {
        return STATUS_INSUFFICIENT_RESOURCES;
    }
    RtlZeroMemory(m_plPeakMeter, m_pWfExt->Format.nChannels * sizeof(LONG));

    if (m_bCapture)
    {
        DWORD toneFrequency = 0;

        if (m_pMiniport->IsLoopbackPin(Pin_))
        {
            //
            // Loopbacks pins use a different frequency for test validation.
            //
            toneFrequency = 3000; // 3 kHz
        }
        else 
        {
            //
            // Init sine wave generator. To exercise the SignalProcessingMode parameter
            // this sample driver selects the frequency based on the parameter.
            //
            toneFrequency = IsEqualGUID(SignalProcessingMode, AUDIO_SIGNALPROCESSINGMODE_RAW) ? 1000 : 2000;
        }

        ntStatus = m_ToneGenerator.Init(toneFrequency, m_pWfExt);
        if (!NT_SUCCESS(ntStatus))
        {
            return ntStatus;
        }
    }
    else if (!g_DoNotCreateDataFiles)
    {
        //
        // Create an output file for the render data.
        //
        DPF(D_TERSE, ("SaveData %p", &m_SaveData));
        ntStatus = m_SaveData.SetDataFormat(DataFormat_);
        if (NT_SUCCESS(ntStatus))
        {
            ntStatus = m_SaveData.Initialize(m_pMiniport->IsOffloadPin(Pin_));
        }
    
        if (!NT_SUCCESS(ntStatus))
        {
            return ntStatus;
        }
    }

    //
    // Register this stream.
    //
    ntStatus = m_pMiniport->StreamCreated(m_ulPin, this);
    if (NT_SUCCESS(ntStatus))
    {
        m_bUnregisterStream = TRUE;
    }

    return ntStatus;
} // Init

//=============================================================================
#pragma code_seg("PAGE")
STDMETHODIMP_(NTSTATUS)
CMiniportWaveRTStream::NonDelegatingQueryInterface
( 
    _In_ REFIID  Interface,
    _COM_Outptr_ PVOID * Object 
)
/*++

Routine Description:

  QueryInterface

Arguments:

  Interface - GUID

  Object - interface pointer to be returned

Return Value:

  NT status code.

--*/
{
    PAGED_CODE();

    ASSERT(Object);

    if (IsEqualGUIDAligned(Interface, IID_IUnknown))
    {
        *Object = PVOID(PUNKNOWN(PMINIPORTWAVERTSTREAM(this)));
    }
    else if (IsEqualGUIDAligned(Interface, IID_IMiniportWaveRTStream))
    {
        *Object = PVOID(PMINIPORTWAVERTSTREAM(this));
    }
    else if (IsEqualGUIDAligned(Interface, IID_IMiniportWaveRTStreamNotification))
    {
        *Object = PVOID(PMINIPORTWAVERTSTREAMNOTIFICATION(this));
    }
    else if (IsEqualGUIDAligned(Interface, IID_IMiniportWaveRTInputStream) && (this->m_bCapture))
    {
        // This interface is supported only on capture streams
        *Object = PVOID(PMINIPORTWAVERTINPUTSTREAM(this));
    }
    else if (IsEqualGUIDAligned(Interface, IID_IMiniportWaveRTOutputStream) && (!this->m_bCapture))
    {
        // This interface is supported only on render streams
        *Object = PVOID(PMINIPORTWAVERTOUTPUTSTREAM(this));
    }
    else if (IsEqualGUIDAligned(Interface, IID_IMiniportStreamAudioEngineNode))
    {
        *Object = (PVOID)(IMiniportStreamAudioEngineNode*)this;
    }
    else if (IsEqualGUIDAligned(Interface, IID_IMiniportStreamAudioEngineNode2))
    {
        *Object = (PVOID)(IMiniportStreamAudioEngineNode2*)this;
    }
    else if (IsEqualGUIDAligned(Interface, IID_IDrmAudioStream))
    {
        *Object = (PVOID)(IDrmAudioStream*)this;
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
NTSTATUS CMiniportWaveRTStream::AllocateBufferWithNotification
(
    _In_    ULONG               NotificationCount_,
    _In_    ULONG               RequestedSize_,
    _Out_   PMDL                *AudioBufferMdl_,
    _Out_   ULONG               *ActualSize_,
    _Out_   ULONG               *OffsetFromFirstPage_,
    _Out_   MEMORY_CACHING_TYPE *CacheType_
)
{
    PAGED_CODE();

    ULONG ulBufferDurationMs = 0;

    if ( (0 == RequestedSize_) || (RequestedSize_ < m_pWfExt->Format.nBlockAlign) )
    { 
        return STATUS_UNSUCCESSFUL; 
    }
    
    if ((NotificationCount_ == 0) || (RequestedSize_ % NotificationCount_ != 0))
    {
        return STATUS_INVALID_PARAMETER;
    }

    RequestedSize_ -= RequestedSize_ % (m_pWfExt->Format.nBlockAlign);
    
    if (!m_bCapture && !g_DoNotCreateDataFiles)
    {
        NTSTATUS ntStatus;
        
        ntStatus = m_SaveData.SetMaxWriteSize(RequestedSize_);    
        if (!NT_SUCCESS(ntStatus))
        {
            return ntStatus;
        }
    }

    PHYSICAL_ADDRESS highAddress;
    highAddress.HighPart = 0;
    highAddress.LowPart = MAXULONG;

    PMDL pBufferMdl = m_pPortStream->AllocatePagesForMdl (highAddress, RequestedSize_);

    if (NULL == pBufferMdl)
    {
        return STATUS_UNSUCCESSFUL;
    }

    // From MSDN: 
    // "Since the Windows audio stack does not support a mechanism to express memory access 
    //  alignment requirements for buffers, audio drivers must select a caching type for mapped
    //  memory buffers that does not impose platform-specific alignment requirements. In other 
    //  words, the caching type used by the audio driver for mapped memory buffers, must not make 
    //  assumptions about the memory alignment requirements for any specific platform.
    //
    //  This method maps the physical memory pages in the MDL into kernel-mode virtual memory. 
    //  Typically, the miniport driver calls this method if it requires software access to the 
    //  scatter-gather list for an audio buffer. In this case, the storage for the scatter-gather 
    //  list must have been allocated by the IPortWaveRTStream::AllocatePagesForMdl or 
    //  IPortWaveRTStream::AllocateContiguousPagesForMdl method. 
    //
    //  A WaveRT miniport driver should not require software access to the audio buffer itself."
    //   
    m_pDmaBuffer = (BYTE*)m_pPortStream->MapAllocatedPages(pBufferMdl, MmCached);
    m_ulNotificationsPerBuffer = NotificationCount_;
    m_ulDmaBufferSize = RequestedSize_;
    ulBufferDurationMs = (RequestedSize_ * 1000) / m_ulDmaMovementRate;
    m_ulNotificationIntervalMs = ulBufferDurationMs / NotificationCount_;

    *AudioBufferMdl_ = pBufferMdl;
    *ActualSize_ = RequestedSize_;
    *OffsetFromFirstPage_ = 0;
    *CacheType_ = MmCached;

    return STATUS_SUCCESS;
}

//=============================================================================
#pragma code_seg("PAGE")
VOID CMiniportWaveRTStream::FreeBufferWithNotification
(
    _In_        PMDL    Mdl_,
    _In_        ULONG   Size_
)
{
    UNREFERENCED_PARAMETER(Size_);

    PAGED_CODE();

    if (Mdl_ != NULL)
    {
        if (m_pDmaBuffer != NULL)
        {
            m_pPortStream->UnmapAllocatedPages(m_pDmaBuffer, Mdl_);
            m_pDmaBuffer = NULL;
        }
        
        m_pPortStream->FreePagesFromMdl(Mdl_);
    }
    
    m_ulDmaBufferSize = 0;
    m_ulNotificationsPerBuffer = 0;

    return;
}

//=============================================================================
#pragma code_seg("PAGE")
NTSTATUS CMiniportWaveRTStream::RegisterNotificationEvent
(
    _In_ PKEVENT NotificationEvent_
)
{
    UNREFERENCED_PARAMETER(NotificationEvent_);

    PAGED_CODE();

    NotificationListEntry *nleNew = (NotificationListEntry*)ExAllocatePoolWithTag( 
        NonPagedPoolNx,
        sizeof(NotificationListEntry),
        MINWAVERTSTREAM_POOLTAG);
    if (NULL == nleNew)
    {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    nleNew->NotificationEvent = NotificationEvent_;

    if (!IsListEmpty(&m_NotificationList))
    {
        PLIST_ENTRY leCurrent = m_NotificationList.Flink;
        while (leCurrent != &m_NotificationList)
        {
            NotificationListEntry* nleCurrent = CONTAINING_RECORD( leCurrent, NotificationListEntry, ListEntry);
            if (nleCurrent->NotificationEvent == NotificationEvent_)
            {
                RemoveEntryList( leCurrent );
                ExFreePoolWithTag( nleNew, MINWAVERTSTREAM_POOLTAG );
                return STATUS_UNSUCCESSFUL;
            }

            leCurrent = leCurrent->Flink;
        }
    }

    InsertTailList(&m_NotificationList, &(nleNew->ListEntry));
    
    return STATUS_SUCCESS;
}

//=============================================================================
#pragma code_seg("PAGE")
NTSTATUS CMiniportWaveRTStream::UnregisterNotificationEvent
(
    _In_ PKEVENT NotificationEvent_
)
{
    UNREFERENCED_PARAMETER(NotificationEvent_);

    PAGED_CODE();

    if (!IsListEmpty(&m_NotificationList))
    {
        PLIST_ENTRY leCurrent = m_NotificationList.Flink;
        while (leCurrent != &m_NotificationList)
        {
            NotificationListEntry* nleCurrent = CONTAINING_RECORD( leCurrent, NotificationListEntry, ListEntry);
            if (nleCurrent->NotificationEvent == NotificationEvent_)
            {
                RemoveEntryList( leCurrent );
                ExFreePoolWithTag( nleCurrent, MINWAVERTSTREAM_POOLTAG );
                return STATUS_SUCCESS;
            }

            leCurrent = leCurrent->Flink;
        }
    }

    return STATUS_NOT_FOUND;
}


//=============================================================================
#pragma code_seg("PAGE")
NTSTATUS CMiniportWaveRTStream::GetClockRegister
(
    _Out_ PKSRTAUDIO_HWREGISTER Register_
)
{
    UNREFERENCED_PARAMETER(Register_);

    PAGED_CODE();

    return STATUS_NOT_IMPLEMENTED;
}

//=============================================================================
#pragma code_seg("PAGE")
NTSTATUS CMiniportWaveRTStream::GetPositionRegister
(
    _Out_ PKSRTAUDIO_HWREGISTER Register_
)
{
    UNREFERENCED_PARAMETER(Register_);

    PAGED_CODE();

    return STATUS_NOT_IMPLEMENTED;
}

//=============================================================================
#pragma code_seg("PAGE")
VOID CMiniportWaveRTStream::GetHWLatency
(
    _Out_ PKSRTAUDIO_HWLATENCY  Latency_
)
{
    PAGED_CODE();

    ASSERT(Latency_);

    Latency_->ChipsetDelay = 0;
    Latency_->CodecDelay = 0;
    Latency_->FifoSize = 0;
}

//=============================================================================
#pragma code_seg("PAGE")
VOID CMiniportWaveRTStream::FreeAudioBuffer
(
_In_opt_    PMDL        Mdl_,
_In_        ULONG       Size_
)
{
    UNREFERENCED_PARAMETER(Size_);

    PAGED_CODE();

    if (Mdl_ != NULL)
    {
        if (m_pDmaBuffer != NULL)
        {
            m_pPortStream->UnmapAllocatedPages(m_pDmaBuffer, Mdl_);
            m_pDmaBuffer = NULL;
        }

        m_pPortStream->FreePagesFromMdl(Mdl_);
    }

    m_ulDmaBufferSize = 0;
    m_ulNotificationsPerBuffer = 0;
}

//=============================================================================
#pragma code_seg("PAGE")
NTSTATUS CMiniportWaveRTStream::AllocateAudioBuffer
(
_In_    ULONG                   RequestedSize_,
_Out_   PMDL                   *AudioBufferMdl_,
_Out_   ULONG                  *ActualSize_,
_Out_   ULONG                  *OffsetFromFirstPage_,
_Out_   MEMORY_CACHING_TYPE    *CacheType_
)
{
    PAGED_CODE();

    if ((0 == RequestedSize_) || (RequestedSize_ < m_pWfExt->Format.nBlockAlign))
    {
        return STATUS_UNSUCCESSFUL;
    }

    RequestedSize_ -= RequestedSize_ % (m_pWfExt->Format.nBlockAlign);

    PHYSICAL_ADDRESS highAddress;
    highAddress.HighPart = 0;
    highAddress.LowPart = MAXULONG;

    PMDL pBufferMdl = m_pPortStream->AllocatePagesForMdl(highAddress, RequestedSize_);

    if (NULL == pBufferMdl)
    {
        return STATUS_UNSUCCESSFUL;
    }

    // From MSDN: 
    // "Since the Windows audio stack does not support a mechanism to express memory access 
    //  alignment requirements for buffers, audio drivers must select a caching type for mapped
    //  memory buffers that does not impose platform-specific alignment requirements. In other 
    //  words, the caching type used by the audio driver for mapped memory buffers, must not make 
    //  assumptions about the memory alignment requirements for any specific platform.
    //
    //  This method maps the physical memory pages in the MDL into kernel-mode virtual memory. 
    //  Typically, the miniport driver calls this method if it requires software access to the 
    //  scatter-gather list for an audio buffer. In this case, the storage for the scatter-gather 
    //  list must have been allocated by the IPortWaveRTStream::AllocatePagesForMdl or 
    //  IPortWaveRTStream::AllocateContiguousPagesForMdl method. 
    //
    //  A WaveRT miniport driver should not require software access to the audio buffer itself."
    //   
    m_pDmaBuffer = (BYTE*)m_pPortStream->MapAllocatedPages(pBufferMdl, MmCached);

    m_ulDmaBufferSize = RequestedSize_;
    m_ulNotificationsPerBuffer = 0;

    *AudioBufferMdl_ = pBufferMdl;
    *ActualSize_ = RequestedSize_;
    *OffsetFromFirstPage_ = 0;
    *CacheType_ = MmCached;

    return STATUS_SUCCESS;
}

//=============================================================================
#pragma code_seg()
NTSTATUS CMiniportWaveRTStream::GetPosition
(
    _Out_   KSAUDIO_POSITION    *Position_
)
{
    NTSTATUS ntStatus;

#ifdef SYSVAD_BTH_BYPASS
    if (m_ScoOpen)
    {
        ntStatus = GetScoStreamNtStatus();
        IF_FAILED_JUMP(ntStatus, Done);
    }
#endif // SYSVAD_BTH_BYPASS

    // Return failure if this is the keyword detector pin
    if (m_pMiniport->m_DeviceFormatsAndModes[m_ulPin].PinType == PINTYPE::KeywordCapturePin)
    {
        return STATUS_NOT_SUPPORTED;
    }

    if (m_KsState == KSSTATE_RUN)
    {
        //
        // Get the current time and update position.
        //
        LARGE_INTEGER ilQPC = KeQueryPerformanceCounter(NULL);
        UpdatePosition(ilQPC);
    }

    Position_->PlayOffset = m_ullPlayPosition;
    Position_->WriteOffset = m_ullWritePosition;

    ntStatus = STATUS_SUCCESS;
    
#ifdef SYSVAD_BTH_BYPASS
Done:
#endif // SYSVAD_BTH_BYPASS
    return ntStatus;
}

//=============================================================================
// CMiniportWaveRTStream::GetReadPacket
//
//  Returns information about the next packet for the OS to read.
//
// Return value
//
//  Returns STATUS_OPERATION_IN_PROGRESS if no new packets are available and
//  the next packet is in progress.
//
// IRQL - PASSIVE_LEVEL
//
// Remarks
//  Although called at passive level, this routine is non-paged code because
//  it is called in the streaming path where page faults should be avoided.
//
// ISSUE-2014/10/4 Will this work correctly across pause/play?
#pragma code_seg()
_IRQL_requires_max_(PASSIVE_LEVEL)
NTSTATUS CMiniportWaveRTStream::GetReadPacket
(
    _Out_ ULONG     *PacketNumber,
    _Out_ DWORD     *Flags,
    _Out_ ULONG64   *PerformanceCounterValue,
    _Out_ BOOL      *MoreData
)
{
    NTSTATUS ntStatus;
    ULONG availablePacketNumber;
    ULONG droppedPackets;

    // The call must be from event driven mode
    if(m_ulNotificationsPerBuffer == 0)
    {
        return STATUS_NOT_SUPPORTED;
    }
    
    *Flags = 0;

    if (m_KsState < KSSTATE_PAUSE)
    {
        return STATUS_INVALID_DEVICE_STATE;
    }

    // If this is the keyword detector pin, then stream from the keyword FIFO
    if (m_pMiniport->m_DeviceFormatsAndModes[m_ulPin].PinType == PINTYPE::KeywordCapturePin)
    {
        // FUTURE-2014/11/18 Drive this with packet counter
        ntStatus = m_pMiniport->m_KeywordDetector.GetReadPacket(m_ulNotificationsPerBuffer, m_ulDmaBufferSize, m_pDmaBuffer, PacketNumber, PerformanceCounterValue, MoreData);
        if (NT_SUCCESS(ntStatus))
        {
            m_ulLastOsReadPacket = *PacketNumber;
        }
        return ntStatus;
    }

    if (m_KsState == KSSTATE_RUN)
    {
        //
        // Get the current time and update simulated position.
        //
        LARGE_INTEGER ilQPC = KeQueryPerformanceCounter(NULL);

        UpdatePosition(ilQPC);
    }

    // The 0-based number of the last completed packet
    // FUTURE-2014/10/27 Update to allow different numbers of packets per WaveRT buffer
    availablePacketNumber = LODWORD(m_llPacketCounter - 1);  // Note this might be ULONG_MAX if called during the first packet

    // If no new packets are available...
    if (availablePacketNumber == m_ulLastOsReadPacket)
    {
        return STATUS_DEVICE_NOT_READY;
    }

    // If more than one packet has transferred since the last packet read by
    // the OS, then those were dropped. That is, a glitch occurred.
    droppedPackets = availablePacketNumber - m_ulLastOsReadPacket - 1;
    if (droppedPackets > 0)
    {
        // Trace a glitch
    }

    // Return next packet number to be read
    *PacketNumber = availablePacketNumber;

    // Compute and return timestamp corresponding to start of the available packet. In a real hardware
    // driver, the timestamp would be computed in a driver and hardware specific manner. In this sample
    // driver, it is extrapolated from the sample driver's internal simulated position corrleation
    // [m_ullLinearPosition @ m_ullDmaTimeStamp] and the sample's internal 64-bit packet counter, subtracting
    // 1 from the packet counter to compute the time at the start of that last completed packet.
    ULONGLONG linearPositionOfAvailablePacket = (m_llPacketCounter - 1) * (m_ulDmaBufferSize / m_ulNotificationsPerBuffer);
    ULONGLONG deltaLinearPosition = m_ullLinearPosition - linearPositionOfAvailablePacket;
    ULONGLONG deltaTimeInHns = deltaLinearPosition * 10000000 / m_ulDmaMovementRate;
    ULONGLONG timeOfAvailablePacketInHns = m_ullDmaTimeStamp - deltaTimeInHns;
    ULONGLONG timeOfAvailablePacketInQpc = timeOfAvailablePacketInHns * m_ullPerformanceCounterFrequency.QuadPart / 10000000;

    *PerformanceCounterValue = timeOfAvailablePacketInQpc;

    // No flags are defined yet
    *Flags = 0;

    // This sample does not internally buffer data so there is never more data
    // than revealed by the results from this routine.
    *MoreData = FALSE;

    // Update the last packet read by the OS
    m_ulLastOsReadPacket = availablePacketNumber;

#if 0
    // For test, embed packet number and timestamp into first two LONGLONGs of the packet
    LONG packetIndex = availablePacketNumber % m_ulNotificationsPerBuffer;
    SIZE_T packetSize = m_ulDmaBufferSize / m_ulNotificationsPerBuffer;
    BYTE *packetDataAsBytes = m_pDmaBuffer + (packetIndex * packetSize);
    LONGLONG *packetDataAsLonglongs = (LONGLONG*)packetDataAsBytes;
    for (int i = 0; i < packetSize / sizeof(LONGLONG); i++)
    {
        packetDataAsLonglongs[i] = i;
    }
    packetDataAsLonglongs[0] = availablePacketNumber;
    packetDataAsLonglongs[1] = timeOfAvailablePacketInQpc;
#endif

    return STATUS_SUCCESS;
}

#pragma code_seg()
_IRQL_requires_max_(PASSIVE_LEVEL)
NTSTATUS CMiniportWaveRTStream::SetWritePacket
(
_In_ ULONG      PacketNumber,
_In_ DWORD      Flags,
_In_ ULONG      EosPacketLength
)
{
    NTSTATUS ntStatus;

    // The call must be from event driven mode
    if (m_ulNotificationsPerBuffer == 0)
    {
        return STATUS_NOT_SUPPORTED;
    }

    ULONG oldLastOsWritePacket = m_ulLastOsWritePacket;

    // This function should not be called once EoS has been set.
    if (m_llEoSPosition >= 0)
    {
        return STATUS_INVALID_DEVICE_STATE;
    }

    // Update linear positions
    if (m_KsState == KSSTATE_RUN)
    {
        // Get the current time and update simulated position.
        LARGE_INTEGER ilQPC = KeQueryPerformanceCounter(NULL);
        UpdatePosition(ilQPC);
    }

    // 1-based count of completed packets, 0-based packet number of current packet
    LONGLONG currentPacket = m_llPacketCounter;

    // If not running, the current packet hasn't actually started transfering so OS should be writing
    // to the current packet. If running, then the current packing is already transfering to hardware
    // so the OS should write the packet after the current packet.
    ULONG expectedPacket = LODWORD(currentPacket);
    if (m_KsState == KSSTATE_RUN)
    {
        expectedPacket++;
    }

    // Check if OS PacketNumber is behind or too far ahead of current packet
    LONG deltaFromExpectedPacket = PacketNumber - expectedPacket;   // Modulo arithemetic
    if (deltaFromExpectedPacket < 0)
    {
        return STATUS_DATA_LATE_ERROR;
    }
    else if (deltaFromExpectedPacket > 0)
    {
        return STATUS_DATA_OVERRUN;
    }

    ULONG packetSize = (m_ulDmaBufferSize / m_ulNotificationsPerBuffer);
    ULONG packetIndex = PacketNumber % m_ulNotificationsPerBuffer;
    ULONG ulCurrentWritePosition = packetIndex * packetSize;

    // Check if EOS flag was passed
    if (Flags & KSSTREAM_HEADER_OPTIONSF_ENDOFSTREAM)
    {
        if (EosPacketLength > packetSize)
        {
            return STATUS_INVALID_PARAMETER;
        }
        else {
            // EOS position will be after the total completed packets, plus the packet in progress,
            // plus this EOS packet length
            m_ulLastOsWritePacket = PacketNumber;
            m_llEoSPosition = ((m_llPacketCounter + 1) * packetSize) + EosPacketLength;
            ntStatus = SetStreamCurrentWritePositionForLastBuffer(ulCurrentWritePosition);
        }
    }
    else
    {
        m_ulLastOsWritePacket = PacketNumber;

        // This function sets the current write position to the specified byte in the DMA buffer.
        // Will check if the write position is smaller than the DMA buffer size.
        // Will not return an error when the passed in parameter is 0.
        // Will also check if this function was called with the same write position(in event mode only)
        // Underruning will also be checked via timer mechanism
        ntStatus = SetCurrentWritePositionInternal(ulCurrentWritePosition);
    }

    if (!NT_SUCCESS(ntStatus))
    {
        m_llEoSPosition = (-1);
        m_ulLastOsWritePacket = oldLastOsWritePacket;
    }

    return ntStatus;
}

//=============================================================================
#pragma code_seg()
_IRQL_requires_max_(PASSIVE_LEVEL)
NTSTATUS CMiniportWaveRTStream::GetOutputStreamPresentationPosition
(
    _Out_ KSAUDIO_PRESENTATION_POSITION *pPresentationPosition
)
{
    ASSERT (pPresentationPosition);
    
    // The call must be from event driven mode
    if(m_ulNotificationsPerBuffer == 0)
    {
        return STATUS_NOT_SUPPORTED;
    }
    
    return GetPresentationPosition(pPresentationPosition);
}

//=============================================================================
#pragma code_seg()
_IRQL_requires_max_(PASSIVE_LEVEL)
NTSTATUS CMiniportWaveRTStream::GetPacketCount
(
    _Out_ ULONG *pPacketCount
)
{
    ASSERT(pPacketCount);
    
    // The call must be from event driven mode
    if(m_ulNotificationsPerBuffer == 0)
    {
        return STATUS_NOT_SUPPORTED;
    }
    
    if (m_KsState == KSSTATE_RUN)
    {
        // Get the current time and update simulated position.
        LARGE_INTEGER ilQPC = KeQueryPerformanceCounter(NULL);
        UpdatePosition(ilQPC);
    }

    *pPacketCount = LODWORD(m_llPacketCounter);

    return STATUS_SUCCESS;
}

//=============================================================================
#pragma code_seg("PAGE")
NTSTATUS CMiniportWaveRTStream::SetState
(
    _In_    KSSTATE State_
)
{
    PAGED_CODE();

    NTSTATUS        ntStatus        = STATUS_SUCCESS;
    PADAPTERCOMMON  pAdapterComm    = m_pMiniport->GetAdapterCommObj();

    // Spew an event for a pin state change request from portcls
    //Event type: eMINIPORT_PIN_STATE
    //Parameter 1: Current linear buffer position	
    //Parameter 2: Current WaveRtBufferWritePosition	
    //Parameter 3: Pin State 0->KS_STOP, 1->KS_ACQUIRE, 2->KS_PAUSE, 3->KS_RUN 
    //Parameter 4:0
    pAdapterComm->WriteEtwEvent(eMINIPORT_PIN_STATE, 
                                100, // replace with the correct "Current linear buffer position"	
                                m_ulCurrentWritePosition, // replace with the previous WaveRtBufferWritePosition that the drive received	
                                State_, // repalce with the correct "Data length completed"
                                0);  // always zero

    switch (State_)
    {
        case KSSTATE_STOP:
            // Reset DMA
            m_llPacketCounter = 0;
            m_ullPlayPosition = 0;
            m_ullWritePosition = 0;
            m_ullLinearPosition = 0;
            
            // Reset OS read/write positions
            m_ulLastOsReadPacket = ULONG_MAX;
            m_ulCurrentWritePosition = 0;
            m_ulLastOsWritePacket = ULONG_MAX;
            m_llEoSPosition = (-1);
            
            // Wait until all work items are completed.
            if (!m_bCapture && !g_DoNotCreateDataFiles)
            {
                m_SaveData.WaitAllWorkItems();
            }

#ifdef SYSVAD_BTH_BYPASS
            if (m_ScoOpen)
            {
                PBTHHFPDEVICECOMMON bthHfpDevice;
                
                ASSERT(m_pMiniport->IsBthHfpDevice());
                bthHfpDevice = m_pMiniport->GetBthHfpDevice(); // weak ref.
                ASSERT(bthHfpDevice != NULL);

                //
                // Close the SCO connection.
                //
                ntStatus = bthHfpDevice->StreamClose();
                if (!NT_SUCCESS(ntStatus))
                {
                    DPF(D_ERROR, ("SetState: KSSTATE_STOP, StreamClose failed, 0x%x", ntStatus));
                }
                
                m_ScoOpen = FALSE;
            }
#endif // SYSVAD_BTH_BYPASS
            break;

        case KSSTATE_ACQUIRE:
#ifdef SYSVAD_BTH_BYPASS
            if (m_pMiniport->IsBthHfpDevice())
            {
                if (m_ScoOpen == FALSE)
                {
                    PBTHHFPDEVICECOMMON bthHfpDevice;

                    bthHfpDevice = m_pMiniport->GetBthHfpDevice(); // weak ref.
                    ASSERT(bthHfpDevice != NULL);

                    //
                    // Open the SCO connection.
                    //
                    ntStatus = bthHfpDevice->StreamOpen();
                    IF_FAILED_ACTION_JUMP(
                        ntStatus,
                        DPF(D_ERROR, ("SetState: KSSTATE_ACQUIRE, StreamOpen failed, 0x%x", ntStatus)),
                        Done);

                    m_ScoOpen = TRUE;
                }
            }
#endif // SYSVAD_BTH_BYPASS
            break;

        case KSSTATE_PAUSE:
            ULONGLONG ullPositionTemp;

            if (m_KsState > KSSTATE_PAUSE)
            {
                //
                // Run -> Pause
                //
                if (m_pMiniport->m_DeviceFormatsAndModes[m_ulPin].PinType == PINTYPE::KeywordCapturePin)
                {
                    m_pMiniport->m_KeywordDetector.Stop();
                }

                // Pause DMA
                if (m_ulNotificationIntervalMs > 0)
                {
                    KeCancelTimer(m_pNotificationTimer);
                    ExSetTimerResolution(0, FALSE);
                    KeFlushQueuedDpcs(); 
                }
            }

            // This call updates the linear buffer position.
            GetLinearBufferPosition(&ullPositionTemp, NULL);
            break;

        case KSSTATE_RUN:
            // Start DMA
            LARGE_INTEGER ullPerfCounterTemp;
            if (m_pMiniport->m_DeviceFormatsAndModes[m_ulPin].PinType == PINTYPE::KeywordCapturePin)
            {
                m_pMiniport->m_KeywordDetector.Run();
            }
            ullPerfCounterTemp = KeQueryPerformanceCounter(&m_ullPerformanceCounterFrequency);
            m_ullDmaTimeStamp = KSCONVERT_PERFORMANCE_TIME(m_ullPerformanceCounterFrequency.QuadPart, ullPerfCounterTemp);
            m_hnsElapsedTimeCarryForward  = 0;

            if (m_ulNotificationIntervalMs > 0)
            {
                LARGE_INTEGER   delay;

                delay.QuadPart = (-2) * (((LONGLONG)m_ulNotificationIntervalMs) * HNSTIME_PER_MILLISECOND);

                // ISSUE-2014/10/20 Set resolution based on packet size
                ExSetTimerResolution(HNSTIME_PER_MILLISECOND, TRUE);

                KeSetTimerEx
                (
                    m_pNotificationTimer,
                    delay,
                    m_ulNotificationIntervalMs,
                    m_pNotificationDpc
                );
            }

            break;
    }

    m_KsState = State_;

#ifdef SYSVAD_BTH_BYPASS
Done:
#endif  // SYSVAD_BTH_BYPASS
    return ntStatus;
}

//=============================================================================
#pragma code_seg("PAGE")
NTSTATUS CMiniportWaveRTStream::SetFormat
(
    _In_    KSDATAFORMAT    *DataFormat_
)
{
    UNREFERENCED_PARAMETER(DataFormat_);

    PAGED_CODE();

    //if (!m_fCapture && !g_DoNotCreateDataFiles)
    //{
    //    ntStatus = m_SaveData.SetDataFormat(Format);
    //}

    return STATUS_NOT_SUPPORTED;
}

#pragma code_seg()

//=============================================================================
#pragma code_seg()
VOID CMiniportWaveRTStream::UpdatePosition
(
    _In_ LARGE_INTEGER ilQPC
)
{
    // Convert ticks to 100ns units.
    LONGLONG  hnsCurrentTime = KSCONVERT_PERFORMANCE_TIME(m_ullPerformanceCounterFrequency.QuadPart, ilQPC);
    
    // Calculate the time elapsed since the last call to GetPosition() or since the
    // DMA engine started.  Note that the division by 10000 to convert to milliseconds
    // may cause us to lose some of the time, so we will carry the remainder forward 
    // to the next GetPosition() call.
    //
    ULONG TimeElapsedInMS = (ULONG)(hnsCurrentTime - m_ullDmaTimeStamp + m_hnsElapsedTimeCarryForward)/10000;
    
    // Carry forward the remainder of this division so we don't fall behind with our position too much.
    //
    m_hnsElapsedTimeCarryForward = (hnsCurrentTime - m_ullDmaTimeStamp + m_hnsElapsedTimeCarryForward) % 10000;
    
    // Calculate how many bytes in the DMA buffer would have been processed in the elapsed
    // time.  Note that the division by 1000 to convert to milliseconds may cause us to 
    // lose some bytes, so we will carry the remainder forward to the next GetPosition() call.
    //
    // need to divide by 1000 because m_ulDmaMovementRate is average bytes per sec.
    ULONG ByteDisplacement = (m_ulDmaMovementRate * TimeElapsedInMS) / 1000;
    
    if (m_bCapture)
    {
        // Write sine wave to buffer.
        WriteBytes(ByteDisplacement);
    }
    else if (!g_DoNotCreateDataFiles)
    {
        // Read from buffer and write to a file.
        ReadBytes(ByteDisplacement);
        
        // If the last packet was rendered(read in the sample driver's case), send out an etw event.
        if ( (m_llEoSPosition >= 0) 
          && ((m_llEoSPosition - m_ullLinearPosition) < ByteDisplacement))
        {
            PADAPTERCOMMON pAdapterComm = m_pMiniport->GetAdapterCommObj();
            pAdapterComm->WriteEtwEvent(eMINIPORT_LAST_BUFFER_RENDERED, 
                                        m_ullLinearPosition + ByteDisplacement, // Current linear buffer position  
                                        m_ulCurrentWritePosition, // The very last WaveRtBufferWritePosition that the driver received
                                        0, 
                                        0);
        }
    }
    
    // Increment the DMA position by the number of bytes displaced since the last
    // call to UpdatePosition() and ensure we properly wrap at buffer length.
    //
    m_ullPlayPosition = m_ullWritePosition =
        (m_ullWritePosition + ByteDisplacement) % m_ulDmaBufferSize;
    
    // m_ullDmaTimeStamp is updated in both GetPostion and GetLinearPosition calls
    // so m_ullLinearPosition needs to be updated accordingly here
    //
    m_ullLinearPosition += ByteDisplacement;
    
    // Update the DMA time stamp for the next call to GetPosition()
    //
    m_ullDmaTimeStamp = hnsCurrentTime;
}

//=============================================================================
#pragma code_seg()
VOID CMiniportWaveRTStream::WriteBytes
(
_In_ ULONG ByteDisplacement
)
/*++

Routine Description:

This function writes the audio buffer using a sine wave generator.

Arguments:

ByteDisplacement - # of bytes to process.

--*/
{
    ULONG bufferOffset = m_ullLinearPosition % m_ulDmaBufferSize;

    // Normally this will loop no more than once for a single wrap, but if
    // many bytes have been displaced then this may loops many times.
    while (ByteDisplacement > 0)
    {
        ULONG runWrite = min(ByteDisplacement, m_ulDmaBufferSize - bufferOffset);
        m_ToneGenerator.GenerateSine(m_pDmaBuffer + bufferOffset, runWrite);
        bufferOffset = (bufferOffset + runWrite) % m_ulDmaBufferSize;
        ByteDisplacement -= runWrite;
    }
}

//=============================================================================
#pragma code_seg()
VOID CMiniportWaveRTStream::ReadBytes
(
_In_ ULONG ByteDisplacement
)
/*++

Routine Description:

This function reads the audio buffer and saves the data in a file.

Arguments:

ByteDisplacement - # of bytes to process.

--*/
{
    ULONG bufferOffset = m_ullLinearPosition % m_ulDmaBufferSize;

    // Normally this will loop no more than once for a single wrap, but if
    // many bytes have been displaced then this may loops many times.
    while (ByteDisplacement > 0)
    {
        ULONG runWrite = min(ByteDisplacement, m_ulDmaBufferSize - bufferOffset);
        m_SaveData.WriteData(m_pDmaBuffer + bufferOffset, runWrite);
        bufferOffset = (bufferOffset + runWrite) % m_ulDmaBufferSize;
        ByteDisplacement -= runWrite;
    }
}

//=============================================================================
#pragma code_seg("PAGE")
STDMETHODIMP_(NTSTATUS) 
CMiniportWaveRTStream::SetContentId
(
    _In_  ULONG                   contentId,
    _In_  PCDRMRIGHTS             drmRights
)
/*++

Routine Description:

  Sets DRM content Id for this stream. Also updates the Mixed content Id.

Arguments:

  contentId - new content id

  drmRights - rights for this stream.

Return Value:

  NT status code.

--*/
{
    PAGED_CODE();

    DPF_ENTER(("[CMiniportWaveRT::SetContentId]"));

    NTSTATUS    ntStatus;
    ULONG       ulOldContentId = contentId;

    m_ulContentId = contentId;

    //
    // Miniport should create a mixed DrmRights.
    //
    ntStatus = m_pMiniport->UpdateDrmRights();

    //
    // Restore the passed-in content Id.
    //
    if (!NT_SUCCESS(ntStatus))
    {
        m_ulContentId = ulOldContentId;
    }

    //
    // SYSVAD writes each stream seperately to disk. If the rights for this
    // stream indicates that the stream is CopyProtected, stop writing to disk.
    //
    m_SaveData.Disable(drmRights->CopyProtect);

    //
    // From MSDN:
    //
    // This sample doesn't forward protected content, but if your driver uses 
    // lower layer drivers or a different stack to properly work, please see the 
    // following info from MSDN:
    //
    // "Before allowing protected content to flow through a data path, the system
    // verifies that the data path is secure. To do so, the system authenticates
    // each module in the data path beginning at the upstream end of the data path
    // and moving downstream. As each module is authenticated, that module gives
    // the system information about the next module in the data path so that it
    // can also be authenticated. To be successfully authenticated, a module's 
    // binary file must be signed as DRM-compliant.
    //
    // Two adjacent modules in the data path can communicate with each other in 
    // one of several ways. If the upstream module calls the downstream module 
    // through IoCallDriver, the downstream module is part of a WDM driver. In 
    // this case, the upstream module calls the DrmForwardContentToDeviceObject
    // function to provide the system with the device object representing the 
    // downstream module. (If the two modules communicate through the downstream
    // module's COM interface or content handlers, the upstream module calls 
    // DrmForwardContentToInterface or DrmAddContentHandlers instead.)
    //
    // DrmForwardContentToDeviceObject performs the same function as 
    // PcForwardContentToDeviceObject and IDrmPort2::ForwardContentToDeviceObject." 
    //
    // Other supported DRM DDIs for down-level module validation are: 
    // DrmForwardContentToInterfaces and DrmAddContentHandlers.
    //
    // For more information, see MSDN's DRM Functions and Interfaces.
    //

    return ntStatus;
} // SetContentId

#ifdef SYSVAD_BTH_BYPASS

//=============================================================================
#pragma code_seg()
NTSTATUS 
CMiniportWaveRTStream::GetScoStreamNtStatus()
/*++

Routine Description:

  Checks if the Bluetooth SCO HFP connection is up, if not, an error is returned.

Return Value:

  NT status code.

--*/
{
    DPF_ENTER(("[CMiniportWaveRTStream::GetScoStreamNtStatus]"));

    NTSTATUS  ntStatus  = STATUS_INVALID_DEVICE_STATE;
        
    if (m_ScoOpen)
    {
        PBTHHFPDEVICECOMMON bthHfpDevice;
        
        ASSERT(m_pMiniport->IsBthHfpDevice());
        bthHfpDevice = m_pMiniport->GetBthHfpDevice(); // weak ref.
        ASSERT(bthHfpDevice != NULL);

        if (bthHfpDevice->GetStreamStatus())
        {
            ntStatus = STATUS_SUCCESS;
        }
    }

    return ntStatus;        
}
#endif // SYSVAD_BTH_BYPASS

//=============================================================================
#pragma code_seg()
void
TimerNotifyRT
(
    _In_      PKDPC         Dpc,
    _In_opt_  PVOID         DeferredContext,
    _In_opt_  PVOID         SA1,
    _In_opt_  PVOID         SA2
)
{
    LARGE_INTEGER qpc;
    LARGE_INTEGER qpcFrequency;

    UNREFERENCED_PARAMETER(Dpc);
    UNREFERENCED_PARAMETER(SA1);
    UNREFERENCED_PARAMETER(SA2);

    _IRQL_limited_to_(DISPATCH_LEVEL);

    qpc = KeQueryPerformanceCounter(&qpcFrequency);

    CMiniportWaveRTStream* _this = (CMiniportWaveRTStream*)DeferredContext;
    
    if (NULL == _this)
    {
        return;
    }

    InterlockedIncrement64(&_this->m_llPacketCounter);

#ifdef SYSVAD_BTH_BYPASS
    if (_this->m_ScoOpen)
    {
        if (!NT_SUCCESS(_this->GetScoStreamNtStatus()))
        {
            return;
        }
    }
#endif  // SYSVAD_BTH_BYPASS

    _this->m_pMiniport->DpcRoutine(qpc.QuadPart, qpcFrequency.QuadPart);

    if (_this->m_KsState != KSSTATE_RUN)
    {
        return;
    }
    
    PADAPTERCOMMON  pAdapterComm = _this->m_pMiniport->GetAdapterCommObj();

    // Simple buffer underrun detection.
    if (!_this->IsCurrentWaveRTWritePositionUpdated())
    {
        //Event type: eMINIPORT_GLITCH_REPORT
        //Parameter 1: Current linear buffer position 
        //Parameter 2: the previous WaveRtBufferWritePosition that the drive received 
        //Parameter 3: major glitch code: 1:WaveRT buffer is underrun
        //Parameter 4: minor code for the glitch cause
        pAdapterComm->WriteEtwEvent(eMINIPORT_GLITCH_REPORT, 
                                    100,    // replace with the correct "Current linear buffer position"   
                                    _this->GetCurrentWaveRTWritePosition(),
                                    1,      // WaveRT buffer is underrun
                                    0); 
    }

    if (!IsListEmpty(&_this->m_NotificationList))
    {
        PLIST_ENTRY leCurrent = _this->m_NotificationList.Flink;
        while (leCurrent != &_this->m_NotificationList)
        {
            NotificationListEntry* nleCurrent = CONTAINING_RECORD( leCurrent, NotificationListEntry, ListEntry);
            //Event type: eMINIPORT_BUFFER_COMPLETE
            //Parameter 1: Current linear buffer position	
            //Parameter 2: the previous WaveRtBufferWritePosition that the drive received
            //Parameter 3: Data length completed	
            //Parameter 4:0
            pAdapterComm->WriteEtwEvent(eMINIPORT_BUFFER_COMPLETE, 
                                        100, // replace with the correct "Current linear buffer position"	
                                        _this->GetCurrentWaveRTWritePosition(), 	
                                        300, // repalce with the correct "Data length completed"
                                        0);  // always zero

            KeSetEvent(nleCurrent->NotificationEvent, 0, 0);

            leCurrent = leCurrent->Flink;
        }
    }
}
//=============================================================================

