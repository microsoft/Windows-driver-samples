/*++

Copyright (c) Microsoft 1998, All Rights Reserved

Module Name:

    ecdisp.c

Abstract:

    This module contains the code to handle the extended calls dialog box
    and the actions that can be performed in the dialog box.

Environment:

    User mode

Revision History:

    May-98 : Created 

--*/

/*****************************************************************************
/* Extended call display include files
/*****************************************************************************/
#include <windows.h>
#include <limits.h>
#include <stdlib.h>
#include <math.h>
#include <stdio.h>
#include <setupapi.h>
#include <vfw.h>
#include <assert.h>
#include <strsafe.h>
#include "hidusage.h"
#include "hidsdi.h"
#include "hid.h"
#include "hclient.h"
#include "resource.h"
#include "buffers.h"
#include "ecdisp.h"
#include "strings.h"

/*****************************************************************************
/* Local macro definitions for the supported function calls
/*****************************************************************************/

#pragma warning(disable:4057) // indirection to slightly different base types
#pragma warning(disable:4201)
#pragma warning(disable:28146) // Warning is meant for kernel mode drivers 
#pragma prefast(disable:38030, "Samples are not localized")

#define HID_DEVCALLS                    20
#define HID_PPDCALLS                    29

#define HID_NUMCALLS                    HID_DEVCALLS + HID_PPDCALLS

#define HIDD_GET_HID_GUID              1
#define HIDD_GET_FREE_PREPARSED_DATA   2
#define HIDD_GET_CONFIGURATION         3
#define HIDD_SET_CONFIGURATION         4
#define HIDD_FLUSH_QUEUE               5
#define HIDD_GET_ATTRIBUTES            6
#define HIDD_SET_FEATURE               7
#define HIDD_GET_FEATURE               8
#define HIDD_GET_INPUT_REPORT          9
#define HIDD_SET_OUTPUT_REPORT        10
#define HIDD_GET_NUM_INPUT_BUFFERS    11
#define HIDD_SET_NUM_INPUT_BUFFERS    12
#define HIDD_GET_PHYSICAL_DESCRIPTOR  13
#define HIDD_GET_MANUFACTURER_STRING  14
#define HIDD_GET_PRODUCT_STRING       15
#define HIDD_GET_INDEXED_STRING       16
#define HIDD_GET_SERIAL_NUMBER_STRING 17
#define HIDD_GET_MS_GENRE_DESCRIPTOR  18
#define HID_READ_REPORT               19
#define HID_WRITE_REPORT              20
#define HIDP_GET_BUTTON_CAPS          21
#define HIDP_GET_BUTTONS              22
#define HIDP_GET_BUTTONS_EX           23
#define HIDP_GET_CAPS                 24 
#define HIDP_GET_DATA                 25
#define HIDP_GET_EXTENDED_ATTRIBUTES  26
#define HIDP_GET_LINK_COLL_NODES      27
#define HIDP_GET_SCALED_USAGE_VALUE   28
#define HIDP_GET_SPECIFIC_BUTTON_CAPS 29
#define HIDP_GET_SPECIFIC_VALUE_CAPS  30
#define HIDP_GET_USAGES               31
#define HIDP_GET_USAGES_EX            32
#define HIDP_GET_USAGE_VALUE          33
#define HIDP_GET_USAGE_VALUE_ARRAY    34
#define HIDP_GET_VALUE_CAPS           35
#define HIDP_INITIALIZE_REPORT_FOR_ID 36
#define HIDP_MAX_DATA_LIST_LENGTH     37
#define HIDP_MAX_USAGE_LIST_LENGTH    38
#define HIDP_SET_BUTTONS              39
#define HIDP_SET_DATA                 40
#define HIDP_SET_SCALED_USAGE_VALUE   41
#define HIDP_SET_USAGES               42
#define HIDP_SET_USAGE_VALUE          43
#define HIDP_SET_USAGE_VALUE_ARRAY    44
#define HIDP_TRANSLATE_USAGES         45
#define HIDP_UNSET_BUTTONS            46
#define HIDP_UNSET_USAGES             47
#define HIDP_USAGE_LIST_DIFFERENCE    48
#define HID_CLEAR_REPORT              49

/*
// These two definitions are not used by the display routines since 
//    the two functions were molded together into one for purpose of execution
*/

#define HIDD_GET_PREPARSED_DATA       50
#define HIDD_FREE_PREPARSED_DATA      51


#define IS_HIDD_FUNCTION(func)        (((func) >= HIDD_GET_HID_GUID) && \
                                       ((func) <= HIDD_GET_MS_GENRE_DESCRIPTOR))


#define IS_HIDP_FUNCTION(func)        (((func) >= HIDP_GET_BUTTON_CAPS) && \
                                       ((func) <= HIDP_USAGE_LIST_DIFFERENCE))

#define IS_HID_FUNCTION(func)         (((func) >= HID_READ_REPORT) && \
                                       ((func) <= HID_WRITE_REPORT))

#define IS_NOT_IMPLEMENTED(func)      (((func) == HIDD_GET_CONFIGURATION) || \
                                      ((func) == HIDD_SET_CONFIGURATION) || \
                                      ((func) == HIDP_TRANSLATE_USAGES) || \
                                      (((func) == HIDP_INITIALIZE_REPORT_FOR_ID) && \
                                       (NULL == pfnHidP_InitializeReportForID)) || \
                                      (((func) == HIDP_GET_EXTENDED_ATTRIBUTES) && \
                                       (NULL == pfnHidP_GetExtendedAttributes)))

/*****************************************************************************
/* Local macro definitions for buffer display sizes
/*****************************************************************************/

#define NUM_INPUT_BUFFERS       16
#define NUM_OUTPUT_BUFFERS      16
#define NUM_FEATURE_BUFFERS     16

/*****************************************************************************
/* Local macro definition for HidP_SetData dialog box
/*****************************************************************************/

#define SETDATA_LISTBOX_FORMAT  "Index: %u,  DataValue: %u"

/*****************************************************************************
/* Local macro definition for display output to output windows
/*****************************************************************************/

#define TEMP_BUFFER_SIZE 1024
#define OUTSTRING(win, str)         SendMessage(win, LB_ADDSTRING, 0, (LPARAM) str)
#define OUTWSTRING(win, str) \
{ \
    SIZE_T  nBytes; \
\
    errno_t result = wcstombs_s(&nBytes, szTempBuffer, TEMP_BUFFER_SIZE, str, TEMP_BUFFER_SIZE-1); \
    if (0 != result) { \
        OUTSTRING(win, "Cannot convert wide-character string"); \
    } \
    else { \
        szTempBuffer[nBytes] = '\0'; \
        OUTSTRING(win, szTempBuffer); \
    } \
}

#define DISPLAY_HIDD_STATUS(win, func, status, strret) \
{ \
    strret = StringCbPrintf(szTempBuffer, \
                   TEMP_BUFFER_SIZE,  \
                   "%s returned: %s", \
                   func, \
                   (status).IsHidError ? "FALSE" : "TRUE"); \
\
    OUTSTRING(win, szTempBuffer); \
\
    if ((status).IsHidError) { \
        strret = StringCbPrintf(szTempBuffer, \
                       TEMP_BUFFER_SIZE, \
                       "ErrorCode: %d", \
                       GetLastError()); \
    }\
    OUTSTRING(win, szTempBuffer); \
}

#define DISPLAY_HIDP_STATUS(win, func, status, strret) \
{ \
    int szTempBuffer_size = sizeof(szTempBuffer); \
    strret = StringCbPrintf(szTempBuffer, \
                   szTempBuffer_size, \
                   "%s returned: %s", \
                   func, \
                   ECDisp_GetHidAppStatusString(status.HidErrorCode)); \
\
    if(SUCCEEDED(strret)) \
    { \
        OUTSTRING(win, szTempBuffer); \
    } \
    else \
    { \
        OUTSTRING(win, "ERROR: StringCbPrintf failed"); \
    } \
}

#define ECDISP_ERROR(win, msg) \
{ \
    MessageBox(win, \
               msg, \
               HCLIENT_ERROR, \
               MB_ICONEXCLAMATION); \
}

#define GET_FUNCTION_NAME(index)     ResolveFunctionName(index)


/*****************************************************************************
/* Local macro definition for retrieving data based on report type
/*****************************************************************************/
#define SELECT_ON_REPORT_TYPE(rt, ival, oval, fval, res) \
{ \
    switch ((rt)) { \
    case HidP_Input: \
        (res) = (ival); \
        break; \
\
    case HidP_Output: \
        (res) = (oval); \
        break; \
\
    case HidP_Feature: \
        (res) = (fval); \
        break; \
\
    } \
}

/*****************************************************************************
/* Local macro definition for calculating size of a usage value array buffer
/*****************************************************************************/
#define ROUND_TO_NEAREST_BYTE(val)  (((val) % 8) ? ((val) / 8) + 1 : ((val) / 8))

/*****************************************************************************
/* Data types local to this module
/*****************************************************************************/

typedef struct _FUNCTION_NAMES
{
    UINT uiIndex;
    char *szFunctionName;
} FUNCTION_NAMES;

typedef struct _PARAMETER_STATE 
{
    BOOL fInputReport;
    BOOL fOutputReport;
    BOOL fFeatureReport;
    BOOL fReportID;
    BOOL fUsagePage;
    BOOL fUsage;
    BOOL fLinkCollection;
    BOOL fInputReportSelect;
    BOOL fOutputReportSelect;
    BOOL fFeatureReportSelect;
} PARAMETER_STATE;

typedef enum { DLGBOX_INIT_FAILED = -1, DLGBOX_ERROR, DLGBOX_CANCEL, DLGBOX_OK } DLGBOX_STATUS;

typedef struct _ECDISPLAY_PARAMS 
{
    HIDP_REPORT_TYPE          ReportType;
    USAGE                     UsagePage;
    USAGE                     Usage;
    USHORT                    LinkCollection;
    UCHAR                     ReportID;
    _Field_size_(ListLength+1) 
    LPSTR                     szListString;
    _Field_size_(ListLength2+1) 
    LPSTR                     szListString2;
    _Field_size_(ListLength+1) 
    PUSAGE                    UsageList;
    _Field_size_(ListLength2+1) 
    PUSAGE                    UsageList2;
    ULONG                     ListLength;
    ULONG                     ListLength2;
    ULONG                     Index;
    union {
        PHIDP_DATA            pDataList;
        PULONG                pValueList;
        LONG                  ScaledValue;
        ULONG                 Value;
    };
} ECDISPLAY_PARAMS, *PECDISPLAY_PARAMS;

typedef struct _READ_PARAMS
{
    PHID_DEVICE device;
    BOOLEAN     stopThread;
} READ_PARAMS, *PREAD_PARAMS;
    
/*****************************************************************************
/* Local data variables
/*****************************************************************************/

static CHAR             szTempBuffer[TEMP_BUFFER_SIZE];

static PBUFFER_DISPLAY  pInputDisplay;
static PBUFFER_DISPLAY  pOutputDisplay;
static PBUFFER_DISPLAY  pFeatureDisplay;

static FUNCTION_NAMES DeviceCalls[HID_DEVCALLS] = {
                             { HIDD_GET_HID_GUID,               "HidD_GetHidGuid" },
                             { HIDD_GET_FREE_PREPARSED_DATA,    "HidD_GetFreePreparsedData" },
                             { HIDD_GET_CONFIGURATION,          "HidD_GetConfiguration" },
                             { HIDD_SET_CONFIGURATION,          "HidD_SetConfiguration" },
                             { HIDD_FLUSH_QUEUE,                "HidD_FlushQueue" },
                             { HIDD_GET_ATTRIBUTES,             "HidD_GetAttributes" },
                             { HIDD_SET_FEATURE,                "HidD_SetFeature" },
                             { HIDD_GET_FEATURE,                "HidD_GetFeature" },
                             { HIDD_GET_INPUT_REPORT,           "HIDD_GetInputReport" },
                             { HIDD_SET_OUTPUT_REPORT,          "HidD_SetOutputReport" },
                             { HIDD_GET_NUM_INPUT_BUFFERS,      "HidD_GetNumInputBuffers" },
                             { HIDD_SET_NUM_INPUT_BUFFERS,      "HidD_SetNumInputBuffers" },
                             { HIDD_GET_PHYSICAL_DESCRIPTOR,    "HidD_GetPhysicalDescriptor" },
                             { HIDD_GET_MANUFACTURER_STRING,    "HidD_GetManufacturerString" },
                             { HIDD_GET_PRODUCT_STRING,         "HidD_GetProductString" },
                             { HIDD_GET_INDEXED_STRING,         "HidD_GetIndexedString" },
                             { HIDD_GET_SERIAL_NUMBER_STRING,   "HidD_GetSerialNumberString" },
                             { HIDD_GET_MS_GENRE_DESCRIPTOR,    "HidD_GetMsGenreDescriptor" },
                             { HID_READ_REPORT,                 "Read Input Report"        },
                             { HID_WRITE_REPORT,                "Write Report Buffer"      }

};

static FUNCTION_NAMES PpdCalls[HID_PPDCALLS] = {
                             { HIDP_GET_BUTTON_CAPS,            "HidP_GetButtonCaps" },
                             { HIDP_GET_BUTTONS,                "HidP_GetButtons" },
                             { HIDP_GET_BUTTONS_EX,             "HidP_GetButtonsEx" },
                             { HIDP_GET_CAPS,                   "HidP_GetCaps" },
                             { HIDP_GET_DATA,                   "HidP_GetData" },
                             { HIDP_GET_EXTENDED_ATTRIBUTES,    "HidP_GetExtendedAttributes" },
                             { HIDP_GET_LINK_COLL_NODES,        "HidP_GetLinkCollectionNodes" },
                             { HIDP_GET_SCALED_USAGE_VALUE,     "HidP_GetScaledUsageValue" },
                             { HIDP_GET_SPECIFIC_BUTTON_CAPS,   "HidP_GetSpecificButtonCaps" },
                             { HIDP_GET_SPECIFIC_VALUE_CAPS,    "HidP_GetSpecificValueCaps" },
                             { HIDP_GET_USAGES,                 "HidP_GetUsages" },
                             { HIDP_GET_USAGES_EX,              "HidP_GetUsagesEx" },
                             { HIDP_GET_USAGE_VALUE,            "HidP_GetUsageValue" },
                             { HIDP_GET_USAGE_VALUE_ARRAY,      "HidP_GetUsageValueArray" },
                             { HIDP_GET_VALUE_CAPS,             "HidP_GetValueCaps" },
                             { HIDP_INITIALIZE_REPORT_FOR_ID,   "HidP_InitializeReportForID" },
                             { HIDP_MAX_DATA_LIST_LENGTH,       "HidP_MaxDataListLength" },
                             { HIDP_MAX_USAGE_LIST_LENGTH,      "HidP_MaxUsageListLength" },
                             { HIDP_SET_BUTTONS,                "HidP_SetButtons" },
                             { HIDP_SET_DATA,                   "HidP_SetData" },
                             { HIDP_SET_SCALED_USAGE_VALUE,     "HidP_SetScaledUsageValue" },
                             { HIDP_SET_USAGES,                 "HidP_SetUsages" },
                             { HIDP_SET_USAGE_VALUE,            "HidP_SetUsageValue" },
                             { HIDP_SET_USAGE_VALUE_ARRAY,      "HidP_SetUsageValueArray" },
                             { HIDP_TRANSLATE_USAGES,           "HidP_TranslateUsagesToI8042ScanCodes" },
                             { HIDP_UNSET_BUTTONS,              "HidP_UnsetButtons" },
                             { HIDP_UNSET_USAGES,               "HidP_UnsetUsages" },
                             { HIDP_USAGE_LIST_DIFFERENCE,      "HidP_UsageListDifference" },
                             { HID_CLEAR_REPORT,                "Clear Report Buffer"      }
};

