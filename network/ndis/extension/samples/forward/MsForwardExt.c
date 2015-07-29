/*++

Copyright (c) Microsoft Corporation. All Rights Reserved.

Module Name:

   MsForwardExt.c

Abstract:

    This file contains the implementation of a forwarding extension
    (MSForardExt) using the SxBase library.
    The forwarding extension does basic MAC forwarding, blocking and allowing
    source MAC addresses based on custom set policy.


--*/

#include "precomp.h"
#include "MsForwardExt.h"

UCHAR SxExtMajorNdisVersion = NDIS_FILTER_MAJOR_VERSION;
UCHAR SxExtMinorNdisVersion = NDIS_FILTER_MINOR_VERSION;
PWCHAR SxExtFriendlyName = L"Microsoft Sample Forwarding Extension";
PWCHAR SxExtUniqueName = L"{37d9eae6-5bae-48c5-bff1-63a7cdd7e4f4}";
PWCHAR SxExtServiceName = L"MSForwardExt";
ULONG SxExtAllocationTag = 'wSxS';
ULONG SxExtOidRequestId = 'wSxS';

// {37d9eae6-5bae-48c5-bff1-63a7cdd7e4f4}
const GUID MsForwardExtGuid = {
    0x37d9eae6,
    0x5bae,
    0x48c5,
    {0xbf, 0xf1, 0x63, 0xa7, 0xcd, 0xd7, 0xe4, 0xf4}
    };

// {EB29F0F2-F5DC-45C6-81BB-3CD9F219BBBB}
const NDIS_SWITCH_OBJECT_ID MacAddressPolicyGuid = {
    0xEB29F0F2,
    0xF5DC,
    0x45C6,
    {0x81, 0xBB, 0x3C, 0xD9, 0xF2, 0x19, 0xBB, 0xBB}
    };

// {A3E2AFF5-E6FA-4E52-AB74-13250BF7E8CF}
const NDIS_SWITCH_OBJECT_ID MacAddressPolicyStatusGuid = {
    0xA3E2AFF5,
    0xE6FA,
    0x4E52,
    {0xAB, 0x74, 0x13, 0x25, 0x0B, 0xF7, 0xE8, 0xCF}
    };
    
    
NDIS_STATUS
SxExtInitialize()
/*++
  
Routine Description:
    No global information needed.
   
--*/
{
    return NDIS_STATUS_SUCCESS;
}


VOID
SxExtUninitialize()
/*++
  
Routine Description:
    No global information needed.
   
--*/
{
    return;
}


_Use_decl_annotations_
NDIS_STATUS
SxExtCreateSwitch(
    PSX_SWITCH_OBJECT Switch,
    PNDIS_HANDLE *ExtensionContext
    )
/*++
  
Routine Description:
    This function allocated the switch context, and initializes
    its necessary members.
   
--*/
{
    NDIS_STATUS status = NDIS_STATUS_SUCCESS;
    PMSFORWARD_CONTEXT switchContext;
        
    switchContext = ExAllocatePoolWithTag(NonPagedPoolNx,
                                          sizeof(MSFORWARD_CONTEXT),
                                          SxExtAllocationTag);
                                          
    if (switchContext == NULL)
    {
        status = NDIS_STATUS_RESOURCES;
        goto Cleanup;
    }
    
    NdisZeroMemory(switchContext, sizeof(MSFORWARD_CONTEXT));
    InitializeListHead(&switchContext->NicList);
    InitializeListHead(&switchContext->PropertyList);
    
    switchContext->DispatchLock = NdisAllocateRWLock(Switch->NdisFilterHandle);
    if (switchContext->DispatchLock == NULL)
    {
        status = NDIS_STATUS_RESOURCES;
        goto Cleanup;
    }
    
    switchContext->IsInitialRestart = TRUE;
    
    *ExtensionContext = (NDIS_HANDLE)switchContext;
    
Cleanup:
    if (status != NDIS_STATUS_SUCCESS)
    {
        if (switchContext != NULL)
        {
            ExFreePoolWithTag(switchContext, SxExtAllocationTag);
        }
    }

    return status;
}


_Use_decl_annotations_
VOID
SxExtDeleteSwitch(
    PSX_SWITCH_OBJECT Switch,
    NDIS_HANDLE ExtensionContext
    )
/*++
  
Routine Description:
    This function deletes the switch by freeing all memory
    previously allocated.
    
--*/
{
    PMSFORWARD_CONTEXT switchContext = (PMSFORWARD_CONTEXT)ExtensionContext;
    
    UNREFERENCED_PARAMETER(Switch);
    
    MsForwardClearNicListUnsafe(switchContext);
    MsForwardClearPropertyListUnsafe(switchContext);
    NdisFreeRWLock(switchContext->DispatchLock);
    ExFreePoolWithTag(ExtensionContext, SxExtAllocationTag);
}


_Use_decl_annotations_
VOID
SxExtActivateSwitch(
    PSX_SWITCH_OBJECT Switch,
    NDIS_HANDLE ExtensionContext
    )
{
    PMSFORWARD_CONTEXT switchContext = (PMSFORWARD_CONTEXT)ExtensionContext;
    MsForwardInitSwitch(Switch, switchContext);
    
    return;
}


_Use_decl_annotations_
NDIS_STATUS
SxExtRestartSwitch(
    PSX_SWITCH_OBJECT Switch,
    NDIS_HANDLE ExtensionContext
    )
/*++
  
Routine Description:
    This function initializes the switch if it is the first restart.
    First it queries all of the MAC addresses set as custom
    switch policy to allow sends from, and adds tme to the property
    list.
    Then it queries the NIC list and verifies it can support
    all of the NICs currently connected to the switch, and
    adds the NICs to the NIC list.
    
--*/
{
    NDIS_STATUS status = NDIS_STATUS_SUCCESS;
    PMSFORWARD_CONTEXT switchContext = (PMSFORWARD_CONTEXT)ExtensionContext;
    NDIS_SWITCH_PARAMETERS switchParameters;

    if (switchContext->IsInitialRestart)
    {
        status = SxLibGetSwitchParametersUnsafe(Switch, &switchParameters);
        
        if (status != NDIS_STATUS_SUCCESS)
        {
            goto Cleanup;
        }
        
        if (switchParameters.IsActive)
        {
            status = MsForwardInitSwitch(Switch, switchContext);
            if (status != NDIS_STATUS_SUCCESS)
            {
                goto Cleanup;
            }
        }
        
        switchContext->IsInitialRestart = FALSE;
    }
    
Cleanup:   
    return status;
}


_Use_decl_annotations_
VOID
SxExtPauseSwitch(
    PSX_SWITCH_OBJECT Switch,
    NDIS_HANDLE ExtensionContext
    )
/*++
  
Routine Description:
    No pause funtionality required.
    
--*/
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
/*++
  
Routine Description:
    This extension does not track ports, only NICs.
    
--*/
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
/*++
  
Routine Description:
    This extension does not track ports, only NICs.
    
--*/
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
/*++
  
Routine Description:
    Allocate NIC, add to NIC list, and correlate with policy.
    
--*/
{
    PMSFORWARD_CONTEXT switchContext = (PMSFORWARD_CONTEXT)ExtensionContext;
    NDIS_STATUS status = NDIS_STATUS_SUCCESS;
    LOCK_STATE_EX lockState;
    
    UNREFERENCED_PARAMETER(Switch);
    
    //
    // Wait for lists to be initialized.
    //
    while (switchContext->IsInitialRestart)
    {
        NdisMSleep(100);
    }
        
    NdisAcquireRWLockWrite(switchContext->DispatchLock, &lockState, 0);
    
    status = MsForwardAddNicUnsafe(switchContext,
                                   Nic->PermanentMacAddress,
                                   Nic->PortId,
                                   Nic->NicIndex,
                                   Nic->NicType,
                                   FALSE);

                                      
    NdisReleaseRWLock(switchContext->DispatchLock, &lockState);
    
    return status;
}


_Use_decl_annotations_
VOID
SxExtConnectNic(
    PSX_SWITCH_OBJECT Switch,
    NDIS_HANDLE ExtensionContext,
    PNDIS_SWITCH_NIC_PARAMETERS Nic
    )
/*++
  
Routine Description:
    Mark already created NIC as connected.
    
--*/
{
    PMSFORWARD_CONTEXT switchContext = (PMSFORWARD_CONTEXT)ExtensionContext;
    PMSFORWARD_NIC_LIST_ENTRY nicEntry = NULL;
    LOCK_STATE_EX lockState;
    
    UNREFERENCED_PARAMETER(Switch);
    
    //
    // Wait for lists to be initialized.
    //
    while (switchContext->IsInitialRestart)
    {
        NdisMSleep(100);
    }
        
    NdisAcquireRWLockWrite(switchContext->DispatchLock, &lockState, 0);
    if (Nic->NicType == NdisSwitchNicTypeExternal &&
        Nic->NicIndex != 0 &&
        switchContext->ExternalPortId == 0)
    {
        switchContext->ExternalPortId = Nic->PortId;
        switchContext->ExternalNicIndex = Nic->NicIndex;
        switchContext->ExternalNicConnected = TRUE;
        ++(switchContext->NumDestinations);
    }
    else if (Nic->NicType != NdisSwitchNicTypeExternal)
    {
        nicEntry = MsForwardFindNicByPortIdUnsafe(switchContext,
                                                  Nic->PortId,
                                                  Nic->NicIndex);
        
        if(nicEntry != NULL)
        {
            nicEntry->Connected = TRUE;
            ++(switchContext->NumDestinations);
        }
        else
        {
            ASSERT(FALSE);
        }
    }
    NdisReleaseRWLock(switchContext->DispatchLock, &lockState);
}


