// Copyright (C) Microsoft Corporation. All rights reserved.
#include "pch.hpp"
#include <stdlib.h>
#include "netvadapter.h"
#include "rxqueue.h"
#include "txqueue.h"
#include "trace.h"
#include "memory.h"
#include "rtl\KLockHolder.h"
#include "enl.tmh"

///////////////////////////////////////////////////////////////////////////////
// BEGIN Generic execution engine thread lib                                 //
///////////////////////////////////////////////////////////////////////////////

ENL_MLINK NetvEnlMLink[MAX_ADAPTER_COUNT / 2];


/*++
The iteration routine performs one full pass over all input queues and
any internal queues (that might be holding previous items waiting to be
processed).
--*/

static
SIZE_T
CopyTxPacketDataToBuffer(
    _Out_writes_bytes_(BufferLength) PUCHAR BufferDest,
    _In_ NET_RING_PACKET_ITERATOR const * Iterator,
    _In_ NET_EXTENSION const * VirtualAddressExtension,
    _In_ SIZE_T BufferLength)
{
    SIZE_T bytesCopied = 0;

    for (NET_RING_FRAGMENT_ITERATOR fi = NetPacketIteratorGetFragments(Iterator);
         NetFragmentIteratorHasAny(&fi) && (BufferLength > 0);
         NetFragmentIteratorAdvance(&fi))
    {
        NET_FRAGMENT const * fragment = NetFragmentIteratorGetFragment(&fi);
        NET_FRAGMENT_VIRTUAL_ADDRESS const * virtualAddress =
            NetExtensionGetFragmentVirtualAddress(VirtualAddressExtension, NetFragmentIteratorGetIndex(&fi));

        UCHAR const * pPacketData = (UCHAR const *)virtualAddress->VirtualAddress + fragment->Offset;
        SIZE_T bytesToCopy =
            (BufferLength < (SIZE_T) fragment->ValidLength) ? BufferLength : (SIZE_T) fragment->ValidLength;
        RtlCopyMemory(BufferDest, pPacketData, bytesToCopy);

        bytesCopied += bytesToCopy;
        BufferDest += bytesToCopy;
        BufferLength -= bytesToCopy;
    }

    return bytesCopied;
}

//
// Emulate the AP relay for Native 802.11 frames.
//
// Both sample stations associate (in software) to the same hard-coded AP
// BSSID, and the ENL link is the wireless medium.  nwifi builds a
// station-to-peer frame as a "to-DS" frame whose Address1/RA is the AP BSSID,
// expecting the (nonexistent) AP to relay it.  Copied verbatim into the peer's
// receive ring, that RA is not the peer's address, so nwifi on the peer drops
// it before it reaches TCP/IP.  Rewrite the header in place into the "from-DS"
// frame the peer accepts, exactly as a real AP would when forwarding:
//     Addr1 (RA) <- old Addr3 (final destination = peer MAC)
//     Addr2 (TA) <- old Addr1 (BSSID)
//     Addr3 (SA) <- old Addr2 (original sender)
//     FrameControl: clear ToDS, set FromDS
// The address offsets are identical for data and QoS-data frames, and the
// guard below makes this a no-op for non-802.11 (802.3 netvmini) traffic.
static
VOID
EnlpRelayWiFiFrame(
    _Inout_updates_bytes_(Length) PUCHAR Frame,
    _In_ SIZE_T Length)
{
    // FrameControl[2] Duration[2] Addr1[6] Addr2[6] Addr3[6] SeqCtrl[2] ...
    SIZE_T const k80211HeaderLength = 24;
    if (Length < k80211HeaderLength)
    {
        return;
    }

    // Relay only 802.11 data-type (Type == 10b) frames headed to the DS.
    UCHAR const type = Frame[0] & 0x0C;
    UCHAR const dsBits = Frame[1] & 0x03;
    if (type != 0x08 || dsBits != 0x01)
    {
        return;
    }

    PUCHAR const addr1 = Frame + 4;
    PUCHAR const addr2 = Frame + 10;
    PUCHAR const addr3 = Frame + 16;

    UCHAR ra[6], ta[6], sa[6];
    RtlCopyMemory(ra, addr3, 6);
    RtlCopyMemory(ta, addr1, 6);
    RtlCopyMemory(sa, addr2, 6);

    RtlCopyMemory(addr1, ra, 6);
    RtlCopyMemory(addr2, ta, 6);
    RtlCopyMemory(addr3, sa, 6);

    Frame[1] = (Frame[1] & ~0x01) | 0x02;     // ToDS -> FromDS
}

