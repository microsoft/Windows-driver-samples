/*++

Copyright (c) Microsoft 1998, All Rights Reserved

Module Name:

    hclient.h

Abstract:

    This module contains the public declarations and definitions that are
    defined in hclient.c and available to other modules outside of it.
              
Environment:

    User mode

Revision History:

    Nov-97 : Created 

--*/

#ifndef __HCLIENT_H__
#define __HCLIENT_H__

#define WM_UNREGISTER_HANDLE    WM_USER+1
#define WM_DISPLAY_READ_DATA    WM_USER+2
#define WM_READ_DONE            WM_USER+3

#define READ_THREAD_TIMEOUT     1000

#define HCLIENT_ERROR           "HClient Error"

#define INFINITE_READS           ((ULONG)-1)

typedef struct _READ_THREAD_CONTEXT 
{
    PHID_DEVICE HidDevice;

    HWND        DisplayWindow;
    HANDLE      DisplayEvent;

    ULONG       NumberOfReads;
    BOOL        TerminateThread;

} READ_THREAD_CONTEXT, *PREAD_THREAD_CONTEXT;


typedef NTSTATUS __stdcall GETEXTATTRIB (
    HIDP_REPORT_TYPE,
    USHORT,
    PHIDP_PREPARSED_DATA,
    PHIDP_EXTENDED_ATTRIBUTES,
    PULONG);

typedef GETEXTATTRIB *PGETEXTATTRIB;

typedef NTSTATUS __stdcall INITREPORT (
    HIDP_REPORT_TYPE,
    UCHAR,
    PHIDP_PREPARSED_DATA,
    PCHAR,
    ULONG);

typedef INITREPORT *PINITREPORT;


extern PGETEXTATTRIB pfnHidP_GetExtendedAttributes;

extern PINITREPORT   pfnHidP_InitializeReportForID;

VOID
vDisplayDeviceAttributes(
    IN PHIDD_ATTRIBUTES pAttrib,
    IN HWND             hControl
);

VOID
vDisplayButtonAttributes(
    IN PHIDP_BUTTON_CAPS pButton,
    IN HWND              hControl
);

VOID
vDisplayDataAttributes(
    PHIDP_DATA pData, 
    BOOL IsButton, 
    HWND hControl
);

VOID
vCreateUsageAndPageString(
    _In_  PUSAGE_AND_PAGE        pUsageList,
    _Out_writes_(uiStringSize) CHAR szString[],
    _In_  UINT                   uiStringSize
);

VOID
vCreateUsageString(
    _In_  PUSAGE        pUsageList,
    _Out_writes_(uiStringSize) CHAR szString[],
    _In_  UINT                   uiStringSize
);

VOID
vDisplayDeviceCaps(
    IN PHIDP_CAPS pCaps,
    IN HWND       hControl
);

VOID 
vDisplayValueAttributes(
    IN PHIDP_VALUE_CAPS pValue,
    IN HWND             hControl
);

VOID
vDisplayLinkCollectionNode(
    IN  PHIDP_LINK_COLLECTION_NODE  pLCNode,
    IN  ULONG                       ulLinkIndex,
    IN  HWND                        hControl
);

VOID
vCreateUsageValueStringFromArray(
    _In_reads_(uiBufferSize)PCHAR       pBuffer,
    _In_ UINT        uiBufferSize,
    _In_ USHORT      BitSize,
    _In_ USHORT      UsageIndex,
    _Out_writes_(uiStringSize)  CHAR        szString[],
    _In_  UINT       uiStringSize
);

VOID 
vDisplayValueAttributes(
    IN PHIDP_VALUE_CAPS pValue,
    IN HWND hControl
);

DWORD WINAPI
AsynchReadThreadProc(
    PREAD_THREAD_CONTEXT    Context
);

DWORD WINAPI
SynchReadThreadProc(
    PREAD_THREAD_CONTEXT    Context
);

#endif


