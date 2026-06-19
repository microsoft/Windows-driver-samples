// Copyright (C) Microsoft Corporation. All rights reserved.
//
// Emulated Network Link (ENL) definitions
//

//
// An ENL connects two virtual network adapters directly. Packets sent over
// one adapter are delivered to the other adapter and vice versa.
//
// An ENL is created with a processor index. This processor is used to emulate
// the NIC hardware for the adapters connected by the ENL.
//
// Caller can send packets as NBLs over a given port, and also receive
// incoming packets placed into NBLs over a given port.
//
// ENL indicates tx and rx NBL completions over the target processor(s)
// determined by the RSS indirection table and the hash value for each NBL.
// The current version of the ENL requires a symmetric Toeplitz hash key
// to be used by the system globally so that both directions of a given
// 4-tuple (or 2-tuple) yield the same hash value. ENL currently indicates
// NBL completions by queueing DPCs to the target processor.
//


#ifndef _KERNEL_MODE
#define ASSERT(x) NT_ASSERT(x)

#endif
#include "rtl/KWaitEvent.h"
#include "rtl/KPushLock.h"

#define ENL_MAX_PROC_COUNT 16
#define ENLP_PORT_COUNT 2

#define TX TRUE
#define RX FALSE

enum ENL_QUEUE_STATE {
    Stopped,
    Started
    };

typedef
VOID
(ENL_INTERRUPT_ROUTINE) (
    _Inout_ PVOID PortContext,
    _In_ bool TxRx
    );

struct ENLP_LINK;
struct DECLSPEC_ALIGN(PAGE_SIZE) ENLP_PORT;
struct ENLP_QUEUE;

typedef NTSTATUS(EVT_ENLP_PDO_WAKE_SIGNAL)(_In_ void* Context);

_IRQL_requires_max_(PASSIVE_LEVEL)
ENLP_QUEUE *
EnlCreateQueue(
    _In_ NETPACKETQUEUE Queue,
    _In_ BOOLEAN Tx
    );

_IRQL_requires_max_(PASSIVE_LEVEL)
VOID
EnlDestroyQueue(
    _In_ ENLP_QUEUE * QueueContext,
    _In_ BOOLEAN Tx
    );

_IRQL_requires_max_(PASSIVE_LEVEL)
VOID
EnlRingDoorBell(
    _In_ ENLP_QUEUE * QueueContext,
    _In_ ULONG EndIndex
    );

_IRQL_requires_max_(PASSIVE_LEVEL)
VOID
EnlArmInterrupt(
    _In_ ENLP_QUEUE * QueueContext,
    _In_ BOOLEAN NotificationEnabled
    );

_IRQL_requires_max_(PASSIVE_LEVEL)
void
EnlSetPdoWakeSignalCallback(
    _In_ ENLP_LINK * EnlLinkHandle,
    _In_ EVT_ENLP_PDO_WAKE_SIGNAL* evtPdoWakeSignal,
    _In_ void* Context
    );

_IRQL_requires_max_(PASSIVE_LEVEL)
void
EnlArmWake(
    _In_ ENLP_LINK * EnlLinkHandle
    );

_IRQL_requires_max_(PASSIVE_LEVEL)
void
EnlDisarmWake(
    _In_ ENLP_LINK * EnlLinkHandle
    );

_IRQL_requires_max_(PASSIVE_LEVEL)
size_t
EnlCopyWakeFrame(
    _In_ ENLP_LINK * EnlLinkHandle,
    _Out_writes_bytes_(BufferSize) unsigned char * Buffer,
    _In_ size_t BufferSize
    );

_IRQL_requires_max_(PASSIVE_LEVEL)
BOOLEAN
EnlIsLinkActive(
    _In_ ENLP_LINK * EnlLinkHandle
    );

_IRQL_requires_max_(PASSIVE_LEVEL)
VOID
EnlIndicateQueueState(
    _In_ ENLP_QUEUE * EnlLinkHandle,
    _In_ ENL_QUEUE_STATE State
    );

///////////////////////////////////////////////////////////////////////////////
//  Multi link wrapper APIs                                                  //
///////////////////////////////////////////////////////////////////////////////

#define ENL_MLINK_MAX 4

typedef struct
{
    _Field_range_(1, ENL_MLINK_MAX) ULONG LinkCount;
    _Field_size_(LinkCount) ENLP_LINK * LinkHandle[ENL_MLINK_MAX];
} ENL_MLINK;

