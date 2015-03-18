/*++
Copyright (c) Microsoft Corporation.  All rights reserved.

    THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY
    KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
    IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR
    PURPOSE.

Module Name:

    txrutil.c

Abstract: 

    Utility functions for working with transaction registry operations

Environment:

    Kernel mode only

--*/

#include "regfltr.h"


static HANDLE ResourceManager = NULL;
static HANDLE TransactionManager = NULL;


NTSTATUS
CreateKTMResourceManager(
    _In_ PTM_RM_NOTIFICATION CallbackRoutine,
    _In_opt_ PVOID RMKey
    )
/*++

Routine Description:

    This method will create a volatile Transaction Manager (TM) and a volatile
    Resource Manager (RM) and enable callback notification through them.
    The RM created here is used to enlist onto a transaction so that the 
    RMCallback routine will be called when the transaction commits or aborts. 

Arguments:

    CallbackRoutine - Pointer to a ResourceManagerNotification Routine

    RMKey - A caller-defined context value that uniquely identifies the 
            resource manager. The callback routine receives this value as 
            input.
            Note: When you are enlisting to a transaction, you can pass in a 
            context that is specific to that particular enlistment.

Return Value:

    NTSTATUS

--*/
{
    OBJECT_ATTRIBUTES ObjAttributes;
    PKRESOURCEMANAGER RMObject;
    NTSTATUS Status = STATUS_SUCCESS;
    HANDLE TMHandle = NULL;
    HANDLE RMHandle = NULL;
    GUID RMGuid;

    InfoPrint("Creating KTM Resource Manager");

    //
    // Create the volatile TM
    //
    
    InitializeObjectAttributes(&ObjAttributes,
                               NULL,
                               OBJ_KERNEL_HANDLE,
                               NULL,
                               NULL);

    Status = ZwCreateTransactionManager(&TMHandle,
                                        TRANSACTIONMANAGER_ALL_ACCESS,
                                        &ObjAttributes,
                                        NULL,
                                        TRANSACTION_MANAGER_VOLATILE,
                                        0);

    if (!NT_SUCCESS(Status)) {
        ErrorPrint("CreateTransactionManager failed. Status 0x%x", Status);
        goto Exit;
    }

    //
    // Create the volatile RM
    //
    
    Status = ExUuidCreate(&RMGuid);

    if (!NT_SUCCESS(Status)) {
        ErrorPrint("ExUuidCreate failed. Status 0x%x", Status);
        goto Exit;
    }

    InitializeObjectAttributes(&ObjAttributes,
                               NULL,
                               OBJ_KERNEL_HANDLE,
                               NULL,
                               NULL);

    Status = ZwCreateResourceManager(&RMHandle,
                                     RESOURCEMANAGER_ALL_ACCESS,
                                     TMHandle,
                                     &RMGuid,
                                     &ObjAttributes,

                                     RESOURCE_MANAGER_VOLATILE,
                                     NULL);

    if (!NT_SUCCESS(Status)) {
        ErrorPrint("CreateResourceManager failed. Status 0x%x", Status);
        goto Exit;
    }

    //
    // Grab the RM object from the handle
    //

    Status = ObReferenceObjectByHandle(RMHandle,
                                       0,
                                       NULL,
                                       KernelMode,
                                       (PVOID *) &RMObject,
                                       NULL);
    
    if (!NT_SUCCESS(Status)) {
        ErrorPrint("ObReferenceObjectbyHandle failed. Status 0x%x", Status);
        goto Exit;
    }

    //
    // Enable callbacks and pass in our notification routine
    //
    
    Status = TmEnableCallbacks(RMObject,
                               CallbackRoutine,
                               RMKey);

    ObDereferenceObject(RMObject);
    
    if (!NT_SUCCESS(Status)) {
        ErrorPrint("TmEnableCallbacks failed. Status 0x%x", Status);
        goto Exit;
    }

  Exit:

    if (!NT_SUCCESS(Status)) {
        if (RMHandle != NULL) {
            ZwClose(RMHandle);
        }
        if (TMHandle!= NULL) {
            ZwClose(TMHandle);
        }
    } else {
        ResourceManager = RMHandle;
        TransactionManager = TMHandle;
    }
    
    return Status;    
}



NTSTATUS
EnlistInTransaction(
    _Out_ PHANDLE EnlistmentHandle,
    _In_ ACCESS_MASK DesiredAccess,
    _In_ PVOID Transaction,
    _In_ NOTIFICATION_MASK NotificationMask,
    _In_opt_ PVOID EnlistmentKey
    )
/*++

Routine Description:

    This method is a wrapper around ZwCreateEnlistment. It outputs a handle
    to the enlistment object which represent's a resource manager's 
    enlistment to a transaction. Enlisting to a transaction allows the 
    resource manager to receive notifications about a transaction's events.

Arguments:

    EnlistmentHandle - Pointer to variable that receives the handle to the 
                       new enlistment object.

    DesiredAccess - Specifies the requested access to the enlistment object.

    Transaction - Transaction object

    NotificationMask - A bitwise OR of TRANSACTION_NOTIFY_Xxx values defined 
                       in Ktmtypes.h. It specifies the types of transaction
                       notifications that KTM will send to the caller.
    
    EnlistmentKey - A caller-defined context value that uniquely identifies the 
                    enlistment. The callback routine registered when callbacks
                    were enabled in the resource manager receives this value.

Return Value:

    NTSTATUS

--*/
{

    NTSTATUS Status;
    HANDLE TransactionHandle = NULL;  
    OBJECT_ATTRIBUTES ObjAttributes;

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

    if (!NT_SUCCESS(Status)) {
        ErrorPrint("ObOpenObjectByPointer failed. Status 0x%x.", Status);
        return Status;
    }
    
    //
    // Use the transaction handle and the volatile RM created in
    // CreateKTMResourceManager() to enlist to the transaction.
    //
    
    InitializeObjectAttributes(&ObjAttributes,
                               NULL,
                               OBJ_KERNEL_HANDLE,
                               NULL,
                               NULL);

    Status = ZwCreateEnlistment(EnlistmentHandle,
                                DesiredAccess,
                                ResourceManager,
                                TransactionHandle,
                                &ObjAttributes,
                                0,
                                NotificationMask,
                                EnlistmentKey);

    ZwClose(TransactionHandle);
    if (!NT_SUCCESS(Status)) {
        ErrorPrint("ZwCreateEnlistment failed. Status 0x%x", Status);
    }
    
    return Status;

}


VOID
DeleteKTMResourceManager(
    )
/*++

Routine Description:

    Clean up any resources associated wtih the resource manager.

--*/
{
    if (ResourceManager != NULL) {
        ZwClose(ResourceManager);
        ResourceManager = NULL;
    }
    if (TransactionManager != NULL) {
        ZwClose(TransactionManager);
        TransactionManager = NULL;
    }
}
