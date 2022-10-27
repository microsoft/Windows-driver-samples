/*++

Copyright (c) Microsoft Corporation. All rights reserved.

Module Name:
    port.h

Abstract:
    Contains port declarations.
    
Revision History:
      When        What
    ----------    ----------------------------------------------
    11-21-2012    Created

Notes:

--*/
#pragma once

#define DEFAULT_NDIS_PORT_NUMBER  0

// port lock manipulation macros
#define MBB_ACQUIRE_PORT_LOCK(_Port)         \
    NdisAcquireSpinLock(&(_Port->Lock))

#define MBB_RELEASE_PORT_LOCK(_Port)         \
    NdisReleaseSpinLock(&(_Port->Lock))

#define MBB_PORT_SET_SESSION_ID(_Port,_SessionId)         \
    (_Port)->SessionId = (_SessionId)

#define MBB_PORT_RESET_SESSION_ID(_Port)         \
    (_Port)->SessionId = MBB_INVALID_SESSION_ID


typedef struct _REF_COUNT *PREF_COUNT;

typedef VOID (*REFERENCE_OBJECT_CLEANUP_FN)(PREF_COUNT RefObj);

    
typedef struct _REF_COUNT
{
    ULONG                       RefCount;
    REFERENCE_OBJECT_CLEANUP_FN Cleanup;
} REF_COUNT, *PREF_COUNT;


LONG 
__inline ReferenceRefCount(PREF_COUNT RefCountObj)
{
    LONG newRef = InterlockedIncrement((volatile LONG *)&RefCountObj->RefCount);
    ASSERT(newRef > 0);

    return newRef;
}

LONG
__inline DereferenceRefCount(PREF_COUNT RefCountObj)
{
    LONG newRef = InterlockedDecrement((volatile LONG *)&RefCountObj->RefCount);

    ASSERT(newRef >= 0);
    
    if( newRef == 0 )
    {
        RefCountObj->Cleanup(RefCountObj);
    }

    return newRef;
}

#define Reference(x)    ReferenceRefCount(&(x)->RefCount)

#define Dereference(x)  DereferenceRefCount(&(x)->RefCount)

VOID
__inline InitializeRefCount(
                IN PREF_COUNT RefCountObj,
                IN REFERENCE_OBJECT_CLEANUP_FN CleanupFn
)
{
    RefCountObj->Cleanup = CleanupFn;
    RefCountObj->RefCount= 1;
}


// Just a busy wait for PortList refcount to go to 0
//#define MBB_WAIT_FOR_PORTLIST_REFCOUNT(_Adapter) \
  //  MP_VERIFY_PASSIVE_IRQL();                   \
   // while (_Adapter->PortRefCount != 0) NdisMSleep(10);
// Macro to verify if this port number is allocated/"registered" with NDIS

#define IS_ALLOCATED_PORT_NUMBER(_PortNumber)   (_PortNumber != DEFAULT_NDIS_PORT_NUMBER)

typedef enum _MBB_PORT_STATE
{
    PortDeactivated = 0,
    PortActivating,
    PortActivated,
    PortDeactivating    
} MBB_PORT_STATE;


typedef struct _MBB_PORT
{
   /**
     * The handle by which NDIS recognizes this adapter. This handle needs to be passed
     * in for many of the calls made to NDIS
     */
    NDIS_HANDLE                 MiniportAdapterHandle;

    PMINIPORT_ADAPTER_CONTEXT   Adapter; // Miniport adapter context

    NDIS_PORT_NUMBER            PortNumber; // NDIS allocate port number

    /** State of the PORT.*/
    MBB_PORT_STATE              State;

    UCHAR                       MACAddress[MBB_MAC_ADDRESS_LENGTH];

    /** Refcount tracking operations that would block Pause/Restart, etc operations */
    REF_COUNT                   RefCount;// Used by Port/MP 

    ULONG                       ConnectionId;

    LONG                        SessionId;
    
    MBB_CONNECTION_STATE        ConnectionState;
    
    PMBB_IP_ADDRESS_INFO        IpAddressInfo;

    BOOLEAN                     HasNoMatchWolPattern;

    /** Lock used to protect the PORT data including PortNumber, State and ConnectionCtx*/
    NDIS_SPIN_LOCK              Lock;
} MBB_PORT, *PMBB_PORT;

