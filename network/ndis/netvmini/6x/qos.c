/*++

Copyright (c) Microsoft Corporation.  All rights reserved.

    THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY
    KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
    IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR
    PURPOSE.

Module Name:

    Qos.c

Abstract:

    This module implements the NDIS QOS related functionality for the adapter.

--*/

#include "netvmin6.h"
#include "qos.tmh"


#pragma NDIS_PAGEABLE_FUNCTION(ReadQOSConfig)
#pragma NDIS_PAGEABLE_FUNCTION(SetQOSParameters)
#pragma NDIS_PAGEABLE_FUNCTION(InitializeQOSConfig)


//
// The priority-to-TC assignment table below replicates the recommended mapping in
// Table 8-4 of the IEEE 802.1Q spec. The table assumes the hardware supports 8
// traffic classes.
//
C_ASSERT(NIC_SUPPORTED_NUM_TCS == 8);
CONST UCHAR DefaultQOSPriorityAssignmentTable[NDIS_QOS_MAXIMUM_PRIORITIES] = { 1, 0, 2, 3, 4, 5, 6, 7 };

//
// By default, all TCs use ETS. Split the 100% bandwidth across all 8 TCs.
//
#define _S NDIS_QOS_TSA_STRICT
#define _E NDIS_QOS_TSA_ETS
CONST UCHAR DefaultQOSTsaAssignmentTable[NDIS_QOS_MAXIMUM_TRAFFIC_CLASSES] = { _E, _E, _E, _E, _E, _E, _E, _E };
#undef _E
#undef _S
CONST UCHAR DefaultQOSTcBandwidthAssignmentTable[NDIS_QOS_MAXIMUM_TRAFFIC_CLASSES] = { 12, 13, 12, 13, 12, 13, 12, 13 };


_IRQL_requires_max_(DISPATCH_LEVEL)
PNDIS_QOS_PARAMETERS
CreateParameters(
    _In_ ULONG NumTrafficClasses,
    _In_reads_(NDIS_QOS_MAXIMUM_PRIORITIES) CONST UCHAR *PriorityAssignments,
    _In_reads_(NDIS_QOS_MAXIMUM_TRAFFIC_CLASSES) CONST UCHAR *TcBandwidthAssignments,
    _In_reads_(NDIS_QOS_MAXIMUM_TRAFFIC_CLASSES) CONST UCHAR *TsaAssignments,
    _In_ ULONG PfcEnable,
    _In_ ULONG NumClassificationElements,
    _Post_
        _When_(return != 0, _Deref_out_range_(>=, sizeof(NDIS_QOS_PARAMETERS)))
        ULONG *MaxParamsSize
    )