VOID
EnlpAffinitizeThread(
    _In_ ULONG ProcIndex,
    _In_ ULONG IdealNode
    )
{
    LogInformation(FLAG_DRIVER, L"ProcIndex=%u", ProcIndex);
    NTSTATUS status;
    PROCESSOR_NUMBER procNumber = { 0 };
    GROUP_AFFINITY affinity = { 0 };

    if (ProcIndex == 999)
    {
        // No affinity at all
        return;
    }

    if (ProcIndex == 1000 || (ProcIndex >= 1001 && ProcIndex <= 1004))
    {
        KeQueryNodeActiveAffinity((USHORT)IdealNode, &affinity, NULL);

        if (ProcIndex == 1000)
        {
            // Affinitize to all the procs in the Node
            NOTHING;
        }
        else
        {
            // Affinitize to the highest numbered proc in the Node for 1001,
            // next highest numbered proc for 1002, ...
            ULONG index;
#ifdef _WIN64
            NT_FRE_ASSERTMSG("Failed to find bit", TRUE == BitScanReverse64(&index, (ULONG64)affinity.Mask));
            affinity.Mask = (KAFFINITY)((ULONG64)1 << (index - (ProcIndex - 1001)));
#else
            NT_FRE_ASSERTMSG("Failed to find bit", TRUE == BitScanReverse(&index, (ULONG)affinity.Mask));
            affinity.Mask = (KAFFINITY)((ULONG)1 << (index - (ProcIndex - 1001)));
#endif
        }
    }
    else
    {
        // Affinitize the specifically requested proc
        status = KeGetProcessorNumberFromIndex(ProcIndex, &procNumber);
        NT_FRE_ASSERTMSG("Bad ProcIndex", NT_SUCCESS(status));

        affinity.Group = procNumber.Group;
        affinity.Mask = AFFINITY_MASK(procNumber.Number);
    }

    EnlThreadSetAffinity(&affinity, NULL);
}

ENL_START_ROUTINE EnlpThreadRoutine;

NTSTATUS
EnlpStartThread(
    _In_ ULONG ProcIndex,
    _In_ ULONG IdealNode,
    _In_ ENLP_ITERATION_ROUTINE* IterationRoutine,
    _In_ PVOID IterationContext,
    _In_opt_ KAutoEvent* ArmWaitEvent,
    _Out_ ENLP_THREAD_STATE* EnlThread
    )
{
    LogInformation(FLAG_DRIVER, L"ProcIndex=%u EnlThread=%p", ProcIndex, EnlThread);

    EnlThread->PauseRequested = TRUE;
    EnlThread->StopRequested = FALSE;
    EnlThread->IterationRoutine = IterationRoutine;
    EnlThread->IterationContext = IterationContext;
    EnlThread->ArmWaitEvent = ArmWaitEvent;

    EnlThread->ProcIndex = ProcIndex;
    EnlThread->IdealNode = IdealNode;

    RETURN_IF_NOT_STATUS_SUCCESS(
        EnlThreadCreate(EnlpThreadRoutine, EnlThread, EnlThread->Thread));

    EnlThreadSetPriority(EnlThread->Thread, 15);

    RETURN_STATUS_SUCCESS();
}

BOOLEAN
EnlpIsThreadPaused(
    _In_ CONST ENLP_THREAD_STATE* EnlThread
    )
{
    return EnlThread->PauseRequested;
}

VOID
EnlpPauseThread(
    _Inout_ ENLP_THREAD_STATE* EnlThread
    )
{
    LogInformation(FLAG_DRIVER, L"EnlThread=%p", EnlThread);
    if (!EnlpIsThreadPaused(EnlThread))
    {
        WriteBooleanNoFence(&EnlThread->PauseRequested, TRUE);
        if (EnlThread->ArmWaitEvent != NULL)
        {
            EnlThread->ArmWaitEvent->Set();
        }
        EnlThread->PausingEvent.Wait();
    }
}

VOID
EnlpResumeThread(
    _Inout_ ENLP_THREAD_STATE* EnlThread
    )
{
    LogInformation(FLAG_DRIVER, L"EnlThread=%p", EnlThread);
    if (EnlpIsThreadPaused(EnlThread))
    {
        WriteBooleanNoFence(&EnlThread->PauseRequested, FALSE);
        KeMemoryBarrier();
        EnlThread->ResumeEvent.Set();
    }
}

VOID
EnlpStopThread(
    _Inout_ ENLP_THREAD_STATE* EnlThread
    )
{
    LogInformation(FLAG_DRIVER, L"EnlThread=%p", EnlThread);
    if (EnlThread->Thread != NULL)
    {
        WriteBooleanNoFence(&EnlThread->StopRequested, TRUE);
        EnlpResumeThread(EnlThread); // in case thread was paused
        EnlThreadWaitForTermination(EnlThread->Thread);
        EnlThread->Thread.reset();
    }
}

