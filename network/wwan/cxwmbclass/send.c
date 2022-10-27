/***************************************************************************

Copyright (c) 2010 Microsoft Corporation

Module Name:

    send.c

Abstract:

    This file contains
    1. Queuing of send NBLs
    2. Conversion of NBL to NTBs
    3. Completion of NBLs

Environment:

    kernel mode only

Notes:

    
Revision History:

    3/20/2010 : TriRoy : Changed queuing to use NblContext, added conversion routines.

Authors:

    BrianL
    TriRoy

****************************************************************************/




////////////////////////////////////////////////////////////////////////////////
//
//  INCLUDES
//
////////////////////////////////////////////////////////////////////////////////
#include "precomp.h"
#include "send.tmh"
#include "WMBClassTelemetry.h"



////////////////////////////////////////////////////////////////////////////////
//
//  DEFINES
//
////////////////////////////////////////////////////////////////////////////////
#define MBB_NDP_DATAGRAM_INDEX_INVALID ((ULONG)-1)




////////////////////////////////////////////////////////////////////////////////
//
//  TYPEDEFS
//
////////////////////////////////////////////////////////////////////////////////
typedef enum
{
    MbbNdpTypeNoCrc = 0,
    MbbNdpTypeCrc,
    MbbNdpTypeIps,
    MbbNdpTypeVendor_1, // Vendor with session id X (need not be 1)
    MbbNdpTypeVendor_2, // Vendor with session id Y (need not be X + 1)
    MbbNdpTypeVendor_3, 
    MbbNdpTypeVendor_Max, // Max 4 vendor sessions in one NTB
    MbbNdpTypeMax

} MBB_NDP_TYPE;

typedef struct _MBB_NB_CONTEXT
{
    PMDL                    DataStartMdl;
    PMDL                    DataEndMdl;
    PMDL                    PaddingMdl;
    PMDL                    ModifiedMdl;
    MDL                     OriginalMdl;
    PNPAGED_LOOKASIDE_LIST  NbLookasideList;

} MBB_NB_CONTEXT,
*PMBB_NB_CONTEXT;

typedef
__callback VOID
(*MBB_NBL_CLEANUP_CALLBACK)(
    __in    PNET_BUFFER_LIST    NetBufferList,
    __in    PVOID               Context
    );

typedef struct _MBB_NBL_CONTEXT
{
    //
    // Protected by send queue lock
    //
    LIST_ENTRY                  NblQLink;
    LIST_ENTRY                  DispatchQLink;
    PNET_BUFFER_LIST            NetBufferList;
    PNET_BUFFER                 CurrentNb;
    BOOLEAN                     Failed;
    ULONG                       NbTotalCount;
    ULONG                       NbDispatchCount;    
    ULONG                       SessionId;
    //
    // Protected by Interlock
    //
    ULONG                       NbCompleteCount;

    MBB_NBL_CLEANUP_CALLBACK    CleanupCallback;
    PVOID                       CleanupCallbackContext;
    PNPAGED_LOOKASIDE_LIST      NblLookasideList;
    PNPAGED_LOOKASIDE_LIST      NbLookasideList;

    //
    // State for Dss data
    //
    MBB_REQUEST_HANDLE          DssRequestHandle;
    ULONG                       DssSessionId;
    
    
} MBB_NBL_CONTEXT,
*PMBB_NBL_CONTEXT;

__inline
BOOLEAN
MbbNblContextIsDssData(
    __in    PMBB_NBL_CONTEXT    NblContext
    )
{
    if( NblContext->DssSessionId != (ULONG)(-1))
        return TRUE;
    else
        return FALSE;
}


typedef struct _MBB_NDP_HEADER_ENTRY
{
    MBB_NDP_TYPE        NdpType;
    ULONG               SessionId;
    ULONG               DatagramOffset;
    ULONG               DatagramLength;
    ULONG               NextEntryIndex;
    PNET_BUFFER         NetBuffer;
    PNET_BUFFER_LIST    NetBufferList;

} MBB_NDP_HEADER_ENTRY,
*PMBB_NDP_HEADER_ENTRY;

typedef struct _MBB_NTB_BUILD_CONTEXT
{
    LIST_ENTRY              NtbQLink;
    //
    // Read-only values
    //
    BOOLEAN                 IsNtb32Bit;
    ULONG                   NtbHeaderSize;
    ULONG                   NdpHeaderFixedSize;
    ULONG                   NdpDatagramEntrySize;
    ULONG                   NtbOutMaxSize;
    USHORT                  NtbOutMaxDatagrams;
    USHORT                  NdpOutDivisor;
    USHORT                  NdpOutPayloadRemainder;
    USHORT                  NdpOutAlignment;
    NDIS_HANDLE             MiniportHandle;
    PVOID                   PaddingBuffer;
    PNPAGED_LOOKASIDE_LIST  NtbLookasideList;
    //
    // Network Transfer Header(NTH)
    //
    union {
        NCM_NTH16           Nth16;
        NCM_NTH32           Nth32;
    };
    PMDL                    NthMdl;
    //
    // NDP Header
    //
    PMDL                    NdpMdl;
    ULONG                   NdpSize;
    PVOID                   NdpBuffer;
    //
    // NDP Datagrams
    //
    ULONG                   DatagramCount;
    ULONG                   DatagramLength;
    PMDL                    DatagramLastMdl;
    //
    // NDP Headers. Varialble length array is limited
    // to NdpMaxDatagrams. NdpFirstDatagramEntry of -1
    // is invalid.
    //
    ULONG                   NdpFirstDatagramEntry[MbbNdpTypeMax];
    MBB_NDP_HEADER_ENTRY    NdpDatagramEntries[ANYSIZE_ARRAY];

} MBB_NTB_BUILD_CONTEXT,
*PMBB_NTB_BUILD_CONTEXT;




////////////////////////////////////////////////////////////////////////////////
//
//  PROTOTYPES
//
////////////////////////////////////////////////////////////////////////////////

//
//  NTB Context
//

PMBB_NTB_BUILD_CONTEXT
MbbNtbAllocateContext(
    __in    NDIS_HANDLE             MiniportHandle,
    __in    PNPAGED_LOOKASIDE_LIST  NtbLookasideList,
    __in    PMBB_BUS_PARAMETERS     BusParams,
    __in    PVOID                   PaddingBuffer,
    __in    ULONG                   NtbSequence
    );

VOID
MbbNtbCleanupContext(
    __in    PMBB_NTB_BUILD_CONTEXT  NtbContext,
    __in    NTSTATUS                NtStatus
    );

NDIS_STATUS
MbbNtbAddNdpHeaders(
    __in PMBB_NTB_BUILD_CONTEXT   NtbContext
    );

ULONG
MbbNtbMapNdpTypeToSignature(
    __in MBB_NDP_TYPE   MbbNdpType,
    __in BOOLEAN        Is32Bit,
    __in ULONG          SessionId
    );

VOID
MbbNtbFillNdp32Header(
    __in PNCM_NDP32             Ndp,
    __in MBB_NDP_TYPE           NdpType,
    __in PMBB_NTB_BUILD_CONTEXT NtbContext
    );

VOID
MbbNtbFillNdp16Header(
    __in PNCM_NDP16             Ndp,
    __in MBB_NDP_TYPE           NdpType,
    __in PMBB_NTB_BUILD_CONTEXT NtbContext
    );

FORCEINLINE
PMDL
MbbNtbGetMdlChainHead(
    __in    PMBB_NTB_BUILD_CONTEXT  NtbContext
    );

NDIS_STATUS
MbbNtbAddNbl(
    __in PMBB_NTB_BUILD_CONTEXT     NtbContext,
    __in PNET_BUFFER_LIST           NetBufferList
    );

NDIS_STATUS
MbbNtbAddNb(
    __in PMBB_NTB_BUILD_CONTEXT NtbContext,
    __in PNPAGED_LOOKASIDE_LIST NbLookasideList,
    __in PNET_BUFFER            NetBuffer,
    __in PNET_BUFFER_LIST       NetBufferList,
    __in MBB_NDP_TYPE           CurrentNdpType,
    __in ULONG                  SessionId
    );

VOID
MbbNtbChainNb(
    __in PMBB_NTB_BUILD_CONTEXT NtbContext,
    __in PNET_BUFFER            NetBuffer
    );

// Test

NDIS_STATUS
MbbNtbValidate(
    __in    PVOID       Nth,
    __in    ULONG       BufferLength,
    __in    BOOLEAN     Is32Bit
    );

#if DBG

NDIS_STATUS
MbbTestValidateNtb(
    __in    PMBB_NTB_BUILD_CONTEXT      NtbContext,
    __in_bcount(ScratchLength) PCHAR    ScratchBuffer,
    __in    ULONG                       ScratchLength
    );
#endif

//
//  NB Context
//

PMBB_NB_CONTEXT
MbbNbAllocateContext(
    __in    PNET_BUFFER             NetBuffer,
    __in    ULONG                   DatagramLength,
    __in    PVOID                   PaddingBuffer,
    __in    ULONG                   PaddingLength,
    __in    PNPAGED_LOOKASIDE_LIST  NbLookasideList,
    __in    NDIS_HANDLE             MiniportHandle
    );

VOID
MbbNbCleanupContext(
    __in PNET_BUFFER    NetBuffer
    );

FORCEINLINE
PMDL
MbbNbGetFirstMdl(
    __in PNET_BUFFER    NetBuffer
    );

FORCEINLINE
PMDL
MbbNbGetLastMdl(
    __in PNET_BUFFER    NetBuffer
    );

VOID
MbbNbSaveAndSetMdl(
    __in PNET_BUFFER    NetBuffer,
    __in PMDL           MdlToSave,
    __in PMDL           MdlToSet
    );

VOID
MbbNbRestoreMdl(
    __in PNET_BUFFER    NetBuffer
    );

//
//  NBL Context
//

PMBB_NBL_CONTEXT
MbbNblAllocateContext(
    __in    PNET_BUFFER_LIST            NetBufferList,
    __in    PNPAGED_LOOKASIDE_LIST      NblLookasideList,
    __in    PNPAGED_LOOKASIDE_LIST      NbLookasideList,
    __in    MBB_NBL_CLEANUP_CALLBACK    CleanupCallback,
    __in    PVOID                       CleanupCallbackContext
    );

VOID
MbbNblFinalizeContext(
    __in    PNET_BUFFER_LIST   NetBufferList,
    __in    ULONG              NbCount,
    __in    NTSTATUS           NtStatus
    );

FORCEINLINE
PNET_BUFFER
MbbNblGetNextDispatchNb(
    __in    PNET_BUFFER_LIST    NetBufferList
    );

FORCEINLINE
PNET_BUFFER
MbbNblAdvanceDispatchNb(
    __in    PNET_BUFFER_LIST    NetBufferList
    );

//
// Send Queue
//

_Acquires_lock_( SendQueue->Lock )
__drv_raisesIRQL(DISPATCH_LEVEL)
__drv_maxIRQL(DISPATCH_LEVEL)
__drv_savesIRQLGlobal( NdisSpinLock, SendQueue )
FORCEINLINE
VOID
MbbSendQLock(
    __in    PMBB_SEND_QUEUE         SendQueue,
    __in    BOOLEAN                 DispatchLevel
    );

_Releases_lock_( SendQueue->Lock )
__drv_maxIRQL(DISPATCH_LEVEL)
__drv_minIRQL(DISPATCH_LEVEL)
__drv_restoresIRQLGlobal( NdisSpinLock, SendQueue )
FORCEINLINE
VOID
MbbSendQUnlock(
    __in    PMBB_SEND_QUEUE         SendQueue,
    __in    BOOLEAN                 DispatchLevel
    );

FORCEINLINE
VOID
MbbSendQResetNtbSequence(
    __in PMBB_SEND_QUEUE            SendQueue
    );

FORCEINLINE
ULONG
MbbSendQGetNtbSequence(
    __in PMBB_SEND_QUEUE            SendQueue
    );

VOID
MbbSendQProcess(
    __in  PMBB_SEND_QUEUE           SendQueue,
    __in  BOOLEAN                   DispatchLevel
    );

_Requires_lock_held_(SendQueue->Lock)
NDIS_STATUS
MbbSendQQueueNbl(
    __in    PMBB_SEND_QUEUE         SendQueue,
    __in    PNET_BUFFER_LIST        NetBufferList,    
    __in    ULONG                   SessionId
    );

VOID
MbbSendQDequeueNbl(
    __in    PNET_BUFFER_LIST        NetBufferList,
    __in    PVOID                   Context
    );