static PARAMETER_STATE pState[HID_NUMCALLS] = {
                                         { FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE }, // HIDD_GET_HID_GUID
                                         { FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE }, // HIDD_GET_FREE_PREPARSED_DATA
                                         { FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE }, // HIDD_GET_CONFIGURATION
                                         { FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE }, // HIDD_SET_CONFIGURATION
                                         { FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE }, // HIDD_FLUSH_QUEUE
                                         { FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE }, // HIDD_GETATTRIBUTES
                                         { FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE,  TRUE }, // HIDD_SET_FEATURE
                                         { FALSE, FALSE, FALSE,  TRUE, FALSE, FALSE, FALSE, FALSE, FALSE,  TRUE }, // HIDD_GET_FEATURE
                                         { FALSE, FALSE, FALSE,  TRUE, FALSE, FALSE, FALSE,  TRUE, FALSE, FALSE }, // HIDD_GET_INPUT_REPORT
                                         { FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE,  TRUE, FALSE }, // HIDD_SET_OUTPUT_REPORT
                                         { FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE }, // HIDD_GET_NUM_INPUT_BUFFERS
                                         { FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE }, // HIDD_SET_NUM_INPUT_BUFFERS
                                         { FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE }, // HIDD_GET_PHYSICAL_DESCRIPTOR
                                         { FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE }, // HIDD_GET_MANUFACTURER_STRING
                                         { FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE }, // HIDD_GET_PRODUCT_STRING
                                         { FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE }, // HIDD_GET_INDEXED_STRING
                                         { FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE }, // HIDD_GET_SERIAL_NUMBER_STRING
                                         { FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE }, // HIDD_GET_MS_GENRE_DESCRIPTOR
                                         { FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE,  TRUE, FALSE, FALSE }, // HID_READ_REPORT
                                         { FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE,  TRUE, FALSE }, // HID_WRITE_BUFFER
                                         {  TRUE,  TRUE,  TRUE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE }, // HIDP_GET_BUTTON_CAPS
                                         {  TRUE,  TRUE,  TRUE, FALSE,  TRUE, FALSE,  TRUE,  TRUE,  TRUE,  TRUE }, // HIDP_GET_BUTTONS
                                         {  TRUE,  TRUE,  TRUE, FALSE, FALSE, FALSE,  TRUE,  TRUE,  TRUE,  TRUE }, // HIDP_GET_BUTTONS_EX
                                         { FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE }, // HIDP_GET_CAPS
                                         {  TRUE,  TRUE,  TRUE, FALSE, FALSE, FALSE, FALSE,  TRUE,  TRUE,  TRUE }, // HIDP_GET_DATA
                                         {  TRUE,  TRUE,  TRUE,  TRUE, FALSE, FALSE, FALSE,  TRUE,  TRUE,  TRUE }, // HIDP_GET_EXTENDED_ATTRIBUTES 
                                         { FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE }, // HIDP_GET_LINK_COLL_NODES
                                         {  TRUE,  TRUE,  TRUE, FALSE,  TRUE,  TRUE,  TRUE,  TRUE,  TRUE,  TRUE }, // HIDP_GET_SCALED_USAGE_VALUE
                                         {  TRUE,  TRUE,  TRUE, FALSE,  TRUE,  TRUE,  TRUE, FALSE, FALSE, FALSE }, // HIDP_GET_SPECIFIC_BUTTON_CAPS
                                         {  TRUE,  TRUE,  TRUE, FALSE,  TRUE,  TRUE,  TRUE, FALSE, FALSE, FALSE }, // HIDP_GET_SPECIFIC_VALUE_CAPS
                                         {  TRUE,  TRUE,  TRUE, FALSE,  TRUE, FALSE,  TRUE,  TRUE,  TRUE,  TRUE }, // HIDP_GET_USAGES
                                         {  TRUE,  TRUE,  TRUE, FALSE, FALSE, FALSE,  TRUE,  TRUE,  TRUE,  TRUE }, // HIDP_GET_USAGES_EX
                                         {  TRUE,  TRUE,  TRUE, FALSE,  TRUE,  TRUE,  TRUE,  TRUE,  TRUE,  TRUE }, // HIDP_GET_USAGE_VALUE
                                         {  TRUE,  TRUE,  TRUE, FALSE,  TRUE,  TRUE,  TRUE,  TRUE,  TRUE,  TRUE }, // HIDP_GET_USAGE_VALUE_ARRAY
                                         {  TRUE,  TRUE,  TRUE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE }, // HIDP_GET_VALUE_CAPS
                                         {  TRUE,  TRUE,  TRUE,  TRUE, FALSE, FALSE, FALSE,  TRUE,  TRUE,  TRUE }, // HIDP_INITIALIZE_REPORT_FOR_ID
                                         {  TRUE,  TRUE,  TRUE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE }, // HIDP_MAX_DATA_LIST_LENGTH
                                         {  TRUE,  TRUE,  TRUE, FALSE,  TRUE, FALSE, FALSE, FALSE, FALSE, FALSE }, // HIDP_MAX_USAGE_LIST_LENGTH
                                         {  TRUE,  TRUE,  TRUE, FALSE,  TRUE, FALSE,  TRUE,  TRUE,  TRUE,  TRUE }, // HIDP_SET_BUTTONS
                                         {  TRUE,  TRUE,  TRUE, FALSE, FALSE, FALSE, FALSE,  TRUE,  TRUE,  TRUE }, // HIDP_SET_DATA
                                         {  TRUE,  TRUE,  TRUE, FALSE,  TRUE,  TRUE,  TRUE,  TRUE,  TRUE,  TRUE }, // HIDP_SET_SCALED_USAGE_VALUE
                                         {  TRUE,  TRUE,  TRUE, FALSE,  TRUE, FALSE,  TRUE,  TRUE,  TRUE,  TRUE }, // HIDP_SET_USAGES                                        
                                         {  TRUE,  TRUE,  TRUE, FALSE,  TRUE,  TRUE,  TRUE,  TRUE,  TRUE,  TRUE }, // HIDP_SET_USAGE_VALUE
                                         {  TRUE,  TRUE,  TRUE, FALSE,  TRUE,  TRUE,  TRUE,  TRUE,  TRUE,  TRUE }, // HIDP_SET_USAGE_VALUE_ARRAY
                                         { FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE }, // HIDP_TRANSLATE_USAGES
                                         {  TRUE,  TRUE,  TRUE, FALSE,  TRUE, FALSE,  TRUE,  TRUE,  TRUE,  TRUE }, // HIDP_UNSET_BUTTONS
                                         {  TRUE,  TRUE,  TRUE, FALSE,  TRUE, FALSE,  TRUE,  TRUE,  TRUE,  TRUE }, // HIDP_UNSET_USAGES
                                         { FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE }, // HIDP_USAGE_LIST_DIFFERENCE
                                         {  TRUE,  TRUE,  TRUE, FALSE, FALSE, FALSE, FALSE,  TRUE,  TRUE,  TRUE }  // HID_CLEAR_BUFFER
                                        }; 


/*****************************************************************************
/* Local function declarations
/*****************************************************************************/

VOID
vLoadExtCalls(
    HWND hExtCalls
);

VOID
vSetReportType(
    HWND hDlg, 
    LONG lId
);

VOID 
vInitEditText(
    HWND   hText, 
    INT    cbTextSize, 
    _In_ LPSTR pchText
);

VOID vEnableParameters(
    HWND     hDlg, 
    LRESULT  iCallSelection
);

BOOL 
fGetAndVerifyParameters(
    HWND              hDlg, 
    PECDISPLAY_PARAMS pParams
);

BOOL
ECDisp_Execute(
    IN     INT             FuncCall,
    IN OUT PEXTCALL_PARAMS CallParams,
    OUT    PEXTCALL_STATUS CallStatus
);

VOID
ECDisp_DisplayOutput(
    IN HWND            hOutputWindow,
    IN INT             FuncCall,
    IN PEXTCALL_PARAMS Results
);

VOID 
vExecuteAndDisplayOutput(
    HWND              hOutputWindow, 
    PHID_DEVICE       pDevice, 
    INT               iFuncCall, 
    PECDISPLAY_PARAMS pParams
);              

CHAR *pchGetHidAppStatusString(
    NTSTATUS StatusCode
);

VOID
vInitECControls(
    HWND            hDlg,
    USHORT          InputReportByteLength,
    PBUFFER_DISPLAY *ppInputDisplay,
    USHORT          OutputReportByteLength,
    PBUFFER_DISPLAY *ppOutputDisplay,
    USHORT          FeatureReportByteLength,
    PBUFFER_DISPLAY *ppFeatureDisplay
);

VOID
BuildReportIDList(
    IN  PHIDP_BUTTON_CAPS  phidButtonCaps,
    IN  USHORT             nButtonCaps,
    IN  PHIDP_VALUE_CAPS   phidValueCaps,
    IN  USHORT             nValueCaps,
    OUT UCHAR            **ppReportIDList,
    OUT INT               *nReportIDs
);

LRESULT CALLBACK
bSetUsagesDlgProc(
    HWND hDlg,
    UINT message,
    WPARAM wParam,
    LPARAM lParam
);

LRESULT CALLBACK
bSetValueDlgProc(
    HWND hDlg,
    UINT message,
    WPARAM wParam,
    LPARAM lParam
);

LRESULT CALLBACK
bSetInputBuffDlgProc(
    HWND hDlg,
    UINT message,
    WPARAM wParam,
    LPARAM lParam
);

LRESULT CALLBACK
bSetDataDlgProc(
    HWND hDlg,
    UINT message,
    WPARAM wParam,
    LPARAM lParam
);

LRESULT CALLBACK
bSetBufLenDlgProc(
    HWND hDlg,
    UINT message,
    WPARAM wParam,
    LPARAM lParam
);

LRESULT CALLBACK
bSetInputBuffersDlgProc(
    HWND hDlg,
    UINT message,
    WPARAM wParam,
    LPARAM lParam
);

LRESULT CALLBACK
bGetIndexedDlgProc(
    HWND hDlg,
    UINT message,
    WPARAM wParam,
    LPARAM lParam
);

LRESULT CALLBACK
bGetUsageDiffDlgProc(
    HWND hDlg,
    UINT message,
    WPARAM wParam,
    LPARAM lParam
);

BOOL
ConvertStringToUlongList(
    _Inout_ LPSTR InString,
    OUT    PULONG  *UlongList,
    OUT    PULONG  nUlongs
);

BOOL
ConvertStringToUsageList(
    _In_    LPSTR InString,
    OUT    PUSAGE  *UsageList,
    OUT    PULONG  nUsages
);

VOID
ECDisp_MakeGUIDString(
    IN  GUID guid, 
    _Out_writes_bytes_(uiBuffSize) CHAR szString[],
    IN  UINT uiBuffSize
);

PCHAR
ECDisp_GetHidAppStatusString(
    NTSTATUS StatusCode
);

BOOL
ECDisp_ConvertUlongListToValueList(
    IN  PULONG  UlongList,
    IN  ULONG   nUlongs,
    IN  USHORT  BitSize,
    IN  USHORT  ReportCount,
    _Outptr_result_bytebuffer_(*ValueListSize) PCHAR   *ValueList,
    OUT PULONG  ValueListSize
);

BOOL
SetDlgItemIntHex(
   HWND hDlg, 
   INT nIDDlgItem, 
   UINT uValue, 
   INT nBytes
);

PCHAR
ResolveFunctionName(
    INT Index
);


DWORD WINAPI
ECDisp_ReadThreadProc(
    LPVOID  ThreadContext
);

VOID
DisplayExtendedAttributes(
    IN  HWND                OutputWindow,
    IN  PHIDP_UNKNOWN_TOKEN UnknownList,
    IN  ULONG               UnknownListLength
);

/*****************************************************************************
/* Global function definitions
/*****************************************************************************/

LRESULT CALLBACK
bExtCallDlgProc(
    HWND hDlg,
    UINT message,
    WPARAM wParam,
    LPARAM lParam
)
{
    static PHID_DEVICE          pDevice;
    static CHAR                 szTempBuff[1024]; 
    static CHAR                 szLabel[512];
    static CHAR                 szValue[512];
    static INT                  iLBCounter;
    static UCHAR                *pucInputReportIDs;
    static UCHAR                *pucOutputReportIDs;
    static UCHAR                *pucFeatureReportIDs;
    static INT                  nInputReportIDs;
    static INT                  nOutputReportIDs;
    static INT                  nFeatureReportIDs;
    static HANDLE               ReadThread;
    static READ_THREAD_CONTEXT  readContext;
    static HID_DEVICE           device;
    
           INT                  iIndex;
           ECDISPLAY_PARAMS     params;
           DWORD                threadID;

    ZeroMemory(&params, sizeof(params));

    switch(message)
    {
    case WM_INITDIALOG:
    
        /*
        // Initializing the dialog box involves the following steps:
        //  1) Determine from the parameter the pointer to the selected device
        //  2) Initializing the controls in the dialog box to their initial values
        //  3) Send a message that our list of routines has changed 
        */
    
        pDevice = (PHID_DEVICE) lParam;

        vInitECControls(hDlg,
                        pDevice -> Caps.InputReportByteLength,
                        &pInputDisplay,
                        pDevice -> Caps.OutputReportByteLength,
                        &pOutputDisplay,
                        pDevice -> Caps.FeatureReportByteLength,
                        &pFeatureDisplay);                        

        ReadThread = NULL;
        
        PostMessage(hDlg,
                    WM_COMMAND,
                    IDC_EXTCALLS + (CBN_SELCHANGE << 16),
                    (LPARAM) GetDlgItem(hDlg,IDC_EXTCALLS));
        break; 

    case WM_COMMAND:
        switch(LOWORD(wParam))
        {
        case IDC_EXTCALLS:
            switch (HIWORD(wParam))
            {
            case CBN_SELCHANGE:
                iIndex = (INT) SendDlgItemMessage(hDlg, 
                                                  IDC_EXTCALLS,
                                                  CB_GETCURSEL,
                                                  0,
                                                  0);
                vEnableParameters(hDlg,
                                  SendDlgItemMessage(hDlg, 
                                                     IDC_EXTCALLS,
                                                     CB_GETITEMDATA,
                                                     iIndex,
                                                     0));
                break;
            }
            break;

        case IDC_INPUT_SELECT:
            if (CBN_SELCHANGE == HIWORD(wParam))
            {
                BufferDisplay_ChangeSelection(pInputDisplay);
            }
            break;

        case IDC_OUTPUT_SELECT:
            if (CBN_SELCHANGE == HIWORD(wParam)) 
            {
                 BufferDisplay_ChangeSelection(pOutputDisplay);
            }
            break;

        case IDC_FEATURE_SELECT:
            if (CBN_SELCHANGE == HIWORD(wParam)) 
            {
                 BufferDisplay_ChangeSelection(pFeatureDisplay);
            }
            break;
             
        case IDC_EXECUTE:
            /*
            // Get the parameters and verify that they are all correct
            //   If there is an error, display an error message and
            //   don't continue any further.
            */

            if ( !fGetAndVerifyParameters(hDlg, &params) ) 
            {
                ECDISP_ERROR(hDlg, "Error: One or more parameters are invalid");
            }

            /*
            // Else the parameters are valid and we can execute the call
            */
              
            else
            {
                iIndex = (INT) SendDlgItemMessage(hDlg, IDC_EXTCALLS, CB_GETCURSEL, 0, 0);
                iIndex = (INT) SendDlgItemMessage(hDlg, IDC_EXTCALLS, CB_GETITEMDATA, iIndex, 0);

                /*
                // Now that we know the function to execute we need to execute it
                //    and output the data
                */

                SendDlgItemMessage(hDlg, IDC_CALLOUTPUT, LB_RESETCONTENT, 0, 0);
                vExecuteAndDisplayOutput(GetDlgItem(hDlg, IDC_CALLOUTPUT), pDevice, iIndex, &params);
            }
            break;  /* end IDC_EXECUTE case */

        /*
        // Start up a read thread that can read input reports while
        //  we operate on the other stuff
        */
        
        case IDC_READ_SYNCH:
        case IDC_READ_ASYNCH:
        
            if (NULL == ReadThread) 
            {
                if (!OpenHidDevice(pDevice->DevicePath,
                    TRUE,
                    FALSE,
                    (LOWORD(wParam) == IDC_READ_SYNCH) ? FALSE : TRUE,
                    FALSE,
                    &device))
                {
                    MessageBox(hDlg,
                        "Failed opening the device for read.",
                        HCLIENT_ERROR,
                        MB_ICONEXCLAMATION);
                        break;
                }

                SendDlgItemMessage(hDlg, IDC_CALLOUTPUT, LB_RESETCONTENT, 0, 0);

                readContext.HidDevice = &device;
                readContext.TerminateThread = FALSE;
                readContext.NumberOfReads = INFINITE_READS;
                readContext.DisplayEvent = NULL;
                readContext.DisplayWindow = hDlg;

                ReadThread = CreateThread(  NULL,
                                            0,
                                            (LOWORD(wParam) == IDC_READ_SYNCH) ? 
                                            SynchReadThreadProc :
                                            AsynchReadThreadProc,
                                            (LPVOID) &readContext,
                                            0,
                                            &threadID);

                if (NULL == ReadThread) 
                {
                    MessageBox(hDlg,
                               "Unable to create read thread",
                               HCLIENT_ERROR,
                               MB_ICONEXCLAMATION);
                }
                else
                {
                    EnableWindow(GetDlgItem(hDlg, IDC_READ_SYNCH),
                                 (LOWORD(wParam) == IDC_READ_SYNCH));

                    EnableWindow(GetDlgItem(hDlg, IDC_READ_ASYNCH),
                                 (LOWORD(wParam) == IDC_READ_ASYNCH));

                    SetWindowText(GetDlgItem(hDlg, LOWORD(wParam)),
                                  "Stop Read Thread");

                    EnableWindow(GetDlgItem(hDlg, IDC_CANCEL), FALSE);
                }
            }
            else
            {
                readContext.TerminateThread = TRUE;

                WaitForSingleObject(ReadThread, INFINITE);

                CloseHidDevice(&device);

                ReadThread = NULL;
                
                SetWindowText(GetDlgItem(hDlg, IDC_READ_SYNCH),
                              "Start Synchronous Read Thread");

                SetWindowText(GetDlgItem(hDlg, IDC_READ_ASYNCH),
                              "Start Asynchronous Read Thread");

                EnableWindow(GetDlgItem(hDlg, IDC_READ_SYNCH), TRUE);
                EnableWindow(GetDlgItem(hDlg, IDC_READ_ASYNCH), TRUE);
                EnableWindow(GetDlgItem(hDlg, IDC_CANCEL), TRUE);
            }
            break;

        case IDC_CANCEL:
            BufferDisplay_Destroy(pInputDisplay);
            BufferDisplay_Destroy(pOutputDisplay);
            BufferDisplay_Destroy(pFeatureDisplay);
            EndDialog(hDlg, 0);
            break;
        }
        break;

    case WM_CLOSE:
        PostMessage(hDlg, WM_COMMAND, IDC_CANCEL, 0);
        break;

    } 
    return FALSE;
}

VOID
vLoadExtCalls(
    HWND hExtCalls
)
{
    INT  iIndex;
    UINT uiIndex;

    /*
    // Load the physical device specific calls
    */
    
    for (uiIndex = 0; uiIndex < HID_DEVCALLS; uiIndex++) 
    {
        iIndex = (INT) SendMessage(hExtCalls, 
                                   CB_ADDSTRING, 
                                   0, 
                                   (LPARAM) DeviceCalls[uiIndex].szFunctionName);

        if (CB_ERR != iIndex && CB_ERRSPACE != iIndex) 
        {
            SendMessage(hExtCalls,
                        CB_SETITEMDATA,
                        iIndex, 
                        DeviceCalls[uiIndex].uiIndex);
        }
    }

    /*
    // Load the other device calls no matter what
    */

    for (uiIndex = 0; uiIndex < HID_PPDCALLS; uiIndex++)
    {
        iIndex = (INT) SendMessage(hExtCalls, 
                                   CB_ADDSTRING, 
                                   0, 
                                   (LPARAM) PpdCalls[uiIndex].szFunctionName);

        if (CB_ERR != iIndex && CB_ERRSPACE != iIndex) 
        {
            SendMessage(hExtCalls,
                        CB_SETITEMDATA,
                        iIndex, 
                        PpdCalls[uiIndex].uiIndex);
        }
    }
    SendMessage(hExtCalls, CB_SETCURSEL, 0, 0);

    return;
}

