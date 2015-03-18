/*++
Copyright (c) Microsoft Corporation.  All rights reserved.

    THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY
    KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
    IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR
    PURPOSE.

Module Name:

    Pre.c

Abstract: 

    Samples that show what callbacks can do during the pre-notification
    phase.

Environment:

    User mode only

--*/



#include "regctrl.h"


VOID 
PreNotificationBlockSample(
    )
/*++

Routine Description:

    This sample shows how to block a registry operation in the
    pre-notification phase. 

    Two keys are created. The create operations should succeed, but one
    is intercepted by the callback and failed with ERROR_ACCESS_DENIED.
    The same is done for two values.

    See ..\sys\Pre.c for the callback routine used in this sample. 

--*/
{

    LONG Res;
    HRESULT hr;
    HKEY Key = NULL;
    HKEY NotModifiedKey = NULL;
    DWORD ValueData = 0xDEADBEEF; 
    DWORD BytesReturned;
    BOOL Result;
    BOOL Success = FALSE;
    REGISTER_CALLBACK_INPUT RegisterCallbackInput = {0};
    REGISTER_CALLBACK_OUTPUT RegisterCallbackOutput = {0};
    UNREGISTER_CALLBACK_INPUT UnRegisterCallbackInput = {0};

    InfoPrint("");
    InfoPrint("=== Pre-Notification Block Sample ====");

    //
    // Register callback
    //
    
    RtlZeroMemory(RegisterCallbackInput.Altitude, MAX_ALTITUDE_BUFFER_LENGTH * sizeof(WCHAR));
    hr = StringCbPrintf(RegisterCallbackInput.Altitude, 
                          MAX_ALTITUDE_BUFFER_LENGTH * sizeof(WCHAR),
                          CALLBACK_ALTITUDE);

    if (!SUCCEEDED(hr)) {
        ErrorPrint("Copying altitude string failed. Error %d", hr);
        goto Exit;
    }

    RegisterCallbackInput.CallbackMode = CALLBACK_MODE_PRE_NOTIFICATION_BLOCK;

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
    // Create two keys.
    // Creating the "not modified" key will succeed.
    // Creating the other key will fail with ERROR_ACCESS_DENIED
    //

    //
    // NOTE: In the kernel debugger output, you will see 3 pre-notifications 
    // for create key even though we only call create key twice.
    // You will also see the message that create key is blocked twice.
    //
    // Kd output:
    //
    // RegFltr:        Callback: Altitude-380010, NotifyClass-RegNtPreCreateKeyEx.
    // RegFltr:        Callback: Altitude-380010, NotifyClass-RegNtPostCreateKeyEx.
    // RegFltr:        Callback: Altitude-380010, NotifyClass-RegNtPreCreateKeyEx.
    // RegFltr:        Callback: Create key _RegFltrKey blocked.
    // RegFltr:        Callback: Altitude-380010, NotifyClass-RegNtPreCreateKeyEx.
    // RegFltr:        Callback: Create key _RegFltrKey blocked.
    //
    // The reason this happens is that RegCreateKeyEx is more than just a 
    // wrapper around NtCreateKey. If the call to NtCreateKey fails, 
    // RegCreateKeyEx will retry the call in slightly different ways 
    // depending on the error returned. 
    //

    Res = RegCreateKeyEx(g_RootKey,
                         NOT_MODIFIED_KEY_NAME,
                         0,
                         L"Regfltr_test_class",
                         0,
                         KEY_ALL_ACCESS,
                         NULL,
                         &NotModifiedKey,
                         NULL);

    if (Res != ERROR_SUCCESS) {
        ErrorPrint("RegCreateKeyEx returned unexpected error %d", Res);
        Success = FALSE;
    }

    Res = RegCreateKeyEx(g_RootKey,
                         KEY_NAME,
                         0,
                         L"Regfltr_test_class",
                         0,
                         KEY_ALL_ACCESS,
                         NULL,
                         &Key,
                         NULL);

    if (Res != ERROR_ACCESS_DENIED) {
        ErrorPrint("RegCreateKeyEx returned unexpected error %d", Res);
        Success = FALSE;
    }

    //
    // Set two values. 
    // Setting the "not modified" value will succeed.
    // Setting the other value will fail with ERROR_ACCESS_DENIED.
    //

    Res = RegSetValueEx(g_RootKey,
                        NOT_MODIFIED_VALUE_NAME,
                        0,
                        REG_DWORD,
                        (BYTE *) &ValueData,
                        sizeof(ValueData));
        
    if(Res != ERROR_SUCCESS) {
        ErrorPrint("RegSetValueEx return unexpected status %d", Res);
        Success = FALSE;
    }

    Res = RegSetValueEx(g_RootKey,
                        VALUE_NAME,
                        0,
                        REG_DWORD,
                        (BYTE *) &ValueData,
                        sizeof(ValueData));
        
    if(Res != ERROR_ACCESS_DENIED) {
        ErrorPrint("RegSetValueEx return unexpected status %d", Res);
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

    //
    // Clean up
    //

    if (Key != NULL) {
        RegCloseKey(Key);
    }
    RegDeleteKey(g_RootKey, KEY_NAME);
    
    if (NotModifiedKey != NULL) {
        RegCloseKey(NotModifiedKey);
    }
    RegDeleteKey(g_RootKey, NOT_MODIFIED_KEY_NAME);
    
    RegDeleteValue(g_RootKey, NOT_MODIFIED_VALUE_NAME);
    RegDeleteValue(g_RootKey, VALUE_NAME);

    if (Success) {
        InfoPrint("Pre-Notification Block Sample succeeded.");
    } else {
        ErrorPrint("Pre-Notification Block Sample FAILED.");
    }
  
}


VOID 
PreNotificationBypassSample(
    )
/*++

Routine Description:

    This sample shows how to bypass a registry operation so that the CM does
    not process the operation. Unlike block, an operation that is bypassed
    is still considered successful so the callback must provide the caller
    with what the CM would have provided.

    A key and a value are created. However both operations are bypassed by the
    callback so that the key and value actually created have different names
    than would is expected.

    See ..\sys\Pre.c for the callback routine used in this sample. 
    
Return Value:

    None

--*/
{
    LONG Res;
    HRESULT hr;
    HKEY Key = NULL;
    DWORD ValueData = 0xDEADBEEF; 
    BOOL Result;
    BOOL Success = FALSE;
    DWORD BytesReturned;
    REGISTER_CALLBACK_INPUT RegisterCallbackInput = {0};
    REGISTER_CALLBACK_OUTPUT RegisterCallbackOutput = {0};
    UNREGISTER_CALLBACK_INPUT UnRegisterCallbackInput = {0};
    

    InfoPrint("");
    InfoPrint("=== Pre-Notification Bypass Sample ====");

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

    RegisterCallbackInput.CallbackMode = CALLBACK_MODE_PRE_NOTIFICATION_BYPASS;

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
    // Create a key and create a value. Both should succeed.
    //

    Res = RegCreateKeyEx(g_RootKey,
                         KEY_NAME,
                         0,
                         L"Regfltr_test_class",
                         0,
                         KEY_ALL_ACCESS,
                         NULL,
                         &Key,
                         NULL);

    if (Res != ERROR_SUCCESS) {
        ErrorPrint("RegCreateKeyEx returned unexpected error %d", Res);
        Success = FALSE;
    }

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
        
    //
    // Check that a key with the expected name KEY_NAME cannot be found 
    // but a key with the "modified" name can be found. 
    //

    if (Key != NULL) {
        RegCloseKey(Key);
    }

    Res = RegOpenKeyEx(g_RootKey,
                        KEY_NAME, 
                        0,
                        KEY_ALL_ACCESS,
                        &Key);

    if (Res != ERROR_FILE_NOT_FOUND) {
        ErrorPrint("RegOpenKeyEx returned unexpected error: %d", Res);
        if (Key != NULL) {
            RegCloseKey(Key);
            Key = NULL;
        }
        Success = FALSE;
    }

    Res = RegOpenKeyEx(g_RootKey,
                        MODIFIED_KEY_NAME, 
                        0,
                        KEY_ALL_ACCESS,
                        &Key);

    if (Res != ERROR_SUCCESS) {
        ErrorPrint("RegOpenKeyEx returned unexpected error: %d", Res);
        Success = FALSE;
    }

    //
    // Do the same check by trying to delete a value with VALUE_NAME and
    // with the "modified" name. 
    //

    Res = RegDeleteValue(g_RootKey, VALUE_NAME);

    if (Res != ERROR_FILE_NOT_FOUND) {
        ErrorPrint("RegDeleteValue on original value returned unexpected status: %d", 
                   Res);
        Success = FALSE;
    }

    Res = RegDeleteValue(g_RootKey, MODIFIED_VALUE_NAME);

    if (Res != ERROR_SUCCESS) {
        ErrorPrint("RegDeleteValue on original value returned unexpected status: %d", 
                   Res);
        Success = FALSE;
    }

  Exit: 

    if (Success) {
        InfoPrint("Pre-Notification Bypass Sample succeeded.");
    } else {
        ErrorPrint("Pre-Notification Bypass Sample failed.");
    }
    
    //
    // Clean up
    //

    if (Key != NULL) {
        RegCloseKey(Key);
    }
    RegDeleteKey(g_RootKey, KEY_NAME);
    RegDeleteKey(g_RootKey, MODIFIED_KEY_NAME);
    
}