VOID
MbbSendQCompleteNtb(
    __in    MBB_PROTOCOL_HANDLE     ProtocolHandle,
    __in    MBB_REQUEST_HANDLE      RequestHandle,
    __in    NTSTATUS                NtStatus,
    __in    PMDL                    Mdl
    );

VOID
MbbSendQCancelRequests(
    __in  PMBB_SEND_QUEUE   SendQueue,
    __in  NDIS_STATUS       Status,
    __in  BOOLEAN           WaitForCompletion,
    __in  BOOLEAN           ExternalDataOnly
    );

_Requires_lock_held_(SendQueue->Lock)
NDIS_STATUS
MbbSendQQueueDssData(
    __in    PMINIPORT_ADAPTER_CONTEXT Adapter,
    __in    PMBB_SEND_QUEUE         SendQueue,
    __in    MBB_REQUEST_HANDLE      RequestHandle,
    __in    ULONG                   RequestId,
    __in    ULONG                   SessionId,
    __in    ULONG                   DataSize,
    __in    PVOID                   Data
    );

VOID
MbbSendQDequeueDssData(
    __in    PNET_BUFFER_LIST        NetBufferList,
    __in    PVOID                   Context
    );

////////////////////////////////////////////////////////////////////////////////
//
//  IMPLEMENTATION
//
////////////////////////////////////////////////////////////////////////////////
_Acquires_lock_( SendQueue->Lock )
__drv_raisesIRQL(DISPATCH_LEVEL)
__drv_maxIRQL(DISPATCH_LEVEL)
__drv_savesIRQLGlobal( NdisSpinLock, SendQueue )
FORCEINLINE
VOID
MbbSendQLock(
    __in    PMBB_SEND_QUEUE SendQueue,
    __in    BOOLEAN             DispatchLevel
    )
{
    if( DispatchLevel == TRUE )
    {
        NdisDprAcquireSpinLock( &SendQueue->Lock );
    }
    else
    {
        NdisAcquireSpinLock( &SendQueue->Lock );
    }
}

_Releases_lock_( SendQueue->Lock )
__drv_maxIRQL(DISPATCH_LEVEL)
__drv_minIRQL(DISPATCH_LEVEL)
__drv_restoresIRQLGlobal( NdisSpinLock, SendQueue )
FORCEINLINE
VOID
MbbSendQUnlock(
    __in    PMBB_SEND_QUEUE SendQueue,
    __in    BOOLEAN             DispatchLevel
    )
{
    if( DispatchLevel == TRUE )
    {
        NdisDprReleaseSpinLock( &SendQueue->Lock );
    }
    else
    {
        NdisReleaseSpinLock( &SendQueue->Lock );
    }
}

NDIS_STATUS
MbbSendQInitialize(
    __in  PMBB_SEND_QUEUE           SendQueue,
    __in  ULONG                     MaxConcurrentSends,
    __in  MBB_DRAIN_COMPLETE        DrainCompleteCallback,
    __in  PVOID                     DrainCompleteCallbackContext,
    __in  PMINIPORT_ADAPTER_CONTEXT Adapter,
    __in  MBB_BUS_HANDLE            BusHandle,
    __in  NDIS_HANDLE               MiniportHandle
    )
{
    ULONG       PaddingLength;
    ULONG       NtbSize;
    NDIS_STATUS NdisStatus = NDIS_STATUS_SUCCESS;
    NET_BUFFER_POOL_PARAMETERS      NbPoolParameters;
    NET_BUFFER_LIST_POOL_PARAMETERS NblPoolParameters;

    do
    {
        RtlZeroMemory( SendQueue, sizeof(MBB_SEND_QUEUE) );

        NdisAllocateSpinLock( &SendQueue->Lock );
        InitializeListHead( &SendQueue->NtbQueue );
        InitializeListHead( &SendQueue->NblTrackList );
        InitializeListHead( &SendQueue->NblDispatchQueue );
 
        KeInitializeEvent(
            &SendQueue->NblQueueEmptyEvent,
            NotificationEvent,
            TRUE
            );
        PaddingLength = MAX( Adapter->BusParams.NdpOutAlignment, Adapter->BusParams.NdpOutDivisor );
        if( (SendQueue->PaddingBuffer = ALLOCATE_NONPAGED_POOL( PaddingLength )) == NULL )
        {
            NdisStatus = NDIS_STATUS_RESOURCES;
            break;
        }
#if DBG
        SendQueue->ScratchLength = Adapter->BusParams.MaxOutNtb;
        if( (SendQueue->ScratchBuffer = (PCHAR)ALLOCATE_NONPAGED_POOL( SendQueue->ScratchLength )) == NULL )
        {
            NdisStatus = NDIS_STATUS_RESOURCES;
            break;
        }
#endif

        RtlZeroMemory( SendQueue->PaddingBuffer, PaddingLength );

        SendQueue->NblPool          = NULL;

        NblPoolParameters.Header.Type           = NDIS_OBJECT_TYPE_DEFAULT;
        NblPoolParameters.Header.Revision       = NET_BUFFER_LIST_POOL_PARAMETERS_REVISION_1;
        NblPoolParameters.Header.Size           = NDIS_SIZEOF_NET_BUFFER_LIST_POOL_PARAMETERS_REVISION_1;
        NblPoolParameters.ProtocolId            = NDIS_PROTOCOL_ID_DEFAULT;
        NblPoolParameters.fAllocateNetBuffer    = TRUE;
        NblPoolParameters.ContextSize           = 0;
        NblPoolParameters.PoolTag               = MbbPoolTagNblPool;
        NblPoolParameters.DataSize              = 0;

        if( (SendQueue->NblPool = NdisAllocateNetBufferListPool(
                                        MiniportHandle,
                                        &NblPoolParameters
                                        )) == NULL )
        {
            TraceError( WMBCLASS_SEND, "[Send] FAILED to allocate NetBufferListPool" );
            NdisStatus = NDIS_STATUS_RESOURCES;
            break;
        }
        
        SendQueue->MaxConcurrentSends   = MaxConcurrentSends;
        SendQueue->AdapterContext       = Adapter;
        SendQueue->BusHandle            = BusHandle;

        NtbSize  = FIELD_OFFSET(MBB_NTB_BUILD_CONTEXT,NdpDatagramEntries);
        NtbSize += (Adapter->BusParams.MaxOutDatagrams * sizeof(MBB_NDP_HEADER_ENTRY));
        NdisInitializeNPagedLookasideList(
            &SendQueue->NtbLookasideList,
            NULL,
            NULL,
            0,  // Flags
            NtbSize,
            MbbPoolTagNtbSend,
            0   // Depth
            );
        NdisInitializeNPagedLookasideList(
            &SendQueue->NblLookasideList,
            NULL,
            NULL,
            0,  // Flags
            sizeof(MBB_NBL_CONTEXT),
            MbbPoolTagNblSend,
            0   // Depth
            );
        NdisInitializeNPagedLookasideList(
            &SendQueue->NbLookasideList,
            NULL,
            NULL,
            0,  // Flags
            sizeof(MBB_NB_CONTEXT),
            MbbPoolTagNbSend,
            0   // Depth
            );
        InitDrainObject(
            &SendQueue->QueueDrainObject,
            DrainCompleteCallback,
            DrainCompleteCallbackContext
            );
        TraceInfo( WMBCLASS_SEND, "[Send] Initialization complete" );
    }
    while( FALSE );

    if( NdisStatus != NDIS_STATUS_SUCCESS )
    {
        if( SendQueue->NblPool != NULL )
        {
            NdisFreeNetBufferListPool( SendQueue->NblPool );
            SendQueue->NblPool = NULL;
        }
    
        if( SendQueue->PaddingBuffer != NULL )
            FREE_POOL( SendQueue->PaddingBuffer );

#if DBG
        if( SendQueue->ScratchBuffer != NULL )
            FREE_POOL( SendQueue->ScratchBuffer );
#endif

    }
    return NdisStatus;
}

VOID
MbbSendQCleanup(
    __in  PMBB_SEND_QUEUE       SendQueue
    )
/*++
    Description
        Frees all resources allocated for the queue.
        This is called from MiniportHalt so the queue should
        already be empty. This routine does not call cancel
        or wait for pending requests to complete since there
        should not be any.
--*/
{
    ASSERT( IsListEmpty( &SendQueue->NblTrackList ) == TRUE );

    NdisDeleteNPagedLookasideList( &SendQueue->NtbLookasideList );
    NdisDeleteNPagedLookasideList( &SendQueue->NblLookasideList );
    NdisDeleteNPagedLookasideList( &SendQueue->NbLookasideList );

    if( SendQueue->PaddingBuffer != NULL )
        FREE_POOL( SendQueue->PaddingBuffer );

#if DBG
    if( SendQueue->ScratchBuffer != NULL )
        FREE_POOL( SendQueue->ScratchBuffer );
#endif

    if( SendQueue->NblPool != NULL )
    {
        NdisFreeNetBufferListPool( SendQueue->NblPool );
        SendQueue->NblPool = NULL;
    }

    TraceInfo( WMBCLASS_SEND, "[Send] Cleanup complete" );
}

VOID
MbbSendQCancel(
    __in  PMBB_SEND_QUEUE   SendQueue,
    __in  NDIS_STATUS       Status,
    __in  BOOLEAN           WaitForCompletion
    )
{
    StartDrain( &SendQueue->QueueDrainObject );

    //
    // NBLs
    //
    MbbSendQCancelRequests(
        SendQueue,
        Status,
        WaitForCompletion,
        FALSE
        );

    TraceInfo( WMBCLASS_SEND, "[Send] Cancel complete" );
}

VOID
MbbSendQCancelRequests(
    __in  PMBB_SEND_QUEUE   SendQueue,
    __in  NDIS_STATUS       Status,
    __in  BOOLEAN           WaitForCompletion,
    __in  BOOLEAN           ExternalDataOnly
    )
{
    PLIST_ENTRY             ListEntry = NULL;
    PLIST_ENTRY             NextEntry = NULL;
    LIST_ENTRY              TempList;
    PLIST_ENTRY             QueueHead;
    PKEVENT                 QueueEmptyEvent;
    PMBB_NBL_CONTEXT        NblContext;
    PMBB_REQUEST_CONTEXT    Request;

    TraceInfo( WMBCLASS_SEND, "[Send] Cancelling %s requests", (ExternalDataOnly? "External": "All") );

    QueueHead       = &SendQueue->NblDispatchQueue;
    QueueEmptyEvent = &SendQueue->NblQueueEmptyEvent;

    InitializeListHead( &TempList );

    MbbSendQLock( SendQueue, FALSE );
    for( ListEntry  = QueueHead->Flink;
         ListEntry != QueueHead;
         ListEntry  = NextEntry )
    {
        NextEntry = ListEntry->Flink;
        RemoveEntryList( ListEntry );
        InsertTailList( &TempList, ListEntry );
    }
    MbbSendQUnlock( SendQueue, FALSE );

    for( ListEntry  = TempList.Flink;
         ListEntry != &TempList;
         ListEntry  = NextEntry )
    {
        NextEntry = ListEntry->Flink;
        RemoveEntryList( ListEntry );
        NblContext = CONTAINING_RECORD( ListEntry, MBB_NBL_CONTEXT, DispatchQLink );
        MbbNblFinalizeContext(
            NblContext->NetBufferList,
            NblContext->NbTotalCount - NblContext->NbDispatchCount,
            Status
            );
    }

    if( WaitForCompletion )
    {
        TraceInfo( WMBCLASS_SEND, "[Send] Cancel, waiting for %s requests to complete", (ExternalDataOnly? "External": "All") );

        KeWaitForSingleObject(
            QueueEmptyEvent,
            Executive,
            KernelMode,
            FALSE,
            NULL
            );
    }
}

