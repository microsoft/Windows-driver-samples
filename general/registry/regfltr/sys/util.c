/*++
Copyright (c) Microsoft Corporation.  All rights reserved.

    THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY
    KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
    IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR
    PURPOSE.

Module Name:

    util.c

Abstract: 

    Utility routines for the sample driver.

Environment:

    Kernel mode only

--*/


#include "regfltr.h"


FAST_MUTEX g_CallbackCtxListLock;
LIST_ENTRY g_CallbackCtxListHead;
USHORT g_NumCallbackCtxListEntries;


ULONG 
ExceptionFilter (
    _In_ PEXCEPTION_POINTERS ExceptionPointers
    )
/*++

Routine Description:

    ExceptionFilter breaks into the debugger if an exception happens
    inside the callback.

Arguments:

    ExceptionPointers - unused

Return Value:

    Always returns EXCEPTION_CONTINUE_SEARCH

--*/
{

    ErrorPrint("Exception %lx, ExceptionPointers = %p", 
               ExceptionPointers->ExceptionRecord->ExceptionCode,
               ExceptionPointers);

    DbgBreakPoint();
    
    return EXCEPTION_EXECUTE_HANDLER;

}


PVOID
CreateCallbackContext(
    _In_ CALLBACK_MODE CallbackMode,
    _In_ PCWSTR AltitudeString
    ) 
/*++

Routine Description:

    Utility method to create a callback context. Callback context 
    should be freed using DeleteCallbackContext.
    
Arguments:

    CallbackMode - the callback mode value

    AltitudeString - a string with the altitude the callback will be 
        registered at

Return Value:

    Pointer to the allocated and initialized callback context 

--*/
{

    PCALLBACK_CONTEXT CallbackCtx = NULL;
    NTSTATUS Status;
    BOOLEAN Success = FALSE;
    
    CallbackCtx = (PCALLBACK_CONTEXT) ExAllocatePoolWithTag (
                        PagedPool, 
                        sizeof(CALLBACK_CONTEXT), 
                        REGFLTR_CONTEXT_POOL_TAG);

    if  (CallbackCtx == NULL) {
        ErrorPrint("CreateCallbackContext failed due to insufficient resources.");
        goto Exit;
    }
    
    RtlZeroMemory(CallbackCtx, sizeof(CALLBACK_CONTEXT));

    CallbackCtx->CallbackMode = CallbackMode;
    CallbackCtx->ProcessId = PsGetCurrentProcessId();

    Status = RtlStringCbPrintfW(CallbackCtx->AltitudeBuffer,
                                 MAX_ALTITUDE_BUFFER_LENGTH * sizeof(WCHAR),
                                 L"%s",
                                 AltitudeString);
    
    if (!NT_SUCCESS(Status)) {
        ErrorPrint("RtlStringCbPrintfW in CreateCallbackContext failed. Status 0x%x", Status);
        goto Exit;
    }
    
    RtlInitUnicodeString (&CallbackCtx->Altitude, CallbackCtx->AltitudeBuffer);

    Success = TRUE;

  Exit:

    if (Success == FALSE) {
        if (CallbackCtx != NULL) {
            ExFreePoolWithTag(CallbackCtx, REGFLTR_CONTEXT_POOL_TAG);
            CallbackCtx = NULL;
        }
    }

    return CallbackCtx;
    
}


BOOLEAN
InsertCallbackContext(
    _In_ PCALLBACK_CONTEXT CallbackCtx
    )
/*++

Routine Description:

    Utility method to insert the callback context into a list. 
    
Arguments:

    CallbackCtx - the callback context to insert

Return Value:

    TRUE if successful, FALSE otherwise

--*/
{

    BOOLEAN Success = FALSE;
    
    ExAcquireFastMutex(&g_CallbackCtxListLock);

    if (g_NumCallbackCtxListEntries < MAX_CALLBACK_CTX_ENTRIES) {
        g_NumCallbackCtxListEntries++;
        InsertHeadList(&g_CallbackCtxListHead, &CallbackCtx->CallbackCtxList);
        Success = TRUE;
    } else {
        ErrorPrint("Insert Callback Ctx failed: Max CallbackCtx entries reached.");
    }

    ExReleaseFastMutex(&g_CallbackCtxListLock);

    return Success;

}


PCALLBACK_CONTEXT
FindCallbackContext(
    _In_ LARGE_INTEGER Cookie
    )
/*++

Routine Description:

    Utility method to find a callback context using the cookie value.
    
Arguments:

    Cookie - the cookie value associated with the callback context. The
             cookie is returned when CmRegisterCallbackEx is called.

Return Value:

    Pointer to the found callback context

--*/
{
   
    PCALLBACK_CONTEXT CallbackCtx = NULL;
    PLIST_ENTRY Entry;
    
    ExAcquireFastMutex(&g_CallbackCtxListLock);

    Entry = g_CallbackCtxListHead.Flink;
    while (Entry != &g_CallbackCtxListHead) {

        CallbackCtx = CONTAINING_RECORD(Entry,
                                        CALLBACK_CONTEXT,
                                        CallbackCtxList);
        if (CallbackCtx->Cookie.QuadPart == Cookie.QuadPart) {
            break;
        }

        Entry = Entry->Flink;
    }

    ExReleaseFastMutex(&g_CallbackCtxListLock);

    if (CallbackCtx == NULL) {
        ErrorPrint("FindCallbackContext failed: No context with specified cookied was found.");
    }

    return CallbackCtx;
    
}

PCALLBACK_CONTEXT
FindAndRemoveCallbackContext(
    _In_ LARGE_INTEGER Cookie
    )
/*++

Routine Description:

    Utility method to find a callback context using the cookie value and then
    remove it.
    
Arguments:

    Cookie - the cookie value associated with the callback context. The
             cookie is returned when CmRegisterCallbackEx is called.

Return Value:

    Pointer to the found callback context

--*/
{
   
    PCALLBACK_CONTEXT CallbackCtx = NULL;
    PLIST_ENTRY Entry;
    
    ExAcquireFastMutex(&g_CallbackCtxListLock);

    Entry = g_CallbackCtxListHead.Flink;
    while (Entry != &g_CallbackCtxListHead) {

        CallbackCtx = CONTAINING_RECORD(Entry,
                                        CALLBACK_CONTEXT,
                                        CallbackCtxList);
        if (CallbackCtx->Cookie.QuadPart == Cookie.QuadPart) {
            RemoveEntryList(&CallbackCtx->CallbackCtxList);
            g_NumCallbackCtxListEntries--;
            break;
        }
    }

    ExReleaseFastMutex(&g_CallbackCtxListLock);

    if (CallbackCtx == NULL) {
        ErrorPrint("FindAndRemoveCallbackContext failed: No context with specified cookied was found.");
    }

    return CallbackCtx;    
}


VOID
DeleteCallbackContext(
    _In_ PCALLBACK_CONTEXT CallbackCtx
    )
/*++

Routine Description:

    Utility method to delete a callback context. 
    
Arguments:

    CallbackCtx - the callback context to insert

Return Value:

    None

--*/
{

    if (CallbackCtx != NULL) {
        ExFreePoolWithTag(CallbackCtx, REGFLTR_CONTEXT_POOL_TAG);
    }

}
