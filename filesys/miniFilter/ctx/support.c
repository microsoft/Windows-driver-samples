/*++

Copyright (c) 1999 - 2002  Microsoft Corporation

Module Name:

    operations.c

Abstract:

    This is the support routines module of the kernel mode filter driver implementing
    context sample.


Environment:

    Kernel mode


--*/



#include "pch.h"

//
//  Assign text sections for each routine.
//

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, CtxAllocateUnicodeString)
#pragma alloc_text(PAGE, CtxFreeUnicodeString)
#endif

//
//  Support Routines
//

_At_(String->Length, _Out_range_(==, 0))
_At_(String->MaximumLength, _In_)
_At_(String->Buffer, _Pre_maybenull_ _Post_notnull_ _Post_writable_byte_size_(String->MaximumLength))
NTSTATUS
CtxAllocateUnicodeString (
    _Out_ PUNICODE_STRING String
    )
/*++

Routine Description:

    This routine allocates a unicode string

Arguments:

    String - supplies the size of the string to be allocated in the MaximumLength field
             return the unicode string

Return Value:

    STATUS_SUCCESS                  - success
    STATUS_INSUFFICIENT_RESOURCES   - failure

--*/
{
    PAGED_CODE();

    String->Buffer = ExAllocatePoolZero( PagedPool,
                                         String->MaximumLength,
                                         CTX_STRING_TAG );

    if (String->Buffer == NULL) {

        DebugTrace( DEBUG_TRACE_ERROR,
                    ("[Ctx]: Failed to allocate unicode string of size 0x%x\n",
                    String->MaximumLength) );

        return STATUS_INSUFFICIENT_RESOURCES;
    }

    String->Length = 0;

    return STATUS_SUCCESS;
}

_At_(String->Length, _Out_range_(==, 0))
_At_(String->MaximumLength, _Out_range_(==, 0))
_At_(String->Buffer, _Pre_notnull_ _Post_null_)
VOID
CtxFreeUnicodeString (
    _Pre_notnull_ PUNICODE_STRING String
    )
/*++

Routine Description:

    This routine frees a unicode string

Arguments:

    String - supplies the string to be freed

Return Value:

    None

--*/
{
    PAGED_CODE();

    ExFreePoolWithTag( String->Buffer,
                       CTX_STRING_TAG );

    String->Length = String->MaximumLength = 0;
    String->Buffer = NULL;
}



