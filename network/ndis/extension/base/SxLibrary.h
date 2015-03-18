/*++

Copyright (c) Microsoft Corporation. All Rights Reserved.

Module Name:

    SxLibrary.h

Abstract:

    This file contains the common library function headers that can be
    used by any extension using the SxBase library.


--*/


/*++

SxLibSendNetBufferListsIngress
  
Routine Description:
    This function is called to forward NBLs on ingress.
    The extension MUST call this function, or call
    SxLibCompleteNetBufferListsIngress for every NBL in NetBufferLists,
    recieved in SxExtStartNetBufferListsIngress.
    
    This function can also be called to inject NBLs.
    If there are NBLs in NetBufferLists that are initiated by the
    extension, NumInjectedNetBufferLists must be the number of new NBLs.
    
Arguments:

    Switch - the Switch context
    
    NetBufferLists - the NBLs to send
    
    SendFlags - the SendFlags equivalent to NDIS flags for
                NdisFSendNetBufferLists
   
    NumInjectedNetBufferLists - the number of NBLs in NetBufferLists initiated
                                by the extension
    
Return Value:
    VOID
   
--*/
VOID
SxLibSendNetBufferListsIngress(
    _In_ PSX_SWITCH_OBJECT Switch,
    _In_ PNET_BUFFER_LIST NetBufferLists,
    _In_ ULONG SendFlags,
    _In_ ULONG NumInjectedNetBufferLists
    );

    
/*++

SxLibSendNetBufferListsEgress
  
Routine Description:
    This function is called to forward NBLs on egress.
    The extension MUST call this function, or call
    SxLibCompleteNetBufferListsEgress for every NBL in NetBufferLists
    recieved in SxExtStartNetBufferListsEgress.
    
Arguments:

    Switch - the Switch context
    
    NetBufferLists - the NBLs to send
    
    NumberOfNetBufferLists - the number of NBLs in NetBufferLists
    
    ReceiveFlags - the ReceiveFlags equivalent to NDIS flags for
                   NdisFIndicateReceiveNetBufferLists
    
Return Value:
    VOID
   
--*/
VOID
SxLibSendNetBufferListsEgress(
    _In_ PSX_SWITCH_OBJECT Switch,
    _In_ PNET_BUFFER_LIST NetBufferLists,
    _In_ ULONG NumberOfNetBufferLists,
    _In_ ULONG ReceiveFlags
    );

    
/*++

SxLibCompleteNetBufferListsEgress
  
Routine Description:
    This function is called to complete NBLs on egress.
    The extension MUST call this function for all NBLs recieved
    in SxExtStartCompleteNetBufferListsEgress.
    
Arguments:

    Switch - the Switch context
    
    NetBufferLists - the NBLs to send
    
    ReturnFlags - the ReceiveFlags equivalent to NDIS flags for
                  NdisFReturnNetBufferLists
    
Return Value:
    VOID
   
--*/
VOID
SxLibCompleteNetBufferListsEgress(
    _In_ PSX_SWITCH_OBJECT Switch,
    _In_ PNET_BUFFER_LIST NetBufferLists,
    _In_ ULONG ReturnFlags
    );

/*++

SxLibCompleteNetBufferListsIngress
  
Routine Description:
    This function is called to complete NBLs on ingress.
    The extension MUST call this function, or
    SxLibCompletedInjectedNetBufferLists for all NBLs recieved in
    SxExtStartCompleteNetBufferListsEgress.
    
Arguments:

    Switch - the Switch context
    
    NetBufferLists - the NBLs to send
    
    SendCompleteFlags - the ReceiveFlags equivalent to NDIS flags for
                        NdisFSendNetBufferListsComplete
    
Return Value:
    VOID
   
--*/
VOID
SxLibCompleteNetBufferListsIngress(
    _In_ PSX_SWITCH_OBJECT Switch,
    _In_ PNET_BUFFER_LIST NetBufferLists,
    _In_ ULONG SendCompleteFlags
    );

    
