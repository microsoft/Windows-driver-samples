// Copyright (C) Microsoft Corporation. All rights reserved.

#pragma once

#include <wil/common.h>

#ifdef _KERNEL_MODE

// nullptr_t is normally automatically defined by the CRT headers, but it
// doesn't get included by kernel code.
namespace std { typedef decltype(__nullptr) nullptr_t; }
using ::std::nullptr_t;

// The stddef.h used for kernel code has the old offsetof macro.
// Let's use the new one instead.
#undef offsetof
#define offsetof(s,m) __builtin_offsetof(s,m)

#endif // _KERNEL_MODE

#define BEGIN_MACRO do {
#define END_MACRO } while (0)

#ifdef _KERNEL_MODE
#define CODE_SEG(segment) __declspec(code_seg(segment))
#else
#define CODE_SEG(segment)
#endif

#ifndef KRTL_PAGE_SEGMENT
#  define KRTL_PAGE_SEGMENT "PAGE"
#endif
#ifndef KRTL_INIT_SEGMENT
#  define KRTL_INIT_SEGMENT "INIT"
#endif
#ifndef KRTL_NONPAGED_SEGMENT
#  define KRTL_NONPAGED_SEGMENT ".text"
#endif

/// Use on pageable functions.
#define PAGED CODE_SEG(KRTL_PAGE_SEGMENT) _IRQL_always_function_max_(PASSIVE_LEVEL)

/// Use on pageable functions, where you don't want the SAL IRQL annotation to say PASSIVE_LEVEL.
#define PAGEDX CODE_SEG(KRTL_PAGE_SEGMENT)

/// Use on code in the INIT segment.  (Code is discarded after DriverEntry returns.)
#define INITCODE CODE_SEG(KRTL_INIT_SEGMENT)

/// Use on code that must always be locked in memory.
#define NONPAGED CODE_SEG(KRTL_NONPAGED_SEGMENT) _IRQL_requires_max_(DISPATCH_LEVEL)

/// Use on code that must always be locked in memory, where you don't want SAL IRQL annotations.
#define NONPAGEDX CODE_SEG(KRTL_NONPAGED_SEGMENT)

#ifndef _KERNEL_MODE

#ifndef PAGED_CODE
#define PAGED_CODE() (void)0
#endif // PAGED_CODE

#endif // _KERNEL_MODE

/// Use on classes or structs.  Class member functions & compiler-generated code
/// will default to the PAGE segment.  You can override any member function with `NONPAGED`.
#define KRTL_CLASS CODE_SEG(KRTL_PAGE_SEGMENT) __declspec(empty_bases)

/// Use on classes or structs.  Class member functions & compiler-generated code
/// will default to the NONPAGED segment.  You can override any member function with `PAGED`.
#define KRTL_CLASS_DPC_ALLOC __declspec(empty_bases)

enum CallRunMode
{
    // This call should complete synchronously on the current thread
    RunSynchronous,
    // This call should return immediately, and complete the operation in a background thread
    RunAsynchronous,
    // This call can return immediately, OR complete synchronously
    // (Use this if you're running on a workitem thread already, and you
    //  don't mind if the callee uses your thread to do its work, but you
    //  can tolerate the call completing asynchronously if the callee doesn't
    //  need your thread.)
    RunAsynchronousButOkayToBlock,
};

