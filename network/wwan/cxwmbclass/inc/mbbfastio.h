//This header file depends on <ndis.h> and defines NDIS_SUPPORT_NDIS670 or higher

#pragma once
#if (NTDDI_VERSION >= NTDDI_WIN10_RS2 || NDIS_SUPPORT_NDIS670)

typedef
_IRQL_requires_max_(DISPATCH_LEVEL)
_Function_class_(FASTIO_SEND_NET_BUFFER_LISTS_COMPLETE)
VOID
FASTIO_SEND_NET_BUFFER_LISTS_COMPLETE(
    _In_ PVOID                    MiniportAdapterContext,
    _In_ PNET_BUFFER_LIST         NetBufferList,
    _In_ ULONG                    SendCompleteFlags
    );
typedef FASTIO_SEND_NET_BUFFER_LISTS_COMPLETE (*FastIOSendNetBufferListsCompleteHandler);
typedef
_IRQL_requires_max_(DISPATCH_LEVEL)
_Function_class_(FASTIO_INDICATE_RECEIVE_NET_BUFFER_LISTS)
VOID
FASTIO_INDICATE_RECEIVE_NET_BUFFER_LISTS(
    _In_ PVOID                    MiniportAdapterContext,
    _In_ PNET_BUFFER_LIST         NetBufferList,
    _In_ ULONG                    SessionId,
    _In_ ULONG                    NumberOfNetBufferLists,
    _In_ ULONG                    ReceiveFlags
    );
typedef FASTIO_INDICATE_RECEIVE_NET_BUFFER_LISTS (*FastIOIndicateReceiveNetBufferListsHandler);
typedef
_IRQL_requires_max_(DISPATCH_LEVEL)
_Function_class_(FASTIO_SEND_NET_BUFFER_LISTS)
VOID
FASTIO_SEND_NET_BUFFER_LISTS(
    _In_ PVOID                    ModemContext,
    _In_ PNET_BUFFER_LIST         NetBufferList,
    _In_ ULONG                    SessionId,
    _In_ ULONG                    SendFlags
    );
typedef FASTIO_SEND_NET_BUFFER_LISTS (*FastIOSendNetBufferListsHandler);
typedef
_IRQL_requires_max_(DISPATCH_LEVEL)
_Function_class_(FASTIO_RETURN_NET_BUFFER_LISTS)
VOID
FASTIO_RETURN_NET_BUFFER_LISTS(
    _In_ PVOID                    ModemContext,
    _In_ PNET_BUFFER_LIST         NetBufferList,
    _In_ ULONG                    ReturnFlags
    );
typedef FASTIO_RETURN_NET_BUFFER_LISTS (*FastIOReturnNetBufferListsHandler);
typedef
_IRQL_requires_max_(DISPATCH_LEVEL)
_Function_class_(FASTIO_CANCEL_SEND)
VOID
FASTIO_CANCEL_SEND(
    _In_ PVOID                    ModemContext,
    _In_ PVOID                    CancelId
    );
typedef FASTIO_CANCEL_SEND (*FastIOCancelSendHandler);
typedef
_IRQL_requires_(PASSIVE_LEVEL)
_Function_class_(FASTIO_HALT)
VOID
FASTIO_HALT(
    _In_ PVOID                    ModemContext,
    _In_ NDIS_HALT_ACTION         HaltAction
    );
typedef FASTIO_HALT (*FastIOHaltHandler);
typedef
_IRQL_requires_(PASSIVE_LEVEL)
_Function_class_(FASTIO_PAUSE)
NDIS_STATUS
FASTIO_PAUSE(
    _In_ PVOID                    ModemContext
    );
typedef FASTIO_PAUSE (*FastIOPauseHandler);
typedef
_When_(ShutdownAction==NdisShutdownPowerOff, _IRQL_requires_(PASSIVE_LEVEL))
_When_(ShutdownAction==NdisShutdownBugCheck, _IRQL_requires_(HIGH_LEVEL))
_Function_class_(FASTIO_SHUTDOWN)
VOID
FASTIO_SHUTDOWN(
    _In_ PVOID                    ModemContext,
    _In_ NDIS_SHUTDOWN_ACTION     ShutdownAction
    );
typedef FASTIO_SHUTDOWN (*FastIOShutdownHandler);
typedef
_IRQL_requires_max_(DISPATCH_LEVEL)
_Function_class_(FASTIO_RESET)
NDIS_STATUS
FASTIO_RESET(
    _In_ PVOID                    ModemContext
    );
typedef FASTIO_RESET (*FastIOResetHandler);
typedef
_IRQL_requires_(PASSIVE_LEVEL)
_Function_class_(FASTIO_RESTART)
NDIS_STATUS
FASTIO_RESTART(
    _In_ PVOID                    ModemContext
    );
typedef FASTIO_RESTART (*FastIORestartHandler);

#endif // (NTDDI_VERSION >= NTDDI_WIN10_RS2 || NDIS_SUPPORT_NDIS670)