VOID vSetReportType(
    HWND hDlg, 
    LONG lId
)
{
    CheckRadioButton(hDlg, IDC_INPUT, IDC_FEATURE, lId);
    return;
}

VOID 
vInitEditText(
    HWND hText, 
    INT  cbTextSize, 
    _In_ LPSTR pchText
)
{
    SendMessage(hText, EM_SETLIMITTEXT, (WPARAM) cbTextSize, 0); 
    SendMessage(hText, EM_REPLACESEL, 0, (LPARAM) pchText);

    return;
}

VOID vEnableParameters(
    HWND     hDlg,
    LRESULT  iCallSelection
)
{
    EnableWindow(GetDlgItem(hDlg, IDC_INPUT), pState[iCallSelection-1].fInputReport);
    EnableWindow(GetDlgItem(hDlg, IDC_OUTPUT), pState[iCallSelection-1].fOutputReport);
    EnableWindow(GetDlgItem(hDlg, IDC_FEATURE), pState[iCallSelection-1].fFeatureReport);
    EnableWindow(GetDlgItem(hDlg, IDC_REPORTID), pState[iCallSelection-1].fReportID);
    EnableWindow(GetDlgItem(hDlg, IDC_USAGEPAGE), pState[iCallSelection-1].fUsagePage);
    EnableWindow(GetDlgItem(hDlg, IDC_USAGE), pState[iCallSelection-1].fUsage);
    EnableWindow(GetDlgItem(hDlg, IDC_LINKCOLL), pState[iCallSelection-1].fLinkCollection);
    EnableWindow(GetDlgItem(hDlg, IDC_INPUT_SELECT), pState[iCallSelection-1].fInputReportSelect);
    EnableWindow(GetDlgItem(hDlg, IDC_OUTPUT_SELECT), pState[iCallSelection-1].fOutputReportSelect);
    EnableWindow(GetDlgItem(hDlg, IDC_FEATURE_SELECT), pState[iCallSelection-1].fFeatureReportSelect);

    return;
}
    
BOOL 
fGetAndVerifyParameters(
    HWND              hDlg, 
    PECDISPLAY_PARAMS pParams
)
{
    /*
    // Declare a text buffer of size 7 since the parameter limit is at most 6
    //   characters in the edit box.  
    */
    
    CHAR    WindowText[7];
    BOOL    fStatus = TRUE;
    PCHAR   nptr;
    
    if (IsDlgButtonChecked(hDlg, IDC_INPUT)) 
    {
        pParams -> ReportType = HidP_Input;
    }
    else if (IsDlgButtonChecked(hDlg, IDC_OUTPUT))
    {
        pParams -> ReportType = HidP_Output;
    }
    else
    {
        pParams -> ReportType = HidP_Feature;
    }

    /*
    // Get and verify the usage page window text;
    */
    
    GetWindowText(GetDlgItem(hDlg, IDC_USAGEPAGE), WindowText, 7);

    pParams -> UsagePage = (USAGE) strtol(WindowText, &nptr, 16);

    if (*nptr != '\0') 
    {
        fStatus = FALSE;
        pParams -> UsagePage = 0;
    }

    /*
    // Get and verify the usage window text
    */

    GetWindowText(GetDlgItem(hDlg, IDC_USAGE), WindowText, 7);

    pParams -> Usage = (USAGE) strtol(WindowText, &nptr, 16);

    if (*nptr != '\0') 
    {
        fStatus = FALSE;
        pParams -> Usage = 0;
    }
    
    /*
    // Get and verify the link collection window text
    */

    GetWindowText(GetDlgItem(hDlg, IDC_LINKCOLL), WindowText, 7);

    pParams -> LinkCollection = (USAGE) strtol(WindowText, &nptr, 16);

    if (*nptr != '\0')
    {
        fStatus = FALSE;
        pParams -> LinkCollection = 0;
    }
    
    GetWindowText(GetDlgItem(hDlg, IDC_REPORTID), WindowText, 7);

    pParams -> ReportID = (UCHAR) strtol(WindowText, &nptr, 10);
 
    if (*nptr != '\0') 
    {
        fStatus = FALSE;
        pParams -> ReportID = 0;
    }

    return (fStatus);
}

VOID
vInitECControls(
    HWND                hDlg,
    USHORT              InputReportByteLength,
    PBUFFER_DISPLAY     *ppInputDisplay,
    USHORT              OutputReportByteLength,
    PBUFFER_DISPLAY     *ppOutputDisplay,
    USHORT              FeatureReportByteLength,
    PBUFFER_DISPLAY     *ppFeatureDisplay
)
{
    BOOLEAN     fInitStatus;

    /*
    // Begin by initializing the combo box with the calls that can be executed
    */

    vLoadExtCalls(GetDlgItem(hDlg, IDC_EXTCALLS));

    /*
    // Set the radio buttons initially to the input report type
    */
    
    vSetReportType(hDlg, IDC_INPUT);
    
    /*
    // Initialize the edit controls text
    */

    vInitEditText(GetDlgItem(hDlg, IDC_USAGEPAGE), 6, "0x0000");
    vInitEditText(GetDlgItem(hDlg, IDC_USAGE), 6, "0x0000");
    vInitEditText(GetDlgItem(hDlg, IDC_LINKCOLL), 2, "0");
    vInitEditText(GetDlgItem(hDlg, IDC_REPORTID), 3, "0");

    /*
    // Initialize the report buffer boxes
    */

    fInitStatus = BufferDisplay_Init(GetDlgItem(hDlg, IDC_INPUT_SELECT),
                                     GetDlgItem(hDlg, IDC_INPUT_BUFFER),
                                     NUM_INPUT_BUFFERS,
                                     InputReportByteLength,
                                     HidP_Input,
                                     ppInputDisplay);

    if (!fInitStatus) 
    {
        ECDISP_ERROR(hDlg, "Error initializing input buffer display");
    }

    fInitStatus = BufferDisplay_Init(GetDlgItem(hDlg, IDC_OUTPUT_SELECT),
                                     GetDlgItem(hDlg, IDC_OUTPUT_BUFFER),
                                     NUM_OUTPUT_BUFFERS,
                                     OutputReportByteLength,
                                     HidP_Output,
                                     ppOutputDisplay);

    if (!fInitStatus) 
    {
        ECDISP_ERROR(hDlg,  "Error initializing output buffer display");
    }

    fInitStatus = BufferDisplay_Init(GetDlgItem(hDlg, IDC_FEATURE_SELECT),
                                     GetDlgItem(hDlg, IDC_FEATURE_BUFFER),
                                     NUM_FEATURE_BUFFERS,
                                     FeatureReportByteLength,
                                     HidP_Feature,
                                     ppFeatureDisplay);

    if (!fInitStatus) 
    {
        ECDISP_ERROR(hDlg, "Error initializing feature buffer display");
    }

    /*
    // Reset the output box content
    */
    
    SendMessage(GetDlgItem(hDlg, IDC_CALLOUTPUT), LB_RESETCONTENT, 0, 0);
    return;
}
    
