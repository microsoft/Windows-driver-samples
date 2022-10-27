/*++

Copyright (c) Microsoft Corporation. All rights reserved.

Module Name:
    port.c

Abstract:
    Implements the functionality needed by ports
    
Revision History:
      When        What
    ----------    ----------------------------------------------
    12-13-2012    Created

Notes:

--*/

#include "precomp.h"
#include "port.tmh"

NDIS_STATUS
MbbWwanAllocateBasePort(
    _In_  PMINIPORT_ADAPTER_CONTEXT              Adapter,
    _Outptr_result_nullonfailure_ PMBB_PORT*     Port
    )
{
    NDIS_STATUS     Status = NDIS_STATUS_SUCCESS;
    PMBB_PORT       newPort = NULL;

    *Port = NULL;

    do
    {
         // Allocate a PORT data structure
        newPort = ALLOCATE_NONPAGED_POOL_WITH_TAG(sizeof(MBB_PORT), MbbPoolTagPort);
        
        if (newPort == NULL)
        {
            TraceError(WMBCLASS_OID, "Failed to allocate %d bytes for port\n",sizeof(MBB_PORT));
            Status = NDIS_STATUS_RESOURCES;
            break;
        }

        // Clear everything
        NdisZeroMemory(newPort, sizeof(MBB_PORT));

        // Allocate memory for fields inside the PORT structure
        NdisAllocateSpinLock(&(newPort->Lock));

        // Reference the adapter since the port is going to be using it
        // this reference will be removed in the port cleanup routine
        MbbAdapterRef(Adapter);
        
        // Save the passed data into the PORT
        newPort->MiniportAdapterHandle = Adapter->MiniportAdapterHandle;
        newPort->Adapter = Adapter;        
        newPort->State = PortDeactivated;
        newPort->PortNumber = DEFAULT_NDIS_PORT_NUMBER;
        newPort->SessionId = MBB_INVALID_SESSION_ID; //no active session
        
         // Return the newly created structure to the caller
        *Port = newPort;   
    }while(FALSE);

   return Status;
}

VOID
MbbWwanFreeBasePort(
    _In_ __drv_freesMem(Mem) PMBB_PORT                Port
    )
{
    if(Port != NULL)
    {
        NdisFreeSpinLock(&(Port->Lock));
        
        if( Port->IpAddressInfo != NULL )
        {
            FREE_POOL(Port->IpAddressInfo);
        }

        // Dereference the adapter
        MbbAdapterDeref(Port->Adapter);

        FREE_POOL(Port);
        Port = NULL;
    }
}

