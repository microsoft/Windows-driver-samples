/*++

Copyright (c) Microsoft Corporation.  All rights reserved.

    THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY
    KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
    IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR
    PURPOSE.

Module Name:

    TESTAPP.C

Abstract:

    Console test app for osrusbfx2 driver.

Environment:

    user mode only

--*/

#include <driverspecs.h>
_Analysis_mode_(_Analysis_code_type_user_code_);

#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

#include "devioctl.h"
#include "strsafe.h"

#pragma warning(push)
#pragma warning(disable:4200)  //
#pragma warning(disable:4201)  // nameless struct/union
#pragma warning(disable:4214)  // bit field types other than int

#include <setupapi.h>
#include <basetyps.h>
#include "usbdi.h"
#include "public.h"

#pragma warning(pop)

#define WHILE(a) \
while(__pragma(warning(disable:4127)) a __pragma(warning(disable:4127)))

#define countof(x) (sizeof(x) / sizeof(x[0]))

#define MAX_DEVPATH_LENGTH 256
#define NUM_ASYNCH_IO   100
#define BUFFER_SIZE     1024
#define READER_TYPE   1
#define WRITER_TYPE   2

BOOL G_fDumpUsbConfig = FALSE;    // flags set in response to console command line switches
BOOL G_fDumpReadData = FALSE;
BOOL G_fRead = FALSE;
BOOL G_fWrite = FALSE;
BOOL G_fPlayWithDevice = FALSE;
BOOL G_fPerformAsyncIo = FALSE;
ULONG G_IterationCount = 1; //count of iterations of the test we are to perform
int G_WriteLen = 512;         // #bytes to write
int G_ReadLen = 512;          // #bytes to read
PCWSTR G_SendFileName = NULL;
ULONG G_SendFileInterval = 1;

BOOL
DumpUsbConfig( // defined in dump.c
    );

typedef enum _INPUT_FUNCTION {
    LIGHT_ONE_BAR = 1,
    CLEAR_ONE_BAR,
    LIGHT_ALL_BARS,
    CLEAR_ALL_BARS,
    GET_BAR_GRAPH_LIGHT_STATE,
    GET_SWITCH_STATE,
    GET_SWITCH_STATE_AS_INTERRUPT_MESSAGE,
    GET_7_SEGEMENT_STATE,
    SET_7_SEGEMENT_STATE,
    RESET_DEVICE,
    REENUMERATE_DEVICE,
} INPUT_FUNCTION;

_Success_ (return != FALSE)
BOOL
GetDevicePath(
    IN  LPGUID InterfaceGuid,
    _Out_writes_(BufLen) PWSTR DevicePath,
    _In_ size_t BufLen
    )
{
    HDEVINFO HardwareDeviceInfo;
    SP_DEVICE_INTERFACE_DATA DeviceInterfaceData;
    PSP_DEVICE_INTERFACE_DETAIL_DATA DeviceInterfaceDetailData = NULL;
    ULONG Length, RequiredLength = 0;
    BOOL bResult;
    HRESULT     hr;

    HardwareDeviceInfo = SetupDiGetClassDevs(
                             InterfaceGuid,
                             NULL,
                             NULL,
                             (DIGCF_PRESENT | DIGCF_DEVICEINTERFACE));

    if (HardwareDeviceInfo == INVALID_HANDLE_VALUE) {
        wprintf(L"SetupDiGetClassDevs failed!\n");
        return FALSE;
    }

    DeviceInterfaceData.cbSize = sizeof(SP_DEVICE_INTERFACE_DATA);

    bResult = SetupDiEnumDeviceInterfaces(HardwareDeviceInfo,
                                              0,
                                              InterfaceGuid,
                                              0,
                                              &DeviceInterfaceData);

    if (bResult == FALSE) {

        LPVOID lpMsgBuf;

        if (FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER |
                          FORMAT_MESSAGE_FROM_SYSTEM |
                          FORMAT_MESSAGE_IGNORE_INSERTS,
                          NULL,
                          GetLastError(),
                          MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
                          (LPWSTR) &lpMsgBuf,
                          0,
                          NULL
                          )) {

            printf("SetupDiEnumDeviceInterfaces failed: %s", (LPSTR)lpMsgBuf);
            LocalFree(lpMsgBuf);
        }

        SetupDiDestroyDeviceInfoList(HardwareDeviceInfo);
        return FALSE;
    }

    SetupDiGetDeviceInterfaceDetail(
        HardwareDeviceInfo,
        &DeviceInterfaceData,
        NULL,
        0,
        &RequiredLength,
        NULL
        );

    DeviceInterfaceDetailData = (PSP_DEVICE_INTERFACE_DETAIL_DATA)
                        LocalAlloc(LMEM_FIXED, RequiredLength);

    if (DeviceInterfaceDetailData == NULL) {
        SetupDiDestroyDeviceInfoList(HardwareDeviceInfo);
        wprintf(L"Failed to allocate memory.\n");
        return FALSE;
    }

    DeviceInterfaceDetailData->cbSize = sizeof(SP_DEVICE_INTERFACE_DETAIL_DATA);

    Length = RequiredLength;

    bResult = SetupDiGetDeviceInterfaceDetail(
                  HardwareDeviceInfo,
                  &DeviceInterfaceData,
                  DeviceInterfaceDetailData,
                  Length,
                  &RequiredLength,
                  NULL);

    if (bResult == FALSE) {

        LPVOID lpMsgBuf;

        if (FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER |
                          FORMAT_MESSAGE_FROM_SYSTEM |
                          FORMAT_MESSAGE_IGNORE_INSERTS,
                          NULL,
                          GetLastError(),
                          MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
                          (LPWSTR) &lpMsgBuf,
                          0,
                          NULL)) {

            printf("Error in SetupDiGetDeviceInterfaceDetail: %s\n", (LPSTR)lpMsgBuf);
            LocalFree(lpMsgBuf);
        }

        SetupDiDestroyDeviceInfoList(HardwareDeviceInfo);
        LocalFree(DeviceInterfaceDetailData);
        return FALSE;
    }

    hr = StringCchCopy(DevicePath,
                       BufLen,
                       DeviceInterfaceDetailData->DevicePath);
    if (FAILED(hr)) {
        SetupDiDestroyDeviceInfoList(HardwareDeviceInfo);
        LocalFree(DeviceInterfaceDetailData);
        return FALSE;
    }

    SetupDiDestroyDeviceInfoList(HardwareDeviceInfo);
    LocalFree(DeviceInterfaceDetailData);

    return TRUE;

}


