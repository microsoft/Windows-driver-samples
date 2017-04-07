//**@@@*@@@****************************************************
//
// Copyright (C) Microsoft Corporation. All rights reserved.
//
//**@@@*@@@****************************************************

#pragma once
#include <initguid.h>
#include <windows.h>
#include <intsafe.h>
#include <ks.h>
#include <mfapi.h>
#include <comdef.h>
#include <Strsafe.h>
#include <mfidl.h>
#include <mferror.h>
#include <Traceloggingprovider.h>
#include <mfcaptureengine.h>
#include <Windows.Foundation.h>
#include <exception>

#include <vector>
    using namespace std;
#include <wrl\client.h>
    using namespace ABI::Windows::Foundation;
    using namespace Microsoft::WRL;


#if !defined(_IKsControl_)
#define _IKsControl_
interface DECLSPEC_UUID("28F54685-06FD-11D2-B27A-00A0C9223196") IKsControl;
#undef INTERFACE
#define INTERFACE IKsControl
DECLARE_INTERFACE_(IKsControl, IUnknown)
{
    STDMETHOD(KsProperty)(
        THIS_
        IN PKSPROPERTY Property,
        IN ULONG PropertyLength,
        IN OUT LPVOID PropertyData,
        IN ULONG DataLength,
        OUT ULONG* BytesReturned
        ) PURE;
    STDMETHOD(KsMethod)(
        THIS_
        IN PKSMETHOD Method,
        IN ULONG MethodLength,
        IN OUT LPVOID MethodData,
        IN ULONG DataLength,
        OUT ULONG* BytesReturned
        ) PURE;
    STDMETHOD(KsEvent)(
        THIS_
        IN PKSEVENT Event OPTIONAL,
        IN ULONG EventLength,
        IN OUT LPVOID EventData,
        IN ULONG DataLength,
        OUT ULONG* BytesReturned
        ) PURE;
};
#endif  // _IKsControl_

#include "SensorTransformSampleMFTPin.h"
#include "SensorTransformSampleMFT.h"