BOOL
ECDisp_Execute(
    IN     INT             FuncCall,
    IN OUT PEXTCALL_PARAMS CallParams,
    OUT    PEXTCALL_STATUS CallStatus
)
/*++
RoutineDescription:
    This routine is a complex routine for executing all of the functions.  The
    routine was originally developed with consideration for future use that 
    never materialized.  

    It makes use of the calls in extcalls.c which basically execute the given
    function and does some verification on the buffers that are passed down to 
    HID.DLL.  

    The input parameters are specify the function call to execute, the 
    call parameters structures and the call status structure.

    If any further buffers are needed for the specific calls, they will be
    allocated here.  

    The CallStatus parameters is a structure set by the ExtCalls_ routines

    Future versions of the HClient sample may remove this routine and/or the
    ExtCalls_ routines to simply the code.
--*/
{
    BOOL                ExecuteStatus;
    HIDP_VALUE_CAPS     ValueCaps;
    USHORT              ValueCapsLength;
    PULONG              ValueList;

    NTSTATUS            status;
    DWORD               numBytes;
    ULONG               size;

    /*
    // Initially assume everything will go correctly and will set otherwise
    //    depending on the function call.
    */

    CallStatus -> IsHidError = FALSE;

    switch (FuncCall) 
    {
    case HID_READ_REPORT:
        CallStatus -> IsHidError = !ReadFile(CallParams -> DeviceHandle,
                                             CallParams -> ReportBuffer,
                                             CallParams -> ReportLength,
                                             &numBytes,
                                             NULL);
        return (TRUE);
        break;

    case HID_WRITE_REPORT:
        CallStatus -> IsHidError = !WriteFile(CallParams -> DeviceHandle,
                                              CallParams -> ReportBuffer,
                                              CallParams -> ReportLength,
                                              &numBytes,
                                              NULL);
        return (TRUE);
        break;

    case HIDD_FLUSH_QUEUE:
        CallStatus -> IsHidError = !HidD_FlushQueue(CallParams -> DeviceHandle);
        return (TRUE);
        break;

    case HIDD_GET_HID_GUID:
        CallParams -> List = malloc(sizeof(GUID));

        if (NULL != CallParams -> List) 
        {
            HidD_GetHidGuid((GUID *) CallParams -> List);
        }
        return (NULL != CallParams -> List);
        break;

    case HIDD_GET_PREPARSED_DATA:
        CallStatus -> IsHidError = !HidD_GetPreparsedData(CallParams -> DeviceHandle,
                                                          CallParams -> ppPd);
        return (TRUE);

    case HIDD_FREE_PREPARSED_DATA:
        CallStatus -> IsHidError = !HidD_FreePreparsedData(CallParams -> Ppd);
        return (TRUE);

    case HIDD_GET_ATTRIBUTES:
        CallParams -> List = malloc(sizeof(HIDD_ATTRIBUTES));

        if (NULL != CallParams -> List) 
        {
            CallStatus -> IsHidError = !HidD_GetAttributes(CallParams -> DeviceHandle,
                                                           CallParams -> List);
        }
        return (NULL != CallParams -> List);
        
    case HIDD_GET_FEATURE:
        *(CallParams -> ReportBuffer) = CallParams -> ReportID;

        CallStatus -> IsHidError = !HidD_GetFeature(CallParams -> DeviceHandle,
                                                    CallParams -> ReportBuffer,
                                                    CallParams -> ReportLength);
        return (TRUE);

    case HIDD_SET_FEATURE:
        CallStatus -> IsHidError = !HidD_SetFeature(CallParams -> DeviceHandle,
                                                    CallParams -> ReportBuffer,
                                                    CallParams -> ReportLength);
        return (TRUE);

    case HIDD_GET_INPUT_REPORT:
        *(CallParams -> ReportBuffer) = CallParams -> ReportID;

        CallStatus -> IsHidError = !HidD_GetInputReport(CallParams -> DeviceHandle,
                                                        CallParams -> ReportBuffer,
                                                        CallParams -> ReportLength);
        return (TRUE);

    case HIDD_SET_OUTPUT_REPORT:
        CallStatus -> IsHidError = !HidD_SetOutputReport(CallParams -> DeviceHandle,
                                                         CallParams -> ReportBuffer,
                                                         CallParams -> ReportLength);
        return (TRUE);

    case HIDD_GET_NUM_INPUT_BUFFERS:
        CallStatus -> IsHidError = !HidD_GetNumInputBuffers(CallParams -> DeviceHandle,
                                                            &CallParams -> Value);
        return (TRUE);

    case HIDD_SET_NUM_INPUT_BUFFERS:
        CallStatus -> IsHidError = !HidD_SetNumInputBuffers(CallParams -> DeviceHandle,
                                                            CallParams -> Value);

        return (TRUE);

    case HIDD_GET_PHYSICAL_DESCRIPTOR:
        CallParams -> List = (PCHAR) malloc (CallParams -> ListLength);

        if (NULL != CallParams -> List ) 
        {
            CallStatus -> IsHidError = !HidD_GetPhysicalDescriptor(CallParams -> DeviceHandle,
                                                                   CallParams -> List,
                                                                   CallParams -> ListLength);
        }
        return (NULL != CallParams -> List);

    case HIDD_GET_MANUFACTURER_STRING:
        
        CallParams -> List = (PWCHAR) malloc (CallParams -> ListLength);

        if (NULL != CallParams -> List ) 
        {
            CallStatus -> IsHidError = !HidD_GetManufacturerString(CallParams -> DeviceHandle,
                                                                   CallParams -> List,
                                                                   CallParams -> ListLength);
        }
        return (NULL != CallParams -> List);

    case HIDD_GET_PRODUCT_STRING:
        CallParams -> List = (PWCHAR) malloc (CallParams -> ListLength);

        if (NULL != CallParams -> List ) 
        {
            CallStatus -> IsHidError = !HidD_GetProductString(CallParams -> DeviceHandle,
                                                             CallParams -> List,
                                                             CallParams -> ListLength);
        }
        return (NULL != CallParams -> List);

    case HIDP_GET_EXTENDED_ATTRIBUTES:
        CallParams -> List = (PWCHAR) malloc (CallParams -> ListLength);
        if (NULL != CallParams -> List )
        {
            status = pfnHidP_GetExtendedAttributes(CallParams -> ReportType,
                                                  (USHORT) CallParams -> Index,
                                                   CallParams -> Ppd,
                                                   CallParams -> List,
                                                   &CallParams -> ListLength);

            CallStatus -> IsHidError = (HIDP_STATUS_SUCCESS != status);
            CallStatus -> HidErrorCode = status;
        }
        return (NULL != CallParams -> List);
        
    case HIDD_GET_INDEXED_STRING:
        CallParams -> List = (PWCHAR) malloc (CallParams -> ListLength);

        if (NULL != CallParams -> List ) 
        {
            CallStatus -> IsHidError = !HidD_GetIndexedString(CallParams -> DeviceHandle,
                                                              CallParams -> Index,
                                                              CallParams -> List,
                                                              CallParams -> ListLength);
        }
        return (NULL != CallParams -> List);
        
    case HIDD_GET_SERIAL_NUMBER_STRING:
        CallParams -> List = (PWCHAR) malloc (CallParams -> ListLength);

        if (NULL != CallParams -> List ) 
        {
            CallStatus -> IsHidError = !HidD_GetSerialNumberString(CallParams -> DeviceHandle,
                                                                   CallParams -> List,
                                                                   CallParams -> ListLength);
        }
        return (NULL != CallParams -> List);

    case HIDD_GET_MS_GENRE_DESCRIPTOR:
        CallParams -> List = (PCHAR) malloc (CallParams -> ListLength);

        if (NULL != CallParams -> List ) 
        {
            CallStatus -> IsHidError = !HidD_GetMsGenreDescriptor(CallParams -> DeviceHandle,
                                                                  CallParams -> List,
                                                                  CallParams -> ListLength);
        }
        return (NULL != CallParams -> List);

    case HIDP_GET_BUTTON_CAPS:
        size = CallParams -> ListLength * sizeof(HIDP_BUTTON_CAPS);

        CallParams -> List = malloc(size);

        if (NULL != CallParams -> List) 
        {
            status = HidP_GetButtonCaps(CallParams -> ReportType,
                                        CallParams -> List,
                                        (PUSHORT) &CallParams -> ListLength,
                                        CallParams -> Ppd);

            CallStatus -> IsHidError = (HIDP_STATUS_SUCCESS != status);
            CallStatus -> HidErrorCode = status;
        }
        return (NULL != CallParams -> List);

    case HIDP_GET_BUTTONS:

        CallParams -> ListLength = HidP_MaxUsageListLength(CallParams -> ReportType,
                                                           CallParams -> UsagePage,
                                                           CallParams -> Ppd);

        CallParams -> List = malloc(CallParams -> ListLength * sizeof(USAGE));

        if (NULL != CallParams -> List) 
        {
            status = HidP_GetButtons(CallParams -> ReportType,
                                     CallParams -> UsagePage,
                                     CallParams -> LinkCollection,
                                     CallParams -> List,
                                     &CallParams -> ListLength,
                                     CallParams -> Ppd,
                                     CallParams -> ReportBuffer,
                                     CallParams -> ReportLength);

            CallStatus -> IsHidError = (HIDP_STATUS_SUCCESS != status);
            CallStatus -> HidErrorCode = status;
        }
        return (NULL != CallParams -> List);

    case HIDP_GET_BUTTONS_EX:

        CallParams -> ListLength = HidP_MaxUsageListLength(CallParams -> ReportType,
                                                           CallParams -> UsagePage,
                                                           CallParams -> Ppd);

        CallParams -> List = malloc(CallParams -> ListLength * sizeof(USAGE_AND_PAGE));

        if (NULL != CallParams -> List) 
        {
            status = HidP_GetButtonsEx(CallParams -> ReportType,
                                       CallParams -> LinkCollection,
                                       CallParams -> List,
                                       &CallParams -> ListLength,
                                       CallParams -> Ppd,
                                       CallParams -> ReportBuffer,
                                       CallParams -> ReportLength);

            CallStatus -> IsHidError = (HIDP_STATUS_SUCCESS != status);
            CallStatus -> HidErrorCode = status;
        }
        return (NULL != CallParams -> List);

        
    case HIDP_GET_CAPS:

        CallParams -> ListLength = sizeof(HIDP_CAPS);

        CallParams -> List = malloc(sizeof(HIDP_CAPS));

        if (NULL != CallParams -> List)
        {
            status = HidP_GetCaps(CallParams -> Ppd, CallParams -> List);

            CallStatus -> IsHidError = (HIDP_STATUS_SUCCESS != status);
            CallStatus -> HidErrorCode = status;
        }
        return (NULL != CallParams -> List);

    case HIDP_GET_DATA:

        CallParams -> ListLength = HidP_MaxDataListLength(CallParams -> ReportType,
                                                          CallParams -> Ppd);

        CallParams -> List = malloc(CallParams -> ListLength * sizeof(HIDP_DATA));

        if (NULL != CallParams -> List)
        {
            status = HidP_GetData(CallParams -> ReportType,
                                  CallParams -> List,
                                  &CallParams -> ListLength,
                                  CallParams -> Ppd,
                                  CallParams -> ReportBuffer,
                                  CallParams -> ReportLength);

            CallStatus -> IsHidError = (HIDP_STATUS_SUCCESS != status);
            CallStatus -> HidErrorCode = status;
        }
        return (NULL != CallParams -> List);

    case HIDP_GET_LINK_COLL_NODES:
        CallParams -> List = malloc(CallParams -> ListLength * sizeof(HIDP_LINK_COLLECTION_NODE));

        if (NULL != CallParams -> List)
        {
            status = HidP_GetLinkCollectionNodes(CallParams -> List,
                                                 &CallParams -> ListLength,
                                                 CallParams -> Ppd);

            CallStatus -> IsHidError = (HIDP_STATUS_SUCCESS != status);
            CallStatus -> HidErrorCode = status;
        }
        return (NULL != CallParams -> List);

    case HIDP_GET_SCALED_USAGE_VALUE:

        status = HidP_GetScaledUsageValue(CallParams -> ReportType,
                                          CallParams -> UsagePage,
                                          CallParams -> LinkCollection,
                                          CallParams -> Usage,
                                         &CallParams -> ScaledValue,
                                          CallParams -> Ppd,
                                          CallParams -> ReportBuffer,
                                          CallParams -> ReportLength);

        CallStatus -> IsHidError = (HIDP_STATUS_SUCCESS != status);
        CallStatus -> HidErrorCode = status;

        return (TRUE);

    case HIDP_GET_SPECIFIC_BUTTON_CAPS:

        CallParams -> List = malloc(CallParams -> ListLength * sizeof(HIDP_BUTTON_CAPS));

        if (NULL != CallParams -> List)
        {
            status = HidP_GetSpecificButtonCaps(CallParams -> ReportType,
                                                CallParams -> UsagePage,
                                                CallParams -> LinkCollection,
                                                CallParams -> Usage,
                                                CallParams -> List,
                                                (PUSHORT) &CallParams -> ListLength,
                                                CallParams -> Ppd);

            CallStatus -> IsHidError = (HIDP_STATUS_SUCCESS != status);
            CallStatus -> HidErrorCode = status;
       }
       return (NULL != CallParams -> List);
        
    case HIDP_GET_SPECIFIC_VALUE_CAPS:

        CallParams -> List = malloc(CallParams -> ListLength * sizeof(HIDP_VALUE_CAPS));

        if (NULL != CallParams -> List)
        {
            status = HidP_GetSpecificValueCaps(CallParams -> ReportType,
                                               CallParams -> UsagePage,
                                               CallParams -> LinkCollection,
                                               CallParams -> Usage,
                                               CallParams -> List,
                                               (PUSHORT) &CallParams -> ListLength,
                                               CallParams -> Ppd);

            CallStatus -> IsHidError = (HIDP_STATUS_SUCCESS != status);
            CallStatus -> HidErrorCode = status;
       }
       return (NULL != CallParams -> List);

    case HIDP_GET_USAGES:
        CallParams -> ListLength = HidP_MaxUsageListLength(CallParams -> ReportType,
                                                           CallParams -> UsagePage,
                                                           CallParams -> Ppd);

        CallParams -> List = malloc(CallParams -> ListLength * sizeof(USAGE));

        if (NULL != CallParams -> List)
        {
            status = HidP_GetUsages(CallParams -> ReportType,
                                    CallParams -> UsagePage,
                                    CallParams -> LinkCollection,
                                    CallParams -> List,
                                   &CallParams -> ListLength,
                                    CallParams -> Ppd,
                                    CallParams -> ReportBuffer,
                                    CallParams -> ReportLength);

            CallStatus -> IsHidError = (HIDP_STATUS_SUCCESS != status);
            CallStatus -> HidErrorCode = status;
       }
       return (NULL != CallParams -> List);

    case HIDP_GET_USAGES_EX:
        CallParams -> ListLength = HidP_MaxUsageListLength(CallParams -> ReportType,
                                                           CallParams -> UsagePage,
                                                           CallParams -> Ppd);

        CallParams -> List = malloc(CallParams -> ListLength * sizeof(USAGE_AND_PAGE));

        if (NULL != CallParams -> List)
        {
            status = HidP_GetUsagesEx(CallParams -> ReportType,
                                      CallParams -> LinkCollection,
                                      CallParams -> List,
                                     &CallParams -> ListLength,
                                      CallParams -> Ppd,
                                      CallParams -> ReportBuffer,
                                      CallParams -> ReportLength);

            CallStatus -> IsHidError = (HIDP_STATUS_SUCCESS != status);
            CallStatus -> HidErrorCode = status;
       }
       return (NULL != CallParams -> List);
         
    case HIDP_GET_USAGE_VALUE:
        status = HidP_GetUsageValue(CallParams -> ReportType,
                                    CallParams -> UsagePage,
                                    CallParams -> LinkCollection,
                                    CallParams -> Usage,
                                    &CallParams -> Value,
                                    CallParams -> Ppd,
                                    CallParams -> ReportBuffer,
                                    CallParams -> ReportLength);

        CallStatus -> IsHidError = (HIDP_STATUS_SUCCESS != status);
        CallStatus -> HidErrorCode = status;

        return (TRUE);

    case HIDP_GET_USAGE_VALUE_ARRAY:

        ValueCapsLength = 1;
        status = HidP_GetSpecificValueCaps(CallParams -> ReportType,
                                           CallParams -> UsagePage,
                                           CallParams -> LinkCollection,
                                           CallParams -> Usage,
                                           &ValueCaps,
                                           &ValueCapsLength,
                                           CallParams -> Ppd);

        if (HIDP_STATUS_SUCCESS != status) 
        {
            return (FALSE);
        }

        CallParams -> BitSize     = ValueCaps.BitSize;
        CallParams -> ReportCount = ValueCaps.ReportCount;
        CallParams -> ListLength
                 = ROUND_TO_NEAREST_BYTE(CallParams -> BitSize * CallParams -> ReportCount);

        CallParams -> List = malloc(CallParams -> ListLength);

        if (NULL != CallParams -> List) 
        {
            status = HidP_GetUsageValueArray(CallParams -> ReportType,
                                             CallParams -> UsagePage,
                                             CallParams -> LinkCollection,
                                             CallParams -> Usage,
                                             CallParams -> List,
                                             (USHORT) CallParams -> ListLength,
                                             CallParams -> Ppd,
                                             CallParams -> ReportBuffer,
                                             CallParams -> ReportLength);

            CallStatus -> IsHidError = (HIDP_STATUS_SUCCESS != status);
            CallStatus -> HidErrorCode = status;
        }
        return (NULL != CallParams -> List);

    case HIDP_GET_VALUE_CAPS:
        CallParams -> List = malloc(CallParams -> ListLength * sizeof(HIDP_VALUE_CAPS));
        if (NULL != CallParams -> List) 
        {
            status = HidP_GetValueCaps(CallParams -> ReportType,
                                       CallParams -> List,
                                       (PUSHORT) &CallParams -> ListLength,
                                       CallParams -> Ppd);

            CallStatus -> IsHidError = (HIDP_STATUS_SUCCESS != status);
            CallStatus -> HidErrorCode = status;
        }


    case HIDP_INITIALIZE_REPORT_FOR_ID:
        
        status = pfnHidP_InitializeReportForID(CallParams -> ReportType,
                                               CallParams -> ReportID,
                                               CallParams -> Ppd,
                                               CallParams -> ReportBuffer,
                                               CallParams -> ReportLength);

        CallStatus -> IsHidError = (HIDP_STATUS_SUCCESS != status);
        CallStatus -> HidErrorCode = status;

        return (TRUE);

    case HIDP_MAX_USAGE_LIST_LENGTH:

        CallParams -> Value = HidP_MaxUsageListLength(CallParams -> ReportType,
                                                      CallParams -> UsagePage,
                                                      CallParams -> Ppd);

        CallStatus -> IsHidError = FALSE;
        CallStatus -> HidErrorCode = HIDP_STATUS_SUCCESS;

        return (TRUE);

    case HIDP_MAX_DATA_LIST_LENGTH:

        CallParams -> Value = HidP_MaxDataListLength(CallParams -> ReportType,
                                                     CallParams -> Ppd);

        CallStatus -> IsHidError = FALSE;
        CallStatus -> HidErrorCode = HIDP_STATUS_SUCCESS;

        return (TRUE);

    case HIDP_SET_BUTTONS:

        status = HidP_SetButtons(CallParams -> ReportType,
                                 CallParams -> UsagePage,
                                 CallParams -> LinkCollection,
                                 CallParams -> List,
                                 &CallParams -> ListLength,
                                 CallParams -> Ppd,
                                 CallParams -> ReportBuffer,
                                 CallParams -> ReportLength);

        CallStatus -> IsHidError = (HIDP_STATUS_SUCCESS != status);
        CallStatus -> HidErrorCode = status;

        return (TRUE);

    case HIDP_SET_DATA:

        status = HidP_SetData(CallParams -> ReportType,
                              CallParams -> List,
                              &CallParams -> ListLength,
                              CallParams -> Ppd,
                              CallParams -> ReportBuffer,
                              CallParams -> ReportLength);

        CallStatus -> IsHidError = (HIDP_STATUS_SUCCESS != status);
        CallStatus -> HidErrorCode = status;

        return (TRUE);

    case HIDP_SET_SCALED_USAGE_VALUE:
        status = HidP_SetUsageValue(CallParams -> ReportType,
                                    CallParams -> UsagePage,
                                    CallParams -> LinkCollection,
                                    CallParams -> Usage,
                                    CallParams -> ScaledValue,
                                    CallParams -> Ppd,
                                    CallParams -> ReportBuffer,
                                    CallParams -> ReportLength);

        CallStatus -> IsHidError = (HIDP_STATUS_SUCCESS != status);
        CallStatus -> HidErrorCode = status;

        return (TRUE);

    case HIDP_SET_USAGES:
        status = HidP_SetUsages(CallParams -> ReportType,
                                CallParams -> UsagePage,
                                CallParams -> LinkCollection,
                                CallParams -> List,
                               &CallParams -> ListLength,
                                CallParams -> Ppd,
                                CallParams -> ReportBuffer,
                                CallParams -> ReportLength);

        CallStatus -> IsHidError = (HIDP_STATUS_SUCCESS != status);
        CallStatus -> HidErrorCode = status;

        return (TRUE);

    case HIDP_SET_USAGE_VALUE:
        status = HidP_SetUsageValue(CallParams -> ReportType,
                                    CallParams -> UsagePage,
                                    CallParams -> LinkCollection,
                                    CallParams -> Usage,
                                    CallParams -> Value,
                                    CallParams -> Ppd,
                                    CallParams -> ReportBuffer,
                                    CallParams -> ReportLength);

        CallStatus -> IsHidError = (HIDP_STATUS_SUCCESS != status);
        CallStatus -> HidErrorCode = status;

        return (TRUE);

    case HIDP_SET_USAGE_VALUE_ARRAY:

        ValueCapsLength = 1;
        status = HidP_GetSpecificValueCaps(CallParams -> ReportType,
                                           CallParams -> UsagePage,
                                           CallParams -> LinkCollection,
                                           CallParams -> Usage,
                                           &ValueCaps,
                                           &ValueCapsLength,
                                           CallParams -> Ppd);

        if (HIDP_STATUS_SUCCESS != status) 
        {
            return (FALSE);
        }
        
        CallParams -> BitSize     = ValueCaps.BitSize;
        CallParams -> ReportCount = ValueCaps.ReportCount;

        ValueList = CallParams -> List;
        ExecuteStatus = ECDisp_ConvertUlongListToValueList(ValueList,
                                                           CallParams -> ListLength,
                                                           CallParams -> BitSize,
                                                           CallParams -> ReportCount,
                                                           (PCHAR *) &CallParams -> List,
                                                           &CallParams -> ListLength);

        if (!ExecuteStatus) 
        {
            return (FALSE);
        }

        status = HidP_SetUsageValueArray(CallParams -> ReportType,
                                         CallParams -> UsagePage,
                                         CallParams -> LinkCollection,
                                         CallParams -> Usage,
                                         CallParams -> List,
                                         (USHORT) CallParams -> ListLength,
                                         CallParams -> Ppd,
                                         CallParams -> ReportBuffer,
                                         CallParams -> ReportLength);

        CallStatus -> IsHidError = (HIDP_STATUS_SUCCESS != status);
        CallStatus -> HidErrorCode = status;

        return (TRUE);

    case HIDP_UNSET_BUTTONS:
        status = HidP_UnsetButtons(CallParams -> ReportType,
                                   CallParams -> UsagePage,
                                   CallParams -> LinkCollection,
                                   CallParams -> List,
                                   &CallParams -> ListLength,
                                   CallParams -> Ppd,
                                   CallParams -> ReportBuffer,
                                   CallParams -> ReportLength);

        CallStatus -> IsHidError = (HIDP_STATUS_SUCCESS != status);
        CallStatus -> HidErrorCode = status;

        return (TRUE);

    case HIDP_UNSET_USAGES:
        status = HidP_UnsetUsages(CallParams -> ReportType,
                                  CallParams -> UsagePage,
                                  CallParams -> LinkCollection,
                                  CallParams -> List,
                                 &CallParams -> ListLength,
                                  CallParams -> Ppd,
                                  CallParams -> ReportBuffer,
                                  CallParams -> ReportLength);
                          
        CallStatus -> IsHidError = (HIDP_STATUS_SUCCESS != status);
        CallStatus -> HidErrorCode = status;
                          
        return (TRUE);    

    case HIDP_USAGE_LIST_DIFFERENCE:
        CallParams -> MakeList = (PUSAGE) malloc (sizeof(USAGE) * CallParams -> ListLength);

        if (NULL == CallParams -> MakeList)
        {
            return (FALSE);
        }

        CallParams -> BreakList = (PUSAGE) malloc (sizeof(USAGE) * CallParams -> ListLength);
        if (NULL == CallParams -> BreakList) 
        {
            free(CallParams -> MakeList);

            return (FALSE);
        }

        status = HidP_UsageListDifference(CallParams -> List,
                                          CallParams -> List2,
                                          CallParams -> BreakList,
                                          CallParams -> MakeList,
                                          CallParams -> ListLength);

        CallStatus -> IsHidError = (HIDP_STATUS_SUCCESS != status);
        CallStatus -> HidErrorCode = status;

        return (TRUE);
    }
    return (FALSE);
}

