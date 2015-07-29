/*++

    THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
    ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
    THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
    PARTICULAR PURPOSE.

    Copyright (c) Microsoft Corporation. All rights reserved

Module Name:

    Internal.h

Abstract:

    This module contains necessary include directives, WPP tracing macros, 
    and string definitions for the Biometric driver sample.

Environment:

    Windows User-Mode Driver Framework (WUDF)

--*/

#pragma once

//
// ATL support
//
#include "atlbase.h"
#include "atlcom.h"

//
// Include the WUDF Headers
//

#include "wudfddi.h"

//
// Use specstrings for in/out annotation of function parameters.
//

#include "specstrings.h"

//
// Get limits on common data types (ULONG_MAX for example)
//

#include "limits.h"

//
// We need usb I/O targets to talk to the USB device.
//

#include "wudfusb.h"

//
// WinUsb structures.
//

#include "usb_hw.h"

//
// Public definitions.
//
#include "public.h"

//
// GUID include
//
#include <initguid.h>

//
// Windows IOCTL definitions.
//
#include "winioctl.h"

//
// WinBio includes
//
#include "winbio_types.h"
#include "winbio_err.h"
#include "winbio_ioctl.h"

//
// RAII helper class for requests
//
#include "RequestHelper.h"

//
// Define the tracing flags.
//
// Tracing GUID defined in BioUsbSample.ctl - 864936A6-DB79-451e-B764-E720D61A9361
//
// TODO: Generate a new tracing GUID for your driver, and replace all
//       instances of the GUID above with your new GUID. 
//

#define WPP_CONTROL_GUIDS                                                   \
    WPP_DEFINE_CONTROL_GUID(                                                \
        WudfBioUsbSampleTraceGuid, (864936A6,DB79,451e,B764,E720D61A9361),     \
                                                                            \
        WPP_DEFINE_BIT(MYDRIVER_ALL_INFO)                                   \
        WPP_DEFINE_BIT(BIOMETRIC_TRACE_DRIVER)                                   \
        WPP_DEFINE_BIT(BIOMETRIC_TRACE_DEVICE)                                   \
        WPP_DEFINE_BIT(BIOMETRIC_TRACE_QUEUE)                                    \
        )                             

#define WPP_FLAG_LEVEL_LOGGER(flag, level)                                  \
    WPP_LEVEL_LOGGER(flag)

#define WPP_FLAG_LEVEL_ENABLED(flag, level)                                 \
    (WPP_LEVEL_ENABLED(flag) &&                                             \
     WPP_CONTROL(WPP_BIT_ ## flag).Level >= level)

#define WPP_LEVEL_FLAGS_LOGGER(lvl,flags) \
           WPP_LEVEL_LOGGER(flags)
               
#define WPP_LEVEL_FLAGS_ENABLED(lvl, flags) \
           (WPP_LEVEL_ENABLED(flags) && WPP_CONTROL(WPP_BIT_ ## flags).Level >= lvl)

//
// This comment block is scanned by the trace preprocessor to define our
// Trace function.
//
// begin_wpp config
// FUNC Trace{FLAG=MYDRIVER_ALL_INFO}(LEVEL, MSG, ...);
// FUNC TraceEvents(LEVEL, FLAGS, MSG, ...);
// end_wpp
//

//
// Forward definition of queue.
//
typedef class CBiometricIoQueue *PCBiometricIoQueue;

//
// Include the type specific headers.
//
#include "Driver.h"
#include "Device.h"
#include "IoQueue.h"

//
// Driver specific #defines
// TODO: Put strings specific to your device here.
//

#define MYDRIVER_TRACING_ID      L"Microsoft\\UMDF\\Biometric USB Sample V1.0"

#define SAMPLE_MANUFACTURER_NAME L"Biometric Sample Manufacturer"
#define SAMPLE_MODEL_NAME L"Biometric Sample Model"
#define SAMPLE_SERIAL_NUMBER L"000-000-000"


template <typename T>
inline void BiometricSafeRelease(T *&t)
{
      if (t)
      {
          t->Release();
      }
      t = NULL;
}
