/***************************************************************************

Copyright (c) 2010 Microsoft Corporation

Module Name:

    Util.h

Abstract:

    This file contains the message definitions for helper functions.
    All data conversion from MBB structures to WWAN structures and 
    vice versa is present in Util.c.

    Note the MbbUtilXxx routines do not perform length validation
    as they require the caller to perform validation and allocate
    sufficient memory.

Environment:

    kernel mode only

Notes:

    
Revision History:

    3/2/2010 : TriRoy : Added MbbUtilXxx routines

Authors:

    BrianL
    TriRoy

****************************************************************************/

////////////////////////////////////////////////////////////////////////////////
//
//  DEFINES
//
////////////////////////////////////////////////////////////////////////////////

//
// Infrastructure for testability.
// Flags can be added to conditionally simulate test situations.
// MbbGlobalTestFlags can be set from the debugger to trigger conditions.
//
//typedef enum
//{
//    MbbTestFlagNone = 0x00000000,
//    MbbTestFlagSimulateQueryDeviceServiceDispatchFailure =  0x00000001,
//    MbbTestFlagSimulateQueryDeviceServiceTimeout =  0x00000002,
//    MbbTestFlagSimulateInvalidResponseBufferSize =  0x00000004
//
//} MBB_TEST_FLAG;
//
//extern volatile ULONG MbbGlobalTestFlags;

typedef union
{
    ULONG       Value;
    struct
    {
        ULONG   NoPostAllocation:1;
    };

} MBB_ALLOC_FLAGS;

typedef __callback
VOID
(*MBB_WORKITEM_ROUTINE)(
    __in PVOID Context1,
    __in PVOID Context2,
    __in PVOID Context3,
    __in PVOID Context4
    );

typedef enum
{
    MbbPoolTagDefault       = '0CBM',
    MbbPoolTagNtbSend       = '1CBM',
    MbbPoolTagNblSend       = '2CBM',
    MbbPoolTagNbSend        = '3CBM',
    MbbPoolTagNtbReceive    = '4CBM',
    MbbPoolTagNblReceive    = '5CBM',
    MbbPoolTagMdlReceive    = '6CBM',
    MbbPoolTagNbPool        = '7CBM',
    MbbPoolTagNblPool       = '8CBM',
    MbbPoolTagTimer         = '9CBM',
    MbbPoolTagRequest       = 'rCBM',
    MbbPoolTagPort          = 'pCBM',
    MbbPoolTagNblFastIOContext = 'cOIF'
} MBB_POOL_TAG;

#define ALLOCATE_PAGED_POOL(_y) ExAllocatePool2(POOL_FLAG_PAGED, _y, MbbPoolTagDefault)
#define ALLOCATE_NONPAGED_POOL(_y) ExAllocatePool2(POOL_FLAG_NON_PAGED, _y, MbbPoolTagDefault)
#define ALLOCATE_NONPAGED_POOL_WITH_TAG(_x, _y) ExAllocatePool2(POOL_FLAG_NON_PAGED, _x, _y)

#define FREE_POOL(_x)               {ExFreePool(_x);_x=NULL;};

#define ALLOCATE_LOOKASIDE(LOOKASIDELIST)       NdisAllocateFromNPagedLookasideList( LOOKASIDELIST )
#define FREE_LOOKASIDE(MEMORY,LOOKASIDELIST)    { NdisFreeToNPagedLookasideList( (LOOKASIDELIST), (MEMORY) ); (MEMORY) = NULL; };

#define MIN( _X_, _Y_ ) (((_X_) < (_Y_))? (_X_): (_Y_))
#define MAX( _X_, _Y_ ) (((_X_) > (_Y_))? (_X_): (_Y_))

#define ALIGN_FLOOR( _VALUE_, _ALIGN_ )     ((SIZE_T)(_VALUE_) - (((SIZE_T)(_VALUE_))%((SIZE_T)(_ALIGN_))))
#define ALIGN_CIELING( _VALUE_, _ALIGN_ )   ALIGN_FLOOR( _VALUE_, _ALIGN_ ) + ((SIZE_T)(_ALIGN_))
#define ALIGN( _VALUE_, _ALIGN_ )           ALIGN_CIELING( _VALUE_, _ALIGN_ )
#define ALIGN_AT_OFFSET( _VALUE_, _ALIGN_, _OFFSET_ ) (((SIZE_T)(_VALUE_) <= (ALIGN_FLOOR(_VALUE_,_ALIGN_)+(_OFFSET_)))? (ALIGN_FLOOR(_VALUE_,_ALIGN_)+(_OFFSET_)): (ALIGN(_VALUE_,_ALIGN_)+(_OFFSET_)))

#define MBB_BAD_POINTER ((PVOID)0x5A5A5A5A)

#define MBB_UTIL_INITIALIZE_NDIS_STATUS_INDICATION( NSI, MINIPORTADAPTERHANDLE, STATUS ) \
{ \
    (NSI)->Header.Type        = NDIS_OBJECT_TYPE_STATUS_INDICATION; \
    (NSI)->Header.Revision    = NDIS_STATUS_INDICATION_REVISION_1; \
    (NSI)->Header.Size        = NDIS_SIZEOF_STATUS_INDICATION_REVISION_1; \
    (NSI)->SourceHandle       = (MINIPORTADAPTERHANDLE); \
    (NSI)->PortNumber         = 0; \
    (NSI)->StatusCode         = (STATUS); \
    (NSI)->Flags              = 0; \
    (NSI)->DestinationHandle  = NULL; \
    (NSI)->RequestId          = 0; \
    RtlZeroMemory( &(NSI)->Guid, sizeof((NSI)->Guid) ); \
}

#define MBB_INITIALIZE_REQUEST_NDIS_STATUS_INDICATION( NSI, REQ, STATUS ) \
{ \
    MBB_UTIL_INITIALIZE_NDIS_STATUS_INDICATION(NSI, (REQ)->RequestManager->AdapterContext->MiniportAdapterHandle, STATUS) \
    (NSI)->DestinationHandle  = (REQ)->OidContext.OidRequestHandle; \
    (NSI)->RequestId          = (REQ)->OidContext.OidRequestId; \
}

