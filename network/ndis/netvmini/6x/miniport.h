/*++

Copyright (c) Microsoft Corporation.  All rights reserved.

    THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY
    KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
    IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR
    PURPOSE.

Module Name:

    Miniport.H

Abstract:

    This module contains structure definitons and function prototypes.

    TODO:
       1.  Set the correct driver version number for your versioning scheme.
       2.  Create unique memory allocation tags.

 --*/


#ifndef _MINIPORT_H
#define _MINIPORT_H



//
// Update the driver version number every time you release a new driver
// The high word is the major version. The low word is the minor version.
// Also make sure that VER_FILEVERSION specified in the .RC file also
// matches with the driver version because NDISTESTER checks for that.
//
// Let's say we're version 4.2.
//
#define NIC_MAJOR_DRIVER_VERSION           0x04
#define NIC_MINOR_DRIVER_VERISON           0x02
#define NIC_VENDOR_DRIVER_VERSION          ((NIC_MAJOR_DRIVER_VERSION << 16) | NIC_MINOR_DRIVER_VERISON)



//
// Define the NDIS miniport interface version that this driver targets.
//
#if defined(NDIS60_MINIPORT)
#  define MP_NDIS_MAJOR_VERSION             6
#  define MP_NDIS_MINOR_VERSION             0
#elif defined(NDIS620_MINIPORT)
#  define MP_NDIS_MAJOR_VERSION             6
#  define MP_NDIS_MINOR_VERSION            20
#elif defined(NDIS630_MINIPORT)
#  define MP_NDIS_MAJOR_VERSION             6
#  define MP_NDIS_MINOR_VERSION            30
#elif defined(NDIS680_MINIPORT)
#  define MP_NDIS_MAJOR_VERSION             6
#  define MP_NDIS_MINOR_VERSION            80
#else
#  error Unsupported NDIS version
#endif


//
// Memory allocation tags to help track and debug memory usage.  Change these
// for your miniport.  You can add or remove tags as needed.
//
#define NIC_TAG                            ((ULONG)'_MVN')  // NVM_
#define NIC_TAG_TCB                        ((ULONG)'TMVN')  // NVMT
#define NIC_TAG_RCB                        ((ULONG)'RMVN')  // NVMR
#define NIC_TAG_RECV_NBL                   ((ULONG)'rMVN')  // NVMr
#define NIC_TAG_FRAME                      ((ULONG)'FMVN')  // NVMF
#define NIC_TAG_DPC                        ((ULONG)'DMVN')  // NVMD
#define NIC_TAG_TIMER                      ((ULONG)'tMVN')  // NVMt

#if (NDIS_SUPPORT_NDIS620)

#define NIC_TAG_QUEUE_INFO                 ((ULONG)'QMVN')  // NVMQ
#define NIC_TAG_QUEUE_SHARED_MEM           ((ULONG)'MMVN')  // NVMM
#define NIC_TAG_QUEUE_SHARED_MEM_BLOCK     ((ULONG)'IMVN')  // NVMB
#define NIC_TAG_QUEUE_WORK_ITEM            ((ULONG)'WMVN')  // NVMW
#define NIC_TAG_QUEUE_SG_LIST              ((ULONG)'SMVN')  // NVMS

#endif

#if (NDIS_SUPPORT_NDIS630)

#define NIC_TAG_QOS_PARAMS                 ((ULONG)'PMVN')  // NVMP

#endif

#define NIC_ADAPTER_CHECK_FOR_HANG_TIME_IN_SECONDS 4



//
// Buffer size passed in NdisMQueryAdapterResources
// We should only need three adapter resources (IO, interrupt and memory),
// Some devices get extra resources, so have room for 10 resources
//
#define NIC_RESOURCE_BUF_SIZE \
        (sizeof(NDIS_RESOURCE_LIST) + \
         (10*sizeof(CM_PARTIAL_RESOURCE_DESCRIPTOR)))


//
// Utility macros
// -----------------------------------------------------------------------------
//

#ifndef min
#define    min(_a, _b)      (((_a) < (_b)) ? (_a) : (_b))
#endif

#ifndef max
#define    max(_a, _b)      (((_a) > (_b)) ? (_a) : (_b))
#endif



#define LIST_ENTRY_FROM_NBL(_NBL) ((PLIST_ENTRY)&(_NBL)->MiniportReserved[0])
#define NBL_FROM_LIST_ENTRY(_ENTRY) (CONTAINING_RECORD(_ENTRY, NET_BUFFER_LIST, MiniportReserved[0]))

// Get a pointer to a LIST_ENTRY for the receive free list, from an NBL pointer
#define RECV_FREE_LIST_FROM_NBL(_NBL) LIST_ENTRY_FROM_NBL(_NBL)

// Get a pointer to a NBL, from a pointer to a LIST_ENTRY on the receive free list
#define NBL_FROM_RECV_FREE_LIST(_ENTRY) NBL_FROM_LIST_ENTRY(_ENTRY)


// Get a pointer to a LIST_ENTRY for the cancel list, from an NBL pointer
#define CANCEL_LIST_FROM_NBL(_NBL) ((PSINGLE_LIST_ENTRY)&(_NBL)->MiniportReserved[0])

