/*++

Copyright (c) Microsoft Corporation.  All rights reserved.

    THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY
    KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
    IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR
    PURPOSE.

Module Name:

    Trace.H

Abstract:


 --*/


#ifndef _TRACE_H
#define _TRACE_H

//
// Debug support macros
// -----------------------------------------------------------------------------
//


#define WPP_CONTROL_GUIDS \
    WPP_DEFINE_CONTROL_GUID(NetVMiniGUID,(AF864470,D828,4526,BF2C,22D3A5C98C4C),  \
        WPP_DEFINE_BIT(MP_ERROR)              \
        WPP_DEFINE_BIT(MP_WARNING)            \
        WPP_DEFINE_BIT(MP_TRACE)              \
        WPP_DEFINE_BIT(MP_INFO)               \
        WPP_DEFINE_BIT(MP_LOUD)               \
        )

#endif