#define MbbUtilPopulateStaticString(_StaticString, _ContainingStruct, _MbbString) \
{                                                                                 \
    ASSERT(sizeof(_StaticString) > _ContainingStruct->_MbbString.Size);           \
                                                                                  \
    RtlZeroMemory(                                                                \
        &_StaticString[0],                                                        \
        sizeof(_StaticString)                                                     \
        );                                                                        \
                                                                                  \
    RtlCopyMemory(                                                                \
        &_StaticString[0],                                                        \
        ((PUCHAR)_ContainingStruct) + _ContainingStruct->_MbbString.Offset,       \
        _ContainingStruct->_MbbString.Size                                        \
        );                                                                        \
}

#define MBB_INIT_MBB_STRING(_X_) \
{ \
    (_X_)->Size   = 0; \
    (_X_)->Offset = 0; \
}


#define MBB_INVALID_SESSION_ID -1
#define MBB_INVALID_PORT_NUMBER -1

#define MBB_NO_MATCH_WOL_PATTERN_ID 0
#define MBB_NO_MATCH_WOL_PATTERN_SIZE 8 // 8 bytes - 0x00000000


////////////////////////////////////////////////////////////////////////////////
//
//  DECLARATIONS
//
////////////////////////////////////////////////////////////////////////////////
PCSTR
GetOidName(
    __in NDIS_OID Oid
    );

BOOLEAN
IsSameGuid(GUID * pgGuid1, GUID *pgGuid2);

BOOLEAN
IsZeroGuid(
    __in    GUID*   pGuid
    );

NDIS_STATUS
GetDeviceFriendlyName(
    __in NDIS_HANDLE          MiniportAdapterHandle,
    __out PANSI_STRING        AnsiName
    );

VOID
FreeAdapterBlock(
    _In_ __drv_freesMem(Mem) PMINIPORT_ADAPTER_CONTEXT Adapter
    );

PCHAR
MbbUtilGetCommandString(
    __in PMBB_COMMAND Command
    );

BOOLEAN
MbbUtilIsBasicCommand(
    __in PMBB_COMMAND Command
    );

BOOLEAN
MbbUtilIsValidDeviceService(
    __in GUID*       DeviceServiceGuid
    );

NDIS_STATUS
MbbUtilSendMessageFragmentsAndLog(
    __in PMBB_REQUEST_CONTEXT Request
    );

NDIS_STATUS
MbbUtilSendMessageFragments(
    __in PMBB_REQUEST_CONTEXT Request
    );

NDIS_STATUS
MbbUtilSetupCommandMessage(
    __in    PMBB_REQUEST_CONTEXT                Request,
    __in    PMBB_COMMAND                        Command,
    __in    MBB_COMMAND_TYPE                    CommandType,
    __in_bcount_opt(InputBufferLength) PUCHAR   InputBuffer,
    __in    ULONG                               InputBufferLength
    );

NDIS_STATUS
MbbUtilInternalCIDQuery(
    __in MBB_PROTOCOL_HANDLE                AdapterHandle,
    __in PMBB_REQUEST_CONTEXT               Request
    );

VOID
MbbUtilInternalCIDCompletion(
    __in    MBB_PROTOCOL_HANDLE     AdapterHandle,
    __in    PMBB_REQUEST_CONTEXT    Request,
    __in    NDIS_STATUS             NdisStatus
    );

VOID
MbbUtilInternalCIDResponse(
    __in PMBB_REQUEST_CONTEXT               Request,
    __in NDIS_STATUS                        NdisStatus,
    __in MBB_STATUS                         MbbStatus,
    __in_bcount_opt(InBufferSize) PUCHAR    InBuffer,
    __in ULONG                              InBufferSize
    );

VOID
MbbUtilCommonCIDResponse(
    __in PMBB_REQUEST_CONTEXT               Request,
    __in NDIS_STATUS                        NdisStatus,
    __in MBB_STATUS                         MbbStatus,
    __in_bcount_opt(InBufferSize) PUCHAR    InBuffer,
    __in ULONG                              InBufferSize,
    __in_opt PMBB_OID_HANDLER_ENTRY         OidHandler
    );

NDIS_STATUS
MbbUtilQueryAttribute(
    __in PMBB_REQUEST_CONTEXT                                   Request,
    __in_bcount_opt(*InBufferSize) PUCHAR                       InBuffer,
    __in PULONG                                                 InBufferSize,
    __out_bcount_part_opt(*OutBufferSize,*OutBufferSize) PUCHAR OutBuffer,
    __inout PULONG                                              OutBufferSize
    );

NDIS_STATUS
MbbUtilQueryAttributeWithParameter(
    __in PMBB_REQUEST_CONTEXT                           Request,
    __in_bcount_opt(GetRequestParametersSize) PUCHAR    GetRequestParameters,
    __in ULONG                                          GetRequestParametersSize
    );

NDIS_STATUS
MbbUtilSetAttributeWithParameter(
    __in PMBB_REQUEST_CONTEXT                   Request,
    __in_bcount_opt(SetParametersSize) PUCHAR   SetParameters,
    __in ULONG                                  SetParametersSize
    );

NDIS_STATUS
MbbUtilDeviceServiceCommand(
    __in PMBB_REQUEST_CONTEXT                   Request,
    __in_bcount_opt(*InBufferSize) PUCHAR       InBuffer,
    __in PULONG                                 InBufferSize
    );

NDIS_STATUS
MbbUtilUiccAccessBinary(
    __in PMBB_REQUEST_CONTEXT                   Request,
    __in_bcount_opt(*InBufferSize) PUCHAR       InBuffer,
    __in PULONG                                 InBufferSize
    );

NDIS_STATUS
MbbUtilUiccAccessRecord(
    __in PMBB_REQUEST_CONTEXT                   Request,
    __in_bcount_opt(*InBufferSize) PUCHAR       InBuffer,
    __in PULONG                                 InBufferSize
    );

PCSTR
MbbUtilGetWwanIndicationString(
    __in NDIS_STATUS Indication
    );

//
// Allocator
//