NDIS_STATUS
MbbWwanAllocatePort(
    _In_  PMINIPORT_ADAPTER_CONTEXT               Adapter,
    _In_  BOOLEAN                                 fAllocateNdisPort,
    _Outptr_result_nullonfailure_  PMBB_PORT*     Port
    )
{

    NDIS_STATUS                 Status = NDIS_STATUS_SUCCESS;
    PMBB_PORT                   newPort = NULL;
    NDIS_PORT_CHARACTERISTICS   portChar;
    
    *Port = NULL;

    do
    {
        Status = MbbWwanAllocateBasePort(Adapter, &newPort);
        
        if (Status != NDIS_STATUS_SUCCESS)
        {
            TraceError( WMBCLASS_OID, "Failed to allocate base port structure\n");
            break;
        }

        // Initialize the intial reference count to 1
        InitializeRefCount(&newPort->RefCount, MbbWwanFreePort);

        if(fAllocateNdisPort)
        {    
            // Call NDIS to allocate the port
            NdisZeroMemory(&portChar, sizeof(NDIS_PORT_CHARACTERISTICS));


            portChar.Header.Type = NDIS_OBJECT_TYPE_DEFAULT;
            portChar.Header.Revision = NDIS_PORT_CHARACTERISTICS_REVISION_1;
            portChar.Header.Size = sizeof(NDIS_PORT_CHARACTERISTICS);         

            portChar.Flags = NDIS_PORT_CHAR_USE_DEFAULT_AUTH_SETTINGS;
            portChar.Type = NdisPortTypeUndefined;
            portChar.MediaConnectState = MediaConnectStateUnknown;
            portChar.XmitLinkSpeed = NDIS_LINK_SPEED_UNKNOWN;
            portChar.RcvLinkSpeed = NDIS_LINK_SPEED_UNKNOWN;
            portChar.Direction = NET_IF_DIRECTION_SENDRECEIVE;
            portChar.SendControlState = NdisPortControlStateUnknown;
            portChar.RcvControlState = NdisPortControlStateUnknown;
            portChar.SendAuthorizationState = NdisPortControlStateUncontrolled; // Ignored
            portChar.RcvAuthorizationState = NdisPortControlStateUncontrolled; // Ignored
                       
            Status = NdisMAllocatePort(Adapter->MiniportAdapterHandle, &portChar);
            
            if (Status != NDIS_STATUS_SUCCESS)
            {
                TraceError( WMBCLASS_OID, "Failed to allocate NDIS port. Status = 0x%08x\n", Status);
                break;
            }
            
            TraceInfo(WMBCLASS_OID, "Associated Port Number %lu with allocated port\n", portChar.PortNumber);
                            
            // Assign the NDIS port number that has been allocated to this port
            newPort->PortNumber = portChar.PortNumber;
        }
        else
        {
            // means it the default ndis port
            newPort->SessionId = 0;
        }
        
        *Port = newPort; 
    }while(FALSE);

    if(Status != NDIS_STATUS_SUCCESS)
    {
        if(newPort != NULL)
        {
            MbbWwanDeallocatePort(newPort);
        }
    }
    
    return Status;
}

VOID
MbbWwanDeallocatePort(
    _In_ PMBB_PORT  Port
    )
{
    // remove the intial reference count
    Dereference(Port);
}


VOID
MbbWwanFreePort(
    PREF_COUNT RefObj
    )
{
    NDIS_STATUS Status = NDIS_STATUS_SUCCESS;
    PMBB_PORT Port = NULL;

    Port = CONTAINING_RECORD(RefObj, MBB_PORT, RefCount);

    ASSERT(Port!=NULL);

    if(IS_ALLOCATED_PORT_NUMBER(Port->PortNumber))
    {
        // Free the NDIS port. We do not manage default NDIS port ( port number 0 )
        Status = NdisMFreePort(Port->Adapter->MiniportAdapterHandle, Port->PortNumber);
        ASSERT(Status == NDIS_STATUS_SUCCESS);
    }

    // Free the base port structure
    MbbWwanFreeBasePort(Port);
}

