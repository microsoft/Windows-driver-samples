/*++


--*/
#include <DriverSpecs.h>
    _Analysis_mode_(_Analysis_code_type_user_code_)
    
#define INITGUID
    
#include <windows.h>
#include <strsafe.h>
#include <cfgmgr32.h>
#include <stdio.h>
#include <stdlib.h>
#include <devioctl.h>
#include <assert.h>
#include "AppInterface.h"

#define COUNT_OF(x) sizeof(x)/sizeof(x[0])

#define MAX_DEVPATH_LENGTH                       256

#define COMPONENT            L"Component"
#define MAX_OUTSTANDING_IO   L"MaxOutStandingIO"
#define DELAY                L"Delay"
#define CANCEL               L"Cancel"

typedef struct _CONFIGURATION {
    PWSTR Option;
    ULONG Value;
} CONFIGURATION, *PCONFIGURATION;

#define RANDOM_COMPONENT    (DWORD)-1
#define UNUSED              COMPONENT_COUNT
#define REQUEST_TIMEOUT     10000
#define MAX_DEVPATH_LENGTH  256

PCONFIGURATION
LookupSwitch(
    _In_ PWSTR Param);

DWORD
ProcessUserInput(
    _In_ int argc,
    _In_reads_(argc) PWSTR argv[]
    );

ULONG
GetSetting(
    _In_ PWSTR Switch);

DWORD
ProcessSwitch(
    _In_ PWSTR Param,
    _In_ PWSTR Value);

void PrintUsage(
    _In_ PWSTR argv[]
    );

BOOL
GetDevicePath(
    IN  LPGUID InterfaceGuid,
    _Out_writes_(BufLen) PWCHAR DevicePath,
    _In_ size_t BufLen
    );


DWORD SendIO(
    _In_ HANDLE DeviceHandle,
    _In_ HANDLE CompletionPortHandle,
    _In_ ULONG Component,
    _In_ ULONG MaxOutstandingIo,
    _In_ ULONG Delay,
    _In_ BOOLEAN Cancel
    );

DWORD
WaitForIoCompletion(
    _In_ HANDLE CompletionPortHandle,
    _In_opt_ LPOVERLAPPED* POvPtr,
    _Out_opt_ PDWORD CompletionStatus);

BOOLEAN
VerifyRequest(
    _In_ PPOWERFX_READ_COMPONENT_INPUT input,
    _In_ PPOWERFX_READ_COMPONENT_OUTPUT output);

DWORD
SendRequest(
    _In_ HANDLE DeviceHandle,
    _In_ LPOVERLAPPED OverlappedPtr,
    _In_ PPOWERFX_READ_COMPONENT_INPUT inputBuffer,
    _In_ PPOWERFX_READ_COMPONENT_OUTPUT outputBuffer);