ENL_THREAD_ROUTINE_RETURN
EnlpThreadRoutine(
    _In_ PVOID Context
    )
{
    ENLP_THREAD_STATE* enlThread = (ENLP_THREAD_STATE*)Context;

    //
    // Affinitize the thread
    //
    EnlpAffinitizeThread(enlThread->ProcIndex, enlThread->IdealNode);

    //
    // ENL thread starts at paused state.
    //
    enlThread->ResumeEvent.Wait();

    for (;;)
    {

        if (ReadBooleanNoFence(&enlThread->PauseRequested))
        {
            enlThread->PausingEvent.Set();
            enlThread->ResumeEvent.Wait();
        }

        if (ReadBooleanNoFence(&enlThread->StopRequested))
        {
            break;
        }

        enlThread->IterationRoutine(enlThread->IterationContext);
    }

    return ENL_THREAD_ROUTINE_RETURN();
}

///////////////////////////////////////////////////////////////////////////////
// END Generic execution engine thread lib                                   //
///////////////////////////////////////////////////////////////////////////////

BOOLEAN
enlpCheckAndArmQueue(
    _Inout_ ENLP_QUEUE* Q
    )
{
    BOOLEAN armed = FALSE;
    auto ringBuffer = Q->Queue->GetPacketRing();
    // If Q is still empty, take the lock, and if still empty under lock, then
    // arm it.
    if (ringBuffer->BeginIndex == ringBuffer->EndIndex)
    {
        WdfSpinLockAcquire(Q->Spinlock);

        if (ringBuffer->BeginIndex == ringBuffer->EndIndex)
        {
            armed = TRUE;
            Q->Armed = TRUE;
        }

        WdfSpinLockRelease(Q->Spinlock);
    }

    return armed;
}

VOID
enlpArmAndWait(
    _Inout_ ENLP_LINK* EnlLink
    )
{
    ULONG pi, ci;

    for (pi = 0; pi < ENLP_PORT_COUNT; pi++)
    {
        ENLP_PORT* port = &EnlLink->Ports[pi];

        for (ci = 0; ci < port->TxQueueCount; ci++)
        {
            if (!enlpCheckAndArmQueue(port->TxQueue))
            {
                return;
            }
        }
    }

    EnlLink->ArmWaitEvent.Wait();
}