/*++
Routine Description:

    This routine creates and initializes a new NDIS_QOS_PARAMETERS structure.

--*/
{
    ULONG FirstElementOffset;
    ULONG ParamSize;
    PNDIS_QOS_PARAMETERS Parameters;

    *MaxParamsSize = 0;

    //
    // Figure out the actual size of NDIS_QOS_PARAMETERS if it were to accommodate
    // NumClassificationElements classification elements. Note that the first
    // element must be aligned at the end of the parent NDIS_QOS_PARAMETERS
    // structure.
    //
    ParamSize = sizeof(NDIS_QOS_PARAMETERS);
    FirstElementOffset = ALIGN_UP(ParamSize, NDIS_QOS_CLASSIFICATION_ELEMENT);
    ParamSize = FirstElementOffset + NumClassificationElements * sizeof(NDIS_QOS_CLASSIFICATION_ELEMENT);

    //
    // Allocate the NDIS_QOS_PARAMETERS structure.
    //
    Parameters =
        NdisAllocateMemoryWithTagPriority(
            NdisDriverHandle,
            ParamSize,
            NIC_TAG_QOS_PARAMS,
            NormalPoolPriority);

    if (Parameters == NULL)
    {
        DEBUGP(MP_ERROR, "Failed to allocate NDIS_QOS_PARAMETERS.\n");
        return NULL;
    }

    //
    // Initialize the structure.
    //
    NdisZeroMemory(Parameters, ParamSize);
    Parameters->Header.Type = NDIS_OBJECT_TYPE_QOS_PARAMETERS;
    Parameters->Header.Revision = NDIS_QOS_PARAMETERS_REVISION_1;
    Parameters->Header.Size = (USHORT)FirstElementOffset;
    Parameters->Flags = NDIS_QOS_PARAMETERS_ETS_CONFIGURED |
                        NDIS_QOS_PARAMETERS_PFC_CONFIGURED |
                        NDIS_QOS_PARAMETERS_CLASSIFICATION_CONFIGURED;
    Parameters->NumTrafficClasses = NumTrafficClasses;
    NdisMoveMemory(
        Parameters->PriorityAssignmentTable,
        PriorityAssignments,
        sizeof(Parameters->PriorityAssignmentTable));
    NdisMoveMemory(
        Parameters->TcBandwidthAssignmentTable,
        TcBandwidthAssignments,
        sizeof(Parameters->TcBandwidthAssignmentTable));
    NdisMoveMemory(
        Parameters->TsaAssignmentTable,
        TsaAssignments,
        sizeof(Parameters->TsaAssignmentTable));
    Parameters->PfcEnable = PfcEnable;
    Parameters->ClassificationElementSize = sizeof(NDIS_QOS_CLASSIFICATION_ELEMENT);
    Parameters->FirstClassificationElementOffset = FirstElementOffset;

    *MaxParamsSize = ParamSize;

    return Parameters;
}

_IRQL_requires_max_(DISPATCH_LEVEL)
PNDIS_QOS_PARAMETERS
CreateDefaultOperationalParameters(
    _In_ ULONG NumClassificationElements,
    _Deref_out_range_(>=, sizeof(NDIS_QOS_PARAMETERS)) ULONG *MaxParamsSize
    )
/*++
Routine Description:

    This routine creates and initializes the default operational parameters, which is often implementation-
    dependent.

Remarks:

    This example shows a simple case where a default set of PFC and ETS parameters are defined. When the
    respective NDIS_QOS_PARAMETERS_ETS_CONFIGURED and NDIS_QOS_PARAMETERS_PFC_CONFIGURED flags are not set
    in the NDIS_QOS_PARAMETERS structure given during OID_QOS_PARAMETERS, the default parameters will be
    used instead.

--*/
{
    return
        CreateParameters(
            NIC_SUPPORTED_NUM_TCS,
            DefaultQOSPriorityAssignmentTable,
            DefaultQOSTcBandwidthAssignmentTable,
            DefaultQOSTsaAssignmentTable,
            0,                              // Flow control disabled on all priorities by default
            NumClassificationElements,
            MaxParamsSize);
}

_IRQL_requires_max_(DISPATCH_LEVEL)
VOID
AddNewClassification(
    _Inout_updates_bytes_to_(MaxParamsSize, Parameters->Header.Size) PNDIS_QOS_PARAMETERS Parameters,
    _In_range_(>=, sizeof(NDIS_QOS_PARAMETERS)) ULONG MaxParamsSize,
    _In_ USHORT ConditionSelector,
    _In_ USHORT ConditionField,
    _In_ USHORT ActionSelector,
    _In_ USHORT ActionField
    )