__drv_allocatesMem(Mem)
__drv_aliasesMem
PVOID
MbbAllocMgrInitialize(
    __in    ULONG           AllocationSize,
    __in    ULONG           AllocationCount,
    __in    ULONG           PoolTag,
    __in    MBB_ALLOC_FLAGS Flags
    );

VOID
MbbAllocMgrCleanup(
    __in __drv_freesMem(Mem) PVOID  AllocMgrHandle
    );

PVOID
MbbAllocMgrAllocate(
    __in    PVOID   AllocMgrHandle
    );

VOID
MbbAllocMgrFree(
    __in    PVOID   AllocHandle
    );

//
// Buffer Manager
//

__drv_allocatesMem(Mem)
__drv_aliasesMem
PVOID
MbbBufMgrInitialize(
    __in  ULONG  BufferCount,
    __in  ULONG  BufferLength
    );

VOID
MbbBufMgrCleanup(
    __in __drv_freesMem(Mem) PVOID ManagerHandle
    );

//
// Work Item
//

__drv_allocatesMem(Mem)
__drv_aliasesMem
PVOID
MbbWorkMgrInitialize(
    __in ULONG  InitialWorkItemCount
    );

VOID
MbbWorkMgrCleanup(
    __in __drv_freesMem(Mem) PVOID  WorkItemManagerHandle
    );

NDIS_STATUS
MbbWorkMgrQueueWorkItem(
    __in     PVOID                  WorkItemManagerHandle,
    __in_opt PVOID                  Context1,
    __in_opt PVOID                  Context2,
    __in_opt PVOID                  Context3,
    __in_opt PVOID                  Context4,
    __in     MBB_WORKITEM_ROUTINE   Routine
    );

//
// Wwan to Mbb
//


VOID
MbbUtilWwanToMbbRadioState(
    __in  WWAN_RADIO*       WwanRadio,
    __out MBB_RADIO_STATE*  MbbRadioState
    );

NTSTATUS
MbbUtilWwanToMbbPinAction(
    __in  PWWAN_PIN_ACTION  WwanPinAction,
    __out PMBB_PIN_ACTION  * MbbPinActionOut,
    __out PULONG            ReturnedBufferSize
    );

NTSTATUS
MbbUtilWwanToMbbPinActionEx2(
    __in  PWWAN_PIN_ACTION_EX2  WwanPinAction,
    __out PMBB_PIN_ACTION_EX2  *MbbPinActionOut,
    __out PULONG                ReturnedBufferSize
    );

ULONG
MbbUtilWwanToMbbProvider(
    __in  PWWAN_PROVIDER                        WwanProvider,
    __in  MBB_CELLULAR_CLASS                    MbbCellularClass,
    __in  ULONG                                 Rssi,
    __in  ULONG                                 ErrorRate,
    __in  ULONG                                 MbbProviderSize,
    __out_bcount(MbbProviderSize) PMBB_PROVIDER MbbProvider
    );

ULONG
MbbUtilWwanToMbbProvider2(
    __in  PWWAN_PROVIDER2                       WwanProvider2,
    __in ULONG                                  MbbProviderSize,
    __out_bcount(MbbProviderSize) PMBB_PROVIDER MbbProvider
    );

NDIS_STATUS
MbbUtilWwanToMbbProviderList(
    __in  PWWAN_LIST_HEADER                                     WwanProviderList,
    __in  BOOLEAN                                               IsMultiCarrierCapable,
    __in_opt MBB_CELLULAR_CLASS                                 MbbCellularClass,
    __out_bcount(*MbbProviderListSize) PMBB_PROVIDER_LIST       MbbProviderList,
    __inout PULONG                                              MbbProviderListSize
    );

NTSTATUS
MbbUtilWwanToMbbSetRegisterState(
    __in  PWWAN_SET_REGISTER_STATE  WwanSetRegisterState,
    __out PMBB_SET_REGISTER_STATE  *MbbSetRegisterStateOut,
    __out PULONG                     ReturnedBufferSize
    );


NDIS_STATUS
MbbUtilWwanToMbbSetPacketService(
    __in  WWAN_PACKET_SERVICE_ACTION PacketServiceAction,
    __out PMBB_SET_PACKET_SERVICE    MbbSetPacketService
    );

VOID
MbbUtilAdjustSignalStateConfigurationForDevice(
    __in     PMINIPORT_ADAPTER_CONTEXT     Adapter,
    __inout  PWWAN_SET_SIGNAL_INDICATION   WwanSetSignalIndication
    );

VOID
MbbUtilWwanToMbbSetSignalStateIndication(
    __in  PWWAN_SET_SIGNAL_INDICATION   WwanSetSignalIndication,
    __out PMBB_SET_SIGNAL_INDICATION    MbbSetSignalStateIndication
    );

VOID
MbbUtilWwanToMbbSetContextState(
    __in  PWWAN_SET_CONTEXT_STATE   WwanSetContextState,
    __in  ULONG                     AccessStringCchLength,
    __in  ULONG                     UserNameCchLength,
    __in  ULONG                     PasswordCchLength,
    __in  ULONG                     MbbSetContextStateSize,
    __in  ULONG                     SessionId,
    __out PMBB_SET_CONTEXT_STATE    MbbSetContextState
    );

NTSTATUS
MbbUtilWwanToMbbSetLteAttachContext(
    __in  PWWAN_SET_LTE_ATTACH                   WwanSetContextBuffer,
    __out PMBB_SET_MS_CONTEXT_LTE_ATTACH_CONFIG *ReturnedMbbSetContext,
    __out ULONG                                 *ReturnedBufferSize
    );

VOID
MbbUtilMbbToWwanLteAttachConfigInfo(
    __in  MBB_MS_LTE_ATTACH_CONFIG_INFO* MbbLteAttachContextList,
    __out PWWAN_LTE_ATTACH_CONFIG        WwanLteAttachContext
    );

VOID
MbbUtilMbbToWwanLteAttachContext(
    __in  PMBB_MS_LTE_ATTACH_CONTEXT MbbLteAttachContext,
    __out WWAN_CONTEXT_V2*           WwanLteAttachContext
    );