// Port will not be deleted while in this function because the caller should take a ref before calling this function.
NDIS_STATUS
MbbWwanActivatePort(
    _In_  PMINIPORT_ADAPTER_CONTEXT  Adapter,
    _In_  PMBB_PORT                  Port
    )
{
    NDIS_STATUS                 Status = NDIS_STATUS_SUCCESS;
    NET_PNP_EVENT_NOTIFICATION  NetPnpEventNotification;
    NDIS_PORT                   NdisPort;
    PNDIS_PORT_CHARACTERISTICS  PortChar;
    NDIS_PORT_NUMBER            PortNumberToActivate = DEFAULT_NDIS_PORT_NUMBER;
    BOOLEAN                     PortActive = FALSE;

    do
    {
        if(!Port)
        {
            Status = NDIS_STATUS_FAILURE;
            break;
        }

        MBB_ACQUIRE_PORT_LOCK(Port);

        if(!IS_ALLOCATED_PORT_NUMBER(Port->PortNumber))
        {
            // we do not handle DEFAULT_NDIS_PORT_NUMBER           
            Status = NDIS_STATUS_SUCCESS;
            MBB_RELEASE_PORT_LOCK(Port);
            break;
        }
              
        if(Port->State == PortActivated)
        {
            TraceInfo(WMBCLASS_OID, "Requested port with ndis port number %lu is already activated",Port->PortNumber);
            Status = NDIS_STATUS_SUCCESS;
            MBB_RELEASE_PORT_LOCK(Port);
            break;
        }

        if(Port->State != PortDeactivated)
        {
            Status = NDIS_STATUS_INVALID_PORT_STATE;
            MBB_RELEASE_PORT_LOCK(Port);
            break;
        }

        // set port state to activating
        Port->State = PortActivating;
        
        PortNumberToActivate = Port->PortNumber;

        MBB_RELEASE_PORT_LOCK(Port);

        NdisZeroMemory(&NetPnpEventNotification, sizeof(NET_PNP_EVENT_NOTIFICATION));
        NdisZeroMemory(&NdisPort, sizeof(NDIS_PORT));

        NetPnpEventNotification.Header.Type = NDIS_OBJECT_TYPE_DEFAULT;
        NetPnpEventNotification.Header.Revision = NET_PNP_EVENT_NOTIFICATION_REVISION_1;
        NetPnpEventNotification.Header.Size = sizeof(NET_PNP_EVENT_NOTIFICATION);

        NetPnpEventNotification.NetPnPEvent.NetEvent = NetEventPortActivation;

        
        // Refill the characteristics structure for the port
        PortChar = &(NdisPort.PortCharacteristics);
        
        PortChar->Header.Type = NDIS_OBJECT_TYPE_DEFAULT;
        PortChar->Header.Revision = NDIS_PORT_CHARACTERISTICS_REVISION_1;
        PortChar->Header.Size = sizeof(NDIS_PORT_CHARACTERISTICS);
        
        PortChar->Flags = NDIS_PORT_CHAR_USE_DEFAULT_AUTH_SETTINGS;
        PortChar->Type = NdisPortTypeUndefined;
        PortChar->MediaConnectState = MediaConnectStateUnknown;
        PortChar->XmitLinkSpeed = NDIS_LINK_SPEED_UNKNOWN;
        PortChar->RcvLinkSpeed = NDIS_LINK_SPEED_UNKNOWN;
        PortChar->Direction = NET_IF_DIRECTION_SENDRECEIVE;
        PortChar->SendControlState = NdisPortControlStateUnknown;
        PortChar->RcvControlState = NdisPortControlStateUnknown;
        PortChar->SendAuthorizationState = NdisPortControlStateUncontrolled; // Ignored
        PortChar->RcvAuthorizationState = NdisPortControlStateUncontrolled; // Ignored
        PortChar->PortNumber = PortNumberToActivate;       
            
        
        // Single port is being activated
        NdisPort.Next = NULL;
        
        // We need to save a pointer to the NDIS_PORT in the NetPnPEvent::Buffer field
        NetPnpEventNotification.NetPnPEvent.Buffer = (PVOID)&NdisPort;
        NetPnpEventNotification.NetPnPEvent.BufferLength = sizeof(NDIS_PORT);
        
        Status = NdisMNetPnPEvent(Port->MiniportAdapterHandle, &NetPnpEventNotification);
        
        if (Status != NDIS_STATUS_SUCCESS)
        {
            TraceError( WMBCLASS_OID, "Failed to activate NDIS port %d. Status = 0x%08x\n", PortNumberToActivate, Status);
        }
        else
        {
            TraceInfo( WMBCLASS_OID, "Activated Port Number %d\n", PortNumberToActivate);
            PortActive = TRUE;
        }


        MBB_ACQUIRE_PORT_LOCK(Port);
        
        if(PortActive)
        {        
            // set the port state to activated or reset it to initialized in case of failure
            Port->State = PortActivated;
            Reference(Port); // for activation
        }
        else
        {
            Port->State = PortDeactivated;
        }

        MBB_RELEASE_PORT_LOCK(Port);
        
    }while(FALSE);   
    
    return Status;
}

