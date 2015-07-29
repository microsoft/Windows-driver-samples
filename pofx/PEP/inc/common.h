/*++

Copyright (c) Microsoft Corporation.  All rights reserved.

    THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY
    KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
    IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR
    PURPOSE.

Module Name:

    common.h

Abstract:

    Header file that provide some utility functionalities to the sample device driver

Environment:

    Kernel mode

--*/

#pragma once

#pragma warning (push) 
#pragma warning(disable:4091)
#include <ntddk.h>
#pragma warning (pop)

#pragma warning(disable:4201)  // disable nameless struct/union warnings
#include <wdf.h>
#pragma warning(default:4201)

#define NTSTRSAFE_LIB
#include <ntstrsafe.h>

#ifndef MAX_USHORT
#define MAX_USHORT ((USHORT)-1)
#endif

#ifndef MAX_ULONG
#define MAX_ULONG ((ULONG)-1)
#endif

#ifndef MAX_ULONG64
#define MAX_ULONG64 ((ULONG64)-1)
#endif

//
// Useful macros for setting and checking flags.
//

#define SET_FLAGS(_x, _f)          ((_x) |= (_f))
#define CLEAR_FLAGS(_x, _f)        ((_x) &= ~(_f))
#define CLEAR_OTHER_FLAGS(_x, _f)  ((_x) &= (_f))
#define CHECK_FLAG(_x, _f)         ((_x) & (_f))

//
// Macros for rounding up or down.
//

#define ROUND_DOWN(_x, _alignment) \
    ((_alignment == 1) ? (_x) : (((_x) / (_alignment)) * (_alignment)))

#define ROUND_UP(_x, _alignment) \
    ROUND_DOWN((_x) + (_alignment) - 1, (_alignment))

//
// Macros for find minimum and maximum of two integers.
//

#define MIN(a,b) (((a) < (b)) ? (a) : (b))
#define MAX(a,b) (((a) < (b)) ? (b) : (a))

//
// Define macros to allow easy pointer arithmetic.
//

#define Add2Ptr(_Ptr, _Value) ((PVOID)((PUCHAR)(_Ptr) + (_Value)))
#define PtrOffset(_Base, _Ptr) ((ULONG_PTR)(_Ptr) - (ULONG_PTR)(_Base))

// 4127 -- Conditional Expression is Constant warning
#define WHILE(constant) \
__pragma(warning(disable: 4127)) while(constant) __pragma(warning(default: 4127))

#define FIELD_OFFSET_AND_SIZE(t, f) \
    (FIELD_OFFSET(t, f) + FIELD_SIZE(t, f))