/*++
Routine Description:

    This routine adds a new NDIS_QOS_CLASSIFICATION_ELEMENT to an existing NDIS_QOS_PARAMETERS created
    using the CreateParameters routine.

Remarks:

    The NDIS_QOS_PARAMETERS structure is assumed to have preallocated enough storage to accommodate
    all the new classification entries.

--*/
{
    PNDIS_QOS_CLASSIFICATION_ELEMENT Element;

    Element =
        (PNDIS_QOS_CLASSIFICATION_ELEMENT)
        ((PCHAR)Parameters +
                Parameters->FirstClassificationElementOffset +
                Parameters->ClassificationElementSize * Parameters->NumClassificationElements);

    if ((PCHAR)Element + Parameters->ClassificationElementSize <= (PCHAR)Parameters + MaxParamsSize)
    {
        Element->Header.Type = NDIS_OBJECT_TYPE_QOS_CLASSIFICATION_ELEMENT;
        Element->Header.Revision = NDIS_QOS_CLASSIFICATION_ELEMENT_REVISION_1;
        Element->Header.Size = NDIS_SIZEOF_QOS_CLASSIFICATION_ELEMENT_REVISION_1;
        Element->Flags = 0;
        Element->ConditionSelector = ConditionSelector;
        Element->ConditionField = ConditionField;
        Element->ActionSelector = ActionSelector;
        Element->ActionField = ActionField;

        NT_ASSERT(Parameters->ClassificationElementSize <= sizeof(*Element));
        _Analysis_assume_(Parameters->ClassificationElementSize <= sizeof(*Element));

        Parameters->Header.Size += sizeof(*Element);
        Parameters->NumClassificationElements++;
    }
    else
    {
        ASSERTMSG("Parameters buffer too small", 0);
    }
}

_IRQL_requires_max_(DISPATCH_LEVEL)
VOID
IndicateParameters(
    _Inout_ struct _MP_ADAPTER *Adapter,
    _In_ NDIS_STATUS StatusCode,
    _In_ PNDIS_QOS_PARAMETERS Parameters
    )
/*++
Routine Description:

    This routine indicates to NDIS a NDIS_QOS_PARAMETERS structure.

--*/
{
    NDIS_STATUS_INDICATION Status = { 0 };

    DEBUGP(MP_TRACE, "[%p] ---> IndicateParameters\n", Adapter);

    Status.Header.Type = NDIS_OBJECT_TYPE_STATUS_INDICATION;
    Status.Header.Revision = NDIS_STATUS_INDICATION_REVISION_1;
    Status.Header.Size = NDIS_SIZEOF_STATUS_INDICATION_REVISION_1;
    Status.StatusCode = StatusCode;
    Status.StatusBuffer = Parameters;
    Status.StatusBufferSize = Parameters->Header.Size;

    NdisMIndicateStatusEx(Adapter->AdapterHandle, &Status);

    DEBUGP(MP_TRACE, "[%p] <--- IndicateParameters\n", Adapter);
}

_IRQL_requires_(PASSIVE_LEVEL)
NDIS_STATUS
ReadQOSConfig(
    _In_ NDIS_HANDLE ConfigurationHandle,
    _Inout_ struct _MP_ADAPTER *Adapter)
/*++
Routine Description:

    This routine will read the QOS configuration from the NDIS registry, and set the results to the QOSData flags field.

Arguments:

    ConfigurationHandle     - Adapter configuration handle
    Adapter                 - Pointer to our adapter

Return Value:

    NDIS_STATUS

--*/
{
    NDIS_STATUS Status = NDIS_STATUS_SUCCESS;
    PNDIS_CONFIGURATION_PARAMETER Parameter;
    NDIS_STRING QOSKeyword = NDIS_STRING_CONST("*QOS");

    DEBUGP(MP_TRACE, "[%p] ---> ReadQOSConfig\n", Adapter);

    PAGED_CODE();

    do
    {
        //
        // Read the *QOS flag (whether QOS is enabled on the adapter).
        //
        NdisReadConfiguration(
                &Status,
                &Parameter,
                ConfigurationHandle,
                &QOSKeyword,
                NdisParameterInteger);
        if (Status != NDIS_STATUS_SUCCESS)
        {
            DEBUGP(MP_ERROR,
                   "[%p] NdisReadConfiguration for *QOS failed Status 0x%08x, defaulting to disabled.\n",
                   Adapter,
                   Status);
            Status = NDIS_STATUS_SUCCESS;
            break;
        }

        if (Parameter->ParameterData.IntegerData != 1)
        {
            break;
        }

        Adapter->QOSData.Flags |= fMP_QOS_ENABLED;

        //
        // Set up the current QOS capabilities. If QOS is enabled, the current
        // capabilities will normally match the hardware capabilities, i.e.
        // they are not configurable.
        //
        Adapter->QOSData.CurrentMaxNumTCs = NIC_SUPPORTED_NUM_TCS;
        Adapter->QOSData.CurrentMaxNumEtsCapableTCs = min(NIC_SUPPORTED_NUM_ETS_CAPABLE_TCS, Adapter->QOSData.CurrentMaxNumTCs);
        Adapter->QOSData.CurrentMaxNumPfcEnabledTCs = min(NIC_SUPPORTED_NUM_PFC_ENABLED_TCS, Adapter->QOSData.CurrentMaxNumTCs);

        Status = NDIS_STATUS_SUCCESS;
    } while (FALSE);

    DEBUGP(MP_TRACE, "[%p] <--- ReadQOSConfig Status 0x%08x\n", Adapter, Status);

    return Status;
}

