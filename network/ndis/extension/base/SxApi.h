/*++

Copyright (c) Microsoft Corporation. All Rights Reserved.

Module Name:

    SxApi.h

Abstract:

    This file contains the API that must be implemented to
    create a switch extension using the SxBase library.


--*/

//
// The major version of NDIS the driver is using.
// For NDIS 6.30, use NDIS_FILTER_MAJOR_VERSION.
//
extern UCHAR  SxExtMajorNdisVersion;

//
// The minor version of NDIS the driver is using.
// For NDIS 6.30, use NDIS_FILTER_MINOR_VERSION.
//
extern UCHAR  SxExtMinorNdisVersion;

//
// The friendly name of the extension.
//
extern PWCHAR SxExtFriendlyName;

//
// The unique name of the extension.
//
extern PWCHAR SxExtUniqueName;

//
// The service name of the extension from the INF.
//
extern PWCHAR SxExtServiceName;

//
// The memory pool tag using in the extension.
//
extern ULONG  SxExtAllocationTag;

//
// The request ID used to identify OIDs initiated from this extension. 
//
extern ULONG  SxExtOidRequestId;


/*++

SxExtInitialize
  
Routine Description:
    This function is called from the SxBase Library during DriverEntry.
    An extension should allocate/initalize all global data in this function.
      
Arguments:
    NULL
    
Return Value:
    NDIS_STATUS_SUCCESS succeeds driver entry.
    
    NDIS_STATUS_*** fails driver entry.
   
--*/
NDIS_STATUS
SxExtInitialize();


/*++

SxExtUninitialize
  
Routine Description:
    This function is called from the SxBase Library during DriverUnload.
    An extension should free/reset all global data in this function.
      
Arguments:
    NULL
    
Return Value:
    VOID
   
--*/
VOID
SxExtUninitialize();


/*++

SxExtCreateSwitch
  
Routine Description:
    This function is called when an extension binds to a new switch.
    All switch specific data should be allocated during this function.
    OIDs cannot be sent from this function, and both the control
    and data paths are inactive.
      
Arguments:
    Switch - the Switch Object currently being created
    
    ExtensionContext -  Extension context specific to the switch being
                        createf. This context will be passed back to the
                        extension for all function calls in SxApi
    
Return Value:
    NDIS_STATUS_SUCCESS succeeds switch creation.
    
    NDIS_STATUS_RESOURCES fails switch creation because of insufficient
                          resources.
                          
    NDIS_STATUS_FAILURE fails switch creation.
   
--*/
NDIS_STATUS
SxExtCreateSwitch(
    _In_ PSX_SWITCH_OBJECT Switch,
    _Outptr_result_maybenull_ PNDIS_HANDLE *ExtensionContext
    );

    
/*++

SxExtDeleteSwitch
  
Routine Description:
    This function is called when an extension binds to a new switch.
    All switch specific data should be allocated/initialized during
    this function.
      
Arguments:
    Switch - the Switch being deleted
    
    ExtensionContext - The extension context allocated in SxExtCreateSwitch
                       for the switch being deleted.
    
Return Value:
    VOID
   
--*/
VOID
SxExtDeleteSwitch(
    _In_ PSX_SWITCH_OBJECT Switch,
    _In_ NDIS_HANDLE ExtensionContext
    );

    
/*++

SxExtActivateSwitch
  
Routine Description:
    This function is called to activate a switch. The function can be called
    while the switch is Running or Paused and should be used to bootstrap
    the switch if it was not Active when it was created.
      
Arguments:
    Switch - the Switch being activated
    
    ExtensionContext - The extension context allocated in SxExtCreateSwitch
                       for the switch being restarted.
    
Return Value:
    VOID
   
--*/   
VOID
SxExtActivateSwitch(
    _In_ PSX_SWITCH_OBJECT Switch,
    _In_ NDIS_HANDLE ExtensionContext
    );

    
