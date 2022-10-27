//
//    Copyright (C) Microsoft.  All rights reserved.
//
////////////////////////////////////////////////////////////////////////////////
//
//  INCLUDES
//
////////////////////////////////////////////////////////////////////////////////
#include "precomp.h"
#include "util.tmh"
#include <MapWwanMbb.h>
#include "WMBClassTelemetry.h"



////////////////////////////////////////////////////////////////////////////////
//
//  DEFINES
//
////////////////////////////////////////////////////////////////////////////////
#define WMBCLASS_MAX_EVENT_DATA_DESCRIPTORS         (10)




////////////////////////////////////////////////////////////////////////////////
//
//  TYPEDEFS
//
////////////////////////////////////////////////////////////////////////////////
// this is for translating a code into an ASCII string
typedef struct Code2Ascii
{
    NDIS_OID                Oid;
    PCHAR                   OidName;
} Code2Ascii;

typedef struct _MBB_ALLOC_MANAGER
{
    NPAGED_LOOKASIDE_LIST   LookasideList;
    NDIS_SPIN_LOCK          Lock;
    LIST_ENTRY              AllocatedList;
    LIST_ENTRY              PreAllocatedFreeList;
    MBB_ALLOC_FLAGS         Flags;

} MBB_ALLOC_MANAGER,
*PMBB_ALLOC_MANAGER;

typedef struct _MBB_ALLOC_ENTRY
{
    LIST_ENTRY          TrackLink;
    LIST_ENTRY          FreeLink;
    BOOLEAN             IsPreAllocated;
    PMBB_ALLOC_MANAGER  AllocMgr;

} MBB_ALLOC_ENTRY,
*PMBB_ALLOC_ENTRY;

typedef struct _MBB_BUFFER_MANAGER
{
    PVOID   AllocMgrHandle;
    ULONG   BufferLength;

} MBB_BUFFER_MANAGER,
*PMBB_BUFFER_MANAGER;

typedef struct _MBB_BUFFER_CONTEXT
{
    PMBB_BUFFER_MANAGER BufferManager;
    PVOID               OwnerContext;

} MBB_BUFFER_CONTEXT,
*PMBB_BUFFER_CONTEXT;

typedef struct _MBB_WORKITEM_ENTRY
{
    LIST_ENTRY              QueueLink;
    PVOID                   Context1;
    PVOID                   Context2;
    PVOID                   Context3;
    PVOID                   Context4;
    MBB_WORKITEM_ROUTINE    Routine;

} MBB_WORKITEM_ENTRY,
*PMBB_WORKITEM_ENTRY;

typedef struct _MBB_WORKITEM_MANAGER
{
    NDIS_SPIN_LOCK  Lock;
    PVOID           AllocMgrHandle;
    KEVENT          ThreadExitEvent;
    KEVENT          WorkItemEvent;
    LIST_ENTRY      WorkItemQueue;
    HANDLE          ThreadHandle;

} MBB_WORKITEM_MANAGER,
*PMBB_WORKITEM_MANAGER;




////////////////////////////////////////////////////////////////////////////////
//
//  GLOBALS
//
////////////////////////////////////////////////////////////////////////////////
const Code2Ascii OidList[] =
{
    OID_GEN_SUPPORTED_LIST,                 "OID_GEN_SUPPORTED_LIST",
    OID_GEN_HARDWARE_STATUS,                "OID_GEN_HARDWARE_STATUS",
    OID_GEN_MEDIA_SUPPORTED,                "OID_GEN_MEDIA_SUPPORTED",
    OID_GEN_MEDIA_IN_USE,                   "OID_GEN_MEDIA_IN_USE",
    OID_GEN_MAXIMUM_LOOKAHEAD,              "OID_GEN_MAXIMUM_LOOKAHEAD",
    OID_GEN_MAXIMUM_FRAME_SIZE,             "OID_GEN_MAXIMUM_FRAME_SIZE",
    OID_GEN_LINK_SPEED,                     "OID_GEN_LINK_SPEED",
    OID_GEN_TRANSMIT_BUFFER_SPACE,          "OID_GEN_TRANSMIT_BUFFER_SPACE",
    OID_GEN_RECEIVE_BUFFER_SPACE,           "OID_GEN_RECEIVE_BUFFER_SPACE",
    OID_GEN_TRANSMIT_BLOCK_SIZE,            "OID_GEN_TRANSMIT_BLOCK_SIZE",
    OID_GEN_RECEIVE_BLOCK_SIZE,             "OID_GEN_RECEIVE_BLOCK_SIZE",
    OID_GEN_VENDOR_ID,                      "OID_GEN_VENDOR_ID",
    OID_GEN_VENDOR_DESCRIPTION,             "OID_GEN_VENDOR_DESCRIPTION",
    OID_GEN_CURRENT_PACKET_FILTER,          "OID_GEN_CURRENT_PACKET_FILTER",
    OID_GEN_TRANSPORT_HEADER_OFFSET,        "OID_GEN_TRANSPORT_HEADER_OFFSET",
    OID_GEN_CURRENT_LOOKAHEAD,              "OID_GEN_CURRENT_LOOKAHEAD",
    OID_GEN_DRIVER_VERSION,                 "OID_GEN_DRIVER_VERSION",
    OID_GEN_VENDOR_DRIVER_VERSION,          "OID_GEN_VENDOR_DRIVER_VERSION",
    OID_GEN_MAXIMUM_TOTAL_SIZE,             "OID_GEN_MAXIMUM_TOTAL_SIZE",
    OID_GEN_PROTOCOL_OPTIONS,               "OID_GEN_PROTOCOL_OPTIONS",
    OID_GEN_MAC_OPTIONS,                    "OID_GEN_MAC_OPTIONS",
    OID_GEN_MEDIA_CONNECT_STATUS,           "OID_GEN_MEDIA_CONNECT_STATUS",
    OID_GEN_MAXIMUM_SEND_PACKETS,           "OID_GEN_MAXIMUM_SEND_PACKETS",
    OID_GEN_SUPPORTED_GUIDS,                "OID_GEN_SUPPORTED_GUIDS",
    OID_GEN_XMIT_OK,                        "OID_GEN_XMIT_OK",
    OID_GEN_RCV_OK,                         "OID_GEN_RCV_OK",
    OID_GEN_XMIT_ERROR,                     "OID_GEN_XMIT_ERROR",
    OID_GEN_RCV_ERROR,                      "OID_GEN_RCV_ERROR",
    OID_GEN_RCV_NO_BUFFER,                  "OID_GEN_RCV_NO_BUFFER",
    OID_GEN_RCV_CRC_ERROR,                  "OID_GEN_RCV_CRC_ERROR",
    OID_GEN_TRANSMIT_QUEUE_LENGTH,          "OID_GEN_TRANSMIT_QUEUE_LENGTH",
    OID_802_3_PERMANENT_ADDRESS,            "OID_802_3_PERMANENT_ADDRESS",
    OID_802_3_CURRENT_ADDRESS,              "OID_802_3_CURRENT_ADDRESS",
    OID_802_3_MULTICAST_LIST,               "OID_802_3_MULTICAST_LIST",
    OID_802_3_MAXIMUM_LIST_SIZE,            "OID_802_3_MAXIMUM_LIST_SIZE",
    OID_802_3_RCV_ERROR_ALIGNMENT,          "OID_802_3_RCV_ERROR_ALIGNMENT",
    OID_802_3_XMIT_ONE_COLLISION,           "OID_802_3_XMIT_ONE_COLLISION",
    OID_802_3_XMIT_MORE_COLLISIONS,         "OID_802_3_XMIT_MORE_COLLISIONS",
    OID_802_3_XMIT_DEFERRED,                "OID_802_3_XMIT_DEFERRED",
    OID_802_3_XMIT_MAX_COLLISIONS,          "OID_802_3_XMIT_MAX_COLLISIONS",
    OID_802_3_RCV_OVERRUN,                  "OID_802_3_RCV_OVERRUN",
    OID_802_3_XMIT_UNDERRUN,                "OID_802_3_XMIT_UNDERRUN",
    OID_802_3_XMIT_HEARTBEAT_FAILURE,       "OID_802_3_XMIT_HEARTBEAT_FAILURE",
    OID_802_3_XMIT_TIMES_CRS_LOST,          "OID_802_3_XMIT_TIMES_CRS_LOST",
    OID_802_3_XMIT_LATE_COLLISIONS,         "OID_802_3_XMIT_LATE_COLLISIONS",
    OID_802_3_MAC_OPTIONS,                  "OID_802_3_MAC_OPTIONS",
    OID_TCP_TASK_OFFLOAD,                   "OID_TCP_TASK_OFFLOAD",
    OID_PNP_CAPABILITIES,                   "OID_PNP_CAPABILITIES",
    OID_PNP_SET_POWER,                      "OID_PNP_SET_POWER",
    OID_PNP_QUERY_POWER,                    "OID_PNP_QUERY_POWER",
    OID_PNP_ADD_WAKE_UP_PATTERN,            "OID_PNP_ADD_WAKE_UP_PATTERN",
    OID_PNP_REMOVE_WAKE_UP_PATTERN,         "OID_PNP_REMOVE_WAKE_UP_PATTERN",
    OID_PNP_ENABLE_WAKE_UP,                 "OID_PNP_ENABLE_WAKE_UP",
    OID_GEN_STATISTICS,                     "OID_GEN_STATISTICS",
    OID_GEN_MEDIA_SUPPORTED,                "OID_GEN_MEDIA_SUPPORTED",
    OID_WWAN_DRIVER_CAPS,                   "OID_WWAN_DRIVER_CAPS",
    OID_WWAN_DEVICE_CAPS,                   "OID_WWAN_DEVICE_CAPS",
    OID_WWAN_READY_INFO,                    "OID_WWAN_READY_INFO",
    OID_WWAN_RADIO_STATE,                   "OID_WWAN_RADIO_STATE",
    OID_WWAN_PIN_EX,                        "OID_WWAN_PIN_EX",
    OID_WWAN_PIN_LIST,                      "OID_WWAN_PIN_LIST",
    OID_WWAN_HOME_PROVIDER,                 "OID_WWAN_HOME_PROVIDER",
    OID_WWAN_PREFERRED_PROVIDERS,           "OID_WWAN_PREFERRED_PROVIDERS",
    OID_WWAN_VISIBLE_PROVIDERS,             "OID_WWAN_VISIBLE_PROVIDERS",
    OID_WWAN_REGISTER_STATE,                "OID_WWAN_REGISTER_STATE",
    OID_WWAN_PACKET_SERVICE,                "OID_WWAN_PACKET_SERVICE",
    OID_WWAN_SIGNAL_STATE,                  "OID_WWAN_SIGNAL_STATE",
    OID_WWAN_CONNECT,                       "OID_WWAN_CONNECT",
    OID_WWAN_PROVISIONED_CONTEXTS,          "OID_WWAN_PROVISIONED_CONTEXTS",
    OID_WWAN_SERVICE_ACTIVATION,            "OID_WWAN_SERVICE_ACTIVATION",
    OID_WWAN_ENUMERATE_DEVICE_SERVICES,     "OID_WWAN_ENUMERATE_DEVICE_SERVICES",
    OID_WWAN_DEVICE_SERVICE_COMMAND,        "OID_WWAN_DEVICE_SERVICE_COMMAND",
    OID_WWAN_SUBSCRIBE_DEVICE_SERVICE_EVENTS,"OID_WWAN_SUBSCRIBE_DEVICE_SERVICE_EVENTS",
    OID_WWAN_SMS_CONFIGURATION,             "OID_WWAN_SMS_CONFIGURATION",
    OID_WWAN_SMS_READ,                      "OID_WWAN_SMS_READ",
    OID_WWAN_SMS_SEND,                      "OID_WWAN_SMS_SEND",
    OID_WWAN_SMS_DELETE,                    "OID_WWAN_SMS_DELETE",
    OID_WWAN_SMS_STATUS,                    "OID_WWAN_SMS_STATUS",
    OID_WWAN_VENDOR_SPECIFIC,               "OID_WWAN_VENDOR_SPECIFIC",
    OID_WWAN_USSD,                          "OID_WWAN_USSD",
    OID_WWAN_AUTH_CHALLENGE,                "OID_WWAN_AUTH_CHALLENGE",
    OID_WWAN_ENUMERATE_DEVICE_SERVICE_COMMANDS,"OID_WWAN_ENUMERATE_DEVICE_SERVICE_COMMANDS",
    OID_WWAN_DEVICE_SERVICE_SESSION,        "OID_WWAN_DEVICE_SERVICE_SESSION",
    OID_WWAN_DEVICE_SERVICE_SESSION_WRITE,  "OID_WWAN_DEVICE_SERVICE_SESSION_WRITE",
    OID_WWAN_DEVICE_SERVICE_SESSION_WRITE,  "OID_WWAN_DEVICE_SERVICE_SESSION_WRITE",
    OID_WWAN_CREATE_MAC,                    "OID_WWAN_CREATE_MAC",
    OID_WWAN_DELETE_MAC,                    "OID_WWAN_DELETE_MAC",
    OID_WWAN_UICC_FILE_STATUS,              "OID_WWAN_UICC_FILE_STATUS",
    OID_WWAN_UICC_ACCESS_BINARY,            "OID_WWAN_UICC_ACCESS_BINARY",
    OID_WWAN_UICC_ACCESS_RECORD,            "OID_WWAN_UICC_ACCESS_RECORD",
    OID_WWAN_PIN_EX2,                       "OID_WWAN_PIN_EX2",
    OID_WWAN_MBIM_VERSION,                  "OID_WWAN_MBIM_VERSION",
    OID_WWAN_SYS_CAPS,                      "OID_WWAN_SYS_CAPS",
    OID_WWAN_DEVICE_CAPS_EX,                "OID_WWAN_DEVICE_CAPS_EX",
    OID_WWAN_SYS_SLOTMAPPINGS,              "OID_WWAN_SYS_SLOTMAPPINGS",
    OID_WWAN_SLOT_INFO_STATUS,              "OID_WWAN_SLOT_INFO_STATUS",
    OID_WWAN_DEVICE_BINDINGS,               "OID_WWAN_DEVICE_BINDINGS",
    OID_WWAN_REGISTER_STATE_EX,             "OID_WWAN_REGISTER_STATE_EX",
    OID_WWAN_IMS_VOICE_STATE,               "OID_WWAN_IMS_VOICE_STATE",
    OID_WWAN_SIGNAL_STATE_EX,               "OID_WWAN_SIGNAL_STATE_EX",
    OID_WWAN_LOCATION_STATE,                "OID_WWAN_LOCATION_STATE",
    OID_WWAN_NITZ,                          "OID_WWAN_NITZ",
    OID_WWAN_NETWORK_IDLE_HINT,             "OID_WWAN_NETWORK_IDLE_HINT",
    OID_WWAN_PRESHUTDOWN,                   "OID_WWAN_PRESHUTDOWN",
    OID_WWAN_UICC_ATR,                      "OID_WWAN_UICC_ATR",
    OID_WWAN_UICC_OPEN_CHANNEL,             "OID_WWAN_UICC_OPEN_CHANNEL",
    OID_WWAN_UICC_CLOSE_CHANNEL,            "OID_WWAN_UICC_CLOSE_CHANNEL",
    OID_WWAN_UICC_APDU,                     "OID_WWAN_UICC_APDU",
    OID_WWAN_UICC_TERMINAL_CAPABILITY,      "OID_WWAN_UICC_TERMINAL_CAPABILITY",
    OID_WWAN_SAR_CONFIG,                    "OID_WWAN_SAR_CONFIG",
    OID_WWAN_SAR_TRANSMISSION_STATUS,       "OID_WWAN_SAR_TRANSMISSION_STATUS",
    OID_WWAN_LTE_ATTACH_CONFIG,             "OID_WWAN_LTE_ATTACH_CONFIG",
    OID_WWAN_LTE_ATTACH_STATUS,             "OID_WWAN_LTE_ATTACH_STATUS",
    OID_WWAN_NETWORK_BLACKLIST,             "OID_WWAN_NETWORK_BLACKLIST",
    OID_WWAN_UICC_RESET,                    "OID_WWAN_UICC_RESET",
    OID_WWAN_DEVICE_RESET,                  "OID_WWAN_DEVICE_RESET",
    OID_WWAN_UICC_APP_LIST,                 "OID_WWAN_UICC_APP_LIST",
};

const UINT NumOid = sizeof(OidList) / sizeof(Code2Ascii);

const Code2Ascii IndicationList[] =
{
    NDIS_STATUS_WWAN_DEVICE_CAPS,                       "NDIS_STATUS_WWAN_DEVICE_CAPS",
    NDIS_STATUS_WWAN_READY_INFO,                        "NDIS_STATUS_WWAN_READY_INFO",
    NDIS_STATUS_WWAN_RADIO_STATE,                       "NDIS_STATUS_WWAN_RADIO_STATE",
    NDIS_STATUS_WWAN_PIN_INFO,                          "NDIS_STATUS_WWAN_PIN_INFO",
    NDIS_STATUS_WWAN_PIN_LIST,                          "NDIS_STATUS_WWAN_PIN_LIST",
    NDIS_STATUS_WWAN_HOME_PROVIDER,                     "NDIS_STATUS_WWAN_HOME_PROVIDER",
    NDIS_STATUS_WWAN_PREFERRED_PROVIDERS,               "NDIS_STATUS_WWAN_PREFERRED_PROVIDERS",
    NDIS_STATUS_WWAN_VISIBLE_PROVIDERS,                 "NDIS_STATUS_WWAN_VISIBLE_PROVIDERS",
    NDIS_STATUS_WWAN_REGISTER_STATE,                    "NDIS_STATUS_WWAN_REGISTER_STATE",
    NDIS_STATUS_WWAN_PACKET_SERVICE,                    "NDIS_STATUS_WWAN_PACKET_SERVICE",
    NDIS_STATUS_WWAN_SIGNAL_STATE,                      "NDIS_STATUS_WWAN_SIGNAL_STATE",
    NDIS_STATUS_WWAN_CONTEXT_STATE,                     "NDIS_STATUS_WWAN_CONTEXT_STATE",
    NDIS_STATUS_WWAN_PROVISIONED_CONTEXTS,              "NDIS_STATUS_WWAN_PROVISIONED_CONTEXTS",
    NDIS_STATUS_WWAN_SERVICE_ACTIVATION,                "NDIS_STATUS_WWAN_SERVICE_ACTIVATION",
    NDIS_STATUS_WWAN_SMS_CONFIGURATION,                 "NDIS_STATUS_WWAN_SMS_CONFIGURATION",
    NDIS_STATUS_WWAN_SMS_RECEIVE,                       "NDIS_STATUS_WWAN_SMS_RECEIVE",
    NDIS_STATUS_WWAN_SMS_SEND,                          "NDIS_STATUS_WWAN_SMS_SEND",
    NDIS_STATUS_WWAN_SMS_DELETE,                        "NDIS_STATUS_WWAN_SMS_DELETE",
    NDIS_STATUS_WWAN_SMS_STATUS,                        "NDIS_STATUS_WWAN_SMS_STATUS",
    NDIS_STATUS_WWAN_DNS_ADDRESS,                       "NDIS_STATUS_WWAN_DNS_ADDRESS",

    NDIS_STATUS_WWAN_VENDOR_SPECIFIC,                   "NDIS_STATUS_WWAN_VENDOR_SPECIFIC",

    NDIS_STATUS_WWAN_SUPPORTED_DEVICE_SERVICES,         "NDIS_STATUS_WWAN_SUPPORTED_DEVICE_SERVICES",
    NDIS_STATUS_WWAN_DEVICE_SERVICE_SUBSCRIPTION,       "NDIS_STATUS_WWAN_DEVICE_SERVICE_SUBSCRIPTION",
    NDIS_STATUS_WWAN_SET_HOME_PROVIDER_COMPLETE,        "NDIS_STATUS_WWAN_SET_HOME_PROVIDER_COMPLETE",
    NDIS_STATUS_WWAN_DEVICE_SERVICE_RESPONSE,           "NDIS_STATUS_WWAN_DEVICE_SERVICE_RESPONSE",
    NDIS_STATUS_WWAN_DEVICE_SERVICE_EVENT,              "NDIS_STATUS_WWAN_DEVICE_SERVICE_EVENT",
    NDIS_STATUS_WWAN_USSD,                              "NDIS_STATUS_WWAN_USSD",
    NDIS_STATUS_WWAN_AUTH_RESPONSE,                     "NDIS_STATUS_WWAN_AUTH_RESPONSE",
    NDIS_STATUS_WWAN_DEVICE_SERVICE_SUPPORTED_COMMANDS, "NDIS_STATUS_WWAN_DEVICE_SERVICE_SUPPORTED_COMMANDS",
    NDIS_STATUS_WWAN_DEVICE_SERVICE_SESSION,            "NDIS_STATUS_WWAN_DEVICE_SERVICE_SESSION",
    NDIS_STATUS_WWAN_DEVICE_SERVICE_SESSION_WRITE_COMPLETE, "NDIS_STATUS_WWAN_DEVICE_SERVICE_SESSION_WRITE_COMPLETE",
    NDIS_STATUS_WWAN_DEVICE_SERVICE_SESSION_READ,       "NDIS_STATUS_WWAN_DEVICE_SERVICE_SESSION_READ",
    NDIS_STATUS_WWAN_PRESHUTDOWN_STATE,                 "NDIS_STATUS_WWAN_PRESHUTDOWN_STATE",
    NDIS_STATUS_WWAN_SYS_CAPS_INFO,                     "NDIS_STATUS_WWAN_SYS_CAPS_INFO",
    NDIS_STATUS_WWAN_DEVICE_SLOT_MAPPING_INFO,          "NDIS_STATUS_WWAN_DEVICE_SLOT_MAPPING_INFO",
    NDIS_STATUS_WWAN_SLOT_INFO,                         "NDIS_STATUS_WWAN_SLOT_INFO",
    NDIS_STATUS_WWAN_DEVICE_CAPS_EX,                    "NDIS_STATUS_WWAN_DEVICE_CAPS_EX",
};

const UINT NumIndications = sizeof(IndicationList) / sizeof(Code2Ascii);

CHAR CommandStringTableBasic[MBB_BASIC_CID_MAXIMUM][48] =
{
    "BASIC_UNKNOWN_0",
    "BASIC_DEVICE_CAPS",
    "BASIC_SUBSCRIBER_READY_INFO",
    "BASIC_RADIO_STATE",
    "BASIC_PIN_INFO",
    "BASIC_PIN_LIST",
    "BASIC_HOME_PROVIDER",
    "BASIC_PREFERRED_PROVIDERS",
    "BASIC_VISIBLE_PROVIDERS",
    "BASIC_REGISTER_STATE",
    "BASIC_PACKET_SERVICE",
    "BASIC_SIGNAL_STATE",
    "BASIC_CONNECT",
    "BASIC_PROVISIONED_CONTEXTS",
    "BASIC_SERVICE_ACTIVATION",
    "BASIC_IP_ADDRESS_INFO",
    "BASIC_CID_DEVICE_SERVICES",
    "BASIC_UNKNOWN_17",
    "BASIC_UNKNOWN_18",
    "BASIC_NOTIFY_DEVICE_SERVICE_UPDATES",
    "BASIC_PACKET_STATISTICS",
    "BASIC_NETWORK_IDLE_HINT",
    "BASIC_EMERGENCY_MODE",
    "BASIC_PACKET_FILTERS"
};

CHAR CommandStringTableSMS[MBB_SMS_CID_MAXIMUM][32] =
{
    "SMS_UNKNOWN",
    "SMS_CONFIGURATION",
    "SMS_READ",
    "SMS_SEND",
    "SMS_DELETE",
    "SMS_STATUS"
};

CHAR CommandStringTableVendor[MBB_VENDOR_CID_MAXIMUM][32] =
{
    "VENDOR_UNKNOWN",
    "VENDOR_MS_SPECIFIC",
};

CHAR CommandStringTableUSSD[MBB_USSD_CID_MAXIMUM][32] =
{
    "USSD_UNKNOWN",
    "USSD_USSD"
};

CHAR CommandStringTablePhoneBook[MBB_PHONEBOOK_CID_MAXIMUM][32] =
{
    "PHONEBOOK_UNKNOWN",
    "PHONEBOOK_CONFIGURATION",
    "PHONEBOOK_READ",
    "PHONEBOOK_DELETE",
    "PHONEBOOK_SAVE"
};

CHAR CommandStringTableSAT[MBB_SAT_CID_MAXIMUM][32] =
{
    "SAT_UNKNOWN",
    "SAT_PAC",
    "SAT_TERMINAL_RESPONSE",
    "SAT_ENVELOPE"
};


CHAR CommandStringTableAuth[MBB_AUTH_CID_MAXIUM][32]=
{
    "AUTH_UNKNOWN",
    "AUTH_AKA",
    "AUTH_AKAP",
    "AUTH_SIM"
};

CHAR CommandStringTableMulticarrier[MBB_MULTICARRIER_CID_MAXIMUM][32]=
{
    "MULTICARRIER_UNKNOWN",
    "MULTICARRIER_CAPABILITIES",
    "MULTICARRIER_LOCATION_INFO",
    "MULTICARRIER_CURRENT_CID_LIST"
};

CHAR CommandStringTableDss[MBB_DSS_CID_MAXIUM][32]=
{
    "DSS_CID_UNKNOWN",
    "DSS_CID_CONNECT"
};

CHAR CommandStringTableHostShutdown[MBB_HOSTSHUTDOWN_CID_MAX][32]=
{
    "HOSTSHUTDOWN_UNKNOWN",
    "HOSTSHUTDOWN_CID_ONE",
    "HOSTSHUTDOWN_CID_PRESHUTDOWN",
};

CHAR CommandStringTableUicc[MBB_UICC_CID_MAXIMUM][32] =
{
    "UICC_UNKNOWN",
    "UICC_CID_ATR",
    "UICC_CID_OPEN_CHANNEL",
    "UICC_CID_CLOSE_CHANNEL",
    "UICC_CID_APDU",
    "UICC_CID_TERMINAL_CAPABILITY",
    "UICC_CID_RESET"
    "UICC_APP_LIST",
    "UICC_FILE_STATUS",
    "UICC_ACCESS_BINARY",
    "UICC_ACCESS_RECORD",
};

CHAR CommandStringTableSar[MBB_SAR_CID_MAXIMUM][32] =
{
    "SAR_UNKNOWN",
    "SAR_CONFIG",
    "SAR_TRANSMISSION_STATUS"
};

CHAR CommandStringTableBasicExt[MBB_BASICCONNECTEXT_CID_MAXIMUM][32] =
{
    "BASICEXT_UNKNOWN",
    "BASICEXT_PROVISIONED_CONTEXT_V2",
    "BASICEXT_NETWORK_BLACKLIST",
    "BASICEXT_LTE_ATTACH_CONFIG",
    "BASICEXT_LTE_ATTACH_STATUS",
    "BASICEXT_CID_SYS_CAPS",
    "BASICEXT_CID_DEVICE_CAPS_V2",
    "BASICEXT_CID_SLOT_MAPPINGS",
    "BASICEXT_CID_SLOT_INFO_STATUS",
    "BASICEXT_CID_PCO",
    "BASICEXT_CID_DEVICE_RESET"
};


////////////////////////////////////////////////////////////////////////////////
//
//  PROTOTYPES
//
////////////////////////////////////////////////////////////////////////////////

//
// FRAGMENT ROUTINES
//

NTSTATUS
MbbUtilBusSendMessageFragment(
    __in    PMBB_REQUEST_CONTEXT    Request,
    __in    MBB_REQUEST_HANDLE      FragmentBufferHandle,
    __in    PVOID                   MessageFragment,
    __in    ULONG                   FragmentLength
    );

VOID
MbbUtilSendMessageFragmentComplete(
    __in    MBB_PROTOCOL_HANDLE     ProtocolHandle,
    __in    MBB_REQUEST_HANDLE      RequestHandle,
    __in    NTSTATUS                NtStatus
    );

//
// WORK ITEM
//

__drv_sameIRQL
__drv_functionClass(KSTART_ROUTINE)
VOID 
MbbWorkMgrProcessWorkItem( 
    __in PVOID  StartContext
    );




////////////////////////////////////////////////////////////////////////////////
//
//  IMPLEMENTATION
//
////////////////////////////////////////////////////////////////////////////////

/****************************************************************************/
/*                          GetOidName                                      */
/****************************************************************************/
/*                                                                          */
/* Routine Description:                                                     */
/*                                                                          */
/*  Returns string with OID name                                            */
/*                                                                          */
/* Arguments:                                                               */
/*                                                                          */
/*  Oid - OID to find                                                       */
/*                                                                          */
/* Return:                                                                  */
/*                                                                          */
/*  PCHAR                                                                   */
/*                                                                          */
/****************************************************************************/
PCSTR
GetOidName(
    __in NDIS_OID Oid
    )
{
    UINT Index;

    //
    // see if the matching Oid and string are in the list
    //
    for(Index = 0; Index < NumOid; Index++)
    {
        if(Oid == OidList[Index].Oid)
        {
            return OidList[Index].OidName;
        }
    }

//    TraceWarn(WMBCLASS_OID, " unknown OID %08lx", Oid);

    return "Unknown OID";

} // GetOidName

BOOLEAN
IsSameGuid(GUID * pgGuid1, GUID *pgGuid2)
{
    ASSERT (pgGuid1);
    ASSERT(pgGuid2);
    
    return (!memcmp(pgGuid1, pgGuid2, sizeof(GUID)));
}

BOOLEAN
IsZeroGuid(
    __in    GUID*   pGuid
    )
{
    GUID    ZeroGuid    =   {0};
    return IsSameGuid(pGuid,&ZeroGuid);;
}

PCSTR
MbbUtilGetWwanIndicationString(
    __in NDIS_STATUS Indication
    )
{
    UINT Index;

    //
    // see if the matching Oid and string are in the list
    //
    for(Index = 0; Index < NumOid; Index++)
    {
        if(Indication == IndicationList[Index].Oid)
        {
            return OidList[Index].OidName;
        }
    }

//    TraceWarn(WMBCLASS_OID, " unknown OID %08lx", Oid);

    return "Unknown Indication";

} // GetOidName



/****************************************************************************/
/*                          GetDeviceFriendlyName                           */
/****************************************************************************/
/*                                                                          */
/* Routine Description:                                                     */
/*                                                                          */
/*  Return the friendly name associated with the given Adapter.             */
/*                                                                          */
/* Arguments:                                                               */
/*                                                                          */
/*  MiniportAdapterHandle - NDIS context for the adapter                    */
/*                                                                          */
/* Return:                                                                  */
/*                                                                          */
/*    NTSTATUS                                                              */
/*                                                                          */
/****************************************************************************/
NDIS_STATUS
GetDeviceFriendlyName(
    __in NDIS_HANDLE    MiniportAdapterHandle,
    __out PANSI_STRING  AnsiName
    )
{
    NDIS_STATUS                 Status = NDIS_STATUS_SUCCESS;
    NTSTATUS                    NtStatus;
    UNICODE_STRING              UnicodeString = {0, 0, NULL};
    ANSI_STRING                 AnsiString = {0, 0, NULL};
    USHORT                      AnsiMaxLength;



    do
    {
        RtlInitAnsiString(AnsiName,NULL);

        Status = NdisMQueryAdapterInstanceName(&UnicodeString, MiniportAdapterHandle);

        if (Status != NDIS_STATUS_SUCCESS)
        {
            break;
        }
            
        //
        //  Allocate space for ANSI version.
        //
        AnsiMaxLength = UnicodeString.MaximumLength+sizeof(WCHAR) / sizeof(WCHAR);

        AnsiString.Buffer =  ALLOCATE_NONPAGED_POOL( AnsiMaxLength );

        if (AnsiString.Buffer == NULL)
        {
            Status = STATUS_INSUFFICIENT_RESOURCES;
            break;
        }

        AnsiString.MaximumLength = AnsiMaxLength;
        AnsiString.Length = 0;
        RtlZeroMemory(AnsiString.Buffer, AnsiString.MaximumLength);

        NtStatus = RtlUnicodeStringToAnsiString(&AnsiString, &UnicodeString, FALSE);

        if (!NT_SUCCESS(NtStatus))
        {
            ASSERT(FALSE);
            Status = NDIS_STATUS_FAILURE;
            break;
        }

        *AnsiName = AnsiString;
        AnsiString.Buffer=NULL;
        Status = NDIS_STATUS_SUCCESS;
        
        break;
    }
    while (FALSE);

    if (UnicodeString.Buffer != NULL)
    {
        NdisFreeMemory(UnicodeString.Buffer, 0, 0);
    }

    if (AnsiString.Buffer != NULL)
    {
        FREE_POOL(AnsiString.Buffer);
    }


    return (Status);
}


VOID
FreeAdapterBlock(
    _In_ __drv_freesMem(Mem) PMINIPORT_ADAPTER_CONTEXT Adapter
    )

{
    ULONG       i;
   
    if ( Adapter != NULL)
    {
        ShutdownAdapterStateObject(&Adapter->AdapterState);

        FreeDeviceServiceState(&Adapter->DeviceServiceState);

        if (Adapter->FriendlyName.Buffer != NULL)
        {
            FREE_POOL(Adapter->FriendlyName.Buffer);
            Adapter->FriendlyName.Buffer=NULL;
        }       

        if (Adapter->BusHandle != NULL)
        {
            MbbBusCleanup(Adapter->BusHandle);
            Adapter->BusHandle=NULL;
        }

        if (Adapter->PowerFilterTable != NULL)
        {

            for (i=0; i<Adapter->BusParams.PowerFiltersSupported; i++)
            {
                if (Adapter->PowerFilterTable[i].InUse)
                {
                    // all the WOL patterns should have been removed by now
                    ASSERT(FALSE);
                    
                    FREE_POOL(Adapter->PowerFilterTable[i].Mask);
                    Adapter->PowerFilterTable[i].Mask=NULL;
                    FREE_POOL(Adapter->PowerFilterTable[i].Pattern);
                    Adapter->PowerFilterTable[i].Pattern=NULL;
                }               
            }

            FREE_POOL(Adapter->PowerFilterTable);
            Adapter->PowerFilterTable=NULL;

        }        

        NdisFreeSpinLock(&Adapter->PortsLock);
        
        NdisFreeSpinLock(&Adapter->SessionIdPortTableLock);
        
        FREE_POOL(Adapter);
        Adapter=NULL;
    }

    return;
}

PCHAR
MbbUtilGetCommandString(
    __in PMBB_COMMAND Command
    )
{
    if( RtlCompareMemory( &Command->ServiceId, &MBB_UUID_BASIC_CONNECT, sizeof(GUID) ) == sizeof(GUID) )
    {
        if( Command->CommandId >= MBB_BASIC_CID_MAXIMUM )
            return CommandStringTableBasic[0];
        else
            return CommandStringTableBasic[Command->CommandId];
    }
    else if( RtlCompareMemory( &Command->ServiceId, &MBB_UUID_SMS, sizeof(GUID) ) == sizeof(GUID) )
    {
        if( Command->CommandId >= MBB_SMS_CID_MAXIMUM )
            return CommandStringTableSMS[0];
        else
            return CommandStringTableSMS[Command->CommandId];
    }
    else if( RtlCompareMemory( &Command->ServiceId, &MBB_UUID_USSD, sizeof(GUID) ) == sizeof(GUID) )
    {
        if( Command->CommandId >= MBB_USSD_CID_MAXIMUM )
            return CommandStringTableUSSD[0];
        else
            return CommandStringTableUSSD[Command->CommandId];
    }
    else if( RtlCompareMemory( &Command->ServiceId, &MBB_UUID_PHONEBOOK, sizeof(GUID) ) == sizeof(GUID) )
    {
        if( Command->CommandId >= MBB_PHONEBOOK_CID_MAXIMUM )
            return CommandStringTablePhoneBook[0];
        else
            return CommandStringTablePhoneBook[Command->CommandId];
    }
    else if( RtlCompareMemory( &Command->ServiceId, &MBB_UUID_SAT, sizeof(GUID) ) == sizeof(GUID) )
    {
        if( Command->CommandId >= MBB_SAT_CID_MAXIMUM )
            return CommandStringTableSAT[0];
        else
            return CommandStringTableSAT[Command->CommandId];
    }
    else if( RtlCompareMemory( &Command->ServiceId, &MBB_UUID_MS_VENDOR_EXTENSION, sizeof(GUID) ) == sizeof(GUID) )
    {
        if( Command->CommandId >= MBB_VENDOR_CID_MAXIMUM )
            return CommandStringTableVendor[0];
        else
            return CommandStringTableVendor[Command->CommandId];
    }
    else if( RtlCompareMemory( &Command->ServiceId, &MBB_UUID_AUTH, sizeof(GUID) ) == sizeof(GUID) )
    {
        if( Command->CommandId >= MBB_AUTH_CID_MAXIUM)
            return CommandStringTableAuth[0];
        else
            return CommandStringTableAuth[Command->CommandId];
    }
    else if( RtlCompareMemory( &Command->ServiceId, &MBB_UUID_MULTICARRIER, sizeof(GUID) ) == sizeof(GUID) )
    {
        if( Command->CommandId >= MBB_MULTICARRIER_CID_MAXIMUM)
            return CommandStringTableMulticarrier[0];
        else
            return CommandStringTableMulticarrier[Command->CommandId];
    }
    else if( RtlCompareMemory( &Command->ServiceId, &MBB_UUID_DSS, sizeof(GUID) ) == sizeof(GUID) )
    {
        if( Command->CommandId >= MBB_DSS_CID_MAXIUM)
            return CommandStringTableDss[0];
        else
            return CommandStringTableDss[Command->CommandId];
    }
    else if( RtlCompareMemory( &Command->ServiceId, &MBB_UUID_HOSTSHUTDOWN, sizeof(GUID) ) == sizeof(GUID) )
    {
        if( Command->CommandId >= MBB_HOSTSHUTDOWN_CID_MAX)
            return CommandStringTableHostShutdown[0];
        else
            return CommandStringTableHostShutdown[Command->CommandId];
    }
    else if( RtlCompareMemory( &Command->ServiceId, &MBB_UUID_UICC_LOW_LEVEL, sizeof(GUID) ) == sizeof(GUID) )
    {
        if( Command->CommandId >= MBB_UICC_CID_MAXIMUM)
            return CommandStringTableUicc[0];
        else
            return CommandStringTableUicc[Command->CommandId];
    }
    else if (RtlCompareMemory(&Command->ServiceId, &MBB_UUID_SARCONTROL, sizeof(GUID)) == sizeof(GUID))
    {
        if (Command->CommandId >= MBB_SAR_CID_MAXIMUM)
            return CommandStringTableSar[0];
        else
            return CommandStringTableSar[Command->CommandId];
    }
    else if (RtlCompareMemory(&Command->ServiceId, &MBB_UUID_BASIC_CONNECT_EXTENSIONS, sizeof(GUID)) == sizeof(GUID))
    {
        if (Command->CommandId >= MBB_BASICCONNECTEXT_CID_MAXIMUM)
            return CommandStringTableBasicExt[0];
        else
            return CommandStringTableBasicExt[Command->CommandId];
    }
    else
    {
        return "UNKNOWN";
    }
}

BOOLEAN
MbbUtilIsBasicCommand(
    __in PMBB_COMMAND Command
    )
{
    if( RtlCompareMemory( &Command->ServiceId, &MBB_UUID_BASIC_CONNECT, sizeof(GUID) ) == sizeof(GUID) )
        return TRUE;
    else
        return FALSE;
}

BOOLEAN
MbbUtilIsValidDeviceService(
    __in GUID*       DeviceServiceGuid
    )
{
    // We could check list of supported services. However we only do one check
    
    if(RtlCompareMemory( &DeviceServiceGuid, &MBB_UUID_INVALID, sizeof(GUID) ) == sizeof(GUID) )
    {
        return FALSE;
    }

    return TRUE;
}


//
// ALLOCATOR
//

__drv_allocatesMem(Mem)
__drv_aliasesMem
PVOID
MbbAllocMgrInitialize(
    __in    ULONG           AllocationSize,
    __in    ULONG           AllocationCount,
    __in    ULONG           PoolTag,
    __in    MBB_ALLOC_FLAGS Flags
    )
{
    ULONG               i;
    PMBB_ALLOC_MANAGER  AllocMgr;
    PMBB_ALLOC_ENTRY    AllocEntry;

    if( (AllocMgr = (PMBB_ALLOC_MANAGER) ALLOCATE_NONPAGED_POOL(sizeof(MBB_ALLOC_MANAGER))) == NULL )
    {
        return NULL;
    }

    NdisAllocateSpinLock( &AllocMgr->Lock );

    NdisInitializeNPagedLookasideList(
        &AllocMgr->LookasideList,
        NULL,   // allocate function
        NULL,   // free function
        0,      // Flags
        AllocationSize + sizeof(MBB_ALLOC_ENTRY),
        PoolTag,
        0       // Depth
        );

    InitializeListHead( &AllocMgr->AllocatedList );
    InitializeListHead( &AllocMgr->PreAllocatedFreeList );

    AllocMgr->Flags.Value = Flags.Value;

    for( i = 0;
         i < AllocationCount;
         i ++ )
    {
        if( (AllocEntry = (PMBB_ALLOC_ENTRY) NdisAllocateFromNPagedLookasideList( &AllocMgr->LookasideList )) == NULL )
        {
            break;
        }
        AllocEntry->AllocMgr        = AllocMgr;
        AllocEntry->IsPreAllocated  = TRUE;

        InsertTailList( &AllocMgr->AllocatedList, &AllocEntry->TrackLink );
        InsertTailList( &AllocMgr->PreAllocatedFreeList, &AllocEntry->FreeLink );
    }

    if( i < AllocationCount )
    {
        MbbAllocMgrCleanup( AllocMgr );
        AllocMgr = NULL;
    }

    return AllocMgr;
}

VOID
MbbAllocMgrCleanup(
    __in __drv_freesMem(Mem) PVOID  AllocMgrHandle
    )
/*++
    Description
        The function assumes that all entries are already freed.
--*/
{
    PLIST_ENTRY             ListEntry;
    PLIST_ENTRY             NextEntry;
    PMBB_ALLOC_MANAGER      AllocMgr = (PMBB_ALLOC_MANAGER) AllocMgrHandle;
    PMBB_ALLOC_ENTRY        AllocEntry;

    NdisAcquireSpinLock( &AllocMgr->Lock );

    for( ListEntry  =  AllocMgr->PreAllocatedFreeList.Flink;
         ListEntry != &AllocMgr->PreAllocatedFreeList;
         ListEntry  =  NextEntry )
    {
        NextEntry = ListEntry->Flink;
        AllocEntry = CONTAINING_RECORD( ListEntry, MBB_ALLOC_ENTRY, FreeLink );
        RemoveEntryList( &AllocEntry->FreeLink );
        RemoveEntryList( &AllocEntry->TrackLink );
        NdisFreeToNPagedLookasideList( &AllocMgr->LookasideList, AllocEntry );
    }

    ASSERT( IsListEmpty( &AllocMgr->AllocatedList ) );

    NdisReleaseSpinLock( &AllocMgr->Lock );

    NdisDeleteNPagedLookasideList( &AllocMgr->LookasideList );
    NdisFreeSpinLock( &AllocMgr->Lock );

    FREE_POOL( AllocMgr );
}

PVOID
MbbAllocMgrAllocate(
    __in    PVOID   AllocMgrHandle
    )
{
    PLIST_ENTRY             ListEntry;
    PVOID                   AllocHandle = NULL;
    PMBB_ALLOC_MANAGER          AllocMgr = (PMBB_ALLOC_MANAGER) AllocMgrHandle;
    PMBB_ALLOC_ENTRY    AllocEntry = NULL;

    NdisAcquireSpinLock( &AllocMgr->Lock );

    // MbbAllocMgrFree puts the free entry to the head of the list, here we just allocate from the tail of the list
    if( (ListEntry = RemoveTailList( &AllocMgr->PreAllocatedFreeList )) != &AllocMgr->PreAllocatedFreeList )
    {
        AllocEntry  = CONTAINING_RECORD( ListEntry, MBB_ALLOC_ENTRY, FreeLink );
    }
    else if( AllocMgr->Flags.NoPostAllocation == 0 )
    {
        if( (AllocEntry = (PMBB_ALLOC_ENTRY) NdisAllocateFromNPagedLookasideList( &AllocMgr->LookasideList )) != NULL )
        {
            AllocEntry->AllocMgr        = AllocMgr;
            AllocEntry->IsPreAllocated  = FALSE;

            InsertTailList( &AllocMgr->AllocatedList, &AllocEntry->TrackLink );
        }
    }

    NdisReleaseSpinLock( &AllocMgr->Lock );

    if( AllocEntry != NULL )
        AllocHandle = AllocEntry + 1;

    return AllocHandle;
}

VOID
MbbAllocMgrFree(
    __in    PVOID   AllocHandle
    )
{
    PLIST_ENTRY         ListEntry;
    PMBB_ALLOC_ENTRY    AllocEntry = ((PMBB_ALLOC_ENTRY)AllocHandle) - 1;
    #pragma prefast(suppress: 26001, "Accessing abstracted header.")
    PMBB_ALLOC_MANAGER  AllocMgr   = AllocEntry->AllocMgr;

    NdisAcquireSpinLock( &AllocMgr->Lock );

    if( AllocEntry->IsPreAllocated )
    {
        InsertHeadList( &AllocMgr->PreAllocatedFreeList, &AllocEntry->FreeLink );
    }
    else
    {
        RemoveEntryList( &AllocEntry->TrackLink );
        NdisFreeToNPagedLookasideList( &AllocMgr->LookasideList, AllocEntry );
    }

    NdisReleaseSpinLock( &AllocMgr->Lock );
}


//
// BUFFER MANAGER
//

__drv_allocatesMem(Mem)
__drv_aliasesMem
PVOID
MbbBufMgrInitialize(
    __in  ULONG  BufferCount,
    __in  ULONG  BufferLength
    )
{
    PMBB_BUFFER_MANAGER BufferManager;
    MBB_ALLOC_FLAGS     AllocFlags = { 0 };

    if( (BufferManager = (PMBB_BUFFER_MANAGER) ALLOCATE_NONPAGED_POOL( sizeof(MBB_BUFFER_MANAGER) )) == NULL )
    {
        return NULL;
    }
    BufferManager->BufferLength = BufferLength;

    AllocFlags.NoPostAllocation = 1;

    if( (BufferManager->AllocMgrHandle =  MbbAllocMgrInitialize(
                                                BufferLength + sizeof(MBB_BUFFER_CONTEXT),
                                                BufferCount,
                                                MbbPoolTagDefault,
                                                AllocFlags
                                                )) == NULL )
    {
        TraceError( WMBCLASS_UTIL, "[BufMgr] FAILED to initialize allocation manager" );
        MbbBufMgrCleanup( BufferManager );
        BufferManager = NULL;
    }

    return BufferManager;
}

VOID
MbbBufMgrCleanup(
    __in __drv_freesMem(Mem) PVOID ManagerHandle
    )
{
    PMBB_BUFFER_MANAGER BufferManager = (PMBB_BUFFER_MANAGER)ManagerHandle;

    if( BufferManager->AllocMgrHandle != NULL )
        MbbAllocMgrCleanup( BufferManager->AllocMgrHandle );

    FREE_POOL( BufferManager );
}

PVOID
MbbBufMgrAllocateBuffer(
    __in PVOID ManagerHandle
    )
{
    PVOID               BufferHandle = NULL;
    PMBB_BUFFER_MANAGER BufferManager = (PMBB_BUFFER_MANAGER)ManagerHandle;
    PMBB_BUFFER_CONTEXT BufferEntry = NULL;

    if( (BufferEntry = (PMBB_BUFFER_CONTEXT) MbbAllocMgrAllocate( BufferManager->AllocMgrHandle )) != NULL )
    {
        BufferEntry->BufferManager = BufferManager;
        BufferHandle = BufferEntry + 1;
    }
    return BufferHandle;
}

VOID
MbbBufMgrFreeBuffer(
    __in PVOID BufferHandle
    )
{
    PMBB_BUFFER_CONTEXT BufferEntry = ((PMBB_BUFFER_CONTEXT)BufferHandle) - 1;

    MbbAllocMgrFree( BufferEntry );
}

FORCEINLINE
ULONG
MbbBufMgrGetBufferLength(
    __in PVOID BufferHandle
    )
{
    PMBB_BUFFER_CONTEXT BufferEntry = ((PMBB_BUFFER_CONTEXT)BufferHandle) - 1;

    #pragma prefast(suppress: 26001, "Accessing abstracted header.")
    return BufferEntry->BufferManager->BufferLength;
}

FORCEINLINE
VOID
MbbBufMgrSetBufferContext(
    __in PVOID BufferHandle,
    __in PVOID Context
    )
{
    PMBB_BUFFER_CONTEXT BufferEntry = ((PMBB_BUFFER_CONTEXT)BufferHandle) - 1;

    #pragma prefast(suppress: 26001, "Accessing abstracted header.")
    BufferEntry->OwnerContext = Context;
}

FORCEINLINE
PVOID
MbbBufMgrGetBufferContext(
    __in PVOID BufferHandle
    )
{
    PMBB_BUFFER_CONTEXT BufferEntry = ((PMBB_BUFFER_CONTEXT)BufferHandle) - 1;

    #pragma prefast(suppress: 26001, "Accessing abstracted header.")
    return BufferEntry->OwnerContext;
}


//
// WORK ITEMS
//

__drv_allocatesMem(Mem)
__drv_aliasesMem
PVOID
MbbWorkMgrInitialize(
    __in ULONG  InitialWorkItemCount
    )
{
    BOOLEAN                 Success = FALSE;
    NTSTATUS                NtStatus;
    PMBB_WORKITEM_MANAGER   WorkItemManager;
    MBB_ALLOC_FLAGS         AllocFlags = { 0 };

    do
    {
        if( (WorkItemManager = (PMBB_WORKITEM_MANAGER) ALLOCATE_NONPAGED_POOL( sizeof(MBB_WORKITEM_MANAGER) )) == NULL )
        {
            TraceError( WMBCLASS_UTIL, "[WorkMgr] FAILED to allocate work item manager" );
            break;
        }

        InitializeListHead( &WorkItemManager->WorkItemQueue );

        NdisAllocateSpinLock( &WorkItemManager->Lock );

        KeInitializeEvent(
            &(WorkItemManager->WorkItemEvent),
            NotificationEvent,
            FALSE
            );
        KeInitializeEvent(
            &(WorkItemManager->ThreadExitEvent),
            NotificationEvent,
            FALSE
            );
        WorkItemManager->ThreadHandle = NULL;

        if( (WorkItemManager->AllocMgrHandle = MbbAllocMgrInitialize(
                                                    sizeof(MBB_WORKITEM_ENTRY),
                                                    InitialWorkItemCount,
                                                    MbbPoolTagDefault,
                                                    AllocFlags
                                                    )) == NULL )
        {
            TraceError( WMBCLASS_UTIL, "[WorkMgr] FAILED to initialize allocation manager" );
            break;
        }

        NtStatus = PsCreateSystemThread(
                        &(WorkItemManager->ThreadHandle),
                        STANDARD_RIGHTS_REQUIRED,
                        NULL,
                        NULL,
                        NULL,
                        MbbWorkMgrProcessWorkItem,
                        WorkItemManager
                        );
        if( ! NT_SUCCESS( NtStatus ) )
        {
            TraceError( WMBCLASS_UTIL, "[WorkMgr] FAILED to create worker thread, status=%!status!", NtStatus );
            break;
        }
        Success = TRUE;
    }
    while( FALSE );

    if( !Success && WorkItemManager )
    {
        MbbWorkMgrCleanup( WorkItemManager );
        WorkItemManager = NULL;
    }
    return WorkItemManager;
}

VOID
MbbWorkMgrCleanup(
    __in __drv_freesMem(Mem) PVOID  WorkItemManagerHandle
    )
{
    NTSTATUS                NtStatus;
    PMBB_WORKITEM_MANAGER   WorkItemManager = (PMBB_WORKITEM_MANAGER)WorkItemManagerHandle;
    PVOID                   ThreadObject;

    //
    // Ask the event thread to cleanup and exit.
    //
    NdisAcquireSpinLock( &WorkItemManager->Lock );
    KeSetEvent( &WorkItemManager->ThreadExitEvent, IO_NO_INCREMENT, FALSE );
    NdisReleaseSpinLock( &WorkItemManager->Lock );
    TraceInfo( WMBCLASS_UTIL, "[WorkMgr] Waiting for worker thread to exit" );
    //
    // Wait for the worker thread to exit
    //
    if( WorkItemManager->ThreadHandle != NULL )
    {
        NtStatus = ObReferenceObjectByHandle(
                        WorkItemManager->ThreadHandle,
                        STANDARD_RIGHTS_REQUIRED,
                        *PsThreadType,
                        KernelMode,
                        &ThreadObject,
                        NULL
                        );
        if( NT_SUCCESS( NtStatus ) )
        {
            NtStatus = KeWaitForSingleObject(
                            ThreadObject,
                            Executive,
                            KernelMode,
                            TRUE,
                            NULL
                            );
            if( NtStatus != STATUS_WAIT_0 )
            {
                TraceError( WMBCLASS_UTIL, "[WorkMgr] FAILED to wait for worker thread with status=%!status!", NtStatus );
            }
            ObDereferenceObject( ThreadObject );
        }
        else
        {
            TraceError( WMBCLASS_UTIL, "[WorkMgr] FAILED to reference worker thread with status=%!status!", NtStatus );
        }
        ZwClose( WorkItemManager->ThreadHandle );
    }

    ASSERT( IsListEmpty( &WorkItemManager->WorkItemQueue ) );

    if( WorkItemManager->AllocMgrHandle != NULL )
        MbbAllocMgrCleanup( WorkItemManager->AllocMgrHandle );

    NdisFreeSpinLock( &WorkItemManager->Lock );

    FREE_POOL( WorkItemManager );
}

NDIS_STATUS
MbbWorkMgrQueueWorkItem(
    __in     PVOID                  WorkItemManagerHandle,
    __in_opt PVOID                  Context1,
    __in_opt PVOID                  Context2,
    __in_opt PVOID                  Context3,
    __in_opt PVOID                  Context4,
    __in     MBB_WORKITEM_ROUTINE   Routine
    )
{
    NDIS_STATUS             NdisStatus = NDIS_STATUS_RESOURCES;
    PMBB_WORKITEM_ENTRY     WorkItemEntry = NULL;
    PMBB_WORKITEM_MANAGER   WorkItemManager = (PMBB_WORKITEM_MANAGER)WorkItemManagerHandle;

    if( (WorkItemEntry = MbbAllocMgrAllocate( WorkItemManager->AllocMgrHandle )) != NULL )
    {
        WorkItemEntry->Context1 = Context1;
        WorkItemEntry->Context2 = Context2;
        WorkItemEntry->Context3 = Context3;
        WorkItemEntry->Context4 = Context4;
        WorkItemEntry->Routine  = Routine;

        NdisAcquireSpinLock( &WorkItemManager->Lock );
        InsertTailList( &WorkItemManager->WorkItemQueue, &WorkItemEntry->QueueLink );
        KeSetEvent( &WorkItemManager->WorkItemEvent, IO_NO_INCREMENT, FALSE );
        NdisReleaseSpinLock( &WorkItemManager->Lock );
        NdisStatus = NDIS_STATUS_SUCCESS;
    }

    return NdisStatus;
}

__drv_sameIRQL
__drv_functionClass(KSTART_ROUTINE)
KSTART_ROUTINE MbbWorkMgrProcessWorkItem;

__drv_sameIRQL
__drv_functionClass(KSTART_ROUTINE)
VOID
MbbWorkMgrProcessWorkItem( 
    __in PVOID  StartContext
    )
{
    NTSTATUS                NtStatus;
    PKEVENT                 EventArray[2];
    LIST_ENTRY              ListHead;
    PLIST_ENTRY             ListEntry;
    PMBB_WORKITEM_ENTRY     WorkItemEntry;
    PMBB_WORKITEM_MANAGER   WorkItemManager = (PMBB_WORKITEM_MANAGER)StartContext;

    EventArray[0] = &(WorkItemManager->ThreadExitEvent);
    EventArray[1] = &(WorkItemManager->WorkItemEvent);

    while( TRUE )
    {
        NtStatus = KeWaitForMultipleObjects(
                        2,
                        EventArray,
                        WaitAny,
                        Executive,
                        KernelMode,
                        TRUE,
                        NULL,
                        NULL
                        );
        if( NtStatus == STATUS_WAIT_0 )
        {
            //
            // Thread exit
            //
            break;
        }
        else if( NtStatus == STATUS_WAIT_1 )
        {
            InitializeListHead( &ListHead );
            NdisAcquireSpinLock( &WorkItemManager->Lock );
            InsertTailList( &WorkItemManager->WorkItemQueue, &ListHead );
            RemoveEntryList( &WorkItemManager->WorkItemQueue );
            InitializeListHead( &WorkItemManager->WorkItemQueue );
            KeResetEvent( &WorkItemManager->WorkItemEvent );
            NdisReleaseSpinLock( &WorkItemManager->Lock );

            for( ListEntry  =  RemoveHeadList( &ListHead );
                 ListEntry != &ListHead;
                 ListEntry  =  RemoveHeadList( &ListHead ) )
            {
                WorkItemEntry = CONTAINING_RECORD( ListEntry, MBB_WORKITEM_ENTRY, QueueLink );

                WorkItemEntry->Routine(
                    WorkItemEntry->Context1,
                    WorkItemEntry->Context2,
                    WorkItemEntry->Context3,
                    WorkItemEntry->Context4
                    );

                MbbAllocMgrFree( WorkItemEntry );
            }
        }
    }
    PsTerminateSystemThread( STATUS_SUCCESS );
}


//
// CID MESSAGE FRAGMENTS
//


NDIS_STATUS
MbbUtilSetupCommandMessage(
    __in    PMBB_REQUEST_CONTEXT                Request,
    __in    PMBB_COMMAND                        Command,
    __in    MBB_COMMAND_TYPE                    CommandType,
    __in_bcount_opt(InputBufferLength) PUCHAR   InputBuffer,
    __in    ULONG                               InputBufferLength
    )
{
    ULONG           FragmentCount;
    ULONG           FragmentDataLength;
    ULONG           FragmentLength;
    NTSTATUS        NtStatus;
    NDIS_STATUS     NdisStatus = NDIS_STATUS_SUCCESS;

    do
    {
        //
        // OS supported commands are sent over the control pipe.
        //
        FragmentLength      = Request->RequestManager->Fragmentation.ControlFragmentLength;
        FragmentDataLength  = FragmentLength;

        //
        // Query the amount of fragment buffers required for the command message.
        //
        FragmentCount   = 0;

        if( (NtStatus = MbbLibFragmentBufferToCommandMessages(
                            Command,
                            CommandType,
                            InputBuffer,
                            InputBufferLength,
                           &Request->TransactionId,
                            0,      // StartingFragment
                           &FragmentCount,
                           &FragmentDataLength,
                            NULL    // FragmentBuffers
                            )) != STATUS_SUCCESS )
        {
            NdisStatus = NDIS_STATUS_FAILURE;
            break;
        }
        Request->HandlerContext.Command.Data            = InputBuffer;
        Request->HandlerContext.Command.DataLength      = InputBufferLength;
        Request->HandlerContext.Command.Command         = *Command;
        Request->HandlerContext.Command.CommandType     = CommandType;
        Request->HandlerContext.Command.FragmentCount   = FragmentCount;
        Request->HandlerContext.Command.FragmentLength  = FragmentLength;
    }
    while( FALSE );

    return NdisStatus;
}

NDIS_STATUS
MbbUtilSendMessageFragmentsAndLog(
    __in PMBB_REQUEST_CONTEXT Request
    )
{
    PCHAR CommandName = MbbUtilGetCommandString(&(Request->HandlerContext.Command.Command));
    NTSTATUS Status = STATUS_SUCCESS;
    LPGUID ActivityId = NULL;
    MBB_MESSAGE_TYPE MessageType = MBB_MESSAGE_TYPE_COMMAND;
    ULONG MessageLength = Request->HandlerContext.Command.DataLength + sizeof(MBB_COMMAND_HEADER);
    DWORD TotalFragments = 1;
    DWORD CurrentFragment = 0;

    // Create an activity Id to be associated with this message
    // This activity Id will be associated with the USB IRP for sending
    // this message so that we can associate USB traces with our traces.
    // Also note that there is one activity Id for sending all the fragments 
    // of a message.

    Status = MbbEventActivityIdCtl(&(Request->ActivityId));

    if(NT_SUCCESS(Status)) 
    {
        ActivityId = &(Request->ActivityId);
    }
    
    // Etw Trace send   
    MbbWriteEvent(
       &CID_COMMAND_SEND,
       ActivityId,
       NULL,
       7,
      &(Request->OidContext.OidRequestId),
       sizeof(Request->OidContext.OidRequestId),
      &(Request->RequestId),
       sizeof(ULONG),
      &(Request->HandlerContext.Command.Command.ServiceId),
       sizeof(GUID),
       CommandName,
       strlen(CommandName) + 1,
      &(Request->HandlerContext.Command.Command.CommandId),
       sizeof(ULONG),
      &(Request->HandlerContext.Command.DataLength),
       sizeof(ULONG),
       Request->HandlerContext.Command.Data,
       Request->HandlerContext.Command.DataLength
       );   
    
    // Etw Trace for MBB OPN. Log an event that renders the entire MBIM message   
    MbbWriteEventOpn(
        &COMMAND_MSG,
        ActivityId,
        NULL,
        10,
        &MessageType,
        sizeof(MessageType),
        &MessageLength,
        sizeof(MessageLength),
        &(Request->TransactionId),
        sizeof(Request->TransactionId),
        &TotalFragments,
        sizeof(TotalFragments),
        &CurrentFragment,
        sizeof(CurrentFragment),
        &(Request->HandlerContext.Command.Command.ServiceId),
        sizeof(Request->HandlerContext.Command.Command.ServiceId),
        &(Request->HandlerContext.Command.Command.CommandId),
        sizeof(Request->HandlerContext.Command.Command.CommandId),
        &(Request->HandlerContext.Command.CommandType),
        sizeof(Request->HandlerContext.Command.CommandType),
        &(Request->HandlerContext.Command.DataLength),
        sizeof(Request->HandlerContext.Command.DataLength),
        Request->HandlerContext.Command.Data,
        Request->HandlerContext.Command.DataLength
        );
    
    return MbbUtilSendMessageFragments( Request );
}


NDIS_STATUS
MbbUtilSendMessageFragments(
    __in PMBB_REQUEST_CONTEXT Request
    )
/*++
    Synchronization:
        Fields updated by the SendCompletion routine
        and the Send routine are protected by the 
        RequestManager lock e.g. NextFrameIndex.

        Some fields are static and need no protection
        e.g. FragmentCount.

        At any time only one instance of the Send
        routine is processing fragments since the
        routines are synchronized by the RequestManager lock.

    Return Value:
        NDIS_STATUS_SUCCESS
            Successfully sent all fragments, SendComplete event is queued to the request.
        NDIS_STATUS_FAILURE
            Failed to send all fragments, SendComplete event is queued to the request.
        NDIS_STATUS_PENDING
            SendComplete event will be queued later.
--*/
{
    ULONG                   FragmentIndex;
    ULONG                   FragmentLength;
    ULONG                   FragmentCount;
    ULONG                   FragmentSentCount;
    ULONG                   FragmentBufferCount;
    PVOID                   BufferManagerHandle;
    PCHAR                   FragmentBuffer;
    NTSTATUS                NtStatus;
    NDIS_STATUS             NdisStatus;
    PMBB_REQUEST_MANAGER    RequestManager; 

    //
    // Cache values for easy access.
    //
    RequestManager      = Request->RequestManager;
    BufferManagerHandle = RequestManager->Fragmentation.BufferManager;
    FragmentCount       = Request->HandlerContext.Command.FragmentCount;
    //
    // Synchronize with other instances of the same routine
    // invoked from completion handlers.
    //
    MbbReqMgrLockManager( RequestManager );
    if( Request->HandlerContext.Command.IsProcessing == TRUE )
    {
        MbbReqMgrUnlockManager( RequestManager );
        return NDIS_STATUS_PENDING;
    }
    Request->HandlerContext.Command.IsProcessing = TRUE;
    //
    // If some fragments failed to be sent then no point
    // sending more. Fail the request right away.
    //
    FragmentSentCount = 0;
    NdisStatus        = Request->HandlerContext.Command.SendStatus;

    for( FragmentIndex = Request->HandlerContext.Command.NextFragmentIndex;
         FragmentIndex < FragmentCount && NdisStatus == NDIS_STATUS_SUCCESS;
         FragmentIndex ++ )
    {
        //
        // Allocate fragment buffer. If there are no buffers then wait
        // for the SendCompletion routine to return the buffers.
        //
        if( (FragmentBuffer= MbbBufMgrAllocateBuffer( BufferManagerHandle )) == NULL )
        {
            //
            // If the first fragment couldnt be sent then fail the send.
            // This should never happen, this is an indication of a buffer leak.
            //
            if( FragmentIndex == 0 )
            {
                ASSERT( FragmentBuffer != NULL );
                NdisStatus = NDIS_STATUS_RESOURCES;
            }
            break;
        }
        //
        // Format fragments one at a time.
        //
        FragmentLength       = Request->HandlerContext.Command.FragmentLength;
        FragmentBufferCount  = 1;

        ASSERT( FragmentLength <= MbbBufMgrGetBufferLength( FragmentBuffer ) );

        NtStatus = MbbLibFragmentBufferToCommandMessages(
                       &Request->HandlerContext.Command.Command,
                        Request->HandlerContext.Command.CommandType,
                        Request->HandlerContext.Command.Data,
                        Request->HandlerContext.Command.DataLength,
                       &Request->TransactionId,
                        FragmentIndex,
                       &FragmentBufferCount,
                       &FragmentLength,
                       &FragmentBuffer
                        );
        if( NtStatus != STATUS_SUCCESS &&
            NtStatus != STATUS_MORE_PROCESSING_REQUIRED )
        {
            NdisStatus = NDIS_STATUS_FAILURE;
            break;
        }
        //
        // Ensure library formatted the data correctly
        //
        ASSERT( FragmentBufferCount == 1 );
        ASSERT( FragmentLength <= Request->HandlerContext.Command.FragmentLength );
        //
        // Save context required by the completion handler.
        //
        MbbBufMgrSetBufferContext( FragmentBuffer, Request );
        //
        // Call the wrapper that will sent the fragment over the control or bulk pipe.
        // Drop the lock before calling in to the bus layer.
        //
        MbbReqMgrUnlockManager( RequestManager );

        NtStatus = MbbUtilBusSendMessageFragment(
                        Request,
                        (MBB_REQUEST_HANDLE)FragmentBuffer,
                        FragmentBuffer,
                        FragmentLength
                        );
        if( NtStatus != STATUS_PENDING )
        {
            MbbBufMgrFreeBuffer( FragmentBuffer );
        }

        if( NtStatus == STATUS_SUCCESS )
        {
            TraceInfo(  WMBCLASS_OID, "[Util][ReqId=0x%04x][TID=0x%08x] Sent Fragment %02d/%02d",
                        Request->RequestId, Request->TransactionId, FragmentIndex, FragmentCount
                        );
            FragmentSentCount++;
        }
        else if( NtStatus != STATUS_PENDING )
        {
            TraceError( WMBCLASS_OID, "[Util][ReqId=0x%04x][TID=0x%08x] FAILED to send Fragment %02d/%02d status=%!status!",
                        Request->RequestId, Request->TransactionId, FragmentIndex, FragmentCount, NtStatus
                        );
            NdisStatus = NDIS_STATUS_FAILURE;
        }
        else
        {
            TraceInfo(  WMBCLASS_OID, "[Util][ReqId=0x%04x][TID=0x%08x] Pending send Fragment %02d/%02d",
                        Request->RequestId, Request->TransactionId, FragmentIndex, FragmentCount
                        );
        }
        //
        // Reacquire lock for evaluating the queue
        //
        MbbReqMgrLockManager( RequestManager );
    } // for

    //
    // On failure, update the status field.
    // Dont send any more fragments, fake remaining fragments are sent.
    //
    if( NdisStatus != NDIS_STATUS_SUCCESS )
    {
        Request->HandlerContext.Command.SendStatus = NdisStatus;
        FragmentSentCount += FragmentCount - FragmentIndex;
    }
    //
    // Update the Request fields
    // Protected by RequestManager lock.
    //
    Request->HandlerContext.Command.NextFragmentIndex  = FragmentIndex;
    Request->HandlerContext.Command.FragmentSentCount += FragmentSentCount;
    Request->HandlerContext.Command.IsProcessing       = FALSE;
    //
    // If any fragments were completed synchronously then check if
    // the request can move to SendComplete
    //
    if( FragmentSentCount > 0 )
        FragmentSentCount = Request->HandlerContext.Command.FragmentSentCount;
    MbbReqMgrUnlockManager( RequestManager );
    //
    // Queue SendComplete event if no fragments are pending
    //
    if( FragmentSentCount == FragmentCount )
    {
        PCHAR CommandName = MbbUtilGetCommandString(&(Request->HandlerContext.Command.Command));

        // Etw Trace send complete
        MbbWriteEvent(
            &CID_SEND_COMPLETE,
            &(Request->ActivityId),
            NULL,
            8,
           &(Request->OidContext.OidRequestId),
            sizeof(Request->OidContext.OidRequestId),
           &(Request->RequestId),
            sizeof(ULONG),
           &(Request->HandlerContext.Command.Command.ServiceId),
            sizeof(GUID),
            CommandName,
            strlen(CommandName) + 1,
           &(Request->HandlerContext.Command.Command.CommandId),
            sizeof(ULONG),
           &(Request->HandlerContext.Command.DataLength),
            sizeof(ULONG),
            Request->HandlerContext.Command.Data,
            Request->HandlerContext.Command.DataLength,
            &NdisStatus,
            sizeof(NDIS_STATUS)
            );
        
        MbbReqMgrQueueEvent(
            Request->RequestManager,
            Request,
            MbbRequestEventSendComplete,
            (PVOID)(NdisStatus),
            0
            );
    }
    else
    {
        NdisStatus = NDIS_STATUS_PENDING;
    }

    return NdisStatus;
}

NTSTATUS
MbbUtilBusSendMessageFragment(
    __in    PMBB_REQUEST_CONTEXT    Request,
    __in    MBB_REQUEST_HANDLE      FragmentBuffer,
    __in    PVOID                   MessageFragment,
    __in    ULONG                   FragmentLength
    )
{
    PMDL            Mdl;
    NTSTATUS        NtStatus;
    MBB_BUS_HANDLE  BusHandle = MbbNdisGetBusHandle( MbbReqMgrGetAdapterHandle( Request ) );
    
    //
    // OS supported commands are sent over the control pipe.
    // Vendor device service commands are send also sent over the control pipe
    //
    NtStatus = MbbBusSendMessageFragment(
                    BusHandle,
                    FragmentBuffer,
                    MessageFragment,
                    FragmentLength,
                     !IsZeroGuid(&(Request->ActivityId)) ? &(Request->ActivityId) : NULL,
                    MbbUtilSendMessageFragmentComplete
                    );

    return NtStatus;
}

VOID
MbbUtilSendMessageFragmentComplete(
    __in    MBB_PROTOCOL_HANDLE     ProtocolHandle,
    __in    MBB_REQUEST_HANDLE      FragmentBuffer,
    __in    NTSTATUS                NtStatus
    )
{
    ULONG                   FragmentCount;
    ULONG                   FragmentSentCount;
    NDIS_STATUS             NdisStatus;
    PMBB_REQUEST_CONTEXT    Request;
    PMBB_REQUEST_MANAGER    RequestManager;

    if( NtStatus == STATUS_SUCCESS )
        NdisStatus = NDIS_STATUS_SUCCESS;
    else
        NdisStatus = NDIS_STATUS_FAILURE;

    Request             = (PMBB_REQUEST_CONTEXT) MbbBufMgrGetBufferContext( FragmentBuffer );
    RequestManager      = Request->RequestManager;
    FragmentCount       = Request->HandlerContext.Command.FragmentCount;
    //
    // Update fields that need synchronization.
    // Note the buffer should be returned with RequestManager lock held.
    //
    MbbReqMgrLockManager( RequestManager );
    MbbBufMgrFreeBuffer( FragmentBuffer );
    FragmentSentCount = ++Request->HandlerContext.Command.FragmentSentCount;
    if( NdisStatus != NDIS_STATUS_SUCCESS )
        Request->HandlerContext.Command.SendStatus = NdisStatus;
    MbbReqMgrUnlockManager( RequestManager );

    TraceInfo(  WMBCLASS_OID, "[Util][ReqId=0x%04x][TID=0x%08x] %02d/%02d fragment completed with status=%!status!",
                Request->RequestId, Request->TransactionId, FragmentSentCount, FragmentCount, NdisStatus
                );
    //
    // If all fragments have completed sending then complete the request.
    // If not, then try to send more fragments.
    //
    if( FragmentSentCount == FragmentCount )
    {
        PCHAR CommandName = MbbUtilGetCommandString(&(Request->HandlerContext.Command.Command));       
        
        // Etw Trace send complete
        MbbWriteEvent(
            &CID_SEND_COMPLETE,
            &(Request->ActivityId),
            NULL,
            8,
           &(Request->OidContext.OidRequestId),
            sizeof(Request->OidContext.OidRequestId),
           &(Request->RequestId),
            sizeof(ULONG),
           &(Request->HandlerContext.Command.Command.ServiceId),
            sizeof(GUID),
            CommandName,
            strlen(CommandName) + 1,
           &(Request->HandlerContext.Command.Command.CommandId),
            sizeof(ULONG),
           &(Request->HandlerContext.Command.DataLength),
            sizeof(ULONG),
            Request->HandlerContext.Command.Data,
            Request->HandlerContext.Command.DataLength,
            &NdisStatus,
            sizeof(NDIS_STATUS)
            );
     
        MbbReqMgrQueueEvent(
            Request->RequestManager,
            Request,
            MbbRequestEventSendComplete,
            (PVOID)(NdisStatus),
            0
            );

        if (NtStatus == STATUS_IO_TIMEOUT && Request->OidHandler != NULL)
        {
            PMBB_SEND_QUEUE SendQueue = &((PMINIPORT_ADAPTER_CONTEXT)MbbReqMgrGetAdapterHandle(Request))->SendQueue;
            switch (Request->OidHandler->Oid)
            {
            case OID_WWAN_CONNECT:
                TraceLoggingWrite(
                    g_hLoggingProvider,
                    "ConnectSendTimeout",
                    TraceLoggingKeyword(MICROSOFT_KEYWORD_MEASURES));
                TryQueueStallState(SendQueue);
                break;
            // Remove OID_WWAN_PIN_EX since there is an observed case that the OID_WWAN_PIN_EX failed but data path is still working
            // case OID_WWAN_PIN_EX: __fallthrough;
            //case OID_WWAN_RADIO_STATE:
            //    TraceLoggingWrite(
            //        g_hLoggingProvider,
            //        "RadioStateSendTimeout",
            //        TraceLoggingKeyword(MICROSOFT_KEYWORD_MEASURES));
            //    TryQueueStallState(SendQueue);
            //    break;
            }
        }
    }
    else
    {
        //
        // All fragments might be already send but some may be inflight.
        // If all fragments are sent then below call will be a no-op.
        //
        MbbUtilSendMessageFragments( Request );
    }
}

VOID
MbbUtilSendMbimErrorComplete(
    __in    MBB_PROTOCOL_HANDLE     ProtocolHandle,
    __in    MBB_REQUEST_HANDLE      SendBuffer,
    __in    NTSTATUS                NtStatus
    )
{
    PMBB_ERROR_MESSAGE ErrorMessage = (PMBB_ERROR_MESSAGE)SendBuffer;

    if( ! NT_SUCCESS( NtStatus ) )
    {
        TraceError( WMBCLASS_UTIL, "[Util][MBIM_ERR][TID=0x%08x] FAILED to send error message to device, Error=%!MbbError!",
                    ErrorMessage->MessageHeader.MessageTransactionId,
                    ErrorMessage->ErrorCode
                    );
    }
    FREE_POOL( ErrorMessage );
}

VOID
MbbUtilSendMbimError(
    __in ULONG                      TransactionId,
    __in MBB_ERROR                  ErrorCode,
    __in PMINIPORT_ADAPTER_CONTEXT  Adapter,
    __in GUID                       ActivityId,
    __in PMBB_COMMAND               Command
    )
/*++
Description
    Helper routine to send a MBB error message to the MBIM device.
    This also logs an ETW event in the event log.
--*/
{
    NTSTATUS                NtStatus;
    PCHAR                   CommandName;
    PMBB_ERROR_MESSAGE      ErrorMessage;

#pragma prefast(suppress: __WARNING_MEMORY_LEAK, "Released by MbbUtilSendMbimErrorComplete")
    if( (ErrorMessage = (PMBB_ERROR_MESSAGE) ALLOCATE_NONPAGED_POOL( sizeof(MBB_ERROR_MESSAGE) )) == NULL )
    {
        TraceError( WMBCLASS_UTIL, "[MBIM_ERR][TID=0x%08x] FAILED to allocate error message for, Error=%!MbbError!",
                    TransactionId,
                    ErrorCode
                    );
        return;
    }

    ErrorMessage->MessageHeader.MessageType         = MBB_MESSAGE_TYPE_HOST_ERROR;
    ErrorMessage->MessageHeader.MessageLength       = sizeof(*ErrorMessage);
    ErrorMessage->MessageHeader.MessageTransactionId= TransactionId;
    ErrorMessage->ErrorCode                         = ErrorCode;

    TraceError( WMBCLASS_UTIL, "[MBIM_ERR][TID=0x%08x] Sending MBIM error message for %!MbbError! to device",
                TransactionId,
                ErrorCode
                );

    NtStatus = MbbBusSendMessageFragment(
                MbbNdisGetBusHandle( Adapter ),
                ErrorMessage,
                ErrorMessage,
                sizeof(*ErrorMessage),
                !IsZeroGuid(&ActivityId) ? &ActivityId : NULL,
                MbbUtilSendMbimErrorComplete
                );
    if( ! NT_SUCCESS( NtStatus ) )
    {
        TraceError( WMBCLASS_UTIL, "[MBIM_ERR][TID=0x%08x] FAILED to send error message to bus layer, Error=%!MbbError!", 
                    TransactionId,
                    ErrorCode
                    );
    }

    if( NtStatus != STATUS_PENDING )
    {
        MbbUtilSendMbimErrorComplete(
            NULL,
            ErrorMessage,
            NtStatus
            );
    }

    CommandName = MbbUtilGetCommandString( Command );

    MbbWriteEvent(
       &MBIM_HOST_ERROR_EVENT,
        NULL,
        NULL,
        6,
       &Adapter->TraceInstance,
        sizeof(Adapter->TraceInstance),
       &ErrorCode,
        sizeof(MBB_ERROR),
       &TransactionId,
        sizeof(ULONG),
       &Command->ServiceId,
        sizeof(Command->ServiceId),
       &Command->CommandId,
        sizeof(Command->CommandId),
        CommandName,
        strlen(CommandName) + 1
        );
}


//
// CID QUERY \ SET WRAPPER
//


NDIS_STATUS
MbbUtilInternalCIDQuery(
    __in MBB_PROTOCOL_HANDLE                AdapterHandle,
    __in PMBB_REQUEST_CONTEXT               Request
    )
/*++
    Description
        This is the internal serialized routine that calls CID handlers.
        Only one instance of this routine will run at any time.
        This is different than MbbNdisProcessSerializeOidRequest( )
        in that it is not related to OIDs. Internal request submissions
        that do not have a OID use this wrapper routine as their dispatch routine.

    Parameters
        __in  PMBB_REQUEST_CONTEXT  Request
            The request to process. This is the internal request representation.
--*/
{
    TraceInfo(  WMBCLASS_OID, "[Util][ReqId=0x%04x] Internal query dispatch for CID %s",
                Request->RequestId,
                MbbUtilGetCommandString( &(Request->OidHandler->ResponseCommand) )
                );

    return Request->OidHandler->QueryHandler(
                                    Request,
                                    NULL,
                                    NULL,
                                    NULL,
                                    NULL
                                    );
}

VOID
MbbUtilInternalCIDCompletion(
    __in    MBB_PROTOCOL_HANDLE     AdapterHandle,
    __in    PMBB_REQUEST_CONTEXT    Request,
    __in    NDIS_STATUS             NdisStatus
    )
/*++
    Description:
        This routine is used by internal CID requestors. This is the default
        handler for SendCompletion that handles common cleanup work.
--*/
{
    if( Request->OidHandler->CompletionHandler != NULL )
    {
        Request->OidHandler->CompletionHandler(
                                Request,
                                NdisStatus
                                );
    }

    TraceInfo(  WMBCLASS_OID, "[Util][ReqId=0x%04x] Internal SendComplete cleanup for CID %s. Status=%!status! DataToFree=0x%p",
                Request->RequestId,
                MbbUtilGetCommandString( &(Request->OidHandler->ResponseCommand) ),
                NdisStatus,
                Request->HandlerContext.DataToFreeOnCompletion
                );

    if( Request->HandlerContext.DataToFreeOnCompletion != NULL )
        FREE_POOL( Request->HandlerContext.DataToFreeOnCompletion );

    if( NdisStatus != NDIS_STATUS_SUCCESS )
    {
        if( Request->HandlerContext.DataToFreeOnResponse != NULL )
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
}

VOID
MbbUtilCommonCIDResponse(
    __in PMBB_REQUEST_CONTEXT               Request,
    __in NDIS_STATUS                        NdisStatus,
    __in MBB_STATUS                         MbbStatus,
    __in_bcount_opt(InBufferSize) PUCHAR    InBuffer,
    __in ULONG                              InBufferSize,
    __in_opt PMBB_OID_HANDLER_ENTRY         OidHandler
    )
{
    NDIS_STATUS HandlerStatus = NDIS_STATUS_SUCCESS;
    PCHAR ResponseType = (MbbReqMgrIsUnsolicitedIndication( Request )) ? "Unsolicited":"Solicited";
    PCHAR CommandName = NULL;
    GUID ServiceId = MBB_UUID_INVALID_CONSTANT;
    ULONG CommandId = 0;
    MBB_MESSAGE_TYPE MessageType;
    ULONG MessageLength;
    DWORD TotalFragments = 1;
    DWORD CurrentFragment = 0;

    if( OidHandler != NULL &&
        OidHandler->ResponseHandler != NULL )
    {
        HandlerStatus = OidHandler->ResponseHandler(
                                        Request,
                                        NdisStatus,
                                        MbbStatus,
                                        InBuffer,
                                        InBufferSize
                                        );
        //
        // Let the handler overwrite the status
        //
        NdisStatus = HandlerStatus;

        CommandName = MbbUtilGetCommandString(&OidHandler->ResponseCommand);
        ServiceId = OidHandler->ResponseCommand.ServiceId;
        CommandId = OidHandler->ResponseCommand.CommandId;
    }

    // Etw trace response received   
    MbbWriteEvent(
        &CID_RESPONSE_RECVD,
        &(Request->ActivityId),
        NULL,
        10,
        ResponseType,
        strlen(ResponseType) + 1,
       &(Request->OidContext.OidRequestId),
        sizeof(Request->OidContext.OidRequestId),
       &(Request->RequestId),
        sizeof(ULONG),
       &ServiceId,
        sizeof(GUID),
        CommandName,
        (CommandName == NULL) ? 0 : (strlen(CommandName) + 1),
       &CommandId,
        sizeof(ULONG),
       &MbbStatus,
        sizeof(MBB_STATUS),
       &NdisStatus,
        sizeof(NDIS_STATUS),
       &InBufferSize,
        sizeof(ULONG),
        InBuffer,
        InBufferSize
        );
    
    // Etw Trace for MBB OPN. Log an event that renders the entire MBIM message   
    if( MbbReqMgrIsUnsolicitedIndication( Request ) )
    {
        MessageType = MBB_MESSAGE_TYPE_INDICATE_STATUS;
        MessageLength = InBufferSize + sizeof(MBB_INDICATE_STATUS_HEADER);
        MbbWriteEventOpn(
            &INDICATE_STATUS_MSG,
            &(Request->ActivityId),
            NULL,
            9,
            &MessageType,
            sizeof(MessageType),
            &MessageLength,
            sizeof(MessageLength),
            &(Request->TransactionId),
            sizeof(Request->TransactionId),
            &TotalFragments,
            sizeof(TotalFragments),
            &CurrentFragment,
            sizeof(CurrentFragment),
            &(ServiceId),
            sizeof(ServiceId),
            &(CommandId),
            sizeof(CommandId),
            &InBufferSize,
            sizeof(InBufferSize),
            InBuffer,
            InBufferSize
            );
    }
    else
    {
        MessageType = MBB_MESSAGE_TYPE_COMMAND_DONE;
        MessageLength = InBufferSize + sizeof(MBB_COMMAND_DONE_HEADER);
        MbbWriteEventOpn(
            &COMMAND_DONE_MSG,
            &(Request->ActivityId),
            NULL,
            10,
            &MessageType,
            sizeof(MessageType),
            &MessageLength,
            sizeof(MessageLength),
            &(Request->TransactionId),
            sizeof(Request->TransactionId),
            &TotalFragments,
            sizeof(TotalFragments),
            &CurrentFragment,
            sizeof(CurrentFragment),
            &(ServiceId),
            sizeof(ServiceId),
            &(CommandId),
            sizeof(CommandId),
            &MbbStatus,
            sizeof(MbbStatus),
            &InBufferSize,
            sizeof(InBufferSize),
            InBuffer,
            InBufferSize
            );
    }
    
    //
    // If the handler wants to keep the request around for MoreData
    // then do not cleanup the request yet.
    //
    if( HandlerStatus == NDIS_STATUS_PENDING )
    {
        TraceInfo(  WMBCLASS_OID, "[Util][ReqId=0x%04x] Not cleaning CID %s",
                    Request->RequestId,
                    MbbUtilGetCommandString( &Request->HandlerContext.Response.Command )
                    );
    }
    else
    {
        TraceInfo(  WMBCLASS_OID, "[Util][ReqId=0x%04x] ResponseReceived cleanup for CID %s. Status=%!status! DataToFree=0x%p",
                    Request->RequestId,
                    MbbUtilGetCommandString( &Request->HandlerContext.Response.Command ),
                    NdisStatus,
                    Request->HandlerContext.DataToFreeOnResponse
                    );
        Request->HandlerContext.Response.NdisStatus = NdisStatus;

        if( Request->HandlerContext.DataToFreeOnResponse != NULL )
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
}

VOID
MbbUtilInternalCIDResponse(
    __in PMBB_REQUEST_CONTEXT               Request,
    __in NDIS_STATUS                        NdisStatus,
    __in MBB_STATUS                         MbbStatus,
    __in_bcount_opt(InBufferSize) PUCHAR    InBuffer,
    __in ULONG                              InBufferSize
    )
/*++
    Description:
        This routine is used by internal CID requestors. This is the deafult
        handler for ResposenReceivethat handles common cleanup work.

        Internal routine requires that there is an OID handler Which means
        that status indidcations are not supported on the internal path.
--*/
{
    MbbUtilCommonCIDResponse(
        Request,
        NdisStatus,
        MbbStatus,
        InBuffer,
        InBufferSize,
        Request->OidHandler
        );
}

NDIS_STATUS
MbbUtilQueryAttribute(
    __in PMBB_REQUEST_CONTEXT                                   Request,
    __in_bcount_opt(*InBufferSize) PUCHAR                       InBuffer,
    __in PULONG                                                 InBufferSize,
    __out_bcount_part_opt(*OutBufferSize,*OutBufferSize) PUCHAR OutBuffer,
    __inout PULONG                                              OutBufferSize
    )
{
    return MbbUtilQueryAttributeWithParameter(
            Request,
            NULL,
            0
            );
}

NDIS_STATUS
MbbUtilQueryAttributeWithParameter(
    __in PMBB_REQUEST_CONTEXT                           Request,
    __in_bcount_opt(GetRequestParametersSize) PUCHAR    GetRequestParameters,
    __in ULONG                                          GetRequestParametersSize
    )
{
    NDIS_STATUS             NdisStatus;

    do
    {
        //
        // Call the wrapper routine to allocate
        // and format the message buffers.
        //
        if( (NdisStatus = MbbUtilSetupCommandMessage(
                                Request,
                               &Request->OidHandler->ResponseCommand,
                                MBB_COMMAND_TYPE_QUERY,
                                GetRequestParameters,
                                GetRequestParametersSize
                                )) != NDIS_STATUS_SUCCESS )
        {
            TraceError( WMBCLASS_OID, "[Util][ReqId=0x%04x] FAILED to setup command message for %s with status=%!status!",
                        Request->RequestId, MbbUtilGetCommandString( &(Request->OidHandler->ResponseCommand) ), NdisStatus );
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
            TraceError( WMBCLASS_OID, "[Util][ReqId=0x%04x] FAILED to send message fragments for %s with status=%!status!",
                        Request->RequestId, MbbUtilGetCommandString( &(Request->OidHandler->ResponseCommand) ), NdisStatus );
        }

    } while( FALSE );

    return NdisStatus;
}

NDIS_STATUS
MbbUtilSetAttributeWithParameter(
    __in PMBB_REQUEST_CONTEXT                   Request,
    __in_bcount_opt(SetParametersSize) PUCHAR   SetParameters,
    __in ULONG                                  SetParametersSize
    )
{
    NDIS_STATUS             NdisStatus;

    do
    {
        //
        // Call the wrapper routine to allocate
        // and format the message buffers.
        //
        if( (NdisStatus = MbbUtilSetupCommandMessage(
                                Request,
                               &Request->OidHandler->ResponseCommand,
                                MBB_COMMAND_TYPE_SET,
                                SetParameters,
                                SetParametersSize
                                )) != NDIS_STATUS_SUCCESS )
        {
            TraceError( WMBCLASS_OID, "[Util][ReqId=0x%04x] FAILED to setup command message for %s with status=%!status!",
                        Request->RequestId, MbbUtilGetCommandString( &(Request->OidHandler->ResponseCommand) ), NdisStatus );
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
            TraceError( WMBCLASS_OID, "[Util][ReqId=0x%04x] FAILED to send message fragments for %s with status=%!status!",
                        Request->RequestId, MbbUtilGetCommandString( &(Request->OidHandler->ResponseCommand) ), NdisStatus );
        }

    } while( FALSE );

    return NdisStatus;
}

NDIS_STATUS
MbbUtilDeviceServiceCommand(
    __in PMBB_REQUEST_CONTEXT                   Request,
    __in_bcount_opt(*InBufferSize) PUCHAR       InBuffer,
    __in PULONG                                 InBufferSize
    )
{
    NDIS_STATUS                         NdisStatus;
    MBB_COMMAND                         MbbCommand;
    PNDIS_WWAN_DEVICE_SERVICE_COMMAND   NdisDeviceServiceCommand;

    do
    {
        if( *InBufferSize < sizeof(NDIS_WWAN_DEVICE_SERVICE_COMMAND) )
        {
            TraceError( WMBCLASS_OID, "[MbbNdis][Ven][ReqId=0x%04x] INSUFFICIENT InputBufferSize=%d Bytes for DEVICE_SERVICE_COMMAND, Expected=%d Bytes",
                        Request->RequestId, *InBufferSize, sizeof(NDIS_WWAN_DEVICE_SERVICE_COMMAND) );

            NdisStatus = NDIS_STATUS_INVALID_LENGTH;
            *InBufferSize = sizeof(NDIS_WWAN_DEVICE_SERVICE_COMMAND);
            break;
        }

        NdisDeviceServiceCommand = (PNDIS_WWAN_DEVICE_SERVICE_COMMAND) InBuffer;


        if( NdisDeviceServiceCommand->Header.Type != NDIS_OBJECT_TYPE_DEFAULT ||
            NdisDeviceServiceCommand->Header.Size < SIZEOF_NDIS_WWAN_DEVICE_SERVICE_COMMAND_1 ||
            NdisDeviceServiceCommand->Header.Revision < NDIS_WWAN_DEVICE_SERVICE_COMMAND_REVISION_1 )
        {
            TraceError( WMBCLASS_OID, "[MbbNdis][Ven][ReqId=0x%04x] INVALID ndis header for DEVICE_SERVICE_COMMAND, Type=0x%x Size=%d Revision=%d",
                        Request->RequestId,
                        NdisDeviceServiceCommand->Header.Type,
                        NdisDeviceServiceCommand->Header.Size,
                        NdisDeviceServiceCommand->Header.Revision
                        );
            NdisStatus = NDIS_STATUS_INVALID_DATA;
            break;
        }

        if (NdisDeviceServiceCommand->Command.uDataSize + RTL_SIZEOF_THROUGH_FIELD(NDIS_WWAN_DEVICE_SERVICE_COMMAND,Command.uDataSize) > *InBufferSize )
        {
            TraceError( WMBCLASS_OID, "[MbbNdis][Ven][ReqId=0x%04x] INSUFFICIENT InputBufferSize=%d Bytes for DEVICE_SERVICE_COMMAND, Expected=%d Bytes",
                        Request->RequestId, *InBufferSize, sizeof(NDIS_WWAN_DEVICE_SERVICE_COMMAND) );

            *InBufferSize = NdisDeviceServiceCommand->Command.uDataSize + RTL_SIZEOF_THROUGH_FIELD(NDIS_WWAN_DEVICE_SERVICE_COMMAND,Command.uDataSize);

            NdisStatus = NDIS_STATUS_INVALID_LENGTH;
            break;
        }

        RtlCopyMemory(
            &MbbCommand.ServiceId,
            &NdisDeviceServiceCommand->Command.DeviceServiceGuid,
            sizeof(GUID)
            );

        MbbCommand.CommandId = NdisDeviceServiceCommand->Command.CommandID;
        //
        // Call the wrapper routine to allocate
        // and format the message buffers.
        //
        if( (NdisStatus = MbbUtilSetupCommandMessage(
                                Request,
                               &MbbCommand,
                                MbbReqMgrIsSetOid( Request )? MBB_COMMAND_TYPE_SET: MBB_COMMAND_TYPE_QUERY,
                                (PUCHAR)(NdisDeviceServiceCommand + 1),
                                NdisDeviceServiceCommand->Command.uDataSize
                                )) != NDIS_STATUS_SUCCESS )
        {
            TraceError( WMBCLASS_OID, "[MbbNdis][Ven][ReqId=0x%04x] FAILED to setup command message for DEVICE_SERVICE_COMMAND with status=%!status!",
                        Request->RequestId, NdisStatus );
            break;
        }
        *InBufferSize += NdisDeviceServiceCommand->Command.uDataSize;
        //
        // Call the wrapper routine to send each fragment.
        // The wrapper will cleanup fragments in case of
        // success or failure.
        //
        NdisStatus = MbbUtilSendMessageFragmentsAndLog( Request );
        if( NdisStatus != NDIS_STATUS_SUCCESS &&
            NdisStatus != NDIS_STATUS_PENDING )
        {
            TraceError( WMBCLASS_OID, "[MbbNdis][Ven][ReqId=0x%04x] FAILED to send message fragments for DEVICE_SERVICE_COMMAND with status=%!status!",
                        Request->RequestId, NdisStatus );
        }

    } while( FALSE );

    return NdisStatus;
}

NDIS_STATUS
MbbUtilUiccAccessBinary(
    __in PMBB_REQUEST_CONTEXT               Request,
    __in_bcount_opt(*InBufferSize) PUCHAR   InBuffer,
    __in PULONG                             InBufferSize
    )
{
    PNDIS_WWAN_UICC_ACCESS_BINARY   NdisUiccAccessBinary = (PNDIS_WWAN_UICC_ACCESS_BINARY)InBuffer;
    PMBB_UICC_ACCESS_BINARY         MbbUiccAccessBinary = NULL;
    NDIS_STATUS                     NdisStatus;
    ULONG                           BufferSize = 0;

    NdisStatus = MbbUtilWwanToMbbUiccAccessBinary(
        &NdisUiccAccessBinary->UiccAccessBinary,
        &MbbUiccAccessBinary,
        &BufferSize
        );

    if (NdisStatus != NDIS_STATUS_SUCCESS)
    {
        return NdisStatus;
    }

    Request->HandlerContext.DataToFreeOnCompletion = MbbUiccAccessBinary;

    return MbbUtilQueryAttributeWithParameter(
                Request,
                (PUCHAR) MbbUiccAccessBinary,
                BufferSize
                );
}

NDIS_STATUS
MbbUtilUiccAccessRecord(
    __in PMBB_REQUEST_CONTEXT               Request,
    __in_bcount_opt(*InBufferSize) PUCHAR   InBuffer,
    __in PULONG                             InBufferSize
    )
{
    PNDIS_WWAN_UICC_ACCESS_RECORD   NdisUiccAccessRecord = (PNDIS_WWAN_UICC_ACCESS_RECORD)InBuffer;
    PMBB_UICC_ACCESS_RECORD         MbbUiccAccessRecord = NULL;
    NDIS_STATUS                     NdisStatus;
    ULONG                           BufferSize = 0;

    NdisStatus = MbbUtilWwanToMbbUiccAccessRecord(
        &NdisUiccAccessRecord->UiccAccessRecord,
        &MbbUiccAccessRecord,
        &BufferSize
        );

    if (NdisStatus != NDIS_STATUS_SUCCESS)
    {
        return NdisStatus;
    }

    Request->HandlerContext.DataToFreeOnCompletion = MbbUiccAccessRecord;

    return MbbUtilQueryAttributeWithParameter(
                Request,
                (PUCHAR) MbbUiccAccessRecord,
                BufferSize
                );
}

//
// Wwan to Mbb
//


VOID
MbbUtilWwanToMbbRadioState(
    __in  WWAN_RADIO*       WwanRadio,
    __out MBB_RADIO_STATE*  MbbRadioState
    )
{
    *MbbRadioState = (MBB_RADIO_STATE)*WwanRadio;
}

NTSTATUS
MbbUtilWwanToMbbPinAction(
    __in  PWWAN_PIN_ACTION  WwanPinAction,
    __out PMBB_PIN_ACTION  * MbbPinActionOut,
    __out PULONG            ReturnedBufferSize
    )
{
    PUCHAR                  Buffer;
    ULONG                   CurrentOffset;
    ULONG                   BufferSize;
    PMBB_PIN_ACTION         MbbPinAction=NULL;
    NTSTATUS                Status;
    SIZE_T                  PinLength;
    SIZE_T                  NewPinLength;

    *ReturnedBufferSize=0;

    BufferSize= ROUND_UP_COUNT(FIELD_OFFSET(MBB_PIN_ACTION,DataBuffer), ALIGN_DWORD);

    CurrentOffset=BufferSize;

    Status=RtlStringCbLengthW(WwanPinAction->Pin, sizeof(WwanPinAction->Pin), &PinLength);

    if (!NT_SUCCESS(Status))
    {
        return Status;
    }

    Status=RtlStringCbLengthW(WwanPinAction->NewPin, sizeof(WwanPinAction->NewPin), &NewPinLength);

    if (!NT_SUCCESS(Status))
    {
        return Status;
    }

    BufferSize+=(ULONG)ROUND_UP_COUNT(PinLength, ALIGN_DWORD);
    BufferSize+=(ULONG)ROUND_UP_COUNT(NewPinLength, ALIGN_DWORD);

    if( (MbbPinAction = ALLOCATE_NONPAGED_POOL( BufferSize )) == NULL )
    {
        return NDIS_STATUS_RESOURCES;
    }

    Buffer=(PUCHAR)MbbPinAction;


    CurrentOffset=MbbUtilWriteStringToBuffer(
        Buffer,
        BufferSize,
        CurrentOffset,
        &MbbPinAction->Pin,
        (PUCHAR)WwanPinAction->Pin,
        (ULONG)PinLength
        );

    CurrentOffset=MbbUtilWriteStringToBuffer(
        Buffer,
        BufferSize,
        CurrentOffset,
        &MbbPinAction->NewPin,
        (PUCHAR)WwanPinAction->NewPin,
        (ULONG)NewPinLength
        );


    MbbPinAction->PinType      = (MBB_PIN_TYPE)WwanPinAction->PinType;
    MbbPinAction->PinOperation = (MBB_PIN_OPERATION)WwanPinAction->PinOperation;


    *MbbPinActionOut=MbbPinAction;

    *ReturnedBufferSize=BufferSize;

    return STATUS_SUCCESS;
}

NTSTATUS
MbbUtilWwanToMbbPinActionEx2(
    __in  PWWAN_PIN_ACTION_EX2  WwanPinAction,
    __out PMBB_PIN_ACTION_EX2  *MbbPinActionOut,
    __out PULONG                ReturnedBufferSize
    )
{
    PUCHAR                  Buffer;
    ULONG                   CurrentOffset;
    ULONG                   BufferSize;
    PMBB_PIN_ACTION_EX2     MbbPinAction=NULL;
    NTSTATUS                Status;
    SIZE_T                  PinLength;
    SIZE_T                  NewPinLength;

    *ReturnedBufferSize=0;

    BufferSize= ROUND_UP_COUNT(FIELD_OFFSET(MBB_PIN_ACTION_EX2,DataBuffer), ALIGN_DWORD);

    CurrentOffset=BufferSize;

    Status=RtlStringCbLengthW(WwanPinAction->Pin, sizeof(WwanPinAction->Pin), &PinLength);

    if (!NT_SUCCESS(Status))
    {
        return Status;
    }

    Status=RtlStringCbLengthW(WwanPinAction->NewPin, sizeof(WwanPinAction->NewPin), &NewPinLength);

    if (!NT_SUCCESS(Status))
    {
        return Status;
    }

    BufferSize+=(ULONG)ROUND_UP_COUNT(PinLength, ALIGN_DWORD);
    BufferSize+=(ULONG)ROUND_UP_COUNT(NewPinLength, ALIGN_DWORD);
    BufferSize+=(ULONG)ROUND_UP_COUNT(WwanPinAction->AppIdLength, ALIGN_DWORD);

    if( (MbbPinAction = ALLOCATE_NONPAGED_POOL( BufferSize )) == NULL )
    {
        return NDIS_STATUS_RESOURCES;
    }

    Buffer=(PUCHAR)MbbPinAction;


    CurrentOffset=MbbUtilWriteStringToBuffer(
        Buffer,
        BufferSize,
        CurrentOffset,
        &MbbPinAction->Pin,
        (PUCHAR)WwanPinAction->Pin,
        (ULONG)PinLength
        );

    CurrentOffset=MbbUtilWriteStringToBuffer(
        Buffer,
        BufferSize,
        CurrentOffset,
        &MbbPinAction->NewPin,
        (PUCHAR)WwanPinAction->NewPin,
        (ULONG)NewPinLength
        );

    CurrentOffset=MbbUtilWriteStringToBuffer(
        Buffer,
        BufferSize,
        CurrentOffset,
        (PMBB_STRING)&MbbPinAction->AppId,
        (PUCHAR)WwanPinAction->AppId,
        (ULONG)WwanPinAction->AppIdLength
        );

    MbbPinAction->PinType      = (MBB_PIN_TYPE)WwanPinAction->PinType;
    MbbPinAction->PinOperation = (MBB_PIN_OPERATION)WwanPinAction->PinOperation;

    *MbbPinActionOut=MbbPinAction;

    *ReturnedBufferSize=BufferSize;

    return STATUS_SUCCESS;
}

ULONG
MbbUtilWwanToMbbProvider(
    __in  PWWAN_PROVIDER                        WwanProvider,
    __in  MBB_CELLULAR_CLASS                    MbbCellularClass,
    __in  ULONG                                 Rssi,
    __in  ULONG                                 ErrorRate,
    __in  ULONG                                 MbbProviderSize,
    __out_bcount(MbbProviderSize) PMBB_PROVIDER MbbProvider
    )
{
    ULONG       MbbProviderOffset;
    SIZE_T      StringCbLength;
    NDIS_STATUS NdisStatus;

    do
    {
        if( sizeof(MBB_PROVIDER) > MbbProviderSize )
        {
            TraceError( WMBCLASS_OID, "[Util] INSUFFICIENT buffer to convert WWAN_PROVIDER, Size=%d Expected=%d",
                        MbbProviderSize,
                        sizeof(MBB_PROVIDER)
                        );
            MbbProviderOffset = 0;
            break;
        }
        MbbProvider->ProviderState      = (ULONG)WwanProvider->ProviderState;
        MbbProvider->Rssi               = Rssi;
        MbbProvider->ErrorRate          = ErrorRate;
        MbbProvider->CellularClass      = MbbCellularClass;

        MBB_INIT_MBB_STRING( &MbbProvider->ProviderId );
        MBB_INIT_MBB_STRING( &MbbProvider->ProviderName );

        MbbProviderOffset = sizeof(MBB_PROVIDER);

        if( (NdisStatus = RtlStringCbLengthW(
                            WwanProvider->ProviderId,
                            sizeof(WwanProvider->ProviderId),
                           &StringCbLength
                            )) != STATUS_SUCCESS )
        {
            TraceError( WMBCLASS_OID, "[Util] FAILED to get string length for WWAN_PROVIDER.ProviderId, NdisStatus=%!STATUS!",
                        NdisStatus
                        );
            MbbProviderOffset = 0;
            break;
        }

        if( MbbProviderOffset + StringCbLength > MbbProviderSize )
        {
            TraceError( WMBCLASS_OID, "[Util] INSUFFICIENT buffer to convert WWAN_PROVIDER.ProviderId, Size=%d Expected=%d",
                        MbbProviderSize,
                        (ULONG)(MbbProviderOffset + StringCbLength)
                        );
            MbbProviderOffset = 0;
            break;
        }

        MbbProviderOffset = MbbUtilWriteStringToBuffer(
                                (PUCHAR)MbbProvider,
                                MbbProviderSize,
                                MbbProviderOffset,
                                &MbbProvider->ProviderId,
                                (PCUCHAR)(WwanProvider->ProviderId),
                                (ULONG)StringCbLength
                                );

        if( (NdisStatus = RtlStringCbLengthW(
                            WwanProvider->ProviderName,
                            sizeof(WwanProvider->ProviderName),
                           &StringCbLength
                            )) != STATUS_SUCCESS )
        {
            TraceError( WMBCLASS_OID, "[Util] FAILED to get string length for WWAN_PROVIDER.ProviderName, NdisStatus=%!STATUS!", NdisStatus );
            MbbProviderOffset = 0;
            break;
        }

        if( MbbProviderOffset + StringCbLength > MbbProviderSize )
        {
            TraceError( WMBCLASS_OID, "[Util] INSUFFICIENT buffer to convert WWAN_PROVIDER.ProviderName, Size=%d Expected=%d",
                        MbbProviderSize,
                        (ULONG)(MbbProviderOffset + StringCbLength)
                        );
            MbbProviderOffset = 0;
            break;
        }

        MbbProviderOffset = MbbUtilWriteStringToBuffer(
                                (PUCHAR)MbbProvider,
                                MbbProviderSize,
                                MbbProviderOffset,
                               &MbbProvider->ProviderName,
                                (PCUCHAR)(WwanProvider->ProviderName),
                                (ULONG)StringCbLength
                                );
    }
    while( FALSE );

    return MbbProviderOffset;
}

ULONG
MbbUtilWwanToMbbProvider2(
    __in  PWWAN_PROVIDER2                       WwanProvider2,
    __in ULONG                                  MbbProviderSize,
    __out_bcount(MbbProviderSize) PMBB_PROVIDER MbbProvider
    )
{
    //
    // There is no MBB mapping for WwanCellularClassWiMAX.
    // Assume that the service will behave correctly and not send bad data.
    //
    return MbbUtilWwanToMbbProvider(
               &WwanProvider2->Provider,
                (MBB_CELLULAR_CLASS)(WwanProvider2->WwanCellularClass),
                WwanProvider2->Rssi,
                WwanProvider2->ErrorRate,
                MbbProviderSize,
                MbbProvider
                );
}

NDIS_STATUS
MbbUtilWwanToMbbProviderList(
    __in  PWWAN_LIST_HEADER                                     WwanProviderList,
    __in  BOOLEAN                                               IsMultiCarrierCapable,
    __in_opt MBB_CELLULAR_CLASS                                 MbbCellularClass,
    __out_bcount(*MbbProviderListSize) PMBB_PROVIDER_LIST       MbbProviderList,
    __inout PULONG                                              MbbProviderListSize
    )
{
    ULONG               ElementIndex;
    ULONG               ElementCount = WwanProviderList->ElementCount;
    ULONG               MbbProviderSize;
    ULONG               MbbProviderListOffset;
    NDIS_STATUS         NdisStatus = NDIS_STATUS_FAILURE;
    PMBB_PROVIDER       MbbProvider;
    PWWAN_PROVIDER2     WwanProviders2 = (PWWAN_PROVIDER2)(WwanProviderList + 1);
    PMBB_ARRAY_ELEMENT  MbbProviderElement = (PMBB_ARRAY_ELEMENT)(MbbProviderList->Providers);

    MbbProviderListOffset  = FIELD_OFFSET(MBB_PROVIDER_LIST, Providers);
    MbbProviderListOffset += ElementCount * sizeof(MBB_ARRAY_ELEMENT);

    if( MbbProviderListOffset > *MbbProviderListSize )
    {
        TraceError( WMBCLASS_OID, "[Util] INSUFFICIENT buffer to convert WWAN_PROVIDER_LIST, BufferSize=%d Expected=%d",
                    *MbbProviderListSize,
                    MbbProviderListOffset
                    );
        return NDIS_STATUS_BUFFER_OVERFLOW;
    }

    for( ElementIndex = 0;
         ElementIndex < ElementCount;
         ElementIndex ++ )
    {
        MbbProviderListOffset = ROUND_UP_COUNT( MbbProviderListOffset, ALIGN_DWORD );
        MbbProviderElement[ElementIndex].Offset = MbbProviderListOffset;
        MbbProvider = (PMBB_PROVIDER)(((PCHAR)MbbProviderList)+MbbProviderListOffset);

        if( MbbProviderListOffset > *MbbProviderListSize )
        {
            TraceError( WMBCLASS_OID, "[Util] INSUFFICIENT buffer to convert WWAN_PROVIDER_LIST.Provider[%02d], BufferSize=%d Expected=%d",
                        *MbbProviderListSize,
                        MbbProviderListOffset,
                        ElementIndex
                        );
            NdisStatus = NDIS_STATUS_BUFFER_OVERFLOW;
            break;
        }

        if( (MbbProviderSize = MbbUtilWwanToMbbProvider2(
                                    &WwanProviders2[ElementIndex],
                                    *MbbProviderListSize - MbbProviderListOffset,
                                    MbbProvider
                                    )) == 0 )
        {
            TraceError( WMBCLASS_OID, "[Util] FAILED to convert %02d# WWAN_PROVIDER2 to MBB_PROVIDER", ElementIndex );
            break;
        }

        MbbProviderElement[ElementIndex].Size = MbbProviderSize;
        MbbProviderListOffset += MbbProviderSize;
    }
    if( ElementIndex == ElementCount )
    {
        MbbProviderList->ProviderCount = ElementCount;
        NdisStatus = NDIS_STATUS_SUCCESS;
        *MbbProviderListSize = MbbProviderListOffset;
    }
    return NdisStatus;
}

NTSTATUS
MbbUtilWwanToMbbSetRegisterState(
    __in  PWWAN_SET_REGISTER_STATE  WwanSetRegisterState,
    __out PMBB_SET_REGISTER_STATE  *MbbSetRegisterStateOut,
    __out PULONG                    ReturnedBufferSize
    )
{
    PUCHAR                  Buffer;
    ULONG                   CurrentOffset;
    ULONG                   BufferSize;
    PMBB_SET_REGISTER_STATE   MbbSetRegisterState=NULL;
    NTSTATUS                Status;
    SIZE_T                  ProviderIdLength;

    *ReturnedBufferSize=0;

    BufferSize= ROUND_UP_COUNT(FIELD_OFFSET(MBB_SET_REGISTER_STATE,DataBuffer), ALIGN_DWORD);

    CurrentOffset=BufferSize;

    Status=RtlStringCbLengthW(WwanSetRegisterState->ProviderId, sizeof(WwanSetRegisterState->ProviderId), &ProviderIdLength);

    if (!NT_SUCCESS(Status))
    {
        return Status;
    }


    BufferSize+=(ULONG)ROUND_UP_COUNT(ProviderIdLength, ALIGN_DWORD);

    if( (MbbSetRegisterState = ALLOCATE_NONPAGED_POOL( BufferSize )) == NULL )
    {
        return NDIS_STATUS_RESOURCES;
    }

    Buffer=(PUCHAR)MbbSetRegisterState;


    CurrentOffset=MbbUtilWriteStringToBuffer(
        Buffer,
        BufferSize,
        CurrentOffset,
        &MbbSetRegisterState->ProviderId,
        (PUCHAR)WwanSetRegisterState->ProviderId,
        (ULONG)ProviderIdLength
        );

    MbbSetRegisterState->DataClass       = (MBB_DATA_CLASS_VALUE)WwanSetRegisterState->WwanDataClass;
    MbbSetRegisterState->RegisterAction  = (MBB_REGISTER_ACTION)WwanSetRegisterState->RegisterAction;


    *MbbSetRegisterStateOut=MbbSetRegisterState;

    *ReturnedBufferSize=BufferSize;

    return STATUS_SUCCESS;

}

NDIS_STATUS
MbbUtilWwanToMbbSetPacketService(
    __in  WWAN_PACKET_SERVICE_ACTION PacketServiceAction,
    __out PMBB_SET_PACKET_SERVICE    MbbSetPacketService
    )
{
    switch( PacketServiceAction )
    {
        case WwanPacketServiceActionAttach:
            MbbSetPacketService->PacketServiceAction = MbbPacketServiceActionAttach;
            return NDIS_STATUS_SUCCESS;
        case WwanPacketServiceActionDetach:
            MbbSetPacketService->PacketServiceAction = MbbPacketServiceActionDetach;
            return NDIS_STATUS_SUCCESS;
        default :
            return NDIS_STATUS_INVALID_DATA;
    }
}

VOID
MbbUtilAdjustSignalStateConfigurationForDevice(
    __in     PMINIPORT_ADAPTER_CONTEXT     Adapter,
    __inout  PWWAN_SET_SIGNAL_INDICATION   WwanSetSignalIndication
    )
/*++
Description
    The signal state interval and threshold values need to be adjusted
    for Blue for efficient power management. This is done only for
    devices that support MBIM 1.0 Errata and the configuration is adjusted
    to (60, 1). The signal state configuration
    for non-errata devices will continue to be the Win 8 WWAN values (30, -1).
    The values are adjusted only if they were set to WWAN_RSSI_DEFAULT (-1).
--*/
{
    //
    // Check if device supports Errata
    //
    if ( Adapter->BusParams.IsErrataDevice )
    {
        //
        // Use new Blue values
        //
        if( WWAN_RSSI_DEFAULT == WwanSetSignalIndication->RssiInterval )
        {
            WwanSetSignalIndication->RssiInterval = WWAN_RSSI_INTERVAL_SECS_BLUE;
        }

        if( WWAN_RSSI_DEFAULT == WwanSetSignalIndication->RssiThreshold )        
        {
            WwanSetSignalIndication->RssiThreshold = WWAN_RSSI_THRESHOLD_CODED_UNIT_BLUE;
        }
    }
    else
    {
        //
        // Continue to use Win 8 values
        //
        if( WWAN_RSSI_DEFAULT == WwanSetSignalIndication->RssiInterval )
        {
            WwanSetSignalIndication->RssiInterval = WWAN_RSSI_INTERVAL_SECS_WIN8;
        }

        if( WWAN_RSSI_DEFAULT == WwanSetSignalIndication->RssiThreshold )        
        {
            WwanSetSignalIndication->RssiThreshold = WWAN_RSSI_THRESHOLD_CODED_UNIT_WIN8;
        }        
    }
}

VOID
MbbUtilWwanToMbbSetSignalStateIndication(
    __in  PWWAN_SET_SIGNAL_INDICATION   WwanSetSignalIndication,
    __out PMBB_SET_SIGNAL_INDICATION    MbbSetSignalStateIndication
    )
/*++
Description
    MBB and WWAN default and disable values are inverted so
    appropriate checks and translation is required.
--*/
{
    if( WWAN_RSSI_DEFAULT == WwanSetSignalIndication->RssiInterval )
        MbbSetSignalStateIndication->RssiInterval = MBB_RSSI_DEFAULT;
    else if( WWAN_RSSI_DISABLE == WwanSetSignalIndication->RssiInterval )
        MbbSetSignalStateIndication->RssiInterval = MBB_RSSI_DISABLE;
    else
        MbbSetSignalStateIndication->RssiInterval = WwanSetSignalIndication->RssiInterval;

    if( WWAN_RSSI_DEFAULT == WwanSetSignalIndication->RssiThreshold )
        MbbSetSignalStateIndication->RssiThreshold = MBB_RSSI_DEFAULT;
    else if( WWAN_RSSI_DISABLE == WwanSetSignalIndication->RssiThreshold )
        MbbSetSignalStateIndication->RssiThreshold = MBB_RSSI_DISABLE;
    else
        MbbSetSignalStateIndication->RssiThreshold = WwanSetSignalIndication->RssiThreshold;

    MbbSetSignalStateIndication->ErrorRateThreshold = MBB_RSSI_DISABLE;
}

VOID
MbbUtilWwanToMbbContextType(
    __in  WWAN_CONTEXT_TYPE WwanContextType,
    __out PGUID             MbbContextType
    )
{
    GUID SrcGuid;

    switch( WwanContextType )
    {
        case WwanContextTypeInternet:
            SrcGuid = MBB_UUID_CONTEXT_TYPE_INTERNET;
            break;

        case WwanContextTypeVpn:
            SrcGuid = MBB_UUID_CONTEXT_TYPE_VPN;
            break;

        case WwanContextTypeVoice:
            SrcGuid = MBB_UUID_CONTEXT_TYPE_VOICE;
            break;

        case WwanContextTypeVideoShare:
            SrcGuid = MBB_UUID_CONTEXT_TYPE_VIDEO_SHARE;
            break;

        case WwanContextTypePurchase:
            SrcGuid = MBB_UUID_CONTEXT_TYPE_PURCHASE;
            break;

        case WwanContextTypeCustom:
            SrcGuid = MBB_UUID_CONTEXT_TYPE_CUSTOM;
            break;

        case WwanContextTypeIms:
            SrcGuid = MBB_UUID_CONTEXT_TYPE_IMS;
            break;

        case WwanContextTypeAdmin:
            SrcGuid = MBB_UUID_CONTEXT_TYPE_MS_ADMIN;
            break;

        case WwanContextTypeApp:
            SrcGuid = MBB_UUID_CONTEXT_TYPE_MS_APP;
            break;

        case WwanContextTypeLteAttach:
            SrcGuid = MBB_UUID_BASIC_CONNECT_EXTENSIONS;
            break;

        case WwanContextTypeNone:
        default:
            SrcGuid = MBB_UUID_CONTEXT_TYPE_NONE;

            break;
    }
    MBB_UUID_TO_NET( MbbContextType, &SrcGuid );
}

VOID
MbbUtilWwanToMbbSetContextState(
    __in  PWWAN_SET_CONTEXT_STATE   WwanSetContextState,
    __in  ULONG                     AccessStringCchLength,
    __in  ULONG                     UserNameCchLength,
    __in  ULONG                     PasswordCchLength,
    __in  ULONG                     MbbSetContextStateSize,
    __in  ULONG                     SessionId,
    __out PMBB_SET_CONTEXT_STATE    MbbSetContextState
    )
{
    ULONG   CurrentOffset;

    MbbSetContextState->SessionId           =  SessionId;
    MbbSetContextState->ActivationCommand   = (MBB_ACTIVATION_COMMAND)WwanSetContextState->ActivationCommand;
    MbbSetContextState->Compression         = (MBB_COMPRESSION)WwanSetContextState->Compression;
    MbbSetContextState->AuthProtocol        = (MBB_AUTH_PROTOCOL)WwanSetContextState->AuthType;
    MbbSetContextState->IPType              = (MBB_CONTEXT_IP_TYPE)WwanSetContextState->IPType;
    if (MbbSetContextState->IPType > MbbContextIPTypeIPv4v6)
    {
        TraceWarn(WMBCLASS_UTIL, "Invalid IPType from the OS. Resetting it to MbbContextIPTypeDefault");
        MbbSetContextState->IPType = MbbContextIPTypeDefault;
    }

    MbbUtilWwanToMbbContextType(
        WwanContextTypeInternet,
        &MbbSetContextState->ContextType
        );

    CurrentOffset = FIELD_OFFSET( MBB_SET_CONTEXT_STATE, DataBuffer );

    CurrentOffset =  MbbUtilWriteStringToBuffer(
                        (PUCHAR)MbbSetContextState,
                        MbbSetContextStateSize,
                        CurrentOffset,
                       &MbbSetContextState->AccessString,
                        (PCUCHAR)(WwanSetContextState->AccessString),
                        AccessStringCchLength * sizeof(WCHAR)
                        );

    CurrentOffset =  MbbUtilWriteStringToBuffer(
                        (PUCHAR)MbbSetContextState,
                        MbbSetContextStateSize,
                        CurrentOffset,
                       &MbbSetContextState->UserName,
                        (PCUCHAR)(WwanSetContextState->UserName),
                        UserNameCchLength * sizeof(WCHAR)
                        );

    CurrentOffset =  MbbUtilWriteStringToBuffer(
                        (PUCHAR)MbbSetContextState,
                        MbbSetContextStateSize,
                        CurrentOffset,
                       &MbbSetContextState->Password,
                        (PCUCHAR)(WwanSetContextState->Password),
                        PasswordCchLength * sizeof(WCHAR)
                        );
}

VOID
MbbUtilMbbToWwanLteAttachConfigInfo(
    __in  MBB_MS_LTE_ATTACH_CONFIG_INFO* MbbLteAttachContextList,
    __out PWWAN_LTE_ATTACH_CONFIG   WwanLteAttachContext
    )
{
    PMBB_MS_LTE_ATTACH_CONTEXT    MbbLteAttachContext;

    for( ULONG ContextIndex = 0;
         ContextIndex < MbbLteAttachContextList->ElementCount;
         ContextIndex ++ )
    {
        MbbLteAttachContext  = (PMBB_MS_LTE_ATTACH_CONTEXT)((PUCHAR)MbbLteAttachContextList + MbbLteAttachContextList->Contexts[ContextIndex].Offset);

        MbbUtilMbbToWwanLteAttachContext(
            MbbLteAttachContext,
            &(WwanLteAttachContext->LteContextArray[ContextIndex])
            );
    }
}

NTSTATUS
MbbUtilWwanToMbbSetLteAttachContext(
    __in  PWWAN_SET_LTE_ATTACH                   WwanSetContextBuffer,
    __out PMBB_SET_MS_CONTEXT_LTE_ATTACH_CONFIG *ReturnedMbbSetContext,
    __out ULONG                                 *ReturnedBufferSize
    )
{
    NTSTATUS                              Status = STATUS_SUCCESS;

    PMBB_SET_MS_CONTEXT_LTE_ATTACH_CONFIG MbbSetContext = NULL;
    ULONG                                 SetBufferSize = 0;
    ULONG                                 SetBufferOffset = 0;

    PMBB_MS_LTE_ATTACH_CONTEXT            MbbLteAttachContext = NULL;
    ULONG                                 BufferSize[TotalRoamingConditions] = {0};
    ULONG                                 MbbLteAttachOffset = 0;
    SIZE_T                                AccessStringLength[TotalRoamingConditions] = {0};
    SIZE_T                                UserNameLength[TotalRoamingConditions] = {0};
    SIZE_T                                PasswordLength[TotalRoamingConditions] = {0};

    PWWAN_SET_CONTEXT_V2                  WwanSetLteAttachContext = NULL; 
    *ReturnedBufferSize=0;
    *ReturnedMbbSetContext=NULL;

    SetBufferSize =  ROUND_UP_COUNT(FIELD_OFFSET(MBB_SET_MS_CONTEXT_LTE_ATTACH_CONFIG, Contexts), ALIGN_DWORD);
    SetBufferSize += ROUND_UP_COUNT(sizeof(MBB_ARRAY_ELEMENT) * TotalRoamingConditions, ALIGN_DWORD);

    SetBufferOffset = SetBufferSize;

    for (ULONG i = 0; i < TotalRoamingConditions; i ++)
    {
        WwanSetLteAttachContext = &(WwanSetContextBuffer->SetContextArray[i]);

        BufferSize[i] = ROUND_UP_COUNT(FIELD_OFFSET(MBB_MS_LTE_ATTACH_CONTEXT, DataBuffer), ALIGN_DWORD);

        Status=RtlStringCbLengthW(WwanSetLteAttachContext->basicInfo.AccessString, sizeof(WwanSetLteAttachContext->basicInfo.AccessString), &(AccessStringLength[i]));
        if (!NT_SUCCESS(Status))
        {
            return Status;
        }

        Status=RtlStringCbLengthW(WwanSetLteAttachContext->basicInfo.UserName, sizeof(WwanSetLteAttachContext->basicInfo.UserName), &(UserNameLength[i]));
        if (!NT_SUCCESS(Status))
        {
            return Status;
        }

        Status=RtlStringCbLengthW(WwanSetLteAttachContext->basicInfo.Password, sizeof(WwanSetLteAttachContext->basicInfo.Password), &(PasswordLength[i]));
        if (!NT_SUCCESS(Status))
        {
            return Status;
        }

        BufferSize[i] += (ULONG)ROUND_UP_COUNT(AccessStringLength[i], ALIGN_DWORD);
        BufferSize[i] += (ULONG)ROUND_UP_COUNT(UserNameLength[i], ALIGN_DWORD);
        BufferSize[i] += (ULONG)ROUND_UP_COUNT(PasswordLength[i], ALIGN_DWORD);

        SetBufferSize += BufferSize[i];
    }

    MbbSetContext = ALLOCATE_NONPAGED_POOL( SetBufferSize );

    if ( MbbSetContext == NULL )
    {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    RtlZeroMemory(MbbSetContext, SetBufferSize);

    MbbSetContext->Operation = WwanLteAttachOperationsMapTable[WwanSetContextBuffer->SetContextArray[0].Operation];
    MbbSetContext->ElementCount = TotalRoamingConditions;

    for (ULONG i = 0; i < TotalRoamingConditions; i ++)
    {
        MbbSetContext->Contexts[i].Offset = SetBufferOffset;
        MbbSetContext->Contexts[i].Size = BufferSize[i];

        MbbLteAttachOffset = FIELD_OFFSET(MBB_MS_LTE_ATTACH_CONTEXT, DataBuffer);
        MbbLteAttachContext = (PMBB_MS_LTE_ATTACH_CONTEXT)((PBYTE)MbbSetContext + SetBufferOffset);
        WwanSetLteAttachContext = &(WwanSetContextBuffer->SetContextArray[i]);

        if (WwanSetLteAttachContext->IPType >= WwanIPTypeDefault &&
            WwanSetLteAttachContext->IPType <= WwanIPTypeIpv4v6)
        {
            MbbLteAttachContext->IPType = WwanIpTypesMapTable[WwanSetLteAttachContext->IPType];
        }
        else
        {
            MbbLteAttachContext->IPType = MbbContextIPTypeMaximum;
        }

        MbbLteAttachContext->Roaming = (MBB_BASICCONNECTEXT_LTEATTACH_ROAMING_CONTROL)WwanSetLteAttachContext->Roaming;

        if (WwanSetLteAttachContext->Source >= WwanUserProvisioned &&
            WwanSetLteAttachContext->Source < WwanMaxProvisionSource)
        {
            MbbLteAttachContext->Source = WwanSourcsMapTable[WwanSetLteAttachContext->Source];
        }
        else
        {
            MbbLteAttachContext->Source = MbbMsContextSourceMaximum;
        }

        MbbLteAttachOffset = MbbUtilWriteStringToBuffer(
                             (PUCHAR)MbbLteAttachContext,
                             BufferSize[i],
                             MbbLteAttachOffset,
                             &MbbLteAttachContext->AccessString,
                             (PCUCHAR)(WwanSetLteAttachContext->basicInfo.AccessString),
                             (ULONG)AccessStringLength[i]
                             );

        MbbLteAttachOffset = MbbUtilWriteStringToBuffer(
                             (PUCHAR)MbbLteAttachContext,
                             BufferSize[i],
                             MbbLteAttachOffset,
                             &MbbLteAttachContext->UserName,
                             (PCUCHAR)(WwanSetLteAttachContext->basicInfo.UserName),
                             (ULONG)UserNameLength[i]
                             );

        MbbLteAttachOffset = MbbUtilWriteStringToBuffer(
                             (PUCHAR)MbbLteAttachContext,
                             BufferSize[i],
                             MbbLteAttachOffset,
                             &MbbLteAttachContext->Password,
                             (PCUCHAR)(WwanSetLteAttachContext->basicInfo.Password),
                             (ULONG)PasswordLength[i]
                             );

        MbbLteAttachContext->Compression      = (MBB_COMPRESSION)WwanSetLteAttachContext->basicInfo.Compression;
        MbbLteAttachContext->AuthProtocol     = (MBB_AUTH_PROTOCOL)WwanSetLteAttachContext->basicInfo.AuthType;

        SetBufferOffset += BufferSize[i];
    }

    *ReturnedBufferSize=SetBufferSize;
    *ReturnedMbbSetContext=MbbSetContext;

    return STATUS_SUCCESS;
}


NTSTATUS
MbbUtilWwanToMbbSetContext(
    __in  PWWAN_SET_CONTEXT     WwanSetContext,
    __out PMBB_SET_CONTEXT     *ReturnedMbbSetContext,
    __out ULONG                *ReturnedBufferSize
    )
{


    ULONG                   CurrentOffset;
    ULONG                   BufferSize;
    PMBB_SET_CONTEXT        MbbSetContext=NULL;
    NTSTATUS                Status;
    SIZE_T                  AccessStringLength;
    SIZE_T                  UserNameLength;
    SIZE_T                  PasswordLength;
    SIZE_T                  ProviderIdLength;

    *ReturnedBufferSize=0;
    *ReturnedMbbSetContext=NULL;

    BufferSize= ROUND_UP_COUNT(FIELD_OFFSET(MBB_SET_CONTEXT,DataBuffer), ALIGN_DWORD);

    Status=RtlStringCbLengthW(WwanSetContext->AccessString, sizeof(WwanSetContext->AccessString), &AccessStringLength);

    if (!NT_SUCCESS(Status))
    {
        return Status;
    }

    Status=RtlStringCbLengthW(WwanSetContext->UserName, sizeof(WwanSetContext->UserName), &UserNameLength);

    if (!NT_SUCCESS(Status))
    {
        return Status;
    }

    Status=RtlStringCbLengthW(WwanSetContext->Password, sizeof(WwanSetContext->Password), &PasswordLength);

    if (!NT_SUCCESS(Status))
    {
        return Status;
    }

   Status=RtlStringCbLengthW(WwanSetContext->ProviderId, sizeof(WwanSetContext->ProviderId), &ProviderIdLength);

    if (!NT_SUCCESS(Status))
    {
        return Status;
    }



    BufferSize+=(ULONG)ROUND_UP_COUNT(AccessStringLength, ALIGN_DWORD);
    BufferSize+=(ULONG)ROUND_UP_COUNT(UserNameLength, ALIGN_DWORD);
    BufferSize+=(ULONG)ROUND_UP_COUNT(PasswordLength, ALIGN_DWORD);
    BufferSize+=(ULONG)ROUND_UP_COUNT(ProviderIdLength, ALIGN_DWORD);

    MbbSetContext = ALLOCATE_NONPAGED_POOL( BufferSize );

    if ( MbbSetContext == NULL )
    {
        return NDIS_STATUS_RESOURCES;
    }

    CurrentOffset = FIELD_OFFSET( MBB_SET_CONTEXT_STATE, DataBuffer );

    CurrentOffset =  MbbUtilWriteStringToBuffer(
                        (PUCHAR)MbbSetContext,
                        BufferSize,
                        CurrentOffset,
                       &MbbSetContext->AccessString,
                        (PCUCHAR)(WwanSetContext->AccessString),
                        (ULONG)AccessStringLength
                        );

    CurrentOffset =  MbbUtilWriteStringToBuffer(
                        (PUCHAR)MbbSetContext,
                        BufferSize,
                        CurrentOffset,
                       &MbbSetContext->UserName,
                        (PCUCHAR)(WwanSetContext->UserName),
                        (ULONG)UserNameLength
                        );

    CurrentOffset =  MbbUtilWriteStringToBuffer(
                        (PUCHAR)MbbSetContext,
                        BufferSize,
                        CurrentOffset,
                       &MbbSetContext->Password,
                        (PCUCHAR)(WwanSetContext->Password),
                        (ULONG)PasswordLength
                        );

    CurrentOffset =  MbbUtilWriteStringToBuffer(
                        (PUCHAR)MbbSetContext,
                        BufferSize,
                        CurrentOffset,
                       &MbbSetContext->ProviderId,
                        (PCUCHAR)(WwanSetContext->ProviderId),
                        (ULONG)ProviderIdLength
                        );


    MbbSetContext->ContextId        = WwanSetContext->ContextId;
    MbbSetContext->Compression      = (MBB_COMPRESSION)WwanSetContext->Compression;
    MbbSetContext->AuthProtocol     = (MBB_AUTH_PROTOCOL)WwanSetContext->AuthType;

    MbbUtilWwanToMbbContextType(
        WwanSetContext->ContextType,
        &MbbSetContext->ContextType
        );

    *ReturnedBufferSize=BufferSize;
    *ReturnedMbbSetContext=MbbSetContext;

    return STATUS_SUCCESS;

}

NTSTATUS
MbbUtilWwanToMbbSetContextV2(
    __in  PWWAN_SET_PROVISIONED_CONTEXT_V2                          WwanSetContext,
    __out PMBB_SET_MS_CONTEXT_PROVISIONED_CONTEXT_V2    *ReturnedMbbSetContext,
    __out ULONG                                         *ReturnedBufferSize
)
{
    ULONG                   CurrentOffset;
    ULONG                   BufferSize;
    PMBB_SET_MS_CONTEXT_PROVISIONED_CONTEXT_V2        MbbSetContext = NULL;
    NTSTATUS                Status;
    SIZE_T                  AccessStringLength;
    SIZE_T                  UserNameLength;
    SIZE_T                  PasswordLength;
    SIZE_T                  ProviderIdLength;

    *ReturnedBufferSize = 0;
    *ReturnedMbbSetContext = NULL;

    BufferSize = ROUND_UP_COUNT(FIELD_OFFSET(MBB_SET_MS_CONTEXT_PROVISIONED_CONTEXT_V2, DataBuffer), ALIGN_DWORD);

    Status = RtlStringCbLengthW(WwanSetContext->basicInfo.AccessString, sizeof(WwanSetContext->basicInfo.AccessString), &AccessStringLength);

    if (!NT_SUCCESS(Status))
    {
        return Status;
    }

    Status = RtlStringCbLengthW(WwanSetContext->basicInfo.UserName, sizeof(WwanSetContext->basicInfo.UserName), &UserNameLength);

    if (!NT_SUCCESS(Status))
    {
        return Status;
    }

    Status = RtlStringCbLengthW(WwanSetContext->basicInfo.Password, sizeof(WwanSetContext->basicInfo.Password), &PasswordLength);

    if (!NT_SUCCESS(Status))
    {
        return Status;
    }

    BufferSize += (ULONG)ROUND_UP_COUNT(AccessStringLength, ALIGN_DWORD);
    BufferSize += (ULONG)ROUND_UP_COUNT(UserNameLength, ALIGN_DWORD);
    BufferSize += (ULONG)ROUND_UP_COUNT(PasswordLength, ALIGN_DWORD);

    MbbSetContext = ALLOCATE_NONPAGED_POOL(BufferSize);

    if (MbbSetContext == NULL)
    {
        return NDIS_STATUS_RESOURCES;
    }

    CurrentOffset = FIELD_OFFSET(MBB_SET_MS_CONTEXT_PROVISIONED_CONTEXT_V2, DataBuffer);

    CurrentOffset = MbbUtilWriteStringToBuffer(
        (PUCHAR)MbbSetContext,
        BufferSize,
        CurrentOffset,
        &MbbSetContext->AccessString,
        (PCUCHAR)(WwanSetContext->basicInfo.AccessString),
        (ULONG)AccessStringLength
    );

    CurrentOffset = MbbUtilWriteStringToBuffer(
        (PUCHAR)MbbSetContext,
        BufferSize,
        CurrentOffset,
        &MbbSetContext->UserName,
        (PCUCHAR)(WwanSetContext->basicInfo.UserName),
        (ULONG)UserNameLength
    );

    CurrentOffset = MbbUtilWriteStringToBuffer(
        (PUCHAR)MbbSetContext,
        BufferSize,
        CurrentOffset,
        &MbbSetContext->Password,
        (PCUCHAR)(WwanSetContext->basicInfo.Password),
        (ULONG)PasswordLength
    );


    MbbSetContext->Compression = (MBB_COMPRESSION)WwanSetContext->basicInfo.Compression;
    MbbSetContext->AuthProtocol = (MBB_AUTH_PROTOCOL)WwanSetContext->basicInfo.AuthType;

    MbbUtilWwanToMbbContextType(
        WwanSetContext->basicInfo.ContextType,
        &MbbSetContext->ContextType
    );

    MbbSetContext->Operation = (MBB_BASICCONNECTEXT_CONTEXT_OPERATIONS)WwanSetContext->Operation;
    MbbSetContext->IPType = WwanIpTypesMapTable[WwanSetContext->IPType];
    MbbSetContext->Enable = (MBB_BASICCONNECTEXT_CONTEXT_ENABLE)WwanSetContext->Enable;
    MbbSetContext->Roaming = (MBB_BASICCONNECTEXT_CONTEXT_ROAMING_CONTROL)WwanSetContext->Roaming;
    MbbSetContext->MediaType = (MBB_BASICCONNECTEXT_CONTEXT_MEDIA_TYPE)WwanSetContext->MediaType;
    MbbSetContext->Source = WwanSourcsMapTable[WwanSetContext->Source];

    *ReturnedBufferSize = BufferSize;
    *ReturnedMbbSetContext = MbbSetContext;

    return STATUS_SUCCESS;
}

// SMS

MBB_SMS_FORMAT
MbbUtilWwanToMbbSmsFormat(
    __in  WWAN_SMS_FORMAT   WwanSmsFormat
    )
{
    switch( WwanSmsFormat )
    {
        case WwanSmsFormatPdu:
            return MbbSmsFormatPdu;
        case WwanSmsFormatCdma:
            return MbbSmsFormatCdma;
        default:
            return MbbSmsFormatMaximum;
    }
}

NTSTATUS
MbbUtilWwanToMbbSmsSetConfiguration(
    __in  PWWAN_SET_SMS_CONFIGURATION    WwanSmsSetConfiguration,
    __in  ULONG                          ScAddressSize,
    __out PMBB_SET_SMS_CONFIGURATION    *MbbSmsSetConfigurationReturn,
    __out PULONG                         ReturnedBufferSize
    )
{
    NTSTATUS        NtStatus;
    ANSI_STRING     AnsiString;
    UNICODE_STRING  UnicodeString;
    WCHAR           WideCharBuffer[WWAN_SMS_ADDRESS_MAX_LEN];
    PMBB_SET_SMS_CONFIGURATION     MbbSmsSetConfiguration=NULL;
    PBYTE           Buffer=NULL;
    ULONG           BufferSize=0;
    ULONG           CurrentOffset=0;

    *MbbSmsSetConfigurationReturn=NULL;
    *ReturnedBufferSize=0;


    AnsiString.Buffer           = WwanSmsSetConfiguration->ScAddress;
    AnsiString.Length           = (USHORT)ScAddressSize;
    AnsiString.MaximumLength    = WWAN_SMS_ADDRESS_MAX_LEN - 1; // No NULL

    UnicodeString.Buffer        = &WideCharBuffer[0];
    UnicodeString.Length        = 0;
    UnicodeString.MaximumLength = sizeof(WideCharBuffer);


    NtStatus = RtlAnsiStringToUnicodeString(
                        &UnicodeString,
                        &AnsiString,
                        FALSE // AllocateDestinationString
                        );

    if ( NtStatus != STATUS_SUCCESS )
    {
        TraceError( WMBCLASS_UTIL, "[MbbNdis] FAILED to convert ScAddress Length=%dbytes, NtStatus=%!STATUS!",
                    AnsiString.Length, NtStatus
                    );
        ASSERT(0);
        return NtStatus;
    }

    BufferSize= ROUND_UP_COUNT(FIELD_OFFSET(MBB_SET_SMS_CONFIGURATION,DataBuffer), ALIGN_DWORD);

    CurrentOffset=BufferSize;


    BufferSize+=(ULONG)ROUND_UP_COUNT(ScAddressSize * sizeof(WCHAR), ALIGN_DWORD);

    MbbSmsSetConfiguration = ALLOCATE_NONPAGED_POOL( BufferSize );

    if (MbbSmsSetConfiguration == NULL )
    {
        return NDIS_STATUS_RESOURCES;
    }

    Buffer=(PUCHAR)MbbSmsSetConfiguration;

    MbbSmsSetConfiguration->SmsFormat = MbbUtilWwanToMbbSmsFormat( WwanSmsSetConfiguration->SmsFormat );


    CurrentOffset=MbbUtilWriteStringToBuffer(
        Buffer,
        BufferSize,
        CurrentOffset,
        &MbbSmsSetConfiguration->ScAddress,
        (PUCHAR)&WideCharBuffer[0],
        (ULONG)ScAddressSize * sizeof(WCHAR)
        );

    *MbbSmsSetConfigurationReturn=MbbSmsSetConfiguration;
    *ReturnedBufferSize=BufferSize;

    return STATUS_SUCCESS;

}

VOID
MbbUtilWwanToMbbSmsRead(
    __in  PWWAN_SMS_READ    WwanSmsRead,
    __out PMBB_SMS_READ     MbbSmsRead
    )
{
    MbbSmsRead->SmsFormat    = MbbUtilWwanToMbbSmsFormat( WwanSmsRead->SmsFormat );
    MbbSmsRead->SmsFlag      = (MBB_SMS_FLAG)WwanSmsRead->ReadFilter.Flag;
    MbbSmsRead->MessageIndex = WwanSmsRead->ReadFilter.MessageIndex;
}

NDIS_STATUS
MbbUtilHexStringToUchar(
    __in_ecount(2)  PCHAR   HexString,
    __out           PUCHAR  Value
    )
/*++
    Description
        On success Value is populated with the value of the hex string.
        On failure, Value is undefined.
--*/
{
    ULONG       NibbleIndex;
    UCHAR       Nibbles[2];
    UCHAR       NibbleAscii;
    NDIS_STATUS NdisStatus = NDIS_STATUS_SUCCESS;

    for( NibbleIndex = 0; NibbleIndex < 2; NibbleIndex++ )
    {
        NibbleAscii = HexString[NibbleIndex];

        if( NibbleAscii >= 'a' && NibbleAscii <= 'f' )
        {
            Nibbles[NibbleIndex] = 10 + NibbleAscii - 'a';
        }
        else if( NibbleAscii >= 'A' && NibbleAscii <= 'F' )
        {
            Nibbles[NibbleIndex] = 10 + NibbleAscii - 'A';
        }
        else if( NibbleAscii >= '0' && NibbleAscii <= '9' )
        {
            Nibbles[NibbleIndex] = NibbleAscii - '0';
        }
        else
        {
            NdisStatus = NDIS_STATUS_FAILURE;
            break;
        }
    }

    if( NdisStatus == NDIS_STATUS_SUCCESS )
        *Value = (Nibbles[0] * 16) + Nibbles[1];

    return NdisStatus;
}

NDIS_STATUS
MbbUtilHexStringToByteStream(
    __in_bcount_z(HexStringSize) PCHAR  HexString,
    __in ULONG                          HexStringSize,
    __out_bcount(ByteStreamSize) PUCHAR ByteStream,
    __in ULONG                          ByteStreamSize
    )
{
    ULONG       i;
    NDIS_STATUS NdisStatus = NDIS_STATUS_SUCCESS;

    for( i = 0;
         i < ByteStreamSize &&
         ((i * 2) + 1) < HexStringSize;
         i ++ )
    {
        if( (NdisStatus = MbbUtilHexStringToUchar(
                               &HexString[i*2],
                               &ByteStream[i]
                                )) != NDIS_STATUS_SUCCESS )
        {
            break;
        }
    }
    return NdisStatus;
}

VOID
MbbUtilByteStreamToHexString(
    __in_bcount(ByteStreamSize) PUCHAR  ByteStream,
    __in ULONG                          ByteStreamSize,
    __out_bcount_z(HexStringSize) PCHAR HexString,
    __in ULONG                          HexStringSize
    )
{
    ULONG i;
    UCHAR UpperNibble;
    UCHAR LowerNibble;
    CHAR  AsciiTable[] = { '0', '1', '2', '3', '4', '5', '6', '7',
                           '8', '9', 'A', 'B', 'C', 'D', 'E', 'F' };

    for( i = 0;
         i < ByteStreamSize &&
         ((i * 2) + 1) < HexStringSize;
         i ++ )
    {
        UpperNibble = ( ByteStream[i] >> 4 ) & 0x0F;
        LowerNibble = ByteStream[i] & 0x0F;

        HexString[i*2]      = AsciiTable[UpperNibble];
        HexString[(i*2)+1]  = AsciiTable[LowerNibble];
    }
    HexString[MIN( (i*2), (HexStringSize-1) )] = 0;
}

NDIS_STATUS
MbbUtilWwanToMbbSmsSendPdu(
    __in  PWWAN_SMS_SEND_PDU                            WwanSmsSendPdu,
    __in  MBB_CELLULAR_CLASS                            CellularClass,
    __out PMBB_SMS_SEND                                *ReturnedMbbSmsSend,
    __out PULONG                                        MbbSmsSendSize
    )
{
    UCHAR       SmscAddressLength = 0;
    SIZE_T      PduDataCbLength;
    ULONG       PduDataLength;
    NDIS_STATUS NdisStatus = NDIS_STATUS_SUCCESS;
    PMBB_SMS_SEND_PDU  MbbSmsSendPdu=NULL;
    ULONG       CurrentOffset=0;
    ULONG       BufferSize = 0;
    PUCHAR      Buffer = NULL;

    ULONG       OuterBufferSize = 0;
    PUCHAR      OuterBuffer = NULL;
    ULONG       InitialOffset=0;

    do
    {
        //
        //  add some space at the beginning, for the contain struct
        //
        InitialOffset = FIELD_OFFSET( MBB_SMS_SEND, u.Pdu );

        if( CellularClass == MbbCellularClassGsm )
        {
            //
            //  the first two bytes are the hex encoded length of smsc address.
            //
            NdisStatus=MbbUtilHexStringToUchar(
                    WwanSmsSendPdu->PduData,
                    &SmscAddressLength
                    );


            if( NdisStatus != NDIS_STATUS_SUCCESS )
            {
                TraceError( WMBCLASS_OID, "[Util][Sms] INVALID hexstring for SMSC Address Length=%1c%1c",
                            WwanSmsSendPdu->PduData[0],
                            WwanSmsSendPdu->PduData[1]
                            );
                NdisStatus = NDIS_STATUS_INVALID_DATA;
                break;
            }

            //
            // The encoded size is for the raw data.
            // Add one byte for the SmscAddressLength itself.
            //
            SmscAddressLength++;
            PduDataLength = WwanSmsSendPdu->Size;

            //
            // Validate the buffer given by the service
            //
            if( (NdisStatus = RtlStringCbLengthA(
                                WwanSmsSendPdu->PduData,
                                sizeof(WwanSmsSendPdu->PduData),
                               &PduDataCbLength
                                )) != NDIS_STATUS_SUCCESS )
            {
                TraceError( WMBCLASS_OID, "[Util][Sms] FAILED to get string length for WwanSmsSendPdu->PduData, NtStatus=%!STATUS!", NdisStatus );
                break;
            }

            //
            //  PduDataCbLength is the length of the null terminated string of hex digits from the service.
            //  PduDataLength is the declarded length of the PDU data from service not including the smsc address
            //
            if( ((PduDataCbLength/2) - SmscAddressLength) != PduDataLength )
            {
                TraceError( WMBCLASS_UTIL, "[Util][Sms] INVALID WwanSmsSendPdu->Size=%d != PduDataStringLength=%d - SmscAddressLength=%d",
                            PduDataLength,
                            (ULONG)(PduDataCbLength/2),
                            SmscAddressLength
                            );
                NdisStatus = NDIS_STATUS_INVALID_DATA;
                break;
            }



            BufferSize= ROUND_UP_COUNT(FIELD_OFFSET(MBB_SMS_SEND_PDU, DataBuffer), ALIGN_DWORD);

            CurrentOffset=BufferSize;

            BufferSize+=(ULONG)ROUND_UP_COUNT(PduDataLength + SmscAddressLength, ALIGN_DWORD);

            //
            //  Outer buffer has storage for the containing structure as well
            //
            OuterBufferSize = InitialOffset + BufferSize;

            OuterBuffer = ALLOCATE_NONPAGED_POOL(OuterBufferSize);

            if (OuterBuffer == NULL )
            {
                return NDIS_STATUS_RESOURCES;
            }

            Buffer=OuterBuffer + InitialOffset;

            MbbSmsSendPdu=(PMBB_SMS_SEND_PDU)Buffer;


            //
            //  PduSize in the mbb message is the whole message len+smsc+pdudata
            //
            MbbSmsSendPdu->PduData.Size = PduDataLength + SmscAddressLength;
            MbbSmsSendPdu->PduData.Offset = ROUND_UP_COUNT(FIELD_OFFSET(MBB_SMS_SEND_PDU, DataBuffer), ALIGN_DWORD);

            //
            //  convert the hex encoded data to binary data
            //
            NdisStatus = MbbUtilHexStringToByteStream(
                                WwanSmsSendPdu->PduData,
                                (PduDataLength + SmscAddressLength) * 2,
                                ((PUCHAR)MbbSmsSendPdu) + MbbSmsSendPdu->PduData.Offset,
                                MbbSmsSendPdu->PduData.Size
                                );

            if (NdisStatus  != NDIS_STATUS_SUCCESS )
            {
                TraceError( WMBCLASS_UTIL, "[Util][Sms] FAILED to convert hexstring to bytestream NdisStatus=%!STATUS!", NdisStatus );
                break;
            }
        }
        else
        {
            BufferSize= ROUND_UP_COUNT(FIELD_OFFSET(MBB_SMS_SEND_PDU, DataBuffer), ALIGN_DWORD);

            CurrentOffset=BufferSize;

            BufferSize+=(ULONG)ROUND_UP_COUNT(WwanSmsSendPdu->Size, ALIGN_DWORD);

            //
            //  Outer buffer has storage for the containing structure as well
            //
            OuterBufferSize = InitialOffset + BufferSize;

            OuterBuffer = ALLOCATE_NONPAGED_POOL(OuterBufferSize);

            if (OuterBuffer == NULL )
            {
                return NDIS_STATUS_RESOURCES;
            }

            Buffer=OuterBuffer + InitialOffset;

            MbbSmsSendPdu=(PMBB_SMS_SEND_PDU)Buffer;

            MbbSmsSendPdu->PduData.Offset=CurrentOffset;
            MbbSmsSendPdu->PduData.Size=WwanSmsSendPdu->Size;

            RtlCopyMemory(
                ((PUCHAR)MbbSmsSendPdu) + MbbSmsSendPdu->PduData.Offset,
                WwanSmsSendPdu->PduData,
                WwanSmsSendPdu->Size
                );

            MbbSmsSendPdu->PduData.Size = WwanSmsSendPdu->Size;
        }

        *MbbSmsSendSize  = OuterBufferSize;

        *ReturnedMbbSmsSend = (PMBB_SMS_SEND)OuterBuffer;

        TraceInfo(  WMBCLASS_UTIL, "[Util][Sms] Received PDU send record, CurrentCellularClass=%!MbbCellularClass! PduDataSize=0x%08x SmscAddressLength=0x%08x",
                    CellularClass,
                    WwanSmsSendPdu->Size,
                    SmscAddressLength
                    );
        TraceInfo(  WMBCLASS_UTIL, "[Util][Sms] Converted to PDU record of size 0x%08x, PduDataSize=0x%08x",
                   *MbbSmsSendSize,
                    MbbSmsSendPdu->PduData.Size
                    );
    }
    while( FALSE );

    if (NdisStatus != NDIS_STATUS_SUCCESS)
    {
        if (OuterBuffer != NULL)
        {
            FREE_POOL(OuterBuffer);
            *MbbSmsSendSize = 0;
            *ReturnedMbbSmsSend = NULL;
        }
    }
    return NdisStatus;
}

NDIS_STATUS
MbbUtilWwanToMbbSmsSendCdma(
    __in  PWWAN_SMS_SEND_CDMA                               WwanSmsSendCdma,
    __out PMBB_SMS_SEND                                    *ReturnedMbbSmsSend,
    __out PULONG                                            MbbSmsSendSize
    )
{
    ULONG           CurrentOffset;
    SIZE_T          StringCbLength;
    ANSI_STRING     AnsiString;
    UNICODE_STRING  UnicodeString;
    NDIS_STATUS     NdisStatus = NDIS_STATUS_SUCCESS;
    WCHAR           TempWideString[WWAN_SMS_CDMA_ADDR_MAX_LEN];
    ULONG           BufferSize = 0;
    PUCHAR          Buffer = NULL;

    ULONG           OuterBufferSize = 0;
    PUCHAR          OuterBuffer = NULL;
    ULONG           InitialOffset=0;

    PMBB_SMS_SEND_CDMA  MbbSmsSendCdma=NULL;

    *ReturnedMbbSmsSend=NULL;
    *MbbSmsSendSize=0;

    do
    {
        //
        //  add some space at the beginning, for the contain struct
        //
        InitialOffset = FIELD_OFFSET( MBB_SMS_SEND, u.Cdma );


        BufferSize += ROUND_UP_COUNT( FIELD_OFFSET( MBB_SMS_SEND_CDMA, DataBuffer ), ALIGN_DWORD );

        //
        // Address
        //
        if( (NdisStatus = RtlStringCbLengthA(
                            WwanSmsSendCdma->Address,
                            WWAN_SMS_CDMA_ADDR_MAX_LEN,
                           &StringCbLength
                            )) != STATUS_SUCCESS )
        {
            TraceError( WMBCLASS_OID, "[Util][Sms] FAILED to get string length for CDMA Address NtStatus=%!STATUS!", NdisStatus );
            return NdisStatus;
        }

        BufferSize += ROUND_UP_COUNT( StringCbLength * sizeof(WCHAR), ALIGN_DWORD );

        BufferSize += ROUND_UP_COUNT( WwanSmsSendCdma->SizeInBytes, ALIGN_DWORD );

        //
        //  Outer buffer has storage for the containing structure as well
        //
        OuterBufferSize = InitialOffset + BufferSize;

#pragma prefast(suppress:__WARNING_MEMORY_LEAK, "By Design: Allocate to a outgoing reference ReturnedMbbSmsSend.")
        OuterBuffer = ALLOCATE_NONPAGED_POOL(OuterBufferSize);

        if (OuterBuffer == NULL )
        {
            return NDIS_STATUS_RESOURCES;
        }

        Buffer=OuterBuffer + InitialOffset;

        MbbSmsSendCdma=(PMBB_SMS_SEND_CDMA)Buffer;

        MbbSmsSendCdma->EncodingId         = (MBB_SMS_CDMA_ENCODING)WwanSmsSendCdma->EncodingId;
        MbbSmsSendCdma->LanguageId         = (MBB_SMS_CDMA_LANGUAGE)WwanSmsSendCdma->LanguageId;
        MbbSmsSendCdma->SizeInCharacters   = WwanSmsSendCdma->SizeInCharacters;



        CurrentOffset  = ROUND_UP_COUNT( FIELD_OFFSET( MBB_SMS_SEND_CDMA, DataBuffer ), ALIGN_DWORD );

        TraceInfo(  WMBCLASS_UTIL, "[Util][Sms] Received CDMA send record, EncodingId=0x%08x LanguageId=0x%08x Address[Sz=0x%08x] EncodedMessage[Sz=0x%08x SzCh=0x%08x]",
                    WwanSmsSendCdma->EncodingId,
                    WwanSmsSendCdma->LanguageId,
                    (ULONG)StringCbLength,
                    WwanSmsSendCdma->SizeInBytes,
                    WwanSmsSendCdma->SizeInCharacters
                    );

        AnsiString.Buffer           = WwanSmsSendCdma->Address;
        AnsiString.Length           = (USHORT)StringCbLength;
        AnsiString.MaximumLength    = WWAN_SMS_CDMA_ADDR_MAX_LEN;

        UnicodeString.Buffer        = &TempWideString[0];
        UnicodeString.Length        = 0;
        UnicodeString.MaximumLength = sizeof(TempWideString);

        if( (NdisStatus = RtlAnsiStringToUnicodeString(
                           &UnicodeString,
                           &AnsiString,
                            FALSE // AllocateDestinationString
                            )) != STATUS_SUCCESS )
        {
            TraceError( WMBCLASS_UTIL, "[Util][Sms] FAILED to convert WwanSmsSendCdma->Address Length=%dbytes, NtStatus=%!STATUS!",
                        AnsiString.Length, NdisStatus
                        );
            break;
        }

        CurrentOffset=MbbUtilWriteStringToBuffer(
            Buffer,
            BufferSize,
            CurrentOffset,
            &MbbSmsSendCdma->Address,
            (PUCHAR)&TempWideString[0],
            (ULONG)StringCbLength * sizeof(WCHAR)
            );

        CurrentOffset=MbbUtilWriteStringToBuffer(
            Buffer,
            BufferSize,
            CurrentOffset,
            (PMBB_STRING)&MbbSmsSendCdma->EncodedMessage,
            (PUCHAR)WwanSmsSendCdma->EncodedMsg,
            WwanSmsSendCdma->SizeInBytes
            );

        //
        // MessageSize
        //
        *MbbSmsSendSize = OuterBufferSize;
        *ReturnedMbbSmsSend = (PMBB_SMS_SEND)OuterBuffer;

        TraceInfo(  WMBCLASS_OID, "[Util][Sms] Converted to CDMA send record of size 0x%08x, Address[Off=0x%08x Sz=0x%08x] EncodedMessage[Off=0x%08x Sz=0x%08x SzCh=0x%08x]",
                   *MbbSmsSendSize,
                    MbbSmsSendCdma->Address.Offset,
                    MbbSmsSendCdma->Address.Size,
                    MbbSmsSendCdma->EncodedMessage.Offset,
                    MbbSmsSendCdma->EncodedMessage.Size,
                    MbbSmsSendCdma->SizeInCharacters
                    );
    }
    while( FALSE );

    return NdisStatus;
}


VOID
MbbUtilWwanToMbbSmsDelete(
    __in  PWWAN_SMS_FILTER  WwanSmsFilter,
    __out PMBB_SMS_DELETE   MbbSmsDelete
    )
{
    MbbSmsDelete->SmsFlags      = (MBB_SMS_FLAG)WwanSmsFilter->Flag;
    MbbSmsDelete->MessageIndex  = WwanSmsFilter->MessageIndex;
}

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
    )
{
    ULONG                               i = 0;
    PMBB_DS                             deviceServiceEntry = NULL;
    NDIS_STATUS                         NdisStatus = NDIS_STATUS_SUCCESS;
    ULONG                               ResultSize = 0;
    PMBB_SUBSCRIBE_EVENT_LIST           MbbSubscribeList = NULL;
    ULONG                               MbbSubscribeListSize = 0;
    PMBB_SUBSCRIBE_EVENT_ENTRY          EventEntry = NULL;
    ULONG                               DeviceServiceCount = 0;
    ULONG                               CurrentOffset;

    *OutputSubscribeList = NULL;
    *OutputSubscribeBufferSize = 0;

    do
    {
        //
        // Determine the size of the total list by going through all device services
        // & combining that with the CID list length
        //
        MbbSubscribeListSize = FIELD_OFFSET(MBB_SUBSCRIBE_EVENT_LIST, ArrayElement);

        for (i = 0; i < GuidCount; i++)
        {
            // If this is an external request, reject native services
            //
            if (ExternalRequest && MbbUtilIsNativeMbnService(&GuidList[i]))
            {            
                // Native device services cannot be subscribed to using this path
                TraceError( WMBCLASS_OID, "[Util] FAILED because of subscribe request for native device service %!GUID!",
                            &GuidList[i]
                            );

                NdisStatus = NDIS_STATUS_INVALID_DATA;
                break;
            }

            //
            // Find this device service so that we can get the CID list
            //
            deviceServiceEntry =  MbbUtilFindDeviceService(Adapter,
                                    &GuidList[i]
                                    );
            if (deviceServiceEntry == NULL)
            {
                // Unsupported device service
                if (ExternalRequest)
                {
                    TraceError( WMBCLASS_OID, "[Util] FAILED because of subscribe request for unsupported device service %!GUID!",
                                &GuidList[i]
                                );

                    NdisStatus = NDIS_STATUS_INVALID_DATA;
                    break;
                }
                else
                {
                    // Just move to the next for internal GUID lists
                    continue;
                }
            }

            MbbSubscribeListSize += sizeof(MBB_ARRAY_ELEMENT);

            // Space for this GUID & its CID list
            MbbSubscribeListSize += (FIELD_OFFSET(MBB_SUBSCRIBE_EVENT_ENTRY, CIDList) + 
                            deviceServiceEntry->CIDCount * sizeof(ULONG)
                            );

            DeviceServiceCount++;

        }
        
        if (NdisStatus != NDIS_STATUS_SUCCESS)
        {
            // Failed due to bad device service in the list
            break;
        }

        if ((ExtSubscribeList != NULL) &&  (ExtSubscribeListSize > (ULONG)FIELD_OFFSET(MBB_SUBSCRIBE_EVENT_LIST, ArrayElement)))
        {
            //
            //  we have another list to merge into this one
            //
            for (i=0; i<ExtSubscribeList->Count; i++)
            {
                DeviceServiceCount++;
                MbbSubscribeListSize += sizeof(MBB_ARRAY_ELEMENT);
                MbbSubscribeListSize += ExtSubscribeList->ArrayElement[i].Size;

            }
        }

        //
        // Now allocate memory for the OID's device services subscription
        //
        MbbSubscribeList = ALLOCATE_NONPAGED_POOL( MbbSubscribeListSize);

        if ( MbbSubscribeList == NULL )
        {
            TraceError( WMBCLASS_OID, "[Util] FAILED to allocate %d bytes for OID MBB_SUBSCRIBE_EVENT_LIST, ElementCount=%d",
                        MbbSubscribeListSize,
                        GuidCount
                        );
            NdisStatus = NDIS_STATUS_RESOURCES;
            break;
        }

        MbbSubscribeList->Count = DeviceServiceCount;
        CurrentOffset=FIELD_OFFSET(MBB_SUBSCRIBE_EVENT_LIST, ArrayElement) + (sizeof(MbbSubscribeList->ArrayElement[0]) * MbbSubscribeList->Count);

        // Start copying

        DeviceServiceCount=0;

        for (i = 0; i < GuidCount ; i++)
        {
            // Find the device service again (validation for the service is already done)
            deviceServiceEntry =  MbbUtilFindDeviceService(Adapter,
                                    &GuidList[i]
                                    );

            if (deviceServiceEntry != NULL)
            {

                EventEntry =(PMBB_SUBSCRIBE_EVENT_ENTRY)((PBYTE)MbbSubscribeList+CurrentOffset);

                // OACR complains below that the size is small, but its really not because we
                // checked before for the size

                // Copy the GUID (changing length)
                MBB_UUID_TO_NET(&EventEntry->DeviceServiceId,
                    &GuidList[i]
                    );

                // CID Count
                EventEntry->CIDCount = deviceServiceEntry->CIDCount;

                // Copy the CIDs
                RtlCopyMemory(EventEntry->CIDList,
                    deviceServiceEntry->CIDList,
                    sizeof(ULONG) * EventEntry->CIDCount
                    );


                MbbSubscribeList->ArrayElement[DeviceServiceCount].Offset=CurrentOffset;
                MbbSubscribeList->ArrayElement[DeviceServiceCount].Size= FIELD_OFFSET(MBB_SUBSCRIBE_EVENT_ENTRY, CIDList) + deviceServiceEntry->CIDCount * sizeof(ULONG);

                CurrentOffset += FIELD_OFFSET(MBB_SUBSCRIBE_EVENT_ENTRY, CIDList) + deviceServiceEntry->CIDCount * sizeof(ULONG);

                DeviceServiceCount++;

            }
            else
            {
                // Not supported by device, skip
            }

        }

        if ((ExtSubscribeList != NULL) &&  (ExtSubscribeListSize > (ULONG)FIELD_OFFSET(MBB_SUBSCRIBE_EVENT_LIST, ArrayElement)))
        {
            //
            //  we have another list to merge into this one
            //
            for (i=0; i<ExtSubscribeList->Count; i++)
            {
                EventEntry =(PMBB_SUBSCRIBE_EVENT_ENTRY)((PBYTE)MbbSubscribeList+CurrentOffset);

                MbbSubscribeList->ArrayElement[DeviceServiceCount].Offset=CurrentOffset;
                MbbSubscribeList->ArrayElement[DeviceServiceCount].Size= ExtSubscribeList->ArrayElement[i].Size;

                RtlCopyMemory(
                    EventEntry,
                    ((PBYTE)ExtSubscribeList+ExtSubscribeList->ArrayElement[i].Offset),
                    ExtSubscribeList->ArrayElement[i].Size
                    );

                CurrentOffset +=  ExtSubscribeList->ArrayElement[i].Size;

                DeviceServiceCount++;
            }
        }

        
        *OutputSubscribeList = MbbSubscribeList;
        *OutputSubscribeBufferSize = MbbSubscribeListSize;

    } while (FALSE);

    if (NdisStatus != NDIS_STATUS_SUCCESS)
    {
        if (MbbSubscribeList)
        {
            FREE_POOL(MbbSubscribeList);
        }
    }

    return NdisStatus;
}

NDIS_STATUS
MbbUtilGenerateLowPowerSubscribeEventList(
    __in PMINIPORT_ADAPTER_CONTEXT      Adapter,
    __in ULONG                          MediaSpecificWakeUpEvents,
    __in ULONG                          WakeUpFlags,
    __out PMBB_SUBSCRIBE_EVENT_LIST*    OutputSubscribeList,
    __out PULONG                        OutputSubscribeListSize
    )
{
    NDIS_STATUS                         NdisStatus = NDIS_STATUS_SUCCESS;
    ULONG                               NativeSubscribeCount=0;
    ULONG                               NativeSubscribeListSize = 0;
    PMBB_SUBSCRIBE_EVENT_LIST           NativeSubscribeList = NULL;
    PMBB_SUBSCRIBE_EVENT_ENTRY          EventEntry = NULL;
    ULONG                               i = 0;
    ULONG                               CurrentOffset = 0;
    ULONG                               CurrentEvent = 0;
    ULONG                               BasicConnectCommands = 0;
    
    do
    {
        // Determine required size based on MediaSpecicWakeUpEvents and WakeUpFlags (grouped by service ID)

        // Count number of MBB_UUID_BASIC_CONNECT_CONSTANT wake ups
        if (MediaSpecificWakeUpEvents & NDIS_WWAN_WAKE_ON_REGISTER_STATE_ENABLED)
        {
            BasicConnectCommands++;
        }

        if (WakeUpFlags & NDIS_PM_WAKE_ON_MEDIA_DISCONNECT_ENABLED)
        {
            BasicConnectCommands++;
        }

        if (MediaSpecificWakeUpEvents & NDIS_WWAN_WAKE_ON_PACKET_STATE_ENABLED)
        {
            BasicConnectCommands++;
        }

        if (MediaSpecificWakeUpEvents & NDIS_WWAN_WAKE_ON_UICC_CHANGE_ENABLED)
        {
            BasicConnectCommands++;
        }

        if (BasicConnectCommands > 0)
        {
            NativeSubscribeCount++;
            NativeSubscribeListSize += RTL_SIZEOF_THROUGH_FIELD(MBB_SUBSCRIBE_EVENT_ENTRY, CIDList) ;

            NativeSubscribeListSize += sizeof(ULONG) * BasicConnectCommands-1;
        }

        // Count number of MBB_UUID_SMS_CONSTANT wake ups
        if (MediaSpecificWakeUpEvents & NDIS_WWAN_WAKE_ON_SMS_RECEIVE_ENABLED)
        {
            NativeSubscribeCount++;
            NativeSubscribeListSize +=  RTL_SIZEOF_THROUGH_FIELD(MBB_SUBSCRIBE_EVENT_ENTRY, CIDList) ;
        }

        // Count number of MBB_UUID_USSD_CONSTANT wake ups
        if (MediaSpecificWakeUpEvents & NDIS_WWAN_WAKE_ON_USSD_RECEIVE_ENABLED)
        {
            NativeSubscribeCount++;
            NativeSubscribeListSize +=  RTL_SIZEOF_THROUGH_FIELD(MBB_SUBSCRIBE_EVENT_ENTRY, CIDList) ;
        }

        NativeSubscribeListSize += FIELD_OFFSET(MBB_SUBSCRIBE_EVENT_LIST, ArrayElement);
        NativeSubscribeListSize += sizeof(MBB_ARRAY_ELEMENT) * NativeSubscribeCount;

        if( (NativeSubscribeList = ALLOCATE_NONPAGED_POOL( NativeSubscribeListSize )) == NULL )
        {
            TraceError( WMBCLASS_OID, "[Util] FAILED to allocate %d bytes for low power MBB_SUBSCRIBE_EVENT_LIST",
                        NativeSubscribeListSize
                        );
            NdisStatus = NDIS_STATUS_RESOURCES;
            break;
        }

        *OutputSubscribeListSize = NativeSubscribeListSize;
        NativeSubscribeList->Count = NativeSubscribeCount;

        CurrentOffset=FIELD_OFFSET(MBB_SUBSCRIBE_EVENT_LIST, ArrayElement) + (sizeof(NativeSubscribeList->ArrayElement[0]) * NativeSubscribeList->Count);

        // Populate the subsribe list with the MBB_UUID_BASIC_CONNECT_CONSTANT wake ups
        if (BasicConnectCommands > 0)
        {
            GUID TempGuid=MBB_UUID_BASIC_CONNECT_CONSTANT;

            EventEntry =(PMBB_SUBSCRIBE_EVENT_ENTRY)((PBYTE)NativeSubscribeList+CurrentOffset);

            MBB_UUID_TO_NET(&EventEntry->DeviceServiceId, &TempGuid);
            EventEntry->CIDCount = BasicConnectCommands;

            i=0;

            if (MediaSpecificWakeUpEvents & NDIS_WWAN_WAKE_ON_REGISTER_STATE_ENABLED)
            {
                EventEntry->CIDList[i]=MBB_BASIC_CID_REGISTER_STATE;
                i++;
            }
            if (WakeUpFlags & NDIS_PM_WAKE_ON_MEDIA_DISCONNECT_ENABLED)
            {
                EventEntry->CIDList[i]=MBB_BASIC_CID_CONNECT;
                i++;
            }
            if (MediaSpecificWakeUpEvents & NDIS_WWAN_WAKE_ON_PACKET_STATE_ENABLED)
            {
                EventEntry->CIDList[i]=MBB_BASIC_CID_PACKET_SERVICE;
                i++;
            }
            if (MediaSpecificWakeUpEvents & NDIS_WWAN_WAKE_ON_UICC_CHANGE_ENABLED)
            {
                EventEntry->CIDList[i]=MBB_BASIC_CID_SUBSCRIBER_READY_INFO;
                i++;
            }

            NativeSubscribeList->ArrayElement[CurrentEvent].Offset=CurrentOffset;
            NativeSubscribeList->ArrayElement[CurrentEvent].Size= RTL_SIZEOF_THROUGH_FIELD(MBB_SUBSCRIBE_EVENT_ENTRY, CIDList[i-1]);

            CurrentEvent++;
            // Next event entry
            CurrentOffset += RTL_SIZEOF_THROUGH_FIELD(MBB_SUBSCRIBE_EVENT_ENTRY, CIDList[i-1]);
        }

        // Populate the subsribe list with the MBB_UUID_SMS_CONSTANT wake ups
        if (MediaSpecificWakeUpEvents & NDIS_WWAN_WAKE_ON_SMS_RECEIVE_ENABLED)
        {
            GUID TempGuid=MBB_UUID_SMS_CONSTANT;

            EventEntry =(PMBB_SUBSCRIBE_EVENT_ENTRY)((PBYTE)NativeSubscribeList+CurrentOffset);

            MBB_UUID_TO_NET(&EventEntry->DeviceServiceId, &TempGuid);
            EventEntry->CIDCount = 1;
            EventEntry->CIDList[0]=MBB_SMS_CID_STATUS;

            // Next event entry
            NativeSubscribeList->ArrayElement[CurrentEvent].Offset=CurrentOffset;
            NativeSubscribeList->ArrayElement[CurrentEvent].Size= RTL_SIZEOF_THROUGH_FIELD(MBB_SUBSCRIBE_EVENT_ENTRY, CIDList);

            CurrentEvent++;

            // Next event entry
            CurrentOffset += RTL_SIZEOF_THROUGH_FIELD(MBB_SUBSCRIBE_EVENT_ENTRY, CIDList);
        }

        // Populate the subsribe list with the MBB_UUID_USSD_CONSTANT wake ups
        if (MediaSpecificWakeUpEvents & NDIS_WWAN_WAKE_ON_USSD_RECEIVE_ENABLED)
        {
            GUID TempGuid=MBB_UUID_USSD_CONSTANT;

            EventEntry =(PMBB_SUBSCRIBE_EVENT_ENTRY)((PBYTE)NativeSubscribeList+CurrentOffset);

            MBB_UUID_TO_NET(&EventEntry->DeviceServiceId, &TempGuid);
            EventEntry->CIDCount = 1;
            EventEntry->CIDList[0]=MBB_USSD_CID_USSD;

            // Next event entry
            NativeSubscribeList->ArrayElement[CurrentEvent].Offset=CurrentOffset;
            NativeSubscribeList->ArrayElement[CurrentEvent].Size= RTL_SIZEOF_THROUGH_FIELD(MBB_SUBSCRIBE_EVENT_ENTRY, CIDList);

            CurrentEvent++;

            // Next event entry
            CurrentOffset += RTL_SIZEOF_THROUGH_FIELD(MBB_SUBSCRIBE_EVENT_ENTRY, CIDList);
        }

        *OutputSubscribeList = NativeSubscribeList;
        NativeSubscribeList = NULL;
    }
    while (FALSE);

    if (NativeSubscribeList != NULL)
        FREE_POOL(NativeSubscribeList);

    return NdisStatus;
}

NDIS_STATUS
MbbUtilGenerateFullPowerSubscribeEventList(
    __in PMINIPORT_ADAPTER_CONTEXT      Adapter,
    __in_bcount(ExtSubscribeListSize) PMBB_SUBSCRIBE_EVENT_LIST ExtSubscribeList,
    __in ULONG                          ExtSubscribeListSize,
    __out PMBB_SUBSCRIBE_EVENT_LIST*    OutputSubscribeList,
    __out PULONG                        OutputSubscribeListSize
    )
{
    NDIS_STATUS                         NdisStatus = NDIS_STATUS_SUCCESS;
    PMBB_SUBSCRIBE_EVENT_LIST           MergedSubscribeList = NULL;
    ULONG                               MergedSubscribeBufferSize = 0;

    *OutputSubscribeList = NULL;
    *OutputSubscribeListSize = 0;
    do
    {
        //
        // This function's job is to take the native device serices & create
        // the subscribe list (with CIDs) for them. Also, take the extensibility device service subscribe list 
        // and merge it into the other list. It optimizes memory management by allocating memory
        // for both lists when it creates the native device service's subscribe list
        //

        // Fill the native one & leave space for the extensibility list
        NdisStatus = MbbUtilWwanToMbbSubscribeEvents(Adapter, 
                            NativeMbnServices,
                            ARRAYSIZE(NativeMbnServices),
                            FALSE,
                            ExtSubscribeList,
                            ExtSubscribeListSize,
                            &MergedSubscribeList,
                            &MergedSubscribeBufferSize
                            );

        if( NdisStatus !=  NDIS_STATUS_SUCCESS)
        {
            TraceError( WMBCLASS_OID, "[Util] FAILED to allocate memory for merged MBB_SUBSCRIBE_EVENT_LIST");
            break;
        }


        *OutputSubscribeList = MergedSubscribeList;
        *OutputSubscribeListSize = MergedSubscribeBufferSize;
        MergedSubscribeList = NULL;
    }
    while (FALSE);

    if (MergedSubscribeList != NULL)
        FREE_POOL(MergedSubscribeList);

    return NdisStatus;
}

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
    )
{
    if (FullPower)
    {
        // Will generate the full list
        return MbbUtilGenerateFullPowerSubscribeEventList(
                    Adapter,
                    ExtSubscribeList,
                    ExtSubscribeListSize,
                    OutputSubscribeList,
                    OutputSubscribeListSize
                    );
    }
    else
    {
        // Generate a limit list
        return MbbUtilGenerateLowPowerSubscribeEventList(
                    Adapter,
                    MediaSpecificWakeUpEvents,
                    WakeUpFlags,
                    OutputSubscribeList,
                    OutputSubscribeListSize
                    );
    }

}

NDIS_STATUS
MbbUtilWwanToMbbAkapAuthChallenge(
    __in  PWWAN_AUTH_AKAP_CHALLENGE                 WwanAuthAkapChallenge,
    __in  ULONG                                     UTF16ByteCount,
    __in  ULONG                                     MbbSize,
    __out_bcount(MbbSize) PMBB_AKAP_AUTH_CHALLENGE  MbbAkapAuthChallenge
    )
{
    ULONG           CurrentOffset;
    NDIS_STATUS     NdisStatus = NDIS_STATUS_SUCCESS;
    ANSI_STRING     AnsiString;
    UNICODE_STRING  UnicodeString;

    if( MbbSize < sizeof(MBB_AKAP_AUTH_CHALLENGE) )
    {
        return NDIS_STATUS_BUFFER_OVERFLOW;
    }
    RtlCopyMemory(
        MbbAkapAuthChallenge->Rand,
        WwanAuthAkapChallenge->Rand,
        WWAN_AUTH_RAND_LEN
        );
    RtlCopyMemory(
        MbbAkapAuthChallenge->Autn,
        WwanAuthAkapChallenge->Autn,
        WWAN_AUTH_AUTN_LEN
        );
    CurrentOffset = ROUND_UP_COUNT( FIELD_OFFSET( MBB_AKAP_AUTH_CHALLENGE, DataBuffer ), ALIGN_DWORD );
    //
    // NetworkName
    //
    if( MbbSize < (CurrentOffset + UTF16ByteCount) )
    {
        return NDIS_STATUS_BUFFER_OVERFLOW;
    }
    NdisStatus = RtlUTF8ToUnicodeN(
                    (PWCHAR)(((PCHAR)MbbAkapAuthChallenge) + CurrentOffset),
                    UTF16ByteCount,
                    NULL,
                    (PCCH)(WwanAuthAkapChallenge->NetworkName),
                    WwanAuthAkapChallenge->NetworkNameLength
                    );
    if( STATUS_SUCCESS != NdisStatus )
    {
        TraceError( WMBCLASS_OID, "[Util] FAILED RtlUTF8ToUnicodeN(AuthAkap.NetworkName=%s)=%!STATUS!, ignoring=%!BOOLEAN!",
                    WwanAuthAkapChallenge->NetworkName,
                    NdisStatus,
                    (STATUS_SOME_NOT_MAPPED == NdisStatus)
                    );
        if( STATUS_SOME_NOT_MAPPED != NdisStatus )
            return NdisStatus;
        else
            NdisStatus = STATUS_SUCCESS;
    }

    MbbAkapAuthChallenge->NetworkName.Size   = UTF16ByteCount;
    MbbAkapAuthChallenge->NetworkName.Offset = CurrentOffset;
    return NdisStatus;
}

NDIS_STATUS
MbbUtilWwanToMbbUiccFilePath(
    __in        PWWAN_UICC_FILE_PATH  WwanUiccFilePath,
    __deref_out PMBB_UICC_FILE_PATH   *MbbUiccFilePathOut,
    __out       PULONG                ReturnedBufferSize
    )
{
    PUCHAR                  Buffer;
    ULONG                   CurrentOffset;
    ULONG                   BufferSize;
    PMBB_UICC_FILE_PATH     MbbUiccFilePath = NULL;

    *ReturnedBufferSize = 0;

    BufferSize = (ULONG)ROUND_UP_COUNT(FIELD_OFFSET(MBB_UICC_FILE_PATH, DataBuffer), ALIGN_DWORD);

    CurrentOffset = BufferSize;

    BufferSize += (ULONG)ROUND_UP_COUNT(WwanUiccFilePath->AppIdLength, ALIGN_DWORD);
    BufferSize += (ULONG)ROUND_UP_COUNT(WwanUiccFilePath->FilePathLength * sizeof(USHORT), ALIGN_DWORD);

    if ((MbbUiccFilePath = ALLOCATE_NONPAGED_POOL(BufferSize)) == NULL)
    {
        return NDIS_STATUS_RESOURCES;
    }

    Buffer = (PUCHAR)MbbUiccFilePath;

    CurrentOffset = MbbUtilWriteStringToBuffer(
        Buffer,
        BufferSize,
        CurrentOffset,
        (PMBB_STRING)&MbbUiccFilePath->AppId,
        (PUCHAR)WwanUiccFilePath->AppId,
        WwanUiccFilePath->AppIdLength
        );

    CurrentOffset = MbbUtilWriteStringToBuffer(
        Buffer,
        BufferSize,
        CurrentOffset,
        (PMBB_STRING)&MbbUiccFilePath->FilePath,
        (PUCHAR)WwanUiccFilePath->FilePath,
        WwanUiccFilePath->FilePathLength * sizeof(USHORT)
        );

    *MbbUiccFilePathOut = MbbUiccFilePath;

    *ReturnedBufferSize = BufferSize;

    return NDIS_STATUS_SUCCESS;
}

NDIS_STATUS
MbbUtilWwanToMbbUiccAccessBinary(
    __in            PWWAN_UICC_ACCESS_BINARY  WwanUiccAccessBinary,
    __deref_out_opt PMBB_UICC_ACCESS_BINARY   *MbbUiccAccessBinaryOut,
    __out           PULONG                    ReturnedBufferSize
    )
{
    PUCHAR                  Buffer;
    ULONG                   CurrentOffset;
    ULONG                   BufferSize;
    PMBB_UICC_ACCESS_BINARY MbbUiccAccessBinary = NULL;
    NDIS_STATUS             NdisStatus = NDIS_STATUS_SUCCESS;
    ULONG                   MbbUiccFilePathLength;
    PMBB_UICC_FILE_PATH     MbbUiccFilePath = NULL;

    *MbbUiccAccessBinaryOut = NULL;
    *ReturnedBufferSize = 0;

    BufferSize = (ULONG)ROUND_UP_COUNT(FIELD_OFFSET(MBB_UICC_ACCESS_BINARY, DataBuffer), ALIGN_DWORD);

    CurrentOffset = BufferSize;

    NdisStatus = MbbUtilWwanToMbbUiccFilePath(
        &WwanUiccAccessBinary->UiccFilePath,
        &MbbUiccFilePath,
        &MbbUiccFilePathLength
        );

    if (NdisStatus != NDIS_STATUS_SUCCESS)
    {
        goto Cleanup;
    }

    BufferSize += (ULONG)ROUND_UP_COUNT(MbbUiccFilePathLength, ALIGN_DWORD);
    BufferSize += (ULONG)ROUND_UP_COUNT(WwanUiccAccessBinary->LocalPinSize, ALIGN_DWORD);
    BufferSize += (ULONG)ROUND_UP_COUNT(WwanUiccAccessBinary->BinaryDataSize, ALIGN_DWORD);

    if ((MbbUiccAccessBinary = ALLOCATE_NONPAGED_POOL(BufferSize)) == NULL)
    {
        NdisStatus = NDIS_STATUS_RESOURCES;
        goto Cleanup;
    }

    Buffer = (PUCHAR)MbbUiccAccessBinary;

    CurrentOffset = MbbUtilWriteStringToBuffer(
        Buffer,
        BufferSize,
        CurrentOffset,
        (PMBB_STRING)&MbbUiccAccessBinary->UiccFilePath,
        (PUCHAR)MbbUiccFilePath,
        MbbUiccFilePathLength
        );

    CurrentOffset = MbbUtilWriteStringToBuffer(
        Buffer,
        BufferSize,
        CurrentOffset,
        (PMBB_STRING)&MbbUiccAccessBinary->LocalPin,
        (PUCHAR)WwanUiccAccessBinary->LocalPin,
        WwanUiccAccessBinary->LocalPinSize
        );

    CurrentOffset = MbbUtilWriteStringToBuffer(
        Buffer,
        BufferSize,
        CurrentOffset,
        (PMBB_STRING)&MbbUiccAccessBinary->BinaryData,
        (PUCHAR)WwanUiccAccessBinary->BinaryData,
        WwanUiccAccessBinary->BinaryDataSize
        );

    MbbUiccAccessBinary->FileOffset     = WwanUiccAccessBinary->FileOffset;
    MbbUiccAccessBinary->NumberOfBytes  = WwanUiccAccessBinary->NumberOfBytes;

    *MbbUiccAccessBinaryOut = MbbUiccAccessBinary;
    *ReturnedBufferSize = BufferSize;

Cleanup:
    if (NULL != MbbUiccFilePath)
    {
        FREE_POOL(MbbUiccFilePath);
    }

    return NdisStatus;
}

NDIS_STATUS
MbbUtilWwanToMbbUiccAccessRecord(
    __in            PWWAN_UICC_ACCESS_RECORD  WwanUiccAccessRecord,
    __deref_out_opt PMBB_UICC_ACCESS_RECORD   *MbbUiccAccessRecordOut,
    __out           PULONG                    ReturnedBufferSize
    )
{
    PUCHAR                  Buffer;
    ULONG                   CurrentOffset;
    ULONG                   BufferSize;
    PMBB_UICC_ACCESS_RECORD MbbUiccAccessRecord = NULL;
    NDIS_STATUS             NdisStatus = NDIS_STATUS_SUCCESS;
    ULONG                   MbbUiccFilePathLength;
    PMBB_UICC_FILE_PATH     MbbUiccFilePath = NULL;

    *MbbUiccAccessRecordOut = NULL;
    *ReturnedBufferSize = 0;

    BufferSize = (ULONG)ROUND_UP_COUNT(FIELD_OFFSET(MBB_UICC_ACCESS_RECORD, DataBuffer), ALIGN_DWORD);

    CurrentOffset = BufferSize;

    NdisStatus = MbbUtilWwanToMbbUiccFilePath(
        &WwanUiccAccessRecord->UiccFilePath,
        &MbbUiccFilePath,
        &MbbUiccFilePathLength
        );

    if (NdisStatus != NDIS_STATUS_SUCCESS)
    {
        goto Cleanup;
    }

    BufferSize += (ULONG)ROUND_UP_COUNT(MbbUiccFilePathLength, ALIGN_DWORD);
    BufferSize += (ULONG)ROUND_UP_COUNT(WwanUiccAccessRecord->LocalPinSize, ALIGN_DWORD);
    BufferSize += (ULONG)ROUND_UP_COUNT(WwanUiccAccessRecord->RecordDataSize, ALIGN_DWORD);

    if ((MbbUiccAccessRecord = ALLOCATE_NONPAGED_POOL(BufferSize)) == NULL)
    {
        NdisStatus = NDIS_STATUS_RESOURCES;
        goto Cleanup;
    }

    Buffer = (PUCHAR)MbbUiccAccessRecord;

    CurrentOffset = MbbUtilWriteStringToBuffer(
        Buffer,
        BufferSize,
        CurrentOffset,
        (PMBB_STRING)&MbbUiccAccessRecord->UiccFilePath,
        (PUCHAR)MbbUiccFilePath,
        MbbUiccFilePathLength
        );

    CurrentOffset = MbbUtilWriteStringToBuffer(
        Buffer,
        BufferSize,
        CurrentOffset,
        (PMBB_STRING)&MbbUiccAccessRecord->LocalPin,
        (PUCHAR)WwanUiccAccessRecord->LocalPin,
        WwanUiccAccessRecord->LocalPinSize
        );

    CurrentOffset = MbbUtilWriteStringToBuffer(
        Buffer,
        BufferSize,
        CurrentOffset,
        (PMBB_STRING)&MbbUiccAccessRecord->RecordData,
        (PUCHAR)WwanUiccAccessRecord->RecordData,
        WwanUiccAccessRecord->RecordDataSize
        );

    MbbUiccAccessRecord->RecordNumber   = WwanUiccAccessRecord->RecordNumber;

    *MbbUiccAccessRecordOut = MbbUiccAccessRecord;
    *ReturnedBufferSize = BufferSize;

Cleanup:
    if (NULL != MbbUiccFilePath)
    {
        FREE_POOL(MbbUiccFilePath);
    }

    return NdisStatus;
}

NDIS_STATUS
MbbUtilWwanToMbbPinApp(
    __in        PWWAN_PIN_APP         WwanPinApp,
    __deref_out PMBB_PIN_APP          *MbbPinAppOut,
    __out       PULONG                ReturnedBufferSize
)
{
    PUCHAR                  Buffer;
    ULONG                   CurrentOffset;
    ULONG                   BufferSize;
    PMBB_PIN_APP            MbbPinApp = NULL;

    *ReturnedBufferSize = 0;
    *MbbPinAppOut = NULL;

    BufferSize = (ULONG)ROUND_UP_COUNT(FIELD_OFFSET(MBB_PIN_APP, DataBuffer), ALIGN_DWORD);

    CurrentOffset = BufferSize;

    BufferSize += (ULONG)ROUND_UP_COUNT(WwanPinApp->AppIdLength, ALIGN_DWORD);

    if ((MbbPinApp = ALLOCATE_NONPAGED_POOL(BufferSize)) == NULL)
    {
        return NDIS_STATUS_RESOURCES;
    }

    Buffer = (PUCHAR)MbbPinApp;

    CurrentOffset = MbbUtilWriteStringToBuffer(
        Buffer,
        BufferSize,
        CurrentOffset,
        (PMBB_STRING)&MbbPinApp->AppId,
        (PUCHAR)WwanPinApp->AppId,
        WwanPinApp->AppIdLength
    );

    *MbbPinAppOut = MbbPinApp;

    *ReturnedBufferSize = BufferSize;

    return NDIS_STATUS_SUCCESS;
}

NDIS_STATUS
MbbUtilWwanToMbbSetDeviceSlotMappingInfo(
    __in            PWWAN_DEVICE_SLOT_MAPPING_INFO    WwanDeviceSlotMappingInfo,
    __deref_out_opt PMBB_MS_DEVICE_SLOT_MAPPING_INFO  *MbbDeviceSlotMappingInfoOut,
    __out           PULONG                            ReturnedBufferSize
)
{
    PUCHAR                              WwanBuffer = NULL;
    ULONG                               CurrentOffsetInWwanBuffer = 0;
    PUCHAR                              MbbBuffer = NULL;
    ULONG                               CurrentOffsetInMbbBuffer = 0;
    ULONG                               MbbBufferSize = 0;
    PMBB_MS_DEVICE_SLOT_MAPPING_INFO   MbbDeviceSlotMappingInfo = NULL;
    ULONG                               MapCount = 0;
    ULONG                               MapIndex = 0;

    WwanBuffer = (PUCHAR)WwanDeviceSlotMappingInfo;
    CurrentOffsetInWwanBuffer = sizeof(WWAN_DEVICE_SLOT_MAPPING_INFO);
    MapCount = WwanDeviceSlotMappingInfo->SlotMapListHeader.ElementCount;
    *MbbDeviceSlotMappingInfoOut = NULL;
    *ReturnedBufferSize = 0;

    // Calculate the buffer size for MbbDeviceSlotMappingInfo.
    MbbBufferSize = (ULONG)ROUND_UP_COUNT(FIELD_OFFSET(MBB_MS_DEVICE_SLOT_MAPPING_INFO, SlotMapList), ALIGN_DWORD);
    MbbBufferSize += (ULONG)ROUND_UP_COUNT(sizeof(MBB_ARRAY_ELEMENT) * MapCount, ALIGN_DWORD);

    CurrentOffsetInMbbBuffer = MbbBufferSize;

    MbbBufferSize += (ULONG)ROUND_UP_COUNT(sizeof(ULONG) * MapCount, ALIGN_DWORD);

    // Allocate the buffer for MbbDeviceSlotMappingInfo.
    MbbDeviceSlotMappingInfo = ALLOCATE_NONPAGED_POOL(MbbBufferSize);
    if (NULL == MbbDeviceSlotMappingInfo)
    {
        return NDIS_STATUS_RESOURCES;
    }

    MbbBuffer = (PUCHAR)MbbDeviceSlotMappingInfo;

    // Set the values of MbbDeviceSlotMappingInfo
    MbbDeviceSlotMappingInfo->MapCount = MapCount;
    CurrentOffsetInMbbBuffer = MbbUtilWwanListToMbbList(&(WwanDeviceSlotMappingInfo->SlotMapListHeader),
        sizeof(ULONG),
        MbbBuffer,
        CurrentOffsetInMbbBuffer,
        &(MbbDeviceSlotMappingInfo->SlotMapList[0]));

    *MbbDeviceSlotMappingInfoOut = MbbDeviceSlotMappingInfo;
    *ReturnedBufferSize = MbbBufferSize;

    return NDIS_STATUS_SUCCESS;
}

ULONG
MbbUtilWwanListToMbbList(
    __in    PWWAN_LIST_HEADER  WwanListHeader,
    __in    ULONG              WwanListElementSize,
    __inout PUCHAR             MbbBuffer,
    __in    ULONG              CurrentOffsetInMbbBuffer,
    __inout PMBB_ARRAY_ELEMENT MbbArray
    )
{
    PUCHAR WwanBuffer = (PUCHAR)WwanListHeader;
    ULONG  CurrentOffsetInWwanBuffer = sizeof(WWAN_LIST_HEADER);
    ULONG  MapCount = WwanListHeader->ElementCount;
    ULONG  MapIndex = 0;
    ULONG  ReturnedOffsetInMbbBuffer = CurrentOffsetInMbbBuffer;

    for (MapIndex = 0; MapIndex < MapCount; MapIndex++)
    {
        // Set the offsets and sizes.
        MbbArray[MapIndex].Offset = ReturnedOffsetInMbbBuffer;
        MbbArray[MapIndex].Size = WwanListElementSize;

        // Set the DataBuffer.
#pragma warning(suppress: 26007)
        RtlCopyMemory(MbbBuffer + ReturnedOffsetInMbbBuffer,
            WwanBuffer + CurrentOffsetInWwanBuffer,
            WwanListElementSize);

        ReturnedOffsetInMbbBuffer += WwanListElementSize;
        CurrentOffsetInWwanBuffer += WwanListElementSize;
    }

    return ReturnedOffsetInMbbBuffer;
}

NDIS_STATUS
MbbUtilWwanToMbbSetDeviceBindingsInfo(
    __in        PWWAN_DEVICE_BINDINGS_INFO    WwanSetDeviceBindingsInfo,
    __deref_out PMBB_DEVICE_BINDINGS_INFO     *MbbSetDeviceBindingsInfoOut,
    __out       PULONG                        ReturnedBufferSize
    )
{
    PUCHAR                      WwanBuffer = NULL;
    ULONG                       CurrentOffsetInWwanBuffer = 0;
    PUCHAR                      MbbBuffer = NULL;
    ULONG                       CurrentOffsetInMbbBuffer = 0;
    ULONG                       MbbBufferSize = 0;
    PMBB_DEVICE_BINDINGS_INFO   MbbDeviceBindingsInfo = NULL;
    NDIS_STATUS                 NdisStatus = NDIS_STATUS_SUCCESS;
    ULONG                       MbbUiccFilePathLength[MAX_CNT_UICC_APPS_BOUND_TO_DEVICE] = { 0 };
    PMBB_UICC_FILE_PATH         MbbUiccFilePath[MAX_CNT_UICC_APPS_BOUND_TO_DEVICE] = { NULL };
    ULONG                       AppCount = 0;
    ULONG                       AppIndex = 0;

    WwanBuffer = (PUCHAR)WwanSetDeviceBindingsInfo;
    CurrentOffsetInWwanBuffer = sizeof(WWAN_DEVICE_BINDINGS_INFO);
    AppCount = WwanSetDeviceBindingsInfo->ApplicationListHeader.ElementCount;
    *ReturnedBufferSize = 0;

    // WWAN_DEVICE_BINDINGS_INFO can specify at most MAX_CNT_UICC_APPS_BOUND_TO_DEVICE (= 2) UICC applications.
    if (AppCount > MAX_CNT_UICC_APPS_BOUND_TO_DEVICE)
    {
        return NDIS_STATUS_INVALID_PARAMETER;
    }

    // Generate the MBB_UICC_FILE_PATH based on WWAN_UICC_FILE_PATH and calculate the MbbBufferSize.
    MbbBufferSize = (ULONG)ROUND_UP_COUNT(FIELD_OFFSET(MBB_DEVICE_BINDINGS_INFO, ApplicationList), ALIGN_DWORD);
    MbbBufferSize += (ULONG)ROUND_UP_COUNT(sizeof(MBB_ARRAY_ELEMENT) * AppCount, ALIGN_DWORD);
    CurrentOffsetInMbbBuffer = MbbBufferSize;

    for (AppIndex = 0; AppIndex < AppCount; AppIndex++)
    {
        NdisStatus = MbbUtilWwanToMbbUiccFilePath(
            (PWWAN_UICC_FILE_PATH)(WwanBuffer + CurrentOffsetInWwanBuffer),
            &(MbbUiccFilePath[AppIndex]),
            &(MbbUiccFilePathLength[AppIndex]));

        if (NdisStatus != NDIS_STATUS_SUCCESS)
        {
            goto Cleanup;
        }

        CurrentOffsetInWwanBuffer += sizeof(WWAN_UICC_FILE_PATH);
        MbbBufferSize += MbbUiccFilePathLength[AppIndex];
    }

    // Allocate the buffer for MbbDeviceSlotMappingInfo.
    MbbDeviceBindingsInfo = ALLOCATE_NONPAGED_POOL(MbbBufferSize);
    if (NULL == MbbDeviceBindingsInfo)
    {
        NdisStatus = NDIS_STATUS_RESOURCES;
        goto Cleanup;
    }

    MbbBuffer = (PUCHAR)MbbDeviceBindingsInfo;

    // Set the values of MbbDeviceBindingsInfo
    MbbDeviceBindingsInfo->ApplicationCount = AppCount;
    for (AppIndex = 0; AppIndex < AppCount; AppIndex++)
    {
        CurrentOffsetInMbbBuffer = MbbUtilWriteStringToBuffer(
            MbbBuffer,
            MbbBufferSize,
            CurrentOffsetInMbbBuffer,
            (PMBB_STRING)(&(MbbDeviceBindingsInfo->ApplicationList[AppIndex])),
            (PUCHAR)(MbbUiccFilePath[AppIndex]),
            MbbUiccFilePathLength[AppIndex]);
    }

    *MbbSetDeviceBindingsInfoOut = MbbDeviceBindingsInfo;
    *ReturnedBufferSize = MbbBufferSize;

Cleanup:
    for (AppIndex = 0; AppIndex < AppCount; AppIndex++)
    {
        if (NULL != MbbUiccFilePath[AppIndex])
        {
            FREE_POOL(MbbUiccFilePath[AppIndex]);
        }
    }

    return NdisStatus;
}

NDIS_STATUS
MbbUtilWwanToMbbSetRegisterStateEx(
    __in        PWWAN_SET_REGISTER_STATE_EX   WwanSetRegisterStateEx,
    __deref_out PMBB_SET_REGISTER_STATE_V2    *MbbSetRegisterStateV2Out,
    __out       PULONG                        ReturnedBufferSize
    )
{
    PUCHAR                          WwanBuffer = NULL;
    ULONG                           CurrentOffsetInWwanBuffer = 0;
    PUCHAR                          MbbBuffer = NULL;
    ULONG                           CurrentOffsetInMbbBuffer = 0;
    ULONG                           MbbBufferSize = 0;
    PMBB_SET_REGISTER_STATE_V2      MbbSetRegisterStateV2 = NULL;
    SIZE_T                          ProviderIdSize = 0;
    ULONG                           DataClassCount = 0;

    WwanBuffer = (PUCHAR)WwanSetRegisterStateEx;
    CurrentOffsetInWwanBuffer = sizeof(WWAN_SET_REGISTER_STATE_EX);
    DataClassCount = WwanSetRegisterStateEx->AcquisitionOrderListHeader.ElementCount;
    *ReturnedBufferSize = 0;

    if (WwanStructRegisterAcquisitionOrder != WwanSetRegisterStateEx->AcquisitionOrderListHeader.ElementType)
    {
        return NDIS_STATUS_INVALID_PARAMETER;
    }

    // Calculate the MbbBufferSize.
    MbbBufferSize = (ULONG)ROUND_UP_COUNT(FIELD_OFFSET(MBB_SET_REGISTER_STATE_V2, AcquisitionOrder), ALIGN_DWORD);
    MbbBufferSize += (ULONG)ROUND_UP_COUNT(sizeof(MBB_ARRAY_ELEMENT), ALIGN_DWORD);
    CurrentOffsetInMbbBuffer = MbbBufferSize;

    if (RtlStringCbLengthW(WwanSetRegisterStateEx->ProviderId, sizeof(WwanSetRegisterStateEx->ProviderId), &ProviderIdSize) 
        != STATUS_SUCCESS)
    {
        ProviderIdSize = sizeof(WwanSetRegisterStateEx->ProviderId);
    }
    MbbBufferSize += ROUND_UP_COUNT((ULONG)ProviderIdSize, ALIGN_DWORD);

    MbbBufferSize += ROUND_UP_COUNT(sizeof(ULONG) * DataClassCount, ALIGN_DWORD);

    // Allocate the buffer for MbbSetRegisterStateV2.
    MbbSetRegisterStateV2 = ALLOCATE_NONPAGED_POOL(MbbBufferSize);
    if (NULL == MbbSetRegisterStateV2)
    {
        return NDIS_STATUS_RESOURCES;
    }

    MbbBuffer = (PUCHAR)MbbSetRegisterStateV2;

    // Set the values of MbbSetRegisterStateV2
    CurrentOffsetInMbbBuffer = MbbUtilWriteStringToBuffer(
        MbbBuffer,
        MbbBufferSize,
        CurrentOffsetInMbbBuffer,
        &(MbbSetRegisterStateV2->ProviderId),
        (PUCHAR)(WwanSetRegisterStateEx->ProviderId),
        (ULONG)ProviderIdSize);

    MbbSetRegisterStateV2->RegisterAction = (MBB_REGISTER_ACTION)WwanSetRegisterStateEx->RegisterAction;
    MbbSetRegisterStateV2->DataClass = WwanSetRegisterStateEx->WwanDataClass;
    MbbSetRegisterStateV2->VoiceClass = (MBB_REGISTRATION_VOICE_CLASS)WwanSetRegisterStateEx->WwanVoiceClass;
    MbbSetRegisterStateV2->VoiceDomain = (MBB_REGISTRATION_VOICE_DOMAIN_PREFERENCE)WwanSetRegisterStateEx->WwanVoiceDomain;
    MbbSetRegisterStateV2->CdmaRoamMode = (MBB_REGISTRATION_CDMA_ROAM_MODE)WwanSetRegisterStateEx->WwanCdmaRoamMode;

    CurrentOffsetInMbbBuffer = MbbUtilWriteStringToBuffer(
        MbbBuffer,
        MbbBufferSize,
        CurrentOffsetInMbbBuffer,
        (PMBB_STRING)(&(MbbSetRegisterStateV2->AcquisitionOrder)),
        (PUCHAR)(WwanBuffer + CurrentOffsetInWwanBuffer),
        sizeof(ULONG) * DataClassCount);

    *MbbSetRegisterStateV2Out = MbbSetRegisterStateV2;
    *ReturnedBufferSize = MbbBufferSize;

    return NDIS_STATUS_SUCCESS;
}

NDIS_STATUS
MbbUtilWwanToMbbSetUiccOpenChannel(
    __in        PWWAN_SET_UICC_OPEN_CHANNEL WwanSetUiccOpenChannel,
    __deref_out PMBB_SET_UICC_OPEN_CHANNEL  *MbbSetUiccOpenChannelOut,
    __out       PULONG                      ReturnedBufferSize
    )
{
    PUCHAR                      Buffer;
    ULONG                       CurrentOffset;
    ULONG                       BufferSize;
    PMBB_SET_UICC_OPEN_CHANNEL  MbbSetUiccOpenChannel = NULL;
    MBB_ARRAY_ELEMENT           AppId = { 0 };

    *MbbSetUiccOpenChannelOut = NULL;
    *ReturnedBufferSize = 0;

    BufferSize = (ULONG)ROUND_UP_COUNT(FIELD_OFFSET(MBB_SET_UICC_OPEN_CHANNEL, DataBuffer), ALIGN_DWORD);

    CurrentOffset = BufferSize;

    BufferSize += (ULONG)ROUND_UP_COUNT(WwanSetUiccOpenChannel->AppIdLength, ALIGN_DWORD);

    if ((MbbSetUiccOpenChannel = ALLOCATE_NONPAGED_POOL(BufferSize)) == NULL)
    {
        return NDIS_STATUS_RESOURCES;
    }

    Buffer = (PUCHAR)MbbSetUiccOpenChannel;

    CurrentOffset = MbbUtilWriteStringToBuffer(
        Buffer,
        BufferSize,
        CurrentOffset,
        (PMBB_STRING)&AppId,
        (PUCHAR)WwanSetUiccOpenChannel->AppId,
        WwanSetUiccOpenChannel->AppIdLength
        );

    // In the MBIM Extension for Low-Level UICC Access, (OFFSET, SIZE) type pairs are spec'ed in
    // reverse order of the MBIM v1.0. Convert the pair that is filled by the utility function.
    MbbSetUiccOpenChannel->AppId.Size   = AppId.Size;
    MbbSetUiccOpenChannel->AppId.Offset = AppId.Offset;

    MbbSetUiccOpenChannel->SelectP2Arg  = WwanSetUiccOpenChannel->SelectP2Arg;
    MbbSetUiccOpenChannel->ChannelGroup = WwanSetUiccOpenChannel->ChannelGroup;

    *MbbSetUiccOpenChannelOut = MbbSetUiccOpenChannel;
    *ReturnedBufferSize = BufferSize;

    return NDIS_STATUS_SUCCESS;
}

VOID
MbbUtilWwanToMbbSetUiccCloseChannel(
    __in  PWWAN_SET_UICC_CLOSE_CHANNEL  WwanSetUiccCloseChannel,
    __out PMBB_SET_UICC_CLOSE_CHANNEL   MbbSetUiccCloseChannel
    )
{
    MbbSetUiccCloseChannel->Channel         = WwanSetUiccCloseChannel->Channel;
    MbbSetUiccCloseChannel->ChannelGroup    = WwanSetUiccCloseChannel->ChannelGroup;
    MbbSetUiccCloseChannel->SelectP2Arg     = WwanSetUiccCloseChannel->SelectP2Arg;
}

NDIS_STATUS
MbbUtilWwanToMbbSetUiccApdu(
    __in        PWWAN_SET_UICC_APDU WwanSetUiccApdu,
    __deref_out PMBB_SET_UICC_APDU  *MbbSetUiccApduOut,
    __out       PULONG              ReturnedBufferSize
    )
{
    PUCHAR              Buffer;
    ULONG               CurrentOffset;
    ULONG               BufferSize;
    PMBB_SET_UICC_APDU  MbbSetUiccApdu = NULL;
    MBB_ARRAY_ELEMENT   Command = { 0 };

    *MbbSetUiccApduOut = NULL;
    *ReturnedBufferSize = 0;

    BufferSize = (ULONG)ROUND_UP_COUNT(FIELD_OFFSET(MBB_SET_UICC_APDU, DataBuffer), ALIGN_DWORD);

    CurrentOffset = BufferSize;

    BufferSize += (ULONG)ROUND_UP_COUNT(WwanSetUiccApdu->CommandLength, ALIGN_DWORD);

    if ((MbbSetUiccApdu = ALLOCATE_NONPAGED_POOL(BufferSize)) == NULL)
    {
        return NDIS_STATUS_RESOURCES;
    }

    Buffer = (PUCHAR)MbbSetUiccApdu;

    CurrentOffset = MbbUtilWriteStringToBuffer(
        Buffer,
        BufferSize,
        CurrentOffset,
        (PMBB_STRING)&Command,
        (PUCHAR)WwanSetUiccApdu->Command,
        WwanSetUiccApdu->CommandLength
        );

    // In the MBIM Extension for Low-Level UICC Access, (OFFSET, SIZE) type pairs are spec'ed in
    // reverse order of the MBIM v1.0. Convert the pair that is filled by the utility function.
    MbbSetUiccApdu->Command.Size    = Command.Size;
    MbbSetUiccApdu->Command.Offset  = Command.Offset;

    MbbSetUiccApdu->Channel         = WwanSetUiccApdu->Channel;
    MbbSetUiccApdu->SecureMessaging = (MBB_UICC_SECURE_MESSAGING)WwanSetUiccApdu->SecureMessaging;
    MbbSetUiccApdu->Type            = (MBB_UICC_CLASS_BYTE_TYPE)WwanSetUiccApdu->Type;

    *MbbSetUiccApduOut = MbbSetUiccApdu;
    *ReturnedBufferSize = BufferSize;

    return NDIS_STATUS_SUCCESS;
}

NDIS_STATUS
MbbUtilWwanToMbbSetUiccTerminalCapability(
    __in        PWWAN_LIST_HEADER                   WwanCapabilityListHeader,
    __deref_out PMBB_SET_UICC_TERMINAL_CAPABILITY   *MbbSetUiccTerminalCapabilityOut,
    __out       PULONG                              ReturnedBufferSize
    )
{
    PUCHAR                              Buffer;
    ULONG                               CurrentOffset;
    ULONG                               BufferSize;
    PMBB_SET_UICC_TERMINAL_CAPABILITY   MbbSetUiccTerminalCapability = NULL;
    ULONG                               CapabilityCount = 0;

    *MbbSetUiccTerminalCapabilityOut = NULL;
    *ReturnedBufferSize = 0;

    CapabilityCount = WwanCapabilityListHeader->ElementCount;

    BufferSize = (ULONG)ROUND_UP_COUNT(FIELD_OFFSET(MBB_SET_UICC_TERMINAL_CAPABILITY, CapabilityList), ALIGN_DWORD);
    BufferSize += (ULONG)ROUND_UP_COUNT(sizeof(MBB_ARRAY_ELEMENT) * CapabilityCount, ALIGN_DWORD);

    CurrentOffset = BufferSize;

    BufferSize += (ULONG)ROUND_UP_COUNT(sizeof(MBB_UICC_TERMINAL_CAPABILITY_TLV) * CapabilityCount, ALIGN_DWORD);

    if ((MbbSetUiccTerminalCapability = ALLOCATE_NONPAGED_POOL(BufferSize)) == NULL)
    {
        return NDIS_STATUS_RESOURCES;
    }

    Buffer = (PUCHAR)MbbSetUiccTerminalCapability;

    MbbSetUiccTerminalCapability->ElementCount = CapabilityCount;
    CurrentOffset = MbbUtilWwanListToMbbList(WwanCapabilityListHeader,
        sizeof(MBB_UICC_TERMINAL_CAPABILITY_TLV),
        Buffer,
        CurrentOffset,
        &(MbbSetUiccTerminalCapability->CapabilityList[0]));

    *MbbSetUiccTerminalCapabilityOut = MbbSetUiccTerminalCapability;
    *ReturnedBufferSize = BufferSize;

    return NDIS_STATUS_SUCCESS;
}

NDIS_STATUS
MbbUtilWwanToMbbSetSarConfig(
    _In_      PWWAN_SET_SAR_CONFIG   WwanSetSarConfig,
    _Outptr_  PMBB_SET_SAR_CONFIG    *MbbSetSarConfigOut,
    _Out_     PULONG                 ReturnedBufferSize
)
{
    ULONG                    CurrentOffsetInWwanBuffer = 0;
    ULONG                    CurrentOffsetInMbbBuffer = 0;
    ULONG                    MbbBufferSize = 0;
    PMBB_SET_SAR_CONFIG      MbbSetSarConfig = NULL;
    ULONG                    SarIndexCount = 0;

    CurrentOffsetInWwanBuffer = sizeof(WWAN_SET_SAR_CONFIG);
    SarIndexCount = WwanSetSarConfig->SarConfigIndexListHeader.ElementCount;
    *ReturnedBufferSize = 0;

    if (WwanStructSarConfig != WwanSetSarConfig->SarConfigIndexListHeader.ElementType)
    {
        return NDIS_STATUS_INVALID_PARAMETER;
    }

    // Calculate the MbbBufferSize.
    MbbBufferSize = (ULONG)ROUND_UP_COUNT(FIELD_OFFSET(MBB_SET_SAR_CONFIG, Configurations), ALIGN_DWORD);
    MbbBufferSize += (ULONG)ROUND_UP_COUNT(sizeof(MBB_ARRAY_ELEMENT) * SarIndexCount, ALIGN_DWORD);
    CurrentOffsetInMbbBuffer = MbbBufferSize;

    MbbBufferSize += ROUND_UP_COUNT(sizeof(MBB_SAR_CONFIG_INDICES) * SarIndexCount, ALIGN_DWORD);

    // Allocate the buffer for MbbSetSarConfig.
    MbbSetSarConfig = ALLOCATE_NONPAGED_POOL(MbbBufferSize);
    if (NULL == MbbSetSarConfig)
    {
        return NDIS_STATUS_RESOURCES;
    }

    // Set the values of MbbSetSarConfig
    MbbSetSarConfig->SarMode = (MBB_SAR_CONTROL_MODE)WwanSetSarConfig->SarMode;
    MbbSetSarConfig->SarBackoffStatus = (MBB_SAR_BACKOFF_STATE)WwanSetSarConfig->SarBackoffStatus;
    MbbSetSarConfig->ElementCount = SarIndexCount;

    for (ULONG i = 0; i < SarIndexCount; i++)
    {
        CurrentOffsetInMbbBuffer = MbbUtilWriteStringToBuffer(
            (PUCHAR)MbbSetSarConfig,
            MbbBufferSize,
            CurrentOffsetInMbbBuffer,
            (PMBB_STRING)(&(MbbSetSarConfig->Configurations[i])),
            (PUCHAR)((PUCHAR)WwanSetSarConfig + CurrentOffsetInWwanBuffer),
            sizeof(WWAN_SAR_CONFIG_INDICES));

        CurrentOffsetInWwanBuffer += sizeof(WWAN_SAR_CONFIG_INDICES);
    }

    *MbbSetSarConfigOut = MbbSetSarConfig;
    *ReturnedBufferSize = MbbBufferSize;

    return NDIS_STATUS_SUCCESS;
}

NDIS_STATUS
MbbUtilWwanToMbbSetNetworkBlacklist(
    _In_        PWWAN_NETWORK_BLACKLIST_INFO        SetNetworkBlacklist,
    _Outptr_    PMBB_MS_NETWORK_BLACKLIST_INFO      *MbbNetworkBlacklistOut,
    _Out_       PULONG                              ReturnedBufferSize
)
{
    ULONG                    CurrentOffsetInWwanBuffer = 0;
    ULONG                    CurrentOffsetInMbbBuffer = 0;
    ULONG                    MbbBufferSize = 0;
    PMBB_MS_NETWORK_BLACKLIST_INFO      MbbNetworkBlacklist = NULL;
    ULONG                    BlacklistedProviderCount = SetNetworkBlacklist->BlacklistProviderList.ElementCount;

    CurrentOffsetInWwanBuffer = sizeof(WWAN_NETWORK_BLACKLIST_INFO);

    *ReturnedBufferSize = 0;

    // Calculate the MbbBufferSize.
    MbbBufferSize = (ULONG)ROUND_UP_COUNT(FIELD_OFFSET(MBB_MS_NETWORK_BLACKLIST_INFO, Contexts), ALIGN_DWORD);
    MbbBufferSize += (ULONG)ROUND_UP_COUNT(sizeof(MBB_ARRAY_ELEMENT) * BlacklistedProviderCount, ALIGN_DWORD);
    CurrentOffsetInMbbBuffer = MbbBufferSize;

    MbbBufferSize += ROUND_UP_COUNT(sizeof(MBB_MS_NETWORK_BLACKLIST_PROVIDER) * BlacklistedProviderCount, ALIGN_DWORD);

    // Allocate the buffer for MbbNetworkBlacklist.
    MbbNetworkBlacklist = ALLOCATE_NONPAGED_POOL(MbbBufferSize);
    if (NULL == MbbNetworkBlacklist)
    {
        return NDIS_STATUS_RESOURCES;
    }

    // Set the values of MbbNetworkBlacklist
    MbbNetworkBlacklist->BlacklistState = (MBB_BASICCONNECTEXT_NETWORK_BLACKLIST_STATE)SetNetworkBlacklist->BlacklistState;
    MbbNetworkBlacklist->ElementCount = BlacklistedProviderCount;

    for (ULONG i = 0; i < BlacklistedProviderCount; i++)
    {
        CurrentOffsetInMbbBuffer = MbbUtilWriteStringToBuffer(
            (PUCHAR)MbbNetworkBlacklist,
            MbbBufferSize,
            CurrentOffsetInMbbBuffer,
            (PMBB_STRING)(&(MbbNetworkBlacklist->Contexts[i])),
            (PUCHAR)((PUCHAR)MbbNetworkBlacklist + CurrentOffsetInWwanBuffer),
            sizeof(MBB_MS_NETWORK_BLACKLIST_PROVIDER));

        CurrentOffsetInWwanBuffer += sizeof(MBB_MS_NETWORK_BLACKLIST_PROVIDER);
    }

    *MbbNetworkBlacklistOut = MbbNetworkBlacklist;
    *ReturnedBufferSize = MbbBufferSize;

    return NDIS_STATUS_SUCCESS;
}


//
// Mbb to Wwan
//

WWAN_STATUS
MbbUtilMbbToWwanStatus(
    __in MBB_STATUS MbbStatus
    )
{
    if( MBB_STATUS_IS_BASIC( MbbStatus ) )
    {
        return MbbStatusMapTableBasic[MBB_STATUS_BASIC_INDEX(MbbStatus)];
    }
    else if( MBB_STATUS_IS_SMS( MbbStatus ) )
    {
        return MbbStatusMapTableSms[MBB_STATUS_SMS_INDEX(MbbStatus)];
    }
    else if( MBB_STATUS_IS_UICC( MbbStatus ) )
    {
        return MbbStatusMapTableUicc[MBB_STATUS_UICC_INDEX(MbbStatus)];
    }
    else
    {
        TraceError( WMBCLASS_OID, "[Util][MBBStatus] INVALID MBB status code=%d",
                      MbbStatus
                      );

        return WWAN_STATUS_FAILURE;
    }
}

VOID
MbbUtilMbbToWwanRadioState(
    __in  MBB_RADIO_STATE*  MbbRadioState,
    __out WWAN_RADIO*       WwanRadio
    )
{
    *WwanRadio = (MBB_RADIO_STATE)*MbbRadioState ;
}

NTSTATUS
MbbIsVariableFieldValid(
    ULONG   TotalMessageLength,
    ULONG   Offset,
    ULONG   Size,
    ULONG   MaxElements,
    ULONG   ElementSize
    )

{
    ULONGLONG   Temp;

    Temp = (ULONGLONG)Offset + Size;

    //
    //  does the field extend beyond the length of the whole message?
    //
    if (Temp > TotalMessageLength)
    {
        TraceError( WMBCLASS_OID, "Offset plus size exceeded total length, Offset=%d, Size=%d, Total=%d",Offset, Size, TotalMessageLength);
        return STATUS_INVALID_PARAMETER;
    }

    //
    //  Is the offset aligned on a dword boundary?
    //
    if (Offset != ROUND_UP_COUNT(Offset,ALIGN_DWORD))
    {
        TraceError( WMBCLASS_OID, "Offset note dword aligned Offset=%d", Offset);
        return STATUS_INVALID_PARAMETER;
    }

    //
    //  Is the field bigger than the max we support?
    //
    if ((Size/ElementSize) > MaxElements)
    {
        TraceError( WMBCLASS_OID, "Size is larger than max elements Size=%d, Max=%d", Size/ElementSize , MaxElements);
        return STATUS_INVALID_PARAMETER;
    }

    //
    //  is the size an integral number of elements
    //
    if (Size % ElementSize != 0)
    {
        TraceError( WMBCLASS_OID, "Size is not a multiple of ElementSize Size=%d, element size=%d", Size, ElementSize);
        return STATUS_INVALID_PARAMETER;
    }

    return STATUS_SUCCESS;
}


NTSTATUS
MbbIsArrayFieldValid(
    ULONG   TotalMessageLength,
    ULONG   Offset,
    ULONG   ElementCount,
    ULONG   ElementSize
    )

{
    ULONGLONG   Temp;
    ULONGLONG   ArraySize;

    ArraySize= (ULONGLONG) ElementCount * ElementSize;

    if (ArraySize > ULONG_MAX)
    {
        TraceError( WMBCLASS_OID, "Array size is too big, Count=%d, Size=%d", ElementCount, ElementSize);
        return STATUS_INVALID_PARAMETER;
    }

    Temp = (ULONGLONG)Offset + ArraySize;

    //
    //  does the field extend beyond the length of the whole message?
    //
    if (Temp > TotalMessageLength)
    {
        TraceError( WMBCLASS_OID, "Offset plus size exceeded total length, Offset=%d, Size=%I64d, Total=%d",Offset, ArraySize, TotalMessageLength);
        return STATUS_INVALID_PARAMETER;
    }

    //
    //  Is the offset aligned on a dword boundary?
    //
    if (Offset != ROUND_UP_COUNT(Offset,ALIGN_DWORD))
    {
        TraceError( WMBCLASS_OID, "Offset note dword aligned Offset=%d", Offset);
        return STATUS_INVALID_PARAMETER;
    }

    return STATUS_SUCCESS;
}

VOID
MbbUtilMbbToWwanDeviceCaps(
    __in  PMBB_DEVICE_CAPS                  MbbDeviceCaps,
    __in_ecount(ManufacturerStrLen) PWCHAR  ManufacturerString,
    __in  ULONG                             ManufacturerStrLen,
    __in_ecount(ModelStrLen) PWCHAR         ModelString,
    __in  ULONG                             ModelStrLen,
    __out PWWAN_DEVICE_CAPS                 WwanDeviceCaps,
    __in  BOOL                              fMultimode
    )
{
    WWAN_CELLULAR_CLASS* pCC = (WWAN_CELLULAR_CLASS*)(((PUCHAR)WwanDeviceCaps) + sizeof(*WwanDeviceCaps));

    RtlZeroMemory(WwanDeviceCaps, sizeof(*WwanDeviceCaps));
    WwanDeviceCaps->CellularClassListHeader.ElementType = WwanStructCellularClass;
    WwanDeviceCaps->CellularClassListHeader.ElementCount = 0;

    MbbDeviceCaps->CellularClass &= MbbCellularClassMaximum;

    WwanDeviceCaps->WwanDeviceType        = (WWAN_DEVICE_TYPE)MbbDeviceCaps->DeviceType;

    if (MbbDeviceCaps->CellularClass & MbbCellularClassGsm)
    {

        WwanDeviceCaps->WwanCellularClass     = WwanCellularClassGsm;
    }
    else if (MbbDeviceCaps->CellularClass & MbbCellularClassCdma)
    {
        WwanDeviceCaps->WwanCellularClass     = WwanCellularClassCdma;
    }


    WwanDeviceCaps->WwanVoiceClass        = (WWAN_VOICE_CLASS)MbbDeviceCaps->VoiceClass;
    WwanDeviceCaps->WwanSimClass          = (WWAN_SIM_CLASS)MbbDeviceCaps->SimClass;
    WwanDeviceCaps->WwanDataClass         = (ULONG)MbbDeviceCaps->DataClass;

    WwanDeviceCaps->WwanGsmBandClass      = (ULONG)WWAN_BAND_CLASS_UNKNOWN;
    WwanDeviceCaps->WwanCdmaBandClass     = (ULONG)WWAN_BAND_CLASS_UNKNOWN;

    WwanDeviceCaps->WwanSmsCaps           = (ULONG)MbbDeviceCaps->SmsCaps;
    WwanDeviceCaps->WwanControlCaps       = (ULONG)MbbDeviceCaps->ControlCaps;


    if(fMultimode)
    {
        WwanDeviceCaps->WwanControlCaps |= WWAN_CTRL_CAPS_MULTI_MODE;
        WwanDeviceCaps->CellularClassListHeader.ElementCount = MbbCellularClassMaximum - 1 ;

        //for now we just have GSM and CDMA so hardcode, this is protected
        //by a C_ASSERT
        *pCC = WwanCellularClassGsm;
        *(pCC+ 1) = WwanCellularClassCdma;
    }
    else
    {
        WwanDeviceCaps->WwanCellularClass = MbbDeviceCaps->CellularClass;
    }

    //
    //  this bit is not in the mbim model
    //
    WwanDeviceCaps->WwanControlCaps &= ~WWAN_CTRL_CAPS_PROTECT_UNIQUEID;

    //
    //  translate this one to the right wwan bit
    //
    if (MbbDeviceCaps->ControlCaps & MbbControlCapsMultiCarrier)
    {
        WwanDeviceCaps->WwanControlCaps |= WWAN_CTRL_CAPS_MODEL_MULTI_CARRIER;
    }
   
    WwanDeviceCaps->MaxActivatedContexts  = MbbDeviceCaps->dwMaxSessions;

    MbbUtilPopulateStaticString(WwanDeviceCaps->CustomDataClass, MbbDeviceCaps, CustomDataClass);

    MbbUtilPopulateStaticString(WwanDeviceCaps->DeviceId, MbbDeviceCaps, DeviceIdString);

    MbbUtilPopulateStaticString(WwanDeviceCaps->FirmwareInfo, MbbDeviceCaps, FirmwareInfo);


    RtlStringCchCopyNW(
        WwanDeviceCaps->Manufacturer,
        WWAN_MANUFACTURER_LEN,
        ManufacturerString,
        ManufacturerStrLen
        );

    RtlStringCchCopyNW(
        WwanDeviceCaps->Model,
        WWAN_MODEL_LEN,
        ModelString,
        ModelStrLen
        );

    return;
}

VOID
MbbUtilMbbToWwanDeviceCapsV2(
    __in  PMBB_MS_DEVICE_CAPS_INFO_V2       MbbDeviceCaps,
    __in_ecount(ManufacturerStrLen) PWCHAR  ManufacturerString,
    __in  ULONG                             ManufacturerStrLen,
    __in_ecount(ModelStrLen) PWCHAR         ModelString,
    __in  ULONG                             ModelStrLen,
    __out PWWAN_DEVICE_CAPS_EX              WwanDeviceCaps,
    __in  BOOL                              fMultimode
)
{
    WWAN_CELLULAR_CLASS* pCC = (WWAN_CELLULAR_CLASS*)(((PUCHAR)WwanDeviceCaps) + sizeof(*WwanDeviceCaps));

    RtlZeroMemory(WwanDeviceCaps, sizeof(*WwanDeviceCaps));
    WwanDeviceCaps->CellularClassListHeader.ElementType = WwanStructCellularClass;
    WwanDeviceCaps->CellularClassListHeader.ElementCount = 0;

    MbbDeviceCaps->CellularClass &= MbbCellularClassMaximum;

    WwanDeviceCaps->WwanDeviceType = (WWAN_DEVICE_TYPE)MbbDeviceCaps->DeviceType;

    if (MbbDeviceCaps->CellularClass & MbbCellularClassGsm)
    {
        WwanDeviceCaps->WwanCellularClass = WwanCellularClassGsm;
    }
    else if (MbbDeviceCaps->CellularClass & MbbCellularClassCdma)
    {
        WwanDeviceCaps->WwanCellularClass = WwanCellularClassCdma;
    }

    WwanDeviceCaps->WwanVoiceClass = (WWAN_VOICE_CLASS)MbbDeviceCaps->VoiceClass;
    WwanDeviceCaps->WwanSimClass = (WWAN_SIM_CLASS)MbbDeviceCaps->SimClass;
    WwanDeviceCaps->WwanDataClass = (ULONG)MbbDeviceCaps->DataClass;

    WwanDeviceCaps->WwanGsmBandClass = (ULONG)WWAN_BAND_CLASS_UNKNOWN;
    WwanDeviceCaps->WwanCdmaBandClass = (ULONG)WWAN_BAND_CLASS_UNKNOWN;

    WwanDeviceCaps->WwanSmsCaps = (ULONG)MbbDeviceCaps->SmsCaps;

    WwanDeviceCaps->WwanControlCaps = (ULONG)MbbDeviceCaps->ControlCaps;

    if (fMultimode)
    {
        WwanDeviceCaps->WwanControlCaps |= WWAN_CTRL_CAPS_MULTI_MODE;
        WwanDeviceCaps->CellularClassListHeader.ElementCount = MbbCellularClassMaximum - 1;

        //for now we just have GSM and CDMA so hardcode, this is protected
        //by a C_ASSERT
        *pCC = WwanCellularClassGsm;
        *(pCC + 1) = WwanCellularClassCdma;
    }

    //
    //  this bit is not in the mbim model
    //
    WwanDeviceCaps->WwanControlCaps &= ~WWAN_CTRL_CAPS_PROTECT_UNIQUEID;

    //
    //  translate this one to the right wwan bit
    //
    if (MbbDeviceCaps->ControlCaps & MbbControlCapsMultiCarrier)
    {
        WwanDeviceCaps->WwanControlCaps |= WWAN_CTRL_CAPS_MODEL_MULTI_CARRIER;
    }

    WwanDeviceCaps->MaxActivatedContexts = MbbDeviceCaps->dwMaxSessions;

    MbbUtilPopulateStaticString(WwanDeviceCaps->CustomDataClass, MbbDeviceCaps, CustomDataClass);

    MbbUtilPopulateStaticString(WwanDeviceCaps->DeviceId, MbbDeviceCaps, DeviceIdString);

    MbbUtilPopulateStaticString(WwanDeviceCaps->FirmwareInfo, MbbDeviceCaps, FirmwareInfo);

    RtlStringCchCopyNW(
        WwanDeviceCaps->Manufacturer,
        WWAN_MANUFACTURER_LEN,
        ManufacturerString,
        ManufacturerStrLen
        );

    RtlStringCchCopyNW(
        WwanDeviceCaps->Model,
        WWAN_MODEL_LEN,
        ModelString,
        ModelStrLen
        );

    // 
    // new field added in WWAN_DEVICE_CAPS_EX
    //
    WwanDeviceCaps->ExecutorIndex = MbbDeviceCaps->ExecutorIndex;

    return;
}

VOID
MbbUtilMbbToWwanReadyInfo(
    __in PMBB_SUBSCRIBER_READY_INFO MbbReadyInfo,
    __in PWWAN_READY_INFO           WwanReadyInfo
    )
/*++
    Note:
        Not all the information required to fill the WWAN_READY_INFO
        structure is present in MBB_SUBSCRIBER_READY_INFO. Some of the
        information will come from MBB_EMERGENCY_MODE and MBB_SMS_CONFIGURATION.
--*/
{
    ULONG       ElementCount = MbbReadyInfo->TelephoneNumberCount;
    ULONG       ElementIndex;
    ULONG       StringCb;
    ULONG       StringCch;
    PWCHAR      WwanTn;
    PWCHAR      MbbTn;
    NTSTATUS    NtStatus;

    WwanReadyInfo->ReadyState = (WWAN_READY_STATE)MbbReadyInfo->ReadyState;

    StringCb = MIN( MbbReadyInfo->SubscriberId.Size, (WWAN_SUBSCRIBERID_LEN-1) * sizeof(WCHAR) );
    RtlCopyMemory(
        WwanReadyInfo->SubscriberId,
        (((PCHAR)MbbReadyInfo) + MbbReadyInfo->SubscriberId.Offset),
        StringCb
        );
    StringCch = StringCb / sizeof(WCHAR);
    WwanReadyInfo->SubscriberId[StringCch] = 0;

    StringCb = MIN( MbbReadyInfo->SimIccId.Size, (WWAN_SIMICCID_LEN-1) * sizeof(WCHAR) );
    RtlCopyMemory(
        WwanReadyInfo->SimIccId,
        (((PCHAR)MbbReadyInfo) + MbbReadyInfo->SimIccId.Offset),
        StringCb
        );
    StringCch = StringCb / sizeof(WCHAR);
    WwanReadyInfo->SimIccId[StringCch] = 0;

    WwanReadyInfo->TNListHeader.ElementType  = WwanStructTN;
    WwanReadyInfo->TNListHeader.ElementCount = ElementCount;

    WwanTn = (PWCHAR)(WwanReadyInfo + 1);

    for( ElementIndex = 0;
         ElementIndex < ElementCount;
         ElementIndex ++ )
    {
        MbbTn = (PWCHAR)((PCHAR)MbbReadyInfo + MbbReadyInfo->TelephoneNumbers[ElementIndex].Offset);
        StringCb = MIN( MbbReadyInfo->TelephoneNumbers[ElementIndex].Size, (WWAN_TN_LEN-1) * sizeof(WCHAR) );
        RtlCopyMemory(
            WwanTn,
            MbbTn,
            StringCb
            );
        StringCch = StringCb / sizeof(WCHAR);
        WwanTn[StringCch] = 0;
        WwanTn += WWAN_TN_LEN;
    }
}

VOID
MbbUtilMbbToWwanPinInfo(
    __in PMBB_PIN_INFO   MbbPinInfo,
    __out PWWAN_PIN_INFO WwanPinInfo
    )
{
    WwanPinInfo->PinType             = (WWAN_PIN_TYPE)MbbPinInfo->PinType;
    WwanPinInfo->PinState            = (WWAN_PIN_STATE)MbbPinInfo->PinState;
    WwanPinInfo->AttemptsRemaining   = (ULONG)MbbPinInfo->AttemptsRemaining;
}

VOID
MbbUtilMbbToWwanPinList(
    __in  PMBB_PIN_LIST     MbbPinList,
    __out PWWAN_PIN_LIST    WwanPinList
    )
{
    ULONG                   PinIndex;
    PWWAN_PIN_DESC          WwanPinDesc;
    PMBB_PIN_DESCRIPTION    MbbPinDesc;

    WwanPinDesc = &WwanPinList->WwanPinDescPin1;
    MbbPinDesc  = &MbbPinList->PinDescPin1;
    //
    // A loop works because both the list are in the same order.
    //
    for( PinIndex = 0;
         PinIndex < 10;
         PinIndex ++ )
    {
        WwanPinDesc[PinIndex].PinMode       = (WWAN_PIN_MODE)MbbPinDesc[PinIndex].PinMode;
        WwanPinDesc[PinIndex].PinFormat     = (WWAN_PIN_FORMAT)MbbPinDesc[PinIndex].PinFormat;
        WwanPinDesc[PinIndex].PinLengthMin  = (ULONG)MbbPinDesc[PinIndex].PinLengthMin;
        WwanPinDesc[PinIndex].PinLengthMax  = (ULONG)MbbPinDesc[PinIndex].PinLengthMax;
    }
}

NDIS_STATUS
MbbUtilValidateMbbProvider(
    __in ULONG                                  MbbProviderSize,
    __in_bcount(MbbProviderSize) PMBB_PROVIDER  MbbProvider
    )
{
    NDIS_STATUS NdisStatus = NDIS_STATUS_INVALID_DATA;

    do
    {
        if( MbbProvider == NULL ||
            MbbProviderSize < sizeof(MBB_PROVIDER) )
        {
            TraceError( WMBCLASS_OID, "[Util] INSUFFICIENT BufferSize for MBB_PROVIDER, BufferSize[Received=%d Expected=%d]",
                        MbbProviderSize,
                        sizeof(MBB_PROVIDER)
                        );
            break;
        }
        if( MbbIsVariableFieldValid(
                MbbProviderSize,
                MbbProvider->ProviderId.Offset,
                MbbProvider->ProviderId.Size,
                MBB_MAXIMUM_PROVIDER_ID_LENGTH,
                sizeof(WCHAR)
                ) != NDIS_STATUS_SUCCESS )
        {
            TraceError( WMBCLASS_OID, "[Util] INVALID MBB_PROVIDER.ProviderId Offset[Received=%d BufferSize=%d] Size[Received=%d MaxExpected=%d]",
                        MbbProvider->ProviderId.Offset,
                        MbbProviderSize,
                        MbbProvider->ProviderId.Size,
                        MBB_MAXIMUM_PROVIDER_ID_LENGTH * sizeof(WCHAR)
                        );
            break;
        }
        if( MbbIsVariableFieldValid(
                MbbProviderSize,
                MbbProvider->ProviderName.Offset,
                MbbProvider->ProviderName.Size,
                MBB_MAXIMUM_PROVIDER_NAME_LENGTH,
                sizeof(WCHAR)
                ) != NDIS_STATUS_SUCCESS )
        {
            TraceError( WMBCLASS_OID, "[Util] INVALID MBB_PROVIDER.ProviderName Offset[Received=%d BufferSize=%d] Size[Received=%d MaxExpected=%d]",
                        MbbProvider->ProviderName.Offset,
                        MbbProviderSize,
                        MbbProvider->ProviderName.Size,
                        MBB_MAXIMUM_PROVIDER_NAME_LENGTH * sizeof(WCHAR)
                        );
            break;
        }
        if( MbbProvider->CellularClass >= MbbCellularClassMaximum )
        {
            TraceError( WMBCLASS_OID, "[Util] INVALID MBB_PROVIDER.CellularClass=0x%x", MbbProvider->CellularClass );
            break;
        }
        NdisStatus = NDIS_STATUS_SUCCESS;
    }
    while( FALSE );

    return NdisStatus;
}

NDIS_STATUS
MbbUtilValidateMbbProviderList(
    __in ULONG                                          MbbProviderListSize,
    __in_bcount(MbbProviderListSize) PMBB_PROVIDER_LIST MbbProviderList
    )
{
    ULONG       ElementIndex;
    ULONG       ElementCount;
    ULONG       ExpectedSize;
    NDIS_STATUS NdisStatus = NDIS_STATUS_INVALID_DATA;
    NTSTATUS    NtStatus = STATUS_SUCCESS;

    do
    {
        if( MbbProviderList == NULL ||
            MbbProviderListSize < RTL_SIZEOF_THROUGH_FIELD(MBB_PROVIDER_LIST, ProviderCount) )
        {
            TraceError( WMBCLASS_OID, "[Util] INSUFFICIENT BufferSize for MBB_PROVIDER_LIST, BufferSize[Received=%d Expected=%d]",
                        MbbProviderListSize,
                        RTL_SIZEOF_THROUGH_FIELD(MBB_PROVIDER_LIST, ProviderCount)
                        );
            break;
        }

        ElementCount = MbbProviderList->ProviderCount;

        NtStatus = RtlULongMult(
            ElementCount,
            sizeof(MBB_ARRAY_ELEMENT),
            &ExpectedSize
            );
        if ( NT_ERROR(NtStatus) )
        {
            TraceError( WMBCLASS_OID, "[Util] Multiplication overflow occured, ElementCount=%d MBB_ARRAY_ELEMENT size=%d",
                        ElementCount,
                        sizeof(MBB_ARRAY_ELEMENT)
                        );
            break;
        }

        NtStatus = RtlULongAdd(
            ExpectedSize,
            FIELD_OFFSET(MBB_PROVIDER_LIST, Providers),
            &ExpectedSize
            );
        if ( NT_ERROR(NtStatus) )
        {
            TraceError( WMBCLASS_OID, "[Util] Addition overflow occured, ExpectedSize=%d FIELD_OFFSET=%d",
                        ExpectedSize,
                        FIELD_OFFSET(MBB_PROVIDER_LIST, Providers)
                        );
            break;
        }

        if( ExpectedSize > MbbProviderListSize )
        {
            TraceError( WMBCLASS_OID, "[Util] INSUFFICIENT BufferSize for MBB_PROVIDER_LIST, BufferSize[Received=%d Expected=%d] ProviderCount=%d",
                        MbbProviderListSize,
                        ExpectedSize,
                        MbbProviderList->ProviderCount
                        );
            break;
        }

        for( ElementIndex = 0;
             ElementIndex < ElementCount;
             ElementIndex ++ )
        {
            if( MbbIsVariableFieldValid(
                    MbbProviderListSize,
                    MbbProviderList->Providers[ElementIndex].Offset,
                    MbbProviderList->Providers[ElementIndex].Size,
                    ULONG_MAX,  // MaxElements
                    1           // ElementSize
                    ) != NDIS_STATUS_SUCCESS )
            {
                TraceError( WMBCLASS_OID, "[Util] INVALID INVALID MBB_PROVIDER_LIST.Providers[%d] Offset[Received=%d BufferSize=%d] Size[Received=%d MaxExpected=%d]",
                            ElementIndex,
                            MbbProviderList->Providers[ElementIndex].Offset,
                            MbbProviderListSize,
                            MbbProviderList->Providers[ElementIndex].Size,
                            ULONG_MAX
                            );
                break;
            }
            if( MbbUtilValidateMbbProvider(
                    MbbProviderList->Providers[ElementIndex].Size,
                    (PMBB_PROVIDER)(((PCHAR)MbbProviderList) + MbbProviderList->Providers[ElementIndex].Offset)
                    ) != NDIS_STATUS_SUCCESS )
            {
                break;
            }
        }
        if( ElementIndex != ElementCount )
            break;

        NdisStatus = NDIS_STATUS_SUCCESS;
    }
    while( FALSE );

    return NdisStatus;
}


VOID
MbbUtilMbbToWwanProvider(
    __in  PMBB_PROVIDER     MbbProvider,
    __in  ULONG             MbbDataClass,
    __out PWWAN_PROVIDER    WwanProvider
    )
{
    ULONG   StringCb;
    ULONG   StringCch;

    WwanProvider->ProviderState  = (ULONG)MbbProvider->ProviderState;
    WwanProvider->WwanDataClass  = MbbDataClass;

    StringCb = MIN( MbbProvider->ProviderId.Size, (WWAN_PROVIDERID_LEN-1) * sizeof(WCHAR) );
    RtlCopyMemory(
        WwanProvider->ProviderId,
        (((PCHAR)MbbProvider) + MbbProvider->ProviderId.Offset),
        StringCb
        );
    StringCch = StringCb / sizeof(WCHAR);
    WwanProvider->ProviderId[StringCch] = 0;

    StringCb = MIN(  MbbProvider->ProviderName.Size, (WWAN_PROVIDERNAME_LEN-1) * sizeof(WCHAR) );
    RtlCopyMemory(
        WwanProvider->ProviderName,
        (((PCHAR)MbbProvider) + MbbProvider->ProviderName.Offset),
        StringCb
        );
    StringCch = StringCb / sizeof(WCHAR);
    WwanProvider->ProviderName[StringCch] = 0;
}

VOID
MbbUtilMbbToWwanProvider2(
    __in  PMBB_PROVIDER     MbbProvider,
    __in  ULONG             MbbDataClass,
    __out PWWAN_PROVIDER2   WwanProvider2
    )
{
    MbbUtilMbbToWwanProvider(
        MbbProvider,
        MbbDataClass,
       &WwanProvider2->Provider
        );
    WwanProvider2->Rssi             = MbbProvider->Rssi;
    WwanProvider2->ErrorRate        = MbbProvider->ErrorRate;
    WwanProvider2->WwanCellularClass= (WWAN_CELLULAR_CLASS)(MbbProvider->CellularClass);
}

VOID
MbbUtilMbbToWwanProviderList(
    __in  PMBB_PROVIDER_LIST    MbbProviderList,
    __in  ULONG                 MbbDataClass,
    __in  BOOLEAN               IsMultiCarrier,
    __out PWWAN_LIST_HEADER     WwanProviderList
    )
/*++
Description:
    Assumption that the Offsets and Sizes are already verified.
--*/
{
    ULONG           ElementIndex;
    ULONG           ElementCount   = MbbProviderList->ProviderCount;
    PWWAN_PROVIDER2 WwanProviders2 = (PWWAN_PROVIDER2)(WwanProviderList + 1);
    PMBB_PROVIDER   MbbProvider;

    UNREFERENCED_PARAMETER( IsMultiCarrier );

    for( ElementIndex = 0;
         ElementIndex < ElementCount;
         ElementIndex ++ )
    {
        MbbProvider = (PMBB_PROVIDER)(((PCHAR)MbbProviderList) + MbbProviderList->Providers[ElementIndex].Offset);

        MbbUtilMbbToWwanProvider2(
            MbbProvider,
            MbbDataClass,
            &WwanProviders2[ElementIndex]
            );
    }
    WwanProviderList->ElementType  = WwanStructProvider2;
    WwanProviderList->ElementCount = ElementIndex;
}

VOID
MbbUtilMbbToWwanRegistrationState(
    __in  PMBB_REGISTRATION_STATE   MbbRegistrationState,
    __out PWWAN_REGISTRATION_STATE  WwanRegistrationState
    )
{
    WwanRegistrationState->uNwError         = MbbRegistrationState->NetworkError;
    WwanRegistrationState->RegisterState    = (WWAN_REGISTER_STATE)MbbRegistrationState->RegisterState;
    WwanRegistrationState->RegisterMode     = (WWAN_REGISTER_MODE)MbbRegistrationState->RegisterMode;
    WwanRegistrationState->WwanRegFlags     = (ULONG)MbbRegistrationState->RegFlags;
    WwanRegistrationState->CurrentCellularClass     = (WWAN_CELLULAR_CLASS)MbbRegistrationState->CurrentCellularClass;

    MbbUtilPopulateStaticString(WwanRegistrationState->ProviderId, MbbRegistrationState, ProviderId);

    MbbUtilPopulateStaticString(WwanRegistrationState->ProviderName, MbbRegistrationState, ProviderName);

    MbbUtilPopulateStaticString(WwanRegistrationState->RoamingText,  MbbRegistrationState, RoamingText);

}

VOID
MbbUtilMbbToWwanPacketService(
    __in  PMBB_PACKET_SERVICE   MbbPacketService,
    __out PWWAN_PACKET_SERVICE  WwanPacketService
    )
{
    WwanPacketService->uNwError           = MbbPacketService->NetworkError;
    WwanPacketService->PacketServiceState = (WWAN_PACKET_SERVICE_STATE)MbbPacketService->PacketServiceState;
    WwanPacketService->CurrentDataClass   = (ULONG)MbbPacketService->HighestAvailableDataClass;
}

VOID
MbbUtilMbbToWwanSignalState(
    __in  PMBB_SIGNAL_STATE     MbbSignalState,
    __out PWWAN_SIGNAL_STATE    WwanSignalState
    )
/*++
Description
    MBB and WWAN default and disable values are inverted so
    appropriate checks and translation is required.
--*/
{
    WwanSignalState->Rssi       = MbbSignalState->Rssi;
    WwanSignalState->ErrorRate  = MbbSignalState->ErrorRate;

    if( MBB_RSSI_DEFAULT == MbbSignalState->RssiInterval )
        WwanSignalState->RssiInterval = WWAN_RSSI_DEFAULT;
    else if( MBB_RSSI_DISABLE == MbbSignalState->RssiInterval )
        WwanSignalState->RssiInterval = WWAN_RSSI_DISABLE;
    else
        WwanSignalState->RssiInterval = MbbSignalState->RssiInterval;

    if( MBB_RSSI_DEFAULT == MbbSignalState->RssiThreshold )
        WwanSignalState->RssiThreshold = WWAN_RSSI_DEFAULT;
    else if( MBB_RSSI_DISABLE == MbbSignalState->RssiThreshold )
        WwanSignalState->RssiThreshold = WWAN_RSSI_DISABLE;
    else
        WwanSignalState->RssiThreshold = MbbSignalState->RssiThreshold;
}

VOID
MbbUtilMbbToWwanContextState(
    __in  PMBB_CONTEXT_STATE     MbbContextState,
    __in  ULONG                  ConnectId,
    __out PWWAN_CONTEXT_STATE    WwanContextState
    )
{
    WwanContextState->uNwError          = MbbContextState->NetworkError;
    WwanContextState->ConnectionId      = ConnectId;
    WwanContextState->ActivationState   = (WWAN_ACTIVATION_STATE)MbbContextState->ActivationState;
    WwanContextState->VoiceCallState    = (WWAN_VOICE_CALL_STATE)MbbContextState->VoiceCallState;

    if(MbbContextState->IPType >= MbbContextIPTypeDefault &&
        MbbContextState->IPType < MbbContextIPTypeMaximum)
    {
        WwanContextState->IPType = MbbIpTypesMapTable [MbbContextState->IPType];
    }
    else
    {
        TraceWarn(WMBCLASS_UTIL, "Invalid IPType returned from the Modem. Resetting it to IPTypeDefault and setting Context State as Deactivated");
        // Just to be sure, we set the ActivationState as DeActivated if at all we see an IPType out of bounds
        WwanContextState->IPType = WwanIPTypeDefault;
        WwanContextState->ActivationState = WwanActivationStateDeactivated;
    }
}

VOID
MbbUtilConvertMbbIPConfigFlagsToWwanIPConfigFlags(
       ULONG                        MbbIPConfigflags,
       WWAN_IP_CONFIGURATION_FLAGS* WwanConfigFlags
       )
{
    WwanConfigFlags->Value = MbbIPConfigflags;
    WwanConfigFlags->AddressAvailable = MbbIPConfigflags & MbbIpFlagsAddressAvailable ? 1 : 0;
    WwanConfigFlags->GatewayAvailable = MbbIPConfigflags & MbbIpFlagsGatewayAvailable ? 1 : 0;
    WwanConfigFlags->DnsServerAvailable = MbbIPConfigflags & MbbIpFlagsDnsServerAvailable ? 1 : 0;
    WwanConfigFlags->MTUAvailable = MbbIPConfigflags & MbbIpFlagsMTUAvailable ? 1 : 0;
}

NDIS_STATUS
MbbUtilPopulateWwanIPAddressState(
    __in PMBB_IP_ADDRESS_INFO       IpAddressInfo,
    __in PMBB_IPADDRESS_ENTRY       IpTable,
    __in PMBB_IPADDRESS_ENTRY       GatewayTable,
    __in PMBB_IPADDRESS_ENTRY       DnsTable,
    __in ULONG                      IpCount,
    __in ULONG                      GatewayCount,
    __in ULONG                      DnsCount,
    __out PWWAN_IP_ADDRESS_STATE    pWwanIPAddressState
    )
{
    NDIS_STATUS Status = NDIS_STATUS_SUCCESS;

    do
    {
        if(!IpAddressInfo)
        {
            Status = NDIS_STATUS_FAILURE;
            TraceError( WMBCLASS_UTIL, "[MbbUtilPopulateWwanIPAddressState] Received NULL Ipaddress information to copy");
            break;            
        }
        
        MbbUtilConvertMbbIPConfigFlagsToWwanIPConfigFlags(IpAddressInfo->IPv4Flags,&(pWwanIPAddressState->IPv4Flags));
        MbbUtilConvertMbbIPConfigFlagsToWwanIPConfigFlags(IpAddressInfo->IPv6Flags,&(pWwanIPAddressState->IPv6Flags));
        pWwanIPAddressState->IPv4MTU = IpAddressInfo->IPv4MTU;
        pWwanIPAddressState->IPv6MTU = IpAddressInfo->IPv6MTU;
        
        if(IpTable)
        {
            pWwanIPAddressState->IpTable= ALLOCATE_NONPAGED_POOL(sizeof(MBB_IPADDRESS_ENTRY) * IpCount);
            
            if(!pWwanIPAddressState->IpTable)
            {
                Status = NDIS_STATUS_RESOURCES;
                TraceError( WMBCLASS_UTIL, "[MbbUtilPopulateWwanIPAddressState] FAILED to allocate memory for IpTable");
            }
            else
            {
               NdisMoveMemory(pWwanIPAddressState->IpTable, IpTable, sizeof(MBB_IPADDRESS_ENTRY) * IpCount);
               pWwanIPAddressState->IpCount = IpCount;
            }
        }

        if(GatewayTable)
        {
            pWwanIPAddressState->GatewayTable = ALLOCATE_NONPAGED_POOL(sizeof(MBB_IPADDRESS_ENTRY) * GatewayCount);

            if(!pWwanIPAddressState->GatewayTable)
            {
                Status = NDIS_STATUS_RESOURCES;
                TraceError( WMBCLASS_UTIL, "[MbbUtilPopulateWwanIPAddressState] FAILED to allocate memory for GatewayTable");
            }
            else
            {
               NdisMoveMemory(pWwanIPAddressState->GatewayTable, GatewayTable, sizeof(MBB_IPADDRESS_ENTRY) * GatewayCount);
               pWwanIPAddressState->GatewayCount = GatewayCount;
            }
        }

        if(DnsTable)
        {
            pWwanIPAddressState->DnsTable = ALLOCATE_NONPAGED_POOL(sizeof(MBB_IPADDRESS_ENTRY) * DnsCount);

            if(!pWwanIPAddressState->DnsTable)
            {
                Status = NDIS_STATUS_RESOURCES;
                TraceError( WMBCLASS_UTIL, "[MbbUtilPopulateWwanIPAddressState] FAILED to allocate memory for DnsTable");
            }
            else
            {
               NdisMoveMemory(pWwanIPAddressState->DnsTable, DnsTable, sizeof(MBB_IPADDRESS_ENTRY) * DnsCount);
               pWwanIPAddressState->DnsCount = DnsCount;
            }
        }
    }while(FALSE);
    
    return Status;
}    

WWAN_CONTEXT_TYPE
MbbUtilMbbToWwanContextType(
    __in PGUID   MbbContextTypeNetworkByteOrder
    )
{
    GUID                MbbContextType;
    WWAN_CONTEXT_TYPE   WwanContextType;

    MBB_UUID_TO_HOST( &MbbContextType, MbbContextTypeNetworkByteOrder );

    if( RtlCompareMemory( &MbbContextType, &MBB_UUID_CONTEXT_TYPE_NONE , sizeof(GUID) ) == sizeof(GUID) )
        WwanContextType = WwanContextTypeNone;
    else if( RtlCompareMemory( &MbbContextType, &MBB_UUID_CONTEXT_TYPE_INTERNET , sizeof(GUID) ) == sizeof(GUID) )
        WwanContextType = WwanContextTypeInternet;
    else if( RtlCompareMemory( &MbbContextType, &MBB_UUID_CONTEXT_TYPE_VPN , sizeof(GUID) ) == sizeof(GUID) )
        WwanContextType = WwanContextTypeVpn;
    else if( RtlCompareMemory( &MbbContextType, &MBB_UUID_CONTEXT_TYPE_VOICE , sizeof(GUID) ) == sizeof(GUID) )
        WwanContextType = WwanContextTypeVoice;
    else if( RtlCompareMemory( &MbbContextType, &MBB_UUID_CONTEXT_TYPE_VIDEO_SHARE , sizeof(GUID) ) == sizeof(GUID) )
        WwanContextType = WwanContextTypeVideoShare;
    else if( RtlCompareMemory( &MbbContextType, &MBB_UUID_CONTEXT_TYPE_PURCHASE , sizeof(GUID) ) == sizeof(GUID) )
        WwanContextType = WwanContextTypePurchase;
    else if( RtlCompareMemory( &MbbContextType, &MBB_UUID_CONTEXT_TYPE_MMS , sizeof(GUID) ) == sizeof(GUID) )
        WwanContextType = WwanContextTypeCustom;
    else if( RtlCompareMemory( &MbbContextType, &MBB_UUID_CONTEXT_TYPE_LOCAL , sizeof(GUID) ) == sizeof(GUID) )
        WwanContextType = WwanContextTypeCustom;
    else if( RtlCompareMemory( &MbbContextType, &MBB_UUID_CONTEXT_TYPE_CUSTOM , sizeof(GUID) ) == sizeof(GUID) )
        WwanContextType = WwanContextTypeCustom;
    else if (RtlCompareMemory(&MbbContextType, &MBB_UUID_CONTEXT_TYPE_IMS, sizeof(GUID)) == sizeof(GUID))
        WwanContextType = WwanContextTypeIms;
    else if (RtlCompareMemory(&MbbContextType, &MBB_UUID_CONTEXT_TYPE_MS_ADMIN, sizeof(GUID)) == sizeof(GUID))
        WwanContextType = WwanContextTypeAdmin;
    else if (RtlCompareMemory(&MbbContextType, &MBB_UUID_CONTEXT_TYPE_MS_APP, sizeof(GUID)) == sizeof(GUID))
        WwanContextType = WwanContextTypeApp;
    else if( RtlCompareMemory( &MbbContextType, &MBB_UUID_BASIC_CONNECT_EXTENSIONS, sizeof(GUID) ) == sizeof(GUID) )
        WwanContextType = WwanContextTypeLteAttach;
    else
        WwanContextType = WwanContextTypeCustom;

    return WwanContextType;
}

VOID
MbbUtilMbbToWwanContext(
    __in  PMBB_CONTEXT  MbbContext,
    __out PWWAN_CONTEXT WwanContext
    )
{
    WwanContext->ContextId      = MbbContext->ContextId;
    WwanContext->ContextType    = MbbUtilMbbToWwanContextType( &MbbContext->ContextType );
    WwanContext->Compression    = (WWAN_COMPRESSION)MbbContext->Compression;
    WwanContext->AuthType       = (WWAN_AUTH_PROTOCOL)MbbContext->AuthProtocol;

    MbbUtilPopulateStaticString(WwanContext->AccessString, MbbContext, AccessString );

    MbbUtilPopulateStaticString(WwanContext->UserName, MbbContext, UserName );

    MbbUtilPopulateStaticString(WwanContext->Password, MbbContext, Password );

}

VOID
MbbUtilMbbToWwanLteAttachContext(
    __in  PMBB_MS_LTE_ATTACH_CONTEXT MbbLteAttachContext,
    __out WWAN_CONTEXT_V2* WwanLteAttachContext
    )
{
    WwanLteAttachContext->basicInfo.ContextId   = 0;
    WwanLteAttachContext->basicInfo.ContextType = WwanContextTypeLteAttach;
    WwanLteAttachContext->basicInfo.Compression = (WWAN_COMPRESSION)MbbLteAttachContext->Compression;
    WwanLteAttachContext->basicInfo.AuthType    = (WWAN_AUTH_PROTOCOL)MbbLteAttachContext->AuthProtocol;

    MbbUtilPopulateStaticString(WwanLteAttachContext->basicInfo.AccessString, MbbLteAttachContext, AccessString );

    MbbUtilPopulateStaticString(WwanLteAttachContext->basicInfo.UserName, MbbLteAttachContext, UserName );

    MbbUtilPopulateStaticString(WwanLteAttachContext->basicInfo.Password, MbbLteAttachContext, Password );

    if(MbbLteAttachContext->IPType >= MbbContextIPTypeDefault &&
        MbbLteAttachContext->IPType < MbbContextIPTypeMaximum)
    {
        WwanLteAttachContext->IPType = MbbIpTypesMapTable [MbbLteAttachContext->IPType];
    }
    else
    {
        TraceWarn(WMBCLASS_UTIL, "Invalid IPType returned from the Modem. Resetting it to IPTypeDefault");
        WwanLteAttachContext->IPType = WwanIPTypeDefault;
    }

    WwanLteAttachContext->Roaming = (WWAN_CONTEXT_ROAMING_CONTROL)MbbLteAttachContext->Roaming;

    if (MbbLteAttachContext->Source >= MbbMsContextSourceAdmin &&
        MbbLteAttachContext->Source < MbbMsContextSourceMaximum)
    {
        WwanLteAttachContext->Source = MbbSourcesMapTable[MbbLteAttachContext->Source];
    }
    else
    {
        TraceWarn(WMBCLASS_UTIL, "Invalid source returned from the Modem. Resetting it to WwanMaxProvisionSource");
        WwanLteAttachContext->Source = WwanMaxProvisionSource;
    }
}

VOID
MbbUtilMbbToWwanLteAttachStatus(
__in  MBB_MS_LTE_ATTACH_STATUS* MbbLteAttachStatus,
__out PWWAN_LTE_ATTACH_STATUS WwanLteAttachStatus
)
{
    WwanLteAttachStatus->LteAttachState = MbbLteAttachStatus->LteAttachState;
    if(MbbLteAttachStatus->IPType >= MbbContextIPTypeDefault &&
        MbbLteAttachStatus->IPType < MbbContextIPTypeMaximum)
    {
        WwanLteAttachStatus->IPType = MbbIpTypesMapTable [MbbLteAttachStatus->IPType];
    }
    else
    {
        TraceWarn(WMBCLASS_UTIL, "Invalid IPType returned from the Modem. Resetting it to IPTypeDefault");
        WwanLteAttachStatus->IPType = WwanIPTypeDefault;
    }

    WwanLteAttachStatus->basicInfo.ContextId   = 0;
    WwanLteAttachStatus->basicInfo.ContextType = WwanContextTypeLteAttach;
    WwanLteAttachStatus->basicInfo.Compression = (WWAN_COMPRESSION)MbbLteAttachStatus->Compression;
    WwanLteAttachStatus->basicInfo.AuthType    = (WWAN_AUTH_PROTOCOL)MbbLteAttachStatus->AuthProtocol;

    MbbUtilPopulateStaticString(WwanLteAttachStatus->basicInfo.AccessString, MbbLteAttachStatus, AccessString );

    MbbUtilPopulateStaticString(WwanLteAttachStatus->basicInfo.UserName, MbbLteAttachStatus, UserName );

    MbbUtilPopulateStaticString(WwanLteAttachStatus->basicInfo.Password, MbbLteAttachStatus, Password );
}


VOID
MbbUtilMbbToWwanContextList(
    __in  PMBB_CONTEXT_LIST MbbContextList,
    __out PWWAN_LIST_HEADER WwanContextList
    )
{
    ULONG           ContextIndex;
    ULONG           ContextCount = MbbContextList->ContextCount;
    PMBB_CONTEXT    MbbContext;
    PWWAN_CONTEXT   WwanContext;


    WwanContext = (PWWAN_CONTEXT)(WwanContextList + 1);

    for( ContextIndex = 0;
         ContextIndex < ContextCount;
         ContextIndex ++ )
    {
        MbbContext  = (PMBB_CONTEXT)((PUCHAR)MbbContextList + MbbContextList->Contexts[ContextIndex].Offset);

        MbbUtilMbbToWwanContext(
            MbbContext,
            &WwanContext[ContextIndex]
            );
    }

    WwanContextList->ElementType  = WwanStructContext;
    WwanContextList->ElementCount = ContextCount;
}

VOID
MbbUtilMbbToWwanContextV2(
    __in  PMBB_MS_CONTEXT_V2    MbbContext,
    __out PWWAN_CONTEXT_V2      WwanContext
)
{
    WwanContext->basicInfo.ContextId = MbbContext->ContextId;
    WwanContext->basicInfo.ContextType = MbbUtilMbbToWwanContextType(&MbbContext->ContextType);
    WwanContext->basicInfo.Compression = (WWAN_COMPRESSION)MbbContext->Compression;
    WwanContext->basicInfo.AuthType = (WWAN_AUTH_PROTOCOL)MbbContext->AuthProtocol;

    MbbUtilPopulateStaticString(WwanContext->basicInfo.AccessString, MbbContext, AccessString);

    MbbUtilPopulateStaticString(WwanContext->basicInfo.UserName, MbbContext, UserName);

    MbbUtilPopulateStaticString(WwanContext->basicInfo.Password, MbbContext, Password);

    WwanContext->IPType = MbbIpTypesMapTable[MbbContext->IPType];
    WwanContext->Enable = (WWAN_CONTEXT_ENABLE)MbbContext->Enable;
    WwanContext->Roaming = (WWAN_CONTEXT_ROAMING_CONTROL)MbbContext->Roaming;
    WwanContext->MediaType = (WWAN_CONTEXT_MEDIA_TYPE)MbbContext->MediaType;
    WwanContext->Source = MbbSourcesMapTable[MbbContext->Source];
}

VOID
MbbUtilMbbToWwanContextV2List(
    __in  PMBB_CONTEXT_LIST MbbContextList,
    __out PWWAN_LIST_HEADER WwanContextList
)
{
    ULONG               ContextIndex;
    ULONG               ContextCount = MbbContextList->ContextCount;
    PMBB_MS_CONTEXT_V2  MbbContext;
    PWWAN_CONTEXT_V2    WwanContext;


    WwanContext = (PWWAN_CONTEXT_V2)(WwanContextList + 1);

    for (ContextIndex = 0;
        ContextIndex < ContextCount;
        ContextIndex++)
    {
        MbbContext = (PMBB_MS_CONTEXT_V2)((PUCHAR)MbbContextList + MbbContextList->Contexts[ContextIndex].Offset);

        MbbUtilMbbToWwanContextV2(
            MbbContext,
            &WwanContext[ContextIndex]
        );
    }

    WwanContextList->ElementType = WwanStructContextV2;
    WwanContextList->ElementCount = ContextCount;
}

VOID
MbbUtilMbbToWwanServiceActivationStatus(
    __in  PMBB_SERVICE_ACTIVATION_STATUS    MbbServiceActivationStatus,
    __in  ULONG                             VendorSpecificBufferSize,
    __out PWWAN_SERVICE_ACTIVATION_STATUS   WwanServiceActivationStatus
    )
{
    WwanServiceActivationStatus->uNwError                   = MbbServiceActivationStatus->NetworkError;
    WwanServiceActivationStatus->uVendorSpecificBufferSize  = VendorSpecificBufferSize;

    RtlCopyMemory(
        WwanServiceActivationStatus + 1,
        MbbServiceActivationStatus->VendorSpecificBuffer,
        VendorSpecificBufferSize
        );
}


// SMS

WWAN_SMS_FORMAT
MbbUtilMbbToWwanSmsFormat(
    __in  MBB_SMS_FORMAT    MbbSmsFormat
    )
{
    switch( MbbSmsFormat )
    {
        case MbbSmsFormatPdu:
            return WwanSmsFormatPdu;
        case MbbSmsFormatCdma:
            return WwanSmsFormatCdma;
        default:
            return WwanSmsFormatMax;
    }
}

VOID
MbbUtilMbbToWwanSmsConfiguration(
    __in  PMBB_SMS_CONFIGURATION    MbbSmsConfiguration,
    __out PWWAN_SMS_CONFIGURATION   WwanSmsConfiguration
    )
{
    NTSTATUS        NtStatus;
    ANSI_STRING     AnsiString;
    UNICODE_STRING  UnicodeString;


    AnsiString.Buffer           = WwanSmsConfiguration->ScAddress;
    AnsiString.MaximumLength    = WWAN_SMS_ADDRESS_MAX_LEN - 1;

    UnicodeString.Buffer        = (PWCHAR)(((PUCHAR)MbbSmsConfiguration) + MbbSmsConfiguration->ScAddress.Offset);
    UnicodeString.Length        = (USHORT)MbbSmsConfiguration->ScAddress.Size;
    UnicodeString.MaximumLength = (USHORT)MbbSmsConfiguration->ScAddress.Size;

    if( (NtStatus = RtlUnicodeStringToAnsiString(
                        &AnsiString,
                        &UnicodeString,
                        FALSE // AllocateDestinationString
                        )) != STATUS_SUCCESS )
    {
        TraceError( WMBCLASS_UTIL, "[MbbNdis] FAILED to convert ScAddress Length=%dbytes", MbbSmsConfiguration->ScAddress.Size );
        WwanSmsConfiguration->ScAddress[0] = 0;
    }
    else
    {
        WwanSmsConfiguration->ScAddress[AnsiString.Length] = 0;
    }

    ASSERT( NtStatus == STATUS_SUCCESS );

    WwanSmsConfiguration->SmsFormat         = MbbUtilMbbToWwanSmsFormat( MbbSmsConfiguration->SmsFormat );
    WwanSmsConfiguration->ulMaxMessageIndex = MbbSmsConfiguration->MaxMessages;
}

NDIS_STATUS
MbbUtilValidateMbbSmsPduRecord(
    __in MBB_CELLULAR_CLASS                                 CellularClass,
    __in ULONG                                              MbbSmsPduRecordSize,
    __in_bcount(MbbSmsPduRecordSize) PMBB_SMS_PDU_RECORD    MbbSmsPduRecord
    )
{
    NDIS_STATUS NdisStatus = NDIS_STATUS_INVALID_DATA;

    do
    {
        if( MbbSmsPduRecordSize < RTL_SIZEOF_THROUGH_FIELD(MBB_SMS_PDU_RECORD, PduData) )
        {
            TraceError( WMBCLASS_OID, "[Util][Sms] INSUFFICIENT Buffer size for MBB_SMS_PDU_RECORD, Received=%d Expected=%d",
                        MbbSmsPduRecordSize,
                        RTL_SIZEOF_THROUGH_FIELD(MBB_SMS_PDU_RECORD, PduData)
                        );
            break;
        }

        if( MbbIsVariableFieldValid(
                MbbSmsPduRecordSize,
                MbbSmsPduRecord->PduData.Offset,
                MbbSmsPduRecord->PduData.Size,
                (CellularClass == MbbCellularClassGsm) ? MBB_MAXIMUM_SMS_GSM_PDU_BUFFER_LENGTH : MBB_MAXIMUM_SMS_CDMA_PDU_BUFFER_LENGTH,
                sizeof(UCHAR)
                ) != STATUS_SUCCESS )
        {
            TraceError( WMBCLASS_OID, "[Util][Sms] INVALID MBB_SMS_PDU_RECORD.Address Offset[Received=%d BufferSize=%d] Size[Received=%d MaxExpected=%d]",
                        MbbSmsPduRecord->PduData.Offset,
                        MbbSmsPduRecordSize,
                        MbbSmsPduRecord->PduData.Size,
                        ((CellularClass == MbbCellularClassGsm) ? MBB_MAXIMUM_SMS_GSM_PDU_BUFFER_LENGTH : MBB_MAXIMUM_SMS_CDMA_PDU_BUFFER_LENGTH) * sizeof(UCHAR)
                        );
            break;
        }

        NdisStatus = NDIS_STATUS_SUCCESS;
    }
    while( FALSE );

    return NdisStatus;
}

NDIS_STATUS
MbbUtilValidateMbbSmsCdmaRecord(
    __in ULONG                                              MbbSmsCdmaRecordSize,
    __in_bcount(MbbSmsCdmaRecordSize) PMBB_SMS_CDMA_RECORD  MbbSmsCdmaRecord
    )
{
    NDIS_STATUS NdisStatus = NDIS_STATUS_INVALID_DATA;

    do
    {
        if( MbbSmsCdmaRecordSize < RTL_SIZEOF_THROUGH_FIELD(MBB_SMS_CDMA_RECORD, SizeInCharacters) )
        {
            TraceError( WMBCLASS_OID, "[Util][Sms] INSUFFICIENT Buffer size for MBB_SMS_CDMA_RECORD, Received=%d Expected=%d",
                        MbbSmsCdmaRecordSize,
                        RTL_SIZEOF_THROUGH_FIELD(MBB_SMS_CDMA_RECORD, SizeInCharacters)
                        );
            break;
        }

        if( MbbIsVariableFieldValid(
                MbbSmsCdmaRecordSize,
                MbbSmsCdmaRecord->Address.Offset,
                MbbSmsCdmaRecord->Address.Size,
                MBB_MAXIMUM_SMS_ADDRESS_LENGTH,
                sizeof(WCHAR)
                ) != STATUS_SUCCESS )
        {
            TraceError( WMBCLASS_OID, "[Util][Sms] INVALID MBB_SMS_CDMA_RECORD.Address Offset[Received=%d BufferSize=%d] Size[Received=%d MaxExpected=%d]",
                        MbbSmsCdmaRecord->Address.Offset,
                        MbbSmsCdmaRecordSize,
                        MbbSmsCdmaRecord->Address.Size,
                        MBB_MAXIMUM_SMS_ADDRESS_LENGTH * sizeof(WCHAR)
                        );
            break;
        }

        if( MbbIsVariableFieldValid(
                MbbSmsCdmaRecordSize,
                MbbSmsCdmaRecord->TimeStamp.Offset,
                MbbSmsCdmaRecord->TimeStamp.Size,
                MBB_MAXIMUM_SMS_CDMA_TIMESTAMP_LENGTH,
                1
                ) != STATUS_SUCCESS )
        {
            TraceError( WMBCLASS_OID, "[Util][Sms] INVALID MBB_SMS_CDMA_RECORD.TimeStamp Offset[Received=%d BufferSize=%d] Size[Received=%d MaxExpected=%d]",
                        MbbSmsCdmaRecord->TimeStamp.Offset,
                        MbbSmsCdmaRecordSize,
                        MbbSmsCdmaRecord->TimeStamp.Size,
                        MBB_MAXIMUM_SMS_CDMA_TIMESTAMP_LENGTH
                        );
            break;
        }

        if( MbbIsVariableFieldValid(
                MbbSmsCdmaRecordSize,
                MbbSmsCdmaRecord->EncodedMessage.Offset,
                MbbSmsCdmaRecord->EncodedMessage.Size,
                MBB_MAXIMUM_SMS_CDMA_BUFFER_LENGTH,
                1
                ) != STATUS_SUCCESS )
        {
            TraceError( WMBCLASS_OID, "[Util][Sms] INVALID MBB_SMS_CDMA_RECORD.EncodedMessage Offset[Received=%d BufferSize=%d] Size[Received=%d MaxExpected=%d]",
                        MbbSmsCdmaRecord->EncodedMessage.Offset,
                        MbbSmsCdmaRecordSize,
                        MbbSmsCdmaRecord->EncodedMessage.Size,
                        MBB_MAXIMUM_SMS_CDMA_BUFFER_LENGTH
                        );
            break;
        }
        NdisStatus = NDIS_STATUS_SUCCESS;
    }
    while( FALSE );

    return NdisStatus;
}

NDIS_STATUS
MbbUtilValidateMbbSmsReceive(
    __in  MBB_CELLULAR_CLASS                        CellularClass,
    __in ULONG                                      MbbSmsReceiveSize,
    __in_bcount(MbbSmsReceiveSize) PMBB_SMS_RECEIVE MbbSmsReceive
    )
{
    ULONG           ElementIndex;
    ULONG           ElementCount;
    ULONG           MbbMaxMessageSize;
    NDIS_STATUS     NdisStatus = NDIS_STATUS_INVALID_DATA;
    ULONGLONG       ExpectedSize;

    do
    {
        ExpectedSize = RTL_SIZEOF_THROUGH_FIELD(MBB_SMS_RECEIVE, MessageCount);

        if( MbbSmsReceive == NULL ||
            MbbSmsReceiveSize < ExpectedSize )
        {
            TraceError( WMBCLASS_OID, "[Util][Sms] INSUFFICIENT BufferSize for MBB_SMS_RECEIVE, BufferSize[Received=%d Expected=%d]",
                        MbbSmsReceiveSize,
                        (ULONG)ExpectedSize
                        );
            break;
        }

        ElementCount = MbbSmsReceive->MessageCount;
        ExpectedSize += (ULONGLONG)ElementCount * sizeof(MBB_ARRAY_ELEMENT);
        if( MbbSmsReceiveSize < ExpectedSize )
        {
            TraceError( WMBCLASS_OID, "[Util][Sms] INSUFFICIENT BufferSize for MBB_SMS_RECEIVE.MessageElement[%d], BufferSize[Received=%d Expected=%I64u]",
                        ElementCount,
                        MbbSmsReceiveSize,
                        ExpectedSize
                        );
            break;
        }

        if( MbbSmsReceive->SmsFormat == MbbSmsFormatPdu )
        {

            if ( CellularClass == MbbCellularClassGsm  )
            {
                MbbMaxMessageSize   = ROUND_UP_COUNT( FIELD_OFFSET(MBB_SMS_PDU_RECORD,DataBuffer), ALIGN_DWORD ) \
                                    + ROUND_UP_COUNT( MBB_MAXIMUM_SMS_GSM_PDU_BUFFER_LENGTH, ALIGN_DWORD );
            }
            else
            {

                MbbMaxMessageSize   = ROUND_UP_COUNT( FIELD_OFFSET(MBB_SMS_PDU_RECORD,DataBuffer), ALIGN_DWORD ) \
                                    + ROUND_UP_COUNT( MBB_MAXIMUM_SMS_CDMA_PDU_BUFFER_LENGTH, ALIGN_DWORD );
            }
        }
        else
        {
            MbbMaxMessageSize   = ROUND_UP_COUNT( FIELD_OFFSET(MBB_SMS_CDMA_RECORD,DataBuffer), ALIGN_DWORD ) \
                                + ROUND_UP_COUNT( (MBB_MAXIMUM_SMS_ADDRESS_LENGTH * sizeof(WCHAR)), ALIGN_DWORD ) \
                                + ROUND_UP_COUNT( MBB_MAXIMUM_SMS_CDMA_TIMESTAMP_LENGTH, ALIGN_DWORD ) \
                                + ROUND_UP_COUNT( MBB_MAXIMUM_SMS_CDMA_BUFFER_LENGTH, ALIGN_DWORD );
        }

        for( ElementIndex = 0;
             ElementIndex < ElementCount;
             ElementIndex ++ )
        {
            if( MbbIsVariableFieldValid(
                    MbbSmsReceiveSize,
                    MbbSmsReceive->MessageElement[ElementIndex].Offset,
                    MbbSmsReceive->MessageElement[ElementIndex].Size,
                    MbbMaxMessageSize,
                    1
                    ) != STATUS_SUCCESS )
            {
                TraceError( WMBCLASS_OID, "[Util][Sms] INVALID Offset/Size for MBB_SMS_RECEIVE.MessageElement[%d], Offset[Received=%d BufferSize=%d] Size[Received=%d MaxExpected=%d]",
                            ElementIndex,
                            MbbSmsReceive->MessageElement[ElementIndex].Offset,
                            MbbSmsReceiveSize,
                            MbbSmsReceive->MessageElement[ElementIndex].Size,
                            MbbMaxMessageSize
                            );
                break;
            }

            if( MbbSmsReceive->SmsFormat == MbbSmsFormatPdu )
            {
                if( MbbUtilValidateMbbSmsPduRecord(
                        CellularClass,
                        MbbSmsReceive->MessageElement[ElementIndex].Size,
                        (PMBB_SMS_PDU_RECORD)(((PUCHAR)MbbSmsReceive)+MbbSmsReceive->MessageElement[ElementIndex].Offset)
                        ) != STATUS_SUCCESS )
                {
                    TraceError( WMBCLASS_OID, "[Util][Sms] INVALID MBB_SMS_PDU_RECORD at MBB_SMS_RECEIVE.MessageElement[%d]", ElementIndex );
                    break;
                }
            }
            else
            {
                if( MbbUtilValidateMbbSmsCdmaRecord(
                        MbbSmsReceive->MessageElement[ElementIndex].Size,
                        (PMBB_SMS_CDMA_RECORD)(((PUCHAR)MbbSmsReceive)+MbbSmsReceive->MessageElement[ElementIndex].Offset)
                        ) != STATUS_SUCCESS )
                {
                    TraceError( WMBCLASS_OID, "[Util][Sms] INVALID MBB_SMS_CDMA_RECORD at MBB_SMS_RECEIVE.MessageElement[%d]", ElementIndex );
                    break;
                }
            }
        }

        if( ElementIndex == ElementCount )
            NdisStatus = NDIS_STATUS_SUCCESS;
    }
    while( FALSE );

    return NdisStatus;
}

VOID
MbbUtilMbbToWwanSmsPduRecord(
    __in  PMBB_SMS_PDU_RECORD   MbbSmsPduRecord,
    __in  MBB_CELLULAR_CLASS    CellularClass,
    __out PWWAN_SMS_PDU_RECORD  WwanSmsPduRecord
    )
/*++
Description:
    For GSM-based devices, messages are hexadecimal strings that 
    represent messages in PDU format as defined in the
    3GPP TS 27.005 and 3GPP TS 23.040 specifications.

    For CDMA-based devices messages are byte arrays that represent
    messages as defined in section 3.4.2.1 SMS Point-to-Point Message
    in 3GPP2 specification C.S0015-A "Short Message Service (SMS)
    for Wideband Spread Spectrum Systems".

Assumption:
    Buffer and length validation is already performed.
--*/
{
    WwanSmsPduRecord->MessageIndex   = MbbSmsPduRecord->MessageIndex;
    WwanSmsPduRecord->MsgStatus      = (WWAN_MSG_STATUS)MbbSmsPduRecord->MessageStatus;
    //
    // Although the MSDN document says that Pdu.Size should excluded the SMSC address length
    // no test code or logo code handles it that way. Pdu.Size is the size of the complete PduData.
    //
    WwanSmsPduRecord->Size           = (UCHAR)MbbSmsPduRecord->PduData.Size;

    if( CellularClass == MbbCellularClassGsm )
    {
        MbbUtilByteStreamToHexString(
            ((PUCHAR)MbbSmsPduRecord) + MbbSmsPduRecord->PduData.Offset,
            MbbSmsPduRecord->PduData.Size,
            WwanSmsPduRecord->PduData,
            WWAN_SMS_PDU_HEX_BUF_LEN
            );
    }
    else if( CellularClass == MbbCellularClassCdma )
    {
        RtlCopyMemory(
            WwanSmsPduRecord->PduData,
            ((PUCHAR)MbbSmsPduRecord) + MbbSmsPduRecord->PduData.Offset,
            MbbSmsPduRecord->PduData.Size
            );
    }
}

VOID
MbbUtilMbbToWwanSmsCdmaRecord(
    __in  PMBB_SMS_CDMA_RECORD  MbbSmsCdmaRecord,
    __out PWWAN_SMS_CDMA_RECORD WwanSmsCdmaRecord
    )
/*++
Assumption:
    Buffer and length validation is already performed.
--*/
{
    ULONG MinSize;
    NTSTATUS        NtStatus;
    ANSI_STRING     AnsiString;
    UNICODE_STRING  UnicodeString;

    RtlZeroMemory(WwanSmsCdmaRecord, sizeof(WWAN_SMS_CDMA_RECORD));

    WwanSmsCdmaRecord->MessageIndex       = MbbSmsCdmaRecord->MessageIndex;
    WwanSmsCdmaRecord->MsgStatus          = (WWAN_MSG_STATUS)MbbSmsCdmaRecord->MessageStatus;
    WwanSmsCdmaRecord->EncodingId         = (WWAN_SMS_CDMA_ENCODING)MbbSmsCdmaRecord->EncodingId;
    WwanSmsCdmaRecord->LanguageId         = (WWAN_SMS_CDMA_LANG)MbbSmsCdmaRecord->LanguageId;
    //
    // Address
    //
    AnsiString.Buffer           = WwanSmsCdmaRecord->Address;
    AnsiString.MaximumLength    = WWAN_SMS_CDMA_ADDR_MAX_LEN - 1;

    UnicodeString.Buffer        = (PWCHAR)(((PCHAR)MbbSmsCdmaRecord) + MbbSmsCdmaRecord->Address.Offset);
    UnicodeString.Length        = (USHORT)MbbSmsCdmaRecord->Address.Size;
    UnicodeString.MaximumLength = (USHORT)MbbSmsCdmaRecord->Address.Size;

    if( (NtStatus = RtlUnicodeStringToAnsiString(
                        &AnsiString,
                        &UnicodeString,
                        FALSE // AllocateDestinationString
                        )) != STATUS_SUCCESS )
    {
        TraceError( WMBCLASS_UTIL, "[Util][Sms] FAILED to convert MBB_SMS_CDMA_RECORD.Address Length=%dbytes", MbbSmsCdmaRecord->Address.Size );
        WwanSmsCdmaRecord->Address[0] = 0;
    }
    else
    {
        WwanSmsCdmaRecord->Address[AnsiString.Length] = 0;
    }
    ASSERT( NtStatus == STATUS_SUCCESS );
    //
    // Timestamp
    //
    RtlZeroMemory(
        WwanSmsCdmaRecord->ScTimeStamp,
        WWAN_SMS_CDMA_TIMESTAMP_MAX_LEN
        );
    RtlCopyMemory(
        WwanSmsCdmaRecord->ScTimeStamp,
        (((PCHAR)MbbSmsCdmaRecord) + MbbSmsCdmaRecord->TimeStamp.Offset),
        MIN( MbbSmsCdmaRecord->TimeStamp.Size, WWAN_SMS_CDMA_TIMESTAMP_MAX_LEN )
        );
    //
    // EncodedMessage
    //
    RtlCopyMemory(
        WwanSmsCdmaRecord->EncodedMsg,
        (((PCHAR)MbbSmsCdmaRecord) + MbbSmsCdmaRecord->EncodedMessage.Offset),
        MIN( MbbSmsCdmaRecord->EncodedMessage.Size, WWAN_SMS_CDMA_MAX_BUF_LEN )
        );
    //
    // If there is truncation SizeInBytes and SizeInCharacters will go out of sync
    // since we dont know how to calculate SizeInCharacters.
    //
    WwanSmsCdmaRecord->SizeInBytes        = (USHORT)MIN( MbbSmsCdmaRecord->EncodedMessage.Size, WWAN_SMS_CDMA_MAX_BUF_LEN );
    WwanSmsCdmaRecord->SizeInCharacters   = (UCHAR)MbbSmsCdmaRecord->SizeInCharacters;
}

NDIS_STATUS
MbbUtilMbbToWwanSmsReceive(
    __in_bcount(MbbSmsReceiveSize) PMBB_SMS_RECEIVE MbbSmsReceive,
    __in  ULONG                                     MbbSmsReceiveSize,
    __in  MBB_CELLULAR_CLASS                        CellularClass,
    __out PWWAN_LIST_HEADER                         SmsListHeader
    )
{
    ULONG                   SmsIndex;

    if( MbbSmsReceive->SmsFormat == MbbSmsFormatPdu )
    {
        PWWAN_SMS_PDU_RECORD    WwanSmsPduRecord;
        PMBB_SMS_PDU_RECORD     MbbSmsPduRecord;

        if( CellularClass != MbbCellularClassGsm &&
            CellularClass != MbbCellularClassCdma )
        {
            TraceError( WMBCLASS_UTIL, "[Util][Sms] FAILED to convert PDU record due to invalid CurrentCellularClass=%!MbbCellularClass!",
                        CellularClass
                        );
            return NDIS_STATUS_INVALID_STATE;
        }

        SmsListHeader->ElementType = WwanStructSmsPdu;

        for(SmsIndex = 0;
            SmsIndex < MbbSmsReceive->MessageCount;
            SmsIndex ++ )
        {
            MbbSmsPduRecord  = (PMBB_SMS_PDU_RECORD)(((PCHAR)MbbSmsReceive) + MbbSmsReceive->MessageElement[SmsIndex].Offset);
            WwanSmsPduRecord = &((PWWAN_SMS_PDU_RECORD)(SmsListHeader+1))[SmsIndex];

            MbbUtilMbbToWwanSmsPduRecord(
                MbbSmsPduRecord,
                CellularClass,
                WwanSmsPduRecord
                );
        }
    }
    else if( MbbSmsReceive->SmsFormat == MbbSmsFormatCdma )
    {
        PMBB_SMS_CDMA_RECORD    MbbSmsCdmaRecord;
        PWWAN_SMS_CDMA_RECORD   WwanSmsCdmaRecord;

        SmsListHeader->ElementType = WwanStructSmsCdma;

        for(SmsIndex = 0;
            SmsIndex < MbbSmsReceive->MessageCount;
            SmsIndex ++ )
        {
            MbbSmsCdmaRecord  = (PMBB_SMS_CDMA_RECORD)(((PCHAR)MbbSmsReceive) + MbbSmsReceive->MessageElement[SmsIndex].Offset);
            WwanSmsCdmaRecord = &((PWWAN_SMS_CDMA_RECORD)(SmsListHeader+1))[SmsIndex];

            MbbUtilMbbToWwanSmsCdmaRecord(
                MbbSmsCdmaRecord,
                WwanSmsCdmaRecord
                );
        }
    }
    else
    {
        TraceError( WMBCLASS_OID, "[Util][Sms] INVALID MBB SmsFormat=%d", MbbSmsReceive->SmsFormat );
        return NDIS_STATUS_INVALID_DATA;
    }
    SmsListHeader->ElementCount = MbbSmsReceive->MessageCount;

    return NDIS_STATUS_SUCCESS;
}

VOID
MbbUtilMbbToWwanSmsStatus(
    __in  PMBB_SMS_STATUS   MbbSmsStatus,
    __out PWWAN_SMS_STATUS  WwanSmsStatus
    )
{
    WwanSmsStatus->MessageIndex = MbbSmsStatus->MessageIndex;
    WwanSmsStatus->uFlag        = WWAN_SMS_FLAG_NONE;

    if( MbbSmsStatus->StatusFlags & MbbSmsFlagMessageStoreFull )
        WwanSmsStatus->uFlag |= WWAN_SMS_FLAG_MESSAGE_STORE_FULL;

    if( MbbSmsStatus->StatusFlags & MbbSmsFlagNewMessage )
        WwanSmsStatus->uFlag |= WWAN_SMS_FLAG_NEW_MESSAGE;
}

VOID
MbbUtilMbbToWwanUiccFileStatus(
    __in PMBB_UICC_FILE_STATUS      MbbUiccFileStatus,
    __out PWWAN_UICC_FILE_STATUS    WwanUiccFileStatus
    )
{
    WwanUiccFileStatus->StatusWord1         = MbbUiccFileStatus->StatusWord1;
    WwanUiccFileStatus->StatusWord2         = MbbUiccFileStatus->StatusWord2;
    WwanUiccFileStatus->FileAccessibility   = (WWAN_UICC_FILE_ACCESSIBILITY)MbbUiccFileStatus->FileAccessibility;
    WwanUiccFileStatus->FileType            = (WWAN_UICC_FILE_TYPE)MbbUiccFileStatus->FileType;
    WwanUiccFileStatus->FileStructure       = (WWAN_UICC_FILE_STRUCTURE)MbbUiccFileStatus->FileStructure;
    WwanUiccFileStatus->ItemCount           = MbbUiccFileStatus->ItemCount;
    WwanUiccFileStatus->ItemSize            = MbbUiccFileStatus->ItemSize;

    RtlCopyMemory(
        WwanUiccFileStatus->FileLockStatus,
        MbbUiccFileStatus->FileLockStatus,
        sizeof(MbbUiccFileStatus->FileLockStatus)
        );
}

VOID
MbbUtilMbbToWwanUiccResponse(
    __in PMBB_UICC_RESPONSE     MbbUiccResponse,
    __out PWWAN_UICC_RESPONSE   WwanUiccResponse
    )
{
    WwanUiccResponse->StatusWord1           = MbbUiccResponse->StatusWord1;
    WwanUiccResponse->StatusWord2           = MbbUiccResponse->StatusWord2;
    WwanUiccResponse->ResponseDataSize      = (USHORT)MbbUiccResponse->ResponseData.Size;

    RtlCopyMemory(
        WwanUiccResponse->ResponseData,
        ((PUCHAR)MbbUiccResponse) + MbbUiccResponse->ResponseData.Offset,
        MbbUiccResponse->ResponseData.Size
        );
}

VOID
MbbUtilMbbToWwanUiccAppList(
    __in PMBB_UICC_APP_LIST       MbbUiccAppList,
    __out PWWAN_UICC_APP_LIST     WwanUiccAppList
)
{
    ULONG AppCount = MbbUiccAppList->AppCount;
    ULONG AppIndex = 0;
    PWWAN_UICC_APP_INFO WwanUiccAppInfo = NULL;
    PMBB_UICC_APP_INFO MbbUiccAppInfo = NULL;

    WwanUiccAppList->Version = MbbUiccAppList->Version;
    WwanUiccAppList->AppCount = MbbUiccAppList->AppCount;
    WwanUiccAppList->ActiveAppIndex = MbbUiccAppList->ActiveAppIndex;
    WwanUiccAppList->AppListSize = MbbUiccAppList->AppListSize;

    for (AppIndex = 0; AppIndex < AppCount; AppIndex++)
    {
        WwanUiccAppInfo = (PWWAN_UICC_APP_INFO)((PUCHAR)WwanUiccAppList
            + sizeof(WWAN_UICC_APP_LIST) + sizeof(WWAN_UICC_APP_INFO) * AppIndex);
        MbbUiccAppInfo = (PMBB_UICC_APP_INFO)((PUCHAR)MbbUiccAppList
            + MbbUiccAppList->DataBuffer[AppIndex].Offset);

        WwanUiccAppInfo->AppType = (WWAN_UICC_APP_TYPE)MbbUiccAppInfo->AppType;
        WwanUiccAppInfo->AppIdSize = (BYTE)(MbbUiccAppInfo->AppId.Size);
        WwanUiccAppInfo->AppNameLength = (BYTE)(MbbUiccAppInfo->AppName.Size);
        WwanUiccAppInfo->NumPins = MbbUiccAppInfo->NumPins;
        RtlCopyMemory(WwanUiccAppInfo->AppId,
            (PUCHAR)WwanUiccAppInfo + MbbUiccAppInfo->AppId.Offset,
            MbbUiccAppInfo->AppId.Size);
        RtlCopyMemory(WwanUiccAppInfo->AppName,
            (PUCHAR)WwanUiccAppInfo + MbbUiccAppInfo->AppName.Offset,
            MbbUiccAppInfo->AppName.Size);
        RtlCopyMemory(WwanUiccAppInfo->PinRef,
            (PUCHAR)WwanUiccAppInfo + MbbUiccAppInfo->PinRef.Offset,
            MbbUiccAppInfo->PinRef.Size);
    }
}

VOID
MbbUtilMbbToWwanSysCapsInfo(
    __in PMBB_MS_SYS_CAPS_INFO  MbbSysCapsInfo,
    __out PWWAN_SYS_CAPS_INFO   WwanSysCapsInfo
    )
{
    WwanSysCapsInfo->NumberOfExecutors = MbbSysCapsInfo->NumberOfExecutors;
    WwanSysCapsInfo->NumberOfSlots = MbbSysCapsInfo->NumberOfSlots;
    WwanSysCapsInfo->Concurrency = MbbSysCapsInfo->Concurrency;
    WwanSysCapsInfo->ModemID = MbbSysCapsInfo->ModemId;
}

VOID
MbbUtilMbbToWwanDeviceSlotMappingInfo(
    __in  PMBB_MS_DEVICE_SLOT_MAPPING_INFO  MbbDeviceSlotMappingInfo,
    __out PWWAN_DEVICE_SLOT_MAPPING_INFO    WwanDeviceSlotMappingInfo
    )
{
    WwanDeviceSlotMappingInfo->SlotMapListHeader.ElementType = WwanStructDeviceSlotMap;
    WwanDeviceSlotMappingInfo->SlotMapListHeader.ElementCount = MbbDeviceSlotMappingInfo->MapCount;

    RtlCopyMemory((PUCHAR)WwanDeviceSlotMappingInfo + sizeof(WWAN_DEVICE_SLOT_MAPPING_INFO),
                  (PUCHAR)MbbDeviceSlotMappingInfo + MbbDeviceSlotMappingInfo->SlotMapList[0].Offset,
                  sizeof(ULONG) * MbbDeviceSlotMappingInfo->MapCount);
}

VOID
MbbUtilMbbToWwanSlotInfo(
    __in  PMBB_MS_SLOT_INFO     MbbSlotInfo,
    __out PWWAN_SLOT_INFO       WwanSlotInfo
    )
{
    WwanSlotInfo->SlotIndex = MbbSlotInfo->SlotIndex;
    WwanSlotInfo->State = (WWAN_UICCSLOT_STATE)(MbbSlotInfo->State);
}

VOID
MbbUtilMbbToWwanDeviceBindingsInfo(
    __in  PMBB_DEVICE_BINDINGS_INFO   MbbDeviceBindingsInfo,
    __out PWWAN_DEVICE_BINDINGS_INFO  WwanDeviceBindingsInfo
    )
{
    ULONG AppCount = MbbDeviceBindingsInfo->ApplicationCount;
    ULONG AppIndex = 0;
    PWWAN_UICC_FILE_PATH WwanUiccFilePath = NULL;
    PMBB_UICC_FILE_PATH MbbUiccFilePath = NULL;

    WwanDeviceBindingsInfo->ApplicationListHeader.ElementType = WwanStructUiccApplication;
    WwanDeviceBindingsInfo->ApplicationListHeader.ElementCount = AppCount;
    
    for (AppIndex = 0; AppIndex < AppCount; AppIndex++)
    {
        WwanUiccFilePath = (PWWAN_UICC_FILE_PATH)((PUCHAR)WwanDeviceBindingsInfo 
            + sizeof(WWAN_DEVICE_BINDINGS_INFO) + sizeof(WWAN_UICC_FILE_PATH) * AppIndex);
        MbbUiccFilePath = (PMBB_UICC_FILE_PATH)((PUCHAR)MbbDeviceBindingsInfo 
            + MbbDeviceBindingsInfo->ApplicationList[AppIndex].Offset);

        RtlZeroMemory(WwanUiccFilePath, sizeof(WWAN_UICC_FILE_PATH));

        WwanUiccFilePath->AppIdLength = (BYTE)(MbbUiccFilePath->AppId.Size);
        RtlCopyMemory(WwanUiccFilePath->AppId, 
                      (PUCHAR)MbbUiccFilePath + MbbUiccFilePath->AppId.Offset,
                      MbbUiccFilePath->AppId.Size);

        // FilePath in MBB_UICC_FILE_PATH is not used for this OID/CID.
    }
}

VOID
MbbUtilMbbToWwanRegistrationStateInfoV2(
    __in  PMBB_REGISTRATION_STATE_INFOS_V2  MbbRegistrationStateInfosV2,
    __out PWWAN_REGISTRATION_STATE_INFOS    WwanRegistrationStateInfos
    )
{
    ULONG RegistrationStateCount = MbbRegistrationStateInfosV2->ElementCount;
    ULONG RegistrationStateIndex = 0;
    PWWAN_REGISTRATION_STATE_EX WwanRegistrationStateEx = NULL;
    PMBB_REGISTRATION_STATE_V2 MbbRegistrationStateV2 = NULL;
    
    WwanRegistrationStateInfos->RegistrationStateListHeader.ElementType = WwanStructRegistrationState;
    WwanRegistrationStateInfos->RegistrationStateListHeader.ElementCount = RegistrationStateCount;
    
    for (RegistrationStateIndex = 0; RegistrationStateIndex < RegistrationStateCount; RegistrationStateIndex++)
    {
        WwanRegistrationStateEx = (PWWAN_REGISTRATION_STATE_EX)((PUCHAR)WwanRegistrationStateInfos
            + sizeof(WWAN_REGISTRATION_STATE_INFOS) + sizeof(WWAN_REGISTRATION_STATE_EX) * RegistrationStateIndex);
        MbbRegistrationStateV2 = (PMBB_REGISTRATION_STATE_V2)((PUCHAR)MbbRegistrationStateInfosV2
            + MbbRegistrationStateInfosV2->RegistrationStateList[RegistrationStateIndex].Offset);

        RtlZeroMemory(WwanRegistrationStateEx, sizeof(WWAN_REGISTRATION_STATE_EX));

        WwanRegistrationStateEx->uNwError = MbbRegistrationStateV2->NetworkError;
        WwanRegistrationStateEx->RegisterState = (WWAN_REGISTER_STATE)(MbbRegistrationStateV2->RegisterState);
        WwanRegistrationStateEx->RegisterMode = (WWAN_REGISTER_MODE)(MbbRegistrationStateV2->RegisterMode);

        MbbUtilPopulateStaticString(WwanRegistrationStateEx->ProviderId, MbbRegistrationStateV2, ProviderId);
        MbbUtilPopulateStaticString(WwanRegistrationStateEx->ProviderName, MbbRegistrationStateV2, ProviderName);
        MbbUtilPopulateStaticString(WwanRegistrationStateEx->RoamingText, MbbRegistrationStateV2, RoamingText);

        WwanRegistrationStateEx->WwanRegFlags = (DWORD)(MbbRegistrationStateV2->RegFlags);
        WwanRegistrationStateEx->CurrentCellularClass = (WWAN_CELLULAR_CLASS)(MbbRegistrationStateV2->CurrentCellularClass);
        WwanRegistrationStateEx->AvailableDataClasses = MbbRegistrationStateV2->AvailableDataClasses;
        WwanRegistrationStateEx->VoiceSupport = MbbRegistrationStateV2->VoiceSupport;
        WwanRegistrationStateEx->CurrentRATClass = MbbRegistrationStateV2->CurrentRATClass;
    }
}

VOID
MbbUtilMbbToWwanSignalStateInfoV2(
    __in  PMBB_SIGNAL_STATE_INFOS_V2    MbbSignalStateInfosV2,
    __out PWWAN_SIGNAL_STATE_INFOS      WwanSignalStateInfos
    )
{
    ULONG SignalStateCount = MbbSignalStateInfosV2->ElementCount;
    ULONG SignalStateIndex = 0;
    PWWAN_SIGNAL_STATE_EX WwanSignalStateEx = NULL;
    PMBB_SIGNAL_STATE_V2  MbbSignalStateV2 = NULL;

    WwanSignalStateInfos->SignalStateListHeader.ElementType = WwanStructSignalState;
    WwanSignalStateInfos->SignalStateListHeader.ElementCount = SignalStateCount;


    for (SignalStateIndex = 0; SignalStateIndex < SignalStateCount; SignalStateIndex++)
    {
        WwanSignalStateEx = (PWWAN_SIGNAL_STATE_EX)((PUCHAR)WwanSignalStateInfos
            + sizeof(WWAN_SIGNAL_STATE_INFOS) + sizeof(WWAN_SIGNAL_STATE_EX) * SignalStateIndex);
        MbbSignalStateV2 = (PMBB_SIGNAL_STATE_V2)((PUCHAR)MbbSignalStateInfosV2
            + MbbSignalStateInfosV2->SignalStateList[SignalStateIndex].Offset);

        WwanSignalStateEx->Rssi = MbbSignalStateV2->Rssi;
        WwanSignalStateEx->ErrorRate = MbbSignalStateV2->ErrorRate;
        WwanSignalStateEx->RssiInterval = MbbSignalStateV2->RssiInterval;
        WwanSignalStateEx->RssiThreshold = MbbSignalStateV2->RssiThreshold;
        WwanSignalStateEx->ErrorRateThreshold = MbbSignalStateV2->ErrorRateThreshold;
        WwanSignalStateEx->Snr = MbbSignalStateV2->Snr;
        WwanSignalStateEx->SnrThreshold = MbbSignalStateV2->SnrThreshold;
        WwanSignalStateEx->DataClass = MbbSignalStateV2->DataClass;
    }
}

VOID
MbbUtilMbbToWwanAtrInfo(
    __in PMBB_ATR_INFO      MbbAtrInfo,
    __out PWWAN_ATR_INFO    WwanAtrInfo
    )
{
    WwanAtrInfo->AtrLength = MbbAtrInfo->Atr.Size;

    RtlCopyMemory(
        WwanAtrInfo->Atr,
        (PUCHAR)MbbAtrInfo + MbbAtrInfo->Atr.Offset,
        MbbAtrInfo->Atr.Size
        );
}

VOID
MbbUtilMbbToWwanUiccOpenChannelInfo(
    __in PMBB_UICC_OPEN_CHANNEL_INFO    MbbUiccOpenChannelInfo,
    __out PWWAN_UICC_OPEN_CHANNEL_INFO  WwanUiccOpenChannelInfo
    )
{
    RtlCopyMemory(
        WwanUiccOpenChannelInfo->Status,
        MbbUiccOpenChannelInfo->Status,
        WWAN_UICC_STATUS_LEN
        );

    WwanUiccOpenChannelInfo->Channel        = MbbUiccOpenChannelInfo->Channel;
    WwanUiccOpenChannelInfo->ResponseLength = MbbUiccOpenChannelInfo->Response.Size;

    RtlCopyMemory(
        WwanUiccOpenChannelInfo->Response,
        (PUCHAR)MbbUiccOpenChannelInfo + MbbUiccOpenChannelInfo->Response.Offset,
        MbbUiccOpenChannelInfo->Response.Size
        );
}

VOID
MbbUtilMbbToWwanUiccCloseChannelInfo(
    __in PMBB_UICC_CLOSE_CHANNEL_INFO   MbbUiccCloseChannelInfo,
    __out PWWAN_UICC_CLOSE_CHANNEL_INFO WwanUiccCloseChannelInfo
    )
{
    RtlCopyMemory(
        WwanUiccCloseChannelInfo->Status,
        MbbUiccCloseChannelInfo->Status,
        WWAN_UICC_STATUS_LEN
        );
}

VOID
MbbUtilMbbToWwanUiccApduInfo(
    __in PMBB_UICC_APDU_INFO    MbbUiccApduInfo,
    __out PWWAN_UICC_APDU_INFO  WwanUiccApduInfo
    )
{
    RtlCopyMemory(
        WwanUiccApduInfo->Status,
        MbbUiccApduInfo->Status,
        WWAN_UICC_STATUS_LEN
        );

    WwanUiccApduInfo->ResponseLength    = MbbUiccApduInfo->Response.Size;

    RtlCopyMemory(
        WwanUiccApduInfo->Response,
        (PUCHAR)MbbUiccApduInfo + MbbUiccApduInfo->Response.Offset,
        MbbUiccApduInfo->Response.Size
        );
}

VOID
MbbUtilMbbToWwanUiccTerminalCapabilityInfo(
    __in PMBB_UICC_TERMINAL_CAPABILITY_INFO     MbbUiccTerminalCapabilityInfo,
    __out PWWAN_LIST_HEADER                     WwanCapabilityListHeader
    )
{
    ULONG CapabilityCount = MbbUiccTerminalCapabilityInfo->ElementCount;
    ULONG CapabilityIndex = 0;
    
    WwanCapabilityListHeader->ElementType = WwanStructUiccTerminalCapability;
    WwanCapabilityListHeader->ElementCount = CapabilityCount;
    
    for (CapabilityIndex = 0; CapabilityIndex < CapabilityCount; CapabilityIndex++)
    {
        RtlCopyMemory(
            (PUCHAR)WwanCapabilityListHeader + sizeof(WWAN_LIST_HEADER) + sizeof(WWAN_UICC_TERMINAL_CAPABILITY_TLV) * CapabilityIndex,
            (PUCHAR)MbbUiccTerminalCapabilityInfo + MbbUiccTerminalCapabilityInfo->CapabilityList[CapabilityIndex].Offset,
            sizeof(MBB_UICC_TERMINAL_CAPABILITY_TLV)
            );
    }
}

VOID
MbbUtilMbbToWwanSarConfigInfo(
    _In_ PMBB_SAR_CONFIG_INFO MbbSarConfigInfo,
    _Out_ PWWAN_SAR_CONFIG_INFO WwanSarConfigInfo
)
{
    ULONG count = MbbSarConfigInfo->ElementCount;

    WwanSarConfigInfo->SarMode = (WWAN_SAR_CONTROL_MODE)MbbSarConfigInfo->SarMode;
    WwanSarConfigInfo->SarBackoffStatus = (WWAN_SAR_BACKOFF_STATE)MbbSarConfigInfo->SarBackoffStatus;
    WwanSarConfigInfo->SarWifiIntegration = (WWAN_SAR_WIFI_HARDWARE_INTEGRATION)MbbSarConfigInfo->SarWifiIntegration;
    WwanSarConfigInfo->SarConfigIndexListHeader.ElementType = WwanStructSarConfig;
    WwanSarConfigInfo->SarConfigIndexListHeader.ElementCount = count;

    RtlCopyMemory(
        (PBYTE)WwanSarConfigInfo + sizeof(WWAN_SAR_CONFIG_INFO),
        (PBYTE)MbbSarConfigInfo + MbbSarConfigInfo->Configurations[0].Offset,
        sizeof(WWAN_SAR_CONFIG_INDICES) * count
    );
}

VOID
MbbUtilMbbToWwanTransmissionStatusInfo(
    _In_ PMBB_SAR_TRANSMISSION_STATUS_INFO MbbTransmissionStatusInfo,
    _Out_ PWWAN_SAR_TRANSMISSION_STATUS_INFO WwanTransmissionStatusInfo
)
{
    WwanTransmissionStatusInfo->ChannelNotification = (WWAN_SAR_TRANSMISSION_STATUS_NOTIFICATION_STATE)MbbTransmissionStatusInfo->ChannelNotification;
    WwanTransmissionStatusInfo->TransmissionStatus = (WWAN_SAR_TRANSMISSION_STATUS)MbbTransmissionStatusInfo->TransmissionStatus;
    WwanTransmissionStatusInfo->HysteresisTimer = MbbTransmissionStatusInfo->HysteresisTimer;
}


VOID
MbbUtilMbbToWwanNetworkBlackList(
    __in  PMBB_MS_NETWORK_BLACKLIST_INFO MbbNetworkBlacklist,
    __out PWWAN_LIST_HEADER WwanBlacklistProviderList
)
{
    ULONG           ProviderIndex;
    ULONG           ProviderCount = MbbNetworkBlacklist->ElementCount;
    PMBB_MS_NETWORK_BLACKLIST_PROVIDER      MbbProvider;
    PWWAN_NETWORK_BLACKLIST_PROVIDER        WwanProvider;

    WwanProvider = (PWWAN_NETWORK_BLACKLIST_PROVIDER)(WwanBlacklistProviderList + 1);

    for (ProviderIndex = 0;
        ProviderIndex < ProviderCount;
        ProviderIndex++)
    {
        MbbProvider = (PMBB_MS_NETWORK_BLACKLIST_PROVIDER)((PUCHAR)MbbNetworkBlacklist + MbbNetworkBlacklist->Contexts[ProviderIndex].Offset);

        WwanProvider[ProviderIndex].MCC = MbbProvider->MCC;
        WwanProvider[ProviderIndex].MNC = MbbProvider->MNC;
        WwanProvider[ProviderIndex].NetworkBlacklistType = MbbProvider->NetworkBlacklistType;
    }

    WwanBlacklistProviderList->ElementType = WwanStructNetworkBlacklistProvider;
    WwanBlacklistProviderList->ElementCount = ProviderCount;
}

//
// Drain Object
//

VOID
InitDrainObject(
    PDRAIN_OBJECT               DrainObject,
    MBB_DRAIN_COMPLETE          DrainCompleteCallback,
    PVOID                       Context
    )
/*++
    The drain object is initialized as draining because
    the first thing that will happen on miniport initialization
    is the Restart handler will get called which will call
    DrainComplete. DrainComplete requires that Draining is 
    set to TRUE and it will set Draining to FALSE starting the
    queue.
--*/
{
    RtlZeroMemory(DrainObject, sizeof(*DrainObject));

    NdisAllocateSpinLock(&DrainObject->Lock);
    DrainObject->Draining=TRUE;
    DrainObject->DrainComplete=TRUE;
    DrainObject->Count=0;
    DrainObject->DrainCompleteCallback=DrainCompleteCallback;
    DrainObject->Context=Context;
}

VOID
StartDrain(
    PDRAIN_OBJECT               DrainObject
    )

{
    BOOLEAN                     CallComplete=FALSE;

    NdisAcquireSpinLock(&DrainObject->Lock);

    ASSERT(!DrainObject->Draining);

    DrainObject->Draining=TRUE;
    DrainObject->DrainComplete=FALSE;

    if (DrainObject->Count == 0)
    {
        CallComplete=TRUE;
        DrainObject->DrainComplete=TRUE;
    }

    NdisReleaseSpinLock(&DrainObject->Lock);

    if (CallComplete)
    {
        DrainObject->DrainCompleteCallback(DrainObject->Context);
    }

    return;
}

VOID
DrainComplete(
    PDRAIN_OBJECT               DrainObject
    )

{
    NdisAcquireSpinLock(&DrainObject->Lock);

    ASSERT(DrainObject->Draining);
    ASSERT(DrainObject->DrainComplete);

    DrainObject->Draining=FALSE;

    NdisReleaseSpinLock(&DrainObject->Lock);

}

BOOLEAN
DrainAddRef(
    PDRAIN_OBJECT               DrainObject
    )

{
    BOOLEAN                     Ready=FALSE;

    NdisAcquireSpinLock(&DrainObject->Lock);

    if ( (Ready = !DrainObject->Draining) )
    {
        DrainObject->Count++;
    }

    NdisReleaseSpinLock(&DrainObject->Lock);

    return Ready;
}

VOID
DrainRelease(
    PDRAIN_OBJECT               DrainObject
    )

{
    BOOLEAN                     CallComplete=FALSE;

    NdisAcquireSpinLock(&DrainObject->Lock);
    DrainObject->Count--;
    if ((DrainObject->Count == 0) && DrainObject->Draining)
    {
        CallComplete=TRUE;
        DrainObject->DrainComplete=TRUE;
    }
    NdisReleaseSpinLock(&DrainObject->Lock);

    if (CallComplete)
    {
        DrainObject->DrainCompleteCallback(DrainObject->Context);
    }

    return;


}

NTSTATUS
MbbEventActivityIdCtl (
   _Inout_  LPGUID ActivityId
    )
/* ++
Routine Description:

   Creates an activity identifier

Arguments:

  
ActivityId [in, out] - The identifier that indicates the activity associated with the event. The ActivityId 
                                parameter provides a way to group related events and is used in end-to-end tracing.  

Return Value:

     STATUS_SUCCESS or an appropriate NTSTATUS value.
    */
{

    if(ActivityId == NULL)
    {
        return STATUS_INVALID_PARAMETER;
    }

    return EtwActivityIdControl(EVENT_ACTIVITY_CTRL_CREATE_ID, ActivityId);
}

VOID
MbbWriteEvent (
    IN PCEVENT_DESCRIPTOR EventDescriptor,
    IN LPGUID ActivityId OPTIONAL,
    IN LPGUID RelatedActivityId OPTIONAL,
    IN USHORT ArgCount,
    ...
    )

/*++

Routine Description:

    Writes an ETW event.

Arguments:

    EventDescriptor - Supplies the event descriptor.

    ActivityId - Supplies the activity GUID to be associated with this event.

    ArgCount - Supplies the number of elements in the variable list.

    ... - Supplies the variable list of event data to write.

Return Value:

    None.

--*/

{
    va_list Arguments;

    va_start(Arguments, ArgCount);

    MbbWriteEventCommon(
        GlobalControl.TraceHandle,
        EventDescriptor,
        ActivityId,
        RelatedActivityId,
        ArgCount,
        Arguments
        );

    va_end(Arguments);

    return;
}

VOID
MbbWriteEventOpn (
    IN PCEVENT_DESCRIPTOR EventDescriptor,
    IN LPGUID ActivityId OPTIONAL,
    IN LPGUID RelatedActivityId OPTIONAL,
    IN USHORT ArgCount,
    ...
    )

/*++

Routine Description:

    Writes an ETW event for OPN.

Arguments:

    EventDescriptor - Supplies the event descriptor.

    ActivityId - Supplies the activity GUID to be associated with this event.

    ArgCount - Supplies the number of elements in the variable list.

    ... - Supplies the variable list of event data to write.

Return Value:

    None.

--*/

{
    va_list Arguments;

    va_start(Arguments, ArgCount);

    MbbWriteEventCommon(
        GlobalControl.TraceHandleOpn,
        EventDescriptor,
        ActivityId,
        RelatedActivityId,
        ArgCount,
        Arguments
        );

    va_end(Arguments);

    return;
}

VOID
MbbWriteEventCommon (
    IN REGHANDLE TraceHandle,
    IN PCEVENT_DESCRIPTOR EventDescriptor,
    IN LPGUID ActivityId OPTIONAL,
    IN LPGUID RelatedActivityId OPTIONAL,
    IN USHORT ArgCount,
    IN va_list Arguments
    )

/*++

Routine Description:

    Writes an ETW event.

Arguments:

    TraceHandle - Supplies the trace handle.

    EventDescriptor - Supplies the event descriptor.

    ActivityId - Supplies the activity GUID to be associated with this event.

    ArgCount - Supplies the number of elements in the variable list.

    Arguments - Supplies the variable list of event data to write.

Return Value:

    None.

--*/

{

    EVENT_DATA_DESCRIPTOR EventData[WMBCLASS_MAX_EVENT_DATA_DESCRIPTORS];
    PEVENT_DATA_DESCRIPTOR DataDescriptors;
    ULONG EventDataCount = 0;
    ULONG Length;
    PVOID Source;

    if (TraceHandle == 0)
    {
        return;
    }

    if (ArgCount > MAX_EVENT_DATA_DESCRIPTORS) {
        return;
    }

    if (ArgCount <= WMBCLASS_MAX_EVENT_DATA_DESCRIPTORS) {
        DataDescriptors = &EventData[0];

    } else {
#pragma prefast(suppress:__WARNING_MEMORY_LEAK, "By Design: Allocate new data descriptors, release when ETW write finishes.")
        DataDescriptors = ALLOCATE_NONPAGED_POOL(sizeof(EVENT_DATA_DESCRIPTOR) * ArgCount);

        if (!DataDescriptors) {
            return;
        }
    }

    //
    // Build up the event data descriptors.
    //

    while (EventDataCount < ArgCount) {
        Source = va_arg(Arguments, PVOID);
        Length = va_arg(Arguments, ULONG);

        //
        // EtwWrite expects that if Size > 0 then Ptr != NULL.
        // Passing a null pointer with Size > 0 causes it to enter a critical
        // region and then throw an exception without exiting the critical
        // region. This makes driver verifier bugcheck because of a mismatched
        // APC Disable count on the thread (even if the exception has been
        // catched).
        //
        NT_ASSERT(Length == 0 || Source != NULL);

        DataDescriptors[EventDataCount].Ptr = (ULONGLONG)Source;
        DataDescriptors[EventDataCount].Size = Length;

        EventDataCount++;
    }

    //
    // Write all the event data.
    //

    if (RelatedActivityId == NULL) {

        EtwWrite(TraceHandle,
                 EventDescriptor,
                 ActivityId,
                 EventDataCount,
                 DataDescriptors);
    } else {

        EtwWriteTransfer(TraceHandle,
                         EventDescriptor,
                         ActivityId,
                         RelatedActivityId,
                         EventDataCount,
                         DataDescriptors);
    }

    if (DataDescriptors != &EventData[0]) {

        NT_ASSERT(DataDescriptors != NULL);
        FREE_POOL(DataDescriptors);
    }

    return;
}

NDIS_STATUS
MbbUtilWwanToMbbSetUSSD(
    __out  PMBB_SET_USSD MbbSetUSSD,
    __in PWWAN_USSD_REQUEST WWanUSSDRequest
)
{    
    NTSTATUS ntStatus;
    //
    // Convert WWAN  -> MBB 
    //
    if(WWanUSSDRequest->RequestType == WwanUssdRequestInitiate){
        MbbSetUSSD->USSDAction = MbbUSSDInitiate;
    }
    else if (WWanUSSDRequest->RequestType == WwanUssdRequestContinue){
        MbbSetUSSD->USSDAction = MbbUSSDContinue;
    }
    else if(WWanUSSDRequest->RequestType == WwanUssdRequestCancel) {
        MbbSetUSSD->USSDAction = MbbUSSDCancel;
    }
    else {
        TraceError( WMBCLASS_OID, "[Util][USSD] INVALID USSD Requestt Type from client =%d", WWanUSSDRequest->RequestType );
        return NDIS_STATUS_INVALID_PARAMETER;       
    }
    
    MbbSetUSSD->USSDDataCodingScheme = WWanUSSDRequest->UssdString.DataCodingScheme;
    MbbSetUSSD->USSDPayload.Size = WWanUSSDRequest->UssdString.StringLength;//bytes
    MbbSetUSSD->USSDPayload.Offset = FIELD_OFFSET(MBB_SET_USSD, DataBuffer);

    RtlCopyMemory(MbbSetUSSD->DataBuffer, 
                  WWanUSSDRequest->UssdString.String, 
                  MbbSetUSSD->USSDPayload.Size);

    return NDIS_STATUS_SUCCESS;

}

NDIS_STATUS
MbbUtilMbbToWwanUSSDEvent(
    __in  PMBB_USSD             MbbUSSD,
    __out PWWAN_USSD_EVENT      WWanUSSDEvt
    )
{
    NDIS_STATUS NdisStatus = NDIS_STATUS_SUCCESS;

    if ( MbbUSSD->USSDResponse == MbbUSSDNoActionRequired){
        WWanUSSDEvt->EventType= WwanUssdEventNoActionRequired;
    }
    else if (MbbUSSD->USSDResponse == MbbUSSDActionRequired){
        WWanUSSDEvt->EventType = WwanUssdEventActionRequired;
    }
    else if (MbbUSSD->USSDResponse == MbbUSSDTerminated){
        WWanUSSDEvt->EventType = WwanUssdEventTerminated;
    }
    else if (MbbUSSD->USSDResponse == MbbUSSDOtherLocalClient){
        WWanUSSDEvt->EventType = WwanUssdEventOtherLocalClient;
    }
    else if (MbbUSSD->USSDResponse == MbbUSSDOperationNotSupported){
        WWanUSSDEvt->EventType = WwanUssdEventOperationNotSupported;
    }
    else if (MbbUSSD->USSDResponse == MbbUSSDNetworkTimeOut){
        WWanUSSDEvt->EventType = WwanUssdEventNetworkTimeOut;
    }
    else{
        //
        // Todo: no invalid status code in WWAN_USSD_RESPONSE, should probably define one.
        //
        //
         // Device/network is returning invalid data.
         //
         TraceError( WMBCLASS_OID, "[Util][USSD] INVALID USSD Response from device =%d", MbbUSSD->USSDResponse  );
         return NDIS_STATUS_INVALID_DATA;         
    }
    
    WWanUSSDEvt->UssdString.DataCodingScheme = (BYTE)MbbUSSD->USSDDataCodingScheme;
   
    if (MbbUSSD->USSDSessionState == MbbUSSDNewSession){
        WWanUSSDEvt->SessionState = WwanUssdSessionStateNew;
    }
    else if (MbbUSSD->USSDSessionState == MbbUSSDExistingSession){
        WWanUSSDEvt->SessionState = WwanUssdSessionStateExisting;
    }
    else
    {
        //
        // Todo: no invalid status code in WWAN_USSD_RESPONSE, should probably define one.
        //
        //
        // Device/network is returning invalid data.
        //
        TraceError( WMBCLASS_OID, "[Util][USSD] INVALID USSD Response.SessionState from device =%d", MbbUSSD->USSDSessionState  );
        return NDIS_STATUS_INVALID_DATA;
    }

    WWanUSSDEvt->UssdString.StringLength = 0;

    if (MbbUSSD->USSDPayload.Size > MBB_USSD_STRING_LEN_MAX)
    {
        //
        // Device/network is returning invalid data.
        //
        TraceError( WMBCLASS_OID, "[Util][USSD] INVALID USSD Response from device =%d", MbbUSSD->USSDPayload.Size );
        NdisStatus = NDIS_STATUS_INVALID_DATA;
        
    }
    else
    {
         WWanUSSDEvt->UssdString.StringLength = (BYTE)MbbUSSD->USSDPayload.Size;

         RtlCopyMemory(WWanUSSDEvt->UssdString.String,
                       MbbUSSD->DataBuffer,
                       MbbUSSD->USSDPayload.Size
                       );
    }
    
    return NdisStatus;
    
}


ULONG
MbbUtilWriteStringToBuffer(
    __in_ecount(BufferSize) PUCHAR      Start,
    __in ULONG       BufferSize,
    __in ULONG       CurrentOffset,
    __in PMBB_STRING StringDesc,
    __in_ecount(SourceLength) PCUCHAR      SourceString,
    __in ULONG       SourceLength
    )

{

    CurrentOffset= ROUND_UP_COUNT(CurrentOffset, ALIGN_DWORD);

    //
    // Only copy if the buffer is not overflown.
    //
    if( ((ULONGLONG)CurrentOffset) + SourceLength <= BufferSize )
    {
        StringDesc->Offset=0;
        StringDesc->Size=SourceLength;

        if (StringDesc->Size > 0)
        {
            //
            //  only set this if the size is non zero. otherwise offset and size are both zero.
            StringDesc->Offset=CurrentOffset;

            RtlCopyMemory(&Start[CurrentOffset], SourceString, SourceLength);

            CurrentOffset= ROUND_UP_COUNT(CurrentOffset+SourceLength, ALIGN_DWORD);

        }
    }
    else
    {
        ASSERT(((ULONGLONG)CurrentOffset) + SourceLength <= BufferSize);
        DbgBreakPoint();
    }
    return CurrentOffset;

}

BOOLEAN
MbbUtilIsNativeMbnService(
    __in GUID*       DeviceServiceGuid
    )
{
    ULONG i = 0;
    
    for (i = 0; i < ARRAYSIZE(NativeMbnServices); i++)
    {
        if (RtlCompareMemory(&NativeMbnServices[i], DeviceServiceGuid, sizeof(GUID)) 
                == sizeof(GUID))
        {
            // Matching ID
            return TRUE;
        }
    }

    return FALSE;
}

PMBB_DS
MbbUtilFindDeviceService(
    __in PMINIPORT_ADAPTER_CONTEXT Adapter,
    __in GUID*       DeviceServiceGuid
    )
{
    ULONG i = 0;
    
    for (i = 0; i < Adapter->DeviceServiceState.ServicesCount; i++)
    {
        if (RtlCompareMemory(&(Adapter->DeviceServiceState.ServicesList[i].DeviceServiceId), DeviceServiceGuid, sizeof(GUID)) 
                == sizeof(GUID))
        {
            // Matching ID
            return &(Adapter->DeviceServiceState.ServicesList[i]);
        }
    }

    return NULL;
}

_Requires_lock_not_held_(&Port->Lock)
NDIS_STATUS
MbbUtilAddNoMatchFilter(
    __in   PMINIPORT_ADAPTER_CONTEXT Adapter,
    __in   PMBB_PORT                 Port
    )
{
    NDIS_STATUS Status = NDIS_STATUS_SUCCESS;
    PUCHAR PatternBuffer = NULL;
    ULONG PatternSize = MBB_NO_MATCH_WOL_PATTERN_SIZE;
    ULONG TotalPatternLength = 0;
    ULONG MaskSize =  0;
    ULONG PatternNameSize = 0;
    RTL_BITMAP BitMap;
    PNDIS_PM_WOL_PATTERN NoMatchWolPattern =  NULL;
    NDIS_PM_COUNTED_STRING PatternFriendlyName = {0};
    NDIS_PORT_NUMBER PortNumber = NDIS_DEFAULT_PORT_NUMBER;

    struct _WOL_BITMAP_PATTERN *WoLBitMapPattern = NULL;

    if(!Port )
    {
        return NDIS_STATUS_INVALID_PARAMETER;
    }

    //
    // First, round up to an integral multiple of the number of bits in ULONG.
    // Then, determine the number of bytes required to store the bit-mask.
    //

    MaskSize = (((PatternSize + RTL_BITS_OF(ULONG) - 1) & ~(RTL_BITS_OF(ULONG) - 1))/ RTL_BITS_OF(UCHAR));

    TotalPatternLength = sizeof(NDIS_PM_WOL_PATTERN) + PatternSize +  MaskSize;

    PatternBuffer = ALLOCATE_NONPAGED_POOL(TotalPatternLength);

    if(PatternBuffer == NULL)
    {
        Status = NDIS_STATUS_RESOURCES;
        goto Cleanup;
    }

    NdisZeroMemory(PatternBuffer, TotalPatternLength);

    NoMatchWolPattern = (PNDIS_PM_WOL_PATTERN) PatternBuffer;    
    WoLBitMapPattern = &NoMatchWolPattern->WoLPattern.WoLBitMapPattern;

    NoMatchWolPattern->Header.Revision  = NDIS_PM_WOL_PATTERN_REVISION_2;
    NoMatchWolPattern->Header.Type      = NDIS_OBJECT_TYPE_DEFAULT;
    NoMatchWolPattern->Header.Size      = NDIS_SIZEOF_NDIS_PM_WOL_PATTERN_REVISION_2;
    NoMatchWolPattern->WoLPacketType    = NdisPMWoLPacketBitmapPattern;
    NoMatchWolPattern->Priority         = NDIS_PM_WOL_PRIORITY_NORMAL;
    NoMatchWolPattern->PatternId        = MBB_NO_MATCH_WOL_PATTERN_ID; // invalid pattern Id
    
    WoLBitMapPattern->MaskOffset        = sizeof(NDIS_PM_WOL_PATTERN);
    WoLBitMapPattern->MaskSize          = MaskSize;
    WoLBitMapPattern->PatternOffset     = sizeof(NDIS_PM_WOL_PATTERN) + MaskSize;
    WoLBitMapPattern->PatternSize       = PatternSize;

    RtlInitializeBitMap(
        &BitMap,
        (PULONG) (PatternBuffer + sizeof(NDIS_PM_WOL_PATTERN)),
        MaskSize * RTL_BITS_OF(UCHAR));

    //set the first bit of the mask
    RtlSetBits(&BitMap, 0, 1);

    MBB_ACQUIRE_PORT_LOCK(Port);
    PortNumber = Port->PortNumber;
    MBB_RELEASE_PORT_LOCK(Port);

    // Add the no-match filter
  
    Status = MbbUtilSetPowerFilterPattern(
                Adapter,
                PortNumber,
                NoMatchWolPattern->PatternId,
                PatternBuffer + NoMatchWolPattern->WoLPattern.WoLBitMapPattern.MaskOffset,
                NoMatchWolPattern->WoLPattern.WoLBitMapPattern.MaskSize,
                PatternBuffer + NoMatchWolPattern->WoLPattern.WoLBitMapPattern.PatternOffset,
                NoMatchWolPattern->WoLPattern.WoLBitMapPattern.PatternSize
                );

    if(Status == NDIS_STATUS_SUCCESS)
    {
        MBB_ACQUIRE_PORT_LOCK(Port);    
        Port->HasNoMatchWolPattern = TRUE;
        MBB_RELEASE_PORT_LOCK(Port);        
    }
  

Cleanup:

    if(PatternBuffer)
    {
        FREE_POOL(PatternBuffer);
        PatternBuffer = NULL;
    }

    if(Status == NDIS_STATUS_SUCCESS)
    {   
        TraceInfo( WMBCLASS_OID, "[Util] Successfully added a no-match filter to adapter powerfilter table for connected port %lu", PortNumber);
    }
    else
    {
        TraceError( WMBCLASS_OID, "[Util] Failed to add no-match filter to adapter powerfilter table for connected port %lu. Status = %!status!", PortNumber, Status);
    }
    
    return Status;    
}


_Requires_lock_not_held_(&Port->Lock)
NTSTATUS
MbbUtilRemoveNoMatchFilter(
    __in   PMINIPORT_ADAPTER_CONTEXT Adapter,
    __in   PMBB_PORT                 Port
    )
{
    // Clear all the no-match patterns for this port number, if any if this is a valid
    // pattern. Doing this will free up the slots in the adapter filter table
    // as it doesnt make sense to have a No-match WOL pattern and 
    // a valid WOL pattern set for a session Id.

    NTSTATUS Status = STATUS_SUCCESS;
    BOOLEAN   fHasNoMatchWolPattern =  FALSE;
    NDIS_PORT_NUMBER PortNumber = NDIS_DEFAULT_PORT_NUMBER;

    if(!Port )
    {
        Status = STATUS_INVALID_PARAMETER;
        goto Cleanup;
    }
    
    MBB_ACQUIRE_PORT_LOCK(Port);
    PortNumber = Port->PortNumber;
    fHasNoMatchWolPattern = Port->HasNoMatchWolPattern;
    MBB_RELEASE_PORT_LOCK(Port);

    if(fHasNoMatchWolPattern)
    {
        Status =  MbbUtilSetPowerFilterPattern(
                    Adapter,
                    PortNumber,
                    MBB_NO_MATCH_WOL_PATTERN_ID,
                    NULL,
                    0,
                    NULL,
                    0
                   );

        if(NT_SUCCESS(Status))
        {
            MBB_ACQUIRE_PORT_LOCK(Port);        
            Port->HasNoMatchWolPattern = FALSE;        
            MBB_RELEASE_PORT_LOCK(Port);

            TraceInfo( WMBCLASS_OID, "[Util] Successfully cleared the no-match WOL pattern for connected port %lu from the adapter power filter table", PortNumber);
        }
    }

    Cleanup:

        if(!NT_SUCCESS(Status))
        {
            TraceError( WMBCLASS_OID, "[Util] Failed to clear the no-match WOL pattern for connected port %lu from the adapter power filter table. Status = %!status!", PortNumber, Status);
        }
        
        return Status;
}

VOID
MbbUtilClearPowerFilterAtIndex(
     __in   PMINIPORT_ADAPTER_CONTEXT Adapter,
     __in   ULONG Index
    )
{
    Adapter->PowerFilterTable[Index].InUse=FALSE;
    Adapter->PowerFilterTable[Index].PortNumber = MBB_INVALID_PORT_NUMBER;
    Adapter->PowerFilterTable[Index].PatternId=0;

    if(Adapter->PowerFilterTable[Index].Mask != NULL)
    {
        FREE_POOL(Adapter->PowerFilterTable[Index].Mask);
        Adapter->PowerFilterTable[Index].Mask=NULL;
    }

    if(Adapter->PowerFilterTable[Index].Pattern !=NULL)
    {
        FREE_POOL(Adapter->PowerFilterTable[Index].Pattern);
        Adapter->PowerFilterTable[Index].Pattern=NULL; 
    }
}


NTSTATUS
MbbUtilSetPowerFilterPattern(
    __in                            PMINIPORT_ADAPTER_CONTEXT Adapter,
    __in                            NDIS_PORT_NUMBER          PortNumber,
    __in                            ULONG                     PatternId,
    __in_bcount_opt(MaskSize)       PCUCHAR                   Mask,
    __in                            ULONG                     MaskSize,
    __in_bcount_opt(PatternSize)    PCUCHAR                   Pattern,
    __in                            ULONG                     PatternSize
    )
{
    ULONG           i=0;
    ULONG           Index=0;
    ULONG           j=0;
    ULONG           k=0;
    ULONG           NcmPatternSize=0;
    NTSTATUS        Status=STATUS_SUCCESS;
    PUCHAR          NcmPattern=NULL;
    PUCHAR          NcmMask=NULL;
    UCHAR           LastMaskByte;

    if (MaskSize > 0xffff)
    {
        //
        //  NCM only has a ushort for mask count
        //
        Status= STATUS_INVALID_PARAMETER;
        goto Cleanup;
    }

    if (MaskSize == 0)
    {
        //
        //  this means the caller wants to clear and existing mask
        //

        for (i=0; i<Adapter->BusParams.PowerFiltersSupported; i++)
        {
            if (Adapter->PowerFilterTable[i].InUse 
                && (Adapter->PowerFilterTable[i].PatternId == PatternId)
                    && Adapter->PowerFilterTable[i].PortNumber == PortNumber) // same pattern may be plumbed for multiple port numbers
            {
                break;
            }
        }

        if (i < Adapter->BusParams.PowerFiltersSupported)
        {
            MbbUtilClearPowerFilterAtIndex(Adapter, i);
        }
        else
        {
            //
            //  mask not found
            //
            TraceError( WMBCLASS_OID, "[Util] power filter id not found =%d", PatternId );
            
            Status= STATUS_INVALID_PARAMETER;            
            
            ASSERT(0);
            
            goto Cleanup;
        }
    }
    else
    {
        // Get the next available slot in adapter power filter table
        for (i = 0; i < Adapter->BusParams.PowerFiltersSupported; i++)
        {
            if (!Adapter->PowerFilterTable[i].InUse)
            {                
                break;
            }
        }

        if (i == Adapter->BusParams.PowerFiltersSupported)
        {
            // this means all the slots in the adapter power filter table
            // are used and we cannot plumb any more wake patterns
            
            TraceError( WMBCLASS_OID, "[Util] max wake patterns already plumbed. Returning");
            
            Status= STATUS_INVALID_PARAMETER;           
            
            goto Cleanup;
        }


        NcmMask=ALLOCATE_NONPAGED_POOL(PatternSize);

        if (NcmMask == NULL)
        {
            TraceError( WMBCLASS_OID, "[Util] MbbUtilSetPowerFilterPattern failed to allocate memory for NcmMask");
            
            Status=STATUS_NO_MEMORY;
            
            goto Cleanup;
        }

        RtlZeroMemory(NcmMask, PatternSize);


        NcmPattern=ALLOCATE_NONPAGED_POOL(PatternSize);

        if (NcmPattern == NULL)
        {
            TraceError( WMBCLASS_OID, "[Util] MbbUtilSetPowerFilterPattern failed to allocate memory for NcmPattern");
            
            Status=STATUS_NO_MEMORY;
            
            goto Cleanup;
        }

        //
        //  translate the mask bit to mask bytes
        //
        for (k=0; k<MaskSize; k++)
        {
            //
            //  for each byte in the mask
            //
            for (j=0; j < sizeof(UCHAR)*8 ; j++)
            {
                ULONG   MaskIndex=k*sizeof(UCHAR)*8 + j;

                //
                //  for each bit in the NDIS mask, Set the whole byte in the MCN mask to 0 or 0xff
                //
                if ( MaskIndex < PatternSize-1)
                {
                    //
                    //  the index is not past the end of the supplied buffer.
                    //
                    NcmMask[MaskIndex] = 0x00;

                    if (Mask[k] & (1 << j))
                    {
                        //
                        //  the bit in the ndis mask is set, set all the bits to 1
                        //
                        NcmMask[MaskIndex] = 0xff;
                    }
                }
            }
        }


        //
        //  the pattern is unchanged from ndis
        //
        RtlCopyMemory(NcmPattern, Pattern, PatternSize);

        //
        //  mark this index as inuse
        //
        TraceInfo( WMBCLASS_OID, "[Util] power filter id %d added to table", PatternId );
     
        Adapter->PowerFilterTable[i].InUse = TRUE;
        Adapter->PowerFilterTable[i].PortNumber = PortNumber;
        Adapter->PowerFilterTable[i].PatternId = PatternId;
        Adapter->PowerFilterTable[i].MaskSize = PatternSize;

        Adapter->PowerFilterTable[i].Mask=NcmMask;
        NcmMask=NULL;

        Adapter->PowerFilterTable[i].Pattern=NcmPattern;
        NcmPattern=NULL;      
    }

Cleanup:

    if (NcmMask != NULL)
    {
        FREE_POOL(NcmMask);
        NcmMask=NULL;
    }

    if (NcmPattern != NULL)
    {
        FREE_POOL(NcmPattern);
        NcmPattern=NULL;
    }

    return Status;
}

NDIS_STATUS
MbbUtilWwanToMbbSetPacketFilter(
    __in  PMINIPORT_ADAPTER_CONTEXT Adapter,
    __in  BOOLEAN                   Set,
    __in  NDIS_PORT_NUMBER          PortNumber,
    __out PMBB_PACKET_FILTERS      *ReturnedMbbPacketFilters,
    __out ULONG                    *ReturnedBufferSize
    )
{
    PUCHAR                  Buffer;
    ULONG                   CurrentOffset;
    ULONG                   LocalOffset;
    ULONG                   BufferSize;
    PMBB_PACKET_FILTERS     MbbPacketFilters=NULL;
    PMBB_SINGLE_PACKET_FILTER MbbSinglePacketFilter=NULL;
    NDIS_STATUS             Status = NDIS_STATUS_SUCCESS;
    ULONG                   FiltersInUse=0;
    ULONG                   i=0;
    ULONG                   CurrentFilter=0;
    PMBB_PORT               Port = NULL;
    ULONG                   SessionId = MBB_INVALID_SESSION_ID;
   
    *ReturnedBufferSize=0;
    *ReturnedMbbPacketFilters=NULL;

    BufferSize= ROUND_UP_COUNT(FIELD_OFFSET(MBB_PACKET_FILTERS, ArrayElement), ALIGN_DWORD);

    do
    {
        // Check if the port number corresponds to an active session
        // Find the port corresponding to the port number
        Port = MbbWwanTranslatePortNumberToPort(Adapter, PortNumber);

        if(!Port)
        {
             TraceError( WMBCLASS_POWER, "MbbUtilWwanToMbbSetPacketFilter: Invalid port number %lu", PortNumber);
             Status = NDIS_STATUS_INVALID_PORT;
             break;
        }

        MBB_ACQUIRE_PORT_LOCK(Port);

        if( Port->ConnectionState.ConnectionUp)
        {
            SessionId = Port->SessionId;
            TraceInfo( WMBCLASS_POWER, "MbbUtilWwanToMbbSetPacketFilter: Read the port session id as %d",SessionId);            
        }
        else
        {   
            Status = NDIS_STATUS_INVALID_PARAMETER;
            TraceError( WMBCLASS_POWER, "MbbUtilWwanToMbbSetPacketFilter: Port %lu is not connected", PortNumber);
            MBB_RELEASE_PORT_LOCK(Port);
            break;            
        }
        
        MBB_RELEASE_PORT_LOCK(Port);

        if(SessionId == MBB_INVALID_SESSION_ID)
        {
            Status = NDIS_STATUS_INVALID_PARAMETER;
            TraceError( WMBCLASS_POWER, "MbbUtilWwanToMbbSetPacketFilter: Invalid session Id %d", SessionId);            
            break;   
        }
    
        if (Set)
        {
            for (i=0; i<Adapter->BusParams.PowerFiltersSupported; i++)
            {
                if (Adapter->PowerFilterTable[i].InUse 
                    && PortNumber == Adapter->PowerFilterTable[i].PortNumber)
                {
                    FiltersInUse++;

                    //
                    //  array elemet header
                    //
                    BufferSize += ROUND_UP_COUNT(sizeof(MBB_SINGLE_PACKET_FILTER), ALIGN_DWORD);

                    BufferSize += ROUND_UP_COUNT(FIELD_OFFSET(MBB_SINGLE_PACKET_FILTER,DataBuffer), ALIGN_DWORD);

                    BufferSize += ROUND_UP_COUNT(Adapter->PowerFilterTable[i].MaskSize, ALIGN_DWORD);
                    BufferSize += ROUND_UP_COUNT(Adapter->PowerFilterTable[i].MaskSize, ALIGN_DWORD);
                }                
            }            
        }


        MbbPacketFilters = ALLOCATE_NONPAGED_POOL( BufferSize );

        if ( MbbPacketFilters == NULL )
        {
            Status = NDIS_STATUS_RESOURCES;
            break;
        }

        Buffer=(PUCHAR)MbbPacketFilters;

        MbbPacketFilters->SessionId = SessionId;
        MbbPacketFilters->PacketFiltersCount=FiltersInUse;

        //
        //  start the current offset after the array elements
        //
        CurrentOffset = FIELD_OFFSET( MBB_PACKET_FILTERS, ArrayElement ) + (sizeof(MbbPacketFilters->ArrayElement[0]) * FiltersInUse);

        if (FiltersInUse > 0)
        {
            for (i=0; i<Adapter->BusParams.PowerFiltersSupported; i++)
            {
                if (Adapter->PowerFilterTable[i].InUse
                     && PortNumber == Adapter->PowerFilterTable[i].PortNumber)
                {
                    MbbPacketFilters->ArrayElement[CurrentFilter].Offset=CurrentOffset;

                    MbbSinglePacketFilter=(PMBB_SINGLE_PACKET_FILTER)&Buffer[CurrentOffset];

                    LocalOffset=FIELD_OFFSET( MBB_SINGLE_PACKET_FILTER, DataBuffer );


                    MbbSinglePacketFilter->FilterSize=Adapter->PowerFilterTable[i].MaskSize;
                    MbbSinglePacketFilter->PacketFilterOffset=LocalOffset;

                    RtlCopyMemory(&Buffer[CurrentOffset+LocalOffset],Adapter->PowerFilterTable[i].Pattern, Adapter->PowerFilterTable[i].MaskSize);

                    LocalOffset += ROUND_UP_COUNT(Adapter->PowerFilterTable[i].MaskSize, ALIGN_DWORD);

                    MbbSinglePacketFilter->PacketMaskOffset=LocalOffset;

                    RtlCopyMemory(&Buffer[CurrentOffset+LocalOffset],Adapter->PowerFilterTable[i].Mask, Adapter->PowerFilterTable[i].MaskSize);

                    LocalOffset += ROUND_UP_COUNT(Adapter->PowerFilterTable[i].MaskSize, ALIGN_DWORD);

                    CurrentOffset+=LocalOffset;

                    MbbPacketFilters->ArrayElement[CurrentFilter].Size=LocalOffset;

                    CurrentFilter++;

                }
            }
        }

        *ReturnedBufferSize=BufferSize;
        *ReturnedMbbPacketFilters=MbbPacketFilters;
    }while(FALSE);

    if(Status != NDIS_STATUS_SUCCESS)
    {
        if(MbbPacketFilters != NULL)
        {
            FREE_POOL(MbbPacketFilters);
            MbbPacketFilters = NULL;
        }
    }

    if(Port)
    {
	// Remove the reference added during find
        Dereference(Port);
    }
    
    return Status;
}

WWAN_STATUS
MbbUtilNdisToWwanStatus(
    __in NDIS_STATUS NdisStatus
    )
{
    switch( NdisStatus )
    {
        case NDIS_STATUS_SUCCESS:
            return WWAN_STATUS_SUCCESS;
        case NDIS_STATUS_PENDING:
            return WWAN_STATUS_SUCCESS;
        default:
            return WWAN_STATUS_FAILURE;
    }
}

VOID
MbbUtilNdisMiniportIndicateStatusEx(
    __in  PMINIPORT_ADAPTER_CONTEXT Adapter,    
    __in  PNDIS_STATUS_INDICATION   StatusIndication
    )
{
    MbbWriteEvent(
        &NDIS_STATUS_INDICATION_EVENT,
        NULL,
        NULL,
        5,
        &Adapter->TraceInstance,
        sizeof(Adapter->TraceInstance),
        &StatusIndication->StatusCode,
        sizeof(StatusIndication->StatusCode),
        &StatusIndication->RequestId,
        sizeof(StatusIndication->RequestId),
        &StatusIndication->DestinationHandle,
        sizeof(StatusIndication->DestinationHandle),
        &StatusIndication->StatusBufferSize,
        sizeof(StatusIndication->StatusBufferSize)
        );

    NdisMIndicateStatusEx(
        Adapter->MiniportAdapterHandle,
        StatusIndication
        );

    return;
}


NDIS_STATUS
MbbUtilWwanToMbbGetVisible(
    __in  PWWAN_GET_VISIBLE_PROVIDERS    WwanVisibleProviders,
    __out PMBB_GET_VISIBLE_PROVIDERS     MbbVisibleProviders
    )
{
    MbbVisibleProviders->Action = WwanVisibleProviders->Action;

    if( MbbVisibleProviders->Action >= MbbVisibleProvidersActionMax )
        return NDIS_STATUS_INVALID_DATA;
    else
        return NDIS_STATUS_SUCCESS;
}

NDIS_STATUS
MbbUtilWwanAssignMacAddress(
    _In_ PMINIPORT_ADAPTER_CONTEXT Adapter,
    _In_ NDIS_WWAN_MAC_ADDRESS macAddress    
    )
{
    NDIS_STATUS Status = NDIS_STATUS_FAILURE;
    BOOLEAN     found = FALSE;
    ULONG       addressId = 1;
    ULONG       i = 0;
    UCHAR       macAddress0;
    ULONG       maxActivatedContexts = 0;

    /*
        * Define the scheme that we select MAC address.
        * The very first MAC will just use the MAC address from the EEPROM.
        * For the rest we set the U/L bit (bit 1) in MAC address and iterate till
        * we find a unique MAC address.
        */
    MbbAdapterPortsLock(Adapter);
    maxActivatedContexts = Adapter->MaxActivatedContexts;
    MbbAdapterPortsUnlock(Adapter);
    
    for(addressId = 1; addressId < maxActivatedContexts; addressId++)
    {
        found = FALSE;
        
        macAddress0 = (UCHAR)(((addressId)<<2)|0x02);

        // check if this unique. Since we only altering the first byte of the
        // mac address we need to compare with only first byte of the mac
        // address of the ports

        MbbAdapterPortsLock(Adapter);

        for(i = 0; i < Adapter->NumberOfPorts; i++)
        {
            if(NdisEqualMemory(&(Adapter->PortList[i]->MACAddress[0]),&macAddress0,sizeof(UCHAR)))
            {
                found = TRUE;
                break;
            }
        }   

        MbbAdapterPortsUnlock(Adapter);

        if(!found)
        {
            macAddress[0] = macAddress0;
            Status = NDIS_STATUS_SUCCESS;
            break;
        }
    }


    return Status;
}

NDIS_STATUS 
MbbUtilProcessSetConnectState(
    __in PMBB_REQUEST_CONTEXT    Request,
    __in PWWAN_SET_CONTEXT_STATE SetContextState,
    __in ULONG                   SessionId
    )
{    
    ULONG                       MbbSetContextStateSize;
    ULONG                       AccessStringCchLength;
    ULONG                       UserNameCchLength;
    ULONG                       PasswordCchLength;
    SIZE_T                      StringCchLength;
    NDIS_STATUS                 NdisStatus;
    PMBB_CONTEXT_STATE          MbbContextState;
    PMBB_SET_CONTEXT_STATE      MbbSetContextState;

    do
    {   
        if(!SetContextState)
        {
            NdisStatus = NDIS_STATUS_FAILURE;
            TraceError( WMBCLASS_OID, "[MbbNdis][ReqID=0x%04x] MbbUtilProcessSetConnectState: SetContextState is NULL parameter",Request->RequestId);
            break;
        }

        //
        // Determine the length of the strings
        //
        NdisStatus = RtlStringCchLengthW(
                        SetContextState->AccessString,
                        WWAN_ACCESSSTRING_LEN,
                        &StringCchLength
                        );
        if( NT_ERROR( NdisStatus ) )
        {
            TraceError( WMBCLASS_OID, "[MbbNdis][ReqID=0x%04x] FAILED to get strlen for AccessString, Status=%!STATUS!",
                        Request->RequestId,
                        NdisStatus
                        );
            break;
        }
        AccessStringCchLength = (ULONG)StringCchLength;

        NdisStatus = RtlStringCchLengthW(
                        SetContextState->UserName,
                        WWAN_USERNAME_LEN,
                        &StringCchLength
                        );
        if( NT_ERROR( NdisStatus ) )
        {
            TraceError( WMBCLASS_OID, "[MbbNdis][ReqID=0x%04x] FAILED to get strlen for UserName, Status=%!STATUS!",
                        Request->RequestId,
                        NdisStatus
                        );
            break;
        }
        UserNameCchLength = (ULONG)StringCchLength;

        NdisStatus = RtlStringCchLengthW(
                        SetContextState->Password,
                        WWAN_PASSWORD_LEN,
                        &StringCchLength
                        );
        if( NT_ERROR( NdisStatus ) )
        {
            TraceError( WMBCLASS_OID, "[MbbNdis][ReqID=0x%04x] FAILED to get strlen for Password, Status=%!STATUS!",
                        Request->RequestId,
                        NdisStatus
                        );
            break;
        }
        PasswordCchLength = (ULONG)StringCchLength;

        MbbSetContextStateSize  = FIELD_OFFSET(MBB_SET_CONTEXT_STATE, DataBuffer);
        MbbSetContextStateSize  = ROUND_UP_COUNT( MbbSetContextStateSize, ALIGN_DWORD );
        MbbSetContextStateSize += MIN(AccessStringCchLength,MBB_MAXIMUM_ACCESS_STRING_LENGTH) * sizeof(WCHAR);
        MbbSetContextStateSize  = ROUND_UP_COUNT( MbbSetContextStateSize, ALIGN_DWORD );
        MbbSetContextStateSize += MIN(UserNameCchLength,MBB_MAXIMUM_USERNAME_LENGTH) * sizeof(WCHAR);
        MbbSetContextStateSize  = ROUND_UP_COUNT( MbbSetContextStateSize, ALIGN_DWORD );
        MbbSetContextStateSize += MIN(PasswordCchLength,MBB_MAXIMUM_PASSWORD_LENGTH) * sizeof(WCHAR);

        if( (MbbSetContextState = (PMBB_SET_CONTEXT_STATE) ALLOCATE_NONPAGED_POOL( MbbSetContextStateSize )) == NULL )
        {
            NdisStatus = NDIS_STATUS_RESOURCES;
            break;
        }
        Request->HandlerContext.DataToFreeOnCompletion = MbbSetContextState;      

        MbbUtilWwanToMbbSetContextState(
            SetContextState,
            AccessStringCchLength,
            UserNameCchLength,
            PasswordCchLength,
            MbbSetContextStateSize,
            SessionId,
            MbbSetContextState
            );

        NdisStatus = MbbUtilSetAttributeWithParameter(
                        Request,
                        (PCHAR) MbbSetContextState,
                        MbbSetContextStateSize
                        );
    }while(FALSE);
    
    return NdisStatus;
}


NDIS_STATUS
MbbUtilWwanCreateMac(
     __in PMBB_REQUEST_CONTEXT     Request
    )
{
    NDIS_STATUS                 Status = NDIS_STATUS_SUCCESS;
    PMINIPORT_ADAPTER_CONTEXT   Adapter= (PMINIPORT_ADAPTER_CONTEXT) MbbReqMgrGetAdapterHandle(Request);
    PNDIS_OID_REQUEST           OidRequest = Request->OidContext.OidRequest;
    PMBB_PORT                   NewPort = NULL;
    PNDIS_WWAN_MAC_INFO         NdisWwanMacInfo = NULL;
    
    do
    {
        OidRequest->DATA.METHOD_INFORMATION.BytesWritten = 0;
        OidRequest->DATA.METHOD_INFORMATION.BytesRead = 0;
        OidRequest->DATA.METHOD_INFORMATION.BytesNeeded = 0;        

        Status = MbbWwanAllocatePort(Adapter, TRUE, &NewPort);

        if (Status != NDIS_STATUS_SUCCESS)
        { 
            TraceError(WMBCLASS_OID,"Failed to allocate a new port\n");
            break;
        }

        // Add it to the adapter list
        MbbWwanAddPortToAdapterList(Adapter,&NewPort);

        // Done creating the new virtual adapter

        NdisWwanMacInfo = (PNDIS_WWAN_MAC_INFO)OidRequest->DATA.METHOD_INFORMATION.InformationBuffer;
        NdisWwanMacInfo->uNdisPortNumber = NewPort->PortNumber;      

        NdisMoveMemory(NdisWwanMacInfo->MacAddr, Adapter->MACAddress, sizeof(NDIS_WWAN_MAC_ADDRESS));

        // Adjust the mac address
        Status = MbbUtilWwanAssignMacAddress(Adapter, NdisWwanMacInfo->MacAddr);

        if(Status != NDIS_STATUS_SUCCESS)
        {
            TraceError(WMBCLASS_OID,"Failed to assign a MAC address to the port with port number %lu\n",NewPort->PortNumber);
            break;
        }

        //Copy the adjusted MAC address to the port
        NdisMoveMemory(NewPort->MACAddress, NdisWwanMacInfo->MacAddr, sizeof(NDIS_WWAN_MAC_ADDRESS));

        OidRequest->DATA.METHOD_INFORMATION.BytesWritten = sizeof(NDIS_WWAN_MAC_INFO);

        // Save the port context in the Request handlercontext so that we can access it 
        // in the completion routine

        // Add a reference to the port which will be removed in the completion routine
        Reference(NewPort);
        Request->HandlerContext.Parameters.NdisPortContext.Port = NewPort;

        TraceInfo(WMBCLASS_OID,"Created a new Port \n");        
    } while (FALSE);
    
    return Status;
}

NDIS_STATUS
MbbUtilWwanCreateMacComplete(
    _In_ PMINIPORT_ADAPTER_CONTEXT Adapter,
    _In_ PMBB_PORT                 Port,
    _In_ NDIS_STATUS               NdisStatus
    )
{
    NDIS_STATUS         Status = NdisStatus;    
    NDIS_PORT_NUMBER    PortNumber;

    if(Port != NULL)
    {  
        if (Status == NDIS_STATUS_SUCCESS)
        { 
            //
            // We can now activate the port. We cannot do this while
            // we are processing the OID as the OS would deadlock. 
            //
        

            // If this port has been allocated with NDIS, activate it. This notification
            // goes upto the OS and it would handle the request           
            Status = MbbWwanActivatePort(Adapter, Port);            
            ASSERT(Status == NDIS_STATUS_SUCCESS);
        }        
        else
        {
            MBB_ACQUIRE_PORT_LOCK(Port);
            PortNumber = Port->PortNumber;
            MBB_RELEASE_PORT_LOCK(Port);
            
            if (IS_ALLOCATED_PORT_NUMBER(PortNumber))
            {
                // If port is not getting activated, remove it from the adapter port list
                // and deallocate it so that it can get freed
                
                if (MbbWwanRemovePortFromAdapterList(Adapter, Port))
                {
                    MbbWwanDeallocatePort(Port);
                }
                else
                {
                    TraceInfo(WMBCLASS_INIT, "%!FUNC!: Port(0x%p) with port number %lu was already removed", Port, Port->PortNumber);
                }
             }
        }
    }

    return Status;
}

NDIS_STATUS
MbbUtilWwanDeleteMac(
   __in PMBB_REQUEST_CONTEXT     Request
    )
{  
    NDIS_STATUS                 Status = NDIS_STATUS_SUCCESS;
    PMINIPORT_ADAPTER_CONTEXT   Adapter= (PMINIPORT_ADAPTER_CONTEXT) MbbReqMgrGetAdapterHandle(Request);
    PNDIS_OID_REQUEST           OidRequest = Request->OidContext.OidRequest;
    PNDIS_WWAN_MAC_INFO         NdisWwanMacInfo = NULL;
    PMBB_PORT                   DestinationPort = NULL;
    NDIS_PORT_NUMBER            PortNumber;
    
    do
    {
        NdisWwanMacInfo = (PNDIS_WWAN_MAC_INFO)OidRequest->DATA.SET_INFORMATION.InformationBuffer;
        PortNumber = NdisWwanMacInfo->uNdisPortNumber;

        //
        // First we would need to translate from the NDIS_PORT_NUMBER
        // to our port structure. This is done by walking the PortList
        // Since OID calls are serialized, we do not expect the Portlist to change
        // while we are trying to find the port or for the port to get deleted
        // until this OID is completed. So we do not need to protect the Port/PortList
        // in any way
        //

        DestinationPort = MbbWwanTranslatePortNumberToPort(
                           Adapter, 
                           PortNumber
                           );
        
        if (DestinationPort == NULL)
        {
           TraceError(WMBCLASS_OID, "Unable to find Port corresponding to PortNumber %d\n", NdisWwanMacInfo->uNdisPortNumber);
           Status = NDIS_STATUS_INVALID_PORT;
           break;
        }
        
        if (!IS_ALLOCATED_PORT_NUMBER(PortNumber))
        {
           // This port has been allocate with NDIS. When we are done, delete the
           // port
           TraceError(WMBCLASS_OID, "Invalid port number specified %lu\n", NdisWwanMacInfo->uNdisPortNumber);
           Status = NDIS_STATUS_INVALID_PORT;
           break;
        }         

        // remove it from the adapter list
        if (MbbWwanRemovePortFromAdapterList(Adapter, DestinationPort))
        {

            //Done deleting the mac state
            OidRequest->DATA.SET_INFORMATION.BytesRead = sizeof(NDIS_WWAN_MAC_INFO);

            // Save the port number in the Request handlercontext so that we can access it 
            // in the completion routine

            // Add a reference to the port which will be removed in the completion routine        
            Reference(DestinationPort);
            Request->HandlerContext.Parameters.NdisPortContext.Port = DestinationPort;

            TraceInfo(WMBCLASS_OID, "Removed the port %lu from adapter", NdisWwanMacInfo->uNdisPortNumber);
        }
        else
        {
            Status = NDIS_STATUS_INVALID_PORT_STATE;
            TraceInfo(WMBCLASS_INIT, "%!FUNC!: Port(0x%p) was already removed", DestinationPort);
        }
    } while (FALSE);


    // Remove the reference which got added in converting port number to port
    if(DestinationPort != NULL)
    {
        Dereference(DestinationPort);
    }

    return Status;
}

NDIS_STATUS
MbbUtilWwanDeleteMacComplete(
    _In_ PMINIPORT_ADAPTER_CONTEXT Adapter,
    _In_ PMBB_PORT                 Port,
    _In_ NDIS_STATUS               NdisStatus
    )
{
    NDIS_STATUS         Status = NdisStatus;    
    NDIS_PORT_NUMBER    PortNumber;

    if(Port != NULL)
    {  
        if (Status == NDIS_STATUS_SUCCESS)
        { 
            // Check if the port has an activated context associated with it.            
            Status = MbbUtilDisconnectPortOnDeleteMac(Adapter,Port);
            if(Status != NDIS_STATUS_SUCCESS)
            {
 
                TraceError( WMBCLASS_OID, "[MbbAdapter]MbbUtilDisconnectPortOnDeleteMac FAILED to deactivate context. Status=%!status!",Status);
            }
            
            //
            // We can now deactivate the port. We cannot do this while we are processing
            // the OID, else the OS would deadlock. 
            //
            Status = MbbWwanDeactivatePort(Adapter, Port);            
            ASSERT(Status == NDIS_STATUS_SUCCESS);

            // Deallocate the port that the NDIS port can be freed
            // and the port can be cleaned up.

            MbbWwanDeallocatePort(Port);
        }
    }

    return Status;
}

NDIS_STATUS
MbbUtilDisconnectPortOnDeleteMac(
    _In_ PMINIPORT_ADAPTER_CONTEXT Adapter,
    _In_ PMBB_PORT Port
    )    
{
    ULONG                   SessionId = MBB_INVALID_SESSION_ID;
    ULONG                   MaxActivatedContexts = 0;
    BOOLEAN                 DeactivateContext = FALSE;
    PMBB_REQUEST_CONTEXT    Request = NULL;
    PMBB_REQUEST_MANAGER    RequestManager = NULL;
    LARGE_INTEGER           Timeout;
    NDIS_STATUS             NdisStatus = NDIS_STATUS_SUCCESS;    
    
    SessionId = MbbWwanGetPortSessionId(Port);

    MbbAdapterPortsLock(Adapter);    
    MaxActivatedContexts = Adapter->MaxActivatedContexts;    
    MbbAdapterPortsUnlock(Adapter);

    // No need of taking port lock as port is already
    // removed from adapter port list
    
    if(SessionId <= MBB_DEFAULT_SESSION_ID 
        || SessionId >= MaxActivatedContexts)
    {
        // Return success here as this will be the case when 
        // deactivate context is done before port is torn down
        return NDIS_STATUS_SUCCESS;
    }
    
    MbbAdapterSessionIdPortTableLock(Adapter);

    if(Adapter->SessionIdPortTable[SessionId].InUse
        && Adapter->SessionIdPortTable[SessionId].PortNumber == Port->PortNumber)
    {
        //means port has an active context associated with it
        DeactivateContext = TRUE;            
    }    

    MbbAdapterSessionIdPortTableUnlock(Adapter);

    if(DeactivateContext)
    {
        MBB_COMMAND Command = { MBB_UUID_BASIC_CONNECT_CONSTANT, MBB_BASIC_CID_CONNECT };

        do
        {
            if( (RequestManager = MbbAdapterGetRequestManager( Adapter )) == NULL )
            {
                TraceError( WMBCLASS_OID, "[MbbAdapter] FAILED to reference RequestManager for deactivate context" );
                NdisStatus = NDIS_STATUS_ADAPTER_NOT_READY;
                break;
            }
        
            if( (Request = MbbReqMgrCreateRequest(
                                RequestManager,
                                NULL,
                                0,
                                &NdisStatus)) == NULL )
            {
                TraceError( WMBCLASS_OID, "[MbbAdapter] FAILED to allocate RequestContext for deactivate context" );
                break;
            }
        
            Request->OidHandler = MbbNdisGetOidHandlerByCommand( &Command );

            Request->HandlerContext.Parameters.ContextSessionId.SessionId = SessionId;
          
            NdisStatus = MbbReqMgrDispatchRequest(
                            Request,
                            TRUE, // Serialized
                            MbbUtilDeactivateContext,
                            MbbUtilInternalCIDCompletion,
                            MbbUtilInternalCIDResponse
                            );
            
            if( NdisStatus != NDIS_STATUS_PENDING )
            {
                MbbReqMgrDestroyRequest(
                        Request->RequestManager,
                        Request
                        );                
            }
        }while(FALSE);
    }  

    if( RequestManager != NULL )
    {
        MbbReqMgrDeref( RequestManager );
    } 

    return NdisStatus;
}

NDIS_STATUS
MbbUtilDeactivateContext(
    __in MBB_PROTOCOL_HANDLE    AdapterHandle,
    __in PMBB_REQUEST_CONTEXT   Request
    )
{
    NDIS_STATUS      NdisStatus = NDIS_STATUS_SUCCESS;
    MBB_COMMAND      Command = { MBB_UUID_BASIC_CONNECT_CONSTANT, MBB_BASIC_CID_CONNECT };
    PWWAN_SET_CONTEXT_STATE  WwanSetContextState = ALLOCATE_NONPAGED_POOL(sizeof(WWAN_SET_CONTEXT_STATE));

    if(!WwanSetContextState)
    {
        TraceError( WMBCLASS_OID, "[MbbAdapter] MbbUtilDeactivateContext: FAILED to allocate WWAN_SET_CONTEXT_STATE" );
        return NDIS_STATUS_RESOURCES;
    }

    NdisZeroMemory(WwanSetContextState, sizeof(*WwanSetContextState));

    WwanSetContextState->ActivationCommand = WwanActivationCommandDeactivate;

    NdisStatus = MbbUtilProcessSetConnectState(
                      Request,
                      WwanSetContextState,
                      Request->HandlerContext.Parameters.ContextSessionId.SessionId);

    if(WwanSetContextState)
    {
        FREE_POOL(WwanSetContextState);
        WwanSetContextState = NULL;
    }

    return NdisStatus;    
}

