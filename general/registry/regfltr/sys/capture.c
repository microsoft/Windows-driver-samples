/*++
Copyright (c) Microsoft Corporation.  All rights reserved.

    THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY
    KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
    IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR
    PURPOSE.

Module Name:

    capture.c

Abstract: 

    This file contains 
        1. Documentation for what parameters need to be captured
        2. A simple sample showing how to capture the parameters for
           SetValueKey and DeleteValueKey operations.
        3. Helper routines for capturing buffers and UNICODE_STRINGs.

Environment:

    Kernel mode only

--*/


#include "regfltr.h"


/*++

    Probing and Capturing Parameters

    This section defines how registry filtering drivers should handle input 
    parameters.


    I. Probed Parameters
    
    All members in related registry structures (e.g. REG_CREATE_KEY_INFORMATION, 
    REG_SAVE_KEY_INFORMATION) and all structures and buffers pointed to by
    their buffers are already probed by registry. Parameters are probed only 
    when ExGetPreviousMode indicates that the previous mode was user mode. If 
    the previous mode is kernel mode (i.e. KernelMode. is returned by 
    ExGetPreviousMode), all parameters are considered valid and no probing is 
    done in this case.

    
    II. Captured parameters
    
    Most but not all members in related registry structures are captured. 
    If the previous mode is kernel mode, all parameters are considered valid 
    and no capturing is done in this case. If the previous mode is user mode, 
    all parameters are probed but not necessarily captured. Since this impacts 
    how registry filtering drivers should handle input parameters, we outline 
    below exactly which parameters are captured. It is important to note that 
    some members currently probed but not captured by the OS may be captured 
    in the future. On the other hand registry filtering drivers can rely on 
    the fact that parameters currently captured will be captured in all future 
    OS releases. 

    * Note: As of Windows 8, all structures except output buffers are fully 
    captured.
    
        1. Fully captured structures:

        Note: A fully captured structure is a captured structure where all 
        structures and buffers pointed to by its members are also fully 
        captured. 
        
        For example: the UNICODE_STRING structure pointed to by the 
        CompleteName member in the fully captured REG_CREATE_KEY_INFORMATION 
        structure is also captured (including the Buffer member of the 
        UNICODE_STRING).

        List of fully captured structures (in Windows 7):
        
            REG_CREATE_KEY_INFORMATION (** incorrect)
            REG_CREATE_KEY_INFORMATION_V1 (** incorrect)
            REG_DELETE_KEY_INFORMATION
            REG_FLUSH_KEY_INFORMATION
            REG_KEY_HANDLE_CLOSE_INFORMATION
            REG_OPEN_KEY_INFORMATION
            REG_OPEN_KEY_INFORMATION_V1
            REG_QUERY_KEY_SECURITY_INFORMATION (** added)
            REG_REPLACE_KEY_INFORMATION
            REG_RESTORE_KEY_INFORMATION
            REG_SAVE_KEY_INFORMATION
            REG_SET_KEY_SECURITY_INFORMATION.
            REG_UNLOAD_KEY_INFORMATION

            ** There were incorrect entries in this list that are now
            corrected. REG_CREATE_KEY_INFORMATION and 
            REG_CREATE_KEY_INFORMATION_V1 are not fully captured and should
            be on the partially captured structures. 
            REG_QUERY_KEY_SECURITY_INFORMATION has now been added to the list
            of fully captured structures.

        2. Partially captured structures 

        Note: This section has been modified for clarity in the Windows 8 
        release of the WDK. However this information only applies to Windows 7
        since in Windows 8 all input buffers and structures are captured.
        
        This section defines which members are captured in the partially 
        captured structures.

        Notes: 

            A. CallContext, ObjectContext members are not consumed by registry 
               and are not treated as probed or captured by this document.
            B. Integer types (e.g. enum, int, …) are considered captured in the 
               structure and are not explicitly listed below.
            C. The Object member is always captured and it's not explicitly 
               listed below.
            D. Output buffers are probed but not captured

        List of partially captured structures and the members that are NOT
        captured:

            REG_CREATE_KEY_INFORMATION: Class->Buffer
            REG_CREATE_KEY_INFORMATION_V1: Class->Buffer
            REG_DELETE_VALUE_KEY_INFORMATION: ValueName->Buffer
            REG_LOAD_KEY_INFORMATION: KeyName->Buffer
            REG_QUERY_VALUE_KEY_INFORMATION: ValueName->Buffer
            REG_QUERY_MULTIPLE_VALUE_KEY_INFORMATION: BufferLength
            REG_RENAME_KEY_INFORMATION: NewName->Buffer
            REG_SET_VALUE_KEY_INFORMATION: Data


        All other REG_Xxx_INFORMATION structures do not have fields that 
        require capturing other than those mentioned in notes B and C. 
        
        Example:

        REG_ENUMERATE_KEY_INFORMATION: 
            Object: captured, see (C) above
            Index: captured, see (B) above
            KeyInformationClass : captured, see (B) above 
            KeyInformation: only probed, see (D) above
            Length: captured, see (B) above
            ResultLength: only probed, see (D) above
            CallContext, ObjectContext: unknown, see (A) above
            Reserved: currently undefined

        List of these structures:

            REG_CALLBACK_CONTEXT_CLEANUP_INFORMATION
            REG_DELETE_KEY_INFORMATION
            REG_ENUMERATE_KEY_INFORMATION
            REG_KEY_HANDLE_CLOSE_INFORMATION 
            REG_QUERY_KEY_INFORMATION
            REG_QUERY_KEY_SECURITY_INFORMATION 
            REG_REPLACE_KEY_INFORMATION 
            REG_RESTORE_KEY_INFORMATION 
            REG_SAVE_KEY_INFORMATION 
            REG_SET_INFORMATION_KEY_INFORMATION
            REG_SET_KEY_SECURITY_INFORMATION 
            REG_UNLOAD_KEY_INFORMATION


    III. Handling Registry Filtering Parameters

    Registry filtering drivers must handle input parameters correctly. If the 
    previous mode is user mode and the driver needs to use a parameter, it must 
    depending on the scenario either wrap every access with a try-except 
    construct or capture the parameter. If the driver wants to call a Zw 
    registry API or any other kernel mode Zw API, it must ensure that all the 
    arguments passed to the call are captured because these APIs will not 
    probe or capture their inputs if the call originated from kernel mode. 

    There is no need for drivers to probe any of the parameters as the registry 
    has already probed them. If the driver uses the OS probe APIs to probe a 
    parameter that has already been captured by the registry, it will throw an 
    exception.

    Note: Special handling should be provided for NULL buffers. In some cases 
    such buffer might be considered valid even for kernel mode.

--*/