NDIS_STATUS
MbbWwanDeactivatePort(
    _In_  PMINIPORT_ADAPTER_CONTEXT  Adapter,
    _In_  PMBB_PORT                  Port
    )
{
    NDIS_STATUS                 Status = NDIS_STATUS_SUCCESS;
    NET_PNP_EVENT_NOTIFICATION  NetPnpEventNotification;
    NDIS_PORT_NUMBER            PortNumberArray[1];
    NDIS_PORT_NUMBER            PortNumberToDeactivate = DEFAULT_NDIS_PORT_NUMBER;

    do
    {
        if(!Port)
        {
            Status = NDIS_STATUS_FAILURE;
            break;
        }

        MBB_ACQUIRE_PORT_LOCK(Port);

        if(!IS_ALLOCATED_PORT_NUMBER(Port->PortNumber))
        {
            // we do not handle DEFAULT_NDIS_PORT_NUMBER           
            Status = NDIS_STATUS_SUCCESS;
            MBB_RELEASE_PORT_LOCK(Port);
            break;
        }

        if(Port->State == PortDeactivated)
        {
            TraceInfo(WMBCLASS_OID, "Requested port with ndis port number %lu is already deactivated",Port->PortNumber);
            Status = NDIS_STATUS_SUCCESS;
            MBB_RELEASE_PORT_LOCK(Port);
            break;
        }

        if(Port->State != PortActivated)
        {
            Status = NDIS_STATUS_INVALID_PORT_STATE;
            MBB_RELEASE_PORT_LOCK(Port);
            break;
        }

        // set port state to deactivating
        Port->State = PortDeactivating;

        PortNumberToDeactivate = Port->PortNumber;

        MBB_RELEASE_PORT_LOCK(Port);


        NdisZeroMemory(&NetPnpEventNotification, sizeof(NET_PNP_EVENT_NOTIFICATION));

        NetPnpEventNotification.Header.Type = NDIS_OBJECT_TYPE_DEFAULT;
        NetPnpEventNotification.Header.Revision = NET_PNP_EVENT_NOTIFICATION_REVISION_1;
        NetPnpEventNotification.Header.Size = sizeof(NET_PNP_EVENT_NOTIFICATION);


        NetPnpEventNotification.NetPnPEvent.NetEvent = NetEventPortDeactivation;

        // We need to save a pointer to the NDIS_PORT_NUMBER in the NetPnPEvent::Buffer field
        PortNumberArray[0] = PortNumberToDeactivate;            
        NetPnpEventNotification.NetPnPEvent.Buffer = (PVOID)PortNumberArray;
        NetPnpEventNotification.NetPnPEvent.BufferLength = sizeof(NDIS_PORT_NUMBER);

        Status = NdisMNetPnPEvent(Port->MiniportAdapterHandle, &NetPnpEventNotification);
        
        if (Status != NDIS_STATUS_SUCCESS)
        {
            TraceError( WMBCLASS_OID, "Failed to deactivate NDIS port %d. Status = 0x%08x\n", PortNumberToDeactivate, Status);
        }
        else
        {
            TraceInfo( WMBCLASS_OID, "Deactivated Port Number %d\n", PortNumberToDeactivate);
        }

        // set the port state to PortDeactivated, because there is little we can do if the call to NdisMSetPnPEvent fails apart
        // from logging the error. Setting the state to PortDeactivated will enable port re-use.

        MBB_ACQUIRE_PORT_LOCK(Port);
        
        Port->State = PortDeactivated;        

        MBB_RELEASE_PORT_LOCK(Port);

        Dereference(Port); // this reference was added for activation
        
    }while(FALSE);   

    return Status;
}

_Requires_lock_not_held_(&Adapter->PortsLock)
VOID
MbbWwanAddPortToAdapterList(
    _In_ PMINIPORT_ADAPTER_CONTEXT Adapter,
    _In_ PMBB_PORT* PortToAdd
    )
{
    if(Adapter && PortToAdd && *PortToAdd)
    {       
        MbbAdapterPortsLock(Adapter);
        
        // Add a reference to the port since now this port is being 
        // added to the adapter's port list
        Reference(*PortToAdd);

        Adapter->PortList[Adapter->NumberOfPorts] = *PortToAdd;
        Adapter->NumberOfPorts++;
        
        MbbAdapterPortsUnlock(Adapter);
    }
}

