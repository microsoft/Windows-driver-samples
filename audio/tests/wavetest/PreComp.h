// ------------------------------------------------------------------------------
//
// Copyright (C) Microsoft. All rights reserved.
//
// File Name:
//
//  PreComp.h
//
// Abstract:
//
//  Precompiled common headers
//
// -------------------------------------------------------------------------------

#include <basiclog.h>
#include <BasicLogHelper.h>
#include <mmsystem.h>
#include <atlbase.h>
#include <atlcom.h>
#include <initguid.h>

#include <AudioEngineEndpoint.h>
#include <AudioEngineEndpointP.h>
#include <Audioclient.h>
#include <mmdeviceapi.h>
#include <mmdeviceapip.h>
#include <devicetopologyp.h> 
#include <propvarutil.h>

#include <wil\com.h>

#include <KsLib.h>
#include <Waveutil.h>
#include <aecommon.h>
#include <ilog.h>
#include <shlflags.h>
#include <basicprintf.h>
#include <wextestclass.h>
#include <resourcelist.h>

// ------------------------------------------------------------------------------
// Log
#define LOG(log, ...) BasicLogPrintf( log, XMSG, 1, __VA_ARGS__ )
#define SKIP(log, ...) BasicLogPrintf( log, XSKIP, 1, __VA_ARGS__ )
#define WARN(log, ...) BasicLogPrintf( log, XWARN, 1, __VA_ARGS__ )
#define BLOCK(log, ...) BasicLogPrintf( log, XBLOCK, 1, __VA_ARGS__ )
#define ERR(log, ...) BasicLogPrintf( log, XFAIL, 1, __VA_ARGS__ )