/*++

SxExtRestartSwitch
  
Routine Description:
    This function is called to restart a switch from a paused state.
      
Arguments:
    Switch - the Switch being restarted
    
    ExtensionContext - The extension context allocated in SxExtCreateSwitch
                       for the switch being restarted.
    
Return Value:
    NDIS_STATUS_SUCCESS succeeds switch restart.
    
    NDIS_STATUS_RESOURCES fails switch restart because of insufficient
                          resources.
                          
    NDIS_STATUS_FAILURE fails switch restart.
   
--*/
NDIS_STATUS
SxExtRestartSwitch(
    _In_ PSX_SWITCH_OBJECT Switch,
    _In_ NDIS_HANDLE ExtensionContext
    );


/*++

SxExtPauseSwitch
  
Routine Description:
    This function is called to pause a switch from a running state.
      
Arguments:
    Switch - the Switch being paused
    
    ExtensionContext - The extension context allocated in SxExtCreateSwitch
                       for the switch being paused
    
Return Value:
    VOID
   
--*/    
VOID
SxExtPauseSwitch(
    _In_ PSX_SWITCH_OBJECT Switch,
    _In_ NDIS_HANDLE ExtensionContext
    );


/*++

SxExtCreatePort
  
Routine Description:
    This function is called to create a new port on a switch.
      
Arguments:
    Switch - the Switch context
    
    ExtensionContext - The extension context allocated in SxExtCreateSwitch
                       for the switch
                       
    Port - the Port being created
    
Return Value:
    NDIS_STATUS_SUCCESS to succeed port creation
    
    NDIS_STATUS_*** to fail port creation
   
--*/  
NDIS_STATUS
SxExtCreatePort(
    _In_ PSX_SWITCH_OBJECT Switch,
    _In_ NDIS_HANDLE ExtensionContext,
    _In_ PNDIS_SWITCH_PORT_PARAMETERS Port
    );

  
/*++

SxExtUpdatePort
  
Routine Description:
    This function is called to update an already created port.
      
Arguments:
    Switch - the Switch context
    
    ExtensionContext - The extension context allocated in SxExtCreateSwitch
                       for the switch
                       
    Port - the port being updated
    
Return Value:
    VOID
   
--*/  
VOID
SxExtUpdatePort(
    _In_ PSX_SWITCH_OBJECT Switch,
    _In_ NDIS_HANDLE ExtensionContext,
    _In_ PNDIS_SWITCH_PORT_PARAMETERS Port
    );

    
/*++

SxExtCreateNic
  
Routine Description:
    This function is called to create a new NIC to be connected
    to a switch.
    The extension may allocate context for this NIC, and traffic may
    start to flow from this NIC, but it may not be used as a destination
    until SxExtConnectNic has been called.
      
Arguments:
    Switch - the Switch context
    
    ExtensionContext - The extension context allocated in SxExtCreateSwitch
                       for the switch
                       
    Nic - the NIC being created
    
Return Value:
    NDIS_STATUS_SUCCESS to succeed NIC creation
    
    NDIS_STATUS_*** to fail NIC creation
   
--*/  
NDIS_STATUS
SxExtCreateNic(
    _In_ PSX_SWITCH_OBJECT Switch,
    _In_ NDIS_HANDLE ExtensionContext,
    _In_ PNDIS_SWITCH_NIC_PARAMETERS Nic
    );

    
/*++

SxExtConnectNic
  
Routine Description:
    This function is called to connect a NIC to a switch.
    After returning from this function the extension can use this NIC
    as a destination.
      
Arguments:
    Switch - the Switch context
    
    ExtensionContext - The extension context allocated in SxExtCreateSwitch
                       for the switch
                       
    Nic - the NIC being connected
    
Return Value:
    VOID
   
--*/  
VOID
SxExtConnectNic(
    _In_ PSX_SWITCH_OBJECT Switch,
    _In_ NDIS_HANDLE ExtensionContext,
    _In_ PNDIS_SWITCH_NIC_PARAMETERS Nic
    );

    