_IRQL_requires_(PASSIVE_LEVEL)
NDIS_STATUS
SetQOSParameters(
    _Inout_ struct _MP_ADAPTER *Adapter,
    _In_ PNDIS_QOS_PARAMETERS Params)
/*++
Routine Description:

    This routine will configure the classification table and validate the ETS and PFC parameters.

Arguments:

    Adapter               - Pointer to our adapter
    Params                - Pointer to the new parameters

Return Value:

    NDIS_STATUS

--*/
{
    NDIS_STATUS Status = NDIS_STATUS_SUCCESS;
    PNDIS_QOS_PARAMETERS OperationalParams;
    ULONG MaxParamsSize;
    UCHAR iSCSIPriority = NIC_ISCSI_PRIORITY;
    UCHAR FCOEPriority = NIC_FCOE_PRIORITY;

    PAGED_CODE();

    DEBUGP(MP_TRACE, "[%p] ---> SetQOSParameters\n", Adapter);

    OperationalParams =
        CreateDefaultOperationalParameters(
            NIC_SUPPORTED_NUM_CLASSIFICATIONS,
            &MaxParamsSize);
    if (OperationalParams == NULL)
    {
        //
        // Unable to allocate operational parameters structure but this is not a deal breaker.
        // Proceed to service the incoming NDIS_QOS_PARAMETERS.
        //
        DEBUGP(MP_ERROR, "Failed to allocate default NDIS_QOS_PARAMETERS\n");
    }

    if ((Params->Flags & (NDIS_QOS_PARAMETERS_ETS_CONFIGURED |
                          NDIS_QOS_PARAMETERS_PFC_CONFIGURED |
                          NDIS_QOS_PARAMETERS_CLASSIFICATION_CONFIGURED)) != 0)
    {
        if ((Params->Flags & (NDIS_QOS_PARAMETERS_ETS_CONFIGURED | NDIS_QOS_PARAMETERS_PFC_CONFIGURED)) !=
                (NDIS_QOS_PARAMETERS_ETS_CONFIGURED | NDIS_QOS_PARAMETERS_PFC_CONFIGURED))
        {
            //
            // ETS and PFC must be configured together.
            //
            Status = NDIS_STATUS_NOT_SUPPORTED;
            goto Exit;
        }

        //
        // Enable ETS using the parameters in:
        //   - Params->PriorityAssignmentTable
        //   - Params->TcBandwidthAssignmentTable
        //   - Params->TsaAssignmentTable
        //
        // A real hardware-based implementation would program the ETS hardware here.
        //
        if (OperationalParams != NULL)
        {
            OperationalParams->NumTrafficClasses = Params->NumTrafficClasses;
            NdisMoveMemory(
                OperationalParams->PriorityAssignmentTable,
                Params->PriorityAssignmentTable,
                sizeof(Params->PriorityAssignmentTable));
            NdisMoveMemory(
                OperationalParams->TcBandwidthAssignmentTable,
                Params->TcBandwidthAssignmentTable,
                sizeof(Params->TcBandwidthAssignmentTable));
            NdisMoveMemory(
                OperationalParams->TsaAssignmentTable,
                Params->TsaAssignmentTable,
                sizeof(Params->TsaAssignmentTable));
        }

        //
        // Enable PFC using the bitmap in Params->PfcEnable
        //
        // A real hardware-based implementation would program the PFC hardware here.
        //
        if (OperationalParams != NULL)
        {
            OperationalParams->PfcEnable = Params->PfcEnable;
        }

        //
        // Process packet classification parameters.
        //
        iSCSIPriority = 0;
        FCOEPriority = 0;

        if (Params->Flags & NDIS_QOS_PARAMETERS_CLASSIFICATION_CONFIGURED)
        {
            //
            // Packet classification enabled. Search for a classification element that matches the
            // traffic type that your related hardware device or software driver (e.g. FCoE driver)
            // sends.
            //
            PNDIS_QOS_CLASSIFICATION_ELEMENT Entry;
            ULONG i;

            Entry = (PNDIS_QOS_CLASSIFICATION_ELEMENT)((PCHAR)Params + Params->FirstClassificationElementOffset);

            for (i = 0; i < Params->NumClassificationElements; i++)
            {
                NT_ASSERT((PCHAR)Entry + sizeof(*Entry) <= (PCHAR)Params + Params->Header.Size);
                _Analysis_assume_((PCHAR)Entry + sizeof(*Entry) <= (PCHAR)Params + Params->Header.Size);

                if (Entry->Header.Type == NDIS_OBJECT_TYPE_QOS_CLASSIFICATION_ELEMENT &&
                    Entry->Header.Revision == NDIS_QOS_CLASSIFICATION_ELEMENT_REVISION_1 &&
                    Entry->Header.Size >= NDIS_SIZEOF_QOS_CLASSIFICATION_ELEMENT_REVISION_1)
                {
                    NT_ASSERT((PCHAR)Entry + Entry->Header.Size <= (PCHAR)Params + Params->Header.Size);
                    _Analysis_assume_((PCHAR)Entry + Entry->Header.Size <= (PCHAR)Params + Params->Header.Size);

                    if (Entry->ConditionSelector == NDIS_QOS_CONDITION_ETHERTYPE &&
                        Entry->ConditionField == NIC_FCOE_ETHERTYPE &&
                        Entry->ActionSelector == NDIS_QOS_ACTION_PRIORITY)
                    {
                        //
                        // Found a classification entry for FCoE. FCoE packets that directly originate
                        // from the device or driver are to be tagged with the 802.1p priority value
                        // stored in Entry->ActionField.
                        //
                        Entry->Flags |= NDIS_QOS_CLASSIFICATION_ENFORCED_BY_MINIPORT;   // Acknowledge the classification
                        FCOEPriority = (UCHAR)Entry->ActionField;
                    }
                    else if (Entry->ConditionSelector == NDIS_QOS_CONDITION_TCP_PORT &&
                             Entry->ConditionField == NIC_ISCSI_TCP_PORT &&
                             Entry->ActionSelector == NDIS_QOS_ACTION_PRIORITY)
                    {
                        //
                        // Found a classification entry for iSCSI. iSCSI packets that directly originate
                        // from the device or driver are to be tagged with the 802.1p priority value
                        // stored in Entry->ActionField.
                        //
                        Entry->Flags |= NDIS_QOS_CLASSIFICATION_ENFORCED_BY_MINIPORT;   // Acknowledge the classification
                        iSCSIPriority = (UCHAR)Entry->ActionField;
                    }
                    else if (Entry->ConditionSelector == NDIS_QOS_CONDITION_DEFAULT &&
                             Entry->ActionSelector == NDIS_QOS_ACTION_PRIORITY)
                    {
                        //
                        // Found a default classification entry. This entry, if present, is always located
                        // at the start of the classification array.
                        //
                        NT_ASSERT(i == 0);
                        iSCSIPriority = FCOEPriority = (UCHAR)Entry->ActionField;
                    }
                }

                Entry = (PNDIS_QOS_CLASSIFICATION_ELEMENT)((PCHAR)Entry + Params->ClassificationElementSize);
            }
        }
    }
    else
    {
        //
        // ETS, PFC, and Classification settings not available. Use default.
        //
    }

    if (OperationalParams != NULL)
    {
        AddNewClassification(
            OperationalParams,
            MaxParamsSize,
            NDIS_QOS_CONDITION_ETHERTYPE,
            NIC_FCOE_ETHERTYPE,
            NDIS_QOS_ACTION_PRIORITY,
            FCOEPriority);

        AddNewClassification(
            OperationalParams,
            MaxParamsSize,
            NDIS_QOS_CONDITION_TCP_PORT,
            NIC_ISCSI_TCP_PORT,
            NDIS_QOS_ACTION_PRIORITY,
            iSCSIPriority);

        //
        // 'OperationalParams' contains the new operational parameters. An actual device will
        // operate on these settings.
        //
        IndicateParameters(Adapter, NDIS_STATUS_QOS_OPERATIONAL_PARAMETERS_CHANGE, OperationalParams);
    }

Exit:
    if (OperationalParams != NULL)
    {
        NdisFreeMemoryWithTagPriority(NdisDriverHandle, OperationalParams, NIC_TAG_QOS_PARAMS);
    }

    DEBUGP(MP_TRACE, "<--- [%p] SetQOSParameters Status 0x%08x\n", Adapter, Status);

    return Status;
}