_IRQL_requires_max_(PASSIVE_LEVEL)
NTSTATUS
EnlMCreateLink(
    _In_range_(1, ENL_MLINK_MAX)ULONG LinkCount,
    _In_reads_(LinkCount) ULONG ProcessorIndex,
    _In_ BOOLEAN Poll,
    _Out_ ENL_MLINK* EnlMLink
    );

_IRQL_requires_max_(PASSIVE_LEVEL)
BOOLEAN
EnlMIsPortActive(
    _In_ CONST ENL_MLINK* EnlMLink,
    _In_ ULONG PortIndex
    );

_IRQL_requires_max_(PASSIVE_LEVEL)
NTSTATUS
EnlMActivateLinkPort(
    _In_ CONST ENL_MLINK* EnlMLink,
    _In_ ULONG PortIndex,
    _In_ ENL_INTERRUPT_ROUTINE Interrupt,
    _In_ PVOID PortContext
    );

_IRQL_requires_max_(PASSIVE_LEVEL)
void
EnlMDeactivateLinkPort(
    _In_ CONST ENL_MLINK* EnlMLink,
    _In_ ULONG PortIndex
    );

_IRQL_requires_max_(PASSIVE_LEVEL)
VOID
EnlMCloseLink(
    _Inout_ ENL_MLINK* EnlMLink
    );

#define ENL_MAXIMUM_WAKE_FRAME_SIZE 1514

typedef
_IRQL_requires_max_(PASSIVE_LEVEL)
VOID
(ENLP_ITERATION_ROUTINE) (
    _In_ PVOID IterationContext
    );

struct ENLP_THREAD_STATE
{
    BOOLEAN PauseRequested{};
    BOOLEAN StopRequested{};
    ENLP_ITERATION_ROUTINE* IterationRoutine{};
    PVOID IterationContext{};
    KAutoEvent *ArmWaitEvent{};
    KAutoEvent ResumeEvent{};
    KAutoEvent PausingEvent{};
    ULONG ProcIndex{};
    ULONG IdealNode{};
    unique_thread Thread{};
};

class NetvQueue;
class NetvRxQueue;
class NetvTxQueue;

struct ENLP_QUEUE
{
    BOOLEAN TxRx{};
    BOOLEAN Notify{};
    ULONG QueueEnd{};
    ULONG QueueNext{};

    ENLP_PORT * EnlPortHandle{};

    BOOLEAN Armed{};
    WDFSPINLOCK Spinlock{};
    KAutoEvent *ArmWaitEvent{};

    ENL_QUEUE_STATE State{};

    NetvQueue * Queue{nullptr};
    union {
        NetvRxQueue * RxQueue;
        NetvTxQueue * TxQueue;
    };

};

struct DECLSPEC_ALIGN(PAGE_SIZE) ENLP_PORT
{
    ENL_INTERRUPT_ROUTINE* Interrupt;
    PVOID PortContext;
    ULONG TxQueueCount;
    ULONG RxQueueCount;

    DECLSPEC_CACHEALIGN
        ENLP_QUEUE TxQueue[ENL_MAX_PROC_COUNT];

    DECLSPEC_CACHEALIGN
        ENLP_QUEUE RxQueue[ENL_MAX_PROC_COUNT];
};

struct ENLP_LINK
{
    KPushLock Lock{};
    ULONG64 Ts{};
    ULONG64 BusyTicks{};
    ULONG64 EmptyTicks{};
    BOOLEAN Poll{};
    BOOLEAN InitializePacketLayout{};

    //
    // Power related features. If PowerInterface is NULL none of the other
    // fields have meaning
    //
    EVT_ENLP_PDO_WAKE_SIGNAL* EvtWakeSignal;
    void* WakeSignalContext;
    BOOLEAN ArmedForWake{};
    unsigned char WakeFrame[ENL_MAXIMUM_WAKE_FRAME_SIZE];
    size_t WakeFrameSize{};

    KAutoEvent ArmWaitEvent{};
    ENLP_THREAD_STATE EnlThread{};
    ENLP_PORT Ports[ENLP_PORT_COUNT];
};
#define MAX_ADAPTER_COUNT 2
extern ENL_MLINK NetvEnlMLink[MAX_ADAPTER_COUNT / 2];
C_ASSERT((ENLP_PORT_COUNT & 0x1) == 0);