/*++

SxExtUpdateNic
  
Routine Description:
    This function is called to update an already created NIC.
      
Arguments:
    Switch - the Switch context
    
    ExtensionContext - The extension context allocated in SxExtCreateSwitch
                       for the switch
                       
    Nic - the NIC being updated
    
Return Value:
    VOID
   
--*/  
VOID
SxExtUpdateNic(
    _In_ PSX_SWITCH_OBJECT Switch,
    _In_ NDIS_HANDLE ExtensionContext,
    _In_ PNDIS_SWITCH_NIC_PARAMETERS Nic
    );

    
/*++

SxExtDisconnectNic
  
Routine Description:
    This function is called to disconnect a NIC from a switch.
    After returning from this function the extension cannot use
    this NIC as a destination.
      
Arguments:
    Switch - the Switch context
    
    ExtensionContext - The extension context allocated in SxExtCreateSwitch
                       for the switch
                       
    Nic - the NIC being disconnected
    
Return Value:
    VOID
   
--*/ 
VOID
SxExtDisconnectNic(
    _In_ PSX_SWITCH_OBJECT Switch,
    _In_ NDIS_HANDLE ExtensionContext,
    _In_ PNDIS_SWITCH_NIC_PARAMETERS Nic
    );

    
/*++

SxExtDeleteNic
  
Routine Description:
    This function is called to delete a NIC from a switch.
    No futher traffic/control will be recieved for this NIC.

Arguments:
    Switch - the Switch context
    
    ExtensionContext - The extension context allocated in SxExtCreateSwitch
                       for the switch
                       
    Nic - the NIC being deleted
    
Return Value:
    VOID
   
--*/
VOID
SxExtDeleteNic(
    _In_ PSX_SWITCH_OBJECT Switch,
    _In_ NDIS_HANDLE ExtensionContext,
    _In_ PNDIS_SWITCH_NIC_PARAMETERS Nic
    );

    
/*++

SxExtTeardownPort
  
Routine Description:
    This function is called to start deletion of a port on a switch.
    Upon recieving this call, no further references may be taken
    on the given port.

Arguments:
    Switch - the Switch context
    
    ExtensionContext - The extension context allocated in SxExtCreateSwitch
                       for the switch
                       
    Port - the Port being deleted
    
Return Value:
    VOID
   
--*/
VOID
SxExtTeardownPort(
    _In_ PSX_SWITCH_OBJECT Switch,
    _In_ NDIS_HANDLE ExtensionContext,
    _In_ PNDIS_SWITCH_PORT_PARAMETERS Port
    );


/*++

SxExtDeletePort
  
Routine Description:
    This function is called to finish deletion of a port on a switch.
    Upon recieving this call, no traffic/control will be recieved
    for this port.

Arguments:
    Switch - the Switch context
    
    ExtensionContext - The extension context allocated in SxExtCreateSwitch
                       for the switch
                       
    Port - the Port being deleted
    
Return Value:
    VOID
   
--*/
VOID
SxExtDeletePort(  
    _In_ PSX_SWITCH_OBJECT Switch,
    _In_ NDIS_HANDLE ExtensionContext,
    _In_ PNDIS_SWITCH_PORT_PARAMETERS Port
    );

    