HANDLE
OpenDevice(
    _In_ BOOL Synchronous
    )

/*++
Routine Description:

    Called by main() to open an instance of our device after obtaining its name

Arguments:

    Synchronous - TRUE, if Device is to be opened for synchronous access.
                  FALSE, otherwise.

Return Value:

    Device handle on success else INVALID_HANDLE_VALUE

--*/

{
    HANDLE hDev;
    WCHAR completeDeviceName[MAX_DEVPATH_LENGTH];

    if ( !GetDevicePath(
            (LPGUID) &GUID_DEVINTERFACE_OSRUSBFX2,
            completeDeviceName,
            countof(completeDeviceName)) )
    {
            return  INVALID_HANDLE_VALUE;
    }

    wprintf(L"DeviceName = (%s)\n", completeDeviceName);

    hDev = CreateFile(completeDeviceName,
                      GENERIC_WRITE | GENERIC_READ,
                      FILE_SHARE_WRITE | FILE_SHARE_READ,
                      NULL, // default security
                      OPEN_EXISTING,
                      ((Synchronous ? FILE_ATTRIBUTE_NORMAL : FILE_FLAG_OVERLAPPED) | SECURITY_IMPERSONATION),
                      NULL);

    if (hDev == INVALID_HANDLE_VALUE) {
        wprintf(L"Failed to open the device, error - %d", GetLastError());
    } else {
        wprintf(L"Opened the device successfully.\n");
    }

    return hDev;
}



VOID
Usage()

/*++
Routine Description:

    Called by main() to dump usage info to the console when
    the app is called with no parms or with an invalid parm

Arguments:

    None

Return Value:

    None

--*/

{
    wprintf(L"Usage for osrusbfx2 testapp:\n");
    wprintf(L"-r <n> where n is number of bytes to read\n");
    wprintf(L"-w <n> where n is number of bytes to write\n");
    wprintf(L"-c <n> where n is number of iterations (default = 1)\n");
    wprintf(L"-v <verbose> -- dumps read data\n");
    wprintf(L"-p to control bar LEDs, seven segment, and dip switch\n");
    wprintf(L"-a to perform asynchronous I/O\n");
    wprintf(L"-u to dump USB configuration and pipe info \n");
    wprintf(L"-f <filename> [interval-seconds] to send a text file to the seven-segment display (UMDF only)\n");

    return;
}


void
Parse(
    _In_ int argc,
    _In_reads_(argc) LPWSTR  *argv
    )

/*++
Routine Description:

    Called by main() to parse command line parms

Arguments:

    argc and argv that was passed to main()

Return Value:

    Sets global flags as per user function request

--*/

