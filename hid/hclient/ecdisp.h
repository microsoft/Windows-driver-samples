/*++

Copyright (c) Microsoft 1998, All Rights Reserved

Module Name:

    ecdisp.h

Abstract:

    This module contains the public declarations for the extended calls dialog
    box.

Environment:

    User mode

Revision History:

    May-98 : Created 

--*/

#ifndef _ECDISP_H_
#define _ECDISP_H_

#if _MSC_VER >= 1200
#pragma warning(push)
#endif
#pragma warning(disable:4201) // nameless struct/union

typedef struct {
    HANDLE                    DeviceHandle;
    HIDP_REPORT_TYPE          ReportType;
    PHIDP_PREPARSED_DATA      Ppd;
    USAGE                     UsagePage;
    USAGE                     Usage;
    USHORT                    LinkCollection;
    UCHAR                     ReportID;
    PCHAR                     ReportBuffer;
    ULONG                     ReportLength;
    PVOID                     List;
    ULONG                     ListLength;
    ULONG                     Index;
    union {              
        struct {
            USHORT            ReportCount;
            USHORT            BitSize;
        };

        struct {
            PUSAGE            List2;
            PUSAGE            MakeList;
            PUSAGE            BreakList;
        };

        PHIDP_PREPARSED_DATA *ppPd;
        ULONG                 Value;
        LONG                  ScaledValue;
    };
} EXTCALL_PARAMS, *PEXTCALL_PARAMS;

typedef struct {
    BOOL                IsHidError;
    NTSTATUS            HidErrorCode;
    
} EXTCALL_STATUS, *PEXTCALL_STATUS;


/*****************************************************************************
/* Global Extended Call display function declarations
/*****************************************************************************/

LRESULT CALLBACK
bExtCallDlgProc(
    HWND   hDlg,
    UINT   message,
    WPARAM wParam, 
    LPARAM lParam
);

#if _MSC_VER >= 1200
#pragma warning(pop)
#else
#pragma warning(default:4201)
#endif

#endif
