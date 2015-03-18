/*++
Copyright (c) Microsoft Corporation.  All rights reserved.

    THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY
    KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
    IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR
    PURPOSE.

Module Name:

    Post.c

Abstract: 

    Samples that show what callbacks can do during the post-notification
    phase.

Environment:

    User mode only

--*/


#include "regctrl.h"

/*++

    In registry callback version 1.0, there is a bug with post-notification
    processing and multiple registry filter drivers that can break the samples 
    here. It is fixed with version 1.1.

    The bug occurs when a driver blocks or bypasses a registry operation in the
    pre-notification phase. Even though the processing of the operation stops 
    there, registry filter drivers registered at higher altitudes will still 
    get a post-notification for the operation. If the higher altitude driver 
    tries to change the status of the operation from failure to success or 
    vice versa, this change will be ignored and the status returned 
    will be the status returned by the driver who bypassed or blocked the 
    operation during the pre-notification phase.

    For more information on how notification processing works with multiple 
    registry filter drivers registered see ..\sys\MultiAlt.c
    
    For more information on issues in version 1.0 and changes in version 1.1
    see ..\sys\Version.c 

    Beginning with Windows 8.1, it is no longer possible to pass the object
    provided to a RegNtPostCreateKeyEx or RegNtPostOpenKeyEx callout to
    ObOpenObjectByPointer. To work around this, filters should perform all
    create key or open key processing in a RegNtPreCreateKeyEx or
    RegNtPreOpenKeyEx callout. If for any reason the desired processing cannot
    be performed in a RegNtPreCreateKeyEx or RegNtPreOpenKeyEx callout, then
    use CmSetCallbackObjectContext to tag a newly-created or newly-opened key
    as unprocessed and process it in the pre-operation callback for a
    subsequent operation.

--*/


VOID
PostNotificationOverrideSuccessSample(
    )
/*++

Routine Description:

    This sample shows how registry callbacks can fail a registry operation 
    in the post-notification phase. 

    Two values are created. The creates normally should succeeded, but one 
    is intercepted by the callback and failed with ERROR_ACCESS_DENIED.

    See ..\sys\Post.c for the callback routine used in this sample. 

--*/
{
    LONG Res;
    HRESULT hr;
    BOOL Result;
    BOOL Success = FALSE;
    DWORD BytesReturned;
    DWORD ValueData = 0xDEADBEEF;
    REGISTER_CALLBACK_INPUT RegisterCallbackInput = {0};
    REGISTER_CALLBACK_OUTPUT RegisterCallbackOutput = {0};
    UNREGISTER_CALLBACK_INPUT UnRegisterCallbackInput = {0};


    InfoPrint("");
    InfoPrint("=== Post-Notification Override Success Sample ====");

    //
    // Register a callback with the specified callback mode and altitude.
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

    RegisterCallbackInput.CallbackMode = CALLBACK_MODE_POST_NOTIFICATION_OVERRIDE_SUCCESS;

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
    // Set two values. 
    // Setting the "not modified" value will succeed.
    // Setting the other value will fail with file not found.
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


    //
    // Verify that the set value call was failed by 
    // checking that the value with VALUE_NAME does not
    // exist.
    //

    Res = RegDeleteValue(g_RootKey, VALUE_NAME);

    if (Res != ERROR_FILE_NOT_FOUND) {
        ErrorPrint("RegDeleteValue on value returned unexpected status: %d", 
                   Res);
        Success = FALSE;
    }

  Exit:

    RegDeleteValue(g_RootKey, VALUE_NAME);
    RegDeleteValue(g_RootKey, NOT_MODIFIED_VALUE_NAME);

    if (Success) {
        InfoPrint("Post-Notification Override Success Sample succeeded.");
    } else {
        ErrorPrint("Post-Notification Override Success Sample FAILED.");
    }
   
}


VOID 
PostNotificationOverrideErrorSample(
    )
/*++

Routine Description:

    This sample shows how a registry callback can change a failed registry
    operation into a successful operation in the post-notification phase. 

    A key that does not exist is opened. The opens should fail, but it is 
    intercepted by the callback and the open is redirected to a key that 
    does exist.

    See ..\sys\Post.c for the callback routine used in this sample. 
    
Return Value:

    None

--*/
{

    LONG Res;
    HRESULT hr;
    BOOL Success = FALSE;
    BOOL Result;
    HKEY Key = NULL;
    HKEY ModifiedKey = NULL;
    DWORD BytesReturned;
    REGISTER_CALLBACK_INPUT RegisterCallbackInput = {0};
    REGISTER_CALLBACK_OUTPUT RegisterCallbackOutput = {0};
    UNREGISTER_CALLBACK_INPUT UnRegisterCallbackInput = {0};
    
    InfoPrint("");
    InfoPrint("=== Post-Notification Override Error Sample ====");

    //
    // Create a key with name MODIFIED_KEY_NAME
    //

    Res = RegCreateKeyEx(g_RootKey,
                         MODIFIED_KEY_NAME,
                         0,
                         NULL,
                         0,
                         KEY_ALL_ACCESS,
                         NULL,
                         &ModifiedKey,
                         NULL);

    if (Res != ERROR_SUCCESS) {
        ErrorPrint("RegCreateKeyEx returned unexpected error %d", Res);
        goto Exit;
    }

    //
    // Now try to open a key by KEY_NAME which does not exist. Verify that 
    // this fails.
    //

    Res = RegOpenKeyEx(g_RootKey,
                       KEY_NAME,
                       0,
                       KEY_ALL_ACCESS,
                       &Key);

    if (Res != ERROR_FILE_NOT_FOUND) {
        ErrorPrint("RegOpenKeyEx returned unexpected error %d", Res);
        goto Exit;
    }
    
    //
    // Register a callback with the specified callback mode and altitude.
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


    RegisterCallbackInput.CallbackMode = CALLBACK_MODE_POST_NOTIFICATION_OVERRIDE_ERROR;

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
    // Open key again. The callback will intercept this and make it succeed.
    //

    Res = RegOpenKeyEx(g_RootKey,
                       KEY_NAME,
                       0,
                       KEY_ALL_ACCESS,
                       &Key);

    if (Res != ERROR_SUCCESS) {
        ErrorPrint("RegOpenKeyEx returned unexpected error %d", Res);
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

    if (Key != NULL) {
        RegCloseKey(Key);
    }
    if (ModifiedKey != NULL) {
        RegCloseKey(ModifiedKey);
    }
    RegDeleteKey(g_RootKey, KEY_NAME);
    RegDeleteKey(g_RootKey, MODIFIED_KEY_NAME);
    
    if (Success) {
        InfoPrint("Post-Notification Override Error Sample succeeded.");
    } else {
        ErrorPrint("Post-Notification Override Error Sample FAILED.");
    }

}

