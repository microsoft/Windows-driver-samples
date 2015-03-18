/*++

Copyright (c) 2011  Microsoft Corporation

Module Name:

    scan.h

Abstract:

    This module contains the scan interface for AV filter to call.

Environment:

    Kernel mode

--*/
#ifndef __SCAN_H__
#define __SCAN_H__

#include "avlib.h"

typedef enum _AV_SCAN_MODE {

    //
    //  AvKernelMode indicates the scanning occurs in the kernel, while
    //  AvUserMode indicates the scanning happens in the user space.
    //
    
    AvKernelMode,    
    AvUserMode

} AV_SCAN_MODE;

NTSTATUS
AvScanInKernel (
    _In_ PCFLT_RELATED_OBJECTS FltObjects,
    _In_ UCHAR IOMajorFunctionAtScan,
    _In_ BOOLEAN IsInTxWriter,
    _In_ PAV_STREAM_CONTEXT StreamContext
    );
    
NTSTATUS
AvScanInUser (
    _Inout_ PFLT_CALLBACK_DATA Data,
    _In_ PCFLT_RELATED_OBJECTS FltObjects,
    _In_ UCHAR IOMajorFunctionAtScan,
    _In_ BOOLEAN IsInTxWriter,
    _In_ DEVICE_TYPE DeviceType
    );
    
NTSTATUS
AvCreateSectionForDataScan (
    _In_    PFLT_INSTANCE Instance,
    _In_    PFILE_OBJECT FileObject,
    _Inout_ PAV_SECTION_CONTEXT SectionContext
    );
    
NTSTATUS
AvCloseSectionForDataScan( 
    _Inout_ PAV_SECTION_CONTEXT SectionContext
    );

#endif

