/*++
Copyright (c) Microsoft Corporation.  All rights reserved.

    THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY
    KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
    IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR
    PURPOSE.

Module Name:

    Version.c

Abstract: 

    Information and samples that describe:
        1. Changes in registry callback version 1.1
        2. How to use the version 1 REG_OPEN_KEY_INFORMATION and 
            REG_CREATE_KEY_INFORMATION structures
        3. Work arounds for issues in callback version 1.0.

Environment:

    Kernel mode only

--*/

#include "regfltr.h"


/*++

    Callback Version 1.1 is available in Windows 7 and Windows Server 2008 R2.
    It is NOT available on Vista or Windows Server 2008 as of Service Pack 2.
    

    Issues in callback version 1.0 that have been fixed in version 1.1:

    1. In the post-notification phase for a create or open key operation,
    the PostInfo->Object field might not be NULL even if the operation was
    unsuccessful as indicated by PostInfo->ReturnStatus. 

    This problem happens when there are multiple registry filter drivers 
    registered and one of the drivers blocks the operation in the 
    pre-notification phase by returning a nonsuccess status. Filter drivers
    that are at higher altitudes will receive a post-notification where 
    PostInfo->ReturnStatus is the nonsuccess status value but 
    PostInfo->Object will not be NULL. PostInfo->Object in this case will
    be equal to PostInfo->PreInfo->RootObject. 

    2. In version 1.0, an uncatched exception in a registry callback 
    routine will be swallowed by the system. In version 1.1 this has been 
    changed so an uncatched exception will cause the machine to bugcheck. 
    We provide a sample in this file (BugCheckSample) but obviously it is not 
    run.     

    NOTE: While bugchecking the system is not a good thing to do, we do not
    recommend putting your entire callback routine in one big try-except block
    and swallow legitimate exceptions like possible pool corruptions. Please
    keep what you wrap with a try-except block to the bare minimum.

--*/

/*++

    Version 1 of the create and open key REG_Xxx_INFORMATION structure is 
    available in Windows 7 and Windows Server 2008 R2. It is NOT available on 
    Vista or Windows Server 2008 as of Service Pack 2.

    NOTE: While Version 1 of the create and open key data structures will 
    likely be available on systems that have callback version 1.1, this 
    relationship is not guaranteed. You must check the create and open key
    data structure to see its version rather than depending on the 
    callback version. See CreateOpenV1Sample on how to check the version.


    Issues addressed by version 1 of the create and open key data structures:

    1. Without the Attributes field provided in the V1 create and open 
    REG_Xxx_INFORMATION structure, there is no way to exactly replicate 
    certain create and open operations. See CreateOpenV1Sample for a 
    demonstration. Unfortunately there is no work around for this issue.

    2. The PreInfo->CompleteName and PreInfo->RootObject fields in a 
    create or open key operation do not behave as expected when the key to be
    opened or created is represented as an absolute path. 
    REG_CREATE_KEY_INFORMATION_V1 and REG_OPEN_KEY_INFORMATION_V1 contain a 
    field PreInfoV1->RemainingName which addresses this issue. 

    Example:

        Open operation on this key: \REGISTRY\MACHINE\Software\_RegFltrRoot
        
        One way of relatively opening the key is to open "Software\_RegFltrRoot"
        relative to \REGISTRY\MACHINE. 

            RegOpenKeyEx(HKEY_LOCAL_MACHINE,
                         "Software\\_RegFltrRoot",
                         0,
                         KEY_ALL_ACCESS,
                         &Key);

        In this case the value of the fields in REG_OPEN_KEY_INFORMATION_V1
        would be:

            RootObject - Handle to the key \REGISTRY\MACHINE
            CompleteName - "Software\\_RegFltrRoot"
            RemainingName - "Software\\_RegFltrRoot"


        If the open uses an absolute path,

            RtlInitUnicodeString(&Name, L"\\REGISTRY\\MACHINE\\Software\\_RegFltrRoot")
            InitializeObjectAttributes(&KeyAttributes,
                                       &Name,
                                       OBJ_CASE_INSENSITIVE | OBJ_KERNEL_HANDLE,
                                       NULL,
                                       NULL);

             ZwOpenKey(&Key, KEY_ALL_ACCESS, &KeyAttributes);        

        the value of the fields in REG_OPEN_KEY_INFORMATION_V1 would be:

            RootObject - Handle to the key \REGISTRY
            CompleteName - "\\REGISTRY\\MACHINE\\Software\\_RegFltrRoot"
            RemainingName - "MACHINE\\Software\\_RegFltrRoot"
        
        Note that RootObject is not NULL even though CompleteName holds the 
        absolute path to the key.


    The work around for this on systems without REG_OPEN_KEY_INFORMATION_V1 is 
    to check if the first character of CompleteName is a '\'. If that is the 
    case you can be sure that CompleteName is holding an absolute path to the 
    key.

--*/



VOID
BugCheckSample(
    )
