/***************************************************************************

Copyright (c) 2010 Microsoft Corporation

Module Name:
    Receive.c

Abstract:
    This module contains routines that parse data coming from the network.
    Verifications on  the receive path are very light. Alignment checks and 
    max size checks are not made. These constraints are meant for the device
    rather than for the host. NTBs are only checked for buffer overflow and
    malformed data.

Environment:
    kernel mode only

Notes:

Revision History:
    3/22/2011 : created

Authors:
    TriRoy

****************************************************************************/




////////////////////////////////////////////////////////////////////////////////
//
//  INCLUDES
//
////////////////////////////////////////////////////////////////////////////////
#include "precomp.h"
#include "receive.tmh"




////////////////////////////////////////////////////////////////////////////////
//
//  DEFINES
//
////////////////////////////////////////////////////////////////////////////////
#define MBB_ETHER_TYPE_IPV4 0x0800
#define MBB_ETHER_TYPE_IPV6 0x86dd




////////////////////////////////////////////////////////////////////////////////
//
//  TYPEDEFS
//
////////////////////////////////////////////////////////////////////////////////

typedef struct _MBB_NDIS_RECEIVE_NDP_CONTEXT
{
    LIST_ENTRY                  NblQueue;
    //
    // Resources
    //
    PNET_BUFFER_LIST            FirstNbl;
    PNET_BUFFER_LIST            CurrentNbl;
    ULONG                       NblCount;

    SESSIONID_PORTNUMBER_ENTRY  SessionIdPortNumberEntry;    
}MBB_NDIS_RECEIVE_NDP_CONTEXT,*PMBB_NDIS_RECEIVE_NDP_CONTEXT;

typedef struct _MBB_NDIS_RECEIVE_CONTEXT
{
    LIST_ENTRY                  ReceivedQLink;
    NDIS_SPIN_LOCK              Lock;

    //
    // Protected by lock
    //

    MBB_NDIS_RECEIVE_NDP_CONTEXT ReceiveNdpContext[MBB_MAX_NUMBER_OF_PORTS];
    
    //
    // Resources
    //
    PMDL                        Mdl;
    PNPAGED_LOOKASIDE_LIST      ReceiveLookasideList;
    PMBB_RECEIVE_QUEUE          RecvQueue;
    PMINIPORT_ADAPTER_CONTEXT   AdapterContext;
    MBB_BUS_HANDLE              BusHandle;
    MBB_RECEIVE_CONTEXT         BusContext;
    //
    // Parse state
    //
    ULONG                       NtbSequence;

} MBB_NDIS_RECEIVE_CONTEXT,
*PMBB_NDIS_RECEIVE_CONTEXT;

typedef struct _MBB_NBL_RECEIVE_CONTEXT
{
    LIST_ENTRY                  NblQLink;
    PNET_BUFFER_LIST            NetBufferList;
    PMBB_NDIS_RECEIVE_CONTEXT   Receive;
} MBB_NBL_RECEIVE_CONTEXT,
*PMBB_NBL_RECEIVE_CONTEXT;

typedef
__callback NDIS_STATUS
(*MBB_NDP_CALLBACK)(
    __in    PVOID   Nth,
    __in    PVOID   Ndp,
    __in    PVOID   Context
    );

typedef
__callback NDIS_STATUS
(*MBB_DATAGRAM_CALLBACK)(
    __in    PVOID   Nth,
    __in    ULONG   NdpSignature,
    __in    PVOID   Datagram,
    __in    ULONG   DatagramLength,
    __in    ULONG   SessionId,
    __in    PVOID   Context
    );




////////////////////////////////////////////////////////////////////////////////
//
//  PROTOTYPES
//
////////////////////////////////////////////////////////////////////////////////

//
// NTB
//

NDIS_STATUS
MbbNtbValidate(
    __in    PVOID       Nth,
    __in    ULONG       BufferLength,
    __in    BOOLEAN     Is32Bit
    );

NDIS_STATUS
MbbNtbParse(
    __in    PVOID               Nth,
    __in    ULONG               BufferLength,
    __in    MBB_NDP_CALLBACK    Callback,
    __in    PVOID               Context
    );

//
// NTB32
//

// Validation

NDIS_STATUS
MbbNtbValidateNth32(
    __in    PNCM_NTH32          Nth,
    __in    ULONG               BufferLength
    );

NDIS_STATUS
MbbNtbValidateNdp32(
    __in    PNCM_NTH32          Nth,
    __in    PNCM_NDP32          Ndp
    );

NDIS_STATUS
MbbNtbDetectNdp32Loop(
    __in PNCM_NTH32  Nth
    );

// Parsing

NDIS_STATUS
MbbNtbParseNth32(
    __in    PNCM_NTH32          Nth,
    __in    ULONG               BufferLength,
    __in    MBB_NDP_CALLBACK    Callback,
    __in    PVOID               Context
    );

NDIS_STATUS
MbbNtbParseNdp32(
    __in    PNCM_NTH32              Nth,
    __in    PNCM_NDP32              Ndp,
    __in    MBB_DATAGRAM_CALLBACK   Callback,
    __in    PVOID                   Context
    );

//
// NTB16
//

// Validation

NDIS_STATUS
MbbNtbValidateNth16(
    __in    PNCM_NTH16          Nth,
    __in    ULONG               BufferLength
    );

NDIS_STATUS
MbbNtbValidateNdp16(
    __in    PNCM_NTH16          Nth,
    __in    PNCM_NDP16          Ndp
    );

NDIS_STATUS
MbbNtbDetectNdp16Loop(
    __in PNCM_NTH16  Nth
    );

// Parsing

NDIS_STATUS
MbbNtbParseNth16(
    __in    PNCM_NTH16          Nth,
    __in    ULONG               BufferLength,
    __in    MBB_NDP_CALLBACK    Callback,
    __in    PVOID               Context
    );

NDIS_STATUS
MbbNtbParseNdp16(
    __in    PNCM_NTH16              Nth,
    __in    PNCM_NDP16              Ndp,
    __in    MBB_DATAGRAM_CALLBACK   Callback,
    __in    PVOID                   Context
    );

//
// RecvQ
//

__drv_maxIRQL(DISPATCH_LEVEL)
__drv_savesIRQLGlobal( SpinLock, RecvQueue->Lock )
__drv_when( DispatchLevel == FALSE, __drv_raisesIRQL(DISPATCH_LEVEL))
FORCEINLINE
VOID
MbbRecvQLock(
    __in __drv_at( RecvQueue->Lock, __drv_acquiresResource( Spinlock ) )
            PMBB_RECEIVE_QUEUE  RecvQueue,
    __in    BOOLEAN             DispatchLevel
    );

__drv_maxIRQL(DISPATCH_LEVEL)
__drv_restoresIRQLGlobal( SpinLock, RecvQueue->Lock )
FORCEINLINE
VOID
MbbRecvQUnlock(
    __in __drv_at( RecvQueue->Lock, __drv_releasesResource( Spinlock ) )
            PMBB_RECEIVE_QUEUE  RecvQueue,
    __in    BOOLEAN             DispatchLevel
    );

PMBB_NDIS_RECEIVE_CONTEXT
MbbRecvQQueueReceive(
    __in    PMBB_RECEIVE_QUEUE          RecvQueue,
    __in    PMINIPORT_ADAPTER_CONTEXT   Adapter,
    __in    MBB_RECEIVE_CONTEXT         BusContext,
    __in    PMDL                        Mdl
    );

VOID
MbbRecvQDequeueReceive(
    __in    PMBB_RECEIVE_QUEUE          RecvQueue,
    __in    PMBB_NDIS_RECEIVE_CONTEXT   Receive
    );

//
// Receive Context
//

NDIS_STATUS
MbbReceiveParseNdp(
    __in PVOID      Nth,
    __in PVOID      Ndp,
    __in PVOID      Context
    );

NDIS_STATUS
MbbReceiveAddNbl(
    __in    PVOID   Nth,
    __in    ULONG   NdpSignature,
    __in    PVOID   Datagram,
    __in    ULONG   DatagramLength,
    __in    ULONG   SessionId,
    __in    PVOID   Context
    );

NDIS_STATUS
MbbReceiveDssData(
    __in    PVOID   Nth,
    __in    ULONG   NdpSignature,
    __in    PVOID   Datagram,
    __in    ULONG   DatagramLength,
    __in    ULONG   SessionId,
    __in    PVOID   Context
    );

VOID
MbbReceiveRemoveNbl(
    __in    PMBB_NDIS_RECEIVE_CONTEXT   Receive,
    __in    PNET_BUFFER_LIST            NetBufferList
    );

VOID
MbbReceiveCleanupNbls(
    __in    PMBB_NDIS_RECEIVE_CONTEXT   Receive
    );

//
// NBL
//

__drv_allocatesMem(mem)
PNET_BUFFER_LIST
MbbNblAlloc(
    __in    PVOID                       Nth,
    __in    PVOID                       Datagram,
    __in    ULONG                       DatagramLength,
    __in    PMBB_NDIS_RECEIVE_CONTEXT   Receive
    );

