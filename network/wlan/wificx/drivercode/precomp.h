// Copyright (C) Microsoft Corporation. All rights reserved.
#pragma once

#ifdef _KERNEL_MODE
    #include <ntddk.h>
#else
    #include <windows.h>
    #include <ndis/types.h> // For NDIS_STATUS
    #include <ndis/status.h> // For NDIS_STATUS codes
    #include <ntddndis.h>
#endif
// WDF Headers
#include <wdf.h>

// Network Device Headers
#include <netadaptercx.h>
#include <netiodef.h>

// WIFI Device Headers
#include <wificx.h>
#include "umkmfusion.h"
#include "dot11wificxintf.h"
#include "dot11wificxtypes.hpp"
#include "TLVGeneratorParser.hpp"
#include "SharedTypes.h"

// WPP Tracing Headers
#include "trace.h"

// Off-by-default datapath trace.  Define WIFICX_DATAPATH_DEBUG in the project
// preprocessor definitions to emit DbgPrintEx diagnostics.
#if defined(WIFICX_DATAPATH_DEBUG) && defined(_KERNEL_MODE)
#define WFC_TRACE(...) DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, __VA_ARGS__)
#else
#define WFC_TRACE(...) ((void)0)
#endif

// Minimal placement-new to match operator new(size_t, void*)
// TLV generator/parser memory interface has the ULONG_PTR version
inline void* operator new(size_t, void* p) noexcept { return p; }
inline void  operator delete(void*, void*) noexcept { /* no-op */ }
