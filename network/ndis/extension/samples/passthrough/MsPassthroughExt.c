/*++

Copyright (c) Microsoft Corporation. All Rights Reserved.

Module Name:

    MsPassthroughExt.c

Abstract:

    This file contains the implementation of a passthrough filter
    utilizing SxBase.lib.


--*/

#include "precomp.h"

UCHAR SxExtMajorNdisVersion = NDIS_FILTER_MAJOR_VERSION;
UCHAR SxExtMinorNdisVersion = NDIS_FILTER_MINOR_VERSION;
PWCHAR SxExtFriendlyName = L"Microsoft Sample Passthrough Extension";
PWCHAR SxExtUniqueName = L"{2A06F1CB-1B9B-43A8-ADF7-B7D44A9EE71C}";
PWCHAR SxExtServiceName = L"MsPassthroughExt";
ULONG SxExtAllocationTag = 'tPsM';
ULONG SxExtOidRequestId = 'tPsM';


NDIS_STATUS
SxExtInitialize()
{
    return NDIS_STATUS_SUCCESS;
}


VOID
SxExtUninitialize()
{
    return;
}


_Use_decl_annotations_
NDIS_STATUS
SxExtCreateSwitch(
    PSX_SWITCH_OBJECT Switch,
    PNDIS_HANDLE *ExtensionContext
    )
{
    UNREFERENCED_PARAMETER(Switch);
    
    *ExtensionContext = NULL;
    return NDIS_STATUS_SUCCESS;
}


_Use_decl_annotations_
VOID
SxExtDeleteSwitch(
    PSX_SWITCH_OBJECT Switch,
    NDIS_HANDLE ExtensionContext
    )
{
    UNREFERENCED_PARAMETER(Switch);
    UNREFERENCED_PARAMETER(ExtensionContext);
    
    return;
}


_Use_decl_annotations_
VOID
SxExtActivateSwitch(
    PSX_SWITCH_OBJECT Switch,
    NDIS_HANDLE ExtensionContext
    )
{
    UNREFERENCED_PARAMETER(Switch);
    UNREFERENCED_PARAMETER(ExtensionContext);
    
    return;
}


_Use_decl_annotations_
NDIS_STATUS
SxExtRestartSwitch(
    PSX_SWITCH_OBJECT Switch,
    NDIS_HANDLE ExtensionContext
    )
{
    UNREFERENCED_PARAMETER(Switch);
    UNREFERENCED_PARAMETER(ExtensionContext);

    return NDIS_STATUS_SUCCESS;
}


_Use_decl_annotations_ 
VOID
SxExtPauseSwitch(
    PSX_SWITCH_OBJECT Switch,
    NDIS_HANDLE ExtensionContext
    )
{
    UNREFERENCED_PARAMETER(Switch);
    UNREFERENCED_PARAMETER(ExtensionContext);
    
    return;
}


_Use_decl_annotations_
NDIS_STATUS
SxExtCreatePort(
    PSX_SWITCH_OBJECT Switch,
    NDIS_HANDLE ExtensionContext,
    PNDIS_SWITCH_PORT_PARAMETERS Port
    )
{
    UNREFERENCED_PARAMETER(Switch);
    UNREFERENCED_PARAMETER(ExtensionContext);
    UNREFERENCED_PARAMETER(Port);
    
    return NDIS_STATUS_SUCCESS;
}


_Use_decl_annotations_
VOID
SxExtUpdatePort(
    PSX_SWITCH_OBJECT Switch,
    NDIS_HANDLE ExtensionContext,
    PNDIS_SWITCH_PORT_PARAMETERS Port
    )
{
    UNREFERENCED_PARAMETER(Switch);
    UNREFERENCED_PARAMETER(ExtensionContext);
    UNREFERENCED_PARAMETER(Port);
    
    return;
}


_Use_decl_annotations_
NDIS_STATUS
SxExtCreateNic(
    PSX_SWITCH_OBJECT Switch,
    NDIS_HANDLE ExtensionContext,
    PNDIS_SWITCH_NIC_PARAMETERS Nic
    )
{
    UNREFERENCED_PARAMETER(Switch);
    UNREFERENCED_PARAMETER(ExtensionContext);
    UNREFERENCED_PARAMETER(Nic);
    
    return NDIS_STATUS_SUCCESS;
}


_Use_decl_annotations_
VOID
SxExtConnectNic(
    PSX_SWITCH_OBJECT Switch,
    NDIS_HANDLE ExtensionContext,
    PNDIS_SWITCH_NIC_PARAMETERS Nic
    )
{
    UNREFERENCED_PARAMETER(Switch);
    UNREFERENCED_PARAMETER(ExtensionContext);
    UNREFERENCED_PARAMETER(Nic);
    
    return;
}