/*++

SxLibCompletedInjectedNetBufferLists
  
Routine Description:
    This function is called after completing NBLs injected
    by the extension.
    
Arguments:

    Switch - the Switch context
    
    NumInjectedNetBufferLists - the number of NBLs completed
    
Return Value:
    VOID
   
--*/
VOID
SxLibCompletedInjectedNetBufferLists(
    _In_ PSX_SWITCH_OBJECT Switch,
    _In_ ULONG NumInjectedNetBufferLists
    );    

    
/*++

SxLibIssueOidRequest

Routine Description:

    Utility routine that forms and sends an NDIS_OID_REQUEST to the
    miniport, waits for it to complete, and returns status
    to the caller.

    NOTE: this assumes that the calling routine ensures validity
    of the filter handle until this returns.
    
    This function can only be called at PASSIVE_LEVEL.

Arguments:

    Switch - pointer to our switch object.

    RequestType - NdisRequest[Set|Query|method]Information.

    Oid - the object being set/queried.

    InformationBuffer - data for the request.

    InformationBufferLength - length of the above.

    OutputBufferLength - valid only for method request.

    MethodId - valid only for method request.
    
    Timeout - The timeout in seconds for the OID. 

    BytesNeeded - place to return bytes read/written.

Return Value:

    NDIS_STATUS_***

--*/
NDIS_STATUS
SxLibIssueOidRequest(
    _In_ PSX_SWITCH_OBJECT SxSwitch,
    _In_ NDIS_REQUEST_TYPE RequestType,
    _In_ NDIS_OID Oid,
    _In_opt_ PVOID InformationBuffer,
    _In_ ULONG InformationBufferLength,
    _In_ ULONG OutputBufferLength,
    _In_ ULONG MethodId,
    _In_ UINT Timeout,
    _Out_ PULONG BytesNeeded
    );
  

/*++

SxLibGetSwitchParametersUnsafe
  
Routine Description:
    This function is called to get the current state of the switch.
    
Arguments:

    Switch - the Switch context
    
    SwitchParameters - the returned switch parameters
    
Return Value:
    NDIS_STATUS_SUCCESS - if SwitchParameters was successfully returned
                          
    NDIS_STATUS_*** - otherwise
   
--*/   
NDIS_STATUS
SxLibGetSwitchParametersUnsafe(
    _In_ PSX_SWITCH_OBJECT Switch,
    _Out_ PNDIS_SWITCH_PARAMETERS SwitchParameters
    );

    
/*++

SxLibGetPortArrayUnsafe
  
Routine Description:
    This function is called to get the current array
    of ports.
    
    NOTE: It is necessary to synchonize this with SxExtPortCreate
    and SxExtPortTeardown.
    
Arguments:

    Switch - the Switch context
    
    PortArray - the returned port array
    
Return Value:
    NDIS_STATUS_SUCCESS - if PortArray was successfully allocated
                          and returned
                          
    NDIS_STATUS_*** - otherwise
   
--*/
NDIS_STATUS
SxLibGetPortArrayUnsafe(
    _In_ PSX_SWITCH_OBJECT Switch,
    _Out_ PNDIS_SWITCH_PORT_ARRAY *PortArray
    );    

    
/*++

SxLibGetNicArrayUnsafe
  
Routine Description:
    This function is called to get the current array
    of NICs.
    
    NOTE: It is necessary to synchonize this with SxExtNicConnect
    and SxExtNicDisconnect.
    
Arguments:

    Switch - the Switch context
    
    NicArray - the returned NIC array
    
Return Value:
    NDIS_STATUS_SUCCESS - if NicArray was successfully allocated
                          and returned
                          
    NDIS_STATUS_*** - otherwise
   
--*/
NDIS_STATUS
SxLibGetNicArrayUnsafe(
    _In_ PSX_SWITCH_OBJECT SxSwitch,
    _Out_ PNDIS_SWITCH_NIC_ARRAY *NicArray
    );