VOID
MbbSendQProcess(
    __in  PMBB_SEND_QUEUE       SendQueue,
    __in  BOOLEAN               DispatchLevel
    )
{
    NTSTATUS                    NtStatus;
    NDIS_STATUS                 NdisStatus;
    BOOLEAN                     AbortDataQueue;
    BOOLEAN                     CompleteNBs;
    PMBB_NBL_CONTEXT            NblContext;
    PMBB_NTB_BUILD_CONTEXT      NtbContext;
    PMBB_REQUEST_CONTEXT        Request;

    //
    // Synchronize with other instances of this routine called from completion handlers.
    //
    MbbSendQLock( SendQueue, DispatchLevel );
    if( SendQueue->ProcessingQueue != FALSE )
    {
        MbbSendQUnlock( SendQueue, DispatchLevel );
        return;
    }
    SendQueue->ProcessingQueue = TRUE;

    while(  SendQueue->ConcurrentSends < SendQueue->MaxConcurrentSends &&
            ! IsListEmpty( &SendQueue->NblDispatchQueue ))
    {
        //
        // Process the data queue
        // Try to add as many NBLs as possible to the NTB
        //
        NtbContext     = NULL;
        NdisStatus     = NDIS_STATUS_SUCCESS;
        AbortDataQueue = FALSE;

        while( IsListEmpty(&SendQueue->NblDispatchQueue) == FALSE &&
               NdisStatus == NDIS_STATUS_SUCCESS )
        {
            CompleteNBs = FALSE;

            if( NtbContext == NULL )
            {
                if( (NtbContext = MbbNtbAllocateContext(
                                    SendQueue->AdapterContext->MiniportAdapterHandle,
                                    &SendQueue->NtbLookasideList,
                                    &SendQueue->AdapterContext->BusParams,
                                    SendQueue->PaddingBuffer,
                                    MbbSendQGetNtbSequence( SendQueue )
                                    )) == NULL )
                {
                    TraceError( WMBCLASS_SEND, "[Send] FAILED to allocate NTB" );
                    AbortDataQueue = TRUE;
                    NdisStatus = NDIS_STATUS_RESOURCES;
                    break;
                }
                InsertTailList( &SendQueue->NtbQueue, &NtbContext->NtbQLink );
                SendQueue->ConcurrentSends++;
            }
            NblContext = CONTAINING_RECORD(
                            RemoveHeadList( &SendQueue->NblDispatchQueue ),
                            MBB_NBL_CONTEXT,
                            DispatchQLink
                            );
            //
            // Drop lock for send processing
            //
            MbbSendQUnlock( SendQueue, DispatchLevel );
            //
            // Add NetBuffers from the NetBufferList to the NTB
            //
            if( (NdisStatus = MbbNtbAddNbl(
                                NtbContext,
                                NblContext->NetBufferList
                                )) == NDIS_STATUS_SUCCESS )
            {
                //
                // If all NBs in the current NBL can be
                // added to the NTB, then try another NBL.
                //
            }
            else if( NdisStatus == NDIS_STATUS_BUFFER_OVERFLOW )
            {
                //
                // If the NTB was empty and this NBL couldnt be added
                // then fail the NBL to prevent retrying forever.
                // If all NBs in the current NBL couldnt be
                // added then requeue the NBL to the head of the queue.
                //
                if( NtbContext->DatagramCount == 0 )
                {
                    TraceError( WMBCLASS_SEND, "[Send][Seq=0x%04x][NBL=0x%p] FAILED to add NBL to empty NTB, status=%!STATUS!",
                                MBB_NTB_GET_SEQUENCE( &NtbContext->Nth32 ),
                                NblContext->NetBufferList,
                                NdisStatus
                                );
                    CompleteNBs = TRUE;
                }
                else
                {
                    MbbSendQLock( SendQueue, DispatchLevel );
                    InsertHeadList( &SendQueue->NblDispatchQueue, &NblContext->DispatchQLink );
                    MbbSendQUnlock( SendQueue, DispatchLevel );
                }
            }
            else
            {
                TraceError( WMBCLASS_SEND, "[Send][Seq=0x%04x][NBL=0x%p] FAILED to send NBL, status=%!STATUS!",
                            MBB_NTB_GET_SEQUENCE( &NtbContext->Nth32 ),
                            NblContext->NetBufferList,
                            NdisStatus
                            );
                CompleteNBs = TRUE;
            }
            //
            // If the call failed for complete the remaining NBs with failure.
            //
            if( CompleteNBs )
            {
                MbbNblFinalizeContext(
                    NblContext->NetBufferList,
                    NblContext->NbTotalCount - NblContext->NbDispatchCount,
                    NdisStatus
                    );
            }
            //
            // Reacquire lock for re-evaluating the send queue.
            //
            MbbSendQLock( SendQueue, DispatchLevel );
        }
        MbbSendQUnlock( SendQueue, DispatchLevel );
        //
        // Send this NTB to the bus and continue processing other requests.
        //
        if( NtbContext != NULL )
        {
            if( NtbContext->DatagramCount > 0 )
            {
                MbbNtbAddNdpHeaders( NtbContext );
#if DBG
                if( MbbTestValidateNtb(
                        NtbContext,
                        SendQueue->ScratchBuffer,
                        SendQueue->ScratchLength
                        ) != NDIS_STATUS_SUCCESS )
                {
                    ASSERT( FALSE );
                    TraceError( WMBCLASS_SEND, "[Send][Seq=0x%04x] FAILED NTB validation, sending anyway", MBB_NTB_GET_SEQUENCE( &NtbContext->Nth32 ) );
                }
#endif
                //
                // Send the data. On failure, cleanup.
                //
                NtStatus = MbbBusWriteData(
                                SendQueue->BusHandle,
                                NtbContext,
                                MbbNtbGetMdlChainHead( NtbContext ),
                                MbbSendQCompleteNtb
                                );
            }
            else
            {
                TraceError( WMBCLASS_SEND, "[Send][Seq=0x%04x] NTB has no datagrams", MBB_NTB_GET_SEQUENCE( &NtbContext->Nth32 ) );
                NtStatus = STATUS_UNSUCCESSFUL;
            }

            if( ! NT_SUCCESS( NtStatus ) )
            {
                MbbSendQCompleteNtb(
                    SendQueue->AdapterContext,
                    NtbContext,
                    NtStatus,
                    MbbNtbGetMdlChainHead( NtbContext )
                    );
            }
        }
        if( AbortDataQueue == TRUE )
        {
            MbbSendQCancel( SendQueue, NdisStatus, FALSE );
        }
        //
        // Reacquire lock for re-evaluating the send queue.
        //
        MbbSendQLock( SendQueue, DispatchLevel );
    }
    //
    // Processing complete
    //
    SendQueue->ProcessingQueue = FALSE;
    MbbSendQUnlock( SendQueue, DispatchLevel );
}

_Requires_lock_held_(SendQueue->Lock)
NDIS_STATUS
MbbSendQQueueNbl(
    __in    PMBB_SEND_QUEUE         SendQueue,
    __in    PNET_BUFFER_LIST        NetBufferList,    
    __in    ULONG                   SessionId
    )
{
    PMBB_NBL_CONTEXT    NblContext;

    if( (NblContext = MbbNblAllocateContext(
                        NetBufferList,
                        &SendQueue->NblLookasideList,
                        &SendQueue->NbLookasideList,
                        MbbSendQDequeueNbl,
                        SendQueue
                        )) == NULL )
    {
        TraceError( WMBCLASS_SEND, "[Send][NBL=0x%p] FAILED to allocated context", NetBufferList );
        return NDIS_STATUS_RESOURCES;
    }
    
    // Set the session Id
    NblContext->SessionId = SessionId;

    if( IsListEmpty( &SendQueue->NblTrackList ) )
        KeResetEvent( &SendQueue->NblQueueEmptyEvent );

    InsertTailList( &SendQueue->NblTrackList, &NblContext->NblQLink );
    InsertTailList( &SendQueue->NblDispatchQueue, &NblContext->DispatchQLink );

    MbbWriteEvent(
        &NBL_QUEUED_EVENT,
        NULL,
        NULL,
        2,
        &SendQueue->AdapterContext->TraceInstance,
        sizeof(SendQueue->AdapterContext->TraceInstance),
        &NetBufferList,
        sizeof(NetBufferList)
        );

    return NDIS_STATUS_SUCCESS;
}

VOID
MbbSendQDequeueNbl(
    __in    PNET_BUFFER_LIST        NetBufferList,
    __in    PVOID                   Context
    )
{
    PMBB_SEND_QUEUE     SendQueue = (PMBB_SEND_QUEUE)Context;
    PMBB_NBL_CONTEXT    NblContext = (PMBB_NBL_CONTEXT)NET_BUFFER_LIST_MINIPORT_RESERVED( NetBufferList )[0];

    NET_BUFFER_LIST_MINIPORT_RESERVED( NetBufferList )[0] = NULL;

    if( NetBufferList->Status != NDIS_STATUS_SUCCESS )
    {
        TraceError( WMBCLASS_SEND, "[Send][NBL=0x%p] Completed with failed NdisStatus=%!STATUS!",
                    NetBufferList,
                    NetBufferList->Status
                    );
    }
    else
    {
        PNET_BUFFER         NetBuffer;
        for( NetBuffer  = NET_BUFFER_LIST_FIRST_NB( NetBufferList );
             NetBuffer != NULL;
             NetBuffer  = NET_BUFFER_NEXT_NB( NetBuffer ) )
        {
            InterlockedAdd64(&SendQueue->AdapterContext->Stats.ifHCOutOctets, NET_BUFFER_DATA_LENGTH(NetBuffer));
            InterlockedIncrement64(&SendQueue->AdapterContext->GenXmitFramesOk);
        }
    }

    NET_BUFFER_LIST_MINIPORT_RESERVED( NetBufferList )[0] = NULL;
    NET_BUFFER_LIST_MINIPORT_RESERVED( NetBufferList )[1] = NblContext;

    MbbWriteEvent(
        &NBL_COMPLETED_EVENT,
        NULL,
        NULL,
        3,
        &SendQueue->AdapterContext->TraceInstance,
        sizeof(SendQueue->AdapterContext->TraceInstance),
        &NetBufferList,
        sizeof(NetBufferList),
        &NetBufferList->Status,
        sizeof(NetBufferList->Status)
        );

    NdisMSendNetBufferListsComplete(
        SendQueue->AdapterContext->MiniportAdapterHandle,
        NetBufferList,
        0
        );
    DrainRelease( &SendQueue->QueueDrainObject );

    MbbSendQLock(SendQueue, FALSE);
    if (RemoveEntryList(&NblContext->NblQLink))
        KeSetEvent(&SendQueue->NblQueueEmptyEvent, 0, FALSE);
    MbbSendQUnlock(SendQueue, FALSE);
}

VOID
MbbSendQCompleteNtb(
    __in    MBB_PROTOCOL_HANDLE     ProtocolHandle,
    __in    MBB_REQUEST_HANDLE      RequestHandle,
    __in    NTSTATUS                NtStatus,
    __in    PMDL                    Mdl
    )
{
    PMBB_SEND_QUEUE         SendQueue = &((PMINIPORT_ADAPTER_CONTEXT)ProtocolHandle)->SendQueue;
    PMBB_NTB_BUILD_CONTEXT  NtbContext = (PMBB_NTB_BUILD_CONTEXT)RequestHandle;
    PSTATE_CHANGE_EVENT     StateChange=NULL;

    UNREFERENCED_PARAMETER( Mdl );

    if (!NT_SUCCESS(NtStatus))
    {
        if (NtStatus == STATUS_NDIS_ADAPTER_NOT_READY)
        {
            TraceError(WMBCLASS_SEND, "%!FUNC!: MbbSendQCompleteNtb failed because the data pipes is being reset");
        }
        else if (NtStatus == STATUS_CANCELLED)
        {
            TraceWarn(WMBCLASS_SEND, "%!FUNC!: MbbSendQCompleteNtb failed because the request is cancelled");
        }
        else
        {
            if (InterlockedExchange((LONG*)&SendQueue->LastDataPathTelemetryStatus, MbbDataPathTelmetryStatusReportHang) != MbbDataPathTelmetryStatusReportHang)
            {
                TraceLoggingWrite(
                    g_hLoggingProvider,
                    "WmbclassTxDataPathStatus",
                    TraceLoggingKeyword(MICROSOFT_KEYWORD_MEASURES),
                    TraceLoggingWideString(L"Hung", "Status")
                );
            }
            TraceError(WMBCLASS_SEND, "%!FUNC!: MbbSendQCompleteNtb failed with %!STATUS!, data pipe will restart", NtStatus);
            TryQueueStallState(SendQueue);
        }
    }
    else
    {
        if (InterlockedExchange((LONG*)&SendQueue->LastDataPathTelemetryStatus, MbbDataPathTelmetryStatusReportSuccess) != MbbDataPathTelmetryStatusReportSuccess)
        {
            TraceLoggingWrite(
                g_hLoggingProvider,
                "WmbclassTxDataPathStatus",
                TraceLoggingKeyword(MICROSOFT_KEYWORD_MEASURES),
                TraceLoggingWideString(L"Success", "Status")
            );
        }
    }

    MbbSendQLock( SendQueue, FALSE );
    RemoveEntryList( &NtbContext->NtbQLink );
    SendQueue->ConcurrentSends--;
    MbbSendQUnlock( SendQueue, FALSE );

    MbbNtbCleanupContext(
        NtbContext,
        NtStatus
        );
    MbbSendQProcess( SendQueue, FALSE );
}


