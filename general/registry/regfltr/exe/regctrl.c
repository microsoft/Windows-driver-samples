/*++
Copyright (c) Microsoft Corporation.  All rights reserved.

    THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY
    KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
    IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR
    PURPOSE.

Module Name:

    regctrl.c

Abstract: 

    Invokes the usermode and kernel mode callback samples.

Environment:

    User mode Win32 console application

Revision History:

--*/

#include "regctrl.h"

//
// Global variables
//

//
// Handle to the driver
//
HANDLE g_Driver;

//
// Handle to the root test key
//
HKEY g_RootKey;

//
// Version number for the registry callback
//
ULONG g_MajorVersion;
ULONG g_MinorVersion;



BOOL
GetCallbackVersion();

VOID
DoKernelModeSamples();

VOID
DoUserModeSamples();

LPCWSTR 
GetKernelModeSampleName (
    _In_ KERNELMODE_SAMPLE Sample
    );


VOID __cdecl
wmain(
    _In_ ULONG argc,
    _In_reads_(argc) LPCWSTR argv[]
    )
{

    BOOL Result;

    UNREFERENCED_PARAMETER(argc);
    UNREFERENCED_PARAMETER(argv);

    Result = UtilLoadDriver(DRIVER_NAME,
                             DRIVER_NAME_WITH_EXT,
                             WIN32_DEVICE_NAME,
                             &g_Driver);

    if (Result != TRUE) {
        ErrorPrint("UtilLoadDriver failed, exiting...");
        exit(1);
    }

    printf("\n");
    printf("Starting Callback samples...\n");
    printf("\n");
    printf("To get more detailed output from the sample, do either one of these steps:\n");
    printf("\n");
    printf("\tA. In kernel debugger: \n");
    printf("\tkd> ed nt!Kd_IHVDRIVER_Mask 0x8\n\n");
    printf("\tB. Run this script and reboot:\n");
    printf("\treg add \"HKLM\\SYSTEM\\CurrentControlSet\\Control\\Session Manager\\Debug Print Filter\" /v IHVDRIVER /t REG_DWORD /d 0x8\n\n");

    //
    // Get the registry callback version to determine what samples can 
    // run on the system.
    //

    if (GetCallbackVersion()) {
        InfoPrint("Callback version is %u.%u", g_MajorVersion, g_MinorVersion);
    }
    
    DoKernelModeSamples();
    DoUserModeSamples();

    UtilUnloadDriver(g_Driver, NULL, DRIVER_NAME);

}


BOOL
GetCallbackVersion(
    ) 
/*++

Routine Description:

    This routine asks the driver for the registry callback version and
    stores it in the global variables g_MajorVersion and g_MinorVersion.

--*/
{

    DWORD BytesReturned = 0;
    BOOL Result;
    GET_CALLBACK_VERSION_OUTPUT Output = {0};
    
    Result = DeviceIoControl(g_Driver,
                              IOCTL_GET_CALLBACK_VERSION,
                              NULL,
                              0,
                              &Output,
                              sizeof(GET_CALLBACK_VERSION_OUTPUT),
                              &BytesReturned,
                              NULL);

    if (Result != TRUE) {
        ErrorPrint("DeviceIoControl for GET_CALLBACK_VERSION failed, error %d\n", GetLastError());
        return FALSE;
    }

    g_MajorVersion = Output.MajorVersion;
    g_MinorVersion = Output.MinorVersion;

    return TRUE;

}

VOID
DoUserModeSamples(
    ) 
/*++

Routine Description:

    Creates the callback root test key and calls the usermode samples.

--*/
{

    LONG Res;

    Res = RegCreateKeyEx(HKEY_LOCAL_MACHINE,
                         ROOT_KEY_REL_PATH,
                         0,
                         NULL,
                         0,
                         KEY_ALL_ACCESS,
                         NULL,
                         &g_RootKey,
                         NULL);

    if (Res != ERROR_SUCCESS) {
        ErrorPrint("Creating root key failed. Error %d", Res);
        goto Exit;
    }

    PreNotificationBlockSample();
    PreNotificationBypassSample();
    PostNotificationOverrideSuccessSample();
    PostNotificationOverrideErrorSample();
    CaptureSample();

  Exit:
    
    if (g_RootKey != NULL) {
        RegCloseKey(g_RootKey);
    }
    RegDeleteKey(HKEY_LOCAL_MACHINE, ROOT_KEY_REL_PATH);
    
}


VOID
DoKernelModeSamples(
    ) 
/*++

Routine Description:

    Tells the driver to run the kernel mode samples and prints out the
    results.

--*/
{

    UINT Index;
    DWORD BytesReturned = 0;
    BOOL Result;
    DO_KERNELMODE_SAMPLES_OUTPUT Output = {0};
    
    Result = DeviceIoControl (g_Driver,
                              IOCTL_DO_KERNELMODE_SAMPLES,
                              NULL,
                              0,
                              &Output,
                              sizeof(DO_KERNELMODE_SAMPLES_OUTPUT),
                              &BytesReturned,
                              NULL);

    if (Result != TRUE) {
        ErrorPrint("DeviceIoControl for DO_KERNELMODE_SAMPLES failed, error %d\n", GetLastError());
        return;
    }

    InfoPrint("");
    InfoPrint("=== Results of KernelMode Samples ===");

    for (Index = 0; Index < MAX_KERNELMODE_SAMPLES; Index++) {
        InfoPrint("\t%S: %s",
                  GetKernelModeSampleName(Index),
                  Output.SampleResults[Index]? "Succeeded" : "FAILED");
    }

}



LPCWSTR 
GetKernelModeSampleName (
    _In_ KERNELMODE_SAMPLE Sample
    )
/*++

Routine Description:

    Converts from a KERNELMODE_SAMPLE value to a string

Arguments:

    Sample - value that identifies a kernel mode sample

Return Value:

    Returns a string of the name of Sample.
    
--*/
{
    switch (Sample) {
        case KERNELMODE_SAMPLE_PRE_NOTIFICATION_BLOCK:
            return L"Pre-Notification Block Sample";
        case KERNELMODE_SAMPLE_PRE_NOTIFICATION_BYPASS:
            return L"Pre-Notification Bypass Sample";
        case KERNELMODE_SAMPLE_POST_NOTIFICATION_OVERRIDE_SUCCESS:
            return L"Post-Notification Override Success Sample";
        case KERNELMODE_SAMPLE_POST_NOTIFICATION_OVERRIDE_ERROR:
            return L"Post-Notification Override Error Sample";
        case KERNELMODE_SAMPLE_TRANSACTION_REPLAY:
            return L"Transaction Replay Sample";
        case KERNELMODE_SAMPLE_TRANSACTION_ENLIST:
            return L"Transaction Enlist Sample";
        case KERNELMODE_SAMPLE_MULTIPLE_ALTITUDE_BLOCK_DURING_PRE:
            return L"Multiple Altitude Block During Pre Sample";
        case KERNELMODE_SAMPLE_MULTIPLE_ALTITUDE_INTERNAL_INVOCATION:
            return L"Multiple Altitude Internal Invocation Sample";
        case KERNELMODE_SAMPLE_SET_CALL_CONTEXT:
            return L"Set Call Context Sample";
        case KERNELMODE_SAMPLE_SET_OBJECT_CONTEXT:
            return L"Set Object Context Sample";
        case KERNELMODE_SAMPLE_VERSION_CREATE_OPEN_V1:
            return L"Create Open V1 Sample";
        default:
            return L"Unsupported Kernel Mode Sample";
    }
}

