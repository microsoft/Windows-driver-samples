/*++

Module Name:

    tdriver.h

Abstract:

    This module declarations for the Ob/Ps callback test driver.


// Notice:
//
//    Use this sample code at your own risk; there is no support from Microsoft for the sample code.
//    In addition, this sample code is licensed to you under the terms of the Microsoft Public License
//    (http://www.microsoft.com/opensource/licenses.mspx)

--*/

#pragma once

#include "shared.h"

#define TD_CALLBACK_REGISTRATION_TAG  '0bCO' // TD_CALLBACK_REGISTRATION structure.
#define TD_CALL_CONTEXT_TAG           '1bCO' // TD_CALL_CONTEXT structure.


typedef struct _TD_CALLBACK_PARAMETERS {
    ACCESS_MASK AccessBitsToClear;
    ACCESS_MASK AccessBitsToSet;
}
TD_CALLBACK_PARAMETERS, *PTD_CALLBACK_PARAMETERS;

//
// TD_CALLBACK_REGISTRATION
//

typedef struct _TD_CALLBACK_REGISTRATION {

    //
    // Handle returned by ObRegisterCallbacks.
    //

    PVOID RegistrationHandle;

    //
    // If not NULL, filter only requests to open/duplicate handles to this
    // process (or one of its threads).
    //

    PVOID TargetProcess;
    HANDLE TargetProcessId;


    //
    // Currently each TD_CALLBACK_REGISTRATION has at most one process and one
    // thread callback. That is, we can't register more than one callback for
    // the same object type with a single ObRegisterCallbacks call.
    //

    TD_CALLBACK_PARAMETERS ProcessParams;
    TD_CALLBACK_PARAMETERS ThreadParams;

    ULONG RegistrationId;        // Index in the global TdCallbacks array.

}
TD_CALLBACK_REGISTRATION, *PTD_CALLBACK_REGISTRATION;

//
// TD_CALL_CONTEXT
//

typedef struct _TD_CALL_CONTEXT
{
    PTD_CALLBACK_REGISTRATION CallbackRegistration;

    OB_OPERATION Operation;
    PVOID Object;
    POBJECT_TYPE ObjectType;
}
TD_CALL_CONTEXT, *PTD_CALL_CONTEXT;

extern KGUARDED_MUTEX TdCallbacksMutex;

NTSTATUS TdDeleteCallback (
    _In_ ULONG RegistrationId
);

// delete the process/thead OB callbacks
NTSTATUS TdDeleteProtectNameCallback ();



NTSTATUS TdProtectNameCallback(
    _In_ PTD_PROTECTNAME_INPUT pProtectName
);

NTSTATUS TdCheckProcessMatch (
    _In_ PCUNICODE_STRING pustrCommand,
    _In_ PEPROCESS Process,
    _In_ HANDLE ProcessId
);

OB_PREOP_CALLBACK_STATUS
CBTdPreOperationCallback (
    _In_ PVOID RegistrationContext,
    _Inout_ POB_PRE_OPERATION_INFORMATION PreInfo
);

VOID
CBTdPostOperationCallback (
    _In_ PVOID RegistrationContext,
    _In_ POB_POST_OPERATION_INFORMATION PostInfo
);

VOID TdSetCallContext (
    _Inout_ POB_PRE_OPERATION_INFORMATION PreInfo,
    _In_ PTD_CALLBACK_REGISTRATION CallbackRegistration
);

VOID TdCheckAndFreeCallContext (
    _Inout_ POB_POST_OPERATION_INFORMATION PostInfo,
    _In_ PTD_CALLBACK_REGISTRATION CallbackRegistration
);