VOID
MbbNblCleanup(
    __in    PNET_BUFFER_LIST    NetBufferList
    );

//
// CID RESPONSE / INDICATION PARSING
//

NTSTATUS
MbbNdisProcessCidData(
    __in_bcount(Length) PUCHAR      Buffer,
    __in    ULONG                   Length,
    __in    PMBB_REQUEST_MANAGER    RequestManager
    );

VOID
MbbNdisProcessDeviceServiceCommandResponse(
    __in_bcount(FragmentLength) PUCHAR  FragmentBuffer,
    __in    ULONG                       FragmentLength,
    __in    PMBB_REQUEST_MANAGER        RequestManager
    );

VOID
MbbNdisProcessDeviceServiceStatusIndication(
    __in_bcount(FragmentLength) PUCHAR  FragmentBuffer,
    __in    ULONG                       FragmentLength,
    __in    PMBB_REQUEST_MANAGER        RequestManager
    );

//
// Test
//

#if DBG

VOID
MbbTestValidateNblChain(
    __in    PNET_BUFFER_LIST    NblChain
    );

#endif


////////////////////////////////////////////////////////////////////////////////
//
//  IMPLEMENTATION
//
////////////////////////////////////////////////////////////////////////////////

//
// NTB
//

NDIS_STATUS
MbbNtbValidate(
    __in    PVOID       Nth,
    __in    ULONG       BufferLength,
    __in    BOOLEAN     Is32Bit
    )
{
    if( Is32Bit == TRUE )
        return MbbNtbValidateNth32( (PNCM_NTH32)Nth, BufferLength );
    else
        return MbbNtbValidateNth16( (PNCM_NTH16)Nth, BufferLength );
}

NDIS_STATUS
MbbNtbParse(
    __in    PVOID               Nth,
    __in    ULONG               BufferLength,
    __in    MBB_NDP_CALLBACK    Callback,
    __in    PVOID               Context
    )
/*++
    NOTE:
        Assumed that NTH has already been validated.
        No validations are performed during parsing.
--*/
{
    if( MBB_NTB_IS_32BIT( Nth ) )
    {
        return MbbNtbParseNth32(
                    ((PNCM_NTH32)Nth),
                    BufferLength,
                    Callback,
                    Context
                    );
    }
    else if( MBB_NTB_IS_16BIT( Nth ) )
    {
        return MbbNtbParseNth16(
                    ((PNCM_NTH16)Nth),
                    BufferLength,
                    Callback,
                    Context
                    );
    }
    else
    {
        TraceError( WMBCLASS_RECEIVE, "[NTB][Seq=0x%04x] BAD NTB signature=0x%04x",
                    MBB_NTB_GET_SEQUENCE( Nth ),
                    MBB_NTB_GET_SIGNATURE( Nth )
                    );
        return NDIS_STATUS_FAILURE;
    }
}

NDIS_STATUS
MbbNtbParseNdp(
    __in    PVOID                   Nth,
    __in    PVOID                   Ndp,
    __in    MBB_DATAGRAM_CALLBACK   Callback,
    __in    PVOID                   CallbackContext
    )
{
    NDIS_STATUS                 NdisStatus = NDIS_STATUS_SUCCESS;
    PMBB_NDIS_RECEIVE_CONTEXT   Receive = (PMBB_NDIS_RECEIVE_CONTEXT)CallbackContext;
    PMINIPORT_ADAPTER_CONTEXT   Adapter = Receive->AdapterContext;
    SESSIONID_PORTNUMBER_ENTRY  SessionIdPortNumberEntry = {0};       
    ULONG                       SessionId = MBB_INVALID_SESSION_ID; 
    BOOLEAN                     IsDeviceServiceStream = FALSE;
   
    do
    {
        if( MBB_NTB_IS_32BIT( Nth ) )
        {
            if(MBB_NDP32_GET_SIGNATURE_TYPE((PNCM_NDP32)Ndp) == NCM_NDP32_VENDOR)
            {
                IsDeviceServiceStream = TRUE;
            }
            else if(MBB_NDP32_GET_SIGNATURE_TYPE((PNCM_NDP32)Ndp) == NCM_NDP32_IPS)
            {
                SessionId = MBB_NDP32_GET_SESSIONID((PNCM_NDP32)Ndp );
            }
            else
            {
                ASSERT(FALSE);
            }
        }        
        else 
        {
            if(MBB_NDP16_GET_SIGNATURE_TYPE((PNCM_NDP16)Ndp) == NCM_NDP16_VENDOR)
            {
                IsDeviceServiceStream = TRUE;                
            }
            else if(MBB_NDP16_GET_SIGNATURE_TYPE((PNCM_NDP16)(Ndp)) == NCM_NDP16_IPS)
            {
                SessionId = MBB_NDP16_GET_SESSIONID((PNCM_NDP16)Ndp);        
            }
            else
            {
                ASSERT(FALSE);
            }
        }  

        if(!IsDeviceServiceStream)
        {
            if( SessionId >= MBB_MAX_NUMBER_OF_PORTS 
                    || SessionId == MBB_INVALID_SESSION_ID)
            {
                TraceError( WMBCLASS_RECEIVE, "[Recv][Seq=0x%04x] INVALID signature sessionid=%lu for NDP at offset=0x%08x",
                            MBB_NTB_GET_SEQUENCE( Nth ),
                            SessionId,
                            MBB_NTB_GET_OFFSET( Nth, Ndp )
                          );            
                NdisStatus = NDIS_STATUS_INVALID_PARAMETER;
                break;
            }

            MbbAdapterSessionIdPortTableLock(Adapter);

            SessionIdPortNumberEntry = Adapter->SessionIdPortTable[SessionId];

            MbbAdapterSessionIdPortTableUnlock(Adapter);

            if(!SessionIdPortNumberEntry.InUse)
            {
                //discard
                TraceError( WMBCLASS_RECEIVE, "[Recv][Seq=0x%04x] FAILED to parse Ndp as session id is not in use.Session Id is %lu",
                            MBB_NTB_GET_SEQUENCE( Nth ),
                            SessionId                        
                            );
                NdisStatus = NDIS_STATUS_INVALID_PARAMETER;
                break;    
            }

            Receive->ReceiveNdpContext[SessionId].SessionIdPortNumberEntry = SessionIdPortNumberEntry;
        }

        if( MBB_NTB_IS_32BIT( Nth ) )
        {
            NdisStatus = MbbNtbParseNdp32(
                            (PNCM_NTH32)Nth,
                            (PNCM_NDP32)Ndp,
                            Callback,
                            CallbackContext
                            );
        }
        else
        {
            NdisStatus = MbbNtbParseNdp16(
                            (PNCM_NTH16)Nth,
                            (PNCM_NDP16)Ndp,
                            Callback,
                            CallbackContext
                            );
        }
    }while(FALSE);

    return NdisStatus;
}

//
// NTB32
//

NDIS_STATUS
MbbNtbDetectNdp32Loop(
    __in PNCM_NTH32  Nth
    )
{
    PNCM_NDP32  FirstNdp;
    PNCM_NDP32  Ndp;
    PNCM_NDP32  LoopNdp;

    if( (FirstNdp = MBB_NTH32_GET_FIRST_NDP( Nth )) == NULL )
        return NDIS_STATUS_SUCCESS;

    if( ! MBB_NTB32_IS_VALID_NDP_LENGTH( Nth, FirstNdp ) )
    {
        TraceError( WMBCLASS_RECEIVE, "[Recv][Seq=0x%04x] INVALID NDP at offset=0x%08x, cannot detect loop",
                    MBB_NTB_GET_SEQUENCE( Nth ),
                    MBB_NTB_GET_OFFSET( Nth, FirstNdp )
                    );
        return NDIS_STATUS_FAILURE;
    }
    LoopNdp = MBB_NDP32_GET_NEXT_NDP( Nth, FirstNdp );

    for( Ndp  = FirstNdp;
         Ndp != NULL && LoopNdp != NULL;
         Ndp  = MBB_NDP32_GET_NEXT_NDP( Nth, Ndp ) )
    {
        if( ! MBB_NTB32_IS_VALID_NDP_LENGTH( Nth, LoopNdp ) )
        {
            TraceError( WMBCLASS_RECEIVE, "[Recv][Seq=0x%04x] INVALID NDP at offset=0x%08x, cannot detect loop",
                        MBB_NTB_GET_SEQUENCE( Nth ),
                        MBB_NTB_GET_OFFSET( Nth, LoopNdp )
                        );
            return NDIS_STATUS_FAILURE;
        }

        if( ! MBB_NTB32_IS_VALID_NDP_LENGTH( Nth, Ndp ) )
        {
            TraceError( WMBCLASS_RECEIVE, "[Recv][Seq=0x%04x] INVALID NDP at offset=0x%08x, cannot detect loop",
                        MBB_NTB_GET_SEQUENCE( Nth ),
                        MBB_NTB_GET_OFFSET( Nth, Ndp )
                        );
            return NDIS_STATUS_FAILURE;
        }

        if( LoopNdp == Ndp )
        {
            TraceError( WMBCLASS_RECEIVE, "[Recv][Seq=0x%04x] Loop detected on NDP at offset=0x%08x",
                        MBB_NTB_GET_SEQUENCE( Nth ),
                        MBB_NTB_GET_OFFSET( Nth, Ndp )
                        );
            return NDIS_STATUS_FAILURE;
        }

        if( (LoopNdp = MBB_NDP32_GET_NEXT_NDP( Nth, LoopNdp )) != NULL )
        {
            if( ! MBB_NTB32_IS_VALID_NDP_LENGTH( Nth, LoopNdp ) )
            {
                TraceError( WMBCLASS_RECEIVE, "[Recv][Seq=0x%04x] INVALID NDP at offset=0x%08x, cannot detect loop",
                            MBB_NTB_GET_SEQUENCE( Nth ),
                            MBB_NTB_GET_OFFSET( Nth, LoopNdp )
                            );
                return NDIS_STATUS_FAILURE;
            }
            LoopNdp = MBB_NDP32_GET_NEXT_NDP( Nth, LoopNdp );
        }
    }
    return NDIS_STATUS_SUCCESS;
}