//
// TODO: Needs to be called from Adapter reset
//
FORCEINLINE
VOID
MbbSendQResetNtbSequence(
    __in PMBB_SEND_QUEUE SendQueue
    )
{
    InterlockedExchange( &SendQueue->NtbSequence, 0 );
}

FORCEINLINE
ULONG
MbbSendQGetNtbSequence(
    __in PMBB_SEND_QUEUE SendQueue
    )
{
    return InterlockedIncrement( &SendQueue->NtbSequence );
}

//
//  NTB Context
//

PMBB_NTB_BUILD_CONTEXT
MbbNtbAllocateContext(
    __in    NDIS_HANDLE             MiniportHandle,
    __in    PNPAGED_LOOKASIDE_LIST  NtbLookasideList,
    __in    PMBB_BUS_PARAMETERS     BusParams,
    __in    PVOID                   PaddingBuffer,
    __in    ULONG                   NtbSequence
    )
{
    MBB_NDP_TYPE            NdpType;
    ULONG                   DatagramEntryIndex;
    NDIS_STATUS             NdisStatus = NDIS_STATUS_SUCCESS;
    ULONG                   NtbSize;
    PMBB_NTB_BUILD_CONTEXT  NtbContext;

    do
    {
        NtbSize  = FIELD_OFFSET(MBB_NTB_BUILD_CONTEXT,NdpDatagramEntries);
        NtbSize += (BusParams->MaxOutDatagrams * sizeof(MBB_NDP_HEADER_ENTRY));
#pragma prefast(suppress:__WARNING_MEMORY_LEAK, "By Design: Allocate ntb context from lookaside pool, released when send queue completes.")
        if( (NtbContext = ALLOCATE_LOOKASIDE( NtbLookasideList )) == NULL )
        {
            NdisStatus = NDIS_STATUS_RESOURCES;
            break;
        }
        RtlZeroMemory( NtbContext, NtbSize );
        NtbContext->MiniportHandle   = MiniportHandle;
        NtbContext->PaddingBuffer    = PaddingBuffer;
        NtbContext->NtbLookasideList = NtbLookasideList;
        //
        // Initialize the NTH and the NTH MDL.
        //
        if( (NtbContext->IsNtb32Bit = BusParams->CurrentMode32Bit) == TRUE )
        {
            if( (NtbContext->NthMdl = NdisAllocateMdl(
                                        MiniportHandle,
                                        &NtbContext->Nth32,
                                        sizeof(NtbContext->Nth32)
                                        )) == NULL )
            {
                NdisStatus = NDIS_STATUS_RESOURCES;
                break;
            }
            NtbContext->Nth32.dwSignature   = NCM_NTH32_SIG;
            NtbContext->Nth32.wHeaderLength = sizeof(NCM_NTH32);
            NtbContext->Nth32.wSequence     = (USHORT)NtbSequence;

            NtbContext->NtbHeaderSize        = sizeof(NCM_NTH32);
            NtbContext->NdpHeaderFixedSize   = sizeof(NCM_NDP32);
            NtbContext->NdpDatagramEntrySize = sizeof(NCM_NDP32_DATAGRAM);
        }
        else
        {
            if( (NtbContext->NthMdl = NdisAllocateMdl(
                                        MiniportHandle,
                                        &NtbContext->Nth16,
                                        sizeof(NtbContext->Nth16)
                                        )) == NULL )
            {
                NdisStatus = NDIS_STATUS_RESOURCES;
                break;
            }
            NtbContext->Nth16.dwSignature   = NCM_NTH16_SIG;
            NtbContext->Nth16.wHeaderLength = sizeof(NCM_NTH16);
            NtbContext->Nth16.wSequence     = (USHORT)NtbSequence;

            NtbContext->NtbHeaderSize        = sizeof(NCM_NTH16);
            NtbContext->NdpHeaderFixedSize   = sizeof(NCM_NDP16);
            NtbContext->NdpDatagramEntrySize = sizeof(NCM_NDP16_DATAGRAM);
        }
        //
        // Initialize NextEntryIndex
        //
        for( NdpType = 0;
             NdpType < MbbNdpTypeMax;
             NdpType++ )
        {
            NtbContext->NdpFirstDatagramEntry[NdpType] = MBB_NDP_DATAGRAM_INDEX_INVALID;
        }
        for( DatagramEntryIndex = 0;
             DatagramEntryIndex < BusParams->MaxOutDatagrams;
             DatagramEntryIndex ++ )
        {
            NtbContext->NdpDatagramEntries[DatagramEntryIndex].NextEntryIndex = MBB_NDP_DATAGRAM_INDEX_INVALID;
        }

        NtbContext->NtbOutMaxSize           = BusParams->MaxOutNtb;
        NtbContext->NtbOutMaxDatagrams      = BusParams->MaxOutDatagrams;
        NtbContext->NdpOutDivisor           = BusParams->NdpOutDivisor;
        NtbContext->NdpOutPayloadRemainder  = BusParams->NdpOutRemainder;
        NtbContext->NdpOutAlignment         = BusParams->NdpOutAlignment;
    }
    while( FALSE );

    if( NdisStatus != NDIS_STATUS_SUCCESS )
    {
        if( NtbContext != NULL )
        {
            MbbNtbCleanupContext( NtbContext, NdisStatus );
        }
        NtbContext = NULL;
    }
    return NtbContext;
}

VOID
MbbNtbCleanupContext(
    __in    PMBB_NTB_BUILD_CONTEXT  NtbContext,
    __in    NTSTATUS                NtStatus
    )
{
    ULONG                   DatagramIndex;
    PNET_BUFFER             NetBuffer;
    PNET_BUFFER_LIST        NetBufferList;

    for( DatagramIndex = 0;
         DatagramIndex < NtbContext->DatagramCount;
         DatagramIndex ++ )
    {
        NetBuffer     = NtbContext->NdpDatagramEntries[DatagramIndex].NetBuffer;
        NetBufferList = NtbContext->NdpDatagramEntries[DatagramIndex].NetBufferList;

        MbbNbCleanupContext( NetBuffer );
        MbbNblFinalizeContext( NetBufferList, 1, NtStatus );
    }
    if( NtbContext->NdpMdl != NULL )
    {
        NdisFreeMdl( NtbContext->NdpMdl );
    }
    if( NtbContext->NthMdl != NULL )
    {
        NdisFreeMdl( NtbContext->NthMdl );
    }
    if( NtbContext->NdpBuffer != NULL )
    {
        FREE_POOL( NtbContext->NdpBuffer );
    }
    FREE_LOOKASIDE( NtbContext, NtbContext->NtbLookasideList );
}

NDIS_STATUS
MbbNtbAddNbl(
    __in    PMBB_NTB_BUILD_CONTEXT  NtbContext,
    __in    PNET_BUFFER_LIST        NetBufferList
    )
{
    ULONG                   NbIndex;
    NDIS_STATUS             NdisStatus = NDIS_STATUS_SUCCESS;
    PNET_BUFFER             NetBuffer;
    MBB_NDP_TYPE            CurrentNdpType;
    PMBB_NBL_CONTEXT        NblContext = NET_BUFFER_LIST_MINIPORT_RESERVED(NetBufferList)[0];
    ULONG                   SessionId = MBB_DEFAULT_SESSION_ID;
    do
    {
        //
        // Pick the NDP type. NoCRC, CRC are not yet used.
        //
        if (MbbNblContextIsDssData(NblContext))
        {
            SessionId = NblContext->DssSessionId;
            
            // Vendor NDP. Determine if we are already have a NDP for
            // this session ID. If yes, add this datagram into that NDP. Else
            // assign new NDP for this datagram
            for( CurrentNdpType = MbbNdpTypeVendor_1;
                 CurrentNdpType <= MbbNdpTypeVendor_Max;
                 CurrentNdpType ++ )
            {
                if( NtbContext->NdpFirstDatagramEntry[CurrentNdpType] != MBB_NDP_DATAGRAM_INDEX_INVALID )
                {
                    // Entry is used. Does it match the session ID I am using?
                    if (NtbContext->NdpDatagramEntries[NtbContext->NdpFirstDatagramEntry[CurrentNdpType]].SessionId
                         == SessionId)
                    {
                        // Matching session ID. We can use this type
                        break;
                    }
                }
                else
                {
                    // Entry is not used. We will use it
                    break;
                }
            }

            if (CurrentNdpType > MbbNdpTypeVendor_Max)
            {
                // Didnt find an empty slot in this NTB
                NdisStatus = NDIS_STATUS_BUFFER_OVERFLOW;
                break;
            }
        }
        else
        {
            CurrentNdpType = MbbNdpTypeIps;   
            
            SessionId = NblContext->SessionId;

            ASSERT(NblContext->SessionId != MBB_INVALID_SESSION_ID); 

            if( NtbContext->NdpFirstDatagramEntry[CurrentNdpType] != MBB_NDP_DATAGRAM_INDEX_INVALID )
            {
                if(NtbContext->NdpDatagramEntries[NtbContext->NdpFirstDatagramEntry[CurrentNdpType]].SessionId
                    != SessionId)
                {
                    NdisStatus = NDIS_STATUS_BUFFER_OVERFLOW;
                    break;
                }
            }
        }
        
        //
        // Add as many NBs as can fit.
        //
        for( NetBuffer  = MbbNblGetNextDispatchNb( NetBufferList );
             NetBuffer != NULL;
             NetBuffer  = MbbNblAdvanceDispatchNb( NetBufferList ) )
        {
            if( (NdisStatus = MbbNtbAddNb(
                                NtbContext,
                                NblContext->NbLookasideList,
                                NetBuffer,
                                NetBufferList,
                                CurrentNdpType,
                                SessionId
                                )) != NDIS_STATUS_SUCCESS )
            {
                break;
            }
        }
    }
    while( FALSE );

    return NdisStatus;
}