/*++

SxExtSaveNic
  
Routine Description:
    This function is called to retrieve save data for a given NIC.
    This function will be called until all extensions have finished
    saving data.
    
    An new save for this NIC will not start until SxExtSaveNicComplete
    has been received.
    
    If returning NDIS_STATUS_SUCCESS from this function, and
    BytesWritten > 0 you must write to the ExtensionId,
    ExtensionFriendlyName, SaveDataSize and SaveData fields in
    SaveState. SxExtUniqueName MUST be written to ExtensionId.
    SxExtFriendlyName should be written to ExtensionFriendlyName.
    
    If returning NDIS_STATUS_SUCCESS with BytesWritten == 0,
    DO NOT write any data to any fields.

Arguments:
    Switch - the Switch context
    
    ExtensionContext - The extension context allocated in SxExtCreateSwitch
                       for the switch
                       
    SaveState - the save information and buffer to save to
    
    BytesWritten - the # of bytes written to the save buffer
    
    BytesNeeded - the length of the save buffer needed
    
Return Value:
    NDIS_STATUS_SUCCESS - if the buffer was successfully written, or not
                          needed and BytesWritten is set to 0
                          
    NDIS_STATUS_BUFFER_TOO_SHORT - if the buffer is too short for the
                                   necessary save, write the length needed
                                   in BytesNeeded

    NDIS_STATUS_*** - to fail the save operation
   
--*/
NDIS_STATUS
SxExtSaveNic(  
    _In_ PSX_SWITCH_OBJECT Switch,
    _In_ NDIS_HANDLE ExtensionContext,
    _Inout_ PNDIS_SWITCH_NIC_SAVE_STATE SaveState,
    _Out_ PULONG BytesWritten,
    _Out_ PULONG BytesNeeded
    );


/*++

SxExtSaveNicComplete
  
Routine Description:
    This function is called to notify the extension that saving
    the given NIC has been completed by all extensions.

Arguments:
    Switch - the Switch context
    
    ExtensionContext - The extension context allocated in SxExtCreateSwitch
                       for the switch
                       
    SaveState - the save information
    
Return Value:
    VOID
   
--*/    
VOID
SxExtSaveNicComplete(  
    _In_ PSX_SWITCH_OBJECT Switch,
    _In_ NDIS_HANDLE ExtensionContext,
    _In_ PNDIS_SWITCH_NIC_SAVE_STATE SaveState
    );

    
/*++

SxExtNicRestore
  
Routine Description:
    This function is called to restore previously saved data.

Arguments:
    Switch - the Switch context
    
    ExtensionContext - The extension context allocated in SxExtCreateSwitch
                       for the switch
                       
    SaveState - the save information
    
    BytesRestored - the number of bytes restored from the saved data
    
Return Value:
    NDIS_STATUS_SUCCESS - if this data belongs to this extension, and was
                          successfully restored (BytesRestored written)
                          OR
                          this data does not belong to this extension
                          (BytesRestored == 0)    
                              
    NDIS_STATUS_*** - there was an error while attempting to restore
                      this data
   
--*/    
NDIS_STATUS
SxExtNicRestore(  
    _In_ PSX_SWITCH_OBJECT Switch,
    _In_ NDIS_HANDLE ExtensionContext,
    _In_ PNDIS_SWITCH_NIC_SAVE_STATE SaveState,
    _Out_ PULONG BytesRestored
    );

    
/*++

SxExtNicRestoreComplete
  
Routine Description:
    This function is called to signify the end of a restore operation.

Arguments:
    Switch - the Switch context
    
    ExtensionContext - The extension context allocated in SxExtCreateSwitch
                       for the switch
                       
    SaveState - the save information
        
Return Value:
    VOID
   
--*/    
VOID
SxExtNicRestoreComplete(  
    _In_ PSX_SWITCH_OBJECT Switch,
    _In_ NDIS_HANDLE ExtensionContext,
    _In_ PNDIS_SWITCH_NIC_SAVE_STATE SaveState
    );
    
    
/*++

SxExtAddSwitchProperty
  
Routine Description:
    This function is called to add a property on the given switch.

Arguments:
    Switch - the Switch context
    
    ExtensionContext - The extension context allocated in SxExtCreateSwitch
                       for the switch
                       
    SwitchProperty - the property to be applied
    
Return Value:
    NDIS_STATUS_NOT_SUPPORTED - if the policy is not consumed by this extension
    
    NDIS_STATUS_SUCCESS - if the policy is consumed by this extension, and
                          can successfully be enforced
                          
    STATUS_DATA_NOT_ACCEPTED - if the policy is consumed by this
                               extension, but cannot be enforced
                               
    NDIS_STATUS_*** - if the policy is consumed by this extension, and
                      setting the valid policy failed
   
--*/ 
NDIS_STATUS
SxExtAddSwitchProperty(
    _In_ PSX_SWITCH_OBJECT Switch,
    _In_ NDIS_HANDLE ExtensionContext,
    _In_ PNDIS_SWITCH_PROPERTY_PARAMETERS SwitchProperty
    );


