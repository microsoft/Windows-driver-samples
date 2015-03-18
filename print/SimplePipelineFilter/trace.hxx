//+--------------------------------------------------------------------------
//
//  Copyright (c) Microsoft Corporation.  All rights reserved.
//
//  This source code is intended only as a supplement to Microsoft
//  Development Tools and/or on-line documentation.  See these other
//  materials for detailed information regarding Microsoft code samples.
//
//  THIS CODE AND INFORMATION ARE PROVIDED AS IS WITHOUT WARRANTY OF ANY
//  KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE IMPLIED
//  WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR PURPOSE.
//
//  Abstract:
//     WDK print filter sample.
//     Declares what is needed to Event Tracing.
//
//----------------------------------------------------------------------------

#ifndef __WDK_SAMPLE_TRACE_HXX__
#define __WDK_SAMPLE_TRACE_HXX__

//
// Generate your own guid. Feel free to add more flags, as needed.
//
#define WPP_CONTROL_GUIDS \
    WPP_DEFINE_CONTROL_GUID(WdkSample, (79ad6dce, adad, 47ae, b49e, 41bcdccf9ed8),   \
        WPP_DEFINE_BIT(WS_ERROR)                                                     \
        WPP_DEFINE_BIT(WS_WARNING)                                                   \
        WPP_DEFINE_BIT(WS_TRACE)                                                     \
        )

void
WppTraceDebugOut(
    _In_z_  const WCHAR *pszFmt,
    ...
    );

void
WppTraceDebugOut(
    _In_z_  const CHAR *pszFmt,
    ...
    );

//
// For checked builds, send output to debugger rather than to file
//
#ifdef DBG
#define WPP_DEBUG(x)  WppTraceDebugOut x
#endif

#endif
