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

    Kernel mode only

--*/

#include "regfltr.h"


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
    registry filter drivers registered see MultiAlt.c
    
    For more information on issues in version 1.0 and changes in version 1.1
    see Version.c 

    
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



BOOLEAN
PostNotificationOverrideSuccessSample(
    )
/*++

Routine Description:

    This sample shows how registry callbacks can fail a registry operation 
    in the post-notification phase. 

    Two values are created. The creates normally should succeeded, but one 
    is intercepted by the callback and failed with STATUS_ACCESS_DENIED.
    
    NOTE: This sample does not take into account transactions. See txr.c for
    examples on how to handle transactional registry operations.

Return Value:

    TRUE if the sample completed successfully.

--*/
{
    PCALLBACK_CONTEXT CallbackCtx = NULL;
    NTSTATUS Status;
    UNICODE_STRING Name;
    DWORD ValueData = 0; 
    BOOLEAN Success = FALSE;

    InfoPrint("");
    InfoPrint("=== Post-Notification Override Success Sample ====");

    //
    // Create the callback context
    //

    CallbackCtx = CreateCallbackContext(CALLBACK_MODE_POST_NOTIFICATION_OVERRIDE_SUCCESS, 
                                         CALLBACK_ALTITUDE);

    if (CallbackCtx == NULL) {
        goto Exit;
    }
    
    //
    // Register callback 
    //

    Status = CmRegisterCallbackEx(Callback,
                                  &CallbackCtx->Altitude,
                                  g_DeviceObj->DriverObject,
                                  (PVOID) CallbackCtx,
                                  &CallbackCtx->Cookie, 
                                  NULL);
    if (!NT_SUCCESS(Status)) {
        ErrorPrint("CmRegisterCallback failed. Status 0x%x", Status);
        goto Exit;
    }

    Success = TRUE;

    //
    // Set two values. 
    // Setting the "not modified" value will succeed.
    // Setting the other value will fail with file not found.
    //
    
    RtlInitUnicodeString(&Name, NOT_MODIFIED_VALUE_NAME);
    Status = ZwSetValueKey(g_RootKey,
                           &Name,
                           0,
                           REG_DWORD,
                           &ValueData,
                           sizeof(ValueData));
    
    if(!NT_SUCCESS(Status)) {
        ErrorPrint("ZwSetValue return unexpected status 0x%x", Status);
        Success = FALSE;
    }

    RtlInitUnicodeString(&Name, VALUE_NAME);
    Status = ZwSetValueKey(g_RootKey,
                           &Name,
                           0,
                           REG_DWORD,
                           &ValueData,
                           sizeof(ValueData));
    
    if(Status != STATUS_ACCESS_DENIED) {
        ErrorPrint("ZwSetValue return unexpected status 0x%x", Status);
        Success = FALSE;
    }

    //
    // Unregister the callback
    //

    Status = CmUnRegisterCallback(CallbackCtx->Cookie);

    if (!NT_SUCCESS(Status)) {
        ErrorPrint("CmUnRegisterCallback failed. Status 0x%x", Status);
        Success = FALSE;
    }


    //
    // Verify that the set value calls were failed by 
    // checking that the value with VALUE_NAME does not
    // exist.
    //  
    // Deleting the other value should return STATUS_OBJECT_NAME_NOT_FOUND
    // Deleting value with the modified name should succeed.
    //

    RtlInitUnicodeString(&Name, VALUE_NAME);
    Status = ZwDeleteValueKey(g_RootKey, &Name);

    if (Status != STATUS_OBJECT_NAME_NOT_FOUND) {
        ErrorPrint("ZwDeleteValueKey on value failed. Status: 0x%x", Status);
        Success = FALSE;
    }

  Exit:
    
    //
    // Clean up
    //

    RtlInitUnicodeString(&Name, VALUE_NAME);
    ZwDeleteValueKey(g_RootKey, &Name);
    RtlInitUnicodeString(&Name, NOT_MODIFIED_VALUE_NAME);
    ZwDeleteValueKey(g_RootKey, &Name);

    if (CallbackCtx != NULL) {
        ExFreePoolWithTag(CallbackCtx, REGFLTR_CONTEXT_POOL_TAG);
    }

    if (Success) {
        InfoPrint("Post-Notification Override Success Sample succeeded.");
    } else {
        ErrorPrint("Post-Notification Override Success Sample FAILED.");
    }
    return Success;
    
}