NDIS_STATUS
MbbNtbValidateNdp32(
    __in    PNCM_NTH32          Nth,
    __in    PNCM_NDP32          Ndp
    )
{
    ULONG               Index;
    ULONG               DatagramCount = MBB_NDP32_GET_DATAGRAM_COUNT(Ndp);
    PNCM_NDP32_DATAGRAM Datagram;

    if( ! MBB_NTB32_IS_VALID_NDP_SIGNATURE( Ndp ) )
    {
            TraceError( WMBCLASS_RECEIVE, "[Recv][Seq=0x%04x] INVALID signature=0x%08x for NDP at offset=0x%08x",
                        MBB_NTB_GET_SEQUENCE( Nth ),
                        MBB_NDP32_GET_SIGNATURE( Ndp ),
                        MBB_NTB_GET_OFFSET( Nth, Ndp )
                        );
        return NDIS_STATUS_FAILURE;
    }

    if (MBB_NDP32_GET_SIGNATURE_TYPE( Ndp) == NCM_NDP32_IPS)
    {
        //
        // Check if the session id is valid, else discard
        //

        // Check if session Id is greater than maximum supported. Here
        // we can also check against MaxActivatedContexts, but this would
        // mean we need to take a lock for getting the value of maxactivatedcontexts
        // The lock can be avoided by checking against the maximum number of ports
        // supported by the class driver. SESSION_PORT_TABLE.InUse can be used 
        // to check whether the session id is in use.
            
        if( MBB_NDP32_GET_SESSIONID( Ndp ) >= MBB_MAX_NUMBER_OF_PORTS 
            || MBB_NDP32_GET_SESSIONID( Ndp ) == MBB_INVALID_SESSION_ID)
        {
            TraceError( WMBCLASS_RECEIVE, "[Recv][Seq=0x%04x] INVALID signature sessionid=0x%02x for NDP at offset=0x%08x",
                            MBB_NTB_GET_SEQUENCE( Nth ),
                            (UCHAR)MBB_NDP32_GET_SESSIONID( Ndp ),
                            MBB_NTB_GET_OFFSET( Nth, Ndp )
                            );            
            return NDIS_STATUS_FAILURE;
        }
    }
  
    if( ! MBB_NTB32_IS_VALID_NDP_LENGTH( Nth, Ndp ) )
    {
            TraceError( WMBCLASS_RECEIVE, "[Recv][Seq=0x%04x] INVALID length=0x%08x for NDP at offset=0x%08x",
                        MBB_NTB_GET_SEQUENCE( Nth ),
                        MBB_NDP32_GET_LENGTH( Ndp ),
                        MBB_NTB_GET_OFFSET( Nth, Ndp )
                        );
        return NDIS_STATUS_FAILURE;
    }

    for( Index = 0;
         Index < DatagramCount;
         Index ++ )
    {
        if( MBB_NTB32_IS_END_DATAGRAM( Nth, Ndp, Index ) )
            return NDIS_STATUS_SUCCESS;

        if( ! MBB_NTB32_IS_VALID_DATAGRAM( Nth, Ndp, Index ) )
        {
            TraceError( WMBCLASS_RECEIVE, "[Recv][Seq=0x%04x] INVALID datagram at index=%02d for NDP at offset=0x%08x",
                        MBB_NTB_GET_SEQUENCE( Nth ),
                        Index,
                        MBB_NTB_GET_OFFSET( Nth, Ndp )
                        );
            return NDIS_STATUS_FAILURE;
        }
    }
    return NDIS_STATUS_SUCCESS;
}

NDIS_STATUS
MbbNtbValidateNth32(
    __in    PNCM_NTH32          Nth,
    __in    ULONG               BufferLength
    )
{
    PNCM_NDP32 Ndp;

    if( BufferLength < sizeof(NCM_NTH32) )
    {
        TraceError( WMBCLASS_RECEIVE, "[Recv] INVALID NTH Buffer length=0x%08x", BufferLength );
        return NDIS_STATUS_FAILURE;
    }

    if( ! MBB_NTH32_IS_VALID_SIGNATURE( Nth ) )
    {
        TraceError( WMBCLASS_RECEIVE, "[Recv] INVALID NTH signature=0x%08x", MBB_NTB_GET_SIGNATURE( Nth ) );
        return NDIS_STATUS_FAILURE;
    }

    if( ! MBB_NTH32_IS_VALID_HEADER_LENGTH( Nth ) )
    {
        TraceError( WMBCLASS_RECEIVE, "[Recv] INVALID NTH header length=0x%08x", MBB_NTB_GET_HEADER_LENGTH( Nth ) );
        return NDIS_STATUS_FAILURE;
    }

    if( ! MBB_NTH32_IS_VALID_BLOCK_LENGTH( Nth, BufferLength ) )
    {
        TraceError( WMBCLASS_RECEIVE, "[Recv] INVALID NTH block length=0x%08x", MBB_NTH32_GET_BLOCK_LENGTH( Nth ) );
        return NDIS_STATUS_FAILURE;
    }

    if( ! MBB_NTH32_IS_VALID_FIRST_NDP( Nth ) )
    {
        TraceError( WMBCLASS_RECEIVE, "[Recv] INVALID NTH first NDP offset=0x%08x", MBB_NTH32_GET_FIRST_NDP_OFFSET( Nth ) );
        return NDIS_STATUS_FAILURE;
    }

    if( MbbNtbDetectNdp32Loop( Nth ) != NDIS_STATUS_SUCCESS )
        return NDIS_STATUS_FAILURE;

    for( Ndp  = MBB_NTH32_GET_FIRST_NDP( Nth );
         Ndp != NULL;
         Ndp  = MBB_NDP32_GET_NEXT_NDP( Nth, Ndp ) )
    {
        if( MbbNtbValidateNdp32( Nth, Ndp ) != NDIS_STATUS_SUCCESS )
            return NDIS_STATUS_FAILURE;
    }

    return NDIS_STATUS_SUCCESS;
}

NDIS_STATUS
MbbNtbParseNdp32(
    __in    PNCM_NTH32              Nth,
    __in    PNCM_NDP32              Ndp,
    __in    MBB_DATAGRAM_CALLBACK   Callback,
    __in    PVOID                   Context
    )
{
    ULONG           Index;
    ULONG           DatagramCount = MBB_NDP32_GET_DATAGRAM_COUNT(Ndp);
    NDIS_STATUS     NdisStatus = NDIS_STATUS_SUCCESS;
    ULONG           SessionId  = MBB_NDP32_GET_SESSIONID( Ndp );

    for( Index = 0;
         Index < DatagramCount;
         Index ++ )
    {
        if( MBB_NTB32_IS_END_DATAGRAM( Nth, Ndp, Index ) )
            break;

        if( ! MBB_NTB32_IS_VALID_DATAGRAM( Nth, Ndp, Index ) )
        {
            TraceError( WMBCLASS_RECEIVE, "[Recv][Seq=0x%04x] INVALID datagram at index=%02d for NDP at offset=0x%08x",
                        MBB_NTB_GET_SEQUENCE( Nth ),
                        Index,
                        MBB_NTB_GET_OFFSET( Nth, Ndp )
                        );
            NdisStatus = NDIS_STATUS_FAILURE;
            break;
        }

        if( (NdisStatus = Callback(
                                Nth,
                                MBB_NDP32_GET_SIGNATURE( Ndp ),
                                MBB_NDP32_GET_DATAGRAM( Nth, Ndp, Index ),
                                MBB_NDP32_GET_DATAGRAM_LENGTH(Ndp,Index),
                                SessionId,
                                Context
                                )) != NDIS_STATUS_SUCCESS )
        {
            break;
        }
    }

    return NdisStatus;
}