_IRQL_requires_(PASSIVE_LEVEL)
NDIS_STATUS
InitializeQOSConfig(
    _Inout_ PMP_ADAPTER Adapter)
/*++
Routine Description:

    This routine will set the supported and actual QOS capabilities for the miniport.

Arguments:

    Adapter               - Pointer to our adapter

Return Value:

    NDIS_STATUS

--*/
{
    NDIS_STATUS Status;
    NDIS_MINIPORT_ADAPTER_HARDWARE_ASSIST_ATTRIBUTES HardwareAssistAttributes;
    NDIS_QOS_CAPABILITIES HwQosCapabilities;
    NDIS_QOS_CAPABILITIES CurrentQosCapabilities;
    PNDIS_QOS_PARAMETERS OperationalParams;
    ULONG MaxParamsSize;

    DEBUGP(MP_TRACE, "[%p] ---> InitializeQOSConfig\n", Adapter);

    PAGED_CODE();

    NdisZeroMemory(&HardwareAssistAttributes, sizeof(HardwareAssistAttributes));
    HardwareAssistAttributes.Header.Type = NDIS_OBJECT_TYPE_MINIPORT_ADAPTER_HARDWARE_ASSIST_ATTRIBUTES;
    HardwareAssistAttributes.Header.Revision = NDIS_MINIPORT_ADAPTER_HARDWARE_ASSIST_ATTRIBUTES_REVISION_3;
    HardwareAssistAttributes.Header.Size = NDIS_SIZEOF_MINIPORT_ADAPTER_HARDWARE_ASSIST_ATTRIBUTES_REVISION_3;

    //
    // Set NIC QOS capabilities.
    //
    NdisZeroMemory(&HwQosCapabilities, sizeof(HwQosCapabilities));
    HwQosCapabilities.Header.Type = NDIS_OBJECT_TYPE_QOS_CAPABILITIES;
    HwQosCapabilities.Header.Revision = NDIS_QOS_CAPABILITIES_REVISION_1;
    HwQosCapabilities.Header.Size = NDIS_SIZEOF_QOS_CAPABILITIES_REVISION_1;

    HwQosCapabilities.Flags = NDIS_QOS_CAPABILITIES_STRICT_TSA_SUPPORTED;
    HwQosCapabilities.MaxNumTrafficClasses = NIC_SUPPORTED_NUM_TCS;
    HwQosCapabilities.MaxNumEtsCapableTrafficClasses = NIC_SUPPORTED_NUM_ETS_CAPABLE_TCS;
    HwQosCapabilities.MaxNumPfcEnabledTrafficClasses = NIC_SUPPORTED_NUM_PFC_ENABLED_TCS;

    HardwareAssistAttributes.HardwareQosCapabilities = &HwQosCapabilities;

    //
    // Set the actual NIC QOS capabilities.
    //
    NdisZeroMemory(&CurrentQosCapabilities, sizeof(CurrentQosCapabilities));
    CurrentQosCapabilities.Header.Type = NDIS_OBJECT_TYPE_QOS_CAPABILITIES;
    CurrentQosCapabilities.Header.Revision = NDIS_QOS_CAPABILITIES_REVISION_1;
    CurrentQosCapabilities.Header.Size = NDIS_SIZEOF_QOS_CAPABILITIES_REVISION_1;

    if (QOS_ENABLED(Adapter))
    {
        //
        // Note: If QOS is disabled, all the following fields are set to 0.
        //
        CurrentQosCapabilities.Flags = NDIS_QOS_CAPABILITIES_STRICT_TSA_SUPPORTED;
        CurrentQosCapabilities.MaxNumTrafficClasses = Adapter->QOSData.CurrentMaxNumTCs;
        CurrentQosCapabilities.MaxNumEtsCapableTrafficClasses = Adapter->QOSData.CurrentMaxNumEtsCapableTCs;
        CurrentQosCapabilities.MaxNumPfcEnabledTrafficClasses = Adapter->QOSData.CurrentMaxNumPfcEnabledTCs;
    }

    HardwareAssistAttributes.CurrentQosCapabilities = &CurrentQosCapabilities;

    Status = NdisMSetMiniportAttributes(
                Adapter->AdapterHandle,
                (PNDIS_MINIPORT_ADAPTER_ATTRIBUTES)&HardwareAssistAttributes);
    if (NDIS_STATUS_SUCCESS != Status)
    {
        DEBUGP(MP_ERROR, "[%p] NdisMSetMiniportAttributes Status 0x%08x\n", Adapter, Status);
    }

    //
    // At this point, we can indicate up the default operational parameters.
    //
    if (QOS_ENABLED(Adapter))
    {
        OperationalParams = CreateDefaultOperationalParameters(NIC_SUPPORTED_NUM_CLASSIFICATIONS, &MaxParamsSize);

        if (OperationalParams != NULL)
        {
            //
            // Add default FCoE classification.
            //
            AddNewClassification(
                OperationalParams,
                MaxParamsSize,
                NDIS_QOS_CONDITION_ETHERTYPE,
                NIC_FCOE_ETHERTYPE,
                NDIS_QOS_ACTION_PRIORITY,
                NIC_FCOE_PRIORITY);

            //
            // Add default iSCSI classification.
            //
            AddNewClassification(
                OperationalParams,
                MaxParamsSize,
                NDIS_QOS_CONDITION_TCP_PORT,
                NIC_ISCSI_TCP_PORT,
                NDIS_QOS_ACTION_PRIORITY,
                NIC_ISCSI_PRIORITY);

            IndicateParameters(Adapter, NDIS_STATUS_QOS_OPERATIONAL_PARAMETERS_CHANGE, OperationalParams);

        
            NdisFreeMemoryWithTagPriority(NdisDriverHandle, OperationalParams, NIC_TAG_QOS_PARAMS);
        }
        else
        {
            DEBUGP(MP_ERROR, "Failed to allocate default NDIS_QOS_PARAMETERS\n");
        }
    }

    DEBUGP(MP_TRACE, "<--- [%p] InitializeQOSConfig Status 0x%08x\n", Adapter, Status);

    return Status;
}

