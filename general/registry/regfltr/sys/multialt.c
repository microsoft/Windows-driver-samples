/*++
Copyright (c) Microsoft Corporation.  All rights reserved.

    THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY
    KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
    IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR
    PURPOSE.

Module Name:

    MultiAlt.c

Abstract: 

    Samples that feature multiple callbacks registered at different
    altitudes and show what notifications they do and don't recieve.

Environment:

    Kernel mode only

--*/

#include "regfltr.h"


BOOLEAN 
MultipleAltitudeBlockDuringPreSample(
    )
/*++

Routine Description:

    This sample features a stack of three callbacks at different altitudes and
    demonstrates what happens when middle callback blocks an operation
    in the pre-notification phase. 

Return Value:

    TRUE if the sample completed successfully.

--*/
{

    PCALLBACK_CONTEXT CallbackCtxHigh = NULL;
    PCALLBACK_CONTEXT CallbackCtxMid = NULL;
    PCALLBACK_CONTEXT CallbackCtxLow = NULL;
    NTSTATUS Status;
    OBJECT_ATTRIBUTES KeyAttributes;
    UNICODE_STRING Name;
    HANDLE Key = NULL;
    BOOLEAN Success = FALSE;

    InfoPrint("");
    InfoPrint("=== Multiple Altitude Block During Pre Sample ====");

    //
    // Create callback contexts for the 3 callbacks.
    // The high and low callbacks will only monitor how many notifications
    // they receive.
    //
    
    CallbackCtxHigh = CreateCallbackContext(CALLBACK_MODE_MULTIPLE_ALTITUDE_MONITOR,
                                            CALLBACK_HIGH_ALTITUDE);
    CallbackCtxMid = CreateCallbackContext(CALLBACK_MODE_MULTIPLE_ALTITUDE_BLOCK_DURING_PRE,
                                           CALLBACK_ALTITUDE);
    CallbackCtxLow = CreateCallbackContext(CALLBACK_MODE_MULTIPLE_ALTITUDE_MONITOR,
                                           CALLBACK_LOW_ALTITUDE);

    if ((CallbackCtxHigh == NULL) ||
        (CallbackCtxMid == NULL) ||
        (CallbackCtxLow == NULL)) {
        goto Exit;
    }

    //
    // Register the callbacks
    //

    Status = CmRegisterCallbackEx(Callback,
                                  &CallbackCtxHigh->Altitude,
                                  g_DeviceObj->DriverObject,
                                  (PVOID) CallbackCtxHigh,
                                  &CallbackCtxHigh->Cookie, 
                                  NULL);
    if (!NT_SUCCESS(Status)) {
        ErrorPrint("CmRegisterCallback failed. Status 0x%x", Status);
        goto Exit;
    }

    Status = CmRegisterCallbackEx(Callback,
                                  &CallbackCtxMid->Altitude,
                                  g_DeviceObj->DriverObject,
                                  (PVOID) CallbackCtxMid,
                                  &CallbackCtxMid->Cookie, 
                                  NULL);
    if (!NT_SUCCESS(Status)) {
        ErrorPrint("CmRegisterCallback failed. Status 0x%x", Status);
        goto Exit;
    }

    Status = CmRegisterCallbackEx(Callback,
                                  &CallbackCtxLow->Altitude,
                                  g_DeviceObj->DriverObject,
                                  (PVOID) CallbackCtxLow,
                                  &CallbackCtxLow->Cookie, 
                                  NULL);
    if (!NT_SUCCESS(Status)) {
        ErrorPrint("CmRegisterCallback failed. Status 0x%x", Status);
        goto Exit;
    }

    Success = TRUE;

    //
    // Do a create key operation which will be blocked by the middle
    // callback and fail with STATUS_ACCESS_DENIED
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

    if (Status != STATUS_ACCESS_DENIED) {
        ErrorPrint("ZwCreateKey returned unexpected status 0x%x", Status);
        Success = FALSE;
    }

    
    //
    // Unregister the callbacks
    //

    Status = CmUnRegisterCallback(CallbackCtxHigh->Cookie);

    if (!NT_SUCCESS(Status)) {
        ErrorPrint("CmUnRegisterCallback failed. Status 0x%x", Status);
    }

    Status = CmUnRegisterCallback(CallbackCtxMid->Cookie);

    if (!NT_SUCCESS(Status)) {
        ErrorPrint("CmUnRegisterCallback failed. Status 0x%x", Status);
    }

    Status = CmUnRegisterCallback(CallbackCtxLow->Cookie);

    if (!NT_SUCCESS(Status)) {
        ErrorPrint("CmUnRegisterCallback failed. Status 0x%x", Status);
    }


    //
    // Verify that the highest alitude callback receives a pre and a post 
    // notification. It receives a post notification because it returned 
    // STATUS_SUCCESS in the pre-notification so it is guaranteed to get a 
    // post notification.
    //
    
    if ((CallbackCtxHigh->PreNotificationCount != 1) ||
        (CallbackCtxHigh->PostNotificationCount != 1)) {
        ErrorPrint("High Callback should have seen 1 pre and 1 post notifications.");
        ErrorPrint("High Callback actually saw %d pre and %d post notifications.",
                   CallbackCtxHigh->PreNotificationCount,
                   CallbackCtxHigh->PostNotificationCount);
        Success = FALSE;
    }

    //
    // Verify the middle callback receives only a pre notification.
    // It does not get a post notification because it return a non-success
    // value in the pre-notification phase.
    //
    
    if ((CallbackCtxMid->PreNotificationCount != 1) ||
        (CallbackCtxMid->PostNotificationCount != 0)) {
        ErrorPrint("Mid Callback should have seen 1 pre and 0 post notifications.");
        ErrorPrint("Mid Callback actually saw %d pre and %d post notifications.",
                   CallbackCtxMid->PreNotificationCount,
                   CallbackCtxMid->PostNotificationCount);
        Success = FALSE;
    }

    //
    // Verify the lowest callback receives no notifications.
    // Once the middle callback blocks, no callbacks at lower altitudes are
    // notified.
    //
    
    if ((CallbackCtxLow->PreNotificationCount != 0) ||
        (CallbackCtxLow->PostNotificationCount != 0)) {
        ErrorPrint("Low Callback should have seen 0 pre and 0 post notifications.");
        ErrorPrint("Low Callback actually saw %d pre and %d post notifications.",
                   CallbackCtxLow->PreNotificationCount,
                   CallbackCtxLow->PostNotificationCount);
        Success = FALSE;
    }

  Exit:

    if (Success) {
        InfoPrint("Multiple Altitude Block During Pre Sample succeeded.");
    } else {
        ErrorPrint("Multiple Altitude Block During Pre Sample FAILED.");
    }

    //
    // Clean up
    //

    if (Key != NULL) {
        ZwDeleteKey(Key);
        ZwClose(Key);
    }

    if (CallbackCtxHigh != NULL) {
        ExFreePoolWithTag(CallbackCtxHigh, REGFLTR_CONTEXT_POOL_TAG);
    }

    if (CallbackCtxMid != NULL) {
        ExFreePoolWithTag(CallbackCtxMid, REGFLTR_CONTEXT_POOL_TAG);
    }

    if (CallbackCtxLow != NULL) {
        ExFreePoolWithTag(CallbackCtxLow, REGFLTR_CONTEXT_POOL_TAG);
    }

    return Success;
}