VOID
MbbUtilMbbToWwanLteAttachStatus(
    __in  MBB_MS_LTE_ATTACH_STATUS* MbbLteAttachStatus,
    __out PWWAN_LTE_ATTACH_STATUS   WwanLteAttachStatus
    );

NTSTATUS
MbbUtilWwanToMbbSetContext(
    __in  PWWAN_SET_CONTEXT     WwanSetContext,
    __out PMBB_SET_CONTEXT     *ReturnedMbbSetContext,
    __out ULONG                *ReturnedBufferSize
    );

NTSTATUS
MbbUtilWwanToMbbSetContextV2(
    __in PWWAN_SET_PROVISIONED_CONTEXT_V2               WwanSetContext,
    __out PMBB_SET_MS_CONTEXT_PROVISIONED_CONTEXT_V2    *ReturnedMbbSetContext,
    __out ULONG                                         *ReturnedBufferSize
);

NTSTATUS
MbbUtilWwanToMbbSmsSetConfiguration(
    __in  PWWAN_SET_SMS_CONFIGURATION    WwanSmsSetConfiguration,
    __in  ULONG                          ScAddressSize,
    __out PMBB_SET_SMS_CONFIGURATION    *MbbSmsSetConfigurationReturn,
    __out PULONG                         ReturnedBufferSize
    );


VOID
MbbUtilWwanToMbbSmsRead(
    __in  PWWAN_SMS_READ    WwanSmsRead,
    __out PMBB_SMS_READ     MbbSmsRead
    );
#if 0
NDIS_STATUS
MbbUtilWwanToMbbSmsSend(
    __in  PWWAN_SMS_SEND                        WwanSmsSend,
    __in  MBB_CELLULAR_CLASS                    CellularClass,
    __out_bcount(MbbSmsSendSize) PMBB_SMS_SEND  MbbSmsSend,
    __in  ULONG                                 MbbSmsSendSize
    );
#endif

MBB_SMS_FORMAT
MbbUtilWwanToMbbSmsFormat(
    __in  WWAN_SMS_FORMAT   WwanSmsFormat
    );


NDIS_STATUS
MbbUtilWwanToMbbSmsSendPdu(
    __in  PWWAN_SMS_SEND_PDU                            WwanSmsSendPdu,
    __in  MBB_CELLULAR_CLASS                            CellularClass,
    __out PMBB_SMS_SEND                                *ReturnedMbbSmsSend,
    __out PULONG                                        MbbSmsSendSize
    );

NDIS_STATUS
MbbUtilWwanToMbbSmsSendCdma(
    __in  PWWAN_SMS_SEND_CDMA                               WwanSmsSendCdma,
    __out PMBB_SMS_SEND                                    *ReturnedMbbSmsSend,
    __out PULONG                                            MbbSmsSendSize
    );



VOID
MbbUtilWwanToMbbSmsDelete(
    __in  PWWAN_SMS_FILTER  WwanSmsFilter,
    __out PMBB_SMS_DELETE   MbbSmsDelete
    );

NDIS_STATUS
MbbUtilWwanToMbbSubscribeEvents(
    __in PMINIPORT_ADAPTER_CONTEXT      Adapter,
    __in_ecount(GuidCount) GUID*        GuidList,
    __in ULONG                          GuidCount,
    __in BOOL                           ExternalRequest,
    __in_bcount(ExtSubscribeListSize) PMBB_SUBSCRIBE_EVENT_LIST ExtSubscribeList,
    __in ULONG                          ExtSubscribeListSize,
    __out PMBB_SUBSCRIBE_EVENT_LIST*    OutputSubscribeList,
    __out PULONG                        OutputSubscribeBufferSize
    );

NDIS_STATUS
MbbUtilGenerateSubscribeEventList(
    __in PMINIPORT_ADAPTER_CONTEXT      Adapter,
    __in BOOLEAN                        FullPower,
    __in ULONG                          MediaSpecificWakeUpEvents,
    __in ULONG                          WakeUpFlags,
    __in_bcount(ExtSubscribeListSize) PMBB_SUBSCRIBE_EVENT_LIST ExtSubscribeList,
    __in ULONG                          ExtSubscribeListSize,
    __out PMBB_SUBSCRIBE_EVENT_LIST*    OutputSubscribeList,
    __out PULONG                        OutputSubscribeListSize
    );

NDIS_STATUS
MbbUtilWwanToMbbAkapAuthChallenge(
    __in  PWWAN_AUTH_AKAP_CHALLENGE                 WwanAuthAkapChallenge,
    __in  ULONG                                     UTF16ByteCount,
    __in  ULONG                                     MbbSize,
    __out_bcount(MbbSize) PMBB_AKAP_AUTH_CHALLENGE  MbbAkapAuthChallenge
    );

NDIS_STATUS
MbbUtilWwanToMbbSetPacketFilter(
    __in  PMINIPORT_ADAPTER_CONTEXT Adapter,
    __in  BOOLEAN                   Set,
    __in  NDIS_PORT_NUMBER          PortNumber,
    __out PMBB_PACKET_FILTERS      *ReturnedMbbPacketFilters,
    __out ULONG                    *ReturnedBufferSize
    );

NDIS_STATUS
MbbUtilWwanToMbbUiccFilePath(
    __in        PWWAN_UICC_FILE_PATH  WwanUiccFilePath,
    __deref_out PMBB_UICC_FILE_PATH   *MbbUiccFilePathOut,
    __out       PULONG                ReturnedBufferSize
    );

NDIS_STATUS
MbbUtilWwanToMbbUiccAccessBinary(
    __in            PWWAN_UICC_ACCESS_BINARY  WwanUiccAccessBinary,
    __deref_out_opt PMBB_UICC_ACCESS_BINARY   *MbbUiccAccessBinaryOut,
    __out           PULONG                    ReturnedBufferSize
    );

NDIS_STATUS
MbbUtilWwanToMbbUiccAccessRecord(
    __in            PWWAN_UICC_ACCESS_RECORD  WwanUiccAccessRecord,
    __deref_out_opt PMBB_UICC_ACCESS_RECORD   *MbbUiccAccessRecordOut,
    __out           PULONG                    ReturnedBufferSize
    );

