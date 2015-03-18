/*++
Copyright (c) Microsoft Corporation.  All rights reserved.

    THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY
    KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
    IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR
    PURPOSE.

Module Name:

    TxR.c

Abstract: 

    Samples that show how to deal with transactional registry operations.

Environment:

    Kernel mode only

--*/

#include "regfltr.h"


BOOLEAN
TransactionEnlistSample(
    )
/*++

Routine Description:

    This sample shows how to enlist to a transaction that a registry operation
    is part of inorder to get notifications when it commits or aborts. 

Return Value:

    TRUE if the sample completed successfully.

--*/

{
    PCALLBACK_CONTEXT CallbackCtx = NULL;
    PRMCALLBACK_CONTEXT RMCallbackCtx = NULL;
    NTSTATUS Status;
    UNICODE_STRING Name;
    OBJECT_ATTRIBUTES KeyAttributes;
    OBJECT_ATTRIBUTES TxAttributes;
    HANDLE Key = NULL;
    HANDLE Transaction = NULL;
    BOOLEAN Success = FALSE;
    
    InfoPrint("");
    InfoPrint("=== Transaction Enlist Sample ====");

    if (!g_RMCreated) {
        ErrorPrint("Sample can't run because KTM data structures were not successfully created.");
        goto Exit;
    }

    //
    // Create the registry callback context and the transaction callback context.
    //
    
    CallbackCtx = CreateCallbackContext(CALLBACK_MODE_TRANSACTION_ENLIST,
                                         CALLBACK_ALTITUDE);
    if (CallbackCtx == NULL) {
        goto Exit;
    }

    RMCallbackCtx = (PRMCALLBACK_CONTEXT) ExAllocatePoolWithTag (
                        PagedPool, 
                        sizeof(RMCALLBACK_CONTEXT), 
                        REGFLTR_CONTEXT_POOL_TAG);
    if (RMCallbackCtx == NULL) {
        goto Exit;
    }
    RtlZeroMemory(RMCallbackCtx, sizeof(RMCALLBACK_CONTEXT));
    CallbackCtx->RMCallbackCtx = RMCallbackCtx;

    //
    // Create a transaction
    //

    InitializeObjectAttributes(&TxAttributes,
                               NULL,
                               OBJ_KERNEL_HANDLE,
                               NULL,
                               NULL);
    
    Status = ZwCreateTransaction(&Transaction,
                                 TRANSACTION_ALL_ACCESS,
                                 &TxAttributes,
                                 NULL,
                                 NULL,
                                 0,
                                 0,
                                 0,
                                 NULL,
                                 NULL);

    if (!NT_SUCCESS(Status)) {
        ErrorPrint("CreateTransaction failed. Status 0x%x", Status);
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
        Success = FALSE;
    }

    Success = TRUE;

    //
    // Create a key
    //

    RtlInitUnicodeString(&Name, KEY_NAME);
    InitializeObjectAttributes(&KeyAttributes,
                               &Name,
                               OBJ_CASE_INSENSITIVE | OBJ_KERNEL_HANDLE,
                               g_RootKey,
                               NULL);

    Status = ZwCreateKeyTransacted(&Key,
                                   KEY_ALL_ACCESS,
                                   &KeyAttributes,
                                   0,
                                   NULL,
                                   0,
                                   Transaction,
                                   NULL);

    if (!NT_SUCCESS(Status)) {
        ErrorPrint("ZwCreateKeyTransacted failed. Status 0x%x", Status);
        Success = FALSE;
    }

    //
    // Commit the transaction
    //

    Status = ZwCommitTransaction(Transaction, TRUE);
    
    if (!NT_SUCCESS(Status)) {
        ErrorPrint("ZwCommitTransaction failed. Status 0x%x", Status);
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
    // Check that the transaction callback context records a commit notification
    //

    if (RMCallbackCtx->Notification != TRANSACTION_NOTIFY_COMMIT) {
        ErrorPrint("RMContext notification mask is 0x%x instead of 0x%x.",
                   RMCallbackCtx->Notification,
                   TRANSACTION_NOTIFY_COMMIT);
        Success = FALSE;
    }

  Exit:
    
    //
    // Clean up
    //

    if (Transaction != NULL) {
        ZwClose(Transaction);
    }

    //
    // Need to reopen the key to delete it because the previous 
    // handle was part of a transaction that is now gone.
    //

    if (Key != NULL) {
        ZwClose(Key);
        Key = NULL;
    }

    RtlInitUnicodeString(&Name, KEY_NAME);
    InitializeObjectAttributes(&KeyAttributes,
                               &Name,
                               OBJ_CASE_INSENSITIVE | OBJ_KERNEL_HANDLE,
                               g_RootKey,
                               NULL);
    
    ZwOpenKey(&Key, KEY_ALL_ACCESS, &KeyAttributes); 
    if (Key != NULL) {
        ZwDeleteKey(Key);
        ZwClose(Key);
    }

    if (CallbackCtx != NULL) {
        ExFreePoolWithTag(CallbackCtx, REGFLTR_CONTEXT_POOL_TAG);
    }

    if (RMCallbackCtx != NULL) {
        ExFreePoolWithTag(RMCallbackCtx, REGFLTR_CONTEXT_POOL_TAG);
    }

    if (Success) {
        InfoPrint("Transaction Enlist Demo succeeded.");
    } else {
        ErrorPrint("Transaction Enlist Demo FAILED.");
    }

    return Success;
}


NTSTATUS 
CallbackTransactionEnlist(
    _In_ PCALLBACK_CONTEXT CallbackCtx,
    _In_ REG_NOTIFY_CLASS NotifyClass,
    _Inout_ PVOID Argument2
)
/*++

Routine Description:

    This helper callback routine shows hot to enlist on a transaction.

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
    PVOID Transaction = NULL;
    
    switch(NotifyClass) {

        case RegNtPreCreateKeyEx:

            PreCreateInfo = (PREG_CREATE_KEY_INFORMATION) Argument2;

            //
            // Get the transaction object
            //
            
            Transaction = PreCreateInfo->Transaction;
            if (Transaction == NULL) {

                //
                // Even if the transaction is not provided in the 
                // REG_Xxx_INFORMATION, we need to call CmGetBoundTransaction 
                // on the RootObject to check that it isn't associated with
                // a transaction.
                //

                Transaction = CmGetBoundTransaction(&CallbackCtx->Cookie,
                                                    PreCreateInfo->RootObject);

                if (Transaction == NULL) {
                    ErrorPrint("CreateKey is unexpectedly not transacted.");
                    break;
                }
            }

            //
            // Use the volatile RM created in CreateKTMResourceManager() 
            // to enlist in the transaction. We want notifications for
            // when the transaction commits or rolls back. 
            // 
            // Note: Make sure the callback routine handles all the 
            // notifications requested here. Look at RMCallback() to see
            // how to handle notifications.
            //

            Status = EnlistInTransaction(&CallbackCtx->RMCallbackCtx->Enlistment,
                                        ENLISTMENT_SUBORDINATE_RIGHTS,
                                        Transaction,
                                        TRANSACTION_NOTIFY_COMMIT |
                                        TRANSACTION_NOTIFY_ROLLBACK,
                                        CallbackCtx->RMCallbackCtx);
            
            if (!NT_SUCCESS(Status)) {
                ErrorPrint("EnlistInTransaction failed. Status 0x%x.", Status);
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
TransactionReplaySample(
    )
/*++

Routine Description:

    This sample shows how to copy a transactional create key operation.

Return Value:

    TRUE if the sample completed successfully.

--*/

{
    PCALLBACK_CONTEXT CallbackCtx = NULL;
    NTSTATUS Status;
    UNICODE_STRING Name;
    OBJECT_ATTRIBUTES KeyAttributes;
    OBJECT_ATTRIBUTES TxAttributes;
    HANDLE Key = NULL;
    HANDLE Transaction = NULL;
    HANDLE TransactedRoot = NULL;
    BOOLEAN bSuccess = FALSE;
    
    InfoPrint("");
    InfoPrint("=== Transaction Replay Sample ====");

    if (!g_RMCreated) {
        ErrorPrint("Sample can't run because KTM data structures were not successfully created.");
        goto Exit;
    }

    //
    // Create the callback context
    //

    CallbackCtx = CreateCallbackContext(CALLBACK_MODE_TRANSACTION_REPLAY,
                                         CALLBACK_ALTITUDE);
    if (CallbackCtx == NULL) {
        goto Exit;
    }
 
    //
    // Create a transaction
    //

    InitializeObjectAttributes(&TxAttributes,
                               NULL,
                               OBJ_KERNEL_HANDLE,
                               NULL,
                               NULL);

    Status = ZwCreateTransaction(&Transaction,
                                 TRANSACTION_ALL_ACCESS,
                                 &TxAttributes,
                                 NULL,
                                 NULL,
                                 0,
                                 0,
                                 0,
                                 NULL,
                                 NULL);

    if (!NT_SUCCESS(Status)) {
        ErrorPrint("CreateTransaction failed. Status 0x%x", Status);
        goto Exit;
    }

    //
    // Open a transacted handle to the root key
    //
    
    RtlInitUnicodeString(&Name, ROOT_KEY_ABS_PATH);
    InitializeObjectAttributes(&KeyAttributes,
                               &Name,
                               OBJ_CASE_INSENSITIVE | OBJ_KERNEL_HANDLE,
                               NULL,
                               NULL);
    
    Status = ZwOpenKeyTransacted(&TransactedRoot,
                                 KEY_ALL_ACCESS,
                                 &KeyAttributes,
                                 Transaction);
    if (!NT_SUCCESS(Status)) {
        ErrorPrint("ZwOpenKeyTransacted failed. Status 0x%x",Status);
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

    bSuccess = TRUE;
    
    //
    // Create a key using the transacted root key handle.
    //

    RtlInitUnicodeString(&Name, KEY_NAME);
    InitializeObjectAttributes(&KeyAttributes,
                               &Name,
                               OBJ_CASE_INSENSITIVE | OBJ_KERNEL_HANDLE,
                               TransactedRoot,
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
        bSuccess = FALSE;
    } 

    //
    // Unregister the callback
    //

    Status = CmUnRegisterCallback(CallbackCtx->Cookie);

    if (!NT_SUCCESS(Status)) {
        ErrorPrint("CmUnRegisterCallback failed. Status 0x%x", Status);
        bSuccess = FALSE;
    }


    //
    // Verify that the key created exists and that a key with the
    // "modified" name is also exists.
    //
    
    if (Key != NULL) {
        ZwClose(Key);
        Key = NULL;
    }

    Status = ZwOpenKey(&Key,
                       KEY_ALL_ACCESS,
                       &KeyAttributes);
    if (!NT_SUCCESS(Status)) {
        ErrorPrint("ZwCreateKey failed. Status 0x%x", Status);
        bSuccess = FALSE;
    } else {
        ZwClose(Key);
        Key = NULL;
    }
    
    RtlInitUnicodeString(&Name, MODIFIED_KEY_NAME);
    InitializeObjectAttributes(&KeyAttributes,
                               &Name,
                               OBJ_CASE_INSENSITIVE | OBJ_KERNEL_HANDLE,
                               TransactedRoot,
                               NULL);

    Status = ZwOpenKey(&Key,
                       KEY_ALL_ACCESS,
                       &KeyAttributes);

    if (!NT_SUCCESS(Status)) {
        ErrorPrint("ZwCreateKey failed. Status 0x%x", Status);
        bSuccess = FALSE;
    } else {
        ZwClose(Key);
        Key = NULL;
    }

    //
    // Roll back transaction
    //

    Status = ZwRollbackTransaction(Transaction, TRUE);
    
    if (!NT_SUCCESS(Status)) {
        ErrorPrint("ZwRollbackTransaction failed. Status 0x%x", Status);
        bSuccess = FALSE;
        goto Exit;
    }
    
    //
    // Check that both keys no longer exist. 
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
        ErrorPrint("ZwOpenKey returned unexpected status 0x%x. Expected 0x%x", 
                   Status,
                   STATUS_OBJECT_NAME_NOT_FOUND);
        bSuccess = FALSE;
    }

    if (Key != NULL) {
        ZwDeleteKey(Key);
        ZwClose(Key);
        Key = NULL;
    }

    RtlInitUnicodeString(&Name, MODIFIED_KEY_NAME);
    InitializeObjectAttributes(&KeyAttributes,
                               &Name,
                               OBJ_CASE_INSENSITIVE | OBJ_KERNEL_HANDLE,
                               g_RootKey,
                               NULL);
    Status = ZwOpenKey(&Key,
                       KEY_ALL_ACCESS,
                       &KeyAttributes);
    if (Status != STATUS_OBJECT_NAME_NOT_FOUND) {
        ErrorPrint("ZwOpenKey returned unexpected status 0x%x. Expected 0x%x", 
                   Status,
                   STATUS_OBJECT_NAME_NOT_FOUND);
        bSuccess = FALSE;
    }

  Exit:
    
    //
    // Clean up 
    //

    if (Key != NULL) {
        ZwDeleteKey(Key);
        ZwClose(Key);
    }

    if (TransactedRoot!= NULL) {
        ZwDeleteKey(TransactedRoot);
        ZwClose(TransactedRoot);
    }

    if (Transaction != NULL) {
        ZwClose(Transaction);
    }

    if (CallbackCtx != NULL) {
        ExFreePoolWithTag(CallbackCtx, REGFLTR_CONTEXT_POOL_TAG);
    }

    if (bSuccess) {
        InfoPrint("Transaction Replay Sample succeeded.");
    } else {
        ErrorPrint("Transaction Replay Sample FAILED.");
    }

    return bSuccess;
    
}



NTSTATUS 
CallbackTransactionReplay(
    _In_ PCALLBACK_CONTEXT CallbackCtx,
    _In_ REG_NOTIFY_CLASS NotifyClass,
    _Inout_ PVOID Argument2
    )
/*++

Routine Description:

    This helper callback routine shows how to get the transaction associated
    with a registry operation and shows how to do another operation in the 
    same transaction.
    
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
    HANDLE TransactionHandle = NULL;
    PVOID Transaction = NULL;
    OBJECT_ATTRIBUTES KeyAttributes;
    UNICODE_STRING Name;
    UNICODE_STRING LocalClass = {0};
    PUNICODE_STRING Class = NULL;
    HANDLE Key = NULL;
    HANDLE RootKey = NULL;
    KPROCESSOR_MODE Mode = KernelMode;

    switch(NotifyClass) {

        case RegNtPreCreateKeyEx:

            PreCreateInfo = (PREG_CREATE_KEY_INFORMATION) Argument2;

            //
            // Get the transaction object
            //

            Transaction = PreCreateInfo->Transaction;
            if (Transaction == NULL) {

                //
                // Even if the transaction is not provided in the 
                // REG_Xxx_INFORMATION, we need to call CmGetBoundTransaction 
                // on the RootObject to check that it isn't associated with
                // a transaction.
                //

                Transaction = CmGetBoundTransaction(&CallbackCtx->Cookie,
                                                    PreCreateInfo->RootObject);
                if (Transaction == NULL) {
                    ErrorPrint("CreateKey is unexpectedly not transacted.");
                    break;
                }
            }

            //
            // Get a handle to the transaction object
            //

            Status = ObOpenObjectByPointer(Transaction,
                                           OBJ_KERNEL_HANDLE,
                                           NULL,
                                           TRANSACTION_ALL_ACCESS,
                                           *TmTransactionObjectType,
                                           KernelMode,
                                           &TransactionHandle);

            if (!NT_SUCCESS (Status)) {
                ErrorPrint("ObReferenceObjectByPointer failed. Status 0x%x", Status);
                break;
            }

            // 
            // Next replay the create key using the transacted version of the
            // API and the transaction handle.
            //

            Status = ObOpenObjectByPointer(PreCreateInfo->RootObject,
                                           OBJ_KERNEL_HANDLE,
                                           NULL,
                                           KEY_ALL_ACCESS,  // Getting handle with all access
                                           PreCreateInfo->ObjectType,
                                           KernelMode,
                                           &RootKey);
            if (!NT_SUCCESS (Status)) {
                ErrorPrint("ObReferenceObjectByPointer failed. Status 0x%x", Status);
                break;
            }

    
            //
            // REG_CREATE_KEY_INFORMATION is a partially structure. The class
            // field's buffer is not captured. Since it is passed to 
            // ZwCreateKeyTransacted, it needs to be captured.
            //
            // *Note: in Windows 8 all fields are captured. See capture.c
            // for more details.
            //

            Mode = ExGetPreviousMode();

            if (!g_IsWin8OrGreater && Mode == UserMode) {
                Status = CaptureUnicodeString(&LocalClass, 
                                              PreCreateInfo->Class, 
                                              REGFLTR_CAPTURE_POOL_TAG);
                if (!NT_SUCCESS(Status)) {
                    break;
                }
                Class = &LocalClass;
                
            } else {
                Class = PreCreateInfo->Class;
            }

            
            RtlInitUnicodeString(&Name, MODIFIED_KEY_NAME);
            InitializeObjectAttributes(&KeyAttributes,
                                       &Name,
                                       OBJ_CASE_INSENSITIVE | OBJ_KERNEL_HANDLE,
                                       RootKey,
                                       PreCreateInfo->SecurityDescriptor);

            Status = ZwCreateKeyTransacted(&Key,
                                           KEY_ALL_ACCESS,
                                           &KeyAttributes,
                                           0,
                                           Class,
                                           PreCreateInfo->CreateOptions,
                                           TransactionHandle,
                                           PreCreateInfo->Disposition);

            ZwClose(RootKey);
            ZwClose(TransactionHandle);

            if (!NT_SUCCESS(Status)) {
                ErrorPrint("ZwCreateKeyTransacted failed. Status 0x%x.", Status);
                break;
            }

            ZwClose(Key);
            InfoPrint("\tCallback: Create key %wZ replayed in same transaction context.",
                      PreCreateInfo->CompleteName);
            Status = STATUS_SUCCESS;
            break;
            
        default:
            //
            // Do nothing for other notifications
            //
            break;
    }

    //
    // Free buffers used for capturing user mode values.
    //

    if (LocalClass.Buffer != NULL) {
        FreeCapturedUnicodeString(&LocalClass, REGFLTR_CAPTURE_POOL_TAG);
    }

    return Status;   
}