VOID
ECDisp_DisplayOutput(
    IN HWND            hOutputWindow,
    IN INT             FuncCall,
    IN PEXTCALL_PARAMS Results
)
/*++
RoutineDescription:
    This routine is responsible for displaying the output from calls to HID.DLL
    functions.  It must extract and interpret the appropriate data from the 
    PEXTCALL_PARAMS structure. 
--*/
{
    PHIDP_LINK_COLLECTION_NODE NodeList;
    PHIDP_BUTTON_CAPS          ButtonCaps;
    PHIDP_VALUE_CAPS           ValueCaps;
    PHIDP_DATA                 DataList;
    PHIDP_EXTENDED_ATTRIBUTES  ExtAttrib;
    PUSAGE_AND_PAGE            UsageAndPageList;
    PUSAGE                     UsageList;
    PCHAR                      UsageValueArray;
    ULONG                      UsageValueArraySize;
    PBUFFER_DISPLAY            pDisplay = NULL;
    PCHAR                      PhysDescString;
    PCHAR                      GenreDescString;
    SIZE_T                     StringLength;
    UINT                       RemainingBuffer=0;

    ULONG                      Index;


    switch (FuncCall) {
    case HIDD_GET_HID_GUID:

        StringCbCopy(szTempBuffer, TEMP_BUFFER_SIZE, "HID Guid: ");
        StringLength = strlen(szTempBuffer);
        if (StringLength < TEMP_BUFFER_SIZE)
        {
            RemainingBuffer = (UINT)(TEMP_BUFFER_SIZE - StringLength);
        }
        ECDisp_MakeGUIDString(*((LPGUID) Results -> List),
                              &szTempBuffer[StringLength],
                              RemainingBuffer);

        OUTSTRING(hOutputWindow, szTempBuffer);
        break;

    case HIDD_GET_ATTRIBUTES:
        vDisplayDeviceAttributes((PHIDD_ATTRIBUTES) Results -> List, 
                                 hOutputWindow);
        break;

    case HIDD_GET_NUM_INPUT_BUFFERS:
        StringCbPrintf(szTempBuffer, TEMP_BUFFER_SIZE,
                       "Number input buffers: %u", Results -> Value);

        OUTSTRING(hOutputWindow, szTempBuffer);
        break;

    case HIDD_GET_MS_GENRE_DESCRIPTOR:
        OUTSTRING(hOutputWindow, "MS Genre Descriptor");
        OUTSTRING(hOutputWindow, "===================");
    
            /*
        // To display a physical descriptor, the procedure currently just
        //   creates a string data buffer by bytes and displays that 
        //   in the results box.  It will display in rows of 16 bytes apiece.
        */
        
        Index = 0;
        while (Index < Results -> ListLength) 
        {
            Strings_CreateDataBufferString(((PCHAR) Results -> List) + Index,
                                           Results -> ListLength - Index,
                                           16,
                                           1,
                                           &GenreDescString);

            if (NULL != GenreDescString) 
            {
                OUTSTRING(hOutputWindow, GenreDescString);
                free(GenreDescString);
            }
            else 
            {
               OUTSTRING(hOutputWindow, "Error trying to display ms genre descriptor");
            }
            Index += 16;
        }
        break;

    case HIDD_GET_PHYSICAL_DESCRIPTOR: 
        OUTSTRING(hOutputWindow, "Physical Descriptor");
        OUTSTRING(hOutputWindow, "===================");

        /*
        // To display a physical descriptor, the procedure currently just
        //   creates a string data buffer by bytes and displays that 
        //   in the results box.  It will display in rows of 16 bytes apiece.
        */
        
        Index = 0;
        while (Index < Results -> ListLength) 
        {
            Strings_CreateDataBufferString(((PCHAR) Results -> List) + Index,
                                           Results -> ListLength - Index,
                                           16,
                                           1,
                                           &PhysDescString);

            if (NULL != PhysDescString) 
            {
                OUTSTRING(hOutputWindow, PhysDescString);
                free(PhysDescString);
            }
            else 
            {
               OUTSTRING(hOutputWindow, "Error trying to display physical descriptor");
            }
            Index += 16;
        }
        break;

    /*
    // For the string descriptor call routines, the returned string is stored
    //   in the Results -> List parameter.  It should be noted that the
    //   strings returned by these calls are wide-char strings and that these
    //   string are terminated with a NULL character if there was space withing
    //   the buffer to add such a character.  If the buffer was only big enough
    //   to hold the characters of the string, there will be no null terminator
    //   and the output string display mechanism may fail to properly display this
    //   type of string.  Fixing of this display mechanism is a future (low priority)
    //   workitem.
    */
    
    case HIDD_GET_PRODUCT_STRING:
        OUTSTRING(hOutputWindow, "Product String");
        OUTSTRING(hOutputWindow, "==============");
        #pragma prefast(suppress: 26014, "Potential overflow check")
        OUTWSTRING(hOutputWindow, Results -> List);
        break;

    case HIDD_GET_MANUFACTURER_STRING:
        OUTSTRING(hOutputWindow, "Manufacturer String");
        OUTSTRING(hOutputWindow, "===================");
        OUTWSTRING(hOutputWindow, Results -> List);
        break;

    case HIDD_GET_INDEXED_STRING:
        StringCbPrintf(szTempBuffer, TEMP_BUFFER_SIZE,
                       "Indexed String #%u:", Results -> Index);

        OUTSTRING(hOutputWindow, szTempBuffer);
        OUTSTRING(hOutputWindow, "===================");
        OUTWSTRING(hOutputWindow, Results -> List);
        break;

    case HIDD_GET_SERIAL_NUMBER_STRING:
        OUTSTRING(hOutputWindow, "Serial Number String");
        OUTSTRING(hOutputWindow, "=====================");
        OUTWSTRING(hOutputWindow, Results -> List);
        break;
        
    case HIDP_GET_BUTTON_CAPS:
    case HIDP_GET_SPECIFIC_BUTTON_CAPS:

        ButtonCaps = (PHIDP_BUTTON_CAPS) (Results -> List);
        for (Index = 0; Index < Results -> ListLength; Index++, ButtonCaps++) 
        {
             OUTSTRING(hOutputWindow, "==========================");
             vDisplayButtonAttributes(ButtonCaps, hOutputWindow);
        }
        break;

    /*
    // HidP_GetButtons and HidP_GetUsages are in reality the same call.  
    //   HidP_GetButtons actually a macro which gets redefined into 
    //   HidP_GetUsages with the same parameter order.  That is why their
    //   display mechanisms are identical.  This call returns in the 
    //   List parameter a list of Usages.  The display mechanism converts
    //   these usages into a string of numbers.
    */
    
    case HIDP_GET_BUTTONS:
    case HIDP_GET_USAGES:

        OUTSTRING(hOutputWindow, "Usages Returned");
        OUTSTRING(hOutputWindow, "===============");

        UsageList = (PUSAGE) Results -> List;
        for (Index = 0; Index < Results -> ListLength; Index++) 
        {
            vCreateUsageString(UsageList + Index, szTempBuffer, TEMP_BUFFER_SIZE);

            OUTSTRING(hOutputWindow, szTempBuffer);
        }
        break;

    /*
    // Like get their siblings, the normal get functions, these routines are
    //   currently one in the same.  The difference between these routines 
    //   and their siblings is the return of a usage page along with each
    //   usage.  Therefore, both values must be displayed at the same time.
    */
    
    case HIDP_GET_BUTTONS_EX:
    case HIDP_GET_USAGES_EX:

        OUTSTRING(hOutputWindow, "Usages Returned");
        OUTSTRING(hOutputWindow, "===============");

        UsageAndPageList = (PUSAGE_AND_PAGE) Results -> List;
        for (Index = 0; Index < Results -> ListLength; Index++) 
        {
            vCreateUsageAndPageString(UsageAndPageList + Index,
                                      szTempBuffer,
                                      TEMP_BUFFER_SIZE);

            OUTSTRING(hOutputWindow, szTempBuffer);
        }
        break;

    case HIDP_GET_CAPS:
        vDisplayDeviceCaps((PHIDP_CAPS) Results -> List, hOutputWindow);
        break;

    case HIDP_GET_DATA:
        OUTSTRING(hOutputWindow, "Data Indices");
        OUTSTRING(hOutputWindow, "============");
        
        DataList = (PHIDP_DATA) Results -> List;
        for (Index = 0; Index < Results -> ListLength; Index++) 
        {
            vDisplayDataAttributes(DataList+Index,
                                   FALSE,
                                   hOutputWindow);
        }
        break;

    case HIDP_GET_EXTENDED_ATTRIBUTES:
        OUTSTRING(hOutputWindow, "Extended Attributes");
        OUTSTRING(hOutputWindow, "===================");
        
        ExtAttrib = (PHIDP_EXTENDED_ATTRIBUTES) Results -> List;

        DisplayExtendedAttributes(hOutputWindow,
                                  ExtAttrib -> GlobalUnknowns, 
                                  ExtAttrib -> NumGlobalUnknowns);
        break;
        
    case HIDP_GET_LINK_COLL_NODES:

        OUTSTRING(hOutputWindow, "Link Collection Nodes");
        OUTSTRING(hOutputWindow, "=====================");
        
        NodeList = (PHIDP_LINK_COLLECTION_NODE) Results -> List;
        for (Index = 0; Index < Results -> ListLength; Index++) 
        {
            OUTSTRING(hOutputWindow, "===========================");
            vDisplayLinkCollectionNode(NodeList+Index,
                                       Index,
                                       hOutputWindow);
        }
        break;

    case HIDP_GET_SCALED_USAGE_VALUE:
    
        StringCbPrintf(szTempBuffer, TEMP_BUFFER_SIZE,
                       "Scaled usage value: %ld", Results -> ScaledValue);
        OUTSTRING(hOutputWindow, szTempBuffer);

        break;

    case HIDP_GET_USAGE_VALUE:
        StringCbPrintf(szTempBuffer, TEMP_BUFFER_SIZE,
                       "Usage value: %lu", Results -> Value);
        OUTSTRING(hOutputWindow, szTempBuffer);
        break;

    /*
    // To display a usage value array, we must extract each of the values
    //   in the array based on the ReportSize.  The ReportSize is not necessarily
    //   an even byte size so we must use the special extraction routine to get
    //   each of the values in the array.
    */
    
    case HIDP_GET_USAGE_VALUE_ARRAY:

        UsageValueArray = (PCHAR) Results -> List;
        UsageValueArraySize = Results->ListLength;

        for (Index = 0; Index < Results -> ReportCount; Index++) 
        {
            vCreateUsageValueStringFromArray(UsageValueArray,
                                             UsageValueArraySize,
                                             Results -> BitSize,
                                             (USHORT) Index,
                                             szTempBuffer,
                                             TEMP_BUFFER_SIZE);
    
            OUTSTRING(hOutputWindow, szTempBuffer);
        }
        break;

    case HIDP_GET_VALUE_CAPS:
    case HIDP_GET_SPECIFIC_VALUE_CAPS:
        
        ValueCaps = (PHIDP_VALUE_CAPS) Results -> List;

        for (Index = 0; Index < (INT) Results -> ListLength; Index++) 
        {
            OUTSTRING(hOutputWindow, "==========================");
            vDisplayValueAttributes(ValueCaps + Index,
                                    hOutputWindow);
        }
        break;

    case HIDP_MAX_DATA_LIST_LENGTH:
        StringCbPrintf(szTempBuffer, TEMP_BUFFER_SIZE,
                       "MaxDataListLength: %u", Results -> Value);
        OUTSTRING(hOutputWindow, szTempBuffer);
        break;

    case HIDP_MAX_USAGE_LIST_LENGTH:
        StringCbPrintf(szTempBuffer, TEMP_BUFFER_SIZE,
                       "MaxUsageListLength: %u", Results -> Value);
        OUTSTRING(hOutputWindow, szTempBuffer);
        break;

    /*
    // For HidP_UsageListDifference, we need to display both of the make and
    //   break lists generated by the function.  Therefore, we end up creating
    //   two different usage list strings.
    */
    
    case HIDP_USAGE_LIST_DIFFERENCE:
        
        OUTSTRING(hOutputWindow, "Make List");
        OUTSTRING(hOutputWindow, "=========");

        UsageList = (PUSAGE) Results -> MakeList;
        Index = 0;

        while (0 != *(UsageList+Index) && Index < Results -> ListLength) 
        {
            vCreateUsageString(UsageList + Index,
                               szTempBuffer,
                               TEMP_BUFFER_SIZE);

            OUTSTRING(hOutputWindow, szTempBuffer);
            Index++;
        }

        OUTSTRING(hOutputWindow, "Break List");
        OUTSTRING(hOutputWindow, "==========");

        UsageList = (PUSAGE) Results -> BreakList;
        Index = 0;

        while (0 != *(UsageList+Index) && Index < Results -> ListLength) 
        {
            vCreateUsageString(UsageList + Index,
                               szTempBuffer,
                               TEMP_BUFFER_SIZE);

            OUTSTRING(hOutputWindow, szTempBuffer);
            Index++;
        }
        break;

    /*
    // These functions simply update the buffer that is specified as the 
    //   input parameter.  We must select the correct display buffer mechanism
    //   based on the ReportType for the call and then update the given report
    //   in that display mechanism.
    */
    
    case HID_READ_REPORT:
    case HIDD_GET_FEATURE:
    case HIDD_GET_INPUT_REPORT:
    case HIDP_INITIALIZE_REPORT_FOR_ID:
    case HIDP_SET_BUTTONS:
    case HIDP_SET_DATA:
    case HIDP_SET_SCALED_USAGE_VALUE:
    case HIDP_SET_USAGES:
    case HIDP_SET_USAGE_VALUE:
    case HIDP_SET_USAGE_VALUE_ARRAY:
    case HIDP_UNSET_BUTTONS:
    case HIDP_UNSET_USAGES:
        SELECT_ON_REPORT_TYPE(Results -> ReportType,
                              pInputDisplay,
                              pOutputDisplay,
                              pFeatureDisplay,
                              pDisplay);

        if(NULL == pDisplay)
        {
            return; // error case
        }

        BufferDisplay_UpdateBuffer(pDisplay,
                                   Results->ReportBuffer,
                                   Results->ReportLength);
        break;
    }
    return;
}