NDIS_STATUS
MbbNtbParseNth32(
    __in    PNCM_NTH32          Nth,
    __in    ULONG               BufferLength,
    __in    MBB_NDP_CALLBACK    Callback,
    __in    PVOID               Context
    )
/*++
    NOTE:
        Assumed that NTH has already been validated.
        No validations are performed during parsing.
--*/
{
    PNCM_NDP32                  Ndp;
    NDIS_STATUS                 NdisStatus = NDIS_STATUS_SUCCESS;   

    for( Ndp  = MBB_NTH32_GET_FIRST_NDP( Nth );
         Ndp != NULL;
         Ndp  = MBB_NDP32_GET_NEXT_NDP( Nth, Ndp ) )
    {   
        if( (NdisStatus = Callback(
                            Nth,
                            Ndp,
                            Context 
                            )) != NDIS_STATUS_SUCCESS )
        {
            break;
        }
    }
    return NdisStatus;
}

//
// NTB16
//

NDIS_STATUS
MbbNtbDetectNdp16Loop(
    __in PNCM_NTH16  Nth
    )
{
    PNCM_NDP16  FirstNdp;
    PNCM_NDP16  Ndp;
    PNCM_NDP16  LoopNdp;

    if( (FirstNdp = MBB_NTH16_GET_FIRST_NDP( Nth )) == NULL )
        return NDIS_STATUS_SUCCESS;

    if( ! MBB_NTB16_IS_VALID_NDP_LENGTH( Nth, FirstNdp ) )
    {
        TraceError( WMBCLASS_RECEIVE, "[Recv][Seq=0x%04x] INVALID NDP at offset=0x%08x, cannot detect loop",
                    MBB_NTB_GET_SEQUENCE( Nth ),
                    MBB_NTB_GET_OFFSET( Nth, FirstNdp )
                    );
        return NDIS_STATUS_FAILURE;
    }
    LoopNdp = MBB_NDP16_GET_NEXT_NDP( Nth, FirstNdp );

    for( Ndp  = FirstNdp;
         Ndp != NULL && LoopNdp != NULL;
         Ndp  = MBB_NDP16_GET_NEXT_NDP( Nth, Ndp ) )
    {
        if( ! MBB_NTB16_IS_VALID_NDP_LENGTH( Nth, LoopNdp ) )
        {
            TraceError( WMBCLASS_RECEIVE, "[Recv][Seq=0x%04x] INVALID NDP at offset=0x%08x, cannot detect loop",
                        MBB_NTB_GET_SEQUENCE( Nth ),
                        MBB_NTB_GET_OFFSET( Nth, LoopNdp )
                        );
            return NDIS_STATUS_FAILURE;
        }

        if( ! MBB_NTB16_IS_VALID_NDP_LENGTH( Nth, Ndp ) )
        {
            TraceError( WMBCLASS_RECEIVE, "[Recv][Seq=0x%04x] INVALID NDP at offset=0x%08x, cannot detect loop",
                        MBB_NTB_GET_SEQUENCE( Nth ),
                        MBB_NTB_GET_OFFSET( Nth, Ndp )
                        );
            return NDIS_STATUS_FAILURE;
        }

        if( LoopNdp == Ndp )
        {
            TraceError( WMBCLASS_RECEIVE, "[Recv][Seq=0x%04x] Loop detected on NDP at offset=0x%08x",
                        MBB_NTB_GET_SEQUENCE( Nth ),
                        MBB_NTB_GET_OFFSET( Nth, Ndp )
                        );
            return NDIS_STATUS_FAILURE;
        }

        if( (LoopNdp = MBB_NDP16_GET_NEXT_NDP( Nth, LoopNdp )) != NULL )
        {
            if( ! MBB_NTB16_IS_VALID_NDP_LENGTH( Nth, LoopNdp ) )
            {
                TraceError( WMBCLASS_RECEIVE, "[Recv][Seq=0x%04x] INVALID NDP at offset=0x%08x, cannot detect loop",
                            MBB_NTB_GET_SEQUENCE( Nth ),
                            MBB_NTB_GET_OFFSET( Nth, LoopNdp )
                            );
                return NDIS_STATUS_FAILURE;
            }
            LoopNdp = MBB_NDP16_GET_NEXT_NDP( Nth, LoopNdp );
        }
    }
    return NDIS_STATUS_SUCCESS;
}

NDIS_STATUS
MbbNtbValidateNdp16(
    __in    PNCM_NTH16          Nth,
    __in    PNCM_NDP16          Ndp
    )
{
    ULONG               Index;
    ULONG               DatagramCount = MBB_NDP16_GET_DATAGRAM_COUNT(Ndp);
    PNCM_NDP16_DATAGRAM Datagram;

    if( ! MBB_NTB16_IS_VALID_NDP_SIGNATURE( Ndp ) )
    {
            TraceError( WMBCLASS_RECEIVE, "[Recv][Seq=0x%04x] INVALID signature=0x%08x for NDP at offset=0x%08x",
                        MBB_NTB_GET_SEQUENCE( Nth ),
                        MBB_NDP16_GET_SIGNATURE( Ndp ),
                        MBB_NTB_GET_OFFSET( Nth, Ndp )
                        );
        return NDIS_STATUS_FAILURE;
    }

    
    if (MBB_NDP16_GET_SIGNATURE_TYPE( Ndp) == NCM_NDP16_IPS)
    {
        //
        // Check if the session id is valid, else discard
        //

        // Check if session Id is greater than maximum supported. Here
        // we can also check against MaxActivatedContexts, but this would
        // mean we need to take a lock for getting the value of maxactivatedcontexts
        // The lock can be avoided by checking against the maximum number of ports
        // supported by the class driver. SESSION_PORT_TABLE.InUse can be used 
        // to check whether the session id is in use.
        
        if( MBB_NDP16_GET_SESSIONID( Ndp ) >= MBB_MAX_NUMBER_OF_PORTS
                || MBB_NDP16_GET_SESSIONID( Ndp ) == MBB_INVALID_SESSION_ID )
        {
                TraceError( WMBCLASS_RECEIVE, "[Recv][Seq=0x%04x] INVALID signature sessionid=0x%02x for NDP at offset=0x%08x",
                            MBB_NTB_GET_SEQUENCE( Nth ),
                            (UCHAR)MBB_NDP16_GET_SESSIONID( Ndp ),
                            MBB_NTB_GET_OFFSET( Nth, Ndp )
                            );
            return NDIS_STATUS_FAILURE;
        }
    }
  
    if( ! MBB_NTB16_IS_VALID_NDP_LENGTH( Nth, Ndp ) )
    {
            TraceError( WMBCLASS_RECEIVE, "[Recv][Seq=0x%04x] INVALID length=0x%08x for NDP at offset=0x%08x",
                        MBB_NTB_GET_SEQUENCE( Nth ),
                        MBB_NDP16_GET_LENGTH( Ndp ),
                        MBB_NTB_GET_OFFSET( Nth, Ndp )
                        );
        return NDIS_STATUS_FAILURE;
    }

    for( Index = 0;
         Index < DatagramCount;
         Index ++ )
    {
        if( MBB_NTB16_IS_END_DATAGRAM( Nth, Ndp, Index ) )
            return NDIS_STATUS_SUCCESS;

        if( ! MBB_NTB16_IS_VALID_DATAGRAM( Nth, Ndp, Index ) )
        {
            TraceError( WMBCLASS_RECEIVE, "[Recv][Seq=0x%04x] INVALID datagram at index=%02d for NDP at offset=0x%08x",
                        MBB_NTB_GET_SEQUENCE( Nth ),
                        Index,
                        MBB_NTB_GET_OFFSET( Nth, Ndp )
                        );
            return NDIS_STATUS_FAILURE;
        }
    }
    return NDIS_STATUS_SUCCESS;
}

NDIS_STATUS
MbbNtbValidateNth16(
    __in    PNCM_NTH16          Nth,
    __in    ULONG               BufferLength
    )
{
    PNCM_NDP16 Ndp;

    if( BufferLength < sizeof(NCM_NTH16) )
    {
        TraceError( WMBCLASS_RECEIVE, "[Recv] INVALID NTH Buffer length=0x%08x", BufferLength );
        return NDIS_STATUS_FAILURE;
    }

    if( ! MBB_NTH16_IS_VALID_SIGNATURE( Nth ) )
    {
        TraceError( WMBCLASS_RECEIVE, "[Recv] INVALID NTH signature=0x%08x", MBB_NTB_GET_SIGNATURE( Nth ) );
        return NDIS_STATUS_FAILURE;
    }

    if( ! MBB_NTH16_IS_VALID_HEADER_LENGTH( Nth ) )
    {
        TraceError( WMBCLASS_RECEIVE, "[Recv] INVALID NTH header length=0x%08x", MBB_NTB_GET_HEADER_LENGTH( Nth ) );
        return NDIS_STATUS_FAILURE;
    }

    if( ! MBB_NTH16_IS_VALID_BLOCK_LENGTH( Nth, BufferLength ) )
    {
        TraceError( WMBCLASS_RECEIVE, "[Recv] INVALID NTH block length=0x%08x", MBB_NTH16_GET_BLOCK_LENGTH( Nth ) );
        return NDIS_STATUS_FAILURE;
    }

    if( ! MBB_NTH16_IS_VALID_FIRST_NDP( Nth ) )
    {
        TraceError( WMBCLASS_RECEIVE, "[Recv] INVALID NTH first NDP offset=0x%08x", MBB_NTH16_GET_FIRST_NDP_OFFSET( Nth ) );
        return NDIS_STATUS_FAILURE;
    }

    if( MbbNtbDetectNdp16Loop( Nth ) != NDIS_STATUS_SUCCESS )
        return NDIS_STATUS_FAILURE;

    for( Ndp  = MBB_NTH16_GET_FIRST_NDP( Nth );
         Ndp != NULL;
         Ndp  = MBB_NDP16_GET_NEXT_NDP( Nth, Ndp ) )
    {
        if( MbbNtbValidateNdp16( Nth, Ndp ) != NDIS_STATUS_SUCCESS )
            return NDIS_STATUS_FAILURE;
    }

    return NDIS_STATUS_SUCCESS;
}