NDIS_STATUS
MbbWwanAllocateBasePort(
    _In_  PMINIPORT_ADAPTER_CONTEXT             Adapter,
    _Outptr_result_nullonfailure_ PMBB_PORT*    Port
    );

VOID
MbbWwanFreeBasePort(
    _In_ __drv_freesMem(Mem) PMBB_PORT          Port
    );

NDIS_STATUS
MbbWwanAllocatePort(
    _In_  PMINIPORT_ADAPTER_CONTEXT             Adapter,
    _In_  BOOLEAN                               fAllocateNdisPort,
    _Outptr_result_nullonfailure_  PMBB_PORT*   Port
    );

    
VOID
MbbWwanDeallocatePort( 
    _In_ PMBB_PORT  Port
    );
    
VOID
MbbWwanFreePort(
    _In_  PREF_COUNT RefObj
    );

NDIS_STATUS
MbbWwanActivatePort(
    _In_  PMINIPORT_ADAPTER_CONTEXT  Adapter,
    _In_  PMBB_PORT                  Port
    );

NDIS_STATUS
MbbWwanDeactivatePort(
    _In_  PMINIPORT_ADAPTER_CONTEXT  Adapter,
    _In_  PMBB_PORT                  Port
    );

_Requires_lock_not_held_(&Adapter->PortsLock)
VOID
MbbWwanAddPortToAdapterList(
    _In_ PMINIPORT_ADAPTER_CONTEXT Adapter,
    _In_ PMBB_PORT*                PortToAdd
    );

_Requires_lock_not_held_(&Adapter->PortsLock)
BOOL
MbbWwanRemovePortFromAdapterList(
    _In_ PMINIPORT_ADAPTER_CONTEXT Adapter,
    _In_ PMBB_PORT                 PortToRemove
    );

_Requires_lock_not_held_(&Adapter->PortsLock)
PMBB_PORT
MbbWwanTranslatePortNumberToPort(
    _In_  PMINIPORT_ADAPTER_CONTEXT  Adapter,
    _In_  NDIS_PORT_NUMBER        PortNumber
    );

_Requires_lock_not_held_(&Adapter->PortsLock)
PMBB_PORT
MbbWwanTranslateSessionIdToPort(
    _In_  PMINIPORT_ADAPTER_CONTEXT  Adapter,
    _In_  ULONG                      SessionId
    );

_Requires_lock_not_held_(&Port->Lock)
VOID
MbbWwanSetPortConnectionId(
    __in  PMBB_PORT         Port,
    __in  ULONG             ConnectionId
    );

_Requires_lock_not_held_(&Port->Lock)
ULONG
MbbWwanGetPortConnectionId(
    __in  PMBB_PORT         Port
    );

_Requires_lock_not_held_(&Port->Lock)
NDIS_STATUS
MbbWwanSetPortSessionId(
    __in  PMBB_PORT         Port
    );

_Requires_lock_not_held_(&Port->Lock)
LONG
MbbWwanGetPortSessionId(
    __in  PMBB_PORT         Port
    );

_Requires_lock_not_held_(&Port->Lock)
VOID
MbbWwanResetPortSessionId(
    __in  PMBB_PORT         Port,
    __in  BOOLEAN           ReturnSessionId
    );

_Requires_lock_not_held_(&Adapter->PortsLock)
_Requires_lock_not_held_(&Adapter->SessionIdPortTableLock)
NDIS_STATUS
MbbWwanReturnSessionId(
    __in PMINIPORT_ADAPTER_CONTEXT Adapter,
    __in ULONG                     SessionId
    );

_Requires_lock_not_held_(&Adapter->PortsLock)
DWORD
MbbWwanGetNumPortsConnected(
     __in PMINIPORT_ADAPTER_CONTEXT  Adapter
     );