NDIS_STATUS
MbbUtilWwanToMbbPinApp(
    __in        PWWAN_PIN_APP         WwanPinApp,
    __deref_out PMBB_PIN_APP          *MbbPinAppOut,
    __out       PULONG                ReturnedBufferSize
    );

NDIS_STATUS
MbbUtilWwanToMbbSetDeviceSlotMappingInfo(
    __in            PWWAN_DEVICE_SLOT_MAPPING_INFO    WwanDeviceSlotMappingInfo,
    __deref_out_opt PMBB_MS_DEVICE_SLOT_MAPPING_INFO  *MbbDeviceSlotMappingInfoOut,
    __out           PULONG                            ReturnedBufferSize
    );

ULONG 
MbbUtilWwanListToMbbList(
    __in    PWWAN_LIST_HEADER  WwanListHeader,
    __in    ULONG              WwanListElementSize,
    __inout PUCHAR             MbbBuffer,
    __in    ULONG              CurrentOffsetInMbbBuffer,
    __inout PMBB_ARRAY_ELEMENT MbbArray
    );

NDIS_STATUS
MbbUtilWwanToMbbSetDeviceBindingsInfo(
    __in        PWWAN_DEVICE_BINDINGS_INFO    WwanSetDeviceBindingsInfo,
    __deref_out PMBB_DEVICE_BINDINGS_INFO     *MbbSetDeviceBindingsInfoOut,
    __out       PULONG                        ReturnedBufferSize
    );

NDIS_STATUS
MbbUtilWwanToMbbSetRegisterStateEx(
    __in        PWWAN_SET_REGISTER_STATE_EX   WwanSetRegisterStateEx,
    __deref_out PMBB_SET_REGISTER_STATE_V2    *MbbSetRegisterStateV2Out,
    __out       PULONG                        ReturnedBufferSize
    );

NDIS_STATUS
MbbUtilWwanToMbbSetUiccOpenChannel(
    __in        PWWAN_SET_UICC_OPEN_CHANNEL WwanSetUiccOpenChannel,
    __deref_out PMBB_SET_UICC_OPEN_CHANNEL  *MbbSetUiccOpenChannelOut,
    __out       PULONG                      ReturnedBufferSize
    );

VOID
MbbUtilWwanToMbbSetUiccCloseChannel(
    __in  PWWAN_SET_UICC_CLOSE_CHANNEL  WwanSetUiccCloseChannel,
    __out PMBB_SET_UICC_CLOSE_CHANNEL   MbbSetUiccCloseChannel
    );

NDIS_STATUS
MbbUtilWwanToMbbSetUiccApdu(
    __in        PWWAN_SET_UICC_APDU WwanSetUiccApdu,
    __deref_out PMBB_SET_UICC_APDU  *MbbSetUiccApduOut,
    __out       PULONG              ReturnedBufferSize
    );

NDIS_STATUS
MbbUtilWwanToMbbSetUiccTerminalCapability(
    __in        PWWAN_LIST_HEADER                   WwanCapabilityListHeader,
    __deref_out PMBB_SET_UICC_TERMINAL_CAPABILITY   *MbbSetUiccTerminalCapabilityOut,
    __out       PULONG                              ReturnedBufferSize
    );

NDIS_STATUS
MbbUtilWwanToMbbSetSarConfig(
    _In_        PWWAN_SET_SAR_CONFIG   WwanSetSarConfig,
    _Outptr_    PMBB_SET_SAR_CONFIG    *MbbSetSarConfigOut,
    _Out_       PULONG                 ReturnedBufferSize
    );

NDIS_STATUS
MbbUtilWwanToMbbSetNetworkBlacklist(
    _In_        PWWAN_NETWORK_BLACKLIST_INFO        SetNetworkBlacklist,
    _Outptr_    PMBB_MS_NETWORK_BLACKLIST_INFO      *MbbNetworkBlacklistOut,
    _Out_       PULONG                              ReturnedBufferSize
    );

//
// Mbb to Wwan
//

WWAN_STATUS
MbbUtilMbbToWwanStatus(
    __in MBB_STATUS MbbStatus
    );

VOID
MbbUtilMbbToWwanDeviceCaps(
    __in  PMBB_DEVICE_CAPS                  MbbDeviceCaps,
    __in_ecount(ManufacturerStrLen) PWCHAR  ManufacturerString,
    __in  ULONG                             ManufacturerStrLen,
    __in_ecount(ModelStrLen) PWCHAR         ModelString,
    __in  ULONG                             ModelStrLen,
    __out PWWAN_DEVICE_CAPS                 WwanDeviceCaps,
    __in  BOOL                              fMultimode
    );

VOID
MbbUtilMbbToWwanDeviceCapsV2(
    __in  PMBB_MS_DEVICE_CAPS_INFO_V2       MbbDeviceCaps,
    __in_ecount(ManufacturerStrLen) PWCHAR  ManufacturerString,
    __in  ULONG                             ManufacturerStrLen,
    __in_ecount(ModelStrLen) PWCHAR         ModelString,
    __in  ULONG                             ModelStrLen,
    __out PWWAN_DEVICE_CAPS_EX              WwanDeviceCaps,
    __in  BOOL                              fMultimode
);

VOID
MbbUtilMbbToWwanRadioState(
    __in  MBB_RADIO_STATE*  MbbRadioState,
    __out WWAN_RADIO*       WwanRadio
    );

VOID
MbbUtilMbbToWwanReadyInfo(
    __in PMBB_SUBSCRIBER_READY_INFO  MbbReadyInfo,
    __in PWWAN_READY_INFO WwanReadyInfo
    );

VOID
MbbUtilMbbToWwanPinInfo(
    __in PMBB_PIN_INFO   MbbPinInfo,
    __out PWWAN_PIN_INFO WwanPinInfo
    );

VOID
MbbUtilMbbToWwanPinList(
    __in  PMBB_PIN_LIST     MbbPinList,
    __out PWWAN_PIN_LIST    WwanPinList
    );

VOID
MbbUtilMbbToWwanProvider(
    __in  PMBB_PROVIDER     MbbProvider,
    __in  ULONG             MbbDataClass,
    __out PWWAN_PROVIDER    WwanProvider
    );