VOID 
vExecuteAndDisplayOutput(
    HWND              hOutputWindow,
    PHID_DEVICE       pDevice,
    INT               iFuncCall,
    PECDISPLAY_PARAMS params
)
/*++
RoutineDescription:
    This routine is a long function that is responsible for retrieving all the 
    paramter for a given function call, setting up the CallParameters structure
    and then call the execute routine to get the necessary results and status of 
    the operation.  It is then responsible for displaying the appropriate status
    and results if the function did not fail

    This routine is a fairly long, complex routine to do a simple task.  It may
    be broken down in future versions to simplify some of the complexity.
--*/
{
    EXTCALL_PARAMS    CallParameters;
    EXTCALL_STATUS    CallStatus;

    DLGBOX_STATUS       iDlgStatus;
    BOOL                ExecuteStatus;
    PBUFFER_DISPLAY     pBufferDisplay = NULL;
    PCHAR               pCopyBuffer = NULL;
    UINT                ulCopyBufferLength = 0;
    PCHAR               endp;
    UINT                DlgBoxNumber = 0;
    BOOL                List2Alloc;
    BOOL                MakeListAlloc;
    BOOL                BreakListAlloc;
    ULONG               newListLength = 0;

    HID_DEVICE      readDevice = {0};
    HID_DEVICE      writeDevice = {0};
    BOOL            status = TRUE;
    HRESULT         stringReturn;
    UINT            tmpLen = 0;

    // Check for overrun condition
    if(params->ListLength2 >= 2*params->ListLength + 2)
    {
        OUTSTRING(hOutputWindow, "Error in input parameters");
        return;
    }

    RtlZeroMemory(&CallStatus, sizeof(CallStatus));

    if(params->szListString2)
    {
        ZeroMemory(params->szListString2, params->ListLength2+1);
    }

    if(params->UsageList)
    {
        ZeroMemory(params->UsageList, params->ListLength2+1);
    }

    /*
    // ExecuteAndDisplayOutput needless to say, consists of two parts: 
    //    Executing and Displaying output.  The first section involves the
    //     execution phase where all parameters are filled in if necessary
    //     and ECDisp_Execute is called
    */

    if (IS_NOT_IMPLEMENTED(iFuncCall)) 
    {
        OUTSTRING(hOutputWindow, "Function not yet implemented");
        return;
    }

    /*
    // Check first to see if this is a HID_CLEAR_REPORT command.  If it is
    //    all we need to do is get the report buffer that is checked and
    //    then call the clear buffer command
    */

    if (HID_CLEAR_REPORT == iFuncCall) 
    {
        SELECT_ON_REPORT_TYPE(params -> ReportType,
                              pInputDisplay,
                              pOutputDisplay,
                              pFeatureDisplay,
                              pBufferDisplay);

        BufferDisplay_ClearBuffer(pBufferDisplay);
        return;
    }

    /*
    // Need to perform the following steps in order to get the parameters for
    //    our call and then execute the call:
    //      1) Get any additional parameters not supplied by the above dialog
    //           procedure.  This occurs for such functions as:
    //                  HIDP_SET_BUTTONS
    //                  HIDP_SET_DATA
    //                  HIDP_SET_USAGES
    //                  HIDP_SET_USAGE_VALUE
    //                  HIDP_SET_SCALED_USAGE_VALUE
    //                  HIDP_SET_USAGE_VALUE_ARRAY
    //                  HIDP_UNSET_BUTTONS
    //                  HIDP_UNSET_USAGES
    //          For these functions, a separate dialog box must be called
    //
    //      2) Fill in the common parameters from the passed in params struct
    //
    */

    /*
    // Step 1: We're storing the values retrieved by these additional dialog
    //          box in the params struct since we may actually be passed in
    //          these values in the future instead of getting them here.  Hence,
    //          we won't break any of the code that follows the switch statement
    */

    switch (iFuncCall) 
    {
    case HIDP_SET_BUTTONS:
    case HIDP_SET_USAGES:
    case HIDP_UNSET_BUTTONS:
    case HIDP_UNSET_USAGES:

        switch (iFuncCall) 
        {
        case HIDP_SET_BUTTONS:
            DlgBoxNumber = IDD_SET_BUTTONS;
            break;

        case HIDP_SET_USAGES:
            DlgBoxNumber = IDD_SET_USAGES;
            break;

        case HIDP_UNSET_BUTTONS:
            DlgBoxNumber = IDD_UNSET_BUTTONS;
            break;

        case HIDP_UNSET_USAGES:
            DlgBoxNumber = IDD_UNSET_USAGES;
            break;
        }

        iDlgStatus = (DLGBOX_STATUS) DialogBoxParam(NULL,
                                                    MAKEINTRESOURCE(DlgBoxNumber),
                                                    GetParent(hOutputWindow),
                                                    bSetUsagesDlgProc,
                                                    (LPARAM) params);
        /*                      
        // If the above call returns 1, then the dialog box routine
        //     successfully acquired a string from the user and put the
        //     pointer to it in params -> szListString.
        //     Now we need to convert the string to a usage list
        */

        if (DLGBOX_OK != iDlgStatus) 
            return;

        if (!ConvertStringToUsageList(params -> szListString,
                                                 &params -> UsageList,
                                                 &newListLength))
        {
            ECDISP_ERROR(GetParent(hOutputWindow),
                         "Error getting usage list");

            free(params -> szListString);             
            return;
        }
        
        if(params->ListLength >= newListLength)
        {
            params->ListLength = newListLength;
        }
        else
        {
            OUTSTRING(hOutputWindow, "Error in ListLength");
            free(params -> szListString);
            return;
        }
        free(params -> szListString);

        break;

    case HIDP_GET_EXTENDED_ATTRIBUTES:
        iDlgStatus = (DLGBOX_STATUS) DialogBoxParam(NULL,
                                                    MAKEINTRESOURCE(IDD_GET_EXT_ATTRIB),
                                                    GetParent(hOutputWindow),
                                                    bGetIndexedDlgProc,
                                                    (LPARAM) params);

        if (DLGBOX_OK != iDlgStatus) 
            return;

        params -> Index = strtoul(params -> szListString, &endp, 10);
        
        if ('\0' != *endp) 
        {
            ECDISP_ERROR(GetParent(hOutputWindow),
                         "Invalid index value");

            free(params -> szListString);
            free(params -> szListString2);
            return;
        }
        
        free(params -> szListString);

#pragma prefast(suppress:__WARNING_INVALID_PARAM_VALUE_1 , "szListString2 cannot be zero")
        tmpLen = strtoul(params -> szListString2, &endp, 10);

        if ('\0' != *endp || tmpLen > params->ListLength) 
        {
            ECDISP_ERROR(GetParent(hOutputWindow),
                         "Invalid buffer size");

            free(params -> szListString2);
            return;
        }
        params -> ListLength = tmpLen;
        free(params -> szListString2);
        break;

    case HIDD_GET_INDEXED_STRING:
        iDlgStatus = (DLGBOX_STATUS) DialogBoxParam(NULL,
                                                    MAKEINTRESOURCE(IDD_GET_INDEX_STRING),
                                                    GetParent(hOutputWindow),
                                                    bGetIndexedDlgProc,
                                                    (LPARAM) params);

        if (DLGBOX_OK != iDlgStatus) 
            return;

        if (NULL == params->szListString || NULL == params->szListString2)
        {   
            OUTSTRING(hOutputWindow, "Error in szListString");
            return;
        }

        params -> Index = strtoul(params -> szListString, &endp, 10);
        
        if ('\0' != *endp) 
        {
            ECDISP_ERROR(GetParent(hOutputWindow),
                         "Invalid index value");

            free(params -> szListString);
            free(params -> szListString2);                            
            return;
        }
        
        free(params -> szListString);
        tmpLen = strtoul(params -> szListString2, &endp, 10);

        if ('\0' != *endp || tmpLen > params->ListLength) 
        {
            ECDISP_ERROR(GetParent(hOutputWindow),
                         "Invalid buffer size");
            free(params -> szListString2);
            return;
        }
        params -> ListLength = tmpLen;
        free(params -> szListString2);
        break;
        
    case HIDD_GET_MS_GENRE_DESCRIPTOR:
    case HIDD_GET_PHYSICAL_DESCRIPTOR:
    case HIDD_GET_MANUFACTURER_STRING:
    case HIDD_GET_PRODUCT_STRING:
    case HIDD_GET_SERIAL_NUMBER_STRING:
        iDlgStatus = (DLGBOX_STATUS) DialogBoxParam(NULL,
                                                    MAKEINTRESOURCE(IDD_SET_BUFFER_LENGTH),
                                                    GetParent(hOutputWindow),
                                                    bSetBufLenDlgProc,
                                                    (LPARAM) params);

        if (DLGBOX_OK != iDlgStatus) 
            return;

        tmpLen = strtoul(params -> szListString, &endp, 10);


        if (tmpLen > params->ListLength || '\0' != *endp) 
        {
            free(params -> szListString);     
            params->ListLength = 0;
            ECDISP_ERROR(GetParent(hOutputWindow),
                         "Invalid buffer length");
            return;
        }
        params -> ListLength = tmpLen;        
        free(params -> szListString);
        break;

    case HIDD_SET_NUM_INPUT_BUFFERS:
        iDlgStatus = (DLGBOX_STATUS) DialogBoxParam(NULL,
                                                    MAKEINTRESOURCE(IDD_SET_INPUT_BUFFERS),
                                                    GetParent(hOutputWindow),
                                                    bSetInputBuffDlgProc,
                                                    (LPARAM) params);
        /*
        // If the above call returns 1, then the dialog box routine
        //     successfully acquired a string from the user and put the
        //     pointer to it in params -> szListString.
        //     Now we need to convert the string to a usage list
        */

        if (DLGBOX_OK != iDlgStatus) 
           return;

        params -> Value = strtoul(params -> szListString, &endp, 10);

        if ('\0' != *endp) 
        {
            ECDISP_ERROR(GetParent(hOutputWindow),
                         "Invalid value specified");

            free(params -> szListString);
            return;
        }
        free(params -> szListString);
        break;

    case HIDP_SET_DATA:
        iDlgStatus = (DLGBOX_STATUS) DialogBoxParam(NULL,
                                                    MAKEINTRESOURCE(IDD_SET_DATA),
                                                    GetParent(hOutputWindow),
                                                    bSetDataDlgProc,
                                                    (LPARAM) params);

        if (DLGBOX_OK != iDlgStatus) 
            return;

        break;

    case HIDP_SET_SCALED_USAGE_VALUE:
        iDlgStatus = (DLGBOX_STATUS) DialogBoxParam(NULL,
                                                    MAKEINTRESOURCE(IDD_SET_SCALED_VALUE),
                                                    GetParent(hOutputWindow),
                                                    bSetValueDlgProc,
                                                    (LPARAM) params);
        /*
        // If the above call returns DLGBOX_OK, then the dialog box routine
        //     successfully acquired a string from the user and put the
        //     pointer to it in params -> szListString.
        //     Now we need to convert the string to a usage list
        */

        if (DLGBOX_OK != iDlgStatus) 
            return;

        params -> ScaledValue = strtol(params -> szListString, &endp, 10);

        if ('\0' != *endp) 
        {
            ECDISP_ERROR(GetParent(hOutputWindow),
                         "Invalid scaled usage value");

            free(params -> szListString);
            return;
        }

        free(params -> szListString);
        break;

    case HIDP_SET_USAGE_VALUE:
        iDlgStatus = (DLGBOX_STATUS) DialogBoxParam(NULL,
                                                    MAKEINTRESOURCE(IDD_SET_USAGE_VALUE),
                                                    GetParent(hOutputWindow),
                                                    bSetValueDlgProc,
                                                    (LPARAM) params);
        /*
        // If the above call returns 1, then the dialog box routine
        //     successfully acquired a string from the user and put the
        //     pointer to it in params -> szListString.
        //     Now we need to convert the string to a usage list
        */

        if (DLGBOX_OK != iDlgStatus) 
           return;

        params -> Value = strtoul(params -> szListString, &endp, 10);
        
        if ('\0' != *endp) 
        {
            ECDISP_ERROR(GetParent(hOutputWindow),
                         "Invalid usage value");

            free(params -> szListString);
            return;
        }
        free(params -> szListString);
        break;


    case HIDP_SET_USAGE_VALUE_ARRAY:
        iDlgStatus = (DLGBOX_STATUS) DialogBoxParam(NULL,
                                                    MAKEINTRESOURCE(IDD_SET_USAGE_VALUE_ARRAY),
                                                    GetParent(hOutputWindow),
                                                    bSetValueDlgProc,
                                                    (LPARAM) params);

        /*
        // If the above call returns 1, then the dialog box routine
        //     successfully acquired a string from the user and put the
        //     pointer to it in params -> szListString.
        //     Now we need to convert the string to a usage list
        */

        if (DLGBOX_OK != iDlgStatus) 
            return;

        ExecuteStatus = ConvertStringToUlongList(params -> szListString,
                                                 &params -> pValueList,
                                                 &params -> ListLength);
        free(params -> szListString);
        params->szListString = NULL;

        if (!ExecuteStatus) 
        {
            ECDISP_ERROR(GetParent(hOutputWindow),
                         "Error getting list of values");
            return;
        }
        break;

    case HIDP_USAGE_LIST_DIFFERENCE:
        iDlgStatus = (DLGBOX_STATUS) DialogBoxParam(NULL,
                                                    MAKEINTRESOURCE(IDD_USAGE_LIST_DIFFERENCE),
                                                    GetParent(hOutputWindow),
                                                    bGetUsageDiffDlgProc,
                                                    (LPARAM) params);

        if (DLGBOX_OK != iDlgStatus) 
        {
            return;
        }

        ExecuteStatus = Strings_StringToUnsignedList(params -> szListString,
                                                     sizeof(USAGE),
                                                     16,
                                                     (PCHAR *) &params -> UsageList,
                                                     &params -> ListLength);

        if (!ExecuteStatus) 
        {
            ECDISP_ERROR(GetParent(hOutputWindow),
                         "Error getting list of values");

            free(params -> szListString);
            params->szListString = NULL;
            free(params -> szListString2);
            params->szListString2 = NULL;
            free(params -> UsageList);
            params->UsageList = NULL;
            free(params -> UsageList2);
            params->UsageList2 = NULL;

            params->ListLength = 0;
            params->ListLength2 = 0; 
            
            return;
        }

#pragma warning(push)
#pragma warning(disable:26045) 

#pragma prefast(suppress:__WARNING_INVALID_PARAM_VALUE_1 , "szListString2 cannot be zero")
        ExecuteStatus = Strings_StringToUnsignedList(params -> szListString2,
                                                     sizeof(USAGE),
                                                     16, 
                                                     (PCHAR *) &params -> UsageList2,
                                                     &params -> ListLength2);

        if (!ExecuteStatus) 
        {
            ECDISP_ERROR(GetParent(hOutputWindow),
                         "Error getting list of values");

            free(params -> szListString);
            params->szListString = NULL;
            free(params -> szListString2);
            params->szListString2 = NULL;
            free(params -> UsageList);
            params->UsageList = NULL;
            free(params -> UsageList2);
            params->UsageList2 = NULL;

            params->ListLength = 0;
            params->ListLength2 = 0; 
            
            return;
        }

        free(params -> szListString);
        params->szListString = NULL;

       
        free(params -> szListString2);
        params->szListString2 = NULL;

        params->ListLength = 0;
        params->ListLength2 = 0; 
        
        break;
    }

    /*
    // Step 2: Extract the common parameters.  It's probably easier
    //    to simply fill in the spots in call parameters whether they are used
    //    or not instead of filling in only those that are relevant to a given
    //    function.  The details of some function relevant parameters are 
    //    handled after this.
    */

    CallParameters.DeviceHandle   = pDevice -> HidDevice;
    CallParameters.ReportType     = params -> ReportType;
    CallParameters.Ppd            = pDevice -> Ppd;
    CallParameters.UsagePage      = params -> UsagePage;
    CallParameters.Usage          = params -> Usage;
    CallParameters.LinkCollection = params -> LinkCollection;
    CallParameters.ReportID       = params -> ReportID;
    CallParameters.List           = NULL;
    CallParameters.List2          = NULL;
    CallParameters.MakeList       = NULL;
    CallParameters.BreakList      = NULL;
    CallParameters.ListLength     = 0;

    List2Alloc     = FALSE;
    MakeListAlloc  = FALSE;
    BreakListAlloc = FALSE;

    /*
    // Step 3: Now we'll deal with those functions that require a report buffer of some kind
    //    which means we'll copy the current buffer of the selected reported
    //    type
    */

    switch (iFuncCall) 
    {
    case HID_READ_REPORT:

        status = OpenHidDevice(pDevice -> DevicePath,
                               TRUE,
                               FALSE,
                               FALSE,
                               FALSE,
                               &readDevice);
        
        if (!status) 
        {
            OUTSTRING(hOutputWindow, "Unable to open device for reading");
        }
        
        CallParameters.DeviceHandle  = readDevice.HidDevice;
        CallParameters.ReportType   = HidP_Input;
        CallParameters.ReportBuffer = readDevice.InputReportBuffer;
        CallParameters.ReportLength = readDevice.Caps.InputReportByteLength;
        break;

    case HID_WRITE_REPORT:

        status = OpenHidDevice(pDevice -> DevicePath,
                               TRUE,
                               FALSE,
                               FALSE,
                               FALSE,
                               &writeDevice);
                               
        if (!status) 
        {
            OUTSTRING(hOutputWindow, "Unable to open device for writing");
        }

#pragma warning(push)
#pragma warning(disable:22116)
        BufferDisplay_CopyCurrentBuffer(pOutputDisplay,
                                        writeDevice.OutputReportBuffer,
                                        writeDevice.Caps.OutputReportByteLength);
#pragma warning(pop)

        CallParameters.DeviceHandle  = writeDevice.HidDevice;
        CallParameters.ReportType    = HidP_Output;
        CallParameters.ReportBuffer  = writeDevice.OutputReportBuffer;
        CallParameters.ReportLength  = writeDevice.Caps.OutputReportByteLength;
        break;

    case HIDD_GET_INPUT_REPORT:
        CallParameters.ReportType   = HidP_Input;
        CallParameters.ReportBuffer = pDevice -> InputReportBuffer;
        CallParameters.ReportLength = pDevice -> Caps.InputReportByteLength;
        break;

    case HIDD_GET_FEATURE:
        CallParameters.ReportType   = HidP_Feature;
        CallParameters.ReportBuffer = pDevice -> FeatureReportBuffer;
        CallParameters.ReportLength = pDevice -> Caps.FeatureReportByteLength;
        break;

    case HIDD_GET_INDEXED_STRING:
    case HIDP_GET_EXTENDED_ATTRIBUTES:
        CallParameters.Index = params -> Index;
        CallParameters.ListLength  = params -> ListLength;
        break;

    case HIDD_SET_OUTPUT_REPORT:
        CallParameters.ReportType = HidP_Output;
        BufferDisplay_CopyCurrentBuffer(pOutputDisplay,
                                        pDevice->OutputReportBuffer,
                                        pDevice->Caps.OutputReportByteLength);
        CallParameters.ReportLength = BufferDisplay_GetBufferSize(pOutputDisplay);
        CallParameters.ReportBuffer = pDevice -> OutputReportBuffer;
        break;

    case HIDD_SET_FEATURE:
       CallParameters.ReportType = HidP_Feature;
       BufferDisplay_CopyCurrentBuffer(pFeatureDisplay,
                                       pDevice->FeatureReportBuffer,
                                       pDevice->Caps.FeatureReportByteLength);
       CallParameters.ReportLength = BufferDisplay_GetBufferSize(pFeatureDisplay);
       CallParameters.ReportBuffer = pDevice -> FeatureReportBuffer;
       break;

    case HIDP_GET_BUTTONS:
    case HIDP_GET_BUTTONS_EX:
    case HIDP_GET_DATA:
    case HIDP_GET_SCALED_USAGE_VALUE:
    case HIDP_GET_USAGES:
    case HIDP_GET_USAGES_EX:
    case HIDP_GET_USAGE_VALUE:
    case HIDP_GET_USAGE_VALUE_ARRAY:
    case HIDP_INITIALIZE_REPORT_FOR_ID:
    case HIDP_SET_BUTTONS:
    case HIDP_SET_DATA:
    case HIDP_SET_SCALED_USAGE_VALUE:
    case HIDP_SET_USAGES:
    case HIDP_SET_USAGE_VALUE:
    case HIDP_SET_USAGE_VALUE_ARRAY:
    case HIDP_UNSET_BUTTONS:
    case HIDP_UNSET_USAGES:
        
        switch (CallParameters.ReportType) 
        {
        case HidP_Input:
            pBufferDisplay = pInputDisplay;
            pCopyBuffer    = pDevice -> InputReportBuffer;
            ulCopyBufferLength = pDevice->Caps.InputReportByteLength;
            break;

        case HidP_Output:
            pBufferDisplay = pOutputDisplay;
            pCopyBuffer    = pDevice -> OutputReportBuffer;
            ulCopyBufferLength = pDevice->Caps.OutputReportByteLength;
            break;

        case HidP_Feature:
            pBufferDisplay = pFeatureDisplay;
            pCopyBuffer    = pDevice -> FeatureReportBuffer;
            ulCopyBufferLength = pDevice->Caps.FeatureReportByteLength;
            break;

        }

        if(NULL == pBufferDisplay)
        {
            return; // error case
        }
        BufferDisplay_CopyCurrentBuffer(pBufferDisplay,
                                        pCopyBuffer,
                                        ulCopyBufferLength);

        CallParameters.ReportLength = BufferDisplay_GetBufferSize(pBufferDisplay);
        CallParameters.ReportBuffer = pCopyBuffer;
        break;

    default:
        CallParameters.ReportLength = 0;
        CallParameters.ReportBuffer = NULL;
    }

    /*
    // Now, we need to deal with those functions which have a List that is 
    //   used for either retrieving or gathering data.  There are two different
    //   cases.  The first involves the user inputting a buffer and the system 
    //   performing some action on the buffer, such as SetButtons.  We'll also 
    //   the other functions that require one of the union fields to be set.
    //   
    */

    /*
    // The second case is where data is retrieved for the device.  In this case,
    //     all we do is specify either the number of elements need for the buffer,
    //     the execute routine will worry about allocating the correct amount of
    //     space for those elements.  Remember, however, that if the Execute routine
    //     allocates space, we need to free it up.
    */

    /*
    // Then there's the third case UsageListDifference which truly changes
    //   everything.  We've got to determine the size of the resulting lists
    //   is the MaxSize of the other two lists.  Plus, we need to insure that 
    //   our buffers are 00 terminated if they are less than the max size, ie
    //   there not the same size as the larger buffer.  This may require
    //   reallocation of the block.
    */

    switch (iFuncCall) 
    {
    /*
    // First Case functions
    */

    case HIDP_SET_DATA:
        CallParameters.List       = (PVOID) params -> pDataList;
        CallParameters.ListLength = params -> ListLength;
        break;

    case HIDP_SET_BUTTONS:
    case HIDP_UNSET_BUTTONS:
    case HIDP_SET_USAGES:
    case HIDP_UNSET_USAGES:
        CallParameters.List       = (PVOID) params -> UsageList;
        CallParameters.ListLength = params -> ListLength;
        break;

    case HIDP_SET_USAGE_VALUE_ARRAY:
        CallParameters.List       = (PVOID) params -> pValueList;
        CallParameters.ListLength = params -> ListLength;
        break;

    /*
    // Second Case functions
    */

    case HIDP_GET_BUTTON_CAPS:
    case HIDP_GET_SPECIFIC_BUTTON_CAPS:
        SELECT_ON_REPORT_TYPE(CallParameters.ReportType,
                              pDevice -> Caps.NumberInputButtonCaps,
                              pDevice -> Caps.NumberOutputButtonCaps,
                              pDevice -> Caps.NumberFeatureButtonCaps,
                              CallParameters.ListLength);
        break;

    case HIDP_GET_LINK_COLL_NODES:
        CallParameters.ListLength = pDevice -> Caps.NumberLinkCollectionNodes;
        break;

    case HIDD_GET_MS_GENRE_DESCRIPTOR:
    case HIDD_GET_PHYSICAL_DESCRIPTOR:
    case HIDD_GET_MANUFACTURER_STRING:
    case HIDD_GET_PRODUCT_STRING:
    case HIDD_GET_SERIAL_NUMBER_STRING:
        CallParameters.ListLength = params -> ListLength;
        break;

    case HIDP_GET_VALUE_CAPS:
    case HIDP_GET_SPECIFIC_VALUE_CAPS:
        SELECT_ON_REPORT_TYPE(CallParameters.ReportType,
                              pDevice -> Caps.NumberInputValueCaps,
                              pDevice -> Caps.NumberOutputValueCaps,
                              pDevice -> Caps.NumberFeatureValueCaps,
                              CallParameters.ListLength);

    case HIDD_GET_FREE_PREPARSED_DATA:
        CallParameters.ppPd = &CallParameters.Ppd;
        break;

    case HIDP_SET_SCALED_USAGE_VALUE:
        CallParameters.ScaledValue = params -> ScaledValue;
        break;

    case HIDP_SET_USAGE_VALUE:
    case HIDD_SET_NUM_INPUT_BUFFERS:
        CallParameters.Value = params -> Value;
        break;

    /*
    // That third case
    */

    case HIDP_USAGE_LIST_DIFFERENCE:
        CallParameters.ListLength = max(params -> ListLength,
                                        params -> ListLength2);

        CallParameters.List  = params -> UsageList;
        CallParameters.List2 = params -> UsageList2;

        if (CallParameters.ListLength > params -> ListLength) 
        {
            CallParameters.List = (PUSAGE) realloc(params -> UsageList,
                                                   (params -> ListLength+1) * sizeof(USAGE));

            if (NULL == CallParameters.List) 
            {
                ECDISP_ERROR(GetParent(hOutputWindow),
                             "Error allocating memory");

                free(params -> UsageList);
                free(params -> UsageList2);

                return;
            }

            *(((PUSAGE) CallParameters.List) + CallParameters.ListLength - 1) = 0;
        }
        else if (CallParameters.ListLength > params -> ListLength2) 
        {
            CallParameters.List2 = (PUSAGE) realloc(params -> UsageList2,
                                                   (params -> ListLength+1) * sizeof(USAGE));

            if (NULL == CallParameters.List2) 
            {
                ECDISP_ERROR(GetParent(hOutputWindow),
                             "Error allocating memory");

                free(params -> UsageList);
                free(params -> UsageList2);
                return;
            }

            *(((PUSAGE) CallParameters.List2) + CallParameters.ListLength - 1) = 0;
        }
        List2Alloc = TRUE;
        MakeListAlloc = TRUE;
        BreakListAlloc = TRUE;
        break;
    }

    /*
    // Params are now set up and ready to go, let's execute
    */

    if (HIDD_GET_FREE_PREPARSED_DATA == iFuncCall) 
    {
        ExecuteStatus = ECDisp_Execute(HIDD_GET_PREPARSED_DATA,
                                       &CallParameters,
                                       &CallStatus);

        if (!ExecuteStatus)
        {
            OUTSTRING(hOutputWindow, "Unknown error: Couldn't execute function");
            return;
        }

        DISPLAY_HIDD_STATUS(hOutputWindow, 
                            "HidD_GetPreparsedData",
                            CallStatus,
                            stringReturn);

        if (!CallStatus.IsHidError)
        {
            ExecuteStatus = ECDisp_Execute(HIDD_FREE_PREPARSED_DATA,
                                           &CallParameters,
                                           &CallStatus);

            OUTSTRING(hOutputWindow, "=======================");
            
            if (!ExecuteStatus) 
            {
                OUTSTRING(hOutputWindow, "Unknown error: Couldn't execute function");
                return;
            }

            DISPLAY_HIDD_STATUS(hOutputWindow, 
                                "HidD_FreePreparsedData",
                                CallStatus,
                                stringReturn);
        }
    }
    else
    {
        if ((HID_READ_REPORT == iFuncCall || HID_WRITE_REPORT == iFuncCall) &&
            (!status)) 
        {
            //
            // Indicate there was an error so we don't display anything further
            //
        
            CallStatus.IsHidError = TRUE;
        }
        else 
        {    
            ExecuteStatus = ECDisp_Execute(iFuncCall,
                                           &CallParameters,
                                           &CallStatus);

            if (!ExecuteStatus) 
            {
                OUTSTRING(hOutputWindow, "Unknown error: Couldn't execute function");
                return;
            }

            if (IS_HIDD_FUNCTION(iFuncCall) || IS_HID_FUNCTION(iFuncCall)) 
            {
                DISPLAY_HIDD_STATUS(hOutputWindow, 
                                    GET_FUNCTION_NAME(iFuncCall),
                                    CallStatus,
                                    stringReturn);

            }
            else 
            {
                DISPLAY_HIDP_STATUS(hOutputWindow,
                                    GET_FUNCTION_NAME(iFuncCall),
                                    CallStatus,
                                    stringReturn);
            }
        }            
    }
    

    /*
    // Display the other results only if there wasn't a HID error
    */

    if (!CallStatus.IsHidError || (HIDP_STATUS_NULL == CallStatus.HidErrorCode)) 
    {
        OUTSTRING(hOutputWindow, "=======================");

        /*
        // Now that general status information has been displayed, we need to
        //   display the info for the parts that are dependent on the function being
        //   called
        */
    
        ECDisp_DisplayOutput(hOutputWindow,
                            iFuncCall,
                            &CallParameters);
    }

    if (CallParameters.List != NULL) 
    {
        free(CallParameters.List);
    }

    if (List2Alloc && CallParameters.List2 != NULL) 
    {
        free(CallParameters.List2);
    }

    if (MakeListAlloc && CallParameters.MakeList != NULL) 
    {
        free(CallParameters.MakeList);
    }

    if (BreakListAlloc && CallParameters.BreakList != NULL)
    {
        free(CallParameters.BreakList);
    }

    return;
}
#pragma warning(pop)