/*++

Routine Description:

    In version 1.1, if a registry filter driver's callback routine throws
    an exception the registry will bugcheck the machine:

        REGISTRY_FILTER_DRIVER_EXCEPTION  (0x135)
        This bugcheck is caused by an unhandled exception in a registry 
        filtering driver.

        PARAMETERS
                1 - ExceptionCode
                2 - Address of the context record for the exception that caused 
                    the bugcheck
                3 - The driver's callback routine address
                4 - Internal

        DESCRIPTION
        This bugcheck indicates that a registry filtering driver didn't handle 
        exception inside its notification routine. One can identify the driver 
        by the 3rd parameter.

    In version 1.0, an exception in the callback routine is simply swallowed and
    ignored.

    This sample uses a simple callback routine that will access NULL to throw 
    an exception. The sample is not normally run and is only here for 
    demonstration purposes.

Return Value:

    TRUE if the sample completed successfully.

--*/
{
    PCALLBACK_CONTEXT CallbackCtx = NULL;
    NTSTATUS Status;
    OBJECT_ATTRIBUTES KeyAttributes;
    UNICODE_STRING Name;
    HANDLE Key = NULL;

    InfoPrint("");
    InfoPrint("=== Bugcheck Sample ====");

    //
    // Create the callback context
    //

    CallbackCtx = CreateCallbackContext(CALLBACK_MODE_VERSION_BUGCHECK, 
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

    //
    // Do an open key just to invoke the callback. 
    // In version 1.1 this will bugcheck and nothing else will run.
    // In version 1.0 the open will simply fail as expected.
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
        ErrorPrint("ZwOpenKey returned unexpected status 0x%x", Status);
    }

    //
    // Unregister the callback
    //

    Status = CmUnRegisterCallback(CallbackCtx->Cookie);

    if (!NT_SUCCESS(Status)) {
        ErrorPrint("CmUnRegisterCallback failed. Status 0x%x", Status);
    }

  Exit:
    
    //
    // Clean up
    //

    if (Key != NULL) {
        ZwDeleteKey(Key);
        ZwClose(Key);
    }

    if (CallbackCtx != NULL) {
        ExFreePoolWithTag(CallbackCtx, REGFLTR_CONTEXT_POOL_TAG);
    }

    return;
    
}


NTSTATUS 
CallbackBugcheck(
    _In_ PCALLBACK_CONTEXT CallbackCtx,
    _In_ REG_NOTIFY_CLASS NotifyClass,
    _Inout_ PVOID Argument2
    )