_Use_decl_annotations_
VOID
SxExtUpdateNic(
    PSX_SWITCH_OBJECT Switch,
    NDIS_HANDLE ExtensionContext,
    PNDIS_SWITCH_NIC_PARAMETERS Nic
    )
{
    UNREFERENCED_PARAMETER(Switch);
    UNREFERENCED_PARAMETER(ExtensionContext);
    UNREFERENCED_PARAMETER(Nic);
    
    return;
}


_Use_decl_annotations_
VOID
SxExtDisconnectNic(
    PSX_SWITCH_OBJECT Switch,
    NDIS_HANDLE ExtensionContext,
    PNDIS_SWITCH_NIC_PARAMETERS Nic
    )
{
    UNREFERENCED_PARAMETER(Switch);
    UNREFERENCED_PARAMETER(ExtensionContext);
    UNREFERENCED_PARAMETER(Nic);
    
    return;
}


_Use_decl_annotations_
VOID
SxExtDeleteNic(
    PSX_SWITCH_OBJECT Switch,
    NDIS_HANDLE ExtensionContext,
    PNDIS_SWITCH_NIC_PARAMETERS Nic
    )
{
    UNREFERENCED_PARAMETER(Switch);
    UNREFERENCED_PARAMETER(ExtensionContext);
    UNREFERENCED_PARAMETER(Nic);
    
    return;
}


_Use_decl_annotations_
VOID
SxExtTeardownPort(
    PSX_SWITCH_OBJECT Switch,
    NDIS_HANDLE ExtensionContext,
    PNDIS_SWITCH_PORT_PARAMETERS Port
    )
{
    UNREFERENCED_PARAMETER(Switch);
    UNREFERENCED_PARAMETER(ExtensionContext);
    UNREFERENCED_PARAMETER(Port);
    
    return;
}


_Use_decl_annotations_
VOID
SxExtDeletePort(  
    PSX_SWITCH_OBJECT Switch,
    NDIS_HANDLE ExtensionContext,
    PNDIS_SWITCH_PORT_PARAMETERS Port
    )
{
    UNREFERENCED_PARAMETER(Switch);
    UNREFERENCED_PARAMETER(ExtensionContext);
    UNREFERENCED_PARAMETER(Port);
    
    return;
}


_Use_decl_annotations_
NDIS_STATUS
SxExtSaveNic(  
    PSX_SWITCH_OBJECT Switch,
    NDIS_HANDLE ExtensionContext,
    PNDIS_SWITCH_NIC_SAVE_STATE SaveState,
    PULONG BytesWritten,
    PULONG BytesNeeded
    )
{
    UNREFERENCED_PARAMETER(Switch);
    UNREFERENCED_PARAMETER(ExtensionContext);
    UNREFERENCED_PARAMETER(SaveState);
    
    *BytesWritten = 0;
    *BytesNeeded = 0;
    return NDIS_STATUS_SUCCESS;
}


_Use_decl_annotations_
VOID
SxExtSaveNicComplete(  
    PSX_SWITCH_OBJECT Switch,
    NDIS_HANDLE ExtensionContext,
    PNDIS_SWITCH_NIC_SAVE_STATE SaveState
    )
{
    UNREFERENCED_PARAMETER(Switch);
    UNREFERENCED_PARAMETER(ExtensionContext);
    UNREFERENCED_PARAMETER(SaveState);
    
    return;
}


_Use_decl_annotations_  
NDIS_STATUS
SxExtNicRestore(  
    PSX_SWITCH_OBJECT Switch,
    NDIS_HANDLE ExtensionContext,
    PNDIS_SWITCH_NIC_SAVE_STATE SaveState,
    PULONG BytesRestored
    )
{
    UNREFERENCED_PARAMETER(Switch);
    UNREFERENCED_PARAMETER(ExtensionContext);
    UNREFERENCED_PARAMETER(SaveState);
    
    *BytesRestored = 0;
    return NDIS_STATUS_SUCCESS;
}


_Use_decl_annotations_
VOID
SxExtNicRestoreComplete(  
    PSX_SWITCH_OBJECT Switch,
    NDIS_HANDLE ExtensionContext,
    PNDIS_SWITCH_NIC_SAVE_STATE SaveState
    )
{
    UNREFERENCED_PARAMETER(Switch);
    UNREFERENCED_PARAMETER(ExtensionContext);
    UNREFERENCED_PARAMETER(SaveState);
    
    return;
}