_Use_decl_annotations_
VOID
SxExtUpdateNic(
    PSX_SWITCH_OBJECT Switch,
    NDIS_HANDLE ExtensionContext,
    PNDIS_SWITCH_NIC_PARAMETERS Nic
    )
/*++
  
Routine Description:
    This extension doesn't use any of the fields that can be updated.
    
--*/
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
/*++
  
Routine Description:
    Mark already created NIC as disconnected.
    
--*/
{
    PMSFORWARD_CONTEXT switchContext = (PMSFORWARD_CONTEXT)ExtensionContext;
    PMSFORWARD_NIC_LIST_ENTRY nicEntry = NULL;
    LOCK_STATE_EX lockState;
    
    UNREFERENCED_PARAMETER(Switch);
    
    //
    // Wait for lists to be initialized.
    //
    while (switchContext->IsInitialRestart)
    {
        NdisMSleep(100);
    }
    
    NdisAcquireRWLockWrite(switchContext->DispatchLock, &lockState, 0);
    if (Nic->NicType == NdisSwitchNicTypeExternal)
    {
        if (Nic->NicIndex == switchContext->ExternalNicIndex)
        {           
            --(switchContext->NumDestinations);
            switchContext->ExternalNicConnected = FALSE;
        }
    }
    else
    {
        nicEntry = MsForwardFindNicByPortIdUnsafe(switchContext,
                                                  Nic->PortId,
                                                  Nic->NicIndex);
        
        if(nicEntry != NULL)
        {
            nicEntry->Connected = FALSE;
            --(switchContext->NumDestinations);
        }
        else
        {
            ASSERT(FALSE);
        }
    }

    NdisReleaseRWLock(switchContext->DispatchLock, &lockState);
}

    
_Use_decl_annotations_
VOID
SxExtDeleteNic(
    PSX_SWITCH_OBJECT Switch,
    NDIS_HANDLE ExtensionContext,
    PNDIS_SWITCH_NIC_PARAMETERS Nic
    )
/*++
  
Routine Description:
    Delete created NIC, free related memory and remove
    from NIC list.
    
--*/
{
    PMSFORWARD_CONTEXT switchContext = (PMSFORWARD_CONTEXT)ExtensionContext;
    LOCK_STATE_EX lockState;
    
    UNREFERENCED_PARAMETER(Switch);
    
    //
    // Wait for lists to be initialized.
    //
    while (switchContext->IsInitialRestart)
    {
        NdisMSleep(100);
    }
        
    NdisAcquireRWLockWrite(switchContext->DispatchLock, &lockState, 0);
    if (Nic->NicType == NdisSwitchNicTypeExternal &&
        Nic->NicIndex == switchContext->ExternalNicIndex)
    {
        ASSERT(switchContext->ExternalNicConnected == FALSE);
        switchContext->ExternalPortId = 0;
        switchContext->ExternalNicIndex = 0;
    }
    else if (Nic->NicType != NdisSwitchNicTypeExternal)
    {
       MsForwardDeleteNicUnsafe(switchContext,
                                Nic->PortId,
                                Nic->NicIndex);
    }

    NdisReleaseRWLock(switchContext->DispatchLock, &lockState);
    return;
}


_Use_decl_annotations_
VOID
SxExtTeardownPort(
    PSX_SWITCH_OBJECT Switch,
    NDIS_HANDLE ExtensionContext,
    PNDIS_SWITCH_PORT_PARAMETERS Port
    )
/*++
  
Routine Description:
    This extension does not track port state.
    
--*/
{
    UNREFERENCED_PARAMETER(Switch);
    UNREFERENCED_PARAMETER(ExtensionContext);
    UNREFERENCED_PARAMETER(Port);
}


_Use_decl_annotations_
VOID
SxExtDeletePort(  
    PSX_SWITCH_OBJECT Switch,
    NDIS_HANDLE ExtensionContext,
    PNDIS_SWITCH_PORT_PARAMETERS Port
    )
