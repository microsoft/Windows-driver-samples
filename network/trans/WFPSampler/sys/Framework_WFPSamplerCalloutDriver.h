////////////////////////////////////////////////////////////////////////////////////////////////////
//
//   Copyright (c) 2014 Microsoft Corporation.  All Rights Reserved.
//
//   Module Name:
//      Framework_WFPSamplerCalloutDriver.h
//
//   Abstract:
//
//   Author:
//      Dusty Harper      (DHarper)
//
//   Revision History:
//
//      [ Month ][Day] [Year] - [Revision]-[ Comments ]
//      May       01,   2010  -     1.0   -  Creation
//      December  13,   2013  -     1.1   -  Add support for multiple injectors and redirectors, and 
//                                              add structure for serializing asynchronous 
//                                              FWPM_LAYER_STREAM injections
//
////////////////////////////////////////////////////////////////////////////////////////////////////

#ifndef FRAMEWORK_WFP_SAMPLER_CALLOUT_DRIVER_H
#define FRAMEWORK_WFP_SAMPLER_CALLOUT_DRIVER_H

extern "C"
{
#pragma warning(push)
#pragma warning(disable: 4201) /// NAMELESS_STRUCT_UNION
#pragma warning(disable: 4324) /// STRUCTURE_PADDED

   #include <ntifs.h>                    /// IfsKit\Inc
   #include <ntddk.h>                    /// Inc
   #include <wdf.h>                      /// Inc\WDF\KMDF\1.9
   #include <ndis.h>                     /// Inc
   #include <fwpmk.h>                    /// Inc
   #include <fwpsk.h>                    /// Inc
   #include <netioddk.h>                 /// Inc
   #include <ntintsafe.h>                /// Inc
   #include <ntstrsafe.h>                /// Inc
   #include <stdlib.h>                   /// SDK\Inc\CRT

#pragma warning(pop)
}

#include "Identifiers.h"                     /// ..\Inc
#include "ProviderContexts.h"                /// ..\Inc
#include "HelperFunctions_Include.h"         /// ..\SysLib
#include "HelperFunctions_ExposedCallouts.h" /// .
#include "ClassifyFunctions_Include.h"       /// .
#include "CompletionFunctions_Include.h"     /// .
#include "NotifyFunctions_Include.h"         /// .
#include "SubscriptionFunctions_Include.h"   /// .

#define WFPSAMPLER_CALLOUT_DRIVER_TAG (UINT32)'DCSW'

#define WPP_COMPID_LEVEL_LOGGER(COMPID,LEVEL)  (WPP_CONTROL(WPP_BIT_Error).Logger),
#define WPP_COMPID_LEVEL_ENABLED(COMPID,LEVEL) (WPP_CONTROL(WPP_BIT_Error).Level >= LEVEL)

typedef struct WFPSAMPLER_DEVICE_DATA_
{
   HANDLE*                  pEngineHandle;
   INJECTION_HANDLE_DATA*   ppIPv4InboundInjectionHandles[2];
   INJECTION_HANDLE_DATA*   ppIPv4OutboundInjectionHandles[2];
   INJECTION_HANDLE_DATA*   ppIPv6InboundInjectionHandles[2];
   INJECTION_HANDLE_DATA*   ppIPv6OutboundInjectionHandles[2];
   INJECTION_HANDLE_DATA*   ppInboundInjectionHandles[2];
   INJECTION_HANDLE_DATA*   ppOutboundInjectionHandles[2];
   HANDLE*                  ppRedirectionHandles[2];
}WFPSAMPLER_DEVICE_DATA, *PWFPSAMPLER_DEVICE_DATA;

typedef struct DEVICE_EXTENSION_
{
   VOID*            pRegistration;
   PCALLBACK_OBJECT pCallbackObject;
}DEVICE_EXTENSION, *PDEVICE_EXTENSION;

extern PDEVICE_OBJECT g_pWDMDevice;

extern WDFDRIVER g_WDFDriver;
extern WDFDEVICE g_WDFDevice;

extern HANDLE g_bfeSubscriptionHandle;

extern WFPSAMPLER_DEVICE_DATA g_WFPSamplerDeviceData;

extern DEVICE_EXTENSION g_deviceExtension;

extern BOOLEAN g_calloutsRegistered;

extern SERIALIZATION_LIST g_bsiSerializationList;

extern KSPIN_LOCK g_bpeSpinLock;

#endif /// FRAMEWORK_WFP_SAMPLER_CALLOUT_DRIVER_H