NTSTATUS 
CallbackPostNotificationOverrideSuccess(
    _In_ PCALLBACK_CONTEXT CallbackCtx,
    _In_ REG_NOTIFY_CLASS NotifyClass,
    _Inout_ PVOID Argument2
    )
/*++

Routine Description:

    This helper callback routine intercepts create key and set value post 
    notifications and fails the operation with STATUS_ACCESS_DENIED.

    NOTE: This sample does not take into account transactions. See txr.c for
    examples on how to handle transactional registry operations.

Arguments:

    CallbackContext - The value that the driver passed to the Context parameter
        of CmRegisterCallbackEx when it registers this callback routine.

    NotifyClass - A REG_NOTIFY_CLASS typed value that identifies the type of 
        registry operation that is being performed and whether the callback
        is being called in the pre or post phase of processing.

    Argument2 - A pointer to a structure that contains information specific
        to the type of the registry operation. The structure type depends
        on the REG_NOTIFY_CLASS value of Argument1. 

Return Value:

    NTSTATUS

--*/
{

    NTSTATUS Status = STATUS_SUCCESS;
    PREG_CREATE_KEY_INFORMATION PreCreateInfo;
    PREG_SET_VALUE_KEY_INFORMATION PreSetValueInfo;
    PREG_POST_OPERATION_INFORMATION PostInfo;
    UNICODE_STRING Name;
    HANDLE Key = NULL;

    UNREFERENCED_PARAMETER(CallbackCtx);
    
    switch(NotifyClass) {
        case RegNtPostCreateKeyEx:
            PostInfo = (PREG_POST_OPERATION_INFORMATION) Argument2;
            PreCreateInfo = (PREG_CREATE_KEY_INFORMATION) PostInfo->PreInformation;

            //
            // REG_CREATE_KEY_INFORMATION is a partially captured 
            // structure however no uncaptured fields are used here. For more 
            // information on what parameters need to be captured, see 
            // capture.c.
            //

            //
            // Only intercept the operation if the key being created has the
            // name KEY_NAME.
            //
            
            RtlInitUnicodeString(&Name, KEY_NAME);
            if (!RtlEqualUnicodeString((PCUNICODE_STRING) &Name, 
                                       (PCUNICODE_STRING) PreCreateInfo->CompleteName, 
                                      TRUE)) {
                break;
            }

            //
            // Make sure the operation is successful so far.
            //
            
            if (!NT_SUCCESS(PostInfo->Status)) {
                ErrorPrint("Operation status in post notification is unexpectedly 0x%x",
                           PostInfo->Status);
                break;
            }

            //
            // Since this is the post-notification phase, the key has
            // already been created. It is stored in PostInfo->Object.
            // Get a handle on the key and delete it.
            //
            
            Status = ObOpenObjectByPointer(PostInfo->Object,
                                           OBJ_KERNEL_HANDLE,
                                           NULL,
                                           KEY_ALL_ACCESS,
                                           PreCreateInfo->ObjectType,
                                           KernelMode,
                                           &Key);

            if (!NT_SUCCESS (Status)) {
                ErrorPrint("ObObjectByPointer failed. Status 0x%x\n", Status);
                break;
            }

            Status = ZwDeleteKey(Key);

            if (!NT_SUCCESS(Status)) {
                ErrorPrint("ZwDeleteKey failed. Status 0x%x\n", Status);
                break;
            }

            ZwClose(Key);
            
            //
            // Dereference the object because it will not be returned to
            // the user. NULL out the references to the object in the
            // post and pre information structures.
            //
            
            ObDereferenceObject(PostInfo->Object);
            PostInfo->Object = NULL;
            *PreCreateInfo->ResultObject = NULL;

            InfoPrint("\tCallback: Create key %wZ overrided from success to error.", 
                      PreCreateInfo->CompleteName);

            //
            // Put the status to be returned in PostInfo->ReturnStatus and
            // return STATUS_CALLBACK_BYPASS to let CM know that
            // we want to change the return status.
            //
            // DO NOT set PostInfo->Status
            //
            
            PostInfo->ReturnStatus = STATUS_ACCESS_DENIED;
            Status = STATUS_CALLBACK_BYPASS;
            break;

        case RegNtPostSetValueKey:

            PostInfo = (PREG_POST_OPERATION_INFORMATION) Argument2;
            PreSetValueInfo = (PREG_SET_VALUE_KEY_INFORMATION) PostInfo->PreInformation;

            //
            // NOTE: REG_SET_VALUE_KEY_INFORMATION is a partially captured 
            // structure. The value name is captured but the data buffer is 
            // not. Since we are only using the value name we do not need to 
            // capture any parameters. For more information on what parameters 
            // need to be captured, see capture.c.
            //

            //
            // Only intercept the operation if the value being set has the
            // name VALUE_NAME.
            //

            RtlInitUnicodeString(&Name, VALUE_NAME);
            if (!RtlEqualUnicodeString((PCUNICODE_STRING) &Name, 
                                       (PCUNICODE_STRING) PreSetValueInfo->ValueName, 
                                       TRUE)) {
                break;
            }

            //
            // Make sure the operation is successful so far.
            //

            if (!NT_SUCCESS(PostInfo->Status)) {
                ErrorPrint("Post notification status is unexpectedly 0x%x.",
                           PostInfo->Status);
                break;
            }

            //
            // To fail the operation, we have to delete the value that has
            // been created. To do so, we need a handle to the root key.
            //

            Status = ObOpenObjectByPointer(PreSetValueInfo->Object,
                                           OBJ_KERNEL_HANDLE,
                                           NULL,
                                           KEY_ALL_ACCESS,
                                           NULL,
                                           KernelMode,
                                           &Key);

            if (!NT_SUCCESS (Status)) {
                ErrorPrint("ObObjectByPointer failed. Status 0x%x\n", Status);
                break;
            }

            Status = ZwDeleteValueKey(Key, PreSetValueInfo->ValueName);

            if (!NT_SUCCESS(Status)) {
                ErrorPrint("ZwDeleteValueKey failed. Status 0x%x\n", Status);
                break;
            }

            ZwClose(Key);

            //
            // Put the status to be returned in PostInfo->ReturnStatus and
            // return STATUS_CALLBACK_BYPASS to let CM know that
            // we want to change the return status.
            //
            // DO NOT set PostInfo->Status
            //
                
            InfoPrint("\tCallback: Value %wZ overrided from success to error.", 
                      PreSetValueInfo->ValueName);
            PostInfo->ReturnStatus = STATUS_ACCESS_DENIED;
            Status = STATUS_CALLBACK_BYPASS;
            break;

        default:
            //
            // Do nothing for other notifications
            //
            break;
    }

    return Status;
    
}