{
    int i;
    PWSTR endchar;

    if ( argc < 2 ) // give usage if invoked with no parms
        Usage();

    for (i=0; i<argc; i++) {
        if (argv[i][0] == L'-' ||
            argv[i][0] == L'/') {
            switch(argv[i][1]) {
            case L'r':
            case L'R':
                if (i+1 >= argc) {
                    Usage();
                    exit(1);
                }
                else {
                    G_ReadLen = wcstoul(&argv[i+1][0], &endchar, 10);
                                    G_fRead = TRUE;
                }
                i++;
                break;
            case L'w':
            case L'W':
                if (i+1 >= argc) {
                    Usage();
                    exit(1);
                }
                else {
                    G_WriteLen = wcstoul(&argv[i+1][0], &endchar, 10);
                                    G_fWrite = TRUE;
                }
                i++;
                break;
            case L'c':
            case L'C':
                if (i+1 >= argc) {
                    Usage();
                    exit(1);
                }
                else {
                    G_IterationCount = wcstoul(&argv[i+1][0], &endchar, 10);
                }
                i++;
                break;
            case L'f':
            case L'F':
                if (i+1 >= argc) {
                    Usage();
                    exit(1);
                }
                else {
                    G_SendFileName = argv[i+1];
                }

                i++;

                if (i+1 < argc) {
                    G_SendFileInterval = wcstoul(&argv[i+1][0], &endchar, 10);
                }
                i++;

                break;
            case L'u':
            case L'U':
                G_fDumpUsbConfig = TRUE;
                break;
            case L'p':
            case L'P':
                G_fPlayWithDevice = TRUE;
                break;
            case L'a':
            case L'A':
                G_fPerformAsyncIo = TRUE;
                break;
            case L'v':
            case L'V':
                G_fDumpReadData = TRUE;
                break;
            default:
                Usage();
            }
        }
    }
}

BOOL
Compare_Buffs(
    _In_reads_bytes_(length) PVOID *buff1,
    _In_reads_bytes_(length) PVOID *buff2,
    _In_ int   length
    )
/*++
Routine Description:

    Called to verify read and write buffers match for loopback test

Arguments:

    buffers to compare and length

Return Value:

    TRUE if buffers match, else FALSE

--*/
{
    int ok = 1;

    if (memcmp(buff1, buff2, length )) {
        // Edi, and Esi point to the mismatching char and ecx indicates the
        // remaining length.
        ok = 0;
    }

    return ok;
}

#define NPERLN 8

VOID
Dump(
   UCHAR *b,
   int len
)

/*++
Routine Description:

    Called to do formatted ascii dump to console of the io buffer

Arguments:

    buffer and length

Return Value:

    none

--*/

{
    ULONG i;
    ULONG longLen = (ULONG)len / sizeof( ULONG );
    PULONG pBuf = (PULONG) b;

    // dump an ordinal ULONG for each sizeof(ULONG)'th byte
    wprintf(L"\n****** BEGIN DUMP LEN decimal %d, 0x%x\n", len,len);
    for (i=0; i<longLen; i++) {
        wprintf(L"%04X ", *pBuf++);
        if (i % NPERLN == (NPERLN - 1)) {
            wprintf(L"\n");
        }
    }
    if (i % NPERLN != 0) {
        wprintf(L"\n");
    }
    wprintf(L"\n****** END DUMP LEN decimal %d, 0x%x\n", len,len);
}


