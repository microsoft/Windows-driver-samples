/*++
Copyright (c) Microsoft Corporation.  All rights reserved.

    THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY
    KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
    IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR
    PURPOSE.

Module Name:

    Capture.c

Abstract: 

    A sample that shows how to capture input parameters

Environment:

    User mode only

--*/

#include "regctrl.h"


VOID 
CaptureSample(
    )
/*++

Routine Description:

    This sample shows how to capture input parameters when the registery 
    operation comes from user mode. 

    The main part of this sample and a detailed explanation of why and how to
    capture user mode parameters can be found in ..\sys\capture.c. The user 
    mode part of this sample simply calls RegSetValueEx and DeleteValue 
    since the REG_XXX_INFORMATION structure for these two operations are 
    only partially captured. 
    
    See ..\sys\Capture.c for the callback routine used in this sample. 

Return Value:

    None

--*/
{
    LONG Res;
    HRESULT hr;
    DWORD ValueData = 0xDEADBEEF; 
    BOOL Result;
    BOOL Success = FALSE;
    DWORD BytesReturned;
    REGISTER_CALLBACK_INPUT RegisterCallbackInput = {0};
    REGISTER_CALLBACK_OUTPUT RegisterCallbackOutput = {0};
    UNREGISTER_CALLBACK_INPUT UnRegisterCallbackInput = {0};
    

    InfoPrint("");
    InfoPrint("=== Capture Sample ====");

    //
    // Register callback
    //

    RtlZeroMemory(RegisterCallbackInput.Altitude, 
                  MAX_ALTITUDE_BUFFER_LENGTH * sizeof(WCHAR));

    hr = StringCbPrintf(RegisterCallbackInput.Altitude, 
                          MAX_ALTITUDE_BUFFER_LENGTH * sizeof(WCHAR),
                          CALLBACK_ALTITUDE);

    if (!SUCCEEDED(hr)) {
        ErrorPrint("Copying altitude string failed. Error %d", hr);
        goto Exit;
    }

    RegisterCallbackInput.CallbackMode = CALLBACK_MODE_CAPTURE;

    Result = DeviceIoControl(g_Driver,
                             IOCTL_REGISTER_CALLBACK,
                             &RegisterCallbackInput,
                             sizeof(REGISTER_CALLBACK_INPUT),
                             &RegisterCallbackOutput,
                             sizeof(REGISTER_CALLBACK_OUTPUT),
                             &BytesReturned,
                             NULL);

    if (Result != TRUE) {    
        ErrorPrint("RegisterCallback failed. Error %d", GetLastError());
        goto Exit;
    }

    Success = TRUE;

    //
    // Create a value and delete it. Both should be successful.
    //

    Res = RegSetValueEx(g_RootKey,
                        VALUE_NAME,
                        0,
                        REG_DWORD,
                        (BYTE *) &ValueData,
                        sizeof(ValueData));
        
    if(Res != ERROR_SUCCESS) {
        ErrorPrint("RegSetValueEx return unexpected status %d", Res);
        Success = FALSE;
    }

    Res = RegDeleteValue(g_RootKey, VALUE_NAME);

    if (Res != ERROR_SUCCESS) {
        ErrorPrint("RegDeleteValue on original value returned unexpected status: %d", 
                   Res);
        Success = FALSE;
    }

    //
    // Unregister the callback
    //

    UnRegisterCallbackInput.Cookie = RegisterCallbackOutput.Cookie;

    Result = DeviceIoControl(g_Driver,
                             IOCTL_UNREGISTER_CALLBACK,
                             &UnRegisterCallbackInput,
                             sizeof(UNREGISTER_CALLBACK_INPUT),
                             NULL,
                             0,
                             &BytesReturned,
                             NULL);

    if (Result != TRUE) {    
        ErrorPrint("UnRegisterCallback failed. Error %d", GetLastError());
        Success = FALSE;
    }

  Exit: 

    if (Success) {
        InfoPrint("Capture Sample succeeded.");
    } else {
        ErrorPrint("Capture Sample failed.");
    }
    
}