BOOLEAN 
PostNotificationOverrideErrorSample(
    )
/*++

Routine Description:

    This sample shows how a registry callback can change a failed registry
    operation into a successful operation in the post-notification phase. 

    A key that does not exist is opened. The opens should fail, but it is 
    intercepted by the callback and the open is redirected to a key that 
    does exist.

    NOTE: This sample does not take into account transactions. See txr.c for
    examples on how to handle transactional registry operations.
    
Return Value:

    TRUE if the sample completed successfully.

--*/
{
    PCALLBACK_CONTEXT CallbackCtx = NULL;
    NTSTATUS Status;
    OBJECT_ATTRIBUTES KeyAttributes;
    UNICODE_STRING Name;
    BOOLEAN Success = FALSE;
    HANDLE Key = NULL;
    HANDLE ModifiedKey = NULL;

    InfoPrint("");
    InfoPrint("=== Post-Notification Override Error Sample ====");

    //
    // Create the callback context 
    //

    CallbackCtx = CreateCallbackContext(CALLBACK_MODE_POST_NOTIFICATION_OVERRIDE_ERROR,
                                         CALLBACK_ALTITUDE);
    if (CallbackCtx == NULL) {
        goto Exit;
    }

    //
    // Create a key with name MODIFIED_KEY_NAME
    //

    RtlInitUnicodeString(&Name, MODIFIED_KEY_NAME);
    InitializeObjectAttributes(&KeyAttributes,
                               &Name,
                               OBJ_CASE_INSENSITIVE | OBJ_KERNEL_HANDLE,
                               g_RootKey,
                               NULL);

    Status = ZwCreateKey(&ModifiedKey,
                         KEY_ALL_ACCESS,
                         &KeyAttributes,
                         0,
                         NULL,
                         0,
                         NULL);

    if (!NT_SUCCESS(Status)) {
        ErrorPrint("Creating modified key failed. Status 0x%x", Status);
        goto Exit;
    }

    //
    // Now try to open a key by KEY_NAME which does not exist. Verify that 
    // this fails.
    //

    RtlInitUnicodeString(&Name, KEY_NAME);
    InitializeObjectAttributes(&KeyAttributes,
                               &Name,
                               OBJ_CASE_INSENSITIVE | OBJ_KERNEL_HANDLE,
                               g_RootKey,
                               NULL);

    Status = ZwOpenKey(&Key,
                       KEY_ALL_ACCESS,
                       &KeyAttributes);

    if (Status != STATUS_OBJECT_NAME_NOT_FOUND) {
        ErrorPrint("ZwCreateKey returned unexpected status 0x%x", Status);
        goto Exit;
    } 
        
    //
    // Register our callback with the context
    //

    Status = CmRegisterCallbackEx(Callback,
                                  &CallbackCtx->Altitude,
                                  g_DeviceObj->DriverObject,
                                  (PVOID) CallbackCtx,
                                  &CallbackCtx->Cookie, 
                                  NULL);
    if (!NT_SUCCESS(Status)) {
        ErrorPrint("CmRegisterCallback failed. Status 0x%x", Status);
        goto Exit;
    }

    Success = TRUE;
    
    //
    // Open key again. The callback will intercept this and make it succeed.
    //

    Status = ZwOpenKey(&Key,
                       KEY_ALL_ACCESS,
                       &KeyAttributes);

    if (!NT_SUCCESS(Status)) {
        ErrorPrint("ZwOpenKey failed unexpectedly. Status 0x%x", Status);
        Success = FALSE;
    }

    //
    // Unregister the callback
    //

    Status = CmUnRegisterCallback(CallbackCtx->Cookie);

    if (!NT_SUCCESS(Status)) {
        ErrorPrint("CmUnRegisterCallback failed. Status 0x%x", Status);
        Success = FALSE;
    }

  Exit:

    //
    // Clean up
    //

    if (Key != NULL) {
        ZwDeleteKey(Key);
        ZwClose(Key);
    }

    if (ModifiedKey != NULL) {
        ZwDeleteKey(ModifiedKey);
        ZwClose(ModifiedKey);
    }

    if (CallbackCtx != NULL) {
        ExFreePoolWithTag(CallbackCtx, REGFLTR_CONTEXT_POOL_TAG);
    }
    
    if (Success) {
        InfoPrint("Post-Notification Override Error Sample succeeded.");
    } else {
        ErrorPrint("Post-Notification Override Error Sample FAILED.");
    }

    return Success;
}


