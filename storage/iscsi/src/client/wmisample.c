/*++

Copyright (C) Microsoft Corporation, 2000

Module Name:

    wmi.c

Abstract:

    This file contains WMI routines for iSCSI miniport driver.

Environment:

    kernel mode only

--*/


UCHAR iScsiQuerySecurityCapabilities(
    PISCSI_ADAPTER_EXTENSION AdapterExtension,
    ULONG BufferAvail,
    PUCHAR Buffer,
    PULONG InstanceLength,
    PULONG SizeNeeded
    );

UCHAR iScsiQueryDiscoveryConfig(
    PISCSI_ADAPTER_EXTENSION AdapterExtension,
    ULONG BufferAvail,
    PUCHAR Buffer,
    PULONG InstanceLength,
    PULONG SizeNeeded
    );

UCHAR iScsiAdvanceString(
    OUT PWCHAR *String,
    OUT PUSHORT StringLen,
    IN OUT PUCHAR *Buffer,
    IN OUT PULONG BufferSize
    );

UCHAR iScsiSetDiscoveryConfig(
    PISCSI_ADAPTER_EXTENSION AdapterExtension,
    ULONG BufferSize,
    PUCHAR Buffer
    );

UCHAR iScsiAddPortalGroup(
    IN OUT PUCHAR *Buffer,
    IN OUT PULONG SizeLeft,
    IN OUT PULONG SizeNeeded,
    IN ULONG PortalCount
    );

VOID iScsiCopyUTF8ToCounted(
    PWCHAR p,
    PUTF8CHAR UTF8,
    ULONG UTF8Len
    );

UCHAR iScsiAddPortalToGroup(
    IN OUT PUCHAR *Buffer,
    IN OUT PULONG SizeLeft,
    IN OUT PULONG SizeNeeded,
    IN ULONG Address,
    IN PUTF8CHAR SymbolicName,
    IN USHORT Socket,
    IN ULONG SecurityBitmap,
    IN ULONG KeySize,
    IN PUCHAR Key
    );

UCHAR iScsiAddDiscoveredTarget(
    IN OUT PUCHAR *Buffer,
    IN OUT PULONG SizeLeft,
    IN OUT PULONG SizeNeeded,
    IN PUTF8CHAR Name,
    IN PUTF8CHAR Alias,
    IN PUTF8CHAR DDName,
    IN ULONG DDID,
    IN ULONG PortalGroupCount
    );

UCHAR iScsiReportDiscoveredTargets2(
    PISCSI_ADAPTER_EXTENSION AdapterExtension,
    ULONG BufferSize,
    PULONG SizeNeeded,
    PUCHAR Buffer
    );

UCHAR
iSpReadTCPConfigInfo(
    IN PISCSI_ADAPTER_EXTENSION AdapterExtension,
    OUT PUCHAR Buffer
   );

UCHAR
iSpReadNICConfigInfo(
    IN PISCSI_ADAPTER_EXTENSION AdapterExtension,
    OUT PUCHAR Buffer
   );

UCHAR
iSpReadNICPerfData(
    IN PISCSI_ADAPTER_EXTENSION AdapterExtension,
    OUT PUCHAR Buffer
   );

UCHAR
iSpSetTCPConfigInfo(
    IN PISCSI_ADAPTER_EXTENSION AdapterExtension,
    IN ULONG BufferSize,
    IN PUCHAR Buffer
    );

UCHAR
iScsiSetWmiDataBlock(
    IN PVOID Context,
    IN PSCSIWMI_REQUEST_CONTEXT DispatchConiScsiQueryWmiDataBlocktext,
    IN ULONG GuidIndex,
    IN ULONG InstanceIndex,
    IN ULONG BufferSize,
    IN PUCHAR Buffer
    );


GUID iSCSI_Operations_GUID = MSiSCSI_OperationsGuid;
GUID iSCSI_HBAInformation_GUID = MSiSCSI_HBAInformationGuid;
GUID iScsiInitiatorInstanceStatisticsGuid = MSiSCSI_InitiatorInstanceStatisticsGuid;
GUID iScsiInitiatorInstanceFailureEventGuid = MSiSCSI_InitiatorInstanceFailureEventGuid;
GUID iScsiConnectStatisticsGuid = MSiSCSI_ConnectionStatisticsGuid;
GUID iScsiSessionStatisticsGuid = MSiSCSI_SessionStatisticsGuid;
GUID iScsi_InitiatorSessionInfo_GUID = MSiSCSI_InitiatorSessionInfoGuid;
GUID iScsi_InitiatorLoginStatistics_GUID = MSiSCSI_InitiatorLoginStatisticsGuid;
GUID iScsiInitiatorNodeFailureEventGuid = MSiSCSI_InitiatorInstanceFailureEventGuid;
GUID iScsiAdapterEventGuid = MSiSCSI_AdapterEventGuid;
GUID iScsiPortalInfoClassGuid = MSiSCSI_PortalInfoClassGuid;
GUID iScsiTargetMappingClassGuid = MSiSCSI_TargetMappingsGuid;
GUID iScsiPersistentLoginsClassGuid = MSiSCSI_PersistentLoginsGuid;
GUID iScsiLBOperationsGuid = MSiSCSI_LB_OperationsGuid;
GUID iScsiQueryLBPolicyGuid = MSiSCSI_QueryLBPolicyGuid;
GUID iSCSIEventlogGuid = MSiSCSI_EventlogGuid;
GUID iSCSIRedirectPortalGuid = MSiSCSI_RedirectPortalInfoClassGuid;
GUID iSCSIReqProcTimeGuid = MSiSCSI_RequestTimeStatisticsGuid;
GUID iSCSIBootInfoGuid = MSiSCSI_BootInformationGuid;

GUID iScsiDiscoveryConfigGuid = MSiSCSI_DiscoveryConfigGuid;
GUID iSCSI_DiscoveryOperationsGuid = MSiSCSI_DiscoveryOperationsGuid;
GUID iSCSI_SecurityCapabilitiesGuid = MSiSCSI_SecurityCapabilitiesGuid;
GUID iSCSI_SecurityConfigOperationsGuid = MSiSCSI_SecurityConfigOperationsGuid;
GUID MSiScsiTCPConfigGuid = MSiSCSI_TCPIPConfigGuid;
GUID MSiScsiNICConfigGuid = MSiSCSI_NICConfigGuid;
GUID MSiScsiNICPerfGuid = MSiSCSI_NICPerformanceGuid;
GUID MSiSCSIManagementOperationsGuid= MSiSCSI_ManagementOperationsGuid;

//
// This GUID will be replaced with the GUID for WMI Tracing in the port driver
//
GUID iSCSIDummyGuid = { 0,0,0, { 0,0,0,0,0,0,0,0 } };

//
// The index numbers should correspond to the offset in
// iScsiWmiGuidList array given below.
//
#define iSCSI_Operations_GUID_Index                        0
#define iSCSI_HBAInformation_GUID_Index                    1
#define iScsi_InitiatorInstanceStatistics_GUID_INDEX       2
#define iScsi_InitiatorInstanceFailureEvent_GUID_INDEX     3
#define iScsi_ConnectStatistics_GUID_INDEX                 4
#define iScsi_SessionStatistics_GUID_INDEX                 5
#define iScsi_InitiatorSessionInfo_GUID_INDEX              6
#define iScsi_InitiatorLoginStatistics_GUID_INDEX          7
#define iScsi_InitiatorNodeFailureEvent_GUID_INDEX         8
#define iScsi_AdapterEvent_GUID_Index                      9
#define iScsi_PortalInfoClass_GUID_Index                   10
#define iScsi_TargetMapping_GUID_Index                     11
#define iScsi_PersistentLogin_GUID_Index                   12
#define iScsi_LB_Operations_GUID_Index                     13
#define iScsi_Query_LB_Policy_GUID_Index                   14
#define iScsi_Eventlog_GUID_Index                          15
#define iScsi_Redirect_Portal_GUID_Index                   16
#define iScsi_RequestProcessingTime_GUID_Index             17
#define iScsi_BootInfo_GUID_Index                          18

#define iScsi_MAX_GUID_INDEX                               iScsi_BootInfo_GUID_Index

#define iScsiDiscoveryConfigGuid_INDEX                     iScsi_MAX_GUID_INDEX + 1
#define iSCSI_DiscoveryOperationsGuid_INDEX                iScsi_MAX_GUID_INDEX + 2
#define iSCSI_SecurityCapabilitiesGuid_INDEX               iScsi_MAX_GUID_INDEX + 3
#define iSCSI_SecurityConfigOperationsGuid_INDEX           iScsi_MAX_GUID_INDEX + 4
#define iScsi_TCPConfig_GUID_Index                         iScsi_MAX_GUID_INDEX + 5
#define iScsi_NICConfig_GUID_Index                         iScsi_MAX_GUID_INDEX + 6
#define iScsi_NICPerf_GUID_Index                           iScsi_MAX_GUID_INDEX + 7
#define iScsi_ManagementOperation_GUID_Index               iScsi_MAX_GUID_INDEX + 8

//
// GUID List and number of GUIDs in the list
//
SCSIWMIGUIDREGINFO iScsiWmiGuidList[] =
{
    {
        &iSCSI_Operations_GUID,
        1,
        0
    },

    {
        &iSCSI_HBAInformation_GUID,
        1,
        0
    },

    {
        &iScsiInitiatorInstanceStatisticsGuid,
        1,
        0
    },

    {
        &iScsiInitiatorInstanceFailureEventGuid,
        1,
        WMIREG_FLAG_EVENT_ONLY_GUID
    },

    {
        &iScsiConnectStatisticsGuid,
        0xffffffff, //dynamic instance names
        0
     },

    {
        &iScsiSessionStatisticsGuid,
        0xffffffff, //dynamic instance names
        0
    },

    {
        &iScsi_InitiatorSessionInfo_GUID,
        1,
        0
    },

    {
        &iScsi_InitiatorLoginStatistics_GUID,
        1,
        0
    },

    {
        &iScsiInitiatorNodeFailureEventGuid,
        1,
        WMIREG_FLAG_EVENT_ONLY_GUID
    },

    {
        &iScsiAdapterEventGuid,
        1,
        WMIREG_FLAG_EVENT_ONLY_GUID
    },

    {
        &iScsiPortalInfoClassGuid,
        1,
        0
    },

    {
        &iScsiTargetMappingClassGuid,
        1,
        0
    },

    {
        &iScsiPersistentLoginsClassGuid,
        1,
        0
    },

    {
        &iScsiLBOperationsGuid,
        1,
        0
    },

    {
        &iScsiQueryLBPolicyGuid,
        1,
        0
    },

    {
        &iSCSIEventlogGuid,
        1,
        0
    },

    {
        &iSCSIRedirectPortalGuid,
        1,
        0
    },

    {
        &iSCSIReqProcTimeGuid,
        0xffffffff, //dynamic instance names
        0
    },

    {
        &iSCSIBootInfoGuid,
        1,
        0
    },

    {
        &iScsiDiscoveryConfigGuid,
        1,
        0
    },

    {
        &iSCSI_DiscoveryOperationsGuid,
        1,
        0
    },

    {
        &iSCSI_SecurityCapabilitiesGuid,
        1,
        0
    },

    {
        &iSCSI_SecurityConfigOperationsGuid,
        1,
        0
    },

    {
        &MSiScsiTCPConfigGuid,
        1,
        0
    },

    {
        &MSiScsiNICConfigGuid,
        1,
        0
    },

    {
        &MSiScsiNICPerfGuid,
        1,
        0
    },

    {
        &MSiSCSIManagementOperationsGuid,
        1,
        0
    },

};

#define iScsiWmiGuidCount (sizeof(iScsiWmiGuidList) / sizeof(SCSIWMIGUIDREGINFO))

//
// WMI Guid list for the PDO
//
GUID iSCSI_LUNMappingInformationGuid = MSiSCSI_LUNMappingInformationGuid;

#define MSiSCSI_LUNMappingInformation_Index 0

SCSIWMIGUIDREGINFO iScsiPDOWmiGuidList[] =
{
    {
        &iSCSI_LUNMappingInformationGuid,
        1,
        0
    }
};

#define iScsiPDOWmiGuidCount (sizeof(iScsiPDOWmiGuidList) / sizeof(SCSIWMIGUIDREGINFO))

VOID
iScsiWmiInitializeContext(
    IN PISCSI_ADAPTER_EXTENSION AdapterExtension
    )
/*+++

Routine Description:

    This routine will initialize the wmilib context structure with the
    guid list and the pointers to the wmilib callback functions.

Arguments:

    AdapterExtension - Adpater extension

Return Value:

    None.

--*/
{
    PSCSI_WMILIB_CONTEXT wmiLibContext;


    //
    // Initialize the wmilib context for the adapter
    //
    wmiLibContext = &(AdapterExtension->WmiLibContext);

    wmiLibContext->GuidList = iScsiWmiGuidList;
    wmiLibContext->GuidCount = iScsiWmiGuidCount;

    //
    // Set pointers to WMI callback routines
    //
    wmiLibContext->QueryWmiRegInfo = iScsiQueryWmiRegInfo;
    wmiLibContext->QueryWmiDataBlock = iScsiQueryWmiDataBlock;
    wmiLibContext->ExecuteWmiMethod = iScsiExecuteWmiMethod;
    wmiLibContext->WmiFunctionControl = NULL;
    wmiLibContext->SetWmiDataItem = NULL;


    wmiLibContext->SetWmiDataBlock = iScsiSetWmiDataBlock;

    //
    // Initialize discovery config
    //
    AdapterExtension->DiscoveryConfigFlags = (DISCOVERY_CONFIG_DO_DISCOVERY |
                                              DISCOVERY_CONFIG_FIND_SNS_AUTOMATICALLY);
    AdapterExtension->SNSServer[0] = 0;


    //
    // Initialize the wmilib context for the PDO
    //
    wmiLibContext = &(AdapterExtension->PDOWmiLibContext);

    wmiLibContext->GuidList = iScsiPDOWmiGuidList;
    wmiLibContext->GuidCount = iScsiPDOWmiGuidCount;

    //
    // Set pointers to WMI callback routines
    //
    wmiLibContext->QueryWmiRegInfo = iScsiQueryWmiRegInfo;
    wmiLibContext->QueryWmiDataBlock = iScsiPDOQueryWmiDataBlock;
    wmiLibContext->ExecuteWmiMethod = NULL;
    wmiLibContext->WmiFunctionControl = NULL;
    wmiLibContext->SetWmiDataItem = NULL;
    wmiLibContext->SetWmiDataBlock = NULL;

    return;
}

BOOLEAN
iScsiWmiSrb(
    IN     PISCSI_ADAPTER_EXTENSION    AdapterExtension,
    IN OUT PSCSI_WMI_REQUEST_BLOCK     Srb
    )
/*++

Routine Description:

   Called from StartIo routine to process an SRB_FUNCTION_WMI request.
   Main entry point for all WMI routines.

Arguments:

   AdapterExtension - ISCSI miniport driver's Adapter extension.

   Srb              - IO request packet.

Return Value:

   Always TRUE.

--*/
{
    PSCSIWMI_REQUEST_CONTEXT requestContext;
    PISCSI_SRB_EXTENSION srbExtension;
    BOOLEAN pending;
    BOOLEAN completeRequest = TRUE;
    BOOLEAN adapterRequest;


    //
    // Validate our assumptions.
    //
    NT_ASSERT(Srb->Function == SRB_FUNCTION_WMI);
    NT_ASSERT(Srb->Length == sizeof(SCSI_WMI_REQUEST_BLOCK));

    srbExtension = (PISCSI_SRB_EXTENSION) Srb->SrbExtension;

    requestContext = &(srbExtension->WmiRequestContext);

    //
    // Save the pointer to the SRB in UserContext
    // of SCSIWMI_REQUEST_CONTEXT
    //
    requestContext->UserContext = Srb;

    //
    // Check if the WMI SRB is targetted for
    // the iScsi adapter or one of the devices
    //
    adapterRequest = (Srb->WMIFlags & SRB_WMI_FLAGS_ADAPTER_REQUEST) == SRB_WMI_FLAGS_ADAPTER_REQUEST;

    //
    // Process the incoming WMI request.
    //
    DebugPrint((iScsiPrtDebugTrace,
        "Entering iScsiWmiSrb\n"));

    ETWDebugPrint(iScsiLevelTrace,
        iScsiFlagWmi,
        "Entering iScsiWmiSrb\n");

    pending = ScsiPortWmiDispatchFunction(adapterRequest ?
                                             &AdapterExtension->WmiLibContext :
                                             &AdapterExtension->PDOWmiLibContext,
                                          Srb->WMISubFunction,
                                          AdapterExtension,
                                          requestContext,
                                          Srb->DataPath,
                                          Srb->DataTransferLength,
                                          Srb->DataBuffer);
    if (pending == FALSE) {

        //
        // We can do this since we assume it is done synchronously
        //
        Srb->DataTransferLength = ScsiPortWmiGetReturnSize(requestContext);;

        //
        // Adapter ready for next request.
        //
        Srb->SrbStatus = ScsiPortWmiGetReturnStatus(requestContext);

    }  else {
        completeRequest = FALSE;
    }

    if (completeRequest == TRUE) {
        ScsiPortNotification(RequestComplete, AdapterExtension, Srb);
    }

    return TRUE;
}

UCHAR
iScsiQueryWmiRegInfo(
    IN PVOID Context,
    IN PSCSIWMI_REQUEST_CONTEXT RequestContext,
    _Out_ PWCHAR *MofResourceName
    )
/*+++

Routine Description:

    This routine returns MofResourceName for this driver.

--*/
{
    *MofResourceName = iScsiWmi_MofResourceName;

    return SRB_STATUS_SUCCESS;
}

BOOLEAN
iScsiQueryWmiDataBlock(
    IN PVOID Context,
    IN PSCSIWMI_REQUEST_CONTEXT DispatchContext,
    IN ULONG GuidIndex,
    IN ULONG InstanceIndex,
    IN ULONG InstanceCount,
    IN OUT PULONG InstanceLengthArray,
    IN ULONG BufferAvail,
    OUT PUCHAR Buffer
    )
/*+++

Routine Description :

    Called to query WMI Data blocks

--*/
{
    PISCSI_ADAPTER_EXTENSION adapterExtension;
    PSCSI_WMI_REQUEST_BLOCK  srb;
    UCHAR status;
    ULONG sizeNeeded = 0;
    NTSTATUS ntStatus;


    adapterExtension = (PISCSI_ADAPTER_EXTENSION) Context;
    srb = (PSCSI_WMI_REQUEST_BLOCK) DispatchContext->UserContext;

    switch (GuidIndex) {
        case iSCSI_Operations_GUID_Index:
        case iScsi_LB_Operations_GUID_Index: {

            //
            // Even though this class only has methods, we need to
            // respond to any queries for it since WMI expects that
            // there is an actual instance of the class on which to
            // execute the method
            //

            sizeNeeded = sizeof(ULONG);
            if (BufferAvail >= sizeNeeded) {
                *InstanceLengthArray = sizeNeeded;
                status = SRB_STATUS_SUCCESS;
            } else {
                status = SRB_STATUS_DATA_OVERRUN;
            }

            break;
        }

        case iSCSI_HBAInformation_GUID_Index: {

            PMSiSCSI_HBAInformation iScsiHBAInformation;

            sizeNeeded = sizeof(MSiSCSI_HBAInformation);
            if (BufferAvail >= sizeNeeded) {

                *InstanceLengthArray = sizeNeeded;

                iScsiHBAInformation = (PMSiSCSI_HBAInformation) Buffer;

                ntStatus = iSpBuildHBAInformationBuffer(adapterExtension, iScsiHBAInformation);

                status = NT_SUCCESS(ntStatus) ? SRB_STATUS_SUCCESS : SRB_STATUS_ERROR;

            } else {

                status = SRB_STATUS_DATA_OVERRUN;
            }

            break;
        }

        case iScsi_InitiatorInstanceStatistics_GUID_INDEX: {

            sizeNeeded = sizeof(MSiSCSI_InitiatorInstanceStatistics);

            if (BufferAvail >= sizeNeeded) {

                status =  iSpReadInitiatorInstanceStatistics(adapterExtension,
                                                             Buffer);

                if (status != SRB_STATUS_ERROR) {
                    *InstanceLengthArray = sizeNeeded;

                    status = SRB_STATUS_SUCCESS;
                }
            } else {
                status = SRB_STATUS_DATA_OVERRUN;
            }

            break;
        }

        case iScsi_ConnectStatistics_GUID_INDEX: {

            status = iSpBuildConnectionStatistics(adapterExtension,
                                                  DispatchContext,
                                                  GuidIndex,
                                                  InstanceIndex,
                                                  InstanceCount,
                                                  InstanceLengthArray,
                                                  BufferAvail,
                                                  Buffer,
                                                  &sizeNeeded);

            break;
        }

        case iScsi_RequestProcessingTime_GUID_Index: {

            status = iSpBuildRequestTimeStatistics(adapterExtension,
                                                   DispatchContext,
                                                   GuidIndex,
                                                   InstanceIndex,
                                                   InstanceCount,
                                                   InstanceLengthArray,
                                                   BufferAvail,
                                                   Buffer,
                                                   &sizeNeeded);

            break;
        }

        case iScsi_SessionStatistics_GUID_INDEX: {


            status = iSpBuildSessionStatistics(adapterExtension,
                                               DispatchContext,
                                               GuidIndex,
                                               InstanceIndex,
                                               InstanceCount,
                                               InstanceLengthArray,
                                               BufferAvail,
                                               Buffer,
                                               &sizeNeeded);
            break;
        }

        case iScsi_InitiatorLoginStatistics_GUID_INDEX: {
            PWMI_CONNECTION_LIST connectionList;
            ULONG numberOfSessions;
            ULONG numberOfConnections;

            sizeNeeded = sizeof(MSiSCSI_InitiatorLoginStatistics);
            if (BufferAvail >= sizeNeeded)
            {
                RtlZeroMemory(Buffer, sizeNeeded);

                connectionList = iSpGetConnectionList(adapterExtension,
                                                      &numberOfSessions,
                                                      &numberOfConnections);

                if (connectionList != NULL)
                {
                    status = iSpReadInitiatorLoginStatistics(adapterExtension,
                                                            connectionList,
                                                            numberOfSessions,
                                                            Buffer);

                    *InstanceLengthArray = sizeNeeded;

                    iSpReleaseConnectionReferences(connectionList,
                                                   numberOfSessions);

                } else {
                    status = SRB_STATUS_ERROR;
                }


            } else {
                status = SRB_STATUS_DATA_OVERRUN;
            }
            break;
        }

        case iScsi_InitiatorSessionInfo_GUID_INDEX: {

            status = iSpBuildInitiatorSessionInfo(adapterExtension,
                                                 DispatchContext,
                                                 BufferAvail,
                                                 Buffer,
                                                 &sizeNeeded);

            *InstanceLengthArray = sizeNeeded;

            break;

        }

        case iScsi_PortalInfoClass_GUID_Index: {
            status = iSpBuildPortalInfo(adapterExtension,
                                        InstanceLengthArray,
                                        BufferAvail,
                                        Buffer,
                                        &sizeNeeded);
            break;
        }

        case iScsi_TargetMapping_GUID_Index: {

            status = iSpBuildTargetMapping(adapterExtension,
                                           InstanceLengthArray,
                                           BufferAvail,
                                           Buffer,
                                           &sizeNeeded);

            break;
        }

        case iScsi_PersistentLogin_GUID_Index: {

            status = iSpBuildPersistentLoginInfo(adapterExtension,
                                                 InstanceLengthArray,
                                                 BufferAvail,
                                                 Buffer,
                                                 &sizeNeeded);
            *InstanceLengthArray = sizeNeeded;

            break;
        }

        case iScsi_Query_LB_Policy_GUID_Index: {

            status = iSpQueryLBPolicy(adapterExtension,
                                      InstanceLengthArray,
                                      BufferAvail,
                                      Buffer,
                                      &sizeNeeded);

            *InstanceLengthArray = sizeNeeded;

            break;
        }

        case iScsi_Redirect_Portal_GUID_Index: {

            status = iSpBuildRedirectTargetInfo(adapterExtension,
                                                InstanceLengthArray,
                                                BufferAvail,
                                                Buffer,
                                                &sizeNeeded);

            *InstanceLengthArray = sizeNeeded;

            break;
        }

        case iScsi_BootInfo_GUID_Index: {

            status = iSpBuildBootInfo(adapterExtension,
                                      InstanceLengthArray,
                                      BufferAvail,
                                      Buffer,
                                      &sizeNeeded);

            *InstanceLengthArray = sizeNeeded;

            break;
        }



        case iSCSI_SecurityCapabilitiesGuid_INDEX:
        {
            status = iScsiQuerySecurityCapabilities(adapterExtension,
                                              BufferAvail,
                                              Buffer,
                                              InstanceLengthArray,
                                              &sizeNeeded);
            break;
        }

        case iScsiDiscoveryConfigGuid_INDEX:
        {
            status = iScsiQueryDiscoveryConfig(adapterExtension,
                                               BufferAvail,
                                               Buffer,
                                               InstanceLengthArray,
                                               &sizeNeeded);
            break;
        }

        case iSCSI_SecurityConfigOperationsGuid_INDEX:
        case iSCSI_DiscoveryOperationsGuid_INDEX:
        case iScsi_ManagementOperation_GUID_Index:
        {
            //
            // Operations only has methods, but we need to support the
            // query since WMI needs to be able to query before
            // executing a method
            //
            sizeNeeded = sizeof(ULONG);
            if (BufferAvail >= sizeNeeded)
            {
                *InstanceLengthArray = sizeNeeded;
                status = SRB_STATUS_SUCCESS;
            } else {
                status = SRB_STATUS_DATA_OVERRUN;
            }
            break;
        }

        case iScsi_TCPConfig_GUID_Index:
        {
          sizeNeeded = sizeof(MSiSCSI_TCPIPConfig);
          if (BufferAvail >= sizeNeeded) {
              *InstanceLengthArray = sizeNeeded;
              status = iSpReadTCPConfigInfo(adapterExtension, Buffer);
          } else {
              status = SRB_STATUS_DATA_OVERRUN;
          }

          break;
        }

        case iScsi_NICConfig_GUID_Index:
        {
          sizeNeeded = sizeof(MSiSCSI_NICConfig);

          if (BufferAvail >= sizeNeeded)
          {
              *InstanceLengthArray = sizeNeeded;
              status = iSpReadNICConfigInfo(adapterExtension, Buffer);

          } else {

              status = SRB_STATUS_DATA_OVERRUN;
          }

          break;
        }

        case iScsi_NICPerf_GUID_Index:
        {
          sizeNeeded = sizeof(MSiSCSI_NICPerformance);

          if (BufferAvail >= sizeNeeded)
          {
              *InstanceLengthArray = sizeNeeded;
              status = iSpReadNICPerfData(adapterExtension, Buffer);

          } else {

              status = SRB_STATUS_DATA_OVERRUN;
          }

          break;
        }

        default: {
            status = SRB_STATUS_ERROR;
            break;
        }

    }

    iSpCompleteWmiRequest(adapterExtension, srb, DispatchContext, status, sizeNeeded);

    //
    // return SRB_STATUS_PENDING since the request has either already been
    // completed in iSpCompleteWmiRequest or the status is really
    // SRB_STATUS_PENDING. This implies that the iScsiWmiSrb routine
    // may not touch the srb since it is already completed.
    //
    return SRB_STATUS_PENDING;

}

VOID
iScsiBuildLUNMapping(
    PISCSI_ADAPTER_EXTENSION AdapterExtension,
    PMSiSCSI_LUNMappingInformation Mapping,
    PSCSI_WMI_REQUEST_BLOCK Srb
    )
/*+++

Routine Description :

    This routine will fill in the target mapping information for a
    particular lun

--*/
{
    PISCSI_SESSION iScsiSession;


    iScsiSession = AdapterExtension->SessionLookupTable[Srb->TargetId];
    if (IsSessionValid(iScsiSession))
    {
        Mapping->UniqueSessionId = iScsiSession->SessionId;
    }


    Mapping->UniqueAdapterId = (ULONGLONG) AdapterExtension;
    Mapping->OSBus = Srb->PathId;
    Mapping->OSTarget = Srb->TargetId;
    Mapping->OSLUN = Srb->Lun;
}

BOOLEAN
iScsiPDOQueryWmiDataBlock(
    IN PVOID Context,
    IN PSCSIWMI_REQUEST_CONTEXT DispatchContext,
    IN ULONG GuidIndex,
    IN ULONG InstanceIndex,
    IN ULONG InstanceCount,
    IN OUT PULONG InstanceLengthArray,
    IN ULONG BufferAvail,
    OUT PUCHAR Buffer
    )
/*+++

Routine Description :

    Called to query WMI Data blocks

--*/
{
    PISCSI_ADAPTER_EXTENSION adapterExtension;
    PSCSI_WMI_REQUEST_BLOCK  srb;
    UCHAR status;
    ULONG sizeNeeded = 0;
    PMSiSCSI_LUNMappingInformation mapping;


    adapterExtension = (PISCSI_ADAPTER_EXTENSION) Context;
    srb = (PSCSI_WMI_REQUEST_BLOCK) DispatchContext->UserContext;

    switch (GuidIndex) {
        case MSiSCSI_LUNMappingInformation_Index:
        {
          sizeNeeded = sizeof(MSiSCSI_LUNMappingInformation);

          if (BufferAvail >= sizeNeeded)
          {
              mapping = (PMSiSCSI_LUNMappingInformation)Buffer;

              iScsiBuildLUNMapping(adapterExtension, mapping, srb);

              *InstanceLengthArray = sizeNeeded;
              status = SRB_STATUS_SUCCESS;

          } else {

              status = SRB_STATUS_DATA_OVERRUN;
          }

          break;
        }

        default: {
            status = SRB_STATUS_ERROR;
            break;
        }

    }

    iSpCompleteWmiRequest(adapterExtension, srb, DispatchContext, status, sizeNeeded);

    //
    // return SRB_STATUS_PENDING since the request has either already been
    // completed in iSpCompleteWmiRequest or the status is really
    // SRB_STATUS_PENDING. This implies that the iScsiWmiSrb routine
    // may not touch the srb since it is already completed.
    //
    return SRB_STATUS_PENDING;

}