BOOL
PlayWithDevice()
{
    HANDLE          deviceHandle;
    DWORD           code;
    ULONG           index;
    INPUT_FUNCTION  function;
    BAR_GRAPH_STATE barGraphState;
    ULONG           bar;
    SWITCH_STATE    switchState;
    UCHAR           sevenSegment = 0;
    UCHAR           i;
    BOOL            result = FALSE;

    deviceHandle = OpenDevice(FALSE);

    if (deviceHandle == INVALID_HANDLE_VALUE) {

        wprintf(L"Unable to find any OSR FX2 devices!\n");

        return FALSE;

    }

    //
    // Infinitely print out the list of choices, ask for input, process
    // the request
    //
    WHILE(TRUE)  {

        printf ("\nUSBFX TEST -- Functions:\n\n");
        printf ("\t1.  Light Bar\n");
        printf ("\t2.  Clear Bar\n");
        printf ("\t3.  Light entire Bar graph\n");
        printf ("\t4.  Clear entire Bar graph\n");
        printf ("\t5.  Get bar graph state\n");
        printf ("\t6.  Get Switch state\n");
        printf ("\t7.  Get Switch Interrupt Message\n");
        printf ("\t8.  Get 7 segment state\n");
        printf ("\t9.  Set 7 segment state\n");
        printf ("\t10. Reset the device\n");
        printf ("\t11. Reenumerate the device\n");
        printf ("\n\t0. Exit\n");
        printf ("\n\tSelection: ");

        if (scanf_s ("%d", &function) <= 0) {

            wprintf(L"Error reading input!\n");
            goto Error;

        }

        switch(function)  {

        case LIGHT_ONE_BAR:

        wprintf(L"Which Bar (input number 1 thru 8)?\n");
        if (scanf_s ("%d", &bar) <= 0) {

            wprintf(L"Error reading input!\n");
            goto Error;

        }

        if(bar == 0 || bar > 8){
            wprintf(L"Invalid bar number!\n");
            goto Error;
        }

        bar--; // normalize to 0 to 7

        barGraphState.BarsAsUChar = 1 << (UCHAR)bar;

        if (!DeviceIoControl(deviceHandle,
                             IOCTL_OSRUSBFX2_SET_BAR_GRAPH_DISPLAY,
                             &barGraphState,           // Ptr to InBuffer
                             sizeof(BAR_GRAPH_STATE),  // Length of InBuffer
                             NULL,         // Ptr to OutBuffer
                             0,            // Length of OutBuffer
                             &index,       // BytesReturned
                             0)) {         // Ptr to Overlapped structure

            code = GetLastError();

            wprintf(L"DeviceIoControl failed with error 0x%x\n", code);

            goto Error;
        }

        break;

        case CLEAR_ONE_BAR:


        wprintf(L"Which Bar (input number 1 thru 8)?\n");
        if (scanf_s ("%d", &bar) <= 0) {

            wprintf(L"Error reading input!\n");
            goto Error;

        }

        if(bar == 0 || bar > 8){
            wprintf(L"Invalid bar number!\n");
            goto Error;
        }

        bar--;

        //
        // Read the current state
        //
        if (!DeviceIoControl(deviceHandle,
                             IOCTL_OSRUSBFX2_GET_BAR_GRAPH_DISPLAY,
                             NULL,             // Ptr to InBuffer
                             0,            // Length of InBuffer
                             &barGraphState,           // Ptr to OutBuffer
                             sizeof(BAR_GRAPH_STATE),  // Length of OutBuffer
                             &index,        // BytesReturned
                             0)) {         // Ptr to Overlapped structure

            code = GetLastError();

            wprintf(L"DeviceIoControl failed with error 0x%x\n", code);

            goto Error;
        }

        if (barGraphState.BarsAsUChar & (1 << bar)) {

            wprintf(L"Bar is set...Clearing it\n");
            barGraphState.BarsAsUChar &= ~(1 << bar);

            if (!DeviceIoControl(deviceHandle,
                                 IOCTL_OSRUSBFX2_SET_BAR_GRAPH_DISPLAY,
                                 &barGraphState,         // Ptr to InBuffer
                                 sizeof(BAR_GRAPH_STATE), // Length of InBuffer
                                 NULL,             // Ptr to OutBuffer
                                 0,            // Length of OutBuffer
                                 &index,        // BytesReturned
                                 0)) {          // Ptr to Overlapped structure

                code = GetLastError();

                wprintf(L"DeviceIoControl failed with error 0x%x\n", code);

                goto Error;

            }

        } else {

            wprintf(L"Bar not set.\n");

        }

        break;

        case LIGHT_ALL_BARS:

        barGraphState.BarsAsUChar = 0xFF;

        if (!DeviceIoControl(deviceHandle,
                             IOCTL_OSRUSBFX2_SET_BAR_GRAPH_DISPLAY,
                             &barGraphState,         // Ptr to InBuffer
                             sizeof(BAR_GRAPH_STATE),   // Length of InBuffer
                             NULL,         // Ptr to OutBuffer
                             0,            // Length of OutBuffer
                             &index,       // BytesReturned
                             0)) {          // Ptr to Overlapped structure

            code = GetLastError();

            wprintf(L"DeviceIoControl failed with error 0x%x\n", code);

            goto Error;
        }

        break;

        case CLEAR_ALL_BARS:

        barGraphState.BarsAsUChar = 0;

        if (!DeviceIoControl(deviceHandle,
                             IOCTL_OSRUSBFX2_SET_BAR_GRAPH_DISPLAY,
                             &barGraphState,                 // Ptr to InBuffer
                             sizeof(BAR_GRAPH_STATE),         // Length of InBuffer
                             NULL,             // Ptr to OutBuffer
                             0,            // Length of OutBuffer
                             &index,         // BytesReturned
                             0)) {        // Ptr to Overlapped structure

            code = GetLastError();

            wprintf(L"DeviceIoControl failed with error 0x%x\n", code);

            goto Error;
        }

        break;


        case GET_BAR_GRAPH_LIGHT_STATE:

        barGraphState.BarsAsUChar = 0;

        if (!DeviceIoControl(deviceHandle,
                             IOCTL_OSRUSBFX2_GET_BAR_GRAPH_DISPLAY,
                             NULL,             // Ptr to InBuffer
                             0,            // Length of InBuffer
                             &barGraphState,          // Ptr to OutBuffer
                             sizeof(BAR_GRAPH_STATE), // Length of OutBuffer
                             &index,                  // BytesReturned
                             0)) {                   // Ptr to Overlapped structure

            code = GetLastError();

            wprintf(L"DeviceIoControl failed with error 0x%x\n", code);

            goto Error;
        }

        wprintf(L"Bar Graph: \n");
        wprintf(L"    Bar8 is %s\n", barGraphState.Bar8 ? L"ON" : L"OFF");
        wprintf(L"    Bar7 is %s\n", barGraphState.Bar7 ? L"ON" : L"OFF");
        wprintf(L"    Bar6 is %s\n", barGraphState.Bar6 ? L"ON" : L"OFF");
        wprintf(L"    Bar5 is %s\n", barGraphState.Bar5 ? L"ON" : L"OFF");
        wprintf(L"    Bar4 is %s\n", barGraphState.Bar4 ? L"ON" : L"OFF");
        wprintf(L"    Bar3 is %s\n", barGraphState.Bar3 ? L"ON" : L"OFF");
        wprintf(L"    Bar2 is %s\n", barGraphState.Bar2 ? L"ON" : L"OFF");
        wprintf(L"    Bar1 is %s\n", barGraphState.Bar1 ? L"ON" : L"OFF");

        break;

        case GET_SWITCH_STATE:

        switchState.SwitchesAsUChar = 0;

        if (!DeviceIoControl(deviceHandle,
                             IOCTL_OSRUSBFX2_READ_SWITCHES,
                             NULL,             // Ptr to InBuffer
                             0,            // Length of InBuffer
                             &switchState,          // Ptr to OutBuffer
                             sizeof(SWITCH_STATE),  // Length of OutBuffer
                             &index,                // BytesReturned
                             0)) {                  // Ptr to Overlapped structure

            code = GetLastError();

            wprintf(L"DeviceIoControl failed with error 0x%x\n", code);

            goto Error;
        }

        wprintf(L"Switches: \n");
        wprintf(L"    Switch8 is %s\n", switchState.Switch8 ? L"ON" : L"OFF");
        wprintf(L"    Switch7 is %s\n", switchState.Switch7 ? L"ON" : L"OFF");
        wprintf(L"    Switch6 is %s\n", switchState.Switch6 ? L"ON" : L"OFF");
        wprintf(L"    Switch5 is %s\n", switchState.Switch5 ? L"ON" : L"OFF");
        wprintf(L"    Switch4 is %s\n", switchState.Switch4 ? L"ON" : L"OFF");
        wprintf(L"    Switch3 is %s\n", switchState.Switch3 ? L"ON" : L"OFF");
        wprintf(L"    Switch2 is %s\n", switchState.Switch2 ? L"ON" : L"OFF");
        wprintf(L"    Switch1 is %s\n", switchState.Switch1 ? L"ON" : L"OFF");

        break;

        case GET_SWITCH_STATE_AS_INTERRUPT_MESSAGE:

        switchState.SwitchesAsUChar = 0;

        if (!DeviceIoControl(deviceHandle,
                             IOCTL_OSRUSBFX2_GET_INTERRUPT_MESSAGE,
                             NULL,             // Ptr to InBuffer
                             0,            // Length of InBuffer
                             &switchState,           // Ptr to OutBuffer
                             sizeof(switchState),    // Length of OutBuffer
                             &index,                // BytesReturned
                             0)) {                  // Ptr to Overlapped structure

            code = GetLastError();

            wprintf(L"DeviceIoControl failed with error 0x%x\n", code);

            goto Error;
        }

        wprintf(L"Switches: %d\n",index);
        wprintf(L"    Switch8 is %s\n", switchState.Switch8 ? L"ON" : L"OFF");
        wprintf(L"    Switch7 is %s\n", switchState.Switch7 ? L"ON" : L"OFF");
        wprintf(L"    Switch6 is %s\n", switchState.Switch6 ? L"ON" : L"OFF");
        wprintf(L"    Switch5 is %s\n", switchState.Switch5 ? L"ON" : L"OFF");
        wprintf(L"    Switch4 is %s\n", switchState.Switch4 ? L"ON" : L"OFF");
        wprintf(L"    Switch3 is %s\n", switchState.Switch3 ? L"ON" : L"OFF");
        wprintf(L"    Switch2 is %s\n", switchState.Switch2 ? L"ON" : L"OFF");
        wprintf(L"    Switch1 is %s\n", switchState.Switch1 ? L"ON" : L"OFF");

        break;

        case GET_7_SEGEMENT_STATE:

        sevenSegment = 0;

        if (!DeviceIoControl(deviceHandle,
                             IOCTL_OSRUSBFX2_GET_7_SEGMENT_DISPLAY,
                             NULL,             // Ptr to InBuffer
                             0,            // Length of InBuffer
                             &sevenSegment,                 // Ptr to OutBuffer
                             sizeof(UCHAR),         // Length of OutBuffer
                             &index,                     // BytesReturned
                             0)) {                       // Ptr to Overlapped structure

            code = GetLastError();

            wprintf(L"DeviceIoControl failed with error 0x%x\n", code);

            goto Error;
        }

        wprintf(L"7 Segment mask:  0x%x\n", sevenSegment);
        break;

        case SET_7_SEGEMENT_STATE:

        for (i = 0; i < 8; i++) {

            sevenSegment = 1 << i;

            if (!DeviceIoControl(deviceHandle,
                                 IOCTL_OSRUSBFX2_SET_7_SEGMENT_DISPLAY,
                                 &sevenSegment,   // Ptr to InBuffer
                                 sizeof(UCHAR),  // Length of InBuffer
                                 NULL,           // Ptr to OutBuffer
                                 0,         // Length of OutBuffer
                                 &index,         // BytesReturned
                                 0)) {           // Ptr to Overlapped structure

                code = GetLastError();

                wprintf(L"DeviceIoControl failed with error 0x%x\n", code);

                goto Error;
            }

            wprintf(L"This is %d\n", i);
            Sleep(500);

        }

        wprintf(L"7 Segment mask:  0x%x\n", sevenSegment);
        break;

        case RESET_DEVICE:

        wprintf(L"Reset the device\n");

        if (!DeviceIoControl(deviceHandle,
                             IOCTL_OSRUSBFX2_RESET_DEVICE,
                             NULL,             // Ptr to InBuffer
                             0,            // Length of InBuffer
                             NULL,                 // Ptr to OutBuffer
                             0,         // Length of OutBuffer
                             &index,   // BytesReturned
                             NULL)) {        // Ptr to Overlapped structure

            code = GetLastError();

            wprintf(L"DeviceIoControl failed with error 0x%x\n", code);

            goto Error;
        }

        break;

        case REENUMERATE_DEVICE:

        wprintf(L"Re-enumerate the device\n");

        if (!DeviceIoControl(deviceHandle,
                             IOCTL_OSRUSBFX2_REENUMERATE_DEVICE,
                             NULL,             // Ptr to InBuffer
                             0,            // Length of InBuffer
                             NULL,                 // Ptr to OutBuffer
                             0,         // Length of OutBuffer
                             &index,   // BytesReturned
                             NULL)) {        // Ptr to Overlapped structure

            code = GetLastError();

            wprintf(L"DeviceIoControl failed with error 0x%x\n", code);

            goto Error;
        }

        //
        // Close the handle to the device and exit out so that
        // the driver can unload when the device is surprise-removed
        // and reenumerated.
        //
        default:

        result = TRUE;
        goto Error;

        }

    }   // end of while loop

Error:

    CloseHandle(deviceHandle);
    return result;

}