NTSTATUS 
CallbackCapture(
    _In_ PCALLBACK_CONTEXT CallbackCtx,
    _In_ REG_NOTIFY_CLASS NotifyClass,
    _Inout_ PVOID Argument2
)
/*++

Routine Description:

    This helper callback routine shows how to capture a buffer and a
    unicode string with the name of a value. The bulk of the work is down
    in the helper capture routines: CaptureBuffer and CaptureUnicodeString.

    In the pre-notification phase, we bypass the set value and delete value
    operations and complete them manually by calling ZwSetValueKey and 
    ZwDeleteValueKey. 
    
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
    PREG_SET_VALUE_KEY_INFORMATION PreSetValueInfo;
    PREG_DELETE_VALUE_KEY_INFORMATION PreDeleteValueInfo;
    HANDLE RootKey = NULL;
    PVOID LocalData = NULL;
    PVOID Data = NULL;
    UNICODE_STRING LocalValueName = {0};
    PUNICODE_STRING ValueName = NULL;
    KPROCESSOR_MODE Mode = KernelMode;
    
    UNREFERENCED_PARAMETER(CallbackCtx);
    
    switch(NotifyClass) {
        
        case RegNtPreSetValueKey:

            PreSetValueInfo = (PREG_SET_VALUE_KEY_INFORMATION) Argument2;

            //
            // REG_SET_VALUE_KEY_INFORMATION is a partially captured structure.
            // The value name is captured but the data is not. Since we are
            // passing the data to a zw* method, we need to capture it.
            //
            // *Note: as of win8, the data buffer is captured as well
            // by the registry.
            //
            
            Mode = ExGetPreviousMode();

            if (!g_IsWin8OrGreater && (Mode == UserMode)) {
                Status = CaptureBuffer(&LocalData, 
                                       PreSetValueInfo->Data, 
                                       PreSetValueInfo->DataSize, 
                                       REGFLTR_CAPTURE_POOL_TAG);
                if (!NT_SUCCESS(Status)) {
                    break;
                }
                Data = LocalData;
            } else {
                Data = PreSetValueInfo->Data;
            }

            //
            // Get a handle to the root key the value is being created under.
            // This is in PreInfo->Object.
            //
            
            Status = ObOpenObjectByPointer(PreSetValueInfo->Object,
                                           OBJ_KERNEL_HANDLE,
                                           NULL,
                                           KEY_ALL_ACCESS,
                                           NULL,
                                           KernelMode,
                                           &RootKey);

            if (!NT_SUCCESS (Status)) {
                ErrorPrint("ObObjectByPointer failed. Status 0x%x", Status);
                break;
            } 

            //
            // Set the value.
            //
            
            Status = ZwSetValueKey(RootKey,
                                   PreSetValueInfo->ValueName,
                                   0,
                                   PreSetValueInfo->Type,
                                   Data,
                                   PreSetValueInfo->DataSize);
            
            if(!NT_SUCCESS(Status)) {
                ErrorPrint("ZwSetValue in CallbackModify failed. Status 0x%x", 
                           Status);
                ZwClose(RootKey);
                break;
            }

            //
            // Finally return STATUS_CALLBACK_BYPASS to tell the registry
            // not to proceed with the original registry operation and to return
            // STATUS_SUCCESS to the caller.
            //

            InfoPrint("\tCallback: Set value %wZ bypassed.", PreSetValueInfo->ValueName);
            Status = STATUS_CALLBACK_BYPASS;
            ZwClose(RootKey);
            break;
           
        case RegNtPreDeleteValueKey:

            PreDeleteValueInfo = (PREG_DELETE_VALUE_KEY_INFORMATION) Argument2;

            //
            // REG_DELETE_VALUE_KEY_INFORMATION is a partially captured 
            // structure. The value name's buffer is not captured. Since we are
            // passing the name to a zw* method, we need to capture it.
            //
            // *Note: as of Win8, the data buffer is captured already
            // by the registry.
            //
            
            Mode = ExGetPreviousMode();

            if (!g_IsWin8OrGreater && (Mode == UserMode)) {
                Status = CaptureUnicodeString(&LocalValueName, 
                                              PreDeleteValueInfo->ValueName,
                                              REGFLTR_CAPTURE_POOL_TAG);
                if (!NT_SUCCESS(Status)) {
                    break;
                }
                ValueName = &LocalValueName;
            } else {
                ValueName = PreDeleteValueInfo->ValueName;
            }

            //
            // Get a handle to the root key the value is being created under.
            // This is in PreInfo->Object.
            //
            
            Status = ObOpenObjectByPointer(PreDeleteValueInfo->Object,
                                           OBJ_KERNEL_HANDLE,
                                           NULL,
                                           KEY_ALL_ACCESS,
                                           NULL,
                                           KernelMode,
                                           &RootKey);

            if (!NT_SUCCESS (Status)) {
                ErrorPrint("ObObjectByPointer failed. Status 0x%x", Status);
                break;
            } 

            //
            // Set the value.
            //
            
            Status = ZwDeleteValueKey(RootKey,
                                      ValueName);
            
            if(!NT_SUCCESS(Status)) {
                ErrorPrint("ZwDeleteValue failed. Status 0x%x", 
                           Status);
                ZwClose(RootKey);
                break;
            }

            //
            // Finally return STATUS_CALLBACK_BYPASS to tell the registry
            // not to proceed with the original registry operation and to return
            // STATUS_SUCCESS to the caller.
            //

            InfoPrint("\tCallback: Delete value %S bypassed.", ValueName->Buffer);
            Status = STATUS_CALLBACK_BYPASS;
            ZwClose(RootKey);
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
    
    if (LocalData != NULL){
        FreeCapturedBuffer(LocalData, REGFLTR_CAPTURE_POOL_TAG);
    }

    if (LocalValueName.Buffer != NULL) {
        FreeCapturedUnicodeString(&LocalValueName, REGFLTR_CAPTURE_POOL_TAG);
    }

    return Status;
}




NTSTATUS
CaptureBuffer(
    _Outptr_result_maybenull_ PVOID *CapturedBuffer,
    _In_reads_bytes_(Length) PVOID Buffer,
    _In_ SIZE_T Length,
    _In_ ULONG PoolTag
    )
/*++

Routine Description:

    Captures a buffer using allocations with the specified pool tag. Captured
    buffer should be freed using FreeCapturedBuffer.

Arguments:

    CapturedBuffer - pointer to a variable that receives the location of the
        captured buffer.

    Buffer - the buffer to capture

    Length - Length of Buffer

    PoolTag - pool tag
    
Return Value:

    NTSTATUS

--*/
{
    NTSTATUS Status = STATUS_SUCCESS;
    PVOID TempBuffer = NULL;

    NT_ASSERT(CapturedBuffer != NULL);

    if (Length == 0) {
        *CapturedBuffer = NULL;
        return Status;
    }

    TempBuffer = (PCALLBACK_CONTEXT) ExAllocatePoolZero(
                        PagedPool,
                        Length,
                        PoolTag);

    //
    // It's a good practice to keep the contents of a try-except block to 
    // the bare minimum. By keeping the pool allocation call outside of the 
    // try-except block we don't mask possible pool corruptions.
    //
    
    if (TempBuffer != NULL) {
        try {
            RtlCopyMemory(TempBuffer, Buffer, Length);
        } except (ExceptionFilter(GetExceptionInformation())) {
            ErrorPrint("Capturing buffer failed with exception");
            ExFreePoolWithTag(TempBuffer, PoolTag);
            TempBuffer = NULL;
            Status = GetExceptionCode();
        }
    } else {
        ErrorPrint("Capturing buffer failed wtih insufficient resources");
        Status = STATUS_INSUFFICIENT_RESOURCES;
    }

    *CapturedBuffer = TempBuffer;
    
    return Status;
 }