BOOLEAN 
MultipleAltitudeInternalInvocationSample(
    )
/*++

Routine Description:

    This sample features a stack of 3 callbacks at different altitudes and
    demonstrates what happens when the middle callback invokes a registry
    operation. 

Return Value:

    TRUE if the sample completed successfully.

--*/
{
    PCALLBACK_CONTEXT CallbackCtxHigh = NULL;
    PCALLBACK_CONTEXT CallbackCtxMid = NULL;
    PCALLBACK_CONTEXT CallbackCtxLow = NULL;
    NTSTATUS Status;
    OBJECT_ATTRIBUTES KeyAttributes;
    UNICODE_STRING Name;
    HANDLE Key = NULL;
    BOOLEAN Success = FALSE;
    

    InfoPrint("");
    InfoPrint("=== Multiple Altitude Internal Invocation Sample ====");

    //
    // Create callback contexts for the 3 callbacks.
    // The high and low callbacks will only monitor how many notifications
    // they receive.
    //
    
    CallbackCtxHigh = CreateCallbackContext(CALLBACK_MODE_MULTIPLE_ALTITUDE_MONITOR,
                                            CALLBACK_HIGH_ALTITUDE);
    CallbackCtxMid = CreateCallbackContext(CALLBACK_MODE_MULTIPLE_ALTITUDE_INTERNAL_INVOCATION,
                                           CALLBACK_ALTITUDE);
    CallbackCtxLow = CreateCallbackContext(CALLBACK_MODE_MULTIPLE_ALTITUDE_MONITOR,
                                           CALLBACK_LOW_ALTITUDE);

    if ((CallbackCtxHigh == NULL) ||
        (CallbackCtxMid == NULL) ||
        (CallbackCtxLow == NULL)) {
        goto Exit;
    }

    //
    // Register the callbacks
    //

    Status = CmRegisterCallbackEx(Callback,
                                  &CallbackCtxHigh->Altitude,
                                  g_DeviceObj->DriverObject,
                                  (PVOID) CallbackCtxHigh,
                                  &CallbackCtxHigh->Cookie, 
                                  NULL);
    if (!NT_SUCCESS(Status)) {
        ErrorPrint("CmRegisterCallback failed. Status 0x%x", Status);
        goto Exit;
    }

    Status = CmRegisterCallbackEx(Callback,
                                  &CallbackCtxMid->Altitude,
                                  g_DeviceObj->DriverObject,
                                  (PVOID) CallbackCtxMid,
                                  &CallbackCtxMid->Cookie, 
                                  NULL);
    if (!NT_SUCCESS(Status)) {
        ErrorPrint("CmRegisterCallback failed. Status 0x%x", Status);
        goto Exit;
    }

    Status = CmRegisterCallbackEx(Callback,
                                  &CallbackCtxLow->Altitude,
                                  g_DeviceObj->DriverObject,
                                  (PVOID) CallbackCtxLow,
                                  &CallbackCtxLow->Cookie, 
                                  NULL);
    if (!NT_SUCCESS(Status)) {
        ErrorPrint("CmRegisterCallback failed. Status 0x%x", Status);
        goto Exit;
    }

    Success = TRUE;

    //
    // Create a key. When the middle callback receives the pre-notification
    // and the post-notification for this create it will perform an open key 
    // and a close key operation.
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
        ErrorPrint("ZwCreateKey returned unexpected status 0x%x", Status);
        Success = FALSE;
    }

    //
    // Unregister the callbacks
    //

    Status = CmUnRegisterCallback(CallbackCtxHigh->Cookie);

    if (!NT_SUCCESS(Status)) {
        ErrorPrint("CmUnRegisterCallback failed. Status 0x%x", Status);
        Success = FALSE;
    }

    Status = CmUnRegisterCallback(CallbackCtxMid->Cookie);

    if (!NT_SUCCESS(Status)) {
        ErrorPrint("CmUnRegisterCallback failed. Status 0x%x", Status);
        Success = FALSE;
    }

    Status = CmUnRegisterCallback(CallbackCtxLow->Cookie);

    if (!NT_SUCCESS(Status)) {
        ErrorPrint("CmUnRegisterCallback failed. Status 0x%x", Status);
        Success = FALSE;
    }


    //
    // Verify the highest altitude callback receives one pre and one post
    // notification. This callback does not get notifications for the 
    // registry operations called by the middle callback.
    //
    
    if ((CallbackCtxHigh->PreNotificationCount != 1) ||
        (CallbackCtxHigh->PostNotificationCount != 1)) {
        ErrorPrint("High Callback should have seen 1 pre and 1 post notifications.");
        ErrorPrint("High Callback actually saw %d pre and %d post notifications.",
                   CallbackCtxHigh->PreNotificationCount,
                   CallbackCtxHigh->PostNotificationCount);
        Success = FALSE;
    }

    //
    // Verify the middle callback receives one pre and one post notification.
    // This callback does not get notifications for the registry operations 
    // that it calls.
    //
    
    if ((CallbackCtxMid->PreNotificationCount != 1) ||
        (CallbackCtxMid->PostNotificationCount != 1)) {
        ErrorPrint("Mid Callback should have seen 1 pre and 1 post notifications.");
        ErrorPrint("Mid Callback actually saw %d pre and %d post notifications.",
                   CallbackCtxMid->PreNotificationCount,
                   CallbackCtxMid->PostNotificationCount);
        Success = FALSE;
    }

    //
    // Verify the lowest callback receives 5 pre-notifications and 5
    // post-notifications. This callback receives 1 pre and 1 post from the 
    // original create key operation. It also receives 2 pre and 2 post for
    // the open key and close key operations called by the middle callback 
    // during the pre phase of the create key and then 2 pre and 2 post again
    // for the calls in the post phase of the create key.
    //
    
    if ((CallbackCtxLow->PreNotificationCount != 5) ||
        (CallbackCtxLow->PostNotificationCount != 5)) {
        ErrorPrint("Low Callback should have seen 5 pre and 5 post notifications.");
        ErrorPrint("Low Callback actually saw %d pre and %d post notifications.",
                   CallbackCtxLow->PreNotificationCount,
                   CallbackCtxLow->PostNotificationCount);
        Success = FALSE;
    }

  Exit:
    
    if (Success) {
        InfoPrint("Multiple Altitude Internal Invocation Sample succeeded.");
    } else {
        ErrorPrint("Multiple Altitude Internal Invocation Sample FAILED.");
    }

    //
    // Clean up
    //

    if (Key != NULL) {
        ZwDeleteKey(Key);
        ZwClose(Key);
    }

    if (CallbackCtxHigh != NULL) {
        ExFreePoolWithTag(CallbackCtxHigh, REGFLTR_CONTEXT_POOL_TAG);
    }

    if (CallbackCtxMid != NULL) {
        ExFreePoolWithTag(CallbackCtxMid, REGFLTR_CONTEXT_POOL_TAG);
    }

    if (CallbackCtxLow != NULL) {
        ExFreePoolWithTag(CallbackCtxLow, REGFLTR_CONTEXT_POOL_TAG);
    }

    return Success;
}