NTSTATUS 
CallbackPostNotificationOverrideError(
    _In_ PCALLBACK_CONTEXT CallbackCtx,
    _In_ REG_NOTIFY_CLASS NotifyClass,
    _Inout_ PVOID Argument2
    )
/*++

Routine Description:

    This helper callback routine intercepts open key post notifications 
    and if they are failing with STATUS_ACCESS_DENIED, it makes 
    the operation successful by redirecting the open to antoher key.

    NOTE: This sample does not take into account transactions. See txr.c for
    examples on how to handle transactional registry operations.
    
Arguments:

    CallbackContext - The value that the driver passed to the Context parameter
        of CmRegisterCallbackEx when it registers this callback routine.

    NotifyClass - A REG_NOTIFY_CLASS typed value that identifies the type of 
        registry operation that is being performed and whether the callback
        is being called in the pre or post phase of processing.

    Argument2 - A pointer to a structure that contains information specific
        to the type of the registry operation. The structure type depends
        on the REG_NOTIFY_CLASS value of Argument1. 

Return Value:

    NTSTATUS

--*/
{
    NTSTATUS Status = STATUS_SUCCESS;
    PREG_OPEN_KEY_INFORMATION PreOpenInfo;
    PREG_POST_OPERATION_INFORMATION PostInfo;
    UNICODE_STRING Name;
    OBJECT_ATTRIBUTES KeyAttributes;
    HANDLE Key = NULL;
    HANDLE RootKey = NULL;
    PVOID Object;
    
    UNREFERENCED_PARAMETER(CallbackCtx);

    switch(NotifyClass) {
        case RegNtPostOpenKeyEx:

            PostInfo = (PREG_POST_OPERATION_INFORMATION) Argument2;
            PreOpenInfo = (PREG_OPEN_KEY_INFORMATION) PostInfo->PreInformation;

            //
            // NOTE: REG_OPEN_KEY_INFORMATION is a fully captured structure
            // so there is no need for the callback to capture any parameters.
            // For more information on what parameters need to be captured, see
            // capture.c.
            //

            //
            // Only intercept the operation if the key being created has the
            // name KEY_NAME.
            //

            RtlInitUnicodeString(&Name, KEY_NAME);
            if (!RtlEqualUnicodeString((PCUNICODE_STRING) &Name, 
                                       (PCUNICODE_STRING) PreOpenInfo->CompleteName, 
                                       TRUE)) {
                break;
            }

            //
            // Verify that operation is currently failing as expected
            //
            
            if (PostInfo->Status != STATUS_OBJECT_NAME_NOT_FOUND) {
                ErrorPrint("Operation did not fail with status not found as expected. Post status: 0x%x",
                           PostInfo->Status);
                break;
            }

            // 
            // To make the operation successful, an object MUST be supplied as
            // the opened handle. In this sample, the object will be a handle
            // to another key that does exist.
            //

            //
            // First open the key and get its handle.
            //

            Status = ObOpenObjectByPointer(PreOpenInfo->RootObject,
                                           OBJ_KERNEL_HANDLE,
                                           NULL,
                                           KEY_ALL_ACCESS, 
                                           PreOpenInfo->ObjectType,
                                           KernelMode,
                                           &RootKey);

            if (!NT_SUCCESS (Status)) {
                ErrorPrint("ObObjectByPointer failed. Status 0x%x", Status);
                break;
            }

            RtlInitUnicodeString(&Name, MODIFIED_KEY_NAME);
            InitializeObjectAttributes(&KeyAttributes,
                                       &Name,
                                       OBJ_CASE_INSENSITIVE | OBJ_KERNEL_HANDLE,
                                       RootKey,
                                       PreOpenInfo->SecurityDescriptor);

            Status = ZwOpenKey(&Key,
                               PreOpenInfo->DesiredAccess,
                               &KeyAttributes);
 
            if (!NT_SUCCESS(Status)) {
                ErrorPrint("ZwOpenKey failed. Status 0x%x", Status);
                ZwClose(RootKey);
                break;
            } 

            ZwClose(RootKey);
            
            //
            // Then, get the object pointer from the new key's handle.
            //
            
            Status = ObReferenceObjectByHandle(Key,
                                               PreOpenInfo->DesiredAccess,
                                               PreOpenInfo->ObjectType,
                                               KernelMode,
                                               &Object,
                                               NULL);
            if (!NT_SUCCESS (Status)) {
                ErrorPrint("ObReferenceObjectByHandle failed. Status 0x%x", Status);
                ZwClose(Key);
                break;
            } 

            //
            // Finally, set the ResultObject field in the PreInfo and 
            // the Object field in the PostInfo to the object just opened.
            //
                
            *PreOpenInfo->ResultObject = Object;
            PreOpenInfo->GrantedAccess = PreOpenInfo->DesiredAccess;

            if (PostInfo->Object != NULL) {
                ErrorPrint("PostInfo->Object should be NULL! Instead is 0x%p", 
                           PostInfo->Object);
            }
            PostInfo->Object = Object;

            ZwClose(Key);
            InfoPrint("\tCallback: Opening key %wZ overrided from error to success.", 
                      PreOpenInfo->CompleteName);

            //
            // Put the status to be returned in PostInfo->ReturnStatus and
            // return STATUS_CALLBACK_BYPASS to let CM know that
            // we want to change the return status.
            //
            // DO NOT set PostInfo->Status
            //
            
            PostInfo->ReturnStatus = STATUS_SUCCESS;
            Status = STATUS_CALLBACK_BYPASS;
            break;

        default:
            //
            // Do nothing for other notifications
            //
            break;
    }

    return Status;
    
}