VOID
EnlpIterationRoutine(
    _In_ PVOID IterationContext
    )
{
    auto enlLink = reinterpret_cast<ENLP_LINK *>(IterationContext);
    bool emptyTx = true;
    //
    // Drain up to TX_BATCH_COUNT items from all tx queues first.
    //

    for (size_t pi = 0U; pi < ENLP_PORT_COUNT; pi++)
    {
        auto txport = &enlLink->Ports[pi];
        auto rxport = &enlLink->Ports[pi ^ 0x1]; // 0 -> 1, 1 -> 0

        for (size_t ci = 0U; ci < txport->TxQueueCount; ci++)
        {
            auto txq = &txport->TxQueue[ci];
            if (txq->State != Started)
                continue;

            NET_RING_PACKET_ITERATOR txPi = {
                txq->Queue->m_rings, nullptr, txq->QueueNext, txq->QueueEnd
            };

            while (NetPacketIteratorHasAny(&txPi))
            {
                bool rxDrop = TRUE;
                auto txPacket = NetPacketIteratorGetPacket(&txPi);

                emptyTx = FALSE;

                if (enlLink->ArmedForWake)
                {
                    // Save wake packet and trigger a wake signal, any other frames queued after the wake
                    // packet will be dropped
                    enlLink->WakeFrameSize = CopyTxPacketDataToBuffer(
                        &enlLink->WakeFrame[0],
                        &txPi,
                        &txq->TxQueue->VirtualAddressExtension,
                        sizeof(enlLink->WakeFrame));
#if _KERNEL_MODE
                    enlLink->EvtWakeSignal(enlLink->WakeSignalContext);
#endif

                    // Make sure to disarm wake, otherwise this thread might overwrite the original wake frame
                    EnlDisarmWake(enlLink);
                }

                if (rxport->RxQueueCount > 0)
                {
                    //TODO: Currently does 1:1 mapping between Tx and Rx. Need to set up indirection table
                    auto rxq = &rxport->RxQueue[ci];

                    if (rxq->State == Started)
                    {
                        NET_RING_FRAGMENT_ITERATOR rxFi = {
                            rxq->Queue->m_rings, nullptr, rxq->QueueNext, rxq->QueueEnd
                        };

                        auto rxPi = NetRingGetPostPackets(rxq->Queue->m_rings);

                        if (NetFragmentIteratorHasAny(&rxFi) && NetPacketIteratorHasAny(&rxPi))
                        {
                            rxDrop = FALSE;

                            auto fragment = NetFragmentIteratorGetFragment(&rxFi);
                            BYTE* fragmentBuffer = nullptr;

                            auto const rxVirtualAddress =
                                NetExtensionGetFragmentVirtualAddress(
                                    &rxq->RxQueue->VirtualAddressExtension,
                                    NetFragmentIteratorGetIndex(&rxFi));

                            fragmentBuffer = static_cast<BYTE *>(rxVirtualAddress->VirtualAddress);

                            fragment->Offset = 0;
                            fragment->ValidLength =
                                CopyTxPacketDataToBuffer(
                                    fragmentBuffer,
                                    &txPi,
                                    &txq->TxQueue->VirtualAddressExtension,
                                    static_cast<SIZE_T>(fragment->Capacity));

                            // Relay the frame as an AP would (see function).
                            EnlpRelayWiFiFrame(
                                fragmentBuffer,
                                static_cast<SIZE_T>(fragment->ValidLength));

                            auto rxPacket = NetPacketIteratorGetPacket(&rxPi);
                            rxPacket->FragmentIndex =  NetFragmentIteratorGetIndex(&rxFi);
                            rxPacket->FragmentCount = 1;

                            if (enlLink->InitializePacketLayout)
                            {
                                rxPacket->Layout = txPacket->Layout;
                            }
                            else
                            {
                                rxPacket->Layout = {};
                            }

                            if (rxq->RxQueue->RxXSumExtension.Enabled)
                            {
                                auto RxXSum = NetExtensionGetPacketChecksum(
                                    &rxq->RxQueue->RxXSumExtension,
                                    NetPacketIteratorGetIndex(&rxPi));
                                RxXSum->Layer2 = NetPacketRxChecksumEvaluationNotChecked;
                                RxXSum->Layer3 = NetPacketRxChecksumEvaluationValid;
                                RxXSum->Layer4 = NetPacketRxChecksumEvaluationValid;
                            }

                            if (rxq->RxQueue->UdpRscExtension.Enabled &&
                                txq->TxQueue->UsoExtension.Enabled)
                            {
                                auto txUso = NetExtensionGetPacketGso(
                                    &txq->TxQueue->UsoExtension,
                                    NetPacketIteratorGetIndex(&txPi));
                                if (txPacket->Layout.Layer4Type == NetPacketLayer4TypeUdp &&
                                    txUso->UDP.Mss > 0)
                                {
                                    if (txPacket->Layout.Layer3Type == NetPacketLayer3TypeIPv6NoExtensions)
                                    {
                                        UINT16* ipv6PayloadLength = (UINT16*)(fragmentBuffer + txPacket->Layout.Layer2HeaderLength + 4);
                                        *ipv6PayloadLength = _byteswap_ushort(
                                            (USHORT)(fragment->ValidLength -
                                            (txPacket->Layout.Layer2HeaderLength + txPacket->Layout.Layer3HeaderLength)));
                                    }
                                    else if (txPacket->Layout.Layer3Type == NetPacketLayer3TypeIPv4NoOptions)
                                    {
                                        UINT16* ipv4TotalLength = (UINT16*)(fragmentBuffer + txPacket->Layout.Layer2HeaderLength + 2);
                                        *ipv4TotalLength = _byteswap_ushort((USHORT)(fragment->ValidLength - txPacket->Layout.Layer2HeaderLength ));
                                        NT_FRE_ASSERT(*ipv4TotalLength > 0);
                                    }

                                    UINT16* udpPayloadLength = (UINT16*)(fragmentBuffer + txPacket->Layout.Layer2HeaderLength +
                                                                         txPacket->Layout.Layer3HeaderLength + 4);
                                    *udpPayloadLength = _byteswap_ushort(
                                        (USHORT)(fragment->ValidLength -
                                        (txPacket->Layout.Layer3HeaderLength + txPacket->Layout.Layer2HeaderLength)));

                                    // Set the checksum to zero
                                    UINT16* udpChecksum = udpPayloadLength + 1;
                                    *udpChecksum = 0;

                                    auto rxUro = NetExtensionGetPacketRsc(
                                        &rxq->RxQueue->UdpRscExtension,
                                        NetPacketIteratorGetIndex(&rxPi));
                                    rxUro->UDP.CoalescedSegmentSize = (UINT16)txUso->UDP.Mss;
                                    rxUro->UDP.CoalescedSegmentCount =
                                        (UINT16)((fragment->ValidLength - txPacket->Layout.Layer3HeaderLength - txPacket->Layout.Layer4HeaderLength
                                         + txUso->UDP.Mss - 1) / txUso->UDP.Mss);
                                    NT_FRE_ASSERT(rxUro->UDP.CoalescedSegmentCount > 1);
                                    NT_FRE_ASSERT(*udpPayloadLength >= 8);
                                }
                            }

                            // prevent any reordering the tx/rx completion flag
                            KeMemoryBarrier();

                            // Use Scratch field as completion flag for the rx fragment
                            fragment->Scratch = 1;
                            rxPacket->Scratch = 1;

                            NetFragmentIteratorAdvance(&rxFi);
                            NetPacketIteratorAdvance(&rxPi);

                            rxq->QueueNext = NetFragmentIteratorGetIndex(&rxFi);
                            NetPacketIteratorSet(&rxPi);

                            if (rxq->Notify)
                            {
                                rxport->Interrupt(rxq->Queue->m_handle, rxq->TxRx);
                            }
                        }
                    }
                }

                if (rxDrop)
                {
                    // TODO - add rxdrop stat
                }

                // Use Scratch field as completion flag for the tx packet
                txPacket->Scratch = 1;
                NetPacketIteratorAdvance(&txPi);
            }

            if (!emptyTx)
            {
                if (txq->Notify)
                {
                    txport->Interrupt(txq->Queue->m_handle, txq->TxRx);
                }

                // Store the next index so that the EnlThread knows which packet to start from in the next iteration
                txq->QueueNext = NetPacketIteratorGetIndex(&txPi);
            }
        }
    }

    ULONG64 ts;

    if (emptyTx)
    {
        if (enlLink->Poll == FALSE)
        {
            enlpArmAndWait(enlLink);
        }

        ts = ReadTimeStampCounter();
        enlLink->EmptyTicks += (ts - enlLink->Ts);
    }
    else
    {
        ts = ReadTimeStampCounter();
        enlLink->BusyTicks += (ts - enlLink->Ts);
    }

    enlLink->Ts = ts;
}

