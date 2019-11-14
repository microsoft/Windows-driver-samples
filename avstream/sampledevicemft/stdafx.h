//
//    Copyright (C) Microsoft.  All rights reserved.
//
#pragma once


#include <SDKDDKVer.h>
#include <windows.h>
#include <winnt.h>
#include <tchar.h>
#include <comdef.h>
#include <initguid.h>
#include <ks.h>
#include <ksmedia.h>
#include <Strsafe.h>
#include <wchar.h>
#include <stdio.h>
#include <assert.h>
#include <mfidl.h>
#include <wincodec.h>
#include <mfapi.h>
#include <mftransform.h>
#include <mfidl.h>
#include <mferror.h>
#include <mftransform.h>
#include <time.h>
#include <initguid.h>
#include <d3d9.h>
#include <dxva2api.h>
#include <d3d11.h>
#include <mfcaptureengine.h>
#include <algorithm>
#include <new>
#include <d3d11_4.h>
#include <vector>
#include <map>
#include <stdexcept>
using namespace std;
#include <Windows.Foundation.h>
#include <wrl\client.h>
using namespace ABI::Windows::Foundation;
using namespace Microsoft::WRL;

// @@@@ README Please check / uncheck various hash defines to enable/ disable features
// MF_DEVICEMFT_ASYNCPIN_NEEDED will show how to use Asynchronous queues
// MF_DEVICEMFT_DECODING_MEDIATYPE_NEEDED will show how to decode a compressed mediatype.. it supports H264 and MJPG
// MF_DEVICEMFT_SET_SPHERICAL_ATTRIBUTES will set the spherical attributes
// MF_DEVICEMFT_ENUM_HW_DECODERS will make the device MFT enumerate hardware decoders
// 
//#define MF_DEVICEMFT_ADD_GRAYSCALER_ 1  
#define MF_DEVICEMFT_ASYNCPIN_NEEDED           1
#define MF_DEVICEMFT_DECODING_MEDIATYPE_NEEDED 1
#define MF_DEVICEMFT_SET_SPHERICAL_ATTRIBUTES  1
//#define MF_DEVICEMFT_ENUM_HW_DECODERS        1


/*Some important notes regarding Device MFT
Device MFT is an asynchronous transform that is loaded in the device source
There are a few key operations that need a special mention
1) InitializeTransform: This is where the device MFT should be initialized
2) ProcessInput is where the samples are supplied to the Device MFT
3) METransformHaveOutput is how deviceMFT notifies the Device Transform Manager (DTM) in the OS to pick up samples in its queues
4) SetOutPutStreamState is where the state transition on a Device MFT will come in
*/