NTSTATUS 
CallbackMonitor(
    _In_ PCALLBACK_CONTEXT CallbackCtx,
    _In_ REG_NOTIFY_CLASS NotifyClass,
    _Inout_ PVOID Argument2
    )
/*++

Routine Description:

    This helper callback routine just monitors how many pre and post registry 
    operations it receives and records it in the callback context. 

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
    UNREFERENCED_PARAMETER(Argument2);
    
    switch(NotifyClass) {
        case RegNtPreDeleteKey:
        case RegNtPreSetValueKey:
        case RegNtPreDeleteValueKey:
        case RegNtPreSetInformationKey:
        case RegNtPreRenameKey:
        case RegNtPreEnumerateKey:
        case RegNtPreEnumerateValueKey:
        case RegNtPreQueryKey:
        case RegNtPreQueryValueKey:
        case RegNtPreQueryMultipleValueKey:
        case RegNtPreKeyHandleClose:
        case RegNtPreCreateKeyEx:
        case RegNtPreOpenKeyEx:
        case RegNtPreFlushKey:
        case RegNtPreLoadKey:
        case RegNtPreUnLoadKey:
        case RegNtPreQueryKeySecurity:
        case RegNtPreSetKeySecurity:
        case RegNtPreRestoreKey:
        case RegNtPreSaveKey:
        case RegNtPreReplaceKey:
            InterlockedIncrement(&CallbackCtx->PreNotificationCount);
            break;
        case RegNtPostDeleteKey:
        case RegNtPostSetValueKey:
        case RegNtPostDeleteValueKey:
        case RegNtPostSetInformationKey:
        case RegNtPostRenameKey:
        case RegNtPostEnumerateKey:
        case RegNtPostEnumerateValueKey:
        case RegNtPostQueryKey:
        case RegNtPostQueryValueKey:
        case RegNtPostQueryMultipleValueKey:
        case RegNtPostKeyHandleClose:
        case RegNtPostCreateKeyEx:
        case RegNtPostOpenKeyEx:
        case RegNtPostFlushKey:
        case RegNtPostLoadKey:
        case RegNtPostUnLoadKey:
        case RegNtPostQueryKeySecurity:
        case RegNtPostSetKeySecurity:
        case RegNtPostRestoreKey:
        case RegNtPostSaveKey:
        case RegNtPostReplaceKey:
            InterlockedIncrement(&CallbackCtx->PostNotificationCount);
            break;
    }

    return STATUS_SUCCESS;
    
}


NTSTATUS 
CallbackMultipleAltitude(
    _In_ PCALLBACK_CONTEXT CallbackCtx,
    _In_ REG_NOTIFY_CLASS NotifyClass,
    _Inout_ PVOID Argument2
    )
/*++

Routine Description:

    This helper callback routine first calls CallbackMonitor to record the
    number of pre and post notifications received by the callback. Then it
    does one of two things depending on the callback mode specified in the 
    callback context.

    If callback mode is CALLBACK_MODE_MULTIPLE_ALTITUDE_BLOCK_DURING_PRE:
    Return STATUS_ACCESS_DENIED when we receive a pre-notification for a
    create key operation. 

    If callback mode is CALLBACK_MODE_MULTIPLE_ALTITUDE_INTERNAL_INVOCATION:
    Call ZwOpenKey and ZwCloseKey when we receive a pre or post notification
    for a create key operation.
  
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
    PVOID Object = NULL;
    HANDLE ObjectHandle = NULL;
    PCUNICODE_STRING ObjectName = NULL;
    UNICODE_STRING CapturedObjectName = {0};
    OBJECT_ATTRIBUTES KeyAttributes = {0};
    PREG_POST_OPERATION_INFORMATION PostInfo;

    CallbackMonitor(CallbackCtx, NotifyClass, Argument2);
    
    if(CallbackCtx->CallbackMode == CALLBACK_MODE_MULTIPLE_ALTITUDE_BLOCK_DURING_PRE) {
        switch(NotifyClass) {
            case RegNtPreSetValueKey:
            case RegNtPreCreateKeyEx:
                InfoPrint("\tCallback: CreateKey/SetValueKey blocked.");
                Status = STATUS_ACCESS_DENIED;
                break;
            default:
                //
                // Do nothing for other notifications
                //
                break;
        }
    }

    if (CallbackCtx->CallbackMode == CALLBACK_MODE_MULTIPLE_ALTITUDE_INTERNAL_INVOCATION) {

        //
        // Get the Object that we will open and close.
        //

        switch(NotifyClass) {
            case RegNtPreCreateKeyEx:
                Object = ((PREG_CREATE_KEY_INFORMATION) Argument2)->RootObject;

                //
                // RootObject should never be NULL.
                //
                
                ASSERT(Object != NULL);

                break;

            case RegNtPostCreateKeyEx:

                PostInfo = (PREG_POST_OPERATION_INFORMATION) Argument2; 

                //
                // Make sure the operation is successful so far.
                //

                if (!NT_SUCCESS(PostInfo->Status)) {
                    ErrorPrint("Post notification status is unexpectedly 0x%x.",
                               PostInfo->Status);
                    break;
                }

                //
                // If the operation si successful so far, PostInfo->Object should
                // not be NULL. However, a misbehaving registry filter driver
                // can make this NULL so we do not ASSERT here as we do in the
                // pre-notification case.
                //
                
                Object = PostInfo->Object;
                if (Object == NULL) {
                    ErrorPrint("PostInfo->Object is unexpectedly null in RegNtPostCreateKeyEx.");
                    ErrorPrint("PostInfo->Status is 0x%x",
                               PostInfo->Status);
                }
                
                break;
                
            default:
                //
                // Do nothing for other notifications
                //
                break;
        }


        if (Object != NULL) {

            //
            // Use CmCallbackGetKeyObjectID to get the absolute path to Object.
            //

#if (NTDDI_VERSION >= NTDDI_WIN8)

            //
            // In Windows 8, CmCallbackGetKeyObjectIDEx was added to give
            // developers a copy of the object name rather than the actual
            // object name string. This is a safer programming approach and
            // allows the system to safely clean up the old object name in 
            // operations like renaming the key.
            //
            // Call CmCallbackReleaseKeyObjectIDEx to release the object name 
            // returned by CmCallbackGetKeyObjectIDEx.
            //

            Status = CmCallbackGetKeyObjectIDEx(&CallbackCtx->Cookie,
                                               Object,
                                               NULL,
                                               &ObjectName,
                                               0);           // Flag: reserved for future

            if (!NT_SUCCESS (Status)) {
                ErrorPrint("CmCallbackGetKeyObjectIDEx failed. Status 0x%x", Status);
                goto Exit;
            } 

            CapturedObjectName.Length = ObjectName->Length;
            CapturedObjectName.MaximumLength = ObjectName->MaximumLength;
            CapturedObjectName.Buffer = ObjectName->Buffer;

#else

            Status = CmCallbackGetKeyObjectID(&CallbackCtx->Cookie,
                                              Object,
                                              NULL,
                                              &ObjectName);

            if (!NT_SUCCESS (Status)) {
                ErrorPrint("CmCallbackGetKeyObjectID failed. Status 0x%x", Status);
                goto Exit;
            } 

            //
            // The UNICODE_STRING referenced by ObjectName from 
            // CmCallbackGetKeyObjectID must not be changed. If you need to
            // modify the string, create a copy. 
            // 
            // Although this sample does not change the path, we show the
            // code to capture the string for demonstration purposes.
            //
            
            Status = CaptureUnicodeString(&CapturedObjectName, ObjectName, 
                                          REGFLTR_CAPTURE_POOL_TAG);

            if (!NT_SUCCESS(Status)) {
                goto Exit;
            }

#endif //NTDDI_VERSION >= NTDDI_WIN8


            InitializeObjectAttributes(&KeyAttributes,
                                       &CapturedObjectName,
                                       OBJ_CASE_INSENSITIVE | OBJ_KERNEL_HANDLE,
                                       NULL,
                                       NULL);
            
            InfoPrint("\tCallback: Internal Invocation of ZwOpenKey");                
            Status = ZwOpenKey(&ObjectHandle,
                               KEY_ALL_ACCESS,
                               &KeyAttributes);
            if (!NT_SUCCESS (Status)) {
                ErrorPrint("ZwOpenKey failed. Status 0x%x", Status);
            } else {
                InfoPrint("\tCallback: Internal Invocation of ZwCloseKey");
                ZwClose(ObjectHandle);
            }

#if (NTDDI_VERSION >= NTDDI_WIN8)

            CmCallbackReleaseKeyObjectIDEx(ObjectName);

#else

            FreeCapturedUnicodeString(&CapturedObjectName, REGFLTR_CAPTURE_POOL_TAG);

#endif //NTDDI_VERSION >= NTDDI_WIN8

        }
    }

 Exit:
    
    return Status;
}