UCHAR
iSpBuildTargetMapping(
    IN PISCSI_ADAPTER_EXTENSION AdapterExtension,
    IN OUT PULONG InstanceLengthArray,
    IN ULONG BufferAvail,
    _Out_writes_bytes_(BufferAvail) OUT PUCHAR Buffer,
    OUT PULONG SizeNeeded
    )
{
    PISCSI_SESSION iScsiSession;
    PMSiSCSI_TargetMappings iSCSITargetMappings;
    PISCSI_TargetMapping    targetMapping;
    PISCSI_SESSION *sessionTable;
    NTSTATUS ntStatus;
    ULONG inx, jnx;
    ULONG targetCount;
    UCHAR status = SRB_STATUS_SUCCESS;

    *SizeNeeded = sizeof(MSiSCSI_TargetMappings);

    if (sessionTable != NULL) {

        NT_ASSERT(targetCount > 0);

        inx = 0;
        while (inx < targetCount) {

            iScsiSession = sessionTable[inx];
            if (iScsiSession != NULL) {

                ULONG lunCount = 0;

                jnx = 0;
                while (jnx < ISCSI_MAX_LOGICAL_UNITS) {

                    if (iScsiSession->TargetLUN[jnx] != ISCSI_INVALID_LUN) {
                        lunCount++;
                    }

                    jnx++;
                }

                *SizeNeeded += FIELD_OFFSET(ISCSI_TargetMapping, LUNList) +
                               (sizeof(ISCSI_LUNList) * lunCount);
            }

            inx++;
        }

        if (BufferAvail >= *SizeNeeded) {

            iSCSITargetMappings = (PMSiSCSI_TargetMappings) Buffer;

            RtlZeroMemory(iSCSITargetMappings, *SizeNeeded);

            iSCSITargetMappings->UniqueAdapterId = (ULONGLONG) AdapterExtension;

            iSCSITargetMappings->TargetMappingCount = targetCount;

            targetMapping = iSCSITargetMappings->TargetMappings;

            inx = 0;
            while (inx < targetCount) {

                iScsiSession = sessionTable[inx];
                if (iScsiSession != NULL) {

                    ULONG targetMappingSize;

                    targetMappingSize =
                        FIELD_OFFSET(ISCSI_TargetMapping, LUNList);

                    //
                    // Size needed is computed above and it is checked above
                    // that the buffer size provided to the function is greater
                    // than the size needed to fill the buffer
                    //
                    targetMapping->OSBus = 0;

                    targetMapping->OSTarget = iScsiSession->OSTargetId;

                    targetMapping->UniqueSessionId = iScsiSession->SessionId;

                    targetMapping->FromPersistentLogin  = iScsiSession->PersistentLogin;

                    targetMapping->LUNCount = 0;

                    jnx = 0;
                    while (jnx < ISCSI_MAX_LOGICAL_UNITS) {

                        if (iScsiSession->TargetLUN[jnx] != ISCSI_INVALID_LUN) {

#pragma prefast ( suppress: __WARNING_POTENTIAL_BUFFER_OVERFLOW_HIGH_PRIORITY, "BufferAvail is being checked against SizeNeeded above")
							targetMapping->LUNList[targetMapping->LUNCount].OSLUN =
                                jnx;

                            targetMapping->LUNList[targetMapping->LUNCount].TargetLUN =
                                iScsiSession->TargetLUN[jnx];

                            targetMapping->LUNCount++;

                            targetMappingSize += sizeof(ISCSI_LUNList);
                        }

                        jnx++;
                    }

                    *(targetMapping->TargetName) = MAX_TARGET_NAME * sizeof(WCHAR);
                    RtlCopyMemory((targetMapping->TargetName + 1),
                                  iScsiSession->TargetName,
                                  (MAX_TARGET_NAME * sizeof(WCHAR)));

                    targetMapping =
                        (PISCSI_TargetMapping) (((PUCHAR)targetMapping) +
                                                  targetMappingSize);
                }

                inx++;
            }

            status = SRB_STATUS_SUCCESS;

        } else {
            status = SRB_STATUS_DATA_OVERRUN;
        }

    } else {
        if (BufferAvail >= *SizeNeeded) {
            RtlZeroMemory(Buffer, BufferAvail);
            status = SRB_STATUS_SUCCESS;
        } else {
            status = SRB_STATUS_DATA_OVERRUN;
        }
    }

    *InstanceLengthArray = *SizeNeeded;

    return status;
}


UCHAR
iSpBuildPortalInfo(
    IN PISCSI_ADAPTER_EXTENSION AdapterExtension,
    IN OUT PULONG InstanceLengthArray,
    IN ULONG BufferAvail,
    OUT PUCHAR Buffer,
    OUT PULONG SizeNeeded
    )
{
    PMSiSCSI_PortalInfoClass portalInfoClass;
    PISCSI_PortalInfo portalInfo;
    ULONG ntStatus;
    ULONG inx;
    UCHAR status = SRB_STATUS_SUCCESS;


    *SizeNeeded = FIELD_OFFSET(MSiSCSI_PortalInfoClass, PortalInformation);

    *InstanceLengthArray = *SizeNeeded;


        *SizeNeeded += sizeof(ISCSI_PortalInfo) * gNumberOfIPAddresses;

        if (BufferAvail >= *SizeNeeded) {

            portalInfoClass = (PMSiSCSI_PortalInfoClass) Buffer;

            RtlZeroMemory(portalInfoClass, *SizeNeeded);

            //
            // Number of entries in PortalInformation array
            //
            portalInfoClass->PortalInfoCount = gNumberOfIPAddresses;

            portalInfo = portalInfoClass->PortalInformation;

            inx = 0;
            while (inx < gNumberOfIPAddresses) {

                portalInfo->Index = inx;
                portalInfo->Port = inx;
                portalInfo->PortalTag = 0;
                portalInfo->Protocol = TCP;
                portalInfo->PortalType = InitiatorPortals;

                iSpCopyIPAddress(&portalInfo->IPAddr,
                                 &(gRegisteredTransports[inx]),
                                 NULL);

                *portalInfo->IPAddr.TextAddress = 256 * sizeof(WCHAR);

                portalInfo++;

                inx++;
            }

            *InstanceLengthArray = *SizeNeeded;

            status = SRB_STATUS_SUCCESS;
        } else {
            status = SRB_STATUS_DATA_OVERRUN;
        }

    } else {
        status = SRB_STATUS_ERROR;
        *InstanceLengthArray = 0;
    }

    return status;
}


NTSTATUS
iSpBuildHBAInformationBuffer(
    IN PISCSI_ADAPTER_EXTENSION AdapterExtension,
    OUT PMSiSCSI_HBAInformation IScsiHBAInformation
    )
{
    NTSTATUS status;


    //
    // Boolean fields in MSiSCSI_HBAInformation by default
    // are set to FALSE through RtlZeroMemory
    //
    RtlZeroMemory(IScsiHBAInformation, sizeof(MSiSCSI_HBAInformation));

    //
    // Unique Id we return is a pointer to
    // the miniport's adapater extension
    //
    IScsiHBAInformation->UniqueAdapterId = (ULONGLONG) AdapterExtension;

    //
    // We want WMI to send us IP Address in binary form.
    //
    IScsiHBAInformation->RequiresBinaryIpAddresses = TRUE;

    //
    // We use TCP/IP integrated with Windows
    //
    IScsiHBAInformation->IntegratedTCPIP = TRUE;

    IScsiHBAInformation->MultifunctionDevice = FALSE;

    IScsiHBAInformation->CacheValid = FALSE;

    //
    // iSCSI Version supported
    //
    IScsiHBAInformation->VersionMin = ISCSI_CLI_VERSION_MIN;
    IScsiHBAInformation->VersionMax = ISCSI_CLI_VERSION_MAX;

    IScsiHBAInformation->NumberOfPorts = 1;

    IScsiHBAInformation->MaxCDBLength = 16;

    IScsiHBAInformation->Status = 0;

    IScsiHBAInformation->FunctionalitySupported =
                             (ISCSI_HBA_PRESHARED_KEY_CACHE         |
                              ISCSI_HBA_ISCSI_AUTHENTICATION_CACHE  |
                              ISCSI_HBA_IPSEC_TUNNEL_MODE);

    *IScsiHBAInformation->VendorID = 255*sizeof(WCHAR);

    status = RtlStringCchCopyNW((IScsiHBAInformation->VendorID+1), 255,
                                ISCSI_VENDOR_ID, wcslen(ISCSI_VENDOR_ID));

    NT_ASSERT(NT_SUCCESS(status));

    if(NT_SUCCESS(status) == FALSE){
        return status;
    }

    *IScsiHBAInformation->VendorModel = 255*sizeof(WCHAR);

    status = RtlStringCchCopyNW((IScsiHBAInformation->VendorModel+1), 255,
                                ISCSI_VENDOR_MODEL,
                                wcslen(ISCSI_VENDOR_MODEL));
    
    NT_ASSERT(NT_SUCCESS(status));


    if(NT_SUCCESS(status) == FALSE){
        return status;
    }

    *IScsiHBAInformation->VendorVersion = 255*sizeof(WCHAR);

    status = RtlStringCchCopyNW((IScsiHBAInformation->VendorVersion+1), 255,
                                ISCSI_VENDOR_VERSION,
                                wcslen(ISCSI_VENDOR_VERSION));

    NT_ASSERT(NT_SUCCESS(status));

    if(NT_SUCCESS(status) == FALSE){
        return status;
    }

    *IScsiHBAInformation->FirmwareVersion = 255*sizeof(WCHAR);

    status = RtlStringCchCopyNW((IScsiHBAInformation->FirmwareVersion+1), 255,
                                ISCSI_VENDOR_VERSION,
                                wcslen(ISCSI_VENDOR_VERSION));

    NT_ASSERT(NT_SUCCESS(status));

    if(NT_SUCCESS(status) == FALSE){
        return status;
    }

    *IScsiHBAInformation->AsicVersion = 255*sizeof(WCHAR);

    status = RtlStringCchCopyNW((IScsiHBAInformation->AsicVersion+1), 255,
                                ISCSI_VENDOR_VERSION,
                                wcslen(ISCSI_VENDOR_VERSION));

    
    NT_ASSERT(NT_SUCCESS(status));


    if(NT_SUCCESS(status) == FALSE){
        return status;
    }

    *IScsiHBAInformation->OptionRomVersion = 255*sizeof(WCHAR);

    *IScsiHBAInformation->SerialNumber = 255*sizeof(WCHAR);

    status = RtlStringCchCopyNW((IScsiHBAInformation->SerialNumber+1), 255,
                                ISCSI_SERIAL_NUMBER,
                                wcslen(ISCSI_SERIAL_NUMBER));

    NT_ASSERT(NT_SUCCESS(status));

    if(NT_SUCCESS(status) == FALSE){
        return status;
    }

    *IScsiHBAInformation->DriverName = 255*sizeof(WCHAR);

    status = RtlStringCchCopyNW((IScsiHBAInformation->DriverName+1), 255,
                                ISCSI_INITIATOR_DRIVER_NAME,
                                wcslen(ISCSI_INITIATOR_DRIVER_NAME));


    NT_ASSERT(NT_SUCCESS(status));
    
    return status;
}

UCHAR
iSpBuildInitiatorSessionInfo(
    IN PISCSI_ADAPTER_EXTENSION AdapterExtension,
    IN PSCSIWMI_REQUEST_CONTEXT DispatchContext,
    IN ULONG BufferAvail,
    OUT PUCHAR Buffer,
    OUT PULONG SizeNeeded
    )
{
    PWMI_CONNECTION_LIST connectionList;
    ULONG size, sessionInx;
    ULONG numberOfSessions, numberOfConnections;
    UCHAR status;


    connectionList = iSpGetConnectionList(AdapterExtension,
                                          &numberOfSessions,
                                          &numberOfConnections);

    DebugPrint((iScsiPrtDebugInfo,
        "Number of Sessions %d, Number of Connections %d\n",
        numberOfSessions, numberOfConnections));

    ETWDebugPrint(iScsiLevelInfo,
        iScsiFlagWmi,
        "Number of Sessions %d, Number of Connections %d\n",
        numberOfSessions, numberOfConnections);

    //
    // determine the size of buffer needed to build all of the session
    // and connection data structures
    //

    //
    // Account for the fixed part of the iScsiInitiatorSessionInfo
    //
    size = FIELD_OFFSET(MSiSCSI_InitiatorSessionInfo,
                        SessionsList);

    if (connectionList == NULL) {
        if (BufferAvail >= size)
        {
            PMSiSCSI_InitiatorSessionInfo sessionInfo;

            sessionInfo = (PMSiSCSI_InitiatorSessionInfo)Buffer;
            sessionInfo->UniqueAdapterId = (ULONGLONG) AdapterExtension;
            sessionInfo->SessionCount = 0;
            *SizeNeeded = size;
            return(SRB_STATUS_SUCCESS);
        } else {
            *SizeNeeded = size;
            return(SRB_STATUS_DATA_OVERRUN);
        }
    }

    //
    // Loop over all sessions and account for the space needed for each
    // session plus all of the connections within each session
    //
    sessionInx = 0;
    while (sessionInx < numberOfSessions) {

        //
        // Since the session structure needs to be 8 bytes aligned, we
        // pad out to 8 bytes
        //
        size = (size + 7) & ~7;

        //
        // Add the fixed size needed for the session structure
        //
        size += FIELD_OFFSET(ISCSI_SessionStaticInfo,
                             ConnectionsList);

        //
        // Account for the size of all connection structures, being
        // sure that the connection structure is padded out to 8 bytes
        // in order to maintain 8 byte alignment
        //
        size += connectionList[sessionInx].Count *
                ( (sizeof(ISCSI_ConnectionStaticInfo)+7) & ~7);

        sessionInx++;
    }

    *SizeNeeded = size;
    if (size <= BufferAvail)
    {
        //
        // If we do have enough space to build the session info data
        // structures then we do so
        //
        RtlZeroMemory(Buffer, size);

        status = iSpReadInitiatorSessionInfo(
                            AdapterExtension,
                            connectionList,
                            numberOfSessions,
                            Buffer);

    } else {
        //
        // If there is not enough space to build the session info data
        // structures then return this error
        //
        status = SRB_STATUS_DATA_OVERRUN;
    }

    iSpReleaseConnectionReferences(connectionList,
                                   numberOfSessions);


    return status;
}

UCHAR
iSpBuildConnectionStatistics(
    IN PISCSI_ADAPTER_EXTENSION AdapterExtension,
    IN PSCSIWMI_REQUEST_CONTEXT DispatchContext,
    IN ULONG GuidIndex,
    IN ULONG InstanceIndex,
    IN ULONG InstanceCount,
    IN OUT PULONG InstanceLengthArray,
    IN ULONG BufferAvail,
    OUT PUCHAR Buffer,
    OUT PULONG SizeNeeded
    )
{
    PWMI_CONNECTION_LIST connectionList;
    PISCSI_CONNECTION iScsiConnection;
    PWCHAR NameOffset;
    PUCHAR currentDataPos;
    ULONG instanceInx;
    ULONG newOutBufferAvil;
    WMIString DynamicInstanceName;
    ULONG numberOfSessions;
    ULONG numberOfConnections;
    UCHAR srbStatus;


    srbStatus = SRB_STATUS_SUCCESS;

    *SizeNeeded = 0;

    connectionList = iSpGetConnectionList(AdapterExtension,
                                          &numberOfSessions,
                                          &numberOfConnections);

    if (connectionList == NULL) {
        DebugPrint((iScsiPrtDebugError,
            "Failed to allocate array for connections\n"));

        ETWDebugPrint(iScsiLevelError,
            iScsiFlagWmi,
            "Failed to allocate array for connections\n");

        return SRB_STATUS_ERROR;
    }

    DebugPrint((iScsiPrtDebugInfo,
        "Number of Sessions %d, Number of Connections %d\n",
        numberOfSessions,
        numberOfConnections));

    ETWDebugPrint(iScsiLevelInfo, iScsiFlagWmi,
        "Number of Sessions %d, Number of Connections %d\n",
        numberOfSessions,
        numberOfConnections);

    if (DispatchContext->MinorFunction == IRP_MN_QUERY_ALL_DATA) {

        ULONG sessionInx;
        ULONG connectionInx;
        ULONG connectionCount;
        BOOLEAN dynamicNameStatus;

        dynamicNameStatus = ScsiPortWmiSetInstanceCount(DispatchContext,
                                                        numberOfConnections,
                                                        &newOutBufferAvil,
                                                        SizeNeeded);
        if (!dynamicNameStatus)
        {
            DebugPrint((iScsiPrtDebugError,
                "iscsiprt: wnode is not for a dynamic instance named GUID\n"));

            ETWDebugPrint(iScsiLevelInfo,
                iScsiFlagWmi,
                "iscsiprt: wnode is not for a dynamic instance named GUID\n");

            *SizeNeeded = 0;

            srbStatus = SRB_STATUS_ERROR;

        } else {

            if (newOutBufferAvil == 0) {

                //
                // The buffer passed to return the data is too small
                //
                srbStatus = SRB_STATUS_DATA_OVERRUN;
            }

            //
            // For the data itself
            //
            instanceInx = 0;

            sessionInx = 0;
            *SizeNeeded = 0;
            while (sessionInx < numberOfSessions) {

                connectionCount = connectionList[sessionInx].Count;

                connectionInx = 0;
                while (connectionInx < connectionCount) {

                    iScsiConnection = connectionList[sessionInx].ISCSIConnection[connectionInx];

                    if (iScsiConnection != NULL) {

                        currentDataPos = (PUCHAR) ScsiPortWmiSetData(DispatchContext,
                                                                     instanceInx,
                                                                     sizeof(MSiSCSI_ConnectionStatistics),
                                                                     &newOutBufferAvil,
                                                                     SizeNeeded);
                        if (newOutBufferAvil == 0)
                        {
                            //
                            // The buffer passed to return the data is too small
                            //
                            srbStatus = SRB_STATUS_DATA_OVERRUN;
                        }


                        if ((srbStatus != SRB_STATUS_DATA_OVERRUN) &&
                            (currentDataPos != NULL)) {

                            srbStatus = iSpReadConnStatistics(AdapterExtension,
                                                              iScsiConnection,
                                                              currentDataPos);

                            if (srbStatus == SRB_STATUS_ERROR)
                            {
                                break;
                            }
                        }

                        RtlZeroMemory(&DynamicInstanceName,
                                      sizeof(DynamicInstanceName));

                        srbStatus = iSpGetDynamicConnectionInstanceName(
                                             iScsiConnection,
                                             &DynamicInstanceName);

                        if (srbStatus == SRB_STATUS_ERROR)
                        {
                            break;
                        }

                        NameOffset = ScsiPortWmiSetInstanceName(
                                             DispatchContext,
                                             instanceInx,
                                             DynamicInstanceName.Length+sizeof(USHORT),
                                             &newOutBufferAvil,
                                             SizeNeeded);

                        if ((newOutBufferAvil == 0) && (NameOffset == NULL))
                        {
                            //
                            // The buffer passed to return the data is too small
                            //
                            srbStatus = SRB_STATUS_DATA_OVERRUN;
                        }

                        if (srbStatus != SRB_STATUS_DATA_OVERRUN)
                        {
                            //
                            // copy the instance name into NameOffset
                            //
                            RtlCopyMemory(NameOffset, (PUCHAR)(&DynamicInstanceName),
                                          (DynamicInstanceName.Length + sizeof(USHORT)));
                        }

                        instanceInx++;
                    }

                    connectionInx++;
                }

                if (srbStatus == SRB_STATUS_ERROR)
                {
                    break;
                }

                sessionInx++;
            }
        }
    } else {

        PWMIString instanceName;
        ULONG givenInstanceNameLength = 0;
        ULONG dynInstanceNameLength = 0;
        ULONG instanceNameLength;
        ULONG sessionInx;
        ULONG connectionInx;
        ULONG connectionCount;
        BOOLEAN found = FALSE;

        //
        // single instance
        //

        srbStatus = SRB_STATUS_ERROR;

        //
        // get the instance name
        //
        instanceName = (PWMIString)ScsiPortWmiGetInstanceName(DispatchContext);

        if (instanceName != NULL)
        {
            givenInstanceNameLength = instanceName->Length / sizeof(instanceName->Buffer[0]);

            *SizeNeeded = sizeof(MSiSCSI_ConnectionStatistics);
            if (BufferAvail < *SizeNeeded)
            {
                //
                // The buffer passed to return the data is too small
                //
                srbStatus = SRB_STATUS_DATA_OVERRUN;

            } else {

                instanceInx = 0;

                sessionInx = 0;

                while (sessionInx < numberOfSessions) {

                    if (connectionList[sessionInx].Count) {

                        connectionInx = 0;

                        while (connectionInx < numberOfConnections) {
                            iScsiConnection = connectionList[sessionInx].ISCSIConnection[0];

                            if (iScsiConnection != NULL) {

                                RtlZeroMemory(&DynamicInstanceName,
                                              sizeof(DynamicInstanceName));

                                srbStatus = iSpGetDynamicConnectionInstanceName(
                                                    iScsiConnection,
                                                    &DynamicInstanceName);

                                if (srbStatus != SRB_STATUS_ERROR)
                                {
                                    dynInstanceNameLength = DynamicInstanceName.Length / sizeof(DynamicInstanceName.Buffer[0]);

                                    if ((givenInstanceNameLength == dynInstanceNameLength) &&
                                        !wcsncmp(
                                            DynamicInstanceName.Buffer,
                                            instanceName->Buffer,
                                            dynInstanceNameLength)) {

                                        srbStatus = iSpReadConnStatistics(AdapterExtension,
                                                                          iScsiConnection,
                                                                          Buffer);

                                        if (srbStatus == SRB_STATUS_SUCCESS)
                                        {
                                            *InstanceLengthArray = *SizeNeeded;
                                        }

                                        found = TRUE;

                                        break;
                                    }
                                }
                            }

                            connectionInx++;
                        }
                    }

                    if (found == TRUE) {
                        break;
                    }

                    sessionInx++;
                }
            }
        } else {
            srbStatus = SRB_STATUS_ERROR;
        }

        if(srbStatus != SRB_STATUS_SUCCESS &&
           srbStatus != SRB_STATUS_DATA_OVERRUN)
        {
            *SizeNeeded = 0;
        }
    }

    iSpReleaseConnectionReferences(connectionList,
                                   numberOfSessions);


    return srbStatus;
}

UCHAR
iSpBuildSessionStatistics(
    IN PISCSI_ADAPTER_EXTENSION AdapterExtension,
    IN PSCSIWMI_REQUEST_CONTEXT DispatchContext,
    IN ULONG GuidIndex,
    IN ULONG InstanceIndex,
    IN ULONG InstanceCount,
    IN OUT PULONG InstanceLengthArray,
    IN ULONG BufferAvail,
    OUT PUCHAR Buffer,
    OUT PULONG SizeNeeded
    )
{
    PWCHAR NameOffset;
    PUCHAR currentDataPos;
    WMIString DynamicInstanceName;
    ULONG connectionCount=0;
    ULONG instanceInx;
    ULONG newOutBufferAvil;
    ULONG initiatorIndx;
    ULONG instanceSize=0;
    WMIString WMIformatString;
    UCHAR srbStatus;

    PWMI_CONNECTION_LIST connectionList;
    ULONG numberOfSessions;
    ULONG numberOfConnections;


    srbStatus = SRB_STATUS_SUCCESS;
    *SizeNeeded = 0;

    connectionList = iSpGetConnectionList(AdapterExtension,
                                          &numberOfSessions,
                                          &numberOfConnections);

    if (connectionList == NULL) {
        DebugPrint((iScsiPrtDebugError,
            "Failed to allocate array for connections\n"));

        ETWDebugPrint(iScsiLevelError, iScsiFlagWmi,
            "Failed to allocate array for connections\n");

        return SRB_STATUS_ERROR;
    }

    DebugPrint((iScsiPrtDebugInfo,
        "Number of Sessions %d, Number of Connections %d\n",
        numberOfSessions,
        numberOfConnections));

    ETWDebugPrint(iScsiLevelInfo,
        iScsiFlagWmi,
        "Number of Sessions %d, Number of Connections %d\n",
        numberOfSessions,
        numberOfConnections);

    if (DispatchContext->MinorFunction == IRP_MN_QUERY_ALL_DATA)
    {

        ULONG sessionInx;
        BOOLEAN dynamicNameStatus;

        //
        // instance counts
        //
        dynamicNameStatus = ScsiPortWmiSetInstanceCount(DispatchContext,
                                                        numberOfSessions,
                                                        &newOutBufferAvil,
                                                        SizeNeeded);

        if (!dynamicNameStatus)
        {
            DebugPrint((iScsiPrtDebugError,
                "iscsiprt: wnode is not for a dynamic instance named GUID\n"));

            ETWDebugPrint(iScsiLevelError, iScsiFlagWmi,
                "iscsiprt: wnode is not for a dynamic instance named GUID\n");

            *SizeNeeded = 0;
            srbStatus = SRB_STATUS_ERROR;

        } else {

            if (numberOfSessions == 0)
            {
                //
                // no instances avaliable
                //
            } else {

                if (newOutBufferAvil == 0)
                {
                    //
                    // The buffer passed to return the data is too small
                    //
                    srbStatus = SRB_STATUS_DATA_OVERRUN;
                }

                //
                // for the data itself
                //
                instanceInx = 0;

                sessionInx = 0;
                
                *SizeNeeded = 0;

                while (sessionInx < numberOfSessions) {

                    PISCSI_SESSION iScsiSession;
                    PISCSI_CONNECTION iScsiConnection;

                    if (connectionList[sessionInx].Count > 0) {

                        iScsiConnection = connectionList[sessionInx].ISCSIConnection[0];
                        iScsiSession = iScsiConnection->ISCSISession;

                        currentDataPos = (PUCHAR) ScsiPortWmiSetData(DispatchContext,
                                                                     instanceInx,
                                                                     sizeof(MSiSCSI_SessionStatistics),
                                                                     &newOutBufferAvil,
                                                                     SizeNeeded);
                        if (newOutBufferAvil == 0)
                        {
                            //
                            // The buffer passed to return the data is too small
                            //
                            srbStatus = SRB_STATUS_DATA_OVERRUN;
                        }

                        if ((srbStatus != SRB_STATUS_DATA_OVERRUN) &&
                            (currentDataPos != NULL))
                        {

                            srbStatus = iSpReadSessionStatistics(AdapterExtension,
                                                                 iScsiSession,
                                                                 &connectionList[sessionInx],
                                                                 currentDataPos);

                        }

                        if (srbStatus == SRB_STATUS_ERROR)
                        {
                            break;
                        }

                        RtlZeroMemory(&DynamicInstanceName,
                                      sizeof(DynamicInstanceName));

                        srbStatus = iSpGetDynamicSessionInstanceName(iScsiSession,
                                                                     &DynamicInstanceName);

                        NameOffset = ScsiPortWmiSetInstanceName(DispatchContext,
                                                                instanceInx,
                                                                DynamicInstanceName.Length+sizeof(USHORT),
                                                                &newOutBufferAvil,
                                                                SizeNeeded);

                        if ((newOutBufferAvil == 0) && (NameOffset == NULL))
                        {
                            //
                            // The buffer passed to return the data is too small
                            //
                            srbStatus = SRB_STATUS_DATA_OVERRUN;
                        }

                        if (srbStatus != SRB_STATUS_DATA_OVERRUN) {

                            //
                            // copy the instance name into NameOffset
                            //
                            RtlCopyMemory(NameOffset,
                                          (PUCHAR)(&DynamicInstanceName),
                                          (DynamicInstanceName.Length + sizeof(USHORT)));
                        }

                        instanceInx++;
                    }

                    sessionInx++;
                }


            }
        }
    } else { //single instance

        PWMIString instanceName;
        ULONG givenInstanceNameLength = 0;
        ULONG dynInstanceNameLength = 0;
        ULONG instanceNameLength;
        ULONG sessionInx;
        BOOLEAN found = FALSE;

        srbStatus = SRB_STATUS_ERROR;

        //
        // get the instance name
        //
        instanceName = (PWMIString) ScsiPortWmiGetInstanceName(DispatchContext);

        if (instanceName != NULL)
        {
            givenInstanceNameLength = instanceName->Length / sizeof(instanceName->Buffer[0]);

            *SizeNeeded = sizeof(MSiSCSI_SessionStatistics);
            if (BufferAvail < *SizeNeeded)
            {
                //
                // The buffer passed to return the data is too small
                //
                srbStatus = SRB_STATUS_DATA_OVERRUN;
            } else {

                sessionInx = 0;
                while (sessionInx < numberOfSessions) {

                    PISCSI_SESSION iScsiSession;
                    PISCSI_CONNECTION iScsiConnection;

                    if (connectionList[sessionInx].Count > 0) {

                        iScsiConnection = connectionList[sessionInx].ISCSIConnection[0];
                        iScsiSession = iScsiConnection->ISCSISession;

                        RtlZeroMemory(&DynamicInstanceName, sizeof(DynamicInstanceName));
                        srbStatus = iSpGetDynamicSessionInstanceName(iScsiSession,
                                                                     &DynamicInstanceName);
                        if (srbStatus == SRB_STATUS_SUCCESS)
                        {
                            dynInstanceNameLength = DynamicInstanceName.Length / sizeof(DynamicInstanceName.Buffer[0]);

                            if (givenInstanceNameLength == dynInstanceNameLength
                                && !wcsncmp(DynamicInstanceName.Buffer,
                                          instanceName->Buffer,
                                          dynInstanceNameLength))
                            {
                                srbStatus = iSpReadSessionStatistics(AdapterExtension,
                                                                     iScsiSession,
                                                                     &connectionList[sessionInx],
                                                                     Buffer);

                                if (srbStatus == SRB_STATUS_SUCCESS)
                                {
                                    *InstanceLengthArray = *SizeNeeded;
                                }

                                break;
                            }
                        }
                    }

                    sessionInx++;
                }
            }
        } else {
            srbStatus = SRB_STATUS_ERROR;
        }

        if(srbStatus != SRB_STATUS_SUCCESS &&
           srbStatus != SRB_STATUS_DATA_OVERRUN)
        {
            *SizeNeeded = 0;
        }
    }


    return srbStatus;
}