/*++

Routine Description:

    This helper callback routine throws an exception by dereferencing a 
    null pointer.

    NOTE: While bugchecking the system is not a good thing to do, we do not
    recommend putting your entire callback routine in one big try-except block
    and swallow legitimate exceptions like possible pool corruptions. Please
    keep what you wrap with a try-except block to the bare minimum.

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
    PULONG NullPointer = NULL;

    UNREFERENCED_PARAMETER(CallbackCtx);
    UNREFERENCED_PARAMETER(NotifyClass);
    UNREFERENCED_PARAMETER(Argument2);


    InfoPrint("\tCallback is about to throw an exception.");
    if (g_MajorVersion == 1 && g_MinorVersion == 0) {
        InfoPrint("\tException will be swallowed by registry");
    } else {
        ErrorPrint("Exception will cause machine to bugcheck");
    }


    #pragma prefast(suppress: 6011, "Sample is purposefully dereferencing a null pointer.");
    *NullPointer = 0;
    
    return Status;
}


BOOLEAN
CreateOpenV1Sample(
    )
/*++

Routine Description:

    This sample shows how the information in the Attributes field of 
    the REG_OPEN_KEY_INFORMATION_V1 data structure can change the outcome of 
    a registry operation. Without the Attributes field, it is impossible for 
    the callback routine to accuratel replay certain registry operations.

    A special key is used in this sample which has security set on it to 
    protect it from being deleted: 

        \REGISTRY\MACHINE\SYSTEM\CurrentControlSet\Enum

    In the sample we try to open this key with DELETE access. Normally this
    will work if we do it in kernel mode since the system bypasses all
    access checks on handles created in kernel mode. However here we set the 
    OBJ_FORCE_ACCESS_CHECK flag which tells the system to perform all access 
    checks on the handle.

    In the callback routine associated with this sample, we will replay the
    open operation with and without the flag to show how the presence of 
    the attributes information can change the outcome of the operation.

Return Value:

    TRUE if the sample completed successfully.

--*/
{
    PCALLBACK_CONTEXT CallbackCtx = NULL;
    NTSTATUS Status;
    OBJECT_ATTRIBUTES KeyAttributes;
    UNICODE_STRING Name;
    WCHAR NameBuffer[] = L"\\registry\\machine\\system\\currentcontrolset\\enum";
    HANDLE Key = NULL;
    BOOLEAN Success = FALSE;

    InfoPrint("");
    InfoPrint("=== Create/Open V1 Sample ====");

    //
    // Create the callback context
    //

    CallbackCtx = CreateCallbackContext(CALLBACK_MODE_VERSION_CREATE_OPEN_V1, 
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
    // Try to open the special key with delete access but have the 
    // OBJ_FORCE_ACCESS_CHECK flag in object attributes. This operation     
    // should fail with access denied.
    //
    
    RtlInitUnicodeString(&Name, NameBuffer);
    InitializeObjectAttributes(&KeyAttributes,
                               &Name,
                               OBJ_CASE_INSENSITIVE | OBJ_KERNEL_HANDLE |
                               OBJ_FORCE_ACCESS_CHECK,
                               NULL,
                               NULL);

    Status = ZwOpenKey(&Key,
                       DELETE,
                       &KeyAttributes);

    if (Status != STATUS_ACCESS_DENIED) {
        ErrorPrint("ZwOpenKey returned unexpected status 0x%x", Status);
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
        ZwClose(Key);
    }

    if (CallbackCtx != NULL) {
        ExFreePoolWithTag(CallbackCtx, REGFLTR_CONTEXT_POOL_TAG);
    }

    if (Success) {
        InfoPrint("Create/Open V1 sample succeeded.");
    } else {
        ErrorPrint("Create/Open V1 sample failed.");
    }

    return Success;
    
}


NTSTATUS
CallbackCreateOpenV1(
    _In_ PCALLBACK_CONTEXT CallbackCtx,
    _In_ REG_NOTIFY_CLASS NotifyClass,
    _Inout_ PVOID Argument2
    )
/*++

Routine Description:

    This helper callback routine will show how to check whether the system
    supports version 1 of the REG_OPEN_KEY_INFORMATION structure. 

    If version 1 is supported, the callback routine will replay the open 
    operation during the pre-notification phase with and without the 
    Attributes field found in REG_OPEN_KEY_INFORMATION_V1 to show how the
    outcome is different.

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
    PREG_OPEN_KEY_INFORMATION_V1 PreOpenInfo;
    OBJECT_ATTRIBUTES KeyAttributes;
    HANDLE Key = NULL;
    HANDLE RootKey = NULL;


    UNREFERENCED_PARAMETER(CallbackCtx);

    //
    // Check for the pre-notification phase of a create operation
    //
    
    if (NotifyClass != RegNtPreOpenKeyEx) {
        goto Exit;
    }

    PreOpenInfo = (PREG_OPEN_KEY_INFORMATION_V1) Argument2;

    //
    // Check if version 1 is available on this system. If not,
    // simply return success.
    //
    
    InfoPrint("\tREG_OPEN_KEY_INFORMATION structure's version is 0x%p", 
              (PVOID)PreOpenInfo->Version);

    if ((ULONG_PTR) PreOpenInfo->Version != 1) {
        InfoPrint("Create/Open v1 sample is only for version 1");
        goto Exit;
    }

    //
    // Open a handle to the root object
    //

    Status = ObOpenObjectByPointer(PreOpenInfo->RootObject,
                                   OBJ_KERNEL_HANDLE,
                                   NULL,
                                   KEY_ALL_ACCESS, 
                                   PreOpenInfo->ObjectType,
                                   KernelMode,
                                   &RootKey);

    if (!NT_SUCCESS(Status)) {
        ErrorPrint("ObObjectByPointer failed. Status 0x%x", Status);
        goto Exit;
    }

    //
    // Do the open with the same attributes as in the original call. This
    // includes the OBJ_FORCE_ACCESS_CHECK flag which will cause the open
    // operation to fail.
    //
    // Note: The openkey operation might have originated from user mode so the 
    // OBJ_KERNEL_HANDLE flag needs to be explicitly added to the attributes
    // to prevent user mode handle spoofing attacks.
    //
    

    InitializeObjectAttributes(&KeyAttributes,
                               PreOpenInfo->RemainingName,
                               PreOpenInfo->Attributes | OBJ_KERNEL_HANDLE,
                               RootKey,
                               PreOpenInfo->SecurityDescriptor);

    Status = ZwOpenKey(&Key,
                       PreOpenInfo->DesiredAccess,
                       &KeyAttributes);

    if (NT_SUCCESS(Status)) {
        ZwClose(Key);
        Key = NULL;
    }

    if (Status != STATUS_ACCESS_DENIED) {
        ErrorPrint("ZwOpenKey with attributes returned unexpected status 0x%x", Status);
        Status = STATUS_UNSUCCESSFUL;
        goto Exit;
    }

    //
    // Do the open without the attributes in the original call. The open
    // operation will succeed now because the access checks will not be 
    // performed.
    //

    InitializeObjectAttributes(&KeyAttributes,
                               PreOpenInfo->RemainingName,
                               OBJ_KERNEL_HANDLE | OBJ_CASE_INSENSITIVE,
                               RootKey,
                               PreOpenInfo->SecurityDescriptor);

    Status = ZwOpenKey(&Key,
                       PreOpenInfo->DesiredAccess,
                       &KeyAttributes);

    if (!NT_SUCCESS(Status)) {
        ErrorPrint("ZwOpenKey without attributes returned unexpected status 0x%x", Status);
        goto Exit;
    }

    Status = STATUS_SUCCESS;


  Exit:

    if (Key != NULL) {
        ZwClose(Key);
    }

    if (RootKey != NULL) {
        ZwClose(RootKey);
    }

    return Status;
}    
 