BOOL
SendFileToDevice(
    _In_ PCWSTR FileName
    )
{
    HANDLE          deviceHandle;

    struct 
    {
        USHORT delay;
        WCHAR buffer[MAX_PATH + 1];
    } playback;

    ULONG bufferCch;

    DWORD           code;
    BOOL            result = FALSE;

    //
    // Open a handle to the device.
    //

    deviceHandle = OpenDevice(FALSE);

    if (deviceHandle == INVALID_HANDLE_VALUE) {

        wprintf(L"Unable to find any OSR FX2 devices!\n");

        return FALSE;

    }

    //
    // Convert the file name from relative to absolute.
    //

    bufferCch = GetFullPathName(FileName, 
                                countof(playback.buffer),
                                playback.buffer,
                                NULL);

    if (bufferCch == 0)
    {
        wprintf(L"Error getting full path name for %s - %d\n", 
                FileName,
                GetLastError());
        goto Error;
    }

    if ((G_SendFileInterval * 1000) >= ((USHORT) 0xffff))
    {
        wprintf(L"Error - delay is too large.  Remember that it's in terms of seconds.\n");
        goto Error;
    }

    playback.delay = (USHORT) (G_SendFileInterval * 1000);

    if (!DeviceIoControl(deviceHandle,
                         IOCTL_OSRUSBFX2_PLAY_FILE,
                         &playback,
                         sizeof(playback),
                         NULL,
                         0,
                         &bufferCch,
                         0)) {

        code = GetLastError();

        wprintf(L"DeviceIoControl failed with error 0x%x\n", code);

        goto Error;
    }

    result = TRUE;

Error:

    CloseHandle(deviceHandle);
    return result;
}