UCHAR
iSpBuildRequestTimeStatistics(
    IN PISCSI_ADAPTER_EXTENSION AdapterExtension,
    IN PSCSIWMI_REQUEST_CONTEXT DispatchContext,
    IN ULONG GuidIndex,
    IN ULONG InstanceIndex,
    IN ULONG InstanceCount,
    IN OUT PULONG InstanceLengthArray,
    IN ULONG BufferAvail,
    OUT PUCHAR Buffer,
    OUT PULONG SizeNeeded
    )
{
    PWMI_CONNECTION_LIST connectionList;
    PISCSI_CONNECTION iScsiConnection;
    PWCHAR NameOffset;
    PUCHAR currentDataPos;
    ULONG instanceInx;
    ULONG newOutBufferAvil;
    WMIString DynamicInstanceName;
    ULONG numberOfSessions;
    ULONG numberOfConnections;
    UCHAR srbStatus;


    srbStatus = SRB_STATUS_SUCCESS;

    *SizeNeeded = 0;

    connectionList = iSpGetConnectionList(AdapterExtension,
                                          &numberOfSessions,
                                          &numberOfConnections);
    if (connectionList == NULL) {

        DebugPrint((iScsiPrtDebugError,
            "Failed to allocate array for connections\n"));

        ETWDebugPrint(iScsiLevelError, iScsiFlagWmi,
            "Failed to allocate array for connections\n");

        return SRB_STATUS_ERROR;
    }

    DebugPrint((iScsiPrtDebugInfo,
        "Number of Sessions %d, Number of Connections %d\n",
        numberOfSessions,
        numberOfConnections));

    ETWDebugPrint(iScsiLevelInfo,
        iScsiFlagWmi,
        "Number of Sessions %d, Number of Connections %d\n",
        numberOfSessions,
        numberOfConnections);

    if (DispatchContext->MinorFunction == IRP_MN_QUERY_ALL_DATA) {

        ULONG sessionInx;
        ULONG connectionInx;
        ULONG connectionCount;
        BOOLEAN dynamicNameStatus;

        dynamicNameStatus = ScsiPortWmiSetInstanceCount(DispatchContext,
                                                        numberOfConnections,
                                                        &newOutBufferAvil,
                                                        SizeNeeded);
        if (!dynamicNameStatus)
        {
            DebugPrint((iScsiPrtDebugError,
                "iscsiprt: wnode is not for a dynamic instance named GUID\n"));

            ETWDebugPrint(iScsiLevelError,
                iScsiFlagWmi,
                "iscsiprt: wnode is not for a dynamic instance named GUID\n");

            *SizeNeeded = 0;

            srbStatus = SRB_STATUS_ERROR;

        } else {

            if (newOutBufferAvil == 0) {

                //
                // The buffer passed to return the data is too small
                //
                srbStatus = SRB_STATUS_DATA_OVERRUN;
            }

            //
            // For the data itself
            //
            instanceInx = 0;

            sessionInx = 0;

            *SizeNeeded = 0;

            while (sessionInx < numberOfSessions) {

                connectionCount = connectionList[sessionInx].Count;

                connectionInx = 0;
                while (connectionInx < connectionCount) {

                    iScsiConnection = connectionList[sessionInx].ISCSIConnection[connectionInx];

                    if (iScsiConnection != NULL) {

                        currentDataPos = (PUCHAR) ScsiPortWmiSetData(DispatchContext,
                                                                     instanceInx,
                                                                     sizeof(MSiSCSI_RequestTimeStatistics),
                                                                     &newOutBufferAvil,
                                                                     SizeNeeded);
                        if (newOutBufferAvil == 0)
                        {
                            //
                            // The buffer passed to return the data is too small
                            //
                            srbStatus = SRB_STATUS_DATA_OVERRUN;
                        }


                        if ((srbStatus != SRB_STATUS_DATA_OVERRUN) &&
                            (currentDataPos != NULL)) {

                            srbStatus = iSpReadRequestTimeStatistics(AdapterExtension,
                                                                     iScsiConnection,
                                                                     currentDataPos);

                            if (srbStatus == SRB_STATUS_ERROR)
                            {
                                break;
                            }
                        }

                        RtlZeroMemory(&DynamicInstanceName,
                                      sizeof(DynamicInstanceName));

                        srbStatus = iSpGetDynamicConnectionInstanceName(
                                             iScsiConnection,
                                             &DynamicInstanceName);

                        if (srbStatus == SRB_STATUS_ERROR)
                        {
                            break;
                        }

                        NameOffset = ScsiPortWmiSetInstanceName(
                                             DispatchContext,
                                             instanceInx,
                                             DynamicInstanceName.Length+sizeof(USHORT),
                                             &newOutBufferAvil,
                                             SizeNeeded);

                        if ((newOutBufferAvil == 0) && (NameOffset == NULL))
                        {
                            //
                            // The buffer passed to return the data is too small
                            //
                            srbStatus = SRB_STATUS_DATA_OVERRUN;
                        }

                        if (srbStatus != SRB_STATUS_DATA_OVERRUN)
                        {
                            //
                            // copy the instance anme into NameOffset
                            //
                            RtlCopyMemory(NameOffset, (PUCHAR)(&DynamicInstanceName),
                                          (DynamicInstanceName.Length + sizeof(USHORT)));
                        }

                        instanceInx++;
                    }

                    connectionInx++;
                }

                if (srbStatus == SRB_STATUS_ERROR)
                {
                    break;
                }

                sessionInx++;
            }
        }
    } else {
        PWMIString instanceName;
        ULONG givenInstanceNameLength = 0;
        ULONG dynInstanceNameLength = 0;
        ULONG instanceNameLength;
        ULONG sessionInx;
        ULONG connectionInx;
        ULONG connectionCount;
        BOOLEAN found = FALSE;

        //
        // single instance
        //

        srbStatus = SRB_STATUS_ERROR;

        //
        // get the instance name
        //
        instanceName = (PWMIString)ScsiPortWmiGetInstanceName(DispatchContext);

        if (instanceName != NULL)
        {
            givenInstanceNameLength = instanceName->Length / sizeof(instanceName->Buffer[0]);

            *SizeNeeded = sizeof(MSiSCSI_RequestTimeStatistics);
            if (BufferAvail < *SizeNeeded)
            {
                //
                // The buffer passed to return the data is too small
                //
                srbStatus = SRB_STATUS_DATA_OVERRUN;

            } else {

                instanceInx = 0;

                sessionInx = 0;

                while (sessionInx < numberOfSessions) {

                    if (connectionList[sessionInx].Count) {

                        connectionInx = 0;

                        while (connectionInx < numberOfConnections) {
                            iScsiConnection = connectionList[sessionInx].ISCSIConnection[0];

                            if (iScsiConnection != NULL) {

                                srbStatus = iSpGetDynamicConnectionInstanceName(
                                                    iScsiConnection,
                                                    &DynamicInstanceName);

                                if (srbStatus != SRB_STATUS_ERROR)
                                {
                                    dynInstanceNameLength = DynamicInstanceName.Length / sizeof(DynamicInstanceName.Buffer[0]);

                                    if ((givenInstanceNameLength == dynInstanceNameLength) &&
                                        !wcsncmp(DynamicInstanceName.Buffer,
                                            instanceName->Buffer,
                                            dynInstanceNameLength))
                                    {
                                        srbStatus = iSpReadRequestTimeStatistics(AdapterExtension,
                                                                                 iScsiConnection,
                                                                                 Buffer);
                                        if (srbStatus == SRB_STATUS_SUCCESS)
                                        {
                                            *InstanceLengthArray = *SizeNeeded;
                                        }

                                        found = TRUE;

                                        break;
                                    }
                                }
                            }
                            connectionInx++;
                        }
                    }

                    if (found == TRUE) {
                        break;
                    }

                    sessionInx++;
                }
            }
        } else {
            srbStatus = SRB_STATUS_ERROR;
        }

        if(srbStatus != SRB_STATUS_SUCCESS &&
           srbStatus != SRB_STATUS_DATA_OVERRUN)
        {
            *SizeNeeded = 0;
        }
    }

    iSpReleaseConnectionReferences(connectionList,
                                   numberOfSessions);


    return srbStatus;
}


UCHAR iScsiQuerySecurityCapabilities(
    PISCSI_ADAPTER_EXTENSION AdapterExtension,
    ULONG BufferAvail,
    PUCHAR Buffer,
    PULONG InstanceLength,
    PULONG SizeNeeded
    )
/*+++

Routine Description :

    This routine implements the WMI query for the security capabilites
    class.

--*/
{
    UCHAR status;
    PMSiSCSI_SecurityCapabilities config;


    *SizeNeeded = sizeof(MSiSCSI_SecurityCapabilities);

    if (BufferAvail >= sizeof(MSiSCSI_SecurityCapabilities))
    {
        config = (PMSiSCSI_SecurityCapabilities)Buffer;
        RtlZeroMemory(Buffer, sizeof(MSiSCSI_SecurityCapabilities));

        config->ProtectiScsiTraffic = FALSE;
        config->ProtectiSNSTraffic = FALSE;
        config->CertificatesSupported = FALSE;

        config->EncryptionAvailableCount = 1;
        config->EncryptionAvailable[0] = ISCSI_ENCRYPT_NONE;

        *InstanceLength = *SizeNeeded;
        status = SRB_STATUS_SUCCESS;
    } else {
        status = SRB_STATUS_DATA_OVERRUN;
    }

    return(status);
}

UCHAR iScsiQueryDiscoveryConfig(
    PISCSI_ADAPTER_EXTENSION AdapterExtension,
    ULONG BufferAvail,
    PUCHAR Buffer,
    PULONG InstanceLength,
    PULONG SizeNeeded
    )
/*+++

Routine Description :

    This routine implements the WMI query for the discovery config
    class.

--*/
{
    NTSTATUS rtlStatus;
    UCHAR status;
    PMSiSCSI_DiscoveryConfig config;


    *SizeNeeded = sizeof(MSiSCSI_DiscoveryConfig);

    if (BufferAvail >= sizeof(MSiSCSI_DiscoveryConfig))
    {
        config = (PMSiSCSI_DiscoveryConfig)Buffer;
        RtlZeroMemory(Buffer, sizeof(MSiSCSI_DiscoveryConfig));
        if (AdapterExtension->DiscoveryConfigFlags & DISCOVERY_CONFIG_DO_DISCOVERY)
        {
            config->PerformiSNSDiscovery = TRUE;
        }

        if (AdapterExtension->DiscoveryConfigFlags & DISCOVERY_CONFIG_FIND_SNS_AUTOMATICALLY)
        {
            config->AutomaticiSNSDiscovery = TRUE;
        }

        config->InitiatorName[0] = 256 * sizeof(WCHAR);

        rtlStatus = RtlStringCchCopyNW(&config->InitiatorName[1],
                                       MAX_INITIATOR_NAME,
                                       AdapterExtension->InitiatorName,
                                       MAX_INITIATOR_NAME);

        config->iSNSServer.Type = ISCSI_IP_ADDRESS_TEXT;
        config->iSNSServer.TextAddress[0] = 256 * sizeof(WCHAR);
        rtlStatus = RtlStringCchCopyNW(&config->iSNSServer.TextAddress[1], 256,
                                       AdapterExtension->SNSServer, 256);

        *InstanceLength = *SizeNeeded;
        status = SRB_STATUS_SUCCESS;
    } else {
        status = SRB_STATUS_DATA_OVERRUN;
    }

    return(status);
}

UCHAR iScsiAdvanceString(
    OUT PWCHAR *String,
    OUT PUSHORT StringLen,
    IN OUT PUCHAR *Buffer,
    IN OUT PULONG BufferSize
    )
{
    UCHAR status = SRB_STATUS_ERROR;
    PUCHAR b;
    ULONG bs;
    USHORT len;

    b = *Buffer;
    bs = *BufferSize;

    if (bs >= sizeof(USHORT))
    {
        //
        // Get the length of the string and advance beyond it
        //
        len = *((PUSHORT)b);
        b += sizeof(USHORT);
        bs -= sizeof(USHORT);
        *String = (PWCHAR)b;
        *StringLen = len;

        if (bs >= len)
        {
            b += len;
            bs -= len;

            *Buffer = b;
            *BufferSize = bs;
            status = SRB_STATUS_SUCCESS;
        }
    }
    return(status);
}

UCHAR iScsiSetDiscoveryConfig(
    PISCSI_ADAPTER_EXTENSION AdapterExtension,
    ULONG BufferSize,
    PUCHAR Buffer
    )
/*+++

Routine Description :

    This routine implements the WMI set for the discovery config
    class.

--*/
{
    NTSTATUS rtlStatus;
    UCHAR status;
    PMSiSCSI_DiscoveryConfig config;
    USHORT stringLen, initiatorNameLen;
    PISCSI_IP_Address ipAddress;
    PWCHAR string;
    PWCHAR initiatorName;
    ULONG pad;

    //
    // Assume the buffer passed will be bad in some way
    //
    status = SRB_STATUS_ERROR;

    //
    // Validate that the buffer passed is large enough for the
    // beginning of the class
    //
    if (BufferSize >= FIELD_OFFSET(MSiSCSI_DiscoveryConfig,
                                  InitiatorName))
    {
        config = (PMSiSCSI_DiscoveryConfig)Buffer;

        //
        // Advance buffer pointer past beginning of config struct
        //
        Buffer += FIELD_OFFSET(MSiSCSI_DiscoveryConfig, InitiatorName);
        BufferSize -= FIELD_OFFSET(MSiSCSI_DiscoveryConfig, InitiatorName);

        //
        // Now validate that there is a valid InitiatorName string
        //
        status = iScsiAdvanceString(&initiatorName,
                                    &initiatorNameLen,
                                    &Buffer,
                                    &BufferSize);

        if (status == SRB_STATUS_SUCCESS)
        {
            status = SRB_STATUS_ERROR;

            //
            // Pad Buffer to a 4 byte boundry since
            // ISCSI_IP_Address is required to be on a 4 byte
            // boundry
            //
            pad = (ULONG)((((ULONG_PTR)Buffer + 3) &~3) - (ULONG_PTR)Buffer);
            if (BufferSize >= pad)
            {
                Buffer += pad;
                BufferSize -= pad;

                //
                // Now validate that there is enough room for the beginning of
                // the IP address struct
                //
                if (BufferSize >= FIELD_OFFSET(ISCSI_IP_Address,
                                               TextAddress))
                {
                    ipAddress = (PISCSI_IP_Address)Buffer;

                    //
                    // Advance buffer pointer beyond IP address struct
                    //
                    Buffer += FIELD_OFFSET(ISCSI_IP_Address, TextAddress);
                    BufferSize -= FIELD_OFFSET(ISCSI_IP_Address, TextAddress);

                    //
                    // Validate that the user passed a text addresss
                    //
                    if ((ipAddress->Type == ISCSI_IP_ADDRESS_TEXT) ||
                        (ipAddress->Type == ISCSI_IP_ADDRESS_IPV4))
                    {
                        //
                        // Validate that there is enough room for the string
                        // length
                        //
                        status = iScsiAdvanceString(&string,
                                                    &stringLen,
                                                    &Buffer,
                                                    &BufferSize);

                        if (status == SRB_STATUS_SUCCESS)
                        {
                            status = SRB_STATUS_ERROR;

                            //
                            // Finally copy out the values from the
                            // passed buffer into the device extension
                            //

                            //
                            // Validate that the input string is not too large
                            //
                            if (initiatorNameLen <= MAX_INITIATOR_NAME) {

                                rtlStatus = RtlStringCchCopyNW(AdapterExtension->InitiatorName,
                                                               MAX_INITIATOR_NAME,
                                                               initiatorName,
                                                               MAX_INITIATOR_NAME);
                                AdapterExtension->InitiatorName[(initiatorNameLen / sizeof(WCHAR))] = 0;

                                if (ipAddress->Type == ISCSI_IP_ADDRESS_TEXT)
                                {
                                    if (stringLen <= 256)
                                    {
                                        rtlStatus = RtlStringCchCopyNW(AdapterExtension->SNSServer,
                                                                       256, string, 256);
                                        AdapterExtension->SNSServer[(stringLen / sizeof(WCHAR))] = 0;
                                    }

                                } else {
                                    PUCHAR i;

                                    i = (PUCHAR)&ipAddress->IpV4Address;

                                    RtlStringCchPrintfW(AdapterExtension->SNSServer,
                                                        256,
                                                        L"%d.%d.%d.%d",
                                                        i[0], i[1], i[2], i[3]);
                                }
                                AdapterExtension->DiscoveryConfigFlags = (config->PerformiSNSDiscovery) ?
                                                                             DISCOVERY_CONFIG_DO_DISCOVERY :
                                                                             0;

                                AdapterExtension->DiscoveryConfigFlags |= (config->AutomaticiSNSDiscovery) ?
                                                                             DISCOVERY_CONFIG_FIND_SNS_AUTOMATICALLY :
                                                                             0;

                                status = SRB_STATUS_SUCCESS;
                            }
                        }
                    }
                }
            }
        }
    }
    return(status);
}

UCHAR iScsiAddPortalGroup(
    IN OUT PUCHAR *Buffer,
    IN OUT PULONG SizeLeft,
    IN OUT PULONG SizeNeeded,
    IN ULONG PortalCount
    )
/*+++

Routine Description :

    This routine will allocate space of an
    ISCSI_DiscoveredTargetPortalGroup structure that can be followed by
    a set of ISCSI_DiscoveredTargetPortal data structures that describe
    the portals associated with the portal group.

    *Buffer on entry has the current position in the buffer that the
        data structures are being built. On return it is updated beyond
        the ISCSI_DiscoveredTargetPortalGroup structure.

    *SizeLeft on entry has the number of bytes left in *Buffer. On
        return it is updated with the number of bytes consumed by the
        ISCSI_DiscoveredTargetPortalGroup structure. If there is not
        enough bytes for the ISCSI_DiscoveredTargetPortalGroup
        structure then it returns with 0 and SRB_STATUS_DATA_OVERRUN is
        the return value from this function. If there is enough bytes
        then SRB_STATUS_SUCCESS is returned. If a previous call to this
        or iScsiAddPortalToGroup returned *SizeLeft as 0, then it is
        permissible to keep calling so that *SizeNeeded gets updated so
        that an accurate size for the entire data structure can be
        computed.

    *SizeNeeded on entry has the number of bytes needed so far to complete the
        request. On return it is updated to include the number of bytes
        needed for the ISCSI_DiscoveredTargetPortalGroup structure.

    *PortalGroup returns pointing at the portal group for which space
        was allocated by this routine.

--*/
{
    UCHAR status;
    ULONG PadSize, GroupSize, TotalSize;
    PISCSI_DiscoveredTargetPortalGroup2 PortalGroup;

    //
    // Be sure that buffer is padded on a 4 byte boundry
    //
    PadSize = (ULONG)((((ULONG_PTR)*Buffer + 3) &~3) - ((ULONG_PTR)*Buffer));

    //
    // Compute size of portal groups
    //
    GroupSize = FIELD_OFFSET(ISCSI_DiscoveredTargetPortalGroup2,
                             Portals);
    TotalSize = PadSize + GroupSize;

    //
    // Update the size needed to complete the entire request
    //
    *SizeNeeded += TotalSize;

    if (*SizeLeft >= TotalSize)
    {
        //
        // There is enough room to build the portal group. Return the
        // pointer to the portal group.
        //
        RtlZeroMemory(*Buffer, TotalSize);

        PortalGroup = (PISCSI_DiscoveredTargetPortalGroup2)((PUCHAR)*Buffer +
                                                            PadSize);
        PortalGroup->PortalCount = PortalCount;

        //
        // Update pointer in current buffer and number of bytes left
        //
        *SizeLeft -= TotalSize;
        status = SRB_STATUS_SUCCESS;
    } else {
        //
        // There is not enough room left to build the portal group
        //
        *SizeLeft = 0;
        status = SRB_STATUS_DATA_OVERRUN;
    }

    //
    // Advance buffer pointer, do this even if there is not enough room
    // so the size calculations can take into account any padding
    //
    *Buffer += TotalSize;

    return(status);
}

VOID
iScsiCopyUTF8ToCounted(
    PWCHAR p,
    PUTF8CHAR UTF8,
    ULONG UTF8Len
    )
{
    ULONG i;

    *p++ = ((USHORT)UTF8Len * sizeof(WCHAR));
    for (i = 0; i < UTF8Len; i++)
    {
        //
        // This is a quick and dirty conversion from UTF8 to
        // UNICODE and is only valid for the US ANSI character set.
        // See RFC 2044 for more details on converting UTF8 to
        // UNICODE
        //
        *p++ = (WCHAR)*UTF8++;
    }
}

UCHAR iScsiAddPortalToGroup(
    IN OUT PUCHAR *Buffer,
    IN OUT PULONG SizeLeft,
    IN OUT PULONG SizeNeeded,
    IN ULONG Address,
    IN PUTF8CHAR SymbolicName,
    IN USHORT Socket,
    IN ULONG SecurityBitmap,
    IN ULONG KeySize,
    IN PUCHAR Key
    )
/*+++

Routine Description :

    This routine will allocate space of an
    ISCSI_DiscoveredTargetPortal structure and fill it with the
    parameters passed as appropriate.

    *Buffer on entry has the current position in the buffer that the
        data structures are being built. On return it is updated beyond
        the ISCSI_DiscoveredTargetPortal structure.

    *SizeLeft on entry has the number of bytes left in *Buffer. On
        return it is updated with the number of bytes consumed by the
        ISCSI_DiscoveredTargetPortal structure. If there is not
        enough bytes for the ISCSI_DiscoveredTargetPortal
        structure then it returns with 0 and SRB_STATUS_DATA_OVERRUN is
        the return value from this function. If there is enough bytes
        then SRB_STATUS_SUCCESS is returned. If a previous call to this
        or iScsiAddPortalGroup returned *SizeLeft as 0, then it is
        permissible to keep calling so that *SizeNeeded gets updated so
        that an accurate size for the entire data structure can be
        computed.

    *SizeNeeded on entry has the number of bytes needed so far to complete the
        request. On return it is updated to include the number of bytes
        needed for the ISCSI_DiscoveredTargetPortal structure.

    Address has the IP Address of the portal

    SymbolicName is a pointer to a UTF8 string that has the portal
        symbolic name.

    Socket has the socket number of the portal

    SecurityBitmap is the security bitmap

    KeySize is the number of bytes in the key

    Key is th preshared key

--*/
{
    UCHAR status;
    PISCSI_DiscoveredTargetPortal2 Portal;
    ULONG PadSize, SymbolicNamePos, SymbolicNameLen, PortalSize;
    ULONG TotalSize, i;
    PWCHAR p;
    PULONG pu;
    ULONG BitmapPos;

    //
    // Portal must be padded on a 4 byte boundry
    //
    PadSize = (ULONG)((((ULONG_PTR)*Buffer + 3) &~3) - ((ULONG_PTR)*Buffer));

    //
    // Compute the portal size out of the size of the individual
    // components. Since these structures are variable length we cannot
    // use sizeof() without being very wasteful of memory
    //
    BitmapPos = FIELD_OFFSET(ISCSI_DiscoveredTargetPortal2, Address) +
                      FIELD_OFFSET(ISCSI_IP_Address, TextAddress) +
                      sizeof(USHORT); // 0 length for IP_Address->TextAddress

    SymbolicNamePos = BitmapPos + (2 * sizeof(ULONG)) + KeySize;

    SymbolicNameLen = SymbolicName ? strlen(SymbolicName) : 0;
    PortalSize = SymbolicNamePos +
                                      // USHORT for length and size in WCHAR of
                                      // symbolic name
                 sizeof(USHORT) + (SymbolicNameLen * sizeof(WCHAR));

    TotalSize = PadSize + PortalSize;
    *SizeNeeded += TotalSize;

    if (*SizeLeft >= TotalSize)
    {
        //
        // There is enough room for the portal so build the portal data
        // structure
        //
        Portal = (PISCSI_DiscoveredTargetPortal2)((PUCHAR)*Buffer + PadSize);

        RtlZeroMemory(Portal, TotalSize);
        Portal->Socket = Socket;
        Portal->Address.Type = ISCSI_IP_ADDRESS_IPV4;
        Portal->Address.IpV4Address = Address;
        Portal->Address.TextAddress[0] = 0;

        pu = (PULONG)((PUCHAR)Portal + BitmapPos);

        *pu++ = SecurityBitmap;
        *pu++ = KeySize;

        memcpy(pu, Key, KeySize);

        //
        // FIll in symbolic name; length followed by unicode chars
        //
        p = (PWCHAR)((PUCHAR)Portal + SymbolicNamePos);

        iScsiCopyUTF8ToCounted(p, SymbolicName, SymbolicNameLen);

        //
        // Update our current buffer pointers
        //
        *SizeLeft -= TotalSize;

        status = SRB_STATUS_SUCCESS;
    } else {
        //
        // There is not enough room for the portal
        //
        *SizeLeft = 0;
        status = SRB_STATUS_DATA_OVERRUN;
    }

    //
    // Advance buffer pointer, do this even if there is not enough room
    // so the size calculations can take into account any padding
    //
    *Buffer += TotalSize;

    return(status);
}

UCHAR iScsiAddDiscoveredTarget(
    IN OUT PUCHAR *Buffer,
    IN OUT PULONG SizeLeft,
    IN OUT PULONG SizeNeeded,
    IN PUTF8CHAR Name,
    IN PUTF8CHAR Alias,
    IN PUTF8CHAR DDName,
    IN ULONG DDID,
    IN ULONG PortalGroupCount
    )
/*+++

Routine Description :

    This routine will allocate space of an
    ISCSI_DiscoveredTargetPortal structure and fill it with the
    parameters passed as appropriate.

    *Buffer on entry has the current position in the buffer that the
        data structures are being built. On return it is updated beyond
        the ISCSI_DiscoveredTargetPortal structure.

    *SizeLeft on entry has the number of bytes left in *Buffer. On
        return it is updated with the number of bytes consumed by the
        ISCSI_DiscoveredTargetPortal structure. If there is not
        enough bytes for the ISCSI_DiscoveredTargetPortal
        structure then it returns with 0 and SRB_STATUS_DATA_OVERRUN is
        the return value from this function. If there is enough bytes
        then SRB_STATUS_SUCCESS is returned. If a previous call to this
        or iScsiAddPortalGroup returned *SizeLeft as 0, then it is
        permissible to keep calling so that *SizeNeeded gets updated so
        that an accurate size for the entire data structure can be
        computed.

    *SizeNeeded on entry has the number of bytes needed so far to complete the
        request. On return it is updated to include the number of bytes
        needed for the ISCSI_DiscoveredTargetPortal structure.

    Name is the target name

    Alias is the target alias

    DDName is the Discovery domain name

    DDID is dicovery domain id

--*/
{
    UCHAR status;
    ULONG NameLen, AliasLen, DDNameLen, PadSize, TotalSize;
    PWCHAR p;
    PISCSI_DiscoveredTarget2 Target;

    NameLen = strlen(Name);
    AliasLen = Alias ? strlen(Alias) : 0;

    PadSize = (ULONG)((((ULONG_PTR)*Buffer + 3) &~3) - ((ULONG_PTR)*Buffer));

    TotalSize = FIELD_OFFSET(ISCSI_DiscoveredTarget2, TargetName) +
                (((NameLen+1) + (AliasLen+1) ) * sizeof(WCHAR)) +
                PadSize;

    *SizeNeeded += TotalSize;

    if (*SizeLeft >= TotalSize)
    {
        Target = (PISCSI_DiscoveredTarget2)((PUCHAR)*Buffer + PadSize);

        Target->TargetPortalGroupCount = PortalGroupCount;

        p = (PWCHAR)((PUCHAR)Target + FIELD_OFFSET(ISCSI_DiscoveredTarget2,
                                                   TargetName));
        iScsiCopyUTF8ToCounted(p, Name, NameLen);
        p += NameLen + 1;

        iScsiCopyUTF8ToCounted(p, Alias, AliasLen);
        p += AliasLen + 1;

        *SizeLeft -= TotalSize;
        status = SRB_STATUS_SUCCESS;
    } else {
        //
        // There is not enough room for the portal
        //
        *SizeLeft = 0;
        status = SRB_STATUS_DATA_OVERRUN;
    }

    //
    // Advance buffer pointer, do this even if there is not enough room
    // so the size calculations can take into account any padding
    //
    *Buffer += TotalSize;

    return(status);
}

UCHAR iScsiSetPresharedKeyForId(
    PISCSI_ADAPTER_EXTENSION AdapterExtension,
    ULONG InBufferSize,
    ULONG OutBufferSize,
    PULONG SizeNeeded,
    PUCHAR Buffer
    )
{
    PSetPresharedKeyForId_IN in;
    PSetPresharedKeyForId_OUT out;
    UCHAR status;
    ULONG sizeExpected;
    PCHAR key;

    *SizeNeeded = sizeof(SetPresharedKeyForId_OUT);

    if (OutBufferSize >= *SizeNeeded)
    {
        status = SRB_STATUS_ERROR;
        if (InBufferSize >= FIELD_OFFSET(SetPresharedKeyForId_IN,
                                         Id))
        {
            in = (PSetPresharedKeyForId_IN)Buffer;
            sizeExpected = FIELD_OFFSET(SetPresharedKeyForId_IN,
                                        Id) +
                in->IdSize +
                in->KeySize;
            if (InBufferSize >= sizeExpected)
            {
                key = OffsetToPtr(in, FIELD_OFFSET(SetPresharedKeyForId_IN,
                                                   Id) +
                                  in->IdSize);

                DebugPrint((iScsiPrtDebugError,
                    "iScsiSetPresharedKeyForId: in %p type %d idlen %x at %p KeyLen is %x at %p\n",
                    in,
                    in->IdType,
                    in->IdSize,
                    in->Id,
                    in->KeySize,
                    key));

                ETWDebugPrint(iScsiLevelError,
                    iScsiFlagWmi,
                    "iScsiSetPresharedKeyForId: in %p type %d idlen %x at %p KeyLen is %x at %p\n",
                    in,
                    in->IdType,
                    in->IdSize,
                    in->Id,
                    in->KeySize,
                    key);

                out = (PSetPresharedKeyForId_OUT)Buffer;
                out->Status = STATUS_SUCCESS;
                status = SRB_STATUS_SUCCESS;
            }
        }
    } else {
        status = SRB_STATUS_DATA_OVERRUN;
    }

    return(status);
}