NDIS_STATUS
MbbNtbParseNdp16(
    __in    PNCM_NTH16              Nth,
    __in    PNCM_NDP16              Ndp,
    __in    MBB_DATAGRAM_CALLBACK   Callback,
    __in    PVOID                   Context
    )
{
    ULONG           Index;
    ULONG           DatagramCount = MBB_NDP16_GET_DATAGRAM_COUNT(Ndp);
    NDIS_STATUS     NdisStatus = NDIS_STATUS_SUCCESS;
    ULONG           SessionId = MBB_NDP16_GET_SESSIONID( Ndp ); 

    for( Index = 0;
         Index < DatagramCount;
         Index ++ )
    {
        if( MBB_NTB16_IS_END_DATAGRAM( Nth, Ndp, Index ) )
            break;

        if( ! MBB_NTB16_IS_VALID_DATAGRAM( Nth, Ndp, Index ) )
        {
            TraceError( WMBCLASS_RECEIVE, "[Recv][Seq=0x%04x] INVALID datagram at index=%02d for NDP at offset=0x%08x",
                        MBB_NTB_GET_SEQUENCE( Nth ),
                        Index,
                        MBB_NTB_GET_OFFSET( Nth, Ndp )
                        );
            NdisStatus = NDIS_STATUS_FAILURE;
            break;
        }

        if( (NdisStatus = Callback(
                                Nth,
                                MBB_NDP16_GET_SIGNATURE( Ndp ),
                                MBB_NDP16_GET_DATAGRAM( Nth, Ndp, Index ),
                                MBB_NDP16_GET_DATAGRAM_LENGTH(Ndp,Index),
                                SessionId,
                                Context
                                )) != NDIS_STATUS_SUCCESS )
        {
            break;
        }
    }

    return NdisStatus;
}

NDIS_STATUS
MbbNtbParseNth16(
    __in    PNCM_NTH16          Nth,
    __in    ULONG               BufferLength,
    __in    MBB_NDP_CALLBACK    Callback,
    __in    PVOID               Context
    )
/*++
    NOTE:
        Assumed that NTH has already been validated.
        No validations are performed during parsing.
--*/
{
    PNCM_NDP16                  Ndp;
    NDIS_STATUS                 NdisStatus = NDIS_STATUS_SUCCESS;
    
    for( Ndp  = MBB_NTH16_GET_FIRST_NDP( Nth );
         Ndp != NULL;
         Ndp  = MBB_NDP16_GET_NEXT_NDP( Nth, Ndp ) )
    {
        if( (NdisStatus = Callback(
                            Nth,
                            Ndp,
                            Context 
                            )) != NDIS_STATUS_SUCCESS )
        {
            break;
        }
    }
    return NdisStatus;
}

//
// NBL
//

__drv_allocatesMem(mem)
PNET_BUFFER_LIST
MbbNblAlloc(
    __in    PVOID                       Nth,
    __in    PVOID                       Datagram,
    __in    ULONG                       DatagramLength,
    __in    PMBB_NDIS_RECEIVE_CONTEXT   Receive
    )
{
    BOOLEAN                     Success = FALSE;
    NDIS_STATUS                 NdisStatus;
    USHORT                      NblContextSize = ALIGN( sizeof(MBB_NBL_RECEIVE_CONTEXT), MEMORY_ALLOCATION_ALIGNMENT );
    PNET_BUFFER_LIST            NetBufferList = NULL;
    PMBB_NBL_RECEIVE_CONTEXT    NblContext;

    do
    {
        if( (NetBufferList = NdisAllocateNetBufferAndNetBufferList(
                                Receive->RecvQueue->NblPool,
                                NblContextSize,
                                0,
                                Receive->Mdl,
                                MBB_NTB_GET_OFFSET( Nth, Datagram ),
                                DatagramLength
                                )) == NULL )
        {
            TraceError( WMBCLASS_RECEIVE, "[Recv][Seq=0X%04X] FAILED to allocate NetBufferList", MBB_NTB_GET_SEQUENCE( Nth ) );
            break;
        }

#pragma prefast(suppress:__WARNING_MEMORY_LEAK, "By Design: Allocate a net buffer list from a pool, released when miniport returns buffer list.")
        if( NdisAllocateNetBufferListContext(
                NetBufferList,
                NblContextSize,
                0,
                MbbPoolTagNblReceive
                ) != NDIS_STATUS_SUCCESS )
        {
            TraceError( WMBCLASS_RECEIVE, "[Recv][Seq=0X%04X] FAILED to allocate NetBufferListContext", MBB_NTB_GET_SEQUENCE( Nth ) );
            break;
        }

        TraceVerbose( WMBCLASS_RECEIVE, "[Recv][Seq=0x%04x][NBL=0x%p] Added NBL",
                      MBB_NTB_GET_SEQUENCE( Nth ),
                      NetBufferList
                      );

        ASSERT( NET_BUFFER_CURRENT_MDL( NET_BUFFER_LIST_FIRST_NB( NetBufferList ) ) != NULL );

        if( DatagramLength >= sizeof(IPV4_HEADER) &&
            ((IPV4_HEADER*)Datagram)->Version == IPV4_VERSION )
        {
            NdisSetNblFlag( NetBufferList, NDIS_NBL_FLAGS_IS_IPV4 );
            NET_BUFFER_LIST_INFO( NetBufferList, NetBufferListFrameType ) = UlongToPtr(RtlUshortByteSwap( MBB_ETHER_TYPE_IPV4 ));
        } 
        else
        if( DatagramLength >= sizeof(IPV6_HEADER) &&
            (((IPV6_HEADER*)Datagram)->VersionClassFlow & IP_VER_MASK) == IPV6_VERSION )
        {
            NdisSetNblFlag( NetBufferList, NDIS_NBL_FLAGS_IS_IPV6 );
            NET_BUFFER_LIST_INFO( NetBufferList, NetBufferListFrameType ) = UlongToPtr(RtlUshortByteSwap( MBB_ETHER_TYPE_IPV6 ));
        }
        else
        {
            TraceError( WMBCLASS_RECEIVE, "[Recv][Seq=0X%04X] INVALID IP Header, DatagramLength=%d Version=%d",
                        MBB_NTB_GET_SEQUENCE( Nth ),
                        DatagramLength,
                        (DatagramLength > 0)? *((PUCHAR)Datagram): 0
                        );
            break;
        }
        NET_BUFFER_LIST_NEXT_NBL( NetBufferList ) = NULL;

        NblContext = (PMBB_NBL_RECEIVE_CONTEXT) NET_BUFFER_LIST_CONTEXT_DATA_START( NetBufferList );
        NblContext->NetBufferList    = NetBufferList;
        NblContext->Receive          = Receive;

        Success = TRUE;
    }
    while( FALSE );

    if( Success != TRUE )
    {
        if( NetBufferList != NULL )
        {
            TraceVerbose( WMBCLASS_RECEIVE, "[Recv][Seq=0x%04x][NBL=0x%p] Removed failed NBL",
                          MBB_NTB_GET_SEQUENCE( Nth ),
                          NetBufferList
                          );
            NdisFreeNetBufferList( NetBufferList );
            NetBufferList = NULL;
        }
    }
    return NetBufferList;
}

VOID
MbbNblCleanup(
    __in    PNET_BUFFER_LIST    NetBufferList
    )
{
    USHORT                      NblContextSize = ALIGN( sizeof(MBB_NBL_RECEIVE_CONTEXT), MEMORY_ALLOCATION_ALIGNMENT );
    PNET_BUFFER                 NetBuffer;
    PNET_BUFFER                 NextNetBuffer;
    PMBB_NBL_RECEIVE_CONTEXT    NblContext;

    NblContext = (PMBB_NBL_RECEIVE_CONTEXT) NET_BUFFER_LIST_CONTEXT_DATA_START( NetBufferList );
    TraceVerbose( WMBCLASS_RECEIVE, "[Recv][Seq=0x%04x][NBL=0x%p] Removed NBL",
                    NblContext->Receive->NtbSequence,
                    NetBufferList
                    );
    NblContext->Receive = MBB_BAD_POINTER;
    NdisFreeNetBufferListContext( NetBufferList, NblContextSize );
    NdisFreeNetBufferList( NetBufferList );
}

