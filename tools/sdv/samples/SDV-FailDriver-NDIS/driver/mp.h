#ifndef _MP_H
#define _MP_H

#ifdef __cplusplus
extern "C" {
#endif
#include <ndis.h>
#ifdef __cplusplus
}
#endif

#define MP_NDIS_MAJOR_VERSION       6
#define MP_NDIS_MINOR_VERSION       0

#pragma warning(disable: 28121)
#pragma warning(disable: 28141)

typedef enum _NIC_PAUSE_STATE
{
    NicPaused,
    NicPausing,
    NicRunning
} NIC_PAUSE_STATE, *PNIC_PAUSE_STATE;

typedef enum _NIC_PNP_STATE
{
    NicStarted,
    NicInReset,
    NicShutdown,
    NicHalted
} NIC_PNP_STATE, *PNIC_PNP_STATE;

#define NIC_TAG                         ((ULONG)'001E')


#define NIC_INTERFACE_TYPE              NdisInterfacePci
#define NIC_CHECK_FOR_HANG_TIME         2
//10000 system time units (100-nanosecond intervals) = 1 milliseconds
#define NIC_LINK_DETECTION_DELAY        -1000000 //-1*(100*10000) 

typedef struct _MP_ADAPTER
{
    NDIS_HANDLE             AdapterHandle;

    NDIS_SPIN_LOCK          Lock;

    NIC_PAUSE_STATE         PauseState;
    NIC_PNP_STATE           PNPState;
    
    ULONG                   RefCount;

    ULONG                   LastRefCount;

    BOOLEAN                 InterruptRegistered;
    NDIS_HANDLE             InterruptHandle;

    BOOLEAN                 TimerRegistered;
    NDIS_HANDLE             TimerHandle;
    
    BOOLEAN                 AdapetrMemoryAllocated;
    NDIS_HANDLE             NdisMiniportDmaHandle;

} MP_ADAPTER, *PMP_ADAPTER;


//--------------------------------------
// Miniport routines in MP_MAIN.C
//--------------------------------------

#ifdef __cplusplus
extern "C"
#endif
NDIS_STATUS
DriverEntry(
    _In_ PDRIVER_OBJECT      DriverObject,
    _In_ PUNICODE_STRING     RegistryPath
    );


MINIPORT_ALLOCATE_SHARED_MEM_COMPLETE MPAllocateComplete;

MINIPORT_HALT MPHalt;
MINIPORT_SET_OPTIONS MPSetOptions;
MINIPORT_INITIALIZE MPInitialize;
MINIPORT_PAUSE MPPause;
MINIPORT_RESTART MPRestart;
MINIPORT_OID_REQUEST MPOidRequest;
MINIPORT_INTERRUPT_DPC MPHandleInterrupt;
MINIPORT_ISR MPIsr;
MINIPORT_RESET MPReset;
MINIPORT_RETURN_NET_BUFFER_LISTS MPReturnNetBufferLists;
MINIPORT_CANCEL_OID_REQUEST MPCancelOidRequest;
MINIPORT_SHUTDOWN MPShutdown;
MINIPORT_SEND_NET_BUFFER_LISTS MPSendNetBufferLists;
MINIPORT_CANCEL_SEND MPCancelSendNetBufferLists;
MINIPORT_DEVICE_PNP_EVENT_NOTIFY MPPnPEventNotify;
MINIPORT_UNLOAD MPUnload;
MINIPORT_CHECK_FOR_HANG MPCheckForHang;
MINIPORT_ENABLE_INTERRUPT MpEnableInterrupt;
MINIPORT_DISABLE_INTERRUPT MpDisableInterrupt;
MINIPORT_SYNCHRONIZE_INTERRUPT MPSynchronizeInterrupt;
MINIPORT_PROCESS_SG_LIST MPProcessSGList;
NDIS_TIMER_FUNCTION MpDemonstrationTimer;
NDIS_IO_WORKITEM_FUNCTION MPQueuedWorkItem;

NDIS_STATUS 
NICAllocAdapterMemory(
    _In_ PMP_ADAPTER     Adapter
    );
    
NDIS_STATUS 
MpRegisterInterrupt(
    _In_ PMP_ADAPTER         pAdapter
    );

NDIS_STATUS
MpQueueTimer(
    _In_ PMP_ADAPTER         pAdapter
    );

#endif  // _MP_H