NDIS_STATUS
MbbNtbAddNb(
    __in PMBB_NTB_BUILD_CONTEXT NtbContext,
    __in PNPAGED_LOOKASIDE_LIST NbLookasideList,
    __in PNET_BUFFER            NetBuffer,
    __in PNET_BUFFER_LIST       NetBufferList,
    __in MBB_NDP_TYPE           CurrentNdpType,
    __in ULONG                  SessionId
    )
{
    ULONG                   DatagramLength;
    ULONG                   DatagramOffset;
    ULONG                   PaddingLength;
    ULONG                   NdpSize;
    ULONG                   NdpIndex;
    PMBB_NB_CONTEXT         NbContext = NULL;
    NDIS_STATUS             NdisStatus = NDIS_STATUS_SUCCESS;
    ULONG                   TotalLength;
    ULONG                   CurrentNdpLastEntryIndex = MBB_NDP_DATAGRAM_INDEX_INVALID;

    do
    {
        if( (NtbContext->DatagramCount + 1) > NtbContext->NtbOutMaxDatagrams )
        {
            NdisStatus = NDIS_STATUS_BUFFER_OVERFLOW;
            break;
        }

        //
        // Size of passed in NET_BUFFER, to be updated later in the NDP Context.
        //
        TotalLength    = NtbContext->NtbHeaderSize + NtbContext->DatagramLength;
        DatagramLength = NET_BUFFER_DATA_LENGTH( NetBuffer );
        PaddingLength  = ALIGN_AT_OFFSET(
                            TotalLength,
                            NtbContext->NdpOutDivisor,
                            NtbContext->NdpOutPayloadRemainder
                            ) - TotalLength;
        //
        // Calculate the new NTB size based on the passed in NET_BUFFER
        //

        //
        // Fixed size NTH & DatagramSize along with Padding for all NDPs
        //
        DatagramOffset  = TotalLength + PaddingLength;
        TotalLength    += DatagramLength + PaddingLength;
        //
        // Calculate NDP HeaderSize for all NDPs
        //
        NdpSize = TotalLength;
        for( NdpIndex = 0;
             NdpIndex < MbbNdpTypeMax;
             NdpIndex ++ )
        {
            ULONG NdpDatagramCount = ( NdpIndex == CurrentNdpType )? 1: 0;
            ULONG NextEntryIndex   = NtbContext->NdpFirstDatagramEntry[NdpIndex];

            while( NextEntryIndex != MBB_NDP_DATAGRAM_INDEX_INVALID )
            {
                if( NdpIndex == CurrentNdpType )
                    CurrentNdpLastEntryIndex = NextEntryIndex;

                NdpDatagramCount++;
                NextEntryIndex = NtbContext->NdpDatagramEntries[NextEntryIndex].NextEntryIndex;
            }

            if( NdpDatagramCount != 0 )
            {
                TotalLength  = ALIGN( TotalLength, NtbContext->NdpOutAlignment );
                TotalLength += NtbContext->NdpHeaderFixedSize;
                TotalLength += (NdpDatagramCount * NtbContext->NdpDatagramEntrySize);
            }
        }
        NdpSize = TotalLength - NdpSize;
        //
        // Can everything fit?
        //
        if( TotalLength > NtbContext->NtbOutMaxSize )
        {
            NdisStatus  = NDIS_STATUS_BUFFER_OVERFLOW;
            break;
        }
        if( (NbContext = MbbNbAllocateContext(
                            NetBuffer,
                            DatagramLength,
                            NtbContext->PaddingBuffer,
                            PaddingLength,
                            NbLookasideList,
                            NtbContext->MiniportHandle
                            )) == NULL )
        {
            NdisStatus = NDIS_STATUS_RESOURCES;
            break;
        }
        //
        // Update the NTB Context for the new NET_BUFFER.
        //
        NtbContext->NdpDatagramEntries[NtbContext->DatagramCount].DatagramOffset = DatagramOffset;
        NtbContext->NdpDatagramEntries[NtbContext->DatagramCount].DatagramLength = DatagramLength;
        NtbContext->NdpDatagramEntries[NtbContext->DatagramCount].NdpType        = CurrentNdpType;
        NtbContext->NdpDatagramEntries[NtbContext->DatagramCount].SessionId      = SessionId;
        NtbContext->NdpDatagramEntries[NtbContext->DatagramCount].NetBuffer      = NetBuffer;
        NtbContext->NdpDatagramEntries[NtbContext->DatagramCount].NetBufferList  = NetBufferList;

        if( CurrentNdpLastEntryIndex != MBB_NDP_DATAGRAM_INDEX_INVALID )
            NtbContext->NdpDatagramEntries[CurrentNdpLastEntryIndex].NextEntryIndex  = NtbContext->DatagramCount;

        if( NtbContext->NdpFirstDatagramEntry[CurrentNdpType] == MBB_NDP_DATAGRAM_INDEX_INVALID )
            NtbContext->NdpFirstDatagramEntry[CurrentNdpType] = NtbContext->DatagramCount;

        NtbContext->NdpSize         = NdpSize;
        NtbContext->DatagramCount  += 1;
        NtbContext->DatagramLength += (DatagramLength + PaddingLength);

        MbbNtbChainNb( NtbContext, NetBuffer );
    }
    while( FALSE );

    if( NdisStatus != NDIS_STATUS_SUCCESS )
    {
        if( NbContext != NULL )
            MbbNbCleanupContext( NetBuffer );
    }
    return NdisStatus;
}

VOID
MbbNtbChainNb(
    __in PMBB_NTB_BUILD_CONTEXT NtbContext,
    __in PNET_BUFFER            NetBuffer
    )
{
    PMDL            MdlToChain;

    if( NtbContext->DatagramLastMdl != NULL )
    {
        TraceVerbose( WMBCLASS_SEND, "[Send][NB=0x%p] Chaining Mdl=0x%p -> Mdl=0x%p",
                      NetBuffer, NtbContext->DatagramLastMdl, MbbNbGetFirstMdl( NetBuffer )
                      );
        NtbContext->DatagramLastMdl->Next = MbbNbGetFirstMdl( NetBuffer );
    }
    else
    {
        TraceVerbose( WMBCLASS_SEND, "[Send][NB=0x%p] Chaining Mdl=0x%p -> Mdl=0x%p",
                      NetBuffer, NtbContext->NthMdl, MbbNbGetFirstMdl( NetBuffer )
                      );
        NtbContext->NthMdl->Next = MbbNbGetFirstMdl( NetBuffer );
    }

    NtbContext->DatagramLastMdl = MbbNbGetLastMdl( NetBuffer );
}

ULONG
MbbNtbMapNdpTypeToSignature(
    __in MBB_NDP_TYPE   MbbNdpType,
    __in BOOLEAN        Is32Bit,
    __in ULONG          SessionId
    )
{
    ULONG SessionMask = ( SessionId << NCM_NDP_SESSION_SHIFT );

    switch( MbbNdpType )
    {
        case MbbNdpTypeIps      : return (( Is32Bit==TRUE )? NCM_NDP32_IPS | SessionMask            
                                                                : NCM_NDP16_IPS | SessionMask);
        default                 : 
            if ((MbbNdpType >= MbbNdpTypeVendor_1) && (MbbNdpType <= MbbNdpTypeVendor_Max))
            {
                return (( Is32Bit==TRUE )? NCM_NDP32_VENDOR | SessionMask  
                                              : NCM_NDP16_VENDOR | SessionMask);
            }
    }
    return 0;
}

VOID
MbbNtbFillNdp32Header(
    __in PNCM_NDP32             Ndp,
    __in MBB_NDP_TYPE           NdpType,
    __in PMBB_NTB_BUILD_CONTEXT NtbContext
    )
{
    ULONG               DatagramIndex;
    ULONG               NdpDatagramCount;
    ULONG               NdpDatagramIndex;
    PNCM_NDP32_DATAGRAM NdpDatagramEntries;

    DatagramIndex = NtbContext->NdpFirstDatagramEntry[NdpType];

    // Caller ensured we are OK here
    ASSERT(DatagramIndex != MBB_NDP_DATAGRAM_INDEX_INVALID);
        
    Ndp->dwSignature   = MbbNtbMapNdpTypeToSignature( 
                            NdpType, 
                            TRUE, 
                            NtbContext->NdpDatagramEntries[DatagramIndex].SessionId 
                            );
    Ndp->dwNextFpIndex = 0;
    NdpDatagramEntries = Ndp->Datagram;
    //
    // Add datagram entries to the NDP Table
    //
    NdpDatagramIndex = 0;
    NdpDatagramCount = 0;

    for( DatagramIndex  = NtbContext->NdpFirstDatagramEntry[NdpType];
         DatagramIndex != MBB_NDP_DATAGRAM_INDEX_INVALID;
         DatagramIndex  = NtbContext->NdpDatagramEntries[DatagramIndex].NextEntryIndex )
    {
        NdpDatagramEntries[NdpDatagramIndex].dwDatagramIndex  = NtbContext->NdpDatagramEntries[DatagramIndex].DatagramOffset;
        NdpDatagramEntries[NdpDatagramIndex].dwDatagramLength = NtbContext->NdpDatagramEntries[DatagramIndex].DatagramLength;
        NdpDatagramIndex++;
        NdpDatagramCount++;
    }
    //
    // Terminating entry is taken in to account
    // in the fixed size NDP Header.
    //
    NdpDatagramEntries[NdpDatagramIndex].dwDatagramIndex  = 0;
    NdpDatagramEntries[NdpDatagramIndex].dwDatagramLength = 0;

    Ndp->wLength = (USHORT)(NtbContext->NdpHeaderFixedSize + (NdpDatagramIndex * NtbContext->NdpDatagramEntrySize));
}

VOID
MbbNtbFillNdp16Header(
    __in PNCM_NDP16             Ndp,
    __in MBB_NDP_TYPE           NdpType,
    __in PMBB_NTB_BUILD_CONTEXT NtbContext
    )
{
    ULONG               DatagramIndex;
    ULONG               NdpDatagramCount;
    ULONG               NdpDatagramIndex;
    PNCM_NDP16_DATAGRAM NdpDatagramEntries;

    DatagramIndex = NtbContext->NdpFirstDatagramEntry[NdpType];

    // Caller ensured we are OK here
    ASSERT(DatagramIndex != MBB_NDP_DATAGRAM_INDEX_INVALID);
        
    Ndp->dwSignature   = MbbNtbMapNdpTypeToSignature( 
                            NdpType, 
                            FALSE, 
                            NtbContext->NdpDatagramEntries[DatagramIndex].SessionId 
                            );

    Ndp->wNextFpIndex  = 0;
    NdpDatagramEntries = Ndp->Datagram;
    //
    // Add datagram entries to the NDP Table
    //
    NdpDatagramIndex = 0;
    NdpDatagramCount = 0;

    for( DatagramIndex  = NtbContext->NdpFirstDatagramEntry[NdpType];
         DatagramIndex != MBB_NDP_DATAGRAM_INDEX_INVALID;
         DatagramIndex  = NtbContext->NdpDatagramEntries[DatagramIndex].NextEntryIndex )
    {
        NdpDatagramEntries[NdpDatagramIndex].wDatagramIndex  = (USHORT)NtbContext->NdpDatagramEntries[DatagramIndex].DatagramOffset;
        NdpDatagramEntries[NdpDatagramIndex].wDatagramLength = (USHORT)NtbContext->NdpDatagramEntries[DatagramIndex].DatagramLength;
        NdpDatagramIndex++;
        NdpDatagramCount++;
    }
    //
    // Terminating entry is taken in to account
    // in the fixed size NDP Header.
    //
    NdpDatagramEntries[NdpDatagramIndex].wDatagramIndex  = 0;
    NdpDatagramEntries[NdpDatagramIndex].wDatagramLength = 0;

    Ndp->wLength = (USHORT)(NtbContext->NdpHeaderFixedSize + (NdpDatagramCount * NtbContext->NdpDatagramEntrySize));
}

NDIS_STATUS
MbbNtbAddNdpHeaders(
    __in PMBB_NTB_BUILD_CONTEXT   NtbContext
    )
{
    // Offset from the start of the NTB buffer to the start of NDP headers
    ULONG           NdpStartOffset;
    // Offset from the start of the NTB Buffer to the current position.
    ULONG           NtbOffset;
    PCHAR           NdpBuffer;
    MBB_NDP_TYPE    NdpType;
    PNCM_NDP16      Ndp16;
    PNCM_NDP32      Ndp32;
    PNCM_NDP16      PreviousNdp16 = NULL;
    PNCM_NDP32      PreviousNdp32 = NULL;
    NDIS_STATUS     NdisStatus = NDIS_STATUS_SUCCESS;

    do
    {
        //
        // Allocate buffer for all NDP headers.
        // This includes padding for NDP Header alignment.
        //
        if( (NtbContext->NdpBuffer = ALLOCATE_NONPAGED_POOL(
                                        NtbContext->NdpSize
                                        )) == NULL )
        {
            NdisStatus = NDIS_STATUS_RESOURCES;
            break;
        }
        NdpBuffer = (PCHAR)(NtbContext->NdpBuffer);
        //
        // Chain the NDP Header through its MDL to datagram MDL
        //
        if( (NtbContext->NdpMdl = NdisAllocateMdl(
                                    NtbContext->MiniportHandle,
                                    NtbContext->NdpBuffer,
                                    NtbContext->NdpSize
                                    )) == NULL )
        {
            NdisStatus = NDIS_STATUS_RESOURCES;
            break;
        }
        NtbContext->DatagramLastMdl->Next = NtbContext->NdpMdl;
        NdpStartOffset = NtbContext->NtbHeaderSize + NtbContext->DatagramLength;

        if( NtbContext->IsNtb32Bit )
            NtbContext->Nth32.dwFpIndex = ALIGN( NdpStartOffset, NtbContext->NdpOutAlignment );
        else
            NtbContext->Nth16.wFpIndex  = (USHORT)ALIGN( NdpStartOffset, NtbContext->NdpOutAlignment );

        NtbOffset = NdpStartOffset;

        for( NdpType = 0;
             NdpType < MbbNdpTypeMax;
             NdpType ++ )
        {
            if( NtbContext->NdpFirstDatagramEntry[NdpType] == MBB_NDP_DATAGRAM_INDEX_INVALID )
                continue;

            NtbOffset = ALIGN( NtbOffset, NtbContext->NdpOutAlignment );

            if( NtbContext->IsNtb32Bit == TRUE )
            {
                Ndp32 = (PNCM_NDP32)(NdpBuffer + (NtbOffset - NdpStartOffset));

                MbbNtbFillNdp32Header(
                    Ndp32,
                    NdpType,
                    NtbContext
                    );

                if( PreviousNdp32 != NULL )
                    PreviousNdp32->dwNextFpIndex = NtbOffset;

                NtbOffset    += Ndp32->wLength;
                PreviousNdp32 = Ndp32;
            }
            else
            {
                Ndp16 = (PNCM_NDP16)(NdpBuffer + (NtbOffset - NdpStartOffset));

                MbbNtbFillNdp16Header(
                    Ndp16,
                    NdpType,
                    NtbContext
                    );

                if( PreviousNdp16 != NULL )
                    PreviousNdp16->wNextFpIndex = (USHORT)NtbOffset;

                NtbOffset    += Ndp16->wLength;
                PreviousNdp16 = Ndp16;
            }
        }

        if( NtbContext->IsNtb32Bit == TRUE )
        {
            NtbContext->Nth32.dwBlockLength  = NtbContext->NtbHeaderSize;
            NtbContext->Nth32.dwBlockLength += NtbContext->DatagramLength;
            NtbContext->Nth32.dwBlockLength += NtbContext->NdpSize;
        }
        else
        {
            NtbContext->Nth16.wBlockLength  = (USHORT)(NtbContext->NtbHeaderSize);
            NtbContext->Nth16.wBlockLength += (USHORT)(NtbContext->DatagramLength);
            NtbContext->Nth16.wBlockLength += (USHORT)(NtbContext->NdpSize);
        }
    }
    while( FALSE );
    //
    // No cleanup. Cleanup done by caller.
    //
    return NdisStatus;
}