UCHAR iScsiSetGroupPresharedKey(
    PISCSI_ADAPTER_EXTENSION AdapterExtension,
    ULONG InBufferSize,
    ULONG OutBufferSize,
    PULONG SizeNeeded,
    PUCHAR Buffer
    )
{
    PSetGroupPresharedKey_IN in;
    PSetGroupPresharedKey_OUT out;
    UCHAR status;
    ULONG sizeExpected;

    *SizeNeeded = sizeof(SetGroupPresharedKey_OUT);

    if (OutBufferSize >= *SizeNeeded)
    {
        status = SRB_STATUS_ERROR;
        if (InBufferSize >= FIELD_OFFSET(SetGroupPresharedKey_IN,
                                         Key))
        {
            in = (PSetGroupPresharedKey_IN)Buffer;
            sizeExpected = FIELD_OFFSET(SetGroupPresharedKey_IN,
                                        Key) + in->KeySize;
            if (InBufferSize >= sizeExpected)
            {
                DebugPrint((iScsiPrtDebugError,
                    "iScsiSetGroupPresharedKey: in %p KeyLen is %x at %p\n",
                    in,
                    in->KeySize,
                    in->Key));

                ETWDebugPrint(iScsiLevelError, iScsiFlagWmi,
                    "iScsiSetGroupPresharedKey: in %p KeyLen is %x at %p\n",
                    in,
                    in->KeySize,
                    in->Key);

                out = (PSetGroupPresharedKey_OUT)Buffer;
                out->Status = STATUS_SUCCESS;
                status = SRB_STATUS_SUCCESS;
            }
        }
    } else {
        status = SRB_STATUS_DATA_OVERRUN;
    }

    return(status);
}

UCHAR iScsiSetTunnelModeOuterAddress(
    PISCSI_ADAPTER_EXTENSION AdapterExtension,
    ULONG InBufferSize,
    ULONG OutBufferSize,
    PULONG SizeNeeded,
    PUCHAR Buffer
    )
{
    PSetTunnelModeOuterAddress_IN in;
    PSetTunnelModeOuterAddress_OUT out;
    UCHAR status;
    ULONG sizeExpected;

    *SizeNeeded = sizeof(SetTunnelModeOuterAddress_OUT);

    if (OutBufferSize >= *SizeNeeded)
    {
        status = SRB_STATUS_ERROR;
        if (InBufferSize >= sizeof(SetTunnelModeOuterAddress_IN))
        {
            in = (PSetTunnelModeOuterAddress_IN)Buffer;

            DebugPrint((iScsiPrtDebugError,
                "iScsiSetTunnelModeOuterAddress: in %p port %x\n",
                in,
                in->PortNumber));

            ETWDebugPrint(iScsiLevelError,
                iScsiFlagWmi,
                "iScsiSetTunnelModeOuterAddress: in %p port %x\n",
                in,
                in->PortNumber);

            out = (PSetTunnelModeOuterAddress_OUT)Buffer;
            out->Status = STATUS_SUCCESS;
            status = SRB_STATUS_SUCCESS;
        }
    } else {
        status = SRB_STATUS_DATA_OVERRUN;
    }

    return(status);
}

UCHAR iScsiReportDiscoveredTargets2(
    PISCSI_ADAPTER_EXTENSION AdapterExtension,
    ULONG BufferSize,
    PULONG SizeNeeded,
    PUCHAR Buffer
    )
{
    UCHAR status;
    PReportDiscoveredTargets2_OUT out;
    ULONG size;


    *SizeNeeded = FIELD_OFFSET(ReportDiscoveredTargets2_OUT,
                               Targets);
    if (BufferSize >= *SizeNeeded)
    {
        out = (PReportDiscoveredTargets2_OUT)Buffer;
        out->Status = STATUS_SUCCESS;
        out->TargetCount = 3;

        BufferSize -= *SizeNeeded;
    }

    //
    // Advance buffer pointer, do this even if there is not enough room
    // so the size calculations can take into account any padding
    //
    Buffer += *SizeNeeded;

    iScsiAddDiscoveredTarget(&Buffer,
                             &BufferSize,
                             SizeNeeded,
                             "Target1",
                             "Alias",
                             "DiscoveryDomain",
                             3,
                             1);

    iScsiAddPortalGroup(&Buffer,
                        &BufferSize,
                        SizeNeeded,
                        1);

    status = iScsiAddPortalToGroup(&Buffer,
                                   &BufferSize,
                                   SizeNeeded,
                                   0x12345678,
                                   "SymbolicName",
                                   5003,
                                   0x11223344,
                                   0,
                                   NULL
                                   );


    //
    // Target 2, 2 portal groups, 1 portal and 2 portals
    //
    iScsiAddDiscoveredTarget(&Buffer,
                             &BufferSize,
                             SizeNeeded,
                             "Target2",
                             NULL,
                             "DiscoveryDomain",
                             3,
                             2);

    iScsiAddPortalGroup(&Buffer,
                        &BufferSize,
                        SizeNeeded,
                        1);

    status = iScsiAddPortalToGroup(&Buffer,
                                   &BufferSize,
                                   SizeNeeded,
                                   0x11111111,
                                   "SymbolicName",
                                   5003,
                                   0x44332211,
                                   8,
                                   "12345678");

    iScsiAddPortalGroup(&Buffer,
                        &BufferSize,
                        SizeNeeded,
                        2);

    status = iScsiAddPortalToGroup(&Buffer,
                                   &BufferSize,
                                   SizeNeeded,
                                   0x12345678,
                                   "SymbolicName1",
                                   5003,
                                   0x23,
                                   8,
                                   "12345678");

    status = iScsiAddPortalToGroup(&Buffer,
                                   &BufferSize,
                                   SizeNeeded,
                                   0x12345679,
                                   "SymbolicName2",
                                   5003,
                                   0x34,
                                   8,
                                   "12345678");

    //
    // Target 3
    //
    iScsiAddDiscoveredTarget(&Buffer,
                             &BufferSize,
                             SizeNeeded,
                             "Target3",
                             NULL,
                             NULL,
                             3,
                             2);

    iScsiAddPortalGroup(&Buffer,
                        &BufferSize,
                        SizeNeeded,
                        5);

    status = iScsiAddPortalToGroup(&Buffer,
                                   &BufferSize,
                                   SizeNeeded,
                                   0x11111111,
                                   "SymbolicName1",
                                   5003,
                                   0x23,
                                   0,
                                   NULL);

    status = iScsiAddPortalToGroup(&Buffer,
                                   &BufferSize,
                                   SizeNeeded,
                                   0x11111113,
                                   "SymbolicName2",
                                   5003,
                                   0x23,
                                   0,
                                   NULL);

    status = iScsiAddPortalToGroup(&Buffer,
                                   &BufferSize,
                                   SizeNeeded,
                                   0x11111113,
                                   NULL,
                                   5003,
                                   0x23,
                                   0,
                                   NULL);

    status = iScsiAddPortalToGroup(&Buffer,
                                   &BufferSize,
                                   SizeNeeded,
                                   0x11111114,
                                   NULL,
                                   5003,
                                   0x23,
                                   0,
                                   NULL);

    status = iScsiAddPortalToGroup(&Buffer,
                                   &BufferSize,
                                   SizeNeeded,
                                   0x11111115,
                                   "SymbolicName5",
                                   5003,
                                   0x23,
                                   0,
                                   NULL);

    iScsiAddPortalGroup(&Buffer,
                        &BufferSize,
                        SizeNeeded,
                        2);

    status = iScsiAddPortalToGroup(&Buffer,
                                   &BufferSize,
                                   SizeNeeded,
                                   0x12345678,
                                   "SymbolicName1",
                                   5003,
                                   0x23,
                                   0,
                                   NULL);

    status = iScsiAddPortalToGroup(&Buffer,
                                   &BufferSize,
                                   SizeNeeded,
                                   0x12345679,
                                   "SymbolicName2",
                                   5003,
                                   0x23,
                                   0,
                                   NULL);

    return(status);
}

UCHAR
iSpReadTCPConfigInfo(
    IN PISCSI_ADAPTER_EXTENSION AdapterExtension,
    OUT PUCHAR Buffer
   )
/*++

Routine Description:

   Read the iScsi Initiator Instance TCP configuration into the OS buffer.

Arguments:

   AdapterExtension - miniport drivers storage
   Buffer  - Used to store all the information of a paticular initiator

Return Value:

   Status - SRB Status for this request

--*/
{
    PMSiSCSI_TCPIPConfig TCPConfigData;
    UCHAR status;


    TCPConfigData = (PMSiSCSI_TCPIPConfig)Buffer;
    RtlZeroMemory(TCPConfigData, sizeof(MSiSCSI_TCPIPConfig));

    RtlCopyMemory(TCPConfigData,
                  &(AdapterExtension->TCPConfigData),
                  sizeof(MSiSCSI_TCPIPConfig));

    if (TCPConfigData->IpAddress.TextAddress[0] == 0)
    {
        TCPConfigData->IpAddress.TextAddress[0] = 256 * sizeof(WCHAR);
    }

    if (TCPConfigData->DefaultGateway.TextAddress[0] == 0)
    {
        TCPConfigData->DefaultGateway.TextAddress[0] = 256 * sizeof(WCHAR);
    }

    if (TCPConfigData->SubnetMask.TextAddress[0] == 0)
    {
        TCPConfigData->SubnetMask.TextAddress[0] = 256 * sizeof(WCHAR);
    }

    if (TCPConfigData->PreferredDNSServer.TextAddress[0] == 0)
    {
        TCPConfigData->PreferredDNSServer.TextAddress[0] = 256 * sizeof(WCHAR);
    }

    if (TCPConfigData->AlternateDNSServer.TextAddress[0] == 0)
    {
        TCPConfigData->AlternateDNSServer.TextAddress[0] = 256 * sizeof(WCHAR);
    }

    return SRB_STATUS_SUCCESS;
}


UCHAR
iSpReadNICConfigInfo(
    IN PISCSI_ADAPTER_EXTENSION AdapterExtension,
    OUT PUCHAR Buffer
   )
/*++

Routine Description:

   Read the iScsi Initiator Instance statistics information into the OS buffer.

Arguments:

   AdapterExtension - miniport drivers storage
   Buffer                  - Used to store all the information of a paticular initiator

Return Value:

   Status - SRB Status for this request

--*/
{
    PMSiSCSI_NICConfig NICConfigData;
    UCHAR status;


    NICConfigData = (PMSiSCSI_NICConfig)Buffer;

    RtlZeroMemory(NICConfigData, sizeof(MSiSCSI_NICConfig));

    // Link Speed
    NICConfigData->LinkSpeed = 100000000;

    // Link State
    NICConfigData->LinkState = ISCSI_NIC_LINKSTATE_CONNECTED;

    NICConfigData->MaxFrameSize = 65536;

    // Ethernet MAC Address
    NICConfigData->MacAddress[0] = 0x31;
    NICConfigData->MacAddress[1] = 0x32;
    NICConfigData->MacAddress[2] = 0x33;
    NICConfigData->MacAddress[3] = 0x34;
    NICConfigData->MacAddress[4] = 0x35;
    NICConfigData->MacAddress[5] = 0x36;

    status = SRB_STATUS_SUCCESS;
    return status;
}


UCHAR
iSpSetTCPConfigInfo(
    IN PISCSI_ADAPTER_EXTENSION AdapterExtension,
    IN ULONG BufferSize,
    IN PUCHAR Buffer
    )
/*++

Routine Description:

   Sets the content in the Buffer to the drivers adapterExtension

Arguments:

   AdapterExtension - iScsi miniport driver's adapter data storage.

   BufferSize - Specifies the size in bytes of the buffer at Buffer.

   Buffer - Points to a buffer that contains new values for the instance.

Return Value:

   UCHAR - SRB Status for this request

--*/
{
    PMSiSCSI_TCPIPConfig PTCPConfigBuffer;
    PISCSI_IP_Address iScsiIPAddress;
    PISCSI_IP_Address destiScsiIPAddress;
    PUCHAR currentIpData;
    ULONG sizeNeeded;
    ULONG inx;
    UCHAR status = SRB_STATUS_SUCCESS;

    PTCPConfigBuffer = (PMSiSCSI_TCPIPConfig)Buffer;

    sizeNeeded = FIELD_OFFSET(MSiSCSI_TCPIPConfig, IpAddress);

    if ((Buffer == NULL) || (BufferSize < sizeNeeded)) {

        return SRB_STATUS_ERROR;
    }

    RtlZeroMemory(&(AdapterExtension->TCPConfigData),
                  sizeof(MSiSCSI_TCPIPConfig));

    AdapterExtension->TCPConfigData.UseLinkLocalAddress =
        PTCPConfigBuffer->UseLinkLocalAddress;

    AdapterExtension->TCPConfigData.EnableDHCP =
        PTCPConfigBuffer->EnableDHCP;

    BufferSize -= sizeNeeded;

    iScsiIPAddress = &(PTCPConfigBuffer->IpAddress);

    destiScsiIPAddress = &(AdapterExtension->TCPConfigData.IpAddress);

    //
    // 5 is the number of ISCSI_IP_Address entries
    // in MSiSCSI_TCPIPConfig structure
    //
    inx = 0;
    while (inx < 5) {

        status = SRB_STATUS_SUCCESS;

        sizeNeeded = FIELD_OFFSET(ISCSI_IP_Address, TextAddress) + sizeof(WCHAR);

        if (BufferSize < sizeNeeded) {

            status = SRB_STATUS_ERROR;
        } else {

            sizeNeeded += iScsiIPAddress->TextAddress[0];

            if (BufferSize < sizeNeeded) {
                status = SRB_STATUS_ERROR;
            }
        }

        if (status == SRB_STATUS_SUCCESS) {

            BufferSize -= sizeNeeded;

            destiScsiIPAddress->Type = iScsiIPAddress->Type;

            iScsiIPAddress->TextAddress[0] =
                MAX_ISCSI_TEXT_ADDRESS_LEN * sizeof(WCHAR);

            switch (iScsiIPAddress->Type) {
                case ISCSI_IP_ADDRESS_TEXT:
                {
                    RtlCopyMemory(&(destiScsiIPAddress->TextAddress[1]),
                                  &(iScsiIPAddress->TextAddress[1]),
                                  iScsiIPAddress->TextAddress[0]);

                    break;
                }

                case ISCSI_IP_ADDRESS_IPV4:
                {
                    destiScsiIPAddress->IpV4Address =
                        iScsiIPAddress->IpV4Address;

                    break;
                }

                case ISCSI_IP_ADDRESS_IPV6:
                {
                    RtlCopyMemory(destiScsiIPAddress->IpV6Address,
                                  iScsiIPAddress->IpV6Address,
                                  sizeof(iScsiIPAddress->IpV6Address));

                    destiScsiIPAddress->IpV6FlowInfo =
                        iScsiIPAddress->IpV6FlowInfo;

                    destiScsiIPAddress->IpV6ScopeId =
                        iScsiIPAddress->IpV6ScopeId;

                    break;
                }

                default:
                {
                    status = SRB_STATUS_ERROR;
                    return(status);
                    break;
                }
            }

            iScsiIPAddress = (PISCSI_IP_Address) ((PUCHAR)(iScsiIPAddress) +
                                                  ((sizeNeeded + 3) & ~3));

            destiScsiIPAddress++;

            inx++;

        } else {

            break;
        }
    }

    return status;
}

UCHAR
iScsiSetWmiDataBlock(
    IN PVOID Context,
    IN PSCSIWMI_REQUEST_CONTEXT DispatchContext,
    IN ULONG GuidIndex,
    IN ULONG InstanceIndex,
    IN ULONG BufferSize,
    IN PUCHAR Buffer
    )
/*+++

Routine Description:

    This routine is called to set a WMI data block.

--*/
{
    PISCSI_ADAPTER_EXTENSION adapterExtension;
    ULONG sizeNeeded = 0;
    PSCSI_WMI_REQUEST_BLOCK  srb;
    UCHAR status;

    DebugPrint((iScsiPrtDebugTrace,
       "SetWmiDataBlock : GuidIndex - %x\n",
       GuidIndex));

    ETWDebugPrint(iScsiLevelTrace,
        iScsiFlagWmi,
       "SetWmiDataBlock : GuidIndex - %x\n",
       GuidIndex);

    adapterExtension = (PISCSI_ADAPTER_EXTENSION) Context;
    srb = (PSCSI_WMI_REQUEST_BLOCK) DispatchContext->UserContext;

    switch (GuidIndex)
    {
        case iScsiDiscoveryConfigGuid_INDEX:
        {
            status = iScsiSetDiscoveryConfig(adapterExtension,
                                             BufferSize,
                                             Buffer);
            sizeNeeded = 0;
            break;
        }

        case iScsi_TCPConfig_GUID_Index:
        {
            status = iSpSetTCPConfigInfo(adapterExtension, BufferSize, Buffer);
            break;
        }

        default:
        {
            status = SRB_STATUS_ERROR;
            break;
        }
    }

    iSpCompleteWmiRequest(adapterExtension, srb, DispatchContext, status, sizeNeeded);

    //
    // return SRB_STATUS_PENDING since the request has either already been
    // completed in iSpCompleteWmiRequest or the status is really
    // SRB_STATUS_PENDING. This implies that the iScsiWmiSrb routine
    // may not touch the srb since it is already completed.
    //
    return SRB_STATUS_PENDING;

}

UCHAR iScsiPerformPing(
    PISCSI_ADAPTER_EXTENSION AdapterExtension,
    PISCSI_IP_Address IpAddress,
    ULONG RequestCount,
    ULONG RequestSize,
    ULONG Timeout,
    ISDSC_STATUS *Status,
    ULONG *ResponseCount
)
{
    //
    // Here is where the HBA should do the ping operations. Note that
    // this code doesn't pend, but your code should pend as the ping
    // operation could be lengthy.
    //
    *Status = STATUS_SUCCESS;
    *ResponseCount = RequestCount;

    return(SRB_STATUS_SUCCESS);
}

UCHAR iSCSIValidateIPAddress(
    PISCSI_IP_Address IpAddress,
    ULONG BufferSize
    )
{
    ULONG sizeUsed;
    UCHAR srbStatus;

    sizeUsed = (FIELD_OFFSET(ISCSI_IP_Address,
                                   TextAddress) +
                       sizeof(USHORT));

    if (BufferSize >= sizeUsed)
    {
        BufferSize -= sizeUsed;
        sizeUsed = IpAddress->TextAddress[0];

        if (BufferSize >= sizeUsed)
        {
            srbStatus = SRB_STATUS_SUCCESS;
        } else {
            srbStatus = SRB_STATUS_INVALID_REQUEST;
        }
    } else {
        srbStatus = SRB_STATUS_INVALID_REQUEST;
    }

    return(srbStatus);
}

UCHAR iSCSIPerformPingOperation(
    PISCSI_ADAPTER_EXTENSION AdapterExtension,
    ULONG InBufferSize,
    ULONG OutBufferSize,
    ULONG *SizeNeeded,
    PUCHAR Buffer
    )
{
    ULONG requestCount;
    ULONG requestSize;
    ULONG timeout;
    ULONG responseCount;
    PISCSI_IP_Address ipAddress;
    UCHAR srbStatus;
    PPingIPAddress_OUT pingOut;
    PPingIPAddress_IN pingIn;
    ISDSC_STATUS status;


    //
    // Make sure there is enough room in out buffer
    //
    *SizeNeeded = sizeof(PingIPAddress_OUT);

    if (OutBufferSize >= *SizeNeeded)
    {
        if (InBufferSize > FIELD_OFFSET(PingIPAddress_IN,
                                        Address))
        {
            pingIn = (PPingIPAddress_IN)Buffer;
            requestCount = pingIn->RequestCount;
            requestSize = pingIn->RequestSize;
            timeout = pingIn->Timeout;
            ipAddress = &pingIn->Address;

            srbStatus = iSCSIValidateIPAddress(ipAddress,
                                               InBufferSize -
                                               FIELD_OFFSET(PingIPAddress_IN,
                                                            Address)
                                              );

            if (srbStatus == SRB_STATUS_SUCCESS)
            {
                srbStatus = iScsiPerformPing(AdapterExtension,
                                          ipAddress,
                                          requestCount,
                                          requestSize,
                                          timeout,
                                          &status,
                                          &responseCount);

                //
                // return the results of the ping
                //
                pingOut = (PPingIPAddress_OUT)Buffer;
                pingOut->Status = status;
                pingOut->ResponsesReceived = responseCount;
            }

        } else {
            //
            // Input buffer is not correct size
            //
            srbStatus = SRB_STATUS_INVALID_REQUEST;
        }

    } else {
        srbStatus = SRB_STATUS_DATA_OVERRUN;
    }

    return(srbStatus);
}


UCHAR
iScsiExecuteWmiMethod(
    IN PVOID Context,
    IN PSCSIWMI_REQUEST_CONTEXT DispatchContext,
    IN ULONG GuidIndex,
    IN ULONG InstanceIndex,
    IN ULONG MethodId,
    IN ULONG InBufferSize,
    IN ULONG OutBufferSize,
    IN OUT PUCHAR Buffer
    )