//
// Receive Context
//

NDIS_STATUS
MbbReceiveParseNdp(
    __in PVOID      Nth,
    __in PVOID      Ndp,
    __in PVOID      Context
    )
/*++
    Just a wrapper routine. It stores the
    current NDP type which will be used for all
    DPs within the NDP.
--*/
{
    NDIS_STATUS                 NdisStatus;
    PMBB_NDIS_RECEIVE_CONTEXT   Receive = (PMBB_NDIS_RECEIVE_CONTEXT)Context;
    MBB_DATAGRAM_CALLBACK       Callback = MbbReceiveAddNbl;

    // If this is a device service stream NDP, switch the callback
    if( MBB_NTB_IS_16BIT( Nth ) )
    {
        if( MBB_NDP16_GET_SIGNATURE_TYPE( (PNCM_NDP16)Ndp ) == NCM_NDP16_VENDOR )
        {
            // Vendor
            Callback = MbbReceiveDssData;
        }
    }
    else
    {
        if( MBB_NDP32_GET_SIGNATURE_TYPE( (PNCM_NDP32)Ndp ) == NCM_NDP32_VENDOR )
        {
            // Vendor
            Callback = MbbReceiveDssData;
        }
    }

    if( (NdisStatus = MbbNtbParseNdp(
                        Nth,
                        Ndp,
                        Callback,
                        Context
                        )) != NDIS_STATUS_SUCCESS )
    {
        TraceError( WMBCLASS_RECEIVE, "[Recv][Seq=0X%04X] FAILED to parse NDP at offset=0x%08x",
                    MBB_NTB_GET_SEQUENCE( Nth ),
                    MBB_NTB_GET_OFFSET( Nth, Ndp )
                    );
    }
    return NdisStatus;
}

NDIS_STATUS
MbbReceiveAddNbl(
    __in    PVOID   Nth,
    __in    ULONG   NdpSignature,
    __in    PVOID   Datagram,
    __in    ULONG   DatagramLength,
    __in    ULONG   SessionId,
    __in    PVOID   Context
    )
{
    NDIS_STATUS                 NdisStatus = NDIS_STATUS_SUCCESS;
    PNET_BUFFER_LIST            NetBufferList;
    PMBB_NBL_RECEIVE_CONTEXT    NblContext;
    PMBB_NDIS_RECEIVE_CONTEXT   Receive = (PMBB_NDIS_RECEIVE_CONTEXT)Context;
    PMBB_NDIS_RECEIVE_NDP_CONTEXT ReceiveNdpContext = &(Receive->ReceiveNdpContext[SessionId]);

    do
    {
        if( (NetBufferList = MbbNblAlloc(
                                Nth,
                                Datagram,
                                DatagramLength,
                                Receive
                                )) == NULL )
        {
            TraceError( WMBCLASS_RECEIVE, "[Recv][Seq=0x%04x] FAILED to allocate NetBufferList for Datagram at offset=0x%08x",
                        MBB_NTB_GET_SEQUENCE( Nth ),
                        MBB_NTB_GET_OFFSET( Nth, Datagram )
                        );
            NdisStatus = NDIS_STATUS_RESOURCES;
            break;
        }
        NblContext = (PMBB_NBL_RECEIVE_CONTEXT) NET_BUFFER_LIST_CONTEXT_DATA_START( NetBufferList );
        //
        // While building the NBL list we dont need the
        // Receive lock since the parsing is serialized
        //
        InsertTailList( &ReceiveNdpContext->NblQueue, &NblContext->NblQLink );

        if( ReceiveNdpContext->CurrentNbl != NULL )
            NET_BUFFER_LIST_NEXT_NBL(ReceiveNdpContext->CurrentNbl) = NetBufferList;
        else
            ReceiveNdpContext->FirstNbl = NetBufferList;

        ReceiveNdpContext->CurrentNbl = NetBufferList;
        ReceiveNdpContext->NblCount++;
    }
    while( FALSE );
    //
    // No cleanup required, caller will cleanup the 
    // Receive context thus freeing all NBLs and NBs.
    //
    return NdisStatus;
}

VOID
MbbReceiveRemoveNbl(
    __in    PMBB_NDIS_RECEIVE_CONTEXT   Receive,
    __in    PNET_BUFFER_LIST            NetBufferList
    )
{
    UINT NonEmptyNblQueue = 0;
    UINT ReceiveNdpIndex = 0;
    PMBB_NBL_RECEIVE_CONTEXT NblContext = (PMBB_NBL_RECEIVE_CONTEXT) NET_BUFFER_LIST_CONTEXT_DATA_START( NetBufferList );

    NdisAcquireSpinLock( &Receive->Lock );
    RemoveEntryList( &NblContext->NblQLink );
    
    for(ReceiveNdpIndex = 0; ReceiveNdpIndex < ARRAYSIZE(Receive->ReceiveNdpContext); ReceiveNdpIndex++)
    {
        if(!IsListEmpty(&Receive->ReceiveNdpContext[ReceiveNdpIndex].NblQueue))
        {
            NonEmptyNblQueue++;
        }
    }
    
    NdisReleaseSpinLock( &Receive->Lock );

    MbbNblCleanup( NetBufferList );

    if( NonEmptyNblQueue == 0 )
        MbbRecvQDequeueReceive( Receive->RecvQueue, Receive );
}

VOID
MbbReceiveCleanupNbls(
    __in    PMBB_NDIS_RECEIVE_CONTEXT   Receive
    )
{
    UINT ReceiveNdpIndex = 0;
    
    for(ReceiveNdpIndex = 0; ReceiveNdpIndex < ARRAYSIZE(Receive->ReceiveNdpContext); ReceiveNdpIndex++)
    {
        LIST_ENTRY                  TempList;
        PLIST_ENTRY                 ListEntry;
        PLIST_ENTRY                 NextEntry;
        PNET_BUFFER_LIST            NetBufferList;
        PMBB_NBL_RECEIVE_CONTEXT    NblContext;
        PMBB_NDIS_RECEIVE_NDP_CONTEXT ReceiveNdpContext = &(Receive->ReceiveNdpContext[ReceiveNdpIndex]);

        InitializeListHead( &TempList );

        NdisAcquireSpinLock( &Receive->Lock );
        InsertHeadList( &ReceiveNdpContext->NblQueue, &TempList );
        RemoveEntryList( &ReceiveNdpContext->NblQueue );
        InitializeListHead( &ReceiveNdpContext->NblQueue );
        NdisReleaseSpinLock( &Receive->Lock );

        for( ListEntry  = TempList.Flink;
             ListEntry != &TempList;
             ListEntry  = NextEntry )
        {
            NextEntry  = ListEntry->Flink;
            NblContext = CONTAINING_RECORD(
                            ListEntry,
                            MBB_NBL_RECEIVE_CONTEXT,
                            NblQLink
                            );
            NetBufferList = NblContext->NetBufferList;
            RemoveEntryList( ListEntry );
            MbbNblCleanup( NetBufferList );
        }
    }
}

//
// Receive Queue
//

PMBB_NDIS_RECEIVE_CONTEXT
MbbRecvQQueueReceive(
    __in    PMBB_RECEIVE_QUEUE          RecvQueue,
    __in    PMINIPORT_ADAPTER_CONTEXT   Adapter,
    __in    MBB_RECEIVE_CONTEXT         BusContext,
    __in    PMDL                        Mdl
    )
/*++
    NOTE:
        DrainAddRef( ) needs to be called before calling routine.
--*/
{
    PMBB_NDIS_RECEIVE_CONTEXT   Receive = NULL;
    UINT                        i = 0;

    MbbRecvQLock( RecvQueue, FALSE );
    do
    {
        if( (Receive = ALLOCATE_LOOKASIDE(&RecvQueue->ReceiveLookasideList)) == NULL )
        {
            TraceError( WMBCLASS_RECEIVE, "[Recv] FAILED to allocate receive context" );
            break;
        }
        NdisAllocateSpinLock( &Receive->Lock );
        
        NdisZeroMemory(Receive->ReceiveNdpContext,sizeof(Receive->ReceiveNdpContext));

        for(i=0;i < ARRAYSIZE(Receive->ReceiveNdpContext);i++)
        {
            InitializeListHead(&Receive->ReceiveNdpContext[i].NblQueue );
        }

        Receive->Mdl                    = Mdl; 
        Receive->RecvQueue              = RecvQueue;
        Receive->AdapterContext         = Adapter;
        Receive->BusHandle              = Adapter->BusHandle;
        Receive->BusContext             = BusContext;
        Receive->ReceiveLookasideList   = &RecvQueue->ReceiveLookasideList;

        InsertTailList(
            &RecvQueue->ReceivedQueue,
            &Receive->ReceivedQLink
            );
    }
    while( FALSE );
    MbbRecvQUnlock( RecvQueue, FALSE );

    return Receive;
}

