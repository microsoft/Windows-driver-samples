/*++

Copyright (c) Microsoft 1998, All Rights Reserved

Module Name:

    buffers.h

Abstract:

    This module declares the functions that and structures that are visible
    to other modules.

Environment:

    User mode

Revision History:

    May-98 : Created 

--*/

#ifndef _BUFFERS_H_
#define _BUFFERS_H_

typedef struct _REPORT_BUFFER
{
    _Field_size_(iBufferSize) 
    PUCHAR      pBuffer;
    UINT        iBufferSize;
    UCHAR       ucReportID;

} REPORT_BUFFER, *PREPORT_BUFFER;
    
typedef struct _BUFFER_DISPLAY
{
    HWND                hBufferComboBox;
    HWND                hBufferEditBox;
    UINT                 iBufferSize;
    UINT                 iCurrSelectionIndex;
    UINT                 nReportBuffers;
    _Field_size_(nReportBuffers) 
    PREPORT_BUFFER      ReportBuffers;
    HIDP_REPORT_TYPE    ReportType;
} BUFFER_DISPLAY, *PBUFFER_DISPLAY;

BOOLEAN
BufferDisplay_Init(
    IN  HWND                hCB,
    IN  HWND                hEB,
    IN  INT                 nBuffers,
    IN  INT                 iBufferSize,
    IN  HIDP_REPORT_TYPE    RType,
    _Outptr_result_maybenull_ PBUFFER_DISPLAY *ppBufferDisplay
);

VOID
BufferDisplay_Destroy(
    IN  PBUFFER_DISPLAY     pBufferDisplay
);

VOID
BufferDisplay_ChangeSelection(
    IN  PBUFFER_DISPLAY     pBufferDisplay
);

BOOLEAN
BufferDisplay_UpdateBuffer(
    _In_ PBUFFER_DISPLAY     pBufferDisplay,
    _In_reads_bytes_(ulNewBufferLength) PCHAR pNewBuffer,
    _In_ ULONG              ulNewBufferLength
);

INT
BufferDisplay_GetBufferSize(
    IN  PBUFFER_DISPLAY      pBufferDisplay
);

VOID
BufferDisplay_CopyCurrentBuffer(
    _In_ PBUFFER_DISPLAY     pBufferDisplay,
    _Out_writes_bytes_(ulCopyBufferLength) PCHAR pCopyBuffer,
    _In_ ULONG              ulCopyBufferLength
);

INT
BufferDisplay_GetCurrentBufferNumber(
    IN  PBUFFER_DISPLAY      pBufferDisplay
);

UCHAR
BufferDisplay_GetCurrentReportID(
    IN  PBUFFER_DISPLAY      pBufferDisplay
);

VOID
BufferDisplay_OutputBuffer(
    HWND            hEditBox,
    PREPORT_BUFFER  pReportBuffer
);

VOID
BufferDisplay_ClearBuffer(
    IN  PBUFFER_DISPLAY pBufferDisplay
);

#endif