/*++

SxExtUpdateSwitchProperty
  
Routine Description:
    This function is called to update a property on the given switch.

Arguments:
    Switch - the Switch context
    
    ExtensionContext - The extension context allocated in SxExtCreateSwitch
                       for the switch
                       
    SwitchProperty - the property to be updated
    
Return Value:
    NDIS_STATUS_NOT_SUPPORTED - if the policy is not consumed by this extension
    
    NDIS_STATUS_SUCCESS - if the policy is consumed by this extension, and
                          can successfully be enforced
                          
    STATUS_DATA_NOT_ACCEPTED - if the policy is consumed by this
                               extension, but cannot be enforced
                               
    NDIS_STATUS_*** - if the policy is consumed by this extension, and
                      setting the valid policy failed
   
--*/ 
NDIS_STATUS
SxExtUpdateSwitchProperty(
    _In_ PSX_SWITCH_OBJECT Switch,
    _In_ NDIS_HANDLE ExtensionContext,
    _In_ PNDIS_SWITCH_PROPERTY_PARAMETERS SwitchProperty
    );

    
/*++

SxExtDeleteSwitchProperty
  
Routine Description:
    This function is called to delete a property on the given switch.

Arguments:
    Switch - the Switch context
    
    ExtensionContext - The extension context allocated in SxExtCreateSwitch
                       for the switch
                       
    SwitchProperty - the property to be deleted
    
Return Value:
    TRUE - if the policy belongs to this extension
    
    FALSE - otherwise
   
--*/ 
BOOLEAN
SxExtDeleteSwitchProperty(
    _In_ PSX_SWITCH_OBJECT Switch,
    _In_ NDIS_HANDLE ExtensionContext,
    _In_ PNDIS_SWITCH_PROPERTY_DELETE_PARAMETERS SwitchProperty
    );


/*++

SxExtAddPortProperty
  
Routine Description:
    This function is called to add a property on the given port,
    on the given switch.

Arguments:
    Switch - the Switch context
    
    ExtensionContext - The extension context allocated in SxExtCreateSwitch
                       for the switch
                       
    PortProperty - the property to be applied
    
Return Value:
    NDIS_STATUS_NOT_SUPPORTED - if the policy is not consumed by this extension
    
    NDIS_STATUS_SUCCESS - if the policy is consumed by this extension, and
                          can successfully be enforced
                          
    STATUS_DATA_NOT_ACCEPTED - if the policy is consumed by this
                               extension, but cannot be enforced
                               
    NDIS_STATUS_*** - if the policy is consumed by this extension, and
                      setting the valid policy failed
   
--*/ 
NDIS_STATUS
SxExtAddPortProperty(
    _In_ PSX_SWITCH_OBJECT Switch,
    _In_ NDIS_HANDLE ExtensionContext,
    _In_ PNDIS_SWITCH_PORT_PROPERTY_PARAMETERS PortProperty
    );


/*++

SxExtUpdatePortProperty
  
Routine Description:
    This function is called to update a property on the given port,
    on the given switch.

Arguments:
    Switch - the Switch context
    
    ExtensionContext - The extension context allocated in SxExtCreateSwitch
                       for the switch
                       
    PortProperty - the property to be applied
    
Return Value:
    NDIS_STATUS_NOT_SUPPORTED - if the policy is not consumed by this extension
    
    NDIS_STATUS_SUCCESS - if the policy is consumed by this extension, and
                          can successfully be enforced
                          
    STATUS_DATA_NOT_ACCEPTED - if the policy is consumed by this
                               extension, but cannot be enforced
                               
    NDIS_STATUS_*** - if the policy is consumed by this extension, and
                      setting the valid policy failed
   
--*/     
NDIS_STATUS
SxExtUpdatePortProperty(
    _In_ PSX_SWITCH_OBJECT Switch,
    _In_ NDIS_HANDLE ExtensionContext,
    _In_ PNDIS_SWITCH_PORT_PROPERTY_PARAMETERS PortProperty
    );

    