ULONG
AsyncIo(
    PVOID  ThreadParameter
    )
{
    HANDLE hDevice = INVALID_HANDLE_VALUE;
    HANDLE hCompletionPort = NULL;
    OVERLAPPED *pOvList = NULL;
    PUCHAR      buf = NULL;
    ULONG_PTR    i;
    ULONG   ioType = (ULONG)(ULONG_PTR)ThreadParameter;
    ULONG   error;

    hDevice = OpenDevice(FALSE);

    if (hDevice == INVALID_HANDLE_VALUE) {
        wprintf(L"Cannot open device %d\n", GetLastError());
        goto Error;
    }

    hCompletionPort = CreateIoCompletionPort(hDevice, NULL, 1, 0);

    if (hCompletionPort == NULL) {
        wprintf(L"Cannot open completion port %d \n",GetLastError());
        goto Error;
    }

    pOvList = (OVERLAPPED *)malloc(NUM_ASYNCH_IO * sizeof(OVERLAPPED));

    if (pOvList == NULL) {
        wprintf(L"Cannot allocate overlapped array \n");
        goto Error;
    }

    buf = (PUCHAR)malloc(NUM_ASYNCH_IO * BUFFER_SIZE);

    if (buf == NULL) {
        wprintf(L"Cannot allocate buffer \n");
        goto Error;
    }

    ZeroMemory(pOvList, NUM_ASYNCH_IO * sizeof(OVERLAPPED));
    ZeroMemory(buf, NUM_ASYNCH_IO * BUFFER_SIZE);

    //
    // Issue asynch I/O
    //

    for (i = 0; i < NUM_ASYNCH_IO; i++) {
        if (ioType == READER_TYPE) {
            if ( ReadFile( hDevice,
                      buf + (i* BUFFER_SIZE),
                      BUFFER_SIZE,
                      NULL,
                      &pOvList[i]) == 0) {

                error = GetLastError();
                if (error != ERROR_IO_PENDING) {
                    wprintf(L" %d th read failed %d \n",i, GetLastError());
                    goto Error;
                }
            }

        } else {
            if ( WriteFile( hDevice,
                      buf + (i* BUFFER_SIZE),
                      BUFFER_SIZE,
                      NULL,
                      &pOvList[i]) == 0) {
                error = GetLastError();
                if (error != ERROR_IO_PENDING) {
                    wprintf(L" %d th write failed %d \n",i, GetLastError());
                    goto Error;
                }
            }
        }
    }

    //
    // Wait for the I/Os to complete. If one completes then reissue the I/O
    //

    WHILE (1) {
        OVERLAPPED *completedOv;
        ULONG_PTR   key;
        ULONG     numberOfBytesTransferred;

        if ( GetQueuedCompletionStatus(hCompletionPort, &numberOfBytesTransferred,
                            &key, &completedOv, INFINITE) == 0) {
            wprintf(L"GetQueuedCompletionStatus failed %d\n", GetLastError());
            goto Error;
        }

        //
        // Read successfully completed. Issue another one.
        //

        if (ioType == READER_TYPE) {

            i = completedOv - pOvList;

            wprintf(L"Number of bytes read by request number %d is %d\n",
                                i, numberOfBytesTransferred);

            if ( ReadFile( hDevice,
                      buf + (i * BUFFER_SIZE),
                      BUFFER_SIZE,
                      NULL,
                      completedOv) == 0) {
                error = GetLastError();
                if (error != ERROR_IO_PENDING) {
                    wprintf(L"%d th Read failed %d \n", i, GetLastError());
                    goto Error;
                }
            }
        } else {

            i = completedOv - pOvList;

            wprintf(L"Number of bytes written by request number %d is %d\n",
                            i, numberOfBytesTransferred);

            if ( WriteFile( hDevice,
                      buf + (i * BUFFER_SIZE),
                      BUFFER_SIZE,
                      NULL,
                      completedOv) == 0) {
                error = GetLastError();
                if (error != ERROR_IO_PENDING) {
                    wprintf(L"%d th write failed %d \n", i, GetLastError());
                    goto Error;
                }
            }
        }
    }

Error:
    if (hDevice != INVALID_HANDLE_VALUE) {
        CloseHandle(hDevice);
    }

    if (hCompletionPort) {
        CloseHandle(hCompletionPort);
    }

    if (pOvList) {
        free(pOvList);
    }
    if (buf) {
        free(buf);
    }

    return 1;

}