FORCEINLINE
PMDL
MbbNtbGetMdlChainHead(
    __in PMBB_NTB_BUILD_CONTEXT NtbContext
    )
{
    return NtbContext->NthMdl;
}

// Test
#if DBG
NDIS_STATUS
MbbTestValidateNtb(
    __in    PMBB_NTB_BUILD_CONTEXT      NtbContext,
    __in_bcount(ScratchLength) PCHAR    ScratchBuffer,
    __in    ULONG                       ScratchLength
    )
{
    PMDL        CurrentMdl;
    ULONGLONG   NtbLength;
    ULONG       MdlLength;
    PVOID       MdlVa;
    PVOID       Nth;

    Nth = ScratchBuffer;
    NtbLength = 0;

    for( CurrentMdl  = MbbNtbGetMdlChainHead( NtbContext );
         CurrentMdl != NULL;
         CurrentMdl  = CurrentMdl->Next )
    {
        MdlLength = MmGetMdlByteCount( CurrentMdl );

        if( (MdlVa = MmGetSystemAddressForMdlSafe(
                        CurrentMdl,
                        NormalPagePriority | MdlMappingNoExecute
                        )) == NULL )
        {
            return NDIS_STATUS_RESOURCES;
        }

        if( (NtbLength+MdlLength) > ScratchLength )
        {
            return NDIS_STATUS_BUFFER_OVERFLOW;
        }

        RtlCopyMemory( ScratchBuffer, MdlVa, MdlLength );

        ScratchBuffer += MdlLength;
        NtbLength     += MdlLength;
    }

    return MbbNtbValidate(
                Nth,
                (ULONG)NtbLength,
                NtbContext->IsNtb32Bit
                );
}
#endif

//
//  NB Context
//

PMBB_NB_CONTEXT
MbbNbAllocateContext(
    __in    PNET_BUFFER             NetBuffer,
    __in    ULONG                   DatagramLength,
    __in    PVOID                   PaddingBuffer,
    __in    ULONG                   PaddingLength,
    __in    PNPAGED_LOOKASIDE_LIST  NbLookasideList,
    __in    NDIS_HANDLE             MiniportHandle
    )
{
    PMDL            NbDataStartMdl = NET_BUFFER_CURRENT_MDL( NetBuffer );
    ULONG           NbDataStartMdlDataOffset = NET_BUFFER_CURRENT_MDL_OFFSET( NetBuffer );
    ULONG           NbDataStartMdlDataLength;
    PCHAR           NbDataStartBuffer;
    PMDL            NbDataEndMdl;
    ULONG           NbDataEndMdlDataLength;
    PCHAR           NbDataEndBuffer;
    PMDL            NbMdl;
    ULONG           NbMdlOffset;
    ULONG           NbMdlLength;
    PMDL            NbPenultimateMdl;
    PMBB_NB_CONTEXT NbContext = NULL;
    NDIS_STATUS     NdisStatus = NDIS_STATUS_SUCCESS;

    do
    {
        //
        // Allocate the NbContext
        //
        if( (NbContext = ALLOCATE_LOOKASIDE(NbLookasideList)) == NULL )
        {
            NdisStatus = NDIS_STATUS_RESOURCES;
            break;
        }
        RtlZeroMemory( NbContext, sizeof(MBB_NB_CONTEXT) );
        NbContext->NbLookasideList = NbLookasideList;

        NET_BUFFER_MINIPORT_RESERVED( NetBuffer )[0] = NbContext;

        if( (NbDataStartBuffer  = (PCHAR) MmGetSystemAddressForMdlSafe(
                                            NbDataStartMdl,
                                            NormalPagePriority | MdlMappingNoExecute
                                            )) == NULL )
        {
            NdisStatus = NDIS_STATUS_RESOURCES;
            break;
        }
        NbDataStartBuffer += NbDataStartMdlDataOffset;
        //
        // Create new DataStart and DataEnd Mdls
        // to remove the unused data space.
        //
        NbDataStartMdlDataLength  = MmGetMdlByteCount( NbDataStartMdl );
        NbDataStartMdlDataLength -= NbDataStartMdlDataOffset;

        if( (NbContext->DataStartMdl = NdisAllocateMdl(
                                            MiniportHandle,
                                            NbDataStartBuffer,
                                            MIN( DatagramLength, NbDataStartMdlDataLength )
                                            )) == NULL )
        {
            NdisStatus = NDIS_STATUS_RESOURCES;
            break;
        }
        TraceVerbose( WMBCLASS_SEND, "[Send][NB=0x%p] Creating DataStartMdl[OrigMdl=0x%p,NewMdl=0x%p]", NetBuffer, NbDataStartMdl, NbContext->DataStartMdl );
        NbContext->DataStartMdl->Next = NbDataStartMdl->Next;
        //
        // Find the end MDL and the amount of data in the end MDL
        //
        NbMdl            = NbDataStartMdl;
        NbMdlOffset      = NbDataStartMdlDataOffset;
        NbPenultimateMdl = NULL;

        for( NbDataEndMdlDataLength  = DatagramLength;
             NbDataEndMdlDataLength  > (MmGetMdlByteCount( NbMdl ) - NbMdlOffset);
             NbDataEndMdlDataLength -= NbMdlLength )
        {
            NbPenultimateMdl = NbMdl;
            NbMdlLength      = MmGetMdlByteCount( NbMdl ) - NbMdlOffset;
            NbMdlOffset      = 0;
            NbMdl            = NbMdl->Next;
        }
        NbDataEndMdl = NbMdl;
        //
        // If the starting and ending MDLs are not the same
        // then build another partial MDL removing any unused
        // data space.
        //
        if( NbDataEndMdl != NbDataStartMdl )
        {
            if( (NbDataEndBuffer = MmGetSystemAddressForMdlSafe(
                                        NbDataEndMdl,
                                        NormalPagePriority | MdlMappingNoExecute
                                        )) == NULL )
            {
                NdisStatus = NDIS_STATUS_RESOURCES;
                break;
            }

            if( (NbContext->DataEndMdl = NdisAllocateMdl(
                                            MiniportHandle,
                                            NbDataEndBuffer,
                                            NbDataEndMdlDataLength
                                            )) == NULL )
            {
                NdisStatus = NDIS_STATUS_RESOURCES;
                break;
            }
            TraceVerbose( WMBCLASS_SEND, "[Send][NB=0x%p] Creating DataEndMdl[OrigMdl=0x%p,NewMdl=0x%p]", NetBuffer, NbDataEndMdl, NbContext->DataEndMdl );
            NbContext->DataEndMdl->Next = NULL;

            if( NbPenultimateMdl != NbDataStartMdl )
            {
                MDL TempMdl  = *NbPenultimateMdl;
                TempMdl.Next = NbContext->DataEndMdl;

                MbbNbSaveAndSetMdl(
                    NetBuffer,
                    NbPenultimateMdl,
                    &TempMdl
                    );
            }

            if( NbContext->DataStartMdl->Next == NbDataEndMdl )
            {
                NbContext->DataStartMdl->Next = NbContext->DataEndMdl;
            }
        }
        //
        // Allocate padding, if needed. The padding buffer is a share buffer.
        // Every padding MDL points to this same buffer. The buffer contains
        // all 0s.
        //
        if( PaddingLength != 0 )
        {
            if( (NbContext->PaddingMdl = NdisAllocateMdl(
                                            MiniportHandle,
                                            PaddingBuffer,
                                            PaddingLength
                                            )) == NULL )
            {
                NdisStatus = NDIS_STATUS_RESOURCES;
                break;
            }
            TraceVerbose( WMBCLASS_SEND, "[Send][NB=0x%p] Creating PaddingMdl=0x%p", NetBuffer, NbContext->PaddingMdl );
            NbContext->PaddingMdl->Next = NbContext->DataStartMdl;
        }
    }
    while( FALSE );

    if( NdisStatus != NDIS_STATUS_SUCCESS )
    {
        if( NbContext != NULL )
        {
            MbbNbCleanupContext( NetBuffer );
            NbContext = NULL;
        }
    }
    return NbContext;
}

VOID
MbbNbCleanupContext(
    __in PNET_BUFFER    NetBuffer
    )
{
    PMBB_NB_CONTEXT NbContext = (PMBB_NB_CONTEXT)NET_BUFFER_MINIPORT_RESERVED( NetBuffer )[0];

    MbbNbRestoreMdl( NetBuffer );

    if( NbContext->PaddingMdl != NULL )
    {
        NdisFreeMdl( NbContext->PaddingMdl );
    }
    if( NbContext->DataEndMdl != NULL )
    {
        NdisFreeMdl( NbContext->DataEndMdl );
    }
    if( NbContext->DataStartMdl != NULL )
    {
        NdisFreeMdl( NbContext->DataStartMdl );
    }
    NET_BUFFER_MINIPORT_RESERVED( NetBuffer )[0] = NULL;
    NET_BUFFER_MINIPORT_RESERVED( NetBuffer )[1] = NbContext;

    FREE_LOOKASIDE( NbContext, NbContext->NbLookasideList );
}

FORCEINLINE
PMDL
MbbNbGetFirstMdl(
    __in PNET_BUFFER    NetBuffer
    )
{
    PMBB_NB_CONTEXT NbContext = (PMBB_NB_CONTEXT)NET_BUFFER_MINIPORT_RESERVED( NetBuffer )[0];

    if( NbContext->PaddingMdl != NULL )
        return NbContext->PaddingMdl;
    else
        return NbContext->DataStartMdl;
}

FORCEINLINE
PMDL
MbbNbGetLastMdl(
    __in PNET_BUFFER    NetBuffer
    )
{
    PMBB_NB_CONTEXT NbContext = (PMBB_NB_CONTEXT)NET_BUFFER_MINIPORT_RESERVED( NetBuffer )[0];

    if( NbContext->DataEndMdl != NULL )
        return NbContext->DataEndMdl;
    else
        return NbContext->DataStartMdl;
}

VOID
MbbNbSaveAndSetMdl(
    __in PNET_BUFFER    NetBuffer,
    __in PMDL           MdlToSave,
    __in PMDL           MdlToSet
    )
{
    PMBB_NB_CONTEXT NbContext = (PMBB_NB_CONTEXT)NET_BUFFER_MINIPORT_RESERVED( NetBuffer )[0];

    if( NbContext->ModifiedMdl == NULL )
    {
        NbContext->ModifiedMdl =  MdlToSave;
        NbContext->OriginalMdl = *MdlToSave;
        TraceVerbose( WMBCLASS_SEND, "[Send][NB=0x%p] Saving Mdl=0x%p", NetBuffer, NbContext->ModifiedMdl );
    }
    *MdlToSave = *MdlToSet;
}

VOID
MbbNbRestoreMdl(
    __in PNET_BUFFER    NetBuffer
    )
{
    PMBB_NB_CONTEXT NbContext = (PMBB_NB_CONTEXT)NET_BUFFER_MINIPORT_RESERVED( NetBuffer )[0];

    if( NbContext->ModifiedMdl != NULL )
    {
        TraceVerbose( WMBCLASS_SEND, "[Send][NB=0x%p] Restoring Mdl=0x%p", NetBuffer, NbContext->ModifiedMdl );
        *NbContext->ModifiedMdl = NbContext->OriginalMdl;
         NbContext->ModifiedMdl = NULL;
    }
}