/*+++

Routine Description:

    This routine is called to execute a WMI method.
    The methods here include Login, Logout, SendTargets, etc

--*/
{
    PISCSI_ADAPTER_EXTENSION adapterExtension;
    PSCSI_WMI_REQUEST_BLOCK  srb;
    PISCSI_SRB_EXTENSION srbExtension;
    ULONG sizeNeeded = 0;
    UCHAR status = SRB_STATUS_SUCCESS;

    DebugPrint((iScsiPrtDebugTrace,
        "ExecuteWMIMethod : GuidIndex - %x, MethodId - %x\n",
        GuidIndex,
        MethodId));

    ETWDebugPrint(iScsiLevelTrace,
        iScsiFlagWmi,
        "ExecuteWMIMethod : GuidIndex - %x, MethodId - %x\n",
        GuidIndex,
        MethodId);

    adapterExtension = (PISCSI_ADAPTER_EXTENSION) Context;

    srb = (PSCSI_WMI_REQUEST_BLOCK) DispatchContext->UserContext;

    srbExtension = (PISCSI_SRB_EXTENSION) srb->SrbExtension;

    //
    // Save the input parameters in the SRB extension
    //
    srbExtension->Buffer = Buffer;
    srbExtension->InBufferSize = InBufferSize;
    srbExtension->OutBufferSize = OutBufferSize;

    //
    // Note that in all the WMI calls, if we are setting the status
    // in Buffer (loginTargetOut, logoutTargetOut, etc), we should
    // always return SRB_STATUS_SUCCESS. Status field will be set
    // to STATUS_SUCCESS if the operation is successful, and will
    // be set to the approproate error code if there was an error.
    //
    switch (GuidIndex) {
        case iSCSI_Operations_GUID_Index: {

            DebugPrint((iScsiPrtDebugTrace,
                "iSCSI Operations method %d\n",
                MethodId));

            ETWDebugPrint(iScsiLevelTrace,
                iScsiFlagWmi,
                "iSCSI Operations method %d\n",
                MethodId);

            switch (MethodId) {
                case LoginToTarget: {

                    PLoginToTarget_IN  loginTargetIn;

                    sizeNeeded = sizeof(LoginToTarget_OUT);

                    if (InBufferSize < sizeof(LoginToTarget_IN)) {
                        status = SRB_STATUS_INVALID_REQUEST;
                        break;
                    }

                    if (OutBufferSize < sizeof(LoginToTarget_OUT)) {
                        status = SRB_STATUS_DATA_OVERRUN;
                        break;
                    }

                    loginTargetIn = (PLoginToTarget_IN) Buffer;

                    status = iSpProcessLoginToTarget(adapterExtension,
                                                     loginTargetIn,
                                                     srb,
                                                     InBufferSize);
                    break;
                }

                case LogoutFromTarget: {

                    PLogoutFromTarget_IN logoutTargetIn;

                    sizeNeeded = sizeof(LogoutFromTarget_OUT);

                    if (InBufferSize < sizeof(LogoutFromTarget_IN)) {
                        status = SRB_STATUS_INVALID_REQUEST;
                        break;
                    }

                    if (OutBufferSize < sizeof(LogoutFromTarget_OUT)) {
                        status = SRB_STATUS_DATA_OVERRUN;
                        break;
                    }

                    logoutTargetIn = (PLogoutFromTarget_IN) Buffer;

                    status = iSpProcessLogoutFromTarget(adapterExtension,
                                                        logoutTargetIn,
                                                        srb);
                    break;
                }

                case AddConnectionToSession: {

                    PAddConnectionToSession_IN addConnectionIn;

                    sizeNeeded = sizeof(AddConnectionToSession_OUT);

                    if (InBufferSize < sizeof(AddConnectionToSession_IN)) {
                        status = SRB_STATUS_INVALID_REQUEST;
                        break;
                    }

                    if (OutBufferSize < sizeof(AddConnectionToSession_OUT)) {
                        status = SRB_STATUS_DATA_OVERRUN;
                        break;
                    }

                    addConnectionIn = (PAddConnectionToSession_IN) Buffer;

                    status = iSpProcessAddConnectionToSession(adapterExtension,
                                                              addConnectionIn,
                                                              srb,
                                                              InBufferSize);
                    break;
                }

                case RemoveConnectionFromSession: {

                    PRemoveConnectionFromSession_IN removeConnectionIn;

                    if (InBufferSize < sizeof(RemoveConnectionFromSession_IN)) {

                        status = SRB_STATUS_INVALID_REQUEST;
                        break;
                    }

                    if (OutBufferSize < sizeof(RemoveConnectionFromSession_OUT)) {

                        status = SRB_STATUS_DATA_OVERRUN;
                        break;
                    }

                    removeConnectionIn = (PRemoveConnectionFromSession_IN) Buffer;

                    sizeNeeded = sizeof(RemoveConnectionFromSession_OUT);

                    status = iSpProcessRemoveConnectionToSession(adapterExtension,
                                                                 removeConnectionIn,
                                                                 srb,
                                                                 InBufferSize);
                    break;
                }

                case SendTargets: {

                    PSendTargets_IN sendTargetsIn;

                    sizeNeeded = sizeof(SendTargets_OUT);

                    if (InBufferSize < sizeof(SendTargets_IN)) {
                        status = SRB_STATUS_INVALID_REQUEST;
                        break;
                    }

                    if (OutBufferSize < sizeof(SendTargets_OUT)) {
                        status = SRB_STATUS_DATA_OVERRUN;
                        break;
                    }

                    sendTargetsIn = (PSendTargets_IN) Buffer;

                    status = iSpProcessSendTargets(adapterExtension,
                                                   sendTargetsIn,
                                                   srb);

                    break;
                }

                case ScsiInquiry: {

                    PScsiInquiry_IN scsiInquiryIn;

                    sizeNeeded = sizeof(ScsiInquiry_OUT);

                    if (InBufferSize < sizeof(ScsiInquiry_IN)) {
                        status = SRB_STATUS_INVALID_REQUEST;
                        break;
                    }

                    if (OutBufferSize < sizeof(ScsiInquiry_OUT)) {
                        status = SRB_STATUS_DATA_OVERRUN;
                        break;
                    }

                    scsiInquiryIn = (PScsiInquiry_IN) Buffer;

                    status = iSpProcessScsiInquiry(adapterExtension,
                                                   scsiInquiryIn,
                                                   srb);
                    break;
                }

                case ScsiReadCapacity: {

                    PScsiReadCapacity_IN scsiReadCapacityIn;

                    sizeNeeded = sizeof(ScsiReadCapacity_OUT);

                    if (InBufferSize < sizeof(ScsiReadCapacity_IN)) {
                        status = SRB_STATUS_INVALID_REQUEST;
                        break;
                    }

                    if (OutBufferSize < sizeof(ScsiReadCapacity_OUT)) {
                        status = SRB_STATUS_DATA_OVERRUN;
                        break;
                    }

                    scsiReadCapacityIn = (PScsiReadCapacity_IN) Buffer;

                    status = iSpProcessScsiReadCapacity(adapterExtension,
                                                        scsiReadCapacityIn,
                                                        srb);

                    break;
                }

                case ScsiReportLuns: {

                    PScsiReportLuns_IN scsiReportLunsIn;

                    sizeNeeded = sizeof(ScsiReportLuns_OUT);

                    if (InBufferSize < sizeof(ScsiReportLuns_IN)) {
                        status = SRB_STATUS_INVALID_REQUEST;
                        break;
                    }

                    if (OutBufferSize < sizeof(ScsiReportLuns_OUT)) {
                        status = SRB_STATUS_DATA_OVERRUN;
                        break;
                    }

                    scsiReportLunsIn = (PScsiReportLuns_IN) Buffer;

                    status = iSpProcessScsiReportLuns(adapterExtension,
                                                      scsiReportLunsIn,
                                                      srb);
                    break;
                }

                case SetCHAPSharedSecret: {

                    PSetCHAPSharedSecret_IN  setCHAPSecretIn;
                    PSetCHAPSharedSecret_OUT setCHAPSecretOut;
                    ULONG secretSize;
                    ULONG bufferSizeNeeded;

                    setCHAPSecretOut = (PSetCHAPSharedSecret_OUT) Buffer;

                    sizeNeeded = 0;

                    RtlZeroMemory(adapterExtension->InitiatorSecret,
                                  sizeof(adapterExtension->InitiatorSecret));

                    adapterExtension->InitiatorSecretLen = 0;

                    if (InBufferSize >= (ULONG)FIELD_OFFSET(SetCHAPSharedSecret_IN,
                                                            SharedSecret)) {

                        setCHAPSecretIn = (PSetCHAPSharedSecret_IN) Buffer;

                        secretSize = setCHAPSecretIn->SharedSecretSize;

                        if (secretSize == 0) {

                            setCHAPSecretOut->Status = STATUS_SUCCESS;

                            status = SRB_STATUS_SUCCESS;

                        } else if (secretSize > MAX_CHAP_SECRET_LENGTH) {

                            setCHAPSecretOut->Status = ISDSC_NON_SPECIFIC_ERROR;

                            status = SRB_STATUS_SUCCESS;

                        } else {

                            bufferSizeNeeded = secretSize +
                                               FIELD_OFFSET(SetCHAPSharedSecret_IN,
                                                            SharedSecret);

                            if (InBufferSize >= bufferSizeNeeded) {

                                RtlCopyMemory(adapterExtension->InitiatorSecret,
                                              setCHAPSecretIn->SharedSecret,
                                              secretSize);

                                adapterExtension->InitiatorSecretLen = secretSize;

                                status = SRB_STATUS_SUCCESS;

                                setCHAPSecretOut->Status = STATUS_SUCCESS;
                            } else {

                                status = SRB_STATUS_INVALID_REQUEST;

                                setCHAPSecretOut->Status = ISDSC_NON_SPECIFIC_ERROR;
                            }
                        }

                        sizeNeeded = sizeof(SetCHAPSharedSecret_OUT);

                    } else {

                        status = SRB_STATUS_INVALID_REQUEST;
                    }

                    break;
                }

                case SetRADIUSSharedSecret: {

                    PSetRADIUSSharedSecret_IN  setRADIUSSecretIn;
                    PSetRADIUSSharedSecret_OUT setRADIUSSecretOut;
                    ULONG secretSize;
                    ULONG bufferSizeNeeded;

                    setRADIUSSecretOut = (PSetRADIUSSharedSecret_OUT) Buffer;

                    sizeNeeded = 0;

                    RtlZeroMemory(adapterExtension->RadiusSecret,
                                  sizeof(adapterExtension->RadiusSecret));

                    adapterExtension->RadiusSecretLen = 0;

                    if (InBufferSize >= (ULONG)FIELD_OFFSET(SetRADIUSSharedSecret_IN,
                                                            SharedSecret)) {

                        setRADIUSSecretIn = (PSetRADIUSSharedSecret_IN) Buffer;

                        secretSize = setRADIUSSecretIn->SharedSecretSize;

                        if (secretSize == 0) {

                            setRADIUSSecretOut->Status = STATUS_SUCCESS;

                            status = SRB_STATUS_SUCCESS;

                        } else if (secretSize > MAX_RADIUS_SECRET_LENGTH) {

                            setRADIUSSecretOut->Status = ISDSC_NON_SPECIFIC_ERROR;

                            status = SRB_STATUS_SUCCESS;

                        } else {

                            bufferSizeNeeded = secretSize +
                                               FIELD_OFFSET(SetRADIUSSharedSecret_IN,
                                                            SharedSecret);

                            if (InBufferSize >= bufferSizeNeeded) {

                                RtlCopyMemory(adapterExtension->RadiusSecret,
                                              setRADIUSSecretIn->SharedSecret,
                                              secretSize);

                                adapterExtension->RadiusSecretLen = secretSize;

                                status = SRB_STATUS_SUCCESS;

                                setRADIUSSecretOut->Status = STATUS_SUCCESS;
                            } else {

                                status = SRB_STATUS_INVALID_REQUEST;

                                setRADIUSSecretOut->Status = ISDSC_NON_SPECIFIC_ERROR;
                            }
                        }

                        sizeNeeded = sizeof(SetRADIUSSharedSecret_OUT);

                    } else {

                        status = SRB_STATUS_INVALID_REQUEST;
                    }

                    break;
                }

                case RemovePersistentLogin: {

                    PRemovePersistentLogin_IN removePersistentLoginIn;

                    sizeNeeded = sizeof(RemovePersistentLogin_OUT);

                    if (InBufferSize < sizeof(RemovePersistentLogin_IN)) {
                        status = SRB_STATUS_INVALID_REQUEST;
                        break;
                    }

                    if (OutBufferSize < sizeof(RemovePersistentLogin_OUT)) {
                        status = SRB_STATUS_DATA_OVERRUN;
                        break;
                    }

                    removePersistentLoginIn = (PRemovePersistentLogin_IN) Buffer;

                    status = iSpRemovePersistentLogin(adapterExtension,
                                                      removePersistentLoginIn,
                                                      srb);
                    break;
                }

                case AddRADIUSServer: {

                    PAddRADIUSServer_IN  addRADIUSServerIn;
                    PAddRADIUSServer_OUT addRADIUSServerOut;
                    ULONG newServerNameLength;
                    PISCSI_IP_Address list;
                    ULONG count;
                    ULONG updatedCount;
                    ULONG size;
                    ULONG inx = 0;
                    BOOLEAN addressFound = FALSE;

                    sizeNeeded = sizeof(AddRADIUSServer_OUT);

                    if (InBufferSize < sizeof(AddRADIUSServer_IN)) {
                        status = SRB_STATUS_INVALID_REQUEST;
                        break;
                    }

                    if (OutBufferSize < sizeof(AddRADIUSServer_OUT)) {
                        status = SRB_STATUS_DATA_OVERRUN;
                        break;
                    }

                    addRADIUSServerIn = (PAddRADIUSServer_IN) Buffer;

                    addRADIUSServerOut = (PAddRADIUSServer_OUT) Buffer;

                    list = adapterExtension->RadiusServerList;
                    count = adapterExtension->RadiusServerListCount;

                    while (inx < count) {

                        if (RtlEqualMemory(&(list[inx]),
                                                         &(addRADIUSServerIn->RADIUSIPAddress),
                                                         sizeof(ISCSI_IP_Address))) {

                            addressFound = TRUE;

                            status = SRB_STATUS_SUCCESS;

                            addRADIUSServerOut->Status = STATUS_SUCCESS;

                            break;
                        }

                        inx++;
                    }

                    if (addressFound) {
                        break;
                    }

                    updatedCount = count + 1;
                    size = updatedCount * sizeof(ISCSI_IP_Address);

                    adapterExtension->RadiusServerList = ExAllocatePoolWithTag(NonPagedPoolNx,
                                                             size,
                                                             'vrSR');

                    if (adapterExtension->RadiusServerList != NULL) {

                        RtlZeroMemory(adapterExtension->RadiusServerList,
                                      size);

                        RtlCopyMemory(adapterExtension->RadiusServerList,
                                      &(addRADIUSServerIn->RADIUSIPAddress),
                                      sizeof(ISCSI_IP_Address));

                        if (count > 0) {

                            RtlCopyMemory((adapterExtension->RadiusServerList + 1),
                                          list,
                                          (count * sizeof(ISCSI_IP_Address)));
                        }

                        adapterExtension->RadiusServerListCount = updatedCount;

                        if (list != NULL) {

                            ExFreePool(list);
                        }

                        status = SRB_STATUS_SUCCESS;

                        addRADIUSServerOut->Status = STATUS_SUCCESS;

                    } else {

                        adapterExtension->RadiusServerList = list;

                        status = SRB_STATUS_INVALID_REQUEST;

                        addRADIUSServerOut->Status = ISDSC_OUT_OF_RESOURCES;

                    }

                    break;
                }

                case RemoveRADIUSServer: {

                    PRemoveRADIUSServer_IN  removeRADIUSServerIn;
                    PRemoveRADIUSServer_OUT removeRADIUSServerOut;
                    PISCSI_IP_Address removeAddress;
                    PISCSI_IP_Address list;
                    ULONG count;
                    PWCHAR updatedList;
                    ULONG updatedCount;
                    ULONG itr;
                    PISCSI_IP_Address itrList;
                    BOOLEAN found = FALSE;
                    ULONG size = 0;


                    sizeNeeded = sizeof(RemoveRADIUSServer_OUT);

                    if (InBufferSize < sizeof(RemoveRADIUSServer_IN)) {
                        status = SRB_STATUS_INVALID_REQUEST;
                        break;
                    }

                    if (OutBufferSize < sizeof(RemoveRADIUSServer_OUT)) {
                        status = SRB_STATUS_DATA_OVERRUN;
                        break;
                    }

                    removeRADIUSServerIn = (PRemoveRADIUSServer_IN) Buffer;

                    removeRADIUSServerOut = (PRemoveRADIUSServer_OUT) Buffer;

                    list = adapterExtension->RadiusServerList;
                    count = adapterExtension->RadiusServerListCount;

                    itr = 0;
                    itrList = list;
                    removeAddress = &(removeRADIUSServerIn->RADIUSIPAddress);
                    while (itr < count) {

                        ULONG inx;

                        if (itrList->Type == removeAddress->Type) {

                            if (itrList->Type == ISCSI_IP_ADDRESS_IPV4) {
                                if (itrList->IpV4Address == removeAddress->IpV4Address) {
                                    found = TRUE;
                                }
                            }

                            if (itrList->Type == ISCSI_IP_ADDRESS_IPV6) {
                                for (inx = 0; inx < 16; inx++) {
                                    if (itrList->IpV6Address[inx] == removeAddress->IpV6Address[inx]) {
                                        found = TRUE;
                                    } else {
                                        found = FALSE;
                                        break;
                                    }
                                }
                            }

                            if (itrList->Type == ISCSI_IP_ADDRESS_TEXT) {
                                if (wcscmp(itrList->TextAddress, removeAddress->TextAddress) == 0) {
                                    found = TRUE;
                                }
                            }
                        }

                        if (found)
                            break;

                        itr++;
                        itrList++;
                    }

                    if (found) {

                        updatedCount = count - 1;
                        size = updatedCount * sizeof(ISCSI_IP_Address);

                        if (updatedCount > 0) {

                            adapterExtension->RadiusServerList = ExAllocatePoolWithTag(NonPagedPoolNx,
                                                                         size,
                                                                         'vrSR');

                            if (adapterExtension->RadiusServerList != NULL) {

                                RtlZeroMemory(adapterExtension->RadiusServerList,
                                              size);

                                RtlCopyMemory(adapterExtension->RadiusServerList,
                                              list,
                                              (itr * sizeof(ISCSI_IP_Address)));

                                itr++;
                                itrList++;

                                if (itr < count) {

                                    RtlCopyMemory((adapterExtension->RadiusServerList) + (itr - 1),
                                                  itrList,
                                                  ((count - itr) * sizeof(ISCSI_IP_Address)));
                                }

                                adapterExtension->RadiusServerListCount = updatedCount;

                                ExFreePool(list);

                                status = SRB_STATUS_SUCCESS;

                                removeRADIUSServerOut->Status = STATUS_SUCCESS;

                            } else {

                                adapterExtension->RadiusServerList = list;

                                status = SRB_STATUS_INVALID_REQUEST;

                                removeRADIUSServerOut->Status = ISDSC_OUT_OF_RESOURCES;

                            }
                            } else {

                                adapterExtension->RadiusServerList = NULL;

                                adapterExtension->RadiusServerListCount = updatedCount;

                                ExFreePool(list);

                                status = SRB_STATUS_SUCCESS;

                             removeRADIUSServerOut->Status = STATUS_SUCCESS;
                            }

                    } else {

                        removeRADIUSServerOut->Status = ISDSC_NON_SPECIFIC_ERROR;

                        status = SRB_STATUS_INVALID_REQUEST;
                    }

                    break;
                }

                default: {

                    status = SRB_STATUS_INVALID_REQUEST;

                    break;
                }
            } // switch (MethodId)

            break;
        } // case iSCSI_Operations_GUID_Index:

        case iScsi_LB_Operations_GUID_Index: {

            DebugPrint((iScsiPrtDebugTrace,
                "Set LB policy\n"));

            ETWDebugPrint(iScsiLevelTrace,
                iScsiFlagWmi,
                "Set LB policy\n");

            switch (MethodId) {
                case SetLoadBalancePolicy: {

                    sizeNeeded = sizeof(SetLoadBalancePolicy_OUT);

                    if (OutBufferSize >= sizeNeeded) {

                        status = iSpSetLoadBalancePolicy(adapterExtension,
                                                         srb,
                                                         Buffer,
                                                         InBufferSize);
                    } else {

                        DebugPrint((iScsiPrtDebugError,
                            "Output buffer too small for SetLBPolicy\n"));

                        ETWDebugPrint(iScsiLevelError, iScsiFlagWmi,
                            "Output buffer too small for SetLBPolicy\n");

                        status = SRB_STATUS_DATA_OVERRUN;
                    }

                    break;
                }

                default: {
                    status = SRB_STATUS_INVALID_REQUEST;
                    break;
                }
            }

            break;
        } // iScsi_LB_Operations_GUID_Index

        case iSCSI_DiscoveryOperationsGuid_INDEX: {
            switch (MethodId)
            {
                case ReportDiscoveredTargets2:
                {
                    status = iScsiReportDiscoveredTargets2(adapterExtension,
                                                          OutBufferSize,
                                                          &sizeNeeded,
                                                          Buffer);
                    break;
                }

                default: {
                    status = SRB_STATUS_INVALID_REQUEST;
                    break;
                }
            }
            break;
        } // case iSCSI_DiscoveryOperationsGuid_INDEX

        case iScsi_ManagementOperation_GUID_Index: {
            switch (MethodId)
            {
                case PingIPAddress:
                {
                    status = iSCSIPerformPingOperation(adapterExtension,
                                                       InBufferSize,
                                                       OutBufferSize,
                                                       &sizeNeeded,
                                                       Buffer);
                    break;
                }

                default: {
                    status = SRB_STATUS_INVALID_REQUEST;
                    break;
                }
            }
            break;
        }

        case iSCSI_SecurityConfigOperationsGuid_INDEX:
        {
            switch (MethodId)
            {
                case SetPresharedKeyForId:
                {
                    status = iScsiSetPresharedKeyForId(adapterExtension,
                                                       InBufferSize,
                                                       OutBufferSize,
                                                       &sizeNeeded,
                                                       Buffer);
                    break;
                }

                case SetGroupPresharedKey:
                {
                    status = iScsiSetGroupPresharedKey(adapterExtension,
                                                       InBufferSize,
                                                       OutBufferSize,
                                                       &sizeNeeded,
                                                       Buffer);
                    break;
                }

                case SetTunnelModeOuterAddress:
                {
                    status = iScsiSetTunnelModeOuterAddress(adapterExtension,
                                                            InBufferSize,
                                                            OutBufferSize,
                                                            &sizeNeeded,
                                                            Buffer);
                    break;
                }

                default: {
                    status = SRB_STATUS_INVALID_REQUEST;
                    break;
                }
            }
            break;
        } // case iSCSI_SecurityConfigOperationsGuid_INDEX

        default: {

            status = SRB_STATUS_INVALID_REQUEST;

            break;
        }
    } // switch (GuidIndex)

    //
    // Complete the request here if the request is NOT pending
    //
    if (status != SRB_STATUS_PENDING) {

        iSpCompleteWmiRequest(adapterExtension,
                              srb,
                              DispatchContext,
                              status,
                              sizeNeeded);
    }

    //
    // return SRB_STATUS_PENDING since the request has either already been
    // completed in iSpCompleteWmiRequest or the status is really
    // SRB_STATUS_PENDING. This implies that the iScsiWmiSrb routine
    // may not touch the srb since it is already completed.
    //
    return SRB_STATUS_PENDING;
}


ISDSC_STATUS
iSpValidateLoginParameters(
    IN PISCSI_ADAPTER_EXTENSION AdapterExtension,
    IN PLoginToTarget_IN LoginTargetIn,
    IN ULONG InBufferSize
    )
{
    ULONG sizeNeeded;
    ISCSI_AUTH_TYPES authType;
    BOOLEAN inputValid;


    //
    // Ensure that the given buffer is large enough to hold
    // all login input data
    //
    sizeNeeded = FIELD_OFFSET(LoginToTarget_IN, Mappings) +
                 FIELD_OFFSET(ISCSI_TargetMapping, LUNList);

    if (InBufferSize >= sizeNeeded ) {

        sizeNeeded += (LoginTargetIn->Mappings.LUNCount * sizeof(ISCSI_LUNList)) +
                      LoginTargetIn->KeySize +
                      LoginTargetIn->UsernameSize +
                      LoginTargetIn->PasswordSize;
    }

    if (InBufferSize < sizeNeeded) {

        DebugPrint((iScsiPrtDebugError,
            "Buffer too small for login. Given %d, Needed %d\n",
            InBufferSize, sizeNeeded));

        ETWDebugPrint(iScsiLevelError,
            iScsiFlagProtocolLogInOut,
            "Buffer too small for login. Given %d, Needed %d\n",
            InBufferSize, sizeNeeded);

        return ISDSC_LOGIN_USER_INFO_BAD;
    }

    //
    // Validate session type
    //
    if (LoginTargetIn->SessionType > ISCSI_LOGINTARGET_DATA) {

        DebugPrint((iScsiPrtDebugError,
            "Invalid session type %d\n",
            LoginTargetIn->SessionType));

        ETWDebugPrint(iScsiLevelError,
            iScsiFlagProtocolLogInOut,
            "Invalid session type %d\n",
            LoginTargetIn->SessionType);

        return ISDSC_SESSION_TYPE_NOT_SUPPORTED;
    }

    //
    // Make sure the size of the username and password, if given,
    // are less than or equal to max length
    //
    if (LoginTargetIn->LoginOptions.InformationSpecified & ISCSI_LOGIN_OPTIONS_AUTH_TYPE) {
        authType = LoginTargetIn->LoginOptions.AuthType;
    } else {
        authType = ISCSI_NO_AUTH_TYPE;
    }

    if ((authType == ISCSI_CHAP_AUTH_TYPE) ||
        (authType == ISCSI_MUTUAL_CHAP_AUTH_TYPE)) {

        //
        // A non-zero CHAP Username must be provided for CHAP
        //
        if ((LoginTargetIn->UsernameSize == 0) ||
            (LoginTargetIn->UsernameSize > MAX_CHAP_USERNAME_LENGTH)) {

            DebugPrint((iScsiPrtDebugError,
                "Invalid username size %d\n",
                LoginTargetIn->UsernameSize));

            ETWDebugPrint(iScsiLevelError,
                iScsiFlagProtocolLogInOut,
                "Invalid username size %d\n",
                LoginTargetIn->UsernameSize);

            return ISDSC_INVALID_CHAP_USER_NAME;
        }

        //
        // If doing CHAP then we need to have the target's chap secret
        //
        if (((LoginTargetIn->PasswordSize == 0) && !(LoginTargetIn->LoginOptions.LoginFlags & ISCSI_LOGIN_FLAG_USE_RADIUS_RESPONSE))||
            (LoginTargetIn->PasswordSize > MAX_CHAP_SECRET_LENGTH)) {

            DebugPrint((iScsiPrtDebugError,
                "Invalid target secret size %d\n",
                LoginTargetIn->PasswordSize));

            ETWDebugPrint(iScsiLevelError,
                iScsiFlagProtocolLogInOut,
                "Invalid target secret size %d\n",
                LoginTargetIn->PasswordSize);

            return ISDSC_INVALID_TARGET_CHAP_SECRET;
        }

        //
        // If we are performing mutual authentication ensure
        // that initiator's secret is set, and it is not
        // same as the target's secret.
        //
        if (authType == ISCSI_MUTUAL_CHAP_AUTH_TYPE) {

            PUCHAR userNamePtr;
            PUCHAR targetSecret;
            ULONG secretLen;

            if (!(LoginTargetIn->LoginOptions.LoginFlags & ISCSI_LOGIN_FLAG_USE_RADIUS_VERIFICATION)) {

                iSpGetUserNameAndPasswordForLogin(LoginTargetIn,
                                                  &userNamePtr,
                                                  &targetSecret);

                secretLen = AdapterExtension->InitiatorSecretLen;
                if ((secretLen  == 0) ||
                    ((secretLen == LoginTargetIn->PasswordSize) &&
                     (RtlCompareMemory(AdapterExtension->InitiatorSecret,
                                       targetSecret,
                                       secretLen) == secretLen))) {

                    DebugPrint((iScsiPrtDebugError,
                        "Either no initiator secret is set or it is same as target secret\n"));

                    ETWDebugPrint(iScsiLevelError,
                        iScsiFlagCHAP,
                        "Either no initiator secret is set or it is same as target secret\n");

                    return ISDSC_INVALID_INITIATOR_CHAP_SECRET;
                }
            }
        }
    } else if (authType != ISCSI_NO_AUTH_TYPE) {

        //
        // Only chap and mutual chap are supported
        //

        DebugPrint((iScsiPrtDebugError,
            "Invalid authentication type %d\n",
            authType));

        ETWDebugPrint(iScsiLevelError,
            iScsiFlagProtocolLogInOut,
            "Invalid authentication type %d\n",
            authType);

        return ISDSC_INVALID_LOGON_AUTH_TYPE;
    }

    //
    // Check the address type. We currently only support IPv4 and IPv6
    //
    if ((LoginTargetIn->TargetPortal.Address.Type != ISCSI_IP_ADDRESS_IPV4) &&
        (LoginTargetIn->TargetPortal.Address.Type != ISCSI_IP_ADDRESS_IPV6)) {

        DebugPrint((iScsiPrtDebugError,
            "Invalid address type %d\n",
            LoginTargetIn->TargetPortal.Address.Type));

        ETWDebugPrint(iScsiLevelError,
            iScsiFlagProtocolLogInOut,
            "Invalid address type %d\n",
            LoginTargetIn->TargetPortal.Address.Type);

        return ISDSC_ADDRESS_TYPE_NOT_SUPPORTED;
    }

    inputValid = TRUE;

    //
    // Check if target mappings are valid
    //
    if ((LoginTargetIn->Mappings.OSBus != -1) &&
        (LoginTargetIn->Mappings.OSBus >= ISCSI_MAX_BUSES)) {

        DebugPrint((iScsiPrtDebugError,
            "Invalid OSBus %x\n",
            LoginTargetIn->Mappings.OSBus));

        ETWDebugPrint(iScsiLevelError,
            iScsiFlagProtocolLogInOut,
            "Invalid OSBus %x\n",
            LoginTargetIn->Mappings.OSBus);

        inputValid = FALSE;
    }

    if ((inputValid == TRUE) &&
        (LoginTargetIn->Mappings.OSTarget != -1) &&
        (LoginTargetIn->Mappings.OSTarget >= ISCSI_MAX_TARGETS)) {

        DebugPrint((iScsiPrtDebugError,
            "Invalid OSTarget %x\n",
            LoginTargetIn->Mappings.OSTarget));

        ETWDebugPrint(iScsiLevelError,
            iScsiFlagProtocolLogInOut,
            "Invalid OSTarget %x\n",
            LoginTargetIn->Mappings.OSTarget);

        inputValid = FALSE;
    }

    if (inputValid == TRUE) {

        PISCSI_LUNList lunList;
        ULONG inx;

        for (inx = 0; inx < LoginTargetIn->Mappings.LUNCount; inx++) {

            lunList = &(LoginTargetIn->Mappings.LUNList[inx]);

            if (lunList->OSLUN >= ISCSI_MAX_LOGICAL_UNITS) {

                DebugPrint((iScsiPrtDebugError,
                    "Invalid OSLun %x\n",
                    lunList->OSLUN));

                ETWDebugPrint(iScsiLevelError,
                    iScsiFlagProtocolLogInOut,
                    "Invalid OSLun %x\n",
                    lunList->OSLUN);

                inputValid = FALSE;

                break;
            }
        }
    }

    if (inputValid == FALSE) {
        return ISDSC_INVALID_TARGET_MAPPING;
    }

    return STATUS_SUCCESS;
}


ISDSC_STATUS
iSpValidateAddConnectionParameters(
    IN PISCSI_ADAPTER_EXTENSION   AdapterExtension,
    IN PAddConnectionToSession_IN AddConnectionIn,
    IN ULONG                      InBufferSize
    )
{
    ISDSC_STATUS status = STATUS_SUCCESS;
    ULONG sizeNeeded;
    ISCSI_AUTH_TYPES authType;


    //
    // Make sure the given buffer is large enough to hold
    // all add connection input data.
    //
    sizeNeeded = FIELD_OFFSET(AddConnectionToSession_IN, Key);
    if (sizeNeeded <= InBufferSize) {

        sizeNeeded += AddConnectionIn->KeySize +
                      AddConnectionIn->UsernameSize +
                      AddConnectionIn->PasswordSize;
    }

    if (sizeNeeded > InBufferSize) {

        DebugPrint((iScsiPrtDebugError,
            "Add connection: Size Needed %d, Given %d\n",
            sizeNeeded,
            InBufferSize));

        ETWDebugPrint(iScsiLevelError,
            iScsiFlagProtocolLogInOut,
            "Add connection: Size Needed %d, Given %d\n",
            sizeNeeded,
            InBufferSize);

        return ISDSC_LOGIN_USER_INFO_BAD;
    }

    //
    // Check the address type. We currently only support IPv4 and IPv6
    //
    if ((AddConnectionIn->TargetPortal.Address.Type != ISCSI_IP_ADDRESS_IPV4) &&
        (AddConnectionIn->TargetPortal.Address.Type != ISCSI_IP_ADDRESS_IPV6)) {

        DebugPrint((iScsiPrtDebugError,
            "Invalid address type %d\n",
            AddConnectionIn->TargetPortal.Address.Type));

        ETWDebugPrint(iScsiLevelError,
            iScsiFlagProtocolLogInOut,
            "Invalid address type %d\n",
            AddConnectionIn->TargetPortal.Address.Type);

        return ISDSC_ADDRESS_TYPE_NOT_SUPPORTED;
    }

    //
    // Make sure the size of the username and password, if given,
    // are less than or equal to max length
    //
    if (AddConnectionIn->LoginOptions.InformationSpecified & ISCSI_LOGIN_OPTIONS_AUTH_TYPE) {
        authType = AddConnectionIn->LoginOptions.AuthType;
    } else {
        authType = ISCSI_NO_AUTH_TYPE;
    }

    if ((authType == ISCSI_CHAP_AUTH_TYPE) ||
        (authType == ISCSI_MUTUAL_CHAP_AUTH_TYPE)) {

        //
        // A non-zero CHAP Username must be provided for CHAP
        //
        if ((AddConnectionIn->UsernameSize == 0) ||
            (AddConnectionIn->UsernameSize > MAX_CHAP_USERNAME_LENGTH)) {

            DebugPrint((iScsiPrtDebugError,
                "Invalid username size %d\n",
                AddConnectionIn->UsernameSize));

            ETWDebugPrint(iScsiLevelError,
                iScsiFlagProtocolLogInOut,
                "Invalid username size %d\n",
                AddConnectionIn->UsernameSize);

            return ISDSC_INVALID_CHAP_USER_NAME;
        }

        //
        // If doing CHAP then we need to have the target's chap secret
        //
        if (((AddConnectionIn->PasswordSize == 0) &&
             !(AddConnectionIn->LoginOptions.LoginFlags & ISCSI_LOGIN_FLAG_USE_RADIUS_RESPONSE)) ||
             (AddConnectionIn->PasswordSize > MAX_CHAP_SECRET_LENGTH)) {

            DebugPrint((iScsiPrtDebugError,
                "Invalid target secret size %d\n",
                AddConnectionIn->PasswordSize));

            ETWDebugPrint(iScsiLevelError,
                iScsiFlagProtocolLogInOut,
                "Invalid target secret size %d\n",
                AddConnectionIn->PasswordSize);

            return ISDSC_INVALID_TARGET_CHAP_SECRET;
        }

        //
        // If we are performing mutual authentication ensure
        // that initiator's secret is set, and it is not
        // same as the target's secret.
        //
        if (authType == ISCSI_MUTUAL_CHAP_AUTH_TYPE) {

            PUCHAR userNamePtr;
            PUCHAR targetSecret;
            ULONG secretLen;

            if (!(AddConnectionIn->LoginOptions.LoginFlags & ISCSI_LOGIN_FLAG_USE_RADIUS_VERIFICATION)) {

                iSpGetUserNameAndPasswordForAddConnection(AddConnectionIn,
                                                          &userNamePtr,
                                                          &targetSecret);

                secretLen = AdapterExtension->InitiatorSecretLen;
                if ((secretLen  == 0) ||
                    ((secretLen == AddConnectionIn->PasswordSize) &&
                     (RtlCompareMemory(AdapterExtension->InitiatorSecret,
                                       targetSecret,
                                       secretLen) == secretLen))) {

                    DebugPrint((iScsiPrtDebugError,
                        "Either no initiator secret is set or it is same as target secret\n"));

                    ETWDebugPrint(iScsiLevelError,
                        iScsiFlagProtocolLogInOut,
                        "Either no initiator secret is set or it is same as target secret\n");

                    return ISDSC_INVALID_INITIATOR_CHAP_SECRET;
                }
            }
        }
    } else if (authType != ISCSI_NO_AUTH_TYPE) {

        //
        // Only chap and mutual chap are supported
        //

        DebugPrint((iScsiPrtDebugError,
            "Invalid authentication type %d\n",
            authType));

        ETWDebugPrint(iScsiLevelError,
            iScsiFlagProtocolLogInOut,
            "Invalid authentication type %d\n",
            authType);

        return ISDSC_INVALID_LOGON_AUTH_TYPE;
    }

    return STATUS_SUCCESS;
}


ULONGLONG
iSpGetSessionID(
    PISCSI_ADAPTER_EXTENSION AdapterExtension,
    PISCSI_SESSION IScsiSession,
    USHORT UniqueSessionId
    )
/*+++

Routine Description:

    This routine returns a unique session id. The session id
    is made up of ISID and Microsoft signature byte

Arguements:

    AdapterExtension - Adapter Extension

    IScsiSession - Pointer to the session

    UniqueSessionId - Two byte session id passed by iSCSI Service

Return Value:

    8 Byte SessionId
--*/
{
    ULONGLONG retVal;
    ULONG tempUlong;

    NT_ASSERT((IScsiSession->InitiatorID[0] == 0) &&
           (IScsiSession->InitiatorID[1] == 0));

    //
    // If SessionId is not initialized, generate a new session id.
    // Else, return the saved session id
    //
    if (IScsiSession->SessionId == 0) {

        tempUlong = (ULONG) UniqueSessionId;

        SetUlongIn2ByteArray((IScsiSession->InitiatorID), tempUlong);

        //
        // The unique 64 Bit Session Id is composed this way -
        //
        // Out of 8 bytes :
        //
        //   Most Significant 4 Bytes are composed of the byte
        //   ISID_TYPE_ENTERPRISE_NUMBER and the value MSFT_ENTERPRISE_NUMBER
        //   The Least Significant 4 Bytes are taken from InitiatorSessionID
        //
        retVal = ((ULONGLONG) ISID_TYPE_ENTERPRISE_NUMBER << 24) | MSFT_ENTERPRISE_NUMBER;
        retVal <<= 32;

        retVal |= InterlockedIncrement(&AdapterExtension->InitiatorSessionID);

        IScsiSession->SessionId = retVal;
    } else {

        retVal = IScsiSession->SessionId;
    }

    DebugPrint((iScsiPrtDebugInfo,
        "Session Id returned - 0x%016I64x\n",
        retVal));

    ETWDebugPrint(iScsiLevelInfo,
        iScsiFlagWmi,
        "Session Id returned - 0x%016I64x\n",
        retVal);

    return retVal;
}