/*++

SxLibGetSwitchPropertyUnsafe
  
Routine Description:
    This function is called to get the current array of the switch
    property queried.
    
    NOTE: It is necessary to synchonize this with SxExtAddSwitchProperty
    and SxExtDeleteSwitchProperty.
    
Arguments:

    Switch - the Switch context
    
    PropertyType - the PropertyType to query for
    
    PropertyId - the GUID of the property (from mof file)
    
    PropertyVersion - the version of the property
    
    SwitchPropertyEnumParameters - the returned property enum
    
Return Value:
    NDIS_STATUS_SUCCESS - if SwitchPropertyEnumParameters was
                          successfully allocated and returned
                          
    NDIS_STATUS_*** - otherwise
   
--*/
NDIS_STATUS
SxLibGetSwitchPropertyUnsafe(
    _In_ PSX_SWITCH_OBJECT Switch,
    _In_ NDIS_SWITCH_PROPERTY_TYPE PropertyType,
    _In_opt_ PNDIS_SWITCH_OBJECT_ID PropertyId,
    _Outptr_ PNDIS_SWITCH_PROPERTY_ENUM_PARAMETERS *SwitchPropertyEnumParameters
    );

/*++

SxLibGetPortPropertyUnsafe
  
Routine Description:
    This function is called to get the current array of the switch
    property queried.
    
    NOTE: It is necessary to synchonize this with SxExtAddPortProperty
    and SxExtDeletePortProperty.
    
Arguments:

    Switch - the Switch context
    
    PortId - the port to query from
    
    PropertyType - the PropertyType to query for
    
    PropertyId - the GUID of the property (from mof file)
        
    PortPropertyEnumParameters - the returned property enum
    
Return Value:
    NDIS_STATUS_SUCCESS - if PortPropertyEnumParameters was
                          successfully allocated and returned
                          
    NDIS_STATUS_*** - otherwise
   
--*/
NDIS_STATUS
SxLibGetPortPropertyUnsafe(
    _In_ PSX_SWITCH_OBJECT Switch,
    _In_ NDIS_SWITCH_PORT_ID PortId,
    _In_ NDIS_SWITCH_PORT_PROPERTY_TYPE PropertyType,
    _In_opt_ PNDIS_SWITCH_OBJECT_ID PropertyId,
    _Outptr_ PNDIS_SWITCH_PORT_PROPERTY_ENUM_PARAMETERS *PortPropertyEnumParameters
    );

/*++

SxLibRevokeVfUnsafe
  
Routine Description:
    This function is called revoke the VF assignment for the
    given VM.
    
    NOTE: This must be synchonized with SxExtNicConnect and
    SxExtNicDisconnect for the PortId given, and ReferenceSwitchNic
    must have been successfully called.
    
Arguments:

    Switch - the Switch context
    
    PortId - the port the VM is connected to
    
Return Value:
    VOID
   
--*/  
VOID
SxLibRevokeVfUnsafe(
    _In_ PSX_SWITCH_OBJECT Switch,
    _In_ NDIS_SWITCH_PORT_ID PortId
    );

/*++

SxLibIssueNicStatusIndicationUnsafe
  
Routine Description:
    This function is called issue a NIC status indication.
    
    NOTE: This must be synchonized with SxExtNicConnect and
    SxExtNicDisconnect for the PortId given, and ReferenceSwitchNic
    must have been successfully called.
    
Arguments:

    Switch - the Switch context
    
    StatusCode - the status code to indicate
    
    PortId - the port to indicate to/from
    
    NicIndex - the nic index to indicate to/from
    
    IsDestination - TRUE if PortId/NicIndex is destination info
                    FALSE if PortId/NicIndex is source info
                    
    StatusBuffer - the StatusBuffer for the indication
    
    StatusBufferSize - the size of StatusBuffer
    
Return Value:
    VOID
   
--*/  
VOID
SxLibIssueNicStatusIndicationUnsafe(
    _In_ PSX_SWITCH_OBJECT Switch,
    _In_ NDIS_STATUS StatusCode,
    _In_ NDIS_SWITCH_PORT_ID PortId,
    _In_ NDIS_SWITCH_NIC_INDEX NicIndex,
    _In_ BOOLEAN IsDestination,
    _In_opt_ PVOID StatusBuffer,
    _In_ ULONG StatusBufferSize
    );
    