int
_cdecl
wmain(
    _In_ int argc,
    _In_reads_(argc) LPWSTR  *argv
    )
/*++
Routine Description:

    Entry point to rwbulk.exe
    Parses cmdline, performs user-requested tests

Arguments:

    argc, argv  standard console  'c' app arguments

Return Value:

    Zero

--*/

{
    PWSTR * pinBuf = NULL;
    PWSTR * poutBuf = NULL;
    int    nBytesRead;
    int    nBytesWrite;
    int    ok;
    int    retValue = 0;
    UINT   success;
    HANDLE hRead = INVALID_HANDLE_VALUE;
    HANDLE hWrite = INVALID_HANDLE_VALUE;
    ULONG  fail = 0L;
    ULONG  i;


    Parse(argc, argv );

    //
    // dump USB configuation and pipe info
    //
    if (G_fDumpUsbConfig) {
        DumpUsbConfig();
    }

    if (G_fPlayWithDevice) {
        PlayWithDevice();
        goto exit;
    }

    if (G_SendFileName != NULL)
    {
        SendFileToDevice(G_SendFileName);
        goto exit;
    }

    if (G_fPerformAsyncIo) {
        HANDLE  th1;

        //
        // Create a reader thread
        //
        th1 = CreateThread( NULL,          // Default Security Attrib.
                            0,             // Initial Stack Size,
                            AsyncIo,       // Thread Func
                            (LPVOID)READER_TYPE,
                            0,             // Creation Flags
                            NULL );        // Don't need the Thread Id.

        if (th1 == NULL) {
            wprintf(L"Couldn't create reader thread - error %d\n", GetLastError());
            retValue = 1;
            goto exit;
        }

        //
        // Use this thread for peforming write.
        //
        AsyncIo((PVOID)WRITER_TYPE);

        goto exit;
    }

    //
    // doing a read, write, or both test
    //
    if ((G_fRead) || (G_fWrite)) {

        if (G_fRead) {
            if ( G_fDumpReadData ) { // round size to sizeof ULONG for readable dumping
                while( G_ReadLen % sizeof( ULONG ) ) {
                    G_ReadLen++;
                }
            }

            //
            // open the output file
            //
            hRead = OpenDevice(TRUE);
            if(hRead == INVALID_HANDLE_VALUE) {
                retValue = 1;
                goto exit;
            }

            pinBuf = malloc(G_ReadLen);
        }

        if (G_fWrite) {
            if ( G_fDumpReadData ) { // round size to sizeof ULONG for readable dumping
                while( G_WriteLen % sizeof( ULONG ) ) {
                    G_WriteLen++;
                }
            }

            //
            // open the output file
            //
            hWrite = OpenDevice(TRUE);
            if(hWrite == INVALID_HANDLE_VALUE) {
               retValue = 1;
               goto exit;
            }

            poutBuf = malloc(G_WriteLen);
        }

        for (i = 0; i < G_IterationCount; i++) {
            ULONG  j;

            if (G_fWrite && poutBuf && hWrite != INVALID_HANDLE_VALUE) {

                PULONG pOut = (PULONG) poutBuf;
                ULONG  numLongs = G_WriteLen / sizeof( ULONG );

                //
                // put some data in the output buffer
                //
                for (j=0; j<numLongs; j++) {
                    *(pOut+j) = j;
                }

                //
                // send the write
                //
                success = WriteFile(hWrite, poutBuf, G_WriteLen,  (PULONG) &nBytesWrite, NULL);
                if(success == 0) {
                    wprintf(L"WriteFile failed - error %d\n", GetLastError());
                    retValue = 1;
                    goto exit;
                }
                wprintf(L"Write (%04.4d) : request %06.6d bytes -- %06.6d bytes written\n",
                        i, G_WriteLen, nBytesWrite);

                assert(nBytesWrite == G_WriteLen);
            }

            if (G_fRead && pinBuf) {

                success = ReadFile(hRead, pinBuf, G_ReadLen, (PULONG) &nBytesRead, NULL);
                if(success == 0) {
                    wprintf(L"ReadFile failed - error %d\n", GetLastError());
                    retValue = 1;
                    goto exit;
                }

                wprintf(L"Read (%04.4d) : request %06.6d bytes -- %06.6d bytes read\n",
                       i, G_ReadLen, nBytesRead);

                if (G_fWrite) {

                    //
                    // validate the input buffer against what
                    // we sent to the 82930 (loopback test)
                    //

                    ok = Compare_Buffs(pinBuf, poutBuf,  nBytesRead);

                    if( G_fDumpReadData ) {
                        wprintf(L"Dumping read buffer\n");
                        Dump( (PUCHAR) pinBuf,  nBytesRead );
                        wprintf(L"Dumping write buffer\n");
                        Dump( (PUCHAR) poutBuf, nBytesRead );
                    }
                    assert(ok);

                    if(ok != 1) {
                        fail++;
                    }

                    assert(G_ReadLen == G_WriteLen);
                    assert(nBytesRead == G_ReadLen);
                }
            }
        }

    }

exit:

    if (pinBuf) {
        free(pinBuf);
    }

    if (poutBuf) {
        free(poutBuf);
    }

    // close devices if needed
    if (hRead != INVALID_HANDLE_VALUE) {
        CloseHandle(hRead);
    }

    if (hWrite != INVALID_HANDLE_VALUE) {
        CloseHandle(hWrite);
    }

    return retValue;
}