UCHAR
iSpReadInitiatorSessionInfo(
    IN PISCSI_ADAPTER_EXTENSION AdapterExtension,
    IN PWMI_CONNECTION_LIST ConnectionList,
    IN ULONG NumberOfSessions,
    OUT PUCHAR Buffer
)
/*++

Routine Description:

   Purpose of this routine is to gather initiatorNode information specified in the mof from
   the port driver and place them in the incoming Buffer.

Arguments:

   AdapterExtension - Adpater extension

   SessionListHead -  pointer to the first session we have got a remove lock for

   SessionListTail   -  pointer ot the last session we have got a remove lock for

   InitiatorName      -  Name of initiator, used to pick out the sessions wanted

   Buffer                  - Used to store all the information of a paticular initiator

Return Value:

   Status - SRB Status for this request

--*/
{
    PMSiSCSI_InitiatorSessionInfo initiatorSessionInfo;
    PISCSI_SessionStaticInfo sessionStaticInfo;
    PISCSI_ConnectionStaticInfo connectionStaticInfo;
    ULONG sessionIndex;
    ULONG connectionIndex;
    ULONG inx, skip;
    PWMIString WMIformatString;
    UCHAR status;


    status = SRB_STATUS_SUCCESS;

    initiatorSessionInfo = (PMSiSCSI_InitiatorSessionInfo) Buffer;

    initiatorSessionInfo->UniqueAdapterId = (ULONGLONG) AdapterExtension;

    //
    // initialize Session count to 0
    //
    initiatorSessionInfo->SessionCount=0;
    sessionStaticInfo = &(initiatorSessionInfo->SessionsList[0]);

    if (ConnectionList == NULL) {

        DebugPrint((iScsiPrtDebugError,
            "Connectionlist NULL in iSpReadInitiatorSessionInfo\n"));

        ETWDebugPrint(iScsiLevelError,
            iScsiFlagWmi,
            "Connectionlist NULL in iSpReadInitiatorSessionInfo\n");

        return status;
    }

    inx = 0;
    while (inx < NumberOfSessions) {

        PISCSI_SESSION iScsiSession;
        PISCSI_CONNECTION iScsiConnection;

        iScsiSession = ConnectionList[inx].ISCSISession;

        if (iScsiSession == NULL) {

            inx++;

            continue;
        }

        sessionStaticInfo->UniqueSessionId = iScsiSession->SessionId;

        WMIformatString = (PWMIString)sessionStaticInfo->InitiatoriSCSIName;
        WMIformatString->Length = (MAX_INITIATOR_NAME) * sizeof(WCHAR);
        RtlCopyMemory(WMIformatString->Buffer,
                      iScsiSession->InitiatorName,
                      (MAX_INITIATOR_NAME * sizeof(WCHAR)));

        WMIformatString = (PWMIString)sessionStaticInfo->TargetiSCSIName;
        WMIformatString->Length = (MAX_TARGET_NAME) * sizeof(WCHAR);
        RtlCopyMemory(WMIformatString->Buffer,
                      iScsiSession->TargetName,
                      (MAX_TARGET_NAME * sizeof(WCHAR)));

        sessionStaticInfo->TSID = iScsiSession->TSIH[1] + (iScsiSession->TSIH[0] << 8);

        sessionStaticInfo->ISID[0] = ISID_TYPE_ENTERPRISE_NUMBER;
        sessionStaticInfo->ISID[1] = (MSFT_ENTERPRISE_NUMBER & 0xFF0000) >> 16;
        sessionStaticInfo->ISID[2] = (MSFT_ENTERPRISE_NUMBER & 0xFF00) >> 8;
        sessionStaticInfo->ISID[3] = MSFT_ENTERPRISE_NUMBER & 0xFF;
        sessionStaticInfo->ISID[4] = iScsiSession->InitiatorID[0];
        sessionStaticInfo->ISID[5] = iScsiSession->InitiatorID[1];

        sessionStaticInfo->InitialR2t  = iScsiSession->IScsiSessionParams.InitialR2T;
        sessionStaticInfo->ImmediateData = iScsiSession->IScsiSessionParams.ImmediateData;

        sessionStaticInfo->Type = iScsiSession->LogonType;

        //False indicates that data PDU Sequences may
        //be transferred in any order.  True indicates that data PDU
        //Sequences must be transferred using continuously increasing offsets,
        //except during error recovery.
        sessionStaticInfo->DataSequenceInOrder = TRUE;

        //False indicates that data PDUs within Sequences may be in any order.
        //True indicates that data PDUs within sequences must be at continuously
        //increasing addresses, with no gaps or overlay between PDUs.
        sessionStaticInfo->DataPduInOrder = TRUE;


        //The level of error recovery negotiated between the initiator and the target.
        //Higher numbers represent more detailed recovery schemes.
        sessionStaticInfo->ErrorRecoveryLevel = (UCHAR) iScsiSession->IScsiSessionParams.ErrorRecoveryLevel;

        sessionStaticInfo->MaxOutstandingR2t = 1;

        //The maximum length supported for unsolicited data
        //sent within this session, in units of 512 bytes.
        sessionStaticInfo->FirstBurstLength = iScsiSession->IScsiSessionParams.FirstBurstLength;

        //The maximum length supported for unsolicited data
        //sent within this session, in units of 512 bytes.
        sessionStaticInfo->MaxBurstLength = iScsiSession->IScsiSessionParams.MaxBurstLength;

        //The maximum number of connections that will be
        //allowed within this session
        sessionStaticInfo->MaxConnections = iScsiSession->IScsiSessionParams.MaxConnections;

        //
        // initialize connection count for this session to 0
        //
        sessionStaticInfo->ConnectionCount = 0;

        connectionStaticInfo = &(sessionStaticInfo->ConnectionsList[0]);

        for (connectionIndex = 0;
             connectionIndex < ConnectionList[inx].Count;
             connectionIndex++)
        {

            iScsiConnection = ConnectionList[inx].ISCSIConnection[connectionIndex];
            if (iScsiConnection != NULL) {

                ULONG socket;

                connectionStaticInfo->UniqueConnectionId = iScsiConnection->UniqueConnectionID;
                connectionStaticInfo->CID = iScsiConnection->ConnectionID[1] +
                                            (iScsiConnection->ConnectionID[0] << 8);

                connectionStaticInfo->HeaderIntegrity =
                    (UCHAR) iScsiConnection->IScsiConnectionParams.HeaderDigest;

                connectionStaticInfo->DataIntegrity =
                    (UCHAR) iScsiConnection->IScsiConnectionParams.DataDigest;

                //
                // The maximum data payload size supported for command
                // or data PDUs within this session, in units of 512 bytes.
                //
                connectionStaticInfo->MaxRecvDataSegmentLength =
                    iScsiConnection->IScsiConnectionParams.MaxRecvDataLength;

                connectionStaticInfo->AuthType = iScsiConnection->AuthType;


                //login          - The TCP connection has been established, but a valid iScsi
                //                    login response with the final bit set has not been sent or received.
                //full            - A valid iScsi login response with the final bit set
                //                   has been sent or received.
                //logout       - A valid iScsi logout command has been sent or received, but
                //                  the TCP connection has not yet been closed.
                connectionStaticInfo->State =  iScsiConnection->CurrentProtocolState;
                //NOTE: CurrentProtocol enum is different then State enum !?!?!?!


                //The transport protocol over which this connection instance is running.
                connectionStaticInfo->Protocol = TCP;  //not saved in deviceExtension yet

                //The Local Inet address used by this connection instance
                iSpCopyIPAddress(&connectionStaticInfo->LocalAddr,
                                 &iScsiConnection->LocalIPAddress,
                                 &socket);

                //
                // Local TCP port is stored in host byte order in LocalIPAddress.
                // Convert that to network byte order here.
                //
                connectionStaticInfo->LocalPort = htons(socket);

                connectionStaticInfo->LocalAddr.TextAddress[0] = MAX_ISCSI_TEXT_ADDRESS_LEN*sizeof(WCHAR);


                //The Remote Inet address used by this connection instance

                iSpCopyIPAddress(&connectionStaticInfo->RemoteAddr,
                                 &iScsiConnection->TargetIPAddress,
                                 &connectionStaticInfo->RemotePort);

                connectionStaticInfo->RemoteAddr.TextAddress[0] = MAX_ISCSI_TEXT_ADDRESS_LEN*sizeof(WCHAR);

                connectionStaticInfo->EstimatedThroughput = iScsiConnection->EstimatedThroughput.QuadPart;

                connectionStaticInfo->MaxDatagramSize = iScsiConnection->MaxDatagramSize;

                //increment the connectionStaticInfo pointer to the next 4 byte alligned address;
                (PUCHAR) connectionStaticInfo += ((sizeof(ISCSI_ConnectionStaticInfo)+7) & ~7);

                //The number of TCP connections that currently belong to this session
                sessionStaticInfo->ConnectionCount ++;
            }
        }

        //
        // Advance to next place to write session information
        //
        skip = FIELD_OFFSET(ISCSI_SessionStaticInfo,
                         ConnectionsList) +
               (ConnectionList[inx].Count *
                ((sizeof(ISCSI_ConnectionStaticInfo)+7) & ~7));

        skip = (skip +7) & ~7;

        (PUCHAR)sessionStaticInfo += skip;

        initiatorSessionInfo->SessionCount++;

        inx++;
    }

    return status;
}

UCHAR
iSpReadConnStatistics(
   IN PISCSI_ADAPTER_EXTENSION AdapterExtension,
   IN PISCSI_CONNECTION IScsiConnection,
   OUT PUCHAR               Buffer
   )
/*++

Routine Description:

   Read the iScsi connection statistics information into the OS buffer.

Arguments:

   IScsiConnection - iScsi miniport driver's connection data storage.

   Buffer - Buffer to hold iScsi connection statistics information.

Return Value:

   Status - SRB Status for this request

--*/
{
    PMSiSCSI_ConnectionStatistics ConnectStatistics;
    PWMIString WMIformatString;
    UCHAR status;


    status = SRB_STATUS_SUCCESS;

    ConnectStatistics = (PMSiSCSI_ConnectionStatistics)Buffer;
    RtlZeroMemory(Buffer, sizeof(MSiSCSI_ConnectionStatistics));

    WMIformatString = (PWMIString)ConnectStatistics->iSCSIName;
    WMIformatString->Length = MAX_INITIATOR_NAME * sizeof(WCHAR);
    RtlCopyMemory(WMIformatString->Buffer,
                  IScsiConnection->ISCSISession->InitiatorName,
                  (MAX_INITIATOR_NAME * sizeof(WCHAR)));

    ConnectStatistics->CID = (IScsiConnection->ConnectionID[0] << 8) +
                                         IScsiConnection->ConnectionID[1];

    //
    // A uniquely generated session ID used only internally.
    // Do not mix this with ISID.
    //
    if (IScsiConnection->ISCSISession != NULL) {

        ConnectStatistics->USID = IScsiConnection->ISCSISession->SessionId;
    }

    ConnectStatistics->UniqueAdapterId = (ULONGLONG) AdapterExtension;

    ConnectStatistics->BytesSent = IScsiConnection->BytesSent;
    ConnectStatistics->BytesReceived = IScsiConnection->BytesReceived;
    ConnectStatistics->PDUCommandsSent = IScsiConnection->PDUCommandsSent;
    ConnectStatistics->PDUResponsesReceived = IScsiConnection->PDUResponsesReceived;

    return status;
}

UCHAR
iSpReadSessionStatistics(
   IN PISCSI_ADAPTER_EXTENSION AdapterExtension,
   IN PISCSI_SESSION IScsiSession,
   IN PWMI_CONNECTION_LIST ConnectionList,
   OUT PUCHAR               Buffer
   )
/*++

Routine Description:

   Read the iScsi session statistics information into the OS buffer.

Arguments:

   IScsiSession - iScsi miniport driver's session data storage.

   Buffer - Buffer to hold iScsi connection statistics information.

Return Value:

   Status - SRB Status for this request

--*/
{
    PMSiSCSI_SessionStatistics SessionStatistics;
    PISCSI_CONNECTION connection;
    PWMIString WMIformatString;
    ULONG inx;
    UCHAR status;


    status = SRB_STATUS_SUCCESS;

    SessionStatistics = (PMSiSCSI_SessionStatistics)Buffer;
    RtlZeroMemory(Buffer, sizeof(MSiSCSI_SessionStatistics));

    if (IScsiSession != NULL) {

        WMIformatString = (PWMIString)SessionStatistics->iSCSIName;
        WMIformatString->Length = MAX_INITIATOR_NAME * sizeof(WCHAR);
        RtlCopyMemory(WMIformatString->Buffer,
                      IScsiSession->InitiatorName,
                      (MAX_INITIATOR_NAME * sizeof(WCHAR)));

        SessionStatistics->USID = IScsiSession->SessionId;

        SessionStatistics->UniqueAdapterId = (ULONGLONG) AdapterExtension;

        SessionStatistics->BytesSent = 0;
        SessionStatistics->BytesReceived = 0;
        SessionStatistics->PDUCommandsSent = 0;
        SessionStatistics->PDUResponsesReceived = 0;

        inx = 0;
        while (inx < ConnectionList->Count)
        {
            PISCSI_CONNECTION iScsiConnection;

            iScsiConnection = ConnectionList->ISCSIConnection[inx];
            if (iScsiConnection != NULL) {
                SessionStatistics->BytesSent += iScsiConnection->BytesSent;
                SessionStatistics->BytesReceived += iScsiConnection->BytesReceived;
                SessionStatistics->PDUCommandsSent += iScsiConnection->PDUCommandsSent;
                SessionStatistics->PDUResponsesReceived += iScsiConnection->PDUResponsesReceived;
            }

            inx++;
        }
    }

    return status;
}

UCHAR
iSpReadInitiatorLoginStatistics(
    IN PISCSI_ADAPTER_EXTENSION AdapterExtension,
    IN PWMI_CONNECTION_LIST ConnectionList,
    IN ULONG NumberOfSessions,
    OUT PUCHAR Buffer
   )
/*++

Routine Description:

   Read the iScsi Initiator Node statistics information into the OS buffer.

Arguments:

   SessionListHead -  pointer to the first session we have got a remove lock for

   SessionListTail   -  pointer ot the last session we have got a remove lock for

   InitiatorName      -  Name of initiator, used to pick out the sessions wanted

   Buffer                  - Used to store all the information of a paticular initiator

Return Value:

   Status - SRB Status for this request

--*/
{
    PMSiSCSI_InitiatorLoginStatistics initiatorLoginStatistics;
    UCHAR status;
    ULONG inx;


    status = SRB_STATUS_SUCCESS;

    initiatorLoginStatistics = (PMSiSCSI_InitiatorLoginStatistics) Buffer;

    initiatorLoginStatistics->UniqueAdapterId = (ULONGLONG) AdapterExtension;

    initiatorLoginStatistics->LogoutNormals = AdapterExtension->LogoutNormals;

    initiatorLoginStatistics->LogoutOtherCodes = AdapterExtension->LogoutOtherRsps;

    inx = 0;
    while (inx < NumberOfSessions) {

        PISCSI_SESSION iScsiSession;
        PISCSI_CONNECTION iScsiConnection;

        if ((ConnectionList != NULL) && (ConnectionList[inx].Count > 0)) {

            iScsiConnection = ConnectionList[inx].ISCSIConnection[0];
            NT_ASSERT(iScsiConnection != NULL);

            iScsiSession = iScsiConnection->ISCSISession;

            if (iScsiSession != NULL) {

                initiatorLoginStatistics->LoginAcceptRsps += iScsiSession->LoginAcceptRsps;

                initiatorLoginStatistics->LoginOtherFailRsps += iScsiSession->LoginOtherFailRsps;

                initiatorLoginStatistics->LoginRedirectRsps += iScsiSession->LoginRedirectRsps;

                initiatorLoginStatistics->LoginAuthFailRsps += iScsiSession->LoginAuthFailRsps;

                initiatorLoginStatistics->LoginAuthenticateFails += iScsiSession->LoginAuthenticateFails;

                initiatorLoginStatistics->LoginNegotiateFails += iScsiSession->LoginNegotiateFails;

                initiatorLoginStatistics->LoginFailures += iScsiSession->LoginFailures;

            }
        }

        inx++;
    }

    return status;
}


VOID
iSpCompleteWmiRequest(
    PISCSI_ADAPTER_EXTENSION AdapterExtension,
    PSCSI_WMI_REQUEST_BLOCK Srb,
    PSCSIWMI_REQUEST_CONTEXT DispatchContext,
    UCHAR Status,
    ULONG SizeNeeded
    )
/*+++

Routine Description:

    This routine calls iScsiPrt to notify that a request
    is completed.

Arguements:

    AdapterExtension - Adapater extension for the initiator

    Srb - The request being completed

    DispatchContext - WMI request context.

    Status - SRB Status for this request

    SizeNeeded - Buffer size needed to process this request

Return Value:

    None
--*/
{
    //
    // Complete the request if the status is NOT pending or NOT
    // already completed within the callback.
    //
    if (Status != SRB_STATUS_PENDING) {

        //
        // Request completed successfully or there was an error.
        //
        ScsiPortWmiPostProcess(DispatchContext,
                               Status,
                               SizeNeeded);

        Srb->SrbStatus = ScsiPortWmiGetReturnStatus(DispatchContext);
        Srb->DataTransferLength = ScsiPortWmiGetReturnSize(DispatchContext);

        //
        // Adapter ready for next request.
        //
        ScsiPortNotification(RequestComplete, AdapterExtension, Srb);
    }
}

UCHAR
iSpGetDynamicConnectionInstanceName(
    IN PISCSI_CONNECTION IScsiConnection,
    IN OUT PWMIString InstanceName
    )
/*++

Routine Description:

   Get the instanceName for the specified connection.  Instance names are formatted as
    follows:  "targetname_#:#"  where the first # is the display session ID, and the second
    # is the CID.

Arguments:

   IScsiConnection - iScsi miniport driver's connection data storage.

   InstanceName - A pointer to WMIString, on return contains the dynamic instance name for the
                           specified connection.

Return Value:

   UCHAR - SRB Status for this request

--*/
{

    WCHAR uniCodeString[MAX_UNICODE_STR_LENGTH];
    PWCHAR currentPos;
    ULONG InstanceNameLength;
    PWMIString WMIformatString;
    KIRQL oldIrql;
    ULONG displaySessionID;
    PISCSI_SESSION sessionListHead;
    PISCSI_SESSION sessionListTail;
    PISCSI_SESSION iScsiSession;
    PISCSI_ADAPTER_EXTENSION adapterExtension;
    NTSTATUS rtlStatus;
    UCHAR status;
    USHORT convertedLength;
    USHORT maxBufferSize;
    UCHAR numString[MAX_UNICODE_STR_LENGTH];


    status = SRB_STATUS_SUCCESS;

    RtlZeroMemory(uniCodeString, sizeof(uniCodeString));

    maxBufferSize = sizeof(uniCodeString) / sizeof(WCHAR);

    rtlStatus = RtlStringCchCopyNW(uniCodeString, maxBufferSize,
                                   IScsiConnection->ISCSISession->TargetName,
                                   wcslen(IScsiConnection->ISCSISession->TargetName));
    if (rtlStatus != STATUS_SUCCESS) {
        return SRB_STATUS_ERROR;
    }

    convertedLength = (USHORT)wcslen(IScsiConnection->ISCSISession->TargetName);

    (PUCHAR)currentPos = (PUCHAR)uniCodeString + convertedLength*sizeof(WCHAR);

    maxBufferSize = (sizeof(uniCodeString)/sizeof(WCHAR)) - convertedLength;

    if (currentPos > uniCodeString + sizeof(uniCodeString)/sizeof(WCHAR)) {
        return SRB_STATUS_ERROR;
    }

    rtlStatus = RtlStringCchCopyW(currentPos, maxBufferSize, L"_");
    if (rtlStatus != STATUS_SUCCESS) {
        return SRB_STATUS_ERROR;
    }
    (PUCHAR)currentPos += sizeof(WCHAR);
    maxBufferSize--;

    if (currentPos > uniCodeString + sizeof(uniCodeString)/sizeof(WCHAR)) {
        return SRB_STATUS_ERROR;
    }

    displaySessionID = IScsiConnection->ISCSISession->DisplaySessionID;

    RtlZeroMemory(numString, sizeof(numString));
     (void) _itoa_s(displaySessionID, numString, _countof(numString), 10);

    convertedLength = AsciiStringToWideCharString(
                               currentPos, 
                               numString, 
                               maxBufferSize); 
    
    //
    // AsciiStringToWideCharString returns the number of characters including the terminating NULL.
    //
    maxBufferSize = maxBufferSize - (convertedLength - 1);
    (PUCHAR)currentPos += ((convertedLength -1 )* sizeof(WCHAR));

    if (currentPos > uniCodeString + sizeof(uniCodeString)/sizeof(WCHAR)) {
        return SRB_STATUS_ERROR;
    }

    rtlStatus = RtlStringCchCopyW(currentPos, maxBufferSize, L":");
    if (rtlStatus != STATUS_SUCCESS) {
        return SRB_STATUS_ERROR;
    }
    (PUCHAR)currentPos += sizeof(WCHAR);
    maxBufferSize--;

    if (currentPos > uniCodeString + sizeof(uniCodeString)/sizeof(WCHAR)) {
        return SRB_STATUS_ERROR;
    }

    RtlZeroMemory(numString, sizeof(numString));
    (void) _itoa_s((IScsiConnection->ConnectionID[0]<<8) + IScsiConnection->ConnectionID[1], numString, _countof(numString), 10);
    convertedLength = AsciiStringToWideCharString(currentPos,
                                                  numString,
                                                  (USHORT)maxBufferSize);

    //
    // AsciiStringToWideCharString returns the number of characters including the terminating NULL.
    //
    (PUCHAR)currentPos += ((convertedLength - 1)* sizeof(WCHAR));

    if (currentPos > uniCodeString + sizeof(uniCodeString)/sizeof(WCHAR)) {
        return SRB_STATUS_ERROR;
    }

    InstanceNameLength = (ULONG)((PUCHAR)currentPos - (PUCHAR)uniCodeString);

    InstanceName->Length = (USHORT)InstanceNameLength;

    RtlStringCchCopyW(InstanceName->Buffer, InstanceName->Length, uniCodeString);

    return status;

}

UCHAR
iSpGetDynamicSessionInstanceName(
    IN PISCSI_SESSION IScsiSession,
    IN OUT PWMIString InstanceName
    )
/*++

Routine Description:

   Get the instanceName for the specified session.  Instance names are formatted as
    follows:  "targetname_#"  where the  # is the display session ID

Arguments:

   IScsiSession - iScsi miniport driver's session data storage.

   InstanceName - A pointer to WMIString, on return contains the dynamic instance name for the
                           specified connection.

Return Value:

   UCHAR - SRB Status for this request

--*/
{

    WCHAR uniCodeString[MAX_UNICODE_STR_LENGTH + 1];
    PWCHAR currentPos;
    ULONG InstanceNameLength;
    PWMIString WMIformatString;
    KIRQL oldIrql;
    ULONG displaySessionID;
    PISCSI_SESSION sessionListHead;
    PISCSI_SESSION sessionListTail;
    PISCSI_SESSION iScsiSession;
    PISCSI_ADAPTER_EXTENSION adapterExtension;
    NTSTATUS rtlStatus;
    UCHAR status = SRB_STATUS_SUCCESS;
    USHORT convertedLength;
    USHORT maxBufferSize;
    UCHAR numString[MAX_UNICODE_STR_LENGTH];


    RtlZeroMemory(uniCodeString, sizeof(uniCodeString));

    convertedLength = (USHORT) wcslen(IScsiSession->TargetName);

    rtlStatus = RtlStringCchCopyNW(uniCodeString, MAX_UNICODE_STR_LENGTH,
                                   IScsiSession->TargetName,
                                   convertedLength);

    if (rtlStatus != STATUS_SUCCESS) {
        return SRB_STATUS_ERROR;
    }
    
    (PUCHAR)currentPos = (PUCHAR)uniCodeString + convertedLength*sizeof(WCHAR);

    maxBufferSize = (sizeof(uniCodeString)/sizeof(WCHAR)) - convertedLength;

    if (currentPos > uniCodeString + sizeof(uniCodeString)/sizeof(WCHAR)) {
        return SRB_STATUS_ERROR;
    }

    rtlStatus = RtlStringCchCopyW(currentPos, maxBufferSize, L"_");
    if (rtlStatus != STATUS_SUCCESS) {
        return SRB_STATUS_ERROR;
    }
    (PUCHAR)currentPos += sizeof(WCHAR);
    maxBufferSize--;

    if (currentPos > uniCodeString + sizeof(uniCodeString)/sizeof(WCHAR)) {
        return SRB_STATUS_ERROR;
    }


    displaySessionID = IScsiSession->DisplaySessionID;

    RtlZeroMemory(numString, sizeof(numString));
    (void) _itoa_s(displaySessionID, numString, _countof(numString), 10);
    convertedLength = AsciiStringToWideCharString(currentPos,
                                                  numString,
                                                  maxBufferSize);

    //
    // AsciiStringToWideCharString returns the number of characters including the terminating NULL.
    //
    (PUCHAR)currentPos += ((convertedLength -1) * sizeof(WCHAR));

    if (currentPos > uniCodeString + sizeof(uniCodeString)/sizeof(WCHAR)) {
        return SRB_STATUS_ERROR;
    }

    InstanceNameLength = (ULONG)((PUCHAR)currentPos - (PUCHAR)uniCodeString);

    if(InstanceNameLength > MAX_UNICODE_STR_LENGTH)
    {
        status = SRB_STATUS_ERROR;
    } else {

        rtlStatus = RtlStringCchCopyNW(
                        InstanceName->Buffer,
                        MAX_UNICODE_STR_LENGTH,
                        uniCodeString,
                        InstanceNameLength);

        if(NT_SUCCESS(rtlStatus) == FALSE)
        {
            status = SRB_STATUS_ERROR;
        } else {
            status = SRB_STATUS_SUCCESS;
        }
    }

    if(status != SRB_STATUS_SUCCESS)
    {
        InstanceName->Length = 0;
    } else {
        InstanceName->Length = (USHORT)InstanceNameLength;
    }

    return status;
}


VOID
iScsiFireAdapterEvent(
    PISCSI_ADAPTER_EXTENSION AdapterExtension,
    ISCSI_ADAPTER_EVENT_CODE EventCode
    )
{
    ScsiPortWmiFireAdapterEvent(AdapterExtension,
                    &iScsiAdapterEventGuid,
                    0,
                    sizeof(ULONG),
                    &EventCode);
}


UCHAR
iSpQueryLBPolicy(
    IN     PISCSI_ADAPTER_EXTENSION AdapterExtension,
    IN OUT PULONG InstanceLengthArray,
    IN     ULONG  BufferAvail,
    OUT    PUCHAR Buffer,
    OUT    PULONG SizeNeeded
    )
{
    PWMI_CONNECTION_LIST connectionList;
    ULONG size, sessionInx;
    ULONG numberOfSessions, numberOfConnections;
    UCHAR status = SRB_STATUS_SUCCESS;;


    *SizeNeeded = 0;

    connectionList = iSpGetConnectionList(AdapterExtension,
                                          &numberOfSessions,
                                          &numberOfConnections);

    DebugPrint((iScsiPrtDebugInfo,
        "Number of Sessions %d, Number of Connections %d\n",
        numberOfSessions,
        numberOfConnections));

    ETWDebugPrint(iScsiLevelInfo,
        iScsiFlagWmi,
        "Number of Sessions %d, Number of Connections %d\n",
        numberOfSessions,
        numberOfConnections);

    //
    // Determine the size of buffer needed to return LB policy settings
    // for all the sessions on this adapter.
    //

    //
    // First account for the fixed part of MSiSCSI_Supported_LB_Policies
    //
    size = FIELD_OFFSET(MSiSCSI_QueryLBPolicy,
                        LoadBalancePolicies);

    if (connectionList == NULL) {

        if (BufferAvail >= size) {
            PMSiSCSI_QueryLBPolicy queryLBPolicy;

            queryLBPolicy = (PMSiSCSI_QueryLBPolicy) Buffer;
            queryLBPolicy->UniqueAdapterId = (ULONGLONG) AdapterExtension;
            queryLBPolicy->SessionCount = 0;
            *SizeNeeded = size;

            return SRB_STATUS_SUCCESS;
        } else {

            *SizeNeeded = size;

            return SRB_STATUS_SUCCESS;
        }
    }

    //
    // Loop over all sessions and account for the space needed for each
    // session plus all of the connections within each session
    //
    sessionInx = 0;
    while (sessionInx < numberOfSessions) {

        //
        // Since the session structure needs to be 8 bytes aligned, we
        // pad out to 8 bytes
        //
        size = (size + 7) & ~7;

        //
        // Add the fixed size needed for the session structure
        //
        size += FIELD_OFFSET(ISCSI_Supported_LB_Policies,
                             iSCSI_Paths);

        //
        // Account for the size of all connection structures, being
        // sure that the connection structure is padded out to 8 bytes
        // in order to maintain 8 byte alignment
        //
        size += connectionList[sessionInx].Count *
                ((sizeof(ISCSI_Path) + 7) & ~7);

        sessionInx++;
    }

    *SizeNeeded = size;

    if (size <= BufferAvail) {

        //
        // There is enough space to build the LB policy info data
        // for all the sessions on this adapter.
        //
        RtlZeroMemory(Buffer, size);

        status = iSpReadLBPolicyInfo(AdapterExtension,
                                     connectionList,
                                     numberOfSessions,
                                     (PMSiSCSI_QueryLBPolicy) Buffer);

    } else {

        //
        // There is not enough space to build the LB policy info data
        // for all the sessions on this adapter.
        //
        status = SRB_STATUS_DATA_OVERRUN;
    }

    *InstanceLengthArray = *SizeNeeded;

    iSpReleaseConnectionReferences(connectionList,
                                   numberOfSessions);


    return status;
}