//
//  NBL Context
//

PMBB_NBL_CONTEXT
MbbNblAllocateContext(
    __in    PNET_BUFFER_LIST            NetBufferList,
    __in    PNPAGED_LOOKASIDE_LIST      NblLookasideList,
    __in    PNPAGED_LOOKASIDE_LIST      NbLookasideList,
    __in    MBB_NBL_CLEANUP_CALLBACK    CleanupCallback,
    __in    PVOID                       CleanupCallbackContext
    )
{
    PMBB_NBL_CONTEXT    NblContext = NULL;
    PNET_BUFFER         NetBuffer;
    ULONG               NbCount;

    do
    {
        if( (NblContext = ALLOCATE_LOOKASIDE(NblLookasideList)) == NULL )
        {
            break;
        }

        InitializeListHead( &NblContext->NblQLink );
        InitializeListHead( &NblContext->DispatchQLink );

        NblContext->NetBufferList    = NetBufferList;
        NblContext->NblLookasideList = NblLookasideList;
        NblContext->NbLookasideList  = NbLookasideList;

        NbCount = 0;
        for( NetBuffer  = NET_BUFFER_LIST_FIRST_NB( NetBufferList );
             NetBuffer != NULL;
             NetBuffer  = NET_BUFFER_NEXT_NB( NetBuffer ) )
        {
            NbCount++;
        }
        NblContext->Failed                  = FALSE;
        NblContext->NbTotalCount            = NbCount;
        NblContext->NbDispatchCount         = 0;
        NblContext->NbCompleteCount         = 0;
        NblContext->CleanupCallback         = CleanupCallback;
        NblContext->CleanupCallbackContext  = CleanupCallbackContext;
        NblContext->CurrentNb               = NET_BUFFER_LIST_FIRST_NB( NetBufferList );
        NblContext->DssRequestHandle        = NULL;
        NblContext->DssSessionId            = (ULONG)(-1);        
        NblContext->SessionId               = MBB_INVALID_SESSION_ID;

        NET_BUFFER_LIST_MINIPORT_RESERVED(NetBufferList)[0] = NblContext;
    }
    while( FALSE );

    return NblContext;
}

VOID
MbbNblFinalizeContext(
    __in PNET_BUFFER_LIST   NetBufferList,
    __in ULONG              NbCount,
    __in NTSTATUS           NtStatus
    )
{
    PMBB_NBL_CONTEXT    NblContext = (PMBB_NBL_CONTEXT)NET_BUFFER_LIST_MINIPORT_RESERVED( NetBufferList )[0];
    ULONG               OriginalCount;
    ULONG               NewCount;

    OriginalCount   = NblContext->NbCompleteCount;
    NewCount        = OriginalCount + NbCount;

    while( InterlockedCompareExchange(
                &NblContext->NbCompleteCount,
                NewCount,
                OriginalCount
                ) != OriginalCount )
    {
        OriginalCount   = NblContext->NbCompleteCount;
        NewCount        = OriginalCount + NbCount;
    }

    if( NtStatus != STATUS_SUCCESS )
    {
        NblContext->Failed = TRUE;
    }

    if( NewCount == NblContext->NbTotalCount )
    {
        ASSERT( NblContext->NbCompleteCount == NblContext->NbTotalCount );

        if( NblContext->Failed == TRUE )
            NetBufferList->Status = NDIS_STATUS_FAILURE;
        else
            NetBufferList->Status = NDIS_STATUS_SUCCESS;

        NblContext->CleanupCallback(
                        NetBufferList,
                        NblContext->CleanupCallbackContext
                        );
        //
        // To catch double completions
        //
        NblContext->CleanupCallback         = MBB_BAD_POINTER;
        NblContext->CleanupCallbackContext  = MBB_BAD_POINTER;
        FREE_LOOKASIDE( NblContext, NblContext->NblLookasideList );
    }
}

FORCEINLINE
PNET_BUFFER
MbbNblGetNextDispatchNb(
    __in    PNET_BUFFER_LIST    NetBufferList
    )
{
    PMBB_NBL_CONTEXT NblContext = NET_BUFFER_LIST_MINIPORT_RESERVED( NetBufferList )[0];
    return NblContext->CurrentNb;
}

FORCEINLINE
PNET_BUFFER
MbbNblAdvanceDispatchNb(
    __in    PNET_BUFFER_LIST    NetBufferList
    )
{
    PMBB_NBL_CONTEXT NblContext = NET_BUFFER_LIST_MINIPORT_RESERVED( NetBufferList )[0];

    if( NblContext->CurrentNb != NULL )
    {
        NblContext->NbDispatchCount++;
        NblContext->CurrentNb = NET_BUFFER_NEXT_NB( NblContext->CurrentNb );
    }
    return NblContext->CurrentNb;
}

//
//  Miniport handlers
//

VOID
MbbNdisMiniportSendNetBufferLists(
    __in    NDIS_HANDLE         MiniportAdapterContext,
    __in    PNET_BUFFER_LIST    NetBufferList,
    __in    NDIS_PORT_NUMBER    PortNumber,
    __in    ULONG               SendFlags
    )
{
    NDIS_STATUS                 NdisStatus;
    PMINIPORT_ADAPTER_CONTEXT   Adapter=(PMINIPORT_ADAPTER_CONTEXT)MiniportAdapterContext;
    PMBB_SEND_QUEUE             SendQueue=&Adapter->SendQueue;
    PNET_BUFFER_LIST            NextNetBufferList=NULL;
    PNET_BUFFER_LIST            CurrentNetBufferList;
    BOOLEAN                     DispatchLevel= (SendFlags & NDIS_SEND_COMPLETE_FLAGS_DISPATCH_LEVEL) == NDIS_SEND_COMPLETE_FLAGS_DISPATCH_LEVEL;
    BOOLEAN                     CompleteNow=FALSE;    
    ULONG                       SessionId = MBB_INVALID_SESSION_ID;
    PMBB_PORT                   Port = NULL;
    BOOLEAN                     IsConnected = FALSE;
    BOOLEAN                     CompleteNBLsOnFailure = FALSE;
    

    // Get the session id for the corresponding portnumber
    Port = MbbWwanTranslatePortNumberToPort(
                    Adapter,
                    PortNumber);

    if(!Port)
    {
        TraceError( WMBCLASS_SEND, "[Send][NBL=0x%p] DROPPING all. Received on invalid port %lu", NetBufferList,PortNumber);
        CompleteNBLsOnFailure = TRUE;
    }
    else
    {
        // Get the connection status and session id for the port.        
        MBB_ACQUIRE_PORT_LOCK(Port);

        IsConnected = Port->ConnectionState.ConnectionUp;
        SessionId = Port->SessionId;
        
        MBB_RELEASE_PORT_LOCK(Port);
      
        if(!IsConnected
             || SessionId == MBB_INVALID_SESSION_ID )
        {
            TraceError( WMBCLASS_SEND, "[Send][NBL=0x%p] DROPPING all. Received on disconnected port %lu", NetBufferList, PortNumber);
            CompleteNBLsOnFailure = TRUE;
        }
    }

    if (MbbBusIsFastIO(Adapter->BusHandle) && CompleteNBLsOnFailure == FALSE)
    {
        USHORT NblContextSize = ALIGN(sizeof(MBB_NBL_TYPE), MEMORY_ALLOCATION_ALIGNMENT);
        MBB_NBL_TYPE* NblContext;
        for (CurrentNetBufferList = NetBufferList;
            CurrentNetBufferList != NULL;
            CurrentNetBufferList = NextNetBufferList)
        {
            if (NdisAllocateNetBufferListContext(
                NetBufferList,
                NblContextSize,
                0,
                MbbPoolTagNblFastIOContext) != NDIS_STATUS_SUCCESS)
            {
                TraceError(WMBCLASS_RECEIVE, "[FastIOSend][DSS NBL=0x%p] FAILED to allocate NetBufferListContext", NetBufferList);
                CompleteNBLsOnFailure = TRUE;
                break;
            }
            NblContext = (MBB_NBL_TYPE*)NET_BUFFER_LIST_CONTEXT_DATA_START(NetBufferList);
            *NblContext = MBB_NBL_TYPE_IP;
        }
    }

    if(CompleteNBLsOnFailure)
    {
        for (CurrentNetBufferList = NetBufferList;  
                CurrentNetBufferList != NULL;  
                 CurrentNetBufferList = NextNetBufferList)  
        {  
            // fail all NBLs
            NextNetBufferList = NET_BUFFER_LIST_NEXT_NBL(CurrentNetBufferList);  
            NET_BUFFER_LIST_STATUS(CurrentNetBufferList) = NDIS_STATUS_FAILURE;  

            InterlockedIncrement64(&Adapter->Stats.ifOutDiscards);
        }  
        
        NdisMSendNetBufferListsComplete(
                Adapter->MiniportAdapterHandle,
                NetBufferList,
                DispatchLevel ? NDIS_SEND_COMPLETE_FLAGS_DISPATCH_LEVEL : 0
                );        
    }
    else if (MbbBusIsFastIO(Adapter->BusHandle))
    {
        MbbBusSendNetBufferLists(Adapter->BusHandle, NetBufferList, SessionId, SendFlags);
    }
    else
    {
        MbbSendQLock( SendQueue, DispatchLevel );


        for( CurrentNetBufferList  = NetBufferList;
             CurrentNetBufferList != NULL;
             CurrentNetBufferList  = NextNetBufferList )
        {
            NextNetBufferList = NET_BUFFER_LIST_NEXT_NBL( CurrentNetBufferList );
            NET_BUFFER_LIST_NEXT_NBL(CurrentNetBufferList) = NULL;

            CompleteNow = !DrainAddRef( &SendQueue->QueueDrainObject );

            if( CompleteNow )
            {
                CurrentNetBufferList->Status = NDIS_STATUS_PAUSED;
                TraceError( WMBCLASS_SEND, "[Send][NBL=0x%p] DROPPING", CurrentNetBufferList );

                MbbSendQUnlock( SendQueue, DispatchLevel );

                NdisMSendNetBufferListsComplete(
                    Adapter->MiniportAdapterHandle,
                    CurrentNetBufferList,
                    DispatchLevel ? NDIS_SEND_COMPLETE_FLAGS_DISPATCH_LEVEL : 0
                    );

                CurrentNetBufferList=NULL;

                InterlockedIncrement64( &Adapter->Stats.ifOutDiscards );

                MbbSendQLock( SendQueue, DispatchLevel );

            }
            else
            {

                if( (NdisStatus = MbbSendQQueueNbl(
                                    SendQueue,
                                    CurrentNetBufferList,
                                    SessionId
                                    )) != NDIS_STATUS_SUCCESS )
                {
                    TraceError( WMBCLASS_SEND, "[Send][NBL=0x%p] DROPPING, queuing failed", CurrentNetBufferList );
                    CurrentNetBufferList->Status = NdisStatus;

                    MbbSendQUnlock( SendQueue, DispatchLevel );

                    NdisMSendNetBufferListsComplete(
                        Adapter->MiniportAdapterHandle,
                        CurrentNetBufferList,
                        DispatchLevel ? NDIS_SEND_COMPLETE_FLAGS_DISPATCH_LEVEL : 0
                        );

                    //
                    //  a ref was added for this nbl, removed it
                    //
                    DrainRelease( &SendQueue->QueueDrainObject );

                    InterlockedIncrement64( &Adapter->Stats.ifOutDiscards );
                    MbbSendQLock( SendQueue, DispatchLevel );
                }
                CurrentNetBufferList=NULL;
            }
        }

        MbbSendQUnlock( SendQueue, DispatchLevel );

        MbbSendQProcess( SendQueue, DispatchLevel );
        
    }

    if(Port)
    {
        //Remove the reference added during find
        Dereference(Port);
    }

    return;
}