VOID
FreeCapturedBuffer(
    _In_ PVOID CapturedBuffer,
    _In_ ULONG PoolTag
    ) 
/*++

Routine Description:

    Frees a captured buffer.

Arguments:

    CapturedBuffer - captured buffer

    PoolTag - pool tag

--*/
{
    if (CapturedBuffer != NULL) {
        ExFreePoolWithTag(CapturedBuffer, PoolTag);
    }
}


NTSTATUS
CaptureUnicodeString(
    _Inout_ UNICODE_STRING *DestString,
    _In_ PCUNICODE_STRING SourceString,
    _In_ ULONG PoolTag
    )
/*++

Routine Description:

    Captures a unicode string. The buffer is captured based on SourceString's
    Length field with the addition of sizeof(WCHAR) bytes for a NULL to 
    signal the end of the string.

    Use FreeCapturedUnicodeString to free the captured string.

Arguments:

    DestString - Pointer to the unicode string that will receive the 
        captured buffer.

    SourceString - Pointer tot he unicode string to be captured.

    PoolTag - pool tag

Return Value:

    NTSTATUS

--*/
{
    NTSTATUS Status = STATUS_SUCCESS;


    if (SourceString->Length == 0) {
        DestString->Length = 0;
        DestString->Buffer = NULL;
        DestString->MaximumLength = 0;
        return Status;
    }

    //
    // Only SourceString->Length should be checked. The registry does not
    // validate SourceString->MaximumLength.
    //
    // An additional sizeof(WCHAR) bytes are added to the buffer size since
    // SourceString->Length does not include the NULL at the end of the string.
    //

    DestString->Length = SourceString->Length;
    DestString->MaximumLength = SourceString->Length + sizeof(WCHAR);

    DestString->Buffer = (PWSTR) ExAllocatePoolZero(
                            PagedPool,
                            DestString->MaximumLength,
                            PoolTag);
    
    if (DestString->Buffer != NULL) {

        //
        // It's a good practice to keep the contents of a try-except block to 
        // the bare minimum. By keeping the pool allocation call outside of the 
        // try-except block we don't mask possible pool corruptions.
        //

        try {
            RtlCopyMemory(DestString->Buffer, 
                          SourceString->Buffer, 
                          SourceString->Length);
        } except (ExceptionFilter(GetExceptionInformation())) {
            ErrorPrint("Capturing Unicode String failed with exception");
            ExFreePoolWithTag(DestString->Buffer, PoolTag);
            DestString->Buffer = NULL;
            Status = GetExceptionCode();
        }

    } else {
        ErrorPrint("Capturing Unicode String failed wtih insufficient resources");
        Status = STATUS_INSUFFICIENT_RESOURCES;
    }

    if (DestString->Buffer == NULL) {
        DestString->Length = 0;
        DestString->MaximumLength = 0;
    }

    return Status;

}


VOID
FreeCapturedUnicodeString(
    _In_ UNICODE_STRING *String,
    _In_ ULONG PoolTag
    )
/*++

Routine Description:

    Frees a captured buffer.

Arguments:

    CapturedBuffer - captured buffer

    PoolTag - pool tag

--*/
{
    if (String->Length != 0) {
        String->Length = 0;
        String->MaximumLength = 0;
        FreeCapturedBuffer(String->Buffer, PoolTag);
        String->Buffer = NULL;
    }
}