// Get a pointer to a NBL, from a pointer to a LIST_ENTRY on the cancel list
#define NBL_FROM_CANCEL_LIST(_ENTRY) NBL_FROM_LIST_ENTRY(_ENTRY)



// Get a pointer to a LIST_ENTRY for the send wait list, from an NB pointer
#define SEND_WAIT_LIST_FROM_NB(_NB) ((PLIST_ENTRY)&(_NB)->MiniportReserved[0])

// Get a pointer to a NB, from a pointer to a LIST_ENTRY on the send wait list
#define NB_FROM_SEND_WAIT_LIST(_ENTRY) (CONTAINING_RECORD(_ENTRY, NET_BUFFER, MiniportReserved[0]))


// Get a pointer to an RCB from an NBL
#define RCB_FROM_NBL(_NBL) (*((PRCB*)&(_NBL)->MiniportReserved[0]))

// Gets the number of outstanding TCBs/NET_BUFFERs associated with
// this NET_BUFFER_LIST
#define SEND_REF_FROM_NBL(_NBL) (*((PLONG)&(_NBL)->MiniportReserved[1]))


#define NBL_FROM_SEND_NB(_NB) (*((PNET_BUFFER_LIST*)&(_NB)->MiniportReserved[2]))

#define FRAME_TYPE_FROM_SEND_NB(_NB) (*((PULONG)&(_NB)->MiniportReserved[3]))


//
// The NDIS_RW_LOCK_EX lock is more efficient (especially in the presence of
// more than 64 CPUs), but it is only available on Windows 7 and later.
//
// These macros let us use the better lock if we are a 6.20 or better miniport,
// or fallback to compiling against the 6.0 lock otherwise.
//

#if (NDIS_SUPPORT_NDIS620)
#  define MP_RW_LOCK_TYPE                           PNDIS_RW_LOCK_EX
#  define MP_LOCK_STATE                             LOCK_STATE_EX
#  define LOCK_ADAPTER_LIST_FOR_READ(STATE, FLAGS)  NdisAcquireRWLockRead (GlobalData.Lock, STATE, FLAGS)
#  define LOCK_ADAPTER_LIST_FOR_WRITE(STATE, FLAGS) NdisAcquireRWLockWrite(GlobalData.Lock, STATE, FLAGS)
#  define UNLOCK_ADAPTER_LIST(STATE)                NdisReleaseRWLock(GlobalData.Lock, STATE)
#else
#  define MP_RW_LOCK_TYPE                           NDIS_RW_LOCK
#  define MP_LOCK_STATE                             LOCK_STATE
#  define LOCK_ADAPTER_LIST_FOR_READ(STATE, FLAGS)  NdisAcquireReadWriteLock(&GlobalData.Lock, FALSE, STATE)
#  define LOCK_ADAPTER_LIST_FOR_WRITE(STATE, FLAGS) NdisAcquireReadWriteLock(&GlobalData.Lock, TRUE, STATE)
#  define UNLOCK_ADAPTER_LIST(STATE)                NdisReleaseReadWriteLock(&GlobalData.Lock, STATE)
#endif

#define ACQUIRE_NDIS_SPINLOCK(_AtDpc, _SpinLock)\
    if(_AtDpc)\
    {\
        NdisDprAcquireSpinLock(_SpinLock);\
    }\
    else\
    {\
        NdisAcquireSpinLock(_SpinLock);\
    }

#define RELEASE_NDIS_SPINLOCK(_AtDpc, _SpinLock)\
    if(_AtDpc)\
    {\
        NdisDprReleaseSpinLock(_SpinLock);\
    }\
    else\
    {\
        NdisReleaseSpinLock(_SpinLock);\
    }

//
// The driver has exactly one instance of the MP_GLOBAL structure.  NDIS keeps
// an opaque handle to this data, (it doesn't attempt to read or interpret this
// data), and it passes the handle back to the miniport in MiniportSetOptions
// and MiniportInitializeEx.
//
typedef struct _MP_GLOBAL
{
    LIST_ENTRY              AdapterList;

    MP_RW_LOCK_TYPE         Lock;

    NPAGED_LOOKASIDE_LIST   FrameDataLookaside;

#define fGLOBAL_LOCK_ALLOCATED        0x0001
#define fGLOBAL_LOOKASIDE_INITIALIZED 0x0002
#define fGLOBAL_MINIPORT_REGISTERED   0x0004
    ULONG                   Flags;
} MP_GLOBAL, *PMP_GLOBAL;

struct _MP_ADAPTER;


// Global data
extern NDIS_HANDLE     NdisDriverHandle;
extern MP_GLOBAL       GlobalData;



// Miniport routines
SET_OPTIONS  MPSetOptions;


void
MPAttachAdapter(
    _In_  struct _MP_ADAPTER *Adapter);

void
MPDetachAdapter(
    _In_  struct _MP_ADAPTER *Adapter);

BOOLEAN
MPIsAdapterAttached(
    _In_ struct _MP_ADAPTER *Adapter);


VOID
DbgPrintOidName(
    _In_  NDIS_OID  OidReqQuery);

VOID
DbgPrintAddress(
    _In_reads_bytes_(NIC_MACADDR_SIZE) PUCHAR Address);


#endif    // _MINIPORT_H