/*++

SxExtDeletePortProperty
  
Routine Description:
    This function is called to delete a property on the given port,
    on the given switch.

Arguments:
    Switch - the Switch context
    
    ExtensionContext - The extension context allocated in SxExtCreateSwitch
                       for the switch
                       
    SwitchProperty - the property to be deleted
    
Return Value:
    TRUE - if the policy is not consumed by this extension
    
    FALSE - otherwise
   
--*/ 
BOOLEAN
SxExtDeletePortProperty(
    _In_ PSX_SWITCH_OBJECT Switch,
    _In_ NDIS_HANDLE ExtensionContext,
    _In_ PNDIS_SWITCH_PORT_PROPERTY_DELETE_PARAMETERS PortProperty
    );

    
/*++

SxExtQuerySwitchFeatureStatus
  
Routine Description:
    This function is called to query the status of a custom property
    on the given switch.

Arguments:
    Switch - the Switch context
    
    ExtensionContext - The extension context allocated in SxExtCreateSwitch
                       for the switch
                       
    SwitchFeatureStatus - the property buffer
    
    BytesNeeded - if SwitchFeatureStatus is too small, this should be set
                  to the size buffer needed
    
Return Value:
    TRUE - return true if this property belongs to this extension, if
           BytesNeeded > 0, the buffer will be reallocated and
           this function will be called again
    
    FALSE - otherwise
   
--*/
BOOLEAN
SxExtQuerySwitchFeatureStatus(
    _In_ PSX_SWITCH_OBJECT Switch,
    _In_ NDIS_HANDLE ExtensionContext,
    _Inout_ PNDIS_SWITCH_FEATURE_STATUS_PARAMETERS SwitchFeatureStatus,
    _Inout_ PULONG BytesNeeded
    );


/*++

SxExtQueryPortFeatureStatus
  
Routine Description:
    This function is called to query the status of a custom property
    on the given port, on the given switch.

Arguments:
    Switch - the Switch context
    
    ExtensionContext - The extension context allocated in SxExtCreateSwitch
                       for the switch
                       
    PortFeatureStatus - the property buffer
    
    BytesNeeded - if PortFeatureStatus is too small, this should be set
                  to the size buffer needed
    
Return Value:
    TRUE - return true if this property belongs to this extension, if
           BytesNeeded > 0, the buffer will be reallocated and
           this function will be called again
    
    FALSE - otherwise
   
--*/
BOOLEAN
SxExtQueryPortFeatureStatus(
    _In_ PSX_SWITCH_OBJECT Switch,
    _In_ NDIS_HANDLE ExtensionContext,
    _Inout_ PNDIS_SWITCH_PORT_FEATURE_STATUS_PARAMETERS PortFeatureStatus,
    _Inout_ PULONG BytesNeeded
    );
    
    
/*++

SxExtProcessNicRequest
  
Routine Description:
    This function is called upon the reciept of an OID_SWITCH_NIC_REQUEST
    to the extension.
    If an extension wishes to redirect the OID, it must return a valid
    DestinationPortId and DestinationNicIndex, which it has taken a
    reference on.
    If an extension wishes to set source information, it must return
    a valid SourcePortId and SourceNicIndex, which it has taken a
    reference on.
    The extension can change the OidRequest if it needs to.
    
    !! This function should only be used by forwarding extensions. !!

Arguments:
    Switch - the Switch context
    
    ExtensionContext - The extension context allocated in SxExtCreateSwitch
                       for the switch
                       
    OidRequest - the OID wrapped by the NIC request
    
    SourcePortId - the source PortId to set
    
    SourceNicIndex - the source NicIndex to set
    
    DestinationPortId - the destination PortId to set
    
    DestinationNicIndex - the destination NicIndex to set
    
Return Value:
    NDIS_STATUS_SUCCESS - sends OID
    
    NDIS_STATUS_*** - complete OID with given status
   
--*/
NDIS_STATUS
SxExtProcessNicRequest(
    _In_ PSX_SWITCH_OBJECT Switch,
    _In_ NDIS_HANDLE ExtensionContext,
    _Inout_ PNDIS_OID_REQUEST OidRequest,
    _Inout_ PNDIS_SWITCH_PORT_ID SourcePortId,
    _Inout_ PNDIS_SWITCH_NIC_INDEX SourceNicIndex,
    _Inout_ PNDIS_SWITCH_PORT_ID DestinationPortId,
    _Inout_ PNDIS_SWITCH_NIC_INDEX DestinationNicIndex
    );