_Use_decl_annotations_
NDIS_STATUS
SxExtAddSwitchProperty(
    PSX_SWITCH_OBJECT Switch,
    NDIS_HANDLE ExtensionContext,
    PNDIS_SWITCH_PROPERTY_PARAMETERS SwitchProperty
    )
{
    UNREFERENCED_PARAMETER(Switch);
    UNREFERENCED_PARAMETER(ExtensionContext);
    UNREFERENCED_PARAMETER(SwitchProperty);
    
    return NDIS_STATUS_NOT_SUPPORTED;
}


_Use_decl_annotations_
NDIS_STATUS
SxExtUpdateSwitchProperty(
    PSX_SWITCH_OBJECT Switch,
    NDIS_HANDLE ExtensionContext,
    PNDIS_SWITCH_PROPERTY_PARAMETERS SwitchProperty
    )
{
    UNREFERENCED_PARAMETER(Switch);
    UNREFERENCED_PARAMETER(ExtensionContext);
    UNREFERENCED_PARAMETER(SwitchProperty);
    
    return NDIS_STATUS_NOT_SUPPORTED;
}


_Use_decl_annotations_
BOOLEAN
SxExtDeleteSwitchProperty(
    PSX_SWITCH_OBJECT Switch,
    NDIS_HANDLE ExtensionContext,
    PNDIS_SWITCH_PROPERTY_DELETE_PARAMETERS SwitchProperty
    )
{
    UNREFERENCED_PARAMETER(Switch);
    UNREFERENCED_PARAMETER(ExtensionContext);
    UNREFERENCED_PARAMETER(SwitchProperty);
    
    return FALSE;
}


_Use_decl_annotations_
NDIS_STATUS
SxExtAddPortProperty(
    PSX_SWITCH_OBJECT Switch,
    NDIS_HANDLE ExtensionContext,
    PNDIS_SWITCH_PORT_PROPERTY_PARAMETERS PortProperty
    )
{
    UNREFERENCED_PARAMETER(Switch);
    UNREFERENCED_PARAMETER(ExtensionContext);
    UNREFERENCED_PARAMETER(PortProperty);
    
    return NDIS_STATUS_NOT_SUPPORTED;
}


_Use_decl_annotations_
NDIS_STATUS
SxExtUpdatePortProperty(
    PSX_SWITCH_OBJECT Switch,
    NDIS_HANDLE ExtensionContext,
    PNDIS_SWITCH_PORT_PROPERTY_PARAMETERS PortProperty
    )
{
    UNREFERENCED_PARAMETER(Switch);
    UNREFERENCED_PARAMETER(ExtensionContext);
    UNREFERENCED_PARAMETER(PortProperty);
    
    return NDIS_STATUS_NOT_SUPPORTED;
}


_Use_decl_annotations_
BOOLEAN
SxExtDeletePortProperty(
    PSX_SWITCH_OBJECT Switch,
    NDIS_HANDLE ExtensionContext,
    PNDIS_SWITCH_PORT_PROPERTY_DELETE_PARAMETERS PortProperty
    )
{
    UNREFERENCED_PARAMETER(Switch);
    UNREFERENCED_PARAMETER(ExtensionContext);
    UNREFERENCED_PARAMETER(PortProperty);
    
    return FALSE;
}


_Use_decl_annotations_
BOOLEAN
SxExtQuerySwitchFeatureStatus(
    PSX_SWITCH_OBJECT Switch,
    NDIS_HANDLE ExtensionContext,
    PNDIS_SWITCH_FEATURE_STATUS_PARAMETERS SwitchFeatureStatus,
    PULONG BytesNeeded
    )
{
    UNREFERENCED_PARAMETER(Switch);
    UNREFERENCED_PARAMETER(ExtensionContext);
    UNREFERENCED_PARAMETER(SwitchFeatureStatus);
    UNREFERENCED_PARAMETER(BytesNeeded);
    
    return FALSE;
}


_Use_decl_annotations_
BOOLEAN
SxExtQueryPortFeatureStatus(
    PSX_SWITCH_OBJECT Switch,
    NDIS_HANDLE ExtensionContext,
    PNDIS_SWITCH_PORT_FEATURE_STATUS_PARAMETERS PortFeatureStatus,
    PULONG BytesNeeded
    )
{
    UNREFERENCED_PARAMETER(Switch);
    UNREFERENCED_PARAMETER(ExtensionContext);
    UNREFERENCED_PARAMETER(PortFeatureStatus);
    UNREFERENCED_PARAMETER(BytesNeeded);
    
    return FALSE;
}
    

