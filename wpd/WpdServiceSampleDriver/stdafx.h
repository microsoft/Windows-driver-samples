#pragma once

#include "resource.h"
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

#define STRSAFE_NO_DEPRECATE

#include <stdio.h>
#include <tchar.h>

#include <atlbase.h>
#include <atlcom.h>
#include <atlcoll.h>
#include <atlstr.h>

// This driver is entirely user-mode
_Analysis_mode_(_Analysis_code_type_user_code_);

// Driver specific tracing #defines
//
// TODO: Change these values to be appropriate for your driver.
//
#define MYDRIVER_TRACING_ID      L"Microsoft\\WPD\\ServiceSampleDriver"

//
// TODO: Choose a different trace control GUID
//
#define WPP_CONTROL_GUIDS \
    WPP_DEFINE_CONTROL_GUID(ServiceSampleDriverCtlGuid,(f0cc34b3,a482,4dc0,b978,b5cf42aec4fd), \
        WPP_DEFINE_BIT(TRACE_FLAG_ALL)                                      \
        WPP_DEFINE_BIT(TRACE_FLAG_DEVICE)                                   \
        WPP_DEFINE_BIT(TRACE_FLAG_DRIVER)                                   \
        WPP_DEFINE_BIT(TRACE_FLAG_QUEUE)                                    \
        )

#define WPP_LEVEL_FLAGS_LOGGER(lvl,flags) \
           WPP_LEVEL_LOGGER(flags)

#define WPP_LEVEL_FLAGS_ENABLED(lvl, flags) \
           (WPP_LEVEL_ENABLED(flags) && WPP_CONTROL(WPP_BIT_ ## flags).Level >= lvl)

//
// This comment block is scanned by the trace preprocessor to define our
// TraceEvents function.
//
// begin_wpp config
// FUNC Trace{FLAG=TRACE_FLAG_ALL}(LEVEL, MSG, ...);
// FUNC TraceEvents(LEVEL, FLAGS, MSG, ...);
// end_wpp

//
// This comment block is scanned by the trace preprocessor to define our
// CHECK_HR function.
//
//
// begin_wpp config
// USEPREFIX (CHECK_HR,"%!STDPREFIX!");
// FUNC CHECK_HR{FLAG=TRACE_FLAG_ALL}(hrCheck, MSG, ...);
// USESUFFIX (CHECK_HR, " hr= %!HRESULT!", hrCheck);
// end_wpp

//
// PRE macro: The name of the macro includes the condition arguments FLAGS and EXP
//            define in FUNC above
//
#define WPP_FLAG_hrCheck_PRE(FLAGS, hrCheck) {if(hrCheck != S_OK) {

//
// POST macro
// The name of the macro includes the condition arguments FLAGS and EXP
//            define in FUNC above
#define WPP_FLAG_hrCheck_POST(FLAGS, hrCheck) ; } }

//
// The two macros below are for checking if the event should be logged and for
// choosing the logger handle to use when calling the ETW trace API
//
#define WPP_FLAG_hrCheck_ENABLED(FLAGS, hrCheck) WPP_FLAG_ENABLED(FLAGS)
#define WPP_FLAG_hrCheck_LOGGER(FLAGS, hrCheck) WPP_FLAG_LOGGER(FLAGS)

#include <PortableDeviceTypes.h>
#include <PortableDeviceClassExtension.h>
#include <PortableDevice.h>

// Service GUID definitions
#include <initguid.h>
#include <propkeydef.h>
#define DEFINE_DEVSVCGUID DEFINE_GUID
#define DEFINE_DEVSVCPROPKEY DEFINE_PROPERTYKEY
#include <DeviceServices.h>
#include <FullEnumSyncDeviceService.h>
#include <ContactDeviceService.h>

// Forward class declarations
class WpdObjectResourceContext;
class WpdObjectEnumeratorContext;
class WpdServiceMethods;

#include "helpers.h"
#include "FakeContent.h"
#include "FakeContactContent.h"
#include "FakeContactsServiceContent.h"
#include "FakeContactsService.h"
#include "FakeStorage.h"
#include "FakeDeviceContent.h"
#include "FakeDevice.h"

#include "WpdServiceSampleDriver.h"
#include "WpdObjectEnum.h"
#include "WpdObjectManagement.h"
#include "WpdObjectProperties.h"
#include "WpdObjectPropertiesBulk.h"
#include "WpdObjectResources.h"
#include "WpdCapabilities.h"
#include "WpdServiceCapabilities.h"
#include "WpdServiceMethods.h"
#include "WpdService.h"
#include "WpdBaseDriver.h"

extern HINSTANCE g_hInstance;