_Requires_lock_not_held_(&Adapter->PortsLock)
BOOL
MbbWwanRemovePortFromAdapterList(
    _In_ PMINIPORT_ADAPTER_CONTEXT Adapter,
    _In_ PMBB_PORT                 PortToRemove
)
{
    ULONG NumberOfPorts = 0;
    ULONG i = 0;
    BOOLEAN IsPortInAdapterList = FALSE;

    if (Adapter && PortToRemove)
    {
        MbbAdapterPortsLock(Adapter);

        NumberOfPorts = Adapter->NumberOfPorts;

        if (NumberOfPorts != 0 && NumberOfPorts <= Adapter->MaxActivatedContexts)
        {
            // first find the index of the port to be deleted in the adapter list
            for (i = 0; i < NumberOfPorts; i++)
            {
                if (Adapter->PortList[i] == PortToRemove)
                {
                    Adapter->PortList[i] = NULL;
                    IsPortInAdapterList = TRUE;
                    break;
                }
            }

            if (IsPortInAdapterList == FALSE)
            {
                MbbAdapterPortsUnlock(Adapter);
                return FALSE;
            }

            _Analysis_assume_(NumberOfPorts  > 0);

            NumberOfPorts = NumberOfPorts - 1;

            // now shift all the ports after this port to the left
            for (; i < NumberOfPorts; i++)
            {
                Adapter->PortList[i] = Adapter->PortList[i + 1];
                Adapter->PortList[i + 1] = NULL;
            }

            // decrement the count of ports
            Adapter->NumberOfPorts--;
        }

        MbbAdapterPortsUnlock(Adapter);

        // Remove the reference to the port since now this port has been 
        // removed from the adapter's port list

        Dereference(PortToRemove);
        return TRUE;
    }
    return FALSE;
}
_Requires_lock_not_held_(&Adapter->PortsLock)
PMBB_PORT
MbbWwanTranslatePortNumberToPort(
    _In_  PMINIPORT_ADAPTER_CONTEXT  Adapter,
    _In_  NDIS_PORT_NUMBER        PortNumber
    )
{
    ULONG        i = 0;
    PMBB_PORT    tempPort;

    MbbAdapterPortsLock(Adapter);

    for (i = 0; i < Adapter->NumberOfPorts; i++)
    {
        tempPort = Adapter->PortList[i];

        MBB_ACQUIRE_PORT_LOCK(tempPort);
        
        if (tempPort->PortNumber == PortNumber)
        {
            // Add a reference and return
            Reference(tempPort);
            MBB_RELEASE_PORT_LOCK(tempPort);
            MbbAdapterPortsUnlock(Adapter);
            return tempPort;
        }

        MBB_RELEASE_PORT_LOCK(tempPort);
    }

    MbbAdapterPortsUnlock(Adapter);

    return NULL;
}

_Requires_lock_not_held_(&Adapter->PortsLock)
PMBB_PORT
MbbWwanTranslateSessionIdToPort(
    _In_  PMINIPORT_ADAPTER_CONTEXT  Adapter,
    _In_  ULONG                      SessionId
    )
{
    ULONG        i = 0;
    PMBB_PORT    tempPort = NULL;

    do
    {
        if(SessionId == MBB_INVALID_SESSION_ID)
        {           
            break;
        }

        MbbAdapterPortsLock(Adapter);

        for (i = 0; i < Adapter->NumberOfPorts; i++)
        {
            tempPort = Adapter->PortList[i];
            
            MBB_ACQUIRE_PORT_LOCK(tempPort);

            if (tempPort->SessionId == SessionId)
            {
                // Add a reference and return
                Reference(tempPort);
                MBB_RELEASE_PORT_LOCK(tempPort);
                MbbAdapterPortsUnlock(Adapter);
                return tempPort;
            }
            
            MBB_RELEASE_PORT_LOCK(tempPort);
        }

        MbbAdapterPortsUnlock(Adapter);
    }while(FALSE);    

    return NULL;
}