VOID
BuildReportIDList(
    IN  PHIDP_BUTTON_CAPS  phidButtonCaps,
    IN  USHORT             nButtonCaps,
    IN  PHIDP_VALUE_CAPS   phidValueCaps,
    IN  USHORT             nValueCaps,
    OUT PUCHAR            *ppReportIDList,
    OUT INT               *nReportIDs
)
/*++
RoutineDescription:
    This routine builds a list of report IDs that are listed in the passed in set
    of ButtonCaps and ValueCaps structure.  It allocates a buffer to store all
    the ReportIDs, if it can.  Otherwise the buffer is returned as NULL.

    Currently, this routine has no purpose in the HClient program. It was written
    for some purpose which never materialized but was left in because it might be
    useful in the future.
--*/
{    
    INT               nAllocatedIDs;
    INT               nFoundIDs;
    INT               nWalkCount;
    USHORT            usIndex;
    BOOL              fIDFound;
    UCHAR             *pucBuffer;
    UCHAR             *pucNewBuffer;
    UCHAR             *pucWalk;
    UCHAR             ucReportID;
    PHIDP_BUTTON_CAPS pButtonWalk;

    UNREFERENCED_PARAMETER(phidValueCaps);

    /*
    // Initialize the output parameters in case there is some sort of failure
    */

    *nReportIDs = 0;
    *ppReportIDList = NULL;

    if (0 == nButtonCaps && 0 == nValueCaps)
        return;

    /*
    // Initialize the beginning array size to 2 report IDs and alloc space
    // for those IDs.  If we need to add more report IDs we allocate more
    // space
    */

    nAllocatedIDs = 2;
    nFoundIDs = 0;
    pButtonWalk = phidButtonCaps;
                                              
    pucBuffer = (UCHAR *) malloc(sizeof(UCHAR) * nAllocatedIDs);
    if (NULL == pucBuffer) 
        return;

    /*
    // Beginning with the button caps and then going to the value caps do the
    // following
    //
    // 1) Take the report ID and search the array of report IDs looking for 
    //       an existing report ID and add to the array if not there.  
    //
    // 2) Add the report ID to the array in sorted order that way we sort the
    //      array at any time.  
    // 
    // 3) Must also realloc the array if we run out of array space
    */

    for (usIndex = 0; usIndex < nButtonCaps; usIndex++, pButtonWalk++) 
    {
        ucReportID = pButtonWalk -> ReportID;
        
        pucWalk = pucBuffer;
        nWalkCount = 0;
        fIDFound = FALSE;

        while (!fIDFound && nWalkCount < nFoundIDs) 
        {
            if (*pucWalk == ucReportID) 
            {
                fIDFound = TRUE;
            }
            else if (ucReportID > *pucWalk) 
            {
                pucWalk++;
                nWalkCount++;
            }
        }

        if (!fIDFound) 
        {
            if (nFoundIDs == nAllocatedIDs) 
            {
                nAllocatedIDs *= 2;

                pucNewBuffer = (UCHAR *) realloc(pucBuffer, sizeof(UCHAR) * nAllocatedIDs);

                if (NULL == pucNewBuffer)
                {
                    free(pucBuffer);
                    pucBuffer = NULL;
                    return;
                }
                else
                {
                    pucBuffer = pucNewBuffer;
                }

                pucWalk = pucBuffer + nWalkCount;
            }

            /*
            // At this point, pucWalk points to the smallest ReportID in the
            //   buffer that is greater than the ReportID we want to insert.
            //   We need to bump all reportIDs beginning at pucWalk up one 
            //   spot and insert the new ReportID at pucWalk
            */

            memmove (pucWalk+1, pucWalk, (nFoundIDs - nWalkCount) * sizeof(UCHAR));
            *pucWalk = ucReportID;
            nFoundIDs++;
        }
    }

    *ppReportIDList = pucBuffer;
    *nReportIDs = nFoundIDs;
    
    return;
}

LRESULT CALLBACK
bSetUsagesDlgProc(
    HWND hDlg,
    UINT message,
    WPARAM wParam,
    LPARAM lParam
)
{
    static PECDISPLAY_PARAMS  pParams;
           INT                StringLength;
           DLGBOX_STATUS      RetValue;

    switch (message) 
    {
    case WM_INITDIALOG:
        
        pParams = (PECDISPLAY_PARAMS) lParam;

        SetDlgItemIntHex(hDlg, 
                         IDC_USAGE_PAGE, 
                         pParams -> UsagePage,
                         2);
        break;

    case WM_COMMAND:
        switch (LOWORD(wParam)) 
        {
        case IDOK:
            StringLength = GetWindowTextLength(GetDlgItem(hDlg, IDC_USAGE_LIST));

            if (StringLength > 0) 
            {
                pParams -> szListString = (PCHAR) malloc(StringLength+1);
                if (NULL == pParams -> szListString) 
                {
                    ECDISP_ERROR(hDlg, "Error allocating memory");
                    RetValue = DLGBOX_ERROR;
                }
                else
                {
                    GetWindowText(GetDlgItem(hDlg, IDC_USAGE_LIST),
                                  pParams -> szListString,
                                  StringLength+1);

                    RetValue = DLGBOX_OK;
                }
            }
            else 
            {
                pParams -> szListString = NULL;
                RetValue = DLGBOX_CANCEL;
            }

            EndDialog(hDlg, RetValue);
            break;

        case IDCANCEL:
            EndDialog(hDlg, DLGBOX_CANCEL);
            break;
        }
        break;
    }
    return (FALSE);
}

LRESULT CALLBACK
bSetValueDlgProc(
    HWND hDlg,
    UINT message,
    WPARAM wParam,
    LPARAM lParam
)
{
    static PECDISPLAY_PARAMS  pParams;
           INT                StringLength;
           DLGBOX_STATUS      RetValue;

    switch (message) 
    {
    case WM_INITDIALOG:

        pParams = (PECDISPLAY_PARAMS) lParam;

        SetDlgItemIntHex(hDlg, 
                         IDC_USAGE_PAGE, 
                         pParams -> UsagePage,
                         sizeof(USAGE));

        SetDlgItemIntHex(hDlg,
                         IDC_USAGE,
                         pParams -> Usage,
                         sizeof(USAGE));
        break;

    case WM_COMMAND:
        switch (LOWORD(wParam)) 
        {
        case IDOK:
            StringLength = GetWindowTextLength(GetDlgItem(hDlg, IDC_VALUE));

            if (StringLength > 0) 
            {
                pParams -> szListString = (PCHAR) malloc(StringLength+1);
                if (NULL == pParams -> szListString) 
                {
                    ECDISP_ERROR(hDlg, "Error allocating memory");

                    RetValue = DLGBOX_ERROR;
                }
                else 
                {
                    GetWindowText(GetDlgItem(hDlg, IDC_VALUE),
                                  pParams -> szListString,
                                  StringLength+1);

                    RetValue = DLGBOX_OK;
                }
            }
            else
            {
                pParams -> szListString = NULL;
                RetValue = DLGBOX_CANCEL;
            }

            EndDialog(hDlg, RetValue);
            break;

        case IDCANCEL:
            EndDialog(hDlg, DLGBOX_CANCEL);
            break;
        }
        break;
    }
    return (FALSE);
}

LRESULT CALLBACK
bSetInputBuffDlgProc(
    HWND hDlg,
    UINT message,
    WPARAM wParam,
    LPARAM lParam
)
{
    static PECDISPLAY_PARAMS  pParams;
           INT                StringLength;
           DLGBOX_STATUS      RetValue;

    switch (message) 
    {
    case WM_INITDIALOG:
        pParams = (PECDISPLAY_PARAMS) lParam;
        break;

    case WM_COMMAND:
        switch (LOWORD(wParam)) 
        {
        case IDOK:
            StringLength = GetWindowTextLength(GetDlgItem(hDlg, IDC_INPUT_BUFFERS));

            if (StringLength > 0) 
            {
                pParams -> szListString = (PCHAR) malloc(StringLength+1);

                if (NULL == pParams -> szListString) 
                {
                    ECDISP_ERROR(hDlg, "Error allocating memory");

                    RetValue = DLGBOX_ERROR;
                }
                else 
                {
                    GetWindowText(GetDlgItem(hDlg, IDC_INPUT_BUFFERS),
                                  pParams -> szListString,
                                  StringLength+1);

                    RetValue = DLGBOX_OK;
                }
            }
            else 
            {
                pParams -> szListString = NULL;
                RetValue = DLGBOX_CANCEL;
            }

            EndDialog(hDlg, RetValue);
            break;

        case IDCANCEL:
            EndDialog(hDlg, DLGBOX_CANCEL);
            break;
        }
        break;
    }
    return (FALSE);
}

LRESULT CALLBACK
bSetDataDlgProc(
    HWND hDlg,
    UINT message,
    WPARAM wParam,
    LPARAM lParam
)
{
    static CHAR               DataString[TEMP_BUFFER_SIZE] = {0};
    static PECDISPLAY_PARAMS  pParams;
           UINT               IndexValue;
           ULONG              Value;
           BOOL               lpTranslated;
           DLGBOX_STATUS      RetValue;
           PCHAR              endp;
           INT                ListBoxStatus;
           PHIDP_DATA         DataList;
           PHIDP_DATA         CurrData;
           ULONG              DataListLength;
           ULONG              Index;

   ZeroMemory(DataString, sizeof(DataString));

   // Give PREFast a hint as it does not fully understand the annotations
   // on memset() yet
   _Analysis_assume_nullterminated_(DataString); 

    switch (message)
    {
    case WM_INITDIALOG:

        pParams = (PECDISPLAY_PARAMS) lParam;
        SendMessage(GetDlgItem(hDlg, IDC_VALUE),
                    EM_SETLIMITTEXT,
                    (WPARAM) 1024,
                    0);
        break;

    case WM_COMMAND:
        switch (LOWORD(wParam)) 
        {
        case IDC_ADD_DATA:
            IndexValue = GetDlgItemInt(hDlg,
                                       IDC_INDEX,
                                       &lpTranslated,
                                       FALSE);
            if (!lpTranslated) 
            {
                ECDISP_ERROR(hDlg,
                           "Invalid index value: must be unsigned integer");
                break;
            }
            
            if (0 == GetWindowText(GetDlgItem(hDlg, IDC_VALUE), 
                                   DataString, 1023)) 
            {                                          
                ECDISP_ERROR(hDlg, "Invalid data value");
                break;
            }

            CharUpperBuff(DataString, (DWORD)strlen(DataString));

            if (0 == lstrcmp(DataString, "TRUE"))
            {
                Value = 1;
            }
            else if (0 == lstrcmp(DataString, "FALSE")) 
            {
                Value = 0;
            }
            else 
            {
                Value = strtoul(DataString, &endp, 10);
                if ( (0 == Value) || (*endp != '\0') ) 
                {
                    ECDISP_ERROR(hDlg, "Invalid data value");
                    break;
                }
            }
            StringCbPrintf(DataString,
                           TEMP_BUFFER_SIZE,
                           SETDATA_LISTBOX_FORMAT, 
                           IndexValue,
                           Value);

            ListBoxStatus = (INT) SendMessage(GetDlgItem(hDlg, IDC_DATA_LIST),
                                              LB_ADDSTRING,
                                              0,
                                              (LPARAM) DataString);

            if (CB_ERR == ListBoxStatus || CB_ERRSPACE == ListBoxStatus) 
            {
                ECDISP_ERROR(hDlg, "Error adding string to data list");
                break;
            }
            break;

        case IDC_REMOVE_DATA:
            SendMessage(GetDlgItem(hDlg, IDC_DATA_LIST),
                        LB_DELETESTRING,
                        SendMessage(GetDlgItem(hDlg, IDC_DATA_LIST),
                                    LB_GETCURSEL,
                                    0, 0),
                        0);
            break;

        case IDOK:
            DataListLength = (ULONG) SendMessage(GetDlgItem(hDlg, IDC_DATA_LIST),
                                                 LB_GETCOUNT,
                                                 0, 0);

            if (0 != DataListLength)
            {
                DataList = malloc(DataListLength * sizeof(HIDP_DATA));
                if (NULL == DataList) 
                {
                    ECDISP_ERROR(hDlg, "Error allocating memory");

                    DataListLength = 0;
                    break;
                }
                
                for (Index = 0, CurrData = DataList; Index < DataListLength; Index++, CurrData++) 
                {
                    if (  SendMessage(GetDlgItem(hDlg, IDC_DATA_LIST), 
                                LB_GETTEXTLEN,
                                Index,
                                0) < ARRAYSIZE( DataString ) )
                    {
                            SendMessage(GetDlgItem(hDlg, IDC_DATA_LIST),
                                    LB_GETTEXT,
                                    Index,
                                    (LPARAM) DataString);
                    }

                    if (2 != sscanf_s(DataString, 
                           SETDATA_LISTBOX_FORMAT,
                           &IndexValue,
                           &Value))
                    {
                        ECDISP_ERROR(hDlg,
                           "Invalid DataString: expected 2 values");

                        CurrData -> DataIndex = (USHORT) IndexValue;
                        CurrData -> RawValue = Value;
                        
                    }
                    else
                    {

                        CurrData -> DataIndex = (USHORT) IndexValue;
                        CurrData -> RawValue = Value;
                    }
                }
                RetValue = DLGBOX_OK;
            }
            else 
            {
                DataList = NULL;
                RetValue = DLGBOX_CANCEL;
            }

            pParams -> pDataList = DataList;
            pParams -> ListLength = DataListLength;
            EndDialog(hDlg, RetValue);
            break;

        case IDCANCEL:
            EndDialog(hDlg, DLGBOX_CANCEL);
            break;
        }
        break;
    }
    return (FALSE);
}

