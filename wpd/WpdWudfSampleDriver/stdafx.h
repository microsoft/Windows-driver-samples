// stdafx.h : include file for standard system include files,
// or project specific include files that are used frequently,
// but are changed infrequently

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

#include <initguid.h>
#include <propkeydef.h>

//
// Driver specific tracing #defines
// Declared here as some headers below use these macros
//
// TODO: Change these values to be appropriate for your driver.
//
#define MYDRIVER_TRACING_ID      L"Microsoft\\WPD\\WudfSampleDriver"

//
// TODO: Choose a different trace control GUID
//
#define WPP_CONTROL_GUIDS \
    WPP_DEFINE_CONTROL_GUID(WudfSampleDriverCtlGuid,(92f71133,1850,4757,899f,96f28bae2f0b), \
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

#include "WpdWudfSampleDriver.h"
#include "PortableDeviceTypes.h"
#include "PortableDeviceClassExtension.h"
#include "PortableDevice.h"
#include "ContextMap.h"
#include "helpers.h"
#include "FakeContent.h"
#include "FakeImageContent.h"
#include "FakeMusicContent.h"
#include "FakeVideoContent.h"
#include "FakeContactContent.h"
#include "FakeMemoContent.h"
#include "FakeFolderContent.h"
#include "RenderingInformationFakeContent.h"
#include "NetworkConfigFakeContent.h"
#include "DeviceObjectFakeContent.h"
#include "StorageObjectFakeContent.h"
#include "FakeDevice.h"
#include "WpdObjectEnum.h"
#include "WpdObjectManagement.h"
#include "WpdObjectProperties.h"
#include "WpdObjectPropertiesBulk.h"
#include "WpdObjectResources.h"
#include "WpdCapabilities.h"
#include "WpdStorage.h"
#include "WpdNetworkConfig.h"
#include "WpdBaseDriver.h"

extern HINSTANCE g_hInstance;