_IRQL_requires_max_(PASSIVE_LEVEL)
ENLP_QUEUE *
EnlCreateQueue(
    _In_ NETPACKETQUEUE Queue,
    _In_ BOOLEAN Tx
    )
{
    ENLP_LINK* enlLink;
    ENLP_PORT* port;
    ENLP_QUEUE* enlQueue;
    NTSTATUS status;

    if (Tx) // Tx
    {
        NetvTxQueue* netvTxQueue = NetvTxQueueGetContext(Queue);
        enlLink = NetvEnlMLink[netvTxQueue->m_adapter.EnlIndex].LinkHandle[0];
        port = &enlLink->Ports[netvTxQueue->m_adapter.EnlPortIndex];
        enlQueue = &port->TxQueue[0]; // Change to accommodate multiple Queues
        enlQueue->Queue = netvTxQueue;
        enlQueue->TxQueue = netvTxQueue;
        enlQueue->State = Stopped;
        enlQueue->ArmWaitEvent = &enlLink->ArmWaitEvent;
        enlQueue->QueueNext = 0;
        enlQueue->QueueEnd = 0;
        enlQueue->TxRx = TX;
        enlQueue->EnlPortHandle = port;
        port->TxQueueCount++;
        LogInformation(FLAG_DRIVER, L"Adapter=%p Queue=%Iu TxQueue=%p",
            &netvTxQueue->m_adapter, reinterpret_cast<ULONG_PTR>(Queue), netvTxQueue);
    }
    else // Rx
    {
        NetvRxQueue* netvRxQueue = NetvRxQueueGetContext(Queue);
        enlLink = NetvEnlMLink[netvRxQueue->m_adapter.EnlIndex].LinkHandle[0];
        port = &enlLink->Ports[netvRxQueue->m_adapter.EnlPortIndex];
        enlQueue = &port->RxQueue[0]; // Change to accommodate multiple Queues
        enlQueue->Queue = netvRxQueue;
        enlQueue->RxQueue = netvRxQueue;
        enlQueue->State = Stopped;
        enlQueue->QueueNext = 0;
        enlQueue->QueueEnd = 0;
        enlQueue->TxRx = RX;
        enlQueue->EnlPortHandle = port;
        port->RxQueueCount++;
        LogInformation(FLAG_DRIVER, L"Adapter=%p Queue=%Iu RxQueue=%p",
            &netvRxQueue->m_adapter, reinterpret_cast<ULONG_PTR>(Queue), netvRxQueue);
    }

    // Create WDF spinlock for the queue, parent to the queue's WDF handle
    WDF_OBJECT_ATTRIBUTES attributes;
    WDF_OBJECT_ATTRIBUTES_INIT(&attributes);
    attributes.ParentObject = enlQueue->Queue->m_handle;
    status = WdfSpinLockCreate(&attributes, &enlQueue->Spinlock);
    NT_ASSERT(NT_SUCCESS(status));


    EnlpResumeThread(&enlLink->EnlThread);

    return enlQueue;
}