/*++

SxExtProcessNicRequestComplete
  
Routine Description:
    This function is called upon the completion of an OID_SWITCH_NIC_REQUEST
    that this extension has previously altered.
    The extension must derefernce all NICs it had previously taken references
    on.
    If the OidRequest buffer was changed (along with underlying buffers),
    the previous data must be replaced.

Arguments:
    Switch - the Switch context
    
    ExtensionContext - The extension context allocated in SxExtCreateSwitch
                       for the switch
                       
    NicOidRequest - the OID buffer, encapsulated with source/destination info
    
    Status - the status the OID completed with
    
Return Value:
    NDIS_STATUS - the status to complete the OID request with
   
--*/    
NDIS_STATUS
SxExtProcessNicRequestComplete(
    _In_ PSX_SWITCH_OBJECT Switch,
    _In_ NDIS_HANDLE ExtensionContext,
    _Inout_ PNDIS_OID_REQUEST OidRequest,
    _In_ NDIS_SWITCH_PORT_ID SourcePortId,
    _In_ NDIS_SWITCH_NIC_INDEX SourceNicIndex,
    _In_ NDIS_SWITCH_PORT_ID DestinationPortId,
    _In_ NDIS_SWITCH_NIC_INDEX DestinationNicIndex,
    _In_ NDIS_STATUS Status
    );
    

/*++

SxExtProcessNicStatus
  
Routine Description:
    This function is called upon the reciept of an NDIS_STATUS_SWITCH_NIC_STATUS
    to the extension.
    If the extension wishes to modify the status indication, it should
    send its own status indication using NdisFIndicateStatus and return a
    failure status.
    If the extension wishes to drop the status indiction, it should return
    failure status, though this should be done very sparingly and carefully.
    
    !! This function should only be used by forwarding extensions. !!

Arguments:
    Switch - the Switch context
    
    ExtensionContext - The extension context allocated in SxExtCreateSwitch
                       for the switch
                       
    StatusIndication - the indication wrapped by the NIC status indication
    
    SourcePortId - the source PortId of the indication
    
    SourceNicIndex - the source NicIndex of the indication
    
Return Value:
    NDIS_STATUS_SUCCESS - return to continue forwarding this indication
    
    NDIS_STATUS_*** - if the extension wants to modify the status
                      if modifying the status the extension should indicate
                      its own modified status using
                      SxLibIssueNicStatusIndicationUnsafe as soon as possible
   
--*/
NDIS_STATUS
SxExtProcessNicStatus(
    _In_ PSX_SWITCH_OBJECT Switch,
    _In_ NDIS_HANDLE ExtensionContext,
    _In_ PNDIS_STATUS_INDICATION StatusIndication,
    _In_ NDIS_SWITCH_PORT_ID SourcePortId,
    _In_ NDIS_SWITCH_NIC_INDEX SourceNicIndex
    );

    
