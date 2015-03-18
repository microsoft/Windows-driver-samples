/*++
Copyright (c) Microsoft Corporation.  All rights reserved.

    THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY
    KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
    IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR
    PURPOSE.

Module Name:

    Context.c

Abstract: 

    Samples that show how to set call contexts and object contexts.

Environment:

    Kernel mode only

--*/

#include "regfltr.h"


BOOLEAN 
SetObjectContextSample(
    )
/*++

Routine Description:

    This sample shows how a registry callback can associate a context on
    a registry object using CmSetCallbackObjectContext. 

    This context is available in the ObjectContext field of the 
    REG_Xxx_KEY_INFORMATION data structures. The registry object is a handle
    to a key and not the registry key itself. When the handle is closed
    or the callback is unregistered, the callback will receive a 
    RegNtCallbackObjectContextCleanup notification to give a chance to 
    clean up the context.

Return Value:

    TRUE if the sample completed successfully.

--*/
{

    PCALLBACK_CONTEXT CallbackCtx = NULL;
    NTSTATUS Status;
    UNICODE_STRING Name;
    OBJECT_ATTRIBUTES KeyAttributes;
    HANDLE RootKeyWithContext = NULL;
    DWORD ValueData = 0;
    BOOLEAN Success = FALSE;


    InfoPrint("");
    InfoPrint("=== Set Object Context Sample ====");

    //
    // Create the callback context
    //

    CallbackCtx = CreateCallbackContext(CALLBACK_MODE_SET_OBJECT_CONTEXT,
                                         CALLBACK_ALTITUDE);

    if (CallbackCtx == NULL) {
        goto Exit;
    }

    //
    // Register the callback
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
    // Open the root key again. The callback will associate an object
    // context with the RootKeyWithContext handle.
    //

    RtlInitUnicodeString(&Name, ROOT_KEY_ABS_PATH);
    InitializeObjectAttributes(&KeyAttributes,
                               &Name,
                               OBJ_CASE_INSENSITIVE | OBJ_KERNEL_HANDLE,
                               NULL,
                               NULL);
    
    Status = ZwOpenKey(&RootKeyWithContext,
                       KEY_ALL_ACCESS,
                       &KeyAttributes);

    if (!NT_SUCCESS(Status)) {
        ErrorPrint("ZwOpenKey on root key failed. Status 0x%x", Status);
        Success = FALSE;
    }

    //
    // Set value using both the g_RootKey handle and the RootKeyWithContext
    // handle.
    //

    RtlInitUnicodeString(&Name, VALUE_NAME);
    Status = ZwSetValueKey(g_RootKey,
                           &Name,
                           0,
                           REG_DWORD,
                           &ValueData,
                           sizeof(ValueData));
    
    if(!NT_SUCCESS(Status)) {
        ErrorPrint("ZwSetValue failed. Status 0x%x", Status);
        Success = FALSE;
    }

    if (RootKeyWithContext != NULL) {
        Status = ZwSetValueKey(RootKeyWithContext,
                               &Name,
                               0,
                               REG_DWORD,
                               &ValueData,
                               sizeof(ValueData));
        
        if(!NT_SUCCESS(Status)) {
            ErrorPrint("ZwSetValue failed. Status 0x%x", Status);
            Success = FALSE;
        }
    }

    //
    // Unregister the callback
    //

    Status = CmUnRegisterCallback(CallbackCtx->Cookie);

    if (!NT_SUCCESS(Status)) {
        ErrorPrint("CmUnRegisterCallback failed. Status 0x%x", Status);
    }

    //
    // Check that the RegNtCallbackObjectContextCleanup notification was
    // received when we unregistered the callback.
    //

    if (CallbackCtx->ContextCleanupCount != 1) {
        ErrorPrint("Callback was not invoked for a context cleanup notification.");
        Success = FALSE;
    } 

    //
    // Check that there were two notifications that had the object context set.
    // These are the pre and post set value using the RootKeyWithContext handle.
    //
    
    if (CallbackCtx->NotificationWithContextCount != 2) {
        ErrorPrint("Callback OperationWithContext count expected 2, instead it was %d",
                   CallbackCtx->NotificationWithContextCount);
        Success = FALSE;
    } 

    //
    // Check that there were two notifications that did not have the object 
    // context set. These are the pre and post set value using the 
    // g_RootKey handle.
    //
    
    if (CallbackCtx->NotificationWithNoContextCount != 2) {
        ErrorPrint("Callback OperationWithNoContext count expected 2, instead it was %d",
                   CallbackCtx->NotificationWithNoContextCount);
        Success = FALSE;
    } 

  Exit:
    
    if (Success == TRUE) {
        InfoPrint("Set Object Context Sample Succeeded.");
    } else {
        ErrorPrint("Set Object Context Sample FAILED.");
    }

    //
    // Clean up
    //
    
    RtlInitUnicodeString(&Name, VALUE_NAME);
    ZwDeleteValueKey(g_RootKey, &Name);
    
    if (RootKeyWithContext != NULL) {
        ZwClose(RootKeyWithContext);
    }

    if (CallbackCtx != NULL) {
        ExFreePoolWithTag(CallbackCtx, REGFLTR_CONTEXT_POOL_TAG);
    }

    return Success;
}