_IRQL_requires_max_(PASSIVE_LEVEL)
VOID
EnlDestroyQueue(
    _In_ ENLP_QUEUE * Queue,
    _In_ BOOLEAN Tx
    )
{
    LogInformation(FLAG_DRIVER, L"Queue=%p", Queue);
    ENLP_PORT* port = Queue->EnlPortHandle;

    if (Tx)
    {
        port->TxQueueCount--;
    }
    else
    {
        port->RxQueueCount--;
    }

    Queue->ArmWaitEvent = nullptr;
}

_IRQL_requires_max_(PASSIVE_LEVEL)
VOID
EnlRingDoorBell(
    _In_ ENLP_QUEUE * Queue,
    _In_ ULONG EndIndex
    )
{
    InterlockedExchange((volatile LONG *)&Queue->QueueEnd, (LONG)EndIndex);

    WdfSpinLockAcquire(Queue->Spinlock);
    
    if (Queue->Armed)
    {
        NT_ASSERT(Queue->ArmWaitEvent != NULL);
        Queue->Armed = FALSE;
        WdfSpinLockRelease(Queue->Spinlock);
        Queue->ArmWaitEvent->Set();
        return;
    }

    WdfSpinLockRelease(Queue->Spinlock);
}

_IRQL_requires_max_(PASSIVE_LEVEL)
VOID
EnlArmInterrupt(
    _In_ ENLP_QUEUE * Queue,
    _In_ BOOLEAN notificationEnabled
    )
{
    Queue->Notify = notificationEnabled;

    // TODO: InterlockedExchange reduces the throughput of the ENL from about 500MB/s to 8MB/s. -> why
    //InterlockedExchange((volatile LONG *)&Queue->Notify, (LONG)notificationEnabled);
}

_Use_decl_annotations_
VOID
EnlIndicateQueueState(
    ENLP_QUEUE * Queue,
    ENL_QUEUE_STATE State
    )
{
    Queue->State = State;
}

_IRQL_requires_max_(PASSIVE_LEVEL)
NTSTATUS
EnlCreateLink(
    _In_ ULONG ProcessorIndex,
    _In_ ULONG IdealNode,
    _In_ BOOLEAN Poll,
    _In_ BOOLEAN InitializePacketLayout,
    _Out_ ENLP_LINK ** EnlLink
    )
{
    LogInformation(FLAG_DRIVER, L"ProcessorIndex=%u", ProcessorIndex);

    auto enlLink = wil::make_unique_nothrow<ENLP_LINK>();
    RETURN_NTSTATUS_IF(
        STATUS_INSUFFICIENT_RESOURCES,
        ! enlLink);

    enlLink->Ts = ReadTimeStampCounter();
    enlLink->Poll = Poll;
    enlLink->InitializePacketLayout = InitializePacketLayout;

    RETURN_IF_NOT_STATUS_SUCCESS(
        EnlpStartThread(
            ProcessorIndex,
            IdealNode,
            EnlpIterationRoutine,
            enlLink.get(),
            enlLink->Poll ? NULL : &enlLink->ArmWaitEvent,
            &enlLink->EnlThread));

    *EnlLink = enlLink.release();

    RETURN_STATUS_SUCCESS();
}

_IRQL_requires_max_(PASSIVE_LEVEL)
BOOLEAN
EnlIsPortActive(
    _In_ ENLP_LINK * EnlLink,
    _In_ ULONG PortIndex
    )
{
    ENLP_PORT* port = &EnlLink->Ports[PortIndex];
    ENLP_QUEUE* txq = &port->TxQueue[0];
    NT_FRE_ASSERT(PortIndex < ENLP_PORT_COUNT);

    return (txq->Queue == nullptr) ? FALSE : TRUE;
}

_IRQL_requires_max_(PASSIVE_LEVEL)
BOOLEAN
EnlIsLinkActive(
    _In_ ENLP_LINK * EnlLink
    )
{
    return (EnlIsPortActive(EnlLink, 0) || EnlIsPortActive(EnlLink, 1));
}

_IRQL_requires_max_(PASSIVE_LEVEL)
NTSTATUS
EnlActivateLinkPort(
    _In_ ENLP_LINK * EnlLink,
    _In_ ULONG PortIndex,
    _In_ ENL_INTERRUPT_ROUTINE Interrupt,
    _In_ PVOID PortContext
    )
{
    ENLP_PORT* port = &EnlLink->Ports[PortIndex];

    LogInformation(FLAG_DRIVER, L"PortIndex=%u Queue=%p", PortIndex, port->TxQueue[0].Queue);

    NT_FRE_ASSERT(!EnlIsPortActive(EnlLink, PortIndex));
    NT_FRE_ASSERT(port->TxQueueCount == 0);
    NT_FRE_ASSERT(port->RxQueueCount == 0);

    port->Interrupt = Interrupt;
    port->PortContext = PortContext;

    RETURN_STATUS_SUCCESS();
}