_Requires_lock_not_held_(&Port->Lock)
VOID
MbbWwanSetPortConnectionId(
    __in  PMBB_PORT         Port,
    __in  ULONG             ConnectionId
    )
{
    if(Port != NULL)
    {
        MBB_ACQUIRE_PORT_LOCK(Port);
        Port->ConnectionId = ConnectionId;
        MBB_RELEASE_PORT_LOCK(Port);
    }
}

_Requires_lock_not_held_(&Port->Lock)
ULONG
MbbWwanGetPortConnectionId(
     __in  PMBB_PORT         Port
    )
{
    ULONG ConnectionId = 0;
    
    if(Port != NULL)
    {
        MBB_ACQUIRE_PORT_LOCK(Port);
        ConnectionId = Port->ConnectionId;
        MBB_RELEASE_PORT_LOCK(Port);    
    }

    return ConnectionId;
}

_Requires_lock_not_held_(&Port->Lock)
NDIS_STATUS
MbbWwanSetPortSessionId(
    __in  PMBB_PORT         Port
    )
{
    NDIS_STATUS Status = NDIS_STATUS_SUCCESS; 
    PMINIPORT_ADAPTER_CONTEXT Adapter = NULL;

    ULONG MaxActivatedContexts = 0;
    BOOLEAN fSessionIdFound = FALSE;
    UINT i = 0;

    if(!Port)
    {
        return NDIS_STATUS_INVALID_PARAMETER;
    }

    Adapter = Port->Adapter;
    
    MbbAdapterPortsLock(Adapter);
    MaxActivatedContexts = Adapter->MaxActivatedContexts;
    MbbAdapterPortsUnlock(Adapter);

    // Get a session Id and assign it to the port
    MbbAdapterSessionIdPortTableLock(Adapter);
            
    MBB_ACQUIRE_PORT_LOCK(Port); 

    if(Port->PortNumber == DEFAULT_NDIS_PORT_NUMBER)
    {
        if(MaxActivatedContexts >= 1) 
        {
            // We dont check for Adapter->SessionIdPortTable[0].InUse flag here
            // because if we send a connect request to the device with a session Id that 
            // is already connected, the device is supposed to succeed the call as per the MBIM spec.
            // Also this is existing Win8 behavior.
            
            Port->SessionId = MBB_DEFAULT_SESSION_ID;
            fSessionIdFound = TRUE;            
        }
        else
        {
            TraceError( WMBCLASS_UTIL, "[MbbWwanSetPortSessionId] Device should support atleast 1 context");
        }  
    }
    else
    {
        for(i = 1; i < MaxActivatedContexts; i++)
        {
            // We check for Adapter->SessionIdPortTable[0].InUse flag here because
            // at this instant there can be multiple PDP contexts connected and we need to 
            // find an available session Id that can be used for this new PDP context.

            if(!Adapter->SessionIdPortTable[i].InUse)
            {
                // free session id slot
                Port->SessionId = i;
                fSessionIdFound = TRUE;
                break;
            }
        }
    }

    if(fSessionIdFound)
    {
        Adapter->SessionIdPortTable[Port->SessionId].InUse = TRUE;        
        Adapter->SessionIdPortTable[Port->SessionId].PortNumber = Port->PortNumber;        
    }
    else
    {
        Status = NDIS_STATUS_FAILURE;
    }

    MBB_RELEASE_PORT_LOCK(Port);

    MbbAdapterSessionIdPortTableUnlock(Adapter);
 
    return Status;
}

_Requires_lock_not_held_(&Port->Lock)
LONG
MbbWwanGetPortSessionId(
      __in  PMBB_PORT         Port
    )
{
    LONG SessionId = MBB_INVALID_SESSION_ID;
    
    if(Port != NULL)
    {
        MBB_ACQUIRE_PORT_LOCK(Port);
        SessionId = Port->SessionId;
        MBB_RELEASE_PORT_LOCK(Port);
    }

    return SessionId;
}