_Use_decl_annotations_
NDIS_STATUS
SxExtProcessNicRequest(
    PSX_SWITCH_OBJECT Switch,
    NDIS_HANDLE ExtensionContext,
    PNDIS_OID_REQUEST OidRequest,
    PNDIS_SWITCH_PORT_ID SourcePortId,
    PNDIS_SWITCH_NIC_INDEX SourceNicIndex,
    PNDIS_SWITCH_PORT_ID DestinationPortId,
    PNDIS_SWITCH_NIC_INDEX DestinationNicIndex
    )
{
    UNREFERENCED_PARAMETER(Switch);
    UNREFERENCED_PARAMETER(ExtensionContext);
    UNREFERENCED_PARAMETER(OidRequest);
    UNREFERENCED_PARAMETER(SourcePortId);
    UNREFERENCED_PARAMETER(SourceNicIndex);
    UNREFERENCED_PARAMETER(DestinationPortId);
    UNREFERENCED_PARAMETER(DestinationNicIndex);
    
    return NDIS_STATUS_SUCCESS;
}


_Use_decl_annotations_
NDIS_STATUS
SxExtProcessNicRequestComplete(
    PSX_SWITCH_OBJECT Switch,
    NDIS_HANDLE ExtensionContext,
    PNDIS_OID_REQUEST OidRequest,
    NDIS_SWITCH_PORT_ID SourcePortId,
    NDIS_SWITCH_NIC_INDEX SourceNicIndex,
    NDIS_SWITCH_PORT_ID DestinationPortId,
    NDIS_SWITCH_NIC_INDEX DestinationNicIndex,
    NDIS_STATUS Status
    )
{
    UNREFERENCED_PARAMETER(Switch);
    UNREFERENCED_PARAMETER(ExtensionContext);
    UNREFERENCED_PARAMETER(OidRequest);
    UNREFERENCED_PARAMETER(SourcePortId);
    UNREFERENCED_PARAMETER(SourceNicIndex);
    UNREFERENCED_PARAMETER(DestinationPortId);
    UNREFERENCED_PARAMETER(DestinationNicIndex);
    UNREFERENCED_PARAMETER(Status);
    
    return NDIS_STATUS_SUCCESS;
}
    

_Use_decl_annotations_
NDIS_STATUS
SxExtProcessNicStatus(
    PSX_SWITCH_OBJECT Switch,
    NDIS_HANDLE ExtensionContext,
    PNDIS_STATUS_INDICATION StatusIndication,
    NDIS_SWITCH_PORT_ID SourcePortId,
    NDIS_SWITCH_NIC_INDEX SourceNicIndex
    )
{
    UNREFERENCED_PARAMETER(Switch);
    UNREFERENCED_PARAMETER(ExtensionContext);
    UNREFERENCED_PARAMETER(StatusIndication);
    UNREFERENCED_PARAMETER(SourcePortId);
    UNREFERENCED_PARAMETER(SourceNicIndex);
    
    return NDIS_STATUS_SUCCESS;
}


_Use_decl_annotations_
VOID
SxExtStartNetBufferListsIngress(
    PSX_SWITCH_OBJECT Switch,
    NDIS_HANDLE ExtensionContext,
    PNET_BUFFER_LIST NetBufferLists,
    ULONG SendFlags
    )
{
    UNREFERENCED_PARAMETER(ExtensionContext);
    
    SxLibSendNetBufferListsIngress(Switch,
                                   NetBufferLists,
                                   SendFlags,
                                   0);
}


_Use_decl_annotations_ 
VOID
SxExtStartNetBufferListsEgress(
    PSX_SWITCH_OBJECT Switch,
    NDIS_HANDLE ExtensionContext,
    PNET_BUFFER_LIST NetBufferLists,
    ULONG NumberOfNetBufferLists,
    ULONG ReceiveFlags
    )
{
    UNREFERENCED_PARAMETER(ExtensionContext);
    
    SxLibSendNetBufferListsEgress(Switch,
                                  NetBufferLists,
                                  NumberOfNetBufferLists,
                                  ReceiveFlags);
}


_Use_decl_annotations_
VOID
SxExtStartCompleteNetBufferListsEgress(
    PSX_SWITCH_OBJECT Switch,
    NDIS_HANDLE ExtensionContext,
    PNET_BUFFER_LIST NetBufferLists,
    ULONG ReturnFlags
    )
{
    UNREFERENCED_PARAMETER(ExtensionContext);
    
    SxLibCompleteNetBufferListsEgress(Switch,
                                      NetBufferLists,
                                      ReturnFlags);
}


_Use_decl_annotations_
VOID
SxExtStartCompleteNetBufferListsIngress(
    PSX_SWITCH_OBJECT Switch,
    NDIS_HANDLE ExtensionContext,
    PNET_BUFFER_LIST NetBufferLists,
    ULONG SendCompleteFlags
    )
{
    UNREFERENCED_PARAMETER(ExtensionContext);
    
    SxLibCompleteNetBufferListsIngress(Switch,
                                       NetBufferLists,
                                       SendCompleteFlags);
}
    