_IRQL_requires_max_(PASSIVE_LEVEL)
void
EnlDeactivateLinkPort(
    _In_ ENLP_LINK * EnlLink,
    _In_ ULONG PortIndex
    )
{
    NT_FRE_ASSERT(PortIndex < ENLP_PORT_COUNT);

    ENLP_PORT* port = &EnlLink->Ports[PortIndex];
    LogInformation(FLAG_DRIVER, L"PortIndex=%u Queue=%p", PortIndex, port->TxQueue[0].Queue);

    NT_FRE_ASSERT(EnlIsPortActive(EnlLink, PortIndex));

    KLockThisExclusive(EnlLink->Lock);
    NT_FRE_ASSERT(!EnlpIsThreadPaused(&EnlLink->EnlThread));
    EnlpPauseThread(&EnlLink->EnlThread);

#if _KERNEL_MODE
    KeFlushQueuedDpcs();
#endif

    port->PortContext = NULL;

    //Clears reference to only first queue - Change for all queues
    port->TxQueue[0].Queue = nullptr;
    port->RxQueue[0].Queue = nullptr;


    // As long as there's one port with active queues, the EnlThread will run.
    for (ULONG i = 0; i < ENLP_PORT_COUNT; i++)
    {
        if (EnlIsPortActive(EnlLink, i))
        {
            EnlpResumeThread(&EnlLink->EnlThread);
            break;
        }
    }
}

_IRQL_requires_max_(PASSIVE_LEVEL)
VOID
EnlCloseLink(
    _In_ ENLP_LINK * EnlLink
    )
{
    ULONG i;

    for (i = 0; i < ENLP_PORT_COUNT; i++)
    {
        NT_FRE_ASSERT(!EnlIsPortActive(EnlLink, i));
    }

    EnlpStopThread(&EnlLink->EnlThread);

    delete EnlLink;
}

///////////////////////////////////////////////////////////////////////////////
//  Multi link wrapper APIs                                                  //
///////////////////////////////////////////////////////////////////////////////

_IRQL_requires_max_(PASSIVE_LEVEL)
NTSTATUS
EnlMCreateLink(
    _In_range_(1, ENL_MLINK_MAX)ULONG LinkCount,
    _In_reads_(LinkCount) ULONG ProcessorIndex,
    _In_ BOOLEAN Poll,
    _Out_ ENL_MLINK* EnlMLink
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    ULONG i = 0;

    RtlZeroMemory(EnlMLink, sizeof(*EnlMLink));

    if (LinkCount < 1 || LinkCount > ENL_MLINK_MAX ||
        (LinkCount & (LinkCount - 1)) != 0)
    {
        status = STATUS_REQUEST_NOT_ACCEPTED;
        goto exit;
    }

    for (i = 0; i < LinkCount; i++)
    {
#ifdef _KERNEL_MODE
        const BOOLEAN initializePacketLayout = TRUE;
#else
        const BOOLEAN initializePacketLayout = FALSE;
#endif
        status = EnlCreateLink(ProcessorIndex, 0, Poll, initializePacketLayout, &EnlMLink->LinkHandle[i]);

        if (!NT_SUCCESS(status))
        {
            goto exit;
        }
    }

    EnlMLink->LinkCount = LinkCount;

exit:

    if (!NT_SUCCESS(status))
    {
        for (; i > 0; i--)
        {
            EnlCloseLink(EnlMLink->LinkHandle[i - 1]);
            EnlMLink->LinkHandle[i - 1] = NULL;
        }
    }
    RETURN_NTSTATUS_IF(
        status,
        status != STATUS_SUCCESS);

    RETURN_STATUS_SUCCESS();
}

_IRQL_requires_max_(PASSIVE_LEVEL)
void EnlSetPdoWakeSignalCallback(
    _In_ ENLP_LINK * EnlLink,
    _In_ EVT_ENLP_PDO_WAKE_SIGNAL* evtPdoWakeSignal,
    _In_ void* Context)
{
    EnlLink->EvtWakeSignal = evtPdoWakeSignal;
    EnlLink->WakeSignalContext = Context;
}

_IRQL_requires_max_(PASSIVE_LEVEL)
void
EnlArmWake(
    _In_ ENLP_LINK * EnlLink
)
{
    EnlLink->ArmedForWake = TRUE;
    RtlZeroMemory(&EnlLink->WakeFrame[0], sizeof(EnlLink->WakeFrame));
    EnlLink->WakeFrameSize = 0;
}