NTSTATUS 
CallbackSetObjectContext(
    _In_ PCALLBACK_CONTEXT CallbackCtx,
    _In_ REG_NOTIFY_CLASS NotifyClass,
    _Inout_ PVOID Argument2
)
/*++

Routine Description:

    This helper callback routine shows how to associate a registry key object 
    with context information using CmSetCallbackObjectContext. The context
    set is then only available to this callback. A callback that sets the 
    object context should be prepared for a RegNtCallbackObjectContextCleanup
    where it must clean up the context.

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

    Always STATUS_SUCCESS;

--*/

{
    NTSTATUS Status = STATUS_SUCCESS;
    PREG_CALLBACK_CONTEXT_CLEANUP_INFORMATION CleanupInfo;
    PREG_POST_OPERATION_INFORMATION PostInfo;
    PVOID ObjectContext = NULL;
    
    switch(NotifyClass) {

        case RegNtPostOpenKeyEx:

            PostInfo = (PREG_POST_OPERATION_INFORMATION) Argument2;

            //
            // If the open key was successful, set an object context
            // to the key object. 
            //
            // Note that one of the parameters of CmSetCallbackObjectContext
            // is the cookie gotten from registering a callback. The object
            // context will only be available to the callback with that
            // particular cookie. 
            //
            
            if (NT_SUCCESS(PostInfo->Status)) {

                //
                // Never call CmSetCallbackObjectContext outside of the
                // callback routine.
                //

                Status = CmSetCallbackObjectContext(PostInfo->Object,
                                                    &CallbackCtx->Cookie,
                                                    CallbackCtx,
                                                    NULL);

                if (!NT_SUCCESS(Status)) {
                    ErrorPrint("CmSetCallbackobjectContext failed. Status 0x%x",
                               Status);
                }
            }
            break;

        case RegNtPreSetValueKey:
        case RegNtPostSetValueKey:

            //
            // All registry operations using the handle received from the open
            // key operation will come with the ObjectContext field set to the
            // context information. Other operations on the same key but 
            // using a different handle will not have the ObjectContext field 
            // set.
            //

            if (NotifyClass == RegNtPreSetValueKey) {
                ObjectContext = ((PREG_SET_VALUE_KEY_INFORMATION) Argument2)->ObjectContext;
            } else {
                ObjectContext = ((PREG_POST_OPERATION_INFORMATION) Argument2)->ObjectContext;
            }

            if (ObjectContext == NULL) {
                InterlockedIncrement(&CallbackCtx->NotificationWithNoContextCount);
            } else if (ObjectContext == CallbackCtx) {
                InterlockedIncrement(&CallbackCtx->NotificationWithContextCount);
            } else {
                ErrorPrint("Unexpected ObjectContext value: 0x%p", ObjectContext);
            }

            break;
            
        case RegNtCallbackObjectContextCleanup:

            //
            // This is a special notification only invoked for callbacks
            // that have set context information to an object. This notification 
            // is either sent when the registry object is being closed or if
            // the callback is being unregistered. In the first case, this 
            // notification comes after the RegNtPreKeyHandleClose
            // notification and before the RegNtPostKeyHandleClose notification.
            //
            
            CleanupInfo = (PREG_CALLBACK_CONTEXT_CLEANUP_INFORMATION) Argument2;
            if (CleanupInfo->ObjectContext != CallbackCtx) {
                ErrorPrint("ContextCleanup's ObjectContext has unexpected value: 0x%p.",
                           CleanupInfo->ObjectContext);
            } else {
                InterlockedIncrement(&CallbackCtx->ContextCleanupCount);
            }
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
SetCallContextSample(
    )
/*++

Routine Description:

    This sample shows how a registry callback can associate a context
    with a registry operation during the pre-notification phase so that it
    is available in the post-notification phase. 

Return Value:

    TRUE if the sample completed successfully.

--*/
{
    PCALLBACK_CONTEXT CallbackCtx = NULL;
    NTSTATUS Status;
    OBJECT_ATTRIBUTES KeyAttributes;
    UNICODE_STRING Name;
    HANDLE Key = NULL;
    DWORD ValueData = 0;
    BOOLEAN Success = FALSE;
    

    InfoPrint("");
    InfoPrint("=== Set Operation Context Sample ====");

    //
    // Create the callback context
    //

    CallbackCtx = CreateCallbackContext(CALLBACK_MODE_SET_CALL_CONTEXT,
                                         CALLBACK_ALTITUDE);

    if (CallbackCtx == NULL) {
        goto Exit;
    }
    
    //
    // Register callback with the context
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
    // Create a key and set a value.
    //

    RtlInitUnicodeString(&Name, KEY_NAME);
    InitializeObjectAttributes(&KeyAttributes,
                               &Name,
                               OBJ_CASE_INSENSITIVE | OBJ_KERNEL_HANDLE,
                               g_RootKey,
                               NULL);

    Status = ZwCreateKey(&Key,
                         KEY_ALL_ACCESS,
                         &KeyAttributes,
                         0,
                         NULL,
                         0,
                         NULL);

    if (!NT_SUCCESS(Status)) {
        ErrorPrint("ZwCreateKey failed. Status 0x%x", Status);
        Success = FALSE;
    }

    RtlInitUnicodeString(&Name, VALUE_NAME);
    Status = ZwSetValueKey(g_RootKey,
                           &Name,
                           0,
                           REG_DWORD,
                           &ValueData,
                           sizeof(ValueData));
    
    if(!NT_SUCCESS(Status)) {
        ErrorPrint("ZwSetValue failed. Status 0x%x", Status);
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
    // Check that the callback records 2 in OperationContextCount.
    // The count should be incremented once in the post-notification for the
    // create key and once for the set value.
    //

    if (CallbackCtx->NotificationWithContextCount != 2) {
        ErrorPrint("Callback OperationWithContextCount expected 2, got %d",
                   CallbackCtx->NotificationWithContextCount);
        Success = FALSE;
    }

  Exit:
    
    if (Success == TRUE) {
        InfoPrint("Set Call Context sample succeeded.");
    } else {
        ErrorPrint("Set Call Context sample FAILED.");
    }

    //
    // Clean up
    //
    
    if (Key != NULL) {
        ZwDeleteKey(Key);
        ZwClose(Key);
    }

    RtlInitUnicodeString(&Name, VALUE_NAME);
    ZwDeleteValueKey(g_RootKey, &Name);

    if (CallbackCtx != NULL) {
        ExFreePoolWithTag(CallbackCtx, REGFLTR_CONTEXT_POOL_TAG);
    }

    return Success;

}



NTSTATUS 
CallbackSetCallContext(
    _In_ PCALLBACK_CONTEXT CallbackCtx,
    _In_ REG_NOTIFY_CLASS NotifyClass,
    _Inout_ PVOID Argument2
    )
/*++

Routine Description:

    This helper callback routine shows how to attach context information to 
    the registry operation itself in the pre-notification phase and 
    have access to that context in the post-notification phase. The context
    is private to this callback.

    ***Note: Any callback that receives a pre-notification will receive
    a post-notifcation EXCEPT if the callback returns a non-success value 
    (this includes STATUS_CALLBACK_BYPASS) during the pre phase.
    
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

    Always STATUS_SUCCESS

--*/

{
    NTSTATUS Status = STATUS_SUCCESS;
    PREG_POST_OPERATION_INFORMATION PostInfo;
    PREG_CREATE_KEY_INFORMATION PreCreateInfo;
    PREG_SET_VALUE_KEY_INFORMATION PreSetValueInfo;
    
    switch(NotifyClass) {

        //
        // Set the call context by setting it to the CallContext field of the
        // REG_XXX_KEY_INFORMATION structure during the pre-notification phase.
        //
        
        case RegNtPreSetValueKey:
            PreSetValueInfo = (PREG_SET_VALUE_KEY_INFORMATION) Argument2;
            PreSetValueInfo->CallContext = CallbackCtx;
            break;
            
        case RegNtPreCreateKeyEx:
            PreCreateInfo = (PREG_CREATE_KEY_INFORMATION) Argument2;
            PreCreateInfo->CallContext = CallbackCtx;
            break;

        //
        // In the post-notification phase, check that the CallContext field
        // of REG_POST_OPERATION_INFORMATION contains the context we set in
        // the pre phase. 
        //
        
        case RegNtPostSetValueKey:
        case RegNtPostCreateKeyEx:
            PostInfo = (PREG_POST_OPERATION_INFORMATION) Argument2;
            if (PostInfo->CallContext != CallbackCtx) {
                ErrorPrint("Unexpected CallContext value: 0x%p", PostInfo->CallContext);
            } else {
                InterlockedIncrement(&CallbackCtx->NotificationWithContextCount);
            }
            break;

        default:
            //
            // Do nothing for other notifications
            //
            break;
    }

    return Status;
}

