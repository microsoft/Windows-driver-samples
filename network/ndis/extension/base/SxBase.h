/*++

Copyright (c) Microsoft Corporation. All Rights Reserved.

Module Name:

    SxBase.c

Abstract:

    This file contains the common code for building a switch extension. This
    file includes all the standard NDIS handling and exposes a function based
    interface for the control path and data path. This also has reusable code
    for basic operations such as pause/resume handling etc.


--*/

extern NDIS_STRING SxExtensionFriendlyName;
extern NDIS_STRING SxExtensionGuid;

typedef enum _SX_SWITCH_DATAFLOW_STATE
{
    SxSwitchPaused,
    SxSwitchRunning
} SX_SWITCH_DATAFLOW_STATE, *PSX_SWITCH_DATAFLOW_STATE;

typedef enum _SX_SWITCH_CONTROFLOW_STATE
{
    SxSwitchUnknown,
    SxSwitchAttached,
    SxSwitchDetached
} SX_SWITCH_CONTROLFLOW_STATE, *PSX_SWITCH_CONTROLFLOW_STATE;

typedef struct _SX_SWITCH_OBJECT
{
    //
    // The Link *must* always be the first field.
    //
    LIST_ENTRY Link;

    //
    // The extension context is the context used in the specific logic
    // of this extension.
    // This is allocated and returned in SxExtSwitchCreate
    //
    PNDIS_HANDLE ExtensionContext;

    //
    // Ndis related fields.
    //
    NDIS_HANDLE NdisFilterHandle;
    NDIS_SWITCH_CONTEXT NdisSwitchContext;
    NDIS_SWITCH_OPTIONAL_HANDLERS NdisSwitchHandlers;

    //
    // Switch state.
    //
    SX_SWITCH_DATAFLOW_STATE DataFlowState;
    SX_SWITCH_CONTROLFLOW_STATE ControlFlowState;
    
    //
    // Management fields.
    //
    volatile LONG PendingInjectedNblCount;
    volatile LONG PendingOidCount;
    
    //
    // Control Path Management.
    //
    PNDIS_SWITCH_NIC_OID_REQUEST OldNicRequest;
    
} SX_SWITCH_OBJECT, *PSX_SWITCH_OBJECT;

typedef struct _SX_OID_REQUEST
{
    NDIS_OID_REQUEST NdisOidRequest;
    NDIS_EVENT ReqEvent;
    NDIS_STATUS Status;
    ULONG BytesNeeded;

} SX_OID_REQUEST, *PSX_OID_REQUEST;

typedef struct _FILTER_DEVICE_EXTENSION
{
    ULONG            Signature;
    NDIS_HANDLE      Handle;
} FILTER_DEVICE_EXTENSION, *PFILTER_DEVICE_EXTENSION;


//
// Function prototypes
//
DRIVER_INITIALIZE DriverEntry;

DRIVER_UNLOAD SxNdisUnload;

FILTER_SET_OPTIONS SxNdisSetOptions;

FILTER_SET_MODULE_OPTIONS SxNdisSetFilterModuleOptions;

FILTER_ATTACH SxNdisAttach;

FILTER_DETACH SxNdisDetach;

FILTER_PAUSE SxNdisPause;

FILTER_RESTART SxNdisRestart;

FILTER_OID_REQUEST SxNdisOidRequest;

FILTER_CANCEL_OID_REQUEST SxNdisCancelOidRequest;

FILTER_OID_REQUEST_COMPLETE SxNdisOidRequestComplete;

FILTER_SEND_NET_BUFFER_LISTS SxNdisSendNetBufferLists;

FILTER_RETURN_NET_BUFFER_LISTS SxNdisReturnNetBufferLists;

FILTER_SEND_NET_BUFFER_LISTS_COMPLETE SxNdisSendNetBufferListsComplete;

FILTER_RECEIVE_NET_BUFFER_LISTS SxNdisReceiveNetBufferLists;

FILTER_CANCEL_SEND_NET_BUFFER_LISTS SxNdisCancelSendNetBufferLists;

FILTER_STATUS SxNdisStatus;

FILTER_NET_PNP_EVENT SxNdisNetPnPEvent;

VOID
SxpNdisCompleteInternalOidRequest(
    _In_ PSX_SWITCH_OBJECT Switch,
    _In_ PNDIS_OID_REQUEST NdisRequest,
    _In_ NDIS_STATUS Status
    );


//
// Some debug stuff.
//
#define DL_EXTRA_LOUD       20
#define DL_VERY_LOUD        10
#define DL_LOUD             8
#define DL_INFO             6
#define DL_TRACE            5
#define DL_WARN             4
#define DL_ERROR            2
#define DL_FATAL            0

#if DBG

extern ULONG SxDebugLevel;

#define DEBUGP(lev, stmt)                                               \
        {                                                               \
            if ((lev) <= SxDebugLevel)                                  \
            {                                                           \
                DbgPrint("%S: ",SxExtServiceName); DbgPrint stmt;       \
            }                                                           \
        }

#else

#define DEBUGP(lev, stmt)

#endif