VOID
MbbNdisMiniportCancelSend(
    __in    NDIS_HANDLE MiniportAdapterContext,
    __in    PVOID       CancelId
    )
{
    PMINIPORT_ADAPTER_CONTEXT   Adapter=(PMINIPORT_ADAPTER_CONTEXT)MiniportAdapterContext;
    PMBB_SEND_QUEUE             SendQueue=&Adapter->SendQueue;
    PLIST_ENTRY                 ListEntry;
    PLIST_ENTRY                 NextEntry;
    LIST_ENTRY                  TempList;
    PMBB_NBL_CONTEXT            NblContext = NULL;

    if (MbbBusIsFastIO(Adapter->BusHandle))
    {
        MbbBusCancelSendHandler(Adapter->BusHandle, CancelId);
        return;
    }

    InitializeListHead( &TempList );

    MbbSendQLock( SendQueue, FALSE );
    for( ListEntry  = SendQueue->NblDispatchQueue.Flink;
         ListEntry != &SendQueue->NblDispatchQueue;
         ListEntry  = NextEntry )
    {
        NextEntry  = ListEntry->Flink;
        NblContext = CONTAINING_RECORD( ListEntry, MBB_NBL_CONTEXT, DispatchQLink );
        if( NDIS_GET_NET_BUFFER_LIST_CANCEL_ID( NblContext->NetBufferList ) == CancelId )
        {
            RemoveEntryList( ListEntry );
            InsertHeadList( &TempList, ListEntry );
        }
    }
    MbbSendQUnlock( SendQueue, FALSE );

    for( ListEntry  =  TempList.Flink;
         ListEntry  != &TempList;
         ListEntry  =  NextEntry )
    {
        NextEntry = ListEntry->Flink;
        RemoveEntryList( ListEntry );
        NblContext = CONTAINING_RECORD( ListEntry, MBB_NBL_CONTEXT, DispatchQLink );

        TraceInfo( WMBCLASS_SEND, "[Send][NBL=0x%p] Cancelling for CancelId=0x%p", NblContext->NetBufferList, CancelId );

        MbbNblFinalizeContext(
            NblContext->NetBufferList,
            NblContext->NbTotalCount - NblContext->NbDispatchCount,
            NDIS_STATUS_SEND_ABORTED
            );
    }
}

PNET_BUFFER_LIST
MbbAllocateDssNbl(
    __in    PMINIPORT_ADAPTER_CONTEXT Adapter,
    __in    PMBB_SEND_QUEUE         SendQueue,
    __in    ULONG                   RequestId,
    __in    ULONG                   DataSize,
    __in    PVOID                   Data
    )
{
    PMDL                        Mdl = NULL;
    PNET_BUFFER_LIST            Nbl = NULL;
    NDIS_STATUS                 NdisStatus = NDIS_STATUS_SUCCESS;
    PVOID                       CopiedData = NULL;

    do
    {
        // Make a copy of the data. We need to do this
        // since the OID may already be freed
#pragma prefast(suppress:__WARNING_MEMORY_LEAK, "By Design: Allocate memory for data copy, released when data dequeued from send queue.")
        if( (CopiedData = ALLOCATE_NONPAGED_POOL(DataSize)) == NULL)
        {
            TraceError( WMBCLASS_SEND, "[Send][DSS Write %d] FAILED to create copy of data", RequestId );
            NdisStatus = NDIS_STATUS_RESOURCES;
            break;
        }
        RtlCopyMemory(CopiedData, Data, DataSize);
    
        if( (Mdl = NdisAllocateMdl(
                        Adapter->MiniportAdapterHandle,
                        CopiedData,
                        DataSize
                        )) == NULL )
        {
            TraceError( WMBCLASS_SEND, "[Send][DSS Write %d] FAILED to allocated Mdl", RequestId );
            NdisStatus = NDIS_STATUS_RESOURCES;
            break;
        }

        if( (Nbl = NdisAllocateNetBufferAndNetBufferList(
                                SendQueue->NblPool,
                                0,
                                0,
                                Mdl,
                                0,
                                DataSize
                                )) == NULL )
        {
            TraceError( WMBCLASS_SEND, "[Send][DSS Write %d] FAILED to allocated Nbl & Nb", RequestId );
            NdisStatus = NDIS_STATUS_RESOURCES;
            break;
        }

        // Log the converted values
        TraceInfo( WMBCLASS_SEND, "[Send][Dss Write %d] mapped to [NBL=0x%p] and [NB=0x%p]", 
            RequestId, Nbl, NET_BUFFER_LIST_FIRST_NB(Nbl));        

    }
    while( FALSE );

    if (NdisStatus != NDIS_STATUS_SUCCESS)
    {
        if (Nbl)
        {
            NdisFreeNetBufferList( Nbl );
            Nbl = NULL;
        }

        if (Mdl)
        {
            NdisFreeMdl( Mdl );
            Mdl = NULL;
        }

        if (CopiedData)
        {
            FREE_POOL(CopiedData);
        }
    }
    return Nbl;
}

VOID
MbbCleanupDssNbl(
    _In_    PMBB_SEND_QUEUE         SendQueue,
    _In_    PNET_BUFFER_LIST        NetBufferList
    )
{
    PMDL                            Mdl = NULL;
    PVOID                           CopiedData = NULL;
    ULONG                           Length;
    
    Mdl = NET_BUFFER_FIRST_MDL(NET_BUFFER_LIST_FIRST_NB(NetBufferList));    
    NdisQueryMdl (Mdl,  &CopiedData, &Length, NormalPagePriority | MdlMappingNoExecute);
    if (CopiedData)
        FREE_POOL(CopiedData);
        
    // The MDL is self allocated
    NdisFreeMdl( Mdl );
    NdisFreeNetBufferList( NetBufferList );
}

_Requires_lock_held_(SendQueue->Lock)
NDIS_STATUS
MbbSendQQueueDssData(
    __in    PMINIPORT_ADAPTER_CONTEXT Adapter,
    __in    PMBB_SEND_QUEUE         SendQueue,
    __in    MBB_REQUEST_HANDLE      RequestHandle,
    __in    ULONG                   RequestId,
    __in    ULONG                   SessionId,
    __in    ULONG                   DataSize,
    __in    PVOID                   Data
    )
{
    PMBB_NBL_CONTEXT    NblContext;
    PNET_BUFFER_LIST    NetBufferList = NULL;

    if( (NetBufferList = MbbAllocateDssNbl(
                            Adapter,
                            SendQueue,
							RequestId,
                            DataSize,
                            Data
                        )) == NULL )
    {
        TraceError( WMBCLASS_SEND, "[Send][DSS Write %d] FAILED to allocated Nbl", RequestId );
        return NDIS_STATUS_RESOURCES;
    }
    
    if( (NblContext = MbbNblAllocateContext(
                        NetBufferList,
                        &SendQueue->NblLookasideList,
                        &SendQueue->NbLookasideList,
                        MbbSendQDequeueDssData,
                        SendQueue
                        )) == NULL )
    {
        TraceError( WMBCLASS_SEND, "[Send][NBL=0x%p] FAILED to allocated context", NetBufferList );
        MbbCleanupDssNbl(SendQueue, NetBufferList);
        return NDIS_STATUS_RESOURCES;
    }

    // Save the DSS specific information for completion
    NblContext->DssSessionId = SessionId;
    NblContext->DssRequestHandle = RequestHandle;

    if( IsListEmpty( &SendQueue->NblTrackList ) )
        KeResetEvent( &SendQueue->NblQueueEmptyEvent );

    InsertTailList( &SendQueue->NblTrackList, &NblContext->NblQLink );
    InsertTailList( &SendQueue->NblDispatchQueue, &NblContext->DispatchQLink );

    return NDIS_STATUS_SUCCESS;
}

VOID
MbbSendQDequeueDssData(
    __in    PNET_BUFFER_LIST        NetBufferList,
    __in    PVOID                   Context
    )
{
    PMBB_SEND_QUEUE     SendQueue = (PMBB_SEND_QUEUE)Context;
    PMBB_NBL_CONTEXT    NblContext = (PMBB_NBL_CONTEXT)NET_BUFFER_LIST_MINIPORT_RESERVED( NetBufferList )[0];

    NET_BUFFER_LIST_MINIPORT_RESERVED( NetBufferList )[0] = NULL;

    if( NetBufferList->Status != NDIS_STATUS_SUCCESS )
    {
        TraceError( WMBCLASS_SEND, "[Send][NBL=0x%p] Completed with failed NdisStatus=%!STATUS!",
                    NetBufferList,
                    NetBufferList->Status
                    );
    }

    NET_BUFFER_LIST_MINIPORT_RESERVED( NetBufferList )[0] = NULL;
    NET_BUFFER_LIST_MINIPORT_RESERVED( NetBufferList )[1] = NblContext;

    // Call the completion handler
    MbbNdisDeviceServiceSessionSendComplete(
        NblContext->DssRequestHandle,
        NetBufferList->Status
        );

    // Free the Nbl & stuff associate with the request
    MbbCleanupDssNbl(SendQueue, NetBufferList);
        
    DrainRelease( &SendQueue->QueueDrainObject );

    MbbSendQLock(SendQueue, FALSE);
    if (RemoveEntryList(&NblContext->NblQLink))
        KeSetEvent(&SendQueue->NblQueueEmptyEvent, 0, FALSE);
    MbbSendQUnlock(SendQueue, FALSE);
}

//
//  Miniport handlers
//

NDIS_STATUS
MbbSendDeviceServiceSessionData(
    __in    PMINIPORT_ADAPTER_CONTEXT           Adapter,
    __in    MBB_REQUEST_HANDLE                  RequestHandle,
    __in    ULONG                               RequestId,
    __in    ULONG                               SessionId,
    __in    ULONG                               DataSize,
    __in    PVOID                               Data
    )
{
    NDIS_STATUS                 NdisStatus;
    PMBB_SEND_QUEUE             SendQueue=&Adapter->SendQueue;
    if (MbbBusIsFastIO(Adapter->BusHandle))
    {
        PNET_BUFFER_LIST    NetBufferList = NULL;
        PVOID               NblContext = NULL;
        USHORT NblContextSize = ALIGN(sizeof(MBB_NBL_TYPE), MEMORY_ALLOCATION_ALIGNMENT);
        MBB_NBL_TYPE NblType;

        if ((NetBufferList = MbbAllocateDssNbl(
            Adapter,
            SendQueue,
            RequestId,
            DataSize,
            Data)) == NULL)
        {
            TraceError(WMBCLASS_SEND, "[FastIOSend][DSS Write %d] FAILED to allocate Nbl", RequestId);
            return NDIS_STATUS_RESOURCES;
        }

#pragma prefast(suppress:__WARNING_MEMORY_LEAK, "By Design: Allocate NetBufferList from SendQueue->NblPool, released when FastIOSendNetBufferListsComplete.")
        if (NdisAllocateNetBufferListContext(
            NetBufferList,
            NblContextSize,
            0,
            MbbPoolTagNblFastIOContext) != NDIS_STATUS_SUCCESS)
        {
            TraceError(WMBCLASS_RECEIVE, "[FastIOSend][DSS NBL=0x%p] FAILED to allocate NetBufferListContext", NetBufferList);
            MbbCleanupDssNbl(SendQueue, NetBufferList);
            return NDIS_STATUS_RESOURCES;
        }

        NblContext = NET_BUFFER_LIST_CONTEXT_DATA_START(NetBufferList);
        *((MBB_NBL_TYPE*)NblContext) = MBB_NBL_TYPE_DSS;

        NET_BUFFER_LIST_MINIPORT_RESERVED(NetBufferList)[0] = RequestHandle;

        MbbBusSendNetBufferLists(Adapter->BusHandle, NetBufferList, SessionId, 0);
    }
    else {
        MbbSendQLock(SendQueue, FALSE);

        if (!DrainAddRef(&SendQueue->QueueDrainObject))
        {
            TraceError(WMBCLASS_SEND, "[Send][DSS Write %d] DROPPING", RequestId);
            MbbSendQUnlock(SendQueue, FALSE);
            return NDIS_STATUS_ADAPTER_NOT_READY;
        }

        if ((NdisStatus = MbbSendQQueueDssData(
            Adapter,
            SendQueue,
            RequestHandle,
            RequestId,
            SessionId,
            DataSize,
            Data
        )) != NDIS_STATUS_SUCCESS)
        {
            TraceError(WMBCLASS_SEND, "[Send][DSS Write %d] DROPPING, queuing failed", RequestId);

            MbbSendQUnlock(SendQueue, FALSE);

            //
            //  a ref was added for this DSS data, removed it
            //
            DrainRelease(&SendQueue->QueueDrainObject);
            return NdisStatus;
        }
        MbbSendQUnlock(SendQueue, FALSE);

        MbbSendQProcess(SendQueue, FALSE);
    }
    return NDIS_STATUS_SUCCESS;
}

