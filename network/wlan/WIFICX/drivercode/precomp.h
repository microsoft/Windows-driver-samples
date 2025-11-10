// Copyright (C) Microsoft Corporation. All rights reserved.
#pragma once

//#include <initguid.h>

#ifdef _KERNEL_MODE
    #include <ntddk.h>
#else
    #include <windows.h>
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

//#include "device.h"
//#include "adapter.h"

//#include "wifirequest.h"