/*++
  
Routine Description:
    This extension does not track port state.
    
--*/
{
    UNREFERENCED_PARAMETER(Switch);
    UNREFERENCED_PARAMETER(ExtensionContext);
    UNREFERENCED_PARAMETER(Port);
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
/*++
  
Routine Description:
    This extension does not save any data.
    
--*/
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
/*++
  
Routine Description:
    This extension does not save any data.
    
--*/
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
/*++
  
Routine Description:
    This extension does not save any data.
    
--*/
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
/*++
  
Routine Description:
    This extension does not save any data.
    
--*/
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
/*++
  
Routine Description:
    This extension enforces one custom switch policy.
    The function verifies the switch property is our MAC policy
    and then adds it to the property list.
    
--*/
{
    NDIS_STATUS status = NDIS_STATUS_NOT_SUPPORTED;
    PNDIS_SWITCH_PROPERTY_CUSTOM customPolicy;
    PMSFORWARD_MAC_ADDRESS_POLICY macPolicy;
    PMSFORWARD_CONTEXT switchContext = (PMSFORWARD_CONTEXT)ExtensionContext;
    LOCK_STATE_EX lockState;
    
    UNREFERENCED_PARAMETER(Switch);
    
    if (SwitchProperty->PropertyType != NdisSwitchPropertyTypeCustom)
    {
        goto Cleanup;
    }
    
    //
    // Check if MAC Address Policy for this extension.
    //
    if (!RtlEqualMemory(&SwitchProperty->PropertyId,
                        &MacAddressPolicyGuid,
                        sizeof(NDIS_SWITCH_OBJECT_ID)))
    {
        goto Cleanup;
    }
    
    if (SwitchProperty->PropertyVersion != MAC_ADDRESS_POLICY_VERSION)
    {
        goto Cleanup;
    }
    
    if (SwitchProperty->SerializationVersion != MAC_ADDRESS_POLICY_SERIALIZATION_VERSION)
    {
        goto Cleanup;
    }
    
    //
    // Wait for lists to be initialized.
    //
    while (switchContext->IsInitialRestart)
    {
        NdisMSleep(100);
    }
    
    customPolicy = NDIS_SWITCH_PROPERTY_PARAMETERS_GET_PROPERTY(SwitchProperty);
    macPolicy = NDIS_SWITCH_PROPERTY_CUSTOM_GET_BUFFER(customPolicy);
    
    if (macPolicy->MacAddressLength != 6)
    {
        status = NDIS_STATUS_DATA_NOT_ACCEPTED;
    } 
    else
    {
        NdisAcquireRWLockWrite(switchContext->DispatchLock, &lockState, 0);
        
        status = MsForwardAddMacPolicyUnsafe(switchContext,
                                             macPolicy,
                                             &SwitchProperty->PropertyInstanceId);
        
        NdisReleaseRWLock(switchContext->DispatchLock, &lockState);
    }
    
Cleanup:
    return status;
}


_Use_decl_annotations_    
NDIS_STATUS
SxExtUpdateSwitchProperty(
    PSX_SWITCH_OBJECT Switch,
    NDIS_HANDLE ExtensionContext,
    PNDIS_SWITCH_PROPERTY_PARAMETERS SwitchProperty
    )
/*++
  
Routine Description:
    This extension enforces one custom switch policy, but
    does not allow updates for that policy.
    
--*/
{
    NDIS_STATUS status = NDIS_STATUS_NOT_SUPPORTED;
    
    UNREFERENCED_PARAMETER(Switch);
    UNREFERENCED_PARAMETER(ExtensionContext);

    if (SwitchProperty->PropertyType != NdisSwitchPropertyTypeCustom)
    {
        goto Cleanup;
    }
    
    //
    // Check if MAC Address Policy for this extension.
    //
    if (!RtlEqualMemory(&SwitchProperty->PropertyId,
                        &MacAddressPolicyGuid,
                        sizeof(NDIS_SWITCH_OBJECT_ID)))
    {
        goto Cleanup;
    }
    
    if (SwitchProperty->PropertyVersion != MAC_ADDRESS_POLICY_VERSION)
    {
        goto Cleanup;
    }
    
    if (SwitchProperty->SerializationVersion != MAC_ADDRESS_POLICY_SERIALIZATION_VERSION)
    {
        goto Cleanup;
    }

    status = NDIS_STATUS_DATA_NOT_ACCEPTED;
    
Cleanup:
    return status;
}


_Use_decl_annotations_
BOOLEAN
SxExtDeleteSwitchProperty(
    PSX_SWITCH_OBJECT Switch,
    NDIS_HANDLE ExtensionContext,
    PNDIS_SWITCH_PROPERTY_DELETE_PARAMETERS SwitchProperty
    )
/*++
  
Routine Description:
    This extension enforces one custom switch policy.
    The function verifies the switch property is our MAC policy
    and then deletes it from the property list.
    
--*/
{
    BOOLEAN delete = FALSE;
    PMSFORWARD_CONTEXT switchContext = (PMSFORWARD_CONTEXT)ExtensionContext;
    LOCK_STATE_EX lockState;
    
    UNREFERENCED_PARAMETER(Switch);
    
    if (SwitchProperty->PropertyType != NdisSwitchPropertyTypeCustom)
    {
        goto Cleanup;
    }
    
    //
    // Check if MAC Address Policy for this extension.
    //
    if (!RtlEqualMemory(&SwitchProperty->PropertyId,
                        &MacAddressPolicyGuid,
                        sizeof(NDIS_SWITCH_OBJECT_ID)))
    {
        goto Cleanup;
    }
    
    //
    // Wait for lists to be initialized.
    //
    while (switchContext->IsInitialRestart)
    {
        NdisMSleep(100);
    }
    
    
    delete = TRUE;

    NdisAcquireRWLockWrite(switchContext->DispatchLock, &lockState, 0);
    
    MsForwardDeleteMacPolicyUnsafe(switchContext,
                                   &SwitchProperty->PropertyInstanceId);
    
    NdisReleaseRWLock(switchContext->DispatchLock, &lockState);
    
Cleanup:
    return delete;
}


_Use_decl_annotations_    
NDIS_STATUS
SxExtAddPortProperty(
    PSX_SWITCH_OBJECT Switch,
    NDIS_HANDLE ExtensionContext,
    PNDIS_SWITCH_PORT_PROPERTY_PARAMETERS PortProperty
    )
/*++
  
Routine Description:
    This extension does not enforce VLAN. Because of this
    the extension fails the adding of these policies.
    
    !! REAL FORWARDING EXTENSIONS SHOULD SUPPORT THESE PROPERTIES !!
    
--*/
{
    NDIS_STATUS status = NDIS_STATUS_NOT_SUPPORTED;
    
    UNREFERENCED_PARAMETER(Switch);
    UNREFERENCED_PARAMETER(ExtensionContext);
    
    switch(PortProperty->PropertyType)
    {
        case NdisSwitchPortPropertyTypeCustom:
            //
            // No Custom Port Properties.
            //
            break;
            
        case NdisSwitchPortPropertyTypeSecurity:
            //
            // This extension does need to look at security policy, pass it down.
            // An extension must always pass through Hyper-V security policy.
            //
            break;
            
        case NdisSwitchPortPropertyTypeVlan:
            //
            // Forwarding extensions must either enforce VLAN, or fail
            // setting VLAN policy.
            // This extension does not enforce VLAN.
            //
            status = NDIS_STATUS_DATA_NOT_ACCEPTED;
            break;
            
        case NdisSwitchPortPropertyTypeProfile:
            //
            // No Processing of Port Profile.
            //
            break;
    }

    return status;
}

_Use_decl_annotations_  
NDIS_STATUS
SxExtUpdatePortProperty(
    PSX_SWITCH_OBJECT Switch,
    NDIS_HANDLE ExtensionContext,
    PNDIS_SWITCH_PORT_PROPERTY_PARAMETERS PortProperty
    )
/*++
  
Routine Description:
    This extension does not enforce VLAN. Because of this
    the extension fails the updating of these policies.
    
    !! REAL FORWARDING EXTENSIONS SHOULD SUPPORT THESE PROPERTIES !!
    
--*/
{
    NDIS_STATUS status = NDIS_STATUS_NOT_SUPPORTED;
    
    UNREFERENCED_PARAMETER(Switch);
    UNREFERENCED_PARAMETER(ExtensionContext);
    
    switch(PortProperty->PropertyType)
    {
        case NdisSwitchPortPropertyTypeCustom:
            //
            // No Custom Port Properties.
            //
            break;
            
        case NdisSwitchPortPropertyTypeSecurity:
            //
            // This extension does need to look at security policy, pass it down.
            // An extension must always pass through Hyper-V security policy.
            //
            break;
            
        case NdisSwitchPortPropertyTypeVlan:
            //
            // Forwarding extensions must either enforce VLAN, or fail
            // setting VLAN policy.
            // This extension does not enforce VLAN.
            //
            status = NDIS_STATUS_DATA_NOT_ACCEPTED;
            break;
            
        case NdisSwitchPortPropertyTypeProfile:
            //
            // No Processing of Port Profile.
            //
            break;
    }

    return status;
}

_Use_decl_annotations_
BOOLEAN
SxExtDeletePortProperty(
    PSX_SWITCH_OBJECT Switch,
    NDIS_HANDLE ExtensionContext,
    PNDIS_SWITCH_PORT_PROPERTY_DELETE_PARAMETERS PortProperty
    )
/*++
  
Routine Description:
    This extension does not enforce VLAN.
    These policies are policies that should be supported by this
    extension, so it returns TRUE to complete the deletion.
    
    !! REAL FORWARDING EXTENSIONS SHOULD SUPPORT THESE PROPERTIES !!
    
--*/
{
    BOOLEAN delete = FALSE;
    
    UNREFERENCED_PARAMETER(Switch);
    UNREFERENCED_PARAMETER(ExtensionContext);
    
    switch(PortProperty->PropertyType)
    {
        case NdisSwitchPortPropertyTypeCustom:
            //
            // No Custom Port Properties.
            //
            break;
            
        case NdisSwitchPortPropertyTypeSecurity:
            //
            // This extension does need to look at security policy, pass it down.
            // An extension must always pass through Hyper-V security policy.
            //
            break;
            
        case NdisSwitchPortPropertyTypeVlan:
            //
            // Forwarding extensions must either enforce VLAN, or fail
            // setting VLAN policy.
            // This extension does not enforce VLAN.
            //
            delete = TRUE;
            break;
            
        case NdisSwitchPortPropertyTypeProfile:
            //
            // No Processing of Port Profile.
            //
            break;
    }

    return delete;
}


_Use_decl_annotations_
BOOLEAN
SxExtQuerySwitchFeatureStatus(
    PSX_SWITCH_OBJECT Switch,
    NDIS_HANDLE ExtensionContext,
    PNDIS_SWITCH_FEATURE_STATUS_PARAMETERS SwitchFeatureStatus,
    PULONG BytesNeeded
    )
/*++
  
Routine Description:
    This extension reports the status of its custom MAC policy
    by returning the list of PortId's currently allowing sends.
    
--*/
{
    BOOLEAN consumed = FALSE;
    PMSFORWARD_CONTEXT switchContext = (PMSFORWARD_CONTEXT)ExtensionContext;
    LOCK_STATE_EX lockState;
    BOOLEAN lockHeld = FALSE;
    ULONG sizeNeeded = 0;
    ULONG numAllowedSourcePorts = 0;
    PLIST_ENTRY nicList = &switchContext->NicList;
    PLIST_ENTRY curEntry = NULL;
    PMSFORWARD_NIC_LIST_ENTRY nic = NULL;
    PNDIS_SWITCH_FEATURE_STATUS_CUSTOM customStatusBuffer;
    PMSFORWARD_MAC_ADDRESS_POLICY_STATUS macAddressPolicyBuffer;
    PNDIS_SWITCH_PORT_ID portIdArray;
    ULONG arrayIndex = 0;
    ULONG customBufferLength = 0;
                       
    UNREFERENCED_PARAMETER(Switch);
        
    if (SwitchFeatureStatus->FeatureStatusType != NdisSwitchFeatureStatusTypeCustom)
    {
        goto Cleanup;
    }
    
    if (!RtlEqualMemory(&SwitchFeatureStatus->FeatureStatusId,
                        &MacAddressPolicyStatusGuid,
                        sizeof(NDIS_SWITCH_OBJECT_ID)))
    {
        goto Cleanup;
    }
    
    if (SwitchFeatureStatus->SerializationVersion != MAC_ADDRESS_POLICY_STATUS_SERIALIZATION_VERSION)
    {
        goto Cleanup;
    }
    
    //
    // Wait for lists to be initialized.
    //
    while (switchContext->IsInitialRestart)
    {
        NdisMSleep(100);
    }
    
    NdisAcquireRWLockRead(switchContext->DispatchLock, &lockState, 0);
    lockHeld = TRUE;
    
    if (switchContext->ExternalPortId != NDIS_SWITCH_DEFAULT_PORT_ID)
    {
        ++numAllowedSourcePorts;
    }
    
    if (!IsListEmpty(nicList))
    {
        curEntry = nicList->Flink;
        
        do {
            nic = CONTAINING_RECORD(curEntry,
                                    MSFORWARD_NIC_LIST_ENTRY,
                                    ListEntry);
                                    
            if (nic->AllowSends)
            {
                ++numAllowedSourcePorts;
            }

            curEntry = curEntry->Flink;
        
        } while(curEntry != nicList);
    }
    
    customBufferLength = sizeof(MSFORWARD_MAC_ADDRESS_POLICY_STATUS) +
                         (sizeof(NDIS_SWITCH_PORT_ID) * numAllowedSourcePorts);
    sizeNeeded = NDIS_SIZEOF_NDIS_SWITCH_FEATURE_STATUS_PARAMETERS_REVISION_1 +
                 NDIS_SIZEOF_NDIS_SWITCH_FEATURE_STATUS_CUSTOM_REVISION_1 +
                 customBufferLength;
                   
    consumed = TRUE;
    
    if (SwitchFeatureStatus->FeatureStatusBufferLength < sizeof(NDIS_SWITCH_FEATURE_STATUS_CUSTOM))
    {
        *BytesNeeded = sizeNeeded;
        goto Cleanup;
    }
    
    customStatusBuffer = (PNDIS_SWITCH_FEATURE_STATUS_CUSTOM)
                         (((PUINT8)SwitchFeatureStatus) +
                         SwitchFeatureStatus->FeatureStatusBufferOffset);
                         
    if (customStatusBuffer->Header.Type != NDIS_OBJECT_TYPE_DEFAULT ||
        customStatusBuffer->Header.Revision != NDIS_SWITCH_FEATURE_STATUS_CUSTOM_REVISION_1 ||
        customStatusBuffer->Header.Size != NDIS_SIZEOF_NDIS_SWITCH_FEATURE_STATUS_CUSTOM_REVISION_1)
    {
        consumed = FALSE;
        goto Cleanup;
    }
    
    
    
    if (customStatusBuffer->FeatureStatusCustomBufferLength < customBufferLength)
    {
        *BytesNeeded = sizeNeeded;
        goto Cleanup;
    }
    
    macAddressPolicyBuffer = (PMSFORWARD_MAC_ADDRESS_POLICY_STATUS)
                             (((PUINT8)customStatusBuffer) +
                             customStatusBuffer->FeatureStatusCustomBufferOffset);
                             
    macAddressPolicyBuffer->PortArrayLength = numAllowedSourcePorts;
    macAddressPolicyBuffer->PortArrayOffset = sizeof(MSFORWARD_MAC_ADDRESS_POLICY_STATUS);
    
    portIdArray = (PNDIS_SWITCH_PORT_ID)
                  (((PUINT8)macAddressPolicyBuffer) +
                  macAddressPolicyBuffer->PortArrayOffset);
    
    
    if (switchContext->ExternalPortId != NDIS_SWITCH_DEFAULT_PORT_ID)
    {
        portIdArray[arrayIndex] = switchContext->ExternalPortId;
        ++arrayIndex;
    }
    
    if (!IsListEmpty(nicList))
    {
        curEntry = nicList->Flink;
        
        do {
            nic = CONTAINING_RECORD(curEntry,
                                    MSFORWARD_NIC_LIST_ENTRY,
                                    ListEntry);
                                    
            if (nic->AllowSends)
            {
                portIdArray[arrayIndex] = nic->PortId;
                ++arrayIndex;
            }

            curEntry = curEntry->Flink;
        
        } while(curEntry != nicList);
    }

    SwitchFeatureStatus->FeatureStatusVersion = MAC_ADDRESS_POLICY_STATUS_VERSION;

Cleanup:
    if (lockHeld)
    {
        NdisReleaseRWLock(switchContext->DispatchLock, &lockState);
    }

    return consumed;
}
 

_Use_decl_annotations_
BOOLEAN
SxExtQueryPortFeatureStatus(
    PSX_SWITCH_OBJECT Switch,
    NDIS_HANDLE ExtensionContext,
    PNDIS_SWITCH_PORT_FEATURE_STATUS_PARAMETERS PortFeatureStatus,
    PULONG BytesNeeded
    )
/*++
  
Routine Description:
    This extension has no custom port properties.
    
--*/
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
/*++
  
Routine Description:
    The only NIC request this extension cares about is 
    OID_NIC_SWITCH_ALLOCATE_VF. We must fail all VF allocations
    so that traffic flows through the extension and we can enforce
    policy.
    
--*/
{
    NDIS_STATUS status = NDIS_STATUS_SUCCESS;
    
    UNREFERENCED_PARAMETER(Switch);
    UNREFERENCED_PARAMETER(ExtensionContext);
    UNREFERENCED_PARAMETER(SourcePortId);
    UNREFERENCED_PARAMETER(SourceNicIndex);
    UNREFERENCED_PARAMETER(DestinationPortId);
    UNREFERENCED_PARAMETER(DestinationNicIndex);
    
    //
    // Do not allow VF allocations, as all VM traffic must flow
    // through our extension.
    //
    if (OidRequest->RequestType == NdisRequestSetInformation &&
        OidRequest->DATA.SET_INFORMATION.Oid == OID_NIC_SWITCH_ALLOCATE_VF)
    {
        status = NDIS_STATUS_FAILURE;
    }
    
    return status;
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
/*++
  
Routine Description:
    This function will never be called because we do not
    redirect or edit any NIC requests.
    
--*/
{
    UNREFERENCED_PARAMETER(Switch);
    UNREFERENCED_PARAMETER(ExtensionContext);
    UNREFERENCED_PARAMETER(OidRequest);
    UNREFERENCED_PARAMETER(SourcePortId);
    UNREFERENCED_PARAMETER(SourceNicIndex);
    UNREFERENCED_PARAMETER(DestinationPortId);
    UNREFERENCED_PARAMETER(DestinationNicIndex);
    UNREFERENCED_PARAMETER(Status);
    
    //
    // This function should never be called as we don't set any
    // source/destination info in SxExtProcessNicRequest.
    //
    ASSERT(FALSE);
    
    return Status;
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
/*++
  
Routine Description:
    This extension does not need to process any status
    indications.
    
--*/
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
/*++
  
Routine Description:
    The function sets the destination lists of the NBLs forwarded
    through the switch.
    The extension only set destinations for NBLs originated from the External NIC,
    Internal NIC, or NICs with MAC Policy set.
    The extension determines the source by searching for the source
    MAC address in the NIC list.
    
    The extension sets destinations by looking at the destination MAC address.
    If the destination MAC address is a multicast or broadcast address,
    the extension broadcasts the NBL to all ports, except the source.
    If the destination MAC is a VM, the extension sets the VM as the destitation.
    Otherwise the extension sets the External port as the destination.
    
--*/
{
    PMSFORWARD_CONTEXT switchContext = (PMSFORWARD_CONTEXT)ExtensionContext;
    NDIS_SWITCH_PORT_ID sourcePort = 0, prevDestinationPort = 0, curDestinationPort = 0;
    NDIS_SWITCH_NIC_INDEX sourceIndex = 0, prevDestinationIndex = 0, curDestinationIndex = 0;
    PNDIS_SWITCH_FORWARDING_DETAIL_NET_BUFFER_LIST_INFO fwdDetail;
    PMSFORWARD_NIC_LIST_ENTRY sourceNicEntry = NULL;
    PMSFORWARD_NIC_LIST_ENTRY destinationNicEntry = NULL;
    BOOLEAN sameSource;
    PNET_BUFFER_LIST curNbl = NULL, nextNbl = NULL;
    PNET_BUFFER_LIST sendNbl = NULL, dropNbl = NULL;
    PNET_BUFFER_LIST *nextDropNbl = &dropNbl;
    PNET_BUFFER_LIST *nextSendNbl = &sendNbl;
    PMSFORWARD_ETHERNET_HEADER curHeader;
    UINT8 prevMacAddress[6] = {0};
    ULONG sendCompleteFlags = 0;
    BOOLEAN dispatch;
    PMDL curMdl;
    PUINT8 curBuffer;
    BOOLEAN broadcast = FALSE;
    NDIS_SWITCH_PORT_DESTINATION newDestination = {0};
    PNDIS_SWITCH_FORWARDING_DESTINATION_ARRAY broadcastArray;
    LOCK_STATE_EX lockState;
    NDIS_STATUS status;
    NDIS_STRING filterReason;
    ULONG numDropNbls;
    PNET_BUFFER_LIST extForwardedNbls = NULL;
    PNET_BUFFER_LIST nativeForwardedNbls = NULL;
    PNET_BUFFER_LIST *nextExtForwardNbl = &extForwardedNbls;
    PNET_BUFFER_LIST *nextNativeForwardedNbl = &nativeForwardedNbls;
    
    dispatch = NDIS_TEST_SEND_FLAG(SendFlags, NDIS_SEND_FLAGS_DISPATCH_LEVEL);
    sameSource = NDIS_TEST_SEND_FLAG(SendFlags, NDIS_SEND_FLAGS_SWITCH_SINGLE_SOURCE);
    
    sendCompleteFlags |= (dispatch) ? NDIS_SEND_COMPLETE_FLAGS_DISPATCH_LEVEL : 0;
    SendFlags |= NDIS_SEND_FLAGS_SWITCH_DESTINATION_GROUP;
    
    //
    // Take DispatchLock so no NICs disconnect while we're setting destinations.
    //
    NdisAcquireRWLockRead(switchContext->DispatchLock, &lockState, dispatch);
    if (sameSource)
    {
        fwdDetail = NET_BUFFER_LIST_SWITCH_FORWARDING_DETAIL(NetBufferLists);
        sourcePort = fwdDetail->SourcePortId;
        sourceIndex = (NDIS_SWITCH_NIC_INDEX)fwdDetail->SourceNicIndex;
        
        sendCompleteFlags |= NDIS_SEND_COMPLETE_FLAGS_SWITCH_SINGLE_SOURCE;
        
        sourceNicEntry = MsForwardFindNicByPortIdUnsafe(switchContext,
                                                        sourcePort,
                                                        sourceIndex);

        if (sourceNicEntry != NULL && !sourceNicEntry->AllowSends)
        {
            numDropNbls = 0;
            for (curNbl = NetBufferLists; curNbl != NULL; curNbl = curNbl->Next)
            {
                ++numDropNbls;
            }
            
            *nextDropNbl = NetBufferLists; 
        
            RtlInitUnicodeString(&filterReason, L"Blocked by Source MAC Policy");
                        
            Switch->NdisSwitchHandlers.ReportFilteredNetBufferLists(
                                         Switch->NdisSwitchContext,
                                         &SxExtensionGuid,
                                         &SxExtensionFriendlyName,
                                         sourcePort,
                                         NDIS_SWITCH_REPORT_FILTERED_NBL_FLAGS_IS_INCOMING,
                                         numDropNbls,
                                         dropNbl,
                                         &filterReason);
                                          
            goto Cleanup;
        }
        
        //
        // If nicEntry is not found, and is not external port,
        // we must have failed to allocate this port.
        //
        else if(sourceNicEntry == NULL &&
                sourcePort != switchContext->ExternalPortId)
        {
            numDropNbls = 0;
            for (curNbl = NetBufferLists; curNbl != NULL; curNbl = curNbl->Next)
            {
                ++numDropNbls;
            }
            
            *nextDropNbl = NetBufferLists;
            
            RtlInitUnicodeString(&filterReason, L"Low Resources");
            
            Switch->NdisSwitchHandlers.ReportFilteredNetBufferLists(
                                         Switch->NdisSwitchContext,
                                         &SxExtensionGuid,
                                         &SxExtensionFriendlyName,
                                         sourcePort,
                                         NDIS_SWITCH_REPORT_FILTERED_NBL_FLAGS_IS_INCOMING,
                                         numDropNbls,
                                         dropNbl,
                                         &filterReason);
                                         
            goto Cleanup;
        }                
    }
        
    //
    // Split NBL list into NBLs to be forwarded by us, and those that require
    // native forwarding.
    //
    for (curNbl = NetBufferLists; curNbl != NULL; curNbl = nextNbl)
    {
        nextNbl = curNbl->Next;
        curNbl->Next = NULL;
        fwdDetail = NET_BUFFER_LIST_SWITCH_FORWARDING_DETAIL(curNbl);
        if (fwdDetail->NativeForwardingRequired)
        {
            *nextNativeForwardedNbl = curNbl;
            nextNativeForwardedNbl = &(curNbl->Next);
        }
        else
        {
            *nextExtForwardNbl = curNbl;
            nextExtForwardNbl = &(curNbl->Next);
        }
    }
    
    for (curNbl = extForwardedNbls; curNbl != NULL; curNbl = nextNbl)
    {
        nextNbl = curNbl->Next;
        curNbl->Next = NULL;
        
        fwdDetail = NET_BUFFER_LIST_SWITCH_FORWARDING_DETAIL(curNbl);
    
        //
        // First check for allowed source if not same source.
        //
        if (!sameSource)
        {
            sourcePort = fwdDetail->SourcePortId;
            sourceIndex = (NDIS_SWITCH_NIC_INDEX)fwdDetail->SourceNicIndex;
            
            sourceNicEntry = MsForwardFindNicByPortIdUnsafe(switchContext,
                                                            sourcePort,
                                                            sourceIndex);
                                                 
            if (sourceNicEntry != NULL && !sourceNicEntry->AllowSends)
            {
                RtlInitUnicodeString(&filterReason, L"Blocked by Source MAC Policy");
                
                Switch->NdisSwitchHandlers.ReportFilteredNetBufferLists(
                                             Switch->NdisSwitchContext,
                                             &SxExtensionGuid,
                                             &SxExtensionFriendlyName,
                                             sourcePort,
                                             NDIS_SWITCH_REPORT_FILTERED_NBL_FLAGS_IS_INCOMING,
                                             1,
                                             curNbl,
                                             &filterReason);
                                             
                *nextDropNbl = curNbl;
                nextDropNbl = &curNbl->Next;
                continue;
            }
            else if(sourceNicEntry == NULL &&
                    sourcePort != switchContext->ExternalPortId)
            {
                RtlInitUnicodeString(&filterReason, L"Low Resources");
                
                Switch->NdisSwitchHandlers.ReportFilteredNetBufferLists(
                                         Switch->NdisSwitchContext,
                                         &SxExtensionGuid,
                                         &SxExtensionFriendlyName,
                                         sourcePort,
                                         NDIS_SWITCH_REPORT_FILTERED_NBL_FLAGS_IS_INCOMING,
                                         1,
                                         curNbl,
                                         &filterReason);
                
                *nextDropNbl = curNbl;
                nextDropNbl = &curNbl->Next;
                continue;
            }        
        }
    
        //
        // Ethernet Header is a guaranteed safe access.
        //
        curMdl = (NET_BUFFER_LIST_FIRST_NB(curNbl))->CurrentMdl;
        curBuffer =  MmGetSystemAddressForMdlSafe(
                        curMdl,
                        LowPagePriority | MdlMappingNoExecute);
        curHeader = (PMSFORWARD_ETHERNET_HEADER)
                        (curBuffer + (NET_BUFFER_LIST_FIRST_NB(curNbl))->CurrentMdlOffset);
                        
        //
        // Check for broadcast. (Broadcast if multicast)
        //
        if (ETH_IS_BROADCAST(curHeader->Destination) ||
            ETH_IS_MULTICAST(curHeader->Destination))
        {
            if(sendNbl != NULL && !broadcast)
            {
                //
                // Send first nbl list, set destinations of this one, continue to work on next.
                //
                SxLibSendNetBufferListsIngress(Switch,
                                               sendNbl,
                                               SendFlags,
                                               0);
                
                sendNbl = NULL;
                nextSendNbl = &sendNbl;
            }
            
            if (fwdDetail->NumAvailableDestinations < (switchContext->NumDestinations - 1))
            {
                status = Switch->NdisSwitchHandlers.GrowNetBufferListDestinations(
                                                    Switch->NdisSwitchContext,
                                                    curNbl,
                                                    (switchContext->NumDestinations - 1 - fwdDetail->NumAvailableDestinations),
                                                    &broadcastArray);
                                                    
                if (status != NDIS_STATUS_SUCCESS)
                {
                    RtlInitUnicodeString(&filterReason, L"Failed to grow destination list.");
                    Switch->NdisSwitchHandlers.ReportFilteredNetBufferLists(
                                         Switch->NdisSwitchContext,
                                         &SxExtensionGuid,
                                         &SxExtensionFriendlyName,
                                         sourcePort,
                                         NDIS_SWITCH_REPORT_FILTERED_NBL_FLAGS_IS_INCOMING,
                                         1,
                                         curNbl,
                                         &filterReason);
                
                    *nextDropNbl = curNbl;
                    nextDropNbl = &curNbl->Next;
                    continue;
                }
            }
            else
            {
                Switch->NdisSwitchHandlers.GetNetBufferListDestinations(
                                                    Switch->NdisSwitchContext,
                                                    curNbl,
                                                    &broadcastArray);
            }
            
            MsForwardMakeBroadcastArrayUnsafe(switchContext,
                                              broadcastArray,
                                              sourcePort,
                                              sourceIndex);

            if (switchContext->NumDestinations == 1)
            {
                RtlInitUnicodeString(&filterReason, L"Zero destinations for broadcast.");
                Switch->NdisSwitchHandlers.ReportFilteredNetBufferLists(
                                     Switch->NdisSwitchContext,
                                     &SxExtensionGuid,
                                     &SxExtensionFriendlyName,
                                     sourcePort,
                                     NDIS_SWITCH_REPORT_FILTERED_NBL_FLAGS_IS_INCOMING,
                                     1,
                                     curNbl,
                                     &filterReason);
                
                *nextDropNbl = curNbl;
                nextDropNbl = &curNbl->Next;
                continue;
            }
            
            status = Switch->NdisSwitchHandlers.UpdateNetBufferListDestinations(
                                                        Switch->NdisSwitchContext,
                                                        curNbl,
                                                        (switchContext->NumDestinations - 1),
                                                        broadcastArray);
            ASSERT(status == NDIS_STATUS_SUCCESS);
                                                                        
            *nextSendNbl = curNbl;
            nextSendNbl = &(curNbl->Next);
            broadcast = TRUE;
            
            continue;
        }
            

        if (RtlEqualMemory(prevMacAddress,
                           curHeader->Destination,
                           sizeof(prevMacAddress)))
        {
            curDestinationPort = prevDestinationPort;
            curDestinationIndex = prevDestinationIndex;
        }
        else
        {
            destinationNicEntry = MsForwardFindNicByMacAddressUnsafe(switchContext,
                                                                     curHeader->Destination);
            //
            // Not a VM or host, send to external.
            //                                            
            if (destinationNicEntry == NULL)
            {
                //
                // If no external, or source is external, drop.
                //
                if (switchContext->ExternalPortId == 0)
                {
                    RtlInitUnicodeString(&filterReason, L"No external NIC");
                    Switch->NdisSwitchHandlers.ReportFilteredNetBufferLists(
                                         Switch->NdisSwitchContext,
                                         &SxExtensionGuid,
                                         &SxExtensionFriendlyName,
                                         sourcePort,
                                         NDIS_SWITCH_REPORT_FILTERED_NBL_FLAGS_IS_INCOMING,
                                         1,
                                         curNbl,
                                         &filterReason);
                                         
                    *nextDropNbl = curNbl;
                    nextDropNbl = &curNbl->Next;
                    continue;
                }
                
                if (sourcePort == switchContext->ExternalPortId)
                {
                    RtlInitUnicodeString(&filterReason, L"Destination == Source");
                    Switch->NdisSwitchHandlers.ReportFilteredNetBufferLists(
                                         Switch->NdisSwitchContext,
                                         &SxExtensionGuid,
                                         &SxExtensionFriendlyName,
                                         sourcePort,
                                         NDIS_SWITCH_REPORT_FILTERED_NBL_FLAGS_IS_INCOMING,
                                         1,
                                         curNbl,
                                         &filterReason);
                                         
                    *nextDropNbl = curNbl;
                    nextDropNbl = &curNbl->Next;
                    continue;
                }
            
                curDestinationPort = switchContext->ExternalPortId;
                curDestinationIndex = switchContext->ExternalNicIndex;
            }
            else if (destinationNicEntry->Connected)
            {
                curDestinationPort = destinationNicEntry->PortId;
                curDestinationIndex = destinationNicEntry->NicIndex;
            }
            else
            {
                RtlInitUnicodeString(&filterReason, L"Destination is NOT connected.");
                Switch->NdisSwitchHandlers.ReportFilteredNetBufferLists(
                                     Switch->NdisSwitchContext,
                                     &SxExtensionGuid,
                                     &SxExtensionFriendlyName,
                                     sourcePort,
                                     NDIS_SWITCH_REPORT_FILTERED_NBL_FLAGS_IS_INCOMING,
                                     1,
                                     curNbl,
                                     &filterReason);
                                         
                *nextDropNbl = curNbl;
                nextDropNbl = &curNbl->Next;
                continue;
            }
        }
        
        RtlMoveMemory(prevMacAddress, curHeader->Destination, sizeof(prevMacAddress));

        newDestination.PortId = curDestinationPort;
        newDestination.NicIndex = curDestinationIndex;
        newDestination.PreserveVLAN = 0;
        
        ASSERT(fwdDetail->NumAvailableDestinations > 0);
        status = Switch->NdisSwitchHandlers.AddNetBufferListDestination(
                                                    Switch->NdisSwitchContext,
                                                    curNbl,
                                                    &newDestination);
        ASSERT(status == NDIS_STATUS_SUCCESS);
          
        if(sendNbl != NULL && 
            (broadcast ||
              (prevDestinationPort != curDestinationPort ||
               prevDestinationIndex != curDestinationIndex)))
        {
            SxLibSendNetBufferListsIngress(Switch,
                                           sendNbl,
                                           SendFlags,
                                           0);
            
            sendNbl = NULL;
            nextSendNbl = &sendNbl;
        }
        
        *nextSendNbl = curNbl;
        nextSendNbl = &(curNbl->Next);
            
        //
        // Done processing this NBL.
        //
        prevDestinationPort = curDestinationPort;
        prevDestinationIndex = curDestinationIndex;
        broadcast = FALSE;
    }
    
Cleanup:
    NdisReleaseRWLock(switchContext->DispatchLock, &lockState);
 
    if (sendNbl != NULL)
    {
        SxLibSendNetBufferListsIngress(Switch,
                                       sendNbl,
                                       SendFlags,
                                       0);
    }
    
    if (nativeForwardedNbls != NULL)
    {
        SxLibSendNetBufferListsIngress(Switch,
                                       nativeForwardedNbls,
                                       SendFlags,
                                       0);
    }
    
    if (dropNbl != NULL)
    {
        SxLibCompleteNetBufferListsIngress(Switch,
                                           dropNbl,
                                           sendCompleteFlags);
    }
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
/*++
  
Routine Description:
    No egress processing necessary.
    
--*/
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
/*++
  
Routine Description:
    No egress processing necessary.
    
--*/
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
/*++
  
Routine Description:
    No ingress complete processing necessary.
    
--*/
{
    UNREFERENCED_PARAMETER(ExtensionContext);
    
    SxLibCompleteNetBufferListsIngress(Switch,
                                       NetBufferLists,
                                       SendCompleteFlags);
}


NDIS_STATUS
MsForwardAddNicUnsafe(
    _In_ PMSFORWARD_CONTEXT SwitchContext,
    _In_reads_bytes_(6) PUCHAR MacAddress,
    _In_ NDIS_SWITCH_PORT_ID PortId,
    _In_ NDIS_SWITCH_NIC_INDEX NicIndex,
    _In_ NDIS_SWITCH_NIC_TYPE NicType,
    _In_ BOOLEAN Connected
    )
/*++
  
Routine Description:
    Add given NIC to the NIC list and correlate with
    MAC policy.
    
--*/
{
    NDIS_STATUS status = NDIS_STATUS_SUCCESS;
    PMSFORWARD_NIC_LIST_ENTRY nicEntry = NULL;
    PLIST_ENTRY nicList = &SwitchContext->NicList;
    
    if (NicType == NdisSwitchNicTypeExternal)
    {
        goto Cleanup;
    }
    
    nicEntry = MsForwardFindNicByPortIdUnsafe(SwitchContext,
                                              PortId,  
                                              NicIndex);
                                       
    if (nicEntry == NULL)
    {
        nicEntry = ExAllocatePoolWithTag(NonPagedPoolNx,
                                         sizeof(MSFORWARD_NIC_LIST_ENTRY),
                                         SxExtAllocationTag);
                                         
        if (nicEntry == NULL)
        {
            status = NDIS_STATUS_RESOURCES;
            goto Cleanup;
        }
        
        NdisZeroMemory(nicEntry, sizeof(MSFORWARD_NIC_LIST_ENTRY));
        NdisMoveMemory(nicEntry->MacAddress, MacAddress, MSFORWARD_MAC_LENGTH);
        
        nicEntry->PortId = PortId;
        nicEntry->NicIndex = NicIndex;
        nicEntry->NicType = NicType;
        nicEntry->Connected = Connected;
        
        if (NicType == NdisSwitchNicTypeInternal)
        {
            nicEntry->AllowSends = TRUE;
        }
        else
        {
            nicEntry->AllowSends = MsForwardNicHasPolicy(SwitchContext, MacAddress);
        }
        
        InsertHeadList(nicList, &nicEntry->ListEntry);
    }
    
Cleanup:
    return status;
}


NDIS_STATUS
MsForwardAddMacPolicyUnsafe(
    _In_ PMSFORWARD_CONTEXT SwitchContext,
    _In_ PMSFORWARD_MAC_ADDRESS_POLICY MacPolicyBuffer,
    _In_ PNDIS_SWITCH_OBJECT_INSTANCE_ID PropertyInstanceId
    )
/*++
  
Routine Description:
    Add the given policy to the policy list and correlate with
    the NIC list.
    
--*/
{
    NDIS_STATUS status = NDIS_STATUS_SUCCESS;
    PMSFORWARD_MAC_POLICY_LIST_ENTRY newPolicy;
    PMSFORWARD_NIC_LIST_ENTRY nic;
    
    newPolicy = MsForwardFindPolicyByMacAddressUnsafe(SwitchContext,
                                                      MacPolicyBuffer->MacAddress);
                                            
    if (newPolicy == NULL)
    {
        newPolicy = ExAllocatePoolWithTag(NonPagedPoolNx,
                                          sizeof(MSFORWARD_MAC_POLICY_LIST_ENTRY),
                                          SxExtAllocationTag);
                                          
        if (newPolicy == NULL)
        {
            status = NDIS_STATUS_RESOURCES;
            goto Cleanup;
        }
        
        NdisMoveMemory(&newPolicy->MacAddress,
                       MacPolicyBuffer->MacAddress,
                       sizeof(newPolicy->MacAddress));
        
        NdisMoveMemory(&newPolicy->PropertyInstanceId,
                       PropertyInstanceId,
                       sizeof(NDIS_SWITCH_OBJECT_INSTANCE_ID));
        
        InsertHeadList(&SwitchContext->PropertyList,
                       &newPolicy->ListEntry);
                       
        nic = MsForwardFindNicByMacAddressUnsafe(SwitchContext,
                                                 MacPolicyBuffer->MacAddress);
                                                
        if (nic != NULL)
        {
            nic->AllowSends = TRUE;
        }
    }
    else
    {
        status = NDIS_STATUS_DATA_NOT_ACCEPTED;
    }
        
Cleanup:
    return status;
}


VOID
MsForwardDeleteMacPolicyUnsafe(
    _In_ PMSFORWARD_CONTEXT SwitchContext,
    _In_ PNDIS_SWITCH_OBJECT_INSTANCE_ID PropertyInstanceId
    )
/*++
  
Routine Description:
    Delete the given MAC policy, and coorelate with
    the NIC list.
    
--*/
{
    PMSFORWARD_MAC_POLICY_LIST_ENTRY deletePolicy;
    PMSFORWARD_NIC_LIST_ENTRY nic;
    
    deletePolicy = MsForwardFindPolicyByPropertyInstanceIdUnsafe(
                                                SwitchContext,
                                                PropertyInstanceId);
                                                
    if (deletePolicy != NULL)
    {
        nic = MsForwardFindNicByMacAddressUnsafe(SwitchContext,
                                                 deletePolicy->MacAddress);
                                                
        if (nic != NULL)
        {
            nic->AllowSends = FALSE;
        }
        
        RemoveEntryList(&deletePolicy->ListEntry);
        ExFreePoolWithTag(deletePolicy, SxExtAllocationTag);
    }
}


PMSFORWARD_NIC_LIST_ENTRY
MsForwardFindNicByPortIdUnsafe(
    _In_ PMSFORWARD_CONTEXT SwitchContext,
    _In_ NDIS_SWITCH_PORT_ID PortId,
    _In_ NDIS_SWITCH_NIC_INDEX NicIndex
    )
/*++
  
Routine Description:
    Search for the NIC needed by port ID.
    
--*/
{
    PLIST_ENTRY nicList = &SwitchContext->NicList;
    PLIST_ENTRY curEntry = nicList->Flink;
    PMSFORWARD_NIC_LIST_ENTRY nic = NULL;
        
    if (IsListEmpty(nicList))
    {
        goto Cleanup;
    }
    
    do {
        nic = CONTAINING_RECORD(curEntry,
                                MSFORWARD_NIC_LIST_ENTRY,
                                ListEntry);
                                
        if (nic->PortId == PortId &&
            nic->NicIndex == NicIndex)
        {
            goto Cleanup;
        }
                         
        curEntry = curEntry->Flink;
    
    } while(curEntry != nicList);
    
    nic = NULL;
    
Cleanup:
    return nic;
}


PMSFORWARD_NIC_LIST_ENTRY
MsForwardFindNicByMacAddressUnsafe(
    _In_ PMSFORWARD_CONTEXT SwitchContext,
    _In_reads_bytes_(6) PUCHAR MacAddress
    )
/*++
  
Routine Description:
    Search for the NIC needed by MAC Address.
    
--*/
{
    PLIST_ENTRY nicList = &SwitchContext->NicList;
    PLIST_ENTRY curEntry = nicList->Flink;
    PMSFORWARD_NIC_LIST_ENTRY nic = NULL;
        
    if (IsListEmpty(nicList))
    {
        goto Cleanup;
    }
    
    do {
        nic = CONTAINING_RECORD(curEntry,
                                MSFORWARD_NIC_LIST_ENTRY,
                                ListEntry);
                                
        if (RtlEqualMemory(MacAddress,
                           nic->MacAddress,
                           sizeof(nic->MacAddress)))
        {
            goto Cleanup;
        }
                         
        curEntry = curEntry->Flink;
    
    } while(curEntry != nicList);
    
    nic = NULL;
    
Cleanup:
    return nic;
}


PMSFORWARD_MAC_POLICY_LIST_ENTRY
MsForwardFindPolicyByMacAddressUnsafe(
    _In_ PMSFORWARD_CONTEXT SwitchContext,
    _In_reads_bytes_(6) PUCHAR MacAddress
    )
/*++
  
Routine Description:
    Search for the policy needed by MAC address.
    
--*/
{
    PLIST_ENTRY propertyList = &SwitchContext->PropertyList;
    PLIST_ENTRY curEntry = propertyList->Flink;
    PMSFORWARD_MAC_POLICY_LIST_ENTRY policy = NULL;
        
    if (IsListEmpty(propertyList))
    {
        goto Cleanup;
    }
    
    do {
        policy = CONTAINING_RECORD(curEntry,
                                   MSFORWARD_MAC_POLICY_LIST_ENTRY,
                                   ListEntry);
                                
        if (RtlEqualMemory(MacAddress,
                           policy->MacAddress,
                           sizeof(policy->MacAddress)))
        {
            goto Cleanup;
        }
                         
        curEntry = curEntry->Flink;
    
    } while(curEntry != propertyList);
    
    policy = NULL;
    
Cleanup:
    return policy;
}


PMSFORWARD_MAC_POLICY_LIST_ENTRY
MsForwardFindPolicyByPropertyInstanceIdUnsafe(
    _In_ PMSFORWARD_CONTEXT SwitchContext,
    _In_ PNDIS_SWITCH_OBJECT_INSTANCE_ID PropertyInstanceId
    )
/*++
  
Routine Description:
    Search for the policy needed by PropertyInstanceId.
    
--*/
{
    PLIST_ENTRY propertyList = &SwitchContext->PropertyList;
    PLIST_ENTRY curEntry = propertyList->Flink;
    PMSFORWARD_MAC_POLICY_LIST_ENTRY policy = NULL;
        
    if (IsListEmpty(propertyList))
    {
        goto Cleanup;
    }
    
    do {
        policy = CONTAINING_RECORD(curEntry,
                                   MSFORWARD_MAC_POLICY_LIST_ENTRY,
                                   ListEntry);
                                
        if (RtlEqualMemory(PropertyInstanceId,
                           &policy->PropertyInstanceId,
                           sizeof(policy->PropertyInstanceId)))
        {
            goto Cleanup;
        }
                         
        curEntry = curEntry->Flink;
    
    } while(curEntry != propertyList);
    
    policy = NULL;
    
Cleanup:
    return policy;
}

    
NDIS_STATUS
MsForwardDeleteNicUnsafe(
   _In_ PMSFORWARD_CONTEXT SwitchContext,
   _In_ NDIS_SWITCH_PORT_ID PortId,
   _In_ NDIS_SWITCH_NIC_INDEX NicIndex
   )
/*++
  
Routine Description:
    Remove the NIC represented by the PortId and NicIndex
    from the NIC list and free its memory.
    
--*/
{
    NDIS_STATUS status = NDIS_STATUS_SUCCESS;
    
    PMSFORWARD_NIC_LIST_ENTRY nicEntry = MsForwardFindNicByPortIdUnsafe(SwitchContext,
                                                                        PortId,
                                                                        NicIndex);
                                                                
    if (nicEntry == NULL)
    {
        ASSERT(FALSE);
        goto Cleanup;
    }
    
    RemoveEntryList(&nicEntry->ListEntry);
    ExFreePoolWithTag(nicEntry, SxExtAllocationTag);

Cleanup:      
    return status;
}
  
  
VOID
MsForwardClearNicListUnsafe(
    _In_ PMSFORWARD_CONTEXT SwitchContext
    )
/*++
  
Routine Description:
    Remove all NICs from the list and free all memory.
    
--*/
{
    PMSFORWARD_NIC_LIST_ENTRY nic;
    PLIST_ENTRY nicList = &SwitchContext->NicList;
    PLIST_ENTRY headList = NULL;
    
    while (!IsListEmpty(nicList))
    {
        headList = RemoveHeadList(nicList);
    
        nic = CONTAINING_RECORD(headList,
                                MSFORWARD_NIC_LIST_ENTRY,
                                ListEntry);
        
        ExFreePoolWithTag(nic, SxExtAllocationTag);
    }

    return;
}


VOID
MsForwardClearPropertyListUnsafe(
    _In_ PMSFORWARD_CONTEXT SwitchContext
    )
/*++
  
Routine Description:
    Remove all properties from the list and free all memory.
    
--*/
{
    PMSFORWARD_MAC_POLICY_LIST_ENTRY policy;
    PLIST_ENTRY propertyList = &SwitchContext->PropertyList;
    PLIST_ENTRY headList = NULL;
    
    while (!IsListEmpty(propertyList))
    {
        headList = RemoveHeadList(propertyList);
    
        policy = CONTAINING_RECORD(headList,
                                   MSFORWARD_MAC_POLICY_LIST_ENTRY,
                                   ListEntry);
        
        ExFreePoolWithTag(policy, SxExtAllocationTag);
    }

    return;
}


BOOLEAN
MsForwardNicHasPolicy(
    _In_ PMSFORWARD_CONTEXT SwitchContext,
    _In_reads_bytes_(6) PUCHAR MacAddress
    )
/*++
  
Routine Description:
    Returns TRUE if there is a correlated policy to the MAC
    address given.
    
--*/
{
    return (MsForwardFindPolicyByMacAddressUnsafe(SwitchContext,
                                                  MacAddress) != NULL);
}
    

VOID
MsForwardMakeBroadcastArrayUnsafe(
    _In_ PMSFORWARD_CONTEXT SwitchContext,
    _In_ PNDIS_SWITCH_FORWARDING_DESTINATION_ARRAY BroadcastArray,
    _In_ NDIS_SWITCH_PORT_ID SourcePortId,
    _In_ NDIS_SWITCH_NIC_INDEX SourceNicIndex
    )
/*++
  
Routine Description:
    Creates the destination array of all connected NICs
    excluding the source given.
    
--*/
{
    PLIST_ENTRY nicList = &SwitchContext->NicList;
    PLIST_ENTRY curEntry = nicList->Flink;
    PMSFORWARD_NIC_LIST_ENTRY nic = NULL;
    UINT32 index = BroadcastArray->NumDestinations;
    PNDIS_SWITCH_PORT_DESTINATION destination;
        
    if (IsListEmpty(nicList))
    {
        goto Cleanup;
    }
    
    do {
        nic = CONTAINING_RECORD(curEntry,
                                MSFORWARD_NIC_LIST_ENTRY,
                                ListEntry);
                                
        if ((SourcePortId == nic->PortId &&
             SourceNicIndex == nic->NicIndex) ||
             !nic->Connected)
        {
            if(curEntry->Flink == nicList)
            {
                break;
            }
            else
            {
                curEntry = curEntry->Flink;
                continue;
            }
        }
        
        destination = NDIS_SWITCH_PORT_DESTINATION_AT_ARRAY_INDEX(BroadcastArray, index);
        NdisZeroMemory(destination, sizeof(NDIS_SWITCH_PORT_DESTINATION));
        
        destination->PortId = nic->PortId;
        destination->NicIndex = nic->NicIndex;
        
        ++index;
        curEntry = curEntry->Flink;
    
    } while(curEntry != nicList);
    
    if (SourcePortId != SwitchContext->ExternalPortId &&
        SwitchContext->ExternalNicConnected)
    {
        destination = NDIS_SWITCH_PORT_DESTINATION_AT_ARRAY_INDEX(BroadcastArray, index);
        destination->PortId = SwitchContext->ExternalPortId;
        destination->NicIndex = SwitchContext->ExternalNicIndex;
    }
    
Cleanup:
    return;
}


NDIS_STATUS
MsForwardInitSwitch(
    _In_ PSX_SWITCH_OBJECT Switch,
    _In_ PMSFORWARD_CONTEXT SwitchContext
    )
/*++
  
Routine Description:
    Initializes the switch state.
    
--*/
{
    NDIS_STATUS status = NDIS_STATUS_SUCCESS;
    PNDIS_SWITCH_NIC_ARRAY nicArray = NULL;
    ULONG arrIndex;
    PNDIS_SWITCH_NIC_PARAMETERS curNic;
    PNDIS_SWITCH_PROPERTY_ENUM_PARAMETERS switchPropertyParameters = NULL;
    PNDIS_SWITCH_PROPERTY_ENUM_INFO switchPropertyInfo = NULL;
    PNDIS_SWITCH_PROPERTY_CUSTOM customPropertyInfo = NULL;
    PMSFORWARD_MAC_ADDRESS_POLICY macAddressPolicy;
    PNDIS_SWITCH_PORT_PROPERTY_ENUM_PARAMETERS portPropertyParameters = NULL;
    PNDIS_SWITCH_PORT_PROPERTY_ENUM_INFO portPropertyInfo = NULL;
    PNDIS_SWITCH_PORT_PROPERTY_VLAN vlanProperty;
    
    ASSERT(!SwitchContext->IsActive);

    //
    // Get Property List.
    //
    status = SxLibGetSwitchPropertyUnsafe(Switch,
                                          NdisSwitchPropertyTypeCustom,
                                          (PNDIS_SWITCH_OBJECT_ID)&MacAddressPolicyGuid,
                                          &switchPropertyParameters);
                                              
    if (status != NDIS_STATUS_SUCCESS)
    {
        status = NDIS_STATUS_RESOURCES;
        goto Cleanup;
    }
    
    switchPropertyInfo =
        NDIS_SWITCH_PROPERTY_ENUM_PARAMETERS_GET_FIRST_INFO(switchPropertyParameters);
    
    for (arrIndex = 0;
         arrIndex < switchPropertyParameters->NumProperties;
         ++arrIndex)
    {
        //
        // Should always get back v1 or later. It is safe to access the 
        // v1 version of the structure if newer property is retrieved.
        //
        ASSERT(switchPropertyInfo->PropertyVersion >= 
               MAC_ADDRESS_POLICY_VERSION);

        customPropertyInfo =
            NDIS_SWITCH_PROPERTY_ENUM_INFO_GET_PROPERTY(switchPropertyInfo);

        macAddressPolicy =
            NDIS_SWITCH_PROPERTY_CUSTOM_GET_BUFFER(customPropertyInfo);

        status = MsForwardAddMacPolicyUnsafe(SwitchContext,
                                             macAddressPolicy,
                                             &switchPropertyInfo->PropertyInstanceId);
        
        if (status != NDIS_STATUS_SUCCESS)
        {
            goto Cleanup;
        }
        
        switchPropertyInfo =
            NDIS_SWITCH_PROPERTY_ENUM_INFO_GET_NEXT(switchPropertyInfo);
    }

    //
    // Now, get NIC list.
    //    
    status = SxLibGetNicArrayUnsafe(Switch, &nicArray);
    if (status != NDIS_STATUS_SUCCESS)
    {
        goto Cleanup;
    }
    
    for (arrIndex = 0; arrIndex < nicArray->NumElements; ++arrIndex)
    {
        curNic = NDIS_SWITCH_NIC_AT_ARRAY_INDEX(nicArray, arrIndex);
        
        status = Switch->NdisSwitchHandlers.ReferenceSwitchPort(
                                        Switch->NdisSwitchContext,
                                        curNic->PortId);
                                        
        ASSERT(status == NDIS_STATUS_SUCCESS);
        
        //
        // Get VLAN Port property to ensure no VLAN set.
        //
        status = SxLibGetPortPropertyUnsafe(Switch,
                                            curNic->PortId,
                                            NdisSwitchPortPropertyTypeVlan,
                                            NULL,
                                            &portPropertyParameters);
                            
        if (status != NDIS_STATUS_SUCCESS)
        {
            status = NDIS_STATUS_RESOURCES;
            goto Cleanup;
        }
            
        portPropertyInfo =
            NDIS_SWITCH_PORT_PROPERTY_ENUM_PARAMETERS_GET_FIRST_INFO(portPropertyParameters);

        //
        // Should always get back v1 or later. It is safe to access the v1  
        // version of the structure if newer property is retrieved.
        //
        ASSERT(portPropertyInfo->PropertyVersion >= 
               NDIS_SWITCH_PORT_PROPERTY_VLAN_REVISION_1);

        vlanProperty =
                NDIS_SWITCH_PORT_PROPERTY_ENUM_INFO_GET_PROPERTY(portPropertyInfo);
        
        //
        // Real production code should support VLAN,
        // and not fail SxExtRestartSwitch.
        //
        if (vlanProperty->OperationMode != NdisSwitchPortVlanModeAccess ||
            vlanProperty->VlanProperties.AccessVlanId != 0)
        {
            status = NDIS_STATUS_FAILURE;
            goto Cleanup;
        }
        
        status = Switch->NdisSwitchHandlers.DereferenceSwitchPort(
                                        Switch->NdisSwitchContext,
                                        curNic->PortId);
                                        
        ASSERT(status == NDIS_STATUS_SUCCESS);
        
        //
        // If a VF is assigned to a NIC, then the traffic
        // flows through the VF and not the switch. This means
        // we have to revoke the VF to enforce our policy.
        //
        if (curNic->VFAssigned)
        {
            status = Switch->NdisSwitchHandlers.ReferenceSwitchNic(
                                        Switch->NdisSwitchContext,
                                        curNic->PortId,
                                        curNic->NicIndex);
                                        
            ASSERT(status == NDIS_STATUS_SUCCESS);
        
            SxLibRevokeVfUnsafe(Switch, curNic->PortId);
            
            status = Switch->NdisSwitchHandlers.DereferenceSwitchNic(
                                        Switch->NdisSwitchContext,
                                        curNic->PortId,
                                        curNic->NicIndex);
                                        
            ASSERT(status == NDIS_STATUS_SUCCESS);
        }
        
        //
        // Now we've verified we can support the NIC, so
        // check if there's a property for it, and add it to
        // the NIC list.
        //            
        status = MsForwardAddNicUnsafe(SwitchContext,
                                       curNic->PermanentMacAddress,
                                       curNic->PortId,
                                       curNic->NicIndex,
                                       curNic->NicType,
                                       (curNic->NicState == NdisSwitchNicStateConnected));
                                       
        if (status != NDIS_STATUS_SUCCESS)
        {
            goto Cleanup;
        }
        
        if (curNic->NicType == NdisSwitchNicTypeExternal &&
            curNic->NicIndex != 0 &&
            SwitchContext->ExternalPortId == 0)
        {
            SwitchContext->ExternalPortId = curNic->PortId;
            SwitchContext->ExternalNicIndex = curNic->NicIndex;
            SwitchContext->ExternalNicConnected = TRUE;
            ++(SwitchContext->NumDestinations);
        }
        else if (curNic->NicType != NdisSwitchNicTypeExternal)
        {
            ++(SwitchContext->NumDestinations);
        }
    }
    
    SwitchContext->IsActive = TRUE;

Cleanup:
    if (switchPropertyParameters != NULL)
    {
        ExFreePoolWithTag(switchPropertyParameters, SxExtAllocationTag);
    }
    
    if (portPropertyParameters != NULL)
    {
        ExFreePoolWithTag(portPropertyParameters, SxExtAllocationTag);
    }
    
    if (nicArray != NULL)
    {
        ExFreePoolWithTag(nicArray, SxExtAllocationTag);
    }
    
    return status;
}