VOID
MbbUtilMbbToWwanProvider2(
    __in  PMBB_PROVIDER     MbbProvider,
    __in  ULONG             MbbDataClass,
    __out PWWAN_PROVIDER2   WwanProvider2
    );

VOID
MbbUtilMbbToWwanProviderList(
    __in  PMBB_PROVIDER_LIST    MbbProviderList,
    __in  ULONG                 MbbDataClass,
    __in  BOOLEAN               IsMultiCarrier,
    __out PWWAN_LIST_HEADER     WwanProviderList
    );

VOID
MbbUtilMbbToWwanRegistrationState(
    __in  PMBB_REGISTRATION_STATE   MbbRegistrationState,
    __out PWWAN_REGISTRATION_STATE  WwanRegistrationState
    );

VOID
MbbUtilMbbToWwanPacketService(
    __in  PMBB_PACKET_SERVICE   MbbPacketService,
    __out PWWAN_PACKET_SERVICE  WwanPacketService
    );

VOID
MbbUtilMbbToWwanSignalState(
    __in  PMBB_SIGNAL_STATE     MbbSignalState,
    __out PWWAN_SIGNAL_STATE    WwanSignalState
    );

VOID
MbbUtilMbbToWwanContextState(
    __in  PMBB_CONTEXT_STATE     MbbContextState,
    __in  ULONG                  ConnectId,
    __out PWWAN_CONTEXT_STATE    WwanContextState
    );

NDIS_STATUS
MbbUtilPopulateWwanIPAddressState(
    __in PMBB_IP_ADDRESS_INFO   IPAddressInfo,
    __in PMBB_IPADDRESS_ENTRY   IpTable,
    __in PMBB_IPADDRESS_ENTRY   GatewayTable,
    __in PMBB_IPADDRESS_ENTRY   DnsTable,
    __in ULONG                  IpCount,
    __in ULONG                  GatewayCount,
    __in ULONG                  DnsCount,
    __out PWWAN_IP_ADDRESS_STATE pWwanIPAddressState
    );

VOID
MbbUtilMbbToWwanContext(
    __in  PMBB_CONTEXT  MbbContext,
    __out PWWAN_CONTEXT WwanContext
    );

VOID
MbbUtilMbbToWwanContextList(
    __in  PMBB_CONTEXT_LIST MbbContextList,
    __out PWWAN_LIST_HEADER WwanContextList
    );

VOID
MbbUtilMbbToWwanContextV2(
    __in  PMBB_MS_CONTEXT_V2    MbbContext,
    __out PWWAN_CONTEXT_V2      WwanContext
    );

VOID
MbbUtilMbbToWwanContextV2List(
    __in  PMBB_CONTEXT_LIST MbbContextList,
    __out PWWAN_LIST_HEADER WwanContextList
    );

VOID
MbbUtilMbbToWwanServiceActivationStatus(
    __in  PMBB_SERVICE_ACTIVATION_STATUS    MbbServiceActivationStatus,
    __in  ULONG                             VendorSpecificBufferSize,
    __out PWWAN_SERVICE_ACTIVATION_STATUS   WwanServiceActivationStatus
    );

VOID
MbbUtilMbbToWwanSmsConfiguration(
    __in  PMBB_SMS_CONFIGURATION    MbbSmsConfiguration,
    __out PWWAN_SMS_CONFIGURATION   WwanSmsConfiguration
    );

NDIS_STATUS
MbbUtilValidateMbbSmsPduRecord(
    __in MBB_CELLULAR_CLASS                                 CellularClass,
    __in ULONG                                              MbbSmsPduRecordSize,
    __in_bcount(MbbSmsPduRecordSize) PMBB_SMS_PDU_RECORD    MbbSmsPduRecord
    );

NDIS_STATUS
MbbUtilValidateMbbSmsCdmaRecord(
    __in ULONG                                              MbbSmsCdmaRecordSize,
    __in_bcount(MbbSmsCdmaRecordSize) PMBB_SMS_CDMA_RECORD  MbbSmsCdmaRecord
    );

NDIS_STATUS
MbbUtilValidateMbbSmsReceive(
    __in MBB_CELLULAR_CLASS                         CellularClass,
    __in ULONG                                      MbbSmsReceiveSize,
    __in_bcount(MbbSmsReceiveSize) PMBB_SMS_RECEIVE MbbSmsReceive
    );

VOID
MbbUtilMbbToWwanSmsPduRecord(
    __in  PMBB_SMS_PDU_RECORD   MbbSmsPduRecord,
    __in  MBB_CELLULAR_CLASS    CellularClass,
    __out PWWAN_SMS_PDU_RECORD  WwanSmsPduRecord
    );

VOID
MbbUtilMbbToWwanSmsCdmaRecord(
    __in  PMBB_SMS_CDMA_RECORD  MbbSmsCdmaRecord,
    __out PWWAN_SMS_CDMA_RECORD WwanSmsCdmaRecord
    );

NDIS_STATUS
MbbUtilMbbToWwanSmsReceive(
    __in_bcount(MbbSmsReceiveSize) PMBB_SMS_RECEIVE MbbSmsReceive,
    __in  ULONG                                     MbbSmsReceiveSize,
    __in  MBB_CELLULAR_CLASS                        CellularClass,
    __out PWWAN_LIST_HEADER                         SmsListHeader
    );

VOID
MbbUtilMbbToWwanSmsStatus(
    __in  PMBB_SMS_STATUS   MbbSmsStatus,
    __out PWWAN_SMS_STATUS  WwanSmsStatus
    );

VOID
MbbUtilMbbToWwanUiccAppList(
    __in PMBB_UICC_APP_LIST      MbbUiccAppList,
    __out PWWAN_UICC_APP_LIST    WwanUiccAppList
    );

VOID
MbbUtilMbbToWwanUiccFileStatus(
    __in PMBB_UICC_FILE_STATUS      MbbUiccFileStatus,
    __out PWWAN_UICC_FILE_STATUS    WwanUiccFileStatus
    );

