/*++

Copyright (c) 1989-2011  Microsoft Corporation

Module Name:

    avscan.h

Abstract:

    Header file which contains the structures, type definitions,
    constants, global variables and function prototypes that are
    only visible within the kernel. Mainly used by avscan module.

Environment:

    Kernel mode

--*/
#ifndef __AVSCAN_H__
#define __AVSCAN_H__

#ifndef RTL_USE_AVL_TABLES
#define RTL_USE_AVL_TABLES
#endif // RTL_USE_AVL_TABLES

#define AV_VISTA    (NTDDI_VERSION >= NTDDI_VISTA)

#include <fltKernel.h>
#include <dontuse.h>
#include <suppress.h>
#include "utility.h"
#include "context.h"
#include "scan.h"
#include "csvfs.h"
#include "avlib.h"


#pragma prefast(disable:__WARNING_ENCODE_MEMBER_FUNCTION_POINTER, "Not valid for kernel mode drivers")

//
//  Scan context.
//
//  We chose to seperate scan context and section context to have one struct per concept.
//  The I/O request thread does not need to know how scanner implement the scan, so 
//  that the I/O request thread has less coupling with scanner threads.
//
//  You can also put all of fields of the scan context into a section context, and allocate
//  section context at the place of allocation of scan context.
//  

typedef struct _AV_SCAN_CONTEXT {

    LONG           RefCount;
    PFLT_INSTANCE  FilterInstance;
    PFILE_OBJECT   FileObject;
    KEVENT         ScanCompleteNotification;
    LIST_ENTRY     List;
    PAV_SECTION_CONTEXT  SectionContext;
    LONGLONG       ScanId;
    ULONG          ScanThreadId;
    
    UCHAR          IOMajorFunctionAtScan;
    BOOLEAN        IsFileInTxWriter;
    BOOLEAN        IoWaitOnScanCompleteNotificationAborted;

} AV_SCAN_CONTEXT, *PAV_SCAN_CONTEXT;

//
//  The global variable
//

typedef struct _AV_SCANNER_GLOBAL_DATA {

    //
    //  A counter for Scan Id
    //
    
    LONGLONG ScanIdCounter;

    //
    //  The global FLT_FILTER pointer. Many API needs this, such as 
    //  FltAllocateContext(...)
    //

    PFLT_FILTER Filter;
    
    //
    //  Server-side communicate ports.
    //
    
    PFLT_PORT ScanServerPort;
    PFLT_PORT AbortServerPort;
    PFLT_PORT QueryServerPort;
    
    //
    //  The scan client ports.
    //  These ports are assigned at AvConnectNotifyCallback and cleaned at AvDisconnectNotifyCallback
    //
    //  ScanClientPort is the connection port regarding the scan message.
    //  AbortClientPort is the connection port regarding the abort message.
    //  QueryClient is the connection port regarding the query command.
    //
    
    PFLT_PORT ScanClientPort;
    PFLT_PORT AbortClientPort;
    PFLT_PORT QueryClientPort;
    
    //
    //  Scan context list head. 
    //  At AvMessageNotifyCallback, when user passes ScanCtxId, we 
    //  have to check the validity of the id by checking this list.
    //
    
    LIST_ENTRY ScanCtxListHead;
    
    //
    //  The lock that synchronizes the accesses of the scan context list above.
    //
    
    ERESOURCE ScanCtxListLock;

    //
    //  Timeout for local file scans in milliseconds
    //
    
    LONGLONG LocalScanTimeout;

    //
    //  Timeout for network file scans in milliseconds
    //
    
    LONGLONG NetworkScanTimeout;

#if DBG

    //
    // Field to control nature of debug output
    //
    
    ULONG DebugLevel;
#endif
    
    //
    //  A flag that indicating that the filter is being unloaded.
    //    
    
    BOOLEAN  Unloading;

} AV_SCANNER_GLOBAL_DATA, *PAV_SCANNER_GLOBAL_DATA;

AV_SCANNER_GLOBAL_DATA Globals;

#if DBG

//
//  Debugging level flags.
//

#define AVDBG_TRACE_ROUTINES            0x00000001
#define AVDBG_TRACE_OPERATION_STATUS    0x00000002
#define AVDBG_TRACE_DEBUG               0x00000004
#define AVDBG_TRACE_ERROR               0x00000008

#define AV_DBG_PRINT( _dbgLevel, _string )          \
    if(FlagOn(Globals.DebugLevel,(_dbgLevel))) {    \
        DbgPrint _string;                           \
    }

#else

#define AV_DBG_PRINT(_dbgLevel, _string)            {NOTHING;}

#endif

FORCEINLINE
VOID
AvCancelFileOpen(
    _Inout_ PFLT_CALLBACK_DATA Data,
    _In_ PCFLT_RELATED_OBJECTS FltObjects,
    _In_ NTSTATUS Status
    )
/*++

Routine Description:

    This function cancel the file open. This is supposed to be called at post create if 
    the I/O is cancelled.
    
Arguments:

    Data - Pointer to the filter callbackData that is passed to us.
    
    FltObjects - Pointer to the FLT_RELATED_OBJECTS data structure containing
        opaque handles to this filter, instance, its associated volume and
        file object.

    Status - The status code to be returned for this IRP.
    
Return Value:

    None.

--*/

{
    FltCancelFileOpen( FltObjects->Instance, FltObjects->FileObject );
    Data->IoStatus.Status = Status;
    Data->IoStatus.Information = 0;
}

        
NTSTATUS
AvPrepareServerPort( 
    _In_  PSECURITY_DESCRIPTOR SecurityDescriptor,
    _In_  AVSCAN_CONNECTION_TYPE  ConnectionType
    );

NTSTATUS
AvSendAbortToUser ( 
    _In_  ULONG  ScanThreadId,
    _In_  LONGLONG  ScanId    
    );

NTSTATUS
AvAllocateScanContext(
    _In_     PFLT_INSTANCE Instance,
    _In_     PFILE_OBJECT FileObject,
    _Outptr_ PAV_SCAN_CONTEXT *ScanContext
    );

NTSTATUS
AvReferenceScanContext(
    _In_ PAV_SCAN_CONTEXT ScanContext
    );

NTSTATUS
AvReleaseScanContext(
    _In_ PAV_SCAN_CONTEXT ScanContext
    );

//
//  Fianlize function for scan context and section context.
//  Wrapper functions of synchronization calling sequences.
//  In the normal cases, the caller should call AvFinalizeScanAndSection
//  when it finishes using it.
//
//  Unless the caller wants to do things about section context inside scan context,
//  then it should call AvFinalizeScanContext(), and followed by 
//  AvFinalizeSectionContext()
//
//  These wrappers are designed to make the synchronization easier.
//
NTSTATUS
AvFinalizeScanAndSection (
    _Inout_  PAV_SCAN_CONTEXT ScanContext
    );

NTSTATUS
AvFinalizeSectionContext (
    _Inout_  PAV_SECTION_CONTEXT SectionContext
    );

VOID
AvFinalizeScanContext (
    _Inout_  PAV_SCAN_CONTEXT ScanContext,
    _Outptr_result_maybenull_ PAV_SECTION_CONTEXT *SectionContext
    );

#endif