UCHAR
iSpReadLBPolicyInfo(
    IN  PISCSI_ADAPTER_EXTENSION AdapterExtension,
    IN  PWMI_CONNECTION_LIST ConnectionList,
    IN  ULONG NumberOfSessions,
    OUT PMSiSCSI_QueryLBPolicy QueryLBPolicy
    )
{
    PISCSI_Supported_LB_Policies supportedLBPolicies;
    PISCSI_Path iScsiPath;
    PISCSI_SESSION iScsiSession;
    PISCSI_CONNECTION iScsiConnection;
    ULONG sessionInx;
    ULONG connectionInx;
    ULONG skip;
    UCHAR status = SRB_STATUS_SUCCESS;

    QueryLBPolicy->UniqueAdapterId = (ULONGLONG) AdapterExtension;

    QueryLBPolicy->SessionCount = 0;

    supportedLBPolicies = QueryLBPolicy->LoadBalancePolicies;

    sessionInx = 0;
    while (sessionInx < NumberOfSessions) {

        iScsiSession = ConnectionList[sessionInx].ISCSISession;

        supportedLBPolicies->UniqueSessionId = iScsiSession->SessionId;

        supportedLBPolicies->LoadBalancePolicy = iScsiSession->LoadBalancePolicy;

        supportedLBPolicies->iSCSI_PathCount = 0;
        connectionInx = 0;
        while (connectionInx < ConnectionList[sessionInx].Count) {

            iScsiConnection = ConnectionList[sessionInx].ISCSIConnection[connectionInx];
            if (iScsiConnection != NULL) {

                iScsiPath = &(supportedLBPolicies->iSCSI_Paths[connectionInx]);

                iScsiPath->UniqueConnectionId = (ULONGLONG) iScsiConnection->UniqueConnectionID;

                iScsiPath->EstimatedLinkSpeed = iScsiConnection->EstimatedThroughput.QuadPart;

                iScsiPath->PathWeight = iScsiConnection->PathWeight;

                iScsiPath->PrimaryPath = iScsiConnection->CurrentPathStatus;

                iScsiPath->TCPOffLoadAvailable = 0;

                switch (iScsiConnection->ConnectionState) {
                    case ConnectionStateConnected : {
                        iScsiPath->ConnectionStatus = CONNECTION_STATE_CONNECTED;
                        break;
                    }

                    case ConnectionStateDisconnected :
                    case ConnectionStateStopping: {
                        iScsiPath->ConnectionStatus = CONNECTION_STATE_DISCONNECTED;
                        break;
                    }

                    case ConnectionStateConnecting : {
                        iScsiPath->ConnectionStatus = CONNECTION_STATE_RECONNECTING;
                        break;
                    }

                    default : {
                        iScsiPath->ConnectionStatus = CONNECTION_STATE_DISCONNECTED;
                        break;
                    }
                }

                supportedLBPolicies->iSCSI_PathCount++;
            }

            connectionInx++;
        }

        //
        // Advance to next place to write session information
        //
        skip = FIELD_OFFSET(ISCSI_Supported_LB_Policies, iSCSI_Paths) +
               (ConnectionList[sessionInx].Count * ((sizeof(ISCSI_Path) + 7) & ~7));

        skip = (skip + 7) & ~7;

        (PUCHAR)supportedLBPolicies += skip;

        QueryLBPolicy->SessionCount++;

        sessionInx++;
    }

    return status;
}

UCHAR
iSpSetLoadBalancePolicy(
    IN  PISCSI_ADAPTER_EXTENSION AdapterExtension,
    IN  PSCSI_WMI_REQUEST_BLOCK Srb,
    IN  PUCHAR Buffer,
    IN  ULONG  InBufferSize
    )
{
    PSetLoadBalancePolicy_IN  setLBPolicyIN;
    PSetLoadBalancePolicy_OUT setLBPolicyOUT;
    PISCSI_Supported_LB_Policies supportedLBPolicy;
    PISCSI_SESSION iScsiSession;
    PPERSISTENT_TARGET_LB_POLICY LBPolicy = NULL;
    PLB_POLICY_DATA LBPolicyData;
    ULONG iScsiStatus;
    ULONG inx;
    BOOLEAN persistLBPolicy = FALSE;
    KIRQL oldIrql;
    NTSTATUS ntStatus;

    setLBPolicyIN = (PSetLoadBalancePolicy_IN) Buffer;

    setLBPolicyOUT = (PSetLoadBalancePolicy_OUT) Buffer;

    iScsiStatus = iSpValidateLoadBalancePolicy(setLBPolicyIN,
                                               InBufferSize);
    if (iScsiStatus != STATUS_SUCCESS) {

        setLBPolicyOUT->Status = iScsiStatus;

        return SRB_STATUS_SUCCESS;
    }

    supportedLBPolicy = &(setLBPolicyIN->LoadBalancePolicies);

    iScsiSession = iSpGetIScsiSession(AdapterExtension,
                                      supportedLBPolicy->UniqueSessionId,
                                      Srb);
    if (iScsiSession == NULL) {

        DebugPrint((iScsiPrtDebugWarning,
            "Invalid session id given %I64x for SetLBPolicy\n",
            supportedLBPolicy->UniqueSessionId));

        ETWDebugPrint(iScsiLevelWarning,
            iScsiFlagWmi,
            "Invalid session id given %I64x for SetLBPolicy\n",
            supportedLBPolicy->UniqueSessionId);

        setLBPolicyOUT->Status = ISDSC_INVALID_SESSION_ID;

        return SRB_STATUS_SUCCESS;
    }

    KeAcquireSpinLock(&iScsiSession->SessionLock, &oldIrql);

    iScsiStatus = iSpValidateConnectionIds(iScsiSession, supportedLBPolicy);

    if (iScsiStatus == STATUS_SUCCESS) {

        switch (supportedLBPolicy->LoadBalancePolicy) {
            case MSiSCSI_LB_FAILOVER: {

                iScsiStatus = iSpSetLBFailOverOnly(iScsiSession,
                                                   supportedLBPolicy);

                break;
            }

            case MSiSCSI_LB_DYN_LEAST_QUEUE_DEPTH:
            case MSiSCSI_LB_WEIGHTED_PATHS: {

                iScsiStatus = iSpSetLB_LQD_WP(iScsiSession, supportedLBPolicy);

                break;
            }

            case MSiSCSI_LB_ROUND_ROBIN:
            case MSiSCSI_LB_ROUND_ROBIN_WITH_SUBSET: {

                iScsiStatus = iSpSetLBRoundRobinSubset(iScsiSession,
                                                       supportedLBPolicy);

                break;
            }

            default: {

                //
                // Should never get here since the input policy has already
                // been validated in iSpValidateLoadBalancePolicy.
                //
                iScsiStatus = ISDSC_INVALID_LOAD_BALANCE_POLICY;
            }
        }
    }

    //
    // Check if we need to persist the new LB policy
    //
    if ((iScsiStatus == STATUS_SUCCESS) &&
        (wcslen(iScsiSession->TargetKeyNameSuffix) > 0)) {

        ULONG sizeNeeded = 0;
        ULONG tempUlong;
        BOOLEAN overFlow = TRUE;

        if (ULongMult(sizeof(LB_POLICY_DATA),
                      (iScsiSession->NumberOfConnections - 1),
                      &tempUlong) == S_OK) {

            if (ULongAdd(sizeof(PERSISTENT_TARGET_LB_POLICY),
                         tempUlong, &sizeNeeded) == S_OK) {

                overFlow = FALSE;
            }
        }

        //
        // This is a persistent target. Need to save the new LB policy
        // in the registry for this persistent target.
        //
        if (!overFlow) {

            LBPolicy = iSpAllocatePool(NonPagedPoolNx,
                                       sizeNeeded,
                                       ISCSI_TAG_LB_POLICY);
            if (LBPolicy != NULL) {

                PISCSI_Path iScsiPath;
                PISCSI_CONNECTION iScsiConnection;

                persistLBPolicy = TRUE;

                RtlStringCchCopyW(LBPolicy->TargetKeyName,
                                  MAX_TARGET_NAME + TARGET_KEYNAME_SUFFIX_SIZE + 1,
                                  iScsiSession->TargetName);

                RtlStringCchCatW(LBPolicy->TargetKeyName,
                                 MAX_TARGET_NAME + TARGET_KEYNAME_SUFFIX_SIZE + 1,
                                 iScsiSession->TargetKeyNameSuffix);

                LBPolicy->LoadBalancePolicy = supportedLBPolicy->LoadBalancePolicy;

                LBPolicy->ConnectionCount = iScsiSession->NumberOfConnections;

                for (inx = 0; inx < supportedLBPolicy->iSCSI_PathCount; inx++) {

                    iScsiPath = &(supportedLBPolicy->iSCSI_Paths[inx]);

                    iScsiConnection = (PISCSI_CONNECTION) iScsiPath->UniqueConnectionId;

                    LBPolicyData = &(LBPolicy->PolicyData[inx]);

                    LBPolicyData->ConnectionNumber = iScsiConnection->ConnectionNumber;

                    LBPolicyData->PathWeight = iScsiConnection->PathWeight;

                    LBPolicyData->PrimaryPath = iScsiConnection->CurrentPathStatus;
                }
            } else {

                DebugPrint((iScsiPrtDebugError,
                    "Failed to allocate memory for persisting LB policy\n"));

                ETWDebugPrint(iScsiLevelError,
                    iScsiFlagWmi,
                    "Failed to allocate memory for persisting LB policy\n");
            }
        }
    }

    KeReleaseSpinLock(&iScsiSession->SessionLock, oldIrql);

    if (persistLBPolicy) {

        NT_ASSERT(LBPolicy != NULL);

        ntStatus = iSpPersisteLBPolicy(AdapterExtension, LBPolicy);

        if(NT_SUCCESS(ntStatus) == FALSE)
        {
            iScsiStatus = ISDSC_FAILURE_TO_PERSIST_LB_POLICY;
        }

        ExFreePool(LBPolicy);
    }

    //
    // Release the reference taken in iSpGetIScsiSession
    //
    IoReleaseRemoveLock(iScsiSession->RemoveLock, REMOVELOCK_TAG(Srb));

    setLBPolicyOUT->Status = iScsiStatus;

    return SRB_STATUS_SUCCESS;
}

ULONG
iSpValidateLoadBalancePolicy(
    IN  PSetLoadBalancePolicy_IN SetLBPolicyIN,
    IN  ULONG                    InBufferSize
    )
{
    PISCSI_Supported_LB_Policies supportedLBPolicy;
    PISCSI_Path iScsiPath0;
    PISCSI_Path iScsiPath1;
    ULONG iScsiStatus = STATUS_SUCCESS;
    ULONG sizeNeeded;
    ULONG inx, jnx;
    KIRQL oldIrql;

    sizeNeeded = FIELD_OFFSET(ISCSI_Supported_LB_Policies, iSCSI_Paths);
    if (InBufferSize < sizeNeeded) {

        DebugPrint((iScsiPrtDebugError,
            "Input buffer too small for SetLBPolicy. Expected %d, Given %d\n",
            sizeNeeded,
            InBufferSize));

        ETWDebugPrint(iScsiLevelError,
            iScsiFlagWmi,
            "Input buffer too small for SetLBPolicy. Expected %d, Given %d\n",
            sizeNeeded,
            InBufferSize);

        return ISDSC_BUFFER_TOO_SMALL;
    }

    supportedLBPolicy = &(SetLBPolicyIN->LoadBalancePolicies);

    sizeNeeded += supportedLBPolicy->iSCSI_PathCount *
                  ((sizeof(ISCSI_Path) + 7) & ~7);

    if (InBufferSize < sizeNeeded) {

        DebugPrint((iScsiPrtDebugError,
            "Input buffer too small for SetLBPolicy. Expected %d, Given %d\n",
            sizeNeeded,
            InBufferSize));

        ETWDebugPrint(iScsiLevelError,
            iScsiFlagWmi,
            "Input buffer too small for SetLBPolicy. Expected %d, Given %d\n",
            sizeNeeded,
            InBufferSize);

        return ISDSC_BUFFER_TOO_SMALL;
    }

    if ((supportedLBPolicy->LoadBalancePolicy < MSiSCSI_LB_FAILOVER) ||
        (supportedLBPolicy->LoadBalancePolicy > MSiSCSI_LB_WEIGHTED_PATHS)){

        DebugPrint((iScsiPrtDebugError,
            "Invalid Load Balance Policy %d\n",
            supportedLBPolicy->LoadBalancePolicy));

        ETWDebugPrint(iScsiLevelError,
            iScsiFlagWmi,
            "Invalid Load Balance Policy %d\n",
            supportedLBPolicy->LoadBalancePolicy);

        return ISDSC_INVALID_LOAD_BALANCE_POLICY;
    }

    //
    // Make sure user did not provide duplicate path ids
    //
    for (inx = 0; inx < supportedLBPolicy->iSCSI_PathCount; inx++) {

        iScsiPath0 = &(supportedLBPolicy->iSCSI_Paths[inx]);

        for (jnx = 0; jnx < supportedLBPolicy->iSCSI_PathCount; jnx++) {

            iScsiPath1 = &(supportedLBPolicy->iSCSI_Paths[jnx]);

            if ((inx != jnx) &&
                (iScsiPath0->UniqueConnectionId == iScsiPath1->UniqueConnectionId)) {

                DebugPrint((iScsiPrtDebugError,
                    "Duplicate path ids at %d and %d\n",
                    inx,
                    jnx));

                ETWDebugPrint(iScsiLevelError,
                    iScsiFlagWmi,
                    "Duplicate path ids at %d and %d\n",
                    inx,
                    jnx);

                iScsiStatus = ISDSC_DUPLICATE_PATH_SPECIFIED;

                break;
            }

            jnx++;
        }

        if (iScsiStatus != STATUS_SUCCESS) {
            break;
        }

        inx++;
    }

    return iScsiStatus;
}

ULONG
iSpValidateConnectionIds(
    IN PISCSI_SESSION IScsiSession,
    IN PISCSI_Supported_LB_Policies SupportedLBPolicy
    )
/*+++

  Note: This routine should be called with IScsiSession->SessionLock held

--*/
{
    PISCSI_Path iScsiPath;
    PISCSI_CONNECTION iScsiConnection;
    PLIST_ENTRY connectionEntry;
    ULONG iScsiStatus = STATUS_SUCCESS;
    ULONG inx;
    BOOLEAN found;

    if (IScsiSession->NumberOfConnections != SupportedLBPolicy->iSCSI_PathCount) {

        DebugPrint((iScsiPrtDebugError,
            "Number of connections %d does not match path count %d\n",
            IScsiSession->NumberOfConnections,
            SupportedLBPolicy->iSCSI_PathCount));

        ETWDebugPrint(iScsiLevelError,
            iScsiFlagWmi,
            "Number of connections %d does not match path count %d\n",
            IScsiSession->NumberOfConnections,
            SupportedLBPolicy->iSCSI_PathCount);

        return ISDSC_PATH_COUNT_MISMATCH;
    }

    for (inx = 0; inx < SupportedLBPolicy->iSCSI_PathCount; inx++) {

        iScsiPath = &(SupportedLBPolicy->iSCSI_Paths[inx]);

        found = FALSE;

        connectionEntry = IScsiSession->ConnectionList.Flink;
        while (connectionEntry != &IScsiSession->ConnectionList) {

            iScsiConnection = CONTAINING_RECORD(connectionEntry,
                                                ISCSI_CONNECTION,
                                                NextConnection);

            connectionEntry = connectionEntry->Flink;

            //
            // Path Id that is returned in QueryLBPolicy is UniqueConnectionID
            //
            if (iScsiConnection->UniqueConnectionID == iScsiPath->UniqueConnectionId) {

                //
                // Set the pointer to iscsi_connection in UniqueConnectionId. This will
                // be used in SetLBPolicy routine to set the load balance policy.
                //
                iScsiPath->UniqueConnectionId = (ULONGLONG) iScsiConnection;

                found = TRUE;

                break;
            }
        }

        if (!found) {

            DebugPrint((iScsiPrtDebugError,
                "Path Id %I64x not found\n",
                iScsiPath->UniqueConnectionId));

            ETWDebugPrint(iScsiLevelError,
                iScsiFlagWmi,
                "Path Id %I64x not found\n",
                iScsiPath->UniqueConnectionId);

            iScsiStatus = ISDSC_INVALID_PATH_ID;

            break;
        }
    }

    return iScsiStatus;
}

ULONG
iSpSetLBFailOverOnly(
    IN PISCSI_SESSION IScsiSession,
    IN PISCSI_Supported_LB_Policies SupportedLBPolicy
    )
/*+++

  Note: This routine should be called with IScsiSession->SessionLock held

--*/
{
    PISCSI_Path iScsiPath;
    PISCSI_CONNECTION iScsiConnection;
    ULONG inx;
    ULONG iScsiStatus = STATUS_SUCCESS;
    BOOLEAN activeFound = FALSE;

    //
    // Save the current value of path status.
    //
    iSpSavePrimaryPath(SupportedLBPolicy);

    for (inx = 0; inx < SupportedLBPolicy->iSCSI_PathCount; inx++) {

        iScsiPath = &(SupportedLBPolicy->iSCSI_Paths[inx]);

        iScsiConnection = (PISCSI_CONNECTION) iScsiPath->UniqueConnectionId;

        if (iScsiPath->PrimaryPath) {

            //
            // Primary path specified. Make sure one hasn't been
            // specified earlier since only one primary path is
            // allowed for FailOverOnly policy.
            //
            if (!activeFound) {

                DebugPrint((iScsiPrtDebugInfo,
                    "%p specified as Primary Path\n",
                    iScsiConnection));

                ETWDebugPrint(iScsiLevelInfo,
                    iScsiFlagWmi,
                    "%p specified as Primary Path\n",
                    iScsiConnection);

                iScsiConnection->CurrentPathStatus = TRUE;

                activeFound = TRUE;
            } else {

                DebugPrint((iScsiPrtDebugError,
                    "Multiple primary paths given for LBFailOverOnly\n"));

                ETWDebugPrint(iScsiLevelError,
                    iScsiFlagWmi,
                    "Multiple primary paths given for LBFailOverOnly\n");

                iScsiStatus = ISDSC_MULTIPLE_PRIMARY_PATHS_SPECIFIED;
            }
        } else {

            //
            // This is a standby path
            //
            iScsiConnection->CurrentPathStatus = FALSE;
        }
    }

    if (!activeFound) {

        DebugPrint((iScsiPrtDebugError,
            "No active path given for FO Only\n"));

        ETWDebugPrint(iScsiLevelError,
            iScsiFlagWmi,
            "No active path given for FO Only\n");

        iScsiStatus = ISDSC_NO_PRIMARY_PATH_SPECIFIED;
    }

    if (NT_SUCCESS(iScsiStatus)) {

        DebugPrint((iScsiPrtDebugInfo,
            "FailOverOnly set successfully for %p\n",
            IScsiSession));

        ETWDebugPrint(iScsiLevelInfo,
            iScsiFlagWmi,
            "FailOverOnly set successfully for %p\n",
            IScsiSession);

        IScsiSession->LoadBalancePolicy = LB_FAIL_OVER_ONLY;
    } else {

        //
        // Failed to set user given LB settings. Restore the value of
        // path status from the saved value.
        //
        iSpRestorePrimaryPath(SupportedLBPolicy);
    }

    return iScsiStatus;
}

ULONG
iSpSetLB_LQD_WP(
    IN PISCSI_SESSION IScsiSession,
    IN PISCSI_Supported_LB_Policies SupportedLBPolicy
    )
/*+++

  Note: This routine should be called with IScsiSession->SessionLock held

--*/
{
    PISCSI_Path iScsiPath;
    PISCSI_CONNECTION iScsiConnection;
    ULONG inx;

    for (inx = 0; inx < SupportedLBPolicy->iSCSI_PathCount; inx++) {

        iScsiPath = &(SupportedLBPolicy->iSCSI_Paths[inx]);

        iScsiConnection = (PISCSI_CONNECTION) iScsiPath->UniqueConnectionId;

        iScsiConnection->CurrentPathStatus = TRUE;

        if (SupportedLBPolicy->LoadBalancePolicy == LB_LEAST_WEIGHT_PATH) {
            iScsiConnection->PathWeight = iScsiPath->PathWeight;
        }
    }

    IScsiSession->LoadBalancePolicy = SupportedLBPolicy->LoadBalancePolicy;

    return STATUS_SUCCESS;
}

ULONG
iSpSetLBRoundRobinSubset(
    IN PISCSI_SESSION IScsiSession,
    IN PISCSI_Supported_LB_Policies SupportedLBPolicy
    )
/*+++

  Note: This routine should be called with IScsiSession->SessionLock held

--*/
{

    PISCSI_Path iScsiPath;
    PISCSI_CONNECTION iScsiConnection;
    ULONG iScsiStatus = STATUS_SUCCESS;
    ULONG inx;
    BOOLEAN activeFound = FALSE;

    //
    // Save the current value of path status.
    //
    iSpSavePrimaryPath(SupportedLBPolicy);

    for (inx = 0; inx < SupportedLBPolicy->iSCSI_PathCount; inx++) {

        iScsiPath = &(SupportedLBPolicy->iSCSI_Paths[inx]);

        iScsiConnection = (PISCSI_CONNECTION) iScsiPath->UniqueConnectionId;

        if (iScsiPath->PrimaryPath) {

            iScsiConnection->CurrentPathStatus = TRUE;

            activeFound = TRUE;
        } else {

            iScsiConnection->CurrentPathStatus = FALSE;
        }
    }

    if (!activeFound) {

        DebugPrint((iScsiPrtDebugError,
            "No active path given for Round Robin.\n"));

        ETWDebugPrint(iScsiLevelError,
            iScsiFlagWmi,
            "No active path given for Round Robin.\n");

        iScsiStatus = ISDSC_NO_PRIMARY_PATH_SPECIFIED;
    }

    if (NT_SUCCESS(iScsiStatus)) {

        DebugPrint((iScsiPrtDebugInfo,
            "Successfully set LB policy\n"));

        ETWDebugPrint(iScsiLevelTrace,
            iScsiFlagWmi,
            "Successfully set LB policy\n");

        IScsiSession->LoadBalancePolicy = SupportedLBPolicy->LoadBalancePolicy;
    } else {

        //
        // Failed to set user given LB settings. Restore the value of
        // path status from the saved value.
        //
        iSpRestorePrimaryPath(SupportedLBPolicy);
    }

    return iScsiStatus;
}

FORCEINLINE
iSpSavePrimaryPath(
    IN PISCSI_Supported_LB_Policies SupportedLBPolicy
    )
/*+++

  Note: This routine should be called with IScsiSession->SessionLock held

--*/
{
    PISCSI_Path iScsiPath;
    PISCSI_CONNECTION iScsiConnection;
    ULONG inx;

    //
    // Save the current LB policy settings before setting new values.
    // In case the update fails, we'll restore the old LB settings.
    //
    for (inx = 0; inx < SupportedLBPolicy->iSCSI_PathCount; inx++) {

        iScsiPath = &(SupportedLBPolicy->iSCSI_Paths[inx]);

        iScsiConnection = (PISCSI_CONNECTION) iScsiPath->UniqueConnectionId;

        iScsiConnection->PrevPathStatus = iScsiConnection->CurrentPathStatus;
    }
}

FORCEINLINE
iSpRestorePrimaryPath(
    IN PISCSI_Supported_LB_Policies SupportedLBPolicy
    )
/*+++

  Note: This routine should be called with IScsiSession->SessionLock held

--*/
{
    PISCSI_Path iScsiPath;
    PISCSI_CONNECTION iScsiConnection;
    ULONG inx;

    //
    // Restore the LB settings from the saved value.
    //
    for (inx = 0; inx < SupportedLBPolicy->iSCSI_PathCount; inx++) {

        iScsiPath = &(SupportedLBPolicy->iSCSI_Paths[inx]);

        iScsiConnection = (PISCSI_CONNECTION) iScsiPath->UniqueConnectionId;

        iScsiConnection->CurrentPathStatus = iScsiConnection->PrevPathStatus;
    }
}


VOID
iSpLogError(
    IN PDEVICE_OBJECT              DeviceObject,
    _In_reads_bytes_opt_(ErrorDataSize) PUCHAR ErrorData,
    IN ULONG                       ErrorDataSize,
    _In_opt_ PWSTR                 TargetName,
    IN ULONG                       ErrorCode
    )
/*+++

Routine Description :

    This routine will build a WMI event for the MSiSCSI_Eventlog class
    and then fire it. The WMI eventlog consumer provider has classes
    that intercept the WMI event and will redirect it to the system
    eventlog.

Arguements:

    DeviceObject is the device object of the adapter

    ErrorData is the data to be placed in the additional data field of
        the eventlog

    ErrorDataSize is the number of bytes of additional data

    TargetName is the name of the target

    ErrorCode is the iSCSI eventlog error code (see iscsilog.h)

Return Value:

--*/
{
    PMSiSCSI_Eventlog eventLog;
    PUCHAR dumpData;
    ULONG totalSize;
    ULONG dumpDataSize;
    ULONG targetNameSize;
    NTSTATUS status;
    GUID eventLogGUID = MSiSCSI_EventlogGuid;

    if (TargetName != NULL) {
        targetNameSize = wcslen(TargetName) * sizeof(WCHAR);
    } else {
        targetNameSize = 0;
    }

    totalSize = sizeof(MSiSCSI_Eventlog) +
                ErrorDataSize +
                targetNameSize;

    //
    // Allocate memory for MSiSCSI_Eventlog object
    //


    if (eventLog != NULL) {
        dumpDataSize = totalSize - FIELD_OFFSET(MSiSCSI_Eventlog,
                                                AdditionalData);
        eventLog->Type = ErrorCode;
        eventLog->LogToEventlog = 1;

        dumpData = eventLog->AdditionalData;

        //
        // Copy the entire error data followed by the target name if given.
        //
        if ((ErrorData != NULL) && (ErrorDataSize > 0)) {
            RtlCopyMemory(dumpData, ErrorData, ErrorDataSize);
            dumpData += ErrorDataSize;
        }

        if (targetNameSize > 0) {
            RtlCopyMemory(dumpData, TargetName, targetNameSize);
        }

        eventLog->Size = dumpDataSize;

        //
        // Fire the WMI event using eventLogGUID GUID, and
        // Free the memory allocated for MSiSCSI_Eventlog object.
        //

    }
}



UCHAR
iSpBuildBootInfo(
    IN PISCSI_ADAPTER_EXTENSION AdapterExtension,
    IN PULONG InstanceLength,
    IN ULONG BufferAvail,
    OUT PUCHAR Buffer,
    OUT PULONG SizeNeeded
    )
{
    UCHAR status;
    ULONG size;

    size = sizeof(MSiSCSI_BootInformation);

    if (BufferAvail >= size) {
        PMSiSCSI_BootInformation bootInfo;

        bootInfo = (PMSiSCSI_BootInformation)Buffer;

        bootInfo->SharedSecretLength = AdapterExtension->BootSecretLength;
        RtlCopyMemory(bootInfo->SharedSecret, AdapterExtension->BootSecret, 255);
        RtlCopyMemory(bootInfo->NodeName,
                      AdapterExtension->BootNodeName,
                      MAX_INITIATOR_NAME);
        *SizeNeeded = size;

        status = SRB_STATUS_SUCCESS;

    } else {

        *SizeNeeded = size;

        status = SRB_STATUS_DATA_OVERRUN;
    }

    *InstanceLength = *SizeNeeded;

    return status;
}

UCHAR
iSpBuildRedirectTargetInfo(
    IN PISCSI_ADAPTER_EXTENSION AdapterExtension,
    IN PULONG InstanceLength,
    IN ULONG BufferAvail,
    OUT PUCHAR Buffer,
    OUT PULONG SizeNeeded
    )
{
    PWMI_CONNECTION_LIST connectionList;
    ULONG size, sessionInx;
    ULONG numberOfSessions, numberOfConnections;
    UCHAR status;


    connectionList = iSpGetConnectionList(AdapterExtension,
                                          &numberOfSessions,
                                          &numberOfConnections);

    DebugPrint((iScsiPrtDebugInfo,
        "Number of Sessions %d, Number of Connections %d\n",
        numberOfSessions,
        numberOfConnections));

    ETWDebugPrint(iScsiLevelInfo,
        iScsiFlagWmi,
        "Number of Sessions %d, Number of Connections %d\n",
        numberOfSessions,
        numberOfConnections);

    //
    // determine the size of buffer needed to build all of the session
    // and connection data structures
    //

    //
    // Account for the fixed part of the iScsiInitiatorSessionInfo
    //
    size = FIELD_OFFSET(MSiSCSI_RedirectPortalInfoClass, RedirectSessionList);

    if (connectionList == NULL) {

        if (BufferAvail >= size)
        {
            PMSiSCSI_RedirectPortalInfoClass redirectPortalInfo;

            redirectPortalInfo = (PMSiSCSI_RedirectPortalInfoClass)Buffer;

            redirectPortalInfo->UniqueAdapterId = (ULONGLONG) AdapterExtension;
            redirectPortalInfo->SessionCount = 0;
            *SizeNeeded = size;

            status = SRB_STATUS_SUCCESS;
        } else {

            *SizeNeeded = size;

            status = SRB_STATUS_DATA_OVERRUN;
        }

        *InstanceLength = *SizeNeeded;

        return status;
    }

    //
    // Loop over all sessions and account for the space needed for each
    // session plus all of the connections within each session
    //
    sessionInx = 0;
    while (sessionInx < numberOfSessions) {

        //
        // Since the session structure needs to be 8 bytes aligned, we
        // pad out to 8 bytes
        //
        size = (size + 7) & ~7;

        //
        // Add the fixed size needed for the session structure
        //
        size += FIELD_OFFSET(ISCSI_RedirectSessionInfo,
                             RedirectPortalList);

        //
        // Account for the size of all connection structures, being
        // sure that the connection structure is padded out to 8 bytes
        // in order to maintain 8 byte alignment
        //
        size += connectionList[sessionInx].Count *
                ((sizeof(ISCSI_RedirectPortalInfo) + 7) & ~7);

        sessionInx++;
    }

    *SizeNeeded = size;
    if (size <= BufferAvail)
    {
        //
        // If we do have enough space to build the session info data
        // structures then we do so
        //
        RtlZeroMemory(Buffer, size);

        status = iSpReadRedirectTargetPortalInfo(AdapterExtension,
                                                 connectionList,
                                                 numberOfSessions,
                                                 Buffer);

    } else {
        //
        // If there is not enough space to build the session info data
        // structures then return this error
        //
        status = SRB_STATUS_DATA_OVERRUN;
    }

    iSpReleaseConnectionReferences(connectionList,
                                   numberOfSessions);


    return status;
}