VOID
MbbRecvQDequeueReceive(
    __in    PMBB_RECEIVE_QUEUE          RecvQueue,
    __in    PMBB_NDIS_RECEIVE_CONTEXT   Receive
    )
{
    MbbBusReturnReceiveBuffer(
        Receive->BusHandle,
        Receive->BusContext,
        Receive->Mdl
        );

    MbbRecvQLock( RecvQueue, FALSE );
    RemoveEntryList( &Receive->ReceivedQLink );
    if( IsListEmpty( &RecvQueue->ReceivedQueue ) )
        KeSetEvent( &RecvQueue->QueueEmptyEvent, 0, FALSE );
    MbbRecvQUnlock( RecvQueue, FALSE );

    Receive->RecvQueue = MBB_BAD_POINTER;
    NdisFreeSpinLock( &Receive->Lock );
    FREE_LOOKASIDE( Receive, Receive->ReceiveLookasideList );

    DrainRelease( &(RecvQueue->QueueDrainObject) );
}

__drv_maxIRQL(DISPATCH_LEVEL)
__drv_savesIRQLGlobal( SpinLock, RecvQueue->Lock )
__drv_when( DispatchLevel == FALSE, __drv_raisesIRQL(DISPATCH_LEVEL))
FORCEINLINE
VOID
MbbRecvQLock(
    __in __drv_at( RecvQueue->Lock, __drv_acquiresResource( Spinlock ) )
            PMBB_RECEIVE_QUEUE  RecvQueue,
    __in    BOOLEAN             DispatchLevel
    )
{
    if( DispatchLevel == TRUE )
    {
        NdisDprAcquireSpinLock( &RecvQueue->Lock );
    }
    else
    {
        NdisAcquireSpinLock( &RecvQueue->Lock );
    }
}

__drv_maxIRQL(DISPATCH_LEVEL)
__drv_restoresIRQLGlobal( SpinLock, RecvQueue->Lock )
FORCEINLINE
VOID
MbbRecvQUnlock(
    __in __drv_at( RecvQueue->Lock, __drv_releasesResource( Spinlock ) )
            PMBB_RECEIVE_QUEUE  RecvQueue,
    __in    BOOLEAN             DispatchLevel
    )
{
    if( DispatchLevel == TRUE )
    {
        NdisDprReleaseSpinLock( &RecvQueue->Lock );
    }
    else
    {
        NdisReleaseSpinLock( &RecvQueue->Lock );
    }
}

NDIS_STATUS
MbbRecvQInitialize(
    __in    PMBB_RECEIVE_QUEUE  RecvQueue,
    __in    MBB_DRAIN_COMPLETE  DrainCompleteCallback,
    __in    PVOID               DrainCompleteCallbackContext,
    __in    NDIS_HANDLE         MiniportHandle
    )
{
    NDIS_STATUS                     NdisStatus = NDIS_STATUS_SUCCESS;
    USHORT                          NblContextSize = ALIGN( sizeof(MBB_NBL_RECEIVE_CONTEXT), MEMORY_ALLOCATION_ALIGNMENT );
    NET_BUFFER_POOL_PARAMETERS      NbPoolParameters;
    NET_BUFFER_LIST_POOL_PARAMETERS NblPoolParameters;

    do
    {
        InitializeListHead( &RecvQueue->ReceivedQueue );
        KeInitializeEvent(
            &RecvQueue->QueueEmptyEvent,
            NotificationEvent,
            TRUE
            );
        NdisAllocateSpinLock( &RecvQueue->Lock );

        RecvQueue->NblPool          = NULL;
        RecvQueue->LookasideList    = FALSE;

        NdisInitializeNPagedLookasideList(
            &RecvQueue->ReceiveLookasideList,
            NULL,
            NULL,
            0,
            sizeof(MBB_NDIS_RECEIVE_CONTEXT),
            MbbPoolTagMdlReceive,
            0
            );
        RecvQueue->LookasideList    = TRUE;

        NblPoolParameters.Header.Type           = NDIS_OBJECT_TYPE_DEFAULT;
        NblPoolParameters.Header.Revision       = NET_BUFFER_LIST_POOL_PARAMETERS_REVISION_1;
        NblPoolParameters.Header.Size           = NDIS_SIZEOF_NET_BUFFER_LIST_POOL_PARAMETERS_REVISION_1;
        NblPoolParameters.ProtocolId            = NDIS_PROTOCOL_ID_DEFAULT;
        NblPoolParameters.fAllocateNetBuffer    = TRUE;
        NblPoolParameters.ContextSize           = NblContextSize;
        NblPoolParameters.PoolTag               = MbbPoolTagNblPool;
        NblPoolParameters.DataSize              = 0;

        if( (RecvQueue->NblPool = NdisAllocateNetBufferListPool(
                                        MiniportHandle,
                                        &NblPoolParameters
                                        )) == NULL )
        {
            TraceError( WMBCLASS_RECEIVE, "[Recv] FAILED to allocate NetBufferListPool" );
            NdisStatus = NDIS_STATUS_RESOURCES;
            break;
        }

        InitDrainObject(
            &RecvQueue->QueueDrainObject,
            DrainCompleteCallback,
            DrainCompleteCallbackContext
            );
        TraceInfo( WMBCLASS_RECEIVE, "[Recv] Initialization complete" );
    }
    while( FALSE );

    if( NdisStatus != NDIS_STATUS_SUCCESS )
    {
        MbbRecvQCleanup( RecvQueue );
    }
    return NdisStatus;
}

VOID
MbbRecvQCleanup(
    __in    PMBB_RECEIVE_QUEUE      RecvQueue
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
    ASSERT( IsListEmpty( &RecvQueue->ReceivedQueue ) == TRUE );

    if( RecvQueue->NblPool != NULL )
    {
        NdisFreeNetBufferListPool( RecvQueue->NblPool );
        RecvQueue->NblPool = NULL;
    }
    if( RecvQueue->LookasideList == TRUE )
    {
        NdisDeleteNPagedLookasideList( &RecvQueue->ReceiveLookasideList );
        RecvQueue->LookasideList = FALSE;
    }
    NdisFreeSpinLock( &RecvQueue->Lock );

    TraceInfo( WMBCLASS_RECEIVE, "[Recv] Cleanup complete" );
}

VOID
MbbRecvQCancel(
    __in  PMBB_RECEIVE_QUEUE        RecvQueue,
    __in  NDIS_STATUS               Status,
    __in  BOOLEAN                   WaitForCompletion
    )
{
    StartDrain( &RecvQueue->QueueDrainObject );
    //
    // Nothing to cancel
    //
    if( WaitForCompletion )
    {
        TraceInfo( WMBCLASS_RECEIVE, "[Recv] Cancel, waiting for request to complete..." );

        KeWaitForSingleObject(
            &RecvQueue->QueueEmptyEvent,
            Executive,
            KernelMode,
            FALSE,
            NULL
            );
    }
    TraceInfo( WMBCLASS_RECEIVE, "[Recv] Cancel complete" );
}



NDIS_STATUS
MbbReceiveDssData(
    __in    PVOID   Nth,
    __in    ULONG   NdpSignature,
    __in    PVOID   Datagram,
    __in    ULONG   DatagramLength,
    __in    ULONG   SessionIdentifier,
    __in    PVOID   Context
    )
{
    PMBB_NDIS_RECEIVE_CONTEXT   Receive = (PMBB_NDIS_RECEIVE_CONTEXT)Context;
    ULONG                       SessionId;

    UNREFERENCED_PARAMETER(SessionIdentifier);
    
    // Pass this to the Dss indication processing code
    SessionId = NdpSignature >> NCM_NDP_SESSION_SHIFT;

    // Forward to the dss receive handler
    MbbNdisDeviceServiceSessionReceive(
        Receive->AdapterContext,
        SessionId,
        Datagram,
        DatagramLength
        );

    return NDIS_STATUS_SUCCESS;
}

//
// NDIS / Bus Callbacks
//