VOID
MbbUtilMbbToWwanUiccResponse(
    __in PMBB_UICC_RESPONSE     MbbUiccResponse,
    __out PWWAN_UICC_RESPONSE   WwanUiccResponse
    );

VOID
MbbUtilMbbToWwanSysCapsInfo(
    __in PMBB_MS_SYS_CAPS_INFO  MbbSysCapsInfo,
    __out PWWAN_SYS_CAPS_INFO   WwanSysCapsInfo
    );

VOID
MbbUtilMbbToWwanDeviceSlotMappingInfo(
    __in  PMBB_MS_DEVICE_SLOT_MAPPING_INFO  MbbDeviceSlotMappingInfo,
    __out PWWAN_DEVICE_SLOT_MAPPING_INFO    WwanDeviceSlotMappingInfo
    );

VOID
MbbUtilMbbToWwanSlotInfo(
    __in  PMBB_MS_SLOT_INFO     MbbSlotInfo,
    __out PWWAN_SLOT_INFO       WwanSlotInfo
    );

VOID
MbbUtilMbbToWwanDeviceBindingsInfo(
    __in  PMBB_DEVICE_BINDINGS_INFO   MbbDeviceBindingsInfo,
    __out PWWAN_DEVICE_BINDINGS_INFO  WwanDeviceBindingsInfo
    );

VOID
MbbUtilMbbToWwanRegistrationStateInfoV2(
    __in  PMBB_REGISTRATION_STATE_INFOS_V2  MbbRegistrationStateInfosV2,
    __out PWWAN_REGISTRATION_STATE_INFOS    WwanRegistrationStateInfos
    );

VOID
MbbUtilMbbToWwanSignalStateInfoV2(
    __in  PMBB_SIGNAL_STATE_INFOS_V2    MbbSignalStateInfosV2,
    __out PWWAN_SIGNAL_STATE_INFOS      WwanSignalStateInfos
    );

VOID
MbbUtilMbbToWwanAtrInfo(
    __in PMBB_ATR_INFO      MbbAtrInfo,
    __out PWWAN_ATR_INFO    WwanAtrInfo
    );

VOID
MbbUtilMbbToWwanUiccOpenChannelInfo(
    __in PMBB_UICC_OPEN_CHANNEL_INFO    MbbUiccOpenChannelInfo,
    __out PWWAN_UICC_OPEN_CHANNEL_INFO  WwanUiccOpenChannelInfo
    );

VOID
MbbUtilMbbToWwanUiccCloseChannelInfo(
    __in PMBB_UICC_CLOSE_CHANNEL_INFO   MbbUiccCloseChannelInfo,
    __out PWWAN_UICC_CLOSE_CHANNEL_INFO WwanUiccCloseChannelInfo
    );

VOID
MbbUtilMbbToWwanUiccApduInfo(
    __in PMBB_UICC_APDU_INFO    MbbUiccApduInfo,
    __out PWWAN_UICC_APDU_INFO  WwanUiccApduInfo
    );

VOID
MbbUtilMbbToWwanUiccTerminalCapabilityInfo(
    __in PMBB_UICC_TERMINAL_CAPABILITY_INFO     MbbUiccTerminalCapabilityInfo,
    __out PWWAN_LIST_HEADER                     WwanCapabilityListHeader
    );

VOID
MbbUtilMbbToWwanSarConfigInfo(
    _In_ PMBB_SAR_CONFIG_INFO MbbSarConfigInfo,
    _Out_ PWWAN_SAR_CONFIG_INFO WwanSarConfigInfo
    );

VOID
MbbUtilMbbToWwanTransmissionStatusInfo(
    _In_ PMBB_SAR_TRANSMISSION_STATUS_INFO MbbTransmissionStatusInfo,
    _Out_ PWWAN_SAR_TRANSMISSION_STATUS_INFO WwanTransmissionStatusInfo
    );

//
// Drain Object
//
VOID
InitDrainObject(
    PDRAIN_OBJECT               DrainObject,
    MBB_DRAIN_COMPLETE          DrainCompleteCallback,
    PVOID                       Context
    );

VOID
StartDrain(
    PDRAIN_OBJECT               DrainObject
    );

VOID
DrainComplete(
    PDRAIN_OBJECT               DrainObject
    );

BOOLEAN
DrainAddRef(
    PDRAIN_OBJECT               DrainObject
    );

VOID
DrainRelease(
    PDRAIN_OBJECT               DrainObject
    );

NTSTATUS
MbbEventActivityIdCtl (
   _Inout_  LPGUID ActivityId
    );


VOID
MbbWriteEvent (
    IN PCEVENT_DESCRIPTOR       EventDescriptor,
    IN LPGUID                   ActivityId   OPTIONAL,
    IN LPGUID                   RelatedActivityId OPTIONAL,
    IN USHORT                   ArgCount,
    ...
    );

VOID
MbbWriteEventOpn (
    IN PCEVENT_DESCRIPTOR       EventDescriptor,
    IN LPGUID                   ActivityId   OPTIONAL,
    IN LPGUID                   RelatedActivityId OPTIONAL,
    IN USHORT                   ArgCount,
    ...
    );

VOID
MbbWriteEventCommon (
    IN REGHANDLE                TraceHandle,
    IN PCEVENT_DESCRIPTOR       EventDescriptor,
    IN LPGUID                   ActivityId   OPTIONAL,
    IN LPGUID                   RelatedActivityId OPTIONAL,
    IN USHORT                   ArgCount,
    IN va_list                  Arguments
    );

NDIS_STATUS
MbbUtilWwanToMbbSetUSSD(
    __out  PMBB_SET_USSD         MbbSetUSSD,
    __in   PWWAN_USSD_REQUEST    WWanUSSDRequest
    );

NDIS_STATUS
MbbUtilMbbToWwanUSSDEvent(
    __in  PMBB_USSD             MbbUSSD,
    __out PWWAN_USSD_EVENT      WWanUSSDEvt
    );

VOID
MbbUtilMbbToWwanNetworkBlackList(
    __in  PMBB_MS_NETWORK_BLACKLIST_INFO MbbNetworkBlacklist,
    __out PWWAN_LIST_HEADER BlacklistProviderList
    );

//
// Validation routines
//