UCHAR
iSpReadRedirectTargetPortalInfo(
    IN PISCSI_ADAPTER_EXTENSION AdapterExtension,
    IN PWMI_CONNECTION_LIST ConnectionList,
    IN ULONG NumberOfSessions,
    IN PUCHAR Buffer
    )
{
    PISCSI_RedirectPortalInfo redirectPortalInfo;
    PISCSI_RedirectSessionInfo redirectSessionInfo;
    PMSiSCSI_RedirectPortalInfoClass redirectPortalInfoClass;
    ULONG sessionIndex;
    ULONG connectionIndex;
    ULONG inx, skip;
    PWMIString WMIformatString;
    UCHAR status = SRB_STATUS_SUCCESS;


    if (ConnectionList == NULL) {

        DebugPrint((iScsiPrtDebugError,
            "Connectionlist NULL in iSpReadInitiatorSessionInfo\n"));

        ETWDebugPrint(iScsiLevelError,
            iScsiFlagWmi,
            "Connectionlist NULL in iSpReadInitiatorSessionInfo\n");

        return status;
    }

    redirectPortalInfoClass = (PMSiSCSI_RedirectPortalInfoClass) Buffer;

    redirectPortalInfoClass->UniqueAdapterId = (ULONGLONG) AdapterExtension;

    //
    // initialize Session count to 0
    //
    redirectPortalInfoClass->SessionCount = 0;

    redirectSessionInfo = redirectPortalInfoClass->RedirectSessionList;

    inx = 0;
    while (inx < NumberOfSessions) {

        PISCSI_SESSION iScsiSession;
        PISCSI_CONNECTION iScsiConnection;

        iScsiSession = ConnectionList[inx].ISCSISession;

        if (iScsiSession == NULL) {

            inx++;

            continue;
        }

        redirectSessionInfo->UniqueSessionId = iScsiSession->SessionId;

        redirectSessionInfo->TargetPortalGroupTag = iScsiSession->TargetPortalGroupTag;

        redirectSessionInfo->ConnectionCount = 0;

        redirectPortalInfo = redirectSessionInfo->RedirectPortalList;

        for (connectionIndex = 0;
             connectionIndex < ConnectionList[inx].Count;
             connectionIndex++)
        {

            iScsiConnection = ConnectionList[inx].ISCSIConnection[connectionIndex];
            if (iScsiConnection != NULL) {

                redirectPortalInfo->UniqueConnectionId = iScsiConnection->UniqueConnectionID;

                iSpCopyIPAddress(&redirectPortalInfo->OriginalIPAddr,
                                 &iScsiConnection->PermanentTargetIPAddress,
                                 &redirectPortalInfo->OriginalPort);

                redirectPortalInfo->OriginalIPAddr.TextAddress[0] = MAX_ISCSI_TEXT_ADDRESS_LEN*sizeof(WCHAR);

                redirectPortalInfo->Redirected = iScsiConnection->TargetMoved;

                if (iScsiConnection->TargetMoved) {

                    iSpCopyIPAddress(&redirectPortalInfo->RedirectedIPAddr,
                                     &iScsiConnection->TargetIPAddress,
                                     &redirectPortalInfo->RedirectedPort);

                    redirectPortalInfo->RedirectedIPAddr.TextAddress[0] = MAX_ISCSI_TEXT_ADDRESS_LEN*sizeof(WCHAR);

                    redirectPortalInfo->TemporaryRedirect = iScsiConnection->TargetMoveTemporary;
                }

                //increment the connectionStaticInfo pointer to the next 4 byte alligned address;
                (PUCHAR) redirectPortalInfo += ((sizeof(ISCSI_RedirectPortalInfo)+7) & ~7);

                //The number of TCP connections that currently belong to this session
                redirectSessionInfo->ConnectionCount ++;
            }
        }

        //
        // Advance to next place to write session information
        //
        skip = FIELD_OFFSET(ISCSI_RedirectSessionInfo, RedirectPortalList) +
                            (ConnectionList[inx].Count *
                             ((sizeof(ISCSI_RedirectPortalInfo) + 7) & ~7));

        skip = (skip + 7) & ~7;

        (PUCHAR) redirectSessionInfo += skip;

        redirectPortalInfoClass->SessionCount++;

        inx++;
    }

    return status;
}

NTSTATUS
iSpFindPersistentTargetKey(
    IN PISCSI_ADAPTER_EXTENSION AdapterExtension,
    _In_ PWSTR TargetName,
    IN PISCSI_TargetPortal TargetPortal,
    IN ULONG PortNumber,
    OUT PHANDLE TargetKey,
    _Out_writes_opt_ (TargetKeyNameSize) PWCHAR TargetKeyName, OPTIONAL
    IN ULONG TargetKeyNameSize
    )
{
    HANDLE persistentTargetsKey = NULL;
    HANDLE pnpDriverKey;
    NTSTATUS status;

    *TargetKey = NULL;

    status = iSpOpenPnPDriverKey(AdapterExtension->PhysicalDeviceObject,
                                 PERSISTENT_TARGETS,
                                 KEY_ALL_ACCESS,
                                 &pnpDriverKey,
                                 &persistentTargetsKey);
    if (NT_SUCCESS(status)) {

        KEY_BASIC_INFORMATION keyInfo[ISCSI_KEY_BASIC_INFO_SIZE];
        ULONG resultSize;
        ULONG index;
        BOOLEAN found;

        index = 0;
        found = FALSE;

        while (!found) {

            resultSize = ISCSI_KEY_BASIC_INFO_SIZE * sizeof(KEY_BASIC_INFORMATION);

            RtlZeroMemory(keyInfo, resultSize);

            //
            // Enumerate all the keys under PersistentTargets key
            //
            status = ZwEnumerateKey(persistentTargetsKey,
                                    index,
                                    KeyBasicInformation,
                                    keyInfo,
                                    resultSize,
                                    &resultSize);
            if (NT_SUCCESS(status)) {

                PUCHAR regValue;
                HANDLE targetNameKey;
                OBJECT_ATTRIBUTES objectAttributes;
                UNICODE_STRING subKeyName;
                WCHAR targetKeyName[512];
                ULONG nameLength;
                ULONG targetnameLen;
                ULONG regValueSize;

                nameLength = keyInfo->NameLength;

                if (nameLength > ISCSI_KEY_BASIC_INFO_NAME_MAXLEN) {
                    nameLength = ISCSI_KEY_BASIC_INFO_NAME_MAXLEN;
                }

                //
                // Find the key which matches the given target name
                //

                status = RtlStringCchCopyNW(
                            targetKeyName,
                            sizeof(targetKeyName)/sizeof(targetKeyName[0]),
                            keyInfo->Name,
                            nameLength);
                
                NT_ASSERT(NT_SUCCESS(status));

                if(NT_SUCCESS(status) == FALSE)
                {
                    goto NextKey;
                }

                targetnameLen = wcslen(TargetName);

                if (!wcsncmp(TargetName, targetKeyName, targetnameLen)) {

                    //
                    // Open LoginTarget under this target key
                    //
                    RtlStringCchCatW(targetKeyName, 255, L"\\");

                    RtlStringCchCatW(targetKeyName, 255, LOGIN_TARGET_STR);

                    RtlInitUnicodeString(&subKeyName, targetKeyName);

                    DebugPrint((iScsiPrtDebugInfo,
                        "Persistent Target Key %ws\n",
                        targetKeyName));

                    ETWDebugPrint(iScsiLevelInfo,
                        iScsiFlagWmi,
                        "Persistent Target Key %ws\n",
                        targetKeyName);

                    RtlZeroMemory(&objectAttributes, sizeof(OBJECT_ATTRIBUTES));

                    InitializeObjectAttributes(&objectAttributes,
                                               &subKeyName,
                                               (OBJ_CASE_INSENSITIVE | OBJ_KERNEL_HANDLE),
                                               persistentTargetsKey,
                                               (PSECURITY_DESCRIPTOR) NULL);

                    status = ZwOpenKey(&targetNameKey,
                                       KEY_ALL_ACCESS,
                                       &objectAttributes);
                    if (NT_SUCCESS(status)) {

                        status = iSpQueryRegistryValue(targetNameKey, LOGIN_TARGET_IN,
                                                       ISCSI_TAG_LOGINTARGET_IN,
                                                       &regValue, &regValueSize);
                        if (NT_SUCCESS(status)) {
                            PLoginToTarget_IN loginTargetIN;
                            PISCSI_TargetPortal targetPortal;

                            loginTargetIN = (PLoginToTarget_IN) regValue;

                            targetPortal = &(loginTargetIN->TargetPortal);

                            if ((PortNumber == loginTargetIN->PortNumber) &&
                                (targetPortal->Address.Type != ISCSI_IP_ADDRESS_EMPTY) &&
                                (IsSamePortal(TargetPortal, targetPortal))) {

                                found = TRUE;

                                //
                                // Open the TargetName key and return a handle to that
                                //
                                status  = RtlStringCchCopyNW(
                                                targetKeyName,
                                                sizeof(targetKeyName)/sizeof(targetKeyName[0]),
                                                keyInfo->Name,
                                                nameLength);
                                
                                NT_ASSERT(NT_SUCCESS(status));
                                
                                if(NT_SUCCESS(status) == TRUE)
                                {
                                    RtlInitUnicodeString(&subKeyName, targetKeyName);

                                    RtlZeroMemory(&objectAttributes, sizeof(OBJECT_ATTRIBUTES));

                                    InitializeObjectAttributes(&objectAttributes,
                                                               &subKeyName,
                                                               (OBJ_CASE_INSENSITIVE | OBJ_KERNEL_HANDLE),
                                                               persistentTargetsKey,
                                                               (PSECURITY_DESCRIPTOR) NULL);

                                    status = ZwOpenKey(TargetKey,
                                                       KEY_ALL_ACCESS,
                                                       &objectAttributes);
                                    if (NT_SUCCESS(status)) {

                                        if (TargetKeyName != NULL) {

                                            NT_ASSERT(TargetKeyNameSize > 0);

                                            status = RtlStringCchCopyNW(
                                                            TargetKeyName,
                                                            TargetKeyNameSize,
                                                            targetKeyName,
                                                            sizeof(targetKeyName)/sizeof(targetKeyName[0]));

                                            NT_ASSERT(NT_SUCCESS(status));

                                            if(NT_SUCCESS(status) == FALSE)
                                            {
                                                DebugPrint((iScsiPrtDebugError,
                                                    "Failed to copy TargetKeyName %ws\n",
                                                    targetKeyName));

                                                ETWDebugPrint(iScsiLevelError,
                                                    iScsiFlagWmi,
                                                    "Failed to copy TargetKeyName %ws\n",
                                                    targetKeyName);

                                                *TargetKey = NULL;
                                            }
                                        }
                                    } else {
                                        *TargetKey = NULL;
                                    }
                                } else {

                                    DebugPrint((iScsiPrtDebugError,
                                        "Failed to copy target name from KeyInfo.\n"));

                                    ETWDebugPrint(iScsiLevelError,
                                        iScsiFlagWmi,
                                        "Failed to copy target name from KeyInfo.\n");
                                }
                            } else {

                                DebugPrint((iScsiPrtDebugError,
                                    "Portal mismatch. Port Numbers %x, %x\n",
                                    PortNumber, loginTargetIN->PortNumber));

                                ETWDebugPrint(iScsiLevelError,
                                    iScsiFlagWmi,
                                    "Portal mismatch. Port Numbers %x, %x\n",
                                    PortNumber, loginTargetIN->PortNumber);
                            }

                            ExFreePool(regValue);
                        }

                        ZwClose(targetNameKey);
                    }
                }
            } else if (status == STATUS_NO_MORE_ENTRIES) {

                DebugPrint((iScsiPrtDebugError,
                    "Enumerated all persistent targets\n"));

                ETWDebugPrint(iScsiLevelError,
                    iScsiFlagWmi,
                    "Enumerated all persistent targets\n");

                break;
            } else {

                DebugPrint((iScsiPrtDebugError,
                    "ZwEnumerateKey failed for persistent targets. Status %x\n",
                    status));

                ETWDebugPrint(iScsiLevelError,
                    iScsiFlagWmi,
                    "ZwEnumerateKey failed for persistent targets. Status %x\n",
                    status);

                break;
            }
NextKey:
            index++;
        }

        iSpClosePnPDriverKey(&pnpDriverKey, &persistentTargetsKey);
    }

    return status;
}

NTSTATUS
iSpDeleteKeyAndSubkeys(
    IN HANDLE RootKey
    )
{
    KEY_BASIC_INFORMATION keyInfo[ISCSI_KEY_BASIC_INFO_SIZE];
    NTSTATUS status = STATUS_SUCCESS;
    ULONG resultSize;
    ULONG index;
    BOOLEAN deleted = FALSE;

    index = 0;

    //
    // Enumerate all the keys under the given key and delete each one of them.
    // Note that this routine assumes there are no subkeys and the subkeys
    // to the Root Key. This routine can only delete a registry tree with
    // one sub level below the Root Key.
    //
    while (status == STATUS_SUCCESS) {

        resultSize = ISCSI_KEY_BASIC_INFO_SIZE * sizeof(KEY_BASIC_INFORMATION);

        RtlZeroMemory(keyInfo, resultSize);

        //
        // Enumerate all the keys under the given key
        //
        status = ZwEnumerateKey(RootKey,
                                index,
                                KeyBasicInformation,
                                keyInfo,
                                resultSize,
                                &resultSize);
        if (NT_SUCCESS(status)) {

            HANDLE subKeyHandle;
            UNICODE_STRING subKey;
            WCHAR subKeyName[ISCSI_KEY_BASIC_INFO_NAME_MAXLEN + 1]; // add 1 for null terminator
            OBJECT_ATTRIBUTES objectAttributes;
            ULONG nameLength;

            deleted = FALSE;

            nameLength = keyInfo->NameLength;
            if (nameLength > ISCSI_KEY_BASIC_INFO_NAME_MAXLEN) {
                nameLength = ISCSI_KEY_BASIC_INFO_NAME_MAXLEN;
            }

            status = RtlStringCchCopyNW(
                        subKeyName,
                        sizeof(subKeyName) / sizeof(subKeyName[0]),
                        keyInfo->Name,
                        nameLength);

            NT_ASSERT(NT_SUCCESS(status));

            if(NT_SUCCESS(status) == FALSE)
            {
                break;
            }

            RtlInitUnicodeString(&subKey, subKeyName);

            RtlZeroMemory(&objectAttributes, sizeof(OBJECT_ATTRIBUTES));

            InitializeObjectAttributes(&objectAttributes,
                                       &subKey,
                                       (OBJ_CASE_INSENSITIVE | OBJ_KERNEL_HANDLE),
                                       RootKey,
                                       (PSECURITY_DESCRIPTOR) NULL);

            //
            // Open this subkey and delete it.
            //
            status = ZwOpenKey(&subKeyHandle,
                               KEY_ALL_ACCESS,
                               &objectAttributes);
            if (NT_SUCCESS(status)) {

                //
                // Note that there should be no subkeys under this key. Otherwise
                // ZwDeleteKey will fail with error STATUS_CANNOT_DELETE.
                //
                status = ZwDeleteKey(subKeyHandle);
                if (NT_SUCCESS(status)) {

                    DebugPrint((iScsiPrtDebugInfo,
                        "Deleted subkey %ws\n",
                        subKeyName));

                    ETWDebugPrint(iScsiLevelTrace,
                        iScsiFlagGeneral,
                        "Deleted subkey %ws\n",
                        subKeyName);

                    deleted = TRUE;
                } else {

                    
                    DebugPrint((iScsiPrtDebugError, 
                                "Failed to delete subkey %ws. Status %x\n",
                                subKeyName, status));

                    ETWDebugPrint(iScsiLevelError,
                        iScsiFlagGeneral,
                        "Failed to delete subkey %ws. Status %x\n",
                        subKeyName, status);
                }
                
                ZwClose(subKeyHandle);

            }
        } else if (status != STATUS_NO_MORE_ENTRIES) {

            DebugPrint((iScsiPrtDebugError,
                "Failed to enumerate subkeys in delete key and subkeys. Status %x\n",
                status));

            ETWDebugPrint(iScsiLevelError,
                iScsiFlagGeneral,
                "Failed to enumerate subkeys in delete key and subkeys. Status %x\n",
                status);
        }

        //
        // If the key was successfully deleted do not increment the index since
        // the number of keys under RootKey is now lesser by one.
        //
        if (!deleted) {
            index++;
        }
    }

    if (!NT_SUCCESS(status) && (status == STATUS_NO_MORE_ENTRIES)) {

        //
        // This indicates that all the subkeys under Root Key have been
        // enumerated and they have been deleted successfully. It is now
        // safe to delete the Root Key.
        //
        status = STATUS_SUCCESS;

        status = ZwDeleteKey(RootKey);
        if (!NT_SUCCESS(status)) {

            DebugPrint((iScsiPrtDebugError,
                "Failed to delete Root Key. Status %x\n",
                status));

            ETWDebugPrint(iScsiLevelError,
                iScsiFlagGeneral,
                "Failed to delete Root Key. Status %x\n",
                status);
        } else {

            DebugPrint((iScsiPrtDebugTrace,
                "Successfully delete all keys and subkeys\n"));

            ETWDebugPrint(iScsiLevelTrace,
                iScsiFlagGeneral,
                "Successfully delete all keys and subkeys\n");
        }
    } else {

        DebugPrint((iScsiPrtDebugError,
            "Failed to delete key and all subkeys. Status %x\n",
            status));

        ETWDebugPrint(iScsiLevelError,
            iScsiFlagGeneral,
            "Failed to delete key and all subkeys. Status %x\n",
            status);
    }

    return status;
}

NTSTATUS
iSpPersisteLBPolicy(
    IN PISCSI_ADAPTER_EXTENSION AdapterExtension,
    IN PPERSISTENT_TARGET_LB_POLICY LBPolicy
    )
{
    HANDLE persistentTargetsKey;
    HANDLE pnpDriverKey;
    PLB_POLICY_DATA LBPolicyData;
    ULONG inx;
    ULONG lbPolicy;
    NTSTATUS status;

    PAGED_CODE();

    status = iSpOpenPnPDriverKey(AdapterExtension->PhysicalDeviceObject,
                                 PERSISTENT_TARGETS,
                                 KEY_ALL_ACCESS,
                                 &pnpDriverKey,
                                 &persistentTargetsKey);
    if (NT_SUCCESS(status)) {

        OBJECT_ATTRIBUTES objectAttributes;
        HANDLE subKeyHandle;
        HANDLE loginTargetKey;
        UNICODE_STRING subKey;
        WCHAR subKeyName[32];

        RtlZeroMemory(subKeyName,sizeof(subKeyName));

        RtlInitUnicodeString(&subKey, LBPolicy->TargetKeyName);

        RtlZeroMemory(&objectAttributes, sizeof(OBJECT_ATTRIBUTES));

        InitializeObjectAttributes(&objectAttributes,
                                   &subKey,
                                   (OBJ_CASE_INSENSITIVE | OBJ_KERNEL_HANDLE),
                                   persistentTargetsKey,
                                   (PSECURITY_DESCRIPTOR) NULL);

        status = ZwOpenKey(&subKeyHandle,
                           KEY_ALL_ACCESS,
                           &objectAttributes);

        if (NT_SUCCESS(status)) {

            //
            // Save the new Load Balance policy for this target
            //
            status = RtlWriteRegistryValue(RTL_REGISTRY_HANDLE,
                                           subKeyHandle,
                                           LOAD_BALANCE_POLICY,
                                           REG_DWORD,
                                           &LBPolicy->LoadBalancePolicy,
                                           sizeof(ISCSI_LOAD_BALANCE_POLICY));
            NT_ASSERT(NT_SUCCESS(status));

            for (inx = 0; NT_SUCCESS(status) && inx < LBPolicy->ConnectionCount; inx++) {

                //
                // Save the PrimaryPath and PathWeight for each connection
                //
                LBPolicyData = &(LBPolicy->PolicyData[inx]);

                if (LBPolicyData->ConnectionNumber == -1) {

                    RtlStringCchCopyW(subKeyName, 32, LOGIN_TARGET_STR);
                } else {

                    RtlStringCchPrintfW(subKeyName,
                                        ((sizeof(subKeyName) - sizeof(WCHAR))/sizeof(WCHAR)),
                                        L"%ws%d",
                                        ADD_CONNECTION_STR,
                                        LBPolicyData->ConnectionNumber);
                }

                DebugPrint((iScsiPrtDebugInfo,
                    "Will open subkey %ws\n",
                    subKeyName));

                ETWDebugPrint(iScsiLevelInfo,
                    iScsiFlagWmi,
                    "Will open subkey %ws\n",
                    subKeyName);

                RtlInitUnicodeString(&subKey, subKeyName);

                RtlZeroMemory(&objectAttributes, sizeof(OBJECT_ATTRIBUTES));

                InitializeObjectAttributes(&objectAttributes,
                                           &subKey,
                                           (OBJ_CASE_INSENSITIVE | OBJ_KERNEL_HANDLE),
                                           subKeyHandle,
                                           (PSECURITY_DESCRIPTOR) NULL);

                status = ZwOpenKey(&loginTargetKey,
                                   KEY_ALL_ACCESS,
                                   &objectAttributes);
                if (NT_SUCCESS(status)) {

                    //
                    // Save PathWeight and PrimaryPath
                    //
                    status = RtlWriteRegistryValue(RTL_REGISTRY_HANDLE,
                                                   loginTargetKey,
                                                   PATH_WEIGHT,
                                                   REG_DWORD,
                                                   &LBPolicyData->PathWeight,
                                                   sizeof(ULONG));
                    if (!NT_SUCCESS(status)) {

                        DebugPrint((iScsiPrtDebugError,
                            "Failed to save Path Weight. Status %x\n",
                            status));

                        ETWDebugPrint(iScsiLevelError,
                            iScsiFlagWmi,
                            "Failed to save Path Weight. Status %x\n",
                            status);
                    }

                    status = RtlWriteRegistryValue(RTL_REGISTRY_HANDLE,
                                                   loginTargetKey,
                                                   PRIMARY_PATH,
                                                   REG_DWORD,
                                                   &LBPolicyData->PrimaryPath,
                                                   sizeof(ULONG));
                    if (!NT_SUCCESS(status)) {

                        DebugPrint((iScsiPrtDebugError,
                            "Failed to save Path Weight. Status %x\n",
                            status));

                        ETWDebugPrint(iScsiLevelError,
                            iScsiFlagWmi,
                            "Failed to save Path Weight. Status %x\n",
                            status);
                    }

                    ZwClose(loginTargetKey);
                } else {

                    DebugPrint((iScsiPrtDebugError,
                        "Failed to open subkey subKeyName to save LB Policy\n"));

                    ETWDebugPrint(iScsiLevelError,
                        iScsiFlagWmi,
                        "Failed to open subkey subKeyName to save LB Policy\n");
                }
            }

            ZwClose(subKeyHandle);
        }

        iSpClosePnPDriverKey(&pnpDriverKey, &persistentTargetsKey);
    }

    return status;
}

NTSTATUS
iSpSetTargetNameSuffix(
    IN PISCSI_ADAPTER_EXTENSION AdapterExtension,
    IN PLoginToTarget_IN LoginTargetIn
    )
{
    PISCSI_SESSION iScsiSession = NULL;
    HANDLE removeTargetKey = NULL;
    WCHAR targetName[MAX_TARGET_NAME + 1];
    WCHAR targetKeyName[MAX_TARGET_NAME + TARGET_KEYNAME_SUFFIX_SIZE + 1];
    ULONG bytesToCopy;
    KIRQL oldIrql;
    NTSTATUS rtlStatus = ERROR_SUCCESS;

    if (LoginTargetIn->SessionType != ISCSI_LOGINTARGET_DATA) {
        return STATUS_INVALID_PARAMETER;
    }

    RtlZeroMemory(targetKeyName, sizeof(targetKeyName));
    RtlZeroMemory(targetName, sizeof(targetName));

    bytesToCopy = *(LoginTargetIn->TargetName);
    if (bytesToCopy > (MAX_TARGET_NAME * sizeof(WCHAR))) {
        bytesToCopy = MAX_TARGET_NAME * sizeof(WCHAR);
    }

    RtlCopyMemory(targetName,
                  (LoginTargetIn->TargetName + 1),
                  bytesToCopy);

    rtlStatus = iSpFindPersistentTargetKey(
                    AdapterExtension,
                    targetName,
                    &(LoginTargetIn->TargetPortal),
                    LoginTargetIn->PortNumber,
                    &removeTargetKey,
                    targetKeyName,
                    sizeof(targetKeyName)/sizeof(targetKeyName[0]));

    if (NT_SUCCESS(rtlStatus) && removeTargetKey != NULL) {

        PWCHAR suffixPtr;
        PISCSI_CONNECTION iScsiConnection;
        PLIST_ENTRY sessionEntry;
        PLIST_ENTRY connectionEntry;
        PISCSI_TRANSPORT_ADDRESS targetIPAddress;
        ISCSI_TargetPortal targetPortal;
        BOOLEAN found = FALSE;

        suffixPtr = wcschr(targetKeyName, L'#');
        if (suffixPtr != NULL) {

            DebugPrint((iScsiPrtDebugInfo,
                "Suffix is %ws\n",
                suffixPtr));

            ETWDebugPrint(iScsiLevelInfo,
                iScsiFlagProtocolLogInOut,
                "Suffix is %ws\n",
                suffixPtr);

            RtlZeroMemory(&targetPortal, sizeof(targetPortal));

            KeAcquireSpinLock(&AdapterExtension->AdapterLock, &oldIrql);

            sessionEntry = AdapterExtension->IScsiSessionList.Flink;
            while (sessionEntry != &AdapterExtension->IScsiSessionList) {

                iScsiSession = CONTAINING_RECORD(sessionEntry,
                                                 ISCSI_SESSION,
                                                 NextSession);

                sessionEntry = sessionEntry->Flink;

                if (wcsncmp(targetName,
                            iScsiSession->TargetName,
                            MAX_TARGET_NAME) == 0) {

                    connectionEntry = iScsiSession->ConnectionList.Flink;
                    while (connectionEntry != &iScsiSession->ConnectionList) {

                        iScsiConnection = CONTAINING_RECORD(connectionEntry,
                                                            ISCSI_CONNECTION,
                                                            NextConnection);

                        connectionEntry = connectionEntry->Flink;

                        targetIPAddress = &(iScsiConnection->TargetIPAddress);

                        if (targetIPAddress->AddressType == TDI_ADDRESS_TYPE_IP) {

                            targetPortal.Address.Type = ISCSI_IP_ADDRESS_IPV4;

                            targetPortal.Socket =
                                targetIPAddress->IPAddress.IPv4Address.sin_port;

                            targetPortal.Address.IpV4Address =
                                targetIPAddress->IPAddress.IPv4Address.in_addr;
                        } else {

                            targetPortal.Address.Type = ISCSI_IP_ADDRESS_IPV6;

                            targetPortal.Socket =
                                targetIPAddress->IPAddress.IPv6Address.sin6_port;

                            targetPortal.Address.IpV6FlowInfo =
                                targetIPAddress->IPAddress.IPv6Address.sin6_flowinfo;

                            targetPortal.Address.IpV6ScopeId =
                                targetIPAddress->IPAddress.IPv6Address.sin6_scope_id;

                            RtlCopyMemory(targetPortal.Address.IpV6Address,
                                          targetIPAddress->IPAddress.IPv6Address.sin6_addr,
                                          sizeof(targetPortal.Address.IpV6Address));
                        }

                        if (IsSamePortal(&targetPortal, &(LoginTargetIn->TargetPortal))) {

                            DebugPrint((iScsiPrtDebugInfo,
                                "Found session %p\n",
                                iScsiSession));

                            ETWDebugPrint(iScsiLevelInfo,
                                iScsiFlagProtocolLogInOut,
                                "Found session %p\n",
                                iScsiSession);

                            found = TRUE;
                            break;
                        }
                    }
                }

                if (found) {
                    break;
                }
            }

            KeReleaseSpinLock(&AdapterExtension->AdapterLock, oldIrql);

            if (found) {

                DebugPrint((iScsiPrtDebugInfo,
                    "Will copy suffix %ws to session %p\n",
                    suffixPtr,
                    iScsiSession));

                ETWDebugPrint(iScsiLevelInfo,
                    iScsiFlagProtocolLogInOut,
                    "Will copy suffix %ws to session %p\n",
                    suffixPtr,
                    iScsiSession);

                rtlStatus = RtlStringCchCopyNW(iScsiSession->TargetKeyNameSuffix,
                                               TARGET_KEYNAME_SUFFIX_SIZE,
                                               suffixPtr,
                                               (TARGET_KEYNAME_SUFFIX_SIZE - 1));

                iScsiSession->PersistentLogin = TRUE;
            }
        } else {

            DebugPrint((iScsiPrtDebugError,
                "No suffix found for target %ws\n",
                targetKeyName));

            ETWDebugPrint(iScsiLevelError,
                iScsiFlagProtocolLogInOut,
                "No suffix found for target %ws\n",
                targetKeyName);
        }

        ZwClose(removeTargetKey);
    } else {

        DebugPrint((iScsiPrtDebugError,
            "%ws is probably not a persistent target\n",
            targetName));

        ETWDebugPrint(iScsiLevelError,
            iScsiFlagProtocolLogInOut,
            "%ws is probably not a persistent target\n",
            targetName);

    }

    return rtlStatus;
}