VOID
MbbNdisReceiveCallback(
    __in    MBB_PROTOCOL_HANDLE     ProtocolHandle,
    __in    MBB_RECEIVE_CONTEXT     ReceiveContext,
    __in    PMDL                    Mdl
    )
{
    PUCHAR                          DataBuffer;
    ULONG                           DataLength;
    PVOID                           Nth;
    BOOLEAN                         Referenced = FALSE;
    BOOLEAN                         ReturnBuffer = TRUE;
    PMINIPORT_ADAPTER_CONTEXT       Adapter = (PMINIPORT_ADAPTER_CONTEXT)ProtocolHandle;
    PMBB_NDIS_RECEIVE_CONTEXT       Receive = NULL;
    PMBB_NDIS_RECEIVE_NDP_CONTEXT   ReceiveNdpContext = NULL;
    int                             ReceiveNdpIndex = 0;
    int                             MaxReceiveNdpIndex = -1;

    do
    {
        if( ! (Referenced = DrainAddRef( &(Adapter->ReceiveQueue.QueueDrainObject) )) )
        {
            break;
        }

        if( (DataBuffer = MmGetSystemAddressForMdlSafe( Mdl, NormalPagePriority | MdlMappingNoExecute )) == NULL )
        {
            TraceError( WMBCLASS_RECEIVE, "[Recv] FAILED to get system address for Mdl=0x%p", Mdl );
            break;
        }
        DataLength = MmGetMdlByteCount( Mdl );

        Nth = DataBuffer;

        if(  MbbNtbValidate(
                Nth,
                DataLength,
                Adapter->BusParams.CurrentMode32Bit
                ) != NDIS_STATUS_SUCCESS )
        {
            TraceError( WMBCLASS_RECEIVE, "[Recv] FAILED to validate NTB" );


            MbbWriteEvent(
                &RECEIVED_BAD_NTB,
                NULL,
                NULL,
                2
                &Adapter->TraceInstance,
                sizeof(Adapter->TraceInstance),
                &DataLength,
                sizeof(DataLength)
                );



            {
                ULONG   i;
                ULONG   j;

                PUCHAR  Buffer=(PUCHAR)Nth;

                for (i=0; i<DataLength; i+=16)
                {
                    UCHAR   NumString[16*3+1];
                    UCHAR   AsciiBuffer[40];
                    UCHAR   Value=0;
                    const char TranslateTable[16]={ '0','1','2','3','4','5','6','7',
                                                    '8','9','a','b','c','d','e','f'};

                    RtlZeroMemory(NumString, sizeof(NumString));
                    RtlZeroMemory(AsciiBuffer,sizeof(AsciiBuffer));

                    for (j=0; j < 16; j++ )
                    {
                        if (i+j >= DataLength)
                        {
                            //
                            //  past the end, just put spaces
                            //
                            NumString[j*3]  =' ';
                            NumString[j*3+1]=' ';
                            NumString[j*3+2]=' ';

                        } else
                        {
                            Value=Buffer[i+j];

                            NumString[j*3]= TranslateTable[ Value >> 4];
                            NumString[j*3+1]= TranslateTable[ Value & 0xf];
                            NumString[j*3+2]=' ';

                            if ((Buffer[i+j] >= 32) && (Buffer[i+j] < 128))
                            {
                                AsciiBuffer[j]=Value;
                            }
                            else
                            {
                                AsciiBuffer[j]='.';
                            }
                        }
                    }

                    TraceError( WMBCLASS_RECEIVE, "%08lx - %s  %s", i, NumString, AsciiBuffer);
                }

            }




            break;
        }

        if( (Receive = MbbRecvQQueueReceive(
                        &Adapter->ReceiveQueue,
                        Adapter,
                        ReceiveContext,
                        Mdl
                        )) == NULL )
        {
            TraceError( WMBCLASS_RECEIVE, "[Recv] FAILED to allocate receive context" );
            break;
        }
        Receive->NtbSequence = MBB_NTB_GET_SEQUENCE( Nth );

        if( MbbNtbParse(
                Nth,
                DataLength,
                MbbReceiveParseNdp,
                Receive
                ) != NDIS_STATUS_SUCCESS )
        {
            TraceError( WMBCLASS_RECEIVE, "[Recv] FAILED to parse NTB" );
            break;
        }

        // Preprocessing here to ensure that Receive Context is not cleaned up when 
        // MbbNdisMiniportReturnNetBufferLists is called for the last NDP NBL chain processed
        
        for(ReceiveNdpIndex = 0; ReceiveNdpIndex < ARRAYSIZE(Receive->ReceiveNdpContext); ReceiveNdpIndex++)
        {     
            ReceiveNdpContext = &(Receive->ReceiveNdpContext[ReceiveNdpIndex]);

            if(!IsListEmpty( &ReceiveNdpContext->NblQueue ) )
            {
                MaxReceiveNdpIndex = ReceiveNdpIndex;
            }
        }
        
        for(ReceiveNdpIndex = 0; ReceiveNdpIndex <= MaxReceiveNdpIndex && ReceiveNdpIndex < ARRAYSIZE(Receive->ReceiveNdpContext); ReceiveNdpIndex++)
        {     
            ReceiveNdpContext = &(Receive->ReceiveNdpContext[ReceiveNdpIndex]);
        
            // If all the Ndps in this receive are Dss stuff, we 
            // wont have any NBLs to indicate
            if( !IsListEmpty( &ReceiveNdpContext->NblQueue ) )
            {
                ASSERT(ReceiveNdpContext->SessionIdPortNumberEntry.InUse);
                
                ReturnBuffer = FALSE;

                {
                    PNET_BUFFER_LIST    CurrentNetBufferList;
                    PNET_BUFFER_LIST    NextNetBufferList;
                    PNET_BUFFER         NetBuffer;

                    for( CurrentNetBufferList  = ReceiveNdpContext->FirstNbl;
                         CurrentNetBufferList != NULL;
                         CurrentNetBufferList  = NextNetBufferList )
                    {
                        NextNetBufferList = NET_BUFFER_LIST_NEXT_NBL( CurrentNetBufferList );

                        for( NetBuffer  = NET_BUFFER_LIST_FIRST_NB( CurrentNetBufferList );
                             NetBuffer != NULL;
                             NetBuffer  = NET_BUFFER_NEXT_NB( NetBuffer ) )
                        {
                            InterlockedAdd64(&Adapter->Stats.ifHCInOctets, NET_BUFFER_DATA_LENGTH(NetBuffer));
                            InterlockedIncrement64(&Adapter->GenRcvFramesOk);
                            InterlockedIncrement64(&Adapter->Stats.ifHCInUcastPkts);
                        }
                    }
                }

#if DBG
                MbbTestValidateNblChain(ReceiveNdpContext->FirstNbl );
#endif
                NdisMIndicateReceiveNetBufferLists(
                    Adapter->MiniportAdapterHandle,
                    ReceiveNdpContext->FirstNbl,
                    ReceiveNdpContext->SessionIdPortNumberEntry.PortNumber,
                    ReceiveNdpContext->NblCount,
                    0
                    );
            }
        }
    }
    while( FALSE );

    if( ReturnBuffer == TRUE )
    {
        TraceError( WMBCLASS_RECEIVE, "[Recv] DROPPING received Mdl=0x%p", Mdl );
        InterlockedIncrement64( &Adapter->Stats.ifInDiscards );

        if( Receive != NULL )
        {
            MbbReceiveCleanupNbls( Receive );

            MbbRecvQDequeueReceive(
                &Adapter->ReceiveQueue,
                Receive
                );
            //
            // Reference dropped by Dequeue
            //
            Referenced = FALSE;
        }
        else
        {
            MbbBusReturnReceiveBuffer(
                Adapter->BusHandle,
                ReceiveContext,
                Mdl
                );
        }
        if( Referenced )
            DrainRelease( &(Adapter->ReceiveQueue.QueueDrainObject) );
    }
}

VOID
MbbNdisMiniportReturnNetBufferLists(
    __in    NDIS_HANDLE         MiniportAdapterContext,
    __in    PNET_BUFFER_LIST    NetBufferLists,
    __in    ULONG               ReturnFlags
    )
{
    PNET_BUFFER_LIST            NetBufferList;
    PNET_BUFFER_LIST            NextNetBufferList;
    PMBB_NBL_RECEIVE_CONTEXT    NblContext;
    PMINIPORT_ADAPTER_CONTEXT   Adapter = (PMINIPORT_ADAPTER_CONTEXT)MiniportAdapterContext;

    if (MbbBusIsFastIO(Adapter->BusHandle))
    {
        MbbBusReturnNetBufferLists(Adapter->BusHandle, NetBufferLists, ReturnFlags);
    }
    else
    {
        for (NetBufferList = NetBufferLists;
            NetBufferList != NULL;
            NetBufferList = NextNetBufferList)
        {
            NextNetBufferList = NET_BUFFER_LIST_NEXT_NBL(NetBufferList);
            NET_BUFFER_LIST_NEXT_NBL(NetBufferList) = NULL;
            NblContext = (PMBB_NBL_RECEIVE_CONTEXT)NET_BUFFER_LIST_CONTEXT_DATA_START(NetBufferList);
            MbbReceiveRemoveNbl(NblContext->Receive, NetBufferList);
        }
    }
}

#if DBG
VOID
MbbTestValidateNblChain(
    __in    PNET_BUFFER_LIST    NblChain
    )
{
    PNET_BUFFER         NetBuffer;
    PNET_BUFFER_LIST    NetBufferList;

    for( NetBufferList  = NblChain;
         NetBufferList != NULL;
         NetBufferList  = NET_BUFFER_LIST_NEXT_NBL( NetBufferList ) )
    {
        NetBuffer = NET_BUFFER_LIST_FIRST_NB( NetBufferList );
        ASSERT( NET_BUFFER_CURRENT_MDL( NetBuffer ) != NULL );
    }
}
#endif
