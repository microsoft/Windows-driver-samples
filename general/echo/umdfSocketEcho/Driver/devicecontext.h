/*++

Copyright (c) Microsoft Corporation. All rights reserved.

Module Name:

    devicecontext.h

Abstract:

    This header file defines the structure type for device context associated with the device object 

Environment:

    user mode only

Revision History:

--*/


#pragma once


typedef struct _DeviceContext
{
    PWSTR  hostStr; 

    PWSTR  portStr;

}DeviceContext;