LRESULT CALLBACK
bSetBufLenDlgProc(
    HWND hDlg,
    UINT message,
    WPARAM wParam,
    LPARAM lParam
)
{
    static PECDISPLAY_PARAMS  pParams;
           INT                StringLength;
           DLGBOX_STATUS      RetValue;

    switch (message) 
    { 
   case WM_INITDIALOG:

        pParams = (PECDISPLAY_PARAMS) lParam;
        break;

    case WM_COMMAND:
        switch (LOWORD(wParam)) 
        {
        case IDOK:
            StringLength = GetWindowTextLength(GetDlgItem(hDlg, IDC_BUFFER_LENGTH));

            if (StringLength > 0) 
            {
                pParams -> szListString = (PCHAR) malloc(StringLength+1);
                if (NULL == pParams -> szListString) 
                {
                    ECDISP_ERROR(hDlg, "Error allocating memory");

                    RetValue = DLGBOX_ERROR;

                }
                else 
                {
                    GetWindowText(GetDlgItem(hDlg, IDC_BUFFER_LENGTH),
                                  pParams -> szListString,
                                  StringLength+1);

                    RetValue = DLGBOX_OK;
                }
            }
            else
            {
                pParams -> szListString = NULL;
                RetValue = DLGBOX_CANCEL;
            }

            EndDialog(hDlg, RetValue);
            break;

        case IDCANCEL:
            EndDialog(hDlg, DLGBOX_CANCEL);
            break;
        }
        break;
    }
    return (FALSE);
}

LRESULT CALLBACK
bSetInputBuffersDlgProc(
    HWND hDlg,
    UINT message,
    WPARAM wParam,
    LPARAM lParam
)
{
    static PECDISPLAY_PARAMS  pParams;
           INT                StringLength;
           DLGBOX_STATUS      RetValue;

    switch (message) 
    {
    case WM_INITDIALOG:

        pParams = (PECDISPLAY_PARAMS) lParam;
        break;

    case WM_COMMAND:
        switch (LOWORD(wParam)) 
        {
        case IDOK:
            StringLength = GetWindowTextLength(GetDlgItem(hDlg, IDC_INPUT_BUFFERS));

            if (StringLength > 0) 
            {
                pParams -> szListString = (PCHAR) malloc(StringLength+1);

                if (NULL == pParams -> szListString) 
                {
                    ECDISP_ERROR(hDlg, "Error allocating memory");
                    RetValue = DLGBOX_ERROR;
                }
                else 
                {
                    GetWindowText(GetDlgItem(hDlg, IDC_INPUT_BUFFERS),
                                  pParams -> szListString,
                                  StringLength+1
                                 );
                    RetValue = DLGBOX_OK;
                }
            }
            else
            {
                pParams -> szListString = NULL;
                RetValue = DLGBOX_CANCEL;
            }

            EndDialog(hDlg, RetValue);
            break;

        case IDCANCEL:
            EndDialog(hDlg, DLGBOX_CANCEL);
            break;
        }
        break;
    }
    return (FALSE);
}

LRESULT CALLBACK
bGetIndexedDlgProc(
    HWND hDlg,
    UINT message,
    WPARAM wParam,
    LPARAM lParam
)
{
    static PECDISPLAY_PARAMS  pParams;
           INT                StringLength;
           INT                StringLength2;
           DLGBOX_STATUS      RetValue;

    switch (message) 
    {
    case WM_INITDIALOG:

        pParams = (PECDISPLAY_PARAMS) lParam;
        break;

    case WM_COMMAND:
        switch (LOWORD(wParam)) 
        {
        case IDOK:       
            StringLength  = GetWindowTextLength(GetDlgItem(hDlg, IDC_INDEX));
            StringLength2 = GetWindowTextLength(GetDlgItem(hDlg, IDC_BUFFER_LENGTH));

            if (StringLength <= 0 || StringLength2 <= 0) 
            {
                pParams -> szListString = NULL;
                pParams -> szListString2 = NULL;
                EndDialog(hDlg, DLGBOX_CANCEL);
            }

            pParams -> szListString = (PCHAR) malloc(StringLength+1);
            pParams -> szListString2 = (PCHAR) malloc(StringLength2+1);

            if (NULL == pParams -> szListString || NULL == pParams -> szListString2) 
            {
                   ECDISP_ERROR(hDlg, "Error allocating memory");

                   if (NULL != pParams -> szListString) 
                       free(pParams -> szListString);

                   if (NULL != pParams -> szListString2) 
                       free(pParams -> szListString2);

                   RetValue = DLGBOX_ERROR;
            }
            else 
            {
                GetWindowText(GetDlgItem(hDlg, IDC_INDEX),
                              pParams -> szListString,
                              StringLength+1);

                GetWindowText(GetDlgItem(hDlg, IDC_BUFFER_LENGTH),
                              pParams -> szListString2,
                              StringLength2+1);

                RetValue = DLGBOX_OK;
            }
            EndDialog(hDlg, RetValue);
            break;

        case IDCANCEL:
            EndDialog(hDlg, DLGBOX_CANCEL);
            break;
        }
        break;
    }
    return (FALSE);
}

LRESULT CALLBACK
bGetUsageDiffDlgProc(
    HWND hDlg,
    UINT message,
    WPARAM wParam,
    LPARAM lParam
)
{
    static PECDISPLAY_PARAMS  pParams;
           INT                StringLength;
           INT                StringLength2;
           DLGBOX_STATUS      RetValue;

    switch (message) 
    {
    case WM_INITDIALOG:

        pParams = (PECDISPLAY_PARAMS) lParam;
        break;

    case WM_COMMAND:
        switch (LOWORD(wParam)) 
        {
        case IDOK:       
            StringLength  = GetWindowTextLength(GetDlgItem(hDlg, IDC_USAGE_LIST1));
            StringLength2 = GetWindowTextLength(GetDlgItem(hDlg, IDC_USAGE_LIST2));

            if (StringLength <= 0) 
            {
                pParams -> szListString = NULL;
            }
            else 
            {
                pParams -> szListString = (PCHAR) malloc(StringLength+1);
                if (NULL == pParams -> szListString) 
                {
                    ECDISP_ERROR(hDlg,
                                 "Error allocating memory");

                    EndDialog(hDlg, DLGBOX_ERROR);
                    break;
                }
            }

            if (StringLength2 <= 0) 
            {
                pParams -> szListString2 = NULL;
            }
            else 
            {
                pParams -> szListString2 = (PCHAR) malloc(StringLength2+1);
                if (NULL == pParams -> szListString2) 
                {
                    ECDISP_ERROR(hDlg,
                                 "Error allocating memory");
                    EndDialog(hDlg, DLGBOX_ERROR);                    
                    if (NULL != pParams -> szListString)
                    {
                        free(pParams -> szListString);
                    }
                    break;
                }
            }

            if ( !(pParams->szListString) )
            {
                if(NULL == pParams->szListString)
                {
                    return(FALSE);  // error case
                }
            
                GetWindowText(GetDlgItem(hDlg, IDC_USAGE_LIST1),
                              pParams -> szListString,
                              StringLength+1);
            }

            if (pParams->szListString2 != NULL)
            {
                GetWindowText(GetDlgItem(hDlg, IDC_USAGE_LIST2),
                              pParams -> szListString2,
                              StringLength2+1);
            }

            RetValue = DLGBOX_OK;

            EndDialog(hDlg, RetValue);
            break;

        case IDCANCEL:
            EndDialog(hDlg, DLGBOX_CANCEL);
            break;
        }
        break;
    }
    return (FALSE);
}

BOOL
ConvertStringToUsageList(
    _In_    LPSTR InString,
    OUT    PUSAGE  *UsageList,
    OUT    PULONG  nUsages
)
/*++
RoutineDescription:
    This routine converts a string of values into a string of usages which are
    currently 2 byte values.  We use base 16 to specify that the usages should 
    be expressed as hexidecimal numbers.
--*/
{
    return (Strings_StringToUnsignedList(InString,
                                         sizeof(ULONG),
                                         16,
                                         (PCHAR *) UsageList,
                                         nUsages));
}

BOOL
ConvertStringToUlongList(
    _Inout_ LPSTR InString,
    OUT    PULONG  *UlongList,
    OUT    PULONG  nUlongs
)
/*++
RoutineDescription
    This routine converts a string of values into a string of ulongs which are
    currently 2 byte values.  It requires that the numbers in the string be in
    base 10
--*/
{
    return (Strings_StringToUnsignedList(InString,
                                         sizeof(ULONG),
                                         10,
                                         (PCHAR *) UlongList,
                                         nUlongs));
}

BOOL
SetDlgItemIntHex(
   HWND hDlg, 
   INT nIDDlgItem, 
   UINT uValue, 
   INT nBytes
)
{
    char szTempBuff[] = "0x00000000";

    assert (1 == nBytes || 2 == nBytes || 4 == nBytes);

    /*
    // Determine the width necessary to store the value
    */

    StringCbPrintf(szTempBuff,
                    (sizeof(szTempBuff)),
                    "0x%*X", (nBytes*2), uValue);

    SetDlgItemText(hDlg, nIDDlgItem, szTempBuff);

    return (TRUE);
}

VOID
ECDisp_MakeGUIDString(
    IN  GUID guid, 
    _Out_writes_bytes_(uiBuffSize) CHAR szString[],
    IN  UINT uiBuffSize
)
{
    CHAR szCharString[18];
    UINT i;

    for (i = 0; i < 8; i++) 
    {
        StringCbPrintf(&(szCharString[i]), sizeof(szCharString) - i,
                       "%x", guid.Data4[i]);
    }
    
    StringCbPrintf(szString, uiBuffSize,
                   "%x-%x%x-%s", guid.Data1, guid.Data2, guid.Data3, szCharString);
    return;
}

PCHAR
ECDisp_GetHidAppStatusString(
    NTSTATUS StatusCode
)
{
    static CHAR hidString[128];

    switch (StatusCode) 
    {
    case HIDP_STATUS_SUCCESS:
        return ("Success");

    case HIDP_STATUS_NULL:
        return ("Status NULL");

    case HIDP_STATUS_INVALID_PREPARSED_DATA:
        return ("Invalid Preparsed Data");

    case HIDP_STATUS_INVALID_REPORT_TYPE:
        return ("Invalid Report Type");

    case HIDP_STATUS_INVALID_REPORT_LENGTH:
        return ("Invalid Report Length");

    case HIDP_STATUS_USAGE_NOT_FOUND:
        return ("Usage not found");

    case HIDP_STATUS_VALUE_OUT_OF_RANGE:
        return ("Value out of range");

    case HIDP_STATUS_BAD_LOG_PHY_VALUES:
        return ("Bad logical physical values");

    case HIDP_STATUS_BUFFER_TOO_SMALL:
        return ("Buffer too small");

    case HIDP_STATUS_INTERNAL_ERROR:
        return ("Internal error");

    case HIDP_STATUS_I8242_TRANS_UNKNOWN:
        return ("I8242 Translation unknown");

    case HIDP_STATUS_INCOMPATIBLE_REPORT_ID:
        return ("Incompatible report ID");

    case HIDP_STATUS_NOT_VALUE_ARRAY:
        return ("Not value array");

    case HIDP_STATUS_IS_VALUE_ARRAY:
        return ("Is value array");

    case HIDP_STATUS_DATA_INDEX_NOT_FOUND:   
        return ("Data index not found");

    case HIDP_STATUS_DATA_INDEX_OUT_OF_RANGE:
        return ("Data index out of range");

    case HIDP_STATUS_BUTTON_NOT_PRESSED:     
        return ("Button not pressed");

    case HIDP_STATUS_REPORT_DOES_NOT_EXIST:
        return ("Report does not exist");

    case HIDP_STATUS_NOT_IMPLEMENTED:        
        return ("Not implemented");

    default:
        StringCbPrintf(hidString, sizeof(hidString),
                       "Unknown HID Status error: 0x%x", StatusCode);
        return (hidString);
    }
}             

BOOL
ECDisp_ConvertUlongListToValueList(
    IN  PULONG  UlongList,
    IN  ULONG   nUlongs,
    IN  USHORT  BitSize,
    IN  USHORT  ReportCount,
    _Outptr_result_bytebuffer_(*ValueListSize) PCHAR   *ValueList,
    OUT PULONG  ValueListSize
)
/*++
RoutineDescription:
    This routine takes a list of ULong values and formats a value list that is 
    used as input to HidP_SetUsageValueArray.  Unfortunately, this HidP function
    requires the caller to format the input buffer which means taking each of
    the values in Ulong, truncating their values to meet bit size and then set 
    those bits at the appropriate spot in the buffer.  That is the purpose of
    this function

    The function will return TRUE if everything succeeded, FALSE otherwise.
--*/
{

    ULONG       ulMask;
    PCHAR       List;
    USHORT      iByteIndex;
    USHORT      iByteOffset;
    ULONG       UlongIndex;
    ULONG       ListSize;
    USHORT      BitsToAdd;
    USHORT      nBits;
    ULONG       ulValue;
    UCHAR       LowByte;

    *ValueList = NULL;
    *ValueListSize = 0;
    
    //
    // Do some parameter validation...ReportCount should never be zero.
    //

    if (0 == ReportCount)
    {
        *ValueList = "ERR";
        SetLastError(ERROR_INVALID_PARAMETER);

        return (FALSE);
    }

    //
    // Check the number of ulongs passed in is actually less than or equal
    //  to the report count and if not, use only the first ReportCount
    //  number of ULongs.
    //

    if (nUlongs > ReportCount)
    {
        nUlongs = ReportCount;
    }

    /*
    // Allocate our buffer for the value list and return FALSE if it couldn't
    //   be done
    */

    ListSize = ROUND_TO_NEAREST_BYTE(BitSize * ReportCount);
    List = (PCHAR) malloc(ListSize);

    if (NULL == List) 
    {
        *ValueList = "ERR";
        SetLastError(ERROR_OUTOFMEMORY);
        return (FALSE);
    }

    /*
    // Initialize the buffer to all zeroes
    */

    memset(List, 0x00, ListSize);

    /*
    // Buffer has been allocated let's convert those values
    */

    /*
    // Determine the mask that will be used to retrieve the cared about bits
    //   of our value
    */

    ulMask = (sizeof(ULONG)*8 == BitSize) ? ULONG_MAX : (1 << BitSize)-1;

    /*
    // Initialize the iByteIndex and iByteOffset fields before entering the
    //    conversion loop.
    */

    iByteIndex = 0;
    iByteOffset = 0;

    /*
    // This is the main conversion loop.  It performs the following steps on
    //    each Ulong in the ulong list
    //      1) Sets BitsToAdd = BitSize
    //      2) Gets the ulValue and the masks off the upper bits that we don't
    //           care about.
    //      3) Determines how many bits can fit at the current byte index based
    //          on the current byte offset and the number of bits left to add
    //      4) Retrieve those bits, shift them to the correct position and 
    //            use bitwise or to get the correct values in the buffer
    //      5) Increment the byte index and set our new byte offset
    //      6) Shift our Ulong value right to get rid of least significant bits
    //           that have already been added
    //      7) Repeat through step 3 until no more bits to add
    */

    for (UlongIndex = 0; UlongIndex < nUlongs; UlongIndex++) 
    {    
        BitsToAdd = BitSize;

        ulValue = *(UlongList + UlongIndex) & ulMask;

        while (BitsToAdd > 0) 
        {
            nBits = (USHORT)min (8 - iByteOffset, BitsToAdd);
            
            LowByte = (UCHAR) (ulValue & 0xFF);
            
            LowByte = LowByte << iByteOffset;

            *(List+iByteIndex) |= LowByte;

            iByteIndex = (iByteOffset+nBits) >= 8 ? iByteIndex+1 : iByteIndex;
            iByteOffset = (iByteOffset + nBits) % 8;

            BitsToAdd -= nBits;

            ulValue = ulValue >> nBits;
        }
    }
        
    *ValueList = List;
    *ValueListSize = ListSize;

    if(NULL == *ValueList)
    {
        *ValueList = "ERR";
        return(FALSE);
    }
    
    return (TRUE);
}

PCHAR
ResolveFunctionName(
    INT Index
)
{
    PCHAR   FuncName;

    if (IS_HIDD_FUNCTION(Index) || IS_HID_FUNCTION(Index))
    {
        FuncName = DeviceCalls[Index-1].szFunctionName;
    }
    else
    {
        if(Index > HID_DEVCALLS)
        {
            FuncName = PpdCalls[Index-HID_DEVCALLS-1].szFunctionName;
        }
        else 
        {
            FuncName = 0;
        }
    }

    return (FuncName);
}

VOID
DisplayExtendedAttributes(
    IN  HWND                OutputWindow,
    IN  PHIDP_UNKNOWN_TOKEN UnknownList,
    IN  ULONG               UnknownListLength
)
{
    PHIDP_UNKNOWN_TOKEN current;
    ULONG               index;

    StringCbPrintf(szTempBuffer, TEMP_BUFFER_SIZE,
                   "Number of attributes: %d", UnknownListLength);
    OUTSTRING(OutputWindow, szTempBuffer);
    
    current = UnknownList;
    for (index = 0; index < UnknownListLength; index++) 
    {
       StringCbPrintf(szTempBuffer,
                      TEMP_BUFFER_SIZE,
                      "Token: %d  BitField: 0x%X", 
                      current -> Token,
                      current -> BitField);
            
        OUTSTRING(OutputWindow, szTempBuffer);
    }

    return;
}