/*++

SxExtStartNetBufferListsIngress
  
Routine Description:
    This function is called upon the receipt on an NBL on ingress.
    The extension should call SxLibSendNetBufferListsIngress to continue
    the send of the NBL on ingress.
    The extension should call SxLibCompleteNetBufferListsIngress to
    drop the NBL.
    This function may also be call from egress to inject an NBL.

Arguments:
    Switch - the Switch context
    
    ExtensionContext - The extension context allocated in SxExtCreateSwitch
                       for the switch
                       
    NetBufferLists - the NBL to be sent
        
    SendFlags - the send flags from NDIS, equivalent to NDIS send flags for
                NdisFSendNetBufferLists
    
Return Value:
    VOID
   
--*/  
VOID
SxExtStartNetBufferListsIngress(
    _In_ PSX_SWITCH_OBJECT Switch,
    _In_ NDIS_HANDLE ExtensionContext,
    _In_ PNET_BUFFER_LIST NetBufferLists,
    _In_ ULONG SendFlags
    );


/*++

SxExtStartNetBufferListsEgress
  
Routine Description:
    This function is called upon the receipt on an NBL on egress.
    The extension should call SxLibSendNetBufferListsEgress to continue
    the send of the NBL on egress.
    The extension should call SxLibSendNetBufferListsEgressComplete to
    drop the NBL.

Arguments:
    Switch - the Switch context
    
    ExtensionContext - The extension context allocated in SxExtCreateSwitch
                       for the switch
                       
    NetBufferLists - the NBL to be sent
        
    NumberOfNetBufferLists - the number of NBLs in NetBufferLists
    
    ReceiveFlags - the receive flags from NDIS, equivalent to NDIS receive flags for
                   NdisFIndicateReceiveNetBufferLists
    
Return Value:
    VOID
   
--*/     
VOID
SxExtStartNetBufferListsEgress(
    _In_ PSX_SWITCH_OBJECT Switch,
    _In_ NDIS_HANDLE ExtensionContext,
    _In_ PNET_BUFFER_LIST NetBufferLists,
    _In_ ULONG NumberOfNetBufferLists,
    _In_ ULONG ReceiveFlags
    );

    
/*++

SxExtStartCompleteNetBufferListsEgress
  
Routine Description:
    This function is called upon the completion of an NBL on egress.
    The extension must call SxLibCompleteNetBufferListsEgress
    once it has finished processing the NBL.

Arguments:
    Switch - the Switch context
    
    ExtensionContext - The extension context allocated in SxExtCreateSwitch
                       for the switch
                       
    NetBufferLists - the NBL being completed
    
    ReturnFlags - the return flags from NDIS, equivalent to NDIS return flags for
                  NdisFReturnNetBufferLists
    
Return Value:
    VOID
   
--*/ 
VOID
SxExtStartCompleteNetBufferListsEgress(
    _In_ PSX_SWITCH_OBJECT Switch,
    _In_ NDIS_HANDLE ExtensionContext,
    _In_ PNET_BUFFER_LIST NetBufferLists,
    _In_ ULONG ReturnFlags
    );

    
/*++

SxExtStartCompleteNetBufferListsIngress
  
Routine Description:
    This function is called upon the completion of an NBL on ingress.
    The extension must call SxLibCompleteNetBufferListsIngress
    once it has finished processing the NBL.
    
    If there are NBLs injected by this extension in NetBufferLists,
    the extension must NOT call SxLibCompleteNetBufferListsIngress, and
    instead call SxLibCompletedInjectedNetBufferLists with the number
    of injected NBLs completed.

Arguments:
    Switch - the Switch context
    
    ExtensionContext - The extension context allocated in SxExtCreateSwitch
                       for the switch
                       
    NetBufferLists - the NBL being completed
        
    SendCompleteFlags - the send complete flags from NDIS, equivalent to
                        NDIS send complete flags for
                        NdisFSendNetBufferListsComplete
    
Return Value:
    VOID
   
--*/ 
VOID
SxExtStartCompleteNetBufferListsIngress(
    _In_ PSX_SWITCH_OBJECT Switch,
    _In_ NDIS_HANDLE ExtensionContext,
    _In_ PNET_BUFFER_LIST NetBufferLists,
    _In_ ULONG SendCompleteFlags
    );
    