NTSTATUS
MbbIsVariableFieldValid(
    ULONG   TotalLength,
    ULONG   Offset,
    ULONG   Size,
    ULONG   MaxLength,
    ULONG   ElementSize
    );

NTSTATUS
MbbIsArrayFieldValid(
    ULONG   TotalMessageLength,
    ULONG   Offset,
    ULONG   ElementCount,
    ULONG   ElementSize
    );


NDIS_STATUS
MbbUtilValidateMbbProvider(
    __in ULONG                                  MbbProviderSize,
    __in_bcount(MbbProviderSize) PMBB_PROVIDER  MbbProvider
    );

NDIS_STATUS
MbbUtilValidateMbbProviderList(
    __in ULONG                                          MbbProviderListSize,
    __in_bcount(MbbProviderListSize) PMBB_PROVIDER_LIST MbbProviderList
    );

ULONG
MbbUtilWriteStringToBuffer(
    __in_ecount(BufferSize) PUCHAR      Start,
    __in ULONG       BufferSize,
    __in ULONG       CurrentOffset,
    __in PMBB_STRING StringDesc,
    __in_ecount(SourceLength) PCUCHAR      SourceString,
    __in ULONG       SourceLength
    );

BOOLEAN
MbbUtilIsNativeMbnService(
    __in GUID*       DeviceServiceGuid
    );

PMBB_DS
MbbUtilFindDeviceService(
    __in PMINIPORT_ADAPTER_CONTEXT Adapter,
    __in GUID*       DeviceServiceGuid
    );

//
// Hex string routines
//
NDIS_STATUS
MbbUtilHexStringToUchar(
    __in_ecount(2)  PCHAR   HexString,
    __out           PUCHAR  Value
    );

NDIS_STATUS
MbbUtilHexStringToByteStream(
    __in_bcount_z(HexStringSize) PCHAR  HexString,
    __in ULONG                          HexStringSize,
    __out_bcount(ByteStreamSize) PUCHAR ByteStream,
    __in ULONG                          ByteStreamSize
    );

VOID
MbbUtilByteStreamToHexString(
    __in_bcount(ByteStreamSize) PUCHAR  ByteStream,
    __in ULONG                          ByteStreamSize,
    __out_bcount_z(HexStringSize) PCHAR HexString,
    __in ULONG                          HexStringSize
    );

_Requires_lock_not_held_(&Port->Lock)
NDIS_STATUS
MbbUtilAddNoMatchFilter(
    __in   PMINIPORT_ADAPTER_CONTEXT Adapter,
    __in   PMBB_PORT                 Port
    );    

_Requires_lock_not_held_(&Port->Lock)
NTSTATUS
MbbUtilRemoveNoMatchFilter(
    __in   PMINIPORT_ADAPTER_CONTEXT Adapter,
    __in   PMBB_PORT                 Port
    );


VOID
MbbUtilClearPowerFilterAtIndex(
     __in   PMINIPORT_ADAPTER_CONTEXT Adapter,
     __in   ULONG Index
    );

NTSTATUS
MbbUtilSetPowerFilterPattern(
    __in                            PMINIPORT_ADAPTER_CONTEXT Adapter,
    __in                            NDIS_PORT_NUMBER          PortNumber,
    __in                            ULONG                     PatternId,
    __in_bcount_opt(MaskSize)       PCUCHAR                   Mask,
    __in                            ULONG                     MaskSize,
    __in_bcount_opt(PatternSize)    PCUCHAR                   Pattern,
    __in                            ULONG                     PatternSize
    );

WWAN_STATUS
MbbUtilNdisToWwanStatus(
    __in NDIS_STATUS NdisStatus
    );

VOID
MbbUtilNdisMiniportIndicateStatusEx(
    __in  PMINIPORT_ADAPTER_CONTEXT Adapter,    
    __in  PNDIS_STATUS_INDICATION   StatusIndication
    );    

VOID
MbbUtilSendMbimError(
    __in ULONG                      TransactionId,
    __in MBB_ERROR                  ErrorCode,
    __in PMINIPORT_ADAPTER_CONTEXT  Adapter,
    __in GUID                       ActivityId,
    __in PMBB_COMMAND               Command
    );

NDIS_STATUS
MbbUtilWwanToMbbGetVisible(
    __in  PWWAN_GET_VISIBLE_PROVIDERS    WwanVisibleProviders,
    __out PMBB_GET_VISIBLE_PROVIDERS     MbbVisibleProviders
    );

NDIS_STATUS
MbbUtilWwanCreateMac(
     __in PMBB_REQUEST_CONTEXT     Request
    );

NDIS_STATUS
MbbUtilWwanDeleteMac(
     __in PMBB_REQUEST_CONTEXT     Request
    );

NDIS_STATUS
MbbUtilWwanAssignMacAddress(
    _In_ PMINIPORT_ADAPTER_CONTEXT Adapter,
    _In_ NDIS_WWAN_MAC_ADDRESS macAddress  
    
    );

NDIS_STATUS 
MbbUtilProcessSetConnectState(
    __in PMBB_REQUEST_CONTEXT   Request,
    __in PWWAN_SET_CONTEXT_STATE SetContextState,
    __in ULONG                  SessionId
    );

NDIS_STATUS
MbbUtilWwanCreateMacComplete(
    _In_ PMINIPORT_ADAPTER_CONTEXT Adapter,
    _In_ PMBB_PORT                 Port,
    _In_ NDIS_STATUS               NdisStatus
    );

NDIS_STATUS
MbbUtilWwanDeleteMacComplete(
    _In_ PMINIPORT_ADAPTER_CONTEXT Adapter,
    _In_ PMBB_PORT                 Port,
    _In_ NDIS_STATUS               NdisStatus
    );

NDIS_STATUS
MbbUtilDisconnectPortOnDeleteMac(
    _In_ PMINIPORT_ADAPTER_CONTEXT Adapter,
    _In_ PMBB_PORT Port
    );

NDIS_STATUS
MbbUtilDeactivateContext(
    __in MBB_PROTOCOL_HANDLE    AdapterHandle,
    __in PMBB_REQUEST_CONTEXT   Request
    );