_IRQL_requires_max_(PASSIVE_LEVEL)
void
EnlDisarmWake(
    _In_ ENLP_LINK * EnlLink
)
{
    EnlLink->ArmedForWake = FALSE;
}

_IRQL_requires_max_(PASSIVE_LEVEL)
size_t
EnlCopyWakeFrame(
    _In_ ENLP_LINK * EnlLink,
    _Out_writes_bytes_(BufferSize) unsigned char * Buffer,
    _In_ size_t BufferSize
)
{
    if (EnlLink->WakeFrameSize == 0)
    {
        // There was no wake
        return 0;
    }

    if (BufferSize < EnlLink->WakeFrameSize)
    {
        // Wake frame is larger than what we can indicate
        return 0;
    }

    RtlCopyMemory(Buffer, &EnlLink->WakeFrame[0], EnlLink->WakeFrameSize);
    auto const wakeFrameSize = EnlLink->WakeFrameSize;

    // Make sure to erase the wake frame, since the network interface might have
    // multiple receive queues
    RtlZeroMemory(&EnlLink->WakeFrame[0], EnlLink->WakeFrameSize);
    EnlLink->WakeFrameSize = 0;

    return wakeFrameSize;
}

_IRQL_requires_max_(PASSIVE_LEVEL)
BOOLEAN
EnlMIsPortActive(
    _In_ CONST ENL_MLINK* EnlMLink,
    _In_ ULONG PortIndex
    )
{
    ULONG i;

    BOOLEAN result = EnlIsPortActive(EnlMLink->LinkHandle[0], PortIndex);

    for (i = 1; i < EnlMLink->LinkCount; i++)
    {
        NT_FRE_ASSERT(EnlIsPortActive(EnlMLink->LinkHandle[i], PortIndex) == result);
    }

    return result;
}

_IRQL_requires_max_(PASSIVE_LEVEL)
BOOLEAN
EnlMIsLinkActive(
    _In_ CONST ENL_MLINK* EnlMLink
    )
{
    ULONG i;

    BOOLEAN result = EnlIsLinkActive(EnlMLink->LinkHandle[0]);

    for (i = 1; i < EnlMLink->LinkCount; i++)
    {
        NT_FRE_ASSERT(EnlIsLinkActive(EnlMLink->LinkHandle[i]) == result);
    }

    return result;
}

_IRQL_requires_max_(PASSIVE_LEVEL)
NTSTATUS
EnlMActivateLinkPort(
    _In_ CONST ENL_MLINK* EnlMLink,
    _In_ ULONG PortIndex,
    _In_ ENL_INTERRUPT_ROUTINE Interrupt,
    _In_ PVOID PortContext
    )
{
    LogInformation(FLAG_DRIVER, L"EnlMLink=%p PortIndex=%u", EnlMLink, PortIndex);

    NTSTATUS status = STATUS_SUCCESS;
    ULONG i;

    for (i = 0; i < EnlMLink->LinkCount; i++)
    {
        status = EnlActivateLinkPort(
            EnlMLink->LinkHandle[i],
            PortIndex,
            Interrupt,
            PortContext);

        if (!NT_SUCCESS(status))
        {
            break;
        }
    }

    if (!NT_SUCCESS(status))
    {
        for (; i > 0; i--)
        {
            EnlDeactivateLinkPort(EnlMLink->LinkHandle[i], PortIndex);
        }
    }

    RETURN_NTSTATUS_IF(status,
        status != STATUS_SUCCESS);

    RETURN_STATUS_SUCCESS();
}

_IRQL_requires_max_(PASSIVE_LEVEL)
void
EnlMDeactivateLinkPort(
    _In_ CONST ENL_MLINK* EnlMLink,
    _In_ ULONG PortIndex
    )
{
    LogInformation(FLAG_DRIVER, L"EnlMLink=%p PortIndex=%u", EnlMLink, PortIndex);

    ULONG i;

    for (i = 0; i < EnlMLink->LinkCount; i++)
    {
        EnlDeactivateLinkPort(EnlMLink->LinkHandle[i], PortIndex);
    }
}

_IRQL_requires_max_(PASSIVE_LEVEL)
VOID
EnlMCloseLink(
    _Inout_ ENL_MLINK* EnlMLink
    )
{
    LogInformation(FLAG_DRIVER, L"EnlMLink=%p", EnlMLink);

    ULONG i;

    for (i = 0; i < EnlMLink->LinkCount; i++)
    {
        EnlCloseLink(EnlMLink->LinkHandle[i]);
        EnlMLink->LinkHandle[i] = NULL;
    }

    EnlMLink->LinkCount = 0;
}