_Requires_lock_not_held_(&Port->Lock)
VOID
MbbWwanResetPortSessionId(
    __in  PMBB_PORT         Port,
    __in  BOOLEAN           ReturnSessionId
    )
{
    if(Port != NULL)
    {
        PMINIPORT_ADAPTER_CONTEXT Adapter = Port->Adapter;
        ULONG SessionId = (ULONG)MBB_INVALID_SESSION_ID;
        
        MBB_ACQUIRE_PORT_LOCK(Port);
        SessionId = Port->SessionId; 

        // set the session id of the default port to 0 always.
        // this is because when service starts, get connection
        // state requests arrive on Port no 0. If the session id
        // is set to -1, these requests will fail. 

        Port->SessionId = (IS_ALLOCATED_PORT_NUMBER(Port->PortNumber)) ? 
                                MBB_INVALID_SESSION_ID : MBB_DEFAULT_SESSION_ID ;
        MBB_RELEASE_PORT_LOCK(Port);

        TraceInfo( WMBCLASS_UTIL, "[MbbWwanResetPortSessionId] Resetting Port session ID for port number %lu. Session ID: [%lu] -> [%lu]", Port->PortNumber, SessionId, Port->SessionId);       

        if(ReturnSessionId)
        {
            // this implies that a session id was actually alloted
            // to this port. Hence it needs to be returned.
            MbbWwanReturnSessionId(Adapter,SessionId);
        }
        else
        {            
            TraceInfo( WMBCLASS_UTIL, "[MbbWwanResetPortSessionId] Not returning session id = %d to adapter for port number %lu. This may be expected.", SessionId, Port->PortNumber);
        }
    }
}

_Requires_lock_not_held_(&Adapter->PortsLock)
_Requires_lock_not_held_(&Adapter->SessionIdPortTableLock)
NDIS_STATUS
MbbWwanReturnSessionId(
    __in PMINIPORT_ADAPTER_CONTEXT Adapter,
    __in ULONG                     SessionId)
{

    ULONG     MaxActivatedContexts = 0;
    
    /* SessionId is acquired whenever a context activation request comes. 
         * But it is returned in the following cases:
         * 1) Successful context deactivation. 
         * 2) Failed context activation attempt.
         * 3) When port is torn down before context deactivation.
         */

    MbbAdapterPortsLock(Adapter);
    MaxActivatedContexts = Adapter->MaxActivatedContexts;
    MbbAdapterPortsUnlock(Adapter);
    
    if(SessionId != MBB_INVALID_SESSION_ID
            && SessionId < MaxActivatedContexts)
    {
        TraceInfo( WMBCLASS_UTIL, "[MbbWwanReturnSessionId] Returning session id %lu to adapter",SessionId);
        
        MbbAdapterSessionIdPortTableLock(Adapter);

        Adapter->SessionIdPortTable[SessionId].InUse = FALSE;
        Adapter->SessionIdPortTable[SessionId].PortNumber = (ULONG)MBB_INVALID_PORT_NUMBER;
        
        MbbAdapterSessionIdPortTableUnlock(Adapter);
    }

    return NDIS_STATUS_SUCCESS;
}

_Requires_lock_not_held_(&Adapter->PortsLock)
DWORD
MbbWwanGetNumPortsConnected(
     __in PMINIPORT_ADAPTER_CONTEXT  Adapter
     )
{ 
    ULONG NumConnectedPorts = 0;
    ULONG i = 0;

    if(Adapter)
    {          
        MbbAdapterPortsLock(Adapter);
        
        for(i = 0; i < Adapter->NumberOfPorts && NumConnectedPorts < MBB_MAX_NUMBER_OF_PORTS; i++)
        {
            PMBB_PORT Port = Adapter->PortList[i];

            MBB_ACQUIRE_PORT_LOCK(Port);

            if(Port->ConnectionState.ConnectionUp)
            {
                ASSERT(Port->SessionId != MBB_INVALID_SESSION_ID && (ULONG)Port->SessionId < Adapter->MaxActivatedContexts);
                NumConnectedPorts++;
            }

            MBB_RELEASE_PORT_LOCK(Port);
        }

        MbbAdapterPortsUnlock(Adapter);      
    }    

    return NumConnectedPorts;
}

