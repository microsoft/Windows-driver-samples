//
//    Copyright (C) Microsoft.  All rights reserved.
//
////////////////////////////////////////////////////////////////////////////////
//
//  INCLUDES
//
////////////////////////////////////////////////////////////////////////////////
#include "precomp.h"
#include "Request.tmh"
#include "WMBClassTelemetry.h"

#pragma prefast(push)
// Disables oacr check for 26014(INCORRECT_VALIDATION) because we use many MBB struct carrying 1-element array for data buffer
// e.g.
//      struct MBB_X {
//        MBB_STRING str; // empty string
//        UCHAR DataBuffer[ 1 ];
//      }
// The real size of payload could be "sizeof(MBB_X) - 1."
// To avoid noise, disable this specific validation in OACR as the implementation already validates the payload
#pragma prefast(disable: __WARNING_INCORRECT_VALIDATION) // allowed off-by-1 struct

C_ASSERT(MbbCellularClassMaximum == 3 );

////////////////////////////////////////////////////////////////////////////////
//
//  DECLARATIONS
//
////////////////////////////////////////////////////////////////////////////////
NDIS_STATUS
MbbNdisProcessSerializedOidRequest(
    __in MBB_PROTOCOL_HANDLE                AdapterHandle,
    __in PMBB_REQUEST_CONTEXT               Request
    );

VOID
MbbNdisIndicateStatus(
    __in PMBB_REQUEST_CONTEXT               Request,
    __in NDIS_STATUS                        NdisStatus,
    __in MBB_STATUS                         MbbStatus,
    __in_bcount_opt(InBufferSize) PUCHAR    InBuffer,
    __in ULONG                              InBufferSize
    );

//
// Get handlers
//

NDIS_STATUS
MbbNdisGetVendorDriverVersion(
    __in PMBB_REQUEST_CONTEXT                                   Request,
    __in_bcount_opt(*InBufferSize) PUCHAR                       InBuffer,
    __in PULONG                                                 InBufferSize,
    __out_bcount_part_opt(*OutBufferSize,*OutBufferSize) PUCHAR OutBuffer,
    __inout PULONG                                              OutBufferSize
    );

NDIS_STATUS
MbbNdisGetVendorId(
    __in PMBB_REQUEST_CONTEXT                                   Request,
    __in_bcount_opt(*InBufferSize) PUCHAR                       InBuffer,
    __in PULONG                                                 InBufferSize,
    __out_bcount_part_opt(*OutBufferSize,*OutBufferSize) PUCHAR OutBuffer,
    __inout PULONG                                              OutBufferSize
    );

NDIS_STATUS
MbbNdisGetVendorDescription(
    __in PMBB_REQUEST_CONTEXT                                   Request,
    __in_bcount_opt(*InBufferSize) PUCHAR                       InBuffer,
    __in PULONG                                                 InBufferSize,
    __out_bcount_part_opt(*OutBufferSize,*OutBufferSize) PUCHAR OutBuffer,
    __inout PULONG                                              OutBufferSize
    );

NDIS_STATUS
MbbNdisGetReceiveBlockSize(
    __in PMBB_REQUEST_CONTEXT                                   Request,
    __in_bcount_opt(*InBufferSize) PUCHAR                       InBuffer,
    __in PULONG                                                 InBufferSize,
    __out_bcount_part_opt(*OutBufferSize,*OutBufferSize) PUCHAR OutBuffer,
    __inout PULONG                                              OutBufferSize
    );

NDIS_STATUS
MbbNdisGetReceiveBufferSpace(
    __in PMBB_REQUEST_CONTEXT                                   Request,
    __in_bcount_opt(*InBufferSize) PUCHAR                       InBuffer,
    __in PULONG                                                 InBufferSize,
    __out_bcount_part_opt(*OutBufferSize,*OutBufferSize) PUCHAR OutBuffer,
    __inout PULONG                                              OutBufferSize
    );

NDIS_STATUS
MbbNdisGetTransmitBlockSize(
    __in PMBB_REQUEST_CONTEXT                                   Request,
    __in_bcount_opt(*InBufferSize) PUCHAR                       InBuffer,
    __in PULONG                                                 InBufferSize,
    __out_bcount_part_opt(*OutBufferSize,*OutBufferSize) PUCHAR OutBuffer,
    __inout PULONG                                              OutBufferSize
    );

NDIS_STATUS
MbbNdisGetTransmitBufferSpace(
    __in PMBB_REQUEST_CONTEXT                                   Request,
    __in_bcount_opt(*InBufferSize) PUCHAR                       InBuffer,
    __in PULONG                                                 InBufferSize,
    __out_bcount_part_opt(*OutBufferSize,*OutBufferSize) PUCHAR OutBuffer,
    __inout PULONG                                              OutBufferSize
    );

NDIS_STATUS
MbbNdisGetMaximumTotalSize(
    __in PMBB_REQUEST_CONTEXT                                   Request,
    __in_bcount_opt(*InBufferSize) PUCHAR                       InBuffer,
    __in PULONG                                                 InBufferSize,
    __out_bcount_part_opt(*OutBufferSize,*OutBufferSize) PUCHAR OutBuffer,
    __inout PULONG                                              OutBufferSize
    );

NDIS_STATUS
MbbNdisGetInterruptModeration(
    __in PMBB_REQUEST_CONTEXT                                   Request,
    __in_bcount_opt(*InBufferSize) PUCHAR                       InBuffer,
    __in PULONG                                                 InBufferSize,
    __out_bcount_part_opt(*OutBufferSize,*OutBufferSize) PUCHAR OutBuffer,
    __inout PULONG                                              OutBufferSize
    );

NDIS_STATUS
MbbNdisGetCurrentLookahead(
    __in PMBB_REQUEST_CONTEXT                                   Request,
    __in_bcount_opt(*InBufferSize) PUCHAR                       InBuffer,
    __in PULONG                                                 InBufferSize,
    __out_bcount_part_opt(*OutBufferSize,*OutBufferSize) PUCHAR OutBuffer,
    __inout PULONG                                              OutBufferSize
    );

NDIS_STATUS
MbbNdisGetFramesReceived(
    __in PMBB_REQUEST_CONTEXT                                   Request,
    __in_bcount_opt(*InBufferSize) PUCHAR                       InBuffer,
    __in PULONG                                                 InBufferSize,
    __out_bcount_part_opt(*OutBufferSize,*OutBufferSize) PUCHAR OutBuffer,
    __inout PULONG                                              OutBufferSize
    );

NDIS_STATUS
MbbNdisGetFramesTransmitted(
    __in PMBB_REQUEST_CONTEXT                                   Request,
    __in_bcount_opt(*InBufferSize) PUCHAR                       InBuffer,
    __in PULONG                                                 InBufferSize,
    __out_bcount_part_opt(*OutBufferSize,*OutBufferSize) PUCHAR OutBuffer,
    __inout PULONG                                              OutBufferSize
    );

NDIS_STATUS
MbbNdisGetMediaSupported(
    __in PMBB_REQUEST_CONTEXT                                   Request,
    __in_bcount_opt(*InBufferSize) PUCHAR                       InBuffer,
    __in PULONG                                                 InBufferSize,
    __out_bcount_part_opt(*OutBufferSize,*OutBufferSize) PUCHAR OutBuffer,
    __inout PULONG                                              OutBufferSize
    );

NDIS_STATUS
MbbNdisGetDriverCaps(
    __in PMBB_REQUEST_CONTEXT                                   Request,
    __in_bcount_opt(*InBufferSize) PUCHAR                       InBuffer,
    __in PULONG                                                 InBufferSize,
    __out_bcount_part_opt(*OutBufferSize,*OutBufferSize) PUCHAR OutBuffer,
    __inout PULONG                                              OutBufferSize
    );

NDIS_STATUS
MbbNdisGetStatistics(
    __in PMBB_REQUEST_CONTEXT                                   Request,
    __in_bcount_opt(*InBufferSize) PUCHAR                       InBuffer,
    __in PULONG                                                 InBufferSize,
    __out_bcount_part_opt(*OutBufferSize,*OutBufferSize) PUCHAR OutBuffer,
    __inout PULONG                                              OutBufferSize
    );

// MAC
NDIS_STATUS
MbbNdisCreateMac(
    __in PMBB_REQUEST_CONTEXT                                   Request,
    __in_bcount_opt(*InBufferSize) PUCHAR                       InBuffer,
    __in PULONG                                                 InBufferSize,
    __out_bcount_part_opt(*OutBufferSize,*OutBufferSize) PUCHAR OutBuffer,
    __inout PULONG                                              OutBufferSize
   );

NDIS_STATUS
MbbNdisGetMbimVersion(
    __in PMBB_REQUEST_CONTEXT                                   Request,
    __in_bcount_opt(*InBufferSize) PUCHAR                       InBuffer,
    __in PULONG                                                 InBufferSize,
    __out_bcount_part_opt(*OutBufferSize, *OutBufferSize) PUCHAR OutBuffer,
    __inout PULONG                                              OutBufferSize
);


#define MbbNdisGetDeviceCaps                MbbUtilQueryAttribute

#define MbbNdisGetReadyInfo                 MbbUtilQueryAttribute

#define MbbNdisGetEmergencyMode             MbbUtilQueryAttribute

#define MbbNdisGetRadioState                MbbUtilQueryAttribute

#define MbbNdisGetPinInfo                   MbbUtilQueryAttribute

#define MbbNdisGetPinList                   MbbUtilQueryAttribute

#define MbbNdisGetHomeProvider              MbbUtilQueryAttribute

#define MbbNdisGetPreferredProviders        MbbUtilQueryAttribute

#define MbbNdisGetRegisterState             MbbUtilQueryAttribute

#define MbbNdisGetPacketService             MbbUtilQueryAttribute

#define MbbNdisGetSignalState               MbbUtilQueryAttribute

#define MbbNdisGetProvisionedContexts       MbbUtilQueryAttribute

NDIS_STATUS
MbbNdisGetProvisionedContextsV2(
    __in PMBB_REQUEST_CONTEXT                                   Request,
    __in_bcount_opt(*InBufferSize) PUCHAR                       InBuffer,
    __in PULONG                                                 InBufferSize,
    __out_bcount_part_opt(*OutBufferSize,*OutBufferSize) PUCHAR OutBuffer,
    __inout PULONG                                              OutBufferSize
);

#define MbbNdisGetMulticarrierProviders     MbbUtilQueryAttribute

#define MbbNdisGetSarConfig                 MbbUtilQueryAttribute

#define MbbNdisGetSarTransmissionStatus     MbbUtilQueryAttribute

#define MbbNdisGetLteAttachConfig           MbbUtilQueryAttribute

#define MbbNdisGetLteAttachStatus           MbbUtilQueryAttribute

NDIS_STATUS
MbbNdisGetPcoStatus(
    __in PMBB_REQUEST_CONTEXT                                   Request,
    __in_bcount_opt(*InBufferSize) PUCHAR                       InBuffer,
    __in PULONG                                                 InBufferSize,
    __out_bcount_part_opt(*OutBufferSize, *OutBufferSize) PUCHAR OutBuffer,
    __inout PULONG                                              OutBufferSize
);


NDIS_STATUS
MbbNdisQueryPower(
    __in PMBB_REQUEST_CONTEXT                                   Request,
    __in_bcount_opt(*InBufferSize) PUCHAR                       InBuffer,
    __in PULONG                                                 InBufferSize,
    __out_bcount_part_opt(*OutBufferSize,*OutBufferSize) PUCHAR OutBuffer,
    __inout PULONG                                              OutBufferSize
    );

// SMS

NDIS_STATUS
MbbNdisSmsRead(
    __in PMBB_REQUEST_CONTEXT                                   Request,
    __in_bcount_opt(*InBufferSize) PUCHAR                       InBuffer,
    __in PULONG                                                 InBufferSize,
    __out_bcount_part_opt(*OutBufferSize,*OutBufferSize) PUCHAR OutBuffer,
    __inout PULONG                                              OutBufferSize
    );


NDIS_STATUS
MbbNdisGetVisibleProviders(
    __in PMBB_REQUEST_CONTEXT                                   Request,
    __in_bcount_opt(*InBufferSize) PUCHAR                       InBuffer,
    __in PULONG                                                 InBufferSize,
    __out_bcount_part_opt(*OutBufferSize,*OutBufferSize) PUCHAR OutBuffer,
    __inout PULONG                                              OutBufferSize
    );

NDIS_STATUS
MbbNdisGetConnectState(
    __in PMBB_REQUEST_CONTEXT                                   Request,
    __in_bcount_opt(*InBufferSize) PUCHAR                       InBuffer,
    __in PULONG                                                 InBufferSize,
    __out_bcount_part_opt(*OutBufferSize,*OutBufferSize) PUCHAR OutBuffer,
    __inout PULONG                                              OutBufferSize
    );

NDIS_STATUS
MbbIpGetConfiguration(
    __in PMBB_REQUEST_CONTEXT                                   Request,
    __in_bcount_opt(*InBufferSize) PUCHAR                       InBuffer,
    __in PULONG                                                 InBufferSize,
    __out_bcount_part_opt(*OutBufferSize,*OutBufferSize) PUCHAR OutBuffer,
    __inout PULONG                                              OutBufferSize
    );


#define MbbNdisSmsGetConfiguration          MbbUtilQueryAttribute

#define MbbNdisSmsGetStatus                 MbbUtilQueryAttribute

// Device Service

NDIS_STATUS
MbbNdisEnumerateDeviceServices(
    __in PMBB_REQUEST_CONTEXT                                   Request,
    __in_bcount_opt(*InBufferSize) PUCHAR                       InBuffer,
    __in PULONG                                                 InBufferSize,
    __out_bcount_part_opt(*OutBufferSize,*OutBufferSize) PUCHAR OutBuffer,
    __inout PULONG                                              OutBufferSize
    );

NDIS_STATUS
MbbNdisGetDeviceServiceCommand(
    __in PMBB_REQUEST_CONTEXT                                   Request,
    __in_bcount_opt(*InBufferSize) PUCHAR                       InBuffer,
    __in PULONG                                                 InBufferSize,
    __out_bcount_part_opt(*OutBufferSize,*OutBufferSize) PUCHAR OutBuffer,
    __inout PULONG                                              OutBufferSize
    );

NDIS_STATUS
MbbNdisEnumerateDeviceServiceCommands(
    __in PMBB_REQUEST_CONTEXT                                   Request,
    __in_bcount_opt(*InBufferSize) PUCHAR                       InBuffer,
    __in PULONG                                                 InBufferSize,
    __out_bcount_part_opt(*OutBufferSize,*OutBufferSize) PUCHAR OutBuffer,
    __inout PULONG                                              OutBufferSize
    );

// EAP Authentication

NDIS_STATUS
MbbNdisGetAuthChallenge(
    __in PMBB_REQUEST_CONTEXT                                   Request,
    __in_bcount_opt(*InBufferSize) PUCHAR                       InBuffer,
    __in PULONG                                                 InBufferSize,
    __out_bcount_part_opt(*OutBufferSize,*OutBufferSize) PUCHAR OutBuffer,
    __inout PULONG                                              OutBufferSize
    );

// Multi-carrier

NDIS_STATUS
MbbNdisGetMultiCarrierDsCidList(
    __in PMBB_REQUEST_CONTEXT                                   Request,
    __in_bcount_opt(*InBufferSize) PUCHAR                       InBuffer,
    __in PULONG                                                 InBufferSize,
    __out_bcount_part_opt(*OutBufferSize,*OutBufferSize) PUCHAR OutBuffer,
    __inout PULONG                                              OutBufferSize
    );

// UICC
#define MbbNdisGetUiccAppList                                   MbbUtilQueryAttribute

NDIS_STATUS
MbbNdisGetUiccFileStatus(
    __in PMBB_REQUEST_CONTEXT                                   Request,
    __in_bcount_opt(*InBufferSize) PUCHAR                       InBuffer,
    __in PULONG                                                 InBufferSize,
    __out_bcount_part_opt(*OutBufferSize,*OutBufferSize) PUCHAR OutBuffer,
    __inout PULONG                                              OutBufferSize
    );

NDIS_STATUS
MbbNdisUiccReadBinary(
    __in PMBB_REQUEST_CONTEXT                                   Request,
    __in_bcount_opt(*InBufferSize) PUCHAR                       InBuffer,
    __in PULONG                                                 InBufferSize,
    __out_bcount_part_opt(*OutBufferSize,*OutBufferSize) PUCHAR OutBuffer,
    __inout PULONG                                              OutBufferSize
    );

NDIS_STATUS
MbbNdisUiccReadRecord(
    __in PMBB_REQUEST_CONTEXT                                   Request,
    __in_bcount_opt(*InBufferSize) PUCHAR                       InBuffer,
    __in PULONG                                                 InBufferSize,
    __out_bcount_part_opt(*OutBufferSize,*OutBufferSize) PUCHAR OutBuffer,
    __inout PULONG                                              OutBufferSize
    );

NDIS_STATUS
MbbNdisGetPinInfoEx(
    __in PMBB_REQUEST_CONTEXT                                   Request,
    __in_bcount_opt(*InBufferSize) PUCHAR                       InBuffer,
    __in PULONG                                                 InBufferSize,
    __out_bcount_part_opt(*OutBufferSize, *OutBufferSize) PUCHAR OutBuffer,
    __inout PULONG                                              OutBufferSize
);

// Device State and Configuration for MBIM 2.0

NDIS_STATUS
MbbNdisGetSysCapsInfo(
    __in PMBB_REQUEST_CONTEXT                                   Request,
    __in_bcount_opt(*InBufferSize) PUCHAR                       InBuffer,
    __in PULONG                                                 InBufferSize,
    __out_bcount_part_opt(*OutBufferSize, *OutBufferSize) PUCHAR OutBuffer,
    __inout PULONG                                              OutBufferSize
);

NDIS_STATUS
MbbNdisGetDeviceCapsEx(
    __in PMBB_REQUEST_CONTEXT                                   Request,
    __in_bcount_opt(*InBufferSize) PUCHAR                       InBuffer,
    __in PULONG                                                 InBufferSize,
    __out_bcount_part_opt(*OutBufferSize, *OutBufferSize) PUCHAR OutBuffer,
    __inout PULONG                                              OutBufferSize
);

#define MbbNdisGetSysSlotMappings                   MbbUtilQueryAttribute

NDIS_STATUS
MbbNdisGetSlotInfoStatus(
    __in PMBB_REQUEST_CONTEXT                                   Request,
    __in_bcount_opt(*InBufferSize) PUCHAR                       InBuffer,
    __in PULONG                                                 InBufferSize,
    __out_bcount_part_opt(*OutBufferSize,*OutBufferSize) PUCHAR OutBuffer,
    __inout PULONG                                              OutBufferSize
);

#define MbbNdisGetDeviceBindings                    MbbUtilQueryAttribute

// Registration, Signal Strength, and NITZ for MBIM 2.0

#define MbbNdisGetRegisterStateEx                   MbbUtilQueryAttribute

#define MbbNdisGetImsVoiceState                     MbbUtilQueryAttribute

#define MbbNdisGetSignalStateEx                     MbbUtilQueryAttribute

#define MbbNdisGetLocationState                     MbbUtilQueryAttribute

#define MbbNdisGetNitzInfo                          MbbUtilQueryAttribute

// Low-Level UICC Access

NDIS_STATUS
MbbNdisGetAtr(
    __in PMBB_REQUEST_CONTEXT                                   Request,
    __in_bcount_opt(*InBufferSize) PUCHAR                       InBuffer,
    __in PULONG                                                 InBufferSize,
    __out_bcount_part_opt(*OutBufferSize, *OutBufferSize) PUCHAR OutBuffer,
    __inout PULONG                                              OutBufferSize
);

#define MbbNdisGetUiccTerminalCapability            MbbUtilQueryAttribute

NDIS_STATUS
MbbNdisGetUiccReset(
    _In_ PMBB_REQUEST_CONTEXT                                   Request,
    _In_reads_bytes_opt_(*InBufferSize) PUCHAR                  InBuffer,
    _In_ PULONG                                                 InBufferSize,
    _Out_writes_bytes_to_opt_(*OutBufferSize, *OutBufferSize)   PUCHAR OutBuffer,
    _Inout_ PULONG                                              OutBufferSize
);

// Base station information
NDIS_STATUS
MbbNdisGetBaseStationsInfo(
    __in PMBB_REQUEST_CONTEXT                                   Request,
    __in_bcount_opt(*InBufferSize) PUCHAR                       InBuffer,
    __in PULONG                                                 InBufferSize,
    __out_bcount_part_opt(*OutBufferSize, *OutBufferSize) PUCHAR OutBuffer,
    __inout PULONG                                              OutBufferSize
);


//
// Set handlers
//

NDIS_STATUS
MbbNdisSetInterruptModeration(
    __in PMBB_REQUEST_CONTEXT               Request,
    __in_bcount_opt(*InBufferSize) PUCHAR   InBuffer,
    __in PULONG                             InBufferSize
    );

NDIS_STATUS
MbbNdisSetCurrentLookahead(
    __in PMBB_REQUEST_CONTEXT               Request,
    __in_bcount_opt(*InBufferSize) PUCHAR   InBuffer,
    __in PULONG                             InBufferSize
    );

NDIS_STATUS
MbbNdisSetRadioState(
    __in PMBB_REQUEST_CONTEXT               Request,
    __in_bcount_opt(*InBufferSize) PUCHAR   InBuffer,
    __in PULONG                             InBufferSize
    );

NDIS_STATUS
MbbNdisSetPinInfo(
    __in PMBB_REQUEST_CONTEXT               Request,
    __in_bcount_opt(*InBufferSize) PUCHAR   InBuffer,
    __in PULONG                             InBufferSize
    );

NDIS_STATUS
MbbNdisSetPinInfoEx2(
    __in PMBB_REQUEST_CONTEXT               Request,
    __in_bcount_opt(*InBufferSize) PUCHAR   InBuffer,
    __in PULONG                             InBufferSize
    );

NDIS_STATUS
MbbNdisSetPreferredProviders(
    __in PMBB_REQUEST_CONTEXT               Request,
    __in_bcount_opt(*InBufferSize) PUCHAR   InBuffer,
    __in PULONG                             InBufferSize
    );

NDIS_STATUS
MbbNdisSetRegisterState(
    __in PMBB_REQUEST_CONTEXT               Request,
    __in_bcount_opt(*InBufferSize) PUCHAR   InBuffer,
    __in PULONG                             InBufferSize
    );

NDIS_STATUS
MbbNdisSetPacketService(
    __in PMBB_REQUEST_CONTEXT               Request,
    __in_bcount_opt(*InBufferSize) PUCHAR   InBuffer,
    __in PULONG                             InBufferSize
    );

NDIS_STATUS
MbbNdisSetPower(
    __in PMBB_REQUEST_CONTEXT               Request,
    __in_bcount_opt(*InBufferSize) PUCHAR   InBuffer,
    __in PULONG                             InBufferSize
    );

NDIS_STATUS
MbbNdisSetPmParameters(
    __in PMBB_REQUEST_CONTEXT               Request,
    __in_bcount_opt(*InBufferSize) PUCHAR   InBuffer,
    __in PULONG                             InBufferSize
    );

NDIS_STATUS
MbbNdisSetAddWolPattern(
    __in PMBB_REQUEST_CONTEXT               Request,
    __in_bcount_opt(*InBufferSize) PUCHAR   InBuffer,
    __in PULONG                             InBufferSize
    );

NDIS_STATUS
MbbNdisSetRemoveAddWolPattern(
    __in PMBB_REQUEST_CONTEXT               Request,
    __in_bcount_opt(*InBufferSize) PUCHAR   InBuffer,
    __in PULONG                             InBufferSize
    );


NDIS_STATUS
MbbNdisSetSignalState(
    __in PMBB_REQUEST_CONTEXT               Request,
    __in_bcount_opt(*InBufferSize) PUCHAR   InBuffer,
    __in PULONG                             InBufferSize
    );


NDIS_STATUS
MbbNdisSetCurrentLookahead(
    __in PMBB_REQUEST_CONTEXT               Request,
    __in_bcount_opt(*InBufferSize) PUCHAR   InBuffer,
    __in PULONG                             InBufferSize
    );

NDIS_STATUS
MbbNdisSetConnectState(
    __in PMBB_REQUEST_CONTEXT               Request,
    __in_bcount_opt(*InBufferSize) PUCHAR   InBuffer,
    __in PULONG                             InBufferSize
    );

NDIS_STATUS
MbbNdisSetProvisionedContexts(
    __in PMBB_REQUEST_CONTEXT               Request,
    __in_bcount_opt(*InBufferSize) PUCHAR   InBuffer,
    __in PULONG                             InBufferSize
    );

NDIS_STATUS
MbbNdisSetProvisionedContextsV2(
    __in PMBB_REQUEST_CONTEXT               Request,
    __in_bcount_opt(*InBufferSize) PUCHAR   InBuffer,
    __in PULONG                             InBufferSize
    );

NDIS_STATUS
MbbNdisSetServiceActivation(
    __in PMBB_REQUEST_CONTEXT               Request,
    __in_bcount_opt(*InBufferSize) PUCHAR   InBuffer,
    __in PULONG                             InBufferSize
    );

NDIS_STATUS
MbbNdisSetHomeProvider(
    __in PMBB_REQUEST_CONTEXT               Request,
    __in_bcount_opt(*InBufferSize) PUCHAR   InBuffer,
    __in PULONG                             InBufferSize
    );


NDIS_STATUS
MbbNdisSetMulticarrierProviders(
    __in PMBB_REQUEST_CONTEXT               Request,
    __in_bcount_opt(*InBufferSize) PUCHAR   InBuffer,
    __in PULONG                             InBufferSize
    );

// SMS

NDIS_STATUS
MbbNdisSmsSetConfiguration(
    __in PMBB_REQUEST_CONTEXT               Request,
    __in_bcount_opt(*InBufferSize) PUCHAR   InBuffer,
    __in PULONG                             InBufferSize
    );

NDIS_STATUS
MbbNdisSmsSend(
    __in PMBB_REQUEST_CONTEXT               Request,
    __in_bcount_opt(*InBufferSize) PUCHAR   InBuffer,
    __in PULONG                             InBufferSize
    );

NDIS_STATUS
MbbNdisSmsDelete(
    __in PMBB_REQUEST_CONTEXT               Request,
    __in_bcount_opt(*InBufferSize) PUCHAR   InBuffer,
    __in PULONG                             InBufferSize
    );

// USSD

NDIS_STATUS
MbbNdisSetUSSD(
    __in PMBB_REQUEST_CONTEXT               Request,
    __in_bcount_opt(*InBufferSize) PUCHAR   InBuffer,
    __in PULONG                             InBufferSize

);

// DEVICE SERVICE

NDIS_STATUS
MbbNdisSetDeviceServiceCommand(
    __in PMBB_REQUEST_CONTEXT               Request,
    __in_bcount_opt(*InBufferSize) PUCHAR   InBuffer,
    __in PULONG                             InBufferSize
    );

NDIS_STATUS
MbbNdisSubscribeDeviceServiceEvents(
    __in PMBB_REQUEST_CONTEXT               Request,
    __in_bcount_opt(*InBufferSize) PUCHAR   InBuffer,
    __in PULONG                             InBufferSize
    );

NDIS_STATUS
MbbNdisSetDeviceServiceSession(
    __in PMBB_REQUEST_CONTEXT               Request,
    __in_bcount_opt(*InBufferSize) PUCHAR   InBuffer,
    __in PULONG                             InBufferSize
    );

NDIS_STATUS
MbbNdisDssWrite(
    __in PMBB_REQUEST_CONTEXT                                   Request,
    __in_bcount_opt(*InBufferSize) PUCHAR                       InBuffer,
    __in PULONG                                                 InBufferSize
    );

// MAC ADDRESS

NDIS_STATUS
MbbNdisDeleteMac(
    __in PMBB_REQUEST_CONTEXT                                   Request,
    __in_bcount_opt(*InBufferSize) PUCHAR                       InBuffer,
    __in PULONG                                                 InBufferSize
    );

// UICC

NDIS_STATUS
MbbNdisUiccUpdateBinary(
    __in PMBB_REQUEST_CONTEXT               Request,
    __in_bcount_opt(*InBufferSize) PUCHAR   InBuffer,
    __in PULONG                             InBufferSize
    );

NDIS_STATUS
MbbNdisUiccUpdateRecord(
    __in PMBB_REQUEST_CONTEXT               Request,
    __in_bcount_opt(*InBufferSize) PUCHAR   InBuffer,
    __in PULONG                             InBufferSize
    );


// Device State and Configuration for MBIM 2.0

NDIS_STATUS
MbbNdisSetSysSlotMappings(
    __in PMBB_REQUEST_CONTEXT               Request,
    __in_bcount_opt(*InBufferSize) PUCHAR   InBuffer,
    __in PULONG                             InBufferSize
);

NDIS_STATUS
MbbNdisSetDeviceBindings(
    __in PMBB_REQUEST_CONTEXT               Request,
    __in_bcount_opt(*InBufferSize) PUCHAR   InBuffer,
    __in PULONG                             InBufferSize
);

// Registration, Signal Strength, and NITZ for MBIM 2.0

NDIS_STATUS
MbbNdisSetRegisterStateEx(
    __in PMBB_REQUEST_CONTEXT               Request,
    __in_bcount_opt(*InBufferSize) PUCHAR   InBuffer,
    __in PULONG                             InBufferSize
);

NDIS_STATUS
MbbNdisSetImsVoiceState(
    __in PMBB_REQUEST_CONTEXT               Request,
    __in_bcount_opt(*InBufferSize) PUCHAR   InBuffer,
    __in PULONG                             InBufferSize
);

NDIS_STATUS
MbbNdisSetSignalStateEx(
    __in PMBB_REQUEST_CONTEXT               Request,
    __in_bcount_opt(*InBufferSize) PUCHAR   InBuffer,
    __in PULONG                             InBufferSize
);

NDIS_STATUS
MbbNdisSetLocationState(
    __in PMBB_REQUEST_CONTEXT               Request,
    __in_bcount_opt(*InBufferSize) PUCHAR   InBuffer,
    __in PULONG                             InBufferSize
);

// Vendor Specific

NDIS_STATUS
MbbNdisSetVendorSpecific(
    __in PMBB_REQUEST_CONTEXT               Request,
    __in_bcount_opt(*InBufferSize) PUCHAR   InBuffer,
    __in PULONG                             InBufferSize
    );

// Network Idle Hint
NDIS_STATUS
MbbNdisSetNetworkIdleHint(
    __in PMBB_REQUEST_CONTEXT                                   Request,
    __in_bcount_opt(*InBufferSize) PUCHAR                       InBuffer,
    __in PULONG                                                 InBufferSize
    );

// System Preshutdown
NDIS_STATUS
MbbNdisSetPreshutdown(
    __in PMBB_REQUEST_CONTEXT                                   Request,
    __in_bcount_opt(*InBufferSize) PUCHAR                       InBuffer,
    __in PULONG                                                 InBufferSize
    );

// Low-Level UICC Access

NDIS_STATUS
MbbNdisSetUiccOpenChannel(
    __in PMBB_REQUEST_CONTEXT               Request,
    __in_bcount_opt(*InBufferSize) PUCHAR   InBuffer,
    __in PULONG                             InBufferSize
    );

NDIS_STATUS
MbbNdisSetUiccCloseChannel(
    __in PMBB_REQUEST_CONTEXT               Request,
    __in_bcount_opt(*InBufferSize) PUCHAR   InBuffer,
    __in PULONG                             InBufferSize
    );

NDIS_STATUS
MbbNdisSetUiccApdu(
    __in PMBB_REQUEST_CONTEXT               Request,
    __in_bcount_opt(*InBufferSize) PUCHAR   InBuffer,
    __in PULONG                             InBufferSize
    );

NDIS_STATUS
MbbNdisSetUiccTerminalCapability(
    __in PMBB_REQUEST_CONTEXT               Request,
    __in_bcount_opt(*InBufferSize) PUCHAR   InBuffer,
    __in PULONG                             InBufferSize
    );

NDIS_STATUS
MbbNdisSetUiccReset(
    _In_ PMBB_REQUEST_CONTEXT                   Request,
    _In_reads_bytes_opt_(*InBufferSize) PUCHAR  InBuffer,
    _In_ PULONG                                 InBufferSize
    );

NDIS_STATUS
MbbNdisSetSarConfig(
    _In_ PMBB_REQUEST_CONTEXT                    Request,
    _In_reads_bytes_opt_(*InBufferSize) PUCHAR   InBuffer,
    _In_ PULONG                                  InBufferSize
    );

NDIS_STATUS
MbbNdisSetSarTransmissionStatus(
    _In_ PMBB_REQUEST_CONTEXT                    Request,
    _In_reads_bytes_opt_(*InBufferSize) PUCHAR   InBuffer,
    _In_ PULONG                                  InBufferSize
    );

NDIS_STATUS
MbbNdisSetLteAttachContexts(
    __in PMBB_REQUEST_CONTEXT               Request,
    __in_bcount_opt(*InBufferSize) PUCHAR   InBuffer,
    __in PULONG                             InBufferSize
    );

NDIS_STATUS
MbbNdisSetNetworkBlacklist(
    __in PMBB_REQUEST_CONTEXT               Request,
    __in_bcount_opt(*InBufferSize) PUCHAR   InBuffer,
    __in PULONG                             InBufferSize
    );

NDIS_STATUS
MbbNdisSetDeviceReset(
    _In_ PMBB_REQUEST_CONTEXT                   Request,
    _In_reads_bytes_opt_(*InBufferSize) PUCHAR  InBuffer,
    _In_ PULONG                                 InBufferSize
    );

//
// Status handlers
//

NDIS_STATUS
MbbNdisDeviceCapsStatusHandler(
    __in PMBB_REQUEST_CONTEXT               Request,
    __in NDIS_STATUS                        NdisStatus,
    __in MBB_STATUS                         MbbStatus,
    __in_bcount_opt(InBufferSize) PUCHAR    InBuffer,
    __in ULONG                              InBufferSize
    );

NDIS_STATUS
MbbNdisReadyInfoStatusHandler(
    __in PMBB_REQUEST_CONTEXT               Request,
    __in NDIS_STATUS                        NdisStatus,
    __in MBB_STATUS                         MbbStatus,
    __in_bcount_opt(InBufferSize) PUCHAR    InBuffer,
    __in ULONG                              InBufferSize
    );

NDIS_STATUS
MbbNdisEmergencyModeStatusHandler(
    __in PMBB_REQUEST_CONTEXT               Request,
    __in NDIS_STATUS                        NdisStatus,
    __in MBB_STATUS                         MbbStatus,
    __in_bcount_opt(InBufferSize) PUCHAR    InBuffer,
    __in ULONG                              InBufferSize
    );

NDIS_STATUS
MbbNdisRadioStateStatusHandler(
    __in PMBB_REQUEST_CONTEXT               Request,
    __in NDIS_STATUS                        NdisStatus,
    __in MBB_STATUS                         MbbStatus,
    __in_bcount_opt(InBufferSize) PUCHAR    InBuffer,
    __in ULONG                              InBufferSize
    );

NDIS_STATUS
MbbNdisPinInfoStatusHandler(
    __in PMBB_REQUEST_CONTEXT               Request,
    __in NDIS_STATUS                        NdisStatus,
    __in MBB_STATUS                         MbbStatus,
    __in_bcount_opt(InBufferSize) PUCHAR    InBuffer,
    __in ULONG                              InBufferSize
    );

NDIS_STATUS
MbbNdisPinListStatusHandler(
    __in PMBB_REQUEST_CONTEXT               Request,
    __in NDIS_STATUS                        NdisStatus,
    __in MBB_STATUS                         MbbStatus,
    __in_bcount_opt(InBufferSize) PUCHAR    InBuffer,
    __in ULONG                              InBufferSize
    );

NDIS_STATUS
MbbNdisHomeProviderStatusHandler(
    __in PMBB_REQUEST_CONTEXT               Request,
    __in NDIS_STATUS                        NdisStatus,
    __in MBB_STATUS                         MbbStatus,
    __in_bcount_opt(InBufferSize) PUCHAR    InBuffer,
    __in ULONG                              InBufferSize
    );

NDIS_STATUS
MbbNdisPreferredProviderStatusHandler(
    __in PMBB_REQUEST_CONTEXT               Request,
    __in NDIS_STATUS                        NdisStatus,
    __in MBB_STATUS                         MbbStatus,
    __in_bcount_opt(InBufferSize) PUCHAR    InBuffer,
    __in ULONG                              InBufferSize
    );

NDIS_STATUS
MbbNdisVisibleProviderStatusHandler(
    __in PMBB_REQUEST_CONTEXT               Request,
    __in NDIS_STATUS                        NdisStatus,
    __in MBB_STATUS                         MbbStatus,
    __in_bcount_opt(InBufferSize) PUCHAR    InBuffer,
    __in ULONG                              InBufferSize
    );

NDIS_STATUS
MbbNdisRegisterStateStatusHandler(
    __in PMBB_REQUEST_CONTEXT               Request,
    __in NDIS_STATUS                        NdisStatus,
    __in MBB_STATUS                         MbbStatus,
    __in_bcount_opt(InBufferSize) PUCHAR    InBuffer,
    __in ULONG                              InBufferSize
    );

VOID
MbbNdisIndicatePacketService(
    __in PMBB_REQUEST_CONTEXT   Request,
    __in NDIS_STATUS            NdisStatus
    );


NDIS_STATUS
MbbNdisPacketServiceStatusHandler(
    __in PMBB_REQUEST_CONTEXT               Request,
    __in NDIS_STATUS                        NdisStatus,
    __in MBB_STATUS                         MbbStatus,
    __in_bcount_opt(InBufferSize) PUCHAR    InBuffer,
    __in ULONG                              InBufferSize
    );

NDIS_STATUS
MbbNdisSignalStateStatusHandler(
    __in PMBB_REQUEST_CONTEXT               Request,
    __in NDIS_STATUS                        NdisStatus,
    __in MBB_STATUS                         MbbStatus,
    __in_bcount_opt(InBufferSize) PUCHAR    InBuffer,
    __in ULONG                              InBufferSize
    );

NDIS_STATUS
MbbNdisConnectStatusHandler(
    __in PMBB_REQUEST_CONTEXT               Request,
    __in NDIS_STATUS                        NdisStatus,
    __in MBB_STATUS                         MbbStatus,
    __in_bcount_opt(InBufferSize) PUCHAR    InBuffer,
    __in ULONG                              InBufferSize
    );

NDIS_STATUS
MbbNdisProvisionedContextStatusHandler(
    __in PMBB_REQUEST_CONTEXT               Request,
    __in NDIS_STATUS                        NdisStatus,
    __in MBB_STATUS                         MbbStatus,
    __in_bcount_opt(InBufferSize) PUCHAR    InBuffer,
    __in ULONG                              InBufferSize
    );

NDIS_STATUS
MbbNdisProvisionedContextV2StatusHandler(
    __in PMBB_REQUEST_CONTEXT               Request,
    __in NDIS_STATUS                        NdisStatus,
    __in MBB_STATUS                         MbbStatus,
    __in_bcount_opt(InBufferSize) PUCHAR    InBuffer,
    __in ULONG                              InBufferSize
    );

NDIS_STATUS
MbbNdisServiceActivationStatusHandler(
    __in PMBB_REQUEST_CONTEXT               Request,
    __in NDIS_STATUS                        NdisStatus,
    __in MBB_STATUS                         MbbStatus,
    __in_bcount_opt(InBufferSize) PUCHAR    InBuffer,
    __in ULONG                              InBufferSize
    );

NDIS_STATUS
MbbNdisMulticarrierProviderStatusHandler(
    __in PMBB_REQUEST_CONTEXT               Request,
    __in NDIS_STATUS                        NdisStatus,
    __in MBB_STATUS                         MbbStatus,
    __in_bcount_opt(InBufferSize) PUCHAR    InBuffer,
    __in ULONG                              InBufferSize
    );

// SMS

NDIS_STATUS
MbbNdisSmsConfigurationStatusHandler(
    __in PMBB_REQUEST_CONTEXT               Request,
    __in NDIS_STATUS                        NdisStatus,
    __in MBB_STATUS                         MbbStatus,
    __in_bcount_opt(InBufferSize) PUCHAR    InBuffer,
    __in ULONG                              InBufferSize
    );

NDIS_STATUS
MbbNdisSmsReadStatusHandler(
    __in PMBB_REQUEST_CONTEXT               Request,
    __in NDIS_STATUS                        NdisStatus,
    __in MBB_STATUS                         MbbStatus,
    __in_bcount_opt(InBufferSize) PUCHAR    InBuffer,
    __in ULONG                              InBufferSize
    );

NDIS_STATUS
MbbNdisSmsSendStatusHandler(
    __in PMBB_REQUEST_CONTEXT               Request,
    __in NDIS_STATUS                        NdisStatus,
    __in MBB_STATUS                         MbbStatus,
    __in_bcount_opt(InBufferSize) PUCHAR    InBuffer,
    __in ULONG                              InBufferSize
    );

NDIS_STATUS
MbbNdisSmsDeleteStatusHandler(
    __in PMBB_REQUEST_CONTEXT               Request,
    __in NDIS_STATUS                        NdisStatus,
    __in MBB_STATUS                         MbbStatus,
    __in_bcount_opt(InBufferSize) PUCHAR    InBuffer,
    __in ULONG                              InBufferSize
    );

NDIS_STATUS
MbbNdisSmsStatusHandler(
    __in PMBB_REQUEST_CONTEXT               Request,
    __in NDIS_STATUS                        NdisStatus,
    __in MBB_STATUS                         MbbStatus,
    __in_bcount_opt(InBufferSize) PUCHAR    InBuffer,
    __in ULONG                              InBufferSize
    );

NDIS_STATUS
MbbNdisGetStatsHandler(
    __in PMBB_REQUEST_CONTEXT               Request,
    __in NDIS_STATUS                        NdisStatus,
    __in MBB_STATUS                         MbbStatus,
    __in_bcount_opt(InBufferSize) PUCHAR    InBuffer,
    __in ULONG                              InBufferSize
    );

// DEVICE SERVICE

NDIS_STATUS
MbbNdisDeviceServiceListStatusHandler(
    __in PMBB_REQUEST_CONTEXT               Request,
    __in NDIS_STATUS                        NdisStatus,
    __in MBB_STATUS                         MbbStatus,
    __in_bcount_opt(InBufferSize) PUCHAR    InBuffer,
    __in ULONG                              InBufferSize
    );

NDIS_STATUS
MbbNdisDeviceServiceStatusHandler(
    __in PMBB_REQUEST_CONTEXT               Request,
    __in NDIS_STATUS                        NdisStatus,
    __in MBB_STATUS                         MbbStatus,
    __in_bcount_opt(InBufferSize) PUCHAR    InBuffer,
    __in ULONG                              InBufferSize
    );

NDIS_STATUS
MbbNdisSubscribeDeviceServiceEventsStatusHandler(
    __in PMBB_REQUEST_CONTEXT               Request,
    __in NDIS_STATUS                        NdisStatus,
    __in MBB_STATUS                         MbbStatus,
    __in_bcount_opt(InBufferSize) PUCHAR    InBuffer,
    __in ULONG                              InBufferSize
    );


// USSD

NDIS_STATUS
MbbNdisUSSDStatusHandler(
    __in PMBB_REQUEST_CONTEXT               Request,
    __in NDIS_STATUS                        NdisStatus,
    __in MBB_STATUS                         MbbStatus,
    __in_bcount_opt(InBufferSize) PUCHAR    InBuffer,
    __in ULONG                              InBufferSize
    );


// EAP Authentication

NDIS_STATUS
MbbNdisAuthAKAStatusHandler(
    __in PMBB_REQUEST_CONTEXT               Request,
    __in NDIS_STATUS                        NdisStatus,
    __in MBB_STATUS                         MbbStatus,
    __in_bcount_opt(InBufferSize) PUCHAR    InBuffer,
    __in ULONG                              InBufferSize
    );

NDIS_STATUS
MbbNdisAuthAKAPStatusHandler(
    __in PMBB_REQUEST_CONTEXT               Request,
    __in NDIS_STATUS                        NdisStatus,
    __in MBB_STATUS                         MbbStatus,
    __in_bcount_opt(InBufferSize) PUCHAR    InBuffer,
    __in ULONG                              InBufferSize
    );


NDIS_STATUS
MbbNdisAuthSIMStatusHandler(
    __in PMBB_REQUEST_CONTEXT               Request,
    __in NDIS_STATUS                        NdisStatus,
    __in MBB_STATUS                         MbbStatus,
    __in_bcount_opt(InBufferSize) PUCHAR    InBuffer,
    __in ULONG                              InBufferSize
    );


NDIS_STATUS
MbbNdisDssOpenStatusHandler(
    __in PMBB_REQUEST_CONTEXT               Request,
    __in NDIS_STATUS                        NdisStatus,
    __in MBB_STATUS                         MbbStatus,
    __in_bcount_opt(InBufferSize) PUCHAR    InBuffer,
    __in ULONG                              InBufferSize
    );


NDIS_STATUS
MbbNdisDssCloseStatusHandler(
    __in PMBB_REQUEST_CONTEXT               Request,
    __in NDIS_STATUS                        NdisStatus,
    __in MBB_STATUS                         MbbStatus,
    __in_bcount_opt(InBufferSize) PUCHAR    InBuffer,
    __in ULONG                              InBufferSize
    );

NDIS_STATUS
MbbNdisDssWriteCompleteStatusHandler(
    __in PMBB_REQUEST_CONTEXT               Request,
    __in NDIS_STATUS                        NdisStatus,
    __in MBB_STATUS                         MbbStatus,
    __in_bcount_opt(InBufferSize) PUCHAR    InBuffer,
    __in ULONG                              InBufferSize
    );

// Multi-carrier

NDIS_STATUS
MbbNdisMultiCarrierDsCidListStatusHandler(
    __in PMBB_REQUEST_CONTEXT               Request,
    __in NDIS_STATUS                        NdisStatus,
    __in MBB_STATUS                         MbbStatus,
    __in_bcount_opt(InBufferSize) PUCHAR    InBuffer,
    __in ULONG                              InBufferSize
    );

// UICC

NDIS_STATUS
MbbNdisUiccAppListStatusHandler(
    __in PMBB_REQUEST_CONTEXT               Request,
    __in NDIS_STATUS                        NdisStatus,
    __in MBB_STATUS                         MbbStatus,
    __in_bcount_opt(InBufferSize) PUCHAR    InBuffer,
    __in ULONG                              InBufferSize
);

NDIS_STATUS
MbbNdisUiccFileStatusHandler(
    __in PMBB_REQUEST_CONTEXT               Request,
    __in NDIS_STATUS                        NdisStatus,
    __in MBB_STATUS                         MbbStatus,
    __in_bcount_opt(InBufferSize) PUCHAR    InBuffer,
    __in ULONG                              InBufferSize
    );

NDIS_STATUS
MbbNdisUiccAccessStatusHandler(
    __in PMBB_REQUEST_CONTEXT               Request,
    __in NDIS_STATUS                        NdisStatus,
    __in MBB_STATUS                         MbbStatus,
    __in_bcount_opt(InBufferSize) PUCHAR    InBuffer,
    __in ULONG                              InBufferSize
    );

// Device State and Configuration for MBIM 2.0

NDIS_STATUS
MbbNdisSysCapsStatusHandler(
    __in PMBB_REQUEST_CONTEXT               Request,
    __in NDIS_STATUS                        NdisStatus,
    __in MBB_STATUS                         MbbStatus,
    __in_bcount_opt(InBufferSize) PUCHAR    InBuffer,
    __in ULONG                              InBufferSize
);

NDIS_STATUS
MbbNdisDeviceCapsExStatusHandler(
    __in PMBB_REQUEST_CONTEXT               Request,
    __in NDIS_STATUS                        NdisStatus,
    __in MBB_STATUS                         MbbStatus,
    __in_bcount_opt(InBufferSize) PUCHAR    InBuffer,
    __in ULONG                              InBufferSize
);

NDIS_STATUS
MbbNdisSysSlotMappingsStatusHandler(
    __in PMBB_REQUEST_CONTEXT               Request,
    __in NDIS_STATUS                        NdisStatus,
    __in MBB_STATUS                         MbbStatus,
    __in_bcount_opt(InBufferSize) PUCHAR    InBuffer,
    __in ULONG                              InBufferSize
);

NDIS_STATUS
MbbNdisSlotInfoStatusHandler(
    __in PMBB_REQUEST_CONTEXT               Request,
    __in NDIS_STATUS                        NdisStatus,
    __in MBB_STATUS                         MbbStatus,
    __in_bcount_opt(InBufferSize) PUCHAR    InBuffer,
    __in ULONG                              InBufferSize
);

NDIS_STATUS
MbbNdisDeviceBindingsStatusHandler(
    __in PMBB_REQUEST_CONTEXT               Request,
    __in NDIS_STATUS                        NdisStatus,
    __in MBB_STATUS                         MbbStatus,
    __in_bcount_opt(InBufferSize) PUCHAR    InBuffer,
    __in ULONG                              InBufferSize
);

// Registration, Signal Strength, and NITZ for MBIM 2.0

NDIS_STATUS
MbbNdisRegisterStateExStatusHandler(
    __in PMBB_REQUEST_CONTEXT               Request,
    __in NDIS_STATUS                        NdisStatus,
    __in MBB_STATUS                         MbbStatus,
    __in_bcount_opt(InBufferSize) PUCHAR    InBuffer,
    __in ULONG                              InBufferSize
);

NDIS_STATUS
MbbNdisImsVoiceStateStatusHandler(
    __in PMBB_REQUEST_CONTEXT               Request,
    __in NDIS_STATUS                        NdisStatus,
    __in MBB_STATUS                         MbbStatus,
    __in_bcount_opt(InBufferSize) PUCHAR    InBuffer,
    __in ULONG                              InBufferSize
);

NDIS_STATUS
MbbNdisSignalStateExStatusHandler(
    __in PMBB_REQUEST_CONTEXT               Request,
    __in NDIS_STATUS                        NdisStatus,
    __in MBB_STATUS                         MbbStatus,
    __in_bcount_opt(InBufferSize) PUCHAR    InBuffer,
    __in ULONG                              InBufferSize
);

NDIS_STATUS
MbbNdisLocationStateStatusHandler(
    __in PMBB_REQUEST_CONTEXT               Request,
    __in NDIS_STATUS                        NdisStatus,
    __in MBB_STATUS                         MbbStatus,
    __in_bcount_opt(InBufferSize) PUCHAR    InBuffer,
    __in ULONG                              InBufferSize
);

NDIS_STATUS
MbbNdisNitzStatusHandler(
    __in PMBB_REQUEST_CONTEXT               Request,
    __in NDIS_STATUS                        NdisStatus,
    __in MBB_STATUS                         MbbStatus,
    __in_bcount_opt(InBufferSize) PUCHAR    InBuffer,
    __in ULONG                              InBufferSize
);

// Vendor Specific

NDIS_STATUS
MbbNdisVendorSpecificStatusHandler(
__in PMBB_REQUEST_CONTEXT               Request,
__in NDIS_STATUS                        NdisStatus,
__in MBB_STATUS                         MbbStatus,
__in_bcount_opt(InBufferSize) PUCHAR    InBuffer,
__in ULONG                              InBufferSize
);

// System Preshutdown

NDIS_STATUS
MbbNdisPreshutdownHandler(
    __in PMBB_REQUEST_CONTEXT               Request,
    __in NDIS_STATUS                        NdisStatus,
    __in MBB_STATUS                         MbbStatus,
    __in_bcount_opt(InBufferSize) PUCHAR    InBuffer,
    __in ULONG                              InBufferSize
    );

// Low-Level UICC Access

NDIS_STATUS
MbbNdisAtrInfoStatusHandler(
    __in PMBB_REQUEST_CONTEXT               Request,
    __in NDIS_STATUS                        NdisStatus,
    __in MBB_STATUS                         MbbStatus,
    __in_bcount_opt(InBufferSize) PUCHAR    InBuffer,
    __in ULONG                              InBufferSize
    );

NDIS_STATUS
MbbNdisUiccOpenChannelInfoStatusHandler(
    __in PMBB_REQUEST_CONTEXT               Request,
    __in NDIS_STATUS                        NdisStatus,
    __in MBB_STATUS                         MbbStatus,
    __in_bcount_opt(InBufferSize) PUCHAR    InBuffer,
    __in ULONG                              InBufferSize
    );

NDIS_STATUS
MbbNdisUiccCloseChannelInfoStatusHandler(
    __in PMBB_REQUEST_CONTEXT               Request,
    __in NDIS_STATUS                        NdisStatus,
    __in MBB_STATUS                         MbbStatus,
    __in_bcount_opt(InBufferSize) PUCHAR    InBuffer,
    __in ULONG                              InBufferSize
    );

NDIS_STATUS
MbbNdisUiccApduInfoStatusHandler(
    __in PMBB_REQUEST_CONTEXT               Request,
    __in NDIS_STATUS                        NdisStatus,
    __in MBB_STATUS                         MbbStatus,
    __in_bcount_opt(InBufferSize) PUCHAR    InBuffer,
    __in ULONG                              InBufferSize
    );

NDIS_STATUS
MbbNdisUiccTerminalCapabilityInfoStatusHandler(
    __in PMBB_REQUEST_CONTEXT               Request,
    __in NDIS_STATUS                        NdisStatus,
    __in MBB_STATUS                         MbbStatus,
    __in_bcount_opt(InBufferSize) PUCHAR    InBuffer,
    __in ULONG                              InBufferSize
    );

NDIS_STATUS
MbbNdisUiccResetInfoStatusHandler(
    _In_ PMBB_REQUEST_CONTEXT                   Request,
    _In_ NDIS_STATUS                            NdisStatus,
    _In_ MBB_STATUS                             MbbStatus,
    _In_reads_bytes_opt_(InBufferSize) PUCHAR   InBuffer,
    _In_ ULONG                                  InBufferSize
    );

NDIS_STATUS
MbbNdisSarConfigHandler(
    _In_ PMBB_REQUEST_CONTEXT                  Request,
    _In_ NDIS_STATUS                           NdisStatus,
    _In_ MBB_STATUS                            MbbStatus,
    _In_reads_bytes_opt_(InBufferSize) PUCHAR  InBuffer,
    _In_ ULONG                                 InBufferSize
    );

NDIS_STATUS
MbbNdisSarTransmissionStatusHandler(
    _In_ PMBB_REQUEST_CONTEXT                  Request,
    _In_ NDIS_STATUS                           NdisStatus,
    _In_ MBB_STATUS                            MbbStatus,
    _In_reads_bytes_opt_(InBufferSize) PUCHAR  InBuffer,
    _In_ ULONG                                 InBufferSize
    );

NDIS_STATUS
MbbNdisLteAttachConfigHandler(
    __in PMBB_REQUEST_CONTEXT               Request,
    __in NDIS_STATUS                        NdisStatus,
    __in MBB_STATUS                         MbbStatus,
    __in_bcount_opt(InBufferSize) PUCHAR    InBuffer,
    __in ULONG                              InBufferSize
    );

NDIS_STATUS
MbbNdisLteAttachStatusHandler(
    __in PMBB_REQUEST_CONTEXT               Request,
    __in NDIS_STATUS                        NdisStatus,
    __in MBB_STATUS                         MbbStatus,
    __in_bcount_opt(InBufferSize) PUCHAR    InBuffer,
    __in ULONG                              InBufferSize
    );

NDIS_STATUS
MbbNdisNetworkBlacklistHandler(
    __in PMBB_REQUEST_CONTEXT               Request,
    __in NDIS_STATUS                        NdisStatus,
    __in MBB_STATUS                         MbbStatus,
    __in_bcount_opt(InBufferSize) PUCHAR    InBuffer,
    __in ULONG                              InBufferSize
    );

NDIS_STATUS
MbbNdisPcoStatusHandler(
    _In_ PMBB_REQUEST_CONTEXT                  Request,
    _In_ NDIS_STATUS                           NdisStatus,
    _In_ MBB_STATUS                            MbbStatus,
    _In_reads_bytes_opt_(InBufferSize) PUCHAR  InBuffer,
    _In_ ULONG                                 InBufferSize
);

NDIS_STATUS
MbbNdisDeviceResetStatusHandler(
    _In_ PMBB_REQUEST_CONTEXT                   Request,
    _In_ NDIS_STATUS                            NdisStatus,
    _In_ MBB_STATUS                             MbbStatus,
    _In_reads_bytes_opt_(InBufferSize) PUCHAR   InBuffer,
    _In_ ULONG                                  InBufferSize
    );

// Base station information

NDIS_STATUS
MbbNdisBaseStationsInfoHandler(
    _In_ PMBB_REQUEST_CONTEXT                  Request,
    _In_ NDIS_STATUS                           NdisStatus,
    _In_ MBB_STATUS                            MbbStatus,
    _In_reads_bytes_opt_(InBufferSize) PUCHAR  InBuffer,
    _In_ ULONG                                 InBufferSize
);


//
// Completion Handlers
//

VOID
MbbNdisReadyInfoCompletionHandler(
    __in PMBB_REQUEST_CONTEXT               Request,
    __in NDIS_STATUS                        NdisStatus
    );

VOID
MbbNdisPacketServiceStateCompletionHandler(
    __in PMBB_REQUEST_CONTEXT               Request,
    __in NDIS_STATUS                        NdisStatus
    );

VOID
MbbNdisMultiCarrierDsCidListCompletionHandler(
    __in PMBB_REQUEST_CONTEXT               Request,
    __in NDIS_STATUS                        NdisStatus
    );
VOID
MbbNdisCreateMacCompletionHandler(
    __in PMBB_REQUEST_CONTEXT               Request,
    __in NDIS_STATUS                        NdisStatus
    );

VOID
MbbNdisDeleteMacCompletionHandler(
    __in PMBB_REQUEST_CONTEXT               Request,
    __in NDIS_STATUS                        NdisStatus
    );


////////////////////////////////////////////////////////////////////////////////
//
//  GLOBALS
//
////////////////////////////////////////////////////////////////////////////////
#define OID_INVALID (-1)

#define MBIM_CID_VENDORSPECIFIC     1


MBB_OID_HANDLER_ENTRY MbbSpecialOidHandlerTable[] =
{
    //
    // Authentication Challenges - AKA, AKAP, and SIM
    //
    {
        OID_WWAN_AUTH_CHALLENGE,                                // Oid
        FALSE,                                                  // IsSettable
        TRUE,                                                   // IsQueryable
        TRUE,                                                   // IsSerialized
        TRUE,                                                   // IsIndicationRequired
        TRUE,                                                   // CompleteRequestOnSendComplete

        NDIS_OBJECT_TYPE_DEFAULT,                               // NdisObjectHeaderType
        NDIS_WWAN_AUTH_CHALLENGE_REVISION_1,                    // NdisObjectHeaderRevision
        sizeof(NDIS_WWAN_AUTH_CHALLENGE),                       // NdisObjectHeaderSize

        NULL,                                                   // Set routine
        0,                                                      // MinSetInBufferSize

        MbbNdisGetAuthChallenge,                              // Query routine
        sizeof(NDIS_WWAN_AUTH_CHALLENGE),                       // MinQueryInBufferSize
        sizeof(NDIS_WWAN_AUTH_RESPONSE),                        // MinQueryOutBufferSize

        { MBB_UUID_AUTH_CONSTANT, MBB_AUTH_CID_AKA},            // ResponseCommand
        sizeof(MBB_USSD),                                       // ResponseBufferLength
        MbbNdisAuthAKAStatusHandler,                            // ResponseHandler
        NULL                                                    // CompletionHandler
    },
    {
        OID_WWAN_AUTH_CHALLENGE,                                // Oid
        FALSE,                                                  // IsSettable
        TRUE,                                                   // IsQueryable
        TRUE,                                                   // IsSerialized
        TRUE,                                                   // IsIndicationRequired
        TRUE,                                                   // CompleteRequestOnSendComplete

        NDIS_OBJECT_TYPE_DEFAULT,                               // NdisObjectHeaderType
        NDIS_WWAN_AUTH_CHALLENGE_REVISION_1,                    // NdisObjectHeaderRevision
        sizeof(NDIS_WWAN_AUTH_CHALLENGE),                       // NdisObjectHeaderSize

        NULL,                                                   // Set routine
        0,                                                      // MinSetInBufferSize

        MbbNdisGetAuthChallenge,                              // Query routine
        sizeof(NDIS_WWAN_AUTH_CHALLENGE),                       // MinQueryInBufferSize
        sizeof(NDIS_WWAN_AUTH_RESPONSE),                        // MinQueryOutBufferSize

        { MBB_UUID_AUTH_CONSTANT, MBB_AUTH_CID_AKAP},           // ResponseCommand
        sizeof(MBB_USSD),                                       // ResponseBufferLength
        MbbNdisAuthAKAPStatusHandler,                           // ResponseHandler
        NULL                                                    // CompletionHandler
    },
    {
        OID_WWAN_AUTH_CHALLENGE,                                // Oid
        FALSE,                                                  // IsSettable
        TRUE,                                                   // IsQueryable
        TRUE,                                                   // IsSerialized
        TRUE,                                                   // IsIndicationRequired
        TRUE,                                                   // CompleteRequestOnSendComplete

        NDIS_OBJECT_TYPE_DEFAULT,                               // NdisObjectHeaderType
        NDIS_WWAN_AUTH_CHALLENGE_REVISION_1,                    // NdisObjectHeaderRevision
        sizeof(NDIS_WWAN_AUTH_CHALLENGE),                       // NdisObjectHeaderSize

        NULL,                                                   // Set routine
        0,                                                      // MinSetInBufferSize

        MbbNdisGetAuthChallenge,                              // Query routine
        sizeof(NDIS_WWAN_AUTH_CHALLENGE),                       // MinQueryInBufferSize
        sizeof(NDIS_WWAN_AUTH_RESPONSE),                        // MinQueryOutBufferSize

        { MBB_UUID_AUTH_CONSTANT, MBB_AUTH_CID_SIM},            // ResponseCommand
        sizeof(MBB_USSD),                                       // ResponseBufferLength
        MbbNdisAuthSIMStatusHandler,                            // ResponseHandler
        NULL                                                    // CompletionHandler
    },
    //
    // Device Service Sessions - Open & Close
    //
    {
        OID_WWAN_DEVICE_SERVICE_SESSION,                        // Oid
        TRUE,                                                   // IsSettable
        FALSE,                                                  // IsQueryable
        TRUE,                                                   // IsSerialized
        TRUE,                                                   // IsIndicationRequired
        TRUE,                                                   // CompleteRequestOnSendComplete

        NDIS_OBJECT_TYPE_DEFAULT,                               // NdisObjectHeaderType
        NDIS_WWAN_SET_DEVICE_SERVICE_SESSION_REVISION_1,        // NdisObjectHeaderRevision
        SIZEOF_NDIS_WWAN_SET_DEVICE_SERVICE_SESSION_1,          // NdisObjectHeaderSize

        MbbNdisSetDeviceServiceSession,                         // Set routine
        SIZEOF_NDIS_WWAN_SET_DEVICE_SERVICE_SESSION_1,          // MinSetInBufferSize

        NULL,                                                   // Query routine
        0,                                                      // MinQueryInBufferSize
        0,                                                      // MinQueryOutBufferSize

        { MBB_UUID_DSS_CONSTANT, MBB_DSS_CID_CONNECT},          // ResponseCommand
        0,                                                      // ResponseBufferLength
        MbbNdisDssOpenStatusHandler,                            // ResponseHandler
        NULL                                                    // CompletionHandler
    },
#if 0
    {
        OID_WWAN_DEVICE_SERVICE_SESSION,                        // Oid
        TRUE,                                                   // IsSettable
        FALSE,                                                  // IsQueryable
        TRUE,                                                   // IsSerialized
        TRUE,                                                   // IsIndicationRequired
        TRUE,                                                   // CompleteRequestOnSendComplete

        NDIS_OBJECT_TYPE_DEFAULT,                               // NdisObjectHeaderType
        NDIS_WWAN_SET_DEVICE_SERVICE_SESSION_REVISION_1,        // NdisObjectHeaderRevision
        SIZEOF_NDIS_WWAN_SET_DEVICE_SERVICE_SESSION_1,          // NdisObjectHeaderSize

        MbbNdisSetDeviceServiceSession,                         // Set routine
        SIZEOF_NDIS_WWAN_SET_DEVICE_SERVICE_SESSION_1,          // MinSetInBufferSize

        NULL,                                                   // Query routine
        0,                                                      // MinQueryInBufferSize
        0,                                                      // MinQueryOutBufferSize

        { MBB_UUID_DSS_CONSTANT, MBB_DSS_CID_CONNECT},          // ResponseCommand
        0,                                                      // ResponseBufferLength
        MbbNdisDssCloseStatusHandler,                           // ResponseHandler
        NULL                                                    // CompletionHandler
    },
#endif
    {0} // terminator
};

MBB_OID_HANDLER_ENTRY MbbOidHandlerTable[] =
{
    {
        OID_GEN_MEDIA_SUPPORTED,                                // Oid

        FALSE,                                                  // IsSettable
        TRUE,                                                   // IsQueryable
        FALSE,                                                  // IsSerialized
        FALSE,                                                  // IsIndicationRequired
        TRUE,                                                   // CompleteRequestOnSendComplete

        0,                                                      // NdisObjectHeaderType
        0,                                                      // NdisObjectHeaderRevision
        0,                                                      // NdisObjectHeaderSize

        NULL,                                                   // Set routine
        0,                                                      // MinSetInBufferSize

        MbbNdisGetMediaSupported,                               // Query routine
        0,                                                      // MinQueryInBufferSize
        sizeof(ULONG),                                          // MinQueryOutBufferSize

        { MBB_UUID_INVALID_CONSTANT, 0 },                       // ResponseCommand
        0,                                                      // ResponseBufferLength
        NULL,                                                   // ResponseHandler
        NULL                                                    // CompletionHandler
    },
    {
        OID_GEN_RCV_OK,                                         // Oid

        FALSE,                                                  // IsSettable
        TRUE,                                                   // IsQueryable
        FALSE,                                                  // IsSerialized
        FALSE,                                                  // IsIndicationRequired
        TRUE,                                                   // CompleteRequestOnSendComplete

        0,                                                      // NdisObjectHeaderType
        0,                                                      // NdisObjectHeaderRevision
        0,                                                      // NdisObjectHeaderSize

        NULL,                                                   // Set routine
        0,                                                      // MinSetInBufferSize

        MbbNdisGetFramesReceived,                               // Query routine
        0,                                                      // MinQueryInBufferSize
        sizeof(ULONGLONG),                                      // MinQueryOutBufferSize

        { MBB_UUID_INVALID_CONSTANT, 0 },                       // ResponseCommand
        0,                                                      // ResponseBufferLength
        NULL,                                                   // ResponseHandler
        NULL                                                    // CompletionHandler
    },
    {
        OID_GEN_XMIT_OK,                                        // Oid

        FALSE,                                                  // IsSettable
        TRUE,                                                   // IsQueryable
        FALSE,                                                  // IsSerialized
        FALSE,                                                  // IsIndicationRequired
        TRUE,                                                   // CompleteRequestOnSendComplete

        0,                                                      // NdisObjectHeaderType
        0,                                                      // NdisObjectHeaderRevision
        0,                                                      // NdisObjectHeaderSize

        NULL,                                                   // Set routine
        0,                                                      // MinSetInBufferSize

        MbbNdisGetFramesTransmitted,                            // Query routine
        0,                                                      // MinQueryInBufferSize
        sizeof(ULONGLONG),                                      // MinQueryOutBufferSize

        { MBB_UUID_INVALID_CONSTANT, 0 },                       // ResponseCommand
        0,                                                      // ResponseBufferLength
        NULL,                                                   // ResponseHandler
        NULL                                                    // CompletionHandler
    },
    {
        OID_GEN_STATISTICS,                                     // Oid

        FALSE,                                                  // IsSettable
        TRUE,                                                   // IsQueryable
        FALSE,                                                  // IsSerialized
        FALSE,                                                   // IsIndicationRequired
        TRUE,                                                  // CompleteRequestOnSendComplete

        0,                                                      // NdisObjectHeaderType
        0,                                                      // NdisObjectHeaderRevision
        0,                                                      // NdisObjectHeaderSize

        NULL,                                                   // Set routine
        0,                                                      // MinSetInBufferSize

        MbbNdisGetStatistics,                                   // Query routine
        0,                                                      // MinQueryInBufferSize
        sizeof(NDIS_STATISTICS_INFO),                           // MinQueryOutBufferSize

        { MBB_UUID_BASIC_CONNECT_CONSTANT, MBB_BASIC_CID_PACKET_STATISTICS },                       // ResponseCommand
        sizeof(MBB_PACKET_STATISTICS),                          // ResponseBufferLength
        NULL, //MbbNdisGetStatsHandler,                                 // ResponseHandler
        NULL                                                    // CompletionHandler
    },
    {
        OID_GEN_INTERRUPT_MODERATION,                           // Oid

        TRUE,                                                   // IsSettable
        TRUE,                                                   // IsQueryable
        FALSE,                                                  // IsSerialized
        FALSE,                                                  // IsIndicationRequired
        TRUE,                                                   // CompleteRequestOnSendComplete

        0,                                                      // NdisObjectHeaderType
        0,                                                      // NdisObjectHeaderRevision
        0,                                                      // NdisObjectHeaderSize

        MbbNdisSetInterruptModeration,                          // Set routine
        sizeof(NDIS_INTERRUPT_MODERATION_PARAMETERS),           // MinSetInBufferSize

        MbbNdisGetInterruptModeration,                          // Query routine
        0,                                                      // MinQueryInBufferSize
        sizeof(NDIS_INTERRUPT_MODERATION_PARAMETERS),           // MinQueryOutBufferSize

        { MBB_UUID_INVALID_CONSTANT, 0 },                       // ResponseCommand
        0,                                                      // ResponseBufferLength
        NULL,                                                   // ResponseHandler
        NULL                                                    // CompletionHandler
    },
    {
        OID_PNP_QUERY_POWER,                                    // Oid

        FALSE,                                                  // IsSettable
        TRUE,                                                   // IsQueryable
        FALSE,                                                  // IsSerialized
        FALSE,                                                  // IsIndicationRequired
        TRUE,                                                   // CompleteRequestOnSendComplete

        0,                                                      // NdisObjectHeaderType
        0,                                                      // NdisObjectHeaderRevision
        0,                                                      // NdisObjectHeaderSize

        NULL,                                                   // Set routine
        0,                                                      // MinSetInBufferSize

        MbbNdisQueryPower,                                      // Query routine
        0,                                                      // MinQueryInBufferSize
        sizeof(ULONG),                                          // MinQueryOutBufferSize

        { MBB_UUID_INVALID_CONSTANT, 0 },                       // ResponseCommand
        0,                                                      // ResponseBufferLength
        NULL,                                                   // ResponseHandler
        NULL                                                    // CompletionHandler
    },
    {
        OID_PNP_SET_POWER,                                      // Oid

        TRUE,                                                   // IsSettable
        FALSE,                                                  // IsQueryable
        FALSE,                                                  // IsSerialized
        FALSE,                                                  // IsIndicationRequired
        TRUE,                                                   // CompleteRequestOnSendComplete

        0,                                                      // NdisObjectHeaderType
        0,                                                      // NdisObjectHeaderRevision
        0,                                                      // NdisObjectHeaderSize

        MbbNdisSetPower,                                        // Set routine
        sizeof(ULONG),                                          // MinSetInBufferSize

        NULL,                                                   // Query routine
        0,                                                      // MinQueryInBufferSize
        0,                                                      // MinQueryOutBufferSize

        { MBB_UUID_INVALID_CONSTANT, 0 },                       // ResponseCommand
        0,                                                      // ResponseBufferLength
        NULL,                                                   // ResponseHandler
        NULL                                                    // CompletionHandler
    },
    {
        OID_INVALID,                                            // Oid

        TRUE,                                                   // IsSettable
        TRUE,                                                   // IsQueryable
        TRUE,                                                   // IsSerialized
        TRUE,                                                   // IsIndicationRequired
        TRUE,                                                   // CompleteRequestOnSendComplete

        0,                                                      // NdisObjectHeaderType
        0,                                                      // NdisObjectHeaderRevision
        0,                                                      // NdisObjectHeaderSize

        NULL,                                                   // Set routine
        0,                                                      // MinSetInBufferSize

        NULL,                                                   // Query routine
        0,                                                      // MinQueryInBufferSize
        0,                                                      // MinQueryOutBufferSize

        { MBB_UUID_BASIC_CONNECT_CONSTANT, MBB_BASIC_CID_NETWORK_IDLE_HINT },  // ResponseCommand
        0,                                                      // ResponseBufferLength
        NULL,                                                   // ResponseHandler
        NULL                                                    // CompletionHandler
    },
    {
        OID_INVALID,                                            // Oid

        TRUE,                                                   // IsSettable
        TRUE,                                                   // IsQueryable
        TRUE,                                                   // IsSerialized
        TRUE,                                                   // IsIndicationRequired
        TRUE,                                                   // CompleteRequestOnSendComplete

        0,                                                      // NdisObjectHeaderType
        0,                                                      // NdisObjectHeaderRevision
        0,                                                      // NdisObjectHeaderSize

        NULL,                                                   // Set routine
        0,                                                      // MinSetInBufferSize

        NULL,                                                   // Query routine
        0,                                                      // MinQueryInBufferSize
        0,                                                      // MinQueryOutBufferSize

        { MBB_UUID_HOSTSHUTDOWN_CONSTANT, MBB_HOSTSHUTDOWN_CID_ONE },  // ResponseCommand
        0,                                                      // ResponseBufferLength
        NULL,                                                   // ResponseHandler
        NULL                                                    // CompletionHandler
    },
    {
        OID_INVALID,                                            // Oid

        TRUE,                                                   // IsSettable
        TRUE,                                                   // IsQueryable
        TRUE,                                                   // IsSerialized
        TRUE,                                                   // IsIndicationRequired
        TRUE,                                                   // CompleteRequestOnSendComplete

        0,                                                      // NdisObjectHeaderType
        0,                                                      // NdisObjectHeaderRevision
        0,                                                      // NdisObjectHeaderSize

        NULL,                                                   // Set routine
        0,                                                      // MinSetInBufferSize

        NULL,                                                   // Query routine
        0,                                                      // MinQueryInBufferSize
        0,                                                      // MinQueryOutBufferSize

        { MBB_UUID_BASIC_CONNECT_CONSTANT, MBB_BASIC_CID_PACKET_FILTERS },  // ResponseCommand
        0,                                                      // ResponseBufferLength
        NULL,                                                   // ResponseHandler
        NULL                                                    // CompletionHandler
    },
    {
        OID_PM_PARAMETERS,                                      // Oid

        TRUE,                                                   // IsSettable
        FALSE,                                                  // IsQueryable
        FALSE,                                                  // IsSerialized
        FALSE,                                                  // IsIndicationRequired
        FALSE,                                                  // CompleteRequestOnSendComplete

        0,                                                      // NdisObjectHeaderType
        0,                                                      // NdisObjectHeaderRevision
        0,                                                      // NdisObjectHeaderSize

        MbbNdisSetPmParameters,                                 // Set routine
        sizeof(NDIS_PM_PARAMETERS),                             // MinSetInBufferSize

        NULL,                                                   // Query routine
        0,                                                      // MinQueryInBufferSize
        0,                                                      // MinQueryOutBufferSize

        { MBB_UUID_BASIC_CONNECT_CONSTANT, MBB_BASIC_CID_PACKET_FILTERS },  // ResponseCommand
        0,                                                      // ResponseBufferLength
        NULL,                                                   // ResponseHandler
        NULL                                                    // CompletionHandler
    },
    {
        OID_PM_ADD_WOL_PATTERN,                                 // Oid

        TRUE,                                                   // IsSettable
        FALSE,                                                  // IsQueryable
        FALSE,                                                  // IsSerialized
        FALSE,                                                  // IsIndicationRequired
        TRUE,                                                   // CompleteRequestOnSendComplete

        0,                                                      // NdisObjectHeaderType
        0,                                                      // NdisObjectHeaderRevision
        0,                                                      // NdisObjectHeaderSize

        MbbNdisSetAddWolPattern,                                // Set routine
        sizeof(NDIS_PM_WOL_PATTERN),                            // MinSetInBufferSize

        NULL,                                                   // Query routine
        0,                                                      // MinQueryInBufferSize
        0,                                                      // MinQueryOutBufferSize

        { MBB_UUID_INVALID_CONSTANT, 0 },                       // ResponseCommand
        0,                                                      // ResponseBufferLength
        NULL,                                                   // ResponseHandler
        NULL                                                    // CompletionHandler
    },
    {
        OID_PM_REMOVE_WOL_PATTERN,                              // Oid

        TRUE,                                                   // IsSettable
        FALSE,                                                  // IsQueryable
        FALSE,                                                  // IsSerialized
        FALSE,                                                  // IsIndicationRequired
        TRUE,                                                   // CompleteRequestOnSendComplete

        0,                                                      // NdisObjectHeaderType
        0,                                                      // NdisObjectHeaderRevision
        0,                                                      // NdisObjectHeaderSize

        MbbNdisSetRemoveAddWolPattern,                          // Set routine
        sizeof(ULONG),                                          // MinSetInBufferSize

        NULL,                                                   // Query routine
        0,                                                      // MinQueryInBufferSize
        0,                                                      // MinQueryOutBufferSize

        { MBB_UUID_INVALID_CONSTANT, 0 },                       // ResponseCommand
        0,                                                      // ResponseBufferLength
        NULL,                                                   // ResponseHandler
        NULL                                                    // CompletionHandler
    },
    {
        OID_GEN_VENDOR_DRIVER_VERSION,                          // Oid

        FALSE,                                                  // IsSettable
        TRUE,                                                   // IsQueryable
        FALSE,                                                  // IsSerialized
        FALSE,                                                  // IsIndicationRequired
        TRUE,                                                   // CompleteRequestOnSendComplete

        0,                                                      // NdisObjectHeaderType
        0,                                                      // NdisObjectHeaderRevision
        0,                                                      // NdisObjectHeaderSize

        NULL,                                                   // Set routine
        0,                                                      // MinSetInBufferSize

        MbbNdisGetVendorDriverVersion,                          // Query routine
        0,                                                      // MinQueryInBufferSize
        sizeof(ULONG),                                          // MinQueryOutBufferSize

        { MBB_UUID_INVALID_CONSTANT, 0 },                       // ResponseCommand
        0,                                                      // ResponseBufferLength
        NULL,                                                   // ResponseHandler
        NULL                                                    // CompletionHandler
    },
    {
        OID_GEN_VENDOR_ID,                                      // Oid

        FALSE,                                                  // IsSettable
        TRUE,                                                   // IsQueryable
        FALSE,                                                  // IsSerialized
        FALSE,                                                  // IsIndicationRequired
        TRUE,                                                   // CompleteRequestOnSendComplete

        0,                                                      // NdisObjectHeaderType
        0,                                                      // NdisObjectHeaderRevision
        0,                                                      // NdisObjectHeaderSize

        NULL,                                                   // Set routine
        0,                                                      // MinSetInBufferSize

        MbbNdisGetVendorId,                                     // Query routine
        0,                                                      // MinQueryInBufferSize
        sizeof(ULONG),                                          // MinQueryOutBufferSize

        { MBB_UUID_INVALID_CONSTANT, 0 },                       // ResponseCommand
        0,                                                      // ResponseBufferLength
        NULL,                                                   // ResponseHandler
        NULL                                                    // CompletionHandler
    },
    {
        OID_GEN_VENDOR_DESCRIPTION,                             // Oid

        FALSE,                                                  // IsSettable
        TRUE,                                                   // IsQueryable
        FALSE,                                                  // IsSerialized
        FALSE,                                                  // IsIndicationRequired
        TRUE,                                                   // CompleteRequestOnSendComplete

        0,                                                      // NdisObjectHeaderType
        0,                                                      // NdisObjectHeaderRevision
        0,                                                      // NdisObjectHeaderSize

        NULL,                                                   // Set routine
        0,                                                      // MinSetInBufferSize

        MbbNdisGetVendorDescription,                            // Query routine
        0,                                                      // MinQueryInBufferSize
        0,                                                      // MinQueryOutBufferSize

        { MBB_UUID_INVALID_CONSTANT, 0 },                       // ResponseCommand
        0,                                                      // ResponseBufferLength
        NULL,                                                   // ResponseHandler
        NULL                                                    // CompletionHandler
    },
    {
        OID_GEN_CURRENT_LOOKAHEAD,                              // Oid

        TRUE,                                                   // IsSettable
        TRUE,                                                   // IsQueryable
        FALSE,                                                  // IsSerialized
        FALSE,                                                  // IsIndicationRequired
        TRUE,                                                   // CompleteRequestOnSendComplete

        0,                                                      // NdisObjectHeaderType
        0,                                                      // NdisObjectHeaderRevision
        0,                                                      // NdisObjectHeaderSize

        MbbNdisSetCurrentLookahead,                             // Set routine
        sizeof(ULONG),                                          // MinSetInBufferSize

        MbbNdisGetCurrentLookahead,                             // Query routine
        0,                                                      // MinQueryInBufferSize
        sizeof(ULONG),                                          // MinQueryOutBufferSize

        { MBB_UUID_INVALID_CONSTANT, 0 },                       // ResponseCommand
        0,                                                      // ResponseBufferLength
        NULL,                                                   // ResponseHandler
        NULL                                                    // CompletionHandler
    },
    {
        OID_GEN_RECEIVE_BLOCK_SIZE,                             // Oid

        TRUE,                                                   // IsSettable
        TRUE,                                                   // IsQueryable
        FALSE,                                                  // IsSerialized
        FALSE,                                                  // IsIndicationRequired
        TRUE,                                                   // CompleteRequestOnSendComplete

        0,                                                      // NdisObjectHeaderType
        0,                                                      // NdisObjectHeaderRevision
        0,                                                      // NdisObjectHeaderSize

        NULL,                                                   // Set routine
        sizeof(ULONG),                                          // MinSetInBufferSize

        MbbNdisGetReceiveBlockSize,                             // Query routine
        0,                                                      // MinQueryInBufferSize
        sizeof(ULONG),                                          // MinQueryOutBufferSize

        { MBB_UUID_INVALID_CONSTANT, 0 },                       // ResponseCommand
        0,                                                      // ResponseBufferLength
        NULL,                                                   // ResponseHandler
        NULL                                                    // CompletionHandler
    },
    {
        OID_GEN_RECEIVE_BUFFER_SPACE,                           // Oid

        TRUE,                                                   // IsSettable
        TRUE,                                                   // IsQueryable
        FALSE,                                                  // IsSerialized
        FALSE,                                                  // IsIndicationRequired
        TRUE,                                                   // CompleteRequestOnSendComplete

        0,                                                      // NdisObjectHeaderType
        0,                                                      // NdisObjectHeaderRevision
        0,                                                      // NdisObjectHeaderSize

        NULL,                                                   // Set routine
        sizeof(ULONG),                                          // MinSetInBufferSize

        MbbNdisGetReceiveBufferSpace,                             // Query routine
        0,                                                      // MinQueryInBufferSize
        sizeof(ULONG),                                          // MinQueryOutBufferSize

        { MBB_UUID_INVALID_CONSTANT, 0 },                       // ResponseCommand
        0,                                                      // ResponseBufferLength
        NULL,                                                   // ResponseHandler
        NULL                                                    // CompletionHandler
    },
    {
        OID_GEN_TRANSMIT_BLOCK_SIZE,                              // Oid

        TRUE,                                                   // IsSettable
        TRUE,                                                   // IsQueryable
        FALSE,                                                  // IsSerialized
        FALSE,                                                  // IsIndicationRequired
        TRUE,                                                   // CompleteRequestOnSendComplete

        0,                                                      // NdisObjectHeaderType
        0,                                                      // NdisObjectHeaderRevision
        0,                                                      // NdisObjectHeaderSize

        NULL,                                                   // Set routine
        sizeof(ULONG),                                          // MinSetInBufferSize

        MbbNdisGetTransmitBlockSize,                            // Query routine
        0,                                                      // MinQueryInBufferSize
        sizeof(ULONG),                                          // MinQueryOutBufferSize

        { MBB_UUID_INVALID_CONSTANT, 0 },                       // ResponseCommand
        0,                                                      // ResponseBufferLength
        NULL,                                                   // ResponseHandler
        NULL                                                    // CompletionHandler
    },
    {
        OID_GEN_TRANSMIT_BUFFER_SPACE,                          // Oid

        TRUE,                                                   // IsSettable
        TRUE,                                                   // IsQueryable
        FALSE,                                                  // IsSerialized
        FALSE,                                                  // IsIndicationRequired
        TRUE,                                                   // CompleteRequestOnSendComplete

        0,                                                      // NdisObjectHeaderType
        0,                                                      // NdisObjectHeaderRevision
        0,                                                      // NdisObjectHeaderSize

        NULL,                                                   // Set routine
        sizeof(ULONG),                                          // MinSetInBufferSize

        MbbNdisGetTransmitBufferSpace,                           // Query routine
        0,                                                      // MinQueryInBufferSize
        sizeof(ULONG),                                          // MinQueryOutBufferSize

        { MBB_UUID_INVALID_CONSTANT, 0 },                       // ResponseCommand
        0,                                                      // ResponseBufferLength
        NULL,                                                   // ResponseHandler
        NULL                                                    // CompletionHandler
    },
    {
        OID_GEN_MAXIMUM_TOTAL_SIZE,                             // Oid

        TRUE,                                                   // IsSettable
        TRUE,                                                   // IsQueryable
        FALSE,                                                  // IsSerialized
        FALSE,                                                  // IsIndicationRequired
        TRUE,                                                   // CompleteRequestOnSendComplete

        0,                                                      // NdisObjectHeaderType
        0,                                                      // NdisObjectHeaderRevision
        0,                                                      // NdisObjectHeaderSize

        NULL,                                                   // Set routine
        sizeof(ULONG),                                          // MinSetInBufferSize

        MbbNdisGetMaximumTotalSize,                             // Query routine
        0,                                                      // MinQueryInBufferSize
        sizeof(ULONG),                                          // MinQueryOutBufferSize

        { MBB_UUID_INVALID_CONSTANT, 0 },                       // ResponseCommand
        0,                                                      // ResponseBufferLength
        NULL,                                                   // ResponseHandler
        NULL                                                    // CompletionHandler
    },
    //
    // BASIC CONNECT
    //
    {
        OID_WWAN_DRIVER_CAPS,                                   // Oid

        FALSE,                                                  // IsSettable
        TRUE,                                                   // IsQueryable
        FALSE,                                                  // IsSerialized
        FALSE,                                                  // IsIndicationRequired
        TRUE,                                                   // CompleteRequestOnSendComplete

        0,                                                      // NdisObjectHeaderType
        0,                                                      // NdisObjectHeaderRevision
        0,                                                      // NdisObjectHeaderSize

        NULL,                                                   // Set routine
        0,                                                      // MinSetInBufferSize

        MbbNdisGetDriverCaps,                                   // Query routine
        0,                                                      // MinQueryInBufferSize
        sizeof(NDIS_WWAN_DRIVER_CAPS),                          // MinQueryOutBufferSize

        { MBB_UUID_INVALID_CONSTANT, 0 },                       // ResponseCommand
        0,                                                      // ResponseBufferLength
        NULL,                                                   // ResponseHandler
        NULL                                                    // CompletionHandler
    },
    {
        OID_WWAN_DEVICE_CAPS,                                   // Oid

        FALSE,                                                  // IsSettable
        TRUE,                                                   // IsQueryable
        TRUE,                                                   // IsSerialized
        TRUE,                                                   // IsIndicationRequired
        TRUE,                                                   // CompleteRequestOnSendComplete

        0,                                                      // NdisObjectHeaderType
        0,                                                      // NdisObjectHeaderRevision
        0,                                                      // NdisObjectHeaderSize

        NULL,                                                   // Set routine
        0,                                                      // MinSetInBufferSize

        MbbNdisGetDeviceCaps,                                   // Query routine
        0,                                                      // MinQueryInBufferSize
        sizeof(NDIS_WWAN_DEVICE_CAPS),                          // MinQueryOutBufferSize

        { MBB_UUID_BASIC_CONNECT_CONSTANT, MBB_BASIC_CID_DEVICE_CAPS },  // ResponseCommand
        sizeof(MBB_DEVICE_CAPS),                                // ResponseBufferLength
        MbbNdisDeviceCapsStatusHandler,                         // ResponseHandler
        NULL                                                    // CompletionHandler
    },
    {
        OID_WWAN_READY_INFO,                                    // Oid

        FALSE,                                                  // IsSettable
        TRUE,                                                   // IsQueryable
        TRUE,                                                   // IsSerialized
        TRUE,                                                   // IsIndicationRequired
        TRUE,                                                   // CompleteRequestOnSendComplete

        0,                                                      // NdisObjectHeaderType
        0,                                                      // NdisObjectHeaderRevision
        0,                                                      // NdisObjectHeaderSize

        NULL,                                                   // Set routine
        0,                                                      // MinSetInBufferSize

        MbbNdisGetReadyInfo,                                    // Query routine
        0,                                                      // MinQueryInBufferSize
        sizeof(NDIS_WWAN_READY_INFO),                           // MinQueryOutBufferSize

        { MBB_UUID_BASIC_CONNECT_CONSTANT, MBB_BASIC_CID_SUBSCRIBER_READY_INFO },   // ResponseCommand
        sizeof(MBB_SUBSCRIBER_READY_INFO),                                 // ResponseBufferLength
        MbbNdisReadyInfoStatusHandler,                          // ResponseHandler
        NULL                                                    // CompletionHandler
    },
    {
        OID_INVALID,                                            // Oid

        FALSE,                                                  // IsSettable
        TRUE,                                                   // IsQueryable
        TRUE,                                                   // IsSerialized
        TRUE,                                                   // IsIndicationRequired
        TRUE,                                                   // CompleteRequestOnSendComplete

        0,                                                      // NdisObjectHeaderType
        0,                                                      // NdisObjectHeaderRevision
        0,                                                      // NdisObjectHeaderSize

        NULL,                                                   // Set routine
        0,                                                      // MinSetInBufferSize

        MbbNdisGetEmergencyMode,                                // Query routine
        0,                                                      // MinQueryInBufferSize
        0,                                                      // MinQueryOutBufferSize

        { MBB_UUID_BASIC_CONNECT_CONSTANT, MBB_BASIC_CID_EMERGENCY_MODE },   // ResponseCommand
        sizeof(MBB_EMERGENCY_MODE),                             // ResponseBufferLength
        MbbNdisEmergencyModeStatusHandler,                      // ResponseHandler
        MbbNdisReadyInfoCompletionHandler                       // CompletionHandler
    },
    {
        OID_WWAN_RADIO_STATE,                                   // Oid

        TRUE,                                                   // IsSettable
        TRUE,                                                   // IsQueryable
        TRUE,                                                   // IsSerialized
        TRUE,                                                   // IsIndicationRequired
        TRUE,                                                   // CompleteRequestOnSendComplete

        NDIS_OBJECT_TYPE_DEFAULT,                               // NdisObjectHeaderType
        NDIS_WWAN_SET_RADIO_STATE_REVISION_1,                   // NdisObjectHeaderRevision
        sizeof(NDIS_WWAN_SET_RADIO_STATE),                      // NdisObjectHeaderSize

        MbbNdisSetRadioState,                                   // Set routine
        sizeof(NDIS_WWAN_SET_RADIO_STATE),                      // MinSetInBufferSize

        MbbNdisGetRadioState,                                   // Query routine
        0,                                                      // MinQueryInBufferSize
        sizeof(NDIS_WWAN_RADIO_STATE),                          // MinQueryOutBufferSize

        { MBB_UUID_BASIC_CONNECT_CONSTANT, MBB_BASIC_CID_RADIO_STATE },  // ResponseCommand
        sizeof(MBB_QUERY_RADIO_STATE),                          // ResponseBufferLength
        MbbNdisRadioStateStatusHandler,                         // ResponseHandler
        NULL                                                    // CompletionHandler
    },

    {
        OID_WWAN_PIN_EX,                                        // Oid

        TRUE,                                                   // IsSettable
        TRUE,                                                   // IsQueryable
        TRUE,                                                   // IsSerialized
        TRUE,                                                   // IsIndicationRequired
        TRUE,                                                   // CompleteRequestOnSendComplete

        NDIS_OBJECT_TYPE_DEFAULT,                               // NdisObjectHeaderType
        NDIS_WWAN_SET_PIN_REVISION_1,                           // NdisObjectHeaderRevision
        sizeof(NDIS_WWAN_SET_PIN),                              // NdisObjectHeaderSize

        MbbNdisSetPinInfo,                                      // Set routine
        sizeof(NDIS_WWAN_SET_PIN),                              // MinSetInBufferSize

        MbbNdisGetPinInfo,                                      // Query routine
        0,                                                      // MinQueryInBufferSize
        sizeof(NDIS_WWAN_PIN_INFO),                             // MinQueryOutBufferSize

        { MBB_UUID_BASIC_CONNECT_CONSTANT, MBB_BASIC_CID_PIN_INFO },     // ResponseCommand
        sizeof(MBB_PIN_INFO),                                   // ResponseBufferLength
        MbbNdisPinInfoStatusHandler,                            // ResponseHandler
        NULL                                                    // CompletionHandler
    },
    {
        OID_WWAN_PIN_LIST,                                      // Oid

        FALSE,                                                  // IsSettable
        TRUE,                                                   // IsQueryable
        TRUE,                                                   // IsSerialized
        TRUE,                                                   // IsIndicationRequired
        TRUE,                                                   // CompleteRequestOnSendComplete

        0,                                                      // NdisObjectHeaderType
        0,                                                      // NdisObjectHeaderRevision
        0,                                                      // NdisObjectHeaderSize

        NULL,                                                   // Set routine
        0,                                                      // MinSetInBufferSize

        MbbNdisGetPinList,                                      // Query routine
        0,                                                      // MinQueryInBufferSize
        sizeof(NDIS_WWAN_PIN_LIST),                             // MinQueryOutBufferSize

        { MBB_UUID_BASIC_CONNECT_CONSTANT, MBB_BASIC_CID_PIN_LIST },     // ResponseCommand
        sizeof(MBB_PIN_LIST),                                   // ResponseBufferLength
        MbbNdisPinListStatusHandler,                            // ResponseHandler
        NULL                                                    // CompletionHandler
    },
    {
        OID_WWAN_HOME_PROVIDER,                                 // Oid

        TRUE,                                                   // IsSettable
        TRUE,                                                   // IsQueryable
        TRUE,                                                   // IsSerialized
        TRUE,                                                   // IsIndicationRequired
        TRUE,                                                   // CompleteRequestOnSendComplete

        NDIS_OBJECT_TYPE_DEFAULT,                               // NdisObjectHeaderType
        NDIS_WWAN_SET_HOME_PROVIDER_REVISION_1,                 // NdisObjectHeaderRevision
        sizeof(NDIS_WWAN_SET_HOME_PROVIDER),                    // NdisObjectHeaderSize

        MbbNdisSetHomeProvider,                                 // Set routine
        sizeof(NDIS_WWAN_SET_HOME_PROVIDER),                    // MinSetInBufferSize

        MbbNdisGetHomeProvider,                                 // Query routine
        0,                                                      // MinQueryInBufferSize
        sizeof(NDIS_WWAN_HOME_PROVIDER2),                       // MinQueryOutBufferSize

        { MBB_UUID_BASIC_CONNECT_CONSTANT, MBB_BASIC_CID_HOME_PROVIDER },// ResponseCommand
        sizeof(MBB_PROVIDER),                                   // ResponseBufferLength
        MbbNdisHomeProviderStatusHandler,                       // ResponseHandler
        NULL                                                    // CompletionHandler
    },
    {
        OID_WWAN_PREFERRED_PROVIDERS,                           // Oid

        TRUE,                                                   // IsSettable
        TRUE,                                                   // IsQueryable
        TRUE,                                                   // IsSerialized
        TRUE,                                                   // IsIndicationRequired
        TRUE,                                                   // CompleteRequestOnSendComplete

        NDIS_OBJECT_TYPE_DEFAULT,                               // NdisObjectHeaderType
        NDIS_WWAN_SET_PREFERRED_PROVIDERS_REVISION_1,           // NdisObjectHeaderRevision
        sizeof(NDIS_WWAN_SET_PREFERRED_PROVIDERS),              // NdisObjectHeaderSize

        MbbNdisSetPreferredProviders,                           // Set routine
        sizeof(NDIS_WWAN_SET_PREFERRED_PROVIDERS),              // MinSetInBufferSize

        MbbNdisGetPreferredProviders,                           // Query routine
        0,                                                      // MinQueryInBufferSize
        sizeof(NDIS_WWAN_PREFERRED_PROVIDERS),                  // MinQueryOutBufferSize

        { MBB_UUID_BASIC_CONNECT_CONSTANT, MBB_BASIC_CID_PREFERRED_PROVIDERS }, // ResponseCommand
        0,                                                      // ResponseBufferLength
        MbbNdisPreferredProviderStatusHandler,                  // ResponseHandler
        NULL                                                    // CompletionHandler
    },
    {
        OID_WWAN_VISIBLE_PROVIDERS,                             // Oid

        FALSE,                                                  // IsSettable
        TRUE,                                                   // IsQueryable
        TRUE,                                                   // IsSerialized
        TRUE,                                                   // IsIndicationRequired
        TRUE,                                                   // CompleteRequestOnSendComplete

        0,                                                      // NdisObjectHeaderType
        0,                                                      // NdisObjectHeaderRevision
        0,                                                      // NdisObjectHeaderSize

        NULL,                                                   // Set routine
        0,                                                      // MinSetInBufferSize

        MbbNdisGetVisibleProviders,                             // Query routine
        sizeof(NDIS_WWAN_GET_VISIBLE_PROVIDERS),                // MinQueryInBufferSize
        sizeof(NDIS_WWAN_VISIBLE_PROVIDERS),                    // MinQueryOutBufferSize

        { MBB_UUID_BASIC_CONNECT_CONSTANT, MBB_BASIC_CID_VISIBLE_PROVIDERS }, // ResponseCommand
        0,                                                      // ResponseBufferLength
        MbbNdisVisibleProviderStatusHandler,                    // ResponseHandler
        NULL                                                    // CompletionHandler
    },
    {
        OID_WWAN_REGISTER_STATE,                                // Oid

        TRUE,                                                   // IsSettable
        TRUE,                                                   // IsQueryable
        TRUE,                                                   // IsSerialized
        TRUE,                                                   // IsIndicationRequired
        TRUE,                                                   // CompleteRequestOnSendComplete

        NDIS_OBJECT_TYPE_DEFAULT,                               // NdisObjectHeaderType
        NDIS_WWAN_SET_REGISTER_STATE_REVISION_1,                // NdisObjectHeaderRevision
        sizeof(NDIS_WWAN_SET_REGISTER_STATE),                   // NdisObjectHeaderSize

        MbbNdisSetRegisterState,                                // Set routine
        sizeof(NDIS_WWAN_SET_REGISTER_STATE),                   // MinSetInBufferSize

        MbbNdisGetRegisterState,                                // Query routine
        0,                                                      // MinQueryInBufferSize
        sizeof(NDIS_WWAN_REGISTRATION_STATE),                   // MinQueryOutBufferSize

        { MBB_UUID_BASIC_CONNECT_CONSTANT, MBB_BASIC_CID_REGISTER_STATE }, // ResponseCommand
        sizeof(MBB_REGISTRATION_STATE),                         // ResponseBufferLength
        MbbNdisRegisterStateStatusHandler,                      // ResponseHandler
        MbbNdisPacketServiceStateCompletionHandler              // CompletionHandler, intentional PacketServiceHandler
    },
    {
        OID_WWAN_PACKET_SERVICE,                                // Oid

        TRUE,                                                   // IsSettable
        TRUE,                                                   // IsQueryable
        TRUE,                                                   // IsSerialized
        TRUE,                                                   // IsIndicationRequired
        TRUE,                                                   // CompleteRequestOnSendComplete

        NDIS_OBJECT_TYPE_DEFAULT,                               // NdisObjectHeaderType
        NDIS_WWAN_SET_PACKET_SERVICE_REVISION_1,                // NdisObjectHeaderRevision
        sizeof(NDIS_WWAN_SET_PACKET_SERVICE),                   // NdisObjectHeaderSize

        MbbNdisSetPacketService,                                // Set routine
        sizeof(NDIS_WWAN_SET_PACKET_SERVICE),                   // MinSetInBufferSize

        MbbNdisGetPacketService,                                // Query routine
        0,                                                      // MinQueryInBufferSize
        sizeof(NDIS_WWAN_PACKET_SERVICE_STATE),                 // MinQueryOutBufferSize

        { MBB_UUID_BASIC_CONNECT_CONSTANT, MBB_BASIC_CID_PACKET_SERVICE }, // ResponseCommand
        sizeof(MBB_PACKET_SERVICE),                             // ResponseBufferLength
        MbbNdisPacketServiceStatusHandler,                      // ResponseHandler
        MbbNdisPacketServiceStateCompletionHandler              // CompletionHandler
    },
    {
        OID_WWAN_SIGNAL_STATE,                                  // Oid

        TRUE,                                                   // IsSettable
        TRUE,                                                   // IsQueryable
        TRUE,                                                   // IsSerialized
        TRUE,                                                   // IsIndicationRequired
        TRUE,                                                   // CompleteRequestOnSendComplete

        NDIS_OBJECT_TYPE_DEFAULT,                               // NdisObjectHeaderType
        NDIS_WWAN_SET_SIGNAL_INDICATION_REVISION_1,             // NdisObjectHeaderRevision
        sizeof(NDIS_WWAN_SET_SIGNAL_INDICATION),                // NdisObjectHeaderSize

        MbbNdisSetSignalState,                                  // Set routine
        sizeof(NDIS_WWAN_SET_SIGNAL_INDICATION),                // MinSetInBufferSize

        MbbNdisGetSignalState,                                  // Query routine
        0,                                                      // MinQueryInBufferSize
        sizeof(NDIS_WWAN_SIGNAL_STATE),                         // MinQueryOutBufferSize

        { MBB_UUID_BASIC_CONNECT_CONSTANT, MBB_BASIC_CID_SIGNAL_STATE }, // ResponseCommand
        sizeof(MBB_SIGNAL_STATE),                               // ResponseBufferLength
        MbbNdisSignalStateStatusHandler,                        // ResponseHandler
        NULL                                                    // CompletionHandler
    },
    {
        OID_WWAN_CONNECT,                                       // Oid

        TRUE,                                                   // IsSettable
        TRUE,                                                   // IsQueryable
        TRUE,                                                   // IsSerialized
        TRUE,                                                   // IsIndicationRequired
        TRUE,                                                   // CompleteRequestOnSendComplete

        NDIS_OBJECT_TYPE_DEFAULT,                               // NdisObjectHeaderType
        NDIS_WWAN_SET_CONTEXT_STATE_REVISION_2,                 // NdisObjectHeaderRevision
        SIZEOF_NDIS_WWAN_SET_CONTEXT_STATE_REVISION_2,          // NdisObjectHeaderSize

        MbbNdisSetConnectState,                                 // Set routine
        SIZEOF_NDIS_WWAN_SET_CONTEXT_STATE_REVISION_2,          // MinSetInBufferSize

        MbbNdisGetConnectState,                                 // Query routine
        0,                                                      // MinQueryInBufferSize
        SIZEOF_NDIS_WWAN_CONTEXT_STATE_REVISION_2,              // MinQueryOutBufferSize

        { MBB_UUID_BASIC_CONNECT_CONSTANT, MBB_BASIC_CID_CONNECT }, // ResponseCommand
        sizeof(MBB_CONTEXT_STATE),                              // ResponseBufferLength
        MbbNdisConnectStatusHandler,                            // ResponseHandler
        NULL                                                    // CompletionHandler
    },
    {
        OID_INVALID,                                            // Oid

        FALSE,                                                  // IsSettable
        TRUE,                                                   // IsQueryable
        TRUE,                                                   // IsSerialized
        TRUE,                                                   // IsIndicationRequired
        FALSE,                                                  // CompleteRequestOnSendComplete

        0,                                                      // NdisObjectHeaderType
        0,                                                      // NdisObjectHeaderRevision
        0,                                                      // NdisObjectHeaderSize

        NULL,                                                   // Set routine
        0,                                                      // MinSetInBufferSize

        MbbIpGetConfiguration,                                  // Query routine
        0,                                                      // MinQueryInBufferSize
        0,                                                      // MinQueryOutBufferSize

        { MBB_UUID_BASIC_CONNECT_CONSTANT, MBB_BASIC_CID_IP_ADDRESS_INFO }, // ResponseCommand
        sizeof(MBB_CONTEXT_STATE),                              // ResponseBufferLength
        MbbIpAddressStatusHandler,                              // ResponseHandler
        NULL                                                    // CompletionHandler
    },
    {
        OID_WWAN_PROVISIONED_CONTEXTS,                          // Oid

        TRUE,                                                   // IsSettable
        TRUE,                                                   // IsQueryable
        TRUE,                                                   // IsSerialized
        TRUE,                                                   // IsIndicationRequired
        TRUE,                                                   // CompleteRequestOnSendComplete

        NDIS_OBJECT_TYPE_DEFAULT,                               // NdisObjectHeaderType
        NDIS_WWAN_SET_PROVISIONED_CONTEXT_REVISION_1,           // NdisObjectHeaderRevision
        sizeof(NDIS_WWAN_SET_PROVISIONED_CONTEXT),              // NdisObjectHeaderSize

        MbbNdisSetProvisionedContexts,                          // Set routine
        sizeof(NDIS_WWAN_SET_PROVISIONED_CONTEXT),              // MinSetInBufferSize

        MbbNdisGetProvisionedContexts,                          // Query routine
        0,                                                      // MinQueryInBufferSize
        sizeof(NDIS_WWAN_PROVISIONED_CONTEXTS),                 // MinQueryOutBufferSize

        { MBB_UUID_BASIC_CONNECT_CONSTANT, MBB_BASIC_CID_PROVISIONED_CONTEXTS }, // ResponseCommand
        0,                                                      // ResponseBufferLength
        MbbNdisProvisionedContextStatusHandler,                 // ResponseHandler
        NULL                                                    // CompletionHandler
    },
    {
        OID_WWAN_PROVISIONED_CONTEXTS,                          // Oid

        TRUE,                                                   // IsSettable
        TRUE,                                                   // IsQueryable
        TRUE,                                                   // IsSerialized
        TRUE,                                                   // IsIndicationRequired
        TRUE,                                                   // CompleteRequestOnSendComplete

        NDIS_OBJECT_TYPE_DEFAULT,                               // NdisObjectHeaderType
        NDIS_WWAN_SET_PROVISIONED_CONTEXT_REVISION_2,           // NdisObjectHeaderRevision
        sizeof(NDIS_WWAN_SET_PROVISIONED_CONTEXT_V2),           // NdisObjectHeaderSize

        MbbNdisSetProvisionedContextsV2,                        // Set routine
        sizeof(NDIS_WWAN_SET_PROVISIONED_CONTEXT_V2),           // MinSetInBufferSize

        MbbNdisGetProvisionedContextsV2,                        // Query routine
        0,                                                      // MinQueryInBufferSize
        sizeof(NDIS_WWAN_PROVISIONED_CONTEXTS),                 // MinQueryOutBufferSize

        { MBB_UUID_BASIC_CONNECT_EXT_CONSTANT, MBB_BASICCONNECTEXT_CID_PROVISIONED_CONTEXT_V2 }, // ResponseCommand
        0,                                                      // ResponseBufferLength
        MbbNdisProvisionedContextV2StatusHandler,               // ResponseHandler
        NULL                                                    // CompletionHandler
    },
    {
        OID_WWAN_SERVICE_ACTIVATION,                            // Oid

        TRUE,                                                   // IsSettable
        FALSE,                                                  // IsQueryable
        TRUE,                                                   // IsSerialized
        TRUE,                                                   // IsIndicationRequired
        TRUE,                                                   // CompleteRequestOnSendComplete

        NDIS_OBJECT_TYPE_DEFAULT,                               // NdisObjectHeaderType
        NDIS_WWAN_SERVICE_ACTIVATION_REVISION_1,                // NdisObjectHeaderRevision
        sizeof(NDIS_WWAN_SERVICE_ACTIVATION),                   // NdisObjectHeaderSize

        MbbNdisSetServiceActivation,                            // Set routine
        sizeof(NDIS_WWAN_SERVICE_ACTIVATION),                   // MinSetInBufferSize

        NULL,                                                   // Query routine
        0,                                                      // MinQueryInBufferSize
        0,                                                      // MinQueryOutBufferSize

        { MBB_UUID_BASIC_CONNECT_CONSTANT, MBB_BASIC_CID_SERVICE_ACTIVATION }, // ResponseCommand
        sizeof(MBB_SERVICE_ACTIVATION_STATUS),                  // ResponseBufferLength
        MbbNdisServiceActivationStatusHandler,                  // ResponseHandler
        NULL                                                    // CompletionHandler
    },
    //
    // SMS
    //
    {
        OID_WWAN_SMS_CONFIGURATION,                             // Oid

        TRUE,                                                   // IsSettable
        TRUE,                                                   // IsQueryable
        TRUE,                                                   // IsSerialized
        TRUE,                                                   // IsIndicationRequired
        TRUE,                                                   // CompleteRequestOnSendComplete

        NDIS_OBJECT_TYPE_DEFAULT,                               // NdisObjectHeaderType
        NDIS_WWAN_SET_SMS_CONFIGURATION_REVISION_1,             // NdisObjectHeaderRevision
        sizeof(NDIS_WWAN_SET_SMS_CONFIGURATION),                // NdisObjectHeaderSize

        MbbNdisSmsSetConfiguration,                             // Set routine
        sizeof(NDIS_WWAN_SET_SMS_CONFIGURATION),                // MinSetInBufferSize

        MbbNdisSmsGetConfiguration,                             // Query routine
        0,                                                      // MinQueryInBufferSize
        sizeof(NDIS_WWAN_SMS_CONFIGURATION),                    // MinQueryOutBufferSize

        { MBB_UUID_SMS_CONSTANT, MBB_SMS_CID_CONFIGURATION },   // ResponseCommand
        sizeof(MBB_SMS_CONFIGURATION),                          // ResponseBufferLength
        MbbNdisSmsConfigurationStatusHandler,                   // ResponseHandler
        MbbNdisReadyInfoCompletionHandler                       // CompletionHandler
    },
    {
        OID_WWAN_SMS_READ,                                      // Oid

        FALSE,                                                  // IsSettable
        TRUE,                                                   // IsQueryable
        TRUE,                                                   // IsSerialized
        TRUE,                                                   // IsIndicationRequired
        TRUE,                                                   // CompleteRequestOnSendComplete

        0,                                                      // NdisObjectHeaderType
        0,                                                      // NdisObjectHeaderRevision
        0,                                                      // NdisObjectHeaderSize

        NULL,                                                   // Set routine
        0,                                                      // MinSetInBufferSize

        MbbNdisSmsRead,                                         // Query routine
        sizeof(NDIS_WWAN_SMS_READ),                             // MinQueryInBufferSize
        sizeof(NDIS_WWAN_SMS_RECEIVE),                          // MinQueryOutBufferSize

        { MBB_UUID_SMS_CONSTANT, MBB_SMS_CID_READ },            // ResponseCommand
        0,                                                      // ResponseBufferLength
        MbbNdisSmsReadStatusHandler,                            // ResponseHandler
        NULL                                                    // CompletionHandler
    },
    {
        OID_WWAN_SMS_SEND,                                      // Oid

        TRUE,                                                   // IsSettable
        FALSE,                                                  // IsQueryable
        TRUE,                                                   // IsSerialized
        TRUE,                                                   // IsIndicationRequired
        TRUE,                                                   // CompleteRequestOnSendComplete

        NDIS_OBJECT_TYPE_DEFAULT,                               // NdisObjectHeaderType
        NDIS_WWAN_SMS_SEND_REVISION_1,                          // NdisObjectHeaderRevision
        sizeof(NDIS_WWAN_SMS_SEND),                             // NdisObjectHeaderSize

        MbbNdisSmsSend,                                         // Set routine
        sizeof(NDIS_WWAN_SMS_SEND),                             // MinSetInBufferSize

        NULL,                                                   // Query routine
        0,                                                      // MinQueryInBufferSize
        0,                                                      // MinQueryOutBufferSize

        { MBB_UUID_SMS_CONSTANT, MBB_SMS_CID_SEND },            // ResponseCommand
        sizeof(MBB_SMS_SEND_STATUS),                            // ResponseBufferLength
        MbbNdisSmsSendStatusHandler,                            // ResponseHandler
        NULL                                                    // CompletionHandler
    },
    {
        OID_WWAN_SMS_DELETE,                                    // Oid

        TRUE,                                                   // IsSettable
        FALSE,                                                  // IsQueryable
        TRUE,                                                   // IsSerialized
        TRUE,                                                   // IsIndicationRequired
        TRUE,                                                   // CompleteRequestOnSendComplete

        NDIS_OBJECT_TYPE_DEFAULT,                               // NdisObjectHeaderType
        NDIS_WWAN_SMS_DELETE_REVISION_1,                        // NdisObjectHeaderRevision
        sizeof(NDIS_WWAN_SMS_DELETE),                           // NdisObjectHeaderSize

        MbbNdisSmsDelete,                                       // Set routine
        sizeof(NDIS_WWAN_SMS_DELETE),                           // MinSetInBufferSize

        NULL,                                                   // Query routine
        0,                                                      // MinQueryInBufferSize
        0,                                                      // MinQueryOutBufferSize

        { MBB_UUID_SMS_CONSTANT, MBB_SMS_CID_DELETE },          // ResponseCommand
        0,                                                      // ResponseBufferLength
        MbbNdisSmsDeleteStatusHandler,                          // ResponseHandler
        NULL                                                    // CompletionHandler
    },
    {
        OID_WWAN_SMS_STATUS,                                    // Oid

        FALSE,                                                  // IsSettable
        TRUE,                                                   // IsQueryable
        TRUE,                                                   // IsSerialized
        TRUE,                                                   // IsIndicationRequired
        TRUE,                                                   // CompleteRequestOnSendComplete

        0,                                                      // NdisObjectHeaderType
        0,                                                      // NdisObjectHeaderRevision
        0,                                                      // NdisObjectHeaderSize

        NULL,                                                   // Set routine
        0,                                                      // MinSetInBufferSize

        MbbNdisSmsGetStatus,                                    // Query routine
        0,                                                      // MinQueryInBufferSize
        sizeof(NDIS_WWAN_SMS_STATUS),                           // MinQueryOutBufferSize

        { MBB_UUID_SMS_CONSTANT, MBB_SMS_CID_STATUS },          // ResponseCommand
        sizeof(MBB_SMS_STATUS),                                 // ResponseBufferLength
        MbbNdisSmsStatusHandler,                                // ResponseHandler
        NULL                                                    // CompletionHandler
    },
    //
    // USSD
    //
    {
        OID_WWAN_USSD,                                          // Oid
        TRUE,                                                   // IsSettable
        FALSE,                                                  // IsQueryable
        TRUE,                                                   // IsSerialized
        TRUE,                                                   // IsIndicationRequired
        TRUE,                                                   // CompleteRequestOnSendComplete

        NDIS_OBJECT_TYPE_DEFAULT,                               // NdisObjectHeaderType
        NDIS_WWAN_USSD_REQUEST_REVISION_1,                      // NdisObjectHeaderRevision
        sizeof(NDIS_WWAN_USSD_REQUEST),                         // NdisObjectHeaderSize

        MbbNdisSetUSSD,                                         // Set routine
        sizeof(NDIS_WWAN_USSD_REQUEST),                         // MinSetInBufferSize

        NULL,                                                   // Query routine
        0,                                                      // MinQueryInBufferSize
        0,                                                      // MinQueryOutBufferSize

        { MBB_UUID_USSD_CONSTANT, MBB_USSD_CID_USSD },          // ResponseCommand
        sizeof(MBB_USSD),                                       // ResponseBufferLength
        MbbNdisUSSDStatusHandler,                               // ResponseHandler
        NULL                                                    // CompletionHandler
    },
    //
    // DEVICE SERVICES
    //
    {
        OID_WWAN_ENUMERATE_DEVICE_SERVICES,                     // Oid

        FALSE,                                                  // IsSettable
        TRUE,                                                   // IsQueryable
        TRUE,                                                   // IsSerialized
        TRUE,                                                   // IsIndicationRequired
        TRUE,                                                   // CompleteRequestOnSendComplete

        0,                                                      // NdisObjectHeaderType
        0,                                                      // NdisObjectHeaderRevision
        0,                                                      // NdisObjectHeaderSize

        NULL,                                                   // Set routine
        0,                                                      // MinSetInBufferSize

        MbbNdisEnumerateDeviceServices,                         // Query routine
        0,                                                      // MinQueryInBufferSize
        0,                                                      // MinQueryOutBufferSize

        { MBB_UUID_BASIC_CONNECT_CONSTANT, MBB_BASIC_CID_DEVICE_SERVICES }, // ResponseCommand
        sizeof(MBB_DEVICE_SERVICES_HEADER),                     // ResponseBufferLength
        MbbNdisDeviceServiceListStatusHandler,                  // ResponseHandler
        NULL                                                    // CompletionHandler
    },
    {
        OID_WWAN_DEVICE_SERVICE_COMMAND,                        // Oid

        TRUE,                                                   // IsSettable
        TRUE,                                                   // IsQueryable
        TRUE,                                                   // IsSerialized
        TRUE,                                                   // IsIndicationRequired
        TRUE,                                                   // CompleteRequestOnSendComplete

        NDIS_OBJECT_TYPE_DEFAULT,                               // NdisObjectHeaderType
        NDIS_WWAN_DEVICE_SERVICE_COMMAND_REVISION_1,            // NdisObjectHeaderRevision
        SIZEOF_NDIS_WWAN_DEVICE_SERVICE_COMMAND_1,              // NdisObjectHeaderSize

        MbbNdisSetDeviceServiceCommand,                         // Set routine
        sizeof(NDIS_WWAN_DEVICE_SERVICE_COMMAND),               // MinSetInBufferSize

        MbbNdisGetDeviceServiceCommand,                         // Query routine
        sizeof(NDIS_WWAN_DEVICE_SERVICE_COMMAND),               // MinQueryInBufferSize
        0,                                                      // MinQueryOutBufferSize

        { MBB_UUID_INVALID_CONSTANT, 0 },                       // ResponseCommand
        0,                                                      // ResponseBufferLength
        MbbNdisDeviceServiceStatusHandler,                      // ResponseHandler
        NULL                                                    // CompletionHandler
    },
    {
        OID_WWAN_SUBSCRIBE_DEVICE_SERVICE_EVENTS,               // Oid

        TRUE,                                                   // IsSettable
        FALSE,                                                  // IsQueryable
        TRUE,                                                   // IsSerialized
        TRUE,                                                   // IsIndicationRequired
        TRUE,                                                   // CompleteRequestOnSendComplete

        NDIS_OBJECT_TYPE_DEFAULT,                               // NdisObjectHeaderType
        NDIS_WWAN_SUBSCRIBE_DEVICE_SERVICE_EVENTS_REVISION_1,   // NdisObjectHeaderRevision
        SIZEOF_NDIS_WWAN_SUBSCRIBE_DEVICE_SERVICE_EVENTS_1,     // NdisObjectHeaderSize

        MbbNdisSubscribeDeviceServiceEvents,                    // Set routine
        sizeof(NDIS_WWAN_SUBSCRIBE_DEVICE_SERVICE_EVENTS),      // MinSetInBufferSize

        NULL,                                                   // Query routine
        0,                                                      // MinQueryInBufferSize
        0,                                                      // MinQueryOutBufferSize

        { MBB_UUID_BASIC_CONNECT_CONSTANT, MBB_BASIC_CID_NOTIFY_DEVICE_SERVICE_UPDATES }, // ResponseCommand
        0,                                                      // ResponseBufferLength
        MbbNdisSubscribeDeviceServiceEventsStatusHandler,     // ResponseHandler
        NULL                                                    // CompletionHandler
    },
    {
        OID_WWAN_ENUMERATE_DEVICE_SERVICE_COMMANDS,             // Oid

        FALSE,                                                  // IsSettable
        TRUE,                                                   // IsQueryable
        FALSE,                                                  // IsSerialized
        TRUE,                                                   // IsIndicationRequired
        TRUE,                                                   // CompleteRequestOnSendComplete

        NDIS_OBJECT_TYPE_DEFAULT,                               // NdisObjectHeaderType
        NDIS_WWAN_ENUMERATE_DEVICE_SERVICE_COMMANDS_REVISION_1, // NdisObjectHeaderRevision
        SIZEOF_NDIS_WWAN_ENUMERATE_DEVICE_SERVICE_COMMANDS_1,   // NdisObjectHeaderSize

        NULL,                                                   // Set routine
        0,                                                      // MinSetInBufferSize

        MbbNdisEnumerateDeviceServiceCommands,                  // Query routine
        sizeof(NDIS_WWAN_ENUMERATE_DEVICE_SERVICE_COMMANDS),    // MinQueryInBufferSize
        0,                                                      // MinQueryOutBufferSize
        { MBB_UUID_BASIC_CONNECT_CONSTANT, MBB_BASIC_CID_MAXIMUM }, // ResponseCommand
        0,                                                      // ResponseBufferLength
        NULL,                                                   // ResponseHandler
        NULL                                                    // CompletionHandler
    },
    {
        OID_WWAN_DEVICE_SERVICE_SESSION_WRITE,                  // Oid

        TRUE,                                                   // IsSettable
        FALSE,                                                  // IsQueryable
        FALSE,                                                  // IsSerialized
        TRUE,                                                   // IsIndicationRequired = Indicate will complete
        TRUE,                                                   // CompleteRequestOnSendComplete

        NDIS_OBJECT_TYPE_DEFAULT,                               // NdisObjectHeaderType
        NDIS_WWAN_DEVICE_SERVICE_SESSION_WRITE_REVISION_1,      // NdisObjectHeaderRevision
        SIZEOF_NDIS_WWAN_DEVICE_SERVICE_SESSION_WRITE_1,        // NdisObjectHeaderSize

        MbbNdisDssWrite,                                        // Set routine
        sizeof(NDIS_WWAN_DEVICE_SERVICE_SESSION_WRITE),         // MinSetInBufferSize

        NULL,                                                   // Query routine
        0,                                                      // MinQueryInBufferSize
        0,                                                      // MinQueryOutBufferSize

        { MBB_UUID_INVALID_CONSTANT, 0 },                       // ResponseCommand
        0,                                                      // ResponseBufferLength
        MbbNdisDssWriteCompleteStatusHandler,                   // ResponseHandler
        NULL                                                    // CompletionHandler
    },
    {
        OID_WWAN_PREFERRED_MULTICARRIER_PROVIDERS,              // Oid

        TRUE,                                                   // IsSettable
        TRUE,                                                   // IsQueryable
        TRUE,                                                   // IsSerialized
        TRUE,                                                   // IsIndicationRequired
        TRUE,                                                   // CompleteRequestOnSendComplete

        NDIS_OBJECT_TYPE_DEFAULT,                               // NdisObjectHeaderType
        NDIS_WWAN_SET_PREFERRED_MULTICARRIER_PROVIDERS_REVISION_1,           // NdisObjectHeaderRevision
        sizeof(NDIS_WWAN_SET_PREFERRED_MULTICARRIER_PROVIDERS),              // NdisObjectHeaderSize

        MbbNdisSetMulticarrierProviders,                        // Set routine
        sizeof(NDIS_WWAN_SET_PREFERRED_MULTICARRIER_PROVIDERS), // MinSetInBufferSize

        MbbNdisGetMulticarrierProviders,                        // Query routine
        0,                                                      // MinQueryInBufferSize
        sizeof(NDIS_WWAN_PREFERRED_MULTICARRIER_PROVIDERS),     // MinQueryOutBufferSize

        { MBB_UUID_BASIC_CONNECT_CONSTANT, MBB_BASIC_CID_MULTICARRIER_PROVIDERS }, // ResponseCommand
        0,                                                      // ResponseBufferLength
        MbbNdisMulticarrierProviderStatusHandler,               // ResponseHandler
        NULL                                                    // CompletionHandler
    },
    //
    // CREATE/DELETE MAC
    //
    {
        OID_WWAN_CREATE_MAC,                                    // Oid

        FALSE,                                                  // IsSettable
        TRUE,                                                   // IsQueryable
        TRUE,                                                   // IsSerialized
        FALSE,                                                  // IsIndicationRequired
        TRUE,                                                   // CompleteRequestOnSendComplete

        NDIS_OBJECT_TYPE_DEFAULT,                               // NdisObjectHeaderType
        NDIS_WWAN_MAC_PARAMETERS_REVISION_1,                    // NdisObjectHeaderRevision
        sizeof(NDIS_WWAN_MAC_PARAMETERS),                       // NdisObjectHeaderSize

        NULL,                                                   // Set routine
        0,                                                      // MinSetInBufferSize

        MbbNdisCreateMac,                                       // Query routine
        SIZEOF_NDIS_WWAN_MAC_PARAMETERS_1,                      // MinQueryInBufferSize
        sizeof(NDIS_WWAN_MAC_INFO),                             // MinQueryOutBufferSize

        {MBB_UUID_INVALID_CONSTANT, 0 },                        // ResponseCommand
        0,                                                      // ResponseBufferLength
        NULL,                                                   // ResponseHandler
        MbbNdisCreateMacCompletionHandler                       // CompletionHandler
    },
    {
        OID_WWAN_DELETE_MAC,                                    // Oid

        TRUE,                                                   // IsSettable
        FALSE,                                                  // IsQueryable
        TRUE,                                                   // IsSerialized
        FALSE,                                                  // IsIndicationRequired
        TRUE,                                                   // CompleteRequestOnSendComplete

        NDIS_OBJECT_TYPE_DEFAULT,                               // NdisObjectHeaderType
        NDIS_WWAN_MAC_PARAMETERS_REVISION_1,                    // NdisObjectHeaderRevision
        sizeof(NDIS_WWAN_MAC_PARAMETERS),                       // NdisObjectHeaderSize

        MbbNdisDeleteMac,                                       // Set routine
        sizeof(NDIS_WWAN_MAC_INFO),                             // MinSetInBufferSize

        NULL,                                                   // Query routine
        0,                                                      // MinQueryInBufferSize
        0,                                                      // MinQueryOutBufferSize

        { MBB_UUID_INVALID_CONSTANT, 0 },                       // ResponseCommand
        0,                                                      // ResponseBufferLength
        NULL,                                                   // ResponseHandler
        MbbNdisDeleteMacCompletionHandler                       // CompletionHandler
    },
    //
    // MULTI-CARRIER
    //
    {
        OID_INVALID,                                            // Oid

        FALSE,                                                  // IsSettable
        TRUE,                                                   // IsQueryable
        TRUE,                                                   // IsSerialized
        TRUE,                                                   // IsIndicationRequired
        TRUE,                                                   // CompleteRequestOnSendComplete

        0,                                                      // NdisObjectHeaderType
        0,                                                      // NdisObjectHeaderRevision
        0,                                                      // NdisObjectHeaderSize

        NULL,                                                   // Set routine
        0,                                                      // MinSetInBufferSize

        MbbNdisGetMultiCarrierDsCidList,                        // Query routine
        0,                                                      // MinQueryInBufferSize
        0,                                                      // MinQueryOutBufferSize

        { MBB_UUID_MULTICARRIER_CONSTANT, MBB_MULTICARRIER_CID_CURRENT_CID_LIST },   // ResponseCommand
        0,                                                      // ResponseBufferLength
        MbbNdisMultiCarrierDsCidListStatusHandler,              // ResponseHandler
        MbbNdisMultiCarrierDsCidListCompletionHandler           // CompletionHandler
    },
    //
    // UICC
    //
    {
        OID_WWAN_UICC_APP_LIST,                                 // Oid

        FALSE,                                                  // IsSettable
        TRUE,                                                   // IsQueryable
        TRUE,                                                   // IsSerialized
        TRUE,                                                   // IsIndicationRequired
        TRUE,                                                   // CompleteRequestOnSendComplete

        NDIS_OBJECT_TYPE_DEFAULT,                               // NdisObjectHeaderType
        NDIS_WWAN_UICC_APP_LIST_REVISION_1,                     // NdisObjectHeaderRevision
        sizeof(NDIS_WWAN_UICC_APP_LIST),                        // NdisObjectHeaderSize

        NULL,                                                   // Set routine
        0,                                                      // MinSetInBufferSize

        MbbNdisGetUiccAppList,                                  // Query routine
        0,                                                      // MinQueryInBufferSize
        sizeof(NDIS_WWAN_UICC_APP_LIST),                        // MinQueryOutBufferSize

        { MBB_UUID_UICC_LOW_LEVEL_CONSTANT, MBB_UICC_CID_APP_LIST },   // ResponseCommand
        sizeof(MBB_UICC_APP_LIST),                              // ResponseBufferLength
        MbbNdisUiccAppListStatusHandler,                        // ResponseHandler
        NULL                                                    // CompletionHandler
    },
    {
        OID_WWAN_UICC_FILE_STATUS,                              // Oid

        FALSE,                                                  // IsSettable
        TRUE,                                                   // IsQueryable
        TRUE,                                                   // IsSerialized
        TRUE,                                                   // IsIndicationRequired
        TRUE,                                                   // CompleteRequestOnSendComplete

        NDIS_OBJECT_TYPE_DEFAULT,                               // NdisObjectHeaderType
        NDIS_WWAN_UICC_FILE_PATH_REVISION_1,                    // NdisObjectHeaderRevision
        sizeof(NDIS_WWAN_UICC_FILE_PATH),                       // NdisObjectHeaderSize

        NULL,                                                   // Set routine
        0,                                                      // MinSetInBufferSize

        MbbNdisGetUiccFileStatus,                               // Query routine
        sizeof(NDIS_WWAN_UICC_FILE_PATH),                       // MinQueryInBufferSize
        sizeof(NDIS_WWAN_UICC_FILE_STATUS),                     // MinQueryOutBufferSize

        { MBB_UUID_UICC_LOW_LEVEL_CONSTANT, MBB_UICC_CID_FILE_STATUS },   // ResponseCommand
        sizeof(MBB_UICC_FILE_STATUS),                           // ResponseBufferLength
        MbbNdisUiccFileStatusHandler,                           // ResponseHandler
        NULL                                                    // CompletionHandler
    },
    {
        OID_WWAN_UICC_ACCESS_BINARY,                            // Oid

        TRUE,                                                   // IsSettable
        TRUE,                                                   // IsQueryable
        TRUE,                                                   // IsSerialized
        TRUE,                                                   // IsIndicationRequired
        TRUE,                                                   // CompleteRequestOnSendComplete

        NDIS_OBJECT_TYPE_DEFAULT,                               // NdisObjectHeaderType
        NDIS_WWAN_UICC_ACCESS_BINARY_REVISION_1,                // NdisObjectHeaderRevision
        sizeof(NDIS_WWAN_UICC_ACCESS_BINARY),                   // NdisObjectHeaderSize

        MbbNdisUiccUpdateBinary,                                // Set routine
        sizeof(NDIS_WWAN_UICC_ACCESS_BINARY),                   // MinSetInBufferSize

        MbbNdisUiccReadBinary,                                  // Query routine
        sizeof(NDIS_WWAN_UICC_ACCESS_BINARY),                   // MinQueryInBufferSize
        sizeof(NDIS_WWAN_UICC_RESPONSE),                        // MinQueryOutBufferSize

        { MBB_UUID_UICC_LOW_LEVEL_CONSTANT, MBB_UICC_CID_ACCESS_BINARY }, // ResponseCommand
        sizeof(MBB_UICC_RESPONSE),                              // ResponseBufferLength
        MbbNdisUiccAccessStatusHandler,                         // ResponseHandler
        NULL                                                    // CompletionHandler
    },
    {
        OID_WWAN_UICC_ACCESS_RECORD,                            // Oid

        TRUE,                                                   // IsSettable
        TRUE,                                                   // IsQueryable
        TRUE,                                                   // IsSerialized
        TRUE,                                                   // IsIndicationRequired
        TRUE,                                                   // CompleteRequestOnSendComplete

        NDIS_OBJECT_TYPE_DEFAULT,                               // NdisObjectHeaderType
        NDIS_WWAN_UICC_ACCESS_RECORD_REVISION_1,                // NdisObjectHeaderRevision
        sizeof(NDIS_WWAN_UICC_ACCESS_RECORD),                   // NdisObjectHeaderSize

        MbbNdisUiccUpdateRecord,                                // Set routine
        sizeof(NDIS_WWAN_UICC_ACCESS_RECORD),                   // MinSetInBufferSize

        MbbNdisUiccReadRecord,                                  // Query routine
        sizeof(NDIS_WWAN_UICC_ACCESS_RECORD),                   // MinQueryInBufferSize
        sizeof(NDIS_WWAN_UICC_RESPONSE),                        // MinQueryOutBufferSize

        { MBB_UUID_UICC_LOW_LEVEL_CONSTANT, MBB_UICC_CID_ACCESS_RECORD }, // ResponseCommand
        sizeof(MBB_UICC_RESPONSE),                              // ResponseBufferLength
        MbbNdisUiccAccessStatusHandler,                         // ResponseHandler
        NULL                                                    // CompletionHandler
    },
    //
    // UICC Security in MBIM 2.0
    //
    {
        OID_WWAN_PIN_EX2,                                       // Oid

        TRUE,                                                   // IsSettable
        TRUE,                                                   // IsQueryable
        TRUE,                                                   // IsSerialized
        TRUE,                                                   // IsIndicationRequired
        TRUE,                                                   // CompleteRequestOnSendComplete

        NDIS_OBJECT_TYPE_DEFAULT,                               // NdisObjectHeaderType
        NDIS_WWAN_SET_PIN_REVISION_3,                           // NdisObjectHeaderRevision
        sizeof(NDIS_WWAN_SET_PIN_EX2),                          // NdisObjectHeaderSize

        MbbNdisSetPinInfoEx2,                                   // Set routine
        sizeof(NDIS_WWAN_SET_PIN_EX2),                          // MinSetInBufferSize

        MbbNdisGetPinInfoEx,                                    // Query routine
        sizeof(NDIS_WWAN_PIN_APP),                              // MinQueryInBufferSize
        sizeof(NDIS_WWAN_PIN_INFO),                             // MinQueryOutBufferSize

        { MBB_UUID_BASIC_CONNECT_EXT_CONSTANT, MBB_BASICCONNECTEXT_CID_PIN_INFO_EX2 },  // ResponseCommand
        sizeof(MBB_PIN_INFO),                                   // ResponseBufferLength
        MbbNdisPinInfoStatusHandler,                            // ResponseHandler
        NULL                                                    // CompletionHandler
    },
    //
    // MBIM-VERSION
    //
    {
        OID_WWAN_MBIM_VERSION,                                  // Oid

        FALSE,                                                  // IsSettable
        TRUE,                                                   // IsQueryable
        FALSE,                                                  // IsSerialized
        FALSE,                                                  // IsIndicationRequired
        TRUE,                                                   // CompleteRequestOnSendComplete

        NDIS_OBJECT_TYPE_DEFAULT,                               // NdisObjectHeaderType
        NDIS_WWAN_MBIM_VERSION_REVISION_1,                      // NdisObjectHeaderRevision
        sizeof(NDIS_WWAN_MBIM_VERSION),                         // NdisObjectHeaderSize

        NULL,                                                   // Set routine
        0,                                                      // MinSetInBufferSize

        MbbNdisGetMbimVersion,                                  // Query routine
        0,                                                      // MinQueryInBufferSize
        sizeof(USHORT),                                         // MinQueryOutBufferSize

        { MBB_UUID_INVALID_CONSTANT, 0 },                       // ResponseCommand
        0,                                                      // ResponseBufferLength
        NULL,                                                   // ResponseHandler
        NULL                                                    // CompletionHandler
    },
    //
    // Device State and Configuration for MBIM 2.0
    //
    {
        OID_WWAN_SYS_CAPS,                                      // Oid

        FALSE,                                                  // IsSettable
        TRUE,                                                   // IsQueryable
        TRUE,                                                   // IsSerialized
        TRUE,                                                   // IsIndicationRequired
        TRUE,                                                   // CompleteRequestOnSendComplete

        0,                                                      // NdisObjectHeaderType
        0,                                                      // NdisObjectHeaderRevision
        0,                                                      // NdisObjectHeaderSize

        NULL,                                                   // Set routine
        0,                                                      // MinSetInBufferSize

        MbbNdisGetSysCapsInfo,                                  // Query routine
        0,                                                      // MinQueryInBufferSize
        sizeof(NDIS_WWAN_SYS_CAPS_INFO),                        // MinQueryOutBufferSize

        { MBB_UUID_BASIC_CONNECT_EXT_CONSTANT, MBB_BASICCONNECTEXT_CID_SYS_CAPS },     // ResponseCommand
        sizeof(MBB_MS_SYS_CAPS_INFO),                           // ResponseBufferLength
        MbbNdisSysCapsStatusHandler,                            // ResponseHandler
        NULL                                                    // CompletionHandler
    },
    {
        OID_WWAN_DEVICE_CAPS_EX,                                // Oid

        FALSE,                                                  // IsSettable
        TRUE,                                                   // IsQueryable
        TRUE,                                                   // IsSerialized
        TRUE,                                                   // IsIndicationRequired
        TRUE,                                                   // CompleteRequestOnSendComplete

        0,                                                      // NdisObjectHeaderType
        0,                                                      // NdisObjectHeaderRevision
        0,                                                      // NdisObjectHeaderSize

        NULL,                                                   // Set routine
        0,                                                      // MinSetInBufferSize

        MbbNdisGetDeviceCapsEx,                                 // Query routine
        0,                                                      // MinQueryInBufferSize
        sizeof(NDIS_WWAN_DEVICE_CAPS_EX),                       // MinQueryOutBufferSize

        { MBB_UUID_BASIC_CONNECT_EXT_CONSTANT, MBB_BASICCONNECTEXT_CID_DEVICE_CAPS_V2 },     // ResponseCommand
        sizeof(MBB_MS_DEVICE_CAPS_INFO_V2),                     // ResponseBufferLength
        MbbNdisDeviceCapsExStatusHandler,                       // ResponseHandler
        NULL                                                    // CompletionHandler
    },
    {
        OID_WWAN_SYS_SLOTMAPPINGS,                              // Oid

        TRUE,                                                   // IsSettable
        TRUE,                                                   // IsQueryable
        TRUE,                                                   // IsSerialized
        TRUE,                                                   // IsIndicationRequired
        TRUE,                                                   // CompleteRequestOnSendComplete

        NDIS_OBJECT_TYPE_DEFAULT,                               // NdisObjectHeaderType
        NDIS_WWAN_SET_DEVICE_SLOT_MAPPING_INFO_REVISION_1,      // NdisObjectHeaderRevision
        sizeof(NDIS_WWAN_SET_DEVICE_SLOT_MAPPING_INFO),         // NdisObjectHeaderSize

        MbbNdisSetSysSlotMappings,                              // Set routine
        sizeof(NDIS_WWAN_SET_DEVICE_SLOT_MAPPING_INFO),         // MinSetInBufferSize

        MbbNdisGetSysSlotMappings,                              // Query routine
        0,                                                      // MinQueryInBufferSize
        sizeof(NDIS_WWAN_DEVICE_SLOT_MAPPING_INFO),             // MinQueryOutBufferSize

        { MBB_UUID_BASIC_CONNECT_EXT_CONSTANT, MBB_BASICCONNECTEXT_CID_DEVICE_SLOT_MAPPINGS },     // ResponseCommand
        sizeof(MBB_MS_DEVICE_SLOT_MAPPING_INFO),                // ResponseBufferLength
        MbbNdisSysSlotMappingsStatusHandler,                    // ResponseHandler
        NULL                                                    // CompletionHandler
    },
    {
        OID_WWAN_SLOT_INFO_STATUS,                              // Oid

        FALSE,                                                  // IsSettable
        TRUE,                                                   // IsQueryable
        TRUE,                                                   // IsSerialized
        TRUE,                                                   // IsIndicationRequired
        TRUE,                                                   // CompleteRequestOnSendComplete

        0,                                                      // NdisObjectHeaderType
        0,                                                      // NdisObjectHeaderRevision
        0,                                                      // NdisObjectHeaderSize

        NULL,                                                   // Set routine
        0,                                                      // MinSetInBufferSize

        MbbNdisGetSlotInfoStatus,                               // Query routine
        sizeof(NDIS_WWAN_GET_SLOT_INFO),                        // MinQueryInBufferSize
        sizeof(NDIS_WWAN_SLOT_INFO),                            // MinQueryOutBufferSize

        { MBB_UUID_BASIC_CONNECT_EXT_CONSTANT, MBB_BASICCONNECTEXT_CID_SLOT_INFO_STATUS },     // ResponseCommand
        sizeof(MBB_MS_SLOT_INFO),                               // ResponseBufferLength
        MbbNdisSlotInfoStatusHandler,                           // ResponseHandler
        NULL                                                    // CompletionHandler
    },
    {
        OID_WWAN_DEVICE_BINDINGS,                               // Oid

        TRUE,                                                   // IsSettable
        TRUE,                                                   // IsQueryable
        TRUE,                                                   // IsSerialized
        TRUE,                                                   // IsIndicationRequired
        TRUE,                                                   // CompleteRequestOnSendComplete

        NDIS_OBJECT_TYPE_DEFAULT,                               // NdisObjectHeaderType
        NDIS_WWAN_SET_DEVICE_BINDINGS_INFO_REVISION_1,          // NdisObjectHeaderRevision
        sizeof(NDIS_WWAN_SET_DEVICE_BINDINGS_INFO),             // NdisObjectHeaderSize

        MbbNdisSetDeviceBindings,                               // Set routine
        sizeof(NDIS_WWAN_SET_DEVICE_BINDINGS_INFO),             // MinSetInBufferSize

        MbbNdisGetDeviceBindings,                               // Query routine
        0,                                                      // MinQueryInBufferSize
        sizeof(NDIS_WWAN_DEVICE_BINDINGS_INFO),                 // MinQueryOutBufferSize

        { MBB_UUID_VOICEEXTENSIONS_CONSTANT, MBB_VOICEEXTENSIONS_CID_DEVICE_BINDINGS },     // ResponseCommand
        sizeof(MBB_DEVICE_BINDINGS_INFO),                       // ResponseBufferLength
        MbbNdisDeviceBindingsStatusHandler,                     // ResponseHandler
        NULL                                                    // CompletionHandler
    },
    //
    // Registration, Signal Strength, and NITZ for MBIM 2.0
    //
    {
        OID_WWAN_REGISTER_STATE_EX,                             // Oid

        TRUE,                                                   // IsSettable
        TRUE,                                                   // IsQueryable
        TRUE,                                                   // IsSerialized
        TRUE,                                                   // IsIndicationRequired
        TRUE,                                                   // CompleteRequestOnSendComplete

        NDIS_OBJECT_TYPE_DEFAULT,                               // NdisObjectHeaderType
        NDIS_WWAN_SET_REGISTER_STATE_REVISION_2,                // NdisObjectHeaderRevision
        sizeof(NDIS_WWAN_SET_REGISTER_STATE_EX),                // NdisObjectHeaderSize

        MbbNdisSetRegisterState,                                // Set routine
        sizeof(NDIS_WWAN_SET_REGISTER_STATE_EX),                // MinSetInBufferSize

        MbbNdisGetRegisterStateEx,                              // Query routine
        0,                                                      // MinQueryInBufferSize
        sizeof(NDIS_WWAN_REGISTRATION_STATE_INFOS_EX),          // MinQueryOutBufferSize

        { MBB_UUID_VOICEEXTENSIONS_CONSTANT, MBB_VOICEEXTENSIONS_CID_REGISTER_STATE_V2 },     // ResponseCommand
        sizeof(MBB_REGISTRATION_STATE_INFOS_V2),                // ResponseBufferLength
        MbbNdisRegisterStateExStatusHandler,                    // ResponseHandler
        NULL                                                    // CompletionHandler
    },
    {
        OID_WWAN_IMS_VOICE_STATE,                               // Oid

        TRUE,                                                   // IsSettable
        TRUE,                                                   // IsQueryable
        TRUE,                                                   // IsSerialized
        TRUE,                                                   // IsIndicationRequired
        TRUE,                                                   // CompleteRequestOnSendComplete

        NDIS_OBJECT_TYPE_DEFAULT,                               // NdisObjectHeaderType
        NDIS_WWAN_SET_IMS_VOICE_STATE_REVISION_1,               // NdisObjectHeaderRevision
        sizeof(NDIS_WWAN_SET_IMS_VOICE_STATE),                  // NdisObjectHeaderSize

        MbbNdisSetImsVoiceState,                                // Set routine
        sizeof(NDIS_WWAN_SET_IMS_VOICE_STATE),                  // MinSetInBufferSize

        MbbNdisGetImsVoiceState,                                // Query routine
        0,                                                      // MinQueryInBufferSize
        sizeof(NDIS_WWAN_IMS_VOICE_STATE),                      // MinQueryOutBufferSize

        { MBB_UUID_VOICEEXTENSIONS_CONSTANT, MBB_VOICEEXTENSIONS_CID_IMS_VOICE_STATE },     // ResponseCommand
        sizeof(MBB_IMS_VOICE_STATE),                            // ResponseBufferLength
        MbbNdisImsVoiceStateStatusHandler,                      // ResponseHandler
        NULL                                                    // CompletionHandler
    },
    {
        OID_WWAN_SIGNAL_STATE_EX,                               // Oid

        TRUE,                                                   // IsSettable
        TRUE,                                                   // IsQueryable
        TRUE,                                                   // IsSerialized
        TRUE,                                                   // IsIndicationRequired
        TRUE,                                                   // CompleteRequestOnSendComplete

        NDIS_OBJECT_TYPE_DEFAULT,                               // NdisObjectHeaderType
        NDIS_WWAN_SET_SIGNAL_INDICATION_REVISION_2,             // NdisObjectHeaderRevision
        sizeof(NDIS_WWAN_SET_SIGNAL_INDICATION_EX),             // NdisObjectHeaderSize

        MbbNdisSetSignalStateEx,                                // Set routine
        sizeof(NDIS_WWAN_SET_SIGNAL_INDICATION_EX),             // MinSetInBufferSize

        MbbNdisGetSignalStateEx,                                // Query routine
        0,                                                      // MinQueryInBufferSize
        sizeof(NDIS_WWAN_SIGNAL_STATE_INFOS_EX),                // MinQueryOutBufferSize

        { MBB_UUID_VOICEEXTENSIONS_CONSTANT, MBB_VOICEEXTENSIONS_CID_SIGNAL_STATE_V2 },     // ResponseCommand
        sizeof(MBB_SIGNAL_STATE_INFOS_V2),                      // ResponseBufferLength
        MbbNdisSignalStateExStatusHandler,                      // ResponseHandler
        NULL                                                    // CompletionHandler
    },
    {
        OID_WWAN_LOCATION_STATE,                                // Oid

        TRUE,                                                   // IsSettable
        TRUE,                                                   // IsQueryable
        TRUE,                                                   // IsSerialized
        TRUE,                                                   // IsIndicationRequired
        TRUE,                                                   // CompleteRequestOnSendComplete

        NDIS_OBJECT_TYPE_DEFAULT,                               // NdisObjectHeaderType
        NDIS_WWAN_LOCATION_TRIGGER_MODE_REVISION_1,             // NdisObjectHeaderRevision
        sizeof(NDIS_WWAN_LOCATION_TRIGGER_MODE),                // NdisObjectHeaderSize

        MbbNdisSetLocationState,                                // Set routine
        sizeof(NDIS_WWAN_LOCATION_TRIGGER_MODE),                // MinSetInBufferSize

        MbbNdisGetLocationState,                                // Query routine
        0,                                                      // MinQueryInBufferSize
        sizeof(NDIS_WWAN_LOCATION_STATE_INFO),                  // MinQueryOutBufferSize

        { MBB_UUID_VOICEEXTENSIONS_CONSTANT, MBB_VOICEEXTENSIONS_CID_LOCATION_STATE },     // ResponseCommand
        sizeof(MBB_LOCATION_STATE_INFO),                        // ResponseBufferLength
        MbbNdisLocationStateStatusHandler,                      // ResponseHandler
        NULL                                                    // CompletionHandler
    },
    {
        OID_WWAN_NITZ,                                          // Oid

        FALSE,                                                  // IsSettable
        TRUE,                                                   // IsQueryable
        TRUE,                                                   // IsSerialized
        TRUE,                                                   // IsIndicationRequired
        TRUE,                                                   // CompleteRequestOnSendComplete

        0,                                                      // NdisObjectHeaderType
        0,                                                      // NdisObjectHeaderRevision
        0,                                                      // NdisObjectHeaderSize

        NULL,                                                   // Set routine
        0,                                                      // MinSetInBufferSize

        MbbNdisGetNitzInfo,                                     // Query routine
        0,                                                      // MinQueryInBufferSize
        sizeof(NDIS_WWAN_NITZ_INFO),                            // MinQueryOutBufferSize

        { MBB_UUID_VOICEEXTENSIONS_CONSTANT, MBB_VOICEEXTENSIONS_CID_NITZ },     // ResponseCommand
        sizeof(MBB_NITZ_INFO),                                  // ResponseBufferLength
        MbbNdisNitzStatusHandler,                               // ResponseHandler
        NULL                                                    // CompletionHandler
    },
    //
    // For MBIM vendor specific
    //
    {
        OID_WWAN_VENDOR_SPECIFIC,                               // Oid

        TRUE,                                                   // IsSettable
        FALSE,                                                  // IsQueryable
        TRUE,                                                   // IsSerialized
        TRUE,                                                   // IsIndicationRequired
        TRUE,                                                   // CompleteRequestOnSendComplete

        NDIS_OBJECT_TYPE_DEFAULT,                               // NdisObjectHeaderType
        NDIS_WWAN_VENDOR_SPECIFIC_REVISION_1,                   // NdisObjectHeaderRevision
        sizeof(NDIS_WWAN_VENDOR_SPECIFIC),                      // NdisObjectHeaderSize

        MbbNdisSetVendorSpecific,                               // Set routine
        sizeof(NDIS_WWAN_VENDOR_SPECIFIC),                      // MinSetInBufferSize

        NULL,                                                   // Query routine
        0,                                                      // MinQueryInBufferSize
        0,                                                      // MinQueryOutBufferSize

        { MBB_UUID_MS_VENDOR_EXTENSION_CONSTANT, MBIM_CID_VENDORSPECIFIC },  // ResponseCommand
        0,                                                      // ResponseBufferLength
        MbbNdisVendorSpecificStatusHandler,                     // ResponseHandler
        NULL                                                    // CompletionHandler
    },
    //
    // Network Idle Hint
    //
    {
        OID_WWAN_NETWORK_IDLE_HINT,                             // Oid

        TRUE,                                                   // IsSettable
        FALSE,                                                  // IsQueryable
        FALSE,                                                  // IsSerialized
        FALSE,                                                  // IsIndicationRequired
        FALSE,                                                  // CompleteRequestOnSendComplete

        NDIS_OBJECT_TYPE_DEFAULT,                               // NdisObjectHeaderType
        NDIS_WWAN_NETWORK_IDLE_HINT_REVISION_1,                 // NdisObjectHeaderRevision
        sizeof(NDIS_WWAN_NETWORK_IDLE_HINT),                    // NdisObjectHeaderSize

        MbbNdisSetNetworkIdleHint,                              // Set routine
        sizeof(NDIS_WWAN_NETWORK_IDLE_HINT),                    // MinSetInBufferSize

        NULL,                                                   // Query routine
        0,                                                      // MinQueryInBufferSize
        0,                                                      // MinQueryOutBufferSize

        { MBB_UUID_INVALID_CONSTANT, 0 },                       // ResponseCommand
        0,                                                      // ResponseBufferLength
        NULL,                                                   // ResponseHandler
        NULL                                                    // CompletionHandler
    },
    //
    // System Preshutdown
    //
    {
        OID_WWAN_PRESHUTDOWN,                                   // Oid

        TRUE,                                                   // IsSettable
        FALSE,                                                  // IsQueryable
        TRUE,                                                   // IsSerialized
        TRUE,                                                   // IsIndicationRequired
        TRUE,                                                   // CompleteRequestOnSendComplete

        NDIS_OBJECT_TYPE_DEFAULT,                               // NdisObjectHeaderType
        NDIS_WWAN_SET_PRESHUTDOWN_STATE_REVISION_1,             // NdisObjectHeaderRevision
        SIZEOF_NDIS_WWAN_SET_PRESHUTDOWN_STATE_1,               // NdisObjectHeaderSize

        MbbNdisSetPreshutdown,                                  // Set routine
        SIZEOF_NDIS_WWAN_SET_PRESHUTDOWN_STATE_1,               // MinSetInBufferSize

        NULL,                                                   // Query routine
        0,                                                      // MinQueryInBufferSize
        0,                                                      // MinQueryOutBufferSize

        { MBB_UUID_HOSTSHUTDOWN_CONSTANT, MBB_HOSTSHUTDOWN_CID_PRESHUTDOWN },   // ResponseCommand
        SIZEOF_NDIS_WWAN_PRESHUTDOWN_STATE_1,                   // ResponseBufferLength
        MbbNdisPreshutdownHandler,                              // ResponseHandler
        NULL                                                    // CompletionHandler
    },
    //
    // Low-Level UICC Access
    //
    {
        OID_WWAN_UICC_ATR,                                      // Oid

        FALSE,                                                  // IsSettable
        TRUE,                                                   // IsQueryable
        TRUE,                                                   // IsSerialized
        TRUE,                                                   // IsIndicationRequired
        TRUE,                                                   // CompleteRequestOnSendComplete

        0,                                                      // NdisObjectHeaderType
        0,                                                      // NdisObjectHeaderRevision
        0,                                                      // NdisObjectHeaderSize

        NULL,                                                   // Set routine
        0,                                                      // MinSetInBufferSize

        MbbNdisGetAtr,                                          // Query routine
        0,                                                      // MinQueryInBufferSize
        sizeof(NDIS_WWAN_ATR_INFO),                             // MinQueryOutBufferSize

        { MBB_UUID_UICC_LOW_LEVEL_CONSTANT, MBB_UICC_CID_ATR }, // ResponseCommand
        sizeof(MBB_ATR_INFO),                                   // ResponseBufferLength
        MbbNdisAtrInfoStatusHandler,                            // ResponseHandler
        NULL                                                    // CompletionHandler
    },
    {
        OID_WWAN_UICC_OPEN_CHANNEL,                             // Oid

        TRUE,                                                   // IsSettable
        FALSE,                                                  // IsQueryable
        TRUE,                                                   // IsSerialized
        TRUE,                                                   // IsIndicationRequired
        TRUE,                                                   // CompleteRequestOnSendComplete

        NDIS_OBJECT_TYPE_DEFAULT,                               // NdisObjectHeaderType
        NDIS_WWAN_SET_UICC_OPEN_CHANNEL_REVISION_1,             // NdisObjectHeaderRevision
        sizeof(NDIS_WWAN_SET_UICC_OPEN_CHANNEL),                // NdisObjectHeaderSize

        MbbNdisSetUiccOpenChannel,                              // Set routine
        sizeof(NDIS_WWAN_SET_UICC_OPEN_CHANNEL),                // MinSetInBufferSize

        NULL,                                                   // Query routine
        0,                                                      // MinQueryInBufferSize
        0,                                                      // MinQueryOutBufferSize

        { MBB_UUID_UICC_LOW_LEVEL_CONSTANT, MBB_UICC_CID_OPEN_CHANNEL }, // ResponseCommand
        sizeof(MBB_UICC_OPEN_CHANNEL_INFO),                     // ResponseBufferLength
        MbbNdisUiccOpenChannelInfoStatusHandler,                // ResponseHandler
        NULL                                                    // CompletionHandler
    },
    {
        OID_WWAN_UICC_CLOSE_CHANNEL,                            // Oid

        TRUE,                                                   // IsSettable
        FALSE,                                                  // IsQueryable
        TRUE,                                                   // IsSerialized
        TRUE,                                                   // IsIndicationRequired
        TRUE,                                                   // CompleteRequestOnSendComplete

        NDIS_OBJECT_TYPE_DEFAULT,                               // NdisObjectHeaderType
        NDIS_WWAN_SET_UICC_CLOSE_CHANNEL_REVISION_1,            // NdisObjectHeaderRevision
        sizeof(NDIS_WWAN_SET_UICC_CLOSE_CHANNEL),               // NdisObjectHeaderSize

        MbbNdisSetUiccCloseChannel,                             // Set routine
        sizeof(NDIS_WWAN_SET_UICC_CLOSE_CHANNEL),               // MinSetInBufferSize

        NULL,                                                   // Query routine
        0,                                                      // MinQueryInBufferSize
        0,                                                      // MinQueryOutBufferSize

        { MBB_UUID_UICC_LOW_LEVEL_CONSTANT, MBB_UICC_CID_CLOSE_CHANNEL }, // ResponseCommand
        sizeof(MBB_UICC_CLOSE_CHANNEL_INFO),                    // ResponseBufferLength
        MbbNdisUiccCloseChannelInfoStatusHandler,               // ResponseHandler
        NULL                                                    // CompletionHandler
    },
    {
        OID_WWAN_UICC_APDU,                                     // Oid

        TRUE,                                                   // IsSettable
        FALSE,                                                  // IsQueryable
        TRUE,                                                   // IsSerialized
        TRUE,                                                   // IsIndicationRequired
        TRUE,                                                   // CompleteRequestOnSendComplete

        NDIS_OBJECT_TYPE_DEFAULT,                               // NdisObjectHeaderType
        NDIS_WWAN_SET_UICC_APDU_REVISION_1,                     // NdisObjectHeaderRevision
        sizeof(NDIS_WWAN_SET_UICC_APDU),                        // NdisObjectHeaderSize

        MbbNdisSetUiccApdu,                                     // Set routine
        sizeof(NDIS_WWAN_SET_UICC_APDU),                        // MinSetInBufferSize

        NULL,                                                   // Query routine
        0,                                                      // MinQueryInBufferSize
        0,                                                      // MinQueryOutBufferSize

        { MBB_UUID_UICC_LOW_LEVEL_CONSTANT, MBB_UICC_CID_APDU },// ResponseCommand
        sizeof(MBB_UICC_APDU_INFO),                             // ResponseBufferLength
        MbbNdisUiccApduInfoStatusHandler,                       // ResponseHandler
        NULL                                                    // CompletionHandler
    },
    {
        OID_WWAN_UICC_TERMINAL_CAPABILITY,                      // Oid

        TRUE,                                                   // IsSettable
        TRUE,                                                   // IsQueryable
        TRUE,                                                   // IsSerialized
        TRUE,                                                   // IsIndicationRequired
        TRUE,                                                   // CompleteRequestOnSendComplete

        NDIS_OBJECT_TYPE_DEFAULT,                               // NdisObjectHeaderType
        NDIS_WWAN_SET_UICC_TERMINAL_CAPABILITY_REVISION_1,      // NdisObjectHeaderRevision
        sizeof(NDIS_WWAN_SET_UICC_TERMINAL_CAPABILITY),         // NdisObjectHeaderSize

        MbbNdisSetUiccTerminalCapability,                       // Set routine
        sizeof(NDIS_WWAN_SET_UICC_TERMINAL_CAPABILITY),         // MinSetInBufferSize

        MbbNdisGetUiccTerminalCapability,                       // Query routine
        0,                                                      // MinQueryInBufferSize
        sizeof(NDIS_WWAN_UICC_TERMINAL_CAPABILITY_INFO),        // MinQueryOutBufferSize

        { MBB_UUID_UICC_LOW_LEVEL_CONSTANT, MBB_UICC_CID_TERMINAL_CAPABILITY }, // ResponseCommand
        sizeof(MBB_UICC_TERMINAL_CAPABILITY_INFO),              // ResponseBufferLength
        MbbNdisUiccTerminalCapabilityInfoStatusHandler,         // ResponseHandler
        NULL                                                    // CompletionHandler
    },
    //
    // SAR backoff
    //
    {
        OID_WWAN_SAR_CONFIG,                                    // Oid

        TRUE,                                                   // IsSettable
        TRUE,                                                   // IsQueryable
        TRUE,                                                   // IsSerialized
        TRUE,                                                   // IsIndicationRequired
        TRUE,                                                   // CompleteRequestOnSendComplete

        NDIS_OBJECT_TYPE_DEFAULT,                               // NdisObjectHeaderType
        NDIS_WWAN_SET_SAR_CONFIG_REVISON_1,                     // NdisObjectHeaderRevision
        sizeof(NDIS_WWAN_SET_SAR_CONFIG),                       // NdisObjectHeaderSize

        MbbNdisSetSarConfig,                                    // Set routine
        sizeof(NDIS_WWAN_SET_SAR_CONFIG),                       // MinSetInBufferSize

        MbbNdisGetSarConfig,                                    // Query routine
        0,                                                      // MinQueryInBufferSize
        sizeof(NDIS_WWAN_SAR_CONFIG_INFO),                      // MinQueryOutBufferSize

        { MBB_UUID_SARCONTROL_CONSTANT, MBB_SAR_CID_CONFIG },   // ResponseCommand
        sizeof(MBB_SAR_CONFIG_INFO),                            // ResponseBufferLength
        MbbNdisSarConfigHandler,                                // ResponseHandler
        NULL                                                    // CompletionHandler
    },
    {
        OID_WWAN_SAR_TRANSMISSION_STATUS,                       // Oid

        TRUE,                                                   // IsSettable
        TRUE,                                                   // IsQueryable
        TRUE,                                                   // IsSerialized
        TRUE,                                                   // IsIndicationRequired
        TRUE,                                                   // CompleteRequestOnSendComplete

        NDIS_OBJECT_TYPE_DEFAULT,                               // NdisObjectHeaderType
        NDIS_WWAN_SET_SAR_TRANSMISSION_STATUS_REVISION_1,        // NdisObjectHeaderRevision
        sizeof(NDIS_WWAN_SET_SAR_TRANSMISSION_STATUS),          // NdisObjectHeaderSize

        MbbNdisSetSarTransmissionStatus,                        // Set routine
        sizeof(NDIS_WWAN_SET_SAR_TRANSMISSION_STATUS),          // MinSetInBufferSize

        MbbNdisGetSarTransmissionStatus,                        // Query routine
        0,                                                      // MinQueryInBufferSize
        sizeof(NDIS_WWAN_SAR_TRANSMISSION_STATUS_INFO),         // MinQueryOutBufferSize

        { MBB_UUID_SARCONTROL_CONSTANT, MBB_SAR_CID_TRANSMISSION_STATUS }, // ResponseCommand
        sizeof(MBB_SAR_TRANSMISSION_STATUS_INFO),               // ResponseBufferLength
        MbbNdisSarTransmissionStatusHandler,                    // ResponseHandler
        NULL                                                    // CompletionHandler
    },
    {
        OID_WWAN_LTE_ATTACH_CONFIG,                             // Oid

        TRUE,                                                   // IsSettable
        TRUE,                                                   // IsQueryable
        TRUE,                                                   // IsSerialized
        TRUE,                                                   // IsIndicationRequired
        TRUE,                                                   // CompleteRequestOnSendComplete

        NDIS_OBJECT_TYPE_DEFAULT,                               // NdisObjectHeaderType
        NDIS_WWAN_SET_LTE_ATTACH_CONTEXT_REVISION_1,            // NdisObjectHeaderRevision
        sizeof(NDIS_WWAN_SET_LTE_ATTACH_CONTEXT),               // NdisObjectHeaderSize

        MbbNdisSetLteAttachContexts,                            // Set routine
        sizeof(NDIS_WWAN_SET_LTE_ATTACH_CONTEXT),               // MinSetInBufferSize

        MbbNdisGetLteAttachConfig,                              // Query routine
        0,                                                      // MinQueryInBufferSize
        sizeof(NDIS_WWAN_LTE_ATTACH_CONTEXTS),                  // MinQueryOutBufferSize

        { MBB_UUID_BASIC_CONNECT_EXT_CONSTANT, MBB_BASICCONNECTEXT_CID_LTE_ATTACH_CONFIG }, // ResponseCommand
        sizeof(MBB_MS_LTE_ATTACH_CONFIG_INFO),                  // ResponseBufferLength
        MbbNdisLteAttachConfigHandler,                         // ResponseHandler
        NULL                                                    // CompletionHandler
    },
    {
        OID_WWAN_LTE_ATTACH_STATUS,                             // Oid

        FALSE,                                                  // IsSettable
        TRUE,                                                   // IsQueryable
        TRUE,                                                   // IsSerialized
        TRUE,                                                   // IsIndicationRequired
        TRUE,                                                   // CompleteRequestOnSendComplete

        0,                                                      // NdisObjectHeaderType
        0,                                                      // NdisObjectHeaderRevision
        0,                                                      // NdisObjectHeaderSize

        NULL,                                                   // Set routine
        0,                                                      // MinSetInBufferSize

        MbbNdisGetLteAttachStatus,                              // Query routine
        0,                                                      // MinQueryInBufferSize
        sizeof(NDIS_WWAN_SYS_CAPS_INFO),                        // MinQueryOutBufferSize

        { MBB_UUID_BASIC_CONNECT_EXT_CONSTANT, MBB_BASICCONNECTEXT_CID_LTE_ATTACH_STATUS }, // ResponseCommand
        sizeof(MBB_MS_LTE_ATTACH_STATUS),                       // ResponseBufferLength
        MbbNdisLteAttachStatusHandler,                          // ResponseHandler
        NULL                                                    // CompletionHandler
    },
    {
        OID_WWAN_NETWORK_BLACKLIST,                          // Oid

        TRUE,                                                   // IsSettable
        TRUE,                                                   // IsQueryable
        TRUE,                                                   // IsSerialized
        TRUE,                                                   // IsIndicationRequired
        TRUE,                                                   // CompleteRequestOnSendComplete

        NDIS_OBJECT_TYPE_DEFAULT,                               // NdisObjectHeaderType
        NDIS_WWAN_SET_NETWORK_BLACKLIST_REVISON_1,              // NdisObjectHeaderRevision
        sizeof(NDIS_WWAN_SET_NETWORK_BLACKLIST),                // NdisObjectHeaderSize

        MbbNdisSetNetworkBlacklist,                             // Set routine
        sizeof(NDIS_WWAN_SET_NETWORK_BLACKLIST),                // MinSetInBufferSize

        MbbUtilQueryAttribute,                                  // Query routine
        0,                                                      // MinQueryInBufferSize
        sizeof(NDIS_WWAN_NETWORK_BLACKLIST),                    // MinQueryOutBufferSize

        { MBB_UUID_BASIC_CONNECT_EXT_CONSTANT, MBB_BASICCONNECTEXT_CID_NETWORK_BLACKLIST }, // ResponseCommand
        0,                                                      // ResponseBufferLength
        MbbNdisNetworkBlacklistHandler,                         // ResponseHandler
        NULL                                                    // CompletionHandler
    },
    {
        OID_WWAN_PCO,                                           // Oid

        FALSE,                                                   // IsSettable
        TRUE,                                                   // IsQueryable
        TRUE,                                                   // IsSerialized
        TRUE,                                                   // IsIndicationRequired
        TRUE,                                                   // CompleteRequestOnSendComplete

        NDIS_OBJECT_TYPE_DEFAULT,                               // NdisObjectHeaderType
        NDIS_WWAN_PCO_STATUS_REVISION_1,                        // NdisObjectHeaderRevision
        sizeof(NDIS_WWAN_PCO_STATUS),                           // NdisObjectHeaderSize

        NULL,                                                   // Set routine
        0,                                                      // MinSetInBufferSize

        MbbNdisGetPcoStatus,                                      // Query routine
        0,                                                      // MinQueryInBufferSize
        sizeof(NDIS_WWAN_PCO_STATUS),                           // MinQueryOutBufferSize

        { MBB_UUID_BASIC_CONNECT_EXT_CONSTANT, MBB_BASICCONNECTEXT_CID_PCO }, // ResponseCommand
        sizeof(MBB_MS_PCO_VALUE),                               // ResponseBufferLength
        MbbNdisPcoStatusHandler,                                // ResponseHandler
        NULL                                                    // CompletionHandler
    },
    {
        OID_WWAN_UICC_RESET,                                    // Oid

        TRUE,                                                   // IsSettable
        TRUE,                                                   // IsQueryable
        TRUE,                                                   // IsSerialized
        TRUE,                                                   // IsIndicationRequired
        TRUE,                                                   // CompleteRequestOnSendComplete

        NDIS_OBJECT_TYPE_DEFAULT,                               // NdisObjectHeaderType
        NDIS_WWAN_SET_UICC_RESET_REVISION_1,                    // NdisObjectHeaderRevision
        sizeof(NDIS_WWAN_SET_UICC_RESET),                       // NdisObjectHeaderSize

        MbbNdisSetUiccReset,                                    // Set routine
        sizeof(NDIS_WWAN_SET_UICC_RESET),                       // MinSetInBufferSize

        MbbNdisGetUiccReset,                                    // Query routine
        0,                                                      // MinQueryInBufferSize
        sizeof(NDIS_WWAN_UICC_RESET_INFO),                      // MinQueryOutBufferSize

        { MBB_UUID_UICC_LOW_LEVEL_CONSTANT, MBB_UICC_CID_RESET }, // ResponseCommand
        sizeof(MBB_UICC_RESET_INFO),                            // ResponseBufferLength
        MbbNdisUiccResetInfoStatusHandler,                      // ResponseHandler
        NULL                                                    // CompletionHandler
    },
    {
        OID_WWAN_DEVICE_RESET,                                  // Oid

        TRUE,                                                   // IsSettable
        FALSE,                                                  // IsQueryable
        TRUE,                                                   // IsSerialized
        TRUE,                                                   // IsIndicationRequired
        TRUE,                                                   // CompleteRequestOnSendComplete

        NDIS_OBJECT_TYPE_DEFAULT,                               // NdisObjectHeaderType
        NDIS_WWAN_SET_DEVICE_RESET_REVISION_1,                  // NdisObjectHeaderRevision
        sizeof(NDIS_WWAN_SET_DEVICE_RESET),                     // NdisObjectHeaderSize

        MbbNdisSetDeviceReset,                                  // Set routine
        sizeof(NDIS_WWAN_SET_DEVICE_RESET),                     // MinSetInBufferSize

        NULL,                                                   // Query routine
        0,                                                      // MinQueryInBufferSize
        0,                                                      // MinQueryOutBufferSize

        { MBB_UUID_BASIC_CONNECT_EXT_CONSTANT, MBB_BASICCONNECTEXT_CID_DEVICE_RESET }, // ResponseCommand
        0,                                                      // ResponseBufferLength
        MbbNdisDeviceResetStatusHandler,                        // ResponseHandler
        NULL                                                    // CompletionHandler
    },
    {
        OID_WWAN_BASE_STATIONS_INFO,                            // Oid

        FALSE,                                                  // IsSettable
        TRUE,                                                   // IsQueryable
        TRUE,                                                   // IsSerialized
        TRUE,                                                   // IsIndicationRequired
        TRUE,                                                   // CompleteRequestOnSendComplete

        NDIS_OBJECT_TYPE_DEFAULT,                               // NdisObjectHeaderType
        NDIS_WWAN_BASE_STATIONS_INFO_REQ_REVISION_1,            // NdisObjectHeaderRevision
        sizeof(NDIS_WWAN_BASE_STATIONS_INFO_REQ),               // NdisObjectHeaderSize

        NULL,                                                   // Set routine
        0,                                                      // MinSetInBufferSize

        MbbNdisGetBaseStationsInfo,                             // Query routine
        sizeof(NDIS_WWAN_BASE_STATIONS_INFO_REQ),               // MinQueryInBufferSize
        FIELD_OFFSET(NDIS_WWAN_BASE_STATIONS_INFO, BaseStationsInfo.BaseStationsData), // MinQueryOutBufferSize

        { MBB_UUID_BASIC_CONNECT_EXT_CONSTANT, MBB_BASICCONNECTEXT_CID_BASE_STATIONS_INFO }, // ResponseCommand
        FIELD_OFFSET(MBB_MS_BASE_STATIONS_INFO, DataBuffer),    // ResponseBufferLength
        MbbNdisBaseStationsInfoHandler,                         // ResponseHandler
        NULL                                                    // CompletionHandler
    },

    { 0 } // terminator
};


////////////////////////////////////////////////////////////////////////////////
//
//  DEFINITIONS
//
////////////////////////////////////////////////////////////////////////////////


NDIS_STATUS
MbbNdisAdapterWriteBoolParameter(
    __in    PMINIPORT_ADAPTER_CONTEXT Adapter,
    __in    BOOLEAN Value,
    __in    PNDIS_STRING RegNameString
    )
{
    NDIS_CONFIGURATION_OBJECT       ConfigObject;
    NDIS_HANDLE                     RegistryConfigurationHandle = NULL;
    NDIS_CONFIGURATION_PARAMETER    Parameter;
    NDIS_STATUS                     NdisStatus;

    ConfigObject.Header.Type = NDIS_OBJECT_TYPE_CONFIGURATION_OBJECT;
    ConfigObject.Header.Revision = NDIS_CONFIGURATION_OBJECT_REVISION_1;
    ConfigObject.Header.Size = sizeof( NDIS_CONFIGURATION_OBJECT );
    ConfigObject.NdisHandle = Adapter->MiniportAdapterHandle;
    ConfigObject.Flags = 0;

    NdisStatus = NdisOpenConfigurationEx(
                    &ConfigObject,
                    &RegistryConfigurationHandle
                    );

    if ((NdisStatus == NDIS_STATUS_SUCCESS) && (RegistryConfigurationHandle != NULL))
    {
        NdisZeroMemory(&Parameter, sizeof(NDIS_CONFIGURATION_PARAMETER));

        Parameter.ParameterData.IntegerData = (Value ? 1 : 0);
        Parameter.ParameterType = NdisParameterInteger;

        NdisWriteConfiguration(
            &NdisStatus,
            RegistryConfigurationHandle,
            RegNameString,
            &Parameter
            );
    }
    else
    {
        TraceError( WMBCLASS_OID, "[MbbAdapter] FAILED to open configuration, status=%!STATUS!", NdisStatus );
    }

    //
    // Close the handle to the registry
    //
    if (RegistryConfigurationHandle)
    {
        NdisCloseConfiguration(RegistryConfigurationHandle);
    }

    return NdisStatus;
}

MBB_BUS_HANDLE
MbbNdisGetBusHandle(
    __in    MBB_PROTOCOL_HANDLE AdapterHandle
    )
{
    return ((PMINIPORT_ADAPTER_CONTEXT)AdapterHandle)->BusHandle;
}

NDIS_OID
MbbNdisGetOid(
    __in PNDIS_OID_REQUEST OidRequest
    )
{
    switch( OidRequest->RequestType )
    {
        case NdisRequestQueryInformation:
        case NdisRequestQueryStatistics:
        {
            return OidRequest->DATA.QUERY_INFORMATION.Oid;
        }
        break;

        case NdisRequestSetInformation:
        {
            return OidRequest->DATA.SET_INFORMATION.Oid;
        }
        break;

        case NdisRequestMethod:
        {
            return OidRequest->DATA.METHOD_INFORMATION.Oid;
        }
        break;
    }

    return (NDIS_OID)0;
}

PMBB_OID_HANDLER_ENTRY
MbbNdisFetchOidHandler(
    __in NDIS_OID Oid,
    __in PNDIS_OID_REQUEST OidRequest
    )
{
    ULONG                       OidTableIndex;
    PMBB_OID_HANDLER_ENTRY      OidHandlerEntry = NULL;
    PNDIS_WWAN_AUTH_CHALLENGE   ndisWwanAuthChallenge;
    ULONG                       CmdId = 0;


    //
    // Crack open the OID request
    //
    switch (Oid){
    case OID_WWAN_AUTH_CHALLENGE:
    {
        if (OidRequest->RequestType == NdisRequestQueryInformation){

            if (OidRequest->DATA.QUERY_INFORMATION.InformationBufferLength < sizeof(*ndisWwanAuthChallenge))
            {
                return NULL;
            }

            ndisWwanAuthChallenge = (PNDIS_WWAN_AUTH_CHALLENGE)OidRequest->DATA.QUERY_INFORMATION.InformationBuffer;
            if (ndisWwanAuthChallenge->AuthChallenge.AuthMethod == WwanAuthAka){
                CmdId = MBB_AUTH_CID_AKA;
            }
            else if (ndisWwanAuthChallenge->AuthChallenge.AuthMethod == WwanAuthAkaPrime){
                    CmdId = MBB_AUTH_CID_AKAP;
            }
            else if (ndisWwanAuthChallenge->AuthChallenge.AuthMethod == WwanAuthSim){
                CmdId = MBB_AUTH_CID_SIM;
            }
        }
    }
    break;
    case OID_WWAN_DEVICE_SERVICE_SESSION:
    {
        if (OidRequest->RequestType == NdisRequestSetInformation){

            CmdId = MBB_DSS_CID_CONNECT;
        }

    }
    break;

    }


    for( OidTableIndex = 0;
         MbbSpecialOidHandlerTable[OidTableIndex].Oid != 0;
         OidTableIndex++ )
    {
        if( MbbSpecialOidHandlerTable[OidTableIndex].Oid == Oid )
        {
            OidHandlerEntry = &MbbSpecialOidHandlerTable[OidTableIndex];
            if (CmdId == OidHandlerEntry->ResponseCommand.CommandId){
                return OidHandlerEntry;
            }
        }
    }

    return NULL;
}

PMBB_OID_HANDLER_ENTRY
MbbNdisFetchOidHandlerWithRev(
    __in NDIS_OID Oid,
    __in PNDIS_OID_REQUEST OidRequest
)
{
    ULONG                       OidTableIndex;
    PMBB_OID_HANDLER_ENTRY      OidHandlerEntry = NULL;
    PNDIS_OBJECT_HEADER         ndisHeader = NULL;

    //
    // Crack open the OID request
    //
    switch (Oid) {
    case OID_WWAN_PROVISIONED_CONTEXTS:
    {
        if (OidRequest->RequestType == NdisRequestQueryInformation) 
        {
            ndisHeader = (PNDIS_OBJECT_HEADER)OidRequest->DATA.QUERY_INFORMATION.InformationBuffer;
        }
        else if (OidRequest->RequestType == NdisRequestSetInformation)
        {
            ndisHeader = (PNDIS_OBJECT_HEADER)OidRequest->DATA.SET_INFORMATION.InformationBuffer;
        }
    }
    break;

    }

    if (ndisHeader)
    {
        for (OidTableIndex = 0;
             MbbOidHandlerTable[OidTableIndex].Oid != 0;
             OidTableIndex++)
        {
            if (MbbOidHandlerTable[OidTableIndex].Oid == Oid &&
                ndisHeader->Revision == MbbOidHandlerTable[OidTableIndex].NdisObjectHeaderRevision)
            {
                OidHandlerEntry = &MbbOidHandlerTable[OidTableIndex];
                break;
            }
        }
    }

    return OidHandlerEntry;
}

PMBB_OID_HANDLER_ENTRY
MbbNdisGetOidHandler(
    __in NDIS_OID Oid
    )
{
    ULONG                   OidTableIndex;
    PMBB_OID_HANDLER_ENTRY  OidHandlerEntry = NULL;

    for( OidTableIndex = 0;
         MbbOidHandlerTable[OidTableIndex].Oid != 0;
         OidTableIndex++ )
    {
        if( MbbOidHandlerTable[OidTableIndex].Oid == Oid )
        {
            OidHandlerEntry = &MbbOidHandlerTable[OidTableIndex];
            break;
        }
    }
    return OidHandlerEntry;
}

PMBB_OID_HANDLER_ENTRY
MbbNdisGetOidHandlerByCommand(
    __in PMBB_COMMAND   Command
    )
{
    ULONG                   OidTableIndex;
    ULONG                   TableNumber;
    PMBB_OID_HANDLER_ENTRY  Table;
    PMBB_OID_HANDLER_ENTRY  OidHandlerEntry = NULL;

    if( Command->CommandId == 0 &&
        memcmp(
            &MBB_UUID_INVALID,
            &(Command->ServiceId),
            sizeof(GUID)
            ) == 0 )
    {
        return NULL;
    }

    for( TableNumber = 0;
         TableNumber < 2;
         TableNumber ++ )
    {
        if( TableNumber == 0 )
            Table = MbbOidHandlerTable;
        else
            Table = MbbSpecialOidHandlerTable;

        for( OidTableIndex = 0;
             Table[OidTableIndex].Oid != 0;
             OidTableIndex++ )
        {
            if( Command->CommandId == Table[OidTableIndex].ResponseCommand.CommandId
                &&
                memcmp(
                    &(Table[OidTableIndex].ResponseCommand.ServiceId),
                    &(Command->ServiceId),
                    sizeof(GUID)
                    ) == 0 )
            {
                OidHandlerEntry = &Table[OidTableIndex];
                break;
            }
        }
    }

    return OidHandlerEntry;
}

__inline
VOID
MbbNdisMiniportIndicateRequestStatus(
    __in PMBB_REQUEST_CONTEXT       Request,
    __in PNDIS_STATUS_INDICATION    StatusIndication
    )
{
    PMINIPORT_ADAPTER_CONTEXT   Adapter=Request->RequestManager->AdapterContext;

    MbbUtilNdisMiniportIndicateStatusEx(Adapter, StatusIndication);
}

VOID
MbbNdisMiniportOidRequestCompletion(
    __in MBB_PROTOCOL_HANDLE    AdapterHandle,
    __in PMBB_REQUEST_CONTEXT   Request,
    __in NDIS_STATUS            NdisStatus
    )
{
    NDIS_STATUS                 LocalNdisStatus;
    PMINIPORT_ADAPTER_CONTEXT   Adapter = (PMINIPORT_ADAPTER_CONTEXT)(AdapterHandle);
    PCSTR                       OidName=NULL;
    ULONG                       OidNameLength=0;
    BOOLEAN                     CompleteNdisRequest=TRUE;
    BOOLEAN                     IndicationSentInHandler=FALSE;

    if (NdisStatus == MBB_STATUS_INDICATION_ALREADY_SENT)
    {
        //
        //  the request handler has indicated the response already, fix up the status
        //  and remember that we don't need to hold the request for a response that is not coming.
        //
        IndicationSentInHandler=TRUE;
        NdisStatus = NDIS_STATUS_SUCCESS;
    }

    if( NdisStatus == NDIS_STATUS_SUCCESS &&
        Request->OidHandler->IsIndicationRequired == TRUE )
    {
        NdisStatus = NDIS_STATUS_INDICATION_REQUIRED;

        if (!Request->OidHandler->CompleteRequestOnSendComplete)
        {
            //
            //  We don't want to complete the request back to ndis at this point.
            //  We will complete the request when the response comes back from the device.
            //
            CompleteNdisRequest=FALSE;
        }

    }

    if ((NdisStatus == NDIS_STATUS_PENDING))
    {
        //
        //  We don't want to complete with status pending
        //
        CompleteNdisRequest=FALSE;
    }

    //
    // Complete the request to NDIS
    //
    if( CompleteNdisRequest )
    {
        PNDIS_OID_REQUEST   NdisRequest=NULL;

        NdisRequest=InterlockedExchangePointer(&Request->OidContext.OidRequest,(PNDIS_OID_REQUEST)-1);

        if (NdisRequest != (PNDIS_OID_REQUEST)-1)
        {
            MBB_NDIS_OID_STATE OldOidState;

            Request->OidContext.OidStatus = NdisStatus;
            //
            // Do not complete the OID request if the Oid handler is still running.
            // Defer the OID completion to the Oid handler
            //
            OldOidState = MbbReqMgrGetSetOidState(
                            Request,
                            MbbNdisOidStateComplete
                            );
            if( OldOidState == MbbNdisOidStatePending ||
                OldOidState == MbbNdisOidStateDispatched )
            {
                TraceInfo(  WMBCLASS_OID, "[MbbNdis][ReqId=0x%04x] Completing OID request in %!MbbOidState! to NDIS with status=%!status!",
                            Request->RequestId, OldOidState, NdisStatus
                            );

                OidName=GetOidName(NdisRequest->DATA.QUERY_INFORMATION.Oid);
                OidNameLength=strlen(OidName)+1;

                MbbWriteEvent(
                    &REQUEST_COMPLETE_EVENT,
                    NULL,
                    NULL,
                    10,
                    &Adapter->TraceInstance,
                    sizeof(Adapter->TraceInstance),
                    &NdisRequest,
                    sizeof(NdisRequest),
                    &NdisRequest->DATA.QUERY_INFORMATION.Oid,
                    sizeof(NdisRequest->DATA.QUERY_INFORMATION.Oid),
                    (PVOID)OidName,
                    OidNameLength,
                    &NdisRequest->RequestId,
                    sizeof(NdisRequest->RequestId),
                    &NdisRequest->RequestHandle,
                    sizeof(NdisRequest->RequestHandle),
                    &NdisRequest->RequestType,
                    sizeof(NdisRequest->RequestType),
                    &NdisRequest->DATA.QUERY_INFORMATION.BytesWritten,
                    sizeof(NdisRequest->DATA.QUERY_INFORMATION.BytesWritten),
                    &NdisRequest->DATA.QUERY_INFORMATION.BytesNeeded,
                    sizeof(NdisRequest->DATA.QUERY_INFORMATION.BytesNeeded),
                    &NdisStatus,
                    sizeof(NdisStatus)
                    );


                NdisMOidRequestComplete(
                    Adapter->MiniportAdapterHandle,
                    NdisRequest,
                    NdisStatus
                    );

                NdisRequest=NULL;
            }
            else
            {
                TraceInfo(  WMBCLASS_OID, "[MbbNdis][ReqId=0x%04x] Deferring oid completion in %!MbbOidState!",
                            Request->RequestId, OldOidState
                            );
            }
        }
        else
        {
            TraceWarn(  WMBCLASS_OID, "[MbbNdis][ReqId=0x%04x] Oid already completed",
                        Request->RequestId
                        );
        }
    }

    if( Request->OidHandler->CompletionHandler != NULL )
    {
        Request->OidHandler->CompletionHandler(
                                Request,
                                NdisStatus
                                );
    }

    if( Request->HandlerContext.DataToFreeOnCompletion != NULL )
        FREE_POOL( Request->HandlerContext.DataToFreeOnCompletion );
    //
    // If the request does not wait for a NDIS INDICATTION
    // then free the request else move the request FSM
    // to the next state.
    //
    if( IndicationSentInHandler ||
        ( NdisStatus != NDIS_STATUS_PENDING &&
          NdisStatus != NDIS_STATUS_INDICATION_REQUIRED ))
    {
        if ( Request->HandlerContext.DataToFreeOnResponse != NULL )
            FREE_POOL( Request->HandlerContext.DataToFreeOnResponse );

        KeSetEvent(
           &Request->WaitEvent,
            IO_NO_INCREMENT,
            FALSE
            );
        MbbReqMgrDestroyRequest(
            Request->RequestManager,
            Request
            );
    }

    if (NdisStatus == NDIS_STATUS_FAILURE)
    {
        PMBB_SEND_QUEUE SendQueue = &Adapter->SendQueue;
        switch (Request->OidHandler->Oid)
        {
        case OID_WWAN_RADIO_STATE:
            TraceLoggingWrite(
                g_hLoggingProvider,
                "RadioStateSendFailure",
                TraceLoggingKeyword(MICROSOFT_KEYWORD_MEASURES));
            TryQueueStallState(SendQueue);
            break;
        }
    }
}

NDIS_STATUS
MbbNdisMiniportOidRequest(
    __in  NDIS_HANDLE       MiniportAdapter,
    __in  PNDIS_OID_REQUEST OidRequest
    )
{
    NDIS_OID                    Oid;
    ULONG                       ResponseBufferLength;
    NDIS_STATUS                 NdisStatus = NDIS_STATUS_SUCCESS;
    PMBB_REQUEST_CONTEXT        Request = NULL;
    PMBB_REQUEST_MANAGER        RequestManager = NULL;
    PMBB_OID_HANDLER_ENTRY      OidHandler;
    PMINIPORT_ADAPTER_CONTEXT   Adapter=(PMINIPORT_ADAPTER_CONTEXT)MiniportAdapter;
    NDIS_STATUS                 Status=NDIS_STATUS_NOT_SUPPORTED;
    PCSTR                       OidName=NULL;
    ULONG                       OidNameLength=0;

    do
    {
        OidName=GetOidName(OidRequest->DATA.QUERY_INFORMATION.Oid);
        OidNameLength=strlen(OidName)+1;

        MbbWriteEvent(
            &REQUEST_EVENT,
            NULL,
            NULL,
            8,
            &Adapter->TraceInstance,
            sizeof(Adapter->TraceInstance),
            &OidRequest,
            sizeof(OidRequest),
            &OidRequest->DATA.QUERY_INFORMATION.Oid,
            sizeof(OidRequest->DATA.QUERY_INFORMATION.Oid),
            (PVOID)OidName,
            OidNameLength,
            &OidRequest->RequestId,
            sizeof(OidRequest->RequestId),
            &OidRequest->RequestHandle,
            sizeof(OidRequest->RequestHandle),
            &OidRequest->RequestType,
            sizeof(OidRequest->RequestType),
            &OidRequest->DATA.QUERY_INFORMATION.InformationBufferLength,
            sizeof(OidRequest->DATA.QUERY_INFORMATION.InformationBufferLength)
            );

        if( (Oid = MbbNdisGetOid( OidRequest )) == 0 )
        {
            TraceError( WMBCLASS_OID, "[MbbNdis] FAILED to get Oid value" );
            NdisStatus = NDIS_STATUS_NOT_SUPPORTED;
            break;
        }

        //
        // Find it in the Oid table. But first check if it is a special OID that needs to handled differently.
        // If its not in the OID table then its not supported.
        //
        if ((OID_WWAN_AUTH_CHALLENGE == Oid ) ||
            (OID_WWAN_DEVICE_SERVICE_SESSION == Oid)) {
            OidHandler = MbbNdisFetchOidHandler(Oid, OidRequest);
            if (NULL == OidHandler) {
                TraceError( WMBCLASS_OID, "[MbbNdis] UNSUPPORTED Oid=0x%x", Oid );
                NdisStatus = NDIS_STATUS_NOT_SUPPORTED;
                break;
            }
        }
        else if (OID_WWAN_PROVISIONED_CONTEXTS == Oid)
        {
            OidHandler = MbbNdisFetchOidHandlerWithRev(Oid, OidRequest);
            if (NULL == OidHandler) {
                TraceError(WMBCLASS_OID, "[MbbNdis] UNSUPPORTED Oid=0x%x", Oid);
                NdisStatus = NDIS_STATUS_NOT_SUPPORTED;
                break;
            }
        }
        else if( (OidHandler = MbbNdisGetOidHandler( Oid )) == NULL )
        {
            TraceError( WMBCLASS_OID, "[MbbNdis] UNSUPPORTED Oid=0x%x", Oid );
            NdisStatus = NDIS_STATUS_NOT_SUPPORTED;
            break;
        }
        //
        // Allocate the internal MBB request context.
        //
        if( OidHandler->IsIndicationRequired == TRUE )
        {
            ResponseBufferLength = OidHandler->ResponseBufferLength;
        }
        else
        {
            ResponseBufferLength = 0;
        }
        if( (RequestManager = MbbAdapterGetRequestManager( Adapter )) == NULL )
        {
            TraceError( WMBCLASS_OID, "[MbbNdis] FAILED to reference RequestManager for Oid=0x%x", Oid);
            break;
        }

        if( (Request = MbbReqMgrCreateRequest(
                            RequestManager,
                            OidRequest,
                            ResponseBufferLength,
                            &NdisStatus)) == NULL )
        {
            TraceError( WMBCLASS_OID, "[MbbNdis] FAILED to allocate RequestContext for Oid=0x%x", Oid );
            break;
        }
        Request->OidHandler = OidHandler;
        //
        // Perform buffer validation
        //
        switch( OidRequest->RequestType )
        {
            case NdisRequestQueryInformation:
            case NdisRequestQueryStatistics:
            {
                if( OidHandler->IsQueryable == FALSE )
                {
                    NdisStatus = NDIS_STATUS_NOT_SUPPORTED;
                    break;
                }
                //
                // Minimum buffer size for query is NOT required for
                // requests that are completed via status indications.
                //
                if( OidHandler->IsIndicationRequired != TRUE &&
                    OidRequest->DATA.QUERY_INFORMATION.InformationBufferLength < OidHandler->MinQueryOutBufferSize )
                {
                    OidRequest->DATA.QUERY_INFORMATION.BytesNeeded = OidHandler->MinQueryOutBufferSize;
                    NdisStatus = NDIS_STATUS_INVALID_LENGTH;
                    break;
                }
            }
            break;

            case NdisRequestSetInformation:
            {
                if( OidHandler->IsSettable == FALSE )
                {
                    NdisStatus = NDIS_STATUS_NOT_SUPPORTED;
                    break;
                }
                if( OidRequest->DATA.SET_INFORMATION.InformationBufferLength < OidHandler->MinSetInBufferSize )
                {
                    OidRequest->DATA.SET_INFORMATION.BytesNeeded = OidHandler->MinSetInBufferSize;
                    NdisStatus = NDIS_STATUS_INVALID_LENGTH;
                    break;
                }
            }
            break;

            case NdisRequestMethod:
            {
                if( OidHandler->IsQueryable == FALSE )
                {
                    NdisStatus = NDIS_STATUS_NOT_SUPPORTED;
                    break;
                }
                if( OidRequest->DATA.METHOD_INFORMATION.InputBufferLength  < OidHandler->MinQueryInBufferSize ||
                    OidRequest->DATA.METHOD_INFORMATION.OutputBufferLength < OidHandler->MinQueryOutBufferSize )
                {
                    OidRequest->DATA.METHOD_INFORMATION.BytesNeeded = MAX( OidHandler->MinQueryInBufferSize, OidHandler->MinQueryOutBufferSize );
                    NdisStatus = NDIS_STATUS_INVALID_LENGTH;
                    break;
                }
            }
            break;

            default:
            {
                NdisStatus = NDIS_STATUS_NOT_SUPPORTED;
            }
            break;
        }
        //
        // If buffer validation failed or the OID Handler failed, bail out.
        //
        if( NdisStatus != NDIS_STATUS_SUCCESS )
        {
            break;
        }
        //
        // Dispatch the request to the queue for processing.
        // If the request completes synchronously cleanup
        // the request context. Else the completion routine
        // will be called at a later time.
        //
        MbbReqMgrGetSetOidState(
            Request,
            MbbNdisOidStatePending
            );
        NdisStatus = MbbReqMgrDispatchRequest(
                        Request,
                        (OidHandler->IsSerialized == 1),
                        MbbNdisProcessSerializedOidRequest,
                        MbbNdisMiniportOidRequestCompletion,
                        MbbNdisIndicateStatus
                        );
     } while( FALSE );

    if( NdisStatus != NDIS_STATUS_PENDING &&
        NdisStatus != NDIS_STATUS_INDICATION_REQUIRED )
    {
        if( Request != NULL )
        {
            MbbReqMgrDestroyRequest(
                Request->RequestManager,
                Request
                );
        }
    }

    if( RequestManager != NULL )
    {
        MbbReqMgrDeref( RequestManager );
    }

    if( NdisStatus != NDIS_STATUS_PENDING )
    {
        MbbWriteEvent(
            &REQUEST_NOT_PENDING_EXIT_EVENT,
            NULL,
            NULL,
            10,
            &Adapter->TraceInstance,
            sizeof(Adapter->TraceInstance),
            &OidRequest,
            sizeof(OidRequest),
            &OidRequest->DATA.QUERY_INFORMATION.Oid,
            sizeof(OidRequest->DATA.QUERY_INFORMATION.Oid),
            (PVOID)OidName,
            OidNameLength,
            &OidRequest->RequestId,
            sizeof(OidRequest->RequestId),
            &OidRequest->RequestHandle,
            sizeof(OidRequest->RequestHandle),
            &OidRequest->RequestType,
            sizeof(OidRequest->RequestType),
            &OidRequest->DATA.QUERY_INFORMATION.BytesWritten,
            sizeof(OidRequest->DATA.QUERY_INFORMATION.BytesWritten),
            &OidRequest->DATA.QUERY_INFORMATION.BytesNeeded,
            sizeof(OidRequest->DATA.QUERY_INFORMATION.BytesNeeded),
            &NdisStatus,
            sizeof(NdisStatus)
            );


    }
    else
    {
        MbbWriteEvent(
            &REQUEST_PENDING_EXIT_EVENT,
            NULL,
            NULL,
            3,
            &Adapter->TraceInstance,
            sizeof(Adapter->TraceInstance),
            &OidRequest,
            sizeof(OidRequest),
            &NdisStatus,
            sizeof(NdisStatus)
            );

    }

    return NdisStatus;
}

NDIS_STATUS
MbbNdisProcessSerializedOidRequest(
    __in MBB_PROTOCOL_HANDLE                AdapterHandle,
    __in PMBB_REQUEST_CONTEXT               Request
    )
/*++
    Description
        This is the internal serialized routine that calls OID handlers.
        Only one instance of this routine will run at any time.
        If an OID is associated with this request, NDIS is called to
        notify about the OID completion via NdisMOidRequestComplete( ).

    Parameters
        __in  PMBB_REQUEST_CONTEXT  Request
            The request to process. This is the internal request representation.
            The request

    Return Value
        NDIS_STATUS_SUCCESS
            The request was successfully processed and the OID, if any,
            was successfully completed to NDIS.

        NDIS_STATUS_NOT_SUPPORTED
            The request is an invalid request and could not be processed.

        NDIS_STATUS_PENDING
            Request completion is deferred by the OID handler.
            The OID handler will call NdisMOidRequestComplete( ).
--*/
{
    NDIS_STATUS             NdisStatus = STATUS_SUCCESS;
    NDIS_OID                Oid = MbbNdisGetOid( Request->OidContext.OidRequest );
    MBB_NDIS_OID_STATE      OidState;
    PMBB_OID_HANDLER_ENTRY  OidHandlerEntry = Request->OidHandler;
    PNDIS_OID_REQUEST       OidRequest = Request->OidContext.OidRequest;
    PMINIPORT_ADAPTER_CONTEXT    Adapter = (PMINIPORT_ADAPTER_CONTEXT)AdapterHandle;

    TraceInfo(WMBCLASS_OID, "Entered OID=%08lx ", Oid );

    MbbReqMgrGetSetOidState(
        Request,
        MbbNdisOidStateDispatching
        );
    //
    // Buffers have already been validated. Call the handler.
    //
    switch( OidRequest->RequestType )
    {
        case NdisRequestQueryInformation:
        case NdisRequestQueryStatistics:
        {
            OidRequest->DATA.QUERY_INFORMATION.BytesWritten = OidRequest->DATA.QUERY_INFORMATION.InformationBufferLength;
            //
            // Ideally, Query requests do not contain an InBuffer.
            // SMS_READ,VISIBLE_PROVIDERS and DEVICE_SERVICE_COMMAND overloads
            // the OutBuffer as an InBuffer.
            // The correct approach would have been to use Method
            // request instead of Query but deal with it anyways.
            //
            switch(Oid)
            {
            case OID_WWAN_SMS_READ:
            case OID_WWAN_DEVICE_SERVICE_COMMAND:
            case OID_WWAN_VISIBLE_PROVIDERS:
            case OID_WWAN_AUTH_CHALLENGE:
            case OID_WWAN_SLOT_INFO_STATUS:
            case OID_WWAN_BASE_STATIONS_INFO:

                if( OidRequest->DATA.QUERY_INFORMATION.InformationBufferLength < OidHandlerEntry->MinQueryInBufferSize )
                {
                    NdisStatus = NDIS_STATUS_INVALID_LENGTH;
                    break;
                }


                NdisStatus = OidHandlerEntry->QueryHandler(
                                Request,
                                OidRequest->DATA.QUERY_INFORMATION.InformationBuffer,
                                &OidRequest->DATA.QUERY_INFORMATION.InformationBufferLength,
                                OidRequest->DATA.QUERY_INFORMATION.InformationBuffer,
                                &OidRequest->DATA.QUERY_INFORMATION.BytesWritten
                                );

                break;

            default:
                NdisStatus = OidHandlerEntry->QueryHandler(
                                Request,
                                NULL,
                                NULL,
                                OidRequest->DATA.QUERY_INFORMATION.InformationBuffer,
                                &OidRequest->DATA.QUERY_INFORMATION.BytesWritten
                                );

                break;
            }
        }
        break;

        case NdisRequestSetInformation:
        {
            OidRequest->DATA.SET_INFORMATION.BytesRead = OidRequest->DATA.SET_INFORMATION.InformationBufferLength;

            NdisStatus = OidHandlerEntry->SetHandler(
                            Request,
                            OidRequest->DATA.SET_INFORMATION.InformationBuffer,
                           &OidRequest->DATA.SET_INFORMATION.BytesRead
                            );
        }
        break;

        case NdisRequestMethod:
        {
            OidRequest->DATA.METHOD_INFORMATION.BytesRead    = OidRequest->DATA.METHOD_INFORMATION.InputBufferLength;
            OidRequest->DATA.METHOD_INFORMATION.BytesWritten = OidRequest->DATA.METHOD_INFORMATION.OutputBufferLength;

            NdisStatus = OidHandlerEntry->QueryHandler(
                            Request,
                            OidRequest->DATA.METHOD_INFORMATION.InformationBuffer,
                           &OidRequest->DATA.METHOD_INFORMATION.BytesRead,
                            OidRequest->DATA.METHOD_INFORMATION.InformationBuffer,
                           &OidRequest->DATA.METHOD_INFORMATION.BytesWritten
                            );
        }
        break;

        default:
        {
            NdisStatus = NDIS_STATUS_NOT_SUPPORTED;
        }
        break;
    }
    //
    // Complete the OID request if the Oid completion happened before this point.
    //
    OidState = MbbReqMgrGetSetOidState(
                    Request,
                    MbbNdisOidStateDispatched
                    );
    if( OidState == MbbNdisOidStateComplete )
    {
        PCSTR   OidName=NULL;
        ULONG   OidNameLength=0;

        TraceInfo(  WMBCLASS_OID, "[MbbNdis][ReqId=0x%04x] Delay completing OID request to NDIS with status=%!status!",
                    Request->RequestId, Request->OidContext.OidStatus
                    );

        OidName = GetOidName(OidRequest->DATA.QUERY_INFORMATION.Oid);
        OidNameLength = strlen(OidName)+1;

        MbbWriteEvent(
            &REQUEST_COMPLETE_EVENT,
            NULL,
            NULL,
            10,
            &Adapter->TraceInstance,
            sizeof(Adapter->TraceInstance),
            &OidRequest,
            sizeof(OidRequest),
            &OidRequest->DATA.QUERY_INFORMATION.Oid,
            sizeof(OidRequest->DATA.QUERY_INFORMATION.Oid),
            (PVOID)OidName,
            OidNameLength,
            &OidRequest->RequestId,
            sizeof(OidRequest->RequestId),
            &OidRequest->RequestHandle,
            sizeof(OidRequest->RequestHandle),
            &OidRequest->RequestType,
            sizeof(OidRequest->RequestType),
            &OidRequest->DATA.QUERY_INFORMATION.BytesWritten,
            sizeof(OidRequest->DATA.QUERY_INFORMATION.BytesWritten),
            &OidRequest->DATA.QUERY_INFORMATION.BytesNeeded,
            sizeof(OidRequest->DATA.QUERY_INFORMATION.BytesNeeded),
            &Request->OidContext.OidStatus,
            sizeof(Request->OidContext.OidStatus)
            );

        NdisMOidRequestComplete(
            Adapter->MiniportAdapterHandle,
            OidRequest,
            Request->OidContext.OidStatus
            );
    }

    return NdisStatus;
}

VOID
MbbNdisMiniportCancelOidRequest(
    __in  NDIS_HANDLE MiniportAdapter,
    __in  PVOID RequestId  // PVOID is of actual type PNDIS_OID_REQUEST
    )

{
    PMINIPORT_ADAPTER_CONTEXT   Adapter=(PMINIPORT_ADAPTER_CONTEXT)MiniportAdapter;
    PMBB_REQUEST_MANAGER        RequestManager = NULL;
    PMBB_REQUEST_CONTEXT        Request;

    if( (RequestManager = MbbAdapterGetRequestManager( Adapter )) == NULL )
    {
        TraceError( WMBCLASS_INIT, "[MbbNdis] FAILED to reference RequestManager to cancel OID RequestId=0x%p", RequestId );
        return;
    }

    if( (Request = MbbReqMgrGetRequestByOidRequestId( RequestManager, RequestId )) != NULL )
    {
        TraceInfo(WMBCLASS_OID, "[MbbNdis][ReqId=0x%04x] Received cancel for OID RequestId=0x%p. Submitting cancel to FSM.", Request->RequestId, RequestId );

        MbbReqMgrQueueEvent(
            RequestManager,
            Request,
            MbbRequestEventCancel,
            NULL,
            0
            );
        MbbReqMgrDerefRequest( Request );
    }
    else
    {
        TraceError( WMBCLASS_OID, "[MbbNdis] FAILED to find request with OID RequestId=0x%p for cancellation", RequestId );
    }
    MbbReqMgrDeref( RequestManager );
}

//
// RESPONSE HANDLING
//

/*++
    RequestManager->FragmentBuffer is a shared buffer used
    to retrieve response fragments. Even if there are multiple
    interrupts on the interrupt pipe there will be only one
    request active doing a GET_ENCAPSULATED_RESPONSE.
    Hence access to the buffer is synchronized.
    The synchronisation is guaranteed by MbbReqMgrCompleteRequest( )
    since it queues the SendComplete for the current active
    request first before queuing the Start for the next request.

    MbbNdisGetResponseFragment( ) uses the same buffer for reading
    each fragment.

    MbbNdisParseCommandResponse( ) reassembles the fragments in
    per request response buffer. If the per request pre-allocated
    response buffer is not sufficient, re-allocation is attempted.

    MbbNdisParseIndicateStatus( ) reassembles the fragments in
    the shared RequestManager->ResponseBuffer.
--*/


VOID
MbbNdisIndicateStatus(
    __in PMBB_REQUEST_CONTEXT               Request,
    __in NDIS_STATUS                        NdisStatus,
    __in MBB_STATUS                         MbbStatus,
    __in_bcount_opt(InBufferSize) PUCHAR    InBuffer,
    __in ULONG                              InBufferSize
    )
{
    PMBB_OID_HANDLER_ENTRY  OidHandler;

    //
    // If the request already has a Oid handler use that.
    // Else if this request is doing a status indication it will not have a OidHandler.
    // In status indication case
    //
    if( (OidHandler = Request->OidHandler) == NULL &&
        (OidHandler = MbbNdisGetOidHandlerByCommand( &(Request->HandlerContext.Response.Command) )) == NULL )
    {
        if( !MbbUtilIsNativeMbnService( &Request->HandlerContext.Response.Command.ServiceId ) )
        {
            OidHandler = MbbNdisGetOidHandler( OID_WWAN_DEVICE_SERVICE_COMMAND );
        }
    }
    if( OidHandler == NULL )
    {
        TraceError( WMBCLASS_OID, "[MbbNdis][ReqId=0x%04x] FAILED to find response Handler for ServiceId=%!GUID! Cid=%02d",
                    Request->RequestId,
                   &Request->HandlerContext.Response.Command.ServiceId,
                    Request->HandlerContext.Response.Command.CommandId
                    );
        NdisStatus = NDIS_STATUS_FAILURE;
    }
    //
    // No NDIS specific work needed.
    // Call the common handler to do cleanup.
    //
    MbbUtilCommonCIDResponse(
        Request,
        NdisStatus,
        MbbStatus,
        InBuffer,
        InBufferSize,
        OidHandler
        );
}


NDIS_STATUS
MbbNdisReassembleResponseFragment(
    __in_bcount(FragmentLength) PUCHAR  FragmentBuffer,
    __in    ULONG                       FragmentLength,
    __in    PMBB_REASSEMBLE_CONTEXT     ReassembleContext,
    __in    BOOLEAN                     DoReassemble,
    __in    PMBB_REQUEST_MANAGER        RequestManager,
    __in    GUID                        ActivityId
    )
/*++
    Return Value
        STATUS_SUCCESS
            All fragments of the message was successfully reassembled.
        STATUS_PENDING
            This fragment was successfully reassembled but there are more.
        Other
            The fragment could not be reassembled.
--*/
{
    ULONG                           TransactionId;
    ULONG                           PayloadLength;
    ULONG                           PayloadOffset = 0;
    PCHAR                           ReassembleBuffer = NULL;
    ULONG                           ReassembleBufferOffset = 0;
    ULONG                           ReassembleDataLength = 0;
    ULONG                           FragmentCount;
    ULONG                           CurrentFragment;
    NTSTATUS                        NtStatus = STATUS_SUCCESS;
    MBB_COMMAND                     Command = {0};
    MBB_STATUS                      MbbStatus = {0};
    PMBB_INDICATE_STATUS_HEADER     IndicateStatusHeader;
    PMBB_COMMAND_DONE_HEADER        CommandDoneHeader;
    PMBB_COMMAND_FRAGMENT_HEADER    MessageFragmentHeader = (PMBB_COMMAND_FRAGMENT_HEADER)FragmentBuffer;


    PayloadLength   = MessageFragmentHeader->MessageHeader.MessageLength;
    TransactionId   = MessageFragmentHeader->MessageHeader.MessageTransactionId;
    FragmentCount   = MessageFragmentHeader->FragmentHeader.TotalFragments;
    CurrentFragment = MessageFragmentHeader->FragmentHeader.CurrentFragment;

    do
    {
        //
        // Get extra information from the first fragment
        //
        if( CurrentFragment == 0 )
        {
            if( MessageFragmentHeader->MessageHeader.MessageType == MBB_MESSAGE_TYPE_COMMAND_DONE )
            {
                if( FragmentLength < sizeof(MBB_COMMAND_DONE_HEADER) )
                {
                    return STATUS_NDIS_INVALID_LENGTH;
                }
                CommandDoneHeader = (PMBB_COMMAND_DONE_HEADER)FragmentBuffer;

                Command.CommandId = CommandDoneHeader->Command.CommandId;
                MBB_UUID_TO_HOST(
                    &Command.ServiceId,
                    &CommandDoneHeader->Command.ServiceId
                    );
                MbbStatus            = CommandDoneHeader->MbbStatus;
                PayloadOffset        = sizeof(MBB_COMMAND_DONE_HEADER);
                ReassembleDataLength = CommandDoneHeader->InformationBufferLength;
            }
            else if( MessageFragmentHeader->MessageHeader.MessageType == MBB_MESSAGE_TYPE_INDICATE_STATUS )
            {
                if( FragmentLength < sizeof(MBB_INDICATE_STATUS_HEADER) )
                {
                    return STATUS_NDIS_INVALID_LENGTH;
                }
                IndicateStatusHeader = (PMBB_INDICATE_STATUS_HEADER)FragmentBuffer;

                Command.CommandId = IndicateStatusHeader->Command.CommandId;
                MBB_UUID_TO_HOST(
                    &Command.ServiceId,
                    &IndicateStatusHeader->Command.ServiceId
                    );
                MbbStatus            = MBB_STATUS_SUCCESS;
                PayloadOffset        = sizeof(MBB_INDICATE_STATUS_HEADER);
                ReassembleDataLength = IndicateStatusHeader->InformationBufferLength;
            }
            else
            {
                NtStatus = STATUS_NDIS_INVALID_DATA;
            }

            if( ReassembleDataLength > ReassembleContext->BufferLength )
            {
                NtStatus = STATUS_BUFFER_OVERFLOW;
                break;
            }
            //
            // Cache the info in the reassemble context for later use.
            //
            if( DoReassemble )
            {
                ReassembleBuffer        = ReassembleContext->Buffer;
                ReassembleBufferOffset  = 0;
            }
            ReassembleContext->MbbStatus     = MbbStatus;
            ReassembleContext->DataLength    = ReassembleDataLength;
        }
        //
        // For subsequent fragments read savved state from the reassemble context
        //
        else
        {
            Command       = ReassembleContext->Command;
            PayloadOffset = sizeof(MBB_COMMAND_FRAGMENT_HEADER);

            if( DoReassemble )
            {
                ReassembleBuffer        = ReassembleContext->Buffer;
                ReassembleDataLength    = ReassembleContext->DataLength;
                ReassembleBufferOffset  = ReassembleContext->BufferOffset;
            }
        }

        if( PayloadLength > FragmentLength ||
            PayloadLength < PayloadOffset )
        {
            TraceError( WMBCLASS_OID,
                        "[MbbNdis][TID=0x%08x] INVALID MessageLength[Receieved=0x%08x < Expected=0x%08x] in Fragment=%02d/%02d",
                        TransactionId,
                        FragmentLength,
                        PayloadLength,
                        CurrentFragment,
                        FragmentCount
                        );
            MbbUtilSendMbimError(
                TransactionId,
                MBB_ERROR_LENGTH_MISMATCH,
                RequestManager->AdapterContext,
                ActivityId,
                &Command
                );
            NtStatus = STATUS_BUFFER_OVERFLOW;
            break;
        }

        if( DoReassemble )
        {
            PayloadLength -= PayloadOffset;

            if( ( PayloadLength + ReassembleBufferOffset ) > ReassembleDataLength )
            {
                TraceError( WMBCLASS_OID,
                            "[MbbNdis][TID=0x%08x] INVALID PayloadLength=0x%08x in Fragment=%02d/%02d. TRUNCATING Response.",
                            TransactionId, PayloadLength, CurrentFragment, FragmentCount
                            );
            }
            TraceInfo(  WMBCLASS_OID,
                        "[MbbNdis][TID=0x%08x] Received Fragment=%02d/%02d",
                        TransactionId, CurrentFragment, FragmentCount
                        );
            RtlCopyMemory(
                ReassembleBuffer + ReassembleBufferOffset,
                FragmentBuffer + PayloadOffset,
                MIN( PayloadLength, (ReassembleDataLength-ReassembleBufferOffset) )
                );
            ReassembleBufferOffset += MIN( PayloadLength, (ReassembleDataLength-ReassembleBufferOffset) );

            ReassembleContext->BufferOffset = ReassembleBufferOffset;
        }
    }
    while( FALSE );

    if( NtStatus == STATUS_SUCCESS )
    {
        if( (CurrentFragment + 1) < FragmentCount )
        {
            NtStatus = STATUS_PENDING;
        }
    }
    //
    // If there are more fragments to come arm the inter-fragment timer.
    // Else set the final status in the reassemble context.
    //
    if( NtStatus != STATUS_PENDING )
    {
        ReassembleContext->NdisStatus = NtStatus;
    }
    else
    {
        MbbReqMgrTimerArm(
            RequestManager,
            MbbTimerTypeFragment,
            0,
            0,
            0,
            FALSE
            );
    }

    return NtStatus;
}

NDIS_STATUS
MbbNdisParseCommandResponse(
    __in ULONG                  TransactionId,
    __in PMBB_REQUEST_CONTEXT   Request
    )
{
    ULONG                           MessageFragmentLength;
    NDIS_STATUS                     NdisStatus = NDIS_STATUS_SUCCESS;
    PMBB_REQUEST_CONTEXT            OriginalRequest = NULL;
    PMBB_REQUEST_MANAGER            RequestManager = Request->RequestManager;
    PMBB_REASSEMBLE_CONTEXT         ReassembleContext = NULL;
    PMBB_COMMAND_FRAGMENT_HEADER    MessageFragmentHeader;

    do
    {
        MessageFragmentLength = Request->HandlerContext.Response.FragmentLength;
        MessageFragmentHeader = (PMBB_COMMAND_FRAGMENT_HEADER)(Request->HandlerContext.Response.FragmentBuffer);
        //
        // If the context cannot be acquired that means that another transaction
        // is using the context. Indicate an out of order fragment to the device
        // and hope it sends the right fragment soon. If not the inter-fragment
        // timeout will clean things up.
        //
        // If fragment is in order but there isnt a request reassemble the
        // fragment and then drop it. Reassembly is done to detect errors.
        //
        if( (ReassembleContext = MbbReqMgrAcquireSharedReassembleContext(
                                    RequestManager,
                                    MessageFragmentHeader,
                                    MessageFragmentLength,
                                    Request->ActivityId
                                    )) != NULL )
        {
            GUID ActivityId = {0};

            OriginalRequest = MbbReqMgrGetRequestByTransactionId(
                                RequestManager,
                                TransactionId
                                );

            // if there is an OriginalRequest corresponding to this response received
            // then we should use the original request activity id so that the original
            // request and its response can be traced end to end. If there is no
            // OriginalRequest then we will use the activity Id of the current request

            ActivityId = (OriginalRequest) ? OriginalRequest->ActivityId : Request->ActivityId;

            NdisStatus = MbbNdisReassembleResponseFragment(
                            (PUCHAR)MessageFragmentHeader,
                            MessageFragmentLength,
                            ReassembleContext,
                            OriginalRequest? TRUE: FALSE,    // Reassemble
                            RequestManager,
                            ActivityId
                            );
        }
        else
        {
            NdisStatus = NDIS_STATUS_INVALID_DATA;
        }
    }
    while( FALSE );

    if( NdisStatus != NDIS_STATUS_PENDING )
    {
        if( OriginalRequest != NULL )
        {
            //
            // Copy the common reassemble context used by the request manager
            // in to the per request reassemble context.
            //
            OriginalRequest->HandlerContext.Response = *ReassembleContext;

            MbbReqMgrQueueEvent(
                RequestManager,
                OriginalRequest,
                MbbRequestEventResponseReceived,
                OriginalRequest->HandlerContext.Response.Buffer,
                OriginalRequest->HandlerContext.Response.DataLength
                );
        }

        if( ReassembleContext != NULL )
        {
            MbbReqMgrReleaseSharedReassembleContext( RequestManager );
        }
    }

    if( OriginalRequest )
    {
        MbbReqMgrDerefRequest( OriginalRequest );
    }

    return NdisStatus;
}

NDIS_STATUS
MbbNdisParseStatusIndication(
    __in ULONG                  TransactionId,
    __in PMBB_REQUEST_CONTEXT   Request
    )
{
    ULONG                           MessageFragmentLength;
    NDIS_STATUS                     NdisStatus = NDIS_STATUS_SUCCESS;
    MBB_COMMAND                     Command;
    PMBB_REQUEST_MANAGER            RequestManager = Request->RequestManager;
    PMBB_REASSEMBLE_CONTEXT         ReassembleContext;
    PMBB_COMMAND_FRAGMENT_HEADER    MessageFragmentHeader;

    do
    {
        MessageFragmentLength = Request->HandlerContext.Response.FragmentLength;
        MessageFragmentHeader = (PMBB_COMMAND_FRAGMENT_HEADER)(Request->HandlerContext.Response.FragmentBuffer);
        //
        // If the context cannot be acquired that means that another transaction
        // is using the context. Indicate an out of order fragment to the device
        // and hope it sends the right fragment soon. If not the inter-fragment
        // timeout will clean things up.
        //
        if( (ReassembleContext = MbbReqMgrAcquireSharedReassembleContext(
                                    RequestManager,
                                    MessageFragmentHeader,
                                    MessageFragmentLength,
                                    Request->ActivityId
                                    )) != NULL )
        {
            NdisStatus = MbbNdisReassembleResponseFragment(
                            (PUCHAR)MessageFragmentHeader,
                            MessageFragmentLength,
                            ReassembleContext,
                            TRUE,    // Reassemble
                            RequestManager,
                            Request->ActivityId
                            );
        }
        else
        {
            NdisStatus = NDIS_STATUS_INVALID_DATA;
        }
    }
    while( FALSE );

    if( NdisStatus == NDIS_STATUS_SUCCESS )
    {
        //
        // Copy the common reassemble context used by the request manager
        // in to the per request reassemble context.
        //
        Request->HandlerContext.Response = RequestManager->Fragmentation.Reassemble;
        //
        // Track that this is an indication and not a response.
        //
        MbbReqMgrSetUnsolicitedIndication( Request );

        MbbReqMgrQueueEvent(
            RequestManager,
            Request,
            MbbRequestEventResponseReceived,
            Request->HandlerContext.Response.Buffer,
            Request->HandlerContext.Response.DataLength
            );
    }

    if( NdisStatus != NDIS_STATUS_PENDING )
    {
        if( ReassembleContext != NULL )
        {
            MbbReqMgrReleaseSharedReassembleContext( RequestManager );
        }
    }

    //
    // Tell the caller whether to keep or destroy the request
    //
    if( NdisStatus == NDIS_STATUS_SUCCESS )
        NdisStatus = NDIS_STATUS_PENDING;
    else if( NdisStatus == NDIS_STATUS_PENDING )
        NdisStatus = NDIS_STATUS_SUCCESS;

    return NdisStatus;
}

NDIS_STATUS
MbbNdisParseFunctionError(
    __in ULONG                  TransactionId,
    __in PMBB_REQUEST_CONTEXT   Request
    )
{
    PMBB_ERROR_MESSAGE      ErrorMessage = (PMBB_ERROR_MESSAGE)(Request->HandlerContext.Response.FragmentBuffer);
//    ULONG                   ErrorMessageLength = Request->HandlerContext.Response.FragmentLength;
    PMBB_REQUEST_CONTEXT    OriginalRequest = NULL;
    MBB_COMMAND             Command = {0};
    PCHAR                   CommandName;
    PMINIPORT_ADAPTER_CONTEXT Adapter = (PMINIPORT_ADAPTER_CONTEXT)MbbReqMgrGetAdapterHandle( Request );

    do
    {
        if( (OriginalRequest = MbbReqMgrGetRequestByTransactionId(
                                    Request->RequestManager,
                                    TransactionId
                                    )) != NULL )
        {
            TraceError( WMBCLASS_OID, "[MBIM_ERR][ReqID=0x%04x][TID=0x%08x] Received %!MbbError!. Cancelling request.",
                        OriginalRequest->RequestId,
                        TransactionId,
                        ErrorMessage->ErrorCode
                        );
            //
            // MBB_UUID_TO_HOST( ) is not needed here since
            // the command is being copied from the original
            // request where it is already in the host format.
            //
            RtlCopyMemory(
               &Command,
               &OriginalRequest->HandlerContext.Command.Command,
                sizeof(MBB_COMMAND)
                );
            MbbReqMgrQueueEvent(
                OriginalRequest->RequestManager,
                OriginalRequest,
                MbbRequestEventCancel,
                (PVOID)(NDIS_STATUS_REQUEST_ABORTED),
                0
                );
            MbbReqMgrDerefRequest( OriginalRequest );
        }
        else
        {
            TraceError( WMBCLASS_OID, "[MBIM_ERR][ReqID=0x%04x][TID=0x%08x] Received %!MbbError! for non-existing request",
                        Request->RequestId,
                        TransactionId,
                        ErrorMessage->ErrorCode
                        );
        }

        CommandName = MbbUtilGetCommandString( &Command );

        MbbWriteEvent(
           &MBIM_FUNCTION_ERROR_EVENT,
            OriginalRequest ? &(OriginalRequest->ActivityId) : NULL,
            NULL,
            6,
           &Adapter->TraceInstance,
            sizeof(Adapter->TraceInstance),
           &ErrorMessage->ErrorCode,
            sizeof(MBB_ERROR),
           &ErrorMessage->MessageHeader.MessageTransactionId,
            sizeof(ULONG),
           &Command.ServiceId,
            sizeof(GUID),
           &Command.CommandId,
            sizeof(ULONG),
            CommandName,
            strlen(CommandName) + 1
            );
    }
    while( FALSE );

    return NDIS_STATUS_SUCCESS;
}

VOID
MbbNdisParseResponseFragment(
    __in    MBB_PROTOCOL_HANDLE     AdapterHandle,
    __in    PMBB_REQUEST_CONTEXT    Request,
    __in    NDIS_STATUS             NdisStatus
    )
{
    ULONG                        RequestId = Request->RequestId;
    BOOLEAN                      DestroyRequest = TRUE;
    PVOID                        MessageBuffer;
    ULONG                        MessageLength;
    PMBB_MESSAGE_HEADER          MessageHeader;

    do
    {
        if( NdisStatus != NDIS_STATUS_SUCCESS )
        {
            TraceError( WMBCLASS_OID, "[MbbNdis][ReqId=0x%04x] GetEncapsulatedResponse FAILED, NdisStatus=%!STATUS!",
                        RequestId,
                        NdisStatus
                        );
            break;
        }

        MessageBuffer = Request->HandlerContext.Response.FragmentBuffer;
        MessageLength = Request->HandlerContext.Response.FragmentLength;
        MessageHeader = (PMBB_MESSAGE_HEADER)MessageBuffer;

        if( MessageLength < sizeof(MBB_MESSAGE_HEADER) )
        {
            TraceError( WMBCLASS_OID, "[MbbNdis][ReqId=0x%04x] INSUFFICIENT MessageBuffer Expected=%d > Received=%d",
                        RequestId,
                        sizeof(MBB_MESSAGE_HEADER),
                        MessageLength
                        );
            break;
        }

        switch( MessageHeader->MessageType )
        {
            case MBB_MESSAGE_TYPE_COMMAND_DONE:
            {
                if( MessageLength >= sizeof(MBB_COMMAND_FRAGMENT_HEADER) )
                {
                    MbbNdisParseCommandResponse( MessageHeader->MessageTransactionId, Request );
                }
                else
                {
                    TraceError( WMBCLASS_OID, "[MbbNdis][ReqId=0x%04x][TID=0x%08x] INSUFFICIENT MessageBuffer for %!MbbMsgType! Expected=%d > Received=%d",
                                RequestId,
                                MessageHeader->MessageTransactionId,
                                MessageHeader->MessageType,
                                sizeof(MBB_COMMAND_FRAGMENT_HEADER),
                                MessageLength
                                );
                }
            }
            break;

            case MBB_MESSAGE_TYPE_INDICATE_STATUS:
            {
                if( MessageLength >= sizeof(MBB_COMMAND_FRAGMENT_HEADER) )
                {
                    if( MbbNdisParseStatusIndication( MessageHeader->MessageTransactionId, Request ) == NDIS_STATUS_PENDING )
                    {
                        DestroyRequest = FALSE;
                    }
                }
                else
                {
                    TraceError( WMBCLASS_OID, "[MbbNdis][ReqId=0x%04x][TID=0x%08x] INSUFFICIENT MessageBuffer for %!MbbMsgType! Expected=%d > Received=%d",
                                RequestId,
                                MessageHeader->MessageTransactionId,
                                MessageHeader->MessageType,
                                sizeof(MBB_COMMAND_FRAGMENT_HEADER),
                                MessageLength
                                );
                }
            }
            break;

            case MBB_MESSAGE_TYPE_FUNCTION_ERROR:
            {
                if( MessageLength >= sizeof(MBB_ERROR_MESSAGE) && MessageHeader->MessageTransactionId > 0)
                {
                    if( MbbNdisParseFunctionError( MessageHeader->MessageTransactionId, Request ) == NDIS_STATUS_PENDING )
                    {
                        DestroyRequest = FALSE;
                    }
                }
                else if(MessageLength < sizeof(MBB_ERROR_MESSAGE))
                {
                    TraceError( WMBCLASS_OID, "[MbbNdis][ReqID=0x%04x][TID=0x%08x] INSUFFICIENT MessageBuffer for %!MbbMsgType! Expected=%d > Received=%d",
                                RequestId,
                                MessageHeader->MessageTransactionId,
                                MessageHeader->MessageType,
                                sizeof(MBB_ERROR_MESSAGE),
                                MessageLength
                                );
                    break;
                }
                else
                {
                    TraceWarn(WMBCLASS_OID, "[MbbNdis][ReqID=0x%04x][TID=0x%08x] MessageTransactionId is 0 for %!MbbMsgType! and ErrorCode %!MbbError!",
                        RequestId,
                        MessageHeader->MessageTransactionId,
                        MessageHeader->MessageType,
                        ((PMBB_ERROR_MESSAGE)MessageHeader)->ErrorCode
                        );
                    break;
                }
            }
            break;

            case MBB_MESSAGE_TYPE_OPEN_DONE:    __fallthrough;
            case MBB_MESSAGE_TYPE_CLOSE_DONE:   __fallthrough;
            default:
            {
                TraceError( WMBCLASS_OID, "[MbbNdis][ReqId=0x%04x][TID=0x%08x] UNEXPECTED MessageType=%!MbbMsgType!",
                            RequestId,
                            MessageHeader->MessageTransactionId,
                            MessageHeader->MessageType
                            );
            }
            break;
        }
    }
    while( FALSE );

    if( DestroyRequest == TRUE )
    {
        MbbReqMgrDestroyRequest( Request->RequestManager, Request );
    }
}

VOID
MbbNdisProcessResponseFragementWorker(
    __in PVOID  Context1,
    __in PVOID  Context2,
    __in PVOID  Context3,
    __in PVOID  Context4
    )
{
    PMBB_REQUEST_CONTEXT Request = (PMBB_REQUEST_CONTEXT)Context1;
    PMBB_REQUEST_MANAGER RequestManager = Request->RequestManager;

    MbbReqMgrQueueEvent(
        RequestManager,
        Request,
        MbbRequestEventSendComplete,
        (PVOID)(Request->HandlerContext.Response.NdisStatus),
        0
        );
    MbbReqMgrDerefRequest( Request );
}

VOID
MbbNdisGetResponseFragementComplete(
    __in    MBB_PROTOCOL_HANDLE     ProtocolHandle,
    __in    MBB_REQUEST_HANDLE      RequestHandle,
    __in    NTSTATUS                Status,
    __in    ULONG_PTR               ReceivedLength
    )
{
    NDIS_STATUS                 NdisStatus;
    PMBB_REQUEST_CONTEXT        Request;
    PMBB_REQUEST_MANAGER        RequestManager;
    PMINIPORT_ADAPTER_CONTEXT   Adapter = (PMINIPORT_ADAPTER_CONTEXT)ProtocolHandle;

    if( (RequestManager = MbbAdapterGetRequestManager( Adapter )) == NULL )
    {
        TraceError( WMBCLASS_REQUEST_MANAGER, "[MbbNdis][ReqId=0x%04x] FAILED to reference RequestManager to process request fragment", (ULONG)RequestHandle );
        return;
    }
    if( (Request = MbbReqMgrGetRequestById( RequestManager, (ULONG)RequestHandle )) != NULL )
    {
        Request->HandlerContext.Response.NdisStatus     = Status;
        Request->HandlerContext.Response.FragmentLength = (ULONG)ReceivedLength;
        //
        // Queue a work item so that the processing happens at PASSIVE_LEVEL.
        // On the response receive path string routines are called which require PASSIVE_LEVEL.
        //
        if( (NdisStatus = MbbWorkMgrQueueWorkItem(
                                RequestManager->WorkItemManagerHandle,
                                Request,
                                NULL,
                                NULL,
                                NULL,
                                MbbNdisProcessResponseFragementWorker
                                )) != NDIS_STATUS_SUCCESS )
        {
            TraceError( WMBCLASS_REQUEST_MANAGER, "[MbbNdis][ReqId=0x%04x] FAILED to queue work item to process request", Request->RequestId );
        }
    }
    else
    {
        TraceError( WMBCLASS_REQUEST_MANAGER, "[MbbNdis] UNABLE to find request, dropping fragment. Handle=%p", RequestHandle );
    }
    MbbReqMgrDeref( RequestManager );
}

NDIS_STATUS
MbbNdisGetResponseFragment(
    __in    MBB_PROTOCOL_HANDLE             AdapterHandle,
    __in    PMBB_REQUEST_CONTEXT            Request
    )
{
    ULONG                       RequestId;
    NTSTATUS                    NtStatus;
    NDIS_STATUS                 NdisStatus;

    RequestId = Request->RequestId;

    NtStatus  = MbbBusReceiveMessageFragment(
                    MbbNdisGetBusHandle( MbbReqMgrGetAdapterHandle( Request ) ),
                    (MBB_REQUEST_HANDLE)RequestId,
                    Request->HandlerContext.Response.FragmentBuffer,
                    Request->HandlerContext.Response.FragmentBufferLength,
                    &(Request->ActivityId),
                    MbbNdisGetResponseFragementComplete
                    );
    if( NtStatus == STATUS_PENDING )
    {
        TraceInfo( WMBCLASS_OID, "[MbbNdis][ReqId=0x%04x] Waiting to retrieve response fragment", RequestId );
        return NDIS_STATUS_PENDING;
    }
    else
    {
        ASSERT( NtStatus != STATUS_SUCCESS );
        TraceError( WMBCLASS_OID, "[MbbNdis][ReqId=0x%04x] FAILED to retrieve response fragment with status=%!status!", RequestId, NtStatus );
        return NDIS_STATUS_FAILURE;
    }
}

VOID
MbbNdisResponseFragmentAvailable(
    __in MBB_PROTOCOL_HANDLE ProtocolHandle
    )
{
    ULONG                       RequestId;
    NDIS_STATUS                 NdisStatus;
    NTSTATUS                    NtStatus;
    PMBB_REQUEST_CONTEXT        Request;
    PMBB_REQUEST_MANAGER        RequestManager = NULL;
    PMINIPORT_ADAPTER_CONTEXT   Adapter = (PMINIPORT_ADAPTER_CONTEXT)ProtocolHandle;

    do
    {
        if( (RequestManager = MbbAdapterGetRequestManager( Adapter )) == NULL )
        {
            TraceError( WMBCLASS_OID, "[MbbNdis] FAILED to reference RequestManager for GetResponse" );
            break;
        }
        if( (Request = MbbReqMgrCreateRequest(
                            RequestManager,
                            NULL,
                            0,
                            &NdisStatus)) == NULL )
        {
            TraceError( WMBCLASS_OID, "[MbbNdis] FAILED to allocate RequestContext for GetResponse" );
            break;
        }

        TraceInfo( WMBCLASS_OID, "[MbbNdis][ReqId=0x%04x] Created request to retrieve response.", Request->RequestId );

        Request->HandlerContext.Response.FragmentBuffer       = Request->RequestManager->Fragmentation.Reassemble.FragmentBuffer;
        Request->HandlerContext.Response.FragmentBufferLength = Request->RequestManager->Fragmentation.Reassemble.FragmentBufferLength;

        // Get an activity id to associate with this request
        NtStatus = MbbEventActivityIdCtl(&(Request->ActivityId));

        if(!NT_SUCCESS(NtStatus))
        {
            // Clear out the activity Id of the request
            NdisZeroMemory(&(Request->ActivityId),sizeof(GUID));
            TraceWarn( WMBCLASS_OID, "[MbbNdis][ReqId=0x%04x] Failed to create an activity Id for request to retrieve response.NtStatus = %d.", Request->RequestId, NtStatus );
        }
        else
        {
            TraceInfo( WMBCLASS_OID, "[MbbNdis][ReqId=0x%04x] Created an activity id %!GUID! for the request",Request->RequestId, &(Request->ActivityId));
        }

        NdisStatus = MbbReqMgrDispatchRequest(
                        Request,
                        TRUE,
                        MbbNdisGetResponseFragment,
                        MbbNdisParseResponseFragment,
                        MbbNdisIndicateStatus
                        );
        if( NdisStatus != NDIS_STATUS_PENDING )
        {
            MbbReqMgrDestroyRequest(
                RequestManager,
                Request
                );
        }
    }
    while( FALSE );

    if( RequestManager != NULL )
        MbbReqMgrDeref( RequestManager );
}


//
// Get Handlers
//


NDIS_STATUS
MbbNdisGetMediaSupported(
    __in PMBB_REQUEST_CONTEXT                                   Request,
    __in_bcount_opt(*InBufferSize) PUCHAR                       InBuffer,
    __in PULONG                                                 InBufferSize,
    __out_bcount_part_opt(*OutBufferSize,*OutBufferSize) PUCHAR OutBuffer,
    __inout PULONG                                              OutBufferSize
    )
{
    PULONG MediaSupported = (PULONG)OutBuffer;

    //
    // Size is already verified in MbbNdisMiniportOidRequest( )
    //
    __analysis_assume( *OutBufferSize >= sizeof(ULONG) );

    *MediaSupported = NdisMediumWirelessWan;
    *OutBufferSize  = sizeof(ULONG);
    return NDIS_STATUS_SUCCESS;
}

NDIS_STATUS
MbbNdisGetInterruptModeration(
    __in PMBB_REQUEST_CONTEXT                                   Request,
    __in_bcount_opt(*InBufferSize) PUCHAR                       InBuffer,
    __in PULONG                                                 InBufferSize,
    __out_bcount_part_opt(*OutBufferSize,*OutBufferSize) PUCHAR OutBuffer,
    __inout PULONG                                              OutBufferSize
    )
{
    PNDIS_INTERRUPT_MODERATION_PARAMETERS InterruptModeration = (PNDIS_INTERRUPT_MODERATION_PARAMETERS)OutBuffer;

    //
    // Size is already verified in MbbNdisMiniportOidRequest( )
    //
    __analysis_assume( *OutBufferSize >= sizeof(NDIS_INTERRUPT_MODERATION_PARAMETERS) );

    InterruptModeration->Header.Type        = NDIS_OBJECT_TYPE_DEFAULT;
    InterruptModeration->Header.Revision    = NDIS_INTERRUPT_MODERATION_PARAMETERS_REVISION_1;
    InterruptModeration->Header.Size        = sizeof(NDIS_INTERRUPT_MODERATION_PARAMETERS);
    InterruptModeration->Flags              = NDIS_INTERRUPT_MODERATION_CHANGE_NEEDS_RESET;
    InterruptModeration->InterruptModeration= NdisInterruptModerationNotSupported;

    *OutBufferSize = sizeof(NDIS_INTERRUPT_MODERATION_PARAMETERS);
    return NDIS_STATUS_SUCCESS;
}

NDIS_STATUS
MbbNdisGetVendorDriverVersion(
    __in PMBB_REQUEST_CONTEXT                                   Request,
    __in_bcount_opt(*InBufferSize) PUCHAR                       InBuffer,
    __in PULONG                                                 InBufferSize,
    __out_bcount_part_opt(*OutBufferSize,*OutBufferSize) PUCHAR OutBuffer,
    __inout PULONG                                              OutBufferSize
    )
{
    PULONG DriverVersion = (PULONG)OutBuffer;

    //
    // Size is already verified in MbbNdisMiniportOidRequest( )
    //
    __analysis_assume( *OutBufferSize >= sizeof(ULONG) );

    *DriverVersion = (WWAN_MAJOR_VERSION << 16) | (WWAN_MINOR_VERSION & 0xffff);
    *OutBufferSize = sizeof(ULONG);
    return NDIS_STATUS_SUCCESS;
}

NDIS_STATUS
MbbNdisGetVendorId(
    __in PMBB_REQUEST_CONTEXT                                   Request,
    __in_bcount_opt(*InBufferSize) PUCHAR                       InBuffer,
    __in PULONG                                                 InBufferSize,
    __out_bcount_part_opt(*OutBufferSize,*OutBufferSize) PUCHAR OutBuffer,
    __inout PULONG                                              OutBufferSize
    )
{
    PULONG VendorId = (PULONG)OutBuffer;

    //
    // Size is already verified in MbbNdisMiniportOidRequest( )
    //
    __analysis_assume( *OutBufferSize >= sizeof(ULONG) );

    //
    //  no mac address
    //
    *VendorId       = 0xFFFFFF;
    *OutBufferSize  = sizeof(ULONG);
    return NDIS_STATUS_SUCCESS;
}

NDIS_STATUS
MbbNdisGetVendorDescription(
    __in PMBB_REQUEST_CONTEXT                                   Request,
    __in_bcount_opt(*InBufferSize) PUCHAR                       InBuffer,
    __in PULONG                                                 InBufferSize,
    __out_bcount_part_opt(*OutBufferSize,*OutBufferSize) PUCHAR OutBuffer,
    __inout PULONG                                              OutBufferSize
    )
{
    PMINIPORT_ADAPTER_CONTEXT   Adapter=NULL;
    NDIS_STATUS                 Status;
    ULONG                       SourceLength=0;

    Adapter=(PMINIPORT_ADAPTER_CONTEXT)MbbReqMgrGetAdapterHandle(Request);

    //
    //  make sure the buffer is big enough to hold the string and a null since the counted string
    //  might not have null
    //
    SourceLength=(ULONG)Adapter->FriendlyName.Length + 1;

    if (*OutBufferSize >= SourceLength )
    {
        RtlZeroMemory(OutBuffer, SourceLength);
        RtlCopyMemory(OutBuffer, Adapter->FriendlyName.Buffer, Adapter->FriendlyName.Length);

        Status = NDIS_STATUS_SUCCESS;
    }
    else
    {
        Status = NDIS_STATUS_BUFFER_TOO_SHORT;
    }

    *OutBufferSize = SourceLength;

    return Status;
}

NDIS_STATUS
MbbNdisGetCurrentLookahead(
    __in PMBB_REQUEST_CONTEXT                                   Request,
    __in_bcount_opt(*InBufferSize) PUCHAR                       InBuffer,
    __in PULONG                                                 InBufferSize,
    __out_bcount_part_opt(*OutBufferSize,*OutBufferSize) PUCHAR OutBuffer,
    __inout PULONG                                              OutBufferSize
    )
{
    PULONG Lookahead = (PULONG)OutBuffer;
    //
    // Size is already verified in MbbNdisMiniportOidRequest( )
    //
    __analysis_assume( *OutBufferSize >= sizeof(ULONG) );
    //
    // Todo: Return a value.
    //
    *Lookahead      = 0;
    *OutBufferSize  = sizeof(ULONG);
    return NDIS_STATUS_SUCCESS;
}

NDIS_STATUS
MbbNdisGetReceiveBlockSize(
    __in PMBB_REQUEST_CONTEXT                                   Request,
    __in_bcount_opt(*InBufferSize) PUCHAR                       InBuffer,
    __in PULONG                                                 InBufferSize,
    __out_bcount_part_opt(*OutBufferSize,*OutBufferSize) PUCHAR OutBuffer,
    __inout PULONG                                              OutBufferSize
    )
{
    PMINIPORT_ADAPTER_CONTEXT   Adapter=NULL;
    PULONG  BlockSize=(PULONG)OutBuffer;

    Adapter=(PMINIPORT_ADAPTER_CONTEXT)MbbReqMgrGetAdapterHandle(Request);

    //
    // Size is already verified in MbbNdisMiniportOidRequest( )
    //
    __analysis_assume( *OutBufferSize >= sizeof(ULONG) );

    if (Adapter->BusParams.IsErrataDevice)
    {
        *BlockSize=Adapter->BusParams.MTU;
    } else {
        *BlockSize=Adapter->BusParams.MaxSegmentSize;
    }

    return NDIS_STATUS_SUCCESS;
}

NDIS_STATUS
MbbNdisGetReceiveBufferSpace(
    __in PMBB_REQUEST_CONTEXT                                   Request,
    __in_bcount_opt(*InBufferSize) PUCHAR                       InBuffer,
    __in PULONG                                                 InBufferSize,
    __out_bcount_part_opt(*OutBufferSize,*OutBufferSize) PUCHAR OutBuffer,
    __inout PULONG                                              OutBufferSize
    )
{
    PMINIPORT_ADAPTER_CONTEXT   Adapter=NULL;
    PULONG  BufferSpace=(PULONG)OutBuffer;

    Adapter=(PMINIPORT_ADAPTER_CONTEXT)MbbReqMgrGetAdapterHandle(Request);

    //
    // Size is already verified in MbbNdisMiniportOidRequest( )
    //
    __analysis_assume( *OutBufferSize >= sizeof(ULONG) );

    *BufferSpace=Adapter->BusParams.MaxOutNtb;

    return NDIS_STATUS_SUCCESS;

}

NDIS_STATUS
MbbNdisGetTransmitBlockSize(
    __in PMBB_REQUEST_CONTEXT                                   Request,
    __in_bcount_opt(*InBufferSize) PUCHAR                       InBuffer,
    __in PULONG                                                 InBufferSize,
    __out_bcount_part_opt(*OutBufferSize,*OutBufferSize) PUCHAR OutBuffer,
    __inout PULONG                                              OutBufferSize
    )
{
    PMINIPORT_ADAPTER_CONTEXT   Adapter=NULL;
    PULONG  BlockSize=(PULONG)OutBuffer;

    Adapter=(PMINIPORT_ADAPTER_CONTEXT)MbbReqMgrGetAdapterHandle(Request);

    //
    // Size is already verified in MbbNdisMiniportOidRequest( )
    //
    __analysis_assume( *OutBufferSize >= sizeof(ULONG) );

    if (Adapter->BusParams.IsErrataDevice)
    {
        *BlockSize=Adapter->BusParams.MTU;
    } else {
        *BlockSize=Adapter->BusParams.MaxSegmentSize;
    }

    return NDIS_STATUS_SUCCESS;
}

NDIS_STATUS
MbbNdisGetTransmitBufferSpace(
    __in PMBB_REQUEST_CONTEXT                                   Request,
    __in_bcount_opt(*InBufferSize) PUCHAR                       InBuffer,
    __in PULONG                                                 InBufferSize,
    __out_bcount_part_opt(*OutBufferSize,*OutBufferSize) PUCHAR OutBuffer,
    __inout PULONG                                              OutBufferSize
    )
{
    PMINIPORT_ADAPTER_CONTEXT   Adapter=NULL;
    PULONG  BufferSpace=(PULONG)OutBuffer;

    Adapter=(PMINIPORT_ADAPTER_CONTEXT)MbbReqMgrGetAdapterHandle(Request);

    //
    // Size is already verified in MbbNdisMiniportOidRequest( )
    //
    __analysis_assume( *OutBufferSize >= sizeof(ULONG) );

    *BufferSpace=Adapter->BusParams.MaxOutNtb;

    return NDIS_STATUS_SUCCESS;
}

NDIS_STATUS
MbbNdisGetMaximumTotalSize(
    __in PMBB_REQUEST_CONTEXT                                   Request,
    __in_bcount_opt(*InBufferSize) PUCHAR                       InBuffer,
    __in PULONG                                                 InBufferSize,
    __out_bcount_part_opt(*OutBufferSize,*OutBufferSize) PUCHAR OutBuffer,
    __inout PULONG                                              OutBufferSize
    )
{
    PMINIPORT_ADAPTER_CONTEXT   Adapter=NULL;
    PULONG  BlockSize=(PULONG)OutBuffer;

    Adapter=(PMINIPORT_ADAPTER_CONTEXT)MbbReqMgrGetAdapterHandle(Request);

    //
    // Size is already verified in MbbNdisMiniportOidRequest( )
    //
    __analysis_assume( *OutBufferSize >= sizeof(ULONG) );

    if (Adapter->BusParams.IsErrataDevice)
    {
        *BlockSize=Adapter->BusParams.MTU;
    } else {
        *BlockSize=Adapter->BusParams.MaxSegmentSize;
    }

    return NDIS_STATUS_SUCCESS;
}

NDIS_STATUS
MbbNdisGetFramesReceived(
    __in PMBB_REQUEST_CONTEXT                                   Request,
    __in_bcount_opt(*InBufferSize) PUCHAR                       InBuffer,
    __in PULONG                                                 InBufferSize,
    __out_bcount_part_opt(*OutBufferSize,*OutBufferSize) PUCHAR OutBuffer,
    __inout PULONG                                              OutBufferSize
    )
{
    PULONGLONG ReceivedFrames = (PULONGLONG)(OutBuffer);
    PMINIPORT_ADAPTER_CONTEXT Adapter = (PMINIPORT_ADAPTER_CONTEXT)(Request->RequestManager->AdapterContext);

    //
    // Size is already verified in MbbNdisMiniportOidRequest( )
    //
    __analysis_assume( *OutBufferSize >= sizeof(ULONGLONG) );

    *ReceivedFrames = Adapter->GenRcvFramesOk;
    *OutBufferSize  = sizeof(ULONGLONG);
    return NDIS_STATUS_SUCCESS;
}

NDIS_STATUS
MbbNdisGetFramesTransmitted(
    __in PMBB_REQUEST_CONTEXT                                   Request,
    __in_bcount_opt(*InBufferSize) PUCHAR                       InBuffer,
    __in PULONG                                                 InBufferSize,
    __out_bcount_part_opt(*OutBufferSize,*OutBufferSize) PUCHAR OutBuffer,
    __inout PULONG                                              OutBufferSize
    )
{
    PULONGLONG TransmittedFrames = (PULONGLONG)(OutBuffer);
    PMINIPORT_ADAPTER_CONTEXT Adapter = (PMINIPORT_ADAPTER_CONTEXT)(Request->RequestManager->AdapterContext);

    //
    // Size is already verified in MbbNdisMiniportOidRequest( )
    //
    __analysis_assume( *OutBufferSize >= sizeof(ULONGLONG) );

    *TransmittedFrames  = Adapter->GenXmitFramesOk;
    *OutBufferSize      = sizeof(ULONGLONG);
    return NDIS_STATUS_SUCCESS;
}

NDIS_STATUS
MbbNdisGetDriverCaps(
    __in PMBB_REQUEST_CONTEXT                                   Request,
    __in_bcount_opt(*InBufferSize) PUCHAR                       InBuffer,
    __in PULONG                                                 InBufferSize,
    __out_bcount_part_opt(*OutBufferSize,*OutBufferSize) PUCHAR OutBuffer,
    __inout PULONG                                              OutBufferSize
    )
{
    PMINIPORT_ADAPTER_CONTEXT Adapter               = (PMINIPORT_ADAPTER_CONTEXT) MbbReqMgrGetAdapterHandle(Request);

    PNDIS_WWAN_DRIVER_CAPS DriverCaps = (PNDIS_WWAN_DRIVER_CAPS)OutBuffer;

    //
    // Size is already verified in MbbNdisMiniportOidRequest( )
    //
    __analysis_assume( *OutBufferSize >= sizeof(NDIS_WWAN_DRIVER_CAPS) );

    DriverCaps->Header.Type                 = NDIS_OBJECT_TYPE_DEFAULT;
    DriverCaps->Header.Revision             = NDIS_WWAN_DRIVER_CAPS_REVISION_1 ;
    DriverCaps->Header.Size                 = sizeof(NDIS_WWAN_DRIVER_CAPS);
    DriverCaps->DriverCaps.ulMajorVersion   = WWAN_MAJOR_VERSION;
    DriverCaps->DriverCaps.ulMinorVersion   = WWAN_MINOR_VERSION;
    DriverCaps->DriverCaps.ulDriverCaps     = WWAN_DRIVER_CAPS_NONE;

    if (MbbBusIsUde(Adapter->BusHandle))
    {
        DriverCaps->DriverCaps.ulDriverCaps = WWAN_DRIVER_CAPS_UDE;
    }
    *OutBufferSize = sizeof(NDIS_WWAN_DRIVER_CAPS);
    return NDIS_STATUS_SUCCESS;
}


NDIS_STATUS
MbbNdisGetConnectState(
    __in PMBB_REQUEST_CONTEXT                                   Request,
    __in_bcount_opt(*InBufferSize) PUCHAR                       InBuffer,
    __in PULONG                                                 InBufferSize,
    __out_bcount_part_opt(*OutBufferSize,*OutBufferSize) PUCHAR OutBuffer,
    __inout PULONG                                              OutBufferSize
    )
{
    PMINIPORT_ADAPTER_CONTEXT Adapter               = (PMINIPORT_ADAPTER_CONTEXT) MbbReqMgrGetAdapterHandle(Request);
    PMBB_CONTEXT_STATE        MbbGetContextState    = NULL;
    PMBB_PORT                 Port                  = NULL;
    NDIS_STATUS               Status                = NDIS_STATUS_SUCCESS;

    UNREFERENCED_PARAMETER(InBuffer);
    UNREFERENCED_PARAMETER(InBufferSize);

    if( (MbbGetContextState = (PMBB_CONTEXT_STATE) ALLOCATE_NONPAGED_POOL( sizeof(MBB_CONTEXT_STATE) )) == NULL )
    {
        return NDIS_STATUS_RESOURCES;
    }
    Request->HandlerContext.DataToFreeOnCompletion = MbbGetContextState;

    Port = MbbWwanTranslatePortNumberToPort(
                Adapter,
                Request->OidContext.OidRequest->PortNumber);

    if(Port!= NULL)
    {
        ULONG MaxActivatedContexts = 0;
        ULONG SessionId = MBB_INVALID_SESSION_ID;

        MbbAdapterPortsLock(Adapter);
        MaxActivatedContexts = Adapter->MaxActivatedContexts;
        MbbAdapterPortsUnlock(Adapter);

        RtlZeroMemory(MbbGetContextState, sizeof(MBB_CONTEXT_STATE));

        SessionId = MbbWwanGetPortSessionId(Port);

        if(SessionId < MaxActivatedContexts)
        {
            MbbGetContextState->SessionId = SessionId;

            Status = MbbUtilQueryAttributeWithParameter(
                     Request,
                     (PUCHAR) MbbGetContextState,
                     sizeof(MBB_CONTEXT_STATE)
                     );
        }
        else
        {
            // Fail this OID request because we cannot query the device with an invalid session ID.
            // If we want to send a context deactivated indication instead of failure, we need to do it outside the
            // context of this OID request because it may lead to deadlocks. Currently returning a failure here
            // because it is equivalent and less complicated. Note that Wwansvc is not sending OID_WWAN_CONNECT
            // query for disconnected additional PDP contexts. The only case when this OID request may come is for
            // a disconnected Port 0 context, which is not an issue because we are setting session ID as 0 for Port 0
            // always and will never end up here.

            Status = NDIS_STATUS_FAILURE;

            ASSERT(IS_ALLOCATED_PORT_NUMBER(Request->OidContext.OidRequest->PortNumber));

            TraceError( WMBCLASS_OID, "[MbbNdis][ReqId=0x%04x] MbbNdisGetConnectState failed for a Port: [%lu]. Requested sessionId: [%lu] Status=%!status!",
                    Request->RequestId,
                    Request->OidContext.OidRequest->PortNumber,
                    SessionId,
                    Status
                    );
        }

        // Remove the reference added during find
        Dereference(Port);
    }
    else
    {
        Status = NDIS_STATUS_INVALID_PORT;
    }

    return Status;
}

NDIS_STATUS
MbbIpGetConfiguration(
    __in PMBB_REQUEST_CONTEXT                                   Request,
    __in_bcount_opt(*InBufferSize) PUCHAR                       InBuffer,
    __in PULONG                                                 InBufferSize,
    __out_bcount_part_opt(*OutBufferSize,*OutBufferSize) PUCHAR OutBuffer,
    __inout PULONG                                              OutBufferSize
    )
{
    PMBB_IP_ADDRESS_INFO MbbGetIpAddressInfo = NULL;

    UNREFERENCED_PARAMETER(InBuffer);
    UNREFERENCED_PARAMETER(InBufferSize);

    if( (MbbGetIpAddressInfo = (PMBB_IP_ADDRESS_INFO) ALLOCATE_NONPAGED_POOL( sizeof(MBB_IP_ADDRESS_INFO) )) == NULL )
    {
        return NDIS_STATUS_RESOURCES;
    }
    Request->HandlerContext.DataToFreeOnCompletion = MbbGetIpAddressInfo;

    RtlZeroMemory(MbbGetIpAddressInfo, sizeof(MBB_IP_ADDRESS_INFO));

    MbbGetIpAddressInfo->SessionId = Request->HandlerContext.Parameters.IpAddress.SessionId;

    return MbbUtilQueryAttributeWithParameter(
                Request,
                (PUCHAR) MbbGetIpAddressInfo,
                sizeof(MBB_IP_ADDRESS_INFO)
                );
}

NDIS_STATUS
MbbNdisGetVisibleProviders(
    __in PMBB_REQUEST_CONTEXT                                   Request,
    __in_bcount_opt(*InBufferSize) PUCHAR                       InBuffer,
    __in PULONG                                                 InBufferSize,
    __out_bcount_part_opt(*OutBufferSize,*OutBufferSize) PUCHAR OutBuffer,
    __inout PULONG                                              OutBufferSize
    )
{
    PNDIS_WWAN_GET_VISIBLE_PROVIDERS NdisGetVisibleProviders = (PNDIS_WWAN_GET_VISIBLE_PROVIDERS)InBuffer;
    PMBB_GET_VISIBLE_PROVIDERS MbbGetVisible;
    NDIS_STATUS NdisStatus;

    if( (MbbGetVisible = (PMBB_GET_VISIBLE_PROVIDERS) ALLOCATE_NONPAGED_POOL( sizeof(MBB_GET_VISIBLE_PROVIDERS) )) == NULL )
    {
        return NDIS_STATUS_RESOURCES;
    }
    Request->HandlerContext.DataToFreeOnCompletion = MbbGetVisible;

    NdisStatus = MbbUtilWwanToMbbGetVisible(
                    &NdisGetVisibleProviders->VisibleProviders,
                    MbbGetVisible
                    );
    if( NdisStatus != NDIS_STATUS_SUCCESS )
    {
        TraceError( WMBCLASS_OID, "[MbbNdis][ReqId=0x%04x] FAILED to convert GET_VISIBLE_PROVIDER with status=%!status!",
                    Request->RequestId,
                    NdisStatus
                    );
        return NdisStatus;
    }
    return MbbUtilQueryAttributeWithParameter(
                Request,
                (PUCHAR) MbbGetVisible,
                sizeof(MBB_GET_VISIBLE_PROVIDERS)
                );
}


NDIS_STATUS
MbbNdisSmsRead(
    __in PMBB_REQUEST_CONTEXT                                   Request,
    __in_bcount_opt(*InBufferSize) PUCHAR                       InBuffer,
    __in PULONG                                                 InBufferSize,
    __out_bcount_part_opt(*OutBufferSize,*OutBufferSize) PUCHAR OutBuffer,
    __inout PULONG                                              OutBufferSize
    )
{
    PNDIS_WWAN_SMS_READ NdisSmsRead = (PNDIS_WWAN_SMS_READ)InBuffer;
    PMBB_SMS_READ       MbbSmsRead;

    if( (MbbSmsRead = (PMBB_SMS_READ) ALLOCATE_NONPAGED_POOL( sizeof(MBB_SMS_READ) )) == NULL )
    {
        return NDIS_STATUS_RESOURCES;
    }
    Request->HandlerContext.DataToFreeOnCompletion = MbbSmsRead;

    MbbUtilWwanToMbbSmsRead(
        &NdisSmsRead->SmsRead,
        MbbSmsRead
        );
    return MbbUtilQueryAttributeWithParameter(
                Request,
                (PUCHAR) MbbSmsRead,
                sizeof(MBB_SMS_READ)
                );
}


NDIS_STATUS
MbbNdisEnumerateDeviceServices(
    __in PMBB_REQUEST_CONTEXT                                   Request,
    __in_bcount_opt(*InBufferSize) PUCHAR                       InBuffer,
    __in PULONG                                                 InBufferSize,
    __out_bcount_part_opt(*OutBufferSize,*OutBufferSize) PUCHAR OutBuffer,
    __inout PULONG                                              OutBufferSize
    )
{
    NDIS_STATUS                     NdisStatus = NDIS_STATUS_SUCCESS;
    ULONG                           DeviceElementCount = 0;
    ULONG                           NdisDeviceServicesInfoSize;
    PNDIS_WWAN_SUPPORTED_DEVICE_SERVICES  NdisDeviceServicesInfo = NULL;
    PWWAN_DEVICE_SERVICE_ENTRY      NdisDeviceServiceEntry = NULL;
    ULONG                           devIndex = 0;
    ULONG                           oidIndex = 0;
    PMINIPORT_ADAPTER_CONTEXT       Adapter = NULL;
    NDIS_STATUS_INDICATION          StatusIndication;

    // Return the device services list from the cached state
    do
    {
        //
        // For OID queries, get the device service list from the cache
        //
        Adapter = ((PMINIPORT_ADAPTER_CONTEXT)MbbReqMgrGetAdapterHandle( Request ));

        // We start with allocating memory for all entries. We would only report the non-native list to the OS
        DeviceElementCount = Adapter->DeviceServiceState.ServicesCount;
        NdisDeviceServicesInfoSize  = sizeof(NDIS_WWAN_SUPPORTED_DEVICE_SERVICES) + ( DeviceElementCount * sizeof(WWAN_DEVICE_SERVICE_ENTRY) );

        if( (NdisDeviceServicesInfo = ALLOCATE_NONPAGED_POOL(NdisDeviceServicesInfoSize)) == NULL )
        {
            TraceError( WMBCLASS_OID, "[MbbNdis][ReqId=0x%04x] FAILED to allocate %d bytes for NDIS_STATUS_WWAN_SUPPORTED_DEVICE_SERVICES, Source Service Count=%d",
                        Request->RequestId,
                        NdisDeviceServicesInfoSize,
                        DeviceElementCount
                        );
            NdisStatus = NDIS_STATUS_RESOURCES;
            break;
        }

        // Start populating the indication
        NdisDeviceServicesInfo->Header.Type     = NDIS_OBJECT_TYPE_DEFAULT;
        NdisDeviceServicesInfo->Header.Size     = SIZEOF_NDIS_WWAN_SUPPORTED_DEVICE_SERVICES_1;
        NdisDeviceServicesInfo->Header.Revision = NDIS_WWAN_SUPPORTED_DEVICE_SERVICES_REVISION_1;

        // Always reporting success
        NdisDeviceServicesInfo->uStatus         = WWAN_STATUS_SUCCESS;

        NdisDeviceServicesInfo->DeviceServices.uMaxCommandDataSize  = Adapter->BusParams.FragmentSize;
        NdisDeviceServicesInfo->DeviceServices.uMaxSessionDataSize  = MBB_FRAGMENATION_BULK_BUFFER_LENGTH;
        NdisDeviceServicesInfo->DeviceServices.uMaxSessionCount     = Adapter->DeviceServiceState.MaxDSSSessions;

        NdisDeviceServicesInfo->DeviceServices.ListHeader.ElementType = WwanStructDeviceServiceEntry;
        NdisDeviceServicesInfo->DeviceServices.ListHeader.ElementCount = 0;

        // Populate the device service entries
        NdisDeviceServiceEntry = (PWWAN_DEVICE_SERVICE_ENTRY)(((PUCHAR)NdisDeviceServicesInfo)
                                    + sizeof(NDIS_WWAN_SUPPORTED_DEVICE_SERVICES));
        for (devIndex = 0; devIndex < DeviceElementCount; devIndex++)
        {
            if (!MbbUtilIsNativeMbnService(&(NdisDeviceServiceEntry[devIndex].DeviceServiceGuid)))
            {
                // Cache is already in host format
                RtlCopyMemory(&(NdisDeviceServiceEntry[oidIndex].DeviceServiceGuid),
                    &(Adapter->DeviceServiceState.ServicesList[devIndex].DeviceServiceId),
                    sizeof(GUID)
                    );

                NdisDeviceServiceEntry[oidIndex].SessionCapability =
                    Adapter->DeviceServiceState.ServicesList[devIndex].DSSCapability;
                NdisDeviceServiceEntry[oidIndex].uMaxSessionInstances =
                    Adapter->DeviceServiceState.ServicesList[devIndex].MaxDSSInstances;

                NdisDeviceServicesInfo->DeviceServices.ListHeader.ElementCount++;
                oidIndex++;
            }
        }

        //
        // Indicate ndis status
        //
        MBB_INITIALIZE_REQUEST_NDIS_STATUS_INDICATION(
            &StatusIndication,
            Request,
            NDIS_STATUS_WWAN_SUPPORTED_DEVICE_SERVICES
            );

        StatusIndication.StatusBuffer       = NdisDeviceServicesInfo;
        // Use the actual size for
        StatusIndication.StatusBufferSize   = sizeof(NDIS_WWAN_SUPPORTED_DEVICE_SERVICES) +
                    ( NdisDeviceServicesInfo->DeviceServices.ListHeader.ElementCount * sizeof(WWAN_DEVICE_SERVICE_ENTRY) );

        TraceInfo(  WMBCLASS_OID, "[MbbNdis][ReqId=0x%04x] Indicating NDIS_STATUS_WWAN_SUPPORTED_DEVICE_SERVICES with %d entries",
                    Request->RequestId,
                    NdisDeviceServicesInfo->DeviceServices.ListHeader.ElementCount
                    );

        //
        //  a special status that the oid completion handler will fix-up and complete the request instead of waiting
        //  a response from the device that is not coming.
        //
        NdisStatus=MBB_STATUS_INDICATION_ALREADY_SENT ;

        NdisMIndicateStatusEx(
            ((PMINIPORT_ADAPTER_CONTEXT)MbbReqMgrGetAdapterHandle( Request ))->MiniportAdapterHandle,
            &StatusIndication
            );

    }
    while( FALSE );

    //
    // Local Cleanup
    //
    if( NdisDeviceServicesInfo != NULL )
    {
        FREE_POOL( NdisDeviceServicesInfo );
    }

    return NdisStatus;
}

NDIS_STATUS
MbbNdisGetDeviceServiceCommand(
    __in PMBB_REQUEST_CONTEXT                                   Request,
    __in_bcount_opt(*InBufferSize) PUCHAR                       InBuffer,
    __in PULONG                                                 InBufferSize,
    __out_bcount_part_opt(*OutBufferSize,*OutBufferSize) PUCHAR OutBuffer,
    __inout PULONG                                              OutBufferSize
    )
{
    return MbbUtilDeviceServiceCommand(
            Request,
            InBuffer,
            InBufferSize
            );
}


NDIS_STATUS
MbbNdisEnumerateDeviceServiceCommands(
    __in PMBB_REQUEST_CONTEXT                                   Request,
    __in_bcount_opt(*InBufferSize) PUCHAR                       InBuffer,
    __in PULONG                                                 InBufferSize,
    __out_bcount_part_opt(*OutBufferSize,*OutBufferSize) PUCHAR OutBuffer,
    __inout PULONG                                              OutBufferSize
    )
{
    NDIS_STATUS                     NdisStatus = NDIS_STATUS_SUCCESS;
    // The input data comes in as OutBuffer
    PNDIS_WWAN_ENUMERATE_DEVICE_SERVICE_COMMANDS pRequestBuffer = (PNDIS_WWAN_ENUMERATE_DEVICE_SERVICE_COMMANDS )OutBuffer;
    PMINIPORT_ADAPTER_CONTEXT       Adapter = NULL;
    NDIS_STATUS_INDICATION          StatusIndication;
    PMBB_DS                         deviceServiceEntry = NULL;
    ULONG                           NdisCommandsInfoSize;
    PNDIS_WWAN_DEVICE_SERVICE_SUPPORTED_COMMANDS  NdisCommandsInfo = NULL;

    do
    {
        //
        // For CID list queries, use the info from the cache
        //
        Adapter = ((PMINIPORT_ADAPTER_CONTEXT)MbbReqMgrGetAdapterHandle( Request ));

        // Find the device service for this GUID
        deviceServiceEntry =  MbbUtilFindDeviceService(Adapter,
                                &pRequestBuffer->DeviceServiceGuid
                                );
        if (deviceServiceEntry == NULL)
        {
            TraceError( WMBCLASS_OID, "[MbbNdis][ReqId=0x%04x] Enumerate COMMANDs request for invalid device service ID",
                        Request->RequestId
                        );
            NdisStatus = NDIS_STATUS_NOT_SUPPORTED;
            break;
        }

        // Allocate the buffer for the indication
        NdisCommandsInfoSize  = sizeof(NDIS_WWAN_DEVICE_SERVICE_SUPPORTED_COMMANDS) +
                                                + ( deviceServiceEntry->CIDCount * sizeof(ULONG) );

        if( (NdisCommandsInfo = ALLOCATE_NONPAGED_POOL(NdisCommandsInfoSize)) == NULL )
        {
            TraceError( WMBCLASS_OID, "[MbbNdis][ReqId=0x%04x] FAILED to allocate %d bytes for NDIS_STATUS_WWAN_DEVICE_SERVICE_SUPPORTED_COMMANDS, Source Service Count=%d",
                        Request->RequestId,
                        NdisCommandsInfoSize,
                        deviceServiceEntry->CIDCount
                        );
            NdisStatus = NDIS_STATUS_RESOURCES;
            break;
        }

        // Start populating the indication
        NdisCommandsInfo->Header.Type     = NDIS_OBJECT_TYPE_DEFAULT;
        NdisCommandsInfo->Header.Size     = SIZEOF_NDIS_WWAN_DEVICE_SERVICE_SUPPORTED_COMMANDS_1;
        NdisCommandsInfo->Header.Revision = NDIS_WWAN_DEVICE_SERVICE_SUPPORTED_COMMANDS_REVISION_1;

        // Always reporting success
        NdisCommandsInfo->uStatus         = WWAN_STATUS_SUCCESS;

        RtlCopyMemory(&NdisCommandsInfo->SupportedCommands.DeviceServiceGuid,
            &pRequestBuffer->DeviceServiceGuid,
            sizeof(GUID)
            );

        NdisCommandsInfo->SupportedCommands.ListHeader.ElementCount = deviceServiceEntry->CIDCount;
        NdisCommandsInfo->SupportedCommands.ListHeader.ElementType  = WwanStructDeviceServiceCommandId;

        // Populate the command list
        RtlCopyMemory(((PUCHAR)NdisCommandsInfo) + sizeof(NDIS_WWAN_DEVICE_SERVICE_SUPPORTED_COMMANDS),
            deviceServiceEntry->CIDList,
            deviceServiceEntry->CIDCount * sizeof(ULONG)
            );

        //
        // Indicate ndis status
        //
        MBB_INITIALIZE_REQUEST_NDIS_STATUS_INDICATION(
            &StatusIndication,
            Request,
            NDIS_STATUS_WWAN_DEVICE_SERVICE_SUPPORTED_COMMANDS
            );

        StatusIndication.StatusBuffer       = NdisCommandsInfo;
        StatusIndication.StatusBufferSize   = NdisCommandsInfoSize;

        TraceInfo(  WMBCLASS_OID, "[MbbNdis][ReqId=0x%04x] Indicating NDIS_STATUS_WWAN_DEVICE_SERVICE_SUPPORTED_COMMANDS with %d entries",
                    Request->RequestId,
                    NdisCommandsInfo->SupportedCommands.ListHeader.ElementCount
                    );

        //
        //  a special status that the oid completion handler will fix-up and complete the request instead of waiting
        //  a response from the device that is not coming.
        //
        NdisStatus=MBB_STATUS_INDICATION_ALREADY_SENT ;

        NdisMIndicateStatusEx(
            ((PMINIPORT_ADAPTER_CONTEXT)MbbReqMgrGetAdapterHandle( Request ))->MiniportAdapterHandle,
            &StatusIndication
            );

    }
    while( FALSE );

    //
    // Local Cleanup
    //
    if( NdisCommandsInfo != NULL )
    {
        FREE_POOL( NdisCommandsInfo );
    }

    return NdisStatus;
}

NDIS_STATUS
MbbNdisGetMultiCarrierDsCidList(
    __in PMBB_REQUEST_CONTEXT                                   Request,
    __in_bcount_opt(*InBufferSize) PUCHAR                       InBuffer,
    __in PULONG                                                 InBufferSize,
    __out_bcount_part_opt(*OutBufferSize,*OutBufferSize) PUCHAR OutBuffer,
    __inout PULONG                                              OutBufferSize
    )
/*++
    HACK - The input parameter is passed in the Request instead of the above parameters.
    The hack is necessary since there is no good way to pass parameters to handlers from
    an internal request.
--*/
{
    MBB_COMMAND Command;
    NDIS_STATUS NdisStatus = NDIS_STATUS_SUCCESS;
    PMINIPORT_ADAPTER_CONTEXT Adapter = NULL;

    UNREFERENCED_PARAMETER(InBuffer);
    UNREFERENCED_PARAMETER(InBufferSize);
    UNREFERENCED_PARAMETER(OutBuffer);
    UNREFERENCED_PARAMETER(OutBufferSize);

    do
    {
        Command = Request->OidHandler->ResponseCommand;

        if( (NdisStatus = MbbUtilSetupCommandMessage(
                                Request,
                                &Command,
                                MBB_COMMAND_TYPE_QUERY,
                                (PUCHAR)&Request->HandlerContext.Parameters.DeviceCaps.CurrentQueriedDeviceService,
                                sizeof(UUID)
                                )) != NDIS_STATUS_SUCCESS )
        {
            TraceError( WMBCLASS_INIT, "[MbbNdis][ReqId=0x%04x] FAILED to setup command message for MbbNdisGetMultiCarrierDsCidList with status=%!status!",
                        Request->RequestId, NdisStatus );
            break;
        }
        //
        // Call the wrapper routine to send each fragment.
        // The wrapper will cleanup fragments in case of
        // success or failure.
        //
        NdisStatus = MbbUtilSendMessageFragmentsAndLog( Request );
        if( NdisStatus != NDIS_STATUS_SUCCESS &&
            NdisStatus != NDIS_STATUS_PENDING )
        {
            TraceError( WMBCLASS_INIT, "[MbbNdis][ReqId=0x%04x] FAILED to send message fragments for MbbNdisGetMultiCarrierDsCidList with status=%!status!",
                        Request->RequestId, NdisStatus );
        }
    }
    while( FALSE );

    return NdisStatus;
}

NDIS_STATUS
MbbNdisGetUiccFileStatus(
    __in PMBB_REQUEST_CONTEXT                                   Request,
    __in_bcount_opt(*InBufferSize) PUCHAR                       InBuffer,
    __in PULONG                                                 InBufferSize,
    __out_bcount_part_opt(*OutBufferSize,*OutBufferSize) PUCHAR OutBuffer,
    __inout PULONG                                              OutBufferSize
    )
{
    PNDIS_WWAN_UICC_FILE_PATH   NdisUiccFilePath = (PNDIS_WWAN_UICC_FILE_PATH)InBuffer;
    PMBB_UICC_FILE_PATH         MbbUiccFilePath = NULL;
    NDIS_STATUS                 NdisStatus;
    ULONG                       BufferSize = 0;

    NdisStatus = MbbUtilWwanToMbbUiccFilePath(
        &NdisUiccFilePath->UiccFilePath,
        &MbbUiccFilePath,
        &BufferSize
        );

    if (NdisStatus != NDIS_STATUS_SUCCESS)
    {
        return NdisStatus;
    }

    Request->HandlerContext.DataToFreeOnCompletion = MbbUiccFilePath;

    return MbbUtilQueryAttributeWithParameter(
                Request,
                (PUCHAR) MbbUiccFilePath,
                BufferSize
                );
}

NDIS_STATUS
MbbNdisUiccReadBinary(
    __in PMBB_REQUEST_CONTEXT                                   Request,
    __in_bcount_opt(*InBufferSize) PUCHAR                       InBuffer,
    __in PULONG                                                 InBufferSize,
    __out_bcount_part_opt(*OutBufferSize,*OutBufferSize) PUCHAR OutBuffer,
    __inout PULONG                                              OutBufferSize
    )
{
    return MbbUtilUiccAccessBinary(
            Request,
            InBuffer,
            InBufferSize
            );
}

NDIS_STATUS
MbbNdisUiccReadRecord(
    __in PMBB_REQUEST_CONTEXT                                   Request,
    __in_bcount_opt(*InBufferSize) PUCHAR                       InBuffer,
    __in PULONG                                                 InBufferSize,
    __out_bcount_part_opt(*OutBufferSize,*OutBufferSize) PUCHAR OutBuffer,
    __inout PULONG                                              OutBufferSize
    )
{
    return MbbUtilUiccAccessRecord(
            Request,
            InBuffer,
            InBufferSize
            );
}

NDIS_STATUS
MbbNdisGetPinInfoEx(
    __in PMBB_REQUEST_CONTEXT                                   Request,
    __in_bcount_opt(*InBufferSize) PUCHAR                       InBuffer,
    __in PULONG                                                 InBufferSize,
    __out_bcount_part_opt(*OutBufferSize, *OutBufferSize) PUCHAR OutBuffer,
    __inout PULONG                                              OutBufferSize
)
{
    PNDIS_WWAN_PIN_APP          NdisPinApp = (PNDIS_WWAN_PIN_APP)InBuffer;
    PMBB_PIN_APP                MbbPinApp = NULL;
    NDIS_STATUS                 NdisStatus;
    ULONG                       BufferSize = 0;

    NdisStatus = MbbUtilWwanToMbbPinApp(
        &NdisPinApp->PinApp,
        &MbbPinApp,
        &BufferSize
    );

    if (NdisStatus != NDIS_STATUS_SUCCESS)
    {
        return NdisStatus;
    }

    Request->HandlerContext.DataToFreeOnCompletion = MbbPinApp;

    return MbbUtilQueryAttributeWithParameter(
        Request,
        (PUCHAR)MbbPinApp,
        BufferSize
    );
}

NDIS_STATUS
MbbNdisGetMbimVersion(
    __in PMBB_REQUEST_CONTEXT                                   Request,
    __in_bcount_opt(*InBufferSize) PUCHAR                       InBuffer,
    __in PULONG                                                 InBufferSize,
    __out_bcount_part_opt(*OutBufferSize, *OutBufferSize) PUCHAR OutBuffer,
    __inout PULONG                                              OutBufferSize
)
{
    PMINIPORT_ADAPTER_CONTEXT       Adapter = NULL;

    PNDIS_WWAN_MBIM_VERSION MbbMbimVersion = (PNDIS_WWAN_MBIM_VERSION)OutBuffer;

    //
    // Size is already verified in MbbNdisMiniportOidRequest( )
    //
    __analysis_assume( *OutBufferSize >= sizeof(NDIS_WWAN_MBIM_VERSION) );

    MbbMbimVersion->Header.Type                    = NDIS_OBJECT_TYPE_DEFAULT;
    MbbMbimVersion->Header.Revision                = NDIS_WWAN_MBIM_VERSION_REVISION_1 ;
    MbbMbimVersion->Header.Size                    = SIZEOF_NDIS_WWAN_MBIM_VERSION_1;

    Adapter = ((PMINIPORT_ADAPTER_CONTEXT)MbbReqMgrGetAdapterHandle(Request));

    MbbMbimVersion->MbimVersion.MbimVersion  = Adapter->BusParams.MbimVersion;

    *OutBufferSize = sizeof(NDIS_WWAN_MBIM_VERSION);

    return NDIS_STATUS_SUCCESS;
}

NDIS_STATUS
MbbNdisGetSlotInfoStatus(
    __in PMBB_REQUEST_CONTEXT                                   Request,
    __in_bcount_opt(*InBufferSize) PUCHAR                       InBuffer,
    __in PULONG                                                 InBufferSize,
    __out_bcount_part_opt(*OutBufferSize,*OutBufferSize) PUCHAR OutBuffer,
    __inout PULONG                                              OutBufferSize
)
{
    PNDIS_WWAN_GET_SLOT_INFO NdisGetSlotInfo = (PNDIS_WWAN_GET_SLOT_INFO)InBuffer;
    PMBB_MS_SLOT_INFO_REQ       MbbSlotInfoReq = NULL;

    MbbSlotInfoReq = (PMBB_MS_SLOT_INFO_REQ)(ALLOCATE_NONPAGED_POOL(sizeof(MBB_MS_SLOT_INFO_REQ)));
    if (NULL == MbbSlotInfoReq)
    {
        return NDIS_STATUS_RESOURCES;
    }

    Request->HandlerContext.DataToFreeOnCompletion = MbbSlotInfoReq;

    RtlZeroMemory(MbbSlotInfoReq, sizeof(MBB_MS_SLOT_INFO_REQ));

    MbbSlotInfoReq->SlotIndex = NdisGetSlotInfo->GetSlotInfo.SlotIndex;

    return MbbUtilQueryAttributeWithParameter(
        Request,
        (PUCHAR)MbbSlotInfoReq,
        sizeof(MBB_MS_SLOT_INFO_REQ)
        );
}

NDIS_STATUS
MbbNdisGetAtr(
    __in PMBB_REQUEST_CONTEXT                                   Request,
    __in_bcount_opt(*InBufferSize) PUCHAR                       InBuffer,
    __in PULONG                                                 InBufferSize,
    __out_bcount_part_opt(*OutBufferSize, *OutBufferSize) PUCHAR OutBuffer,
    __inout PULONG                                              OutBufferSize
)
{
    PMINIPORT_ADAPTER_CONTEXT           Adapter = MbbReqMgrGetAdapterHandle(Request);

    MbbAdapterLock(Adapter);
    BOOL IsUiccLowLevelCapable = Adapter->AdapterFlags.IsUiccLowLevelCapable;
    MbbAdapterUnlock(Adapter);

    if (!IsUiccLowLevelCapable)
    {
        TraceInfo(WMBCLASS_OID, "[MbbAdapter] device does not support UICC low level operations (GET_ATR)");
        return NDIS_STATUS_NOT_SUPPORTED;
    }

    return MbbUtilQueryAttributeWithParameter(
        Request,
        NULL,
        0
    );
}

NDIS_STATUS
MbbNdisGetUiccReset(
    _In_ PMBB_REQUEST_CONTEXT                                   Request,
    _In_reads_bytes_opt_(*InBufferSize) PUCHAR                  InBuffer,
    _In_ PULONG                                                 InBufferSize,
    _Out_writes_bytes_to_opt_(*OutBufferSize, *OutBufferSize)   PUCHAR OutBuffer,
    _Inout_ PULONG                                              OutBufferSize
)
{
    PMINIPORT_ADAPTER_CONTEXT           Adapter = MbbReqMgrGetAdapterHandle(Request);

    MbbAdapterLock(Adapter);
    BOOL IsUiccLowLevelCapable = Adapter->AdapterFlags.IsUiccLowLevelCapable;
    MbbAdapterUnlock(Adapter);

    if (!IsUiccLowLevelCapable)
    {
        TraceInfo(WMBCLASS_OID, "[MbbAdapter] device does not support UICC low level operations (GET_UICC_RESET)");
        return NDIS_STATUS_NOT_SUPPORTED;
    }

    return MbbUtilQueryAttributeWithParameter(
        Request,
        NULL,
        0
    );
}

NDIS_STATUS
MbbNdisGetProvisionedContextsV2(
    __in PMBB_REQUEST_CONTEXT                                   Request,
    __in_bcount_opt(*InBufferSize) PUCHAR                       InBuffer,
    __in PULONG                                                 InBufferSize,
    __out_bcount_part_opt(*OutBufferSize,*OutBufferSize) PUCHAR OutBuffer,
    __inout PULONG                                              OutBufferSize
)
{
    PMINIPORT_ADAPTER_CONTEXT           Adapter = MbbReqMgrGetAdapterHandle(Request);

    MbbAdapterLock(Adapter);
    BOOL IsProvisionedContextV2Capable = Adapter->AdapterFlags.IsProvisionedContextV2Capable;
    MbbAdapterUnlock(Adapter);

    if (!IsProvisionedContextV2Capable)
    {
        TraceInfo(WMBCLASS_OID, "[MbbAdapter] device does not support ProvisionedContextsV2");
        return NDIS_STATUS_NOT_SUPPORTED;
    }

    return MbbUtilQueryAttributeWithParameter(
        Request,
        NULL,
        0
    );
}

NDIS_STATUS
MbbNdisGetPcoStatus(
    __in PMBB_REQUEST_CONTEXT                                    Request,
    __in_bcount_opt(*InBufferSize) PUCHAR                        InBuffer,
    __in PULONG                                                  InBufferSize,
    __out_bcount_part_opt(*OutBufferSize, *OutBufferSize) PUCHAR OutBuffer,
    __inout PULONG                                               OutBufferSize
)
{
    UNREFERENCED_PARAMETER(InBuffer);
    UNREFERENCED_PARAMETER(InBufferSize);

    PMINIPORT_ADAPTER_CONTEXT  Adapter        = MbbReqMgrGetAdapterHandle(Request);
    PMBB_PORT                  Port           = NULL;
    NDIS_STATUS                Status         = NDIS_STATUS_SUCCESS;
    MBB_MS_PCO_VALUE           MbbPcoValue    = { 0 };

    MbbAdapterLock(Adapter);
    BOOL IsPcoCapable = Adapter->AdapterFlags.IsPcoCapable;
    MbbAdapterUnlock(Adapter);

    if (!IsPcoCapable)
    {
        TraceInfo(WMBCLASS_OID, "[MbbAdapter] device does not support PCO");
        return NDIS_STATUS_NOT_SUPPORTED;
    }

    Port = MbbWwanTranslatePortNumberToPort(
        Adapter,
        Request->OidContext.OidRequest->PortNumber);

    if (Port != NULL)
    {
        ULONG MaxActivatedContexts = 0;
        ULONG SessionId = MBB_INVALID_SESSION_ID;

        MbbAdapterPortsLock(Adapter);
        MaxActivatedContexts = Adapter->MaxActivatedContexts;
        MbbAdapterPortsUnlock(Adapter);

        SessionId = MbbWwanGetPortSessionId(Port);

        if (SessionId < MaxActivatedContexts)
        {
            MbbPcoValue.SessionId = SessionId;

            Status = MbbUtilQueryAttributeWithParameter(
                Request,
                (PUCHAR)&MbbPcoValue,
                sizeof(MBB_MS_PCO_VALUE)
            );
        }
        else
        {
            // Fail this OID request because we cannot query the device with an invalid session ID.

            Status = NDIS_STATUS_FAILURE;

            ASSERT(IS_ALLOCATED_PORT_NUMBER(Request->OidContext.OidRequest->PortNumber));

            TraceError(WMBCLASS_OID, "[MbbNdis][ReqId=0x%04x] MbbNdisGetPcoStatus failed for a Port: [%lu]. Requested sessionId: [%lu] Status=%!status!",
                Request->RequestId,
                Request->OidContext.OidRequest->PortNumber,
                SessionId,
                Status
            );
        }

        // Remove the reference added during find
        Dereference(Port);
    }
    else
    {
        Status = NDIS_STATUS_INVALID_PORT;
    }

    return Status;
}

NDIS_STATUS
MbbNdisGetSysCapsInfo(
    __in PMBB_REQUEST_CONTEXT                                   Request,
    __in_bcount_opt(*InBufferSize) PUCHAR                       InBuffer,
    __in PULONG                                                 InBufferSize,
    __out_bcount_part_opt(*OutBufferSize, *OutBufferSize) PUCHAR OutBuffer,
    __inout PULONG                                              OutBufferSize
)
{
    PMINIPORT_ADAPTER_CONTEXT           Adapter = MbbReqMgrGetAdapterHandle(Request);

    MbbAdapterLock(Adapter);
    BOOL IsMultiSIMCapable = Adapter->AdapterFlags.IsMultiSIMCapable;
    MbbAdapterUnlock(Adapter);

    if (!IsMultiSIMCapable)
    {
        TraceInfo(WMBCLASS_OID, "[MbbAdapter] device does not support system caps");
        return NDIS_STATUS_NOT_SUPPORTED;
    }

    return MbbUtilQueryAttributeWithParameter(
        Request,
        NULL,
        0
    );
}

NDIS_STATUS
MbbNdisGetDeviceCapsEx(
    __in PMBB_REQUEST_CONTEXT                                   Request,
    __in_bcount_opt(*InBufferSize) PUCHAR                       InBuffer,
    __in PULONG                                                 InBufferSize,
    __out_bcount_part_opt(*OutBufferSize, *OutBufferSize) PUCHAR OutBuffer,
    __inout PULONG                                              OutBufferSize
)
{
    PMINIPORT_ADAPTER_CONTEXT           Adapter = MbbReqMgrGetAdapterHandle(Request);

    MbbAdapterLock(Adapter);
    BOOL IsDeviceCapsV2Capable = Adapter->AdapterFlags.IsDeviceCapsV2Capable;
    MbbAdapterUnlock(Adapter);

    if (!IsDeviceCapsV2Capable)
    {
        TraceInfo(WMBCLASS_OID, "[MbbAdapter] device does not support system caps, so assume it doesn't support device caps extention either");
        return NDIS_STATUS_NOT_SUPPORTED;
    }

    return MbbUtilQueryAttributeWithParameter(
        Request,
        NULL,
        0
    );
}

NDIS_STATUS
MbbNdisGetBaseStationsInfo(
    __in PMBB_REQUEST_CONTEXT                                   Request,
    __in_bcount_opt(*InBufferSize) PUCHAR                       InBuffer,
    __in PULONG                                                 InBufferSize,
    __out_bcount_part_opt(*OutBufferSize, *OutBufferSize) PUCHAR OutBuffer,
    __inout PULONG                                              OutBufferSize
)
{
    PMINIPORT_ADAPTER_CONTEXT  Adapter = MbbReqMgrGetAdapterHandle(Request);

    MbbAdapterLock(Adapter);
    BOOL IsBaseStationsInfoCapable = Adapter->AdapterFlags.IsBaseStationsInfoCapable;
    MbbAdapterUnlock(Adapter);

    if (!IsBaseStationsInfoCapable)
    {
        TraceInfo(WMBCLASS_OID, "[MbbAdapter] device does not support base station information");
        return NDIS_STATUS_NOT_SUPPORTED;
    }

    PNDIS_WWAN_BASE_STATIONS_INFO_REQ NdisGetBaseStationsInfoRequest = (PNDIS_WWAN_BASE_STATIONS_INFO_REQ)InBuffer;
    PWWAN_BASE_STATIONS_INFO_REQ WwanBaseStationsInfoRequest = &(NdisGetBaseStationsInfoRequest->BaseStationsInfoRequest);
    PMBB_MS_BASE_STATIONS_INFO_REQ MbbBaseStationsInfoReq = NULL;

    MbbBaseStationsInfoReq = (PMBB_MS_BASE_STATIONS_INFO_REQ)(ALLOCATE_NONPAGED_POOL(sizeof(MBB_MS_BASE_STATIONS_INFO_REQ)));

    Request->HandlerContext.DataToFreeOnCompletion = MbbBaseStationsInfoReq;

    RtlZeroMemory(MbbBaseStationsInfoReq, sizeof(MBB_MS_BASE_STATIONS_INFO_REQ));

    MbbBaseStationsInfoReq->MaxGSMCount = WwanBaseStationsInfoRequest->MaxGSMCount;
    MbbBaseStationsInfoReq->MaxUMTSCount = WwanBaseStationsInfoRequest->MaxUMTSCount;
    MbbBaseStationsInfoReq->MaxTDSCDMACount = WwanBaseStationsInfoRequest->MaxTDSCDMACount;
    MbbBaseStationsInfoReq->MaxLTECount = WwanBaseStationsInfoRequest->MaxLTECount;
    MbbBaseStationsInfoReq->MaxCDMACount = WwanBaseStationsInfoRequest->MaxCDMACount;

    return MbbUtilQueryAttributeWithParameter(
        Request,
        (PUCHAR)MbbBaseStationsInfoReq,
        sizeof(MBB_MS_BASE_STATIONS_INFO_REQ)
    );
}

//
// Set Handlers
//


NDIS_STATUS
MbbNdisSetCurrentLookahead(
    __in PMBB_REQUEST_CONTEXT               Request,
    __in_bcount_opt(*InBufferSize) PUCHAR   InBuffer,
    __in PULONG                             InBufferSize
    )
{
    //
    // Size is already verified in MbbNdisMiniportOidRequest( )
    //
    __analysis_assume( *InBufferSize >= sizeof(ULONG) );
    //
    // Todo: Record this value.
    //
    *InBufferSize = sizeof(ULONG);
    return NDIS_STATUS_SUCCESS;
}

NDIS_STATUS
MbbNdisSetInterruptModeration(
    __in PMBB_REQUEST_CONTEXT               Request,
    __in_bcount_opt(*InBufferSize) PUCHAR   InBuffer,
    __in PULONG                             InBufferSize
    )
{
    //
    // Size is already verified in MbbNdisMiniportOidRequest( )
    //
    __analysis_assume( *InBufferSize >= sizeof(NDIS_INTERRUPT_MODERATION_PARAMETERS) );

    *InBufferSize = sizeof(NDIS_INTERRUPT_MODERATION_PARAMETERS);
    return NDIS_STATUS_SUCCESS;
}

NDIS_STATUS
MbbNdisSetRadioState(
    __in PMBB_REQUEST_CONTEXT               Request,
    __in_bcount_opt(*InBufferSize) PUCHAR   InBuffer,
    __in PULONG                             InBufferSize
    )
{
    MBB_RADIO_STATE*            MbbRadioState;
    PNDIS_WWAN_SET_RADIO_STATE  NdisRadioState = (PNDIS_WWAN_SET_RADIO_STATE)InBuffer;

    //
    // Size is already verified in MbbNdisMiniportOidRequest( )
    //
    __analysis_assume( *InBufferSize >= sizeof(NDIS_WWAN_SET_RADIO_STATE) );
    *InBufferSize = sizeof(NDIS_WWAN_SET_RADIO_STATE);

    if( (MbbRadioState = (MBB_RADIO_STATE*) ALLOCATE_NONPAGED_POOL( sizeof(MBB_RADIO_STATE) )) == NULL )
    {
        return NDIS_STATUS_RESOURCES;
    }
    Request->HandlerContext.DataToFreeOnCompletion = MbbRadioState;

    // Save the setting so that when the request completes we know what
    // action was performed
    Request->HandlerContext.Parameters.RadioState.SetAction = NdisRadioState->RadioAction;

    MbbUtilWwanToMbbRadioState(
        &NdisRadioState->RadioAction,
        MbbRadioState
        );

    return MbbUtilSetAttributeWithParameter(
                Request,
                (PCHAR) MbbRadioState,
                sizeof(MBB_RADIO_STATE)
                );
}

NDIS_STATUS
MbbNdisSetPinInfo(
    __in PMBB_REQUEST_CONTEXT               Request,
    __in_bcount_opt(*InBufferSize) PUCHAR   InBuffer,
    __in PULONG                             InBufferSize
    )
{
    PMBB_PIN_ACTION             MbbPinAction=NULL;
    PNDIS_WWAN_SET_PIN          NdisSetPin = (PNDIS_WWAN_SET_PIN)InBuffer;
    NTSTATUS                    Status;
    ULONG                       BufferSize=0;

    //
    // Size is already verified in MbbNdisMiniportOidRequest( )
    //
    __analysis_assume( *InBufferSize >= sizeof(NDIS_WWAN_SET_PIN) );
    *InBufferSize = sizeof(NDIS_WWAN_SET_PIN);



    Status=MbbUtilWwanToMbbPinAction(
        &NdisSetPin->PinAction,
        &MbbPinAction,
        &BufferSize
        );

    if (!NT_SUCCESS(Status))
    {
        return Status;
    }

    Request->HandlerContext.DataToFreeOnCompletion = MbbPinAction;

    return MbbUtilSetAttributeWithParameter(
                Request,
                (PCHAR) MbbPinAction,
                BufferSize
                );
}

NDIS_STATUS
MbbNdisSetPinInfoEx2(
    __in PMBB_REQUEST_CONTEXT               Request,
    __in_bcount_opt(*InBufferSize) PUCHAR   InBuffer,
    __in PULONG                             InBufferSize
    )
{
    PMBB_PIN_ACTION_EX2             MbbPinAction=NULL;
    PNDIS_WWAN_SET_PIN_EX2          NdisSetPin = (PNDIS_WWAN_SET_PIN_EX2)InBuffer;
    NTSTATUS                        Status;
    ULONG                           BufferSize=0;

    //
    // Size is already verified in MbbNdisMiniportOidRequest( )
    //
    __analysis_assume( *InBufferSize >= sizeof(NDIS_WWAN_SET_PIN_EX2) );
    *InBufferSize = sizeof(NDIS_WWAN_SET_PIN_EX2);



    Status=MbbUtilWwanToMbbPinActionEx2(
        &NdisSetPin->PinAction,
        &MbbPinAction,
        &BufferSize
        );

    if (!NT_SUCCESS(Status))
    {
        return Status;
    }

    Request->HandlerContext.DataToFreeOnCompletion = MbbPinAction;

    return MbbUtilSetAttributeWithParameter(
                Request,
                (PCHAR) MbbPinAction,
                BufferSize
                );
}


NDIS_STATUS
MbbNdisSetPreferredProviders(
    __in PMBB_REQUEST_CONTEXT               Request,
    __in_bcount_opt(*InBufferSize) PUCHAR   InBuffer,
    __in PULONG                             InBufferSize
    )
{
    ULONG                               ElementIndex;
    ULONG                               ElementCount;
    ULONG                               ValidDataSize;
    ULONGLONG                           MbbPreferredProvidersSize;
    ULONGLONG                           NdisSetPreferredProvidersSize;
    PWWAN_PROVIDER2                     WwanProvider;
    NDIS_STATUS                         NdisStatus;
    PMBB_PROVIDER_LIST                  MbbPreferredProviders = NULL;
    MBB_CELLULAR_CLASS                  MbbCellularClass;
    PNDIS_WWAN_SET_PREFERRED_PROVIDERS  NdisSetPreferredProviders = (PNDIS_WWAN_SET_PREFERRED_PROVIDERS)InBuffer;
    PMINIPORT_ADAPTER_CONTEXT           Adapter = MbbReqMgrGetAdapterHandle( Request );
    BOOLEAN                             IsMultiCarrierCapable = MbbAdapterIsMultiCarrierCapable( Adapter );

    //
    // Size is already verified in MbbNdisMiniportOidRequest( )
    //
    __analysis_assume( *InBufferSize >= sizeof(NDIS_WWAN_SET_PREFERRED_PROVIDERS) );
    do
    {
        //
        // Verify input buffer
        //
        if( NdisSetPreferredProviders->PreferredListHeader.ElementType != WwanStructProvider2 )
        {
            TraceError( WMBCLASS_OID, "[MbbNdis][ReqID=0x%04x] INVALID WWAN_LIST Expecting=%d Received=%d",
                        Request->RequestId,
                        WwanStructProvider2,
                        NdisSetPreferredProviders->PreferredListHeader.ElementType
                        );
            NdisStatus = NDIS_STATUS_INVALID_DATA;
            break;
        }

        ElementCount = NdisSetPreferredProviders->PreferredListHeader.ElementCount;

        NdisSetPreferredProvidersSize = sizeof(NDIS_WWAN_SET_PREFERRED_PROVIDERS) ;

        NdisSetPreferredProvidersSize += ((ULONGLONG)ElementCount) * sizeof(WWAN_PROVIDER2);

        if( *InBufferSize < NdisSetPreferredProvidersSize )
        {
            TraceError( WMBCLASS_OID, "[MbbNdis][ReqID=0x%04x] INSUFFICIENT buffer size for NDIS_WWAN_SET_PREFERRED_PROVIDERS, Expecting=%d Received=%d, ProviderCount=%d",
                        Request->RequestId,
                        (ULONG)NdisSetPreferredProvidersSize,
                       *InBufferSize,
                        ElementCount
                        );
            NdisStatus = NDIS_STATUS_INVALID_LENGTH;
            break;
        }
        //
        // Query for the required size
        //
        WwanProvider = (PWWAN_PROVIDER2)(((PCHAR)NdisSetPreferredProviders) + RTL_SIZEOF_THROUGH_FIELD( NDIS_WWAN_SET_PREFERRED_PROVIDERS, PreferredListHeader ));

        MbbPreferredProvidersSize  = FIELD_OFFSET(MBB_PROVIDER_LIST, Providers);
        MbbPreferredProvidersSize += (ULONGLONG)ElementCount * sizeof(MBB_ARRAY_ELEMENT);

        for( ElementIndex = 0;
             ElementIndex < ElementCount;
             ElementIndex ++ )
        {
            SIZE_T StringCbLength;

            MbbPreferredProvidersSize  = ROUND_UP_COUNT( MbbPreferredProvidersSize, ALIGN_DWORD );
            MbbPreferredProvidersSize += sizeof(MBB_PROVIDER);

            if( (NdisStatus = RtlStringCbLengthW(
                                WwanProvider->Provider.ProviderId,
                                sizeof(WwanProvider->Provider.ProviderId),
                               &StringCbLength
                                )) != STATUS_SUCCESS )
            {
                StringCbLength = sizeof(WwanProvider->Provider.ProviderId);
            }
            MbbPreferredProvidersSize += ROUND_UP_COUNT( (ULONG)StringCbLength, ALIGN_DWORD );

            if( (NdisStatus = RtlStringCbLengthW(
                                WwanProvider->Provider.ProviderName,
                                sizeof(WwanProvider->Provider.ProviderName),
                               &StringCbLength
                                )) != STATUS_SUCCESS )
            {
                StringCbLength = sizeof(WwanProvider->Provider.ProviderName);
            }
            MbbPreferredProvidersSize += ROUND_UP_COUNT( (ULONG)StringCbLength, ALIGN_DWORD );
            WwanProvider++;
        }
        //
        // Allocate & setup Mbb buffer
        //
        if( MbbPreferredProvidersSize > ULONG_MAX ||
           (MbbPreferredProviders = ALLOCATE_NONPAGED_POOL( (ULONG)MbbPreferredProvidersSize )) == NULL )
        {
            TraceError( WMBCLASS_OID, "[MbbNdis][ReqID=0x%04x] FAILED to allocate MBB_PROVIDER_LIST of Size=%d for ProviderCount=%d",
                        Request->RequestId,
                        (ULONG)MbbPreferredProvidersSize,
                        ElementCount
                        );
            NdisStatus = NDIS_STATUS_RESOURCES;
            break;
        }
        Request->HandlerContext.DataToFreeOnCompletion = MbbPreferredProviders;
        //
        // Convert from WWAN to MBB
        //
        if( IsMultiCarrierCapable == TRUE )
            MbbCellularClass = MbbCellularClassInvalid;
        else
            MbbCellularClass = MbbAdapterGetSupportedCellularClass( Adapter );

        ValidDataSize = (ULONG)MbbPreferredProvidersSize;

        if( (NdisStatus = MbbUtilWwanToMbbProviderList(
                           &NdisSetPreferredProviders->PreferredListHeader,
                            IsMultiCarrierCapable,
                            MbbCellularClass,
                            MbbPreferredProviders,
                           &ValidDataSize
                            )) != NDIS_STATUS_SUCCESS )
        {
            TraceError( WMBCLASS_OID, "[MbbNdis][ReqID=0x%04x] FAILED to convert WWAN_PROVIDER_LIST to MBB_PROVIDER_LIST ", Request->RequestId );
            break;
        }
        //
        // Send the data
        //
        NdisStatus = MbbUtilSetAttributeWithParameter(
                        Request,
                        (PCHAR)MbbPreferredProviders,
                        ValidDataSize
                        );
        if( NdisStatus == NDIS_STATUS_SUCCESS ||
            NdisStatus == NDIS_STATUS_PENDING )
        {
            *InBufferSize = (ULONG)NdisSetPreferredProvidersSize;
        }
    }
    while( FALSE );

    return NdisStatus;
}


NDIS_STATUS
MbbNdisSetRegisterState(
    __in PMBB_REQUEST_CONTEXT               Request,
    __in_bcount_opt(*InBufferSize) PUCHAR   InBuffer,
    __in PULONG                             InBufferSize
    )
{
    PMBB_SET_REGISTER_STATE         MbbSetRegisterState;
    PNDIS_WWAN_SET_REGISTER_STATE   NdisSetRegisterState = (PNDIS_WWAN_SET_REGISTER_STATE)InBuffer;
    ULONG                           BufferSize=0;
    NDIS_STATUS                     Status;

    //
    // Size is already verified in MbbNdisMiniportOidRequest( )
    //
    __analysis_assume( *InBufferSize >= sizeof(NDIS_WWAN_SET_REGISTER_STATE) );
    *InBufferSize = sizeof(NDIS_WWAN_SET_REGISTER_STATE);


    Status=MbbUtilWwanToMbbSetRegisterState(
        &NdisSetRegisterState->SetRegisterState,
        &MbbSetRegisterState,
        &BufferSize
        );

    if (!NT_SUCCESS(Status))
    {

        return Status;
    }

    Request->HandlerContext.DataToFreeOnCompletion = MbbSetRegisterState;

    return MbbUtilSetAttributeWithParameter(
                Request,
                (PCHAR) MbbSetRegisterState,
                BufferSize
                );
}

NDIS_STATUS
MbbNdisSetPacketService(
    __in PMBB_REQUEST_CONTEXT               Request,
    __in_bcount_opt(*InBufferSize) PUCHAR   InBuffer,
    __in PULONG                             InBufferSize
    )
{
    NDIS_STATUS                     NdisStatus;
    PMBB_SET_PACKET_SERVICE         MbbSetPacketService;
    PNDIS_WWAN_SET_PACKET_SERVICE   NdisSetPacketService = (PNDIS_WWAN_SET_PACKET_SERVICE)InBuffer;

    //
    // Size is already verified in MbbNdisMiniportOidRequest( )
    //
    __analysis_assume( *InBufferSize >= sizeof(NDIS_WWAN_SET_PACKET_SERVICE) );
    *InBufferSize = sizeof(NDIS_WWAN_SET_PACKET_SERVICE);

    if( (MbbSetPacketService = (PMBB_SET_PACKET_SERVICE) ALLOCATE_NONPAGED_POOL( sizeof(MBB_SET_PACKET_SERVICE) )) == NULL )
    {
        TraceError( WMBCLASS_OID, "[MbbNdis][ReqID=0x%04x] FAILED to allocate MBB_SET_PACKET_SERVICE", Request->RequestId );
        return NDIS_STATUS_RESOURCES;
    }
    Request->HandlerContext.DataToFreeOnCompletion = MbbSetPacketService;

    NdisStatus = MbbUtilWwanToMbbSetPacketService(
                    NdisSetPacketService->PacketServiceAction,
                    MbbSetPacketService
                    );
    if( NT_ERROR( NdisStatus ) )
    {
        TraceError( WMBCLASS_OID, "[MbbNdis][ReqID=0x%04x] FAILED to convert WWAN_PACKET_SERVICE_ACTION, NdisStatus=%!STATUS!", Request->RequestId, NdisStatus );
        return NdisStatus;
    }

    return MbbUtilSetAttributeWithParameter(
                Request,
                (PCHAR) MbbSetPacketService,
                sizeof(MBB_SET_PACKET_SERVICE)
                );
}

NDIS_STATUS
MbbNdisSetSignalState(
    __in PMBB_REQUEST_CONTEXT               Request,
    __in_bcount_opt(*InBufferSize) PUCHAR   InBuffer,
    __in PULONG                             InBufferSize
    )
{
    PMBB_SET_SIGNAL_INDICATION          MbbSetSignalStateIndication;
    PNDIS_WWAN_SET_SIGNAL_INDICATION    NdisSetSignalStateIndication = (PNDIS_WWAN_SET_SIGNAL_INDICATION)InBuffer;
    PMINIPORT_ADAPTER_CONTEXT           Adapter;

    //
    // Size is already verified in MbbNdisMiniportOidRequest( )
    //
    __analysis_assume( *InBufferSize >= sizeof(NDIS_WWAN_SET_SIGNAL_INDICATION) );
    *InBufferSize = sizeof(NDIS_WWAN_SET_SIGNAL_INDICATION);

    if( (MbbSetSignalStateIndication = (PMBB_SET_SIGNAL_INDICATION) ALLOCATE_NONPAGED_POOL( sizeof(MBB_SET_SIGNAL_INDICATION) )) == NULL )
    {
        return NDIS_STATUS_RESOURCES;
    }
    Request->HandlerContext.DataToFreeOnCompletion = MbbSetSignalStateIndication;

    //
    // Adjust signal state configuration if needed
    //
    Adapter = MbbReqMgrGetAdapterHandle( Request );

    MbbUtilAdjustSignalStateConfigurationForDevice(
        Adapter,
        &NdisSetSignalStateIndication->SignalIndication
        );

    //
    // Map to MBB values
    //
    MbbUtilWwanToMbbSetSignalStateIndication(
        &NdisSetSignalStateIndication->SignalIndication,
        MbbSetSignalStateIndication
        );

    return MbbUtilSetAttributeWithParameter(
                Request,
                (PCHAR) MbbSetSignalStateIndication,
                sizeof(MBB_SET_SIGNAL_INDICATION)
                );
}

NDIS_STATUS
MbbNdisSetConnectState(
    __in PMBB_REQUEST_CONTEXT               Request,
    __in_bcount_opt(*InBufferSize) PUCHAR   InBuffer,
    __in PULONG                             InBufferSize
    )
{
    NDIS_STATUS                         NdisStatus;
    PNDIS_WWAN_SET_CONTEXT_STATE        NdisSetContextState = (PNDIS_WWAN_SET_CONTEXT_STATE)InBuffer;
    PNDIS_OID_REQUEST                   OidRequest = Request->OidContext.OidRequest;
    PMINIPORT_ADAPTER_CONTEXT           Adapter = (PMINIPORT_ADAPTER_CONTEXT) MbbReqMgrGetAdapterHandle(Request);
    NDIS_PORT_NUMBER                    PortNumber =  OidRequest->PortNumber;
    PMBB_PORT                           Port = NULL;
    ULONG                               SessionId;
    BOOLEAN                             PortSessionIdSet = FALSE;

     // Find the port corresponding to the port number
    Port = MbbWwanTranslatePortNumberToPort(Adapter, PortNumber);

    if(!Port)
    {
        return NDIS_STATUS_INVALID_PORT;
    }

    do
    {
        //
        // Size is already verified in MbbNdisMiniportOidRequest( )
        //
        __analysis_assume( *InBufferSize >= sizeof(NDIS_WWAN_SET_CONTEXT_STATE) );
        *InBufferSize = sizeof(NDIS_WWAN_SET_CONTEXT_STATE);
        //
        // Cache values for later use
        //
        Adapter = MbbReqMgrGetAdapterHandle( Request );

        if( NdisSetContextState->SetContextState.ActivationCommand == WwanActivationCommandActivate )
        {
            // Set a session ID
            NdisStatus = MbbWwanSetPortSessionId(Port);

            if(NdisStatus != NDIS_STATUS_SUCCESS)
            {
                PortSessionIdSet = FALSE;
                TraceError( WMBCLASS_OID, "[MbbNdis][ReqID=0x%04x] FAILED to obtain a session id for the connection", Request->RequestId);
                break;
            }
            else
            {
                PortSessionIdSet = TRUE;
            }

            Request->HandlerContext.Parameters.Connect.Activate = TRUE;
            //
            // Henceforth any context_state indication \ reponse coming from the device
            // will be reported using this ConnectionId. The ConnectionId is valid till
            // the time the context is not de-activated or the activation fails.
            //
            MbbWwanSetPortConnectionId(
                Port,
                NdisSetContextState->SetContextState.ConnectionId
                );

        }
        else
        if( NdisSetContextState->SetContextState.ActivationCommand == WwanActivationCommandDeactivate )
        {
            ULONG ConnectionId = MbbWwanGetPortConnectionId(Port);

            //
            // If the ConnectionId isnt activated then fail the request.
            //
            if( ConnectionId != NdisSetContextState->SetContextState.ConnectionId )
            {
                TraceError( WMBCLASS_OID, "[MbbNdis][ReqID=0x%04x] FAILED to de-activate invalid ConnectionId[Received=%d, Expected=%d]",
                            Request->RequestId,
                            NdisSetContextState->SetContextState.ConnectionId,
                            ConnectionId
                            );
                NdisStatus = WWAN_STATUS_CONTEXT_NOT_ACTIVATED;
                break;
            }

            // always assume that the session id is set for a valid deactivate context request
            PortSessionIdSet = TRUE;
        }
        else
        {
            TraceError( WMBCLASS_OID, "[MbbNdis][ReqID=0x%04x] bad Command = %d",
                    Request->RequestId,
                    NdisSetContextState->SetContextState.ActivationCommand
                    );

            NdisStatus = NDIS_STATUS_INVALID_DATA;
            break;
        }

        SessionId = MbbWwanGetPortSessionId(Port);

        TraceInfo( WMBCLASS_OID, "[MbbNdis][ReqID=0x%04x] Processing set context request = %d for port number = %d with session id = %d",
                                  Request->RequestId,
                                  NdisSetContextState->SetContextState.ActivationCommand,
                                  PortNumber,
                                  SessionId
                                  );

        Request->HandlerContext.Parameters.Connect.SessionId = SessionId;

        NdisStatus = MbbUtilProcessSetConnectState(Request, &(NdisSetContextState->SetContextState), SessionId);

        if(NdisStatus != NDIS_STATUS_SUCCESS
            && NdisStatus != NDIS_STATUS_PENDING)
        {
             TraceError( WMBCLASS_OID, "[MbbNdis][ReqID=0x%04x] MbbProcessSetConnectState failed with status=%!STATUS!",Request->RequestId, NdisStatus);
             break;
        }
    }
    while( FALSE );
    //
    // If the ACTIVATION request was not successful then wipe out the connectionid.
    //
    if( NdisStatus != NDIS_STATUS_SUCCESS &&
        NdisStatus != NDIS_STATUS_PENDING &&
        NdisSetContextState->SetContextState.ActivationCommand == WwanActivationCommandActivate )
    {
        MbbWwanSetPortConnectionId(
            Port,
            0
            );

        // Return the session id to the pool only when it was actually set for the port
        MbbWwanResetPortSessionId(
            Port,
            PortSessionIdSet
            );
    }

    //Remove the reference added during find
    Dereference(Port);

    return NdisStatus;
}


NDIS_STATUS
MbbNdisGetStatistics(
    __in PMBB_REQUEST_CONTEXT                                   Request,
    __in_bcount_opt(*InBufferSize) PUCHAR                       InBuffer,
    __in PULONG                                                 InBufferSize,
    __out_bcount_part_opt(*OutBufferSize,*OutBufferSize) PUCHAR OutBuffer,
    __inout PULONG                                              OutBufferSize
    )
{
    PMINIPORT_ADAPTER_CONTEXT Adapter = (PMINIPORT_ADAPTER_CONTEXT)(Request->RequestManager->AdapterContext);

    PNDIS_STATISTICS_INFO       Stats=(PNDIS_STATISTICS_INFO)OutBuffer;

    __analysis_assume( *OutBufferSize >= sizeof(NDIS_STATISTICS_INFO) );

    *OutBufferSize = sizeof(*Stats);

    RtlCopyMemory(Stats, &Adapter->Stats, sizeof(*Stats));

    Stats->Header.Type=NDIS_OBJECT_TYPE_DEFAULT;
    Stats->Header.Revision=NDIS_STATISTICS_INFO_REVISION_1;
    Stats->Header.Size=NDIS_SIZEOF_STATISTICS_INFO_REVISION_1;

    Stats->SupportedStatistics = NDIS_STATISTICS_FLAGS_VALID_BYTES_RCV |
                                 NDIS_STATISTICS_FLAGS_VALID_BYTES_XMIT |
                                 NDIS_STATISTICS_FLAGS_VALID_DIRECTED_FRAMES_RCV;


    return STATUS_SUCCESS;
}

NDIS_STATUS
MbbNdisCreateMac(
        __in PMBB_REQUEST_CONTEXT                                   Request,
        __in_bcount_opt(*InBufferSize) PUCHAR                       InBuffer,
        __in PULONG                                                 InBufferSize,
        __out_bcount_part_opt(*OutBufferSize,*OutBufferSize) PUCHAR OutBuffer,
        __inout PULONG                                              OutBufferSize
        )
{
    NDIS_STATUS         ndisStatus = NDIS_STATUS_SUCCESS;
    PNDIS_OID_REQUEST   oidRequest = Request->OidContext.OidRequest;
    PMINIPORT_ADAPTER_CONTEXT Adapter = (PMINIPORT_ADAPTER_CONTEXT) MbbReqMgrGetAdapterHandle(Request);

    UNREFERENCED_PARAMETER(InBuffer);
    UNREFERENCED_PARAMETER(InBufferSize);

    OutBuffer = oidRequest->DATA.METHOD_INFORMATION.InformationBuffer;

    do
    {
        oidRequest->DATA.METHOD_INFORMATION.BytesWritten = 0;
        oidRequest->DATA.METHOD_INFORMATION.BytesRead = 0;
        oidRequest->DATA.METHOD_INFORMATION.BytesNeeded = 0;

        if (oidRequest->RequestType != NdisRequestMethod)
        {
            TraceError( WMBCLASS_OID, "[MbbNdis][ReqID=0x%04x] Invalid request type %d for OID_WWAN_CREATE_MAC\n",
                        Request->RequestId,
                        oidRequest->RequestType
                        );
            ndisStatus = NDIS_STATUS_NOT_SUPPORTED;
            break;
        }

        ASSERT(oidRequest->DATA.METHOD_INFORMATION.OutputBufferLength >= sizeof(NDIS_WWAN_MAC_INFO));

        //
        // Since OID calls are serialized, we do not expect the NumberOfPorts to change
        // while we are checking the following until this OID is completed. So we do not need
        // to protect the NumberOfPorts in any way
        //
        if (Adapter->NumberOfPorts >= Adapter->MaxActivatedContexts)
        {
            TraceError( WMBCLASS_OID, "[MbbNdis][ReqID=0x%04x] Number of existing ports exceed max supported. Failing new port creation\n",
                        Request->RequestId);
            ndisStatus = NDIS_STATUS_NOT_SUPPORTED;
            break;
        }

        ndisStatus = MbbUtilWwanCreateMac(Request);

        if (ndisStatus != NDIS_STATUS_SUCCESS)
        {
             TraceError( WMBCLASS_OID, "[MbbNdis][ReqID=0x%04x] MbbUtilWwanCreateMac failed. Status = 0x%08x\n",
                        Request->RequestId,ndisStatus);
             break;
        }
    } while (FALSE);


    *OutBufferSize = oidRequest->DATA.METHOD_INFORMATION.BytesWritten;

    return ndisStatus;
}



NDIS_STATUS
MbbNdisSetPower(
    __in PMBB_REQUEST_CONTEXT               Request,
    __in_bcount_opt(*InBufferSize) PUCHAR   InBuffer,
    __in PULONG                             InBufferSize
    )

{
    PMINIPORT_ADAPTER_CONTEXT Adapter = (PMINIPORT_ADAPTER_CONTEXT)(Request->RequestManager->AdapterContext);
    NDIS_STATUS                 Status;
    PSTATE_CHANGE_EVENT         StateChange=NULL;

    PULONG      PowerState=(PULONG)InBuffer;

    TraceInfo( WMBCLASS_POWER, "SetPower D%d", *PowerState - NetDeviceStateD0);

    *InBufferSize=sizeof(*PowerState);


    StateChange=AllocateStateChangeEvent(&Adapter->AdapterState);

    if (StateChange != NULL)
    {
        StateChange->EventType=STATE_CHANGE_TYPE_POWER;
        StateChange->Context1=Adapter;
        StateChange->Power.Request=Request;
        StateChange->Power.NewPower=*PowerState;

        QueueStateChangeEvent(&Adapter->AdapterState,StateChange);

        return STATUS_PENDING;

    }

    return STATUS_INSUFFICIENT_RESOURCES;

}

NDIS_STATUS
MbbNdisSetHomeProvider(
    __in PMBB_REQUEST_CONTEXT               Request,
    __in_bcount_opt(*InBufferSize) PUCHAR   InBuffer,
    __in PULONG                             InBufferSize
    )
{
    ULONG                               ValidDataSize;
    ULONGLONG                           MbbHomeProviderSize;
    ULONG                               NdisSetHomeProvidersSize;
    NDIS_STATUS                         NdisStatus;
    PMBB_PROVIDER                       MbbHomeProvider = NULL;
    WWAN_PROVIDER2                      HomeProvider;
    PMINIPORT_ADAPTER_CONTEXT           Adapter = MbbReqMgrGetAdapterHandle( Request );
    BOOLEAN                             IsMultiCarrierCapable = MbbAdapterIsMultiCarrierCapable( Adapter );
    PNDIS_WWAN_SET_HOME_PROVIDER        NdisSetHomeProvider = (PNDIS_WWAN_SET_HOME_PROVIDER)InBuffer;
    SIZE_T                              StringCbLength= 0;
    //
    // Size is already verified in MbbNdisMiniportOidRequest( )
    //
    __analysis_assume( *InBufferSize >= sizeof(NDIS_WWAN_SET_HOME_PROVIDER) );
    do
    {
        //
        // Verify input buffer
        //
        if( !IsMultiCarrierCapable)
        {
            TraceError( WMBCLASS_OID, "[MbbNdis][ReqID=0x%04x] SetHome unsupported for non-Multicarrier",
                        Request->RequestId
                        );
            NdisStatus = NDIS_STATUS_NOT_SUPPORTED;
            break;
        }

        NdisSetHomeProvidersSize = sizeof(NDIS_WWAN_SET_HOME_PROVIDER);

        if( *InBufferSize < NdisSetHomeProvidersSize )
        {
            TraceError( WMBCLASS_OID,
                        "[MbbNdis][ReqID=0x%04x] INSUFFICIENT buffer size for NDIS_WWAN_SET_HOME_PROVIDER, Expecting=%d Received=%d",
                        Request->RequestId,
                        (ULONG)NdisSetHomeProvidersSize,
                       *InBufferSize
                        );
            NdisStatus = NDIS_STATUS_INVALID_LENGTH;
            break;
        }
        //
        // Query for the required size
        //
        HomeProvider = NdisSetHomeProvider->HomeProvider;

        MbbHomeProviderSize  = sizeof(MBB_PROVIDER);

        if( (NdisStatus = RtlStringCbLengthW(
                            HomeProvider.Provider.ProviderId,
                            sizeof(HomeProvider.Provider.ProviderId),
                            &StringCbLength
                            )) != STATUS_SUCCESS )
        {
            StringCbLength = sizeof(HomeProvider.Provider.ProviderId);
        }

        MbbHomeProviderSize += ROUND_UP_COUNT( (ULONG)StringCbLength, ALIGN_DWORD );

        if( (NdisStatus = RtlStringCbLengthW(
                            HomeProvider.Provider.ProviderName,
                            sizeof(HomeProvider.Provider.ProviderName),
                            &StringCbLength
                            )) != STATUS_SUCCESS )
        {
            StringCbLength = sizeof(HomeProvider.Provider.ProviderName);
        }
        MbbHomeProviderSize += ROUND_UP_COUNT( (ULONG)StringCbLength, ALIGN_DWORD );

        //
        // Allocate & setup Mbb buffer
        //
        if( MbbHomeProviderSize > ULONG_MAX ||
           (MbbHomeProvider = ALLOCATE_NONPAGED_POOL( (ULONG)MbbHomeProviderSize )) == NULL )
        {
            TraceError( WMBCLASS_OID, "[MbbNdis][ReqID=0x%04x] FAILED to allocate MBB_PROVIDER_LIST of Size=%d",
                        Request->RequestId,
                        (ULONG)MbbHomeProviderSize
                        );
            NdisStatus = NDIS_STATUS_RESOURCES;
            break;
        }
        Request->HandlerContext.DataToFreeOnCompletion = MbbHomeProvider;


        //
        // Convert from WWAN to MBB
        //
        if( (MbbHomeProviderSize = MbbUtilWwanToMbbProvider2(
                                   &HomeProvider,
                                   (ULONG)MbbHomeProviderSize,
                                   MbbHomeProvider
                                    )) == 0 )
        {
            TraceError( WMBCLASS_OID, "[Util] FAILED to convert WWAN_PROVIDER2 to MBB_PROVIDER");
            break;
        }

        //
        // Send the data
        //
        NdisStatus = MbbUtilSetAttributeWithParameter(
                        Request,
                        (PCHAR)MbbHomeProvider,
                        (ULONG)MbbHomeProviderSize
                        );
        if( NdisStatus == NDIS_STATUS_SUCCESS ||
            NdisStatus == NDIS_STATUS_PENDING )
        {
            *InBufferSize = (ULONG)NdisSetHomeProvidersSize;
        }
    }
    while( FALSE );

    return NdisStatus;
}

NDIS_STATUS
MbbNdisSetMulticarrierProviders(
    __in PMBB_REQUEST_CONTEXT               Request,
    __in_bcount_opt(*InBufferSize) PUCHAR   InBuffer,
    __in PULONG                             InBufferSize
    )
{
    ULONG                               ElementIndex;
    ULONG                               ElementCount;
    ULONG                               ValidDataSize;
    ULONGLONG                           MbbPreferredProvidersSize;
    ULONGLONG                           NdisSetPreferredProvidersSize;
    PWWAN_PROVIDER2                     WwanProvider;
    NDIS_STATUS                         NdisStatus;
    PMBB_PROVIDER_LIST                  MbbPreferredProviders = NULL;
    PMINIPORT_ADAPTER_CONTEXT           Adapter = MbbReqMgrGetAdapterHandle( Request );
    BOOLEAN                             IsMultiCarrierCapable = MbbAdapterIsMultiCarrierCapable( Adapter );

    PNDIS_WWAN_SET_PREFERRED_MULTICARRIER_PROVIDERS  NdisSetPreferredProviders = (PNDIS_WWAN_SET_PREFERRED_MULTICARRIER_PROVIDERS)InBuffer;

    //
    // Size is already verified in MbbNdisMiniportOidRequest( )
    //
    __analysis_assume( *InBufferSize >= sizeof(NDIS_WWAN_SET_PREFERRED_MULTICARRIER_PROVIDERS) );
    do
    {
        //
        // Verify input buffer
        //
        if( NdisSetPreferredProviders->PreferredListHeader.ElementType != WwanStructProvider2 )
        {
            TraceError( WMBCLASS_OID, "[MbbNdis][ReqID=0x%04x] INVALID WWAN_LIST Expecting=%d Received=%d",
                        Request->RequestId,
                        WwanStructProvider2,
                        NdisSetPreferredProviders->PreferredListHeader.ElementType
                        );
            NdisStatus = NDIS_STATUS_INVALID_DATA;
            break;
        }
        ElementCount = NdisSetPreferredProviders->PreferredListHeader.ElementCount;
        NdisSetPreferredProvidersSize = sizeof(NDIS_WWAN_SET_PREFERRED_MULTICARRIER_PROVIDERS) + (((ULONGLONG)ElementCount) * sizeof(WWAN_PROVIDER2));

        if( *InBufferSize < NdisSetPreferredProvidersSize )
        {
            TraceError( WMBCLASS_OID,
                        "[MbbNdis][ReqID=0x%04x] INSUFFICIENT buffer size for NDIS_WWAN_SET_PREFERRED_MULTICARRIER_PROVIDERS, Expecting=%d Received=%d, ProviderCount=%d",
                        Request->RequestId,
                        (ULONG)NdisSetPreferredProvidersSize,
                       *InBufferSize,
                        ElementCount
                        );
            NdisStatus = NDIS_STATUS_INVALID_LENGTH;
            break;
        }
        //
        // Query for the required size
        //
        WwanProvider = (PWWAN_PROVIDER2)(((PCHAR)NdisSetPreferredProviders) +
                                         RTL_SIZEOF_THROUGH_FIELD( NDIS_WWAN_SET_PREFERRED_MULTICARRIER_PROVIDERS, PreferredListHeader ));

        MbbPreferredProvidersSize  = FIELD_OFFSET(MBB_PROVIDER_LIST, Providers);
        MbbPreferredProvidersSize += (ULONGLONG)ElementCount * sizeof(MBB_ARRAY_ELEMENT);

        for( ElementIndex = 0;
             ElementIndex < ElementCount;
             ElementIndex ++ )
        {
            SIZE_T StringCbLength;

            MbbPreferredProvidersSize  = ROUND_UP_COUNT( MbbPreferredProvidersSize, ALIGN_DWORD );
            MbbPreferredProvidersSize += sizeof(MBB_PROVIDER);

            if( (NdisStatus = RtlStringCbLengthW(
                               WwanProvider->Provider.ProviderId,
                               sizeof(WwanProvider->Provider.ProviderId),
                               &StringCbLength
                                )) != STATUS_SUCCESS )
            {
                StringCbLength = sizeof(WwanProvider->Provider.ProviderId);
            }
            MbbPreferredProvidersSize += ROUND_UP_COUNT( (ULONG)StringCbLength, ALIGN_DWORD );

            if( (NdisStatus = RtlStringCbLengthW(
                                WwanProvider->Provider.ProviderName,
                                sizeof(WwanProvider->Provider.ProviderName),
                               &StringCbLength
                                )) != STATUS_SUCCESS )
            {
                StringCbLength = sizeof(WwanProvider->Provider.ProviderName);
            }
            MbbPreferredProvidersSize += ROUND_UP_COUNT( (ULONG)StringCbLength, ALIGN_DWORD );
            WwanProvider++;
        }
        //
        // Allocate & setup Mbb buffer
        //
        if( MbbPreferredProvidersSize > ULONG_MAX ||
           (MbbPreferredProviders = (PMBB_PROVIDER_LIST)ALLOCATE_NONPAGED_POOL( (ULONG)MbbPreferredProvidersSize )) == NULL )
        {
            TraceError( WMBCLASS_OID, "[MbbNdis][ReqID=0x%04x] FAILED to allocate MBB_PROVIDER_LIST of Size=%d for ProviderCount=%d",
                        Request->RequestId,
                        (ULONG)MbbPreferredProvidersSize,
                        ElementCount
                        );
            NdisStatus = NDIS_STATUS_RESOURCES;
            break;
        }
        Request->HandlerContext.DataToFreeOnCompletion = MbbPreferredProviders;
        //
        // Convert from WWAN to MBB
        //
        ValidDataSize = (ULONG)MbbPreferredProvidersSize;

        if( (NdisStatus = MbbUtilWwanToMbbProviderList(
                           &NdisSetPreferredProviders->PreferredListHeader,
                            IsMultiCarrierCapable,
                            MbbCellularClassInvalid,
                            MbbPreferredProviders,
                           &ValidDataSize
                            )) != NDIS_STATUS_SUCCESS )
        {
            TraceError( WMBCLASS_OID, "[MbbNdis][ReqID=0x%04x] FAILED to convert WWAN_PROVIDER_LIST to MBB_PROVIDER_LIST ", Request->RequestId );
            break;
        }
        //
        // Send the data
        //
        NdisStatus = MbbUtilSetAttributeWithParameter(
                        Request,
                        (PCHAR)MbbPreferredProviders,
                        ValidDataSize
                        );
        if( NdisStatus == NDIS_STATUS_SUCCESS ||
            NdisStatus == NDIS_STATUS_PENDING )
        {
            *InBufferSize = (ULONG)NdisSetPreferredProvidersSize;
        }
    }
    while( FALSE );

    return NdisStatus;
}


NDIS_STATUS
MbbNdisSetPmParameters(
    __in PMBB_REQUEST_CONTEXT               Request,
    __in_bcount_opt(*InBufferSize) PUCHAR   InBuffer,
    __in PULONG                             InBufferSize
    )

{
    PMINIPORT_ADAPTER_CONTEXT   Adapter = (PMINIPORT_ADAPTER_CONTEXT)(Request->RequestManager->AdapterContext);
    PMBB_PACKET_FILTERS         MbbPacketFilters=NULL;
    ULONG                       BufferSize=0;
    NDIS_STATUS                 Status=STATUS_SUCCESS;
    PNDIS_OID_REQUEST           OidRequest = Request->OidContext.OidRequest;
    NDIS_PORT_NUMBER            PortNumber = OidRequest->PortNumber;
    PMBB_PORT                   Port = NULL;
    ULONG                       SessionId = MBB_INVALID_SESSION_ID;
    PNDIS_PM_PARAMETERS         PmParameters=(PNDIS_PM_PARAMETERS)InBuffer;

    *InBufferSize=sizeof(*PmParameters);

    //Only honor this request when its received on port number 0. This is because
    // this OID is sent by NDIS only during low power transitions <=D2. This should be seen
    // by the miniport only during low power transitions as NDIS absorbs it in other cases.
    // As per the current design, the WWAN virtual miniports are also going to send OID_PM_PARAMETERS
    // to the filter driver, after every OID_PM_ADD/REMOVE_WOL_PATTERN OID request. In this case also
    // OID_PM_PARAMETERS will be absorbed by NDIS and sent to the physical miniport on port 0.

    if(IS_ALLOCATED_PORT_NUMBER(PortNumber))
    {
        ASSERT(FALSE);
        TraceError( WMBCLASS_POWER, "Unexpected: OID_PM_PARAMETERS received on port number %lu instead of port number 0. Not handling and returning success", PortNumber);
        return NDIS_STATUS_SUCCESS;
    }

    if ((PmParameters->WakeUpFlags & NDIS_PM_SELECTIVE_SUSPEND_ENABLED) != 0)
    {
        TraceInfo( WMBCLASS_POWER, "Set PM Params: enable SS");
    }
    else
    {
        TraceInfo( WMBCLASS_POWER, "Set PM Params: enabling wake: events= %08lx, enabled patterns=%d",
            PmParameters->MediaSpecificWakeUpEvents,
            (PmParameters->EnabledWoLPacketPatterns & NDIS_PM_WOL_BITMAP_PATTERN_ENABLED) != 0
            );

        Status = MbbAdapterConfigurePacketFilters(Adapter,((PmParameters->EnabledWoLPacketPatterns & NDIS_PM_WOL_BITMAP_PATTERN_ENABLED) != 0));

        if (!NT_SUCCESS(Status))
        {
            TraceError( WMBCLASS_POWER, "Could not arm patterns");

            return Status;
        }

        Status=MbbAdapterConfigureDeviceServiceSubscription(Adapter, FALSE, PmParameters->MediaSpecificWakeUpEvents, PmParameters->WakeUpFlags);

        if (!NT_SUCCESS(Status))
        {
            TraceError( WMBCLASS_POWER, "Could not enabled wake events");
            return Status;
        }
        //
        //  tell the device we expect to not be sending traffic for awhile
        //
        Status = MbbAdapterSendNetworkIdleHint(Adapter);
    }

    return Status;
}

NDIS_STATUS
MbbNdisSetAddWolPattern(
    __in PMBB_REQUEST_CONTEXT               Request,
    __in_bcount_opt(*InBufferSize) PUCHAR   InBuffer,
    __in PULONG                             InBufferSize
    )
{
    PMINIPORT_ADAPTER_CONTEXT   Adapter = (PMINIPORT_ADAPTER_CONTEXT)(Request->RequestManager->AdapterContext);
    NDIS_STATUS                 Status;
    ULONGLONG                   BufferEnd;
    PNDIS_PM_WOL_PATTERN        Pattern=(PNDIS_PM_WOL_PATTERN)InBuffer;
    NDIS_PORT_NUMBER            PortNumber = Request->OidContext.OidRequest->PortNumber;

    TraceInfo( WMBCLASS_POWER, "Entered MbbNdisSetAddWolPattern for NDIS port number %lu", PortNumber);

    if (Pattern->WoLPacketType != NdisPMWoLPacketBitmapPattern)
    {
        TraceError( WMBCLASS_POWER, "Unsupported packet type");

        return NDIS_STATUS_NOT_SUPPORTED;
    }

    BufferEnd = (ULONGLONG)Pattern->WoLPattern.WoLBitMapPattern.MaskOffset + Pattern->WoLPattern.WoLBitMapPattern.MaskSize;

    if (BufferEnd > *InBufferSize)
    {
        TraceError( WMBCLASS_POWER, "buffer overflow in mask");

        return NDIS_STATUS_INVALID_PARAMETER;
    }

    BufferEnd = (ULONGLONG)Pattern->WoLPattern.WoLBitMapPattern.PatternOffset + Pattern->WoLPattern.WoLBitMapPattern.PatternSize;

    if (BufferEnd > *InBufferSize)
    {
        TraceError( WMBCLASS_POWER, "buffer overflow in pattern");

        return NDIS_STATUS_INVALID_PARAMETER;
    }

    //
    //  Each bit in the ndis mask represents a byte of data. For NCM each byte of mask is a bit filter for the pattern
    //
    if (Pattern->WoLPattern.WoLBitMapPattern.MaskSize * 8 > Adapter->BusParams.MaxPowerFilterSize)
    {
        TraceError( WMBCLASS_POWER, "Mask size is larger than supported by the device");

        return NDIS_STATUS_INVALID_PARAMETER;
    }

    // This request can arrive on disconnected connections too. Hence we just add the WOL
    // pattern to the adapter power filter table without looking at the connection state of the port.
    // During low power transition, only WOL patterns corresponding to connected ports will be
    // plumbed to the device.

    Status = MbbUtilSetPowerFilterPattern(
                Adapter,
                PortNumber,
                Pattern->PatternId,
                InBuffer + Pattern->WoLPattern.WoLBitMapPattern.MaskOffset,
                Pattern->WoLPattern.WoLBitMapPattern.MaskSize,
                InBuffer + Pattern->WoLPattern.WoLBitMapPattern.PatternOffset,
                Pattern->WoLPattern.WoLBitMapPattern.PatternSize
                );

    return Status;
}

NDIS_STATUS
MbbNdisSetRemoveAddWolPattern(
    __in PMBB_REQUEST_CONTEXT               Request,
    __in_bcount_opt(*InBufferSize) PUCHAR   InBuffer,
    __in PULONG                             InBufferSize
    )
{
    PMINIPORT_ADAPTER_CONTEXT   Adapter = (PMINIPORT_ADAPTER_CONTEXT)(Request->RequestManager->AdapterContext);
    ULONG                       PatternId=*(PULONG)InBuffer;
    NDIS_STATUS                 Status;
    NDIS_PORT_NUMBER            PortNumber = Request->OidContext.OidRequest->PortNumber;

    TraceInfo( WMBCLASS_POWER, "Entered MbbNdisSetRemoveAddWolPattern for PatternId=%d and NDIS port number %lu", PatternId, PortNumber  );

    Status = MbbUtilSetPowerFilterPattern(
                Adapter,
                PortNumber,
                PatternId,
                0,
                0,
                0,
                0
                );

    return Status;
}


NDIS_STATUS
MbbNdisQueryPower(
    __in PMBB_REQUEST_CONTEXT               Request,
    __in_bcount_opt(*InBufferSize) PUCHAR   InBuffer,
    __in PULONG                             InBufferSize,
    __out_bcount_part_opt(*OutBufferSize,*OutBufferSize) PUCHAR OutBuffer,
    __inout PULONG                                              OutBufferSize
    )

{
    PULONG      PowerState=(PULONG)OutBuffer;

    TraceInfo( WMBCLASS_POWER, "Query power D%d", *PowerState - NetDeviceStateD0);

    __analysis_assume( *OutBufferSize >= sizeof(*PowerState) );

    *OutBufferSize=sizeof(*PowerState);

    return NDIS_STATUS_SUCCESS;

}

NDIS_STATUS
MbbNdisSetLteAttachContexts(
    __in PMBB_REQUEST_CONTEXT               Request,
    __in_bcount_opt(*InBufferSize) PUCHAR   InBuffer,
    __in PULONG                             InBufferSize
    )
{
    PMBB_SET_MS_CONTEXT_LTE_ATTACH_CONFIG MbbSetLteAttachContext = NULL;
    PNDIS_WWAN_SET_LTE_ATTACH_CONTEXT    NdisSetLteAttachContext= (PNDIS_WWAN_SET_LTE_ATTACH_CONTEXT)InBuffer;
    ULONG                               BufferSize=0;
    NDIS_STATUS                         Status;
    PMINIPORT_ADAPTER_CONTEXT  Adapter = Request->RequestManager->AdapterContext;

    MbbAdapterLock(Adapter);
    BOOL IsLteAttachCapable = Adapter->AdapterFlags.IsLTEAttachConfigCapable;
    MbbAdapterUnlock(Adapter);

    if (!IsLteAttachCapable)
    {
        TraceInfo(WMBCLASS_INIT, "[MbbAdapter] device does not support set LTE Attach configurations");
        return NDIS_STATUS_NOT_SUPPORTED;
    }

    //
    // Size is already verified in MbbNdisMiniportOidRequest( )
    //
    __analysis_assume( *InBufferSize >= sizeof(NDIS_WWAN_SET_LTE_ATTACH_CONTEXT) );

    // LTE attach scenario
    Status=MbbUtilWwanToMbbSetLteAttachContext(
        &NdisSetLteAttachContext->SetLteAttachContext,
        &MbbSetLteAttachContext,
        &BufferSize
        );
    if (!NT_SUCCESS(Status))
    {
        return Status;
    }

    Request->HandlerContext.DataToFreeOnCompletion = MbbSetLteAttachContext;

    return MbbUtilSetAttributeWithParameter(
                Request,
                (PCHAR) MbbSetLteAttachContext,
                BufferSize
                );

}

NDIS_STATUS
MbbNdisSetProvisionedContexts(
    __in PMBB_REQUEST_CONTEXT               Request,
    __in_bcount_opt(*InBufferSize) PUCHAR   InBuffer,
    __in PULONG                             InBufferSize
    )
{
    PMBB_SET_CONTEXT                    MbbSetContext=NULL;
    PNDIS_WWAN_SET_PROVISIONED_CONTEXT  NdisSetProvisionedContext= (PNDIS_WWAN_SET_PROVISIONED_CONTEXT)InBuffer;
    ULONG                               BufferSize=0;
    NDIS_STATUS                         Status;

    //
    // Size is already verified in MbbNdisMiniportOidRequest( )
    //
    __analysis_assume( *InBufferSize >= sizeof(NDIS_WWAN_SET_PROVISIONED_CONTEXT) );

    *InBufferSize = sizeof(NDIS_WWAN_SET_PROVISIONED_CONTEXT);

    Status=MbbUtilWwanToMbbSetContext(
        &NdisSetProvisionedContext->ProvisionedContext,
        &MbbSetContext,
        &BufferSize
        );

    if (!NT_SUCCESS(Status))
    {
        return Status;
    }

    Request->HandlerContext.DataToFreeOnCompletion = MbbSetContext;

    return MbbUtilSetAttributeWithParameter(
                Request,
                (PCHAR) MbbSetContext,
                BufferSize
                );
}

NDIS_STATUS
MbbNdisSetProvisionedContextsV2(
    __in PMBB_REQUEST_CONTEXT               Request,
    __in_bcount_opt(*InBufferSize) PUCHAR   InBuffer,
    __in PULONG                             InBufferSize
)
{
    PMBB_SET_MS_CONTEXT_PROVISIONED_CONTEXT_V2  MbbSetContext = NULL;
    PNDIS_WWAN_SET_PROVISIONED_CONTEXT_V2       NdisSetProvisionedContext = (PNDIS_WWAN_SET_PROVISIONED_CONTEXT_V2)InBuffer;
    ULONG                                       BufferSize = 0;
    NDIS_STATUS                                 Status;
    PMINIPORT_ADAPTER_CONTEXT                   Adapter = Request->RequestManager->AdapterContext;

    MbbAdapterLock(Adapter);
    BOOL IsProvisionedContextV2Capable = Adapter->AdapterFlags.IsProvisionedContextV2Capable;
    MbbAdapterUnlock(Adapter);

    if (!IsProvisionedContextV2Capable)
    {
        TraceInfo(WMBCLASS_INIT, "[MbbAdapter] device does not support ProvisionedContextV2");
        return NDIS_STATUS_NOT_SUPPORTED;
    }

    //
    // Size is already verified in MbbNdisMiniportOidRequest( )
    //
    __analysis_assume(*InBufferSize >= sizeof(NDIS_WWAN_SET_PROVISIONED_CONTEXT_V2));

    *InBufferSize = sizeof(NDIS_WWAN_SET_PROVISIONED_CONTEXT_V2);

    Status = MbbUtilWwanToMbbSetContextV2(
        &NdisSetProvisionedContext->ProvisionedContext,
        &MbbSetContext,
        &BufferSize
    );

    if (!NT_SUCCESS(Status))
    {
        return Status;
    }

    Request->HandlerContext.DataToFreeOnCompletion = MbbSetContext;

    return MbbUtilSetAttributeWithParameter(
        Request,
        (PCHAR)MbbSetContext,
        BufferSize
    );
}

NDIS_STATUS
MbbNdisSetServiceActivation(
    __in PMBB_REQUEST_CONTEXT               Request,
    __in_bcount_opt(*InBufferSize) PUCHAR   InBuffer,
    __in PULONG                             InBufferSize
    )
{
    NDIS_STATUS                         NdisStatus = NDIS_STATUS_SUCCESS;
    ULONG                               MbbServiceActivationSize;
    PMBB_SERVICE_ACTIVATION             MbbServiceActivation = NULL;
    PNDIS_WWAN_SERVICE_ACTIVATION       NdisServiceActivation = (PNDIS_WWAN_SERVICE_ACTIVATION)InBuffer;

    //
    // Size is already verified in MbbNdisMiniportOidRequest( )
    //
    __analysis_assume( *InBufferSize >= sizeof(NDIS_WWAN_SERVICE_ACTIVATION) );
    *InBufferSize = sizeof(NDIS_WWAN_SERVICE_ACTIVATION);

    do
    {
        MbbServiceActivationSize = NdisServiceActivation->ServiceActivation.uVendorSpecificBufferSize;

        if (MbbServiceActivationSize > MBB_MAX_SERVICE_ACTIVATION_BUFFER)
        {
            TraceError( WMBCLASS_OID, "[MbbNdis][ReqId=0x%04x] actrivation buffer too big %d", Request->RequestId, MbbServiceActivationSize );

            NdisStatus = NDIS_STATUS_INVALID_LENGTH;
            break;
        }

        if (MbbServiceActivationSize + RTL_SIZEOF_THROUGH_FIELD( NDIS_WWAN_SERVICE_ACTIVATION, ServiceActivation.uVendorSpecificBufferSize ) > *InBufferSize)
        {
            TraceError( WMBCLASS_OID, "[MbbNdis][ReqId=0x%04x] activation buffer larger than Input Buffer %d", Request->RequestId, MbbServiceActivationSize );

            NdisStatus = NDIS_STATUS_BUFFER_TOO_SHORT;
            break;
        }


        if( (MbbServiceActivation = ALLOCATE_NONPAGED_POOL( MbbServiceActivationSize )) == NULL )
        {
            TraceError( WMBCLASS_OID, "[MbbNdis][ReqId=0x%04x] FAILED to allocate SERVICE_ACTIVATION buffer", Request->RequestId );
            NdisStatus = NDIS_STATUS_RESOURCES;
            break;
        }
        Request->HandlerContext.DataToFreeOnCompletion = MbbServiceActivation;

        RtlCopyMemory(
            MbbServiceActivation->VendorSpecificBuffer,
            NdisServiceActivation + 1,
            MbbServiceActivationSize
            );

        NdisStatus = MbbUtilSetAttributeWithParameter(
                        Request,
                        (PCHAR)(MbbServiceActivation),
                        MbbServiceActivationSize
                        );
    }
    while( FALSE );

    return NdisStatus;
}

// SMS

NDIS_STATUS
MbbNdisSmsSetConfiguration(
    __in PMBB_REQUEST_CONTEXT               Request,
    __in_bcount_opt(*InBufferSize) PUCHAR   InBuffer,
    __in PULONG                             InBufferSize
    )
{
    ULONG                               MbbBufferSize;
    SIZE_T                              ScAddressSize;
    NTSTATUS                            NtStatus;
    PMBB_SET_SMS_CONFIGURATION          MbbSetSmsConfiguration;
    PNDIS_WWAN_SET_SMS_CONFIGURATION    NdisSmsSetConfiguration = (PNDIS_WWAN_SET_SMS_CONFIGURATION)InBuffer;

    //
    // Size is already verified in MbbNdisMiniportOidRequest( )
    //
    __analysis_assume( *InBufferSize >= sizeof(NDIS_WWAN_SET_SMS_CONFIGURATION) );
    *InBufferSize = sizeof(NDIS_WWAN_SET_SMS_CONFIGURATION);

    NtStatus = RtlStringCbLengthA(
                        NdisSmsSetConfiguration->SetSmsConfiguration.ScAddress,
                        WWAN_SMS_ADDRESS_MAX_LEN,
                       &ScAddressSize
                       );

    if ( NtStatus != STATUS_SUCCESS )
    {
        TraceError( WMBCLASS_OID, "[MbbNdis][Sms][ReqId=0x%04x] INVALID ScAddress OID_WWAN_SMS_CONFIGURATION SET, status=%!STATUS!",
                    Request->RequestId,
                    NtStatus
                    );

        return NtStatus;
    }

    NtStatus=MbbUtilWwanToMbbSmsSetConfiguration(
       &NdisSmsSetConfiguration->SetSmsConfiguration,
        (ULONG)ScAddressSize,
        &MbbSetSmsConfiguration,
        &MbbBufferSize
        );

    if ( NtStatus != STATUS_SUCCESS )
    {
        TraceError( WMBCLASS_OID, "[MbbNdis][Sms][ReqId=0x%04x] could not convert SMS config to MBB, status=%!STATUS!",
                    Request->RequestId,
                    NtStatus
                    );

        return NtStatus;
    }

    Request->HandlerContext.DataToFreeOnCompletion = MbbSetSmsConfiguration;

    return  MbbUtilSetAttributeWithParameter(
                Request,
                (PCHAR) MbbSetSmsConfiguration,
                MbbBufferSize
                );
}

NDIS_STATUS
MbbNdisSmsSend(
    __in PMBB_REQUEST_CONTEXT               Request,
    __in_bcount_opt(*InBufferSize) PUCHAR   InBuffer,
    __in PULONG                             InBufferSize
    )
{
    ULONG                   MbbSmsSendSize;
    NDIS_STATUS             NdisStatus;
    PMBB_SMS_SEND           MbbSmsSend;
    PNDIS_WWAN_SMS_SEND     NdisSmsSend = (PNDIS_WWAN_SMS_SEND)InBuffer;
    MBB_CELLULAR_CLASS      CellularClass = MbbAdapterGetCurrentCellularClass( MbbReqMgrGetAdapterHandle( Request ) );

    //
    // Size is already verified in MbbNdisMiniportOidRequest( )
    //
    __analysis_assume( *InBufferSize >= sizeof(NDIS_WWAN_SMS_SEND) );
    *InBufferSize = sizeof(NDIS_WWAN_SMS_SEND);

    if (MbbAdapterIsMultimodeCapable(MbbReqMgrGetAdapterHandle( Request )))
    {
        //
        //  multimode device always use GSM PDU's
        //
        CellularClass = MbbCellularClassGsm;
    }


    if( NdisSmsSend->SmsSend.SmsFormat == WwanSmsFormatPdu )
    {

        NdisStatus=MbbUtilWwanToMbbSmsSendPdu(
            &NdisSmsSend->SmsSend.u.Pdu,
            CellularClass,
            &MbbSmsSend,
            &MbbSmsSendSize
            );

    }
    else
    {

        NdisStatus=MbbUtilWwanToMbbSmsSendCdma(
            &NdisSmsSend->SmsSend.u.Cdma,
            &MbbSmsSend,
            &MbbSmsSendSize
            );


    }

    if (NdisStatus != NDIS_STATUS_SUCCESS)
    {
        TraceError( WMBCLASS_OID, "[MbbNdis][Sms][ReqId=0x%04x] FAILED to convert data for OID_WWAN_SMS_SEND, status=%!STATUS!",
                    Request->RequestId,
                    NdisStatus
                    );
        return NdisStatus;

    }

    MbbSmsSend->SmsFormat = MbbUtilWwanToMbbSmsFormat( NdisSmsSend->SmsSend.SmsFormat );
    Request->HandlerContext.DataToFreeOnCompletion = MbbSmsSend;

    //
    // Send CID
    //
    return  MbbUtilSetAttributeWithParameter(
                Request,
                (PCHAR) MbbSmsSend,
                MbbSmsSendSize
                );
}

NDIS_STATUS
MbbNdisSmsDelete(
    __in PMBB_REQUEST_CONTEXT               Request,
    __in_bcount_opt(*InBufferSize) PUCHAR   InBuffer,
    __in PULONG                             InBufferSize
    )
{
    PMBB_SMS_DELETE         MbbSmsDelete;
    PNDIS_WWAN_SMS_DELETE   NdisSmsDelete = (PNDIS_WWAN_SMS_DELETE)InBuffer;

    //
    // Size is already verified in MbbNdisMiniportOidRequest( )
    //
    __analysis_assume( *InBufferSize >= sizeof(NDIS_WWAN_SMS_DELETE) );
    *InBufferSize = sizeof(NDIS_WWAN_SMS_DELETE);
    //
    // TODO: If WWAN_SMS_FILTER and MBB_SMS_DELETE are of the same layout.
    // avoid unneccesary data copy and use the passed in
    // NDIS format and do not convert to MBB format.
    //
    if( (MbbSmsDelete = (PMBB_SMS_DELETE) ALLOCATE_NONPAGED_POOL( sizeof(MBB_SMS_DELETE) )) == NULL )
    {
        return NDIS_STATUS_RESOURCES;
    }
    Request->HandlerContext.DataToFreeOnCompletion = MbbSmsDelete;

    MbbUtilWwanToMbbSmsDelete(
        &NdisSmsDelete->SmsFilter,
        MbbSmsDelete
        );
    return  MbbUtilSetAttributeWithParameter(
                Request,
                (PCHAR) MbbSmsDelete,
                sizeof(MBB_SMS_DELETE)
                );
}

// DEVICE SERVICE

NDIS_STATUS
MbbNdisSubscribeDeviceServiceEvents(
    __in PMBB_REQUEST_CONTEXT               Request,
    __in_bcount_opt(*InBufferSize) PUCHAR   InBuffer,
    __in PULONG                             InBufferSize
    )
{
    NDIS_STATUS                                 NdisStatus = NDIS_STATUS_SUCCESS;
    ULONG                                       OidSubscribeListSize = 0;
    PMBB_SUBSCRIBE_EVENT_LIST                   OidSubscribeList = NULL;
    PNDIS_WWAN_SUBSCRIBE_DEVICE_SERVICE_EVENTS  NdisSubscribeEvents = (PNDIS_WWAN_SUBSCRIBE_DEVICE_SERVICE_EVENTS)InBuffer;
    GUID*                                       NdisSubscribeGuid = NULL;
    PMBB_SUBSCRIBE_EVENT_LIST                   MbbSubscribeList = NULL;
    ULONG                                       MbbSubscribeListSize = 0;
    ULONGLONG                                   RequiredInputSize = 0;
    PMINIPORT_ADAPTER_CONTEXT                   Adapter = NULL;


    do
    {
        //
        // Size is already verified in MbbNdisMiniportOidRequest( )
        //
        __analysis_assume( *InBufferSize >= sizeof(NDIS_WWAN_SUBSCRIBE_DEVICE_SERVICE_EVENTS) );

        //
        // First, for the specified device services, generate the list
        // of device service + CIDs that we want to subscribe to
        //
        RequiredInputSize = sizeof(NDIS_WWAN_SUBSCRIBE_DEVICE_SERVICE_EVENTS) +
                            (ULONGLONG)NdisSubscribeEvents->DeviceServiceListHeader.ElementCount * sizeof(GUID);

        if (*InBufferSize < RequiredInputSize)
        {
            TraceError( WMBCLASS_OID, "[MbbNdis][ReqId=0x%04x] INSUFFICIENT BufferSize for NDIS_WWAN_SUBSCRIBE_DEVICE_SERVICE_EVENTS, BufferSize[Received=%d Expected=%I64d]",
                        Request->RequestId,
                        *InBufferSize,
                        RequiredInputSize
                        );

            if (RequiredInputSize > ULONG_MAX)
            {
                *InBufferSize = ULONG_MAX;
            }
            else
            {
                *InBufferSize = (ULONG)RequiredInputSize;
            }
            NdisStatus = NDIS_STATUS_INVALID_LENGTH;
            break;
        }

        Adapter = ((PMINIPORT_ADAPTER_CONTEXT)MbbReqMgrGetAdapterHandle( Request ));

        if (NdisSubscribeEvents->DeviceServiceListHeader.ElementCount != 0)
        {
            // Get the specified GUIDs
            NdisSubscribeGuid = (GUID *)((PUCHAR)InBuffer +
                    sizeof(NDIS_WWAN_SUBSCRIBE_DEVICE_SERVICE_EVENTS));

            // Generate the Mbb structure for this request
            NdisStatus = MbbUtilWwanToMbbSubscribeEvents(Adapter,
                NdisSubscribeGuid,
                NdisSubscribeEvents->DeviceServiceListHeader.ElementCount,
                TRUE,                                   // External request, so validate
                NULL,
                0,                                      // No padding
                &OidSubscribeList,
                &OidSubscribeListSize
                );

            if (NdisStatus != NDIS_STATUS_SUCCESS)
            {
                TraceError( WMBCLASS_OID, "[MbbNdis][ReqId=0x%04x] FAILED to convert OID subscribe list to MBB subscribe list",
                            Request->RequestId
                            );
                break;
            }
        }
        else
        {
            OidSubscribeList = NULL;
            OidSubscribeListSize = 0;
        }

        // Save this as the OID configured subscribe list till we finish the
        // transaction. If the transaction succeeds, we will persist this list
        Request->HandlerContext.Parameters.EventSubscribe.ExtList = OidSubscribeList;
        Request->HandlerContext.Parameters.EventSubscribe.ExtSize = OidSubscribeListSize;

        // Save the pointer to the buffer so that it gets freed automatically on failure paths
        Request->HandlerContext.DataToFreeOnResponse = OidSubscribeList;

        //
        // Now, merge this with the internally generated list
        // that we want to subscribe to
        //
        NdisStatus = MbbUtilGenerateSubscribeEventList(
            (PMINIPORT_ADAPTER_CONTEXT)(Request->RequestManager->AdapterContext),
            TRUE,               // Awake
            0,
            0,
            OidSubscribeList,
            OidSubscribeListSize,
            &MbbSubscribeList,
            &MbbSubscribeListSize
            );
        if (NdisStatus != NDIS_STATUS_SUCCESS)
        {
            TraceError( WMBCLASS_OID, "[MbbNdis][ReqId=0x%04x] FAILED to generate merged MBB_SUBSCRIBE_EVENT_LIST, status=%!STATUS!",
                        Request->RequestId, NdisStatus
                        );
            break;
        }

        // The buffer we are using for the request is saved for freeing on completion
        Request->HandlerContext.DataToFreeOnCompletion = MbbSubscribeList;

        NdisStatus = MbbUtilSetAttributeWithParameter(
                Request,
                (PCHAR) MbbSubscribeList,
                MbbSubscribeListSize
                );
        if ((NdisStatus != NDIS_STATUS_SUCCESS) &&
            (NdisStatus != NDIS_STATUS_PENDING))
        {
            TraceError( WMBCLASS_OID, "[MbbNdis][ReqId=0x%04x] FAILED to submit merged MBB_SUBSCRIBE_EVENT_LIST, status=%!STATUS!",
                        Request->RequestId, NdisStatus
                        );
            break;
        }


    } while (FALSE);

    if ((NdisStatus != NDIS_STATUS_SUCCESS) &&
        (NdisStatus != NDIS_STATUS_PENDING))
    {
        if (Request->HandlerContext.DataToFreeOnCompletion)
            FREE_POOL(Request->HandlerContext.DataToFreeOnCompletion);

        if (Request->HandlerContext.DataToFreeOnResponse)
            FREE_POOL(Request->HandlerContext.DataToFreeOnResponse);
    }

    return NdisStatus;
}

NDIS_STATUS
MbbNdisSetDeviceServiceCommand(
    __in PMBB_REQUEST_CONTEXT               Request,
    __in_bcount_opt(*InBufferSize) PUCHAR   InBuffer,
    __in PULONG                             InBufferSize
    )
{
    return MbbUtilDeviceServiceCommand(
            Request,
            InBuffer,
            InBufferSize
            );
}

NDIS_STATUS
MbbNdisSetVendorSpecific(
    __in PMBB_REQUEST_CONTEXT               Request,
    __in_bcount_opt(*InBufferSize) PUCHAR   InBuffer,
    __in PULONG                             InBufferSize
    )
{
    NDIS_STATUS                         NdisStatus;
    MBB_COMMAND                         MbbCommand;
    PNDIS_WWAN_VENDOR_SPECIFIC          NdisVendorSpecific;

    do
    {
        if( *InBufferSize < sizeof(NDIS_WWAN_VENDOR_SPECIFIC) )
        {
            TraceError( WMBCLASS_OID, "[MbbNdis][Ven][ReqId=0x%04x] INSUFFICIENT InputBufferSize=%d Bytes for VENDOR_SPECIFIC, Expected=%d Bytes",
                        Request->RequestId, *InBufferSize, sizeof(NDIS_WWAN_VENDOR_SPECIFIC) );

            NdisStatus = NDIS_STATUS_INVALID_LENGTH;
            *InBufferSize = sizeof(NDIS_WWAN_VENDOR_SPECIFIC);
            break;
        }

        NdisVendorSpecific = (PNDIS_WWAN_VENDOR_SPECIFIC) InBuffer;


        if( NdisVendorSpecific->Header.Type != NDIS_OBJECT_TYPE_DEFAULT ||
            NdisVendorSpecific->Header.Size < sizeof(NDIS_WWAN_VENDOR_SPECIFIC) ||
            NdisVendorSpecific->Header.Revision < NDIS_WWAN_VENDOR_SPECIFIC_REVISION_1 )
        {
            TraceError( WMBCLASS_OID, "[MbbNdis][Ven][ReqId=0x%04x] INVALID ndis header for VENDOR_SPECIFIC, Type=0x%x Size=%d Revision=%d",
                        Request->RequestId,
                        NdisVendorSpecific->Header.Type,
                        NdisVendorSpecific->Header.Size,
                        NdisVendorSpecific->Header.Revision
                        );
            NdisStatus = NDIS_STATUS_INVALID_DATA;
            break;
        }

        if (NdisVendorSpecific->VendorSpecificData.uVendorSpecificBufferSize +
            RTL_SIZEOF_THROUGH_FIELD(NDIS_WWAN_VENDOR_SPECIFIC,VendorSpecificData.uVendorSpecificBufferSize) > *InBufferSize )
        {
            TraceError( WMBCLASS_OID, "[MbbNdis][Ven][ReqId=0x%04x] INSUFFICIENT InputBufferSize=%d Bytes for VENDOR_SPECIFIC, Expected=%d Bytes",
                        Request->RequestId, *InBufferSize, sizeof(NDIS_WWAN_VENDOR_SPECIFIC) );

            *InBufferSize = NdisVendorSpecific->VendorSpecificData.uVendorSpecificBufferSize +
                            RTL_SIZEOF_THROUGH_FIELD(NDIS_WWAN_VENDOR_SPECIFIC,VendorSpecificData.uVendorSpecificBufferSize);

            NdisStatus = NDIS_STATUS_INVALID_LENGTH;
            break;
        }

        RtlCopyMemory(
            &MbbCommand.ServiceId,
            &MBB_UUID_MS_VENDOR_EXTENSION,
            sizeof(GUID)
            );

        MbbCommand.CommandId = MBIM_CID_VENDORSPECIFIC;
        //
        // Call the wrapper routine to allocate
        // and format the message buffers.
        //
        if( (NdisStatus = MbbUtilSetupCommandMessage(
                                Request,
                               &MbbCommand,
                                MbbReqMgrIsSetOid( Request )? MBB_COMMAND_TYPE_SET: MBB_COMMAND_TYPE_QUERY,
                                (PUCHAR)(NdisVendorSpecific + 1),
                                NdisVendorSpecific->VendorSpecificData.uVendorSpecificBufferSize
                                )) != NDIS_STATUS_SUCCESS )
        {
            TraceError( WMBCLASS_OID, "[MbbNdis][Ven][ReqId=0x%04x] FAILED to setup command message for VENDOR_SPECIFIC with status=%!status!",
                        Request->RequestId, NdisStatus );
            break;
        }

        //
        // Call the wrapper routine to send each fragment.
        // The wrapper will cleanup fragments in case of
        // success or failure.
        //
        NdisStatus = MbbUtilSendMessageFragmentsAndLog( Request );
        if( NdisStatus != NDIS_STATUS_SUCCESS &&
            NdisStatus != NDIS_STATUS_PENDING )
        {
            TraceError( WMBCLASS_OID, "[MbbNdis][Ven][ReqId=0x%04x] FAILED to send message fragments for VENDOR_SPECIFIC with status=%!status!",
                        Request->RequestId, NdisStatus );
        }

    } while( FALSE );

    return NdisStatus;
}


NDIS_STATUS
MbbNdisSetDeviceServiceSession(
    __in PMBB_REQUEST_CONTEXT               Request,
    __in_bcount_opt(*InBufferSize) PUCHAR   InBuffer,
    __in PULONG                             InBufferSize
    )
{
    MBB_SET_DSS_CONNECT         MbbDssConnect;

    PNDIS_WWAN_SET_DEVICE_SERVICE_SESSION   NdisDssRequest  = (PNDIS_WWAN_SET_DEVICE_SERVICE_SESSION)InBuffer;
    NDIS_STATUS                 Status;

    //
    // Size is already verified in MbbNdisMiniportOidRequest( )
    //
    __analysis_assume( *InBufferSize >= sizeof(NDIS_WWAN_SET_DEVICE_SERVICE_SESSION) );
    *InBufferSize = sizeof(NDIS_WWAN_SET_DEVICE_SERVICE_SESSION);

    if (MbbUtilIsNativeMbnService(&(NdisDssRequest->Session.DeviceServiceGuid)))
    {
        // Native device services cannot be used using this path
        TraceError( WMBCLASS_OID, "[MbbNdis][ReqId=0x%04x] FAILED because of request for native device service %!GUID!",
                    Request->RequestId,
                    &(NdisDssRequest->Session.DeviceServiceGuid)
                    );

        return NDIS_STATUS_INVALID_DATA;
    }

    //
    // Save stuff about the request in the set so that we can send the relevant
    // info in the indication
    //
    RtlCopyMemory(
        &Request->HandlerContext.Parameters.DssSession.DeviceServiceGuid,
        &(NdisDssRequest->Session.DeviceServiceGuid),
        sizeof(GUID)
        );
    Request->HandlerContext.Parameters.DssSession.SessionId = NdisDssRequest->Session.uSessionID;


    if ((NdisDssRequest->Session.State == WwanDeviceServiceSessionOpen)
        ||
        (NdisDssRequest->Session.State == WwanDeviceServiceSessionClosed))
    {

        // Send the Open Request

        // Copy the device service in device format
        MBB_UUID_TO_NET(&MbbDssConnect.DeviceServiceId,
            &(NdisDssRequest->Session.DeviceServiceGuid)
            );

        MbbDssConnect.DssSessionId = NdisDssRequest->Session.uSessionID;

        MbbDssConnect.DssLinkState = (NdisDssRequest->Session.State == WwanDeviceServiceSessionOpen) ? MbbDssLinkActivate : MbbDssLinkDeactivate;

        Request->HandlerContext.Parameters.DssSession.LinkState = MbbDssConnect.DssLinkState;

        return MbbUtilSetAttributeWithParameter(
                    Request,
                    (PCHAR)(&MbbDssConnect),
                    sizeof(MbbDssConnect)
                    );
    }
    else
    {

        TraceError( WMBCLASS_OID, "[MbbNdis][ReqId=0x%04x] Device Service Session state specified is invalid, %d",
                    Request->RequestId,
                    NdisDssRequest->Session.State);

        return NDIS_STATUS_INVALID_PARAMETER;
    }
}

NDIS_STATUS
MbbNdisDssWrite(
    __in PMBB_REQUEST_CONTEXT               Request,
    __in_bcount_opt(*InBufferSize) PUCHAR   InBuffer,
    __in PULONG                             InBufferSize
    )
{
    NDIS_STATUS                         NdisStatus;
    MBB_COMMAND                         MbbCommand;
    PNDIS_WWAN_DEVICE_SERVICE_SESSION_WRITE   NdisDssWriteRequest;

    // The session write gets pushed through the send path to the device
    do
    {
        __analysis_assume( *InBufferSize >= sizeof(NDIS_WWAN_DEVICE_SERVICE_SESSION_WRITE) );
        *InBufferSize = sizeof(NDIS_WWAN_DEVICE_SERVICE_SESSION_WRITE);

        NdisDssWriteRequest = (PNDIS_WWAN_DEVICE_SERVICE_SESSION_WRITE) InBuffer;

        if (NdisDssWriteRequest->WriteData.uDataSize == 0)
        {
            TraceError( WMBCLASS_OID, "[MbbNdis][Ven][ReqId=0x%04x] Invalid size for device service session write",
                        Request->RequestId);
            NdisStatus = NDIS_STATUS_INVALID_DATA;
            break;
        }

        // Ref this request so that we can pass a reference to it to send path. When
        // the send path completes the request, we will release the ref
        MbbReqMgrRefRequest(Request);

        NdisStatus = MbbSendDeviceServiceSessionData(
                        ((PMINIPORT_ADAPTER_CONTEXT)MbbReqMgrGetAdapterHandle( Request )),
                        Request,
                        Request->RequestId,
                        NdisDssWriteRequest->WriteData.uSessionID,
                        NdisDssWriteRequest->WriteData.uDataSize,
                        (PUCHAR)(NdisDssWriteRequest + 1)
                        );
        if( NdisStatus != NDIS_STATUS_SUCCESS )
        {
            MbbReqMgrDerefRequest(Request);
            TraceError( WMBCLASS_OID, "[MbbNdis][Ven][ReqId=0x%04x] FAILED to write device service data with status=%!status!",
                        Request->RequestId, NdisStatus );
        }

    } while( FALSE );

    return NdisStatus;
}

NDIS_STATUS
MbbNdisDeleteMac(
    __in PMBB_REQUEST_CONTEXT               Request,
    __in_bcount_opt(*InBufferSize) PUCHAR   InBuffer,
    __in PULONG                             InBufferSize
    )
{
    NDIS_STATUS         ndisStatus = NDIS_STATUS_SUCCESS;
    PNDIS_WWAN_MAC_INFO ndisWwanMacInfo = NULL;
    PNDIS_OID_REQUEST   oidRequest = Request->OidContext.OidRequest;

    UNREFERENCED_PARAMETER(InBuffer);
    UNREFERENCED_PARAMETER(InBufferSize);

    do
    {
        oidRequest->DATA.SET_INFORMATION.BytesRead = 0;
        oidRequest->DATA.SET_INFORMATION.BytesNeeded = 0;

        if (oidRequest->RequestType != NdisRequestSetInformation)
        {
            TraceError( WMBCLASS_OID, "[MbbNdis][ReqID=0x%04x] Invalid request type %d for OID_WWAN_DELETE_MAC\n",
                        Request->RequestId,
                        oidRequest->RequestType
                        );
            ndisStatus = NDIS_STATUS_NOT_SUPPORTED;
            break;
        }

        ASSERT(oidRequest->DATA.SET_INFORMATION.InformationBufferLength >= sizeof(NDIS_WWAN_MAC_INFO));

        ndisWwanMacInfo = (PNDIS_WWAN_MAC_INFO)oidRequest->DATA.SET_INFORMATION.InformationBuffer;

        if (!IS_ALLOCATED_PORT_NUMBER(ndisWwanMacInfo->uNdisPortNumber))
        {
            TraceError (WMBCLASS_OID, "[MbbNdis][ReqID=0x%04x] The port number (%d) being passed in is invalid",
                        Request->RequestId,
                        ndisWwanMacInfo->uNdisPortNumber);

            ndisStatus = NDIS_STATUS_INVALID_DATA;
            break;
        }

        ndisStatus = MbbUtilWwanDeleteMac(Request);

        if (ndisStatus != NDIS_STATUS_SUCCESS)
        {
             TraceError(WMBCLASS_OID,"MbbNdis][ReqID=0x%04x] MbbUtilWwanDeleteMac failed. Status = 0x%08x\n",
                        Request->RequestId,ndisStatus);
            break;
        }
    }while (FALSE);

    return ndisStatus;
}

NDIS_STATUS
MbbNdisUiccUpdateBinary(
    __in PMBB_REQUEST_CONTEXT               Request,
    __in_bcount_opt(*InBufferSize) PUCHAR   InBuffer,
    __in PULONG                             InBufferSize
    )
{
    return MbbUtilUiccAccessBinary(
            Request,
            InBuffer,
            InBufferSize
            );
}

NDIS_STATUS
MbbNdisUiccUpdateRecord(
    __in PMBB_REQUEST_CONTEXT               Request,
    __in_bcount_opt(*InBufferSize) PUCHAR   InBuffer,
    __in PULONG                             InBufferSize
    )
{
    return MbbUtilUiccAccessRecord(
            Request,
            InBuffer,
            InBufferSize
            );
}

NDIS_STATUS
MbbNdisSetSysSlotMappings(
    __in PMBB_REQUEST_CONTEXT               Request,
    __in_bcount_opt(*InBufferSize) PUCHAR   InBuffer,
    __in PULONG                             InBufferSize
)
{
    PNDIS_WWAN_SET_DEVICE_SLOT_MAPPING_INFO NdisSetDeviceSlotMappingInfo = (PNDIS_WWAN_SET_DEVICE_SLOT_MAPPING_INFO)InBuffer;
    PMBB_MS_DEVICE_SLOT_MAPPING_INFO MbbSetDeviceSlotMappingInfo = NULL;
    NDIS_STATUS NdisStatus = NDIS_STATUS_SUCCESS;
    ULONG BufferSize = 0;

    NdisStatus = MbbUtilWwanToMbbSetDeviceSlotMappingInfo(
        &(NdisSetDeviceSlotMappingInfo->SetDeviceSlotMappingInfo),
        &MbbSetDeviceSlotMappingInfo,
        &BufferSize);

    if (NdisStatus != NDIS_STATUS_SUCCESS)
    {
        return NdisStatus;
    }

    Request->HandlerContext.DataToFreeOnCompletion = MbbSetDeviceSlotMappingInfo;

    return MbbUtilSetAttributeWithParameter(
        Request,
        (PCHAR)MbbSetDeviceSlotMappingInfo,
        BufferSize);
}

NDIS_STATUS
MbbNdisSetDeviceBindings(
    __in PMBB_REQUEST_CONTEXT               Request,
    __in_bcount_opt(*InBufferSize) PUCHAR   InBuffer,
    __in PULONG                             InBufferSize
)
{
    PNDIS_WWAN_SET_DEVICE_BINDINGS_INFO NdisSetDeviceBindingsInfo = (PNDIS_WWAN_SET_DEVICE_BINDINGS_INFO)InBuffer;
    PMBB_DEVICE_BINDINGS_INFO MbbSetDeviceBindingsInfo = NULL;
    NDIS_STATUS NdisStatus = NDIS_STATUS_SUCCESS;
    ULONG BufferSize = 0;

    NdisStatus = MbbUtilWwanToMbbSetDeviceBindingsInfo(
        &(NdisSetDeviceBindingsInfo->SetDeviceBindingsInfo),
        &MbbSetDeviceBindingsInfo,
        &BufferSize);

    if (NdisStatus != NDIS_STATUS_SUCCESS)
    {
        return NdisStatus;
    }

    Request->HandlerContext.DataToFreeOnCompletion = MbbSetDeviceBindingsInfo;

    return MbbUtilSetAttributeWithParameter(
        Request,
        (PCHAR)MbbSetDeviceBindingsInfo,
        BufferSize);
}

NDIS_STATUS
MbbNdisSetRegisterStateEx(
    __in PMBB_REQUEST_CONTEXT               Request,
    __in_bcount_opt(*InBufferSize) PUCHAR   InBuffer,
    __in PULONG                             InBufferSize
)
{
    PNDIS_WWAN_SET_REGISTER_STATE_EX NdisSetRegisterStateEx = (PNDIS_WWAN_SET_REGISTER_STATE_EX)InBuffer;
    PMBB_SET_REGISTER_STATE_V2  MbbSetRegisterStateV2 = NULL;
    NDIS_STATUS NdisStatus = NDIS_STATUS_SUCCESS;
    ULONG BufferSize = 0;

    NdisStatus = MbbUtilWwanToMbbSetRegisterStateEx(
        &(NdisSetRegisterStateEx->SetRegisterState),
        &MbbSetRegisterStateV2,
        &BufferSize);

    if (NdisStatus != NDIS_STATUS_SUCCESS)
    {
        return NdisStatus;
    }

    Request->HandlerContext.DataToFreeOnCompletion = MbbSetRegisterStateV2;

    return MbbUtilSetAttributeWithParameter(
        Request,
        (PCHAR)MbbSetRegisterStateV2,
        BufferSize);
}

NDIS_STATUS
MbbNdisSetImsVoiceState(
    __in PMBB_REQUEST_CONTEXT               Request,
    __in_bcount_opt(*InBufferSize) PUCHAR   InBuffer,
    __in PULONG                             InBufferSize
)
{
    PNDIS_WWAN_SET_IMS_VOICE_STATE NdisSetImsVoiceState = (PNDIS_WWAN_SET_IMS_VOICE_STATE)InBuffer;
    PMBB_SET_IMS_VOICE_STATE MbbSetImsVoiceState = NULL;
    ULONG BufferSize = sizeof(MBB_SET_IMS_VOICE_STATE);

    MbbSetImsVoiceState = ALLOCATE_NONPAGED_POOL(BufferSize);
    if (NULL == MbbSetImsVoiceState)
    {
        return NDIS_STATUS_RESOURCES;
    }
    Request->HandlerContext.DataToFreeOnCompletion = MbbSetImsVoiceState;

    RtlZeroMemory(MbbSetImsVoiceState, BufferSize);

    MbbSetImsVoiceState->ImsVoiceStatus = (MBB_IMS_VOICE_STATUS)(NdisSetImsVoiceState->SetImsVoiceState.ImsVoiceStatus);

    return MbbUtilQueryAttributeWithParameter(
        Request,
        (PUCHAR)MbbSetImsVoiceState,
        BufferSize
        );
}

NDIS_STATUS
MbbNdisSetSignalStateEx(
    __in PMBB_REQUEST_CONTEXT               Request,
    __in_bcount_opt(*InBufferSize) PUCHAR   InBuffer,
    __in PULONG                             InBufferSize
)
{
    PNDIS_WWAN_SET_SIGNAL_INDICATION_EX NdisSetSignalIndicationEx = (PNDIS_WWAN_SET_SIGNAL_INDICATION_EX)InBuffer;
    PMBB_SET_SIGNAL_INDICATION_V2 MbbSetSignalIndicationV2 = NULL;
    ULONG BufferSize = sizeof(MBB_SET_SIGNAL_INDICATION_V2);

    MbbSetSignalIndicationV2 = ALLOCATE_NONPAGED_POOL(BufferSize);
    if (NULL == MbbSetSignalIndicationV2)
    {
        return NDIS_STATUS_RESOURCES;
    }
    Request->HandlerContext.DataToFreeOnCompletion = MbbSetSignalIndicationV2;

    RtlZeroMemory(MbbSetSignalIndicationV2, BufferSize);

    RtlCopyMemory(MbbSetSignalIndicationV2, &(NdisSetSignalIndicationEx->SignalIndication), BufferSize);

    return MbbUtilQueryAttributeWithParameter(
        Request,
        (PUCHAR)MbbSetSignalIndicationV2,
        BufferSize
        );
}

NDIS_STATUS
MbbNdisSetLocationState(
    __in PMBB_REQUEST_CONTEXT               Request,
    __in_bcount_opt(*InBufferSize) PUCHAR   InBuffer,
    __in PULONG                             InBufferSize
)
{
    PNDIS_WWAN_LOCATION_TRIGGER_MODE NdisLocationTriggerMode = (PNDIS_WWAN_LOCATION_TRIGGER_MODE)InBuffer;
    PMBB_SET_LOCATION_STATE MbbSetLocationState = NULL;
    ULONG BufferSize = sizeof(MBB_SET_LOCATION_STATE);

    MbbSetLocationState = ALLOCATE_NONPAGED_POOL(BufferSize);
    if (NULL == MbbSetLocationState)
    {
        return NDIS_STATUS_RESOURCES;
    }
    Request->HandlerContext.DataToFreeOnCompletion = MbbSetLocationState;

    RtlZeroMemory(MbbSetLocationState, BufferSize);

    MbbSetLocationState->Trigger = (MBB_LOCATION_TRIGGER_MODE)(NdisLocationTriggerMode->SetLocationState.Trigger);

    return MbbUtilQueryAttributeWithParameter(
        Request,
        (PUCHAR)MbbSetLocationState,
        BufferSize
        );
}

NDIS_STATUS
MbbNdisSetNetworkIdleHint(
    __in PMBB_REQUEST_CONTEXT                                   Request,
    __in_bcount_opt(*InBufferSize) PUCHAR                       InBuffer,
    __in PULONG                                                 InBufferSize
)
{
    PNDIS_WWAN_NETWORK_IDLE_HINT        NdisNetworkIdleHint = (PNDIS_WWAN_NETWORK_IDLE_HINT)InBuffer;
    PMINIPORT_ADAPTER_CONTEXT           Adapter = MbbReqMgrGetAdapterHandle(Request);
    PNDIS_OID_REQUEST                   OidRequest = Request->OidContext.OidRequest;
    NDIS_PORT_NUMBER                    PortNumber = OidRequest->PortNumber;
    NDIS_STATUS                         ndisStatus = NDIS_STATUS_SUCCESS;

    // Honor this OID Request only if we get the request on the Physical Miniport which is on Port 0

    do
    {
        if (IS_ALLOCATED_PORT_NUMBER(PortNumber))
        {
            TraceError(WMBCLASS_OID, "[MbbNdis][ReqID=0x%04x] OID_WWAN_NETWORK_IDLE_HINT received on port number %lu instead of port number 0. Not handling and returning success\n",
                Request->RequestId,
                PortNumber
                );
            ndisStatus = NDIS_STATUS_SUCCESS;
            break;
        }

        // if we are getting a call from above to put the device into Network Idle mode, then do so
        if (NdisNetworkIdleHint->IdleHint.IsEnabled)
        {
            ndisStatus = MbbAdapterSendNetworkIdleHint(Adapter);
        }

    } while (FALSE);

    return ndisStatus;
}

NDIS_STATUS
MbbNdisSetPreshutdown(
    __in PMBB_REQUEST_CONTEXT                                   Request,
    __in_bcount_opt(*InBufferSize) PUCHAR                       InBuffer,
    __in PULONG                                                 InBufferSize
    )
{
    NDIS_STATUS                         NdisStatus = NDIS_STATUS_SUCCESS;

    PNDIS_WWAN_PRESHUTDOWN_STATE        NdisPreshutdownState = (PNDIS_WWAN_PRESHUTDOWN_STATE)InBuffer;
    PMINIPORT_ADAPTER_CONTEXT           Adapter = MbbReqMgrGetAdapterHandle(Request);
    PNDIS_OID_REQUEST                   OidRequest = Request->OidContext.OidRequest;
    NDIS_PORT_NUMBER                    PortNumber = OidRequest->PortNumber;
    BOOLEAN                             IsPreshutdownCapable = FALSE;

    do
    {
        // Honor this OID Request only if we get the request on the Physical Miniport (i.e. Port 0)
        if (IS_ALLOCATED_PORT_NUMBER(PortNumber))
        {
            TraceError(WMBCLASS_OID, "[MbbNdis][ReqID=0x%04x] OID_WWAN_PRESHUTDOWN received on port number %lu instead of port number 0. Not handling and returning success\n",
                Request->RequestId,
                PortNumber
                );
            NdisStatus = NDIS_STATUS_NOT_SUPPORTED;
            break;
        }

        // Check for device support and bail if not supported
        MbbAdapterLock(Adapter);
        IsPreshutdownCapable = (TRUE == Adapter->AdapterFlags.IsPreshutdownCapable);
        MbbAdapterUnlock(Adapter);

        if (!IsPreshutdownCapable)
        {
            TraceInfo(WMBCLASS_OID, "[MbbNdis][ReqId=0x%04x] OID_WWAN_PRESHUTDOWN - Device does not support preshutdown)",
                Request->RequestId
                );
            NdisStatus = NDIS_STATUS_NOT_SUPPORTED;
            break;
        }

        NdisStatus =  MbbUtilSetAttributeWithParameter(
            Request,
            NULL,
            0);
        if ((NdisStatus != NDIS_STATUS_SUCCESS) &&
            (NdisStatus != NDIS_STATUS_PENDING))
        {
            TraceError(WMBCLASS_OID, "[MbbNdis][ReqID=0x%04x] OID_WWAN_PRESHUTDOWN Device SET failed",
                Request->RequestId
                );
            break;
        }

        TraceLoggingWrite(
            g_hLoggingProvider,
            "OID_Wwan_Preshutdown_Request",
            TraceLoggingUInt64(Request->RequestId, "RequestID"),
            TraceLoggingKeyword(MICROSOFT_KEYWORD_TELEMETRY));

    } while (FALSE);

    return NdisStatus;
}

NDIS_STATUS
MbbNdisSetUiccOpenChannel(
    __in PMBB_REQUEST_CONTEXT               Request,
    __in_bcount_opt(*InBufferSize) PUCHAR   InBuffer,
    __in PULONG                             InBufferSize
    )
{
    PNDIS_WWAN_SET_UICC_OPEN_CHANNEL    NdisSetUiccOpenChannel = (PNDIS_WWAN_SET_UICC_OPEN_CHANNEL)InBuffer;
    PMBB_SET_UICC_OPEN_CHANNEL          MbbSetUiccOpenChannel = NULL;
    NDIS_STATUS                         NdisStatus;
    ULONG                               BufferSize = 0;

    __analysis_assume(*InBufferSize >= sizeof(NDIS_WWAN_SET_UICC_OPEN_CHANNEL));

    NdisStatus = MbbUtilWwanToMbbSetUiccOpenChannel(
        &NdisSetUiccOpenChannel->SetUiccOpenChannel,
        &MbbSetUiccOpenChannel,
        &BufferSize
        );

    if (NdisStatus != NDIS_STATUS_SUCCESS)
    {
        return NdisStatus;
    }

    Request->HandlerContext.DataToFreeOnCompletion = MbbSetUiccOpenChannel;

    return MbbUtilSetAttributeWithParameter(
                Request,
                (PUCHAR) MbbSetUiccOpenChannel,
                BufferSize
                );
}

NDIS_STATUS
MbbNdisSetUiccCloseChannel(
    __in PMBB_REQUEST_CONTEXT               Request,
    __in_bcount_opt(*InBufferSize) PUCHAR   InBuffer,
    __in PULONG                             InBufferSize
    )
{
    PNDIS_WWAN_SET_UICC_CLOSE_CHANNEL   NdisSetUiccCloseChannel = (PNDIS_WWAN_SET_UICC_CLOSE_CHANNEL)InBuffer;
    PMBB_SET_UICC_CLOSE_CHANNEL         MbbSetUiccCloseChannel = NULL;

    __analysis_assume(*InBufferSize >= sizeof(NDIS_WWAN_SET_UICC_CLOSE_CHANNEL));

    if( (MbbSetUiccCloseChannel = (PMBB_SET_UICC_CLOSE_CHANNEL) ALLOCATE_NONPAGED_POOL( sizeof(MBB_SET_UICC_CLOSE_CHANNEL) )) == NULL )
    {
        return NDIS_STATUS_RESOURCES;
    }

    MbbUtilWwanToMbbSetUiccCloseChannel(
        &NdisSetUiccCloseChannel->SetUiccCloseChannel,
        MbbSetUiccCloseChannel
        );

    Request->HandlerContext.DataToFreeOnCompletion = MbbSetUiccCloseChannel;

    return MbbUtilSetAttributeWithParameter(
                Request,
                (PUCHAR) MbbSetUiccCloseChannel,
                sizeof(MBB_SET_UICC_CLOSE_CHANNEL)
                );
}

NDIS_STATUS
MbbNdisSetUiccApdu(
    __in PMBB_REQUEST_CONTEXT               Request,
    __in_bcount_opt(*InBufferSize) PUCHAR   InBuffer,
    __in PULONG                             InBufferSize
    )
{
    PNDIS_WWAN_SET_UICC_APDU    NdisSetUiccApdu = (PNDIS_WWAN_SET_UICC_APDU)InBuffer;
    PMBB_SET_UICC_APDU          MbbSetUiccApdu = NULL;
    NDIS_STATUS                 NdisStatus;
    ULONG                       BufferSize = 0;

    __analysis_assume(*InBufferSize >= sizeof(NDIS_WWAN_SET_UICC_APDU));

    NdisStatus = MbbUtilWwanToMbbSetUiccApdu(
        &NdisSetUiccApdu->SetUiccApdu,
        &MbbSetUiccApdu,
        &BufferSize
        );

    if (NdisStatus != NDIS_STATUS_SUCCESS)
    {
        return NdisStatus;
    }

    Request->HandlerContext.DataToFreeOnCompletion = MbbSetUiccApdu;

    return MbbUtilSetAttributeWithParameter(
                Request,
                (PUCHAR) MbbSetUiccApdu,
                BufferSize
                );
}

NDIS_STATUS
MbbNdisSetUiccTerminalCapability(
    __in PMBB_REQUEST_CONTEXT               Request,
    __in_bcount_opt(*InBufferSize) PUCHAR   InBuffer,
    __in PULONG                             InBufferSize
    )
{
    PNDIS_WWAN_SET_UICC_TERMINAL_CAPABILITY NdisSetUiccTerminalCapability = (PNDIS_WWAN_SET_UICC_TERMINAL_CAPABILITY)InBuffer;
    PMBB_SET_UICC_TERMINAL_CAPABILITY       MbbSetUiccTerminalCapability = NULL;
    NDIS_STATUS                             NdisStatus;
    ULONG                                   BufferSize = 0;

    __analysis_assume(*InBufferSize >= sizeof(NDIS_WWAN_SET_UICC_TERMINAL_CAPABILITY));

    NdisStatus = MbbUtilWwanToMbbSetUiccTerminalCapability(
        &NdisSetUiccTerminalCapability->SetUiccTerminalCapability.CapabilityListHeader,
        &MbbSetUiccTerminalCapability,
        &BufferSize
        );

    if (NdisStatus != NDIS_STATUS_SUCCESS)
    {
        return NdisStatus;
    }

    Request->HandlerContext.DataToFreeOnCompletion = MbbSetUiccTerminalCapability;

    return MbbUtilSetAttributeWithParameter(
                Request,
                (PUCHAR) MbbSetUiccTerminalCapability,
                BufferSize
                );
}

NDIS_STATUS
MbbNdisSetUiccReset(
    _In_ PMBB_REQUEST_CONTEXT                   Request,
    _In_reads_bytes_opt_(*InBufferSize) PUCHAR  InBuffer,
    _In_ PULONG                                 InBufferSize
    )
{
    PNDIS_WWAN_SET_UICC_RESET   NdisSetUiccReset = (PNDIS_WWAN_SET_UICC_RESET)InBuffer;
    PMBB_SET_UICC_RESET         MbbSetUiccReset = NULL;
    PMINIPORT_ADAPTER_CONTEXT   Adapter = MbbReqMgrGetAdapterHandle(Request);

    MbbAdapterLock(Adapter);
    BOOL IsUiccLowLevelCapable = Adapter->AdapterFlags.IsUiccLowLevelCapable;
    MbbAdapterUnlock(Adapter);

    if (!IsUiccLowLevelCapable)
    {
        TraceInfo(WMBCLASS_OID, "[MbbAdapter] device does not support UICC low level operations (SET_UICC_RESET)");
        return NDIS_STATUS_NOT_SUPPORTED;
    }

    __analysis_assume(*InBufferSize >= sizeof(NDIS_WWAN_SET_UICC_RESET));

    if( (MbbSetUiccReset = (PMBB_SET_UICC_RESET) ALLOCATE_NONPAGED_POOL( sizeof(MBB_SET_UICC_RESET) )) == NULL )
    {
        return NDIS_STATUS_RESOURCES;
    }

    MbbSetUiccReset->PassThroughAction = (MBB_UICC_PASSTHROUGH_ACTION)NdisSetUiccReset->SetUiccReset.PassThroughAction;

    Request->HandlerContext.DataToFreeOnCompletion = MbbSetUiccReset;

    return MbbUtilSetAttributeWithParameter(
                Request,
                (PUCHAR) MbbSetUiccReset,
                sizeof(MBB_SET_UICC_RESET)
                );
}

// SAR backoff set
NDIS_STATUS
MbbNdisSetSarConfig(
    _In_ PMBB_REQUEST_CONTEXT                    Request,
    _In_reads_bytes_opt_(*InBufferSize) PUCHAR   InBuffer,
    _In_ PULONG                                  InBufferSize
    )
{
    PMBB_SET_SAR_CONFIG        MbbSetSarConfig = NULL;
    PNDIS_WWAN_SET_SAR_CONFIG  NdisSetSarConfig = (PNDIS_WWAN_SET_SAR_CONFIG)InBuffer;
    NDIS_STATUS                NdisStatus = NDIS_STATUS_SUCCESS;
    ULONG                      BufferSize = 0;
    PMINIPORT_ADAPTER_CONTEXT  Adapter = Request->RequestManager->AdapterContext;

    MbbAdapterLock(Adapter);
    BOOL IsSARCapable = Adapter->AdapterFlags.IsSARCapable;
    MbbAdapterUnlock(Adapter);

    if (!IsSARCapable)
    {
        TraceInfo(WMBCLASS_INIT, "[MbbAdapter] device does not support SAR control service");
        return NDIS_STATUS_NOT_SUPPORTED;
    }

    NdisStatus = MbbUtilWwanToMbbSetSarConfig(
        &(NdisSetSarConfig->SetSarConfig),
        &MbbSetSarConfig,
        &BufferSize);

    if (NdisStatus != NDIS_STATUS_SUCCESS)
    {
        return NdisStatus;
    }

    Request->HandlerContext.DataToFreeOnCompletion = MbbSetSarConfig;

    return MbbUtilSetAttributeWithParameter(
        Request,
        (PCHAR)MbbSetSarConfig,
        BufferSize);
}

NDIS_STATUS
MbbNdisSetSarTransmissionStatus(
    _In_ PMBB_REQUEST_CONTEXT                    Request,
    _In_reads_bytes_opt_(*InBufferSize) PUCHAR   InBuffer,
    _In_ PULONG                                  InBufferSize
    )
{
    PMBB_SET_SAR_TRANSMISSION_STATUS        MbbSetSarTransmissionStatus = NULL;
    PNDIS_WWAN_SET_SAR_TRANSMISSION_STATUS  NdisSetSarTransmissionStatus = (PNDIS_WWAN_SET_SAR_TRANSMISSION_STATUS)InBuffer;
    ULONG                                   BufferSize = sizeof(MBB_SET_SAR_TRANSMISSION_STATUS);

    MbbSetSarTransmissionStatus = ALLOCATE_NONPAGED_POOL(BufferSize);
    if (NULL == MbbSetSarTransmissionStatus)
    {
        return NDIS_STATUS_RESOURCES;
    }

    Request->HandlerContext.DataToFreeOnCompletion = MbbSetSarTransmissionStatus;

    RtlZeroMemory(MbbSetSarTransmissionStatus, BufferSize);

    MbbSetSarTransmissionStatus->ChannelNotification =
        (MBB_SAR_TRANSMISSION_STATUS_NOTIFICATION_STATE)(NdisSetSarTransmissionStatus->SetTransmissionStatus.ChannelNotification);
    MbbSetSarTransmissionStatus->HysteresisTimer = NdisSetSarTransmissionStatus->SetTransmissionStatus.HysteresisTimer;

    return MbbUtilSetAttributeWithParameter(
        Request,
        (PCHAR)MbbSetSarTransmissionStatus,
        BufferSize);
}

NDIS_STATUS
MbbNdisSetNetworkBlacklist(
    __in PMBB_REQUEST_CONTEXT               Request,
    __in_bcount_opt(*InBufferSize) PUCHAR   InBuffer,
    __in PULONG                             InBufferSize
)
{
    PMBB_MS_NETWORK_BLACKLIST_INFO      MbbNetworkBlacklist = NULL;
    PNDIS_WWAN_SET_NETWORK_BLACKLIST    NdisSetNetworkBlacklist = (PNDIS_WWAN_SET_NETWORK_BLACKLIST)InBuffer;
    ULONG                               BufferSize = 0;
    NDIS_STATUS                         Status;
    PMINIPORT_ADAPTER_CONTEXT           Adapter = Request->RequestManager->AdapterContext;

    MbbAdapterLock(Adapter);
    BOOL IsNetworkBlacklistCapable = Adapter->AdapterFlags.IsNetworkBlacklistCapable;
    MbbAdapterUnlock(Adapter);

    if (!IsNetworkBlacklistCapable)
    {
        TraceInfo(WMBCLASS_INIT, "[MbbAdapter] device does not support network blacklist");
        return NDIS_STATUS_NOT_SUPPORTED;
    }

    //
    // Size is already verified in MbbNdisMiniportOidRequest( )
    //
    __analysis_assume(*InBufferSize >= sizeof(NDIS_WWAN_SET_NETWORK_BLACKLIST));

    *InBufferSize = sizeof(NDIS_WWAN_SET_NETWORK_BLACKLIST);

    Status = MbbUtilWwanToMbbSetNetworkBlacklist(
        &NdisSetNetworkBlacklist->SetNetworkBlacklist,
        &MbbNetworkBlacklist,
        &BufferSize
    );

    if (!NT_SUCCESS(Status))
    {
        return Status;
    }

    Request->HandlerContext.DataToFreeOnCompletion = MbbNetworkBlacklist;

    return MbbUtilSetAttributeWithParameter(
        Request,
        (PCHAR)MbbNetworkBlacklist,
        BufferSize
    );
}

NDIS_STATUS
MbbNdisSetDeviceReset(
    _In_ PMBB_REQUEST_CONTEXT                   Request,
    _In_reads_bytes_opt_(*InBufferSize) PUCHAR  InBuffer,
    _In_ PULONG                                 InBufferSize
    )
{
    PMINIPORT_ADAPTER_CONTEXT   Adapter = MbbReqMgrGetAdapterHandle(Request);

    UNREFERENCED_PARAMETER(InBuffer);

    MbbAdapterLock(Adapter);
    BOOL IsDeviceResetCapable = Adapter->AdapterFlags.IsDeviceResetCapable;
    MbbAdapterUnlock(Adapter);

    if (!IsDeviceResetCapable)
    {
        TraceInfo(WMBCLASS_OID, "[MbbAdapter] device does not support device reset");
        return NDIS_STATUS_NOT_SUPPORTED;
    }

    __analysis_assume(*InBufferSize >= sizeof(NDIS_WWAN_SET_DEVICE_RESET));

    return MbbUtilSetAttributeWithParameter(
                Request,
                NULL,
                0
                );
}


//
// Status Handlers
//

/*++
General handling of responses \ indications:
    If the response \ indication couldnt be retrieved at all i.e.
    NdisStatus != NDIS_STATUS_SUCCESS then bail out immediately.

    If the operational status is unsuccessful i.e. MbbStatus !=
    MBB_STATUS_SUCCESS the device may or may not have returned
    additional data. In case it has propagate that data to svc.
    If not then preserve the device returned failure code.

Parameters
    NdisStatus
        Conveys whether the response was successfully retrieved.

    MbbStatus
        The MBIM specification based operational status that the device returned.
--*/

VOID
MbbNdisIndicateDeviceCaps(
    __in PMBB_REQUEST_CONTEXT   Request,
    __in NDIS_STATUS            NdisStatus
    )
{
    BOOLEAN                     IndicateFailure = TRUE;
    ULONG                       NdisDeviceCapsSize = Request->HandlerContext.Parameters.DeviceCaps.NdisDeviceCapsSize;
    PMINIPORT_ADAPTER_CONTEXT   Adapter;
    PNDIS_WWAN_DEVICE_CAPS      NdisDeviceCaps = Request->HandlerContext.Parameters.DeviceCaps.NdisDeviceCaps;
    NDIS_WWAN_DEVICE_CAPS       LocalNdisDeviceCaps;
    NDIS_STATUS_INDICATION      StatusIndication;

    do
    {
        MBB_INITIALIZE_REQUEST_NDIS_STATUS_INDICATION(
            &StatusIndication,
            Request,
            NDIS_STATUS_WWAN_DEVICE_CAPS
            );

        Adapter = (PMINIPORT_ADAPTER_CONTEXT)MbbReqMgrGetAdapterHandle( Request );
        //
        // If the CID response wasnt receieved then bail out.
        //
        if( NDIS_STATUS_SUCCESS != NdisStatus ||
            NULL == NdisDeviceCaps )
        {
            break;
        }
        //
        // Translate device services to caps.
        //
        if( Request->HandlerContext.Parameters.DeviceCaps.IsUssdCapsValid == 0 ||
            Request->HandlerContext.Parameters.DeviceCaps.IsAuthCapsValid == 0 )
        {
            NdisStatus = MbbAdapterFWDeviceServicesToCapabilities(
                            Adapter,
                            Request
                            );
            if( NDIS_STATUS_PENDING == NdisStatus )
            {
                //
                // The status handler will take care of the rest.
                //
                NdisDeviceCaps = NULL;
                IndicateFailure = FALSE;
                break;
            }
            else if( NDIS_STATUS_SUCCESS != NdisStatus )
            {
                break;
            }
        }
        //
        // All information is available. Indicate status.
        //
        NdisDeviceCaps->DeviceCaps.WwanAuthAlgoCaps = WWAN_AUTH_ALGO_CAPS_NONE;
        if( Adapter->AdapterFlags.IsUssdCapable == TRUE )
            NdisDeviceCaps->DeviceCaps.WwanControlCaps |= WWAN_CTRL_CAPS_USSD;
        if( Adapter->AdapterFlags.IsAkaAuthCapable == TRUE )
            NdisDeviceCaps->DeviceCaps.WwanAuthAlgoCaps |= WWAN_AUTH_ALGO_CAPS_AKA;
        if( Adapter->AdapterFlags.IsAkapAuthCapable == TRUE )
            NdisDeviceCaps->DeviceCaps.WwanAuthAlgoCaps |= WWAN_AUTH_ALGO_CAPS_AKAPRIME;
        if( Adapter->AdapterFlags.IsSimAuthCapable == TRUE )
            NdisDeviceCaps->DeviceCaps.WwanAuthAlgoCaps |= WWAN_AUTH_ALGO_CAPS_SIM;
        //
        // By default set this flag. When ready info comes later,
        // MBCD will send a ReadyInfoFlag indication clearing the flag
        // if its not set. The flag is set by default for buggy apps that
        // may be caching the device caps. To prevent buggy apps from
        // violating ProtectId laws this conservative approach is taken.
        //
        if( Adapter->AdapterFlags.IsShowIMSI == FALSE )
            NdisDeviceCaps->DeviceCaps.WwanControlCaps |= WWAN_CTRL_CAPS_PROTECT_UNIQUEID;

        NdisDeviceCaps->Header.Type         = NDIS_OBJECT_TYPE_DEFAULT;
        NdisDeviceCaps->Header.Size         = (USHORT)NdisDeviceCapsSize;
        NdisDeviceCaps->Header.Revision     = NDIS_WWAN_DEVICE_CAPS_REVISION_2;
        NdisDeviceCaps->uStatus             = WWAN_STATUS_SUCCESS;

        StatusIndication.StatusBuffer       = NdisDeviceCaps;
        StatusIndication.StatusBufferSize   = NdisDeviceCapsSize;

        TraceInfo( WMBCLASS_OID, "[MbbNdis][ReqId=0x%04x] Indicating NDIS_STATUS_WWAN_DEVICE_CAPS size=%d", Request->RequestId, NdisDeviceCapsSize );

        MbbNdisMiniportIndicateRequestStatus(
            Request,
            &StatusIndication
            );
        IndicateFailure = FALSE;
    }
    while( FALSE );

    if( IndicateFailure )
    {
        RtlZeroMemory( &LocalNdisDeviceCaps, sizeof(LocalNdisDeviceCaps) );

        LocalNdisDeviceCaps.Header.Type     = NDIS_OBJECT_TYPE_DEFAULT;
        LocalNdisDeviceCaps.Header.Size     = sizeof(LocalNdisDeviceCaps);
        LocalNdisDeviceCaps.Header.Revision = NDIS_WWAN_DEVICE_CAPS_REVISION_2;
        LocalNdisDeviceCaps.uStatus         = NdisStatus;

        StatusIndication.StatusBuffer       = &LocalNdisDeviceCaps;
        StatusIndication.StatusBufferSize   = sizeof(LocalNdisDeviceCaps);

        TraceInfo( WMBCLASS_OID, "[MbbNdis][ReqId=0x%04x] FAILED to indicate NDIS_STATUS_WWAN_DEVICE_CAPS WwanStatus=%!WwanStatus!", Request->RequestId, NdisStatus );

        MbbNdisMiniportIndicateRequestStatus(
            Request,
            &StatusIndication
            );
    }

    if( NdisDeviceCaps != NULL )
    {
        FREE_POOL( NdisDeviceCaps );
    }
}

NDIS_STATUS
MbbNdisDeviceCapsStatusHandler(
    __in PMBB_REQUEST_CONTEXT               Request,
    __in NDIS_STATUS                        NdisStatus,
    __in MBB_STATUS                         MbbStatus,
    __in_bcount_opt(InBufferSize) PUCHAR    InBuffer,
    __in ULONG                              InBufferSize
    )
{
    WWAN_STATUS                 WwanStatus = WWAN_STATUS_FAILURE;
    PMBB_DEVICE_CAPS            MbbDeviceCaps = (PMBB_DEVICE_CAPS)InBuffer;
    PMINIPORT_ADAPTER_CONTEXT   Adapter = NULL;
    ULONG                       i = 0;
    NDIS_WWAN_DEVICE_CAPS       LocalNdisDeviceCaps;
    PNDIS_WWAN_DEVICE_CAPS      NdisDeviceCaps = NULL;
    USHORT                      NdisDeviceCapsSize = (USHORT)sizeof(NDIS_WWAN_DEVICE_CAPS);
    BOOL                        fMultimode = FALSE;

    do
    {
        Adapter = (PMINIPORT_ADAPTER_CONTEXT)MbbReqMgrGetAdapterHandle(Request);

        MbbAdapterResetCapabilities( Adapter );

        if( NdisStatus != NDIS_STATUS_SUCCESS )
        {
            TraceError( WMBCLASS_INIT, "[MbbNdis][ReqId=0x%04x] FAILED to get data for NDIS_STATUS_WWAN_DEVICE_CAPS, status=%!STATUS!",
                        Request->RequestId,
                        NdisStatus
                        );
            WwanStatus = WWAN_STATUS_FAILURE;
            break;
        }

        WwanStatus = MbbUtilMbbToWwanStatus( MbbStatus );

        if( WwanStatus != WWAN_STATUS_SUCCESS)
        {
            TraceError( WMBCLASS_OID, "[MbbNdis][ReqId=0x%04x] WwanStatus from device not success, status=%!STATUS!",
                        Request->RequestId,
                        WwanStatus
                        );

            break;
        }


        if( MbbDeviceCaps == NULL ||
            InBufferSize < (ULONG)FIELD_OFFSET(MBB_DEVICE_CAPS, DataBuffer) )
        {
            TraceError( WMBCLASS_INIT, "[MbbNdis][ReqId=0x%04x] INSUFFICIENT BufferSize for NDIS_STATUS_WWAN_DEVICE_CAPS, BufferSize[Received=%d Expected=%d]",
                        Request->RequestId,
                        InBufferSize,
                        (ULONG)FIELD_OFFSET(MBB_DEVICE_CAPS, DataBuffer)
                        );
            WwanStatus = WWAN_STATUS_FAILURE;
            break;
        }

        NdisStatus=MbbIsVariableFieldValid(
            InBufferSize,
            MbbDeviceCaps->CustomDataClass.Offset,
            MbbDeviceCaps->CustomDataClass.Size,
            MBB_MAXIMUM_DATA_CLASS_NAME_LENGTH,
            sizeof(WCHAR)
            );

        if (NdisStatus != NDIS_STATUS_SUCCESS)
        {
            TraceError( WMBCLASS_INIT, "[MbbNdis][ReqId=0x%04x] Bad custom data field, Offset=%d, Size=%d",
                        Request->RequestId,
                        MbbDeviceCaps->CustomDataClass.Offset,
                        MbbDeviceCaps->CustomDataClass.Size
                        );
            WwanStatus = WWAN_STATUS_FAILURE;
            break;

        }

        NdisStatus=MbbIsVariableFieldValid(
            InBufferSize,
            MbbDeviceCaps->DeviceIdString.Offset,
            MbbDeviceCaps->DeviceIdString.Size,
            MBB_MAXIMUM_DEVICE_ID_LENGTH,
            sizeof(WCHAR)
            );

        if (NdisStatus != NDIS_STATUS_SUCCESS)
        {
            TraceError( WMBCLASS_INIT, "[MbbNdis][ReqId=0x%04x] DeviceId data field, Offset=%d, Size=%d",
                        Request->RequestId,
                        MbbDeviceCaps->DeviceIdString.Offset,
                        MbbDeviceCaps->DeviceIdString.Size
                        );
            WwanStatus = WWAN_STATUS_FAILURE;
            break;

        }

        NdisStatus=MbbIsVariableFieldValid(
            InBufferSize,
            MbbDeviceCaps->FirmwareInfo.Offset,
            MbbDeviceCaps->FirmwareInfo.Size,
            MBB_MAXIMUM_FIRMWARE_INFO_LENGTH,
            sizeof(WCHAR)
            );

        if (NdisStatus != NDIS_STATUS_SUCCESS)
        {
            TraceError( WMBCLASS_INIT, "[MbbNdis][ReqId=0x%04x] FirmwareInfo data field, Offset=%d, Size=%d",
                        Request->RequestId,
                        MbbDeviceCaps->FirmwareInfo.Offset,
                        MbbDeviceCaps->FirmwareInfo.Size
                        );
            WwanStatus = WWAN_STATUS_FAILURE;
            break;

        }

        MbbAdapterSetSupportedCellularClass( Adapter, MbbDeviceCaps->CellularClass );
        MbbAdapterSetMaxActivatedContexts(Adapter,MbbDeviceCaps->dwMaxSessions);
        fMultimode = MbbAdapterIsMultimodeCapable( Adapter );

        //
        // Remember whether this adapter is multi-carrier capable and also the data class.
        // This information is used in reporting home and preferred and visible provider.
        //
        if( MbbDeviceCaps->ControlCaps & MbbControlCapsMultiCarrier )
            MbbAdapterSetMultiCarrierCapable( Adapter, TRUE );

        MbbAdapterSetDataClass( Adapter, MbbDeviceCaps->DataClass );

        if(!fMultimode)
        {
            MbbAdapterSetCurrentCellularClass( Adapter, MbbDeviceCaps->CellularClass );
        }

        //
        // If this is an internal request then cache the device id and exit out.
        // Internal query is generated from the miniport initialize path to query
        // the device id.
        //
        if( MbbReqMgrIsInternalRequest( Request ) )
        {
            MbbUtilPopulateStaticString(
                Request->HandlerContext.Parameters.DeviceCaps.DeviceId,
                MbbDeviceCaps,
                DeviceIdString
                );
            //
            // If this is a test device the DeviceId may  be absent.
            // In this case fabricate an IMEI for GSM or ESN for CDMA.
            //
            if( Request->HandlerContext.Parameters.DeviceCaps.DeviceId[0] == 0 )
            {
                if( fMultimode ||
                    MbbDeviceCaps->CellularClass == MbbCellularClassGsm )
                {
                    RtlStringCbCopyW(
                        Request->HandlerContext.Parameters.DeviceCaps.DeviceId,
                        sizeof(Request->HandlerContext.Parameters.DeviceCaps.DeviceId),
                        MBB_PSEUDO_IMEI
                        );
                }
                else
                if( MbbDeviceCaps->CellularClass == MbbCellularClassCdma )
                {
                    RtlStringCbCopyW(
                        Request->HandlerContext.Parameters.DeviceCaps.DeviceId,
                        sizeof(Request->HandlerContext.Parameters.DeviceCaps.DeviceId),
                        MBB_PSEUDO_ESN
                        );
                }
                else
                {
                    TraceError( WMBCLASS_INIT, "[MbbNdis][ReqId=0x%04x] FAILED to get DeviceId, invalid CellularClass=%!MbbCellularClass!",
                                Request->RequestId,
                                MbbDeviceCaps->CellularClass
                                );
                    return NDIS_STATUS_FAILURE;
                }
            }
            return NDIS_STATUS_SUCCESS;
        }

        //calculate variable buffer size for multimode devices
        if(fMultimode)
        {
            //for now we support only GSM and CDMA so keep it simple
            NdisDeviceCapsSize+= sizeof(WWAN_CELLULAR_CLASS) * (MbbCellularClassMaximum - 1);
        }

        if( (NdisDeviceCaps = ALLOCATE_NONPAGED_POOL(NdisDeviceCapsSize)) == NULL )
        {
            TraceError( WMBCLASS_INIT, "[MbbNdis][ReqId=0x%04x] FAILED to allocate %d bytes for NDIS_WWAN_DEVICE_CAPS",
                        Request->RequestId,
                        NdisDeviceCapsSize
                        );
            WwanStatus = WWAN_STATUS_FAILURE;
            break;
        }

        MbbUtilMbbToWwanDeviceCaps(
            MbbDeviceCaps,
            Adapter->BusParams.Manufacturer,
            MAX_PARAMETER_STRING,
            Adapter->BusParams.Model,
            MAX_PARAMETER_STRING,
            &NdisDeviceCaps->DeviceCaps,
            fMultimode
            );
    }
    while( FALSE );
    //
    // Restore the device returned operational status, if any.
    //
    if( WwanStatus != WWAN_STATUS_SUCCESS &&
        MbbUtilMbbToWwanStatus( MbbStatus ) != WWAN_STATUS_SUCCESS )
    {
        WwanStatus = MbbUtilMbbToWwanStatus( MbbStatus );
    }
    Request->HandlerContext.Parameters.DeviceCaps.NdisDeviceCaps     = NdisDeviceCaps;
    Request->HandlerContext.Parameters.DeviceCaps.NdisDeviceCapsSize = NdisDeviceCapsSize;

    MbbNdisIndicateDeviceCaps(
        Request,
        WwanStatus
        );

    return NDIS_STATUS_SUCCESS;
}

VOID
MbbNdisIndicateReadyInfoFailure(
    __in PMBB_REQUEST_CONTEXT   Request
    )
{
    NDIS_STATUS_INDICATION  StatusIndication;
    NDIS_WWAN_READY_INFO    LocalNdisReadyInfo;
    PMINIPORT_ADAPTER_CONTEXT Adapter;


    Adapter = (PMINIPORT_ADAPTER_CONTEXT) MbbReqMgrGetAdapterHandle( Request );
    Adapter->AdapterFlags.IsShowIMSI = FALSE;

    RtlZeroMemory(
        &LocalNdisReadyInfo,
        sizeof(LocalNdisReadyInfo)
        );

    LocalNdisReadyInfo.Header.Type          = NDIS_OBJECT_TYPE_DEFAULT;
    LocalNdisReadyInfo.Header.Size          = sizeof(NDIS_WWAN_READY_INFO);
    LocalNdisReadyInfo.Header.Revision      = NDIS_WWAN_READY_INFO_REVISION_1;
    LocalNdisReadyInfo.ReadyInfo.ReadyState = WwanReadyStateFailure;

    MBB_INITIALIZE_REQUEST_NDIS_STATUS_INDICATION(
       &StatusIndication,
        Request,
        NDIS_STATUS_WWAN_READY_INFO
        );

    StatusIndication.StatusBuffer       = &LocalNdisReadyInfo;
    StatusIndication.StatusBufferSize   = sizeof(NDIS_WWAN_READY_INFO);

    TraceError( WMBCLASS_OID, "[MbbNdis][ReqId=0x%04x] READY_STATE_FAILURE", Request->RequestId );

    MbbNdisMiniportIndicateRequestStatus(
        Request,
       &StatusIndication
        );
}

VOID
MbbNdisIndicateReadyInfo(
    __in PMBB_REQUEST_CONTEXT   Request,
    __in NDIS_STATUS            NdisStatus
    )
{
    BOOLEAN                 IndicateReadyInfoFailure = TRUE;
    ULONG                   NdisReadyInfoSize = Request->HandlerContext.Parameters.SubscriberReadyInfo.NdisReadyInfoSize;
    PNDIS_WWAN_READY_INFO   NdisReadyInfo = Request->HandlerContext.Parameters.SubscriberReadyInfo.NdisReadyInfo;

    do
    {
        MBB_COMMAND          NewCommand;
        PMBB_REQUEST_CONTEXT NewRequest;

        //
        // Track whether this is a indication or a response to a query.
        // If this is a response to a query readyinfo then ndis indication needs to
        // happen on any failure. In case of indications ignore failures.
        //
        if( Request->HandlerContext.Parameters.SubscriberReadyInfo.IsFirstCid == 0 )
        {
            Request->HandlerContext.Parameters.SubscriberReadyInfo.IsFirstCid = 1;

            if( memcmp(
                   &Request->HandlerContext.Response.Command.ServiceId,
                   &MBB_UUID_BASIC_CONNECT,
                    sizeof(GUID)
                    ) == 0
                &&
                Request->HandlerContext.Response.Command.CommandId == MBB_BASIC_CID_SUBSCRIBER_READY_INFO
                &&
                MbbReqMgrIsUnsolicitedIndication( Request ) == FALSE )
            {
                Request->HandlerContext.Parameters.SubscriberReadyInfo.IsQueryReadyInfo = 1;
            }
        }
        //
        // If the CID response wasnt receieved then bail out.
        //
        if( NdisStatus != NDIS_STATUS_SUCCESS )
            break;
        //
        // When switching FW in multi-carrier scenario the device
        // indicates readystateoff and then re-enumerates.
        // Thus the class driver will fail to query for emergencymode
        // and smsconfiguration in this scenario and hence fail to
        // indicate readystateoff. To forward the readystateoff
        // indication to service fill in defaults on ReadyStateOff.
        //
        if( NdisReadyInfo != NULL &&
            WwanReadyStateOff == NdisReadyInfo->ReadyInfo.ReadyState )
        {
            if( 0 == Request->HandlerContext.Parameters.SubscriberReadyInfo.IsWwanEmergencyModeValid )
            {
                Request->HandlerContext.Parameters.SubscriberReadyInfo.WwanEmergencyMode        = WwanEmergencyModeOff;
                Request->HandlerContext.Parameters.SubscriberReadyInfo.IsWwanEmergencyModeValid = 1;
            }

            if( 0 == Request->HandlerContext.Parameters.SubscriberReadyInfo.IsCdmaShortMsgSizeValid )
            {
                Request->HandlerContext.Parameters.SubscriberReadyInfo.CdmaShortMsgSize         = WWAN_CDMA_SHORT_MSG_SIZE_UNKNOWN;
                Request->HandlerContext.Parameters.SubscriberReadyInfo.IsCdmaShortMsgSizeValid  = 1;
            }
        }
        //
        // If we have all the information then indicate ready_info.
        //
        if( Request->HandlerContext.Parameters.SubscriberReadyInfo.NdisReadyInfo != NULL &&
            Request->HandlerContext.Parameters.SubscriberReadyInfo.IsCdmaShortMsgSizeValid != 0 &&
            Request->HandlerContext.Parameters.SubscriberReadyInfo.IsWwanEmergencyModeValid != 0 )
        {
            ULONG ReadyInfoFlags;
            PMINIPORT_ADAPTER_CONTEXT Adapter;
            NDIS_STATUS_INDICATION  StatusIndication;

            //
            // Indicate NDIS_STATUS_WWAN_READY_INFO_FLAGS first.
            // This is always a status indication.
            //
            Adapter = (PMINIPORT_ADAPTER_CONTEXT) MbbReqMgrGetAdapterHandle( Request );

            if( 0 == Adapter->AdapterFlags.IsShowIMSI )
                ReadyInfoFlags = WWAN_CTRL_CAPS_PROTECT_UNIQUEID;
            else
                ReadyInfoFlags = 0;

            MBB_UTIL_INITIALIZE_NDIS_STATUS_INDICATION(
                &StatusIndication,
                Adapter->MiniportAdapterHandle,
                NDIS_STATUS_WWAN_RESERVED_2
                );
            StatusIndication.DestinationHandle  = NULL;
            StatusIndication.RequestId          = 0;
            StatusIndication.StatusBuffer       = &ReadyInfoFlags;
            StatusIndication.StatusBufferSize   = sizeof(ULONG);

            TraceInfo( WMBCLASS_OID, "[MbbNdis][ReqId=0x%04x] Indicating NDIS_STATUS_WWAN_READY_INFO_FLAGS", Request->RequestId );

            NdisMIndicateStatusEx(
                Adapter->MiniportAdapterHandle,
                &StatusIndication
                );
            //
            // Now indicate NDIS_STATUS_WWAN_READY_INFO
            //
            NdisReadyInfo->ReadyInfo.EmergencyMode    = Request->HandlerContext.Parameters.SubscriberReadyInfo.WwanEmergencyMode;
            NdisReadyInfo->ReadyInfo.CdmaShortMsgSize = Request->HandlerContext.Parameters.SubscriberReadyInfo.CdmaShortMsgSize;

            MBB_INITIALIZE_REQUEST_NDIS_STATUS_INDICATION(
               &StatusIndication,
                Request,
                NDIS_STATUS_WWAN_READY_INFO
                );

            StatusIndication.StatusBuffer       = NdisReadyInfo;
            StatusIndication.StatusBufferSize   = NdisReadyInfoSize;

            TraceInfo( WMBCLASS_OID, "[MbbNdis][ReqId=0x%04x] Indicating NDIS_STATUS_WWAN_READY_INFO size=%d", Request->RequestId, NdisReadyInfoSize );

            MbbNdisMiniportIndicateRequestStatus(
                Request,
               &StatusIndication
                );
            IndicateReadyInfoFailure = FALSE;
            break;
        }
        //
        // If we dont have any info then only indicate failure if this was a ready_info oid query.
        // Fail the request in this case.
        //
        if( Request->HandlerContext.Parameters.SubscriberReadyInfo.NdisReadyInfo == NULL &&
            Request->HandlerContext.Parameters.SubscriberReadyInfo.IsCdmaShortMsgSizeValid == 0 &&
            Request->HandlerContext.Parameters.SubscriberReadyInfo.IsWwanEmergencyModeValid == 0 )
        {
            if( Request->HandlerContext.Parameters.SubscriberReadyInfo.IsQueryReadyInfo != 1 )
            {
                IndicateReadyInfoFailure = FALSE;
            }
            break;
        }
        //
        // We have something, get the other information.
        // Select the next command to query
        //
        if( Request->HandlerContext.Parameters.SubscriberReadyInfo.NdisReadyInfo == NULL )
        {
            NewCommand.ServiceId = MBB_UUID_BASIC_CONNECT;
            NewCommand.CommandId = MBB_BASIC_CID_SUBSCRIBER_READY_INFO;
        } else
        if( Request->HandlerContext.Parameters.SubscriberReadyInfo.IsWwanEmergencyModeValid == 0 )
        {
            NewCommand.ServiceId = MBB_UUID_BASIC_CONNECT;
            NewCommand.CommandId = MBB_BASIC_CID_EMERGENCY_MODE;
        } else
        if( Request->HandlerContext.Parameters.SubscriberReadyInfo.IsCdmaShortMsgSizeValid == 0 )
        {
            NewCommand.ServiceId = MBB_UUID_SMS;
            NewCommand.CommandId = MBB_SMS_CID_CONFIGURATION;
        }

        if( (NewRequest = MbbReqMgrCreateRequest(
                            Request->RequestManager,
                            NULL,
                            0,
                            &NdisStatus)) == NULL )
        {
            TraceError( WMBCLASS_OID, "[MbbNdis][ReqId=0x%04x] FAILED to allocate Request for retrieving %s",
                        Request->RequestId,
                        MbbUtilGetCommandString( &NewCommand )
                        );
            break;
        }
        NewRequest->OidHandler = MbbNdisGetOidHandlerByCommand( &NewCommand );
        //
        // Cache the context from the previous request since it will be destroyed
        //
        NewRequest->OidContext.OidRequestId     = Request->OidContext.OidRequestId;
        NewRequest->OidContext.OidRequestHandle = Request->OidContext.OidRequestHandle;
        NewRequest->HandlerContext.Parameters.SubscriberReadyInfo = Request->HandlerContext.Parameters.SubscriberReadyInfo;
        //
        // Dispatch the new CID request
        //
        TraceInfo(  WMBCLASS_OID, "[MbbNdis][ReqId=0x%04x] Dispatching Request[ReqId=0x%04x] for %s",
                    Request->RequestId,
                    NewRequest->RequestId,
                    MbbUtilGetCommandString( &NewCommand )
                    );
        NdisStatus = MbbReqMgrDispatchRequest(
                        NewRequest,
                        (NewRequest->OidHandler->IsSerialized == 1),
                        MbbUtilInternalCIDQuery,
                        MbbUtilInternalCIDCompletion,
                        MbbUtilInternalCIDResponse
                        );
        //
        // If dispatch failed, then indicate ReadyInfoFailure
        //
        if( NdisStatus != NDIS_STATUS_PENDING )
        {
            TraceError( WMBCLASS_OID, "[MbbNdis][ReqId=0x%04x] FAILED to dispatch %s, NdisStatus=%!STATUS!",
                        NewRequest->RequestId,
                        MbbUtilGetCommandString( &NewCommand ),
                        NdisStatus
                        );
            MbbReqMgrDestroyRequest(
                NewRequest->RequestManager,
                NewRequest
                );
            break;
        }
        //
        // The status handler will take care of the rest.
        //
        NdisReadyInfo = NULL;
        IndicateReadyInfoFailure = FALSE;
    }
    while( FALSE );

    if( IndicateReadyInfoFailure &&
        Request->HandlerContext.Parameters.SubscriberReadyInfo.IsQueryReadyInfo == 1 )
    {
        MbbNdisIndicateReadyInfoFailure( Request );
    }

    if( NdisReadyInfo != NULL )
        FREE_POOL( NdisReadyInfo );
}

NDIS_STATUS
MbbNdisReadyInfoStatusHandler(
    __in PMBB_REQUEST_CONTEXT               Request,
    __in NDIS_STATUS                        NdisStatus,
    __in MBB_STATUS                         MbbStatus,
    __in_bcount_opt(InBufferSize) PUCHAR    InBuffer,
    __in ULONG                              InBufferSize
    )
{
    WWAN_STATUS                 WwanStatus = WWAN_STATUS_FAILURE;
    ULONG                       ElementIndex;
    ULONG                       ElementCount;
    ULONGLONG                   NdisReadyInfoSize = 0;
    ULONGLONG                   MbbReadyInfoSize;
    PNDIS_WWAN_READY_INFO       NdisReadyInfo = NULL;
    PMBB_SUBSCRIBER_READY_INFO  MbbReadyInfo = (PMBB_SUBSCRIBER_READY_INFO)InBuffer;
    PMINIPORT_ADAPTER_CONTEXT   Adapter;

    //
    // Extract partial ready_info from the response / indication buffer
    //
    do
    {
        Adapter = (PMINIPORT_ADAPTER_CONTEXT) MbbReqMgrGetAdapterHandle( Request );

        if( NdisStatus != NDIS_STATUS_SUCCESS )
        {
            TraceError( WMBCLASS_OID, "[MbbNdis][ReqId=0x%04x] FAILED to get data for NDIS_STATUS_WWAN_READY_INFO, status=%!STATUS!",
                        Request->RequestId,
                        NdisStatus
                        );
            break;
        }

        WwanStatus = MbbUtilMbbToWwanStatus( MbbStatus );

        if( WwanStatus != WWAN_STATUS_SUCCESS )
        {
            TraceError( WMBCLASS_OID, "[MbbNdis][ReqId=0x%04x] WwanStatus from device not success, WwanStatus=%!WwanStatus!. MbbStatus=%!MbbStatus!",
                        Request->RequestId,
                        WwanStatus,
                        MbbStatus
                        );

            NdisStatus=WwanStatus;

            break;
        }


        if( MbbReadyInfo == NULL ||
            InBufferSize < RTL_SIZEOF_THROUGH_FIELD(MBB_SUBSCRIBER_READY_INFO, TelephoneNumberCount) )
        {
            TraceError( WMBCLASS_OID, "[MbbNdis][ReqId=0x%04x] INSUFFICIENT BufferSize for NDIS_STATUS_WWAN_READY_INFO, BufferSize[Received=%d Expected=%d]",
                        Request->RequestId,
                        InBufferSize,
                        RTL_SIZEOF_THROUGH_FIELD(MBB_SUBSCRIBER_READY_INFO, TelephoneNumberCount)
                        );
            NdisStatus = NDIS_STATUS_INVALID_DATA;
            break;
        }


        ElementCount      = MbbReadyInfo->TelephoneNumberCount;

        if (ElementCount > MBB_MAX_READY_INFO_PHONE_NUMBERS)
        {
            TraceWarn( WMBCLASS_OID, "[MbbNdis][ReqId=0x%04x] Too many phone numbers NDIS_STATUS_WWAN_READY_INFO, %d Truncating",
                        Request->RequestId,
                        ElementCount
                        );

            ElementCount = MBB_MAX_READY_INFO_PHONE_NUMBERS;
        }


        MbbReadyInfoSize  = FIELD_OFFSET(MBB_SUBSCRIBER_READY_INFO, TelephoneNumbers) + ((ULONGLONG)ElementCount * sizeof(MBB_STRING));
        NdisReadyInfoSize = sizeof(NDIS_WWAN_READY_INFO) + ((ULONGLONG)ElementCount * WWAN_TN_LEN * sizeof(WCHAR));

        if( InBufferSize < MbbReadyInfoSize )
        {
            TraceError( WMBCLASS_OID, "[MbbNdis][ReqId=0x%04x] INSUFFICIENT BufferSize for NDIS_STATUS_WWAN_READY_INFO, BufferSize[Received=%d Expected=%I64u] TNCount=%d",
                        Request->RequestId,
                        InBufferSize,
                        MbbReadyInfoSize,
                        MbbReadyInfo->TelephoneNumberCount
                        );
            NdisStatus = NDIS_STATUS_INVALID_DATA;
            break;
        }

#ifndef MBIM081d
        if (MbbReadyInfo->SubscriberId.Size > (MBB_MAXIMUM_SUBSCRIBER_ID_LENGTH) * sizeof(WCHAR))
        {
            TraceError( WMBCLASS_OID, "[MbbNdis][ReqId=0x%04x] truncating SubScriberSize from %d  to %d", Request->RequestId, MbbReadyInfo->SubscriberId.Size, (MBB_MAXIMUM_SUBSCRIBER_ID_LENGTH) * sizeof(WCHAR));

            MbbReadyInfo->SubscriberId.Size=(MBB_MAXIMUM_SUBSCRIBER_ID_LENGTH) * sizeof(WCHAR);
        }
#endif

        if( MbbIsVariableFieldValid(
                InBufferSize,
                MbbReadyInfo->SubscriberId.Offset,
                MbbReadyInfo->SubscriberId.Size,
                MBB_MAXIMUM_SUBSCRIBER_ID_LENGTH,
                sizeof(WCHAR)
                ) != STATUS_SUCCESS )
        {
            TraceError( WMBCLASS_OID, "[MbbNdis][ReqId=0x%04x] INVALID MBB_SUBSCRIBER_READY_INFO.SubscriberId Offset[Received=%d BufferSize=%d] Size[Received=%d MaxExpected=%d]",
                        Request->RequestId,
                        MbbReadyInfo->SubscriberId.Offset,
                        InBufferSize,
                        MbbReadyInfo->SubscriberId.Size,
                        MBB_MAXIMUM_SUBSCRIBER_ID_LENGTH * sizeof(WCHAR)
                        );
            NdisStatus = NDIS_STATUS_INVALID_DATA;
            break;
        }

#ifndef MBIM081d
        if (MbbReadyInfo->SimIccId.Size > (MBB_MAXIMUM_SIM_ICC_ID_LENGTH) * sizeof(WCHAR))
        {
            TraceError( WMBCLASS_OID, "[MbbNdis][ReqId=0x%04x] truncating SimIcci from %d  to %d", Request->RequestId, MbbReadyInfo->SimIccId.Size, (MBB_MAXIMUM_SIM_ICC_ID_LENGTH) * sizeof(WCHAR));

            MbbReadyInfo->SimIccId.Size=(MBB_MAXIMUM_SIM_ICC_ID_LENGTH) * sizeof(WCHAR);
        }
#endif

        if( MbbIsVariableFieldValid(
                InBufferSize,
                MbbReadyInfo->SimIccId.Offset,
                MbbReadyInfo->SimIccId.Size,
                MBB_MAXIMUM_SIM_ICC_ID_LENGTH,
                sizeof(WCHAR)
                ) != STATUS_SUCCESS )
        {
            TraceError( WMBCLASS_OID, "[MbbNdis][ReqId=0x%04x] INVALID MBB_SUBSCRIBER_READY_INFO.SimIccId Offset[Received=%d BufferSize=%d] Size[Received=%d MaxExpected=%d]",
                        Request->RequestId,
                        MbbReadyInfo->SimIccId.Offset,
                        InBufferSize,
                        MbbReadyInfo->SimIccId.Size,
                        MBB_MAXIMUM_SIM_ICC_ID_LENGTH * sizeof(WCHAR)
                        );
            NdisStatus = NDIS_STATUS_INVALID_DATA;
            break;
        }

        for( ElementIndex = 0;
             ElementIndex < ElementCount;
             ElementIndex ++ )
        {
            ULONG TelNumLen = Adapter->BusParams.IsErrataDevice ? MBB_MAXIMUM_TELEPHONE_NUMBER_ERRATA_LENGTH : MBB_MAXIMUM_TELEPHONE_NUMBER_LENGTH;

            if( MbbIsVariableFieldValid(
                    InBufferSize,
                    MbbReadyInfo->TelephoneNumbers[ElementIndex].Offset,
                    MbbReadyInfo->TelephoneNumbers[ElementIndex].Size,
                    TelNumLen,
                    sizeof(WCHAR)
                    ) != STATUS_SUCCESS )
            {
                TraceError( WMBCLASS_OID, "[MbbNdis][ReqId=0x%04x] INVALID MBB_SUBSCRIBER_READY_INFO.TelephoneNumber[%d] Offset[Received=%d BufferSize=%d] Size[Received=%d MaxExpected=%d]",
                            Request->RequestId,
                            ElementIndex,
                            MbbReadyInfo->TelephoneNumbers[ElementIndex].Offset,
                            InBufferSize,
                            MbbReadyInfo->TelephoneNumbers[ElementIndex].Size,
                            TelNumLen * sizeof(WCHAR)
                            );
                NdisStatus = NDIS_STATUS_INVALID_DATA;
                break;
            }
        }
        if( NdisStatus != NDIS_STATUS_SUCCESS )
        {
            break;
        }

        if( (NdisReadyInfo = ALLOCATE_NONPAGED_POOL( (SIZE_T)NdisReadyInfoSize )) == NULL )
        {
            TraceError( WMBCLASS_OID, "[MbbNdis][ReqId=0x%04x] FAILED to allocate %I64u bytes for NDIS_STATUS_WWAN_READY_INFO, TNCount=%d",
                        Request->RequestId,
                        NdisReadyInfoSize,
                        MbbReadyInfo->TelephoneNumberCount
                        );
            NdisStatus = NDIS_STATUS_RESOURCES;
            break;
        }
        NdisReadyInfo->Header.Type      = NDIS_OBJECT_TYPE_DEFAULT;
        NdisReadyInfo->Header.Size      = sizeof(NDIS_WWAN_READY_INFO);
        NdisReadyInfo->Header.Revision  = NDIS_WWAN_READY_INFO_REVISION_1;

        Request->HandlerContext.Parameters.SubscriberReadyInfo.NdisReadyInfoSize = (ULONG)NdisReadyInfoSize;
        Request->HandlerContext.Parameters.SubscriberReadyInfo.NdisReadyInfo     = NdisReadyInfo;

        MbbUtilMbbToWwanReadyInfo(
            MbbReadyInfo,
            &NdisReadyInfo->ReadyInfo
            );

        if( (
                MbbReadyInfo->ReadyState == MbbReadyStateInitialized ||
                MbbReadyInfo->ReadyState == MbbReadyStateNotActivated ||
                MbbReadyInfo->ReadyState == MbbReadyStateDeviceLocked
            ) &&
            ! ( MbbReadyInfo->ReadyInfoFlags & MbbReadyInfoFlagsUniqueId ) )
        {
            Adapter->AdapterFlags.IsShowIMSI = TRUE;
        }
        else
        {
            Adapter->AdapterFlags.IsShowIMSI = FALSE;
        }
    }
    while( FALSE );

    MbbNdisIndicateReadyInfo(
        Request,
        NdisStatus
        );

    return NDIS_STATUS_SUCCESS;
}

NDIS_STATUS
MbbNdisEmergencyModeStatusHandler(
    __in PMBB_REQUEST_CONTEXT               Request,
    __in NDIS_STATUS                        NdisStatus,
    __in MBB_STATUS                         MbbStatus,
    __in_bcount_opt(InBufferSize) PUCHAR    InBuffer,
    __in ULONG                              InBufferSize
    )
/*++
    Description:
        This routine does not do a NDIS indication for a CID indication
        because there is no EMERGENCY_MODE ndis indication. EmergencyMode
        is indicated via ReadyInfo. This routine instead does a ReadyInfo
        indication. To do so this routine will query ReadyInfo and SmsConfiguration.
--*/
{
    MBB_EMERGENCY_MODE*     MbbEmergencyMode = (MBB_EMERGENCY_MODE*)InBuffer;

    do
    {
        if( NdisStatus != NDIS_STATUS_SUCCESS )
        {
            TraceError( WMBCLASS_OID, "[MbbNdis][ReqId=0x%04x] FAILED to get data for MBB_BASIC_CID_EMERGENCY_MODE, status=%!STATUS!",
                        Request->RequestId,
                        NdisStatus
                        );
            break;
        }

        if( MbbStatus != MBB_STATUS_SUCCESS )
        {
            TraceError( WMBCLASS_OID, "[MbbNdis][ReqId=0x%04x] FAILED to get data for MBB_BASIC_CID_EMERGENCY_MODE, status=%!MbbStatus!",
                        Request->RequestId,
                        MbbStatus
                        );
            NdisStatus = NDIS_STATUS_FAILURE;
            break;
        }

        if( MbbEmergencyMode == NULL ||
            InBufferSize < sizeof(MBB_EMERGENCY_MODE) )
        {
            TraceError( WMBCLASS_OID, "[MbbNdis][ReqId=0x%04x] INSUFFICIENT BufferSize for MBB_BASIC_CID_EMERGENCY_MODE, BufferSize[Received=%d Expected=%d]",
                        Request->RequestId,
                        InBufferSize,
                        sizeof(MBB_EMERGENCY_MODE)
                        );
            NdisStatus = NDIS_STATUS_INVALID_DATA;
            break;
        }
        TraceInfo(  WMBCLASS_OID, "[MbbNdis][ReqId=0x%04x] EMERGENCY_MODE=%d indication received",
                    Request->RequestId,
                    *MbbEmergencyMode
                    );
        Request->HandlerContext.Parameters.SubscriberReadyInfo.IsWwanEmergencyModeValid = 1;
        Request->HandlerContext.Parameters.SubscriberReadyInfo.WwanEmergencyMode        = (WWAN_EMERGENCY_MODE)*MbbEmergencyMode;
    }
    while( FALSE );
    //
    // Internal requests are created for indicating ready info.
    // Do not fail ready info if emergency mode cannot be queried.
    //
    if( MbbReqMgrIsInternalRequest( Request ) == TRUE ||
        NdisStatus != NDIS_STATUS_SUCCESS )
    {
        Request->HandlerContext.Parameters.SubscriberReadyInfo.IsWwanEmergencyModeValid = 1;
        Request->HandlerContext.Parameters.SubscriberReadyInfo.WwanEmergencyMode        = WwanEmergencyModeOff;
        NdisStatus = NDIS_STATUS_SUCCESS;
    }

    MbbNdisIndicateReadyInfo(
        Request,
        NdisStatus
        );

    return NDIS_STATUS_SUCCESS;
}

NDIS_STATUS
MbbNdisRadioStateStatusHandler(
    __in PMBB_REQUEST_CONTEXT               Request,
    __in NDIS_STATUS                        NdisStatus,
    __in MBB_STATUS                         MbbStatus,
    __in_bcount_opt(InBufferSize) PUCHAR    InBuffer,
    __in ULONG                              InBufferSize
    )
{
    WWAN_STATUS                 WwanStatus = WWAN_STATUS_FAILURE;
    NDIS_STATUS                 HandlerNdisStatus = NDIS_STATUS_SUCCESS;
    NDIS_STATUS_INDICATION      StatusIndication;
    NDIS_WWAN_RADIO_STATE       NdisRadioState;
    PMBB_QUERY_RADIO_STATE      MbbRadioState = (PMBB_QUERY_RADIO_STATE)InBuffer;
    NDIS_STRING                 RadioOffRegString = RTL_CONSTANT_STRING(MBB_REGVAL_RADIO_OFF);
    PMINIPORT_ADAPTER_CONTEXT   Adapter = NULL;

    do
    {
        RtlZeroMemory(&NdisRadioState, sizeof(NdisRadioState));

        Adapter = (PMINIPORT_ADAPTER_CONTEXT)MbbReqMgrGetAdapterHandle(Request);

        if( NdisStatus != NDIS_STATUS_SUCCESS )
        {
            TraceError( WMBCLASS_OID, "[MbbNdis][ReqId=0x%04x] FAILED to get data for NDIS_STATUS_WWAN_RADIO_STATE, status=%!STATUS!",
                        Request->RequestId,
                        NdisStatus
                        );
            break;
        }

        if( (WwanStatus = MbbUtilMbbToWwanStatus( MbbStatus )) != WWAN_STATUS_SUCCESS )
        {
            TraceError( WMBCLASS_OID, "[MbbNdis][ReqId=0x%04x] WwanStatus from device not success, status=%!STATUS!",
                        Request->RequestId,
                        WwanStatus
                        );

            NdisStatus = NDIS_STATUS_FAILURE;
            break;
        }

        if( MbbRadioState == NULL ||
            InBufferSize < sizeof(MBB_QUERY_RADIO_STATE) )
        {
            TraceError( WMBCLASS_OID, "[MbbNdis][ReqId=0x%04x] INSUFFICIENT BufferSize for NDIS_STATUS_WWAN_RADIO_STATE, BufferSize[Received=%d Expected=%d]",
                        Request->RequestId,
                        InBufferSize,
                        sizeof(MBB_QUERY_RADIO_STATE)
                        );
            NdisStatus = NDIS_STATUS_INVALID_DATA;
            WwanStatus = WWAN_STATUS_FAILURE;
            break;
        }

        TraceInfo( WMBCLASS_OID, "[MbbNdis][ReqId=0x%04x] MBIM radio state hw=%d, sw=%d",
             Request->RequestId,
             MbbRadioState->HwRadioState,
             MbbRadioState->SwRadioState
             );


        //
        // We will be sending the radio state notification up.
        // If this is a set requested by the OS and the request
        // succeeded, persist the radio state in registry and locally
        //
        if ((Request->OidContext.OidRequestId != 0) &&
            (MbbReqMgrIsSetOid( Request )) &&
            (WwanStatus == WWAN_STATUS_SUCCESS) )
        {
            //
            // OS initiated SET request that succeeded. The set action
            // was saved in the Parameters portion of the request.
            // Persist it
            //
            if (Request->HandlerContext.Parameters.RadioState.SetAction == WwanRadioOff)
            {
                // Radio OFF was requested & has successfully completed
                TraceInfo(  WMBCLASS_OID, "[MbbNdis][ReqId=0x%04x] Persisting RadioState OFF",
                            Request->RequestId
                            );
                Request->RequestManager->AdapterContext->RadioOff = TRUE;
            }
            else
            {
                // Radio ON was requested & has successfully completed
                TraceInfo(  WMBCLASS_OID, "[MbbNdis][ReqId=0x%04x] Persisting RadioState ON",
                            Request->RequestId
                            );
                Request->RequestManager->AdapterContext->RadioOff = FALSE;
            }

            HandlerNdisStatus = MbbNdisAdapterWriteBoolParameter(
                Adapter,
                Adapter->RadioOff,
                &RadioOffRegString
                );
            if (HandlerNdisStatus != NDIS_STATUS_SUCCESS)
            {
                TraceError( WMBCLASS_OID, "[MbbNdis][ReqId=0x%04x] Failed to persist Radio state in registry, status=%!STATUS!",
                        Request->RequestId,
                        HandlerNdisStatus
                        );
                // We still indicate the current radio state to the OS
            }

        }

        MbbUtilMbbToWwanRadioState(
            &MbbRadioState->HwRadioState,
            &NdisRadioState.RadioState.HwRadioState
            );

        MbbUtilMbbToWwanRadioState(
            &MbbRadioState->SwRadioState,
            &NdisRadioState.RadioState.SwRadioState
            );
    }
    while( FALSE );

    if (NdisStatus != NDIS_STATUS_SUCCESS)
    {
        TraceLoggingWrite(
            g_hLoggingProvider,
            "RadioStateFailure",
            TraceLoggingKeyword(MICROSOFT_KEYWORD_MEASURES));
        PMBB_SEND_QUEUE SendQueue = &((PMINIPORT_ADAPTER_CONTEXT)MbbReqMgrGetAdapterHandle(Request))->SendQueue;
        TryQueueStallState(SendQueue);
    }

    //
    // Restore the device returned operational status, if any.
    //
    if( WwanStatus != WWAN_STATUS_SUCCESS &&
        MbbUtilMbbToWwanStatus( MbbStatus ) != WWAN_STATUS_SUCCESS )
    {
        WwanStatus = MbbUtilMbbToWwanStatus( MbbStatus );
    }

    if( MbbReqMgrIsInternalRequest( Request ) == FALSE ||
        (
            MbbReqMgrIsUnsolicitedIndication( Request ) == TRUE &&
            NdisStatus == NDIS_STATUS_SUCCESS
        ) )
    {
        MBB_INITIALIZE_REQUEST_NDIS_STATUS_INDICATION(
           &StatusIndication,
            Request,
            NDIS_STATUS_WWAN_RADIO_STATE
            );

        NdisRadioState.Header.Type      = NDIS_OBJECT_TYPE_DEFAULT;
        NdisRadioState.Header.Size      = sizeof(NDIS_WWAN_RADIO_STATE);
        NdisRadioState.Header.Revision  = NDIS_WWAN_RADIO_STATE_REVISION_1;
        NdisRadioState.uStatus          = WwanStatus;

        StatusIndication.StatusBuffer       = &NdisRadioState;
        StatusIndication.StatusBufferSize   = sizeof(NDIS_WWAN_RADIO_STATE);

        if ((Request->RequestManager->AdapterContext->RadioOff && (NdisRadioState.RadioState.SwRadioState != WwanRadioOff))
            ||
            (!Request->RequestManager->AdapterContext->RadioOff && (NdisRadioState.RadioState.SwRadioState != WwanRadioOn)))
        {

            TraceError(  WMBCLASS_OID, "[MbbNdis][ReqId=0x%04x] reported radio state does not match it was configured to. config=%!BOOLEAN!, reported=%d",
                Request->RequestId,
                Request->RequestManager->AdapterContext->RadioOff,
                NdisRadioState.RadioState.SwRadioState
                );
        }

        TraceInfo(  WMBCLASS_OID, "[MbbNdis][ReqId=0x%04x] Indicating NDIS_STATUS_WWAN_RADIO_STATE SwState=%u with status=%!WwanStatus!",
                    Request->RequestId,
                    NdisRadioState.RadioState.SwRadioState,
                    WwanStatus
                    );
        MbbNdisMiniportIndicateRequestStatus(
            Request,
            &StatusIndication
            );
    }
    //
    // If this request was submitted internally during adapter init
    // then check if the radio state is set correctly.
    // NOTE: Assumption that only adapter init path submits internal radio state request.
    //
    if( MbbReqMgrIsInternalRequest( Request ) == TRUE &&
        NdisStatus == NDIS_STATUS_SUCCESS )
    {
        // Check if we are in the right state now
        if (Adapter->RadioOff)
        {
            // We configured Software Radio OFF
            if (MbbRadioState->SwRadioState != MbbRadioStateOff)
            {
                TraceError( WMBCLASS_INIT, "[MbbAdapter] Invalid radio state after ConfigureRadioState, Received ON Expected OFF");
                HandlerNdisStatus = NDIS_STATUS_INVALID_STATE;
            }
        }
        else
        {
            // We configured Software Radio ON
            if (MbbRadioState->SwRadioState != MbbRadioStateOn)
            {
                TraceError( WMBCLASS_INIT, "[MbbAdapter] Invalid radio state after ConfigureRadioState, Received OFF Expected ON");
                HandlerNdisStatus = NDIS_STATUS_INVALID_STATE;
            }
        }
    }

    return HandlerNdisStatus;
}

NDIS_STATUS
MbbNdisPinInfoStatusHandler(
    __in PMBB_REQUEST_CONTEXT               Request,
    __in NDIS_STATUS                        NdisStatus,
    __in MBB_STATUS                         MbbStatus,
    __in_bcount_opt(InBufferSize) PUCHAR    InBuffer,
    __in ULONG                              InBufferSize
    )
{
    WWAN_STATUS                 WwanStatus = WWAN_STATUS_FAILURE;
    NDIS_STATUS_INDICATION      StatusIndication;
    NDIS_WWAN_PIN_INFO          NdisPinInfo;
    PMBB_PIN_INFO               MbbPinInfo = (PMBB_PIN_INFO)InBuffer;

    do
    {
        RtlZeroMemory(&NdisPinInfo, sizeof(NdisPinInfo));

        MBB_INITIALIZE_REQUEST_NDIS_STATUS_INDICATION(
            &StatusIndication,
            Request,
            NDIS_STATUS_WWAN_PIN_INFO
            );

        NdisPinInfo.Header.Type      = NDIS_OBJECT_TYPE_DEFAULT;
        NdisPinInfo.Header.Size      = sizeof(NDIS_WWAN_PIN_INFO);
        NdisPinInfo.Header.Revision  = NDIS_WWAN_PIN_INFO_REVISION_1;

        if( NdisStatus != NDIS_STATUS_SUCCESS )
        {
            TraceError( WMBCLASS_OID, "[MbbNdis][ReqId=0x%04x] FAILED to get data for NDIS_STATUS_WWAN_PIN_INFO, status=%!STATUS!",
                        Request->RequestId,
                        NdisStatus
                        );
            break;
        }

        WwanStatus = MbbUtilMbbToWwanStatus( MbbStatus );


        if( MbbPinInfo == NULL ||
            InBufferSize < sizeof(MBB_PIN_INFO) )
        {
            TraceError( WMBCLASS_OID, "[MbbNdis][ReqId=0x%04x] INSUFFICIENT BufferSize for NDIS_STATUS_WWAN_PIN_INFO, BufferSize[Received=%d Expected=%d]",
                        Request->RequestId,
                        InBufferSize,
                        sizeof(MBB_PIN_INFO)
                        );
            WwanStatus = WWAN_STATUS_FAILURE;
            break;
        }

        MbbUtilMbbToWwanPinInfo(
            MbbPinInfo,
            &NdisPinInfo.PinInfo
            );
    }
    while( FALSE );
    //
    // Restore the device returned operational status, if any.
    //
    if( WwanStatus != WWAN_STATUS_SUCCESS &&
        MbbUtilMbbToWwanStatus( MbbStatus ) != WWAN_STATUS_SUCCESS )
    {
        WwanStatus = MbbUtilMbbToWwanStatus( MbbStatus );
    }

    NdisPinInfo.uStatus = WwanStatus;

    StatusIndication.StatusBuffer       = &NdisPinInfo;
    StatusIndication.StatusBufferSize   = sizeof(NDIS_WWAN_PIN_INFO);

    TraceInfo(  WMBCLASS_OID, "[MbbNdis][ReqId=0x%04x] Indicating NDIS_STATUS_WWAN_PIN_INFO with status=%!WwanStatus!",
                Request->RequestId,
                WwanStatus
                );

    MbbNdisMiniportIndicateRequestStatus(
        Request,
        &StatusIndication
        );

    return NDIS_STATUS_SUCCESS;
}

NDIS_STATUS
MbbNdisPinListStatusHandler(
    __in PMBB_REQUEST_CONTEXT               Request,
    __in NDIS_STATUS                        NdisStatus,
    __in MBB_STATUS                         MbbStatus,
    __in_bcount_opt(InBufferSize) PUCHAR    InBuffer,
    __in ULONG                              InBufferSize
    )
{
    WWAN_STATUS                 WwanStatus = WWAN_STATUS_FAILURE;
    NDIS_STATUS_INDICATION      StatusIndication;
    NDIS_WWAN_PIN_LIST          NdisPinList;
    PMBB_PIN_LIST               MbbPinList = (PMBB_PIN_LIST)InBuffer;

    do
    {
        RtlZeroMemory(&NdisPinList, sizeof(NdisPinList));

        MBB_INITIALIZE_REQUEST_NDIS_STATUS_INDICATION(
            &StatusIndication,
            Request,
            NDIS_STATUS_WWAN_PIN_LIST
            );

        NdisPinList.Header.Type      = NDIS_OBJECT_TYPE_DEFAULT;
        NdisPinList.Header.Size      = sizeof(NDIS_WWAN_PIN_LIST);
        NdisPinList.Header.Revision  = NDIS_WWAN_PIN_LIST_REVISION_1;
        NdisPinList.uStatus          = NdisStatus;

        if( NdisStatus != NDIS_STATUS_SUCCESS )
        {
            TraceError( WMBCLASS_OID, "[MbbNdis][ReqId=0x%04x] FAILED to get data for NDIS_STATUS_WWAN_PIN_LIST, status=%!STATUS!",
                        Request->RequestId,
                        NdisStatus
                        );
            break;
        }

        WwanStatus = MbbUtilMbbToWwanStatus( MbbStatus );

        if( WwanStatus != WWAN_STATUS_SUCCESS)
        {
            TraceError( WMBCLASS_OID, "[MbbNdis][ReqId=0x%04x] WwanStatus from device not success, status=%!STATUS!",
                        Request->RequestId,
                        WwanStatus
                        );

            break;
        }


        if( MbbPinList == NULL ||
            InBufferSize < sizeof(MBB_PIN_LIST) )
        {
            TraceError( WMBCLASS_OID, "[MbbNdis][ReqId=0x%04x] INSUFFICIENT BufferSize for NDIS_STATUS_WWAN_PIN_LIST, BufferSize[Received=%d Expected=%d]",
                        Request->RequestId,
                        InBufferSize,
                        sizeof(MBB_PIN_DESCRIPTION)
                        );
            WwanStatus = WWAN_STATUS_FAILURE;
            break;
        }

        MbbUtilMbbToWwanPinList(
            MbbPinList,
            &NdisPinList.PinList
            );
    } while( FALSE );
    //
    // Restore the device returned operational status, if any.
    //
    if( WwanStatus != WWAN_STATUS_SUCCESS &&
        MbbUtilMbbToWwanStatus( MbbStatus ) != WWAN_STATUS_SUCCESS )
    {
        WwanStatus = MbbUtilMbbToWwanStatus( MbbStatus );
    }

    NdisPinList.uStatus = WwanStatus;

    StatusIndication.StatusBuffer       = &NdisPinList;
    StatusIndication.StatusBufferSize   = sizeof(NDIS_WWAN_PIN_LIST);

    TraceInfo(  WMBCLASS_OID, "[MbbNdis][ReqId=0x%04x] Indicating NDIS_STATUS_WWAN_PIN_LIST with status=%!WwanStatus!",
                Request->RequestId,
                WwanStatus
                );
    MbbNdisMiniportIndicateRequestStatus(
        Request,
        &StatusIndication
        );


    return NDIS_STATUS_SUCCESS;
}

NDIS_STATUS
MbbNdisHomeProviderStatusHandler(
    __in PMBB_REQUEST_CONTEXT               Request,
    __in NDIS_STATUS                        NdisStatus,
    __in MBB_STATUS                         MbbStatus,
    __in_bcount_opt(InBufferSize) PUCHAR    InBuffer,
    __in ULONG                              InBufferSize
    )
{
    WWAN_STATUS                 WwanStatus = WWAN_STATUS_FAILURE;
    NDIS_WWAN_HOME_PROVIDER2    NdisHomeProvider2;
    NDIS_STATUS_INDICATION      StatusIndication;
    PMBB_PROVIDER               MbbProvider = (PMBB_PROVIDER)InBuffer;
    PMINIPORT_ADAPTER_CONTEXT   Adapter = MbbReqMgrGetAdapterHandle( Request );
    BOOLEAN                     IsMultiCarrierCapable = MbbAdapterIsMultiCarrierCapable( Adapter );
    ULONG                       AdapterDataClass;

    do
    {
        RtlZeroMemory(&NdisHomeProvider2, sizeof(NdisHomeProvider2));

        MBB_INITIALIZE_REQUEST_NDIS_STATUS_INDICATION(
            &StatusIndication,
            Request,
            MbbReqMgrIsSetOid( Request )? NDIS_STATUS_WWAN_SET_HOME_PROVIDER_COMPLETE : NDIS_STATUS_WWAN_HOME_PROVIDER
            );

        NdisHomeProvider2.Header.Type     = NDIS_OBJECT_TYPE_DEFAULT;
        NdisHomeProvider2.Header.Size     = sizeof(NDIS_WWAN_HOME_PROVIDER2);
        NdisHomeProvider2.Header.Revision = NDIS_WWAN_HOME_PROVIDER_REVISION_2;

        if( NdisStatus != NDIS_STATUS_SUCCESS )
        {
            TraceError( WMBCLASS_OID, "[MbbNdis][ReqId=0x%04x] FAILED to get data for NDIS_STATUS_WWAN_HOME_PROVIDER, status=%!STATUS!",
                        Request->RequestId,
                        NdisStatus
                        );
            break;
        }

        WwanStatus = MbbUtilMbbToWwanStatus( MbbStatus );

        if( WwanStatus != WWAN_STATUS_SUCCESS )
        {
            TraceError( WMBCLASS_OID, "[MbbNdis][ReqId=0x%04x] WwanStatus from device not success, status=%!STATUS!",
                        Request->RequestId,
                        WwanStatus
                        );
            break;
        }

        if( MbbUtilValidateMbbProvider(
                InBufferSize,
                MbbProvider
                ) != NDIS_STATUS_SUCCESS )
        {
            TraceError( WMBCLASS_OID, "[MbbNdis][ReqId=0x%04x] FAILED to validate data for NDIS_STATUS_WWAN_HOME_PROVIDER",
                        Request->RequestId
                        );
            WwanStatus = WWAN_STATUS_FAILURE;
            break;
        }

        MbbAdapterGetDataClass( Adapter, &AdapterDataClass );

        MbbUtilMbbToWwanProvider2(
            MbbProvider,
            AdapterDataClass,
            &NdisHomeProvider2.Provider
            );
    }
    while( FALSE);
    //
    // Restore the device returned operational status, if any.
    //
    if( WwanStatus != WWAN_STATUS_SUCCESS &&
        MbbUtilMbbToWwanStatus( MbbStatus ) != WWAN_STATUS_SUCCESS )
    {
        WwanStatus = MbbUtilMbbToWwanStatus( MbbStatus );
    }

    NdisHomeProvider2.uStatus           = WwanStatus;
    StatusIndication.StatusBuffer       = &NdisHomeProvider2;
    StatusIndication.StatusBufferSize   = sizeof(NDIS_WWAN_HOME_PROVIDER2);

    TraceInfo(  WMBCLASS_OID, "[MbbNdis][ReqId=0x%04x] Indicating NDIS_STATUS_WWAN_HOME_PROVIDER for %s-carrier with status=%!WwanStatus!",
                Request->RequestId,
                IsMultiCarrierCapable? "Multi": "Single",
                WwanStatus
                );

    MbbNdisMiniportIndicateRequestStatus(
        Request,
        &StatusIndication
        );
    return NDIS_STATUS_SUCCESS;
}

NDIS_STATUS
MbbNdisPreferredProviderStatusHandler(
    __in PMBB_REQUEST_CONTEXT               Request,
    __in NDIS_STATUS                        NdisStatus,
    __in MBB_STATUS                         MbbStatus,
    __in_bcount_opt(InBufferSize) PUCHAR    InBuffer,
    __in ULONG                              InBufferSize
    )
{
    WWAN_STATUS                     WwanStatus = WWAN_STATUS_FAILURE;
    ULONG                           AdapterDataClass;
    PMBB_PROVIDER_LIST              MbbPreferredProviders = (PMBB_PROVIDER_LIST)InBuffer;
    NDIS_STATUS_INDICATION          StatusIndication;
    PMINIPORT_ADAPTER_CONTEXT       Adapter = MbbReqMgrGetAdapterHandle( Request );
    BOOLEAN                         IsMultiCarrierCapable = MbbAdapterIsMultiCarrierCapable( Adapter );
    ULONGLONG                       NdisPreferredProvidersSize = 0;
    PNDIS_WWAN_PREFERRED_PROVIDERS  NdisPreferredProviders = NULL;
    NDIS_WWAN_PREFERRED_PROVIDERS   LocalNdisPreferredProviders;
    ULONG                           MulTemp = 0;
    NTSTATUS                        NtStatus = STATUS_SUCCESS;

    do
    {
        RtlZeroMemory(&LocalNdisPreferredProviders, sizeof(LocalNdisPreferredProviders));

        MBB_INITIALIZE_REQUEST_NDIS_STATUS_INDICATION(
            &StatusIndication,
            Request,
            NDIS_STATUS_WWAN_PREFERRED_PROVIDERS
            );

        if( NdisStatus != NDIS_STATUS_SUCCESS )
        {
            TraceError( WMBCLASS_OID, "[MbbNdis][ReqId=0x%04x] FAILED to get data for NDIS_STATUS_WWAN_PREFERRED_PROVIDERS, status=%!STATUS!",
                        Request->RequestId,
                        NdisStatus
                        );
            break;
        }
        //
        // On error there is no data buffer.
        //
        if( (WwanStatus = MbbUtilMbbToWwanStatus( MbbStatus )) != WWAN_STATUS_SUCCESS )
        {
            TraceError( WMBCLASS_OID, "[MbbNdis][ReqId=0x%04x] WwanStatus from device not success, status=%!WwanStatus!",
                        Request->RequestId,
                        WwanStatus
                        );

            break;
        }
        //
        // On set requests the MB Driver Model requires that the preferred provider list is empty.
        //
        if( MbbReqMgrIsSetOid( Request ) == TRUE )
        {
            break;
        }
        //
        // Validate the received data and every provider structure in the list.
        //
        if( MbbUtilValidateMbbProviderList(
                InBufferSize,
                MbbPreferredProviders
                ) != NDIS_STATUS_SUCCESS )
        {
            TraceError( WMBCLASS_OID, "[MbbNdis][ReqId=0x%04x] FAILED to validate MBB_PROVIDER_LIST",
                        Request->RequestId
                        );
            WwanStatus = WWAN_STATUS_FAILURE;
            break;
        }
        //
        // Convert the provider_list to the WWAN format.
        //
        if (MbbPreferredProviders->ProviderCount > MBB_MAX_PROVIDER_LIST_SIZE)
        {
            TraceError( WMBCLASS_OID, "[MbbNdis][ReqId=0x%04x] Provider count exceeds max, MbbPreferredProviders->ProviderCount=%d",
                        Request->RequestId,
                        MbbPreferredProviders->ProviderCount
                        );
            WwanStatus = WWAN_STATUS_FAILURE;
            break;

        }


        NtStatus = RtlULongMult(
            MbbPreferredProviders->ProviderCount,
            sizeof(WWAN_PROVIDER2),
            &MulTemp
            );
        if ( NT_ERROR(NtStatus) )
        {
            TraceError( WMBCLASS_OID, "[MbbNdis][ReqId=0x%04x] Multiplication overflow occured, MbbPreferredProviders->ProviderCount=%d WWAN_PROVIDER2 size=%d",
                        Request->RequestId,
                        MbbPreferredProviders->ProviderCount,
                        sizeof(WWAN_PROVIDER2)
                        );
            WwanStatus = WWAN_STATUS_FAILURE;
            break;
        }

        NdisPreferredProvidersSize  = sizeof(NDIS_WWAN_PREFERRED_PROVIDERS);
        NdisPreferredProvidersSize += MulTemp;

        if( NdisPreferredProvidersSize > ULONG_MAX ||
            (NdisPreferredProviders = (PNDIS_WWAN_PREFERRED_PROVIDERS)ALLOCATE_NONPAGED_POOL( (ULONG)NdisPreferredProvidersSize ) ) == NULL )
        {
            TraceError( WMBCLASS_OID, "[MbbNdis][ReqId=0x%04x] FAILED to allocate %d bytes for NDIS_STATUS_WWAN_PREFERRED_PROVIDERS, ProviderCount=%d",
                        Request->RequestId,
                        (ULONG)NdisPreferredProvidersSize,
                        MbbPreferredProviders->ProviderCount
                        );
            WwanStatus = WWAN_STATUS_FAILURE;
            break;
        }

        NdisPreferredProviders->Header.Type      = NDIS_OBJECT_TYPE_DEFAULT;
        NdisPreferredProviders->Header.Size      = sizeof(NDIS_WWAN_PREFERRED_PROVIDERS);
        NdisPreferredProviders->Header.Revision  = NDIS_WWAN_PREFERRED_PROVIDERS_REVISION_1;

        MbbAdapterGetDataClass( Adapter, &AdapterDataClass );

        MbbUtilMbbToWwanProviderList(
            MbbPreferredProviders,
            AdapterDataClass,
            IsMultiCarrierCapable,
            &NdisPreferredProviders->PreferredListHeader
            );
    }
    while( FALSE );

    if( NdisPreferredProviders == NULL )
    {
        LocalNdisPreferredProviders.Header.Type      = NDIS_OBJECT_TYPE_DEFAULT;
        LocalNdisPreferredProviders.Header.Size      = sizeof(NDIS_WWAN_PREFERRED_PROVIDERS);
        LocalNdisPreferredProviders.Header.Revision  = NDIS_WWAN_PREFERRED_PROVIDERS_REVISION_1;

        LocalNdisPreferredProviders.PreferredListHeader.ElementCount = 0;
        LocalNdisPreferredProviders.PreferredListHeader.ElementType  = WwanStructProvider2;

        StatusIndication.StatusBuffer       = &LocalNdisPreferredProviders;
        StatusIndication.StatusBufferSize   = sizeof(LocalNdisPreferredProviders);
    }
    else
    {
        StatusIndication.StatusBuffer       = NdisPreferredProviders;
        StatusIndication.StatusBufferSize   = (ULONG)NdisPreferredProvidersSize;
    }
    ((PNDIS_WWAN_PREFERRED_PROVIDERS)(StatusIndication.StatusBuffer))->uStatus = WwanStatus;

    TraceInfo(  WMBCLASS_OID, "[MbbNdis][ReqId=0x%04x] Indicating NDIS_STATUS_WWAN_PREFERRED_PROVIDERS with status=%!WwanStatus!",
                Request->RequestId,
                WwanStatus
                );

    MbbNdisMiniportIndicateRequestStatus(
        Request,
        &StatusIndication
        );

    if( NdisPreferredProviders != NULL )
        FREE_POOL( NdisPreferredProviders );

    return NDIS_STATUS_SUCCESS;
}

NDIS_STATUS
MbbNdisVisibleProviderStatusHandler(
    __in PMBB_REQUEST_CONTEXT               Request,
    __in NDIS_STATUS                        NdisStatus,
    __in MBB_STATUS                         MbbStatus,
    __in_bcount_opt(InBufferSize) PUCHAR    InBuffer,
    __in ULONG                              InBufferSize
    )
{
    WWAN_STATUS                     WwanStatus = WWAN_STATUS_FAILURE;
    ULONG                           AdapterDataClass;
    PMBB_PROVIDER_LIST              MbbVisibleProviders = (PMBB_PROVIDER_LIST)InBuffer;
    NDIS_STATUS_INDICATION          StatusIndication;
    PMINIPORT_ADAPTER_CONTEXT       Adapter = MbbReqMgrGetAdapterHandle( Request );
    BOOLEAN                         IsMultiCarrierCapable = MbbAdapterIsMultiCarrierCapable( Adapter );
    ULONGLONG                       NdisVisibleProvidersSize = 0;
    PNDIS_WWAN_VISIBLE_PROVIDERS    NdisVisibleProviders = NULL;
    NDIS_WWAN_VISIBLE_PROVIDERS     LocalNdisVisibleProviders;
    ULONG                           MulTemp = 0;
    NTSTATUS                        NtStatus = STATUS_SUCCESS;

    do
    {
        RtlZeroMemory(&LocalNdisVisibleProviders, sizeof(LocalNdisVisibleProviders));

        MBB_INITIALIZE_REQUEST_NDIS_STATUS_INDICATION(
            &StatusIndication,
            Request,
            NDIS_STATUS_WWAN_VISIBLE_PROVIDERS
            );

        if( NdisStatus != NDIS_STATUS_SUCCESS )
        {
            TraceError( WMBCLASS_OID, "[MbbNdis][ReqId=0x%04x] FAILED to get data for NDIS_STATUS_WWAN_VISIBLE_PROVIDERS, status=%!STATUS!",
                        Request->RequestId,
                        NdisStatus
                        );
            break;
        }
        //
        // On error there is no data buffer.
        //
        if( (WwanStatus = MbbUtilMbbToWwanStatus( MbbStatus )) != WWAN_STATUS_SUCCESS )
        {
            TraceError( WMBCLASS_OID, "[MbbNdis][ReqId=0x%04x] WwanStatus from device not success, status=%!WwanStatus!",
                        Request->RequestId,
                        WwanStatus
                        );

            break;
        }
        //
        // Validate the received data and every provider structure in the list.
        //
        if( MbbUtilValidateMbbProviderList(
                InBufferSize,
                MbbVisibleProviders
                ) != NDIS_STATUS_SUCCESS )
        {
            TraceError( WMBCLASS_OID, "[MbbNdis][ReqId=0x%04x] FAILED to validate MBB_PROVIDER_LIST",
                        Request->RequestId
                        );
            WwanStatus = WWAN_STATUS_FAILURE;
            break;
        }

        //
        // Convert the provider_list to the WWAN format.
        //
        if (MbbVisibleProviders->ProviderCount > MBB_MAX_PROVIDER_LIST_SIZE)
        {
            TraceError( WMBCLASS_OID, "[MbbNdis][ReqId=0x%04x] Provider count exceeds max, MbbPreferredProviders->ProviderCount=%d",
                        Request->RequestId,
                        MbbVisibleProviders->ProviderCount
                        );
            WwanStatus = WWAN_STATUS_FAILURE;
            break;

        }


        //
        // Convert the provider_list to the WWAN format.
        //
        NtStatus = RtlULongMult(
            MbbVisibleProviders->ProviderCount,
            sizeof(WWAN_PROVIDER2),
            &MulTemp
            );
        if ( NT_ERROR(NtStatus) )
        {
            TraceError( WMBCLASS_OID, "[MbbNdis][ReqId=0x%04x] Multiplication overflow occured, MbbVisibleProviders->ProviderCount=%d WWAN_PROVIDER2 size=%d",
                        Request->RequestId,
                        MbbVisibleProviders->ProviderCount,
                        sizeof(WWAN_PROVIDER2)
                        );
            WwanStatus = WWAN_STATUS_FAILURE;
            break;
        }

        NdisVisibleProvidersSize  = sizeof(NDIS_WWAN_VISIBLE_PROVIDERS);
        NdisVisibleProvidersSize += MulTemp;

        if( NdisVisibleProvidersSize > ULONG_MAX ||
            (NdisVisibleProviders = (PNDIS_WWAN_VISIBLE_PROVIDERS)ALLOCATE_NONPAGED_POOL( (ULONG)NdisVisibleProvidersSize )) == NULL )
        {
            TraceError( WMBCLASS_OID, "[MbbNdis][ReqId=0x%04x] FAILED to allocate %d bytes for NDIS_STATUS_WWAN_VISIBLE_PROVIDERS, ProviderCount=%d",
                        Request->RequestId,
                        (ULONG)NdisVisibleProvidersSize,
                        MbbVisibleProviders->ProviderCount
                        );
            WwanStatus = WWAN_STATUS_FAILURE;
            break;
        }

        NdisVisibleProviders->Header.Type      = NDIS_OBJECT_TYPE_DEFAULT;
        NdisVisibleProviders->Header.Size      = sizeof(NDIS_WWAN_VISIBLE_PROVIDERS);
        NdisVisibleProviders->Header.Revision  = NDIS_WWAN_VISIBLE_PROVIDERS_REVISION_1;

        MbbAdapterGetDataClass( Adapter, &AdapterDataClass );

        MbbUtilMbbToWwanProviderList(
            MbbVisibleProviders,
            AdapterDataClass,
            IsMultiCarrierCapable,
            &NdisVisibleProviders->VisibleListHeader
            );
    }
    while( FALSE );

    if( NdisVisibleProviders == NULL )
    {
        LocalNdisVisibleProviders.Header.Type      = NDIS_OBJECT_TYPE_DEFAULT;
        LocalNdisVisibleProviders.Header.Size      = sizeof(NDIS_WWAN_VISIBLE_PROVIDERS);
        LocalNdisVisibleProviders.Header.Revision  = NDIS_WWAN_VISIBLE_PROVIDERS_REVISION_1;

        LocalNdisVisibleProviders.VisibleListHeader.ElementCount = 0;
        LocalNdisVisibleProviders.VisibleListHeader.ElementType  = WwanStructProvider2;

        StatusIndication.StatusBuffer       = &LocalNdisVisibleProviders;
        StatusIndication.StatusBufferSize   = sizeof(LocalNdisVisibleProviders);
    }
    else
    {
        StatusIndication.StatusBuffer       = NdisVisibleProviders;
        StatusIndication.StatusBufferSize   = (ULONG)NdisVisibleProvidersSize;
    }
    ((PNDIS_WWAN_VISIBLE_PROVIDERS)(StatusIndication.StatusBuffer))->uStatus = WwanStatus;

    TraceInfo(  WMBCLASS_OID, "[MbbNdis][ReqId=0x%04x] Indicating NDIS_STATUS_WWAN_VISIBLE_PROVIDERS with status=%!WwanStatus!",
                Request->RequestId,
                WwanStatus
                );

    MbbNdisMiniportIndicateRequestStatus(
        Request,
        &StatusIndication
        );

    if( NdisVisibleProviders != NULL )
        FREE_POOL( NdisVisibleProviders );

    return NDIS_STATUS_SUCCESS;
}

NDIS_STATUS
MbbNdisRegisterStateStatusHandler(
    __in PMBB_REQUEST_CONTEXT               Request,
    __in NDIS_STATUS                        NdisStatus,
    __in MBB_STATUS                         MbbStatus,
    __in_bcount_opt(InBufferSize) PUCHAR    InBuffer,
    __in ULONG                              InBufferSize
    )
{
    PMINIPORT_ADAPTER_CONTEXT       Adapter = (PMINIPORT_ADAPTER_CONTEXT)(Request->RequestManager->AdapterContext);
    WWAN_STATUS                     WwanStatus = WWAN_STATUS_FAILURE;
    NDIS_STATUS_INDICATION          StatusIndication;
    NDIS_WWAN_REGISTRATION_STATE    NdisRegistrationState;
    PMBB_REGISTRATION_STATE         MbbRegistrationState = (PMBB_REGISTRATION_STATE)InBuffer;

    do
    {
        RtlZeroMemory(&NdisRegistrationState,sizeof(NdisRegistrationState));

        MBB_INITIALIZE_REQUEST_NDIS_STATUS_INDICATION(
            &StatusIndication,
            Request,
            NDIS_STATUS_WWAN_REGISTER_STATE
            );

        NdisRegistrationState.Header.Type      = NDIS_OBJECT_TYPE_DEFAULT;
        NdisRegistrationState.Header.Size      = sizeof(NDIS_WWAN_REGISTRATION_STATE);
        NdisRegistrationState.Header.Revision  = NDIS_WWAN_REGISTRATION_STATE_REVISION_2;

        if( NdisStatus != NDIS_STATUS_SUCCESS )
        {
            TraceError( WMBCLASS_OID, "[MbbNdis][ReqId=0x%04x] FAILED to get data for NDIS_STATUS_WWAN_REGISTER_STATE, status=%!STATUS!",
                        Request->RequestId,
                        NdisStatus
                        );
            break;
        }

        WwanStatus = MbbUtilMbbToWwanStatus( MbbStatus );


        if( MbbRegistrationState == NULL ||
            InBufferSize < (ULONG)FIELD_OFFSET(MBB_REGISTRATION_STATE, DataBuffer) )
        {
            TraceError( WMBCLASS_OID, "[MbbNdis][ReqId=0x%04x] INSUFFICIENT BufferSize for NDIS_STATUS_WWAN_REGISTER_STATE, BufferSize[Received=%d Expected=%d]",
                        Request->RequestId,
                        InBufferSize,
                        sizeof(MBB_REGISTRATION_STATE)
                        );
            WwanStatus = WWAN_STATUS_FAILURE;
            NdisStatus = NDIS_STATUS_INVALID_DATA;
            break;
        }

        //
        //  pick this up now so we can return it in the case of an error
        //
        NdisRegistrationState.RegistrationState.uNwError=MbbRegistrationState->NetworkError;

        if ( WwanStatus != WWAN_STATUS_SUCCESS )
        {
            NdisStatus = NDIS_STATUS_FAILURE;

            TraceError( WMBCLASS_OID, "[MbbNdis][ReqId=0x%04x] bad status for NDIS_STATUS_WWAN_REGISTER_STATE, mbbstatus=%!MbbStatus!",
                        Request->RequestId,
                        MbbStatus
                        );

            break;
        }



        NdisStatus=MbbIsVariableFieldValid(
            InBufferSize,
            MbbRegistrationState->ProviderId.Offset,
            MbbRegistrationState->ProviderId.Size,
            MBB_MAXIMUM_PROVIDER_ID_LENGTH,
            sizeof(WCHAR)
            );

        if (NdisStatus != NDIS_STATUS_SUCCESS)
        {
            TraceError( WMBCLASS_OID, "[MbbNdis][ReqId=0x%04x] Bad provider id field, Offset=%d, Size=%d",
                        Request->RequestId,
                        MbbRegistrationState->ProviderId.Offset,
                        MbbRegistrationState->ProviderId.Size
                        );
            WwanStatus = WWAN_STATUS_FAILURE;
            NdisStatus = NDIS_STATUS_INVALID_DATA;
            break;

        }

        NdisStatus=MbbIsVariableFieldValid(
            InBufferSize,
            MbbRegistrationState->ProviderName.Offset,
            MbbRegistrationState->ProviderName.Size,
            MBB_MAXIMUM_PROVIDER_NAME_LENGTH,
            sizeof(WCHAR)
            );

        if (NdisStatus != NDIS_STATUS_SUCCESS)
        {
            TraceError( WMBCLASS_OID, "[MbbNdis][ReqId=0x%04x] Bad provider Name field, Offset=%d, Size=%d",
                        Request->RequestId,
                        MbbRegistrationState->ProviderName.Offset,
                        MbbRegistrationState->ProviderName.Size
                        );
            WwanStatus = WWAN_STATUS_FAILURE;
            NdisStatus = NDIS_STATUS_INVALID_DATA;
            break;

        }

        NdisStatus=MbbIsVariableFieldValid(
            InBufferSize,
            MbbRegistrationState->RoamingText.Offset,
            MbbRegistrationState->RoamingText.Size,
            MBB_MAXIMUM_ROAMING_TEXT_LENGTH,
            sizeof(WCHAR)
            );

        if (NdisStatus != NDIS_STATUS_SUCCESS)
        {
            TraceError( WMBCLASS_OID, "[MbbNdis][ReqId=0x%04x] Bad provider roaming text, Offset=%d, Size=%d",
                        Request->RequestId,
                        MbbRegistrationState->RoamingText.Offset,
                        MbbRegistrationState->RoamingText.Size
                        );
            WwanStatus = WWAN_STATUS_FAILURE;
            NdisStatus = NDIS_STATUS_INVALID_DATA;
            break;

        }

        if( (
                MBB_IS_DEVICE_REGISTERED( MbbRegistrationState->RegisterState ) &&
                MbbRegistrationState->CurrentCellularClass == MbbCellularClassInvalid
            ) ||
            MbbRegistrationState->CurrentCellularClass >= MbbCellularClassMaximum )
        {
            TraceError( WMBCLASS_OID, "[MbbNdis][ReqId=0x%04x] INVALID CurrentCellularClass=0x%x",
                        Request->RequestId,
                        MbbRegistrationState->CurrentCellularClass
                        );
            WwanStatus = WWAN_STATUS_FAILURE;
            NdisStatus = NDIS_STATUS_INVALID_DATA;
            break;
        }

        MbbUtilMbbToWwanRegistrationState(
            MbbRegistrationState,
            &NdisRegistrationState.RegistrationState
            );

        Adapter->AvailableDataClassValid=TRUE;
        Adapter->AvailableDataClass=MbbRegistrationState->AvailableDataClasses;

        MbbAdapterSetCurrentCellularClass(Adapter, MbbRegistrationState->CurrentCellularClass);
    }
    while( FALSE );

    //
    // When to do SMS Configuration indication -
    //  - This is a response to a OID query
    //  - This is an unsolicited indication
    // When to do Ready Info indication -
    //  - This is a response to an internal query
    //  - This is an unsolicited indication
    //



    if (!MbbReqMgrIsInternalRequest( Request ) || MbbReqMgrIsUnsolicitedIndication( Request ))
    {

        //
        //  Not an internal request, external reaponse or indication
        //
        if (!MbbReqMgrIsUnsolicitedIndication( Request ) || (NdisStatus == NDIS_STATUS_SUCCESS))
        {

            //
            //  response or it is an unsolicited indication the was processed without error,
            //  don't propagate indication that don't parse
            //

            NdisRegistrationState.uStatus = WwanStatus;

            StatusIndication.StatusBuffer       = &NdisRegistrationState;
            StatusIndication.StatusBufferSize   = sizeof(NDIS_WWAN_REGISTRATION_STATE);

            TraceInfo(  WMBCLASS_OID, "[MbbNdis][ReqId=0x%04x] Indicating NDIS_STATUS_WWAN_REGISTER_STATE with status=%!WwanStatus!",
                        Request->RequestId,
                        WwanStatus
                        );

            MbbNdisMiniportIndicateRequestStatus(
                Request,
                &StatusIndication
                );
        }
    }

    if( MbbReqMgrIsInternalRequest( Request ) ||
        (
            ( NdisStatus == NDIS_STATUS_SUCCESS ) &&
            MBB_IS_DEVICE_REGISTERED( MbbRegistrationState->RegisterState )
        ) )
    {
        //
        //  it is an internal request which needs to be complete regardless, or it external or unsolictied indication that succeeded
        //  and is in a registered state.
        //  run the packet service locic to send a packet service with updated data classes.
        //

        Request->HandlerContext.Parameters.PacketServiceState.IsWwanAvailableDataClassValid = 1;
        Request->HandlerContext.Parameters.PacketServiceState.AvailableDataClass        = WWAN_DATA_CLASS_NONE;

        if( NdisStatus == NDIS_STATUS_SUCCESS )
        {
            Request->HandlerContext.Parameters.PacketServiceState.AvailableDataClass    = MbbRegistrationState->AvailableDataClasses;

            TraceInfo(  WMBCLASS_OID, "[MbbNdis][ReqId=0x%04x] Saving AvailableDataClass %08lx for Packet Service in registerstate=%d",
                        Request->RequestId,
                        MbbRegistrationState->AvailableDataClasses,
                        MbbRegistrationState->RegisterState
                        );


        }


        MbbNdisIndicatePacketService(
            Request,
            NdisStatus
            );

    }


    return NDIS_STATUS_SUCCESS;
}

NDIS_STATUS
MbbNdisMulticarrierProviderStatusHandler(
    __in PMBB_REQUEST_CONTEXT               Request,
    __in NDIS_STATUS                        NdisStatus,
    __in MBB_STATUS                         MbbStatus,
    __in_bcount_opt(InBufferSize) PUCHAR    InBuffer,
    __in ULONG                              InBufferSize
    )
{
    WWAN_STATUS                     WwanStatus = WWAN_STATUS_FAILURE;
    ULONG                           AdapterDataClass;
    PMBB_PROVIDER_LIST              MbbPreferredProviders = (PMBB_PROVIDER_LIST)InBuffer;
    NDIS_STATUS_INDICATION          StatusIndication;
    PMINIPORT_ADAPTER_CONTEXT       Adapter = MbbReqMgrGetAdapterHandle( Request );
    BOOLEAN                         IsMultiCarrierCapable = MbbAdapterIsMultiCarrierCapable( Adapter );
    ULONGLONG                       NdisPreferredProvidersSize = 0;

    PNDIS_WWAN_PREFERRED_MULTICARRIER_PROVIDERS  NdisPreferredProviders = NULL;
    NDIS_WWAN_PREFERRED_MULTICARRIER_PROVIDERS   LocalNdisPreferredProviders;

    do
    {
        RtlZeroMemory(&LocalNdisPreferredProviders, sizeof(LocalNdisPreferredProviders));

        MBB_INITIALIZE_REQUEST_NDIS_STATUS_INDICATION(
            &StatusIndication,
            Request,
            NDIS_STATUS_WWAN_PREFERRED_MULTICARRIER_PROVIDERS
            );

        if( NdisStatus != NDIS_STATUS_SUCCESS )
        {
            TraceError( WMBCLASS_OID,
                        "[MbbNdis][ReqId=0x%04x] FAILED to get data for NDIS_STATUS_WWAN_PREFERRED_MULTICARRIER_PROVIDERS, status=%!STATUS!",
                        Request->RequestId,
                        NdisStatus
                        );
            break;
        }
        //
        // On error there is no data buffer.
        //
        if( (WwanStatus = MbbUtilMbbToWwanStatus( MbbStatus )) != WWAN_STATUS_SUCCESS )
        {
            break;
        }
        //
        // On set requests the MB Driver Model requires that the preferred provider list is empty.
        //
        if( MbbReqMgrIsSetOid( Request ) == TRUE )
        {
            break;
        }

        if(!IsMultiCarrierCapable)
        {
            break;
        }

        //
        // Validate the received data and every provider structure in the list.
        //
        if( MbbUtilValidateMbbProviderList(
                InBufferSize,
                MbbPreferredProviders
                ) != NDIS_STATUS_SUCCESS )
        {
            TraceError( WMBCLASS_OID, "[MbbNdis][ReqId=0x%04x] FAILED to validate MBB_PROVIDER_LIST",
                        Request->RequestId
                        );
            WwanStatus = WWAN_STATUS_FAILURE;
            break;
        }

        //
        // Convert the provider_list to the WWAN format.
        //
        if (MbbPreferredProviders->ProviderCount > MBB_MAX_PROVIDER_LIST_SIZE)
        {
            TraceError( WMBCLASS_OID, "[MbbNdis][ReqId=0x%04x] Provider count exceeds max, MbbPreferredProviders->ProviderCount=%d",
                        Request->RequestId,
                        MbbPreferredProviders->ProviderCount
                        );
            WwanStatus = WWAN_STATUS_FAILURE;
            break;

        }

        //
        // Convert the provider_list to the WWAN format.
        //
        NdisPreferredProvidersSize  = sizeof(NDIS_WWAN_PREFERRED_MULTICARRIER_PROVIDERS);
        NdisPreferredProvidersSize += ( (ULONGLONG)MbbPreferredProviders->ProviderCount * sizeof(WWAN_PROVIDER2) );

        if( NdisPreferredProvidersSize > ULONG_MAX ||
            (NdisPreferredProviders = (PNDIS_WWAN_PREFERRED_MULTICARRIER_PROVIDERS)ALLOCATE_NONPAGED_POOL( (ULONG)NdisPreferredProvidersSize ) ) == NULL )
        {
            TraceError( WMBCLASS_OID,
                        "[MbbNdis][ReqId=0x%04x] FAILED to allocate %d bytes for NDIS_STATUS_WWAN_PREFERRED_MULTICARRIER_PROVIDERS, ProviderCount=%d",
                        Request->RequestId,
                        (ULONG)NdisPreferredProvidersSize,
                        MbbPreferredProviders->ProviderCount
                        );
            WwanStatus = WWAN_STATUS_FAILURE;
            break;
        }

        NdisPreferredProviders->Header.Type      = NDIS_OBJECT_TYPE_DEFAULT;
        NdisPreferredProviders->Header.Size      = sizeof(NDIS_WWAN_PREFERRED_MULTICARRIER_PROVIDERS);
        NdisPreferredProviders->Header.Revision  = NDIS_WWAN_PREFERRED_MULTICARRIER_PROVIDERS_REVISION_1;

        MbbAdapterGetDataClass( Adapter, &AdapterDataClass );

        MbbUtilMbbToWwanProviderList(
            MbbPreferredProviders,
            AdapterDataClass,
            TRUE,
            &NdisPreferredProviders->PreferredListHeader
            );
    }
    while( FALSE );

    if( NdisPreferredProviders == NULL )
    {
        LocalNdisPreferredProviders.Header.Type      = NDIS_OBJECT_TYPE_DEFAULT;
        LocalNdisPreferredProviders.Header.Size      = sizeof(NDIS_WWAN_PREFERRED_MULTICARRIER_PROVIDERS);
        LocalNdisPreferredProviders.Header.Revision  = NDIS_WWAN_PREFERRED_MULTICARRIER_PROVIDERS_REVISION_1;

        LocalNdisPreferredProviders.PreferredListHeader.ElementCount = 0;
        LocalNdisPreferredProviders.PreferredListHeader.ElementType  = WwanStructProvider2;

        StatusIndication.StatusBuffer       = &LocalNdisPreferredProviders;
        StatusIndication.StatusBufferSize   = sizeof(LocalNdisPreferredProviders);
    }
    else
    {
        StatusIndication.StatusBuffer       = NdisPreferredProviders;
        StatusIndication.StatusBufferSize   = (ULONG)NdisPreferredProvidersSize;
    }
    ((PNDIS_WWAN_PREFERRED_PROVIDERS)(StatusIndication.StatusBuffer))->uStatus = WwanStatus;

    TraceInfo(  WMBCLASS_OID, "[MbbNdis][ReqId=0x%04x] Indicating NDIS_STATUS_WWAN_PREFERRED_MULTICARRIER_PROVIDERS with status=%!WwanStatus!",
                Request->RequestId,
                WwanStatus
                );

    MbbNdisMiniportIndicateRequestStatus(
        Request,
        &StatusIndication
        );

    if( NdisPreferredProviders != NULL )
        FREE_POOL( NdisPreferredProviders );

    return NDIS_STATUS_SUCCESS;
}

VOID
MbbNdisIndicatePacketServiceFailure(
    __in PMBB_REQUEST_CONTEXT   Request,
    __in NDIS_STATUS            Status
    )
{
    NDIS_STATUS_INDICATION  StatusIndication;
    NDIS_WWAN_PACKET_SERVICE_STATE    LocalNdisPacketService;

    RtlZeroMemory(
        &LocalNdisPacketService,
        sizeof(LocalNdisPacketService)
        );

    LocalNdisPacketService.Header.Type          = NDIS_OBJECT_TYPE_DEFAULT;
    LocalNdisPacketService.Header.Size          = sizeof(NDIS_WWAN_PACKET_SERVICE_STATE);
    LocalNdisPacketService.Header.Revision      = NDIS_WWAN_PACKET_SERVICE_STATE_REVISION_1;

    LocalNdisPacketService.uStatus=Status;

    MBB_INITIALIZE_REQUEST_NDIS_STATUS_INDICATION(
       &StatusIndication,
        Request,
        NDIS_STATUS_WWAN_PACKET_SERVICE
        );

    StatusIndication.StatusBuffer       = &LocalNdisPacketService;
    StatusIndication.StatusBufferSize   = sizeof(NDIS_WWAN_PACKET_SERVICE_STATE);

    TraceError( WMBCLASS_OID, "[MbbNdis][ReqId=0x%04x] PACKET_SERVICE_FAILURE", Request->RequestId );

    MbbNdisMiniportIndicateRequestStatus(
        Request,
       &StatusIndication
        );
}

VOID
MbbNdisIndicatePacketService(
    __in PMBB_REQUEST_CONTEXT   Request,
    __in NDIS_STATUS            NdisStatus
    )
{

    BOOLEAN                 IndicateFailure = TRUE;
    ULONG                   NdisPacketServiceStateSize = Request->HandlerContext.Parameters.PacketServiceState.NdisPacketServiceStateSize;
    PNDIS_WWAN_PACKET_SERVICE_STATE NdisPacketServiceState = Request->HandlerContext.Parameters.PacketServiceState.NdisPacketServiceState;

    do
    {
        MBB_COMMAND          NewCommand;
        PMBB_REQUEST_CONTEXT NewRequest;

        //
        // Track whether this is a indication or a response to a query.
        // If this is a response to a query readyinfo then ndis indication needs to
        // happen on any failure. In case of indications ignore failures.
        //
        if( Request->HandlerContext.Parameters.PacketServiceState.IsFirstCid == 0 )
        {
            Request->HandlerContext.Parameters.PacketServiceState.IsFirstCid = 1;

            if( memcmp(
                   &Request->HandlerContext.Response.Command.ServiceId,
                   &MBB_UUID_BASIC_CONNECT,
                    sizeof(GUID)
                    ) == 0
                &&
                Request->HandlerContext.Response.Command.CommandId == MBB_BASIC_CID_PACKET_SERVICE
                &&
                !MbbReqMgrIsUnsolicitedIndication( Request ) )
            {
                Request->HandlerContext.Parameters.PacketServiceState.IsQuery = 1;
            }
        }

        //
        // If the CID response wasnt receieved then bail out.
        //
        if( NdisStatus != NDIS_STATUS_SUCCESS )
        {
            break;
        }

        //
        // If we have all the information then indicate ready_info.
        //
        if( Request->HandlerContext.Parameters.PacketServiceState.NdisPacketServiceState != NULL &&
            Request->HandlerContext.Parameters.PacketServiceState.IsWwanAvailableDataClassValid != 0 )
        {
            NDIS_STATUS_INDICATION  StatusIndication;

            NdisPacketServiceState->PacketService.AvailableDataClass    = Request->HandlerContext.Parameters.PacketServiceState.AvailableDataClass;

            MBB_INITIALIZE_REQUEST_NDIS_STATUS_INDICATION(
               &StatusIndication,
                Request,
                NDIS_STATUS_WWAN_PACKET_SERVICE
                );

            StatusIndication.StatusBuffer       = NdisPacketServiceState;
            StatusIndication.StatusBufferSize   = NdisPacketServiceStateSize;

            TraceInfo( WMBCLASS_OID, "[MbbNdis][ReqId=0x%04x] Indicating NDIS_STATUS_WWAN_PACKET_SERVICE size=%d", Request->RequestId, NdisPacketServiceStateSize );

            MbbNdisMiniportIndicateRequestStatus(
                Request,
               &StatusIndication
                );
            IndicateFailure = FALSE;
            break;
        }
        //
        // If we dont have any info then only indicate failure if this was a ready_info oid query.
        // Fail the request in this case.
        //
        if( Request->HandlerContext.Parameters.PacketServiceState.NdisPacketServiceState == NULL &&
            Request->HandlerContext.Parameters.PacketServiceState.IsWwanAvailableDataClassValid == 0 )
        {
            if( Request->HandlerContext.Parameters.PacketServiceState.IsQuery != 1 )
            {
                IndicateFailure = FALSE;
            }
            break;
        }
        //
        // We have something, get the other information.
        // Select the next command to query
        //
        if( Request->HandlerContext.Parameters.PacketServiceState.NdisPacketServiceState == NULL )
        {
            NewCommand.ServiceId = MBB_UUID_BASIC_CONNECT;
            NewCommand.CommandId = MBB_BASIC_CID_PACKET_SERVICE;
        } else
        if( Request->HandlerContext.Parameters.PacketServiceState.IsWwanAvailableDataClassValid == 0 )
        {
            NewCommand.ServiceId = MBB_UUID_BASIC_CONNECT;
            NewCommand.CommandId = MBB_BASIC_CID_REGISTER_STATE;
        }

        if( (NewRequest = MbbReqMgrCreateRequest(
                            Request->RequestManager,
                            NULL,
                            0,
                            &NdisStatus)) == NULL )
        {
            TraceError( WMBCLASS_OID, "[MbbNdis][ReqId=0x%04x] FAILED to allocate Request for retrieving %s",
                        Request->RequestId,
                        MbbUtilGetCommandString( &NewCommand )
                        );
            break;
        }
        NewRequest->OidHandler = MbbNdisGetOidHandlerByCommand( &NewCommand );
        //
        // Cache the context from the previous request since it will be destroyed
        //
        if (Request->HandlerContext.Parameters.PacketServiceState.IsQuery)
        {
            //
            //  if this is the packet service query, save this since the context will be destroyed
            //
            NewRequest->OidContext.OidRequestId     = Request->OidContext.OidRequestId;
            NewRequest->OidContext.OidRequestHandle = Request->OidContext.OidRequestHandle;
        }

        NewRequest->HandlerContext.Parameters.PacketServiceState = Request->HandlerContext.Parameters.PacketServiceState;
        //
        // Dispatch the new CID request
        //
        TraceInfo(  WMBCLASS_OID, "[MbbNdis][ReqId=0x%04x] Dispatching Request[ReqId=0x%04x] for %s",
                    Request->RequestId,
                    NewRequest->RequestId,
                    MbbUtilGetCommandString( &NewCommand )
                    );

        NdisStatus = MbbReqMgrDispatchRequest(
                        NewRequest,
                        (NewRequest->OidHandler->IsSerialized == 1),
                        MbbUtilInternalCIDQuery,
                        MbbUtilInternalCIDCompletion,
                        MbbUtilInternalCIDResponse
                        );
        //
        // If dispatch failed, then indicate ReadyInfoFailure
        //
        if( NdisStatus != NDIS_STATUS_PENDING )
        {
            TraceError( WMBCLASS_OID, "[MbbNdis][ReqId=0x%04x] FAILED to dispatch %s, NdisStatus=%!STATUS!",
                        NewRequest->RequestId,
                        MbbUtilGetCommandString( &NewCommand ),
                        NdisStatus
                        );
            MbbReqMgrDestroyRequest(
                NewRequest->RequestManager,
                NewRequest
                );
            break;
        }
        //
        // The status handler will take care of the rest.
        //
        NdisPacketServiceState = NULL;
        IndicateFailure = FALSE;
    }
    while( FALSE );

    if( IndicateFailure &&
        Request->HandlerContext.Parameters.PacketServiceState.IsQuery == 1 )
    {
        MbbNdisIndicatePacketServiceFailure( Request , NdisStatus);
    }

    if( NdisPacketServiceState != NULL )
    {
        FREE_POOL( NdisPacketServiceState );
    }
}

NDIS_STATUS
MbbNdisPacketServiceStatusHandler(
    __in PMBB_REQUEST_CONTEXT               Request,
    __in NDIS_STATUS                        NdisStatus,
    __in MBB_STATUS                         MbbStatus,
    __in_bcount_opt(InBufferSize) PUCHAR    InBuffer,
    __in ULONG                              InBufferSize
    )
{
    PMINIPORT_ADAPTER_CONTEXT   Adapter = (PMINIPORT_ADAPTER_CONTEXT)(Request->RequestManager->AdapterContext);
    PNDIS_WWAN_PACKET_SERVICE_STATE NdisPacketServiceState=NULL;
    PMBB_PACKET_SERVICE             MbbPacketService = (PMBB_PACKET_SERVICE)InBuffer;


    do
    {

        if( NdisStatus != NDIS_STATUS_SUCCESS )
        {
            TraceError( WMBCLASS_OID, "[MbbNdis][ReqId=0x%04x] FAILED to get data for NDIS_STATUS_WWAN_PACKET_SERVICE, status=%!STATUS!",
                        Request->RequestId,
                        NdisStatus
                        );
            break;
        }

        if( MbbPacketService == NULL || (InBufferSize < sizeof(MBB_PACKET_SERVICE) ) )
        {
            TraceError( WMBCLASS_OID, "[MbbNdis][ReqId=0x%04x] INSUFFICIENT BufferSize for NDIS_STATUS_WWAN_PACKET_SERVICE, BufferSize[Received=%d Expected=%d]",
                        Request->RequestId,
                        InBufferSize,
                        sizeof(MBB_PACKET_SERVICE)
                        );
            NdisStatus = NDIS_STATUS_INVALID_DATA;
            break;
        }

        NdisPacketServiceState =(PNDIS_WWAN_PACKET_SERVICE_STATE) ALLOCATE_NONPAGED_POOL(sizeof(*NdisPacketServiceState));

        if (NdisPacketServiceState == NULL)
        {
            TraceError( WMBCLASS_OID, "[MbbNdis][ReqId=0x%04x] FAILED to allocate ndis structure",
                    Request->RequestId
                    );
            NdisStatus = NDIS_STATUS_RESOURCES;
            break;
        }

        RtlZeroMemory(NdisPacketServiceState, sizeof(*NdisPacketServiceState));

        NdisPacketServiceState->Header.Type      = NDIS_OBJECT_TYPE_DEFAULT;
        NdisPacketServiceState->Header.Size      = sizeof(NDIS_WWAN_PACKET_SERVICE_STATE);
        NdisPacketServiceState->Header.Revision  = NDIS_WWAN_PACKET_SERVICE_STATE_REVISION_1;

        Request->HandlerContext.Parameters.PacketServiceState.NdisPacketServiceStateSize=sizeof(*NdisPacketServiceState);
        Request->HandlerContext.Parameters.PacketServiceState.NdisPacketServiceState=NdisPacketServiceState;


        NdisPacketServiceState->PacketService.uNwError = MbbPacketService->NetworkError;

        NdisPacketServiceState->uStatus = MbbUtilMbbToWwanStatus( MbbStatus );

        if ( MbbUtilMbbToWwanStatus( MbbStatus ) != WWAN_STATUS_SUCCESS )
        {
            break;
        }

        MbbUtilMbbToWwanPacketService(
            MbbPacketService,
            &NdisPacketServiceState->PacketService
            );

        Adapter->UplinkSpeed = MbbPacketService->UplinkSpeed;
        Adapter->DownlinkSpeed = MbbPacketService->DownlinkSpeed;

        if (Adapter->AvailableDataClassValid)
        {
            //
            //  if we have a current data class fill it in.
            //
            Request->HandlerContext.Parameters.PacketServiceState.IsWwanAvailableDataClassValid = 1;
            Request->HandlerContext.Parameters.PacketServiceState.AvailableDataClass=Adapter->AvailableDataClass;
        }
    }
    while( FALSE );
    //
    // Restore the device returned operational status, if any.
    //
    if( NdisStatus != NDIS_STATUS_SUCCESS &&
        MbbUtilMbbToWwanStatus( MbbStatus ) != WWAN_STATUS_SUCCESS )
    {
        NdisStatus = MbbUtilMbbToWwanStatus( MbbStatus );
    }

    TraceInfo(  WMBCLASS_OID, "[MbbNdis][ReqId=0x%04x] Indicating NDIS_STATUS_WWAN_PACKET_SERVICE with status=%!STATUS!",
                Request->RequestId,
                NdisStatus
                );

    MbbNdisIndicatePacketService(
        Request,
        NdisStatus
        );

    return NDIS_STATUS_SUCCESS;
}

NDIS_STATUS
MbbNdisSignalStateStatusHandler(
    __in PMBB_REQUEST_CONTEXT               Request,
    __in NDIS_STATUS                        NdisStatus,
    __in MBB_STATUS                         MbbStatus,
    __in_bcount_opt(InBufferSize) PUCHAR    InBuffer,
    __in ULONG                              InBufferSize
    )
{
    WWAN_STATUS             WwanStatus = WWAN_STATUS_FAILURE;
    NDIS_STATUS_INDICATION  StatusIndication;
    NDIS_WWAN_SIGNAL_STATE  NdisSignalState;
    PMBB_SIGNAL_STATE       MbbSignalState = (PMBB_SIGNAL_STATE)InBuffer;

    do
    {
        RtlZeroMemory(&NdisSignalState, sizeof(NdisSignalState));

        MBB_INITIALIZE_REQUEST_NDIS_STATUS_INDICATION(
            &StatusIndication,
            Request,
            NDIS_STATUS_WWAN_SIGNAL_STATE
            );

        NdisSignalState.Header.Type      = NDIS_OBJECT_TYPE_DEFAULT;
        NdisSignalState.Header.Size      = sizeof(NDIS_WWAN_SIGNAL_STATE);
        NdisSignalState.Header.Revision  = NDIS_WWAN_SIGNAL_STATE_REVISION_1;

        if( NdisStatus != NDIS_STATUS_SUCCESS )
        {
            TraceError( WMBCLASS_OID, "[MbbNdis][ReqId=0x%04x] FAILED to get data for NDIS_STATUS_WWAN_SIGNAL_STATE, status=%!STATUS!",
                        Request->RequestId,
                        NdisStatus
                        );
            break;
        }

        WwanStatus = MbbUtilMbbToWwanStatus( MbbStatus );

        if( WwanStatus != WWAN_STATUS_SUCCESS)
        {
            break;
        }


        if( MbbSignalState == NULL ||
            InBufferSize < sizeof(MBB_SIGNAL_STATE) )
        {
            TraceError( WMBCLASS_OID, "[MbbNdis][ReqId=0x%04x] INSUFFICIENT BufferSize for NDIS_STATUS_WWAN_SIGNAL_STATE, BufferSize[Received=%d Expected=%d]",
                        Request->RequestId,
                        InBufferSize,
                        sizeof(MBB_SIGNAL_STATE)
                        );
            WwanStatus = WWAN_STATUS_FAILURE;
            break;
        }

        MbbUtilMbbToWwanSignalState(
            MbbSignalState,
            &NdisSignalState.SignalState
            );
    }
    while( FALSE );
    //
    // Restore the device returned operational status, if any.
    //
    if( WwanStatus != WWAN_STATUS_SUCCESS &&
        MbbUtilMbbToWwanStatus( MbbStatus ) != WWAN_STATUS_SUCCESS )
    {
        WwanStatus = MbbUtilMbbToWwanStatus( MbbStatus );
    }

    NdisSignalState.uStatus = WwanStatus;

    StatusIndication.StatusBuffer       = &NdisSignalState;
    StatusIndication.StatusBufferSize   = sizeof(NDIS_WWAN_SIGNAL_STATE);

    TraceInfo(  WMBCLASS_OID, "[MbbNdis][ReqId=0x%04x] Indicating NDIS_STATUS_WWAN_SIGNAL_STATE with status=%!WwanStatus!",
                Request->RequestId,
                WwanStatus
                );

    MbbNdisMiniportIndicateRequestStatus(
        Request,
        &StatusIndication
        );

    return NDIS_STATUS_SUCCESS;
}


NDIS_STATUS
MbbNdisConnectStatusHandler(
    __in PMBB_REQUEST_CONTEXT               Request,
    __in NDIS_STATUS                        NdisStatus,
    __in MBB_STATUS                         MbbStatus,
    __in_bcount_opt(InBufferSize) PUCHAR    InBuffer,
    __in ULONG                              InBufferSize
    )
{
    WWAN_STATUS                 WwanStatus = WWAN_STATUS_FAILURE;
    NDIS_STATUS_INDICATION      StatusIndication;
    NDIS_WWAN_CONTEXT_STATE     NdisContextState;
    PMBB_CONTEXT_STATE          MbbContextState = (PMBB_CONTEXT_STATE)InBuffer;
    PMINIPORT_ADAPTER_CONTEXT   Adapter = (PMINIPORT_ADAPTER_CONTEXT)MbbReqMgrGetAdapterHandle( Request );
    PMBB_PORT                   Port = NULL;
    NDIS_PORT_NUMBER            PortNumber = NDIS_DEFAULT_PORT_NUMBER;
    BOOLEAN                     ResetPort = FALSE;

    do
    {
        RtlZeroMemory(&NdisContextState, sizeof(NdisContextState));

        MBB_INITIALIZE_REQUEST_NDIS_STATUS_INDICATION(
            &StatusIndication,
            Request,
            NDIS_STATUS_WWAN_CONTEXT_STATE
            );

        NdisContextState.Header.Type      = NDIS_OBJECT_TYPE_DEFAULT;
        NdisContextState.Header.Size      = SIZEOF_NDIS_WWAN_CONTEXT_STATE_REVISION_2;
        NdisContextState.Header.Revision  = NDIS_WWAN_CONTEXT_STATE_REVISION_2;

        if( NdisStatus != NDIS_STATUS_SUCCESS )
        {
            TraceError( WMBCLASS_OID, "[MbbNdis][ReqId=0x%04x] FAILED to get data for NDIS_STATUS_WWAN_CONTEXT_STATE, status=%!STATUS!",
                        Request->RequestId,
                        NdisStatus
                        );
            break;
        }
        //
        // A MBB_CONTEXT_STATE structure may follow even when MBB_STATUS
        // is not successful. It is required, for example, to communicate
        // the Network specific error.
        //
        WwanStatus = MbbUtilMbbToWwanStatus( MbbStatus );

        if ((WwanStatus != WWAN_STATUS_SUCCESS) && ((InBufferSize < sizeof(MBB_CONTEXT_STATE) || (MbbContextState == NULL))))
        {
            //
            //  if the request failed and the buffer returned is too small, break out now and return the error.
            //  In the case of an error with a valid buffer, continue to try to get the uNwError to return
            //
            TraceError( WMBCLASS_OID, "[MbbNdis][ReqId=0x%04x] WwanStatus not successful, WwanStatus=%!WwanStatus!, MbbStatus=%!MbbStatus!",
                        Request->RequestId,
                        WwanStatus,
                        MbbStatus
                        );
            break;
        }

        if( MbbContextState == NULL || InBufferSize < sizeof(MBB_CONTEXT_STATE) )
        {
            //
            //  because of the check above, this is only hit for the success case.
            //
            TraceError( WMBCLASS_OID, "[MbbNdis][ReqId=0x%04x] INSUFFICIENT BufferSize for NDIS_STATUS_WWAN_CONTEXT_STATE, BufferSize[Received=%d Expected=%d]",
                        Request->RequestId,
                        InBufferSize,
                        sizeof(MBB_CONTEXT_STATE)
                        );
            //
            //  only change the error if the current status is success
            //
            if (WwanStatus == WWAN_STATUS_SUCCESS)
            {
                WwanStatus = WWAN_STATUS_FAILURE;
            }
            break;
        }

        // Get the port. Always rely on the session id returned by the device as this
        // responsehandler may be triggered due to an OID request from above
        // or some MB device specific notification from below. Session Id is set in both cases.
        Port = MbbWwanTranslateSessionIdToPort(Adapter, MbbContextState->SessionId);

        if( Port == NULL )
        {
            // invalid port context
            TraceError( WMBCLASS_OID, "[MbbNdis][ReqId=0x%04x] Received invalid port context in MbbNdisConnectStatusHandler", Request->RequestId);

            if( MbbContextState->ActivationState == MbbActivationStateDeactivated )
            {
                // Handling the case when port ( virtual interface) may have been brought down before context deactivation. In that
                // case the port will not be found but the session Id is still valid. Hence return the session id here. It will be returned
                // successfully if its a valid session Id. Else this will be a no-op.
                MbbWwanReturnSessionId(Adapter, MbbContextState->SessionId);

                TraceWarn( WMBCLASS_OID, "[MbbNdis][ReqId=0x%04x] Returned session id for CONTEXT DEACTIVATION response for session Id[%lu] for an invalid port context in MbbNdisConnectStatusHandler",
                                            Request->RequestId,
                                            MbbContextState->SessionId);
            }
            else
            {
                // This means that we have the port removed while the context activation was queued.
                TraceWarn( WMBCLASS_OID, "[MbbNdis][ReqId=0x%04x] Not returning session id for context request: [%d] response for session Id[%lu] for an invalid port context in MbbNdisConnectStatusHandler",
                                           Request->RequestId,
                                           (int)MbbContextState->ActivationState,
                                           MbbContextState->SessionId);
            }

            if (WwanStatus == WWAN_STATUS_SUCCESS)
            {
                WwanStatus = WWAN_STATUS_FAILURE;
            }

            break;
        }

        // Get the port number
        MBB_ACQUIRE_PORT_LOCK(Port);
        PortNumber = Port->PortNumber;
        MBB_RELEASE_PORT_LOCK(Port);

        MbbUtilMbbToWwanContextState(
            MbbContextState,
            MbbWwanGetPortConnectionId(Port),
            &NdisContextState.ContextState
            );
    }
    while( FALSE );

    if( (MbbContextState == NULL || InBufferSize < sizeof(MBB_CONTEXT_STATE)) &&
        (MbbReqMgrIsInternalRequest( Request ) == FALSE) &&
        (MbbReqMgrIsUnsolicitedIndication( Request ) == FALSE)
        )
    {
        //
        // We cannot get the SessionId from the MbbContextState because it is invalid.
        // Since this is a transactional response, we do a best-effort to send the response up
        // on the default port.
        // NOTE: Unsolicited indications with invalid context state are dropped.
        //

        MBB_CONTEXT_STATE DummyMbbContextState = {0};

        PortNumber = NDIS_DEFAULT_PORT_NUMBER;
        Port = MbbWwanTranslatePortNumberToPort(Adapter, PortNumber);

        if (Port != NULL)
        {
            TraceInfo(  WMBCLASS_OID, "[MbbNdis][ReqId=0x%04x] MbbContextState is invalid. Using NDIS_DEFAULT_PORT_NUMBER (%lu) to indicate response",
                        Request->RequestId,
                        PortNumber
                        );

            MbbUtilMbbToWwanContextState(
                &DummyMbbContextState,
                MbbWwanGetPortConnectionId(Port),
                &NdisContextState.ContextState
                );
        }
        else
        {
            TraceError( WMBCLASS_OID, "[MbbNdis][ReqId=0x%04x] MbbContextState is invalid AND we failed to retrieve the default port info",
                        Request->RequestId);
        }
    }

    //
    // If this is an ACTIVATION failure reset the port
    //
    if( WwanStatus != WWAN_STATUS_SUCCESS &&
        MbbReqMgrIsSetOid( Request ) == TRUE &&
        Request->HandlerContext.Parameters.Connect.Activate == TRUE &&
        Port != NULL
        )
    {
        ResetPort = TRUE;
    }

    if(Port != NULL)
    {
        // Indicate status indication on the correct port. If port is not found
        // we dont know which port number to indicate the status on.

        NdisContextState.uStatus = WwanStatus;

        StatusIndication.StatusBuffer       = &NdisContextState;
        StatusIndication.StatusBufferSize   = sizeof(NDIS_WWAN_CONTEXT_STATE);
        StatusIndication.PortNumber         = PortNumber;

        TraceInfo(  WMBCLASS_OID, "[MbbNdis][ReqId=0x%04x] Indicating NDIS_STATUS_WWAN_CONTEXT_STATE with status=%!WwanStatus! for Port number=%lu",
                    Request->RequestId,
                    WwanStatus,
                    PortNumber
                    );

        MbbNdisMiniportIndicateRequestStatus(
            Request,
            &StatusIndication
            );
    }
    else
    {
        TraceError( WMBCLASS_OID, "[MbbNdis][ReqId=0x%04x] Not indicating OID_WWAN_CONNECT completion as the port context is invalid.", Request->RequestId);
    }

    //
    // TODO: Get the speed information from the adpater.
    // The adpater caches the information received from packet_service.
    //
    if( MbbContextState != NULL &&
        Port != NULL &&
        NdisStatus == NDIS_STATUS_SUCCESS &&
        WwanStatus == WWAN_STATUS_SUCCESS
        )
    {
        if( MbbContextState->ActivationState == MbbActivationStateActivated )
        {
            MBB_CONNECTION_STATE   ConnectionState;

            //
            // Verify that the device is reporting the correct SessionId.
            // There isnt much to do apart from logging.
            //
            ASSERT(MbbContextState->SessionId == MbbWwanGetPortSessionId(Port));

            if(MbbContextState->SessionId != MbbWwanGetPortSessionId(Port))
            {
                TraceError( WMBCLASS_OID, "[MbbNdis][ReqId=0x%04x] INVALID SessionId[Receieved=%d Expected=%d] on Connect",
                            Request->RequestId,
                            MbbContextState->SessionId,
                            MbbWwanGetPortSessionId(Port)
                            );
            }

            ConnectionState.ConnectionUp        = TRUE;
            ConnectionState.UpStreamBitRate     = Adapter->UplinkSpeed;
            ConnectionState.DownStreamBitRate   = Adapter->DownlinkSpeed;

            MbbAdapterConnectionChange(
                MbbReqMgrGetAdapterHandle( Request ),
                &ConnectionState,
                PortNumber
                );
        }
        else if( MbbContextState->ActivationState == MbbActivationStateDeactivated )
        {
            MBB_CONNECTION_STATE   ConnectionState;

            ConnectionState.ConnectionUp        = FALSE;
            ConnectionState.UpStreamBitRate     = 0;
            ConnectionState.DownStreamBitRate   = 0;

            MbbAdapterConnectionChange(
                MbbReqMgrGetAdapterHandle( Request ),
                &ConnectionState,
                PortNumber
                );

            // this is a successful deactivation. Reset the port
            ResetPort = (MbbReqMgrIsUnsolicitedIndication( Request ) /*Unsolicited deactivate context*/
                          || MbbReqMgrIsSetOid( Request )); /*Solicited deactivate context*/

        }
    }

    if(Port)
    {
        if(ResetPort)
        {
            TraceInfo( WMBCLASS_OID, "[MbbNdis][ReqId=0x%04x] Resetting port with port number= %d",
                            Request->RequestId,
                            PortNumber);

            MbbWwanSetPortConnectionId(
                                Port,
                                0
                                );

            MbbWwanResetPortSessionId(
                                Port,
                                TRUE // session id should be returned to the session pool
                                );

            MBB_ACQUIRE_PORT_LOCK(Port);

            if(Port->IpAddressInfo != NULL)
            {
                FREE_POOL(Port->IpAddressInfo);
                Port->IpAddressInfo = NULL;
            }

            MBB_RELEASE_PORT_LOCK(Port);
        }

        // Remove the reference added during find
        Dereference(Port);
    }

    // Return the session id to the adapter session id pool if-
    //
    // a) This was a context activation request
    // b) Current state of the request is "MbbRequestStateCancelled"
    // c) Last state of the request < MbbRequestStateResponseReceived.
    //
    // This will ensure that we are processing the request for whom the response hasnt been received as yet.
    //
    // If the response is received for these cancelled requests, that will be a no-op.
    //
    // Requests whose responses have already been received will be ( or are being ) processed by their
    // respective response handlers and the upper layers are responsible for further actions.
    //

    if(Adapter
        && Request->HandlerContext.Parameters.Connect.Activate == TRUE)
    {
        // return session id to the session id pool of the adapter
        // for the cancelled request. Otherwise this session id
        // will leak.

        MBB_REQUEST_STATE CurrentRequestState = MbbRequestStateInvalid;
        MBB_REQUEST_STATE LastRequestState = MbbRequestStateInvalid;
        ULONG SessionId = MBB_INVALID_SESSION_ID;

        MbbReqMgrGetState(
            Request,
            &CurrentRequestState,
            &LastRequestState
            );

        if(CurrentRequestState == MbbRequestStateCancelled)
        {
            if(LastRequestState <  MbbRequestStateResponseReceived)
            {
                SessionId = Request->HandlerContext.Parameters.Connect.SessionId;

                TraceInfo( WMBCLASS_OID, "[MbbNdis][ReqId=0x%04x] Attempting to return the session id of the cancelled context activation request to adapter session id pool.Session Id = %d, Current request state = %!MbbRequestState!, Last request state = %!MbbRequestState!",
                                           Request->RequestId,
                                           SessionId,
                                           CurrentRequestState,
                                           LastRequestState
                                           );

                MbbWwanReturnSessionId(Adapter, SessionId);
            }
            else
            {
               TraceInfo( WMBCLASS_OID, "[MbbNdis][ReqId=0x%04x] Not attempting to return the session id of the cancelled context activation request to adapter session id pool. Session Id = %d, Current request state = %!MbbRequestState!, Last request state = %!MbbRequestState!",
                                           Request->RequestId,
                                           SessionId,
                                           CurrentRequestState,
                                           LastRequestState
                                           );
            }
       }
    }

    return NDIS_STATUS_SUCCESS;
}

NDIS_STATUS
MbbNdisLteAttachConfigHandler(
    __in PMBB_REQUEST_CONTEXT               Request,
    __in NDIS_STATUS                        NdisStatus,
    __in MBB_STATUS                         MbbStatus,
    __in_bcount_opt(InBufferSize) PUCHAR    InBuffer,
    __in ULONG                              InBufferSize
    )
{
    ULONG                           NdisLteAttachContextSize = sizeof(NDIS_WWAN_LTE_ATTACH_CONTEXTS);
    WWAN_STATUS                     WwanStatus = WWAN_STATUS_SUCCESS;
    NDIS_STATUS_INDICATION          StatusIndication;
    PNDIS_WWAN_LTE_ATTACH_CONTEXTS  NdisLteAttachContext = NULL;
    PMBB_MS_LTE_ATTACH_CONFIG_INFO  MbbLteAttchConfigInfo = (PMBB_MS_LTE_ATTACH_CONFIG_INFO)InBuffer;
    PMBB_MS_LTE_ATTACH_CONTEXT      MbbContext=NULL;
    ULONGLONG                       TempSize=0;
    ULONG                           MulTempSize=0;
    ULONG                           i;

    NdisLteAttachContext = ALLOCATE_NONPAGED_POOL( NdisLteAttachContextSize );

    if ( NdisLteAttachContext == NULL )
    {
        TraceError( WMBCLASS_OID, "[MbbNdis][ReqId=0x%04x] FAILED to allocate %d bytes for NDIS_WWAN_LTE_ATTACH_CONTEXTS, ContextCount=%d",
                    Request->RequestId,
                    NdisLteAttachContextSize,
                    MbbLteAttchConfigInfo->ElementCount
                    );
        return NDIS_STATUS_RESOURCES;
    }
    RtlZeroMemory(NdisLteAttachContext, NdisLteAttachContextSize);

    do
    {
        MBB_INITIALIZE_REQUEST_NDIS_STATUS_INDICATION(
            &StatusIndication,
            Request,
            NDIS_STATUS_WWAN_LTE_ATTACH_CONFIG
            );

        if( NdisStatus != NDIS_STATUS_SUCCESS )
        {
            TraceError( WMBCLASS_OID, "[MbbNdis][ReqId=0x%04x] FAILED to get data for NDIS_STATUS_WWAN_LTE_ATTACH_CONFIG, status=%!STATUS!",
                        Request->RequestId,
                        NdisStatus
                        );
            WwanStatus = WWAN_STATUS_FAILURE;
            break;
        }

        WwanStatus = MbbUtilMbbToWwanStatus( MbbStatus );

        if( WwanStatus != WWAN_STATUS_SUCCESS)
        {
            break;
        }

        TempSize = RTL_SIZEOF_THROUGH_FIELD(MBB_MS_LTE_ATTACH_CONFIG_INFO, Contexts);

        if( MbbLteAttchConfigInfo == NULL ||
            InBufferSize < RTL_SIZEOF_THROUGH_FIELD(MBB_MS_LTE_ATTACH_CONFIG_INFO, Contexts) )
        {
            TraceError( WMBCLASS_OID, "[MbbNdis][ReqId=0x%04x] INSUFFICIENT BufferSize for NDIS_STATUS_WWAN_LTE_ATTACH_CONFIG, BufferSize[Received=%d Expected=%d]",
                        Request->RequestId,
                        InBufferSize,
                        RTL_SIZEOF_THROUGH_FIELD(MBB_MS_LTE_ATTACH_CONFIG_INFO, Contexts)
                        );
            WwanStatus = WWAN_STATUS_FAILURE;
            break;
        }

        if (MbbLteAttchConfigInfo->ElementCount != TotalRoamingConditions)
        {
            TraceError( WMBCLASS_OID, "[MbbNdis][ReqId=0x%04x] wrong size element count returned, MbbLteAttchConfigInfo->ContextCount=%d, exepcting=%d",
                        Request->RequestId,
                        MbbLteAttchConfigInfo->ElementCount,
                        TotalRoamingConditions
                        );

            WwanStatus = WWAN_STATUS_FAILURE;
            break;

        }

        //
        //  make sure it is big enough for the array of elements
        //
        NdisStatus = RtlULongMult(
            MbbLteAttchConfigInfo->ElementCount,
            sizeof(MBB_ARRAY_ELEMENT),
            &MulTempSize
            );

        if ( NT_ERROR(NdisStatus) )
        {
            TraceError( WMBCLASS_OID, "[MbbNdis][ReqId=0x%04x] Multiplication overflow occured, MbbLteAttchConfigInfo->ContextCount=%d MBB_ARRAY_ELEMENT size=%d",
                        Request->RequestId,
                        MbbLteAttchConfigInfo->ElementCount,
                        sizeof(MBB_ARRAY_ELEMENT)
                        );

            WwanStatus = WWAN_STATUS_FAILURE;
            break;
        }

        TempSize += MulTempSize;

        if( InBufferSize < TempSize )
        {
            TraceError( WMBCLASS_OID, "[MbbNdis][ReqId=0x%04x] INSUFFICIENT BufferSize for NDIS_STATUS_WWAN_LTE_ATTACH_CONFIG, BufferSize[Received=%d Expected=%d]",
                        Request->RequestId,
                        InBufferSize,
                        (ULONG)TempSize
                        );

            WwanStatus = WWAN_STATUS_FAILURE;
            break;
        }

        // validation of the data buffer
        for (i = 0; i < MbbLteAttchConfigInfo->ElementCount; i++)
        {
            // The max buffer size couldn't be larger than the maximum string length defined in WWAN struct
            // otherwise, the conversion from MBB to WWAN could fail
            ULONG maxContextSize =
                (ULONG)FIELD_OFFSET(MBB_MS_LTE_ATTACH_CONTEXT, DataBuffer) +
                WWAN_ACCESSSTRING_LEN +
                WWAN_USERNAME_LEN +
                WWAN_PASSWORD_LEN;

            NdisStatus = MbbIsVariableFieldValid(
                InBufferSize,
                MbbLteAttchConfigInfo->Contexts[i].Offset,
                MbbLteAttchConfigInfo->Contexts[i].Size,
                maxContextSize,
                sizeof(UCHAR)
            );

            if (!NT_SUCCESS(NdisStatus))
            {
                TraceError(WMBCLASS_OID, "[MbbNdis][ReqId=0x%04x] bad array element in MBB_MS_LTE_ATTACH_CONFIG_INFO",
                    Request->RequestId
                );

                WwanStatus = WWAN_STATUS_FAILURE;
                break;
            }

            if (MbbLteAttchConfigInfo->Contexts[i].Size < (ULONG)FIELD_OFFSET(MBB_MS_LTE_ATTACH_CONTEXT, DataBuffer))
            {
                TraceError( WMBCLASS_OID, "[MbbNdis][ReqId=0x%04x] INSUFFICIENT BufferSize for MBB_CONTEXT, BufferSize[Received=%d Expected=%d]",
                            Request->RequestId,
                            MbbLteAttchConfigInfo->Contexts[i].Size,
                            (ULONG)FIELD_OFFSET(MBB_MS_LTE_ATTACH_CONTEXT, DataBuffer)
                            );

                WwanStatus = WWAN_STATUS_FAILURE;
                break;
            }

            MbbContext = (PMBB_MS_LTE_ATTACH_CONTEXT)(InBuffer + MbbLteAttchConfigInfo->Contexts[i].Offset);
            ULONG MbbContextSize = (ULONG)FIELD_OFFSET(MBB_MS_LTE_ATTACH_CONTEXT, DataBuffer);

            NdisStatus = MbbIsVariableFieldValid(
                MbbLteAttchConfigInfo->Contexts[i].Size,
                MbbContext->AccessString.Offset,
                MbbContext->AccessString.Size,
                MBB_MAXIMUM_ACCESS_STRING_LENGTH,
                sizeof(WCHAR)
                );

            if (!NT_SUCCESS(NdisStatus))
            {
                TraceError( WMBCLASS_OID, "[MbbNdis][ReqId=0x%04x] Invalid access string for MBB_MS_LTE_ATTACH_CONTEXT",
                            Request->RequestId
                            );

                WwanStatus = WWAN_STATUS_FAILURE;
                break;
            }
            MbbContextSize += ROUND_UP_COUNT(MbbContext->AccessString.Size, ALIGN_DWORD);

            NdisStatus = MbbIsVariableFieldValid(
                MbbLteAttchConfigInfo->Contexts[i].Size,
                MbbContext->UserName.Offset,
                MbbContext->UserName.Size,
                MBB_MAXIMUM_USERNAME_LENGTH,
                sizeof(WCHAR)
                );

            if ( !NT_SUCCESS(NdisStatus))
            {
                TraceError( WMBCLASS_OID, "[MbbNdis][ReqId=0x%04x] Invalid access string for MBB_MS_LTE_ATTACH_CONTEXT",
                            Request->RequestId
                            );

                WwanStatus = WWAN_STATUS_FAILURE;
                break;
            }
            MbbContextSize += ROUND_UP_COUNT(MbbContext->UserName.Size, ALIGN_DWORD);

            NdisStatus = MbbIsVariableFieldValid(
                MbbLteAttchConfigInfo->Contexts[i].Size,
                MbbContext->Password.Offset,
                MbbContext->Password.Size,
                MBB_MAXIMUM_PASSWORD_LENGTH,
                sizeof(WCHAR)
                );

            if ( !NT_SUCCESS(NdisStatus))
            {
                TraceError( WMBCLASS_OID, "[MbbNdis][ReqId=0x%04x] Invalid access string for MBB_MS_LTE_ATTACH_CONTEXT",
                            Request->RequestId
                            );

                WwanStatus = WWAN_STATUS_FAILURE;
                break;
            }
            MbbContextSize += ROUND_UP_COUNT(MbbContext->Password.Size, ALIGN_DWORD);

            if (MbbLteAttchConfigInfo->Contexts[i].Size != MbbContextSize)
            {
                TraceError(WMBCLASS_OID, "[MbbNdis][ReqId=0x%04x] Incorrect BufferSize for MBB_MS_LTE_ATTACH_CONTEXT, BufferSize[Received=%d Expected=%d]",
                    Request->RequestId,
                    MbbLteAttchConfigInfo->Contexts[i].Size,
                    MbbContextSize
                );

                WwanStatus = WWAN_STATUS_FAILURE;
                break;
            }
        }

        if (WwanStatus != WWAN_STATUS_SUCCESS)
        {
            break;
        }

        NdisLteAttachContext->Header.Type      = NDIS_OBJECT_TYPE_DEFAULT;
        NdisLteAttachContext->Header.Size      = sizeof(NDIS_WWAN_LTE_ATTACH_CONTEXTS);
        NdisLteAttachContext->Header.Revision  = NDIS_WWAN_LTE_ATTACH_CONTEXT_REVISION_1;
        NdisLteAttachContext->LteAttachContext.ElementCount = MbbLteAttchConfigInfo->ElementCount;

        MbbUtilMbbToWwanLteAttachConfigInfo(
            MbbLteAttchConfigInfo,
            &NdisLteAttachContext->LteAttachContext
            );
    }
    while( FALSE );
    //
    // Restore the device returned operational status, if any.
    //
    if( WwanStatus != WWAN_STATUS_SUCCESS &&
        MbbUtilMbbToWwanStatus( MbbStatus ) != WWAN_STATUS_SUCCESS )
    {
        WwanStatus = MbbUtilMbbToWwanStatus( MbbStatus );
    }

    StatusIndication.StatusBuffer       = NdisLteAttachContext;
    StatusIndication.StatusBufferSize   = NdisLteAttachContextSize;
    ((PNDIS_WWAN_LTE_ATTACH_CONTEXTS)(StatusIndication.StatusBuffer))->uStatus = WwanStatus;

    TraceInfo(  WMBCLASS_OID, "[MbbNdis][ReqId=0x%04x] Indicating NDIS_STATUS_WWAN_PROVISIONED_CONTEXTS with status=%!WwanStatus!",
                Request->RequestId,
                WwanStatus
                );

    MbbNdisMiniportIndicateRequestStatus(
        Request,
        &StatusIndication
        );

    FREE_POOL( NdisLteAttachContext );

    return NdisStatus;
}

NDIS_STATUS
MbbNdisLteAttachStatusHandler(
    __in PMBB_REQUEST_CONTEXT               Request,
    __in NDIS_STATUS                        NdisStatus,
    __in MBB_STATUS                         MbbStatus,
    __in_bcount_opt(InBufferSize) PUCHAR    InBuffer,
    __in ULONG                              InBufferSize
    )
{
    ULONG                           NdisLteAttachStatusSize = sizeof(NDIS_WWAN_LTE_ATTACH_STATUS);
    PNDIS_WWAN_LTE_ATTACH_STATUS    NdisLteAttachStatus = NULL;
    WWAN_STATUS                     WwanStatus = WWAN_STATUS_SUCCESS;
    NDIS_STATUS_INDICATION          StatusIndication;
    PMBB_MS_LTE_ATTACH_STATUS        MbbLteAttchStatus = (PMBB_MS_LTE_ATTACH_STATUS)InBuffer;
    ULONGLONG                       TempSize=0;

    NdisLteAttachStatus = ALLOCATE_NONPAGED_POOL(NdisLteAttachStatusSize);

    if ( NdisLteAttachStatus == NULL )
    {
        TraceError( WMBCLASS_OID, "[MbbNdis][ReqId=0x%04x] FAILED to allocate %d bytes for NDIS_WWAN_LTE_ATTACH_STATUS",
                    Request->RequestId,
                    NdisLteAttachStatusSize
                    );
        return NDIS_STATUS_RESOURCES;
    }

    RtlZeroMemory(NdisLteAttachStatus, NdisLteAttachStatusSize);

    do
    {
        MBB_INITIALIZE_REQUEST_NDIS_STATUS_INDICATION(
            &StatusIndication,
            Request,
            NDIS_STATUS_WWAN_LTE_ATTACH_STATUS
            );

        if( NdisStatus != NDIS_STATUS_SUCCESS )
        {
            TraceError( WMBCLASS_OID, "[MbbNdis][ReqId=0x%04x] FAILED to get data for NDIS_STATUS_WWAN_LTE_ATTACH_STATUS, status=%!STATUS!",
                        Request->RequestId,
                        NdisStatus
                        );
            WwanStatus = WWAN_STATUS_FAILURE;
            break;
        }

        WwanStatus = MbbUtilMbbToWwanStatus( MbbStatus );

        if( WwanStatus != WWAN_STATUS_SUCCESS)
        {
            break;
        }

        if( MbbReqMgrIsSetOid( Request ) == TRUE )
        {
            break;
        }

        TempSize = (ULONG)FIELD_OFFSET(MBB_MS_LTE_ATTACH_STATUS, DataBuffer);

        if( MbbLteAttchStatus == NULL ||
            InBufferSize < TempSize )
        {
            TraceError( WMBCLASS_OID, "[MbbNdis][ReqId=0x%04x] INSUFFICIENT BufferSize for NDIS_STATUS_WWAN_LTE_ATTACH_STATUS, BufferSize[Received=%d Expected=%d]",
                        Request->RequestId,
                        InBufferSize,
                        (ULONG)TempSize
                        );
            WwanStatus = WWAN_STATUS_FAILURE;
            break;
        }

        NdisStatus = MbbIsVariableFieldValid(
            InBufferSize,
            MbbLteAttchStatus->AccessString.Offset,
            MbbLteAttchStatus->AccessString.Size,
            MBB_MAXIMUM_ACCESS_STRING_LENGTH,
            sizeof(WCHAR)
            );

        if ( !NT_SUCCESS(NdisStatus))
        {
            TraceError( WMBCLASS_OID, "[MbbNdis][ReqId=0x%04x] Invalid access string for MBB_MS_LTE_ATTACH_CONTEXT",
                        Request->RequestId
                        );

            WwanStatus = WWAN_STATUS_FAILURE;
            break;
        }


        NdisStatus = MbbIsVariableFieldValid(
            InBufferSize,
            MbbLteAttchStatus->UserName.Offset,
            MbbLteAttchStatus->UserName.Size,
            MBB_MAXIMUM_USERNAME_LENGTH,
            sizeof(WCHAR)
            );

        if ( !NT_SUCCESS(NdisStatus))
        {

            TraceError( WMBCLASS_OID, "[MbbNdis][ReqId=0x%04x] Invalid access string for MBB_MS_LTE_ATTACH_CONTEXT",
                        Request->RequestId
                        );

            WwanStatus = WWAN_STATUS_FAILURE;
            break;
        }


        NdisStatus = MbbIsVariableFieldValid(
            InBufferSize,
            MbbLteAttchStatus->Password.Offset,
            MbbLteAttchStatus->Password.Size,
            MBB_MAXIMUM_PASSWORD_LENGTH,
            sizeof(WCHAR)
            );

        if ( !NT_SUCCESS(NdisStatus))
        {
            TraceError( WMBCLASS_OID, "[MbbNdis][ReqId=0x%04x] Invalid access string for MBB_MS_LTE_ATTACH_CONTEXT",
                        Request->RequestId
                        );

            WwanStatus = WWAN_STATUS_FAILURE;
            break;
        }

        if (WwanStatus != WWAN_STATUS_SUCCESS)
        {
            break;
        }

        NdisLteAttachStatus->Header.Type      = NDIS_OBJECT_TYPE_DEFAULT;
        NdisLteAttachStatus->Header.Size      = sizeof(NDIS_WWAN_LTE_ATTACH_STATUS);
        NdisLteAttachStatus->Header.Revision  = NDIS_WWAN_LTE_ATTACH_STATUS_REVISION_1;

        MbbUtilMbbToWwanLteAttachStatus(
            MbbLteAttchStatus,
            &NdisLteAttachStatus->LteAttachStatus
            );
    }
    while( FALSE );
    //
    // Restore the device returned operational status, if any.
    //
    if( WwanStatus != WWAN_STATUS_SUCCESS &&
        MbbUtilMbbToWwanStatus( MbbStatus ) != WWAN_STATUS_SUCCESS )
    {
        WwanStatus = MbbUtilMbbToWwanStatus( MbbStatus );
    }

    StatusIndication.StatusBuffer       = NdisLteAttachStatus;
    StatusIndication.StatusBufferSize   = NdisLteAttachStatusSize;

    ((PNDIS_WWAN_LTE_ATTACH_STATUS)(StatusIndication.StatusBuffer))->uStatus = WwanStatus;

    TraceInfo(  WMBCLASS_OID, "[MbbNdis][ReqId=0x%04x] Indicating NDIS_STATUS_WWAN_PROVISIONED_CONTEXTS with status=%!WwanStatus!",
                Request->RequestId,
                WwanStatus
                );

    MbbNdisMiniportIndicateRequestStatus(
        Request,
        &StatusIndication
        );

    FREE_POOL( NdisLteAttachStatus );
    return NdisStatus;
}

NDIS_STATUS
MbbNdisProvisionedContextStatusHandler(
    __in PMBB_REQUEST_CONTEXT               Request,
    __in NDIS_STATUS                        NdisStatus,
    __in MBB_STATUS                         MbbStatus,
    __in_bcount_opt(InBufferSize) PUCHAR    InBuffer,
    __in ULONG                              InBufferSize
    )
{
    ULONG                           NdisProvisionedContextSize = 0;
    WWAN_STATUS                     WwanStatus = WWAN_STATUS_FAILURE;
    NDIS_STATUS_INDICATION          StatusIndication;
    PNDIS_WWAN_PROVISIONED_CONTEXTS NdisProvisionedContext = NULL;
    NDIS_WWAN_PROVISIONED_CONTEXTS  LocalNdisProvisionedContext;
    PMBB_CONTEXT_LIST               MbbContextList = (PMBB_CONTEXT_LIST)InBuffer;
    PMBB_CONTEXT                    MbbContext=NULL;
    ULONGLONG                       TempSize=0;
    ULONG                           MulTempSize=0;
    ULONG                           i;
    NTSTATUS                        NtStatus = STATUS_SUCCESS;

    do
    {
        RtlZeroMemory(&LocalNdisProvisionedContext, sizeof(LocalNdisProvisionedContext));

        MBB_INITIALIZE_REQUEST_NDIS_STATUS_INDICATION(
            &StatusIndication,
            Request,
            NDIS_STATUS_WWAN_PROVISIONED_CONTEXTS
            );

        if( NdisStatus != NDIS_STATUS_SUCCESS )
        {
            TraceError( WMBCLASS_OID, "[MbbNdis][ReqId=0x%04x] FAILED to get data for NDIS_STATUS_WWAN_PROVISIONED_CONTEXTS, status=%!STATUS!",
                        Request->RequestId,
                        NdisStatus
                        );
            break;
        }

        WwanStatus = MbbUtilMbbToWwanStatus( MbbStatus );

        if( WwanStatus != WWAN_STATUS_SUCCESS)
        {
            break;
        }


        if( MbbReqMgrIsSetOid( Request ) == TRUE )
        {
            break;
        }

        TempSize = RTL_SIZEOF_THROUGH_FIELD(MBB_CONTEXT_LIST, ContextCount);

        if( MbbContextList == NULL ||
            InBufferSize < RTL_SIZEOF_THROUGH_FIELD(MBB_CONTEXT_LIST, ContextCount) )
        {
            TraceError( WMBCLASS_OID, "[MbbNdis][ReqId=0x%04x] INSUFFICIENT BufferSize for NDIS_STATUS_WWAN_PROVISIONED_CONTEXTS, BufferSize[Received=%d Expected=%d]",
                        Request->RequestId,
                        InBufferSize,
                        RTL_SIZEOF_THROUGH_FIELD(MBB_CONTEXT_LIST, ContextCount)
                        );
            WwanStatus = WWAN_STATUS_FAILURE;
            break;
        }

        if (MbbContextList->ContextCount > MBB_MAX_PROVISIONED_CONTEXTS)
        {
            TraceError( WMBCLASS_OID, "[MbbNdis][ReqId=0x%04x] Too many contexts returned, MbbContextList->ContextCount=%d",
                        Request->RequestId,
                        MbbContextList->ContextCount
                        );

            WwanStatus = WWAN_STATUS_FAILURE;
            break;

        }

        //
        //  make sure it is big enough for the array of elements
        //
        NtStatus = RtlULongMult(
            MbbContextList->ContextCount,
            sizeof(MBB_ARRAY_ELEMENT),
            &MulTempSize
            );

        if ( NT_ERROR(NtStatus) )
        {
            TraceError( WMBCLASS_OID, "[MbbNdis][ReqId=0x%04x] Multiplication overflow occured, MbbContextList->ContextCount=%d MBB_ARRAY_ELEMENT size=%d",
                        Request->RequestId,
                        MbbContextList->ContextCount,
                        sizeof(MBB_ARRAY_ELEMENT)
                        );

            WwanStatus = WWAN_STATUS_FAILURE;
            break;
        }

        TempSize += MulTempSize;

        if( InBufferSize < TempSize )
        {
            TraceError( WMBCLASS_OID, "[MbbNdis][ReqId=0x%04x] INSUFFICIENT BufferSize for NDIS_STATUS_WWAN_PROVISIONED_CONTEXTS, BufferSize[Received=%d Expected=%d]",
                        Request->RequestId,
                        InBufferSize,
                        (ULONG)TempSize
                        );

            WwanStatus = WWAN_STATUS_FAILURE;
            break;
        }

        for (i=0; i < MbbContextList->ContextCount; i++)
        {

            NtStatus = MbbIsVariableFieldValid(
                InBufferSize,
                MbbContextList->Contexts[i].Offset,
                MbbContextList->Contexts[i].Size,
                2048,                 // some reasonable upper limit
                sizeof(UCHAR)
                );

            if ( !NT_SUCCESS(NtStatus))
            {
                TraceError( WMBCLASS_OID, "[MbbNdis][ReqId=0x%04x] bad array element in MBB_CONTEXT_LIST",
                            Request->RequestId
                            );

                WwanStatus = WWAN_STATUS_FAILURE;
                break;
            }

            if (MbbContextList->Contexts[i].Size < (ULONG)FIELD_OFFSET(MBB_CONTEXT, DataBuffer))
            {
                TraceError( WMBCLASS_OID, "[MbbNdis][ReqId=0x%04x] INSUFFICIENT BufferSize for MBB_CONTEXT, BufferSize[Received=%d Expected=%d]",
                            Request->RequestId,
                            MbbContextList->Contexts[i].Size,
                            (ULONG)FIELD_OFFSET(MBB_CONTEXT, DataBuffer)
                            );

                WwanStatus = WWAN_STATUS_FAILURE;
                break;


            }

            MbbContext=(PMBB_CONTEXT)(InBuffer+MbbContextList->Contexts[i].Offset);

            NtStatus = MbbIsVariableFieldValid(
                MbbContextList->Contexts[i].Size,
                MbbContext->AccessString.Offset,
                MbbContext->AccessString.Size,
                MBB_MAXIMUM_ACCESS_STRING_LENGTH,
                sizeof(WCHAR)
                );

            if ( !NT_SUCCESS(NtStatus))
            {
                TraceError( WMBCLASS_OID, "[MbbNdis][ReqId=0x%04x] Invalid access string for MBB_CONTEXT",
                            Request->RequestId
                            );

                WwanStatus = WWAN_STATUS_FAILURE;
                break;
            }


            NtStatus = MbbIsVariableFieldValid(
                MbbContextList->Contexts[i].Size,
                MbbContext->UserName.Offset,
                MbbContext->UserName.Size,
                MBB_MAXIMUM_USERNAME_LENGTH,
                sizeof(WCHAR)
                );

            if ( !NT_SUCCESS(NtStatus))
            {

                TraceError( WMBCLASS_OID, "[MbbNdis][ReqId=0x%04x] Invalid access string for MBB_CONTEXT",
                            Request->RequestId
                            );

                WwanStatus = WWAN_STATUS_FAILURE;
                break;
            }


            NtStatus = MbbIsVariableFieldValid(
                MbbContextList->Contexts[i].Size,
                MbbContext->Password.Offset,
                MbbContext->Password.Size,
                MBB_MAXIMUM_PASSWORD_LENGTH,
                sizeof(WCHAR)
                );

            if ( !NT_SUCCESS(NtStatus))
            {
                TraceError( WMBCLASS_OID, "[MbbNdis][ReqId=0x%04x] Invalid access string for MBB_CONTEXT",
                            Request->RequestId
                            );

                WwanStatus = WWAN_STATUS_FAILURE;
                break;
            }
        }

        if (WwanStatus != WWAN_STATUS_SUCCESS)
        {
            break;
        }

        NtStatus = RtlULongMult(
            MbbContextList->ContextCount,
            sizeof(WWAN_CONTEXT),
            &NdisProvisionedContextSize
            );
        if ( NT_ERROR(NtStatus) )
        {
            TraceError( WMBCLASS_OID, "[MbbNdis][ReqId=0x%04x] Multiplication overflow occured, MbbContextList->ContextCount=%d WWAN_CONTEXT size=%d",
                        Request->RequestId,
                        MbbContextList->ContextCount,
                        sizeof(WWAN_CONTEXT)
                        );
            WwanStatus = WWAN_STATUS_FAILURE;
            break;
        }

        NtStatus = RtlULongAdd(
            NdisProvisionedContextSize,
            sizeof(NDIS_WWAN_PROVISIONED_CONTEXTS),
            &NdisProvisionedContextSize
            );
        if ( NT_ERROR(NtStatus) )
        {
            TraceError( WMBCLASS_OID, "[MbbNdis][ReqId=0x%04x] Addition overflow occured, NdisProvisionedContextSize=%d NDIS_WWAN_PROVISIONED_CONTEXTS size=%d",
                        Request->RequestId,
                        NdisProvisionedContextSize,
                        sizeof(NDIS_WWAN_PROVISIONED_CONTEXTS)
                        );
            WwanStatus = WWAN_STATUS_FAILURE;
            break;
        }

        NdisProvisionedContext = ALLOCATE_NONPAGED_POOL( NdisProvisionedContextSize );

        if ( NdisProvisionedContext == NULL )
        {
            TraceError( WMBCLASS_OID, "[MbbNdis][ReqId=0x%04x] FAILED to allocate %d bytes for NDIS_STATUS_WWAN_PROVISIONED_CONTEXTS, ContextCount=%d",
                        Request->RequestId,
                        NdisProvisionedContextSize,
                        MbbContextList->ContextCount
                        );
            WwanStatus = WWAN_STATUS_FAILURE;
            break;
        }

        RtlZeroMemory(NdisProvisionedContext, NdisProvisionedContextSize);

        NdisProvisionedContext->Header.Type      = NDIS_OBJECT_TYPE_DEFAULT;
        NdisProvisionedContext->Header.Size      = sizeof(NDIS_WWAN_PROVISIONED_CONTEXTS);
        NdisProvisionedContext->Header.Revision  = NDIS_WWAN_PROVISIONED_CONTEXTS_REVISION_1;

        MbbUtilMbbToWwanContextList(
            MbbContextList,
            &NdisProvisionedContext->ContextListHeader
            );
    }
    while( FALSE );
    //
    // Restore the device returned operational status, if any.
    //
    if( WwanStatus != WWAN_STATUS_SUCCESS &&
        MbbUtilMbbToWwanStatus( MbbStatus ) != WWAN_STATUS_SUCCESS )
    {
        WwanStatus = MbbUtilMbbToWwanStatus( MbbStatus );
    }

    if( NdisProvisionedContext == NULL )
    {
        LocalNdisProvisionedContext.Header.Type      = NDIS_OBJECT_TYPE_DEFAULT;
        LocalNdisProvisionedContext.Header.Size      = sizeof(NDIS_WWAN_PROVISIONED_CONTEXTS);
        LocalNdisProvisionedContext.Header.Revision  = NDIS_WWAN_PROVISIONED_CONTEXTS_REVISION_1;

        LocalNdisProvisionedContext.ContextListHeader.ElementType  = WwanStructContext;
        LocalNdisProvisionedContext.ContextListHeader.ElementCount = 0;

        StatusIndication.StatusBuffer       = &LocalNdisProvisionedContext;
        StatusIndication.StatusBufferSize   = sizeof(NDIS_WWAN_PROVISIONED_CONTEXTS);
    }
    else
    {
        StatusIndication.StatusBuffer       = NdisProvisionedContext;
        StatusIndication.StatusBufferSize   = NdisProvisionedContextSize;
    }

    ((PNDIS_WWAN_PROVISIONED_CONTEXTS)(StatusIndication.StatusBuffer))->uStatus = WwanStatus;

    TraceInfo(  WMBCLASS_OID, "[MbbNdis][ReqId=0x%04x] Indicating NDIS_STATUS_WWAN_PROVISIONED_CONTEXTS with status=%!WwanStatus!",
                Request->RequestId,
                WwanStatus
                );

    MbbNdisMiniportIndicateRequestStatus(
        Request,
        &StatusIndication
        );

    if( NdisProvisionedContext != NULL )
    {
        FREE_POOL( NdisProvisionedContext );
    }
    return NDIS_STATUS_SUCCESS;
}


NDIS_STATUS
MbbNdisProvisionedContextV2StatusHandler(
    __in PMBB_REQUEST_CONTEXT               Request,
    __in NDIS_STATUS                        NdisStatus,
    __in MBB_STATUS                         MbbStatus,
    __in_bcount_opt(InBufferSize) PUCHAR    InBuffer,
    __in ULONG                              InBufferSize
)
{
    ULONG                           NdisProvisionedContextSize = 0;
    WWAN_STATUS                     WwanStatus = WWAN_STATUS_FAILURE;
    NDIS_STATUS_INDICATION          StatusIndication;
    PNDIS_WWAN_PROVISIONED_CONTEXTS NdisProvisionedContext = NULL;
    NDIS_WWAN_PROVISIONED_CONTEXTS  LocalNdisProvisionedContext;
    PMBB_CONTEXT_LIST               MbbContextList = (PMBB_CONTEXT_LIST)InBuffer;
    PMBB_MS_CONTEXT_V2              MbbContext = NULL;
    ULONGLONG                       TempSize = 0;
    ULONG                           MulTempSize = 0;
    ULONG                           i;
    NTSTATUS                        NtStatus = STATUS_SUCCESS;

    do
    {
        RtlZeroMemory(&LocalNdisProvisionedContext, sizeof(LocalNdisProvisionedContext));

        MBB_INITIALIZE_REQUEST_NDIS_STATUS_INDICATION(
            &StatusIndication,
            Request,
            NDIS_STATUS_WWAN_PROVISIONED_CONTEXTS
        );

        if (NdisStatus != NDIS_STATUS_SUCCESS)
        {
            TraceError(WMBCLASS_OID, "[MbbNdis][ReqId=0x%04x] FAILED to get data for NDIS_STATUS_WWAN_PROVISIONED_CONTEXTS, status=%!STATUS!",
                Request->RequestId,
                NdisStatus
            );
            break;
        }

        WwanStatus = MbbUtilMbbToWwanStatus(MbbStatus);

        if (WwanStatus != WWAN_STATUS_SUCCESS)
        {
            break;
        }


        if (MbbReqMgrIsSetOid(Request) == TRUE)
        {
            break;
        }

        TempSize = RTL_SIZEOF_THROUGH_FIELD(MBB_CONTEXT_LIST, ContextCount);

        if (MbbContextList == NULL ||
            InBufferSize < RTL_SIZEOF_THROUGH_FIELD(MBB_CONTEXT_LIST, ContextCount))
        {
            TraceError(WMBCLASS_OID, "[MbbNdis][ReqId=0x%04x] INSUFFICIENT BufferSize for NDIS_STATUS_WWAN_PROVISIONED_CONTEXTS, BufferSize[Received=%d Expected=%d]",
                Request->RequestId,
                InBufferSize,
                RTL_SIZEOF_THROUGH_FIELD(MBB_CONTEXT_LIST, ContextCount)
            );
            WwanStatus = WWAN_STATUS_FAILURE;
            break;
        }

        if (MbbContextList->ContextCount > MBB_MAX_PROVISIONED_CONTEXTS)
        {
            TraceError(WMBCLASS_OID, "[MbbNdis][ReqId=0x%04x] Too many contexts returned, MbbContextList->ContextCount=%d",
                Request->RequestId,
                MbbContextList->ContextCount
            );

            WwanStatus = WWAN_STATUS_FAILURE;
            break;

        }

        //
        //  make sure it is big enough for the array of elements
        //
        NtStatus = RtlULongMult(
            MbbContextList->ContextCount,
            sizeof(MBB_ARRAY_ELEMENT),
            &MulTempSize
        );

        if (NT_ERROR(NtStatus))
        {
            TraceError(WMBCLASS_OID, "[MbbNdis][ReqId=0x%04x] Multiplication overflow occured, MbbContextList->ContextCount=%d MBB_ARRAY_ELEMENT size=%d",
                Request->RequestId,
                MbbContextList->ContextCount,
                sizeof(MBB_ARRAY_ELEMENT)
            );

            WwanStatus = WWAN_STATUS_FAILURE;
            break;
        }

        TempSize += MulTempSize;

        if (InBufferSize < TempSize)
        {
            TraceError(WMBCLASS_OID, "[MbbNdis][ReqId=0x%04x] INSUFFICIENT BufferSize for NDIS_STATUS_WWAN_PROVISIONED_CONTEXTS, BufferSize[Received=%d Expected=%d]",
                Request->RequestId,
                InBufferSize,
                (ULONG)TempSize
            );

            WwanStatus = WWAN_STATUS_FAILURE;
            break;
        }

        for (i = 0; i < MbbContextList->ContextCount; i++)
        {

            NtStatus = MbbIsVariableFieldValid(
                InBufferSize,
                MbbContextList->Contexts[i].Offset,
                MbbContextList->Contexts[i].Size,
                2048,                 // some reasonable upper limit
                sizeof(UCHAR)
            );

            if (!NT_SUCCESS(NtStatus))
            {
                TraceError(WMBCLASS_OID, "[MbbNdis][ReqId=0x%04x] bad array element in MBB_CONTEXT_LIST",
                    Request->RequestId
                );

                WwanStatus = WWAN_STATUS_FAILURE;
                break;
            }

            if (MbbContextList->Contexts[i].Size < (ULONG)FIELD_OFFSET(MBB_MS_CONTEXT_V2, DataBuffer))
            {
                TraceError(WMBCLASS_OID, "[MbbNdis][ReqId=0x%04x] INSUFFICIENT BufferSize for MBB_MS_CONTEXT_V2, BufferSize[Received=%d Expected=%d]",
                    Request->RequestId,
                    MbbContextList->Contexts[i].Size,
                    (ULONG)FIELD_OFFSET(MBB_MS_CONTEXT_V2, DataBuffer)
                );

                WwanStatus = WWAN_STATUS_FAILURE;
                break;


            }

            MbbContext = (PMBB_MS_CONTEXT_V2)(InBuffer + MbbContextList->Contexts[i].Offset);

            NtStatus = MbbIsVariableFieldValid(
                MbbContextList->Contexts[i].Size,
                MbbContext->AccessString.Offset,
                MbbContext->AccessString.Size,
                MBB_MAXIMUM_ACCESS_STRING_LENGTH,
                sizeof(WCHAR)
            );

            if (!NT_SUCCESS(NtStatus))
            {
                TraceError(WMBCLASS_OID, "[MbbNdis][ReqId=0x%04x] Invalid access string for MBB_MS_CONTEXT_V2",
                    Request->RequestId
                );

                WwanStatus = WWAN_STATUS_FAILURE;
                break;
            }


            NtStatus = MbbIsVariableFieldValid(
                MbbContextList->Contexts[i].Size,
                MbbContext->UserName.Offset,
                MbbContext->UserName.Size,
                MBB_MAXIMUM_USERNAME_LENGTH,
                sizeof(WCHAR)
            );

            if (!NT_SUCCESS(NtStatus))
            {

                TraceError(WMBCLASS_OID, "[MbbNdis][ReqId=0x%04x] Invalid access string for MBB_MS_CONTEXT_V2",
                    Request->RequestId
                );

                WwanStatus = WWAN_STATUS_FAILURE;
                break;
            }


            NtStatus = MbbIsVariableFieldValid(
                MbbContextList->Contexts[i].Size,
                MbbContext->Password.Offset,
                MbbContext->Password.Size,
                MBB_MAXIMUM_PASSWORD_LENGTH,
                sizeof(WCHAR)
            );

            if (!NT_SUCCESS(NtStatus))
            {
                TraceError(WMBCLASS_OID, "[MbbNdis][ReqId=0x%04x] Invalid access string for MBB_MS_CONTEXT_V2",
                    Request->RequestId
                );

                WwanStatus = WWAN_STATUS_FAILURE;
                break;
            }
        }

        if (WwanStatus != WWAN_STATUS_SUCCESS)
        {
            break;
        }

        NtStatus = RtlULongMult(
            MbbContextList->ContextCount,
            sizeof(WWAN_CONTEXT_V2),
            &NdisProvisionedContextSize
        );
        if (NT_ERROR(NtStatus))
        {
            TraceError(WMBCLASS_OID, "[MbbNdis][ReqId=0x%04x] Multiplication overflow occured, MbbContextList->ContextCount=%d MBB_MS_CONTEXT_V2 size=%d",
                Request->RequestId,
                MbbContextList->ContextCount,
                sizeof(WWAN_CONTEXT_V2)
            );
            WwanStatus = WWAN_STATUS_FAILURE;
            break;
        }

        NtStatus = RtlULongAdd(
            NdisProvisionedContextSize,
            sizeof(NDIS_WWAN_PROVISIONED_CONTEXTS),
            &NdisProvisionedContextSize
        );
        if (NT_ERROR(NtStatus))
        {
            TraceError(WMBCLASS_OID, "[MbbNdis][ReqId=0x%04x] Addition overflow occured, NdisProvisionedContextSize=%d NDIS_WWAN_PROVISIONED_CONTEXTS size=%d",
                Request->RequestId,
                NdisProvisionedContextSize,
                sizeof(NDIS_WWAN_PROVISIONED_CONTEXTS)
            );
            WwanStatus = WWAN_STATUS_FAILURE;
            break;
        }

        NdisProvisionedContext = ALLOCATE_NONPAGED_POOL(NdisProvisionedContextSize);

        if (NdisProvisionedContext == NULL)
        {
            TraceError(WMBCLASS_OID, "[MbbNdis][ReqId=0x%04x] FAILED to allocate %d bytes for NDIS_STATUS_WWAN_PROVISIONED_CONTEXTS, ContextCount=%d",
                Request->RequestId,
                NdisProvisionedContextSize,
                MbbContextList->ContextCount
            );
            WwanStatus = WWAN_STATUS_FAILURE;
            break;
        }

        RtlZeroMemory(NdisProvisionedContext, NdisProvisionedContextSize);

        NdisProvisionedContext->Header.Type = NDIS_OBJECT_TYPE_DEFAULT;
        NdisProvisionedContext->Header.Size = sizeof(NDIS_WWAN_PROVISIONED_CONTEXTS);
        NdisProvisionedContext->Header.Revision = NDIS_WWAN_PROVISIONED_CONTEXTS_REVISION_2;

        MbbUtilMbbToWwanContextV2List(
            MbbContextList,
            &NdisProvisionedContext->ContextListHeader
        );
    } while (FALSE);
    //
    // Restore the device returned operational status, if any.
    //
    if (WwanStatus != WWAN_STATUS_SUCCESS &&
        MbbUtilMbbToWwanStatus(MbbStatus) != WWAN_STATUS_SUCCESS)
    {
        WwanStatus = MbbUtilMbbToWwanStatus(MbbStatus);
    }

    if (NdisProvisionedContext == NULL)
    {
        LocalNdisProvisionedContext.Header.Type = NDIS_OBJECT_TYPE_DEFAULT;
        LocalNdisProvisionedContext.Header.Size = sizeof(NDIS_WWAN_PROVISIONED_CONTEXTS);
        LocalNdisProvisionedContext.Header.Revision = NDIS_WWAN_PROVISIONED_CONTEXTS_REVISION_2;

        LocalNdisProvisionedContext.ContextListHeader.ElementType = WwanStructContextV2;
        LocalNdisProvisionedContext.ContextListHeader.ElementCount = 0;

        StatusIndication.StatusBuffer = &LocalNdisProvisionedContext;
        StatusIndication.StatusBufferSize = sizeof(NDIS_WWAN_PROVISIONED_CONTEXTS);
    }
    else
    {
        StatusIndication.StatusBuffer = NdisProvisionedContext;
        StatusIndication.StatusBufferSize = NdisProvisionedContextSize;
    }

    ((PNDIS_WWAN_PROVISIONED_CONTEXTS)(StatusIndication.StatusBuffer))->uStatus = WwanStatus;

    TraceInfo(WMBCLASS_OID, "[MbbNdis][ReqId=0x%04x] Indicating NDIS_STATUS_WWAN_PROVISIONED_CONTEXTS with status=%!WwanStatus!",
        Request->RequestId,
        WwanStatus
    );

    MbbNdisMiniportIndicateRequestStatus(
        Request,
        &StatusIndication
    );

    if (NdisProvisionedContext != NULL)
    {
        FREE_POOL(NdisProvisionedContext);
    }
    return NDIS_STATUS_SUCCESS;
}

NDIS_STATUS
MbbNdisServiceActivationStatusHandler(
    __in PMBB_REQUEST_CONTEXT               Request,
    __in NDIS_STATUS                        NdisStatus,
    __in MBB_STATUS                         MbbStatus,
    __in_bcount_opt(InBufferSize) PUCHAR    InBuffer,
    __in ULONG                              InBufferSize
    )
{
    ULONG                                   VendorSpecificBufferSize;
    ULONG                                   NdisServiceActivationStatusSize = 0;
    WWAN_STATUS                             WwanStatus = WWAN_STATUS_FAILURE;
    NDIS_STATUS_INDICATION                  StatusIndication;
    PNDIS_WWAN_SERVICE_ACTIVATION_STATUS    NdisServiceActivationStatus = NULL;
    NDIS_WWAN_SERVICE_ACTIVATION_STATUS     LocalNdisServiceActivationStatus;
    PMBB_SERVICE_ACTIVATION_STATUS          MbbServiceActivationStatus = (PMBB_SERVICE_ACTIVATION_STATUS)InBuffer;
    NTSTATUS                                NtStatus = STATUS_SUCCESS;

    do
    {
        RtlZeroMemory(&LocalNdisServiceActivationStatus, sizeof(LocalNdisServiceActivationStatus));

        MBB_INITIALIZE_REQUEST_NDIS_STATUS_INDICATION(
            &StatusIndication,
            Request,
            NDIS_STATUS_WWAN_SERVICE_ACTIVATION
            );

        if( NdisStatus != NDIS_STATUS_SUCCESS )
        {
            TraceError( WMBCLASS_OID, "[MbbNdis][ReqId=0x%04x] FAILED to get data for NDIS_STATUS_WWAN_SERVICE_ACTIVATION, status=%!STATUS!",
                        Request->RequestId,
                        NdisStatus
                        );
            break;
        }

        WwanStatus = MbbUtilMbbToWwanStatus( MbbStatus );

        if( MbbServiceActivationStatus == NULL ||
            InBufferSize < RTL_SIZEOF_THROUGH_FIELD(MBB_SERVICE_ACTIVATION_STATUS, NetworkError) )
        {
            TraceError( WMBCLASS_OID, "[MbbNdis][ReqId=0x%04x] INSUFFICIENT BufferSize for NDIS_STATUS_WWAN_SERVICE_ACTIVATION, BufferSize[Received=%d Expected=%d]",
                        Request->RequestId,
                        InBufferSize,
                        RTL_SIZEOF_THROUGH_FIELD(MBB_SERVICE_ACTIVATION_STATUS, NetworkError)
                        );
            WwanStatus = WWAN_STATUS_FAILURE;
            break;
        }

        VendorSpecificBufferSize = InBufferSize - RTL_SIZEOF_THROUGH_FIELD(MBB_SERVICE_ACTIVATION_STATUS, NetworkError);

        NtStatus = RtlULongAdd(
            VendorSpecificBufferSize,
            sizeof(NDIS_WWAN_SERVICE_ACTIVATION_STATUS),
            &NdisServiceActivationStatusSize
            );
        if ( NT_ERROR(NtStatus) )
        {
            TraceError( WMBCLASS_OID, "[MbbNdis][ReqId=0x%04x] Addition overflow occured, VendorSpecificBufferSize=%d NDIS_WWAN_SERVICE_ACTIVATION_STATUS size=%d",
                        Request->RequestId,
                        VendorSpecificBufferSize,
                        sizeof(NDIS_WWAN_SERVICE_ACTIVATION_STATUS)
                        );
            WwanStatus = WWAN_STATUS_FAILURE;
            break;
        }

        if( (NdisServiceActivationStatus = ALLOCATE_NONPAGED_POOL( NdisServiceActivationStatusSize )) == NULL )
        {
            TraceError( WMBCLASS_OID, "[MbbNdis][ReqId=0x%04x] FAILED to allocate %d bytes for NDIS_STATUS_WWAN_SERVICE_ACTIVATION",
                        Request->RequestId,
                        NdisServiceActivationStatusSize
                        );
            WwanStatus = WWAN_STATUS_FAILURE;
            break;
        }

        NdisServiceActivationStatus->Header.Type      = NDIS_OBJECT_TYPE_DEFAULT;
        NdisServiceActivationStatus->Header.Size      = sizeof(NDIS_WWAN_SERVICE_ACTIVATION_STATUS);
        NdisServiceActivationStatus->Header.Revision  = NDIS_WWAN_SERVICE_ACTIVATION_STATUS_REVISION_1;

        MbbUtilMbbToWwanServiceActivationStatus(
            MbbServiceActivationStatus,
            VendorSpecificBufferSize,
            &NdisServiceActivationStatus->ServiceActivationStatus
            );
    }
    while( FALSE );
    //
    // Restore the device returned operational status, if any.
    //
    if( WwanStatus != WWAN_STATUS_SUCCESS &&
        MbbUtilMbbToWwanStatus( MbbStatus ) != WWAN_STATUS_SUCCESS )
    {
        WwanStatus = MbbUtilMbbToWwanStatus( MbbStatus );
    }

    if( NdisServiceActivationStatus == NULL )
    {
        LocalNdisServiceActivationStatus.Header.Type      = NDIS_OBJECT_TYPE_DEFAULT;
        LocalNdisServiceActivationStatus.Header.Size      = sizeof(NDIS_WWAN_SERVICE_ACTIVATION_STATUS);
        LocalNdisServiceActivationStatus.Header.Revision  = NDIS_WWAN_SERVICE_ACTIVATION_STATUS_REVISION_1;

        LocalNdisServiceActivationStatus.ServiceActivationStatus.uNwError                   = 0;
        LocalNdisServiceActivationStatus.ServiceActivationStatus.uVendorSpecificBufferSize  = 0;

        StatusIndication.StatusBuffer       = &LocalNdisServiceActivationStatus;
        StatusIndication.StatusBufferSize   = sizeof(NDIS_WWAN_SERVICE_ACTIVATION_STATUS);
    }
    else
    {
        StatusIndication.StatusBuffer       = NdisServiceActivationStatus;
        StatusIndication.StatusBufferSize   = NdisServiceActivationStatusSize;
    }

    ((PNDIS_WWAN_SERVICE_ACTIVATION_STATUS)(StatusIndication.StatusBuffer))->uStatus = WwanStatus;

    TraceInfo(  WMBCLASS_OID, "[MbbNdis][ReqId=0x%04x] Indicating NDIS_STATUS_WWAN_SERVICE_ACTIVATION with status=%!WwanStatus!",
                Request->RequestId,
                WwanStatus
                );

    MbbNdisMiniportIndicateRequestStatus(
        Request,
        &StatusIndication
        );

    if( NdisServiceActivationStatus != NULL )
    {
        FREE_POOL( NdisServiceActivationStatus );
    }
    return NDIS_STATUS_SUCCESS;
}

// SMS

NDIS_STATUS
MbbNdisSmsConfigurationStatusHandler(
    __in PMBB_REQUEST_CONTEXT               Request,
    __in NDIS_STATUS                        NdisStatus,
    __in MBB_STATUS                         MbbStatus,
    __in_bcount_opt(InBufferSize) PUCHAR    InBuffer,
    __in ULONG                              InBufferSize
    )
{
    WWAN_STATUS                 WwanStatus = WWAN_STATUS_FAILURE;
    NDIS_STATUS_INDICATION      StatusIndication;
    NDIS_WWAN_SMS_CONFIGURATION NdisSmsConfiguration;
    PMBB_SMS_CONFIGURATION      MbbSmsConfiguration = (PMBB_SMS_CONFIGURATION)InBuffer;

    do
    {
        RtlZeroMemory(&NdisSmsConfiguration, sizeof(NdisSmsConfiguration));

        MBB_INITIALIZE_REQUEST_NDIS_STATUS_INDICATION(
            &StatusIndication,
            Request,
            NDIS_STATUS_WWAN_SMS_CONFIGURATION
            );

        NdisSmsConfiguration.Header.Type      = NDIS_OBJECT_TYPE_DEFAULT;
        NdisSmsConfiguration.Header.Size      = sizeof(NDIS_WWAN_SMS_CONFIGURATION);
        NdisSmsConfiguration.Header.Revision  = NDIS_WWAN_SMS_CONFIGURATION_REVISION_1;

        if( NdisStatus != NDIS_STATUS_SUCCESS )
        {
            TraceError( WMBCLASS_OID, "[MbbNdis][Sms][ReqId=0x%04x] FAILED to get data for NDIS_STATUS_WWAN_SMS_CONFIGURATION, status=%!STATUS!",
                        Request->RequestId,
                        NdisStatus
                        );
            break;
        }

        if( (WwanStatus = MbbUtilMbbToWwanStatus( MbbStatus )) != WWAN_STATUS_SUCCESS )
        {
            NdisStatus = NDIS_STATUS_FAILURE;
            break;
        }

        if( MbbSmsConfiguration == NULL ||
            InBufferSize < (ULONG)FIELD_OFFSET(MBB_SMS_CONFIGURATION, DataBuffer) )
        {
            TraceError( WMBCLASS_OID, "[MbbNdis][Sms][ReqId=0x%04x] INSUFFICIENT BufferSize for NDIS_STATUS_WWAN_SMS_CONFIGURATION, BufferSize[Received=%d Expected=%d]",
                        Request->RequestId,
                        InBufferSize,
                        FIELD_OFFSET(MBB_SMS_CONFIGURATION, ScAddress)
                        );
            WwanStatus = WWAN_STATUS_FAILURE;
            NdisStatus = NDIS_STATUS_INVALID_DATA;
            break;
        }



        //
        // If the SMS store is not initialized then other fields are not valid.
        //
        if( MbbSmsConfiguration->SmsStorageState != MbbSmsStorageInitialized )
        {
            NdisStatus = NDIS_STATUS_FAILURE;
            WwanStatus = WWAN_STATUS_BUSY;
            break;
        }

        NdisStatus=MbbIsVariableFieldValid(
            InBufferSize,
            MbbSmsConfiguration->ScAddress.Offset,
            MbbSmsConfiguration->ScAddress.Size,
            MBB_MAXIMUM_SMS_ADDRESS_LENGTH,
            sizeof(WCHAR)
            );

        if (NdisStatus != NDIS_STATUS_SUCCESS)
        {
            TraceError( WMBCLASS_INIT, "[MbbNdis][ReqId=0x%04x] ScAddress data field, Offset=%d, Size=%d",
                        Request->RequestId,
                        MbbSmsConfiguration->ScAddress.Offset,
                        MbbSmsConfiguration->ScAddress.Size
                        );

            WwanStatus = WWAN_STATUS_FAILURE;
            break;

        }

        MbbUtilMbbToWwanSmsConfiguration(
            MbbSmsConfiguration,
           &NdisSmsConfiguration.SmsConfiguration
            );
    }
    while( FALSE );
    //
    // When to do SMS Configuration indication -
    //  - This is a response to a OID query
    //  - This is an unsolicited indication
    // When to do Ready Info indication -
    //  - This is a response to an internal query
    //  - This is an unsolicited indication
    //
    if( MbbReqMgrIsInternalRequest( Request ) == FALSE ||
        (
            MbbReqMgrIsUnsolicitedIndication( Request ) == TRUE &&
            NdisStatus == NDIS_STATUS_SUCCESS
        ) )
    {
        NdisSmsConfiguration.uStatus = WwanStatus;

        StatusIndication.StatusBuffer       = &NdisSmsConfiguration;
        StatusIndication.StatusBufferSize   = sizeof(NDIS_WWAN_SMS_CONFIGURATION);

        TraceInfo(  WMBCLASS_OID, "[MbbNdis][Sms][ReqId=0x%04x] Indicating NDIS_STATUS_WWAN_SMS_CONFIGURATION with status=%!WwanStatus!",
                    Request->RequestId,
                    WwanStatus
                    );

        MbbNdisMiniportIndicateRequestStatus(
            Request,
            &StatusIndication
            );
    }

    if( MbbReqMgrIsInternalRequest( Request ) == TRUE ||
        (
            MbbReqMgrIsUnsolicitedIndication( Request ) == TRUE &&
            NdisStatus == NDIS_STATUS_SUCCESS
        ) )
    {
        Request->HandlerContext.Parameters.SubscriberReadyInfo.IsCdmaShortMsgSizeValid = 1;
        Request->HandlerContext.Parameters.SubscriberReadyInfo.CdmaShortMsgSize        = WWAN_CDMA_SHORT_MSG_SIZE_UNKNOWN;

        if( NdisStatus == NDIS_STATUS_SUCCESS )
            Request->HandlerContext.Parameters.SubscriberReadyInfo.CdmaShortMsgSize    = (UCHAR)MbbSmsConfiguration->CdmaShortMessageSize;
        //
        // Ignore SMS Configuration failures and return UNKNOWN CdmaShrotMessageSize
        // when SMS_CONFIGURATION was queried internally for a READY_INFO.
        //
        MbbNdisIndicateReadyInfo(
            Request,
            NDIS_STATUS_SUCCESS
            );
    }

    return NDIS_STATUS_SUCCESS;
}

NDIS_STATUS
MbbNdisSmsReadStatusHandler(
    __in PMBB_REQUEST_CONTEXT               Request,
    __in NDIS_STATUS                        NdisStatus,
    __in MBB_STATUS                         MbbStatus,
    __in_bcount_opt(InBufferSize) PUCHAR    InBuffer,
    __in ULONG                              InBufferSize
    )
{
    ULONG                       WwanMessageSize;
    ULONG                       NdisSmsReceiveSize = 0;
    ULONG                       MessageBufferSize = 0;
    WWAN_STATUS                 WwanStatus = WWAN_STATUS_FAILURE;
    NDIS_STATUS_INDICATION      StatusIndication;
    NDIS_WWAN_SMS_RECEIVE       LocalNdisSmsReceive;
    PNDIS_WWAN_SMS_RECEIVE      NdisSmsReceive = NULL;
    PMBB_SMS_RECEIVE            MbbSmsReceive = (PMBB_SMS_RECEIVE)InBuffer;
    NTSTATUS                    Status;
    MBB_CELLULAR_CLASS          CellularClass;

    CellularClass = MbbAdapterGetCurrentCellularClass( MbbReqMgrGetAdapterHandle( Request ) );

    if (MbbAdapterIsMultimodeCapable(MbbReqMgrGetAdapterHandle( Request )))
    {
        //
        //  multimode device always use GSM PDU's
        //
        CellularClass = MbbCellularClassGsm;
    }

    do
    {
        RtlZeroMemory(&LocalNdisSmsReceive, sizeof(LocalNdisSmsReceive));

        MBB_INITIALIZE_REQUEST_NDIS_STATUS_INDICATION(
            &StatusIndication,
            Request,
            NDIS_STATUS_WWAN_SMS_RECEIVE
            );

        if( NdisStatus != NDIS_STATUS_SUCCESS )
        {
            TraceError( WMBCLASS_OID, "[MbbNdis][Sms][ReqId=0x%04x] FAILED to get data for NDIS_STATUS_WWAN_SMS_RECEIVE, status=%!STATUS!",
                        Request->RequestId,
                        NdisStatus
                        );
            break;
        }

        WwanStatus = MbbUtilMbbToWwanStatus( MbbStatus );

        if( WwanStatus != WWAN_STATUS_SUCCESS)
        {
            break;
        }


        //
        // Validate received buffer
        //
        if( MbbUtilValidateMbbSmsReceive(
                CellularClass,
                InBufferSize,
                MbbSmsReceive
                ) != NDIS_STATUS_SUCCESS )
        {
            TraceError( WMBCLASS_OID, "[MbbNdis][Sms][ReqId=0x%04x] FAILED to validate data for MBB_SMS_RECEIVE", Request->RequestId );
            WwanStatus = WWAN_STATUS_FAILURE;
            break;
        }

        if( MbbSmsReceive->SmsFormat == MbbSmsFormatPdu )
            WwanMessageSize = sizeof(WWAN_SMS_PDU_RECORD);
        else
            WwanMessageSize = sizeof(WWAN_SMS_CDMA_RECORD);


        Status=RtlULongMult(MbbSmsReceive->MessageCount,  WwanMessageSize, &MessageBufferSize);

        if (!NT_SUCCESS(Status))
        {
            TraceError( WMBCLASS_OID, "[MbbNdis][Sms][ReqId=0x%04x] overflow Count=%d, Size=%d", Request->RequestId, MbbSmsReceive->MessageCount,  WwanMessageSize );
            WwanStatus = WWAN_STATUS_FAILURE;
            break;
        }

        Status=RtlULongAdd( sizeof(NDIS_WWAN_SMS_RECEIVE), MessageBufferSize, &NdisSmsReceiveSize);

        if (!NT_SUCCESS(Status))
        {
            TraceError( WMBCLASS_OID, "[MbbNdis][Sms][ReqId=0x%04x] overflow fixed=%d, MessageSize=%d", Request->RequestId, sizeof(NDIS_WWAN_SMS_RECEIVE), MessageBufferSize );
            WwanStatus = WWAN_STATUS_FAILURE;
            break;
        }

        NdisSmsReceive = (PNDIS_WWAN_SMS_RECEIVE)ALLOCATE_NONPAGED_POOL( (ULONG)NdisSmsReceiveSize );

        if( NdisSmsReceive  == NULL )
        {
            TraceError( WMBCLASS_OID, "[MbbNdis][Sms][ReqId=0x%04x] FAILED to allocate %u bytes for NDIS_STATUS_WWAN_SMS_RECEIVE, MessageCount=%d",
                        Request->RequestId,
                        NdisSmsReceiveSize,
                        MbbSmsReceive->MessageCount
                        );
            WwanStatus = WWAN_STATUS_FAILURE;
            break;
        }

        NdisSmsReceive->Header.Type      = NDIS_OBJECT_TYPE_DEFAULT;
        NdisSmsReceive->Header.Size      = sizeof(NDIS_WWAN_SMS_RECEIVE);
        NdisSmsReceive->Header.Revision  = NDIS_WWAN_SMS_RECEIVE_REVISION_1;

        if( (NdisStatus = MbbUtilMbbToWwanSmsReceive(
                            MbbSmsReceive,
                            InBufferSize,
                            CellularClass,
                           &NdisSmsReceive->SmsListHeader
                            )) != NDIS_STATUS_SUCCESS )
        {
            TraceError( WMBCLASS_OID, "[MbbNdis][Sms][ReqId=0x%04x] FAILED to convert data for NDIS_STATUS_WWAN_SMS_RECEIVE, status=%!STATUS!",
                        Request->RequestId,
                        NdisStatus
                        );
            WwanStatus = WWAN_STATUS_FAILURE;
            break;
        }
    }
    while( FALSE );
    //
    // Restore the device returned operational status, if any.
    //
    if( WwanStatus != WWAN_STATUS_SUCCESS &&
        MbbUtilMbbToWwanStatus( MbbStatus ) != WWAN_STATUS_SUCCESS )
    {
        WwanStatus = MbbUtilMbbToWwanStatus( MbbStatus );
    }

    if( WwanStatus != WWAN_STATUS_SUCCESS &&
        WwanStatus != WWAN_STATUS_SMS_MORE_DATA )
    {
        StatusIndication.StatusBuffer       = &LocalNdisSmsReceive;
        StatusIndication.StatusBufferSize   = sizeof(NDIS_WWAN_SMS_RECEIVE);

        LocalNdisSmsReceive.Header.Type     = NDIS_OBJECT_TYPE_DEFAULT;
        LocalNdisSmsReceive.Header.Size     = sizeof(NDIS_WWAN_SMS_RECEIVE);
        LocalNdisSmsReceive.Header.Revision = NDIS_WWAN_SMS_RECEIVE_REVISION_1;

        LocalNdisSmsReceive.SmsListHeader.ElementType  = WwanStructSmsPdu;
        LocalNdisSmsReceive.SmsListHeader.ElementCount = 0;
    }
    else
    {
        StatusIndication.StatusBuffer       = NdisSmsReceive;
        StatusIndication.StatusBufferSize   = (ULONG)NdisSmsReceiveSize;
    }

    ((PNDIS_WWAN_SMS_RECEIVE)(StatusIndication.StatusBuffer))->uStatus = WwanStatus;

    TraceInfo(  WMBCLASS_OID, "[MbbNdis][Sms][ReqId=0x%04x] Indicating NDIS_STATUS_WWAN_SMS_RECEIVE with status=%!WwanStatus!",
                Request->RequestId,
                WwanStatus
                );

    MbbNdisMiniportIndicateRequestStatus(
        Request,
        &StatusIndication
        );

    if( NdisSmsReceive != NULL )
        FREE_POOL( NdisSmsReceive );

    if( WwanStatus == WWAN_STATUS_SMS_MORE_DATA )
        return NDIS_STATUS_PENDING;
    else
        return NDIS_STATUS_SUCCESS;
}

NDIS_STATUS
MbbNdisSmsSendStatusHandler(
    __in PMBB_REQUEST_CONTEXT               Request,
    __in NDIS_STATUS                        NdisStatus,
    __in MBB_STATUS                         MbbStatus,
    __in_bcount_opt(InBufferSize) PUCHAR    InBuffer,
    __in ULONG                              InBufferSize
    )
{
    WWAN_STATUS                 WwanStatus = WWAN_STATUS_FAILURE;
    NDIS_STATUS_INDICATION      StatusIndication;
    NDIS_WWAN_SMS_SEND_STATUS   NdisSmsSendStatus;
    PMBB_SMS_SEND_STATUS        MbbSmsSendStatus = (PMBB_SMS_SEND_STATUS)InBuffer;

    do
    {
        RtlZeroMemory(&NdisSmsSendStatus, sizeof(NdisSmsSendStatus));

        MBB_INITIALIZE_REQUEST_NDIS_STATUS_INDICATION(
            &StatusIndication,
            Request,
            NDIS_STATUS_WWAN_SMS_SEND
            );

        NdisSmsSendStatus.Header.Type      = NDIS_OBJECT_TYPE_DEFAULT;
        NdisSmsSendStatus.Header.Size      = sizeof(NDIS_WWAN_SMS_SEND_STATUS);
        NdisSmsSendStatus.Header.Revision  = NDIS_WWAN_SMS_SEND_STATUS_REVISION_1;

        if( NdisStatus != NDIS_STATUS_SUCCESS )
        {
            TraceError( WMBCLASS_OID, "[MbbNdis][Sms][ReqId=0x%04x] FAILED to get data for NDIS_STATUS_WWAN_SMS_SEND, status=%!STATUS!",
                        Request->RequestId,
                        NdisStatus
                        );
            break;
        }

        WwanStatus = MbbUtilMbbToWwanStatus( MbbStatus );

        if( WwanStatus != WWAN_STATUS_SUCCESS)
        {
            break;
        }

        if( MbbSmsSendStatus == NULL ||
            InBufferSize < sizeof(MBB_SMS_SEND_STATUS) )
        {
            TraceError( WMBCLASS_OID, "[MbbNdis][Sms][ReqId=0x%04x] INSUFFICIENT BufferSize for NDIS_STATUS_WWAN_SMS_SEND, BufferSize[Received=%d Expected=%d]",
                        Request->RequestId,
                        InBufferSize,
                        sizeof(MBB_SMS_SEND_STATUS)
                        );
            WwanStatus = WWAN_STATUS_FAILURE;
            break;
        }

        NdisSmsSendStatus.MessageReference = MbbSmsSendStatus->MessageReference;
    }
    while( FALSE );
    //
    // Restore the device returned operational status, if any.
    //
    if( WwanStatus != WWAN_STATUS_SUCCESS &&
        MbbUtilMbbToWwanStatus( MbbStatus ) != WWAN_STATUS_SUCCESS )
    {
        WwanStatus = MbbUtilMbbToWwanStatus( MbbStatus );
    }

    NdisSmsSendStatus.uStatus = WwanStatus;

    StatusIndication.StatusBuffer       = &NdisSmsSendStatus;
    StatusIndication.StatusBufferSize   = sizeof(NDIS_WWAN_SMS_SEND_STATUS);

    TraceInfo(  WMBCLASS_OID, "[MbbNdis][Sms][ReqId=0x%04x] Indicating NDIS_STATUS_WWAN_SMS_SEND with status=%!WwanStatus!",
                Request->RequestId,
                WwanStatus
                );

    MbbNdisMiniportIndicateRequestStatus(
        Request,
        &StatusIndication
        );


    return NDIS_STATUS_SUCCESS;
}

NDIS_STATUS
MbbNdisSmsDeleteStatusHandler(
    __in PMBB_REQUEST_CONTEXT               Request,
    __in NDIS_STATUS                        NdisStatus,
    __in MBB_STATUS                         MbbStatus,
    __in_bcount_opt(InBufferSize) PUCHAR    InBuffer,
    __in ULONG                              InBufferSize
    )
{
    WWAN_STATUS                 WwanStatus = WWAN_STATUS_FAILURE;
    NDIS_STATUS_INDICATION      StatusIndication;
    NDIS_WWAN_SMS_DELETE_STATUS NdisSmsDeleteStatus;

    do
    {
        RtlZeroMemory(&NdisSmsDeleteStatus, sizeof(NdisSmsDeleteStatus));

        MBB_INITIALIZE_REQUEST_NDIS_STATUS_INDICATION(
            &StatusIndication,
            Request,
            NDIS_STATUS_WWAN_SMS_DELETE
            );

        NdisSmsDeleteStatus.Header.Type      = NDIS_OBJECT_TYPE_DEFAULT;
        NdisSmsDeleteStatus.Header.Size      = sizeof(NDIS_WWAN_SMS_DELETE_STATUS);
        NdisSmsDeleteStatus.Header.Revision  = NDIS_WWAN_SMS_DELETE_STATUS_REVISION_1;

        if( NdisStatus != NDIS_STATUS_SUCCESS )
        {
            TraceError( WMBCLASS_OID, "[MbbNdis][Sms][ReqId=0x%04x] FAILED to get data for NDIS_STATUS_WWAN_SMS_DELETE, status=%!STATUS!",
                        Request->RequestId,
                        NdisStatus
                        );
            break;
        }

        WwanStatus = MbbUtilMbbToWwanStatus( MbbStatus );
    }
    while( FALSE );

    NdisSmsDeleteStatus.uStatus = WwanStatus;

    StatusIndication.StatusBuffer       = &NdisSmsDeleteStatus;
    StatusIndication.StatusBufferSize   = sizeof(NDIS_WWAN_SMS_DELETE_STATUS);

    TraceInfo(  WMBCLASS_OID, "[MbbNdis][Sms][ReqId=0x%04x] Indicating NDIS_STATUS_WWAN_SMS_DELETE with status=%!WwanStatus!",
                Request->RequestId,
                WwanStatus
                );
    MbbNdisMiniportIndicateRequestStatus(
        Request,
        &StatusIndication
        );

    return NDIS_STATUS_SUCCESS;
}

NDIS_STATUS
MbbNdisSmsStatusHandler(
    __in PMBB_REQUEST_CONTEXT               Request,
    __in NDIS_STATUS                        NdisStatus,
    __in MBB_STATUS                         MbbStatus,
    __in_bcount_opt(InBufferSize) PUCHAR    InBuffer,
    __in ULONG                              InBufferSize
    )
{
    WWAN_STATUS                 WwanStatus = WWAN_STATUS_FAILURE;
    NDIS_STATUS_INDICATION      StatusIndication;
    NDIS_WWAN_SMS_STATUS        NdisSmsStatus;
    PMBB_SMS_STATUS             MbbSmsStatus = (PMBB_SMS_STATUS)InBuffer;

    do
    {
        RtlZeroMemory(&NdisSmsStatus, sizeof(NdisSmsStatus));

        MBB_INITIALIZE_REQUEST_NDIS_STATUS_INDICATION(
            &StatusIndication,
            Request,
            NDIS_STATUS_WWAN_SMS_STATUS
            );

        NdisSmsStatus.Header.Type      = NDIS_OBJECT_TYPE_DEFAULT;
        NdisSmsStatus.Header.Size      = sizeof(NDIS_WWAN_SMS_STATUS);
        NdisSmsStatus.Header.Revision  = NDIS_WWAN_SMS_STATUS_REVISION_1;

        if( NdisStatus != NDIS_STATUS_SUCCESS )
        {
            TraceError( WMBCLASS_OID, "[MbbNdis][Sms][ReqId=0x%04x] FAILED to get data for NDIS_STATUS_WWAN_SMS_STATUS, status=%!STATUS!",
                        Request->RequestId,
                        NdisStatus
                        );
            break;
        }

        WwanStatus = MbbUtilMbbToWwanStatus( MbbStatus );

        if( WwanStatus != WWAN_STATUS_SUCCESS)
        {
            break;
        }

        if( MbbSmsStatus == NULL ||
            InBufferSize < sizeof(MBB_SMS_STATUS) )
        {
            TraceError( WMBCLASS_OID, "[MbbNdis][Sms][ReqId=0x%04x] INSUFFICIENT BufferSize for NDIS_STATUS_WWAN_SMS_STATUS, BufferSize[Received=%d Expected=%d]",
                        Request->RequestId,
                        InBufferSize,
                        sizeof(MBB_SMS_STATUS)
                        );
            WwanStatus = WWAN_STATUS_FAILURE;
            break;
        }

        MbbUtilMbbToWwanSmsStatus( MbbSmsStatus, &NdisSmsStatus.SmsStatus );
    }
    while( FALSE );
    //
    // Restore the device returned operational status, if any.
    //
    if( WwanStatus != WWAN_STATUS_SUCCESS &&
        MbbUtilMbbToWwanStatus( MbbStatus ) != WWAN_STATUS_SUCCESS )
    {
        WwanStatus = MbbUtilMbbToWwanStatus( MbbStatus );
    }

    NdisSmsStatus.uStatus = WwanStatus;

    StatusIndication.StatusBuffer       = &NdisSmsStatus;
    StatusIndication.StatusBufferSize   = sizeof(NDIS_WWAN_SMS_STATUS);

    TraceInfo(  WMBCLASS_OID, "[MbbNdis][Sms][ReqId=0x%04x] Indicating NDIS_STATUS_WWAN_SMS_STATUS with status=%!WwanStatus!",
                Request->RequestId,
                WwanStatus
                );

    MbbNdisMiniportIndicateRequestStatus(
        Request,
        &StatusIndication
        );

    return NDIS_STATUS_SUCCESS;
}

#if 0
NDIS_STATUS
MbbNdisGetStatsHandler(
    __in PMBB_REQUEST_CONTEXT               Request,
    __in NDIS_STATUS                        NdisStatus,
    __in MBB_STATUS                         MbbStatus,
    __in_bcount_opt(InBufferSize) PUCHAR    InBuffer,
    __in ULONG                              InBufferSize
    )

{
    PMINIPORT_ADAPTER_CONTEXT   Adapter = (PMINIPORT_ADAPTER_CONTEXT)(Request->RequestManager->AdapterContext);
    PNDIS_STATISTICS_INFO       Stats=NULL;
    PMBB_PACKET_STATISTICS      MbbStats=NULL;
    PCSTR                       OidName=NULL;
    ULONG                       OidNameLength=0;

    PNDIS_OID_REQUEST           NdisRequest=NULL;

    if ((NdisStatus != NDIS_STATUS_SUCCESS) || (MbbStatus != MBB_STATUS_SUCCESS) || (InBufferSize < sizeof(*MbbStats)))
    {
        TraceWarn(  WMBCLASS_OID, "[MbbNdis][ReqId=0x%04x] Device failed OID_GEN_STATISTICS",
                    Request->RequestId
                    );

    }

    NdisRequest=InterlockedExchangePointer(&Request->OidContext.OidRequest,(PNDIS_OID_REQUEST)-1);

    if (NdisRequest !=(PNDIS_OID_REQUEST)-1)
    {
        MBB_NDIS_OID_STATE OldOidState;

        //
        //  The oid has not been competed
        //

        Stats=(PNDIS_STATISTICS_INFO)NdisRequest->DATA.QUERY_INFORMATION.InformationBuffer;

        if (NdisStatus == NDIS_STATUS_SUCCESS)
        {

            RtlZeroMemory(Stats, sizeof(*Stats));

            Stats->Header.Type=NDIS_OBJECT_TYPE_DEFAULT;
            Stats->Header.Revision=NDIS_STATISTICS_INFO_REVISION_1;
            Stats->Header.Size=NDIS_SIZEOF_STATISTICS_INFO_REVISION_1;

            Stats->SupportedStatistics = NDIS_STATISTICS_FLAGS_VALID_BYTES_RCV |
                                         NDIS_STATISTICS_FLAGS_VALID_RCV_DISCARDS |
                                         NDIS_STATISTICS_FLAGS_VALID_RCV_ERROR |
                                         NDIS_STATISTICS_FLAGS_VALID_DIRECTED_FRAMES_RCV |
                                         NDIS_STATISTICS_FLAGS_VALID_BYTES_XMIT |
                                         NDIS_STATISTICS_FLAGS_VALID_XMIT_DISCARDS |
                                         NDIS_STATISTICS_FLAGS_VALID_DIRECTED_FRAMES_XMIT;


            if (InBufferSize >=  sizeof(*MbbStats))
            {

                MbbStats=(PMBB_PACKET_STATISTICS)InBuffer;

                Stats->ifHCInOctets = MbbStats->InOctets;
                Stats->ifInDiscards = MbbStats->InDiscards;
                Stats->ifInErrors   = MbbStats->InErrors;
                Stats->ifHCInUcastPkts = MbbStats->InPackets;

                Stats->ifHCOutOctets = MbbStats->OutOctets;
                Stats->ifOutDiscards = MbbStats->OutDiscards;
                Stats->ifOutErrors   = MbbStats->OutErrors;
                Stats->ifHCOutUcastPkts = MbbStats->OutPackets;

                NdisRequest->DATA.QUERY_INFORMATION.BytesWritten=sizeof(*Stats);
            }
            else
            {
                NdisStatus=NDIS_STATUS_FAILURE;
            }
        }

        if (NdisStatus != NDIS_STATUS_SUCCESS)
        {

            TraceInfo(  WMBCLASS_OID, "[MbbNdis][ReqId=0x%04x] Completing OID request to NDIS with status=%!status!",
                    Request->RequestId, NdisStatus
                    );


            RtlCopyMemory(Stats, &Adapter->Stats, sizeof(*Stats));

            Stats->Header.Type=NDIS_OBJECT_TYPE_DEFAULT;
            Stats->Header.Revision=NDIS_STATISTICS_INFO_REVISION_1;
            Stats->Header.Size=NDIS_SIZEOF_STATISTICS_INFO_REVISION_1;

            Stats->SupportedStatistics = NDIS_STATISTICS_FLAGS_VALID_BYTES_RCV |
                                         NDIS_STATISTICS_FLAGS_VALID_BYTES_XMIT |
                                         NDIS_STATISTICS_FLAGS_VALID_DIRECTED_FRAMES_RCV;


            NdisRequest->DATA.QUERY_INFORMATION.BytesWritten=sizeof(*Stats);

            NdisStatus = NDIS_STATUS_SUCCESS;

        }

        Request->OidContext.OidStatus = NdisStatus;
        //
        // Do not complete the OID request if the Oid handler is still running.
        // Defer the OID completion to the Oid handler
        //
        OldOidState = MbbReqMgrGetSetOidState(
                        Request,
                        MbbNdisOidStateComplete
                        );
        if( OldOidState == MbbNdisOidStatePending ||
            OldOidState == MbbNdisOidStateDispatched )
        {
            TraceInfo(  WMBCLASS_OID, "[MbbNdis][ReqId=0x%04x] Completing OID request in %!MbbOidState! to NDIS with status=%!status!",
                        Request->RequestId, OldOidState, NdisStatus
                        );
            OidName=GetOidName(NdisRequest->DATA.QUERY_INFORMATION.Oid);
            OidNameLength=strlen(OidName)+1;

            MbbWriteEvent(
                &REQUEST_COMPLETE_EVENT,
                NULL,
                NULL,
                10,
                &Adapter->TraceInstance,
                sizeof(Adapter->TraceInstance),
                &NdisRequest,
                sizeof(NdisRequest),
                &NdisRequest->DATA.QUERY_INFORMATION.Oid,
                sizeof(NdisRequest->DATA.QUERY_INFORMATION.Oid),
                (PVOID)OidName,
                OidNameLength,
                &NdisRequest->RequestId,
                sizeof(NdisRequest->RequestId),
                &NdisRequest->RequestHandle,
                sizeof(NdisRequest->RequestHandle),
                &NdisRequest->RequestType,
                sizeof(NdisRequest->RequestType),
                &NdisRequest->DATA.QUERY_INFORMATION.BytesWritten,
                sizeof(NdisRequest->DATA.QUERY_INFORMATION.BytesWritten),
                &NdisRequest->DATA.QUERY_INFORMATION.BytesNeeded,
                sizeof(NdisRequest->DATA.QUERY_INFORMATION.BytesNeeded),
                &NdisStatus,
                sizeof(NdisStatus)
                );
            NdisMOidRequestComplete(
                Adapter->MiniportAdapterHandle,
                NdisRequest,
                NdisStatus
                );
            NdisRequest=NULL;
        }
        else
        {
            TraceInfo(  WMBCLASS_OID, "[MbbNdis][ReqId=0x%04x] Deferring oid completion in %!MbbOidState!",
                        Request->RequestId, OldOidState
                        );
        }
    }
    else
    {
        TraceWarn(  WMBCLASS_OID, "[MbbNdis][ReqId=0x%04x] Oid already completed",
                    Request->RequestId
                    );


    }


    return NDIS_STATUS_SUCCESS;

}
#endif


// Device Services
#pragma warning(disable:6014)
NDIS_STATUS
MbbNdisDeviceServiceListStatusHandler(
    __in PMBB_REQUEST_CONTEXT               Request,
    __in NDIS_STATUS                        NdisStatus,
    __in MBB_STATUS                         MbbStatus,
    __in_bcount_opt(InBufferSize) PUCHAR    InBuffer,
    __in ULONG                              InBufferSize
    )
{
    ULONG                           ElementCount = 0;
    ULONG                           RequiredLength = 0;
    WWAN_STATUS                     WwanStatus = WWAN_STATUS_FAILURE;
    PMBB_DEVICE_SERVICES_HEADER     MbbDeviceServicesInfo = (PMBB_DEVICE_SERVICES_HEADER)InBuffer;
    PMBB_DEVICE_SERVICE_ELEMENT     MbbDeviceServiceElement = NULL;
    PMINIPORT_ADAPTER_CONTEXT       Adapter = NULL;
    PMBB_DS                         CachedDSInfo = NULL;
    ULONG                           i;
    NTSTATUS                        NtStatus = STATUS_SUCCESS;

    do
    {
        //
        // This function should only be called once when we initialize for caching
        // the supported device list. From that point on, we should only be using
        // the cached list (from MbbNdisEnumerateDeviceServices)
        //
        if( (MbbReqMgrIsInternalRequest( Request ) == FALSE) ||
            (MbbReqMgrIsUnsolicitedIndication( Request ) == TRUE) )
        {
            // Ignore unsolicited indications or stuff not due to internal requests
            TraceError( WMBCLASS_OID, "[MbbNdis][ReqId=0x%04x] UNEXPECTED DEVICE_SERVICES list notification",
                        Request->RequestId
                        );
            NdisStatus = NDIS_STATUS_NOT_ACCEPTED;
            break;
        }

        if( NdisStatus != NDIS_STATUS_SUCCESS )
        {
            TraceError( WMBCLASS_OID, "[MbbNdis][ReqId=0x%04x] FAILED to get data for NDIS_STATUS_WWAN_SUPPORTED_DEVICE_SERVICES, status=%!STATUS!",
                        Request->RequestId,
                        NdisStatus
                        );
            break;
        }

        //
        // There is no device service list on failure.
        //
        if( (WwanStatus = MbbUtilMbbToWwanStatus( MbbStatus )) != WWAN_STATUS_SUCCESS )
        {
            TraceError( WMBCLASS_OID, "[MbbNdis][ReqId=0x%04x] FAILED DeviceServices request MbbStatus=%!MbbStatus!",
                        Request->RequestId,
                        MbbStatus
                        );
            NdisStatus = NDIS_STATUS_INVALID_DATA;
            break;
        }
        if( MbbDeviceServicesInfo == NULL ||
            InBufferSize < RTL_SIZEOF_THROUGH_FIELD(MBB_DEVICE_SERVICES_HEADER, MaxDSSSessions) )
        {
            TraceError( WMBCLASS_OID, "[MbbNdis][ReqId=0x%04x] INSUFFICIENT BufferSize for MBB_DEVICE_SERVICES_HEADER, BufferSize[Received=%d Expected=%d]",
                        Request->RequestId,
                        InBufferSize,
                        RTL_SIZEOF_THROUGH_FIELD(MBB_DEVICE_SERVICES_HEADER, MaxDSSSessions)
                        );
            NdisStatus = NDIS_STATUS_INVALID_DATA;
            break;
        }

        //
        // Cache everything in the device services state
        //
        Adapter = ((PMINIPORT_ADAPTER_CONTEXT)MbbReqMgrGetAdapterHandle( Request ));

        ElementCount = MbbDeviceServicesInfo->DeviceServicesCount;

        if (ElementCount == 0)
        {
            TraceError( WMBCLASS_OID, "[MbbNdis][ReqId=0x%04x] Zero device services supported",
                        Request->RequestId
                        );

            Adapter->DeviceServiceState.ServicesList = NULL;
            Adapter->DeviceServiceState.ServicesCount = 0;
            Adapter->DeviceServiceState.MaxDSSSessions = 0;
            ASSERT(FALSE);

            NdisStatus = NDIS_STATUS_NOT_SUPPORTED;
            break;
        }

        // Not worrying about the CIDs yet. Those will be checked in a bit
        NtStatus = RtlULongMult(
            ElementCount,
            sizeof(MBB_DEVICE_SERVICE_ELEMENT),
            &RequiredLength
            );

        if ( NT_ERROR(NtStatus) )
        {
            TraceError( WMBCLASS_OID, "[MbbNdis][ReqId=0x%04x] Multiplication overflow occured, ElementCount=%d MBB_DEVICE_SERVICE_ELEMENT size=%d",
                        Request->RequestId,
                        ElementCount,
                        sizeof(MBB_DEVICE_SERVICE_ELEMENT)
                        );
            NdisStatus = NDIS_STATUS_INVALID_DATA;
            break;
        }

        NtStatus = RtlULongAdd(
            RequiredLength,
            FIELD_OFFSET(MBB_DEVICE_SERVICES_HEADER, ArrayElement),
            &RequiredLength
            );

        if ( NT_ERROR(NtStatus) )
        {
            TraceError( WMBCLASS_OID, "[MbbNdis][ReqId=0x%04x] Addition overflow occured, RequiredLength=%d FIELD_OFFSET=%d",
                        Request->RequestId,
                        RequiredLength,
                        FIELD_OFFSET(MBB_DEVICE_SERVICES_HEADER, ArrayElement)
                        );
            NdisStatus = NDIS_STATUS_INVALID_DATA;
            break;
        }

        if( InBufferSize < RequiredLength )
        {
            TraceError( WMBCLASS_OID, "[MbbNdis][ReqId=0x%04x] INSUFFICIENT BufferSize for MBB_DEVICE_SERVICES_HEADER, BufferSize[Received=%d Expected=%d] ServiceCount=%d",
                        Request->RequestId,
                        InBufferSize,
                        RequiredLength,
                        ElementCount
                        );
            NdisStatus = NDIS_STATUS_INVALID_DATA;
            break;
        }

        CachedDSInfo = ALLOCATE_NONPAGED_POOL(ElementCount * sizeof(MBB_DS));

        if ( CachedDSInfo == NULL )
        {
            TraceError( WMBCLASS_OID, "[MbbNdis][ReqId=0x%04x] FAILED to allocate %d bytes for MBB_DS list, ServiceCount=%d",
                        Request->RequestId,
                        ElementCount * sizeof(MBB_DS),
                        ElementCount
                        );
            NdisStatus = NDIS_STATUS_RESOURCES;
            break;
        }

        RtlZeroMemory(CachedDSInfo, ElementCount * sizeof(MBB_DS));

        // Presaved so that we can cleanup easier
        Adapter->DeviceServiceState.ServicesList = CachedDSInfo;
        Adapter->DeviceServiceState.ServicesCount = 0;  // Will be incremented as we add stuff in
        Adapter->DeviceServiceState.MaxDSSSessions = MbbDeviceServicesInfo->MaxDSSSessions;


        for (i = 0; i < ElementCount; i++)
        {


            NdisStatus=MbbIsVariableFieldValid(
                InBufferSize,
                MbbDeviceServicesInfo->ArrayElement[i].Offset,
                MbbDeviceServicesInfo->ArrayElement[i].Size,
                512,
                sizeof(ULONG)
                );

            if (NdisStatus != NDIS_STATUS_SUCCESS)
            {
                TraceError( WMBCLASS_OID, "[MbbNdis][ReqId=0x%04x] Array element %d is not valid", Request->RequestId, i);
                break;
            }

            MbbDeviceServiceElement = (PMBB_DEVICE_SERVICE_ELEMENT)(((PUCHAR)MbbDeviceServicesInfo) +
                                          MbbDeviceServicesInfo->ArrayElement[i].Offset);

            RequiredLength = FIELD_OFFSET(MBB_DEVICE_SERVICE_ELEMENT, CIDList) +
                                    MbbDeviceServiceElement->CIDCount * sizeof(ULONG);

            if ( InBufferSize < RequiredLength + MbbDeviceServicesInfo->ArrayElement[i].Offset)
            {
                TraceError( WMBCLASS_OID, "[MbbNdis][ReqId=0x%04x] INSUFFICIENT remaining size for MBB_DEVICE_SERVICE_HEADER, BufferSize[Received=%d Expected=%d] ServiceIndex=%d",
                            Request->RequestId,
                            InBufferSize,
                            RequiredLength,
                            i
                            );

                NdisStatus = NDIS_STATUS_INVALID_DATA;
                break;
            }


            //
            // Copy it while swapping order
            //
            MBB_UUID_TO_HOST(&(CachedDSInfo[i].DeviceServiceId),
                &MbbDeviceServiceElement->DeviceServiceId
                );

            //
            // This is a direct mapping
            //
            CachedDSInfo[i].DSSCapability = MbbDeviceServiceElement->DSSPayload;

            CachedDSInfo[i].MaxDSSInstances = MbbDeviceServiceElement->MaxDSSInstances;

            if (MbbDeviceServiceElement->CIDCount != 0)
            {
                CachedDSInfo[i].CIDList = ALLOCATE_NONPAGED_POOL(MbbDeviceServiceElement->CIDCount * sizeof(ULONG));

                if( CachedDSInfo[i].CIDList == NULL )
                {
                    TraceError( WMBCLASS_OID, "[MbbNdis][ReqId=0x%04x] FAILED to allocate %d bytes for CID list, ServiceIndex=%d",
                                Request->RequestId,
                                MbbDeviceServiceElement->CIDCount * sizeof(ULONG),
                                i
                                );
                    NdisStatus = NDIS_STATUS_RESOURCES;
                    break;
                }

                //
                // Copy the CID list
                //
                CachedDSInfo[i].CIDCount = MbbDeviceServiceElement->CIDCount;

                RtlCopyMemory(CachedDSInfo[i].CIDList,
                    MbbDeviceServiceElement->CIDList,
                    MbbDeviceServiceElement->CIDCount * sizeof(ULONG)
                    );
            }

            //
            // One more entry added
            //
            Adapter->DeviceServiceState.ServicesCount++;

        }
        if (NdisStatus != NDIS_STATUS_SUCCESS)
        {
            break;
        }
    }
    while( FALSE );

    if (NdisStatus != NDIS_STATUS_SUCCESS)
    {
        if (Adapter != NULL)
        {
            // Free anything that we have ended up allocating
            FreeDeviceServiceState(&(Adapter->DeviceServiceState));
        }
    }

    return NdisStatus;
}

NDIS_STATUS
MbbNdisDeviceServiceResponseStatusHandler(
    __in PMBB_REQUEST_CONTEXT               Request,
    __in NDIS_STATUS                        NdisStatus,
    __in MBB_STATUS                         MbbStatus,
    __in_bcount_opt(InBufferSize) PUCHAR    InBuffer,
    __in ULONG                              InBufferSize
    )
{
    WWAN_STATUS                         WwanStatus = WWAN_STATUS_FAILURE;
    NDIS_STATUS_INDICATION              StatusIndication;
    ULONG                               NdisDeviceServiceResponseSize = 0;
    NDIS_WWAN_DEVICE_SERVICE_RESPONSE   LocalNdisDeviceServiceResponse;
    PNDIS_WWAN_DEVICE_SERVICE_RESPONSE  NdisDeviceServiceResponse = NULL;

    do
    {
        RtlZeroMemory(&LocalNdisDeviceServiceResponse, sizeof(LocalNdisDeviceServiceResponse));

        MBB_INITIALIZE_REQUEST_NDIS_STATUS_INDICATION(
            &StatusIndication,
            Request,
            NDIS_STATUS_WWAN_DEVICE_SERVICE_RESPONSE
            );

        if( NdisStatus != NDIS_STATUS_SUCCESS )
        {
            TraceError( WMBCLASS_OID, "[MbbNdis][Ven][ReqId=0x%04x] FAILED to get data for NDIS_STATUS_WWAN_DEVICE_SERVICE_RESPONSE, status=%!STATUS!",
                        Request->RequestId,
                        NdisStatus
                        );
            break;
        }

        WwanStatus = MbbUtilMbbToWwanStatus( MbbStatus );

        if( WwanStatus != WWAN_STATUS_SUCCESS )
        {
            break;
        }


        if (InBufferSize > (ULONG_MAX -  sizeof(NDIS_WWAN_DEVICE_SERVICE_RESPONSE)))
        {
            TraceError( WMBCLASS_OID, "[MbbNdis][Ven][ReqId=0x%04x] Device service response too big for NDIS_STATUS_WWAN_DEVICE_SERVICE_RESPONSE, VendorDataSize=%dBytes",
                        Request->RequestId,
                        InBufferSize

                        );
            WwanStatus = WWAN_STATUS_FAILURE;
            break;
        }

        NdisDeviceServiceResponseSize = sizeof(NDIS_WWAN_DEVICE_SERVICE_RESPONSE) + InBufferSize;

        if( (NdisDeviceServiceResponse = (PNDIS_WWAN_DEVICE_SERVICE_RESPONSE) ALLOCATE_NONPAGED_POOL(NdisDeviceServiceResponseSize)) == NULL )
        {
            TraceError( WMBCLASS_OID, "[MbbNdis][Ven][ReqId=0x%04x] FAILED to allocate memory for NDIS_STATUS_WWAN_DEVICE_SERVICE_RESPONSE, VendorDataSize=%dBytes",
                        Request->RequestId,
                        InBufferSize
                        );

            WwanStatus = WWAN_STATUS_FAILURE;
            break;
        }

        NdisDeviceServiceResponse->Header.Type      = NDIS_OBJECT_TYPE_DEFAULT;
        NdisDeviceServiceResponse->Header.Size      = SIZEOF_NDIS_WWAN_DEVICE_SERVICE_RESPONSE_1;
        NdisDeviceServiceResponse->Header.Revision  = NDIS_WWAN_DEVICE_SERVICE_RESPONSE_REVISION_1;

        NdisDeviceServiceResponse->Response.DeviceServiceGuid   = Request->HandlerContext.Response.Command.ServiceId;
        NdisDeviceServiceResponse->Response.ResponseID          = Request->HandlerContext.Response.Command.CommandId;
        NdisDeviceServiceResponse->Response.uDataSize           = InBufferSize;

        // No fragmentation support

        if( InBufferSize != 0 )
        {
            RtlCopyMemory(
                NdisDeviceServiceResponse + 1,
                InBuffer,
                InBufferSize
                );
        }
    }
    while( FALSE );
    //
    // Restore the device returned operational status, if any.
    //
    if( WwanStatus != WWAN_STATUS_SUCCESS &&
        MbbUtilMbbToWwanStatus( MbbStatus ) != WWAN_STATUS_SUCCESS )
    {
        WwanStatus = MbbUtilMbbToWwanStatus( MbbStatus );
    }

    if( NdisDeviceServiceResponse != NULL )
    {
        StatusIndication.StatusBuffer       = NdisDeviceServiceResponse;
        StatusIndication.StatusBufferSize   = NdisDeviceServiceResponseSize;
    }
    else
    {
        LocalNdisDeviceServiceResponse.Header.Type      = NDIS_OBJECT_TYPE_DEFAULT;
        LocalNdisDeviceServiceResponse.Header.Size      = SIZEOF_NDIS_WWAN_DEVICE_SERVICE_RESPONSE_1;
        LocalNdisDeviceServiceResponse.Header.Revision  = NDIS_WWAN_DEVICE_SERVICE_RESPONSE_REVISION_1;

        LocalNdisDeviceServiceResponse.Response.DeviceServiceGuid   = Request->HandlerContext.Response.Command.ServiceId;
        LocalNdisDeviceServiceResponse.Response.ResponseID          = Request->HandlerContext.Response.Command.CommandId;
        LocalNdisDeviceServiceResponse.Response.uDataSize           = 0;

        StatusIndication.StatusBuffer       = &LocalNdisDeviceServiceResponse;
        StatusIndication.StatusBufferSize   = sizeof(NDIS_WWAN_DEVICE_SERVICE_RESPONSE);
    }
    ((PNDIS_WWAN_DEVICE_SERVICE_RESPONSE)(StatusIndication.StatusBuffer))->uStatus = WwanStatus;

    TraceInfo(  WMBCLASS_OID, "[MbbNdis][Ven][ReqId=0x%04x] Indicating NDIS_STATUS_WWAN_DEVICE_SERVICE_RESPONSE with status=%!WwanStatus!",
                Request->RequestId,
                WwanStatus
                );

    MbbNdisMiniportIndicateRequestStatus(
        Request,
        &StatusIndication
        );

    if( NdisDeviceServiceResponse != NULL )
    {
        FREE_POOL( NdisDeviceServiceResponse );
    }
    return (WwanStatus == WWAN_STATUS_SMS_MORE_DATA)? NDIS_STATUS_PENDING: NDIS_STATUS_SUCCESS;
}

NDIS_STATUS
MbbNdisDeviceServiceEventStatusHandler(
    __in PMBB_REQUEST_CONTEXT               Request,
    __in NDIS_STATUS                        NdisStatus,
    __in MBB_STATUS                         MbbStatus,
    __in_bcount_opt(InBufferSize) PUCHAR    InBuffer,
    __in ULONG                              InBufferSize
    )
{
    WWAN_STATUS                         WwanStatus = WWAN_STATUS_FAILURE;
    NDIS_STATUS_INDICATION              StatusIndication;
    ULONG                               NdisDeviceServiceEventSize;
    PNDIS_WWAN_DEVICE_SERVICE_EVENT     NdisDeviceServiceEvent = NULL;
    NTSTATUS                            NtStatus = STATUS_SUCCESS;

    do
    {


        MBB_INITIALIZE_REQUEST_NDIS_STATUS_INDICATION(
            &StatusIndication,
            Request,
            NDIS_STATUS_WWAN_DEVICE_SERVICE_EVENT
            );

        if( NdisStatus != NDIS_STATUS_SUCCESS )
        {
            TraceError( WMBCLASS_OID, "[MbbNdis][Ven][ReqId=0x%04x] FAILED to get data for NDIS_STATUS_WWAN_DEVICE_SERVICE_EVENT, status=%!STATUS!",
                        Request->RequestId,
                        NdisStatus
                        );
            break;
        }

        NtStatus = RtlULongAdd(
            InBufferSize,
            sizeof(NDIS_WWAN_DEVICE_SERVICE_EVENT),
            &NdisDeviceServiceEventSize
            );
        if ( NT_ERROR(NtStatus) )
        {
            TraceError( WMBCLASS_OID, "[MbbNdis][Ven][ReqId=0x%04x] Addition overflow occured, InBufferSize=%d NDIS_WWAN_DEVICE_SERVICE_EVENT size=%d",
                        Request->RequestId,
                        InBufferSize,
                        sizeof(NDIS_WWAN_DEVICE_SERVICE_EVENT)
                        );
            break;
        }

        if( (NdisDeviceServiceEvent = (PNDIS_WWAN_DEVICE_SERVICE_EVENT) ALLOCATE_NONPAGED_POOL(NdisDeviceServiceEventSize)) == NULL )
        {
            TraceError( WMBCLASS_OID, "[MbbNdis][Ven][ReqId=0x%04x] FAILED to allocate memory for NDIS_STATUS_WWAN_DEVICE_SERVICE_EVENT, VendorDataSize=%dBytes",
                        Request->RequestId,
                        InBufferSize
                        );
            break;
        }
        WwanStatus = MbbUtilMbbToWwanStatus( MbbStatus );

        NdisDeviceServiceEvent->Header.Type      = NDIS_OBJECT_TYPE_DEFAULT;
        NdisDeviceServiceEvent->Header.Size      = SIZEOF_NDIS_WWAN_DEVICE_SERVICE_EVENT_1;
        NdisDeviceServiceEvent->Header.Revision  = NDIS_WWAN_DEVICE_SERVICE_EVENT_REVISION_1;

        NdisDeviceServiceEvent->Event.DeviceServiceGuid   = Request->HandlerContext.Response.Command.ServiceId;
        NdisDeviceServiceEvent->Event.EventID             = Request->HandlerContext.Response.Command.CommandId;
        NdisDeviceServiceEvent->Event.uDataSize           = InBufferSize;

        if( InBufferSize != 0 )
        {
            RtlCopyMemory(
                NdisDeviceServiceEvent + 1,
                InBuffer,
                InBufferSize
                );
        }

        StatusIndication.StatusBuffer       = NdisDeviceServiceEvent;
        StatusIndication.StatusBufferSize   = NdisDeviceServiceEventSize;

        TraceInfo(  WMBCLASS_OID, "[MbbNdis][Ven][ReqId=0x%04x] Indicating NDIS_STATUS_WWAN_DEVICE_SERVICE_EVENT",
                    Request->RequestId
                    );

        MbbNdisMiniportIndicateRequestStatus(
            Request,
            &StatusIndication
            );

    }
    while( FALSE );
    //
    // Restore the device returned operational status, if any.
    //
    if( WwanStatus != WWAN_STATUS_SUCCESS &&
        MbbUtilMbbToWwanStatus( MbbStatus ) != WWAN_STATUS_SUCCESS )
    {
        WwanStatus = MbbUtilMbbToWwanStatus( MbbStatus );
    }


    if( NdisDeviceServiceEvent != NULL )
    {
        FREE_POOL( NdisDeviceServiceEvent );
    }
    return (WwanStatus == WWAN_STATUS_SMS_MORE_DATA)? NDIS_STATUS_PENDING: NDIS_STATUS_SUCCESS;
}

NDIS_STATUS
MbbNdisDeviceServiceStatusHandler(
    __in PMBB_REQUEST_CONTEXT               Request,
    __in NDIS_STATUS                        NdisStatus,
    __in MBB_STATUS                         MbbStatus,
    __in_bcount_opt(InBufferSize) PUCHAR    InBuffer,
    __in ULONG                              InBufferSize
    )
{
    if (!MbbUtilIsValidDeviceService(&(Request->HandlerContext.Response.Command.ServiceId)))
    {
        TraceError(  WMBCLASS_OID, "[MbbNdis][Ven][ReqId=0x%04x] Dropping indication with invalid device service %!GUID!",
                    Request->RequestId,
                    &(Request->HandlerContext.Response.Command.ServiceId)
                    );
        return NDIS_STATUS_INVALID_DATA;
    }

    if (MbbReqMgrIsUnsolicitedIndication(Request))
    {
        // This is an event
        return MbbNdisDeviceServiceEventStatusHandler(Request,
                NdisStatus,
                MbbStatus,
                InBuffer,
                InBufferSize
                );
    }
    else
    {
        // This is a response
        return MbbNdisDeviceServiceResponseStatusHandler(Request,
                NdisStatus,
                MbbStatus,
                InBuffer,
                InBufferSize
                );
    }
}

NDIS_STATUS
MbbNdisVendorSpecificStatusHandler(
    __in PMBB_REQUEST_CONTEXT               Request,
    __in NDIS_STATUS                        NdisStatus,
    __in MBB_STATUS                         MbbStatus,
    __in_bcount_opt(InBufferSize) PUCHAR    InBuffer,
    __in ULONG                              InBufferSize
    )
{
    WWAN_STATUS                         WwanStatus = WWAN_STATUS_FAILURE;
    NDIS_STATUS_INDICATION              StatusIndication;
    ULONG                               NdisVendorSpecificResponseSize = 0;
    NDIS_WWAN_VENDOR_SPECIFIC           LocalNdisVendorSpecific;
    PNDIS_WWAN_VENDOR_SPECIFIC          NdisVendorSpecific = NULL;

    do
    {
        RtlZeroMemory(&LocalNdisVendorSpecific, sizeof(LocalNdisVendorSpecific));

        MBB_INITIALIZE_REQUEST_NDIS_STATUS_INDICATION(
            &StatusIndication,
            Request,
            NDIS_STATUS_WWAN_VENDOR_SPECIFIC
            );

        if( NdisStatus != NDIS_STATUS_SUCCESS )
        {
            TraceError( WMBCLASS_OID, "[MbbNdis][Ven][ReqId=0x%04x] FAILED to get data for NDIS_STATUS_WWAN_VENDOR_SPECIFIC, status=%!STATUS!",
                        Request->RequestId,
                        NdisStatus
                        );
            break;
        }

        WwanStatus = MbbUtilMbbToWwanStatus( MbbStatus );

        if( WwanStatus != WWAN_STATUS_SUCCESS )
        {
            break;
        }


        if (InBufferSize > (ULONG_MAX - sizeof(NDIS_WWAN_VENDOR_SPECIFIC)))
        {
            TraceError( WMBCLASS_OID, "[MbbNdis][Ven][ReqId=0x%04x] Vendor specific response too big for NDIS_STATUS_WWAN_VENDOR_SPECIFIC, VendorDataSize=%dBytes",
                        Request->RequestId,
                        InBufferSize

                        );
            WwanStatus = WWAN_STATUS_FAILURE;
            break;
        }

        NdisVendorSpecificResponseSize = sizeof(NDIS_WWAN_VENDOR_SPECIFIC) + InBufferSize;

        if( (NdisVendorSpecific = (PNDIS_WWAN_VENDOR_SPECIFIC) ALLOCATE_NONPAGED_POOL(NdisVendorSpecificResponseSize)) == NULL )
        {
            TraceError( WMBCLASS_OID, "[MbbNdis][Ven][ReqId=0x%04x] FAILED to allocate memory for NDIS_STATUS_WWAN_VENDOR_SPECIFIC, VendorDataSize=%dBytes",
                        Request->RequestId,
                        InBufferSize
                        );

            WwanStatus = WWAN_STATUS_FAILURE;
            break;
        }

        NdisVendorSpecific->Header.Type      = NDIS_OBJECT_TYPE_DEFAULT;
        NdisVendorSpecific->Header.Size      = sizeof(NDIS_WWAN_VENDOR_SPECIFIC);
        NdisVendorSpecific->Header.Revision  = NDIS_WWAN_VENDOR_SPECIFIC_REVISION_1;

        NdisVendorSpecific->VendorSpecificData.uVendorSpecificBufferSize = InBufferSize;

        if( InBufferSize != 0 )
        {
            RtlCopyMemory(
                NdisVendorSpecific + 1,
                InBuffer,
                InBufferSize
                );
        }
    }
    while( FALSE );

    //
    // Restore the device returned operational status, if any.
    //
    if( WwanStatus != WWAN_STATUS_SUCCESS &&
        MbbUtilMbbToWwanStatus( MbbStatus ) != WWAN_STATUS_SUCCESS )
    {
        WwanStatus = MbbUtilMbbToWwanStatus( MbbStatus );
    }

    if( NdisVendorSpecific != NULL )
    {
        StatusIndication.StatusBuffer       = NdisVendorSpecific;
        StatusIndication.StatusBufferSize   = NdisVendorSpecificResponseSize;
    }
    else
    {
        LocalNdisVendorSpecific.Header.Type      = NDIS_OBJECT_TYPE_DEFAULT;
        LocalNdisVendorSpecific.Header.Size      = sizeof(NDIS_WWAN_VENDOR_SPECIFIC);
        LocalNdisVendorSpecific.Header.Revision  = NDIS_WWAN_VENDOR_SPECIFIC_REVISION_1;

        LocalNdisVendorSpecific.VendorSpecificData.uVendorSpecificBufferSize = 0;

        StatusIndication.StatusBuffer       = &LocalNdisVendorSpecific;
        StatusIndication.StatusBufferSize   = sizeof(NDIS_WWAN_VENDOR_SPECIFIC);
    }

    // WwanStatus is only used for trace purpose. NDIS_WWAN_VENDOR_SPECIFIC doesn't have uStaus field.
    TraceInfo(  WMBCLASS_OID, "[MbbNdis][Ven][ReqId=0x%04x] Indicating NDIS_STATUS_WWAN_VENDOR_SPECIFIC with status=%!WwanStatus!",
                Request->RequestId,
                WwanStatus
                );

    MbbNdisMiniportIndicateRequestStatus(
        Request,
        &StatusIndication
        );

    if( NdisVendorSpecific != NULL )
    {
        FREE_POOL( NdisVendorSpecific );
    }
    return NDIS_STATUS_SUCCESS;
}

NDIS_STATUS
MbbNdisSubscribeDeviceServiceEventsStatusHandler(
    __in PMBB_REQUEST_CONTEXT               Request,
    __in NDIS_STATUS                        NdisStatus,
    __in MBB_STATUS                         MbbStatus,
    __in_bcount_opt(InBufferSize) PUCHAR    InBuffer,
    __in ULONG                              InBufferSize
    )
{
    WWAN_STATUS                             WwanStatus = WWAN_STATUS_FAILURE;
    NDIS_STATUS_INDICATION                  StatusIndication;
    NDIS_WWAN_DEVICE_SERVICE_SUBSCRIPTION   NdisDeviceServiceSubscription;
    PMINIPORT_ADAPTER_CONTEXT   Adapter = NULL;

    do
    {
        if( NdisStatus != NDIS_STATUS_SUCCESS )
        {
            TraceError( WMBCLASS_OID, "[MbbNdis][Ven][ReqId=0x%04x] FAILED to get data for NDIS_STATUS_WWAN_DEVICE_SERVICE_SUBSCRIPTION, status=%!STATUS!",
                        Request->RequestId,
                        NdisStatus
                        );

            WwanStatus = WWAN_STATUS_FAILURE;
            break;
        }

        WwanStatus = MbbUtilMbbToWwanStatus( MbbStatus );

        //
        // We will be sending the notification up.
        // If this is a set requested by the OS and the request
        // succeeded, persist the radio state in registry and locally
        //
        if ((Request->OidContext.OidRequestId != 0) &&
            (MbbReqMgrIsSetOid( Request )) &&
            (WwanStatus == WWAN_STATUS_SUCCESS) )
        {
            //
            // OS initiated SET request that succeeded. The subscription
            // list from the OS has been persisted in DataToFreeOnResponse
            // Save it
            //
            Adapter = (PMINIPORT_ADAPTER_CONTEXT)MbbReqMgrGetAdapterHandle(Request);

            MbbAdapterLock(Adapter);
            if (Adapter->DeviceServiceState.ExtSubscribeList != NULL)
                FREE_POOL( Adapter->DeviceServiceState.ExtSubscribeList );

            Adapter->DeviceServiceState.ExtSubscribeList =
                Request->HandlerContext.Parameters.EventSubscribe.ExtList;
            Adapter->DeviceServiceState.ExtSubscribeListBufferSize =
                Request->HandlerContext.Parameters.EventSubscribe.ExtSize;

            MbbAdapterUnlock(Adapter);

            // We have taken ownership of the pointer
            Request->HandlerContext.DataToFreeOnResponse = NULL;
        }

    }
    while (FALSE);

    //
    // If this is for an OS request, send the notification up
    //
    if( (MbbReqMgrIsInternalRequest( Request ) == FALSE))
    {
        MBB_INITIALIZE_REQUEST_NDIS_STATUS_INDICATION(
            &StatusIndication,
            Request,
            NDIS_STATUS_WWAN_DEVICE_SERVICE_SUBSCRIPTION
            );

        NdisDeviceServiceSubscription.Header.Type      = NDIS_OBJECT_TYPE_DEFAULT;
        NdisDeviceServiceSubscription.Header.Size      = SIZEOF_NDIS_WWAN_DEVICE_SERVICE_SUBSCRIPTION_1;
        NdisDeviceServiceSubscription.Header.Revision  = NDIS_WWAN_DEVICE_SERVICE_SUBSCRIPTION_REVISION_1;

        NdisDeviceServiceSubscription.DeviceServiceListHeader.ElementType  = WwanStructDeviceServiceGuid;
        NdisDeviceServiceSubscription.DeviceServiceListHeader.ElementCount = 0;


        NdisDeviceServiceSubscription.uStatus = WwanStatus;

        StatusIndication.StatusBuffer       = &NdisDeviceServiceSubscription;
        StatusIndication.StatusBufferSize   = sizeof(NDIS_WWAN_DEVICE_SERVICE_SUBSCRIPTION);

        TraceInfo(  WMBCLASS_OID, "[MbbNdis][Ven][ReqId=0x%04x] Indicating NDIS_STATUS_WWAN_DEVICE_SERVICE_SUBSCRIPTION with status=%!WwanStatus!",
                    Request->RequestId,
                    WwanStatus
                    );
        MbbNdisMiniportIndicateRequestStatus(
            Request,
            &StatusIndication
            );

    }

    return NdisStatus;
}


NDIS_STATUS
MbbNdisSetUSSD(
    __in PMBB_REQUEST_CONTEXT               Request,
    __in_bcount_opt(*InBufferSize) PUCHAR   InBuffer,
    __in PULONG                             InBufferSize
    )
{
    PMBB_SET_USSD              MbbSetUSSD;
    PNDIS_WWAN_USSD_REQUEST    NdisUSSDRequest  = (PNDIS_WWAN_USSD_REQUEST)InBuffer;
    NDIS_STATUS                Status;
    ULONG                      MbbSetUSSDSize;
    ULONG                      UssdStringLengthRounded;
    //
    // Size is already verified in MbbNdisMiniportOidRequest( )
    //
    __analysis_assume( *InBufferSize >= sizeof(NDIS_WWAN_USSD_REQUEST) );
    *InBufferSize = sizeof(NDIS_WWAN_USSD_REQUEST);
    UssdStringLengthRounded = NdisUSSDRequest->UssdRequest.UssdString.StringLength;
    UssdStringLengthRounded = ROUND_UP_COUNT( UssdStringLengthRounded, ALIGN_DWORD );
    //
    // Both lengths are checked to detect overflow.
    //
    if( NdisUSSDRequest->UssdRequest.UssdString.StringLength > MBB_USSD_STRING_LEN_MAX ||
        UssdStringLengthRounded > MBB_USSD_STRING_LEN_MAX )
    {
           TraceError( WMBCLASS_OID, "[MbbNdis] USSD string too long: 0x%x",
                       NdisUSSDRequest->UssdRequest.UssdString.StringLength
                      );
           Status = NDIS_STATUS_INVALID_LENGTH;
           return Status;
    }

    //determine length of buffer
    MbbSetUSSDSize  = FIELD_OFFSET(MBB_SET_USSD, DataBuffer);
    MbbSetUSSDSize  = ROUND_UP_COUNT( MbbSetUSSDSize, ALIGN_DWORD );
    MbbSetUSSDSize += UssdStringLengthRounded;

    if( (MbbSetUSSD = (PMBB_SET_USSD) ALLOCATE_NONPAGED_POOL( MbbSetUSSDSize )) == NULL )
    {
        return NDIS_STATUS_RESOURCES;
    }
    Request->HandlerContext.DataToFreeOnCompletion = MbbSetUSSD;

    RtlZeroMemory( MbbSetUSSD, MbbSetUSSDSize );

    Status = MbbUtilWwanToMbbSetUSSD(
                MbbSetUSSD,
                &NdisUSSDRequest->UssdRequest
                );
    if (Status != NDIS_STATUS_SUCCESS){
        return Status;
    }


    return MbbUtilSetAttributeWithParameter(
                Request,
                (PCHAR)MbbSetUSSD,
                MbbSetUSSDSize
                );
}



NDIS_STATUS
MbbNdisUSSDStatusHandler(
    __in PMBB_REQUEST_CONTEXT               Request,
    __in NDIS_STATUS                        NdisStatus,
    __in MBB_STATUS                         MbbStatus,
    __in_bcount_opt(InBufferSize) PUCHAR    InBuffer,
    __in ULONG                              InBufferSize
    )
{
    WWAN_STATUS             WwanStatus = WWAN_STATUS_FAILURE;
    NDIS_STATUS_INDICATION  StatusIndication;
    NDIS_WWAN_USSD_EVENT    NdisWWanUSSDEvt;
    PMBB_USSD               MbbUSSDResponse = (PMBB_USSD)InBuffer;
    NDIS_STATUS             LocalNdisStatus = NDIS_STATUS_SUCCESS;

    do
    {

        RtlZeroMemory(&NdisWWanUSSDEvt, sizeof(NdisWWanUSSDEvt));

        MBB_INITIALIZE_REQUEST_NDIS_STATUS_INDICATION(
            &StatusIndication,
            Request,
            NDIS_STATUS_WWAN_USSD
            );

        NdisWWanUSSDEvt.Header.Type      = NDIS_OBJECT_TYPE_DEFAULT;
        NdisWWanUSSDEvt.Header.Size      = sizeof(NDIS_WWAN_USSD_EVENT);
        NdisWWanUSSDEvt.Header.Revision  = NDIS_WWAN_USSD_EVENT_REVISION_1;

        if( NdisStatus != NDIS_STATUS_SUCCESS )
        {
            TraceError( WMBCLASS_OID, "[MbbNdis][ReqId=0x%04x] FAILED to get data for NDIS_STATUS_WWAN_USSD, status=%!STATUS!",
                        Request->RequestId,
                        NdisStatus
                        );
            break;
        }

        WwanStatus = MbbUtilMbbToWwanStatus( MbbStatus );
        //
        // If the device returned MBIM error status and didnt provide any data
        // propagate the error code to the service. Sometimes data can be sent
        // by the device even if the device reports error.
        //
        if ( (WwanStatus != WWAN_STATUS_SUCCESS) &&
            InBufferSize < (ULONG)FIELD_OFFSET(MBB_USSD,DataBuffer)  )
        {
            TraceError( WMBCLASS_OID, "[MbbNdis][ReqId=0x%04x] Device FAILED USSD request, status=%!MbbStatus!",
                        Request->RequestId,
                        MbbStatus
                        );
            break;
        }

        //
        //  the request succeeded, or if failed and returned a minimal valid buffer
        //

        //
        // Check for min or max data size from device
        //
        if( MbbUSSDResponse == NULL
                            ||
            InBufferSize < (ULONG)FIELD_OFFSET(MBB_USSD,DataBuffer)

          )
        {

            TraceError( WMBCLASS_OID, "[MbbNdis][ReqId=0x%04x] Insufficient BufferSize for NDIS_STATUS_WWAN_USSD, BufferSize[Received=%d Min Expected=%d",
                        Request->RequestId,
                        InBufferSize,
                        FIELD_OFFSET(MBB_USSD,DataBuffer)
                        );
            WwanStatus = WWAN_STATUS_FAILURE;
            break;
        }

        NdisStatus=MbbIsVariableFieldValid(
            InBufferSize,
            MbbUSSDResponse->USSDPayload.Offset,
            MbbUSSDResponse->USSDPayload.Size,
            MBB_USSD_STRING_LEN_MAX,
            sizeof(UCHAR)
            );

        if (NdisStatus != NDIS_STATUS_SUCCESS)
        {
            TraceError( WMBCLASS_OID, "[MbbNdis][ReqId=0x%04x] Incorrect BufferSize for NDIS_STATUS_WWAN_USSD, BufferSize[Received=%d Min Expected=%d payload=%d",
                        Request->RequestId,
                        InBufferSize,
                        FIELD_OFFSET(MBB_USSD,DataBuffer),
                        MbbUSSDResponse->USSDPayload.Size
                        );

            WwanStatus = WWAN_STATUS_FAILURE;
            break;

        }


        LocalNdisStatus =  MbbUtilMbbToWwanUSSDEvent(
                            MbbUSSDResponse,
                            &NdisWWanUSSDEvt.UssdEvent
                            );

        if( LocalNdisStatus != NDIS_STATUS_SUCCESS )
        {
            WwanStatus = WWAN_STATUS_FAILURE;
            break;
        }

    }
    while( FALSE );

    NdisWWanUSSDEvt.uStatus = WwanStatus;

    StatusIndication.StatusBuffer       = &NdisWWanUSSDEvt;
    StatusIndication.StatusBufferSize   = sizeof(NDIS_WWAN_USSD_EVENT);

    TraceInfo(  WMBCLASS_OID, "[MbbNdis][ReqId=0x%04x] Indicating NDIS_STATUS_WWAN_USSD with status=%!WwanStatus!",
                Request->RequestId,
                WwanStatus
                );

    MbbNdisMiniportIndicateRequestStatus(
        Request,
        &StatusIndication
        );

    return NDIS_STATUS_SUCCESS;
}

NDIS_STATUS
MbbNdisGetAuthChallenge(
    __in PMBB_REQUEST_CONTEXT                                   Request,
    __in_bcount_opt(*InBufferSize) PUCHAR                       InBuffer,
    __in PULONG                                                 InBufferSize,
    __out_bcount_part_opt(*OutBufferSize,*OutBufferSize) PUCHAR OutBuffer,
    __inout PULONG                                              OutBufferSize
    )
{
    MBB_SIM_AUTH_CHALLENGE      MbbSimAuthChallenge;
    MBB_AKA_AUTH_CHALLENGE      MbbAKAAuthChallenge;
    PMBB_AKAP_AUTH_CHALLENGE    MbbAKAPAuthChallenge;

    PNDIS_WWAN_AUTH_CHALLENGE   NdisAuthRequest  = (PNDIS_WWAN_AUTH_CHALLENGE)InBuffer;
    NDIS_STATUS                Status;

    //
    // Size is already verified in MbbNdisMiniportOidRequest( )
    //
    __analysis_assume( *InBufferSize >= sizeof(NDIS_WWAN_AUTH_CHALLENGE) );
    *InBufferSize = sizeof(NDIS_WWAN_AUTH_CHALLENGE);

    if(NdisAuthRequest->AuthChallenge.AuthMethod == WwanAuthAka){
        RtlCopyMemory(  &MbbAKAAuthChallenge,
                        &NdisAuthRequest->AuthChallenge.u.AuthAka,
                        sizeof(MbbAKAAuthChallenge) );

        return MbbUtilQueryAttributeWithParameter(
                    Request,
                    (PCHAR)(&MbbAKAAuthChallenge),
                    sizeof(MbbAKAAuthChallenge)
                    );
    }
    else if (NdisAuthRequest->AuthChallenge.AuthMethod == WwanAuthAkaPrime)
    {
        ULONG MbbSize;
        ULONG UTF16ByteCount;
        //
        // Calculate required length
        //
        if (NdisAuthRequest->AuthChallenge.u.AuthAkap.NetworkNameLength > WWAN_AUTH_NETWORK_NAME_MAX_LEN)
        {
            TraceError( WMBCLASS_OID, "[MbbNdis][ReqId=0x%04x] NetworkNameLength invalid, %d",
                    Request->RequestId,
                    NdisAuthRequest->AuthChallenge.u.AuthAkap.NetworkNameLength
                    );

            return NDIS_STATUS_INVALID_PARAMETER;
        }

        Status = RtlUTF8ToUnicodeN(
                    NULL,
                    0,
                    &UTF16ByteCount,
                    (PCCH)(NdisAuthRequest->AuthChallenge.u.AuthAkap.NetworkName),
                    NdisAuthRequest->AuthChallenge.u.AuthAkap.NetworkNameLength
                    );
        if( STATUS_SUCCESS != Status )
        {
            TraceError( WMBCLASS_OID, "[MbbNdis][ReqID=0x%04x] FAILED RtlUTF8ToUnicodeN(%s)=%!STATUS!, ignoring=%!BOOLEAN!",
                        Request->RequestId,
                        NdisAuthRequest->AuthChallenge.u.AuthAkap.NetworkName,
                        Status,
                        (STATUS_SOME_NOT_MAPPED == Status)
                        );
            if( STATUS_SOME_NOT_MAPPED != Status )
                return Status;
            else
                Status = STATUS_SUCCESS;
        }

        MbbSize  = ROUND_UP_COUNT( FIELD_OFFSET( MBB_AKAP_AUTH_CHALLENGE, DataBuffer ), ALIGN_DWORD );
        MbbSize += ROUND_UP_COUNT( UTF16ByteCount, ALIGN_DWORD );

        //
        // Allocate the buffer and convert
        //
        if( (MbbAKAPAuthChallenge = (PMBB_AKAP_AUTH_CHALLENGE) ALLOCATE_NONPAGED_POOL( MbbSize )) == NULL )
        {
            TraceError( WMBCLASS_OID, "[MbbNdis][ReqID=0x%04x] FAILED to allocate AuthAkapChallenge for %dbytes",
                        Request->RequestId,
                        MbbSize
                        );
            return NDIS_STATUS_RESOURCES;
        }
        Request->HandlerContext.DataToFreeOnCompletion = MbbAKAPAuthChallenge;

        if( (Status = MbbUtilWwanToMbbAkapAuthChallenge(
                       &NdisAuthRequest->AuthChallenge.u.AuthAkap,
                        UTF16ByteCount,
                        MbbSize,
                        MbbAKAPAuthChallenge
                        )) != NDIS_STATUS_SUCCESS )
        {
            TraceError( WMBCLASS_OID, "[MbbNdis][ReqID=0x%04x] FAILED to convert AuthAkapChallenge, NtStatus=%!STATUS!",
                        Request->RequestId,
                        Status
                        );
            return Status;
        }
        return MbbUtilQueryAttributeWithParameter(
                    Request,
                    (PCHAR)MbbAKAPAuthChallenge,
                    MbbSize
                    );
    }
    else if (NdisAuthRequest->AuthChallenge.AuthMethod == WwanAuthSim){
        RtlCopyMemory(  &MbbSimAuthChallenge,
                        &NdisAuthRequest->AuthChallenge.u.AuthSim,
                        sizeof(MbbSimAuthChallenge) );

        return MbbUtilQueryAttributeWithParameter(
                    Request,
                    (PCHAR)(&MbbSimAuthChallenge),
                    sizeof(MbbSimAuthChallenge)
                    );
    }
    else{
        TraceError( WMBCLASS_OID, "[MbbNdis][ReqId=0x%04x] AUTH CHALLENGE specified is invalid, %d",
                    Request->RequestId,
                    NdisAuthRequest->AuthChallenge.AuthMethod);
        return NDIS_STATUS_INVALID_PARAMETER;
    }
}

NDIS_STATUS
MbbNdisAuthAKAStatusHandler(
    __in PMBB_REQUEST_CONTEXT               Request,
    __in NDIS_STATUS                        NdisStatus,
    __in MBB_STATUS                         MbbStatus,
    __in_bcount_opt(InBufferSize) PUCHAR    InBuffer,
    __in ULONG                              InBufferSize
    )
{
    WWAN_STATUS             WwanStatus = WWAN_STATUS_FAILURE;
    NDIS_STATUS_INDICATION  StatusIndication;
    NDIS_WWAN_AUTH_RESPONSE NdisWWanAuthRes;
    PMBB_AKA_AUTH_RESPONSE  MbbAuthResponse = (PMBB_AKA_AUTH_RESPONSE)InBuffer;
    NDIS_STATUS             LocalNdisStatus = NDIS_STATUS_SUCCESS;

    do
    {
        RtlZeroMemory(&NdisWWanAuthRes, sizeof(NdisWWanAuthRes));

        MBB_INITIALIZE_REQUEST_NDIS_STATUS_INDICATION(
            &StatusIndication,
            Request,
            NDIS_STATUS_WWAN_AUTH_RESPONSE
            );

        NdisWWanAuthRes.Header.Type      = NDIS_OBJECT_TYPE_DEFAULT;
        NdisWWanAuthRes.Header.Size      = sizeof(NDIS_WWAN_AUTH_RESPONSE);
        NdisWWanAuthRes.Header.Revision  = NDIS_WWAN_AUTH_CHALLENGE_REVISION_1;

        if( NdisStatus != NDIS_STATUS_SUCCESS )
        {
            TraceError( WMBCLASS_OID, "[MbbNdis][ReqId=0x%04x] FAILED to get data for NDIS_STATUS_WWAN_AUTH_RESPONSE, status=%!STATUS!",
                        Request->RequestId,
                        NdisStatus
                        );
            break;
        }

        //
        // On certain error statuses there might be valid data
        //
        WwanStatus = MbbUtilMbbToWwanStatus( MbbStatus );
        if( WwanStatus != WWAN_STATUS_SUCCESS &&
            WwanStatus != WWAN_STATUS_AUTH_SYNC_FAILURE )
        {
            break;
        }

        if( MbbAuthResponse == NULL ||
            InBufferSize < sizeof(*MbbAuthResponse) )
        {
            TraceError( WMBCLASS_OID, "[MbbNdis][ReqId=0x%04x] INSUFFICIENT BufferSize for NDIS_WWAN_AUTH_RESPONSE, BufferSize[Received=%d Expected=%d]",
                        Request->RequestId,
                        InBufferSize,
                        sizeof(*MbbAuthResponse)
                        );
            WwanStatus = WWAN_STATUS_FAILURE;
            break;
        }

        //
        // Convert from MBB to WWAN messages.
        //
        RtlCopyMemory(  &NdisWWanAuthRes.AuthResponse.u.AuthAka,
                        MbbAuthResponse,
                        sizeof(*MbbAuthResponse) );


    }
    while( FALSE );
    //
    // On internal error do not restore the device returned operational status
    // since the service may be expecting certain data on certain status.
    //
    NdisWWanAuthRes.uStatus = WwanStatus;
    NdisWWanAuthRes.AuthResponse.AuthMethod = WwanAuthAka;

    StatusIndication.StatusBuffer       = &NdisWWanAuthRes;
    StatusIndication.StatusBufferSize   = sizeof(NDIS_WWAN_AUTH_RESPONSE);

    TraceInfo(  WMBCLASS_OID, "[MbbNdis][ReqId=0x%04x] Indicating NDIS_WWAN_AUTH_RESPONSE with status=%!WwanStatus!",
                Request->RequestId,
                WwanStatus
                );

    MbbNdisMiniportIndicateRequestStatus(
        Request,
        &StatusIndication
        );

    return NDIS_STATUS_SUCCESS;
}


NDIS_STATUS
MbbNdisAuthAKAPStatusHandler(
    __in PMBB_REQUEST_CONTEXT               Request,
    __in NDIS_STATUS                        NdisStatus,
    __in MBB_STATUS                         MbbStatus,
    __in_bcount_opt(InBufferSize) PUCHAR    InBuffer,
    __in ULONG                              InBufferSize
    )
{
    WWAN_STATUS             WwanStatus = WWAN_STATUS_FAILURE;
    NDIS_STATUS_INDICATION  StatusIndication;
    NDIS_WWAN_AUTH_RESPONSE NdisWWanAuthRes;
    PMBB_AKAP_AUTH_RESPONSE MbbAuthResponse = (PMBB_AKAP_AUTH_RESPONSE)InBuffer;
    NDIS_STATUS             LocalNdisStatus = NDIS_STATUS_SUCCESS;

    do
    {
        RtlZeroMemory(&NdisWWanAuthRes, sizeof(NdisWWanAuthRes));

        MBB_INITIALIZE_REQUEST_NDIS_STATUS_INDICATION(
            &StatusIndication,
            Request,
            NDIS_STATUS_WWAN_AUTH_RESPONSE
            );

        NdisWWanAuthRes.Header.Type      = NDIS_OBJECT_TYPE_DEFAULT;
        NdisWWanAuthRes.Header.Size      = sizeof(NDIS_WWAN_AUTH_RESPONSE);
        NdisWWanAuthRes.Header.Revision  = NDIS_WWAN_AUTH_CHALLENGE_REVISION_1;

        if( NdisStatus != NDIS_STATUS_SUCCESS )
        {
            TraceError( WMBCLASS_OID, "[MbbNdis][ReqId=0x%04x] FAILED to get data for NDIS_STATUS_WWAN_AUTH_RESPONSE, status=%!STATUS!",
                        Request->RequestId,
                        NdisStatus
                        );
            break;
        }

        //
        // On certain error statuses there might be valid data
        //
        WwanStatus = MbbUtilMbbToWwanStatus( MbbStatus );
        if( WwanStatus != WWAN_STATUS_SUCCESS &&
            WwanStatus != WWAN_STATUS_AUTH_SYNC_FAILURE )
        {
            break;
        }

        if( MbbAuthResponse == NULL ||
            InBufferSize < sizeof(*MbbAuthResponse) )
        {
            TraceError( WMBCLASS_OID, "[MbbNdis][ReqId=0x%04x] INSUFFICIENT BufferSize for NDIS_WWAN_AUTH_RESPONSE, BufferSize[Received=%d Expected=%d]",
                        Request->RequestId,
                        InBufferSize,
                        sizeof(*MbbAuthResponse)
                        );
            WwanStatus = WWAN_STATUS_FAILURE;
            break;
        }

        //
        // Convert from MBB to WWAN messages.
        //
        RtlCopyMemory(  &NdisWWanAuthRes.AuthResponse.u.AuthAkap,
                        MbbAuthResponse,
                        sizeof(*MbbAuthResponse) );


    }
    while( FALSE );
    //
    // On internal error do not restore the device returned operational status
    // since the service may be expecting certain data on certain status.
    //
    NdisWWanAuthRes.uStatus = WwanStatus;
    NdisWWanAuthRes.AuthResponse.AuthMethod = WwanAuthAkaPrime;

    StatusIndication.StatusBuffer       = &NdisWWanAuthRes;
    StatusIndication.StatusBufferSize   = sizeof(NDIS_WWAN_AUTH_RESPONSE);

    TraceInfo(  WMBCLASS_OID, "[MbbNdis][ReqId=0x%04x] Indicating NDIS_WWAN_AUTH_RESPONSE with status=%!WwanStatus!",
                Request->RequestId,
                WwanStatus
                );

    MbbNdisMiniportIndicateRequestStatus(
        Request,
        &StatusIndication
        );

    return NDIS_STATUS_SUCCESS;
}


NDIS_STATUS
MbbNdisAuthSIMStatusHandler(
    __in PMBB_REQUEST_CONTEXT               Request,
    __in NDIS_STATUS                        NdisStatus,
    __in MBB_STATUS                         MbbStatus,
    __in_bcount_opt(InBufferSize) PUCHAR    InBuffer,
    __in ULONG                              InBufferSize
    )
{
    WWAN_STATUS             WwanStatus = WWAN_STATUS_FAILURE;
    NDIS_STATUS_INDICATION  StatusIndication;
    NDIS_WWAN_AUTH_RESPONSE NdisWWanAuthRes;
    PMBB_SIM_AUTH_RESPONSE  MbbAuthResponse = (PMBB_SIM_AUTH_RESPONSE)InBuffer;
    NDIS_STATUS             LocalNdisStatus = NDIS_STATUS_SUCCESS;

    do
    {
        RtlZeroMemory(&NdisWWanAuthRes, sizeof(NdisWWanAuthRes));

        MBB_INITIALIZE_REQUEST_NDIS_STATUS_INDICATION(
            &StatusIndication,
            Request,
            NDIS_STATUS_WWAN_AUTH_RESPONSE
            );

        NdisWWanAuthRes.Header.Type      = NDIS_OBJECT_TYPE_DEFAULT;
        NdisWWanAuthRes.Header.Size      = sizeof(NDIS_WWAN_AUTH_RESPONSE);
        NdisWWanAuthRes.Header.Revision  = NDIS_WWAN_AUTH_CHALLENGE_REVISION_1;

        if( NdisStatus != NDIS_STATUS_SUCCESS )
        {
            TraceError( WMBCLASS_OID, "[MbbNdis][ReqId=0x%04x] FAILED to get data for NDIS_STATUS_WWAN_AUTH_RESPONSE, status=%!STATUS!",
                        Request->RequestId,
                        NdisStatus
                        );
            break;
        }

        WwanStatus = MbbUtilMbbToWwanStatus( MbbStatus );
        if (WwanStatus != WWAN_STATUS_SUCCESS){
            break;
        }

        if( MbbAuthResponse == NULL ||
            InBufferSize < sizeof(*MbbAuthResponse) )
        {
            TraceError( WMBCLASS_OID, "[MbbNdis][ReqId=0x%04x] INSUFFICIENT BufferSize for NDIS_WWAN_AUTH_RESPONSE, BufferSize[Received=%d Expected=%d]",
                        Request->RequestId,
                        InBufferSize,
                        sizeof(*MbbAuthResponse)
                        );
            WwanStatus = WWAN_STATUS_FAILURE;
            break;
        }

        //
        // Convert from MBB to WWAN messages.
        //
        RtlCopyMemory(  &NdisWWanAuthRes.AuthResponse.u.AuthSim,
                        MbbAuthResponse,
                        sizeof(*MbbAuthResponse) );


    }
    while( FALSE );

    NdisWWanAuthRes.uStatus = WwanStatus;
    NdisWWanAuthRes.AuthResponse.AuthMethod = WwanAuthSim;

    StatusIndication.StatusBuffer       = &NdisWWanAuthRes;
    StatusIndication.StatusBufferSize   = sizeof(NDIS_WWAN_AUTH_RESPONSE);

    TraceInfo(  WMBCLASS_OID, "[MbbNdis][ReqId=0x%04x] Indicating NDIS_WWAN_AUTH_RESPONSE with status=%!WwanStatus!",
                Request->RequestId,
                WwanStatus
                );

    MbbNdisMiniportIndicateRequestStatus(
        Request,
        &StatusIndication
        );

    return NDIS_STATUS_SUCCESS;
}



NDIS_STATUS
MbbNdisDssStateStatusHandler(
    __in PMBB_REQUEST_CONTEXT               Request,
    __in NDIS_STATUS                        NdisStatus,
    __in MBB_STATUS                         MbbStatus,
    __in_bcount_opt(InBufferSize) PUCHAR    InBuffer,
    __in ULONG                              InBufferSize,
    __in WWAN_DEVICE_SERVICE_SESSION_STATE  NewState
    )
{
    WWAN_STATUS             WwanStatus = WWAN_STATUS_FAILURE;
    NDIS_STATUS_INDICATION  StatusIndication;
    NDIS_WWAN_DEVICE_SERVICE_SESSION_INFO NdisWWanSessionInfo;
    NDIS_STATUS             LocalNdisStatus = NDIS_STATUS_SUCCESS;

    if (MbbReqMgrIsUnsolicitedIndication(Request))
    {
        TraceError( WMBCLASS_OID, "[MbbNdis][ReqId=0x%04x] Dropping unsolicited Device Service Session state notification",
                    Request->RequestId
                    );
        return NDIS_STATUS_NOT_ACCEPTED;
    }

    do
    {

        RtlZeroMemory(&NdisWWanSessionInfo, sizeof(NdisWWanSessionInfo));

        MBB_INITIALIZE_REQUEST_NDIS_STATUS_INDICATION(
            &StatusIndication,
            Request,
            NDIS_STATUS_WWAN_DEVICE_SERVICE_SESSION
            );

        NdisWWanSessionInfo.Header.Type      = NDIS_OBJECT_TYPE_DEFAULT;
        NdisWWanSessionInfo.Header.Size      = SIZEOF_NDIS_WWAN_DEVICE_SERVICE_SESSION_INFO_1;
        NdisWWanSessionInfo.Header.Revision  = NDIS_WWAN_DEVICE_SERVICE_SESSION_INFO_REVISION_1;

        //
        // Populate the saved info for the request
        //
        NdisWWanSessionInfo.Session.State = Request->HandlerContext.Parameters.DssSession.LinkState == MbbDssLinkActivate ?
                                                WwanDeviceServiceSessionOpen : WwanDeviceServiceSessionClosed;

        RtlCopyMemory(
            &NdisWWanSessionInfo.Session.DeviceServiceGuid,
            &Request->HandlerContext.Parameters.DssSession.DeviceServiceGuid,
            sizeof(GUID)
            );

        NdisWWanSessionInfo.Session.uSessionID = Request->HandlerContext.Parameters.DssSession.SessionId;

        if( NdisStatus != NDIS_STATUS_SUCCESS )
        {
            TraceError( WMBCLASS_OID, "[MbbNdis][ReqId=0x%04x] FAILED to get data for NDIS_STATUS_WWAN_DEVICE_SERVICE_SESSION, status=%!STATUS!",
                        Request->RequestId,
                        NdisStatus
                        );
            WwanStatus = WWAN_STATUS_FAILURE;
            break;
        }

        WwanStatus = MbbUtilMbbToWwanStatus( MbbStatus );
        if (WwanStatus != WWAN_STATUS_SUCCESS){
            break;
        }

    }
    while( FALSE );

    NdisWWanSessionInfo.uStatus = WwanStatus;

    StatusIndication.StatusBuffer       = &NdisWWanSessionInfo;
    StatusIndication.StatusBufferSize   = sizeof(NDIS_WWAN_DEVICE_SERVICE_SESSION_INFO);

    TraceInfo(  WMBCLASS_OID, "[MbbNdis][ReqId=0x%04x] Indicating NDIS_STATUS_WWAN_DEVICE_SERVICE_SESSION with status=%!WwanStatus!",
                Request->RequestId,
                WwanStatus
                );

    MbbNdisMiniportIndicateRequestStatus(
        Request,
        &StatusIndication
        );

    return NDIS_STATUS_SUCCESS;
}



NDIS_STATUS
MbbNdisDssOpenStatusHandler(
    __in PMBB_REQUEST_CONTEXT               Request,
    __in NDIS_STATUS                        NdisStatus,
    __in MBB_STATUS                         MbbStatus,
    __in_bcount_opt(InBufferSize) PUCHAR    InBuffer,
    __in ULONG                              InBufferSize
    )
{
    // Forward to the generic handler
    return MbbNdisDssStateStatusHandler(
        Request,
        NdisStatus,
        MbbStatus,
        InBuffer,
        InBufferSize,
        WwanDeviceServiceSessionOpen
        );
}


NDIS_STATUS
MbbNdisDssCloseStatusHandler(
    __in PMBB_REQUEST_CONTEXT               Request,
    __in NDIS_STATUS                        NdisStatus,
    __in MBB_STATUS                         MbbStatus,
    __in_bcount_opt(InBufferSize) PUCHAR    InBuffer,
    __in ULONG                              InBufferSize
    )
{
    // Forward to the generic handler
    return MbbNdisDssStateStatusHandler(
        Request,
        NdisStatus,
        MbbStatus,
        InBuffer,
        InBufferSize,
        WwanDeviceServiceSessionClosed
        );

}


NDIS_STATUS
MbbNdisDssWriteCompleteStatusHandler(
    __in PMBB_REQUEST_CONTEXT               Request,
    __in NDIS_STATUS                        NdisStatus,
    __in MBB_STATUS                         MbbStatus,
    __in_bcount_opt(InBufferSize) PUCHAR    InBuffer,
    __in ULONG                              InBufferSize
    )
{
    WWAN_STATUS             WwanStatus = WWAN_STATUS_FAILURE;
    NDIS_STATUS_INDICATION  StatusIndication;
    NDIS_WWAN_DEVICE_SERVICE_SESSION_WRITE_COMPLETE NdisDssWriteComplete;

    do
    {

        RtlZeroMemory(&NdisDssWriteComplete, sizeof(NdisDssWriteComplete));

        MBB_INITIALIZE_REQUEST_NDIS_STATUS_INDICATION(
            &StatusIndication,
            Request,
            NDIS_STATUS_WWAN_DEVICE_SERVICE_SESSION_WRITE_COMPLETE
            );

        NdisDssWriteComplete.Header.Type      = NDIS_OBJECT_TYPE_DEFAULT;
        NdisDssWriteComplete.Header.Size      = SIZEOF_NDIS_WWAN_DEVICE_SERVICE_SESSION_WRITE_COMPLETE_1;
        NdisDssWriteComplete.Header.Revision  = NDIS_WWAN_DEVICE_SERVICE_SESSION_WRITE_COMPLETE_REVISION_1;

        WwanStatus = MbbUtilNdisToWwanStatus( NdisStatus );
        if (WwanStatus != WWAN_STATUS_SUCCESS)
            break;

    }
    while( FALSE );

    NdisDssWriteComplete.uStatus = WwanStatus;

    StatusIndication.StatusBuffer       = &NdisDssWriteComplete;
    StatusIndication.StatusBufferSize   = sizeof(NDIS_WWAN_DEVICE_SERVICE_SESSION_WRITE_COMPLETE);

    TraceInfo(  WMBCLASS_OID, "[MbbNdis][ReqId=0x%04x] Indicating NDIS_STATUS_WWAN_DEVICE_SERVICE_SESSION_WRITE_COMPLETE with status=%!WwanStatus!",
                Request->RequestId,
                WwanStatus
                );

    MbbNdisMiniportIndicateRequestStatus(
        Request,
        &StatusIndication
        );

    // Derefence the request
    MbbReqMgrDerefRequest(Request);

    return NDIS_STATUS_SUCCESS;
}


VOID
MbbNdisDeviceServiceSessionSendComplete(
    __in    MBB_REQUEST_HANDLE      RequestHandle,
    __in    NDIS_STATUS             NdisStatus
    )
{
    PMBB_OID_HANDLER_ENTRY          OidHandler;

    OidHandler = MbbNdisGetOidHandler(OID_WWAN_DEVICE_SERVICE_SESSION_WRITE);

    if (OidHandler)
    {
        // Call the common handler to handle the indication
        MbbUtilCommonCIDResponse(
            (PMBB_REQUEST_CONTEXT)RequestHandle,
            NdisStatus,
            WWAN_STATUS_SUCCESS,
            NULL,
            0,
            OidHandler
            );
    }
}

NDIS_STATUS
MbbNdisDeviceServiceSessionReceive(
    __in PMINIPORT_ADAPTER_CONTEXT          AdapterContext,
    __in ULONG                              SessionId,
    __in_bcount_opt(InBufferSize) PUCHAR    InBuffer,
    __in ULONG                              InBufferSize
    )
{
    NDIS_STATUS_INDICATION  StatusIndication;
    NDIS_STATUS             NdisStatus = NDIS_STATUS_SUCCESS;
    PNDIS_WWAN_DEVICE_SERVICE_SESSION_READ NdisWwanDssRead = NULL;
    ULONG                   TotalNdisWwanSize = sizeof(NDIS_WWAN_DEVICE_SERVICE_SESSION_READ) + InBufferSize;

    do
    {
        // Allocate memory for the output buffer
        if( (NdisWwanDssRead = (PNDIS_WWAN_DEVICE_SERVICE_SESSION_READ) ALLOCATE_NONPAGED_POOL( TotalNdisWwanSize )) == NULL )
        {
            TraceError( WMBCLASS_RECEIVE, "[Receive][DSS Read] FAILED to allocate for PNDIS_WWAN_DEVICE_SERVICE_SESSION_READ %d bytes",
                        TotalNdisWwanSize
                        );
            NdisStatus = NDIS_STATUS_RESOURCES;
            break;
        }

        NdisWwanDssRead->Header.Type      = NDIS_OBJECT_TYPE_DEFAULT;
        NdisWwanDssRead->Header.Size      = SIZEOF_NDIS_WWAN_DEVICE_SERVICE_SESSION_READ_1;
        NdisWwanDssRead->Header.Revision  = NDIS_WWAN_DEVICE_SERVICE_SESSION_READ_REVISION_1;

        NdisWwanDssRead->ReadData.uDataSize = InBufferSize;
        NdisWwanDssRead->ReadData.uSessionID = SessionId;

        // Copy the data. We cannot use the original buffer because it doesnt
        // have space for the session Id, etc
        RtlCopyMemory(
            (PUCHAR)(NdisWwanDssRead + 1),
            InBuffer,
            InBufferSize
            );

        // Create the status indication
        MBB_UTIL_INITIALIZE_NDIS_STATUS_INDICATION(
            &StatusIndication,
            AdapterContext->MiniportAdapterHandle,
            NDIS_STATUS_WWAN_DEVICE_SERVICE_SESSION_READ
            );

        StatusIndication.StatusBuffer       = NdisWwanDssRead;
        StatusIndication.StatusBufferSize   = TotalNdisWwanSize;

        TraceInfo(  WMBCLASS_RECEIVE, "[Receive][DSS Read] Indicating NDIS_STATUS_WWAN_DEVICE_SERVICE_SESSION_READ");

        MbbUtilNdisMiniportIndicateStatusEx(
            AdapterContext,
            &StatusIndication
            );

    }
    while( FALSE );

    if (NdisWwanDssRead)
    {
        FREE_POOL(NdisWwanDssRead);
    }

    return NdisStatus;
}

NDIS_STATUS
MbbNdisMultiCarrierDsCidListStatusHandler(
    __in PMBB_REQUEST_CONTEXT               Request,
    __in NDIS_STATUS                        NdisStatus,
    __in MBB_STATUS                         MbbStatus,
    __in_bcount_opt(InBufferSize) PUCHAR    InBuffer,
    __in ULONG                              InBufferSize
    )
{
    GUID DsServiceId;
    ULONG CidIndex;
    ULONG ExpectedSize;
    WWAN_STATUS WwanStatus;
    PMINIPORT_ADAPTER_CONTEXT Adapter;
    PMBB_MULTICARRIER_CURRENT_CID_LIST MbbMulticarrierCidList;

    do
    {
        Adapter = (PMINIPORT_ADAPTER_CONTEXT)MbbReqMgrGetAdapterHandle( Request );
        //
        // If we couldnt retrieve the data then bail out.
        //
        if( NdisStatus != NDIS_STATUS_SUCCESS )
        {
            TraceError( WMBCLASS_INIT, "[MbbNdis][ReqId=0x%04x] FAILED to get data for MBB_MULTICARRIER_CID_CURRENT_CID_LIST, status=%!STATUS!",
                        Request->RequestId,
                        NdisStatus
                        );
            break;
        }

        WwanStatus = MbbUtilMbbToWwanStatus( MbbStatus );
        if( WWAN_STATUS_SUCCESS != WwanStatus )
        {
            TraceError( WMBCLASS_INIT, "[MbbNdis][ReqId=0x%04x] FAILED to get data for MBB_MULTICARRIER_CID_CURRENT_CID_LIST, status=%!STATUS!",
                        Request->RequestId,
                        WwanStatus
                        );
            break;
        }
        //
        // Verify the received data
        //
        MbbMulticarrierCidList = (PMBB_MULTICARRIER_CURRENT_CID_LIST)InBuffer;
        ExpectedSize = RTL_SIZEOF_THROUGH_FIELD(MBB_MULTICARRIER_CURRENT_CID_LIST, CIDCount);

        if( MbbMulticarrierCidList == NULL ||
            InBufferSize < ExpectedSize )
        {
            TraceError( WMBCLASS_INIT, "[MbbNdis][ReqId=0x%04x] INSUFFICIENT BufferSize for MBB_MULTICARRIER_CID_CURRENT_CID_LIST, BufferSize[Received=%d Expected=%d]",
                        Request->RequestId,
                        InBufferSize,
                        ExpectedSize
                        );
            break;
        }

        NdisStatus = RtlULongMult(
                        sizeof(ULONG),
                        MbbMulticarrierCidList->CIDCount,
                        &ExpectedSize
                        );
        if( NT_ERROR( NdisStatus ) )
        {
            TraceError( WMBCLASS_INIT, "[MbbNdis][ReqId=0x%04x] INTEGER OVERFLOW : 0x%x * 0x%x = 0x%I64x",
                        Request->RequestId,
                        MbbMulticarrierCidList->CIDCount,
                        sizeof(ULONG),
                        ((ULONGLONG)(MbbMulticarrierCidList->CIDCount)) * sizeof(ULONG)
                        );
            break;
        }

        NdisStatus = RtlULongAdd(
                        ExpectedSize,
                        RTL_SIZEOF_THROUGH_FIELD(MBB_MULTICARRIER_CURRENT_CID_LIST, CIDCount),
                        &ExpectedSize
                        );
        if( NT_ERROR( NdisStatus ) )
        {
            TraceError( WMBCLASS_INIT, "[MbbNdis][ReqId=0x%04x] INTEGER OVERFLOW : 0x%x + 0x%x = 0x%I64x",
                        Request->RequestId,
                        ExpectedSize,
                        RTL_SIZEOF_THROUGH_FIELD(MBB_MULTICARRIER_CURRENT_CID_LIST, CIDCount),
                        ((ULONGLONG)ExpectedSize) + RTL_SIZEOF_THROUGH_FIELD(MBB_MULTICARRIER_CURRENT_CID_LIST, CIDCount)
                        );
            break;
        }

        if( InBufferSize < ExpectedSize )
        {
            TraceError( WMBCLASS_INIT, "[MbbNdis][ReqId=0x%04x] INSUFFICIENT BufferSize for MBB_MULTICARRIER_CID_CURRENT_CID_LIST, CidCount=%d BufferSize[Received=%d Expected=%d]",
                        Request->RequestId,
                        MbbMulticarrierCidList->CIDCount,
                        InBufferSize,
                        ExpectedSize
                        );
            break;
        }
        //
        // There is valid data, convert the CIDList to capabilities
        //
        MBB_UUID_TO_NET(
            &DsServiceId,
            &Request->HandlerContext.Parameters.DeviceCaps.CurrentQueriedDeviceService
            );
        if( RtlCompareMemory(
                &DsServiceId,
                &MBB_UUID_USSD,
                sizeof(GUID)
                ) == sizeof(GUID) )
        {
            if( MbbMulticarrierCidList->CIDCount > 0 )
            {
                MbbAdapterLock( Adapter );
                Adapter->AdapterFlags.IsUssdCapable = TRUE;
                MbbAdapterUnlock( Adapter );
            }
            Request->HandlerContext.Parameters.DeviceCaps.IsUssdCapsValid = 1;
        } else if( RtlCompareMemory(
                        &DsServiceId,
                        &MBB_UUID_AUTH,
                        sizeof(GUID)
                        ) == sizeof(GUID) )
        {
            MbbAdapterLock( Adapter );
            for(CidIndex = 0;
                CidIndex < MbbMulticarrierCidList->CIDCount;
                CidIndex++ )
            {
                switch( MbbMulticarrierCidList->CIDList[CidIndex] )
                {
                    case MBB_AUTH_CID_AKA:
                    {
                        Adapter->AdapterFlags.IsAkaAuthCapable = TRUE;
                    }
                    break;

                    case MBB_AUTH_CID_AKAP:
                    {
                        Adapter->AdapterFlags.IsAkapAuthCapable = TRUE;
                    }
                    break;

                    case MBB_AUTH_CID_SIM:
                    {
                        Adapter->AdapterFlags.IsSimAuthCapable = TRUE;
                    }
                    break;
                }
            }
            MbbAdapterUnlock( Adapter );
            Request->HandlerContext.Parameters.DeviceCaps.IsAuthCapsValid = 1;
        }
    }
    while( FALSE );

    MbbNdisIndicateDeviceCaps(
        Request,
        NdisStatus
        );

    return NdisStatus;
}

NDIS_STATUS
MbbNdisUiccAppListStatusHandler(
    __in PMBB_REQUEST_CONTEXT               Request,
    __in NDIS_STATUS                        NdisStatus,
    __in MBB_STATUS                         MbbStatus,
    __in_bcount_opt(InBufferSize) PUCHAR    InBuffer,
    __in ULONG                              InBufferSize
)
{
    WWAN_STATUS                 WwanStatus;
    NDIS_STATUS_INDICATION      StatusIndication;
    NDIS_WWAN_UICC_APP_LIST     NdisUiccAppList;
    PMBB_UICC_APP_LIST          MbbUiccAppList = (PMBB_UICC_APP_LIST)InBuffer;

    do
    {
        MBB_INITIALIZE_REQUEST_NDIS_STATUS_INDICATION(
            &StatusIndication,
            Request,
            NDIS_STATUS_WWAN_UICC_APP_LIST
        );

        RtlZeroMemory(&NdisUiccAppList, sizeof(NdisUiccAppList));

        NdisUiccAppList.Header.Type = NDIS_OBJECT_TYPE_DEFAULT;
        NdisUiccAppList.Header.Size = sizeof(NDIS_WWAN_UICC_APP_LIST);
        NdisUiccAppList.Header.Revision = NDIS_WWAN_UICC_APP_LIST_REVISION_1;

        WwanStatus = MbbUtilMbbToWwanStatus(MbbStatus);

        if (NdisStatus != NDIS_STATUS_SUCCESS)
        {
            TraceError(WMBCLASS_OID, "[MbbNdis][ReqId=0x%04x] FAILED to get data for NDIS_STATUS_WWAN_UICC_APP_LIST, status=%!STATUS!",
                Request->RequestId,
                NdisStatus
            );
            WwanStatus = WWAN_STATUS_FAILURE;
            break;
        }

        if (MbbUiccAppList == NULL ||
            InBufferSize < sizeof(MBB_UICC_APP_LIST))
        {
            TraceError(WMBCLASS_OID, "[MbbNdis][ReqId=0x%04x] INSUFFICIENT BufferSize for NDIS_STATUS_WWAN_UICC_APP_LIST, BufferSize[Received=%d Expected=%d]",
                Request->RequestId,
                InBufferSize,
                sizeof(MBB_UICC_APP_LIST)
            );
            WwanStatus = WWAN_STATUS_FAILURE;
            break;
        }

        MbbUtilMbbToWwanUiccAppList(
            MbbUiccAppList,
            &NdisUiccAppList.UiccAppList
        );
    } while (FALSE);

    //
    // Restore the device returned operational status, if any.
    //
    if (WwanStatus != WWAN_STATUS_SUCCESS &&
        MbbUtilMbbToWwanStatus(MbbStatus) != WWAN_STATUS_SUCCESS)
    {
        WwanStatus = MbbUtilMbbToWwanStatus(MbbStatus);
    }

    NdisUiccAppList.uStatus = WwanStatus;

    StatusIndication.StatusBuffer = &NdisUiccAppList;
    StatusIndication.StatusBufferSize = sizeof(NDIS_WWAN_UICC_APP_LIST);

    TraceInfo(WMBCLASS_OID, "[MbbNdis][ReqId=0x%04x] Indicating NDIS_STATUS_WWAN_UICC_APP_LIST with status=%!WwanStatus!",
        Request->RequestId,
        WwanStatus
    );

    MbbNdisMiniportIndicateRequestStatus(
        Request,
        &StatusIndication
    );

    return NDIS_STATUS_SUCCESS;
}

NDIS_STATUS
MbbNdisUiccFileStatusHandler(
    __in PMBB_REQUEST_CONTEXT               Request,
    __in NDIS_STATUS                        NdisStatus,
    __in MBB_STATUS                         MbbStatus,
    __in_bcount_opt(InBufferSize) PUCHAR    InBuffer,
    __in ULONG                              InBufferSize
    )
{
    WWAN_STATUS                 WwanStatus;
    NDIS_STATUS_INDICATION      StatusIndication;
    NDIS_WWAN_UICC_FILE_STATUS  NdisUiccFileStatus;
    PMBB_UICC_FILE_STATUS       MbbUiccFileStatus = (PMBB_UICC_FILE_STATUS)InBuffer;

    do
    {
        MBB_INITIALIZE_REQUEST_NDIS_STATUS_INDICATION(
            &StatusIndication,
            Request,
            NDIS_STATUS_WWAN_UICC_FILE_STATUS
            );

        RtlZeroMemory(&NdisUiccFileStatus, sizeof(NdisUiccFileStatus));

        NdisUiccFileStatus.Header.Type      = NDIS_OBJECT_TYPE_DEFAULT;
        NdisUiccFileStatus.Header.Size      = sizeof(NDIS_WWAN_UICC_FILE_STATUS);
        NdisUiccFileStatus.Header.Revision  = NDIS_WWAN_UICC_FILE_STATUS_REVISION_1;

        WwanStatus = MbbUtilMbbToWwanStatus(MbbStatus);

        if( NdisStatus != NDIS_STATUS_SUCCESS )
        {
            TraceError( WMBCLASS_OID, "[MbbNdis][ReqId=0x%04x] FAILED to get data for NDIS_STATUS_WWAN_UICC_FILE_STATUS, status=%!STATUS!",
                        Request->RequestId,
                        NdisStatus
                        );
            WwanStatus = WWAN_STATUS_FAILURE;
            break;
        }

        if( MbbUiccFileStatus == NULL ||
            InBufferSize < sizeof(MBB_UICC_FILE_STATUS) )
        {
            TraceError( WMBCLASS_OID, "[MbbNdis][ReqId=0x%04x] INSUFFICIENT BufferSize for NDIS_STATUS_WWAN_UICC_FILE_STATUS, BufferSize[Received=%d Expected=%d]",
                        Request->RequestId,
                        InBufferSize,
                        sizeof(MBB_UICC_FILE_STATUS)
                        );
            WwanStatus = WWAN_STATUS_FAILURE;
            break;
        }

        MbbUtilMbbToWwanUiccFileStatus(
            MbbUiccFileStatus,
            &NdisUiccFileStatus.UiccFileStatus
            );
    }
    while( FALSE );

    //
    // Restore the device returned operational status, if any.
    //
    if( WwanStatus != WWAN_STATUS_SUCCESS &&
        MbbUtilMbbToWwanStatus( MbbStatus ) != WWAN_STATUS_SUCCESS )
    {
        WwanStatus = MbbUtilMbbToWwanStatus( MbbStatus );
    }

    NdisUiccFileStatus.uStatus = WwanStatus;

    StatusIndication.StatusBuffer       = &NdisUiccFileStatus;
    StatusIndication.StatusBufferSize   = sizeof(NDIS_WWAN_UICC_FILE_STATUS);

    TraceInfo(  WMBCLASS_OID, "[MbbNdis][ReqId=0x%04x] Indicating NDIS_STATUS_WWAN_UICC_FILE_STATUS with status=%!WwanStatus!",
                Request->RequestId,
                WwanStatus
                );

    MbbNdisMiniportIndicateRequestStatus(
        Request,
        &StatusIndication
        );

    return NDIS_STATUS_SUCCESS;
}

NDIS_STATUS
MbbNdisUiccAccessStatusHandler(
    __in PMBB_REQUEST_CONTEXT               Request,
    __in NDIS_STATUS                        NdisStatus,
    __in MBB_STATUS                         MbbStatus,
    __in_bcount_opt(InBufferSize) PUCHAR    InBuffer,
    __in ULONG                              InBufferSize
    )
{
    BOOLEAN                     IndicationFailure = TRUE;
    WWAN_STATUS                 WwanStatus;
    NDIS_STATUS_INDICATION      StatusIndication;
    ULONG                       NdisUiccResponseSize = 0;
    PNDIS_WWAN_UICC_RESPONSE    NdisUiccResponse = NULL;
    NDIS_WWAN_UICC_RESPONSE     LocalNdisUiccResponse;
    PMBB_UICC_RESPONSE          MbbUiccResponse = (PMBB_UICC_RESPONSE)InBuffer;

    do
    {
        MBB_INITIALIZE_REQUEST_NDIS_STATUS_INDICATION(
            &StatusIndication,
            Request,
            NDIS_STATUS_WWAN_UICC_RESPONSE
            );

        WwanStatus = MbbUtilMbbToWwanStatus( MbbStatus );

        if (NdisStatus != NDIS_STATUS_SUCCESS)
        {
            TraceError( WMBCLASS_OID, "[MbbNdis][ReqId=0x%04x] FAILED to get data for NDIS_STATUS_WWAN_UICC_RESPONSE, status=%!STATUS!",
                Request->RequestId,
                NdisStatus
                );
            WwanStatus = WWAN_STATUS_FAILURE;
            break;
        }

        if( MbbUiccResponse == NULL ||
            InBufferSize < (ULONG)FIELD_OFFSET(MBB_UICC_RESPONSE, DataBuffer))
        {
            TraceError( WMBCLASS_OID, "[MbbNdis][ReqId=0x%04x] INSUFFICIENT BufferSize for NDIS_STATUS_WWAN_UICC_RESPONSE, BufferSize[Received=%d Expected=%d]",
                        Request->RequestId,
                        InBufferSize,
                        FIELD_OFFSET(MBB_UICC_RESPONSE, DataBuffer)
                        );
            WwanStatus = WWAN_STATUS_FAILURE;
            break;
        }

        NdisUiccResponseSize  = FIELD_OFFSET(NDIS_WWAN_UICC_RESPONSE, UiccResponse.ResponseData);
        NdisUiccResponseSize += MbbUiccResponse->ResponseData.Size;

        NdisUiccResponse = (PNDIS_WWAN_UICC_RESPONSE)ALLOCATE_NONPAGED_POOL( NdisUiccResponseSize );

        if( NdisUiccResponse == NULL )
        {
            TraceError( WMBCLASS_OID, "[MbbNdis][ReqId=0x%04x] FAILED to allocate %d bytes for NDIS_STATUS_WWAN_UICC_RESPONSE",
                        Request->RequestId,
                        NdisUiccResponseSize
                        );
            WwanStatus = WWAN_STATUS_FAILURE;
            break;
        }

        RtlZeroMemory(NdisUiccResponse, NdisUiccResponseSize);

        NdisUiccResponse->Header.Type       = NDIS_OBJECT_TYPE_DEFAULT;
        NdisUiccResponse->Header.Size       = sizeof(NDIS_WWAN_UICC_RESPONSE);
        NdisUiccResponse->Header.Revision   = NDIS_WWAN_UICC_RESPONSE_REVISION_1;

        MbbUtilMbbToWwanUiccResponse(
            MbbUiccResponse,
            &NdisUiccResponse->UiccResponse
            );
        IndicationFailure = FALSE;
    }
    while( FALSE );

    //
    // Restore the device returned operational status, if any.
    //
    if( WwanStatus != WWAN_STATUS_SUCCESS &&
        MbbUtilMbbToWwanStatus( MbbStatus ) != WWAN_STATUS_SUCCESS )
    {
        WwanStatus = MbbUtilMbbToWwanStatus( MbbStatus );
    }

    if (!IndicationFailure)
    {
        NdisUiccResponse->uStatus = WwanStatus;

        StatusIndication.StatusBuffer = NdisUiccResponse;
        StatusIndication.StatusBufferSize = NdisUiccResponseSize;
    }
    else
    {
        RtlZeroMemory(&LocalNdisUiccResponse, sizeof(NDIS_WWAN_UICC_RESPONSE));

        LocalNdisUiccResponse.Header.Type = NDIS_OBJECT_TYPE_DEFAULT;
        LocalNdisUiccResponse.Header.Size = sizeof(NDIS_WWAN_UICC_RESPONSE);
        LocalNdisUiccResponse.Header.Revision = NDIS_WWAN_UICC_RESPONSE_REVISION_1;
        LocalNdisUiccResponse.uStatus = WwanStatus;

        StatusIndication.StatusBuffer = &LocalNdisUiccResponse;
        StatusIndication.StatusBufferSize = sizeof(NDIS_WWAN_UICC_RESPONSE);
    }

    TraceInfo(  WMBCLASS_OID, "[MbbNdis][ReqId=0x%04x] Indicating NDIS_STATUS_WWAN_UICC_RESPONSE with status=%!WwanStatus!",
                Request->RequestId,
                WwanStatus
                );

    MbbNdisMiniportIndicateRequestStatus(
        Request,
        &StatusIndication
        );

    if (NdisUiccResponse != NULL)
    {
        FREE_POOL(NdisUiccResponse);
    }

    return NDIS_STATUS_SUCCESS;
}

NDIS_STATUS
MbbNdisSysCapsStatusHandler(
    __in PMBB_REQUEST_CONTEXT               Request,
    __in NDIS_STATUS                        NdisStatus,
    __in MBB_STATUS                         MbbStatus,
    __in_bcount_opt(InBufferSize) PUCHAR    InBuffer,
    __in ULONG                              InBufferSize
    )
{
    WWAN_STATUS                 WwanStatus = WWAN_STATUS_SUCCESS;
    NDIS_STATUS_INDICATION      StatusIndication = { 0 };
    NDIS_WWAN_SYS_CAPS_INFO     NdisSysCapsInfo = { 0 };
    PMBB_MS_SYS_CAPS_INFO       MbbSysCapsInfo = (PMBB_MS_SYS_CAPS_INFO)InBuffer;

    do
    {
        MBB_INITIALIZE_REQUEST_NDIS_STATUS_INDICATION(
            &StatusIndication,
            Request,
            NDIS_STATUS_WWAN_SYS_CAPS_INFO
            );

        RtlZeroMemory(&NdisSysCapsInfo, sizeof(NDIS_WWAN_SYS_CAPS_INFO));

        NdisSysCapsInfo.Header.Type = NDIS_OBJECT_TYPE_DEFAULT;
        NdisSysCapsInfo.Header.Size = sizeof(NDIS_WWAN_SYS_CAPS_INFO);
        NdisSysCapsInfo.Header.Revision = NDIS_WWAN_SYS_CAPS_INFO_REVISION_1;

        WwanStatus = MbbUtilMbbToWwanStatus(MbbStatus);

        if (NdisStatus != NDIS_STATUS_SUCCESS)
        {
            TraceError(WMBCLASS_OID, "[MbbNdis][ReqId=0x%04x] FAILED to get data for NDIS_STATUS_WWAN_SYS_CAPS_INFO, status=%!STATUS!",
                Request->RequestId,
                NdisStatus
                );
            WwanStatus = WWAN_STATUS_FAILURE;
            break;
        }

        if (MbbSysCapsInfo == NULL ||
            InBufferSize < sizeof(MBB_MS_SYS_CAPS_INFO))
        {
            TraceError(WMBCLASS_OID, "[MbbNdis][ReqId=0x%04x] INSUFFICIENT BufferSize for NDIS_STATUS_WWAN_SYS_CAPS_INFO, BufferSize[Received=%d Expected=%d]",
                Request->RequestId,
                InBufferSize,
                sizeof(MBB_MS_SYS_CAPS_INFO)
                );
            WwanStatus = WWAN_STATUS_FAILURE;
            break;
        }

        MbbUtilMbbToWwanSysCapsInfo(
            MbbSysCapsInfo,
            &(NdisSysCapsInfo.SysCapsInfo)
            );
    } while (FALSE);

    //
    // Restore the device returned operational status, if any.
    //
    if (WwanStatus != WWAN_STATUS_SUCCESS &&
        MbbUtilMbbToWwanStatus(MbbStatus) != WWAN_STATUS_SUCCESS)
    {
        WwanStatus = MbbUtilMbbToWwanStatus(MbbStatus);
    }

    NdisSysCapsInfo.uStatus = WwanStatus;

    StatusIndication.StatusBuffer = &NdisSysCapsInfo;
    StatusIndication.StatusBufferSize = sizeof(NDIS_WWAN_SYS_CAPS_INFO);

    TraceInfo(WMBCLASS_OID, "[MbbNdis][ReqId=0x%04x] Indicating NDIS_STATUS_WWAN_SYS_CAPS_INFO with status=%!WwanStatus!",
        Request->RequestId,
        WwanStatus
        );

    MbbNdisMiniportIndicateRequestStatus(
        Request,
        &StatusIndication
        );

    return NDIS_STATUS_SUCCESS;
}

NDIS_STATUS
MbbNdisDeviceCapsExStatusHandler(
    __in PMBB_REQUEST_CONTEXT               Request,
    __in NDIS_STATUS                        NdisStatus,
    __in MBB_STATUS                         MbbStatus,
    __in_bcount_opt(InBufferSize) PUCHAR    InBuffer,
    __in ULONG                              InBufferSize
    )
{
    BOOLEAN                     IndicationFailure = TRUE;
    BOOLEAN                     IsMultiMode = FALSE;
    WWAN_STATUS                 WwanStatus = WWAN_STATUS_SUCCESS;
    NDIS_STATUS_INDICATION      StatusIndication = { 0 };
    ULONG                       NdisDeviceCapsExSize = 0;
    PNDIS_WWAN_DEVICE_CAPS_EX   NdisDeviceCapsEx = NULL;
    NDIS_WWAN_DEVICE_CAPS_EX    LocalNdisDeviceCapsEx = { 0 };
    PMBB_MS_DEVICE_CAPS_INFO_V2 MbbDeviceCapsEx2 = (PMBB_MS_DEVICE_CAPS_INFO_V2)InBuffer;
    PMINIPORT_ADAPTER_CONTEXT   Adapter = NULL;

    do
    {
        MBB_INITIALIZE_REQUEST_NDIS_STATUS_INDICATION(
            &StatusIndication,
            Request,
            NDIS_STATUS_WWAN_DEVICE_CAPS_EX
            );

        WwanStatus = MbbUtilMbbToWwanStatus(MbbStatus);

        if (NdisStatus != NDIS_STATUS_SUCCESS)
        {
            TraceError(WMBCLASS_OID, "[MbbNdis][ReqId=0x%04x] FAILED to get data for NDIS_STATUS_WWAN_DEVICE_CAPS, status=%!STATUS!",
                Request->RequestId,
                NdisStatus
                );
            WwanStatus = WWAN_STATUS_FAILURE;
            break;
        }

        if (MbbDeviceCapsEx2 == NULL ||
            InBufferSize < (ULONG)FIELD_OFFSET(MBB_MS_DEVICE_CAPS_INFO_V2, DataBuffer))
        {
            TraceError(WMBCLASS_OID, "[MbbNdis][ReqId=0x%04x] INSUFFICIENT BufferSize for NDIS_STATUS_WWAN_DEVICE_CAPS_EX, BufferSize[Received=%d Expected=%d]",
                Request->RequestId,
                InBufferSize,
                FIELD_OFFSET(MBB_MS_DEVICE_CAPS_INFO_V2, DataBuffer)
                );
            WwanStatus = WWAN_STATUS_FAILURE;
            break;
        }

        NdisStatus = MbbIsVariableFieldValid(
            InBufferSize,
            MbbDeviceCapsEx2->CustomDataClass.Offset,
            MbbDeviceCapsEx2->CustomDataClass.Size,
            MBB_MAXIMUM_DATA_CLASS_NAME_LENGTH,
            sizeof(WCHAR)
            );

        if (NdisStatus != NDIS_STATUS_SUCCESS)
        {
            TraceError(WMBCLASS_OID, "[MbbNdis][ReqId=0x%04x] Bad custom data field, Offset=%d, Size=%d, ReceivedSize=%d",
                Request->RequestId,
                MbbDeviceCapsEx2->CustomDataClass.Offset,
                MbbDeviceCapsEx2->CustomDataClass.Size,
                InBufferSize
                );
            WwanStatus = WWAN_STATUS_FAILURE;
            break;
        }

        NdisStatus = MbbIsVariableFieldValid(
            InBufferSize,
            MbbDeviceCapsEx2->DeviceIdString.Offset,
            MbbDeviceCapsEx2->DeviceIdString.Size,
            MBB_MAXIMUM_DEVICE_ID_LENGTH,
            sizeof(WCHAR)
            );

        if (NdisStatus != NDIS_STATUS_SUCCESS)
        {
            TraceError(WMBCLASS_OID, "[MbbNdis][ReqId=0x%04x] DeviceId data field, Offset=%d, Size=%d, ReceivedSize=%d",
                Request->RequestId,
                MbbDeviceCapsEx2->DeviceIdString.Offset,
                MbbDeviceCapsEx2->DeviceIdString.Size,
                InBufferSize
                );
            WwanStatus = WWAN_STATUS_FAILURE;
            break;
        }

        NdisStatus = MbbIsVariableFieldValid(
            InBufferSize,
            MbbDeviceCapsEx2->FirmwareInfo.Offset,
            MbbDeviceCapsEx2->FirmwareInfo.Size,
            MBB_MAXIMUM_FIRMWARE_INFO_LENGTH,
            sizeof(WCHAR)
            );

        if (NdisStatus != NDIS_STATUS_SUCCESS)
        {
            TraceError(WMBCLASS_OID, "[MbbNdis][ReqId=0x%04x] FirmwareInfo data field, Offset=%d, Size=%d, ReceivedSize=%d",
                Request->RequestId,
                MbbDeviceCapsEx2->FirmwareInfo.Offset,
                MbbDeviceCapsEx2->FirmwareInfo.Size,
                InBufferSize
                );
            WwanStatus = WWAN_STATUS_FAILURE;
            break;
        }

        NdisDeviceCapsExSize = sizeof(NDIS_WWAN_DEVICE_CAPS_EX);
        // If only a single bit is set in CellularClass then the following expression should be zero.
        // Otherwise the following expression should be non-zero.
        IsMultiMode = (((MbbDeviceCapsEx2->CellularClass) & (MbbDeviceCapsEx2->CellularClass - 1)) != 0);
        if (IsMultiMode)
        {
            NdisDeviceCapsExSize += sizeof(WWAN_CELLULAR_CLASS) * (MbbCellularClassMaximum - 1);
        }

        NdisDeviceCapsEx = (PNDIS_WWAN_DEVICE_CAPS_EX)ALLOCATE_NONPAGED_POOL(NdisDeviceCapsExSize);

        if (NdisDeviceCapsEx == NULL)
        {
            TraceError(WMBCLASS_OID, "[MbbNdis][ReqId=0x%04x] FAILED to allocate %d bytes for NDIS_STATUS_WWAN_DEVICE_CAPS",
                Request->RequestId,
                NdisDeviceCapsExSize
                );
            WwanStatus = WWAN_STATUS_FAILURE;
            break;
        }

        RtlZeroMemory(NdisDeviceCapsEx, NdisDeviceCapsExSize);

        NdisDeviceCapsEx->Header.Type = NDIS_OBJECT_TYPE_DEFAULT;
        NdisDeviceCapsEx->Header.Size = sizeof(NDIS_WWAN_DEVICE_CAPS_EX);
        NdisDeviceCapsEx->Header.Revision = NDIS_WWAN_DEVICE_CAPS_EX_REVISION_1;

        // We need the Adapter to retrieve the Manufacturer and Model which are absent in MBB_MS_DEVICE_CAPS_INFO_V2.
        Adapter = (PMINIPORT_ADAPTER_CONTEXT)MbbReqMgrGetAdapterHandle(Request);

        MbbUtilMbbToWwanDeviceCapsV2(
            MbbDeviceCapsEx2,
            Adapter->BusParams.Manufacturer,
            MAX_PARAMETER_STRING,
            Adapter->BusParams.Model,
            MAX_PARAMETER_STRING,
            &(NdisDeviceCapsEx->DeviceCaps),
            IsMultiMode
            );

        NdisDeviceCapsEx->DeviceCaps.WwanOptionalServiceCaps = WWAN_OPTIONAL_SERVICE_CAPS_NONE;
        NdisDeviceCapsEx->DeviceCaps.WwanOptionalServiceCaps |=
            Adapter->AdapterFlags.IsProvisionedContextV2Capable ? WWAN_OPTIONAL_SERVICE_CAPS_PROVISIONED_CONTEXT_MGMT : 0;
        NdisDeviceCapsEx->DeviceCaps.WwanOptionalServiceCaps |=
            Adapter->AdapterFlags.IsNetworkBlacklistCapable ? WWAN_OPTIONAL_SERVICE_CAPS_NETWORK_BLACKLIST : 0;
        NdisDeviceCapsEx->DeviceCaps.WwanOptionalServiceCaps |=
            Adapter->AdapterFlags.IsLTEAttachConfigCapable ? WWAN_OPTIONAL_SERVICE_CAPS_LTE_ATTACH : 0;
        NdisDeviceCapsEx->DeviceCaps.WwanOptionalServiceCaps |=
            Adapter->AdapterFlags.IsMultiSIMCapable ? WWAN_OPTIONAL_SERVICE_CAPS_MULTI_SIM : 0;
        NdisDeviceCapsEx->DeviceCaps.WwanOptionalServiceCaps |=
            Adapter->AdapterFlags.IsSARCapable ? WWAN_OPTIONAL_SERVICE_CAPS_SAR : 0;
        NdisDeviceCapsEx->DeviceCaps.WwanOptionalServiceCaps |=
            Adapter->AdapterFlags.IsPcoCapable ? WWAN_OPTIONAL_SERVICE_CAPS_PCO : 0;
        NdisDeviceCapsEx->DeviceCaps.WwanOptionalServiceCaps |=
            Adapter->AdapterFlags.IsUiccLowLevelCapable ? WWAN_OPTIONAL_SERVICE_CAPS_UICC_RESET : 0;
        NdisDeviceCapsEx->DeviceCaps.WwanOptionalServiceCaps |=
            Adapter->AdapterFlags.IsDeviceResetCapable ? WWAN_OPTIONAL_SERVICE_CAPS_DEVICE_RESET : 0;
        NdisDeviceCapsEx->DeviceCaps.WwanOptionalServiceCaps |=
            Adapter->AdapterFlags.IsBaseStationsInfoCapable ? WWAN_OPTIONAL_SERVICE_CAPS_BASE_STATIONS_INFO : 0;

        IndicationFailure = FALSE;
    } while (FALSE);

    //
    // Restore the device returned operational status, if any.
    //
    if (WwanStatus != WWAN_STATUS_SUCCESS &&
        MbbUtilMbbToWwanStatus(MbbStatus) != WWAN_STATUS_SUCCESS)
    {
        WwanStatus = MbbUtilMbbToWwanStatus(MbbStatus);
    }

    if (!IndicationFailure)
    {
        NdisDeviceCapsEx->uStatus = WwanStatus;

        StatusIndication.StatusBuffer = NdisDeviceCapsEx;
        StatusIndication.StatusBufferSize = NdisDeviceCapsExSize;
    }
    else
    {
        RtlZeroMemory(&LocalNdisDeviceCapsEx, sizeof(NDIS_WWAN_DEVICE_CAPS_EX));

        LocalNdisDeviceCapsEx.Header.Type = NDIS_OBJECT_TYPE_DEFAULT;
        LocalNdisDeviceCapsEx.Header.Size = sizeof(NDIS_WWAN_DEVICE_CAPS_EX);
        LocalNdisDeviceCapsEx.Header.Revision = NDIS_WWAN_DEVICE_CAPS_EX_REVISION_1;
        LocalNdisDeviceCapsEx.uStatus = WwanStatus;

        StatusIndication.StatusBuffer = &LocalNdisDeviceCapsEx;
        StatusIndication.StatusBufferSize = sizeof(NDIS_WWAN_DEVICE_CAPS_EX);
    }

    TraceInfo(WMBCLASS_OID, "[MbbNdis][ReqId=0x%04x] Indicating NDIS_STATUS_WWAN_DEVICE_CAPS with status=%!WwanStatus!",
        Request->RequestId,
        WwanStatus
        );

    MbbNdisMiniportIndicateRequestStatus(
        Request,
        &StatusIndication
        );

    if (NdisDeviceCapsEx != NULL)
    {
        FREE_POOL(NdisDeviceCapsEx);
    }

    return NDIS_STATUS_SUCCESS;
}

NDIS_STATUS
MbbNdisSysSlotMappingsStatusHandler(
    __in PMBB_REQUEST_CONTEXT               Request,
    __in NDIS_STATUS                        NdisStatus,
    __in MBB_STATUS                         MbbStatus,
    __in_bcount_opt(InBufferSize) PUCHAR    InBuffer,
    __in ULONG                              InBufferSize
    )
{
    BOOLEAN                                 IndicationFailure = TRUE;
    WWAN_STATUS                             WwanStatus = WWAN_STATUS_SUCCESS;
    NDIS_STATUS_INDICATION                  StatusIndication = { 0 };
    ULONG                                   NdisDeviceSlotMappingInfoSize = 0;
    PNDIS_WWAN_DEVICE_SLOT_MAPPING_INFO     NdisDeviceSlotMappingInfo = NULL;
    NDIS_WWAN_DEVICE_SLOT_MAPPING_INFO      LocalNdisDeviceSlotMappingInfo = { 0 };
    PMBB_MS_DEVICE_SLOT_MAPPING_INFO        MbbDeviceSlotMappingInfo = (PMBB_MS_DEVICE_SLOT_MAPPING_INFO)InBuffer;

    do
    {
        MBB_INITIALIZE_REQUEST_NDIS_STATUS_INDICATION(
            &StatusIndication,
            Request,
            NDIS_STATUS_WWAN_DEVICE_SLOT_MAPPING_INFO
            );

        WwanStatus = MbbUtilMbbToWwanStatus(MbbStatus);

        if (NdisStatus != NDIS_STATUS_SUCCESS)
        {
            TraceError(WMBCLASS_OID, "[MbbNdis][ReqId=0x%04x] FAILED to get data for NDIS_STATUS_WWAN_DEVICE_SLOT_MAPPING_INFO, status=%!STATUS!",
                Request->RequestId,
                NdisStatus
                );
            WwanStatus = WWAN_STATUS_FAILURE;
            break;
        }

        if (MbbDeviceSlotMappingInfo == NULL ||
            InBufferSize < (ULONG)FIELD_OFFSET(MBB_MS_DEVICE_SLOT_MAPPING_INFO, SlotMapList))
        {
            TraceError(WMBCLASS_OID, "[MbbNdis][ReqId=0x%04x] INSUFFICIENT BufferSize for NDIS_STATUS_WWAN_DEVICE_SLOT_MAPPING_INFO, BufferSize[Received=%d Expected=%d]",
                Request->RequestId,
                InBufferSize,
                FIELD_OFFSET(MBB_MS_DEVICE_SLOT_MAPPING_INFO, SlotMapList)
                );
            WwanStatus = WWAN_STATUS_FAILURE;
            break;
        }

        NdisStatus = MbbIsArrayFieldValid(
            InBufferSize,
            MbbDeviceSlotMappingInfo->SlotMapList[0].Offset,
            MbbDeviceSlotMappingInfo->MapCount,
            sizeof(ULONG)
            );

        if (NdisStatus != NDIS_STATUS_SUCCESS)
        {
            TraceError(WMBCLASS_OID, "[MbbNdis][ReqId=0x%04x] Bad slot map list, Offset=%d, ElementCount=%d, ElementSize=%d, ReceivedSize=%d",
                Request->RequestId,
                MbbDeviceSlotMappingInfo->SlotMapList[0].Offset,
                MbbDeviceSlotMappingInfo->MapCount,
                sizeof(ULONG),
                InBufferSize
                );
            WwanStatus = WWAN_STATUS_FAILURE;
            break;
        }

        NdisDeviceSlotMappingInfoSize = sizeof(NDIS_WWAN_DEVICE_SLOT_MAPPING_INFO);
        NdisDeviceSlotMappingInfoSize += sizeof(ULONG) * (MbbDeviceSlotMappingInfo->MapCount);

        NdisDeviceSlotMappingInfo = (PNDIS_WWAN_DEVICE_SLOT_MAPPING_INFO)ALLOCATE_NONPAGED_POOL(NdisDeviceSlotMappingInfoSize);

        if (NdisDeviceSlotMappingInfo == NULL)
        {
            TraceError(WMBCLASS_OID, "[MbbNdis][ReqId=0x%04x] FAILED to allocate %d bytes for NDIS_STATUS_WWAN_DEVICE_SLOT_MAPPING_INFO",
                Request->RequestId,
                NdisDeviceSlotMappingInfoSize
                );
            WwanStatus = WWAN_STATUS_FAILURE;
            break;
        }

        RtlZeroMemory(NdisDeviceSlotMappingInfo, NdisDeviceSlotMappingInfoSize);

        NdisDeviceSlotMappingInfo->Header.Type = NDIS_OBJECT_TYPE_DEFAULT;
        NdisDeviceSlotMappingInfo->Header.Size = sizeof(NDIS_WWAN_DEVICE_SLOT_MAPPING_INFO);
        NdisDeviceSlotMappingInfo->Header.Revision = NDIS_WWAN_DEVICE_SLOT_MAPPING_INFO_REVISION_1;

        MbbUtilMbbToWwanDeviceSlotMappingInfo(
            MbbDeviceSlotMappingInfo,
            &(NdisDeviceSlotMappingInfo->DeviceSlotMappingInfo)
            );
        IndicationFailure = FALSE;
    } while (FALSE);

    //
    // Restore the device returned operational status, if any.
    //
    if (WwanStatus != WWAN_STATUS_SUCCESS &&
        MbbUtilMbbToWwanStatus(MbbStatus) != WWAN_STATUS_SUCCESS)
    {
        WwanStatus = MbbUtilMbbToWwanStatus(MbbStatus);
    }

    if (!IndicationFailure)
    {
        NdisDeviceSlotMappingInfo->uStatus = WwanStatus;

        StatusIndication.StatusBuffer = NdisDeviceSlotMappingInfo;
        StatusIndication.StatusBufferSize = NdisDeviceSlotMappingInfoSize;
    }
    else
    {
        RtlZeroMemory(&LocalNdisDeviceSlotMappingInfo, sizeof(NDIS_WWAN_DEVICE_SLOT_MAPPING_INFO));

        LocalNdisDeviceSlotMappingInfo.Header.Type = NDIS_OBJECT_TYPE_DEFAULT;
        LocalNdisDeviceSlotMappingInfo.Header.Size = sizeof(NDIS_WWAN_DEVICE_SLOT_MAPPING_INFO);
        LocalNdisDeviceSlotMappingInfo.Header.Revision = NDIS_WWAN_DEVICE_SLOT_MAPPING_INFO_REVISION_1;
        LocalNdisDeviceSlotMappingInfo.uStatus = WwanStatus;

        StatusIndication.StatusBuffer = &LocalNdisDeviceSlotMappingInfo;
        StatusIndication.StatusBufferSize = sizeof(NDIS_WWAN_DEVICE_SLOT_MAPPING_INFO);
    }

    TraceInfo(WMBCLASS_OID, "[MbbNdis][ReqId=0x%04x] Indicating NDIS_STATUS_WWAN_DEVICE_SLOT_MAPPING_INFO with status=%!WwanStatus!",
        Request->RequestId,
        WwanStatus
        );

    MbbNdisMiniportIndicateRequestStatus(
        Request,
        &StatusIndication
        );

    if (NdisDeviceSlotMappingInfo != NULL)
    {
        FREE_POOL(NdisDeviceSlotMappingInfo);
    }

    return NDIS_STATUS_SUCCESS;
}

NDIS_STATUS
MbbNdisSlotInfoStatusHandler(
    __in PMBB_REQUEST_CONTEXT               Request,
    __in NDIS_STATUS                        NdisStatus,
    __in MBB_STATUS                         MbbStatus,
    __in_bcount_opt(InBufferSize) PUCHAR    InBuffer,
    __in ULONG                              InBufferSize
    )
{
    WWAN_STATUS                 WwanStatus = WWAN_STATUS_SUCCESS;
    NDIS_STATUS_INDICATION      StatusIndication = { 0 };
    NDIS_WWAN_SLOT_INFO         NdisSlotInfo = { 0 };
    PMBB_MS_SLOT_INFO           MbbSlotInfo = (PMBB_MS_SLOT_INFO)InBuffer;

    do
    {
        MBB_INITIALIZE_REQUEST_NDIS_STATUS_INDICATION(
            &StatusIndication,
            Request,
            NDIS_STATUS_WWAN_SLOT_INFO
            );

        RtlZeroMemory(&NdisSlotInfo, sizeof(NDIS_WWAN_SLOT_INFO));

        NdisSlotInfo.Header.Type = NDIS_OBJECT_TYPE_DEFAULT;
        NdisSlotInfo.Header.Size = sizeof(NDIS_WWAN_SLOT_INFO);
        NdisSlotInfo.Header.Revision = NDIS_WWAN_SLOT_INFO_REVISION_1;

        WwanStatus = MbbUtilMbbToWwanStatus(MbbStatus);

        if (NdisStatus != NDIS_STATUS_SUCCESS)
        {
            TraceError(WMBCLASS_OID, "[MbbNdis][ReqId=0x%04x] FAILED to get data for NDIS_STATUS_WWAN_SLOT_INFO, status=%!STATUS!",
                Request->RequestId,
                NdisStatus
                );
            WwanStatus = WWAN_STATUS_FAILURE;
            break;
        }

        if (MbbSlotInfo == NULL ||
            InBufferSize < sizeof(MBB_MS_SLOT_INFO))
        {
            TraceError(WMBCLASS_OID, "[MbbNdis][ReqId=0x%04x] INSUFFICIENT BufferSize for NDIS_STATUS_WWAN_SLOT_INFO, BufferSize[Received=%d Expected=%d]",
                Request->RequestId,
                InBufferSize,
                sizeof(MBB_MS_SLOT_INFO)
                );
            WwanStatus = WWAN_STATUS_FAILURE;
            break;
        }

        MbbUtilMbbToWwanSlotInfo(
            MbbSlotInfo,
            &(NdisSlotInfo.SlotInfo)
            );
    } while (FALSE);

    //
    // Restore the device returned operational status, if any.
    //
    if (WwanStatus != WWAN_STATUS_SUCCESS &&
        MbbUtilMbbToWwanStatus(MbbStatus) != WWAN_STATUS_SUCCESS)
    {
        WwanStatus = MbbUtilMbbToWwanStatus(MbbStatus);
    }

    NdisSlotInfo.uStatus = WwanStatus;

    StatusIndication.StatusBuffer = &NdisSlotInfo;
    StatusIndication.StatusBufferSize = sizeof(NDIS_WWAN_SLOT_INFO);

    TraceInfo(WMBCLASS_OID, "[MbbNdis][ReqId=0x%04x] Indicating NDIS_STATUS_WWAN_SLOT_INFO with status=%!WwanStatus!",
        Request->RequestId,
        WwanStatus
        );

    MbbNdisMiniportIndicateRequestStatus(
        Request,
        &StatusIndication
        );

    return NDIS_STATUS_SUCCESS;
}

NDIS_STATUS
MbbNdisDeviceBindingsStatusHandler(
    __in PMBB_REQUEST_CONTEXT               Request,
    __in NDIS_STATUS                        NdisStatus,
    __in MBB_STATUS                         MbbStatus,
    __in_bcount_opt(InBufferSize) PUCHAR    InBuffer,
    __in ULONG                              InBufferSize
    )
{
    BOOLEAN                         IndicationFailure = TRUE;
    WWAN_STATUS                     WwanStatus = WWAN_STATUS_SUCCESS;
    NDIS_STATUS_INDICATION          StatusIndication = { 0 };
    ULONG                           NdisDeviceBindingsInfoSize = 0;
    PNDIS_WWAN_DEVICE_BINDINGS_INFO NdisDeviceBindingsInfo = NULL;
    NDIS_WWAN_DEVICE_BINDINGS_INFO  LocalNdisDeviceBindingsInfo = { 0 };
    PMBB_DEVICE_BINDINGS_INFO       MbbDeviceBindingsInfo = (PMBB_DEVICE_BINDINGS_INFO)InBuffer;
    ULONG                           AppCount = 0;
    ULONG                           AppIndex = 0;
    PMBB_ARRAY_ELEMENT              AppId = NULL;

    do
    {
        MBB_INITIALIZE_REQUEST_NDIS_STATUS_INDICATION(
            &StatusIndication,
            Request,
            NDIS_STATUS_WWAN_DEVICE_BINDINGS_INFO
            );

        WwanStatus = MbbUtilMbbToWwanStatus(MbbStatus);

        if (NdisStatus != NDIS_STATUS_SUCCESS)
        {
            TraceError(WMBCLASS_OID, "[MbbNdis][ReqId=0x%04x] FAILED to get data for NDIS_STATUS_WWAN_DEVICE_BINDINGS_INFO, status=%!STATUS!",
                Request->RequestId,
                NdisStatus
                );
            WwanStatus = WWAN_STATUS_FAILURE;
            break;
        }

        if (MbbDeviceBindingsInfo == NULL ||
            InBufferSize < (ULONG)FIELD_OFFSET(MBB_DEVICE_BINDINGS_INFO, ApplicationList))
        {
            TraceError(WMBCLASS_OID, "[MbbNdis][ReqId=0x%04x] INSUFFICIENT BufferSize for NDIS_STATUS_WWAN_DEVICE_BINDINGS_INFO, BufferSize[Received=%d Expected=%d]",
                Request->RequestId,
                InBufferSize,
                FIELD_OFFSET(MBB_DEVICE_BINDINGS_INFO, ApplicationList)
                );
            WwanStatus = WWAN_STATUS_FAILURE;
            break;
        }

        AppCount = MbbDeviceBindingsInfo->ApplicationCount;
        for (AppIndex = 0; AppIndex < AppCount; AppIndex++)
        {
            // Check whether the struct MBB_UICC_FILE_PATH is valid.
            NdisStatus = MbbIsVariableFieldValid(
                InBufferSize,
                MbbDeviceBindingsInfo->ApplicationList[AppIndex].Offset,
                MbbDeviceBindingsInfo->ApplicationList[AppIndex].Size,
                FIELD_OFFSET(MBB_UICC_FILE_PATH, DataBuffer) + sizeof(BYTE) * WWAN_UICC_APP_ID_MAX_LEN + sizeof(USHORT) * WWAN_UICC_FILE_PATH_MAX_LEN,
                sizeof(BYTE)
                );

            if (NdisStatus != NDIS_STATUS_SUCCESS)
            {
                TraceError(WMBCLASS_OID, "[MbbNdis][ReqId=0x%04x] Bad MBB_UICC_FILE_PATH for app %d, Offset=%d, Size=%d, ReceivedSize=%d",
                    Request->RequestId,
                    AppIndex,
                    MbbDeviceBindingsInfo->ApplicationList[AppIndex].Offset,
                    MbbDeviceBindingsInfo->ApplicationList[AppIndex].Size,
                    InBufferSize
                    );
                WwanStatus = WWAN_STATUS_FAILURE;
                break;
            }

            // Check whether the AppId in the struct MBB_UICC_FILE_PATH is valid. Note that the FilePath in the struct MBB_UICC_FILE_PATH is not used.
            AppId = &(((PMBB_UICC_FILE_PATH)((PUCHAR)MbbDeviceBindingsInfo + MbbDeviceBindingsInfo->ApplicationList[AppIndex].Offset))->AppId);
            NdisStatus = MbbIsVariableFieldValid(
                MbbDeviceBindingsInfo->ApplicationList[AppIndex].Size,
                AppId->Offset,
                AppId->Size,
                WWAN_UICC_APP_ID_MAX_LEN,
                sizeof(BYTE)
                );

            if (NdisStatus != NDIS_STATUS_SUCCESS)
            {
                TraceError(WMBCLASS_OID, "[MbbNdis][ReqId=0x%04x] Bad AppId for app %d, Offset=%d, Size=%d, ReceivedSize=%d",
                    Request->RequestId,
                    AppIndex,
                    AppId->Offset,
                    AppId->Size,
                    MbbDeviceBindingsInfo->ApplicationList[AppIndex].Size
                    );
                WwanStatus = WWAN_STATUS_FAILURE;
                break;
            }
        }

        NdisDeviceBindingsInfoSize = sizeof(NDIS_WWAN_DEVICE_BINDINGS_INFO);
        NdisDeviceBindingsInfoSize += sizeof(WWAN_UICC_FILE_PATH) * AppCount;

        NdisDeviceBindingsInfo = (PNDIS_WWAN_DEVICE_BINDINGS_INFO)ALLOCATE_NONPAGED_POOL(NdisDeviceBindingsInfoSize);

        if (NdisDeviceBindingsInfo == NULL)
        {
            TraceError(WMBCLASS_OID, "[MbbNdis][ReqId=0x%04x] FAILED to allocate %d bytes for NDIS_STATUS_WWAN_DEVICE_BINDINGS_INFO",
                Request->RequestId,
                NdisDeviceBindingsInfoSize
                );
            WwanStatus = WWAN_STATUS_FAILURE;
            break;
        }

        RtlZeroMemory(NdisDeviceBindingsInfo, NdisDeviceBindingsInfoSize);

        NdisDeviceBindingsInfo->Header.Type = NDIS_OBJECT_TYPE_DEFAULT;
        NdisDeviceBindingsInfo->Header.Size = sizeof(NDIS_WWAN_DEVICE_BINDINGS_INFO);
        NdisDeviceBindingsInfo->Header.Revision = NDIS_WWAN_DEVICE_BINDINGS_INFO_REVISION_1;

        MbbUtilMbbToWwanDeviceBindingsInfo(
            MbbDeviceBindingsInfo,
            &(NdisDeviceBindingsInfo->DeviceBindingsInfo)
            );
        IndicationFailure = FALSE;
    } while (FALSE);

    //
    // Restore the device returned operational status, if any.
    //
    if (WwanStatus != WWAN_STATUS_SUCCESS &&
        MbbUtilMbbToWwanStatus(MbbStatus) != WWAN_STATUS_SUCCESS)
    {
        WwanStatus = MbbUtilMbbToWwanStatus(MbbStatus);
    }

    if (!IndicationFailure)
    {
        NdisDeviceBindingsInfo->uStatus = WwanStatus;

        StatusIndication.StatusBuffer = NdisDeviceBindingsInfo;
        StatusIndication.StatusBufferSize = NdisDeviceBindingsInfoSize;
    }
    else
    {
        RtlZeroMemory(&LocalNdisDeviceBindingsInfo, sizeof(NDIS_WWAN_DEVICE_BINDINGS_INFO));

        LocalNdisDeviceBindingsInfo.Header.Type = NDIS_OBJECT_TYPE_DEFAULT;
        LocalNdisDeviceBindingsInfo.Header.Size = sizeof(NDIS_WWAN_DEVICE_BINDINGS_INFO);
        LocalNdisDeviceBindingsInfo.Header.Revision = NDIS_WWAN_DEVICE_BINDINGS_INFO_REVISION_1;
        LocalNdisDeviceBindingsInfo.uStatus = WwanStatus;

        StatusIndication.StatusBuffer = &LocalNdisDeviceBindingsInfo;
        StatusIndication.StatusBufferSize = sizeof(NDIS_WWAN_DEVICE_BINDINGS_INFO);
    }

    TraceInfo(WMBCLASS_OID, "[MbbNdis][ReqId=0x%04x] Indicating NDIS_STATUS_WWAN_DEVICE_BINDINGS_INFO with status=%!WwanStatus!",
        Request->RequestId,
        WwanStatus
        );

    MbbNdisMiniportIndicateRequestStatus(
        Request,
        &StatusIndication
        );

    if (NdisDeviceBindingsInfo != NULL)
    {
        FREE_POOL(NdisDeviceBindingsInfo);
    }

    return NDIS_STATUS_SUCCESS;
}

NDIS_STATUS
MbbNdisRegisterStateExStatusHandler(
    __in PMBB_REQUEST_CONTEXT               Request,
    __in NDIS_STATUS                        NdisStatus,
    __in MBB_STATUS                         MbbStatus,
    __in_bcount_opt(InBufferSize) PUCHAR    InBuffer,
    __in ULONG                              InBufferSize
    )
{
    BOOLEAN                                 IndicationFailure = TRUE;
    WWAN_STATUS                             WwanStatus = WWAN_STATUS_SUCCESS;
    NDIS_STATUS_INDICATION                  StatusIndication = { 0 };
    ULONG                                   NdisRegistrationStateInfosExSize = 0;
    PNDIS_WWAN_REGISTRATION_STATE_INFOS_EX  NdisRegistrationStateInfosEx = NULL;
    NDIS_WWAN_REGISTRATION_STATE_INFOS_EX   LocalNdisRegistrationStateInfosEx = { 0 };
    PMBB_REGISTRATION_STATE_INFOS_V2        MbbRegistrationStateInfosV2 = (PMBB_REGISTRATION_STATE_INFOS_V2)InBuffer;
    ULONG                                   RegistrationStateCount = 0;
    ULONG                                   RegistrationStateIndex = 0;
    PMBB_REGISTRATION_STATE_V2              RegistrationState = NULL;

    do
    {
        MBB_INITIALIZE_REQUEST_NDIS_STATUS_INDICATION(
            &StatusIndication,
            Request,
            NDIS_STATUS_WWAN_REGISTER_STATE
            );

        WwanStatus = MbbUtilMbbToWwanStatus(MbbStatus);

        if (NdisStatus != NDIS_STATUS_SUCCESS)
        {
            TraceError(WMBCLASS_OID, "[MbbNdis][ReqId=0x%04x] FAILED to get data for NDIS_STATUS_WWAN_REGISTER_STATE, status=%!STATUS!",
                Request->RequestId,
                NdisStatus
                );
            WwanStatus = WWAN_STATUS_FAILURE;
            break;
        }

        if (MbbRegistrationStateInfosV2 == NULL ||
            InBufferSize < (ULONG)FIELD_OFFSET(MBB_REGISTRATION_STATE_INFOS_V2, RegistrationStateList))
        {
            TraceError(WMBCLASS_OID, "[MbbNdis][ReqId=0x%04x] INSUFFICIENT BufferSize for NDIS_STATUS_WWAN_REGISTER_STATE, BufferSize[Received=%d Expected=%d]",
                Request->RequestId,
                InBufferSize,
                FIELD_OFFSET(MBB_REGISTRATION_STATE_INFOS_V2, RegistrationStateList)
                );
            WwanStatus = WWAN_STATUS_FAILURE;
            break;
        }

        RegistrationStateCount = MbbRegistrationStateInfosV2->ElementCount;
        for (RegistrationStateIndex = 0; RegistrationStateIndex < RegistrationStateCount; RegistrationStateIndex++)
        {
            // Check whether the struct MBB_REGISTRATION_STATE_V2 is valid.
            NdisStatus = MbbIsVariableFieldValid(
                InBufferSize,
                MbbRegistrationStateInfosV2->RegistrationStateList[RegistrationStateIndex].Offset,
                MbbRegistrationStateInfosV2->RegistrationStateList[RegistrationStateIndex].Size,
                FIELD_OFFSET(MBB_REGISTRATION_STATE_V2, DataBuffer) + sizeof(WCHAR) * (WWAN_PROVIDERID_LEN + WWAN_PROVIDERNAME_LEN + WWAN_ROAMTEXT_LEN - 3), // Minus 3 null terminators.
                sizeof(BYTE)
                );

            if (NdisStatus != NDIS_STATUS_SUCCESS)
            {
                TraceError(WMBCLASS_OID, "[MbbNdis][ReqId=0x%04x] Bad MBB_REGISTRATION_STATE_V2 for RegistrationState %d, Offset=%d, Size=%d, ReceivedSize=%d",
                    Request->RequestId,
                    RegistrationStateIndex,
                    MbbRegistrationStateInfosV2->RegistrationStateList[RegistrationStateIndex].Offset,
                    MbbRegistrationStateInfosV2->RegistrationStateList[RegistrationStateIndex].Size,
                    InBufferSize
                    );
                WwanStatus = WWAN_STATUS_FAILURE;
                break;
            }

            // Check whether the ProviderId in the struct MBB_REGISTRATION_STATE_V2 is valid.
            RegistrationState = (PMBB_REGISTRATION_STATE_V2)((PUCHAR)MbbRegistrationStateInfosV2 + MbbRegistrationStateInfosV2->RegistrationStateList[RegistrationStateIndex].Offset);
            NdisStatus = MbbIsVariableFieldValid(
                MbbRegistrationStateInfosV2->RegistrationStateList[RegistrationStateIndex].Size,
                RegistrationState->ProviderId.Offset,
                RegistrationState->ProviderId.Size,
                WWAN_PROVIDERID_LEN - 1,    // Minus the null terminator.
                sizeof(WCHAR)
                );

            if (NdisStatus != NDIS_STATUS_SUCCESS)
            {
                TraceError(WMBCLASS_OID, "[MbbNdis][ReqId=0x%04x] Bad ProviderId for RegistrationState %d, Offset=%d, Size=%d, ReceivedSize=%d",
                    Request->RequestId,
                    RegistrationStateIndex,
                    RegistrationState->ProviderId.Offset,
                    RegistrationState->ProviderId.Size,
                    MbbRegistrationStateInfosV2->RegistrationStateList[RegistrationStateIndex].Size
                    );
                WwanStatus = WWAN_STATUS_FAILURE;
                break;
            }

            // Check whether the ProviderName in the struct MBB_REGISTRATION_STATE_V2 is valid.
            NdisStatus = MbbIsVariableFieldValid(
                MbbRegistrationStateInfosV2->RegistrationStateList[RegistrationStateIndex].Size,
                RegistrationState->ProviderName.Offset,
                RegistrationState->ProviderName.Size,
                WWAN_PROVIDERNAME_LEN - 1,    // Minus the null terminator.
                sizeof(WCHAR)
                );

            if (NdisStatus != NDIS_STATUS_SUCCESS)
            {
                TraceError(WMBCLASS_OID, "[MbbNdis][ReqId=0x%04x] Bad ProviderName for RegistrationState %d, Offset=%d, Size=%d, ReceivedSize=%d",
                    Request->RequestId,
                    RegistrationStateIndex,
                    RegistrationState->ProviderName.Offset,
                    RegistrationState->ProviderName.Size,
                    MbbRegistrationStateInfosV2->RegistrationStateList[RegistrationStateIndex].Size
                    );
                WwanStatus = WWAN_STATUS_FAILURE;
                break;
            }

            // Check whether the RoamingText in the struct MBB_REGISTRATION_STATE_V2 is valid.
            NdisStatus = MbbIsVariableFieldValid(
                MbbRegistrationStateInfosV2->RegistrationStateList[RegistrationStateIndex].Size,
                RegistrationState->RoamingText.Offset,
                RegistrationState->RoamingText.Size,
                WWAN_ROAMTEXT_LEN - 1,    // Minus the null terminator.
                sizeof(WCHAR)
                );

            if (NdisStatus != NDIS_STATUS_SUCCESS)
            {
                TraceError(WMBCLASS_OID, "[MbbNdis][ReqId=0x%04x] Bad RoamingText for RegistrationState %d, Offset=%d, Size=%d, ReceivedSize=%d",
                    Request->RequestId,
                    RegistrationStateIndex,
                    RegistrationState->RoamingText.Offset,
                    RegistrationState->RoamingText.Size,
                    MbbRegistrationStateInfosV2->RegistrationStateList[RegistrationStateIndex].Size
                    );
                WwanStatus = WWAN_STATUS_FAILURE;
                break;
            }
        }

        NdisRegistrationStateInfosExSize = sizeof(NDIS_WWAN_REGISTRATION_STATE_INFOS_EX);
        NdisRegistrationStateInfosExSize += sizeof(WWAN_REGISTRATION_STATE_EX) * RegistrationStateCount;

        NdisRegistrationStateInfosEx = (PNDIS_WWAN_REGISTRATION_STATE_INFOS_EX)ALLOCATE_NONPAGED_POOL(NdisRegistrationStateInfosExSize);

        if (NdisRegistrationStateInfosEx == NULL)
        {
            TraceError(WMBCLASS_OID, "[MbbNdis][ReqId=0x%04x] FAILED to allocate %d bytes for NDIS_STATUS_WWAN_REGISTER_STATE",
                Request->RequestId,
                NdisRegistrationStateInfosExSize
                );
            WwanStatus = WWAN_STATUS_FAILURE;
            break;
        }

        RtlZeroMemory(NdisRegistrationStateInfosEx, NdisRegistrationStateInfosExSize);

        NdisRegistrationStateInfosEx->Header.Type = NDIS_OBJECT_TYPE_DEFAULT;
        NdisRegistrationStateInfosEx->Header.Size = sizeof(NDIS_WWAN_REGISTRATION_STATE_INFOS_EX);
        NdisRegistrationStateInfosEx->Header.Revision = NDIS_WWAN_REGISTRATION_STATE_REVISION_2;

        MbbUtilMbbToWwanRegistrationStateInfoV2(
            MbbRegistrationStateInfosV2,
            &(NdisRegistrationStateInfosEx->RegistrationStateInfos)
            );
        IndicationFailure = FALSE;
    } while (FALSE);

    //
    // Restore the device returned operational status, if any.
    //
    if (WwanStatus != WWAN_STATUS_SUCCESS &&
        MbbUtilMbbToWwanStatus(MbbStatus) != WWAN_STATUS_SUCCESS)
    {
        WwanStatus = MbbUtilMbbToWwanStatus(MbbStatus);
    }

    if (!IndicationFailure)
    {
        NdisRegistrationStateInfosEx->uStatus = WwanStatus;

        StatusIndication.StatusBuffer = NdisRegistrationStateInfosEx;
        StatusIndication.StatusBufferSize = NdisRegistrationStateInfosExSize;
    }
    else
    {
        RtlZeroMemory(&LocalNdisRegistrationStateInfosEx, sizeof(NDIS_WWAN_REGISTRATION_STATE_INFOS_EX));

        LocalNdisRegistrationStateInfosEx.Header.Type = NDIS_OBJECT_TYPE_DEFAULT;
        LocalNdisRegistrationStateInfosEx.Header.Size = sizeof(NDIS_WWAN_REGISTRATION_STATE_INFOS_EX);
        LocalNdisRegistrationStateInfosEx.Header.Revision = NDIS_WWAN_REGISTRATION_STATE_REVISION_2;
        LocalNdisRegistrationStateInfosEx.uStatus = WwanStatus;

        StatusIndication.StatusBuffer = &LocalNdisRegistrationStateInfosEx;
        StatusIndication.StatusBufferSize = sizeof(NDIS_WWAN_REGISTRATION_STATE_INFOS_EX);
    }

    TraceInfo(WMBCLASS_OID, "[MbbNdis][ReqId=0x%04x] Indicating NDIS_STATUS_WWAN_REGISTER_STATE with status=%!WwanStatus!",
        Request->RequestId,
        WwanStatus
        );

    MbbNdisMiniportIndicateRequestStatus(
        Request,
        &StatusIndication
        );

    if (NdisRegistrationStateInfosEx != NULL)
    {
        FREE_POOL(NdisRegistrationStateInfosEx);
    }

    return NDIS_STATUS_SUCCESS;
}

NDIS_STATUS
MbbNdisImsVoiceStateStatusHandler(
    __in PMBB_REQUEST_CONTEXT               Request,
    __in NDIS_STATUS                        NdisStatus,
    __in MBB_STATUS                         MbbStatus,
    __in_bcount_opt(InBufferSize) PUCHAR    InBuffer,
    __in ULONG                              InBufferSize
    )
{
    WWAN_STATUS                 WwanStatus = WWAN_STATUS_SUCCESS;
    NDIS_STATUS_INDICATION      StatusIndication = { 0 };
    NDIS_WWAN_IMS_VOICE_STATE   NdisImsVoiceStatus = { 0 };
    PMBB_IMS_VOICE_STATE        MbbImsVoiceStatus = (PMBB_IMS_VOICE_STATE)InBuffer;

    do
    {
        MBB_INITIALIZE_REQUEST_NDIS_STATUS_INDICATION(
            &StatusIndication,
            Request,
            NDIS_STATUS_WWAN_IMS_VOICE_STATE
            );

        RtlZeroMemory(&NdisImsVoiceStatus, sizeof(NdisImsVoiceStatus));

        NdisImsVoiceStatus.Header.Type = NDIS_OBJECT_TYPE_DEFAULT;
        NdisImsVoiceStatus.Header.Size = sizeof(NDIS_WWAN_IMS_VOICE_STATE);
        NdisImsVoiceStatus.Header.Revision = NDIS_WWAN_IMS_VOICE_STATE_REVISION_1;

        WwanStatus = MbbUtilMbbToWwanStatus(MbbStatus);

        if (NdisStatus != NDIS_STATUS_SUCCESS)
        {
            TraceError(WMBCLASS_OID, "[MbbNdis][ReqId=0x%04x] FAILED to get data for NDIS_STATUS_WWAN_IMS_VOICE_STATE, status=%!STATUS!",
                Request->RequestId,
                NdisStatus
                );
            WwanStatus = WWAN_STATUS_FAILURE;
            break;
        }

        if (MbbImsVoiceStatus == NULL ||
            InBufferSize < sizeof(MBB_IMS_VOICE_STATE))
        {
            TraceError(WMBCLASS_OID, "[MbbNdis][ReqId=0x%04x] INSUFFICIENT BufferSize for NDIS_STATUS_WWAN_IMS_VOICE_STATE, BufferSize[Received=%d Expected=%d]",
                Request->RequestId,
                InBufferSize,
                sizeof(MBB_IMS_VOICE_STATE)
                );
            WwanStatus = WWAN_STATUS_FAILURE;
            break;
        }

        NdisImsVoiceStatus.ImsVoiceState.ImsVoiceStatus = (WWAN_IMS_VOICE_STATUS)(MbbImsVoiceStatus->ImsVoiceStatus);
    } while (FALSE);

    //
    // Restore the device returned operational status, if any.
    //
    if (WwanStatus != WWAN_STATUS_SUCCESS &&
        MbbUtilMbbToWwanStatus(MbbStatus) != WWAN_STATUS_SUCCESS)
    {
        WwanStatus = MbbUtilMbbToWwanStatus(MbbStatus);
    }

    NdisImsVoiceStatus.uStatus = WwanStatus;

    StatusIndication.StatusBuffer = &NdisImsVoiceStatus;
    StatusIndication.StatusBufferSize = sizeof(NDIS_WWAN_IMS_VOICE_STATE);

    TraceInfo(WMBCLASS_OID, "[MbbNdis][ReqId=0x%04x] Indicating NDIS_STATUS_WWAN_IMS_VOICE_STATE with status=%!WwanStatus!",
        Request->RequestId,
        WwanStatus
        );

    MbbNdisMiniportIndicateRequestStatus(
        Request,
        &StatusIndication
        );

    return NDIS_STATUS_SUCCESS;
}

NDIS_STATUS
MbbNdisSignalStateExStatusHandler(
    __in PMBB_REQUEST_CONTEXT               Request,
    __in NDIS_STATUS                        NdisStatus,
    __in MBB_STATUS                         MbbStatus,
    __in_bcount_opt(InBufferSize) PUCHAR    InBuffer,
    __in ULONG                              InBufferSize
    )
{
    BOOLEAN                             IndicationFailure = TRUE;
    WWAN_STATUS                         WwanStatus = WWAN_STATUS_SUCCESS;
    NDIS_STATUS_INDICATION              StatusIndication = { 0 };
    ULONG                               NdisSignalStateInfosExSize = 0;
    PNDIS_WWAN_SIGNAL_STATE_INFOS_EX    NdisSignalStateInfosEx = NULL;
    NDIS_WWAN_SIGNAL_STATE_INFOS_EX     LocalNdisSignalStateInfosEx = { 0 };
    PMBB_SIGNAL_STATE_INFOS_V2          MbbSignalStateInfosV2 = (PMBB_SIGNAL_STATE_INFOS_V2)InBuffer;
    ULONG                               SignalStateCount = 0;
    ULONG                               SignalStateIndex = 0;
    PMBB_SIGNAL_STATE_V2                SignalState = NULL;

    do
    {
        MBB_INITIALIZE_REQUEST_NDIS_STATUS_INDICATION(
            &StatusIndication,
            Request,
            NDIS_STATUS_WWAN_SIGNAL_STATE
            );

        WwanStatus = MbbUtilMbbToWwanStatus(MbbStatus);

        if (NdisStatus != NDIS_STATUS_SUCCESS)
        {
            TraceError(WMBCLASS_OID, "[MbbNdis][ReqId=0x%04x] FAILED to get data for NDIS_STATUS_WWAN_SIGNAL_STATE, status=%!STATUS!",
                Request->RequestId,
                NdisStatus
                );
            WwanStatus = WWAN_STATUS_FAILURE;
            break;
        }

        if (MbbSignalStateInfosV2 == NULL ||
            InBufferSize < (ULONG)FIELD_OFFSET(MBB_SIGNAL_STATE_INFOS_V2, SignalStateList))
        {
            TraceError(WMBCLASS_OID, "[MbbNdis][ReqId=0x%04x] INSUFFICIENT BufferSize for NDIS_STATUS_WWAN_SIGNAL_STATE, BufferSize[Received=%d Expected=%d]",
                Request->RequestId,
                InBufferSize,
                FIELD_OFFSET(MBB_SIGNAL_STATE_INFOS_V2, SignalStateList)
                );
            WwanStatus = WWAN_STATUS_FAILURE;
            break;
        }

        SignalStateCount = MbbSignalStateInfosV2->ElementCount;
        for (SignalStateIndex = 0; SignalStateIndex < SignalStateCount; SignalStateIndex++)
        {
            // Check whether the struct MBB_SIGNAL_STATE_V2 is valid.
            NdisStatus = MbbIsVariableFieldValid(
                InBufferSize,
                MbbSignalStateInfosV2->SignalStateList[SignalStateIndex].Offset,
                MbbSignalStateInfosV2->SignalStateList[SignalStateIndex].Size,
                1,
                sizeof(MBB_SIGNAL_STATE_V2)
                );

            if (NdisStatus != NDIS_STATUS_SUCCESS)
            {
                TraceError(WMBCLASS_OID, "[MbbNdis][ReqId=0x%04x] Bad MBB_SIGNAL_STATE_V2 for SignalStateIndex %d, Offset=%d, Size=%d, ReceivedSize=%d",
                    Request->RequestId,
                    SignalStateIndex,
                    MbbSignalStateInfosV2->SignalStateList[SignalStateIndex].Offset,
                    MbbSignalStateInfosV2->SignalStateList[SignalStateIndex].Size,
                    InBufferSize
                    );
                WwanStatus = WWAN_STATUS_FAILURE;
                break;
            }
        }

        NdisSignalStateInfosExSize = sizeof(NDIS_WWAN_SIGNAL_STATE_INFOS_EX);
        NdisSignalStateInfosExSize += sizeof(WWAN_SIGNAL_STATE_EX) * SignalStateCount;

        NdisSignalStateInfosEx = (PNDIS_WWAN_SIGNAL_STATE_INFOS_EX)ALLOCATE_NONPAGED_POOL(NdisSignalStateInfosExSize);

        if (NdisSignalStateInfosEx == NULL)
        {
            TraceError(WMBCLASS_OID, "[MbbNdis][ReqId=0x%04x] FAILED to allocate %d bytes for NDIS_STATUS_WWAN_SIGNAL_STATE",
                Request->RequestId,
                NdisSignalStateInfosExSize
                );
            WwanStatus = WWAN_STATUS_FAILURE;
            break;
        }

        RtlZeroMemory(NdisSignalStateInfosEx, NdisSignalStateInfosExSize);

        NdisSignalStateInfosEx->Header.Type = NDIS_OBJECT_TYPE_DEFAULT;
        NdisSignalStateInfosEx->Header.Size = sizeof(NDIS_WWAN_SIGNAL_STATE_INFOS_EX);
        NdisSignalStateInfosEx->Header.Revision = NDIS_WWAN_SIGNAL_STATE_REVISION_2;

        MbbUtilMbbToWwanSignalStateInfoV2(
            MbbSignalStateInfosV2,
            &(NdisSignalStateInfosEx->SignalStateInfos)
            );
        IndicationFailure = FALSE;
    } while (FALSE);

    //
    // Restore the device returned operational status, if any.
    //
    if (WwanStatus != WWAN_STATUS_SUCCESS &&
        MbbUtilMbbToWwanStatus(MbbStatus) != WWAN_STATUS_SUCCESS)
    {
        WwanStatus = MbbUtilMbbToWwanStatus(MbbStatus);
    }

    if (!IndicationFailure)
    {
        NdisSignalStateInfosEx->uStatus = WwanStatus;

        StatusIndication.StatusBuffer = NdisSignalStateInfosEx;
        StatusIndication.StatusBufferSize = NdisSignalStateInfosExSize;
    }
    else
    {
        RtlZeroMemory(&LocalNdisSignalStateInfosEx, sizeof(NDIS_WWAN_SIGNAL_STATE_INFOS_EX));

        LocalNdisSignalStateInfosEx.Header.Type = NDIS_OBJECT_TYPE_DEFAULT;
        LocalNdisSignalStateInfosEx.Header.Size = sizeof(NDIS_WWAN_SIGNAL_STATE_INFOS_EX);
        LocalNdisSignalStateInfosEx.Header.Revision = NDIS_WWAN_SIGNAL_STATE_REVISION_2;
        LocalNdisSignalStateInfosEx.uStatus = WwanStatus;

        StatusIndication.StatusBuffer = &LocalNdisSignalStateInfosEx;
        StatusIndication.StatusBufferSize = sizeof(NDIS_WWAN_SIGNAL_STATE_INFOS_EX);
    }

    TraceInfo(WMBCLASS_OID, "[MbbNdis][ReqId=0x%04x] Indicating NDIS_STATUS_WWAN_SIGNAL_STATE with status=%!WwanStatus!",
        Request->RequestId,
        WwanStatus
        );

    MbbNdisMiniportIndicateRequestStatus(
        Request,
        &StatusIndication
        );

    if (NdisSignalStateInfosEx != NULL)
    {
        FREE_POOL(NdisSignalStateInfosEx);
    }

    return NDIS_STATUS_SUCCESS;
}

NDIS_STATUS
MbbNdisLocationStateStatusHandler(
    __in PMBB_REQUEST_CONTEXT               Request,
    __in NDIS_STATUS                        NdisStatus,
    __in MBB_STATUS                         MbbStatus,
    __in_bcount_opt(InBufferSize) PUCHAR    InBuffer,
    __in ULONG                              InBufferSize
)
{
    WWAN_STATUS                     WwanStatus = WWAN_STATUS_SUCCESS;
    NDIS_STATUS_INDICATION          StatusIndication = { 0 };
    NDIS_WWAN_LOCATION_STATE_INFO   NdisLocationStateInfo = { 0 };
    PMBB_LOCATION_STATE_INFO        MbbLocationStateInfo = (PMBB_LOCATION_STATE_INFO)InBuffer;

    do
    {
        MBB_INITIALIZE_REQUEST_NDIS_STATUS_INDICATION(
            &StatusIndication,
            Request,
            NDIS_STATUS_WWAN_LOCATION_STATE_INFO
            );

        RtlZeroMemory(&NdisLocationStateInfo, sizeof(NdisLocationStateInfo));

        NdisLocationStateInfo.Header.Type = NDIS_OBJECT_TYPE_DEFAULT;
        NdisLocationStateInfo.Header.Size = sizeof(NDIS_WWAN_LOCATION_STATE_INFO);
        NdisLocationStateInfo.Header.Revision = NDIS_WWAN_LOCATION_STATE_INFO_REVISION_1;

        WwanStatus = MbbUtilMbbToWwanStatus(MbbStatus);

        if (NdisStatus != NDIS_STATUS_SUCCESS)
        {
            TraceError(WMBCLASS_OID, "[MbbNdis][ReqId=0x%04x] FAILED to get data for NDIS_STATUS_WWAN_LOCATION_STATE_INFO, status=%!STATUS!",
                Request->RequestId,
                NdisStatus
                );
            WwanStatus = WWAN_STATUS_FAILURE;
            break;
        }

        if (MbbLocationStateInfo == NULL ||
            InBufferSize < sizeof(MBB_LOCATION_STATE_INFO))
        {
            TraceError(WMBCLASS_OID, "[MbbNdis][ReqId=0x%04x] INSUFFICIENT BufferSize for NDIS_STATUS_WWAN_LOCATION_STATE_INFO, BufferSize[Received=%d Expected=%d]",
                Request->RequestId,
                InBufferSize,
                sizeof(MBB_LOCATION_STATE_INFO)
                );
            WwanStatus = WWAN_STATUS_FAILURE;
            break;
        }

        NdisLocationStateInfo.LocationStateInfo.Trigger = (WWAN_LOCATION_TRIGGER_MODE)(MbbLocationStateInfo->Trigger);
        NdisLocationStateInfo.LocationStateInfo.DataClass = MbbLocationStateInfo->DataClass;
        NdisLocationStateInfo.LocationStateInfo.AreaCode = MbbLocationStateInfo->AreaCode;
        NdisLocationStateInfo.LocationStateInfo.CellId = MbbLocationStateInfo->CellId;
    } while (FALSE);

    //
    // Restore the device returned operational status, if any.
    //
    if (WwanStatus != WWAN_STATUS_SUCCESS &&
        MbbUtilMbbToWwanStatus(MbbStatus) != WWAN_STATUS_SUCCESS)
    {
        WwanStatus = MbbUtilMbbToWwanStatus(MbbStatus);
    }

    NdisLocationStateInfo.uStatus = WwanStatus;

    StatusIndication.StatusBuffer = &NdisLocationStateInfo;
    StatusIndication.StatusBufferSize = sizeof(NDIS_WWAN_LOCATION_STATE_INFO);

    TraceInfo(WMBCLASS_OID, "[MbbNdis][ReqId=0x%04x] Indicating NDIS_STATUS_WWAN_LOCATION_STATE_INFO with status=%!WwanStatus!",
        Request->RequestId,
        WwanStatus
        );

    MbbNdisMiniportIndicateRequestStatus(
        Request,
        &StatusIndication
        );

    return NDIS_STATUS_SUCCESS;
}

NDIS_STATUS
MbbNdisNitzStatusHandler(
    __in PMBB_REQUEST_CONTEXT               Request,
    __in NDIS_STATUS                        NdisStatus,
    __in MBB_STATUS                         MbbStatus,
    __in_bcount_opt(InBufferSize) PUCHAR    InBuffer,
    __in ULONG                              InBufferSize
)
{
    WWAN_STATUS             WwanStatus = WWAN_STATUS_SUCCESS;
    NDIS_STATUS_INDICATION  StatusIndication = { 0 };
    NDIS_WWAN_NITZ_INFO     NdisNitzInfo = { 0 };
    PMBB_NITZ_INFO          MbbNitzInfo = (PMBB_NITZ_INFO)InBuffer;

    do
    {
        MBB_INITIALIZE_REQUEST_NDIS_STATUS_INDICATION(
            &StatusIndication,
            Request,
            NDIS_STATUS_WWAN_NITZ_INFO
            );

        RtlZeroMemory(&NdisNitzInfo, sizeof(NdisNitzInfo));

        NdisNitzInfo.Header.Type = NDIS_OBJECT_TYPE_DEFAULT;
        NdisNitzInfo.Header.Size = sizeof(NDIS_WWAN_NITZ_INFO);
        NdisNitzInfo.Header.Revision = NDIS_WWAN_NITZ_INFO_REVISION_1;

        WwanStatus = MbbUtilMbbToWwanStatus(MbbStatus);

        if (NdisStatus != NDIS_STATUS_SUCCESS)
        {
            TraceError(WMBCLASS_OID, "[MbbNdis][ReqId=0x%04x] FAILED to get data for NDIS_STATUS_WWAN_NITZ_INFO, status=%!STATUS!",
                Request->RequestId,
                NdisStatus
                );
            WwanStatus = WWAN_STATUS_FAILURE;
            break;
        }

        if (MbbNitzInfo == NULL ||
            InBufferSize < sizeof(MBB_NITZ_INFO))
        {
            TraceError(WMBCLASS_OID, "[MbbNdis][ReqId=0x%04x] INSUFFICIENT BufferSize for NDIS_STATUS_WWAN_NITZ_INFO, BufferSize[Received=%d Expected=%d]",
                Request->RequestId,
                InBufferSize,
                sizeof(MBB_NITZ_INFO)
                );
            WwanStatus = WWAN_STATUS_FAILURE;
            break;
        }

        NdisNitzInfo.NitzInfo.Year = MbbNitzInfo->Year;
        NdisNitzInfo.NitzInfo.Month = MbbNitzInfo->Month;
        NdisNitzInfo.NitzInfo.Day = MbbNitzInfo->Day;
        NdisNitzInfo.NitzInfo.Hour = MbbNitzInfo->Hour;
        NdisNitzInfo.NitzInfo.Minute = MbbNitzInfo->Minute;
        NdisNitzInfo.NitzInfo.Second = MbbNitzInfo->Second;
        NdisNitzInfo.NitzInfo.TimeZoneOffsetMinutes = MbbNitzInfo->TimeZoneOffsetMinutes;
        NdisNitzInfo.NitzInfo.DaylightSavingTimeOffsetMinutes = MbbNitzInfo->DaylightSavingTimeOffsetMinutes;
        NdisNitzInfo.NitzInfo.DataClasses = MbbNitzInfo->DataClasses;
    } while (FALSE);

    //
    // Restore the device returned operational status, if any.
    //
    if (WwanStatus != WWAN_STATUS_SUCCESS &&
        MbbUtilMbbToWwanStatus(MbbStatus) != WWAN_STATUS_SUCCESS)
    {
        WwanStatus = MbbUtilMbbToWwanStatus(MbbStatus);
    }

    NdisNitzInfo.uStatus = WwanStatus;

    StatusIndication.StatusBuffer = &NdisNitzInfo;
    StatusIndication.StatusBufferSize = sizeof(NDIS_WWAN_NITZ_INFO);

    TraceInfo(WMBCLASS_OID, "[MbbNdis][ReqId=0x%04x] Indicating NDIS_STATUS_WWAN_NITZ_INFO with status=%!WwanStatus!",
        Request->RequestId,
        WwanStatus
        );

    MbbNdisMiniportIndicateRequestStatus(
        Request,
        &StatusIndication
        );

    return NDIS_STATUS_SUCCESS;
}

NDIS_STATUS
MbbNdisPreshutdownHandler(
    __in PMBB_REQUEST_CONTEXT               Request,
    __in NDIS_STATUS                        NdisStatus,
    __in MBB_STATUS                         MbbStatus,
    __in_bcount_opt(InBufferSize) PUCHAR    InBuffer,
    __in ULONG                              InBufferSize
    )
{
    WWAN_STATUS             WwanStatus = WWAN_STATUS_SUCCESS;
    NDIS_STATUS_INDICATION  StatusIndication = { 0 };
    NDIS_WWAN_PRESHUTDOWN_STATE NdisPreshutdownState = { 0 };

    UNREFERENCED_PARAMETER(InBuffer);
    UNREFERENCED_PARAMETER(InBufferSize);

    do
    {
        MBB_INITIALIZE_REQUEST_NDIS_STATUS_INDICATION(
            &StatusIndication,
            Request,
            NDIS_STATUS_WWAN_PRESHUTDOWN_STATE
            );

        RtlZeroMemory(&NdisPreshutdownState, sizeof(NdisPreshutdownState));

        NdisPreshutdownState.Header.Type = NDIS_OBJECT_TYPE_DEFAULT;
        NdisPreshutdownState.Header.Size = SIZEOF_NDIS_WWAN_PRESHUTDOWN_STATE_1;
        NdisPreshutdownState.Header.Revision = NDIS_WWAN_PRESHUTDOWN_STATE_REVISION_1;

        WwanStatus = MbbUtilMbbToWwanStatus(MbbStatus);

        if (NdisStatus != NDIS_STATUS_SUCCESS)
        {
            TraceError(WMBCLASS_OID, "[MbbNdis][ReqId=0x%04x] FAILED to get data for NDIS_STATUS_WWAN_PRESHUTDOWN_STATE, status=%!STATUS!",
                Request->RequestId,
                NdisStatus
                );
            WwanStatus = WWAN_STATUS_FAILURE;
            break;
        }
    } while (FALSE);

    //
    // Restore the device returned operational status, if any.
    //
    if (WwanStatus != WWAN_STATUS_SUCCESS &&
        MbbUtilMbbToWwanStatus(MbbStatus) != WWAN_STATUS_SUCCESS)
    {
        WwanStatus = MbbUtilMbbToWwanStatus(MbbStatus);
    }

    NdisPreshutdownState.uStatus = WwanStatus;

    StatusIndication.StatusBuffer = &NdisPreshutdownState;
    StatusIndication.StatusBufferSize = sizeof(NdisPreshutdownState);

    TraceInfo(WMBCLASS_OID, "[MbbNdis][ReqId=0x%04x] Indicating NDIS_STATUS_WWAN_PRESHUTDOWN_STATE with status=%!WwanStatus!",
        Request->RequestId,
        WwanStatus
        );

    TraceLoggingWrite(
        g_hLoggingProvider,
        "OID_Wwan_Preshutdown_Response",
        TraceLoggingUInt64(Request->RequestId, "RequestID"),
        TraceLoggingUInt64(WwanStatus, "WwanStatus"),
        TraceLoggingKeyword(MICROSOFT_KEYWORD_TELEMETRY));

    MbbNdisMiniportIndicateRequestStatus(
        Request,
        &StatusIndication
        );

    return NDIS_STATUS_SUCCESS;
}

NDIS_STATUS
MbbNdisAtrInfoStatusHandler(
    __in PMBB_REQUEST_CONTEXT               Request,
    __in NDIS_STATUS                        NdisStatus,
    __in MBB_STATUS                         MbbStatus,
    __in_bcount_opt(InBufferSize) PUCHAR    InBuffer,
    __in ULONG                              InBufferSize
    )
{
    WWAN_STATUS             WwanStatus;
    NDIS_STATUS_INDICATION  StatusIndication;
    NDIS_WWAN_ATR_INFO      NdisAtrInfo;
    PMBB_ATR_INFO           MbbAtrInfo = (PMBB_ATR_INFO)InBuffer;

    do
    {
        MBB_INITIALIZE_REQUEST_NDIS_STATUS_INDICATION(
            &StatusIndication,
            Request,
            NDIS_STATUS_WWAN_ATR_INFO
            );

        WwanStatus = MbbUtilMbbToWwanStatus(MbbStatus);

        if( NdisStatus != NDIS_STATUS_SUCCESS )
        {
            TraceError( WMBCLASS_OID, "[MbbNdis][ReqId=0x%04x] FAILED to get data for NDIS_STATUS_WWAN_ATR_INFO, status=%!STATUS!",
                        Request->RequestId,
                        NdisStatus
                        );
            WwanStatus = WWAN_STATUS_FAILURE;
            break;
        }

        if( MbbAtrInfo == NULL ||
            InBufferSize < (ULONG)FIELD_OFFSET(MBB_ATR_INFO, DataBuffer))
        {
            TraceError( WMBCLASS_OID, "[MbbNdis][ReqId=0x%04x] INSUFFICIENT BufferSize for NDIS_STATUS_WWAN_ATR_INFO, BufferSize[Received=%d Expected=%d]",
                        Request->RequestId,
                        InBufferSize,
                        FIELD_OFFSET(MBB_ATR_INFO, DataBuffer)
                        );
            WwanStatus = WWAN_STATUS_FAILURE;
            break;
        }

        RtlZeroMemory(&NdisAtrInfo, sizeof(NdisAtrInfo));

        NdisAtrInfo.Header.Type     = NDIS_OBJECT_TYPE_DEFAULT;
        NdisAtrInfo.Header.Size     = sizeof(NDIS_WWAN_ATR_INFO);
        NdisAtrInfo.Header.Revision = NDIS_WWAN_ATR_INFO_REVISION_1;

        MbbUtilMbbToWwanAtrInfo(
            MbbAtrInfo,
            &NdisAtrInfo.AtrInfo
            );
    }
    while( FALSE );

    //
    // Restore the device returned operational status, if any.
    //
    if( WwanStatus != WWAN_STATUS_SUCCESS &&
        MbbUtilMbbToWwanStatus( MbbStatus ) != WWAN_STATUS_SUCCESS )
    {
        WwanStatus = MbbUtilMbbToWwanStatus( MbbStatus );
    }

    NdisAtrInfo.uStatus = WwanStatus;

    StatusIndication.StatusBuffer       = &NdisAtrInfo;
    StatusIndication.StatusBufferSize   = sizeof(NDIS_WWAN_ATR_INFO);

    TraceInfo(  WMBCLASS_OID, "[MbbNdis][ReqId=0x%04x] Indicating NDIS_STATUS_WWAN_ATR_INFO with status=%!WwanStatus!",
                Request->RequestId,
                WwanStatus
                );

    MbbNdisMiniportIndicateRequestStatus(
        Request,
        &StatusIndication
        );

    return NDIS_STATUS_SUCCESS;
}

NDIS_STATUS
MbbNdisUiccOpenChannelInfoStatusHandler(
    __in PMBB_REQUEST_CONTEXT               Request,
    __in NDIS_STATUS                        NdisStatus,
    __in MBB_STATUS                         MbbStatus,
    __in_bcount_opt(InBufferSize) PUCHAR    InBuffer,
    __in ULONG                              InBufferSize
    )
{
    BOOLEAN                             IndicationFailure = TRUE;
    WWAN_STATUS                         WwanStatus;
    NDIS_STATUS_INDICATION              StatusIndication;
    ULONG                               NdisUiccOpenChannelInfoSize = 0;
    PNDIS_WWAN_UICC_OPEN_CHANNEL_INFO   NdisUiccOpenChannelInfo = NULL;
    NDIS_WWAN_UICC_OPEN_CHANNEL_INFO    LocalNdisUiccOpenChannelInfo;
    PMBB_UICC_OPEN_CHANNEL_INFO         MbbUiccOpenChannelInfo = (PMBB_UICC_OPEN_CHANNEL_INFO)InBuffer;

    do
    {
        MBB_INITIALIZE_REQUEST_NDIS_STATUS_INDICATION(
            &StatusIndication,
            Request,
            NDIS_STATUS_WWAN_UICC_OPEN_CHANNEL_INFO
            );

        WwanStatus = MbbUtilMbbToWwanStatus( MbbStatus );

        if (NdisStatus != NDIS_STATUS_SUCCESS)
        {
            TraceError( WMBCLASS_OID, "[MbbNdis][ReqId=0x%04x] FAILED to get data for NDIS_STATUS_WWAN_UICC_OPEN_CHANNEL_INFO, status=%!STATUS!",
                Request->RequestId,
                NdisStatus
                );
            WwanStatus = WWAN_STATUS_FAILURE;
            break;
        }

        if( MbbUiccOpenChannelInfo == NULL ||
            InBufferSize < (ULONG)FIELD_OFFSET(MBB_UICC_OPEN_CHANNEL_INFO, DataBuffer))
        {
            TraceError( WMBCLASS_OID, "[MbbNdis][ReqId=0x%04x] INSUFFICIENT BufferSize for NDIS_STATUS_WWAN_UICC_OPEN_CHANNEL_INFO, BufferSize[Received=%d Expected=%d]",
                        Request->RequestId,
                        InBufferSize,
                        FIELD_OFFSET(MBB_UICC_OPEN_CHANNEL_INFO, DataBuffer)
                        );
            WwanStatus = WWAN_STATUS_FAILURE;
            break;
        }

        NdisUiccOpenChannelInfoSize = FIELD_OFFSET(NDIS_WWAN_UICC_OPEN_CHANNEL_INFO, UiccOpenChannelInfo.Response);
        NdisUiccOpenChannelInfoSize += MbbUiccOpenChannelInfo->Response.Size;

        NdisUiccOpenChannelInfo = (PNDIS_WWAN_UICC_OPEN_CHANNEL_INFO)ALLOCATE_NONPAGED_POOL( NdisUiccOpenChannelInfoSize );

        if( NdisUiccOpenChannelInfo == NULL )
        {
            TraceError( WMBCLASS_OID, "[MbbNdis][ReqId=0x%04x] FAILED to allocate %d bytes for NDIS_STATUS_WWAN_UICC_OPEN_CHANNEL_INFO",
                        Request->RequestId,
                        NdisUiccOpenChannelInfoSize
                        );
            WwanStatus = WWAN_STATUS_FAILURE;
            break;
        }

        RtlZeroMemory(NdisUiccOpenChannelInfo, NdisUiccOpenChannelInfoSize);

        // Open channel can have a response payload even when the command fails
        NdisUiccOpenChannelInfo->Header.Type        = NDIS_OBJECT_TYPE_DEFAULT;
        NdisUiccOpenChannelInfo->Header.Size        = (USHORT)NdisUiccOpenChannelInfoSize;
        NdisUiccOpenChannelInfo->Header.Revision    = NDIS_WWAN_UICC_OPEN_CHANNEL_INFO_REVISION_1;
        NdisUiccOpenChannelInfo->uStatus            = WwanStatus;

        StatusIndication.StatusBuffer               = NdisUiccOpenChannelInfo;
        StatusIndication.StatusBufferSize           = NdisUiccOpenChannelInfoSize;

        MbbUtilMbbToWwanUiccOpenChannelInfo(
            MbbUiccOpenChannelInfo,
            &NdisUiccOpenChannelInfo->UiccOpenChannelInfo
            );
        IndicationFailure = FALSE;
    }
    while( FALSE );

    //
    // Restore the device returned operational status, if any.
    //
    if( WwanStatus != WWAN_STATUS_SUCCESS &&
        MbbUtilMbbToWwanStatus( MbbStatus ) != WWAN_STATUS_SUCCESS )
    {
        WwanStatus = MbbUtilMbbToWwanStatus( MbbStatus );
    }

    if (IndicationFailure)
    {
        RtlZeroMemory(&LocalNdisUiccOpenChannelInfo, sizeof(NDIS_WWAN_UICC_OPEN_CHANNEL_INFO));

        LocalNdisUiccOpenChannelInfo.Header.Type        = NDIS_OBJECT_TYPE_DEFAULT;
        LocalNdisUiccOpenChannelInfo.Header.Size        = sizeof(NDIS_WWAN_UICC_OPEN_CHANNEL_INFO);
        LocalNdisUiccOpenChannelInfo.Header.Revision    = NDIS_WWAN_UICC_OPEN_CHANNEL_INFO_REVISION_1;
        LocalNdisUiccOpenChannelInfo.uStatus            = WwanStatus;

        StatusIndication.StatusBuffer                   = &LocalNdisUiccOpenChannelInfo;
        StatusIndication.StatusBufferSize               = sizeof(NDIS_WWAN_UICC_OPEN_CHANNEL_INFO);
    }

    TraceInfo(  WMBCLASS_OID, "[MbbNdis][ReqId=0x%04x] Indicating NDIS_STATUS_WWAN_UICC_OPEN_CHANNEL_INFO with status=%!WwanStatus!",
                Request->RequestId,
                WwanStatus
                );

    MbbNdisMiniportIndicateRequestStatus(
        Request,
        &StatusIndication
        );

    if (NdisUiccOpenChannelInfo != NULL)
    {
        FREE_POOL(NdisUiccOpenChannelInfo);
    }

    return NDIS_STATUS_SUCCESS;
}

NDIS_STATUS
MbbNdisUiccCloseChannelInfoStatusHandler(
    __in PMBB_REQUEST_CONTEXT               Request,
    __in NDIS_STATUS                        NdisStatus,
    __in MBB_STATUS                         MbbStatus,
    __in_bcount_opt(InBufferSize) PUCHAR    InBuffer,
    __in ULONG                              InBufferSize
    )
{
    WWAN_STATUS                         WwanStatus;
    NDIS_STATUS_INDICATION              StatusIndication;
    NDIS_WWAN_UICC_CLOSE_CHANNEL_INFO   NdisUiccCloseChannelInfo;
    PMBB_UICC_CLOSE_CHANNEL_INFO        MbbUiccCloseChannelInfo = (PMBB_UICC_CLOSE_CHANNEL_INFO)InBuffer;

    do
    {
        MBB_INITIALIZE_REQUEST_NDIS_STATUS_INDICATION(
            &StatusIndication,
            Request,
            NDIS_STATUS_WWAN_UICC_CLOSE_CHANNEL_INFO
            );

        WwanStatus = MbbUtilMbbToWwanStatus(MbbStatus);

        if( NdisStatus != NDIS_STATUS_SUCCESS )
        {
            TraceError( WMBCLASS_OID, "[MbbNdis][ReqId=0x%04x] FAILED to get data for NDIS_STATUS_WWAN_UICC_CLOSE_CHANNEL_INFO, status=%!STATUS!",
                        Request->RequestId,
                        NdisStatus
                        );
            WwanStatus = WWAN_STATUS_FAILURE;
            break;
        }

        if( MbbUiccCloseChannelInfo == NULL ||
            InBufferSize < sizeof(MBB_UICC_CLOSE_CHANNEL_INFO) )
        {
            TraceError( WMBCLASS_OID, "[MbbNdis][ReqId=0x%04x] INSUFFICIENT BufferSize for NDIS_STATUS_WWAN_UICC_CLOSE_CHANNEL_INFO, BufferSize[Received=%d Expected=%d]",
                        Request->RequestId,
                        InBufferSize,
                        sizeof(MBB_UICC_CLOSE_CHANNEL_INFO)
                        );
            WwanStatus = WWAN_STATUS_FAILURE;
            break;
        }

        RtlZeroMemory(&NdisUiccCloseChannelInfo, sizeof(NdisUiccCloseChannelInfo));

        NdisUiccCloseChannelInfo.Header.Type        = NDIS_OBJECT_TYPE_DEFAULT;
        NdisUiccCloseChannelInfo.Header.Size        = sizeof(NDIS_WWAN_UICC_CLOSE_CHANNEL_INFO);
        NdisUiccCloseChannelInfo.Header.Revision    = NDIS_WWAN_UICC_CLOSE_CHANNEL_INFO_REVISION_1;

        MbbUtilMbbToWwanUiccCloseChannelInfo(
            MbbUiccCloseChannelInfo,
            &NdisUiccCloseChannelInfo.UiccCloseChannelInfo
            );
    }
    while( FALSE );

    //
    // Restore the device returned operational status, if any.
    //
    if( WwanStatus != WWAN_STATUS_SUCCESS &&
        MbbUtilMbbToWwanStatus( MbbStatus ) != WWAN_STATUS_SUCCESS )
    {
        WwanStatus = MbbUtilMbbToWwanStatus( MbbStatus );
    }

    NdisUiccCloseChannelInfo.uStatus = WwanStatus;

    StatusIndication.StatusBuffer       = &NdisUiccCloseChannelInfo;
    StatusIndication.StatusBufferSize   = sizeof(NDIS_WWAN_UICC_CLOSE_CHANNEL_INFO);

    TraceInfo(  WMBCLASS_OID, "[MbbNdis][ReqId=0x%04x] Indicating NDIS_STATUS_WWAN_UICC_CLOSE_CHANNEL_INFO with status=%!WwanStatus!",
                Request->RequestId,
                WwanStatus
                );

    MbbNdisMiniportIndicateRequestStatus(
        Request,
        &StatusIndication
        );

    return NDIS_STATUS_SUCCESS;
}

NDIS_STATUS
MbbNdisUiccApduInfoStatusHandler(
    __in PMBB_REQUEST_CONTEXT               Request,
    __in NDIS_STATUS                        NdisStatus,
    __in MBB_STATUS                         MbbStatus,
    __in_bcount_opt(InBufferSize) PUCHAR    InBuffer,
    __in ULONG                              InBufferSize
    )
{
    BOOLEAN                     IndicationFailure = TRUE;
    WWAN_STATUS                 WwanStatus;
    NDIS_STATUS_INDICATION      StatusIndication;
    ULONG                       NdisUiccApduInfoSize = 0;
    PNDIS_WWAN_UICC_APDU_INFO   NdisUiccApduInfo = NULL;
    NDIS_WWAN_UICC_APDU_INFO    LocalNdisUiccApduInfo;
    PMBB_UICC_APDU_INFO         MbbUiccApduInfo = (PMBB_UICC_APDU_INFO)InBuffer;

    do
    {
        MBB_INITIALIZE_REQUEST_NDIS_STATUS_INDICATION(
            &StatusIndication,
            Request,
            NDIS_STATUS_WWAN_UICC_APDU_INFO
            );

        WwanStatus = MbbUtilMbbToWwanStatus( MbbStatus );

        if (NdisStatus != NDIS_STATUS_SUCCESS)
        {
            TraceError( WMBCLASS_OID, "[MbbNdis][ReqId=0x%04x] FAILED to get data for NDIS_STATUS_WWAN_UICC_APDU_INFO, status=%!STATUS!",
                Request->RequestId,
                NdisStatus
                );
            WwanStatus = WWAN_STATUS_FAILURE;
            break;
        }

        if( MbbUiccApduInfo == NULL ||
            InBufferSize < (ULONG)FIELD_OFFSET(MBB_UICC_APDU_INFO, DataBuffer))
        {
            TraceError( WMBCLASS_OID, "[MbbNdis][ReqId=0x%04x] INSUFFICIENT BufferSize for NDIS_STATUS_WWAN_UICC_APDU_INFO, BufferSize[Received=%d Expected=%d]",
                        Request->RequestId,
                        InBufferSize,
                        FIELD_OFFSET(MBB_UICC_APDU_INFO, DataBuffer)
                        );
            WwanStatus = WWAN_STATUS_FAILURE;
            break;
        }

        NdisUiccApduInfoSize = FIELD_OFFSET(NDIS_WWAN_UICC_APDU_INFO, UiccApduInfo.Response);
        NdisUiccApduInfoSize += MbbUiccApduInfo->Response.Size;

        NdisUiccApduInfo = (PNDIS_WWAN_UICC_APDU_INFO)ALLOCATE_NONPAGED_POOL( NdisUiccApduInfoSize );

        if( NdisUiccApduInfo == NULL )
        {
            TraceError( WMBCLASS_OID, "[MbbNdis][ReqId=0x%04x] FAILED to allocate %d bytes for NDIS_STATUS_WWAN_UICC_APDU_INFO",
                        Request->RequestId,
                        NdisUiccApduInfoSize
                        );
            WwanStatus = WWAN_STATUS_FAILURE;
            break;
        }

        RtlZeroMemory(NdisUiccApduInfo, NdisUiccApduInfoSize);

        NdisUiccApduInfo->Header.Type       = NDIS_OBJECT_TYPE_DEFAULT;
        NdisUiccApduInfo->Header.Size       = (USHORT)NdisUiccApduInfoSize;
        NdisUiccApduInfo->Header.Revision   = NDIS_WWAN_UICC_OPEN_CHANNEL_INFO_REVISION_1;
        NdisUiccApduInfo->uStatus           = WwanStatus;

        StatusIndication.StatusBuffer       = NdisUiccApduInfo;
        StatusIndication.StatusBufferSize   = NdisUiccApduInfoSize;

        MbbUtilMbbToWwanUiccApduInfo(
            MbbUiccApduInfo,
            &NdisUiccApduInfo->UiccApduInfo
            );
        IndicationFailure = FALSE;
    }
    while( FALSE );

    //
    // Restore the device returned operational status, if any.
    //
    if( WwanStatus != WWAN_STATUS_SUCCESS &&
        MbbUtilMbbToWwanStatus( MbbStatus ) != WWAN_STATUS_SUCCESS )
    {
        WwanStatus = MbbUtilMbbToWwanStatus( MbbStatus );
    }

    if (IndicationFailure)
    {
        RtlZeroMemory(&LocalNdisUiccApduInfo, sizeof(NDIS_WWAN_UICC_APDU_INFO));

        LocalNdisUiccApduInfo.Header.Type       = NDIS_OBJECT_TYPE_DEFAULT;
        LocalNdisUiccApduInfo.Header.Size       = sizeof(NDIS_WWAN_UICC_APDU_INFO);
        LocalNdisUiccApduInfo.Header.Revision   = NDIS_WWAN_UICC_APDU_INFO_REVISION_1;
        LocalNdisUiccApduInfo.uStatus           = WwanStatus;

        StatusIndication.StatusBuffer           = &LocalNdisUiccApduInfo;
        StatusIndication.StatusBufferSize       = sizeof(NDIS_WWAN_UICC_APDU_INFO);
    }

    TraceInfo(  WMBCLASS_OID, "[MbbNdis][ReqId=0x%04x] Indicating NDIS_STATUS_WWAN_UICC_APDU_INFO with status=%!WwanStatus!",
                Request->RequestId,
                WwanStatus
                );

    MbbNdisMiniportIndicateRequestStatus(
        Request,
        &StatusIndication
        );

    if (NdisUiccApduInfo != NULL)
    {
        FREE_POOL(NdisUiccApduInfo);
    }

    return NDIS_STATUS_SUCCESS;
}

NDIS_STATUS
MbbNdisUiccTerminalCapabilityInfoStatusHandler(
    __in PMBB_REQUEST_CONTEXT               Request,
    __in NDIS_STATUS                        NdisStatus,
    __in MBB_STATUS                         MbbStatus,
    __in_bcount_opt(InBufferSize) PUCHAR    InBuffer,
    __in ULONG                              InBufferSize
    )
{
    BOOLEAN                                     IndicationFailure = TRUE;
    WWAN_STATUS                                 WwanStatus;
    NDIS_STATUS_INDICATION                      StatusIndication;
    ULONG                                       NdisUiccTerminalCapabilityInfoSize = 0;
    PNDIS_WWAN_UICC_TERMINAL_CAPABILITY_INFO    NdisUiccTerminalCapabilityInfo = NULL;
    NDIS_WWAN_UICC_TERMINAL_CAPABILITY_INFO     LocalNdisUiccTerminalCapabilityInfo;
    PMBB_UICC_TERMINAL_CAPABILITY_INFO          MbbUiccTerminalCapabilityInfo = (PMBB_UICC_TERMINAL_CAPABILITY_INFO)InBuffer;
    ULONG                                       CapabilityCount = 0;

    do
    {
        MBB_INITIALIZE_REQUEST_NDIS_STATUS_INDICATION(
            &StatusIndication,
            Request,
            NDIS_STATUS_WWAN_UICC_TERMINAL_CAPABILITY_INFO
            );

        WwanStatus = MbbUtilMbbToWwanStatus( MbbStatus );

        if (NdisStatus != NDIS_STATUS_SUCCESS)
        {
            TraceError( WMBCLASS_OID, "[MbbNdis][ReqId=0x%04x] FAILED to get data for NDIS_STATUS_WWAN_UICC_TERMINAL_CAPABILITY_INFO, status=%!STATUS!",
                Request->RequestId,
                NdisStatus
                );
            WwanStatus = WWAN_STATUS_FAILURE;
            break;
        }

        if( MbbUiccTerminalCapabilityInfo == NULL ||
            InBufferSize < (ULONG)FIELD_OFFSET(MBB_UICC_TERMINAL_CAPABILITY_INFO, CapabilityList))
        {
            TraceError( WMBCLASS_OID, "[MbbNdis][ReqId=0x%04x] INSUFFICIENT BufferSize for NDIS_STATUS_WWAN_UICC_TERMINAL_CAPABILITY_INFO, BufferSize[Received=%d Expected=%d]",
                        Request->RequestId,
                        InBufferSize,
                        FIELD_OFFSET(MBB_UICC_TERMINAL_CAPABILITY_INFO, CapabilityList)
                        );
            WwanStatus = WWAN_STATUS_FAILURE;
            break;
        }

        CapabilityCount = MbbUiccTerminalCapabilityInfo->ElementCount;

        NdisUiccTerminalCapabilityInfoSize = sizeof(NDIS_WWAN_UICC_TERMINAL_CAPABILITY_INFO);
        NdisUiccTerminalCapabilityInfoSize += sizeof(WWAN_UICC_TERMINAL_CAPABILITY_TLV) * CapabilityCount;

        NdisUiccTerminalCapabilityInfo = (PNDIS_WWAN_UICC_TERMINAL_CAPABILITY_INFO)ALLOCATE_NONPAGED_POOL( NdisUiccTerminalCapabilityInfoSize );

        if( NdisUiccTerminalCapabilityInfo == NULL )
        {
            TraceError( WMBCLASS_OID, "[MbbNdis][ReqId=0x%04x] FAILED to allocate %d bytes for NDIS_STATUS_WWAN_UICC_TERMINAL_CAPABILITY_INFO",
                        Request->RequestId,
                        NdisUiccTerminalCapabilityInfoSize
                        );
            WwanStatus = WWAN_STATUS_FAILURE;
            break;
        }

        RtlZeroMemory(NdisUiccTerminalCapabilityInfo, NdisUiccTerminalCapabilityInfoSize);

        NdisUiccTerminalCapabilityInfo->Header.Type     = NDIS_OBJECT_TYPE_DEFAULT;
        NdisUiccTerminalCapabilityInfo->Header.Size     = (USHORT)NdisUiccTerminalCapabilityInfoSize;
        NdisUiccTerminalCapabilityInfo->Header.Revision = NDIS_WWAN_UICC_OPEN_CHANNEL_INFO_REVISION_1;
        NdisUiccTerminalCapabilityInfo->uStatus         = WwanStatus;

        StatusIndication.StatusBuffer                   = NdisUiccTerminalCapabilityInfo;
        StatusIndication.StatusBufferSize               = NdisUiccTerminalCapabilityInfoSize;

        MbbUtilMbbToWwanUiccTerminalCapabilityInfo(
            MbbUiccTerminalCapabilityInfo,
            &NdisUiccTerminalCapabilityInfo->UiccCapabilityInfo.CapabilityListHeader
            );
        IndicationFailure = FALSE;
    }
    while( FALSE );

    //
    // Restore the device returned operational status, if any.
    //
    if( WwanStatus != WWAN_STATUS_SUCCESS &&
        MbbUtilMbbToWwanStatus( MbbStatus ) != WWAN_STATUS_SUCCESS )
    {
        WwanStatus = MbbUtilMbbToWwanStatus( MbbStatus );
    }

    if (IndicationFailure)
    {
        RtlZeroMemory(&LocalNdisUiccTerminalCapabilityInfo, sizeof(NDIS_WWAN_UICC_TERMINAL_CAPABILITY_INFO));

        LocalNdisUiccTerminalCapabilityInfo.Header.Type     = NDIS_OBJECT_TYPE_DEFAULT;
        LocalNdisUiccTerminalCapabilityInfo.Header.Size     = sizeof(NDIS_WWAN_UICC_TERMINAL_CAPABILITY_INFO);
        LocalNdisUiccTerminalCapabilityInfo.Header.Revision = NDIS_WWAN_UICC_TERMINAL_CAPABILITY_INFO_REVISION_1;
        LocalNdisUiccTerminalCapabilityInfo.uStatus         = WwanStatus;

        StatusIndication.StatusBuffer                       = &LocalNdisUiccTerminalCapabilityInfo;
        StatusIndication.StatusBufferSize                   = sizeof(NDIS_WWAN_UICC_TERMINAL_CAPABILITY_INFO);
    }

    TraceInfo(  WMBCLASS_OID, "[MbbNdis][ReqId=0x%04x] Indicating NDIS_STATUS_WWAN_UICC_TERMINAL_CAPABILITY_INFO with status=%!WwanStatus!",
                Request->RequestId,
                WwanStatus
                );

    MbbNdisMiniportIndicateRequestStatus(
        Request,
        &StatusIndication
        );

    if (NdisUiccTerminalCapabilityInfo != NULL)
    {
        FREE_POOL(NdisUiccTerminalCapabilityInfo);
    }

    return NDIS_STATUS_SUCCESS;
}

NDIS_STATUS
MbbNdisUiccResetInfoStatusHandler(
    _In_ PMBB_REQUEST_CONTEXT                   Request,
    _In_ NDIS_STATUS                            NdisStatus,
    _In_ MBB_STATUS                             MbbStatus,
    _In_reads_bytes_opt_(InBufferSize) PUCHAR   InBuffer,
    _In_ ULONG                                  InBufferSize
    )
{
    WWAN_STATUS                         WwanStatus;
    NDIS_STATUS_INDICATION              StatusIndication;
    NDIS_WWAN_UICC_RESET_INFO           NdisUiccResetInfo;
    PMBB_UICC_RESET_INFO                MbbUiccResetInfo = (PMBB_UICC_RESET_INFO)InBuffer;

    do
    {
        MBB_INITIALIZE_REQUEST_NDIS_STATUS_INDICATION(
            &StatusIndication,
            Request,
            NDIS_STATUS_WWAN_UICC_RESET_INFO
            );

        WwanStatus = MbbUtilMbbToWwanStatus(MbbStatus);

        if( NdisStatus != NDIS_STATUS_SUCCESS )
        {
            TraceError( WMBCLASS_OID, "[MbbNdis][ReqId=0x%04x] FAILED to get data for NDIS_STATUS_WWAN_UICC_RESET_INFO, status=%!STATUS!",
                        Request->RequestId,
                        NdisStatus
                        );
            WwanStatus = WWAN_STATUS_FAILURE;
            break;
        }

        if( MbbUiccResetInfo == NULL ||
            InBufferSize < sizeof(MBB_UICC_RESET_INFO) )
        {
            TraceError( WMBCLASS_OID, "[MbbNdis][ReqId=0x%04x] INSUFFICIENT BufferSize for NDIS_STATUS_WWAN_UICC_RESET_INFO, BufferSize[Received=%d Expected=%d]",
                        Request->RequestId,
                        InBufferSize,
                        sizeof(MBB_UICC_RESET_INFO)
                        );
            WwanStatus = WWAN_STATUS_FAILURE;
            break;
        }

        RtlZeroMemory(&NdisUiccResetInfo, sizeof(NdisUiccResetInfo));

        NdisUiccResetInfo.Header.Type       = NDIS_OBJECT_TYPE_DEFAULT;
        NdisUiccResetInfo.Header.Size       = sizeof(NDIS_WWAN_UICC_RESET_INFO);
        NdisUiccResetInfo.Header.Revision   = NDIS_WWAN_UICC_RESET_INFO_REVISION_1;

        NdisUiccResetInfo.UiccResetInfo.PassThroughStatus = (WWAN_UICC_PASSTHROUGH_STATUS)MbbUiccResetInfo->PassThroughStatus;
    }
    while( FALSE );

    //
    // Restore the device returned operational status, if any.
    //
    if( WwanStatus != WWAN_STATUS_SUCCESS &&
        MbbUtilMbbToWwanStatus( MbbStatus ) != WWAN_STATUS_SUCCESS )
    {
        WwanStatus = MbbUtilMbbToWwanStatus( MbbStatus );
    }

    NdisUiccResetInfo.uStatus = WwanStatus;

    StatusIndication.StatusBuffer       = &NdisUiccResetInfo;
    StatusIndication.StatusBufferSize   = sizeof(NDIS_WWAN_UICC_RESET_INFO);

    TraceInfo(  WMBCLASS_OID, "[MbbNdis][ReqId=0x%04x] Indicating NDIS_STATUS_WWAN_UICC_RESET_INFO with status=%!WwanStatus!",
                Request->RequestId,
                WwanStatus
                );

    MbbNdisMiniportIndicateRequestStatus(
        Request,
        &StatusIndication
        );

    return NDIS_STATUS_SUCCESS;
}

NDIS_STATUS
MbbNdisSarConfigHandler(
    _In_ PMBB_REQUEST_CONTEXT                  Request,
    _In_ NDIS_STATUS                           NdisStatus,
    _In_ MBB_STATUS                            MbbStatus,
    _In_reads_bytes_opt_(InBufferSize) PUCHAR  InBuffer,
    _In_ ULONG                                 InBufferSize
    )
{
    BOOLEAN                     IndicateFailure = TRUE;
    WWAN_STATUS                 WwanStatus = WWAN_STATUS_FAILURE;
    ULONG                       ElementIndex;
    ULONG                       ElementCount;
    ULONG                       NdisSarConfigSize = 0;
    ULONG                       MbbSarConfigSize = 0;
    NDIS_STATUS_INDICATION      StatusIndication;
    PNDIS_WWAN_SAR_CONFIG_INFO  NdisSarConfigInfo = NULL;
    NDIS_WWAN_SAR_CONFIG_INFO   LocalNdisSarConfigInfo;
    PMBB_SAR_CONFIG_INFO        MbbSarConfigInfo = (PMBB_SAR_CONFIG_INFO)InBuffer;

    do
    {
        RtlZeroMemory(&LocalNdisSarConfigInfo, sizeof(LocalNdisSarConfigInfo));

        MBB_INITIALIZE_REQUEST_NDIS_STATUS_INDICATION(
            &StatusIndication,
            Request,
            NDIS_STATUS_WWAN_SAR_CONFIG
        );

        if (NdisStatus != NDIS_STATUS_SUCCESS)
        {
            TraceError(WMBCLASS_OID, "[MbbNdis][ReqId=0x%04x] FAILED to get data for NDIS_STATUS_WWAN_SAR_CONFIG, status=%!STATUS!",
                Request->RequestId,
                NdisStatus
            );
            break;
        }

        WwanStatus = MbbUtilMbbToWwanStatus(MbbStatus);

        if (WwanStatus != WWAN_STATUS_SUCCESS)
        {
            TraceError(WMBCLASS_OID, "[MbbNdis][ReqId=0x%04x] WwanStatus from device not success, WwanStatus=%!WwanStatus!. MbbStatus=%!MbbStatus!",
                Request->RequestId,
                WwanStatus,
                MbbStatus
            );
            NdisStatus = WwanStatus;
            break;
        }

        if (MbbSarConfigInfo == NULL ||
            InBufferSize < RTL_SIZEOF_THROUGH_FIELD(MBB_SAR_CONFIG_INFO, Configurations))
        {
            TraceError(WMBCLASS_OID, "[MbbNdis][ReqId=0x%04x] INSUFFICIENT BufferSize for NDIS_STATUS_WWAN_SAR_CONFIG, BufferSize[Received=%d Expected=%d]",
                Request->RequestId,
                InBufferSize,
                RTL_SIZEOF_THROUGH_FIELD(MBB_SAR_CONFIG_INFO, ElementCount)
            );
            NdisStatus = NDIS_STATUS_INVALID_DATA;
            break;
        }

        ElementCount = MbbSarConfigInfo->ElementCount;
        MbbSarConfigSize = FIELD_OFFSET(MBB_SAR_CONFIG_INFO, Configurations) + (ULONG)ElementCount * sizeof(MBB_ARRAY_ELEMENT);
        NdisSarConfigSize = sizeof(NDIS_WWAN_SAR_CONFIG_INFO) + (ULONG)ElementCount * sizeof(WWAN_SAR_CONFIG_INDICES);

        if (InBufferSize < MbbSarConfigSize)
        {
            TraceError(WMBCLASS_OID, "[MbbNdis][ReqId=0x%04x] INSUFFICIENT BufferSize for NDIS_STATUS_WWAN_SAR_CONFIG, BufferSize[Received=%d Expected=%I64u] ElementCount=%d",
                Request->RequestId,
                InBufferSize,
                MbbSarConfigSize,
                MbbSarConfigInfo->ElementCount
            );
            NdisStatus = NDIS_STATUS_INVALID_DATA;
            break;
        }

        for (ElementIndex = 0;
            ElementIndex < ElementCount;
            ElementIndex++)
        {
            if (MbbIsVariableFieldValid(
                InBufferSize,
                MbbSarConfigInfo->Configurations[ElementIndex].Offset,
                MbbSarConfigInfo->Configurations[ElementIndex].Size,
                2,  // Expect two indices in a configuration
                sizeof(ULONG)
            ) != STATUS_SUCCESS)
            {
                TraceError(WMBCLASS_OID, "[MbbNdis][ReqId=0x%04x] INVALID MBB_SAR_CONFIG_INFO.Configurations[%d] Offset[Received=%d BufferSize=%d] Size[Received=%d MaxExpected=%d]",
                    Request->RequestId,
                    ElementIndex,
                    MbbSarConfigInfo->Configurations[ElementIndex].Offset,
                    InBufferSize,
                    MbbSarConfigInfo->Configurations[ElementIndex].Size,
                    2 * sizeof(ULONG)
                );
                NdisStatus = NDIS_STATUS_INVALID_DATA;
                break;
            }
        }
        if (NdisStatus != NDIS_STATUS_SUCCESS)
        {
            break;
        }

        if ((NdisSarConfigInfo = ALLOCATE_NONPAGED_POOL(NdisSarConfigSize)) == NULL)
        {
            TraceError(WMBCLASS_OID, "[MbbNdis][ReqId=0x%04x] FAILED to allocate %I64u bytes for NDIS_STATUS_WWAN_SAR_CONFIG, ElementCount=%d",
                Request->RequestId,
                NdisSarConfigSize,
                MbbSarConfigInfo->ElementCount
            );
            NdisStatus = NDIS_STATUS_RESOURCES;
            break;
        }
        NdisSarConfigInfo->Header.Type     = NDIS_OBJECT_TYPE_DEFAULT;
        NdisSarConfigInfo->Header.Size     = sizeof(NDIS_WWAN_SAR_CONFIG_INFO);
        NdisSarConfigInfo->Header.Revision = NDIS_WWAN_SAR_CONFIG_INFO_REVISION_1;
        NdisSarConfigInfo->uStatus         = WWAN_STATUS_SUCCESS;

        StatusIndication.StatusBuffer     = NdisSarConfigInfo;
        StatusIndication.StatusBufferSize = NdisSarConfigSize;

        MbbUtilMbbToWwanSarConfigInfo(
            MbbSarConfigInfo,
            &NdisSarConfigInfo->SarConfig
        );

        MbbNdisMiniportIndicateRequestStatus(
            Request,
            &StatusIndication
        );

        IndicateFailure = FALSE;

    } while (FALSE);

    if (IndicateFailure)
    {

        LocalNdisSarConfigInfo.Header.Type = NDIS_OBJECT_TYPE_DEFAULT;
        LocalNdisSarConfigInfo.Header.Size = sizeof(LocalNdisSarConfigInfo);
        LocalNdisSarConfigInfo.Header.Revision = NDIS_WWAN_SAR_CONFIG_INFO_REVISION_1;
        LocalNdisSarConfigInfo.uStatus = NdisStatus;

        StatusIndication.StatusBuffer = &LocalNdisSarConfigInfo;
        StatusIndication.StatusBufferSize = sizeof(LocalNdisSarConfigInfo);

        TraceInfo(WMBCLASS_OID, "[MbbNdis][ReqId=0x%04x] FAILED to indicate NDIS_STATUS_WWAN_SAR_CONFIG WwanStatus=%!WwanStatus!", Request->RequestId, NdisStatus);

        MbbNdisMiniportIndicateRequestStatus(
            Request,
            &StatusIndication
        );
    }

    if (NdisSarConfigInfo != NULL)
    {
        FREE_POOL(NdisSarConfigInfo);
    }

    return NDIS_STATUS_SUCCESS;
}

NDIS_STATUS
MbbNdisSarTransmissionStatusHandler(
    _In_ PMBB_REQUEST_CONTEXT                  Request,
    _In_ NDIS_STATUS                           NdisStatus,
    _In_ MBB_STATUS                            MbbStatus,
    _In_reads_bytes_opt_(InBufferSize) PUCHAR  InBuffer,
    _In_ ULONG                                 InBufferSize
    )
{
    WWAN_STATUS                 WwanStatus = WWAN_STATUS_FAILURE;
    NDIS_STATUS_INDICATION      StatusIndication;
    NDIS_WWAN_SAR_TRANSMISSION_STATUS_INFO          NdisTransmissionStatusInfo;
    PMBB_SAR_TRANSMISSION_STATUS_INFO               MbbTransmissionStatusInfo = (PMBB_SAR_TRANSMISSION_STATUS_INFO)InBuffer;

    do
    {
        RtlZeroMemory(&NdisTransmissionStatusInfo, sizeof(NdisTransmissionStatusInfo));

        MBB_INITIALIZE_REQUEST_NDIS_STATUS_INDICATION(
            &StatusIndication,
            Request,
            NDIS_STATUS_WWAN_SAR_TRANSMISSION_STATUS
        );

        NdisTransmissionStatusInfo.Header.Type = NDIS_OBJECT_TYPE_DEFAULT;
        NdisTransmissionStatusInfo.Header.Size = sizeof(NDIS_WWAN_SAR_TRANSMISSION_STATUS_INFO);
        NdisTransmissionStatusInfo.Header.Revision = NDIS_WWAN_SAR_TRANSMISSION_STATUS_INFO_REVISION_1;

        if (NdisStatus != NDIS_STATUS_SUCCESS)
        {
            TraceError(WMBCLASS_OID, "[MbbNdis][ReqId=0x%04x] FAILED to get data for NDIS_STATUS_WWAN_SAR_TRANSMISSION_STATUS_INFO, status=%!STATUS!",
                Request->RequestId,
                NdisStatus
            );
            break;
        }

        WwanStatus = MbbUtilMbbToWwanStatus(MbbStatus);

        if (WwanStatus != WWAN_STATUS_SUCCESS)
        {
            TraceError(WMBCLASS_OID, "[MbbNdis][ReqId=0x%04x] WwanStatus from device not success, WwanStatus=%!WwanStatus!. MbbStatus=%!MbbStatus!",
                Request->RequestId,
                WwanStatus,
                MbbStatus
            );
            NdisStatus = WwanStatus;
            break;
        }

        if (MbbTransmissionStatusInfo == NULL ||
            InBufferSize < sizeof(MBB_SAR_TRANSMISSION_STATUS_INFO))
        {
            TraceError(WMBCLASS_OID, "[MbbNdis][ReqId=0x%04x] INSUFFICIENT BufferSize for NDIS_STATUS_WWAN_SAR_TRANSMISSION_STATUS_INFO, BufferSize[Received=%d Expected=%d]",
                Request->RequestId,
                InBufferSize,
                sizeof(MBB_SAR_TRANSMISSION_STATUS_INFO)
            );
            WwanStatus = WWAN_STATUS_FAILURE;
            break;
        }

        MbbUtilMbbToWwanTransmissionStatusInfo(
            MbbTransmissionStatusInfo,
            &NdisTransmissionStatusInfo.TransmissionStatus
        );
    } while (FALSE);
    //
    // Restore the device returned operational status, if any.
    //
    if (WwanStatus != WWAN_STATUS_SUCCESS &&
        MbbUtilMbbToWwanStatus(MbbStatus) != WWAN_STATUS_SUCCESS)
    {
        WwanStatus = MbbUtilMbbToWwanStatus(MbbStatus);
    }

    NdisTransmissionStatusInfo.uStatus = WwanStatus;

    StatusIndication.StatusBuffer = &NdisTransmissionStatusInfo;
    StatusIndication.StatusBufferSize = sizeof(NDIS_WWAN_SAR_TRANSMISSION_STATUS_INFO);

    TraceInfo(WMBCLASS_OID, "[MbbNdis][ReqId=0x%04x] Indicating NDIS_STATUS_WWAN_SAR_TRANSMISSION_STATUS with status=%!WwanStatus!",
        Request->RequestId,
        WwanStatus
    );

    MbbNdisMiniportIndicateRequestStatus(
        Request,
        &StatusIndication
    );

    return NDIS_STATUS_SUCCESS;
}


NDIS_STATUS
MbbNdisNetworkBlacklistHandler(
    __in PMBB_REQUEST_CONTEXT               Request,
    __in NDIS_STATUS                        NdisStatus,
    __in MBB_STATUS                         MbbStatus,
    __in_bcount_opt(InBufferSize) PUCHAR    InBuffer,
    __in ULONG                              InBufferSize
)
{
    ULONG                           NdisNetworkBlacklistSize = 0;
    WWAN_STATUS                     WwanStatus = WWAN_STATUS_FAILURE;
    NDIS_STATUS_INDICATION          StatusIndication;
    PNDIS_WWAN_NETWORK_BLACKLIST    NdisNetworkBlacklist = NULL;
    NDIS_WWAN_NETWORK_BLACKLIST     LocalNdisNetworkBlacklist;
    PMBB_MS_NETWORK_BLACKLIST_INFO  MbbNetworkBlacklist = (PMBB_MS_NETWORK_BLACKLIST_INFO)InBuffer;
    PMBB_MS_NETWORK_BLACKLIST_PROVIDER  MbbNetworkBlacklistProvider = NULL;
    ULONGLONG                       TempSize = 0;
    ULONG                           MulTempSize = 0;
    ULONG                           i;
    NTSTATUS                        NtStatus = STATUS_SUCCESS;

    do
    {
        RtlZeroMemory(&LocalNdisNetworkBlacklist, sizeof(LocalNdisNetworkBlacklist));

        MBB_INITIALIZE_REQUEST_NDIS_STATUS_INDICATION(
            &StatusIndication,
            Request,
            NDIS_STATUS_WWAN_NETWORK_BLACKLIST
        );

        if (NdisStatus != NDIS_STATUS_SUCCESS)
        {
            TraceError(WMBCLASS_OID, "[MbbNdis][ReqId=0x%04x] FAILED to get data for NDIS_STATUS_WWAN_NETWORK_BLACKLIST, status=%!STATUS!",
                Request->RequestId,
                NdisStatus
            );
            break;
        }

        WwanStatus = MbbUtilMbbToWwanStatus(MbbStatus);

        if (WwanStatus != WWAN_STATUS_SUCCESS)
        {
            break;
        }


        if (MbbReqMgrIsSetOid(Request) == TRUE)
        {
            break;
        }

        TempSize = RTL_SIZEOF_THROUGH_FIELD(MBB_MS_NETWORK_BLACKLIST_INFO, ElementCount);

        if (MbbNetworkBlacklist == NULL ||
            InBufferSize < RTL_SIZEOF_THROUGH_FIELD(MBB_MS_NETWORK_BLACKLIST_INFO, ElementCount))
        {
            TraceError(WMBCLASS_OID, "[MbbNdis][ReqId=0x%04x] INSUFFICIENT BufferSize for NDIS_STATUS_WWAN_NETWORK_BLACKLIST, BufferSize[Received=%d Expected=%d]",
                Request->RequestId,
                InBufferSize,
                RTL_SIZEOF_THROUGH_FIELD(MBB_MS_NETWORK_BLACKLIST_INFO, ElementCount)
            );
            WwanStatus = WWAN_STATUS_FAILURE;
            break;
        }

        //
        //  make sure it is big enough for the array of elements
        //
        NtStatus = RtlULongMult(
            MbbNetworkBlacklist->ElementCount,
            sizeof(MBB_ARRAY_ELEMENT),
            &MulTempSize
        );

        if (NT_ERROR(NtStatus))
        {
            TraceError(WMBCLASS_OID, "[MbbNdis][ReqId=0x%04x] Multiplication overflow occured, MbbNetworkBlacklist->ElementCount=%d MBB_ARRAY_ELEMENT size=%d",
                Request->RequestId,
                MbbNetworkBlacklist->ElementCount,
                sizeof(MBB_ARRAY_ELEMENT)
            );

            WwanStatus = WWAN_STATUS_FAILURE;
            break;
        }

        TempSize += MulTempSize;

        if (InBufferSize < TempSize)
        {
            TraceError(WMBCLASS_OID, "[MbbNdis][ReqId=0x%04x] INSUFFICIENT BufferSize for NDIS_STATUS_WWAN_NETWORK_BLACKLIST, BufferSize[Received=%d Expected=%d]",
                Request->RequestId,
                InBufferSize,
                (ULONG)TempSize
            );

            WwanStatus = WWAN_STATUS_FAILURE;
            break;
        }

        for (i = 0; i < MbbNetworkBlacklist->ElementCount; i++)
        {

            NdisStatus = MbbIsVariableFieldValid(
                InBufferSize,
                MbbNetworkBlacklist->Contexts[i].Offset,
                MbbNetworkBlacklist->Contexts[i].Size,
                2048,                 // some reasonable upper limit
                sizeof(UCHAR)
            );

            if (!NT_SUCCESS(NdisStatus))
            {
                TraceError(WMBCLASS_OID, "[MbbNdis][ReqId=0x%04x] bad array element in MBB_MS_NETWORK_BLACKLIST_INFO",
                    Request->RequestId
                );

                WwanStatus = WWAN_STATUS_FAILURE;
                break;
            }

            MbbNetworkBlacklistProvider = (PMBB_MS_NETWORK_BLACKLIST_PROVIDER)(InBuffer + MbbNetworkBlacklist->Contexts[i].Offset);
        }

        if (WwanStatus != WWAN_STATUS_SUCCESS)
        {
            break;
        }

        NtStatus = RtlULongMult(
            MbbNetworkBlacklist->ElementCount,
            sizeof(MBB_MS_NETWORK_BLACKLIST_PROVIDER),
            &NdisNetworkBlacklistSize
        );
        if (NT_ERROR(NtStatus))
        {
            TraceError(WMBCLASS_OID, "[MbbNdis][ReqId=0x%04x] Multiplication overflow occured, MbbNetworkBlacklist->ElementCount=%d MBB_MS_NETWORK_BLACKLIST_PROVIDER size=%d",
                Request->RequestId,
                MbbNetworkBlacklist->ElementCount,
                sizeof(WWAN_CONTEXT)
            );
            WwanStatus = WWAN_STATUS_FAILURE;
            break;
        }

        NtStatus = RtlULongAdd(
            NdisNetworkBlacklistSize,
            sizeof(NDIS_WWAN_NETWORK_BLACKLIST),
            &NdisNetworkBlacklistSize
        );
        if (NT_ERROR(NtStatus))
        {
            TraceError(WMBCLASS_OID, "[MbbNdis][ReqId=0x%04x] Addition overflow occured, NdisNetworkBlacklistSize=%d NDIS_WWAN_NETWORK_BLACKLIST size=%d",
                Request->RequestId,
                NdisNetworkBlacklistSize,
                sizeof(NDIS_WWAN_NETWORK_BLACKLIST)
            );
            WwanStatus = WWAN_STATUS_FAILURE;
            break;
        }

        NdisNetworkBlacklist = ALLOCATE_NONPAGED_POOL(NdisNetworkBlacklistSize);

        if (NdisNetworkBlacklist == NULL)
        {
            TraceError(WMBCLASS_OID, "[MbbNdis][ReqId=0x%04x] FAILED to allocate %d bytes for NDIS_STATUS_WWAN_NETWORK_BLACKLIST, ElementCount=%d",
                Request->RequestId,
                NdisNetworkBlacklistSize,
                MbbNetworkBlacklist->ElementCount
            );
            WwanStatus = WWAN_STATUS_FAILURE;
            break;
        }

        RtlZeroMemory(NdisNetworkBlacklist, NdisNetworkBlacklistSize);

        NdisNetworkBlacklist->Header.Type = NDIS_OBJECT_TYPE_DEFAULT;
        NdisNetworkBlacklist->Header.Size = sizeof(NDIS_WWAN_NETWORK_BLACKLIST);
        NdisNetworkBlacklist->Header.Revision = NDIS_WWAN_NETWORK_BLACKLIST_REVISION_1;

        NdisNetworkBlacklist->NetworkBlacklistInfo.BlacklistState = MbbNetworkBlacklist->BlacklistState;

        MbbUtilMbbToWwanNetworkBlackList(
            MbbNetworkBlacklist,
            &NdisNetworkBlacklist->NetworkBlacklistInfo.BlacklistProviderList
        );
    } while (FALSE);
    //
    // Restore the device returned operational status, if any.
    //
    if (WwanStatus != WWAN_STATUS_SUCCESS &&
        MbbUtilMbbToWwanStatus(MbbStatus) != WWAN_STATUS_SUCCESS)
    {
        WwanStatus = MbbUtilMbbToWwanStatus(MbbStatus);
    }

    if (NdisNetworkBlacklist == NULL)
    {
        LocalNdisNetworkBlacklist.Header.Type = NDIS_OBJECT_TYPE_DEFAULT;
        LocalNdisNetworkBlacklist.Header.Size = sizeof(NDIS_WWAN_NETWORK_BLACKLIST);
        LocalNdisNetworkBlacklist.Header.Revision = NDIS_WWAN_NETWORK_BLACKLIST_REVISION_1;

        LocalNdisNetworkBlacklist.NetworkBlacklistInfo.BlacklistProviderList.ElementType = WwanStructNetworkBlacklistProvider;
        LocalNdisNetworkBlacklist.NetworkBlacklistInfo.BlacklistProviderList.ElementCount = 0;

        StatusIndication.StatusBuffer = &LocalNdisNetworkBlacklist;
        StatusIndication.StatusBufferSize = sizeof(NDIS_WWAN_NETWORK_BLACKLIST);
    }
    else
    {
        StatusIndication.StatusBuffer = NdisNetworkBlacklist;
        StatusIndication.StatusBufferSize = NdisNetworkBlacklistSize;
    }

    ((PNDIS_WWAN_PROVISIONED_CONTEXTS)(StatusIndication.StatusBuffer))->uStatus = WwanStatus;

    TraceInfo(WMBCLASS_OID, "[MbbNdis][ReqId=0x%04x] Indicating NDIS_STATUS_WWAN_NETWORK_BLACKLIST with status=%!WwanStatus!",
        Request->RequestId,
        WwanStatus
    );

    MbbNdisMiniportIndicateRequestStatus(
        Request,
        &StatusIndication
    );

    if (NdisNetworkBlacklist != NULL)
    {
        FREE_POOL(NdisNetworkBlacklist);
    }
    return NDIS_STATUS_SUCCESS;
}

NDIS_STATUS
MbbNdisPcoStatusHandler(
    _In_ PMBB_REQUEST_CONTEXT                  Request,
    _In_ NDIS_STATUS                           NdisStatus,
    _In_ MBB_STATUS                            MbbStatus,
    _In_reads_bytes_opt_(InBufferSize) PUCHAR  InBuffer,
    _In_ ULONG                                 InBufferSize
    )
{
    WWAN_STATUS                 WwanStatus = WWAN_STATUS_FAILURE;
    NDIS_STATUS_INDICATION      StatusIndication;
    NDIS_WWAN_PCO_STATUS        NdisPcoStatus;
    PMBB_MS_PCO_VALUE           pMbbPcoValue = (PMBB_MS_PCO_VALUE)InBuffer;
    PMINIPORT_ADAPTER_CONTEXT   Adapter = (PMINIPORT_ADAPTER_CONTEXT)MbbReqMgrGetAdapterHandle(Request);
    PMBB_PORT                   Port = NULL;
    NDIS_PORT_NUMBER            PortNumber = NDIS_DEFAULT_PORT_NUMBER;
    BOOLEAN                     ResetPort = FALSE;

    do
    {
        RtlZeroMemory(&NdisPcoStatus, sizeof(NdisPcoStatus));

        MBB_INITIALIZE_REQUEST_NDIS_STATUS_INDICATION(
            &StatusIndication,
            Request,
            NDIS_STATUS_WWAN_PCO_STATUS
        );

        NdisPcoStatus.Header.Type = NDIS_OBJECT_TYPE_DEFAULT;
        NdisPcoStatus.Header.Size = sizeof(NDIS_WWAN_PCO_STATUS);
        NdisPcoStatus.Header.Revision = NDIS_WWAN_PCO_STATUS_REVISION_1;

        if (NdisStatus != NDIS_STATUS_SUCCESS)
        {
            TraceError(WMBCLASS_OID, "[MbbNdis][ReqId=0x%04x] FAILED to get data for NDIS_STATUS_WWAN_PCO_STATUS, status=%!STATUS!",
                Request->RequestId,
                NdisStatus
            );
            break;
        }

        if ((WwanStatus = MbbUtilMbbToWwanStatus(MbbStatus)) != WWAN_STATUS_SUCCESS)
        {
            TraceError(WMBCLASS_OID, "[MbbNdis][ReqId=0x%04x] WwanStatus from device not success, WwanStatus=%!WwanStatus!. MbbStatus=%!MbbStatus!",
                Request->RequestId,
                WwanStatus,
                MbbStatus
            );
            NdisStatus = NDIS_STATUS_FAILURE;
            break;
        }

        if (pMbbPcoValue == NULL ||
            InBufferSize < sizeof(MBB_MS_PCO_VALUE))
        {
            TraceError(WMBCLASS_OID, "[MbbNdis][ReqId=0x%04x] INSUFFICIENT BufferSize for NDIS_STATUS_WWAN_PCO_STATUS, BufferSize[Received=%d Expected=%d]",
                Request->RequestId,
                InBufferSize,
                sizeof(MBB_MS_PCO_VALUE)
            );
            NdisStatus = NDIS_STATUS_INVALID_DATA;
            WwanStatus = WWAN_STATUS_FAILURE;
            break;
        }

        // Get the port. Always rely on the session id returned by the device as this
        // responsehandler may be triggered due to an OID request from above
        // or some MB device specific notification from below. Session Id is set in both cases.
        Port = MbbWwanTranslateSessionIdToPort(Adapter, pMbbPcoValue->SessionId);

        if (Port == NULL)
        {
            // invalid port context
            NdisStatus = NDIS_STATUS_INVALID_PORT;
            WwanStatus = WWAN_STATUS_FAILURE;
            break;
        }

        // Get the port number
        MBB_ACQUIRE_PORT_LOCK(Port);
        PortNumber = Port->PortNumber;
        MBB_RELEASE_PORT_LOCK(Port);

        // Copy PCO data from MBB payload to NDIS payload
        NdisPcoStatus.PcoValue.Size = pMbbPcoValue->PcoDataSize;
        NdisPcoStatus.PcoValue.Type = pMbbPcoValue->PcoDataType;
        RtlCopyMemory(
            (PBYTE)&NdisPcoStatus.PcoValue.PcoData[0],
            (PBYTE)&pMbbPcoValue->PcoDataBuffer[0],
            pMbbPcoValue->PcoDataSize);

    } while (FALSE);

    if ((NDIS_STATUS_INVALID_DATA == NdisStatus) && !MbbReqMgrIsUnsolicitedIndication(Request))
    {
        //
        // We cannot get the SessionId from the MbbContextState because it is invalid.
        // Since this is a transactional response, we do a best-effort to send the response up
        // on the port from OidRequest.
        // NOTE: Unsolicited indications with invalid SessionId are dropped.
        //

        PortNumber = Request->OidContext.OidRequest->PortNumber;
        Port = MbbWwanTranslatePortNumberToPort(Adapter, PortNumber);

        if (Port != NULL)
        {
            TraceInfo(WMBCLASS_OID, "[MbbNdis][ReqId=0x%04x] MbbPcoValue is invalid. Using NDIS_DEFAULT_PORT_NUMBER (%lu) to indicate response",
                Request->RequestId,
                PortNumber);

            NdisPcoStatus.PcoValue.Size = 0;
            NdisPcoStatus.PcoValue.Type = WwanPcoTypeMax;
        }
        else
        {
            TraceError(WMBCLASS_OID, "[MbbNdis][ReqId=0x%04x] MbbPcoValue is invalid AND we failed to retrieve the default port info",
                Request->RequestId);
        }
    }

    if (Port != NULL)
    {
        // Indicate status indication on the correct port.

        NdisPcoStatus.uStatus = WwanStatus;

        StatusIndication.StatusBuffer     = &NdisPcoStatus;
        StatusIndication.StatusBufferSize = sizeof(NDIS_WWAN_PCO_STATUS);
        StatusIndication.PortNumber       = PortNumber;

        TraceInfo(WMBCLASS_OID, "[MbbNdis][ReqId=0x%04x] Indicating NDIS_STATUS_WWAN_PCO_STATUS with status=%!WwanStatus! for Port number=%lu",
            Request->RequestId,
            WwanStatus,
            PortNumber);

        MbbNdisMiniportIndicateRequestStatus(
            Request,
            &StatusIndication);

        Dereference(Port);
    }
    else
    {
        TraceError(WMBCLASS_OID, "[MbbNdis][ReqId=0x%04x] Not indicating OID_WWAN_PCO completion as the port context is invalid.",
            Request->RequestId);
    }

    return NdisStatus;
}

NDIS_STATUS
MbbNdisDeviceResetStatusHandler(
    _In_ PMBB_REQUEST_CONTEXT                   Request,
    _In_ NDIS_STATUS                            NdisStatus,
    _In_ MBB_STATUS                             MbbStatus,
    _In_reads_bytes_opt_(InBufferSize) PUCHAR   InBuffer,
    _In_ ULONG                                  InBufferSize
)
{
    WWAN_STATUS                         WwanStatus;
    NDIS_STATUS_INDICATION              StatusIndication;
    NDIS_WWAN_DEVICE_RESET_STATUS       NdisDeviceResetStatus;

    UNREFERENCED_PARAMETER(InBuffer);
    UNREFERENCED_PARAMETER(InBufferSize);

    do
    {
        MBB_INITIALIZE_REQUEST_NDIS_STATUS_INDICATION(
            &StatusIndication,
            Request,
            NDIS_STATUS_WWAN_DEVICE_RESET_STATUS
        );

        WwanStatus = MbbUtilMbbToWwanStatus(MbbStatus);

        if (NdisStatus != NDIS_STATUS_SUCCESS)
        {
            TraceError(WMBCLASS_OID, "[MbbNdis][ReqId=0x%04x] FAILED to get data for NDIS_STATUS_WWAN_DEVICE_RESET_STATUS, status=%!STATUS!",
                Request->RequestId,
                NdisStatus
            );
            WwanStatus = WWAN_STATUS_FAILURE;
            break;
        }

        RtlZeroMemory(&NdisDeviceResetStatus, sizeof(NdisDeviceResetStatus));

        NdisDeviceResetStatus.Header.Type = NDIS_OBJECT_TYPE_DEFAULT;
        NdisDeviceResetStatus.Header.Size = sizeof(NDIS_WWAN_DEVICE_RESET_STATUS);
        NdisDeviceResetStatus.Header.Revision = NDIS_WWAN_DEVICE_RESET_STATUS_REVISION_1;
    } while (FALSE);

    //
    // Restore the device returned operational status, if any.
    //
    if (WwanStatus != WWAN_STATUS_SUCCESS &&
        MbbUtilMbbToWwanStatus(MbbStatus) != WWAN_STATUS_SUCCESS)
    {
        WwanStatus = MbbUtilMbbToWwanStatus(MbbStatus);
    }

    NdisDeviceResetStatus.uStatus = WwanStatus;

    StatusIndication.StatusBuffer = &NdisDeviceResetStatus;
    StatusIndication.StatusBufferSize = sizeof(NDIS_WWAN_DEVICE_RESET_STATUS);

    TraceInfo(WMBCLASS_OID, "[MbbNdis][ReqId=0x%04x] Indicating NDIS_STATUS_WWAN_DEVICE_RESET_STATUS with status=%!WwanStatus!",
        Request->RequestId,
        WwanStatus
    );

    MbbNdisMiniportIndicateRequestStatus(
        Request,
        &StatusIndication
    );

    return NDIS_STATUS_SUCCESS;
}

NDIS_STATUS
MbbNdisBaseStationsInfoHandler(
    _In_ PMBB_REQUEST_CONTEXT                  Request,
    _In_ NDIS_STATUS                           NdisStatus,
    _In_ MBB_STATUS                            MbbStatus,
    _In_reads_bytes_opt_(InBufferSize) PUCHAR  InBuffer,
    _In_ ULONG                                 InBufferSize
)
{
    BOOLEAN                         IndicationFailure = TRUE;
    WWAN_STATUS                     WwanStatus;
    NDIS_STATUS_INDICATION          StatusIndication;
    ULONG                           NdisBaseStationsInfoSize = 0;
    PNDIS_WWAN_BASE_STATIONS_INFO   NdisBaseStationsInfoResponse = NULL;
    PMBB_MS_BASE_STATIONS_INFO      MbbBaseStationInfoResponse = (PMBB_MS_BASE_STATIONS_INFO)InBuffer;

    do
    {
        MBB_INITIALIZE_REQUEST_NDIS_STATUS_INDICATION(
            &StatusIndication,
            Request,
            NDIS_STATUS_WWAN_BASE_STATIONS_INFO
        );

        WwanStatus = MbbUtilMbbToWwanStatus(MbbStatus);

        if (NdisStatus != NDIS_STATUS_SUCCESS)
        {
            TraceError(WMBCLASS_OID, "[MbbNdis][ReqId=0x%04x] FAILED to get data for NDIS_STATUS_WWAN_BASE_STATIONS_INFO, status=%!STATUS!",
                Request->RequestId,
                NdisStatus
            );
            WwanStatus = WWAN_STATUS_FAILURE;
            break;
        }

        if (MbbBaseStationInfoResponse == NULL ||
            InBufferSize < (ULONG)FIELD_OFFSET(MBB_MS_BASE_STATIONS_INFO, DataBuffer))
        {
            TraceError(WMBCLASS_OID, "[MbbNdis][ReqId=0x%04x] INSUFFICIENT BufferSize for NDIS_STATUS_WWAN_BASE_STATIONS_INFO, BufferSize[Received=%d Expected=%d]",
                Request->RequestId,
                InBufferSize,
                FIELD_OFFSET(MBB_MS_BASE_STATIONS_INFO, DataBuffer)
            );
            WwanStatus = WWAN_STATUS_FAILURE;
            break;
        }

        // ----------------------------------------------------------
        // NDIS_WWAN_BASE_STATIONS_INFO
        // |---- header
        // |---- uStatus
        // |---- BaseStationsInfo (WWAN_BASE_STATIONS_INFO) ------------+
        // |     |---- SystemTypes (ULONG)                              |
        // |     |---- Offsets and sizes (ULONGs)                       + Same content
        // |     |---- BaseStationsData (ANYSIZE_ARRAY)                 |
        // -------------------------------                              |
        // MBB_MS_BASE_STATIONS_INFO -----------------------------------+
        // |---- SystemType (MBB_DATA_CLASS_VALUE)
        // |---- Offsets and sizes (MBB_ARRAY_ELEMENTs)
        // |---- DataBuffer[1]
        // ----------------------------------------------------------
        NdisBaseStationsInfoSize = FIELD_OFFSET(NDIS_WWAN_BASE_STATIONS_INFO, BaseStationsInfo.BaseStationsData);
        ULONG MbbBaseStationDataSize = 
            MbbBaseStationInfoResponse->GSMServingCell.Size +
            MbbBaseStationInfoResponse->UMTSServingCell.Size +
            MbbBaseStationInfoResponse->TDSCDMAServingCell.Size +
            MbbBaseStationInfoResponse->LTEServingCell.Size +
            MbbBaseStationInfoResponse->GSMNmr.Size +
            MbbBaseStationInfoResponse->UMTSMrl.Size +
            MbbBaseStationInfoResponse->TDSCDMAMrl.Size +
            MbbBaseStationInfoResponse->LTEMrl.Size +
            MbbBaseStationInfoResponse->CDMAMrl.Size;
        NdisBaseStationsInfoSize += MbbBaseStationDataSize;

        NdisBaseStationsInfoResponse = (PNDIS_WWAN_BASE_STATIONS_INFO)ALLOCATE_NONPAGED_POOL(NdisBaseStationsInfoSize);

        if (NdisBaseStationsInfoResponse == NULL)
        {
            TraceError(WMBCLASS_OID, "[MbbNdis][ReqId=0x%04x] FAILED to allocate %d bytes for NDIS_STATUS_WWAN_BASE_STATIONS_INFO",
                Request->RequestId,
                NdisBaseStationsInfoSize
            );
            WwanStatus = WWAN_STATUS_FAILURE;
            break;
        }

        RtlZeroMemory(NdisBaseStationsInfoResponse, NdisBaseStationsInfoSize);

        NdisBaseStationsInfoResponse->Header.Type = NDIS_OBJECT_TYPE_DEFAULT;
        NdisBaseStationsInfoResponse->Header.Size = (USHORT)NdisBaseStationsInfoSize;
        NdisBaseStationsInfoResponse->Header.Revision = NDIS_WWAN_BASE_STATIONS_INFO_REVISION_1;

        // Copy MBB_MS_BASE_STATIONS_INFO to WWAN_BASE_STATIONS_INFO
        RtlCopyMemory(
            &NdisBaseStationsInfoResponse->BaseStationsInfo,
            MbbBaseStationInfoResponse,
            FIELD_OFFSET(WWAN_BASE_STATIONS_INFO, BaseStationsData) + MbbBaseStationDataSize);

        IndicationFailure = FALSE;
    } while (FALSE);

    //
    // Restore the device returned operational status, if any.
    //
    if (WwanStatus != WWAN_STATUS_SUCCESS &&
        MbbUtilMbbToWwanStatus(MbbStatus) != WWAN_STATUS_SUCCESS)
    {
        WwanStatus = MbbUtilMbbToWwanStatus(MbbStatus);
    }

    if (!IndicationFailure)
    {
        NdisBaseStationsInfoResponse->uStatus = WwanStatus;

        StatusIndication.StatusBuffer = NdisBaseStationsInfoResponse;
        StatusIndication.StatusBufferSize = NdisBaseStationsInfoSize;
    }
    else
    {
        NDIS_WWAN_BASE_STATIONS_INFO    LocalNdisBaseStationsInfoResponse;
        RtlZeroMemory(&LocalNdisBaseStationsInfoResponse, FIELD_OFFSET(NDIS_WWAN_BASE_STATIONS_INFO, BaseStationsInfo.BaseStationsData));

        LocalNdisBaseStationsInfoResponse.Header.Type = NDIS_OBJECT_TYPE_DEFAULT;
        LocalNdisBaseStationsInfoResponse.Header.Size = FIELD_OFFSET(NDIS_WWAN_BASE_STATIONS_INFO, BaseStationsInfo.BaseStationsData);
        LocalNdisBaseStationsInfoResponse.Header.Revision = NDIS_WWAN_BASE_STATIONS_INFO_REVISION_1;
        LocalNdisBaseStationsInfoResponse.uStatus = WwanStatus;

        StatusIndication.StatusBuffer = &LocalNdisBaseStationsInfoResponse;
        StatusIndication.StatusBufferSize = FIELD_OFFSET(NDIS_WWAN_BASE_STATIONS_INFO, BaseStationsInfo.BaseStationsData);
    }

    TraceInfo(WMBCLASS_OID, "[MbbNdis][ReqId=0x%04x] Indicating NDIS_STATUS_WWAN_BASE_STATIONS_INFO with status=%!WwanStatus!",
        Request->RequestId,
        WwanStatus
    );

    MbbNdisMiniportIndicateRequestStatus(
        Request,
        &StatusIndication
    );

    if (NdisBaseStationsInfoResponse != NULL)
    {
        FREE_POOL(NdisBaseStationsInfoResponse);
    }

    return NDIS_STATUS_SUCCESS;
}

//
// Completion Handlers
//

VOID
MbbNdisReadyInfoCompletionHandler(
    __in PMBB_REQUEST_CONTEXT               Request,
    __in NDIS_STATUS                        NdisStatus
    )
{
    if( NdisStatus != NDIS_STATUS_SUCCESS )
    {
        if( Request->HandlerContext.Parameters.SubscriberReadyInfo.IsQueryReadyInfo == 1 )
            MbbNdisIndicateReadyInfoFailure( Request );

        if( Request->HandlerContext.Parameters.SubscriberReadyInfo.NdisReadyInfo != NULL )
            FREE_POOL( Request->HandlerContext.Parameters.SubscriberReadyInfo.NdisReadyInfo );
    }
}


VOID
MbbNdisPacketServiceStateCompletionHandler(
    __in PMBB_REQUEST_CONTEXT               Request,
    __in NDIS_STATUS                        NdisStatus
    )
/*++
Description:
    This is the completion handler for both PacketService CIDs
    and RegisterState CIDs. RegisterState CID is issued when a
    packet service response or indication is received since some
    fields in the PacketService OID (OID model) require data not
    present in the PacketService CID but present in the
    RegisterState CID.
--*/
{
    if( NdisStatus != NDIS_STATUS_SUCCESS )
    {
        if( Request->HandlerContext.Parameters.PacketServiceState.IsQuery == 1 )
        {
            MbbNdisIndicatePacketServiceFailure( Request, NdisStatus );
        }

        if( Request->HandlerContext.Parameters.PacketServiceState.NdisPacketServiceState != NULL )
        {
            FREE_POOL( Request->HandlerContext.Parameters.PacketServiceState.NdisPacketServiceState );
            Request->HandlerContext.Parameters.PacketServiceState.NdisPacketServiceState=NULL;
        }
    }
}

VOID
MbbNdisMultiCarrierDsCidListCompletionHandler(
    __in PMBB_REQUEST_CONTEXT               Request,
    __in NDIS_STATUS                        NdisStatus
    )
{
    if( NdisStatus != NDIS_STATUS_SUCCESS )
    {
        MbbNdisIndicateDeviceCaps(
            Request,
            NdisStatus
            );
    }
}

VOID
MbbNdisCreateMacCompletionHandler(
    __in PMBB_REQUEST_CONTEXT               Request,
    __in NDIS_STATUS                        NdisStatus
    )
{
    NDIS_STATUS                 Status = NDIS_STATUS_SUCCESS;
    PMINIPORT_ADAPTER_CONTEXT   Adapter = (PMINIPORT_ADAPTER_CONTEXT) MbbReqMgrGetAdapterHandle(Request);
    NDIS_PORT_NUMBER            PortNumber = DEFAULT_NDIS_PORT_NUMBER;
    PMBB_PORT                   Port = NULL;
    BOOLEAN                     fInAdapterPortList = FALSE;

    Port = Request->HandlerContext.Parameters.NdisPortContext.Port;

    if(Port)
    {
        MBB_ACQUIRE_PORT_LOCK(Port);
        PortNumber = Port->PortNumber;
        MBB_RELEASE_PORT_LOCK(Port);

        if(IS_ALLOCATED_PORT_NUMBER(PortNumber))
        {
            // We should never activate the default port

            // Check whether the port is in the adapter port list
            PMBB_PORT PortToCheck = NULL;

            fInAdapterPortList = (PortToCheck = MbbWwanTranslatePortNumberToPort(Adapter,PortNumber))!= NULL;

            if(fInAdapterPortList)
            {
                // Remove the reference added for port find
                Dereference(PortToCheck);

                Status = MbbUtilWwanCreateMacComplete(
                            Adapter,
                            Port,
                            NdisStatus);
            }
        }

        // Remove the reference added when port was added to handlercontext
        Dereference(Port);
    }
    else
    {
        Status = NDIS_STATUS_INVALID_PORT;
    }

    if(Status != NDIS_STATUS_SUCCESS)
    {
        TraceError( WMBCLASS_OID, "[MbbNdis][ReqID=0x%04x] MbbNdisCreateMacCompletionHandler failed. Status = 0x%08x\n",
                    Request->RequestId,Status);
    }
    else
    {
        TraceError( WMBCLASS_OID, "[MbbNdis][ReqID=0x%04x] MbbNdisCreateMacCompletionHandler completed successfully.\n",
                Request->RequestId);
    }
}


VOID
MbbNdisDeleteMacCompletionHandler(
    __in PMBB_REQUEST_CONTEXT               Request,
    __in NDIS_STATUS                        NdisStatus
    )
{
    NDIS_STATUS                 Status = NDIS_STATUS_SUCCESS;
    PMINIPORT_ADAPTER_CONTEXT   Adapter = (PMINIPORT_ADAPTER_CONTEXT) MbbReqMgrGetAdapterHandle(Request);
    NDIS_PORT_NUMBER            PortNumber = DEFAULT_NDIS_PORT_NUMBER;
    PMBB_PORT                   Port = Request->HandlerContext.Parameters.NdisPortContext.Port;

    if(Port)
    {
        MBB_ACQUIRE_PORT_LOCK(Port);
        PortNumber = Port->PortNumber;
        MBB_RELEASE_PORT_LOCK(Port);

        if(IS_ALLOCATED_PORT_NUMBER(PortNumber))
        {
            Status = MbbUtilWwanDeleteMacComplete(
                                   Adapter,
                                   Port,
                                   NdisStatus);
        }
        else
        {
            Status = NDIS_STATUS_INVALID_PORT;
        }

        // Remove the reference added when the port context was
        // passed on as the handlercontext
        Dereference(Port);
    }

    if(Status != NDIS_STATUS_SUCCESS)
    {
       TraceError( WMBCLASS_OID, "[MbbNdis][ReqID=0x%04x] MbbNdisDeleteMacCompletionHandler failed. Status = 0x%08x\n",
                   Request->RequestId,Status);
    }
    else
    {
       TraceError( WMBCLASS_OID, "[MbbNdis][ReqID=0x%04x] MbbNdisDeleteMacCompletionHandler completed successfully.\n",
               Request->RequestId);
    }
}

#pragma prefast(pop)